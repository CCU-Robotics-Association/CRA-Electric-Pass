################################################################################
#
# epass_drm_app
#
################################################################################


EPASS_DRM_APP_VERSION = 2e28cdf3f26afb844e3d060a5d9965c9ceba1a44
EPASS_DRM_APP_SITE = https://github.com/CCU-Robotics-Association/CRA-Electric-Pass.git
EPASS_DRM_APP_SITE_METHOD = git
EPASS_DRM_APP_SUBDIR = epass_dev/drm_app_neo
EPASS_DRM_APP_DEPENDENCIES = libcedarx libcedarc libdrm libpng libevdev jpeg-turbo
EPASS_DRM_APP_GIT_SUBMODULES = YES
EPASS_DRM_APP_CONF_OPTS = -DBUILD_SHARED_LIBS=OFF --fresh

define EPASS_DRM_APP_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(EPASS_DRM_APP_BUILDDIR)/epass_drm_app $(TARGET_DIR)/root/
endef


$(eval $(cmake-package))
