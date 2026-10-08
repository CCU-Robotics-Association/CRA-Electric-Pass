#pragma once

#include <stdbool.h>
#include <stdint.h>

/*
 * Hand-written Cat page layered into an EEZ-owned screen. The generated
 * object identifiers retain their legacy DisplayImg names for compatibility.
 * The generated screen only supplies the page/header and content container.
 */
void cat_pet_init(void);
void cat_pet_key_event(uint32_t key);
bool cat_pet_is_active(void);
