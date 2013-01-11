# vim:set ft=make:

TARGET_A 	:= $(TARGET).a
TARGET_E_A	:= $(TARGET)_e.a

PC_FILENAMES             := $(PC_FILENAME) $(PC_FILENAME)_e
PC_LIBS_$(PC_FILENAME)   := -l$(patsubst lib%,%,$(TARGET))
PC_LIBS_$(PC_FILENAME)_e := -l$(patsubst lib%,%,$(TARGET))_e

CXXFLAGS_amd64-l4f += -mcmodel=medium
CFLAGS_amd64-l4f += -mcmodel=medium

CXXFLAGS_F_A	      := $(patsubst %.cc,%.cxxflags,$(SRC_CC))
CXXFLAGS_F_E_A	      := $(patsubst %.cc,%.e.cxxflags,$(SRC_CC))

SRC_CC_$(TARGET_A)    := $(SRC_CC)
SRC_CC_$(TARGET_E_A)  := $(patsubst %.cc,%.e.cc,$(SRC_CC))
SRC_CC 		      :=

PRIVATE_INCDIR	+= $(SRC_DIR)/

TARGET := $(TARGET_A) $(TARGET_E_A)

ifneq ($(MAKECMDGOALS),)
  MAKEGOALS	:= $(MAKECMDGOALS)
else
  MAKEGOALS	:= all
endif

ifeq ($(findstring OBJ,$(shell pwd)),OBJ)
ifneq ($(filter-out clean cleanall scrub,$(MAKEGOALS)),)
  -include $(CXXFLAGS_F_A) $(CXXFLAGS_F_E_A)
endif
endif

all::

$(CXXFLAGS_F_A): %.cxxflags: %.cc $(L4DIR)/pkg/cxx/lib/Makefile.inc
	$(VERBOSE)echo "CXXFLAGS_$(notdir $<) := -DL4_NO_RTTI -Wall -fno-exceptions -fno-rtti" > $@

$(CXXFLAGS_F_E_A): %.cxxflags: %.cc $(L4DIR)/pkg/cxx/lib/Makefile.inc
	$(VERBOSE)echo "CXXFLAGS_$(notdir $<) := -Wall" > $@

%.e.cc: %.cc
	$(VERBOSE)ln -s $< $@
