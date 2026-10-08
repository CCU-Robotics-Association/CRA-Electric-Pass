################################################################################
#
# srgn_config
#
################################################################################


SRGN_CONFIG_VERSION = V0.1
SRGN_CONFIG_SITE = https://github.com/rhodesepass/srgn_config.git
SRGN_CONFIG_SITE_METHOD = git

define SRGN_CONFIG_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/srgn_config $(TARGET_DIR)/usr/bin
endef

$(eval $(cmake-package))