#  Makefile to build the Java implementation of the Mercury API
#
#ifneq ($(MODULES_MERCURYAPI_JAVA_MAKE),1)
MODULES_MERCURYAPI_JAVA_MAKE := 1
#
# Definitions
#
MERCURYAPI_JARFILE         := $(BUILD)/mercuryapi.jar
ifdef TM_DIR
MERCURYAPI_JAVA_DIR        := $(TOP_LEVEL)/modules/mercuryapi/java
else
MERCURYAPI_JAVA_DIR        := .
endif
MERCURYAPI_JAVA_BIN_TARGET := $(MERCURYAPI_JARFILE)
ARCH_OS_DIR       ?= $(ARCHDIR)/linux
ARCH_MODULES_DIR  ?= $(ARCH_OS_DIR)/src/modules
ARCH_CHIP_DIR     ?= $(ARCHDIR)/x86
AUTOPARAMS ?= python $(MERCURYAPI_JAVA_DIR)/../autoparams.py

MERCURYAPI_JAVA_SUBDIR=com/thingmagic
MERCURYAPI_JAVA_NETBEANS_SRCDIR=$(MERCURYAPI_JAVA_DIR)/mercuryapi_nb/src
MERCURYAPI_JAVA_SRCDIR=$(MERCURYAPI_JAVA_NETBEANS_SRCDIR)/$(MERCURYAPI_JAVA_SUBDIR)
MERCURYAPI_JAVA_BUILD=$(BUILD)/$(MERCURYAPI_JAVA_SUBDIR)

