PKGDIR		?= ../../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= libcxx_atexit
SYSTEMS		= $(SYSTEMS_PLAIN)
SRC_CC		= cxx_atexit.cc
PC_FILENAME     = cxx_atexit

include $(PKGDIR)/lib/Makefile.inc
include $(L4DIR)/mk/lib.mk

$(GENERAL_D_LOC): $(L4DIR)/pkg/cxx/lib/Makefile.inc
