# Cygwin Makefile for MercuryAPI C# and auxiliary materials
#
# make bin -- Build binaries (MercuryAPI.dll, Demo.exe, TestM5eCommands.exe)
#  * Prerequisites:
#    * .NET Framework 2.0 SDK ( http://www.microsoft.com/downloads/details.aspx?FamilyID=fe6f2099-b7b4-4f47-a244-c96d69c35dec )
#    * .NET Framework 2.0 Service Pack 1 ( http://www.microsoft.com/downloads/details.aspx?familyid=79BC3B77-E02C-4AD3-AACF-A7633F706BA5 )
#
# make doc -- Auto-generated documentation
#  * Prerequisites: (For details, see http://www.ewoodruff.us/shfbdocs/Index.aspx?topic=html/8c0c97d0-c968-4c15-9fe9-e8f3a443c50a.htm)
#    * Sandcastle, Sandcastle Styles patches, Sandcastle Help File Builder
#    * Cygwin zip (There might also be a native Windows version, but I don't know if the syntax is the same.)

#ifneq ($(MODULES_MERCURYAPI_CS_MAKE),1)
MODULES_MERCURYAPI_CS_MAKE := 1
include $(TOP_LEVEL)/make_functions.mk
#
# Definitions
#
MERCURYAPI_CS_DIR        := $(TOP_LEVEL)/modules/mercuryapi/cs
ARCH_OS_DIR       ?= $(ARCHDIR)/linux
ARCH_MODULES_DIR  ?= $(ARCH_OS_DIR)/src/modules
ARCH_CHIP_DIR     ?= $(ARCHDIR)/ARM/ixp42x
MERCURYAPI_CS_OUTPUTS += $(MERCURYAPI_CS_DIR)/MercuryAPI.dll $(MERCURYAPI_CS_DIR)/MercuryAPI.xml $(MERCURYAPI_CS_DIR)/MercuryAPICE.dll
MERCURYAPI_CS_OUTPUTS += $(MERCURYAPI_CS_DIR)/Doc/MercuryAPI.chm $(MERCURYAPI_CS_DIR)/Doc/MercuryAPIHelp
CLEANS                += $(MERCURYAPI_CS_OUTPUTS) $(MERCURYAPI_CS_TARGETS) $(MERCURYAPI_CS_CLEANS) $(MERCURYAPI_CS_DIR)/Doc/MercuryAPI.chm
NODEPTARGETS += mapi_cs_set_product_number mapi_cs_set_major_number
NODEPTARGETS += mapi_cs_set_minor_number mapi_cs_set_build_number


#.PHONY: bin clean default doc
#all: bin doc
#bin: MercuryAPI.dll MercuryAPI.xml MercuryAPICE.dll
#

# Uncomment to stub out Microsoft tools to speed up Makefile debugging.
# Be sure to run unstubbed first to put targets in place.
#STUB_MSBUILDS := 1  # Stubs

# MSBuild
# Note multiple versions, each one for a different version of the .NET framework
# v3.5 is capable of building older v2.0 projects, but I'm afraid of accidentally
# introducing forward dependencies on the 3.5 framework.
ifdef STUB_MSBUILDS
MSBUILD2 ?= echo MSBUILD
MSBUILD3 ?= echo MSBUILD
MSBUILD4 ?= echo MSBUILD
else
MSBUILD2 ?= "/cygdrive/c/WINDOWS/Microsoft.NET/Framework/v2.0.50727/MSBuild.exe"
MSBUILD3 ?= "/cygdrive/c/WINDOWS/Microsoft.NET/Framework/v3.5/MSBuild.exe"
MSBUILD4 ?= "/cygdrive/c/WINDOWS/Microsoft.NET/Framework/v4.0.30319/MSBuild.exe"
endif

# CabWiz: Builds CAB files from command line, given .INF description file
CABWIZ ?= "/cygdrive/c/Program Files/Windows Mobile 6 SDK/Tools/CabWiz/Cabwiz.exe"

# Add a solution file (that lives within $(MERCURYAPI_CS_DIR))
# Usage: $(call AddSolution,name.sln,list of targets made by solution)
# Example:
# $(eval $(call AddSolution,MercuryAPI.sln,\
#  MercuryAPI.dll\
#  MercuryAPI.xml\
#  MercuryAPICE.dll\
# ))
define AddSolution
SLNS += $(MERCURYAPI_CS_DIR)/$(2)
MERCURYAPI_CS_TARGETS += target-$(2)
.PHONY: target-$(2)
target-$(2):
	$(1) /t:Build /p:Configuration=Release $(2)
