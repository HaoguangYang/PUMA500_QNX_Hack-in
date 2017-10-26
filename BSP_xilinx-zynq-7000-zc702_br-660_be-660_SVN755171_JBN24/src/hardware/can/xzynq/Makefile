LIST=CPU

ifdef CONFIG_PMM
CCFLAGS+= -DCONFIG_PMM
LIBS=gpio-xzynq pmm-xzynq
else
LIBS=gpio-xzynq
endif

include recurse.mk
