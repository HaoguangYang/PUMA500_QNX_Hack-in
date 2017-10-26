ifndef QCONFIG
QCONFIG=qconfig.mk
endif
include $(QCONFIG)

define PINFO
PINFO DESCRIPTION=DMA engine driver
endef

ifndef USEFILE
USEFILE=$(PROJECT_ROOT)/dma.use
endif
EXTRA_SRCVPATH += $(EXTRA_SRCVPATH_$(SECTION))
PUBLIC_INCVPATH += $(PROJECT_ROOT)/public

LIBS += drvrS

PRE_SRCVPATH += $(foreach var,$(filter a, $(VARIANTS)),$(CPU_ROOT)/$(subst $(space),.,$(patsubst a,dll,$(filter-out g, $(VARIANTS)))))

EXTRA_SILENT_VARIANTS = $(subst -, ,$(SECTION))
NAME= dma-$(PRODUCT)
include $(MKFILES_ROOT)/qmacros.mk
-include $(PROJECT_ROOT)/roots.mk
-include $(PROJECT_ROOT)/$(SECTION)/pinfo.mk

#####AUTO-GENERATED by packaging script... do not checkin#####
   INSTALL_ROOT_nto = $(PROJECT_ROOT)/../../../../../install
   USE_INSTALL_ROOT=1
##############################################################

include $(MKFILES_ROOT)/qtargets.mk

