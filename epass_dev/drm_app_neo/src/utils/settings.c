#include "utils/settings.h"
#include "utils/log.h"
#include "config.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint64_t magic;
    uint32_t version;
    int brightness;
    sw_interval_t switch_interval;
    sw_mode_t switch_mode;
    usb_mode_t usb_mode;
    settings_ctrl_word_t ctrl_word;
} settings_v2_disk_t;

typedef struct {
    uint64_t magic;
    uint32_t version;
    int brightness;
    sw_interval_t switch_interval;
    sw_mode_t switch_mode;
    usb_mode_t usb_mode;
    sleep_timeout_t sleep_timeout;
    settings_ctrl_word_t ctrl_word;
} settings_v3_disk_t;

void log_settings(settings_t *settings){
    log_info("==> Settings Log <==");
    log_info("magic: %08lx", settings->magic);
    log_info("version: %d", settings->version);
    log_info("brightness: %d", settings->brightness);
    log_info("switch_interval: %d", settings->switch_interval);
    log_info("switch_mode: %d", settings->switch_mode);
    log_info("usb_mode: %d", settings->usb_mode);
    log_info("sleep_timeout: %d", settings->sleep_timeout);
    log_info("theme_color: %d", settings->theme_color);
    log_info("ctrl.lowbat: %d", settings->ctrl_word.lowbat_trip);
    log_info("ctrl.no_intro: %d", settings->ctrl_word.no_intro_block);
    log_info("ctrl.no_overlay: %d", settings->ctrl_word.no_overlay_block);
}

void settings_apply_brightness(int brightness){
    FILE *f = fopen(SETTINGS_BRIGHTNESS_PATH, "w");
    if (f) {
        fprintf(f, "%d\n", brightness);
        fclose(f);
    } else {
        log_error("Failed to set brightness");
    }
}

void settings_set_usb_mode(usb_mode_t usb_mode){
    switch(usb_mode){
        case usb_mode_t_MTP:
            log_info("setting usb mode to MTP");
            system("usbctl mtp &");
            break;
        case usb_mode_t_SERIAL:
            log_info("setting usb mode to SERIAL");
            system("usbctl serial &");
            break;
        case usb_mode_t_RNDIS:
            log_info("setting usb mode to RNDIS");
            system("usbctl rndis &");
            break;
        case usb_mode_t_EPMANAGER:
            log_info("setting usb mode to EPMANAGER");
            system("usbctl epass &");
            break;
        default:
            log_info("setting usb mode to NONE");
            system("usbctl none &");
            break;
    }

}

static void settings_save(settings_t *settings){
    FILE *f = fopen(SETTINGS_FILE_PATH, "wb");
    if (!f) {
        log_error("Failed to open settings file for writing");
        return;
    }
    settings->magic = SETTINGS_MAGIC;
    settings->version = SETTINGS_VERSION;
    if (fwrite(settings, SETTINGS_LENGTH, 1, f) != 1) {
        log_error("Failed to write settings");
    }
    fclose(f);

    log_info("setting saved!");
    // log_settings(settings);
}

void settings_init(settings_t *settings){
    bool loaded = false;
    bool migrated = false;
    FILE *f = fopen(SETTINGS_FILE_PATH, "rb");
    if(f == NULL){
        log_error("failed to open settings file");
    }
    else{
        uint64_t magic = 0;
        uint32_t version = 0;
        if(fread(&magic, sizeof(magic), 1, f) != 1 ||
           fread(&version, sizeof(version), 1, f) != 1) {
            log_error("failed to read settings header");
        }
        else if(magic != SETTINGS_MAGIC){
            log_error("invalid settings file");
        }
        else if(version == SETTINGS_VERSION) {
            rewind(f);
            if(fread(settings, SETTINGS_LENGTH, 1, f) == 1) {
                loaded = true;
            }
            else {
                log_error("failed to read complete settings file");
            }
        }
        else if(version == 2) {
            settings_v2_disk_t old_settings;
            rewind(f);
            if(fread(&old_settings, sizeof(old_settings), 1, f) == 1) {
                memset(settings, 0, sizeof(*settings));
                settings->magic = old_settings.magic;
                settings->version = SETTINGS_VERSION;
                settings->brightness = old_settings.brightness;
                settings->switch_interval = old_settings.switch_interval;
                settings->switch_mode = old_settings.switch_mode;
                settings->usb_mode = old_settings.usb_mode;
                settings->sleep_timeout = sleep_timeout_t_SLEEP_TIMEOUT_3MIN;
                settings->theme_color = theme_color_t_THEME_WHITE;
                settings->ctrl_word = old_settings.ctrl_word;
                loaded = true;
                migrated = true;
                log_info("migrated settings from version 2 to version %d",
                         SETTINGS_VERSION);
            }
            else {
                log_error("failed to read version 2 settings file");
            }
        }
        else if(version == 3) {
            settings_v3_disk_t old_settings;
            rewind(f);
            if(fread(&old_settings, sizeof(old_settings), 1, f) == 1) {
                memset(settings, 0, sizeof(*settings));
                settings->magic = old_settings.magic;
                settings->version = SETTINGS_VERSION;
                settings->brightness = old_settings.brightness;
                settings->switch_interval = old_settings.switch_interval;
                settings->switch_mode = old_settings.switch_mode;
                settings->usb_mode = old_settings.usb_mode;
                settings->sleep_timeout = old_settings.sleep_timeout;
                settings->theme_color = theme_color_t_THEME_WHITE;
                settings->ctrl_word = old_settings.ctrl_word;
                loaded = true;
                migrated = true;
                log_info("migrated settings from version 3 to version %d",
                         SETTINGS_VERSION);
            }
            else {
                log_error("failed to read version 3 settings file");
            }
        }
        else {
            log_error("invalid settings file version");
        }
        fclose(f);
    }

    if(loaded) {
        pthread_mutex_init(&settings->mtx, NULL);
        settings_apply_brightness(settings->brightness);
        settings_set_usb_mode(settings->usb_mode);
        if(migrated) {
            settings_save(settings);
        }
        return;
    }

    log_info("creating new settings file");
    settings->magic = SETTINGS_MAGIC;
    settings->version = SETTINGS_VERSION;
    settings->brightness = 5;
    settings->switch_interval = sw_interval_t_SW_INTERVAL_5MIN;
    settings->switch_mode = sw_mode_t_SW_MODE_SEQUENCE;
    settings->usb_mode = usb_mode_t_EPMANAGER;
    settings->sleep_timeout = sleep_timeout_t_SLEEP_TIMEOUT_3MIN;
    settings->theme_color = theme_color_t_THEME_WHITE;
    settings->ctrl_word.lowbat_trip = 1;
    settings->ctrl_word.no_intro_block = 0;
    settings->ctrl_word.no_overlay_block = 0;
    settings_set_usb_mode(settings->usb_mode);
    pthread_mutex_init(&settings->mtx, NULL);
    settings_save(settings);
    return;
    
}

void settings_destroy(settings_t *settings){
    pthread_mutex_destroy(&settings->mtx);
}

void settings_lock(settings_t *settings){
    pthread_mutex_lock(&settings->mtx);
}
void settings_unlock(settings_t *settings){
    pthread_mutex_unlock(&settings->mtx);
}

void settings_update(settings_t *settings){
    settings_save(settings);
    settings_apply_brightness(settings->brightness);
}


