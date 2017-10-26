ifndef QCONFIG
QCONFIG=qconfig.mk
endif
include $(QCONFIG)

include $(MKFILES_ROOT)/qmacros.mk
-include $(PROJECT_ROOT)/roots.mk

define PINFO
PINFO DESCRIPTION=Zync clock Driver
endef

NAME := clock-$(PRODUCT)
USEFILE = $(PROJECT_ROOT)/Usemsg
INSTALLDIR = sbin
LIBS =  drvrS


#####AUTO-GENERATED by packaging script... do not checkin#####
   INSTALL_ROOT_nto = $(PROJECT_ROOT)/../../../../../install
   USE_INSTALL_ROOT=1
##############################################################

include $(MKFILES_ROOT)/qtargets.mk

