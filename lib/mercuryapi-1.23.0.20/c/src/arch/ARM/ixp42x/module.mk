#
#  uCLinux Tools specification
#
#

TOOLS := GNU
PROCESSOR := arm

#
# Paths
#
TOOLS_ROOT      ?= /usr/local/arm-linux
TOOLS_PATH	    ?= $(TOOLS_ROOT)/bin
TOOLS_LIB_PATH	?= $(TOOLS_ROOT)/lib
TOOLS_INC	      ?= $(TOOLS_ROOT)/include

#
# Tools 
#
CC 	= $(TOOLS_PATH)/gcc
CPP	 ?= $(TOOLS_PATH)/g++
C++	 ?= $(TOOLS_PATH)/g++
OBJDUMP ?= $(TOOLS_PATH)/objdump
ifeq ($(BUILD), Debug)
STRIP ?= ls
else
STRIP ?= $(TOOLS_PATH)/strip
endif
# For some reason, gmake on devel1 (gentoo GNU Make 3.80) always predefines AR=ar
# so conditional assignment (?=) is useless -- use regular assignment (=) instead.
AR = $(TOOLS_PATH)/ar
CFLAGS += -O2 -I $(TOP_LEVEL)/arch/ARM/ixp42x/include

IXP425_FLAGS := -mcpu=strongarm -mtune=xscale
ifeq ($(ENDIANNESS), LITTLE)
  IXP425_FLAGS += -mlittle-endian
else
  IXP425_FLAGS += -mbig-endian
endif

ETH_NAME ?= ixp1
#CFLAGS += -DETHERNET_KERNEL_NAME=\"ixp1\"
CFLAGS += $(IXP425_FLAGS)
LDFLAGS += $(IXP425_FLAGS)

ifeq ($(BUILD), Debug)
CFLAGS          += -g
CDEFINES        += -DDEBUG
endif