MERCURYAPI_JAVA_SRCS= $(wildcard $(MERCURYAPI_JAVA_SRCDIR)/*.java)

## ${MERCURYAPI_JAVA_SRCDIR}/ClientHttpRequest.java \
##  ${MERCURYAPI_JAVA_SRCDIR}/EmbeddedReaderMessage.java \
##  ${MERCURYAPI_JAVA_SRCDIR}/Gen2.java \
##  ${MERCURYAPI_JAVA_SRCDIR}/Ipx64.java \
##  ${MERCURYAPI_JAVA_SRCDIR}/Ipx256.java \
##  ${MERCURYAPI_JAVA_SRCDIR}/Iso180006b.java\
##  ${MERCURYAPI_JAVA_SRCDIR}/MultiFilter.java\
##  ${MERCURYAPI_JAVA_SRCDIR}/MultiReadPlan.java\
##  ${MERCURYAPI_JAVA_SRCDIR}/ReaderCodeException.java\
##  ${MERCURYAPI_JAVA_SRCDIR}/ReaderCommException.java\
##  ${MERCURYAPI_JAVA_SRCDIR}/ReaderParseException.java\
##  ${MERCURYAPI_JAVA_SRCDIR}/ReaderFatalException.java\
##  ${MERCURYAPI_JAVA_SRCDIR}/ReaderException.java\
##  ${MERCURYAPI_JAVA_SRCDIR}/Reader.java\
##  ${MERCURYAPI_JAVA_SRCDIR}/ReadExceptionListener.java\
##  ${MERCURYAPI_JAVA_SRCDIR}/ReadListener.java\
##  ${MERCURYAPI_JAVA_SRCDIR}/ReadPlan.java\
##  ${MERCURYAPI_JAVA_SRCDIR}/RqlReader.java\
##  ${MERCURYAPI_JAVA_SRCDIR}/SerialTransport.java\
##  ${MERCURYAPI_JAVA_SRCDIR}/SerialTransportNative.java\
##  ${MERCURYAPI_JAVA_SRCDIR}/SerialReader.java\
##  ${MERCURYAPI_JAVA_SRCDIR}/SimpleReadPlan.java\
##  ${MERCURYAPI_JAVA_SRCDIR}/TagAuthentication.java\
##  ${MERCURYAPI_JAVA_SRCDIR}/TagData.java\
##  ${MERCURYAPI_JAVA_SRCDIR}/TagFilter.java\
##  ${MERCURYAPI_JAVA_SRCDIR}/TagLockAction.java\
##  ${MERCURYAPI_JAVA_SRCDIR}/TagProtocol.java\
##  ${MERCURYAPI_JAVA_SRCDIR}/TagReadData.java \
##  ${MERCURYAPI_JAVA_SRCDIR}/TMConstants.java \
##  ${MERCURYAPI_JAVA_SRCDIR}/FirmwareLoadOptions.java \
##  ${MERCURYAPI_JAVA_SRCDIR}/FeatureNotSupportedException.java \
##  ${MERCURYAPI_JAVA_SRCDIR}/SerialFirmwareLoadOptions.java \
##  ${MERCURYAPI_JAVA_SRCDIR}/RqlFirmwareLoadOptions.java \
##  ${MERCURYAPI_JAVA_SRCDIR}/LLRPFirmwareLoadOptions.java \
##  ${MERCURYAPI_JAVA_SRCDIR}/LLRPReader.java \
##  ${MERCURYAPI_JAVA_SRCDIR}/FixedReaderFirmwareLoadOptions.java \
##  ${MERCURYAPI_JAVA_SRCDIR}/TransportListener.java \
##  ${MERCURYAPI_JAVA_SRCDIR}/StatusListener.java \
##  ${MERCURYAPI_JAVA_SRCDIR}/ReaderUtil.java \
##  ${MERCURYAPI_JAVA_SRCDIR}/TagOp.java \
##  #${MERCURYAPI_JAVA_SRCDIR}/llrp/LlrpException.java \
##  #${MERCURYAPI_JAVA_SRCDIR}/llrp/LlrpMessage.java \
##  #${MERCURYAPI_JAVA_SRCDIR}/llrp/LlrpConnection.java \
##  #${MERCURYAPI_JAVA_SRCDIR}/llrp/LlrpCustomMessage.java

MERCURYAPI_JAVA_SRC_OBJS := $(MERCURYAPI_JAVA_SRCS:.java=.class)
MERCURYAPI_JAVA_OBJS     := $(subst ${MERCURYAPI_JAVA_SRCDIR},,${MERCURYAPI_JAVA_SRC_OBJS})
MERCURYAPI_JAVA_OBJS     := $(MERCURYAPI_JAVA_OBJS:%=${MERCURYAPI_JAVA_BUILD}/%)

ALL_OBJS += $(MERCURYAPI_JAVA_OBJS)
MERCURYAPI_JAVA_TARGETS  := $(MERCURYAPI_JAVA_BIN_TARGET)  $(BUILD)/demo.jar
# linux-x86.lib comes from C API JNI Makfile-generated .so
ifdef TM_DIR
  JNILIBDIR ?= $(BUILD)/$(MERCURYAPI_JAVA_SUBDIR)
else
  JNILIBDIR ?= .
endif
NATIVELIBS  += $(JNILIBDIR)/linux-amd64.lib
NATIVELIBS  += $(JNILIBDIR)/linux-x86.lib
NATIVELIBS  += $(JNILIBDIR)/mac-universal.lib
# win-x86.lib comes from C API JNI project-generated .dll
NATIVELIBS  += $(JNILIBDIR)/win-x86.lib
NATIVELIBS  += $(JNILIBDIR)/win-x64.lib
TARGETS  += $(NATIVELIBS)
TARGETS  += $(MERCURYAPI_JAVA_TARGETS)
TARGETS  += $(BUILD)/doc/index.html
TARGETS  += $(BUILD)/ltkjava-1.0.0.6.jar
TARGETS  += rfidreader_apk
TARGETS  += $(BUILD)/demo.sh $(BUILD)/demo.bat 
TARGETS  += $(BUILD)/sample.sh $(BUILD)/sample.bat 
TARGETS  += $(BUILD)/read.class $(BUILD)/readasync.class $(BUILD)/readasyncfilter.class 
TARGETS  += $(BUILD)/readasynctrack.class $(BUILD)/locktag.class
#TARGETS  += $(BUILD)/serialcommand.class $(BUILD)/serialtime.class $(BUILD)/commandtime.class 
TARGETS  += $(BUILD)/serialtime.class 
TARGETS  += $(BUILD)/filter.class $(BUILD)/BlockPermalock.class $(BUILD)/LicenseKey.class  
TARGETS  += $(BUILD)/SavedConfig.class $(BUILD)/BlockWrite.class $(BUILD)/MultiProtocol.class $(BUILD)/multireadasync.class
TARGETS  += $(BUILD)/writetag.class $(BUILD)/ReadAsyncFilterISO18k6b.class $(BUILD)/EmbeddedReadTID.class

CLEANS   += $(MERCURYAPI_JAVA_OBJS) $(MERCURYAPI_JAVA_BUILD)/*.class $(BUILD)/*.class $(BUILD)/*.jar $(BUILD)/*.sh $(BUILD)/*.bar $(BUILD)/*.so $(BUILD)/*.dylib $(BUILD)/*.dll $(BUILD)/manifest
NODEPTARGETS += mapi_java_set_product_number mapi_java_set_major_number
NODEPTARGETS += mapi_java_set_minor_number mapi_java_set_build_number

#
# Targets
#
$(MERCURYAPI_JARFILE): $(BUILD)/manifest $(MERCURYAPI_JAVA_OBJS) $(NATIVELIBS) 
	cp $(MERCURYAPI_JAVA_NETBEANS_SRCDIR)/log4j.properties $(BUILD)
	(cd $(BUILD); jar -cfm $(notdir $@) manifest log4j.properties $(MERCURYAPI_JAVA_SUBDIR)/*.class $(subst $(BUILD)/,,$(NATIVELIBS)))

$(MERCURYAPI_JAVA_BUILD)/%.class : $(MERCURYAPI_JAVA_DIR)/mercuryapi_nb/src/$(MERCURYAPI_JAVA_SUBDIR)/%.java $(MERCURYAPI_JAVA_DIR)/ltkjava-1.0.0.6.jar
	javac -target 1.5 -source 1.5 -d ${BUILD} -cp $(MERCURYAPI_JAVA_NETBEANS_SRCDIR):$(MERCURYAPI_JAVA_DIR)/ltkjava-1.0.0.6.jar $<

$(BUILD)/demo.jar : $(BUILD)/demo.class $(BUILD)/read.class $(BUILD)/readasync.class $(BUILD)/readasyncfilter.class $(BUILD)/readasynctrack.class
#$(BUILD)/demo.jar : $(BUILD)/locktag.class $(BUILD)/serialcommand.class $(BUILD)/serialtime.class $(BUILD)/commandtime.class $(BUILD)/filter.class
$(BUILD)/demo.jar : $(BUILD)/locktag.class $(BUILD)/serialtime.class $(BUILD)/filter.class
$(BUILD)/demo.jar : $(BUILD)/BlockPermalock.class $(BUILD)/BlockWrite.class $(BUILD)/LicenseKey.class  
$(BUILD)/demo.jar : $(BUILD)/SavedConfig.class $(BUILD)/MultiProtocol.class $(BUILD)/multireadasync.class
$(BUILD)/demo.jar : $(BUILD)/writetag.class $(BUILD)/ReadAsyncFilterISO18k6b.class $(BUILD)/EmbeddedReadTID.class
	(cd $(BUILD); jar -cf $(notdir $@) samples/demo*.class samples/read*.class samples/locktag*.class samples/serial*.class samples/filter*.class samples/multireadasync*.class samples/Block*.class samples/LicenseKey*.class samples/SavedConfig*.class samples/MultiProtocol*.class samples/writetag*.class samples/ReadAsyncFilterISO18k6b*.class samples/EmbeddedReadTID*.class)


RFIDREADER_DIR := $(MERCURYAPI_JAVA_DIR)/samples/android/RFIDReader
.PHONY: rfidreader_apk
rfidreader_apk: $(RFIDREADER_DIR)/RFIDReader.apk
$(RFIDREADER_DIR)/RFIDReader.apk: $(RFIDREADER_DIR)/bin/ReaderActivity-release.apk
	cp -p $< $@

JAVA_HOME ?= $(shell readlink -f /usr/bin/java |sed -e 's^/bin/java$$^^')

# If you don't have this distribution file, just download the Android SDK Tools
# (http://developer.android.com/sdk/index.html, then click on USE AN EXISTING IDE)
# Unpack to get the android-sdk directory,
# then run android-sdk/tools/android and install all the platform tools packages
ifdef TM_DIR
   ANDROID_SDK_DIST ?= $(TM_DIR)/../vendor/Android/android-sdk-with-platform-tools-linux-20130228.tar.bz2
else
   ANDROID_SDK_DIST ?= ../../../../vendor/Android/android-sdk-with-platform-tools-linux-20130228.tar.bz2
endif

ANDROID_SDK_DIR ?= $(BUILD)/android-sdk-linux
$(ANDROID_SDK_DIR)/tools/android: $(ANDROID_SDK_DIST)
	tar xf $< -C $(BUILD) 
	touch $@
CLEANS += $(ANDROID_SDK_DIR)

# If you don't have this distribution file, just download Apache ant
# (http://ant.apache.org/bindownload.cgi) and unpack to get the apache-ant directory
APACHE_ANT ?= apache-ant-1.8.4
ifdef TM_DIR
  ANT_DIST ?= $(TM_DIR)/../vendor/Apache/$(APACHE_ANT)-bin.tar.bz2
else
  ANT_DIST ?= ../../../../vendor/Apache/$(APACHE_ANT)-bin.tar.bz2
endif

ANT_HOME ?= $(BUILD)/$(APACHE_ANT)
ANT ?= $(ANT_HOME)/bin/ant
$(ANT): $(ANT_DIST)
	tar xf $< -C $(BUILD)
	touch $@
CLEANS += $(ANT_HOME)

RFIDREADER_APK_DEPS :=\
 $(ANDROID_SDK_DIR)/tools/android\
 $(ANT)\
 $(RFIDREADER_DIR)/libs/ltkjava-1.0.0.6.jar\
 $(RFIDREADER_DIR)/libs/mercuryapi.jar\
# TODO: Find a way to reference the master copies of mercuryapi.jar and ltkjava-1.0.0.6.jar
#   (in ../../..) rather than making local copies to <project>/libs

$(RFIDREADER_DIR)/bin/ReaderActivity-release.apk: $(RFIDREADER_APK_DEPS)
	cd $(RFIDREADER_DIR); ANT_HOME=$(realpath $(ANT_HOME)) JAVA_HOME=$(realpath $(JAVA_HOME)) $(realpath $(ANT)) -Dsdk.dir=$(realpath $(ANDROID_SDK_DIR)) release
$(RFIDREADER_DIR)/libs/ltkjava-1.0.0.6.jar: $(BUILD)/ltkjava-1.0.0.6.jar
	cp -p $(realpath $<) $@
$(RFIDREADER_DIR)/libs/mercuryapi.jar: $(BUILD)/mercuryapi.jar
	cp -p $(realpath $<) $@
CLEANS += $(RFIDREADER_DIR)/RFIDReader.apk
CLEANS += $(RFIDREADER_DIR)/bin/ReaderActivity-release.apk
CLEANS += $(RFIDREADER_APK_DEPS)


$(BUILD)/%.class : $(MERCURYAPI_JAVA_DIR)/samples_nb/src/samples/%.java $(MERCURYAPI_JARFILE)
	javac -target 1.5 -source 1.5 -d ${BUILD} -cp $(MERCURYAPI_JARFILE) $<

TM_DIR_FOUND := $(wildcard ../../../../tm)
TM_INTERNAL_BUILD := $(if $(TM_DIR_FOUND),1,0)
ifdef TM_DIR
  TM_INTERNAL_BUILD := 1
endif

ifneq ($(TM_INTERNAL_BUILD),0)
$(BUILD)/ltkjava-1.0.0.6.jar: $(MERCURYAPI_JAVA_DIR)/ltkjava-1.0.0.6.jar
	cp $< $@

$(BUILD)/manifest: $(MERCURYAPI_JAVA_DIR)/manifest
	cp $< $@

$(BUILD)/demo.sh: $(MERCURYAPI_JAVA_DIR)/samples_nb/src/samples/demo.sh
	cp $< $@

$(BUILD)/demo.bat: $(MERCURYAPI_JAVA_DIR)/samples_nb/src/samples/demo.bat
	cp $< $@

$(BUILD)/sample.sh: $(MERCURYAPI_JAVA_DIR)/samples_nb/src/samples/sample.sh
	cp $< $@

$(BUILD)/sample.bat: $(MERCURYAPI_JAVA_DIR)/samples_nb/src/samples/sample.bat
	cp $< $@

$(BUILD)/$(MERCURYAPI_JAVA_SUBDIR)/%.lib: $(MERCURYAPI_JAVA_DIR)/%.lib
	mkdir -p $(BUILD)/$(MERCURYAPI_JAVA_SUBDIR)
	cp $< $@

$(BUILD)/doc/index.html: $(MERCURYAPI_JAVA_SRCS) autoparams_java $(MERCURYAPI_JAVA_DIR)/ltkjava-1.0.0.6.jar
	mkdir -p $(BUILD)/doc
	javadoc -public -sourcepath $(MERCURYAPI_JAVA_NETBEANS_SRCDIR) -classpath $(MERCURYAPI_JAVA_DIR)/ltkjava-1.0.0.6.jar  -d $(BUILD)/doc com.thingmagic

autoparams_java: $(MERCURYAPI_JAVA_SRCS)
	$(AUTOPARAMS) -o $(MERCURYAPI_JAVA_SRCDIR)/Reader.java --template=$(MERCURYAPI_JAVA_SRCDIR)/Reader.java $^

AUTOPARAM_CLEANS += $(MERCURYAPI_JAVA_SRCDIR)/Reader.java

endif

# Change the build number in the version string
# The version string must look like "1.2.3.0"
# The 0 build number will be replaced with the contents of the variable BUILDNUMBER
PRODUCTNUMBER?=0
mapi_java_set_product_number: $(MERCURYAPI_JAVA_DIR)/manifest
# Set the java manifest Specification Version
	@$(RSED) 's/^(Specification-Version:\ *\")([0-9]+).([0-9]+).([0-9]+)(\")/\1$(PRODUCTNUMBER).\3.\4\5/' < $(MERCURYAPI_JAVA_DIR)/manifest > $(MERCURYAPI_JAVA_DIR)/manifest.new
	@mv $(MERCURYAPI_JAVA_DIR)/manifest.new $(MERCURYAPI_JAVA_DIR)/manifest
# Set the java manifest Implementation Version
	@$(RSED) 's/^(Implementation-Version:\ *\")([0-9]+).([0-9]+).([0-9]+).([0-9]+)(\")/\1$(PRODUCTNUMBER).\3.\4.\5\6/' < $(MERCURYAPI_JAVA_DIR)/manifest > $(MERCURYAPI_JAVA_DIR)/manifest.new
	@mv $(MERCURYAPI_JAVA_DIR)/manifest.new $(MERCURYAPI_JAVA_DIR)/manifest

MAJORNUMBER?=0
mapi_java_set_major_number: $(MERCURYAPI_JAVA_DIR)/manifest
# Set the java manifest Specification Version
	@$(RSED) 's/^(Specification-Version:\ *\")([0-9]+).([0-9]+).([0-9]+)(\")/\1\2.$(MAJORNUMBER).\4\5/' < $(MERCURYAPI_JAVA_DIR)/manifest > $(MERCURYAPI_JAVA_DIR)/manifest.new
	@mv $(MERCURYAPI_JAVA_DIR)/manifest.new $(MERCURYAPI_JAVA_DIR)/manifest
# Set the java manifest Implementation Version
	@$(RSED) 's/^(Implementation-Version:\ *\")([0-9]+).([0-9]+).([0-9]+).([0-9]+)(\")/\1\2.$(MAJORNUMBER).\4.\5\6/' < $(MERCURYAPI_JAVA_DIR)/manifest > $(MERCURYAPI_JAVA_DIR)/manifest.new
	@mv $(MERCURYAPI_JAVA_DIR)/manifest.new $(MERCURYAPI_JAVA_DIR)/manifest

MINORNUMBER?=0
mapi_java_set_minor_number: $(MERCURYAPI_JAVA_DIR)/manifest
# Set the java manifest Specification Version
	@$(RSED) 's/^(Specification-Version:\ *\")([0-9]+).([0-9]+).([0-9]+)(\")/\1\2.\3.$(MINORNUMBER)\5/' < $(MERCURYAPI_JAVA_DIR)/manifest > $(MERCURYAPI_JAVA_DIR)/manifest.new
	@mv $(MERCURYAPI_JAVA_DIR)/manifest.new $(MERCURYAPI_JAVA_DIR)/manifest
# Set the java manifest Implementation Version
	@$(RSED) 's/^(Implementation-Version:\ *\")([0-9]+).([0-9]+).([0-9]+).([0-9]+)(\")/\1\2.\3.$(MINORNUMBER).\5\6/' < $(MERCURYAPI_JAVA_DIR)/manifest > $(MERCURYAPI_JAVA_DIR)/manifest.new
	@mv $(MERCURYAPI_JAVA_DIR)/manifest.new $(MERCURYAPI_JAVA_DIR)/manifest

BUILDNUMBER?=0
mapi_java_set_build_number: $(MERCURYAPI_JAVA_DIR)/manifest
# Set the java manifest Implementation Version
	@$(RSED) 's/^(Implementation-Version:\ *\")([0-9]+).([0-9]+).([0-9]+).([0-9]+)(\")/\1\2.\3.\4.$(BUILDNUMBER)\6/' < $(MERCURYAPI_JAVA_DIR)/manifest > $(MERCURYAPI_JAVA_DIR)/manifest.new
	@mv $(MERCURYAPI_JAVA_DIR)/manifest.new $(MERCURYAPI_JAVA_DIR)/manifest

.INTERMEDIATE: $(MERCURYAPI_JAVA_SRC_OBJS)
#endif