SLNCLEANS += clean-$(2)
clean-$(2):
	$(1) /t:Clean /p:Configuration=Release $(2)
endef

# $(MERCURYAPI_CS_DIR)/MercuryAPI.dll $(MERCURYAPI_CS_DIR)/MercuryAPI.xml $(MERCURYAPI_CS_DIR)/MercuryAPICE.dll
$(eval $(call AddSolution,$(MSBUILD2),MercuryAPI.sln,\
 MercuryAPICE.dll\
 MercuryAPI.dll\
 MercuryAPI.xml\
))

$(eval $(call AddSolution,$(MSBUILD2),Samples/Codelets/Codelets.sln,\
 $(foreach codelet,\
   BlockPermaLock\
   BlockWrite\
   CommandTime\
   EmbeddedReadTID\
   Filter\
   LicenseKey\
   LockTag\
   MultiProtocolRead\
   Read\
   ReadAsync\
   ReadAsyncFilter\
   ReadAsyncTrack\
   SavedConfig\
   SerialCommand\
   SerialTime\
   SL900AProjectGetCalibrationData\
   SL900AProjectGetSensorValue\
   SL900AProjectSetCalibrationData\
   SL900AProjectSetSFEParamters\
   WriteTag\
   ReadAsyncFilter-ISO18k-6b\
   DenatranIAVCustomTagOperations\
   FastId\
   ReaderInformation\
   AntennaList\
   SecureReadData\
   ReaderStats\
   ReadStopTrigger\
   RebootReader\
   Firmware\
   ReadCustomTransport\
   MultireadAsync\
   ,$(codelet).exe)\
))


# Add solution from Samples subdirectory
# Example: $(call AddSample,$(MSBUILD2),Demo-AssetTrackingTool)
define AddSample
$(call AddSolution,$(1),Samples/$(2)/$(2).sln,$(2).exe)
endef

$(eval $(call AddSample,$(MSBUILD2),MercuryApiLongTest))

$(eval $(call AddSample,$(MSBUILD3),Demo-AssetTrackingTool))
$(eval $(call AddSample,$(MSBUILD3),Demo-ReadingWithGPIO))
$(eval $(call AddSample,$(MSBUILD3),DemoProg-WindowsForm))
$(eval $(call AddSample,$(MSBUILD3),M6e-AssetTracking-Demo-Tool))
$(eval $(call AddSample,$(MSBUILD3),ThingMagic-Reader-Firmware-Upgrade))
$(eval $(call AddSample,$(MSBUILD3),RFIDSearchLight))
$(eval $(call AddSample,$(MSBUILD4),UniversalReaderAssistant2.0))
$(eval $(call AddSample,$(MSBUILD3),WinCE-ReadApp))

# TODO: Factor out CAB file build definition

MERCURYAPI_CS_TARGETS += $(MERCURYAPI_CS_DIR)/RFIDSearchLightInstaller.CAB
.PHONY: $(MERCURYAPI_CS_DIR)/RFIDSearchLightInstaller.CAB
$(MERCURYAPI_CS_DIR)/RFIDSearchLightInstaller.CAB: $(MERCURYAPI_CS_DIR)/RFIDSearchLight.CAB
	# CabWiz has no dependency checking, so always build clean
	rm -f Samples/RFIDSearchLight/RFIDSearchLightMultiCab/ManualEdit/RFIDSearchLightInstaller.CAB
	# CabWiz workarounds
	# (NOTE: error reporting is very poor, tends to just say,
	#  Error: CAB file "..." could not be created):
	#   * Sends error output to UTF-16 file instead of stderr
	#   * Needs TMP dir, which is not set by "ssh winbuild"
	#   * Wants absolute path to input filename
	TMP='C:\temp' $(CABWIZ) '$(shell cygpath -aw Samples/RFIDSearchLight/RFIDSearchLightMultiCab/ManualEdit/RFIDSearchLightInstaller.inf)' /err CabWiz.log || (cat Samples/RFIDSearchLight/RFIDSearchLightMultiCab/ManualEdit/CabWiz.log |tr -d '\000'; exit 1)
	cp -p Samples/RFIDSearchLight/RFIDSearchLightMultiCab/ManualEdit/RFIDSearchLightInstaller.CAB $@

