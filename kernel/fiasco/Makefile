#
#
# GLOBAL Makefile for FIASCO

# enviroment
DFLBUILDDIR	:= build
ALLBUILDDIR	:= build-all
RANDBUILDDIR	:= build-rand
TEMPLDIR	:= src/templates
MAKEFILETEMPL	:= $(TEMPLDIR)/Makefile.builddir.templ
MANSUBDIRS	:= man
INSTALLSUBDIRS	:= $(MANSUBDIRS)
CLEANSUBDIRS	:= $(MANSUBDIRS) $(wildcard $(DFLBUILDDIR))
CONFIG_FILE	:= $(TEMPLDIR)/globalconfig.out
TEST_TEMPLATES	:= $(patsubst $(CONFIG_FILE).%,%,$(wildcard $(CONFIG_FILE).*))
TEST_TEMPLATES  := $(if $(TEMPLATE_FILTER),$(filter $(TEMPLATE_FILTER),$(TEST_TEMPLATES)),$(TEST_TEMPLATES))
DFL_TEMPLATE	:= ia32-1
PL		?= 1

getdir 		= $(shell						\
		    bd="$(1)";						\
		    if [ "$${bd\#/}" = "$$bd" -a			\
		         "$${bd\#*..}" = "$$bd" ]; then			\
		      relp="..";					\
		      while [ "$${bd\#*/}" != "$$bd" ]; do		\
		        relp="$$relp/..";				\
			bd="$${bd\#*/}";				\
		      done;						\
		      echo "$$relp";					\
		    else						\
		      pwd;						\
		    fi							\
		  )

buildmakefile = mkdir -p "$(1)";                                         \
		perl -p -i -e '$$s = "$(CURDIR)/src"; s/\@SRCDIR\@/$$s/' \
			< $(MAKEFILETEMPL) > $(1)/Makefile

ifneq ($(strip $(B)),)
BUILDDIR := $(B)
endif
ifneq ($(strip $(BUILDDIR)),)
builddir:
	@echo "Creating build directory \"$(BUILDDIR)\"..."
	@if [ -e "$(BUILDDIR)" ]; then			\
		echo "Already exists, aborting.";	\
		exit 1;					\
	fi
	@$(call buildmakefile,$(BUILDDIR))
	@echo "done."
endif

ifneq ($(strip $(T)),)
this:
	set -e;							      \
	test -f $(TEMPLDIR)/globalconfig.out.$(T);		      \
	bdir=T-$(DFLBUILDDIR)-$(T);				      \
	rm -rf $$bdir;						      \
	$(call buildmakefile,T-$(DFLBUILDDIR)-$(T));		      \
	cp $(TEMPLDIR)/globalconfig.out.$(T) $$bdir/globalconfig.out; \
	$(MAKE) -C $$bdir
endif

$(DFLBUILDDIR): fiasco.builddir.create
	$(MAKE) -C $@ -j$(PL)

all:	fiasco man

clean cleanall:
	set -e; for i in $(CLEANSUBDIRS); do $(MAKE) -C $$i $@; done

purge: cleanall
	$(RM) -r $(DFLBUILDDIR)

man:
	set -e; for i in $(MANSUBDIRS); do $(MAKE) -C $$i; done

fiasco.builddir.create:
	[ -e $(DFLBUILDDIR)/Makefile ] ||			 \
		($(call buildmakefile,$(DFLBUILDDIR)))
	[ -f $(DFLBUILDDIR)/globalconfig.out ] || {		 \
		cp  $(TEMPLDIR)/globalconfig.out.$(DFL_TEMPLATE) \
			$(DFLBUILDDIR)/globalconfig.out;	 \
	}

config $(filter config %config,$(MAKECMDGOALS)): fiasco.builddir.create
	$(MAKE) -C $(DFLBUILDDIR) $@

fiasco: fiasco.builddir.create
	$(MAKE) -C $(DFLBUILDDIR) -j$(PL)

checkallseq:
	@error=0;						      \
	$(RM) -r $(ALLBUILDDIR);				      \
	for X in $(TEST_TEMPLATES); do				      \
		echo -e "\n= Building configuration: $$X\n\n";	      \
		$(call buildmakefile,$(ALLBUILDDIR)/$$X);	      \
		cp $(TEMPLDIR)/globalconfig.out.$$X		      \
		   $(ALLBUILDDIR)/$$X/globalconfig.out;		      \
		if $(MAKE) -C $(ALLBUILDDIR)/$$X -j$(PL); then	      \
			[ -z "$(KEEP_BUILD_DIRS)" ] &&		      \
			   $(RM) -r $(ALLBUILDDIR)/$$X;		      \
		else						      \
			error=$$?;				      \
			failed="$$failed $$X";			      \
		fi						      \
	done;							      \
	rmdir $(ALLBUILDDIR) >/dev/null 2>&1;			      \
	[ "$$failed" ] && echo -e "\nFailed configurations:$$failed"; \
	exit $$error;

