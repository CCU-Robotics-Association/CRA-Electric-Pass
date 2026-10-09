################################################################################
#
# epass_usb_responder
#
################################################################################


EPASS_USB_RESPONDER_VERSION = 1.0.0
EPASS_USB_RESPONDER_SITE = $(TOPDIR)/../components
EPASS_USB_RESPONDER_SITE_METHOD = local
EPASS_USB_RESPONDER_SUBDIR = usb-responder
EPASS_USB_RESPONDER_LICENSE = GPL-3.0-or-later
EPASS_USB_RESPONDER_CONF_OPTS = -DBUILD_TESTING=OFF

$(eval $(cmake-package))