MERCURYAPI_CS_TARGETS += $(MERCURYAPI_CS_DIR)/RFIDSearchLight.CAB
.PHONY: $(MERCURYAPI_CS_DIR)/RFIDSearchLight.CAB
$(MERCURYAPI_CS_DIR)/RFIDSearchLight.CAB:
	# CabWiz has no dependency checking, so always build clean
	rm -f Samples/RFIDSearchLight/RFIDSearchLightCab/ManualEdit/RFIDSearchLight.CAB
	# CabWiz workarounds
	# (NOTE: error reporting is very poor, tends to just say,
	#  Error: CAB file "..." could not be created):
	#   * Sends error output to UTF-16 file instead of stderr
	#   * Needs TMP dir, which is not set by "ssh winbuild"
	#   * Wants absolute path to input filename
	TMP='C:\temp' $(CABWIZ) '$(shell cygpath -aw Samples/RFIDSearchLight/RFIDSearchLightCab/ManualEdit/RFIDSearchLight.inf)' /err CabWiz.log || (cat Samples/RFIDSearchLight/RFIDSearchLightCab/ManualEdit/CabWiz.log |tr -d '\000'; exit 1)
	cp -p Samples/RFIDSearchLight/RFIDSearchLightCab/ManualEdit/RFIDSearchLight.CAB $@

MERCURYAPI_CS_TARGETS += $(MERCURYAPI_CS_DIR)/WinceReadApp.CAB
.PHONY: $(MERCURYAPI_CS_DIR)/WinceReadApp.CAB
$(MERCURYAPI_CS_DIR)/WinceReadApp.CAB:
	# CabWiz has no dependency checking, so always build clean
	rm -f Samples/WinCE-ReadApp/WinceReadAppCabInstaller/ManualEdit/WinceReadApp.CAB
	# CabWiz workarounds
	# (NOTE: error reporting is very poor, tends to just say,
	#  Error: CAB file "..." could not be created):
	#   * Sends error output to UTF-16 file instead of stderr
	#   * Needs TMP dir, which is not set by "ssh winbuild"
	#   * Wants absolute path to input filename
	TMP='C:\temp' $(CABWIZ) '$(shell cygpath -aw Samples/WinCE-ReadApp/WinceReadAppCabInstaller/ManualEdit/WinceReadApp.inf)' /err CabWiz.log || (cat Samples/WinCE-ReadApp/WinceReadAppCabInstaller/ManualEdit/CabWiz.log |tr -d '\000'; exit 1)
	cp -p Samples/WinCE-ReadApp/WinceReadAppCabInstaller/ManualEdit/WinceReadApp.CAB $@
	
#MERCURYAPI_CS_TARGETS += $(MERCURYAPI_CS_DIR)/Samples/exe/URA2Installer.exe
#.PHONY: $(MERCURYAPI_CS_DIR)/Samples/exe/URA2Installer.exe
#$(MERCURYAPI_CS_DIR)/URA2Installer.exe:
#	# Copy URA MSI
#	cp -p Samples/UniversalReaderAssistant2.0/URA2Installer/bin/Release/URA2Installer.msi $@
#	rm -f Samples/UniversalReaderAssistant2.0/URA2Installer/bin/Release/URA2Installer.msi
## TODO: Do we really need to delete the source afterwards?  Is this supposed to work around some dependency deficiency?

# Sandcastle Help File Builder
ifdef STUB_MSBUILDS
SHFB ?= echo Faking SHFB: Too slow for debugging...; mkdir -p $(MERCURYAPI_CS_DIR)/Doc/MercuryAPIHelp; touch $(MERCURYAPI_CS_DIR)/Doc/MercuryAPIHelp/MercuryAPI.chm; touch $(MERCURYAPI_CS_DIR)/Doc/MercuryAPIHelp/Index.html
else
SHFB ?= "/cygdrive/c/Program Files/EWSoftware/Sandcastle Help File Builder/SandcastleBuilderConsole.exe"
endif

# $(MERCURYAPI_CS_DIR)/Doc/MercuryAPI.chm $(MERCURYAPI_CS_DIR)/Doc/MercuryAPIHelp
.PHONY: target-mercuryapi-cs-help
MERCURYAPI_CS_TARGETS += target-mercuryapi-cs-help
target-mercuryapi-cs-help:\
 $(MERCURYAPI_CS_DIR)/Doc/Reader.shfb\
 target-MercuryAPI.sln
	TMP=/tmp; TEMP=/tmp; $(SHFB) $<
	mv Doc/MercuryAPIHelp/MercuryAPI.chm Doc/MercuryAPI.chm

