PKGDIR ?= .
L4DIR  ?= $(PKGDIR)/../..

LIBCSRC_DIR = $(PKGDIR)/../uclibc/lib/uclibc

all:: include
include:

include $(LIBCSRC_DIR)/make_vars.mk

HEADERS_LST := $(LIBCSRC_DIR_ABS)/target_headers.lst
DST_HEADERS := $(shell cat $(HEADERS_LST))
HEADER_DIR  := $(OBJ_DIR)/install_includes

# just a few additions, handle them here directly
DST_HEADERS-arm     := bits/arm_asm.h
DST_HEADERS-arm     += bits/arm_bx.h
DST_HEADERS-sparc   := linux/sockios.h asm/traps.h
DST_HEADERS-ppc32   := asm/sockios.h asm/ioctl.h

DST_HEADERS += $(DST_HEADERS-$(BUILD_ARCH))

vpath %.h $(LIBCSRC_DIR)/ARCH-$(BUILD_ARCH)/include
vpath %.h $(LIBCSRC_DIR)/ARCH-all/include
vpath %.h $(CONTRIB_DIR)/libc/sysdeps/linux/$(UCLIBC_ARCH)
vpath %.h $(CONTRIB_DIR)/libc/sysdeps/linux/common
vpath %.h $(CONTRIB_DIR)/include

all:: $(HEADERS_LST)

DST_HEADERS   := $(addprefix $(HEADER_DIR)/,$(DST_HEADERS))
INCDIRS_OBJ_m := $(HEADER_DIR)/Makefile

$(DST_HEADERS): $(HEADER_DIR)/%: %
	@$(INSTALL_LINK_MESSAGE)
	$(VERBOSE)[ -d $(@D) ] || $(MKDIR) -p $(@D)
	$(VERBOSE)$(LN) -sf $(call absfilename,$<) $@


$(INCDIRS_OBJ_m):
	$(VERBOSE)install -d $(dir $@)
	$(VERBOSE)echo 'PKGDIR   ?= $(PKGDIR_ABS)' > $@
	$(VERBOSE)echo 'L4DIR    ?= $(L4DIR_ABS)' >> $@
	$(VERBOSE)echo 'OBJ_BASE ?= $(OBJ_BASE)' >> $@
	$(VERBOSE)echo 'OBJ_DIR  ?= $(OBJ_DIR)' >> $@
	$(VERBOSE)echo 'INSTALL_INC_PREFIX = uclibc' >> $@
	$(VERBOSE)echo 'include $$(L4DIR)/mk/include.mk' >> $@

include: $(DST_HEADERS) $(INCDIRS_OBJ_m)
	$(VERBOSE)$(MAKE) $(MKFLAGS) -C $(HEADER_DIR)
	$(VERBOSE)$(MAKE) $(MKFLAGS) -C $(LIBCSRC_DIR)/../libpthread/include


#all include::
#	$(MAKE) $(MKFLAGS) -C $(PKGDIR)/../uclibc/lib include

