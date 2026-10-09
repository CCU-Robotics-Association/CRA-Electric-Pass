################################################################################
#
# epassctl
#
################################################################################


EPASSCTL_VERSION = 1.0.0
EPASSCTL_SITE = $(TOPDIR)/../components
EPASSCTL_SITE_METHOD = local
EPASSCTL_SUBDIR = epassctl
EPASSCTL_LICENSE = GPL-3.0-or-later
EPASSCTL_CONF_OPTS = -DBUILD_TESTING=OFF

$(eval $(cmake-package))
