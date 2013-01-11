L4DIR ?= ..

# Be sure to add a "/" to config to make the target different from
# BID's "config" target
TARGET := $(wildcard config/ gendep elf-patcher)
TARGET_doc := $(wildcard gendep)

all::
doc:
	$(if $(TARGET_doc), $(VERBOSE)set -e ; for d in $(TARGET_doc) ; do \
		PWD=$(PWD)/$$d $(MAKE) -C $$d $@ ; done )

include $(L4DIR)/mk/subdir.mk
