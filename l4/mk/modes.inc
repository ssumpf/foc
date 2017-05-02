# -*- Makefile -*-
# vim:et:ts=8:ft=make:
#
# Makefile-Include for defining modes

ifneq ($(L4_MULTITHREADED),)
  $(error L4_MULTITHREADED is obsolete, just remove, reentrance is default now, you might want to add 'libpthread' to REQUIRES_LIBS)
endif

ifneq ($(MODE_USE_C),)
  $(error MODE_USE_C is obsolete, add l4re_c and/or l4re_c-util to REQUIRES_LIBS as needed)
endif

CRTBEGIN_so       = $(if $(GCCIS_sparc_leon),$(GCCLIB_FILE_crtbegin.o),$(GCCLIB_FILE_crtbeginS.o))
CRTBEGIN_stat_bin = $(if $(GCCIS_sparc_leon),$(GCCLIB_FILE_crtbegin.o),$(GCCLIB_FILE_crtbeginT.o))

CRTBEGIN_dyn_bin  = $(GCCLIB_FILE_crtbegin.o)

CRTEND_so         = $(GCCLIB_FILE_crtendS.o)
CRTEND_dyn_bin    = $(GCCLIB_FILE_crtend.o)
CRTEND_stat_bin   = $(GCCLIB_FILE_crtend.o)


CRT1_so           =
CRT1_dyn_bin      = $(call findfile,crt1.o,$(L4LIBDIR))
CRT1_stat_bin     = $(call findfile,crt1.o,$(L4LIBDIR))

CRTI_so           = $(call findfile,crti.s.o,$(L4LIBDIR))
CRTI_dyn_bin      = $(call findfile,crti.o,$(L4LIBDIR))
CRTI_stat_bin     = $(call findfile,crti.o,$(L4LIBDIR))

CRTN_so           = $(if $(GCCIS_sparc_leon),,$(CRTEND_so)) $(call findfile,crtn.s.o,$(L4LIBDIR))
CRTN_dyn_bin      = $(CRTEND_dyn_bin)  $(call findfile,crtn.o,$(L4LIBDIR))
CRTN_stat_bin     = $(CRTEND_stat_bin) $(call findfile,crtn.o,$(L4LIBDIR))

LDS_so            = $(call findfile,main_rel.ld,$(L4LIBDIR))
LDS_dyn_bin       = $(call findfile,main_dyn.ld,$(L4LIBDIR))
LDS_stat_bin      = $(call findfile,main_stat.ld,$(L4LIBDIR))


LIBC_INCLUDE   = $(BID_NOSTDINC)                            \
                 -I$(OBJ_BASE)/include/contrib/libstdc++-v3 \
                 -I$(OBJ_BASE)/include/uclibc               \
                 $(if $(CONFIG_USE_DROPS_STDDIR),           \
                  -I$(DROPS_STDDIR)/include/contrib/libstdc++-v3,) \
                  -I$(DROPS_STDDIR)/include/uclibc          \
                 $(I_GCCINCDIR)

#############################################################################

################################
# sigma0 mode
################################

BID_SUPPORTED_all_sigma0        = y
LIBCINCDIR_all_sigma0           = $(LIBC_INCLUDE)
REQUIRES_LIBS_all_sigma0        =
L4_LIBS_all_sigma0		= -static -nostdlib                     \
                                  $(GCCLIB) $(GCCLIB_EH) -lcrt0         \
				  $(EXTRA_LIBS)
CRT0_all_sigma0                 = $(CRTI_stat_bin) $(CRTBEGIN_stat_bin) $(CRT1_stat_bin)
CRTN_all_sigma0                 = $(CRTN_stat_bin)
LDSCRIPT_all_sigma0             = $(LDS_stat_bin)
LDFLAGS_all_sigma0              = -Bstatic


################################
# static mode
################################

BID_SUPPORTED_all_static        = y
LIBCINCDIR_all_static           = $(LIBC_INCLUDE)
REQUIRES_LIBS_all_static        = stdlibs
L4_LIBS_all_static		= -static -nostdlib  $(GCCLIB) $(GCCLIB_EH) \
                                  $(EXTRA_LIBS)
CRT0_all_static                 = $(CRTI_stat_bin) $(CRTBEGIN_stat_bin) $(CRT1_stat_bin)
CRTN_all_static                 = $(CRTN_stat_bin)
LDSCRIPT_all_static             = $(LDS_stat_bin)
LDFLAGS_all_static              = -Bstatic

#####################################
# shared mode -- programs are linked against shared libs
#####################################
BID_SUPPORTED_all_shared        = y
LIBCINCDIR_all_shared           = $(LIBC_INCLUDE)
CRT0_all_shared                 = $(CRTI_dyn_bin) $(CRTBEGIN_dyn_bin) $(CRT1_dyn_bin)
CRTN_all_shared                 = $(CRTN_dyn_bin)
LDSCRIPT_all_shared             = $(LDS_dyn_bin)
REQUIRES_LIBS_all_shared        = stdlibs-sh
L4_LIBS_all_shared             += -nostdlib -luc_c_nonshared.p \
				  $(GCCLIB_SO) $(GCCLIB_EH)    \
				  $(EXTRA_LIBS)
LDFLAGS_all_shared              = --eh-frame-hdr

################################
# l4linux mode
################################

BID_SUPPORTED_all_l4linux       = y
LIBCINCDIR_all_l4linux          =
REQUIRES_LIBS_all_l4linux       = 
L4_LIBS_all_l4linux		= -l4re-util                        \
                                  -l4lx-re.o                        \
				  -ll4sys-l4x -l4re                 \
				  $(EXTRA_LIBS)                     \
                                  -ll4util                          \
				  -lcxx_base -lcxx_io               \
				  -lcxx_libc_io
CRT0_all_l4linux                 =
CRTN_all_l4linux                 =
LDSCRIPT_all_l4linux             =
LDFLAGS_all_l4linux              =


#####################################
# lib mode -- options when building libs
#####################################

BID_SUPPORTED_all_lib = y
LIBCINCDIR_all_lib    = $(LIBC_INCLUDE)
CRT0_all_lib          = $(CRTI_dyn_bin) $(CRTBEGIN_dyn_bin) $(CRT1_dyn_bin)
CRTN_all_lib          = $(CRTN_dyn_bin)
LDSCRIPT_all_lib      = $(LDS_so)
LDFLAGS_all_lib       = --eh-frame-hdr


#####################################
# host mode -- no L4 specifics
#####################################

BID_SUPPORTED_all_host = y
LIBCINCDIR_all_host    =
REQUIRES_LIBS_all_host =
L4_LIBS_all_host       =
CRT0_all_host          =
CRTN_all_host          =
LDSCRIPT_all_host      =
LDFLAGS_all_host       =
