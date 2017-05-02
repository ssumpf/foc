# -*- Makefile -*-
#
# DROPS (Dresden Realtime OPerating System) Component
#
# Makefile-Template for binary directories
#
# Makeconf is used, see there for further documentation
# install.inc is used, see there for further documentation
# binary.inc is used, see there for further documentation

ifeq ($(origin _L4DIR_MK_PROG_MK),undefined)
_L4DIR_MK_PROG_MK=y

ROLE = prog.mk

include $(L4DIR)/mk/Makeconf
$(GENERAL_D_LOC): $(L4DIR)/mk/prog.mk

# define INSTALLDIRs prior to including install.inc, where the install-
# rules are defined.
ifeq ($(MODE),host)
INSTALLDIR_BIN		?= $(DROPS_STDDIR)/bin/host
INSTALLDIR_BIN_LOCAL	?= $(OBJ_BASE)/bin/host
else
INSTALLDIR_BIN		?= $(DROPS_STDDIR)/bin/$(subst -,/,$(SYSTEM))
INSTALLDIR_BIN_LOCAL	?= $(OBJ_BASE)/bin/$(subst -,/,$(SYSTEM))
endif
ifeq ($(CONFIG_BID_STRIP_PROGS),y)
INSTALLFILE_BIN 	?= $(STRIP) --strip-unneeded $(1) -o $(2) && \
			   chmod 755 $(2)
INSTALLFILE_BIN_LOCAL 	?= $(STRIP) --strip-unneeded $(1) -o $(2) && \
			   chmod 755 $(2)
else
INSTALLFILE_BIN 	?= $(INSTALL) -m 755 $(1) $(2)
INSTALLFILE_BIN_LOCAL 	?= $(INSTALL) -m 755 $(1) $(2)
endif

INSTALLFILE		= $(INSTALLFILE_BIN)
INSTALLDIR		= $(INSTALLDIR_BIN)
INSTALLFILE_LOCAL	= $(INSTALLFILE_BIN_LOCAL)
INSTALLDIR_LOCAL	= $(INSTALLDIR_BIN_LOCAL)

# our mode
MODE 			?= static

# include all Makeconf.locals, define common rules/variables
include $(L4DIR)/mk/binary.inc

ifneq ($(SYSTEM),) # if we have a system, really build

TARGET_STANDARD := $(TARGET) $(TARGET_$(OSYSTEM))
TARGET_PROFILE := $(addsuffix .pr,$(filter $(BUILD_PROFILE),$(TARGET)))

$(call GENERATE_PER_TARGET_RULES,$(TARGET_STANDARD))
$(call GENERATE_PER_TARGET_RULES,$(TARGET_PROFILE),.pr)

TARGET	+= $(TARGET_$(OSYSTEM)) $(TARGET_PROFILE)

LDFLAGS_DYNAMIC_LINKER     := --dynamic-linker=rom/libld-l4.so
LDFLAGS_DYNAMIC_LINKER_GCC := $(addprefix -Wl$(BID_COMMA),$(LDFLAGS_DYNAMIC_LINKER))

# define some variables different for lib.mk and prog.mk
ifeq ($(MODE),shared)
LDFLAGS += $(LDFLAGS_DYNAMIC_LINKER)
endif
ifeq ($(CONFIG_BID_GENERATE_MAPFILE),y)
LDFLAGS += -Map $(strip $@).map
endif
LDFLAGS += $(addprefix -L, $(PRIVATE_LIBDIR) $(PRIVATE_LIBDIR_$(OSYSTEM)) $(PRIVATE_LIBDIR_$@) $(PRIVATE_LIBDIR_$@_$(OSYSTEM)))

# here because order of --defsym's is important
ifeq ($(MODE),l4linux)
  L4LX_USER_KIP_ADDR = 0xbfdfd000
  LDFLAGS += --defsym __L4_KIP_ADDR__=$(L4LX_USER_KIP_ADDR) \
             --defsym __l4sys_invoke_direct=$(L4LX_USER_KIP_ADDR)+$(L4_KIP_OFFS_SYS_INVOKE) \
             --defsym __l4sys_debugger_direct=$(L4LX_USER_KIP_ADDR)+$(L4_KIP_OFFS_SYS_DEBUGGER)
  CPPFLAGS += -DL4SYS_USE_UTCB_WRAP=1
else
ifneq ($(HOST_LINK),1)
  LDFLAGS += --defsym __L4_KIP_ADDR__=$(L4_KIP_ADDR) \
	     --defsym __L4_STACK_ADDR__=$(L4_STACK_ADDR)
endif
endif

ifneq ($(HOST_LINK),1)
  # linking for our L4 platform
  LDFLAGS += $(addprefix -L, $(L4LIBDIR))
  LDFLAGS += $(addprefix -T, $(LDSCRIPT))
  LDFLAGS += --start-group $(LIBS) $(L4_LIBS) --end-group
  LDFLAGS += --warn-common
else
  # linking for some POSIX platform
  ifeq ($(MODE),host)
    # linking for the build-platform
    LDFLAGS += -L$(OBJ_BASE)/lib/host
    LDFLAGS += $(LIBS)
  else
    # linking for L4Linux, we want to look for Linux-libs before the L4-libs
    LDFLAGS += $(GCCSYSLIBDIRS)
    LDFLAGS += $(addprefix -L, $(L4LIBDIR))
    LDFLAGS += $(LIBS) -Wl,-Bstatic $(L4_LIBS)
    # -Wl,-Bdynamic is only applicable for dynamically linked programs,
    ifeq ($(filter -static, $(LDFLAGS)),)
      LDFLAGS += -Wl,-Bdynamic
    endif
  endif
