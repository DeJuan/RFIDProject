#
#  Makefile to build CSharp implementation of the Mercury API
#

#
# Default target
#
default: all

BUILD      ?= .
TM_LEVEL   ?= ../../..
TOP_LEVEL  ?= ../../..
SRCDIR     ?= .
MODULESDIR ?= ${TM_DIR}/modules
PLATFORM   ?= PC
WINSRV     ?= tm-winbuild.tm.local
WINDIR     ?= swTreeM4
CFLAGS     += -Wall
ifeq ($(PLATFORM), PC)
  PLATFORM_CODE := x86
  PROCESSOR     := x86
else
  PLATFORM_CODE := arm
  PROCESSOR     := arm
endif
RSYNC   ?= cd ${SRCDIR}; rsync -e ssh -IWtrclpv --blocking-io --exclude '*.d' --exclude '*.pp*'
LN      ?= ln -fs
RM      ?= rm -f
CP      ?= scp -p
TEST    ?= test
MKDIR   ?=mkdir -p
ifeq ($(OSTYPE),Darwin)
  ISO_DATE := $(shell date ""+%Y-%m-%d"")
  RSED :=sed -E
else
  ISO_DATE := $(shell date --iso-8601)
  RSED :=sed -r
endif
ISO_TIME := $(shell date ""+%Y-%m-%dT%H:%M:%S%z"")


NODEPTARGETS = clean
TARGETS :=
OBJECTS :=
CLEANS :=
ALL_OBJS :=
ALL_C_SRCS :=

#
# Inherit rules
#
include $(TM_LEVEL)/swTree.mk

include ./module.mk
TARGETS += $(MERCURYAPI_CS_TARGETS)
#CLEANS  += $(MERCURYAPI_CS_CLEANS)

include $(TM_LEVEL)/rules.mk

DEPS := $(ALL_OBJS:$(OBJ_SUFFIX)=.d)
ifeq ($(OMITDEPS),0)
-include $(DEPS)
endif

# Create the build directory
create_build_dir := $(shell $(TEST) -d $(BUILD) || $(MKDIR) $(BUILD))

#
# main target rules
#
.PHONY: bin clean default doc
all: $(TARGETS)
#bin: MercuryAPI.dll MercuryAPI.xml MercuryAPICE.dll
#doc: MercuryAPI.chm

# Clean
clean: $(SLNCLEANS)
#	@rm -rf $(CLEANS) $(TARGETS) $(DEPS) $(ALL_OBJS)
	@echo "All generated files removed."