autoparams_cs: $(wildcard $(MERCURYAPI_CS_DIR)/ThingMagic.Reader/*.cs)
	$(AUTOPARAMS) -o $(MERCURYAPI_CS_DIR)/ThingMagic.Reader/Reader.cs --template=$(MERCURYAPI_CS_DIR)/ThingMagic.Reader/Reader.cs $^

AUTOPARAM_CLEANS += $(MERCURYAPI_CS_DIR)/ThingMagic.Reader/Reader.cs

#
#
# Clean
#clean:
#	rm -fr MercuryAPI.chm MercuryAPIHelp
#	$(MSBUILD) /t:Clean /p:Configuration=Release MercuryAPI.sln
#	rm -fr MercuryAPI.dll MercuryAPI.xml MercuryAPICE.dll

########################################
.PHONY: mapi_cs_synctowin mapi_cs_winmakedsp mapi_cs_syncfromwin mapi_cs_winclean 
.PHONY: mapi_cs_syncfilelist.txt

mapi_cs_winbuild: mapi_cs_synctowin mapi_cs_winmakeAPI mapi_cs_syncfromwin 

# Try to rsync only necessary files for DSP winbuild
mapi_cs_syncfilelist.txt:
	cp /dev/null $@
	# Get top-level sub-makefiles
	ls ${TM_LEVEL}/*.mk >>$@
	# Get subdirectories
	#  but omit things that are obviously not necessary: e.g., wrong version of tools
	#  factory bundle, lint (handled by winmakelint target), linux distribution
	for dirname in modules/mercuryapi; do \
	  find ${TM_LEVEL}/$${dirname} \( -name .svn \) -prune -o \( -type f -print \) \
	  |fgrep -v .svn/ |fgrep -v CVS \
	  |fgrep -v ccsCgtoolsV3_2_2 \
	  |fgrep -v factory |fgrep -v lint |fgrep -v linux |fgrep -v walmart |fgrep -v win32 \
	  |fgrep -v Release_ | fgrep -v MercuryOSLinux \
	  |fgrep -v /tmfw- |grep -v '\.tmfw$$' \
	  |grep -v '\.d$$' |grep -v '\.o$$' \
	  |grep -v '\.m5f$$' |grep -v '\.src$$' \
	  |grep -v '/java/' \
	  >>$@ \
	;done
	# Fix syntax for rsync by stripping dir prefix (rsync wants relative paths)
	cat $@ |sed -e 's|^${TM_LEVEL}/||' |sort -u >$@.tmp && mv $@.tmp $@
CLEANS += mapi_cs_syncfilelist.txt

mapi_cs_synctowin: mapi_cs_syncfilelist.txt autoparams_cs
	ssh $(WINSRV) mkdir -p $(WINDIR)/tm
	${RSYNC} --files-from=$< $(TM_LEVEL)/ $(WINSRV):$(WINDIR)/tm

mapi_cs_winmakeAPI:
	ssh $(WINSRV) 'cd $(WINDIR)/tm/modules/mercuryapi/cs; make'

mapi_cs_syncfromwin:
	$(RSYNC) '$(WINSRV):$(WINDIR)/tm/modules/mercuryapi/cs/*' $(MERCURYAPI_CS_DIR)
	# Sometimes, MercuryAPI.dll comes back uncapitalized.  Fix it.
	# TODO: Figure out why mercuryapi.dll becomes uncapitalized
	if [ -e $(MERCURYAPI_CS_DIR)/mercuryapi.dll ]; then\
	  echo "WARNING: cs/mercuryapi.dll is uncapitalized.  Changing to MercuryAPI.dll";\
	  mv $(MERCURYAPI_CS_DIR)/mercuryapi.dll $(MERCURYAPI_CS_DIR)/MercuryAPI.dll;\
	fi

mapi_cs_winclean:
	$(RM) $(MERCURYAPI_CS_DIR)/MercuryAPI.dll
	$(RM) $(MERCURYAPI_CS_DIR)/MercuryAPICE.dll
	$(RM) $(MERCURYAPI_CS_DIR)/MercuryAPI.xml
	$(RM) $(MERCURYAPI_CS_DIR)/Doc/MercuryAPI.chm
	$(RM) -r $(MERCURYAPI_CS_DIR)/Doc/MercuryAPIHelp
	$(RM) -r $(MERCURYAPI_CS_DIR)/Release
	$(RM) -r $(MERCURYAPI_CS_DIR)/bin
	$(RM) -r $(MERCURYAPI_CS_DIR)/obj
	$(RM) -r $(MERCURYAPI_CS_DIR)/Samples/Demo/DemoCE/bin
	$(RM) -r $(MERCURYAPI_CS_DIR)/Samples/DemoCE/obj
	$(RM) -r $(MERCURYAPI_CS_DIR)/Samples/TestM5eCommands/bin
	$(RM) -r $(MERCURYAPI_CS_DIR)/Samples/TestM5eCommands/obj
	$(RM) -r $(MERCURYAPI_CS_DIR)/Samples/RFIDSearchLight/RFIDSearchLightMultiCab/ManualEdit/RFIDSearchLightInstaller.CAB
	$(RM) -r $(MERCURYAPI_CS_DIR)/Samples/RFIDSearchLight/RFIDSearchLightCab/ManualEdit/RFIDSearchLight.CAB
	$(RM) -r $(MERCURYAPI_CS_DIR)/ThingMagic.Reader/bin
	$(RM) -r $(MERCURYAPI_CS_DIR)/ThingMagic.Reader/obj
	$(RM) -r $(MERCURYAPI_CS_DIR)/ThingMagic.ReaderCE/bin
	$(RM) -r $(MERCURYAPI_CS_DIR)/ThingMagic.ReaderCE/obj
#	ssh $(WINSRV) 'cd $(WINDIR)/tm/modules/mercuryapi/cs; make clean'
	ssh $(WINSRV) 'rm -fr $(WINDIR)'

# Change the build number in the version string
# The version string must look like "1.2.3.0"
# The numbers will be replaced with the variables PRODUCTNUMBER.MAJORNUMBER.MINORNUMBER.BUILDNUMBER
PRODUCTNUMBER?=0
mapi_cs_set_product_number:
	$(RSED) -i 's/^(\[assembly: Assembly.*Version\(\")([0-9]+)\.([0-9]+)\.([0-9]+)\.([0-9]+)(\")/\1$(PRODUCTNUMBER).\3.\4.\5\6/' $^
MAJORNUMBER?=0
mapi_cs_set_major_number:
	@$(RSED) -i 's/^(\[assembly: Assembly.*Version\(\")([0-9]+)\.([0-9]+)\.([0-9]+)\.([0-9]+)(\")/\1\2.$(MAJORNUMBER).\4.\5\6/' $^
MINORNUMBER?=0
mapi_cs_set_minor_number:
	@$(RSED) -i 's/^(\[assembly: Assembly.*Version\(\")([0-9]+)\.([0-9]+)\.([0-9]+)\.([0-9]+)(\")/\1\2.\3.$(MINORNUMBER).\5\6/' $^
BUILDNUMBER?=0
mapi_cs_set_build_number:
	@$(RSED) -i 's/^(\[assembly: Assembly.*Version\(\")([0-9]+)\.([0-9]+)\.([0-9]+)\.([0-9]+)(\")/\1\2.\3.\4.$(BUILDNUMBER)\6/' $^

# Declare the version number rules for all version files
# Usage: $(eval $(call AddVersionFile,$(MERCURYAPI_CS_DIR)/ThingMagic.Reader/Properties/AssemblyInfo.cs $(MERCURYAPI_CS_DIR)/ThingMagic.ReaderCE/Properties/AssemblyInfo.cs <etc...>))
define AddVersionFile
mapi_cs_set_product_number: $(1)
mapi_cs_set_major_number: $(1)
mapi_cs_set_minor_number: $(1)
mapi_cs_set_build_number: $(1)
endef

$(eval $(call AddVersionFile,$(MERCURYAPI_CS_DIR)/ThingMagic.Reader/Properties/AssemblyInfo.cs))
$(eval $(call AddVersionFile,$(MERCURYAPI_CS_DIR)/ThingMagic.ReaderCE/Properties/AssemblyInfo.cs))
$(eval $(call AddVersionFile,$(MERCURYAPI_CS_DIR)/Samples/RFIDSearchLight/RFIDSearchLight/Properties/AssemblyInfo.cs))
mapi_cs_set_ura2_build_number: $(MERCURYAPI_CS_DIR)/Samples/UniversalReaderAssistant2.0/UniversalReaderAssistant2.0/Properties/AssemblyInfo.cs
	@$(RSED) -i 's/^(\[assembly: Assembly.*Version\(\")([0-9]+)\.([0-9]+)\.([0-9]+)\.([0-9]+)(\")/\1\2.\3.$(BUILDNUMBER).$(BUILDNUMBER)\6/' $^

#endif
