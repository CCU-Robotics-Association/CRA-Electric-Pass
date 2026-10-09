################################################################################
#
# cra_device_config
#
################################################################################

CRA_DEVICE_CONFIG_VERSION = 1.0.0
CRA_DEVICE_CONFIG_SITE = $(TOPDIR)/../components
CRA_DEVICE_CONFIG_SITE_METHOD = local
CRA_DEVICE_CONFIG_SUBDIR = device-config
CRA_DEVICE_CONFIG_LICENSE = GPL-3.0-or-later
CRA_DEVICE_CONFIG_CONF_OPTS = -DBUILD_TESTING=OFF

$(eval $(cmake-package))
