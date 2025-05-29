# SPDX-License-Identifier: GPL-2.0
# Copyright (C) 2025 Synopsys, Inc.
# Copyright (C) 2025 Synaptics Incorporated

#
# Makefile snipet to provide support for gcov tool
#

##############################################################################
# Compiler Options	
CFLAGS += \
	-fprofile-arcs \
	-ftest-coverage

##############################################################################
# Linker Options
LDFLAGS += \
	-fprofile-arcs

##############################################################################
# O U T P U T    A R T I F A C T S
##############################################################################

GCOV_DIR := gcov

GCNO_FILES := $(OBJS:.o=.gcno)
GCDA_FILES := $(OBJS:.o=.gcda)

GCOV_FILES := $(addsuffix .c.gcov,$(basename $(notdir $(wildcard $(OBJ_DIR)/*.gcda))))

##############################################################################
# M A K E F I L E    T A R G E T S
##############################################################################
.PHONY : coverage

%.c.gcov: $(OBJ_DIR)/%.gcda
	$(VERBOSE)gcov -o $(OBJ_DIR) $(basename $@)
	$(VERBOSE)mv $@ $(GCOV_DIR)/$(PREFIX).$@

coverage: $(GCOV_DIR) $(GCOV_FILES)

ifndef GCOV_MKDIR
$(GCOV_DIR):
	$(MKDIR) -p $(GCOV_DIR)
GCOV_MKDIR := 1
endif
	
clean::
	$(VERBOSE)$(RM) $(GCDA_FILES) $(GCNO_FILES) 
	$(VERBOSE)$(RM) $(addprefix $(GCOV_DIR)/$(PREFIX),$(GCOV_FILES))

help::
	@echo "make coverage	Create gcov files"