endif

LDFLAGS += $(LDFLAGS_$@)

ifeq ($(notdir $(LDSCRIPT)),main_stat.ld)
# ld denies -gc-section when linking against shared libraries
ifeq ($(findstring FOO,$(patsubst -l%.s,FOO,$(LIBS) $(L4_LIBS))),)
LDFLAGS += -gc-sections
endif
endif


include $(L4DIR)/mk/install.inc

#VPATHEX = $(foreach obj, $(OBJS), $(firstword $(foreach dir, \
#          . $(VPATH),$(wildcard $(dir)/$(obj)))))

# target-rule:

# Make looks for obj-files using VPATH only when looking for dependencies
# and applying implicit rules. Though make adapts its automatic variables,
# we cannot use them: The dependencies contain files which have not to be
# linked to the binary. Therefore the foreach searches the obj-files like
# make does, using the VPATH variable.
# Use a surrounding strip call to avoid ugly linebreaks in the commands
# output.

# Dependencies: When we have ld.so, we use MAKEDEP to build our
# library dependencies. If not, we fall back to LIBDEPS, an
# approximation of the correct dependencies for the binary. Note, that
# MAKEDEP will be empty if we dont have ld.so, LIBDEPS will be empty
# if we have ld.so.

ifeq ($(CONFIG_HAVE_LDSO),)
LIBDEPS = $(foreach file, \
                    $(patsubst -l%,lib%.a,$(filter-out -L%,$(LDFLAGS))) \
                    $(patsubst -l%,lib%.so,$(filter-out -L%,$(LDFLAGS))),\
                    $(word 1, $(foreach dir, \
                           $(patsubst -L%,%,\
                           $(filter -L%,$(LDFLAGS) $(L4ALL_LIBDIR))),\
                      $(wildcard $(dir)/$(file)))))
endif

DEPS	+= $(foreach file,$(TARGET), $(dir $(file)).$(notdir $(file)).d)

LINK_PROGRAM-C-host-1   := $(CC)
LINK_PROGRAM-CXX-host-1 := $(CXX)

LINK_PROGRAM  := $(LINK_PROGRAM-C-host-$(HOST_LINK))
ifneq ($(SRC_CC),)
LINK_PROGRAM  := $(LINK_PROGRAM-CXX-host-$(HOST_LINK))
endif

BID_LDFLAGS_FOR_LINKING_LD  = $(LDFLAGS)
BID_LDFLAGS_FOR_GCC         = $(filter     -static -shared -nostdlib -Wl$(BID_COMMA)% -L% -l%,$(LDFLAGS))
BID_LDFLAGS_FOR_LD          = $(filter-out -static -shared -nostdlib -Wl$(BID_COMMA)% -L% -l%,$(LDFLAGS))
BID_LDFLAGS_FOR_LINKING_GCC = $(addprefix -Wl$(BID_COMMA),$(BID_LDFLAGS_FOR_LD)) $(BID_LDFLAGS_FOR_GCC)

ifeq ($(LINK_PROGRAM),)
LINK_PROGRAM  := $(LD)
BID_LDFLAGS_FOR_LINKING = $(BID_LDFLAGS_FOR_LINKING_LD)
BID_LD_WHOLE_ARCHIVE = --whole-archive $1 --no-whole-archive
else
BID_LDFLAGS_FOR_LINKING = $(if $(HOST_LINK_TARGET),$(CCXX_FLAGS)) $(BID_LDFLAGS_FOR_LINKING_GCC)
BID_LD_WHOLE_ARCHIVE = -Wl,--whole-archive $1 -Wl,--no-whole-archive
endif

$(TARGET): $(OBJS) $(LIBDEPS) $(CRT0) $(CRTN)
	@$(LINK_MESSAGE)
	$(VERBOSE)$(call MAKEDEP,$(INT_LD_NAME),,,ld) $(LINK_PROGRAM) -o $@ $(CRT0) \
	            $(call BID_LD_WHOLE_ARCHIVE, $(OBJS)) \
	            $(BID_LDFLAGS_FOR_LINKING) $(CRTN)
	$(if $(BID_GEN_CONTROL),$(VERBOSE)echo "Requires: $(REQUIRES_LIBS)" >> $(PKGDIR)/Control)
	$(if $(BID_POST_PROG_LINK_MSG_$@),@$(BID_POST_PROG_LINK_MSG_$@))
	$(if $(BID_POST_PROG_LINK_$@),$(BID_POST_PROG_LINK_$@))
	@$(BUILT_MESSAGE)

endif	# architecture is defined, really build

-include $(DEPSVAR)
.PHONY: all clean cleanall config help install oldconfig txtconfig
help::
	@echo "  all            - compile and install the binaries"
ifneq ($(SYSTEM),)
	@echo "                   to $(INSTALLDIR_LOCAL)"
endif
	@echo "  install        - compile and install the binaries"
ifneq ($(SYSTEM),)
	@echo "                   to $(INSTALLDIR)"
endif
	@echo "  relink         - relink and install the binaries"
ifneq ($(SYSTEM),)
	@echo "                   to $(INSTALLDIR_LOCAL)"
endif
	@echo "  disasm         - disassemble first target"
	@echo "  scrub          - delete backup and temporary files"
	@echo "  clean          - delete generated object files"
	@echo "  cleanall       - delete all generated, backup and temporary files"
	@echo "  help           - this help"
	@echo
ifneq ($(SYSTEM),)
	@echo "  binaries are: $(TARGET)"
else
	@echo "  build for architectures: $(TARGET_SYSTEMS)"
endif

endif	# _L4DIR_MK_PROG_MK undefined
