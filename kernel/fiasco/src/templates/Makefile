# sample configurations

SAMPLE_CONFIGS := $(subst globalconfig.out.,,$(wildcard globalconfig.out.*))

srcdir = $(shell pwd)/../

all:

runconfig = export INCLUDE_PPC32=y;                      \
	    export INCLUDE_SPARC=y;                      \
	    cp globalconfig.out.$(2) globalconfig.out ;  \
            make $(1) && sed -e '3,4d'                   \
		globalconfig.out > globalconfig.out.$(2)

cleanup = make mrproper && \
	  $(RM) -r globalconfig.out.old globalconfig.h Kconfig include

testconfigs:
	$(foreach cfg,$(SAMPLE_CONFIGS), $(call runconfig,menuconfig,$(cfg)))
	$(call cleanup)

testconfig:
	if [ "$$T" != "" ]; then $(call runconfig,menuconfig,$$T); \
	else echo "use: make T=configname testconfig"; fi
	$(call cleanup)

refresh:
	for cfg in $(SAMPLE_CONFIGS); do                       \
	  echo "********************************************"; \
	  echo "** $$cfg **";                                  \
	  echo "********************************************"; \
	  $(call runconfig,oldconfig,$$cfg);                   \
	done
	$(call cleanup)

%:
	$(MAKE) -f $(srcdir)/Makefile \
	  srcdir=$(srcdir) objbase=$(shell pwd) "$@"


.PHONY: refresh testconfig

