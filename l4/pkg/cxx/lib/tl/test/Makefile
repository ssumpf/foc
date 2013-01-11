L4DIR := ../../../../..
INCLUDEDIR := ../include $(L4DIR)/include
CXXFLAGS += -g $(addprefix -I,$(INCLUDEDIR))
TESTS := avl_tree_test
all: do_test

do_test: $(addsuffix .output, $(TESTS))
	$(foreach TEST,$(TESTS),diff -Nu $(TEST).reference $(TEST).output &&) true

vpath %.h = $(INCLUDEDIR)

avl_tree_test: avl_tree_test.cc avl_tree.h

%.output: %
	$< >$@ 2>&1

%.reference: %
	$< >$@ 2>&1

references: $(addsuffix .reference,$(TESTS))

clean:
	rm -rf $(addsuffix .output,$(TESTS))
	rm -rf $(TESTS)

.PHONY: do_test references clean
