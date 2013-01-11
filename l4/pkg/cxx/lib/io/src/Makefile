PKGDIR		?= ../../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= libcxx_io
SRC_CC		= iob_write.cc
PC_FILENAME     = cxx_io
SYSTEMS		= $(SYSTEMS_PLAIN)

include $(PKGDIR)/lib/Makefile.inc
include $(L4DIR)/mk/lib.mk

$(GENERAL_D_LOC): $(L4DIR)/pkg/cxx/lib/Makefile.inc