checkall l4check:
	$(RM) -r $(ALLBUILDDIR)
	$(MAKE) dobuildparallel SHELL=bash

.PHONY: dobuildparallel checkallp

dobuildparallel: $(addprefix $(ALLBUILDDIR)/,$(TEST_TEMPLATES))
	@error=0;							      \
	echo "======================================================";        \
	for d in $(TEST_TEMPLATES); do                                        \
	  if [ -e $(ALLBUILDDIR)/$$d/build.failed ]; then                     \
	    error=1; failed="$$failed $$d";                                   \
	  fi;                                                                 \
	done;                                                                 \
	for f in $$failed; do echo "====== Failed Build Log: $$f ======";     \
	  tail -60 $(ALLBUILDDIR)/$$f/build.log;                              \
	done;                                                                 \
	rmdir $(ALLBUILDDIR) >/dev/null 2>&1;			              \
	[ "$$failed" ] && echo -e "\nFailed configurations:$$failed";         \
	exit $$error;

$(addprefix $(ALLBUILDDIR)/,$(TEST_TEMPLATES)):
	$(call buildmakefile,$@)
	cp $(TEMPLDIR)/globalconfig.out.$(patsubst $(ALLBUILDDIR)/%,%,$@)     \
	   $@/globalconfig.out
	$(MAKE) -C $@ 2>&1 | tee $@/build.log;                                \
	if [ $${PIPESTATUS[0]} = 0 ];                                         \
	then [ -z "$(KEEP_BUILD_DIRS)" ] && $(RM) -r $@;                      \
	else echo $${PIPESTATUS[0]} > $@/build.failed; fi; true

list:
	@echo "Templates:"
	@echo $(TEST_TEMPLATES)

randcheck:
	$(RM) -r $(RANDBUILDDIR);                                     \
	$(call buildmakefile,$(RANDBUILDDIR)/b);                      \
	while true; do                                                \
		$(RM) $(RANDBUILDDIR)/b/globalconfig.out;             \
		$(MAKE) -C $(RANDBUILDDIR)/b randconfig;              \
		fn=$$(cat $(RANDBUILDDIR)/b/globalconfig.out          \
		      | grep -e "^CONFIG_" | sort | sha1sum           \
		      | cut -f1 -d\   );                              \
		if [ -e "ok-$$fn" -o -e "failed-$$fn" ]; then         \
		  echo "Configuration $$fn already checked."          \
		  continue;                                           \
		fi;                                                   \
		if $(MAKE) -C $(RANDBUILDDIR)/b -j$(PL); then         \
			cp $(RANDBUILDDIR)/b/globalconfig.out         \
			  $(RANDBUILDDIR)/ok-$$fn;                    \
		else                                                  \
			[ -n "$$STOP_ON_ERROR" ] && exit 1;           \
			cp -a $(RANDBUILDDIR)/b                       \
			      $(RANDBUILDDIR)/failed-$$fn;            \
		fi;                                                   \
	done

randcheckstop:
	$(MAKE) STOP_ON_ERROR=1 randcheck

randcheckagain:
	for f in $(RANDBUILDDIR)/failed-*; do                         \
		if $(MAKE) -C $$f -j$(PL); then                       \
			$(RM) -rf $$f;                                \
		else                                                  \
			[ -n "$$STOP_ON_ERROR" ] && exit 1;           \
		fi                                                    \
	done

randcheckagainstop:
	$(MAKE) STOP_ON_ERROR=1 randcheckagain

help:
	@echo
	@echo "fiasco                    Builds the default configuration ($(DFL_TEMPLATE))"
	@echo "T=template                Build a certain configuration"
	@echo "checkall                  Build all template configurations in one go"
	@echo "list                      List templates"
	@echo
	@echo "config menuconfig xconfig oldconfig"
	@echo "                          Configure kernel in \"$(DFLBUILDDIR)\""
	@echo "$(DFLBUILDDIR)                     Build kernel in \"$(DFLBUILDDIR)\""
	@echo "clean cleanall            clean or cleanall in \"$(CLEANSUBDIRS)\""
	@echo "purge                     cleanall, remove \"$(DFLBUILDDIR)\" and build helper"
	@echo
	@echo "Creating a custom kernel:"
	@echo
	@echo "  Create a build directory with:"
	@echo "     make BUILDDIR=builddir"
	@echo "  Then build the kernel:"
	@echo "     cd builddir"
	@echo "     make config"
	@echo "     make"
	@echo
	@echo "Call \"make help\" in the build directory for more information on build targets."
	@echo
	@echo "Default target: $(DFLBUILDDIR)"
	@echo

.PHONY:	man install clean cleanall fiasco.builddir.create fiasco \
	l4check checkall config oldconfig menuconfig nconfig xconfig \
	randcheck randcheckstop help
