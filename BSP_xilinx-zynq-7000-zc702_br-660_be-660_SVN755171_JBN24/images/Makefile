ROOT_DIR := $(notdir $(CURDIR))
ifndef QCONFIG
QCONFIG=qconfig.mk
endif
include $(QCONFIG)

HOST_MKIFS := mkifs

SUFFIXES := .build .ifs .raw

.PHONY: all clean

all: ifs-xzynq-zc702.raw

clean: 
	$(RM_HOST) ifs-xzynq-zc702.raw 

ifs-xzynq-zc702.raw: zc702.build
	$(HOST_MKIFS) -vvv -r../install $(MKIFSFLAGS) $^ $@
