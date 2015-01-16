PKGDIR ?= 	../..
L4DIR  ?= 	$(PKGDIR)/../..

TARGET         ?= libstdc++.a libstdc++.so
PC_FILENAME    ?= libstdc++
PC_LIBS        ?= -lstdc++
REQUIRES_LIBS  ?= libsupc++
STDCXX_PKG_DIR ?= $(PKGDIR_ABS)

PRIVATE_INCDIR += $(STDCXX_PKG_DIR)/private_inc

include $(L4DIR)/mk/lib.mk
include $(STDCXX_PKG_DIR)/contrib.inc
include $(STDCXX_PKG_DIR)/build/src/Makefile-$(STDCXX_CONTRIB_VERSION)

$(GENERAL_D_LOC): $(STDCXX_PKG_DIR)/build/src/Makefile-$(STDCXX_CONTRIB_VERSION)

vpath %.cc $(OBJ_DIR)
vpath %.cc $(STDCXX_CONTRIB_DIR)/config/io
vpath %.cc $(STDCXX_CONTRIB_DIR)/config/locale/generic

vpath %.cc $(STDCXX_CONTRIB_DIR)/src
ifeq ($(STDCXX_CONTRIB_VERSION_4.7_PLUS),)
else
endif

