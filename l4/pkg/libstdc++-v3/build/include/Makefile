PKGDIR = 	../..
L4DIR ?= 	$(PKGDIR)/../..

$(error Tschüss)

CONTRIB_HEADERS = y
INCSRC_DIR      = $(PKGDIR_ABS)/contrib/libstdc++-v3/include
TARGET	= $(shell (cd $(INCSRC_DIR) && find . -type f -not -name 'Make*' -print))

LIBSUPC_H := exception_defines.h exception new typeinfo cxxabi.h cxxabi-forced.h

ALLOCATOR_H        := config/allocator/new_allocator_base.h
CLOCALE_H          := config/locale/generic/c_locale.h
ATOMIC_WORD_H      := config/cpu/generic/atomic_word.h
CPU_DEFINES_H      := config/cpu/generic/cpu_defines.h
TIME_MEMBERS_H     := config/locale/generic/time_members.h
MESSAGES_MEMBERS_H := config/locale/generic/messages_members.h
BASIC_FILE_H       := config/io/basic_file_stdio.h
CSTDIO_H           := config/io/c_io_stdio.h
CMATH_TCC          := include/c_std/cmath.tcc

CTYPE_HEADERS := config/os/generic/ctype_base.h config/os/generic/ctype_inline.h config/os/generic/ctype_noninline.h
# FIX BID:

ARCH=x86
CXX_ABI_TWEAKS_arm   = config/cpu/arm/cxxabi_tweaks.h
CXX_ABI_TWEAKS_x86   = config/cpu/generic/cxxabi_tweaks.h
CXX_ABI_TWEAKS_amd64 = config/cpu/generic/cxxabi_tweaks.h


include $(L4DIR)/mk/include.mk

INSTALL_INCDIR_X = $(INSTALLDIR_LOCAL)/$(INSTALL_INC_PREFIX)
SRCDIR_X         = $(PKGDIR_ABS)/contrib/libstdc++-v3

install_std_header = $(VERBOSE)$(LN) -sf $(INSTALL_INCDIR_X)/$(strip $(1))/* $(INSTALL_INCDIR_X)/$(strip $(2))

install_header = $(VERBOSE)$(LN) -sf $(SRCDIR_X)/$(strip $(1)) $(INSTALL_INCDIR_X)/$(strip $(2))

install_config_header = $(call install_header, $(1), bits/$(strip $(2)))

LIBSUPC_HI = $(addprefix $(INSTALL_INCDIR_X)/, $(LIBSUPC_H))

$(LIBSUPC_HI): %:
	$(call install_header, libsupc++/$(notdir $@), $(notdir $@))

all:: $(LIBSUPC_HI)
	@$(INSTALL_LINK_MESSAGE)
	$(call install_config_header, $(ALLOCATOR_H),     c++allocator.h)
	$(call install_config_header, $(CLOCALE_H),       c++locale.h)
	$(call install_config_header, $(ATOMIC_WORD_H),   atomic_word.h)
	$(call install_config_header, $(CPU_DEFINES_H),   cpu_defines.h)
#	$(call install_config_header, $(CTYPE_HEADERS),   .)
	$(call install_config_header, $(TIME_MEMBERS_H),  time_members.h)
	$(call install_config_header, $(MESSAGES_MEMBERS_H),  messages_members.h)
	$(call install_config_header, $(CXX_ABI_TWEAKS_$(ARCH)), cxxabi_tweaks.h)
	$(call install_config_header, $(BASIC_FILE_H), basic_file.h)
	$(call install_config_header, $(CSTDIO_H), c++io.h)
	$(call install_config_header, $(CMATH_TCC), cmath.tcc)
	$(call install_std_header, std)
#	$(call install_std_header, c)
	$(call install_std_header, c_std)
#	$(call install_std_header, c_global)
