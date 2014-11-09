# TODO: Clean out unused targets (cloned from toollink Makefile)

ifdef PRODUCT
	PRODUCT_NAME := $(PRODUCT)
else
	PRODUCT_NAME := RFIDSearchLight
endif

ifndef WINSRV
  WINSRV := tm-winbuild
endif
  WINDIR ?= src$(PRODUCT_NAME)

CWD := $(shell pwd)
SRCDIR := $(CWD)
SWTREE_DIR ?= ../../../..
TM_LEVEL := $(CWD)/../../..
TOP_LEVEL := $(CWD)/../../..
RSYNC := cd ${SRCDIR}; rsync -e ssh -IWtrclpv --blocking-io --exclude '*.d' --exclude '*.pp*'

CABWIZ ?= '/cygdrive/c/Program Files/Windows Mobile 6 SDK/Tools/CabWiz/Cabwiz.exe'
MSBUILD2 ?= 'C:\WINDOWS\Microsoft.NET\Framework\v2.0.50727\MSBuild.exe'
MSBUILD3 ?= 'C:\WINDOWS\Microsoft.NET\Framework\v3.5\MSBuild.exe'
MSBUILD ?= $(MSBUILD3)
SLNFILE ?= 'RFIDSearchLight.sln'
# Strangely, cygpath-ization is not required when the program comes first in the command line.
# Things go wonky only in commands like "cd $(FOODIR); $(7Z)"
7Z ?= "$(shell cygpath 'C:\Program Files\7-Zip\7z.exe')"
ZIP ?= $(7Z) a -tzip
DEVENV ?= 'C:\Program Files\Microsoft Visual Studio 8\Common7\IDE\devenv.exe'

DATESTRING ?= $(shell date +%Y%m%d)
RELDIR ?= //file/engineering/releases/daily/rfidsearchlight

COPY ?= cp -p

default: synctowin winmake syncfromwin
#default: increment_minor_number build dist


MERCURYAPI_RELEASE ?= M6_SPRUCE-1.11.2.82
MERCURYAPI_ZIP ?= mercuryapi-M6_SPRUCE-1.11.2.82.zip
MercuryAPICE.dll LLRP.dll: $(SRCDIR)/$(MERCURYAPI_ZIP)
	unzip -j -o $< mercuryapi-$(MERCURYAPI_RELEASE)/cs/$@
$(SRCDIR)/$(MERCURYAPI_ZIP):
	wget -O $@ http://file/engineering/releases/mercuryapi/$(MERCURYAPI_ZIP)

prereqs: MercuryAPICE.dll LLRP.dll



.PHONY: build
build: devenvrebuild

.PHONY: cabfile slnbuild
.PHONY: msbuild msrebuild msclean
.PHONY: msbuildDemo msrebuildDemo mscleanDemo
.PHONY: msbuildCE msrebuildCE mscleanCE

msbuild: slnbuild cabfile

CABFILE = RFIDSearchLightCab/Release/RFIDSearchLight.CAB
cabfile:
	# CabWiz has no dependency checking, so always build clean
	rm -f $(CABFILE)
	# CabWiz workarounds
	# (NOTE: error reporting is very poor, tends to just say,
	#  Error: CAB file "..." could not be created):
	#   * Sends error output to UTF-16 file instead of stderr
	#   * Needs TMP dir, which is not set by "ssh winbuild"
	#   * Wants absolute path to input filename
	TMP='C:\temp' $(CABWIZ) 'C:\cygwin$(subst /,\,$(CWD))\RFIDSearchLightCab\Release\RFIDSearchLight.inf' /err CabWiz.log || (cat RFIDSearchLightCab/Release/CabWiz.log |tr -d '\000'; exit 1)

slnbuild:
	$(MSBUILD) $(SLNFILE) /t:Build /p:Configuration=Release

msbuildDemo:
	$(MSBUILD) $(SLNFILE) /t:ThingMagic_ToolLinkDemo /p:Configuration=Release
msbuildCE: msbuildSplashLoaderCE
	$(MSBUILD) $(SLNFILE) /t:ThingMagic_ToolLinkCE /p:Configuration=Release
msbuildSplashLoaderCE:
	$(MSBUILD) $(SLNFILE) /t:SplashLoaderCE /p:Configuration=Release
#
msrebuild:
	$(MSBUILD) $(SLNFILE) /t:Rebuild /p:Configuration=Release
msrebuildDemo:
	$(MSBUILD) $(SLNFILE /t:ThingMagic_ToolLinkDemo:Rebuild /p:Configuration=Release
msrebuildCE:
	$(MSBUILD) $(SLNFILE) /t:ThingMagic_ToolLinkCE:Rebuild /p:Configuration=Release
#
msclean:
	$(MSBUILD) $(SLNFILE) /t:Clean /p:Configuration=Release
mscleanDemo:
	$(MSBUILD) $(SLNFILE) /t:ThingMagic_ToolLinkDemo:Clean /p:Configuration=Release
mscleanCE:
	$(MSBUILD) $(SLNFILE) /t:ThingMagic_ToolLinkCE:Clean /p:Configuration=Release

.PHONY: clean
clean:
	$(MSBUILD) $(SLNFILE) /t:Clean
	rm -fr $(CLEANFILES)

.PHONY: dist
dist: dist-ce dist-desktop

TOOLLINKDIR ?= ThingMagic.ToolLink
CEDIR ?= $(TOOLLINKDIR)/ThingMagic.ToolLinkCE/bin/Release
CETLEXEFILE ?= ThingMagic.ToolLinkCE.exe
#CEAUXFILES ?= ClickableTextBox.dll gpioint.dll LzmaDecompressCE.dll
CEAUXFILES ?= *.dll
CEFILES ?= $(CETLEXEFILE) $(CEAUXFILES)
CEDESTBASE ?= $(RELDIR)/ToolLinkReleaseBuild$(DATESTRING)

# Files need to be shuffled around to make 2-stage loader work.
# Assemble new files here before packaging them.
CESTAGEDIR ?= CESTAGE

# Stage files from VS build directories to our own staging directory
stage-ce:
	rm -fr $(CESTAGEDIR)
	mkdir $(CESTAGEDIR)
	$(COPY) $(CEDIR)/$(CETLEXEFILE) $(CESTAGEDIR)/MainApp.exe
	$(COPY) $(patsubst %,$(CEDIR)/%,$(CEAUXFILES)) $(CESTAGEDIR)/
	$(COPY) $(TOOLLINKDIR)/SplashLoaderCE/bin/Release/SplashLoaderCE.exe $(CESTAGEDIR)/$(CETLEXEFILE)

builddist-ce: stage-ce
	cd $(CESTAGEDIR); rm -f ../latest.zip
	cd $(CESTAGEDIR); zip -9r ../latest.zip *
	# Magneti-Marelli's mail server chops .exe files out of .zips.  Use 7z to subvert it.
	cd $(CESTAGEDIR); rm -f ../latest.7z
	cd $(CESTAGEDIR); $(7Z) a ../latest.7z *

# Build CE installer
CEINSTDIR ?= CEINSTALL
buildinst-ce: builddist-ce
	rm -fr $(CEINSTDIR)
	mkdir $(CEINSTDIR)
	cd $(CEINSTDIR); $(7Z) x ../TLCEInstall-Template.7z
	cd $(CEINSTDIR)/ToolLinkAddOn/User/TLCE; rm *; $(7Z) x ../../../../latest.7z
	rm -f TLCEInstall-latest.7z
	cd $(CEINSTDIR); $(7Z) a ../TLCEInstall-latest.7z

dist-ce: builddist-ce
	cd $(CESTADEDIR); cp ../latest.7z $(CEDESTBASE).7z

# Copy CE files to flashdisk for transfer to device
FLASHDIR ?= D:/Release
dist-flashdisk: stage-ce
	$(COPY) $(CESTAGEDIR)/* $(FLASHDIR)

dist-desktop:
	cd ThingMagic.ToolLink/ToolLinkDesktop/Release; zip -9r $(RELDIR)/ToolLinkDesktop$(DATESTRING).zip *


.PHONY: dist-readerdemo
dist-readerdemo: dist-readerdemoxp dist-readerdemoce
dist-readerdemoxp:
	$(COPY) ThingMagic.ToolLink/ReaderDemo/bin/Release/ReaderDemo.exe $(RELDIR)/ReaderDemo$(DATESTRING).exe
dist-readerdemoce:
	$(COPY) ThingMagic.ToolLink/ReaderDemoCE/bin/Release/ReaderDemoCE.exe $(RELDIR)/ReaderDemoCE$(DATESTRING).exe

.PHONY: tm_product
tm_product: toollink_phase2_wincleanbuilddist

.PHONY: release_product
release_product:
	cp $(CESTAGEDIR)/../TLCEInstall-latest.7z $(FILEDROP)/$(PRODUCT_NAME)-$(BRANCHNAME)-`$(GET_VERSION_NUMBER)`.7z
#	chmod 644 $(FILEDROP)/$(PRODUCT_NAME)-$(BRANCHNAME)-`$(GET_VERSION_NUMBER)`.7z


# Version Numbers
VERSIONFILES =\
 ThingMagic.ToolLink/ThingMagic.ToolLinkDemo/Properties/AssemblyInfo.cs\
 ThingMagic.ToolLink/ThingMagic.ToolLinkCE/Properties/AssemblyInfo.cs\

GET_VERSION_NUMBER :=sed -ne 's|^.*AssemblyVersion("\(.*\)").*$$|\1|p' $(VERSIONFILES) | sort -u
#GET_VERSION_NUMBER :=cat current.version
current.version: $(VERSIONFILES)
	@$(GET_VERSION_NUMBER) > $@
#	@sed -ne 's|^.*AssemblyVersion("\(.*\)").*$$|\1|p' $^ |sort -u >$@
current.product.number: current.version
	@cat $< |awk -F . '{print $$1}' >$@
current.major.number: current.version
	@cat $< |awk -F . '{print $$2}' >$@
current.minor.number: current.version
	@cat $< |awk -F . '{print $$3}' >$@
current.build.number: current.version
	@cat $< |awk -F . '{print $$4}' >$@

.PHONY : get_version get_product_number get_major_number get_minor_number get_build_number
get_version: current.version
	@cat $<
get_product_number: current.product.number
	@cat $<
get_major_number: current.major.number
	@cat $<
get_minor_number: current.minor.number
	@cat $<
get_build_number: current.build.number
	@cat $<

CLEANFILES += current.version current.product.number current.major.number current.minor.number current.build.number

increment_minor_number: current.minor.number
	export MINORNUMBER=$$(expr $$(cat $<) + 1); make set_minor_number

PRODUCTNUMBER ?= 0
set_product_number:
	for fn in $(VERSIONFILES); do\
	  $(RSED) '/AssemblyVersion/s|([0-9]+).([0-9]+).([0-9]+).([0-9]+)|$(PRODUCTNUMBER).\2.\3.\4|' <$$fn >$$fn.new && mv $$fn.new $$fn && grep AssemblyVersion $$fn;\
	done

MAJORNUMBER ?= 0
set_major_number:
	for fn in $(VERSIONFILES); do\
	  $(RSED) '/AssemblyVersion/s|([0-9]+).([0-9]+).([0-9]+).([0-9]+)|\1.$(MAJORNUMBER).\3.\4|' <$$fn >$$fn.new && mv $$fn.new $$fn && grep AssemblyVersion $$fn;\
	done

MINORNUMBER ?= 0
set_minor_number:
	for fn in $(VERSIONFILES); do\
	  $(RSED) '/AssemblyVersion/s|([0-9]+).([0-9]+).([0-9]+).([0-9]+)|\1.\2.$(MINORNUMBER).\4|' <$$fn >$$fn.new && mv $$fn.new $$fn && grep AssemblyVersion $$fn;\
	done

BUILD_FROM_FILE :=awk '{printf "%d", strtonum($$1)}' $(BUILD_NUMBER_FILE)
BUILDNUMBER ?= 0
set_build_number:
	for fn in $(VERSIONFILES); do\
	  $(RSED) '/AssemblyVersion/s|([0-9]+).([0-9]+).([0-9]+).([0-9]+)|\1.\2.\3.$(BUILDNUMBER)|' <$$fn >$$fn.new && mv $$fn.new $$fn && grep AssemblyVersion $$fn;\
	done
_set_build_number_from_file:
	@$(MAKE) -s BUILDNUMBER=`$(BUILD_FROM_FILE)` set_build_number
_inc_build_number_in_file:
	@(rm $(BUILD_NUMBER_FILE) && awk '{printf "0x%X", strtonum($$1)+1}' > $(BUILD_NUMBER_FILE)) < $(BUILD_NUMBER_FILE)
	@$(GET_BUILD_NUMBER)

# Use year.monthday as minor.build numbers
set_auto_version_from_date:
	export MAJORNUMBER=0; export MINORNUMBER=`date +%Y`; export BUILDNUMBER=`date +%m%d`; make set_major_number && make set_minor_number && make set_build_number

tag_daily_build:
	svn cp -m"Daily Build tag for Tool Link version `$(GET_VERSION_NUMBER)`" $(SWTREE_DIR) http://svn.thingmagic.com/repos/swtree/tags/daily/toollink/toollink_`$(GET_VERSION_NUMBER)`

daily_build: _set_build_number_from_file tm_product release_product _inc_build_number_in_file tag_daily_build
	@echo Build of Tool Link version `$(GET_VERSION_NUMBER)` complete

########################################
.PHONY: toollink_phase2_synctowin toollink_phase2_winmake toollink_phase2_syncfromwin toollink_phase2_winclean 
.PHONY: toollink_phase2_windist toollink_phase2_wincleanbuild toollink_phase2_wincleanbuilddist
.PHONY: toollink_phase2_syncfilelist.txt

toollink_phase2_wincleanbuilddist: toollink_phase2_synctowin toollink_phase2_winclean toollink_phase2_winmake toollink_phase2_windist toollink_phase2_syncfromwin 
toollink_phase2_wincleanbuild: toollink_phase2_synctowin toollink_phase2_winclean toollink_phase2_winmake toollink_phase2_syncfromwin 
toollink_phase2_winbuild: toollink_phase2_synctowin toollink_phase2_winmake toollink_phase2_syncfromwin 

toollink_phase2_syncfilelist.txt:
	cp /dev/null $@
	# Get top-level sub-makefiles
	ls ${TM_LEVEL}/*.mk >>$@
	# Get subdirectories
	#  but omit things that are obviously not necessary: e.g., wrong version of tools
	#  factory bundle, lint (handled by winmakelint target), linux distribution
	for dirname in product/toollink/phase2; do \
	  find ${TM_LEVEL}/$${dirname} \( -name .svn \) -prune -o \( -type f -print \) \
	  |fgrep -v .svn/ |fgrep -v CVS \
	  |fgrep -v ccsCgtoolsV3_2_2 \
	  |grep -v '\.d$$' |grep -v '\.o$$' \
	  >>$@ \
	;done
	# HACK: FaultCodes.h and TagIDBuf.h still live in MercuryE directory
	#   To be refactored to platform-independent location, but not yet.
	ls ${TM_LEVEL}/product/MercuryE/include/FaultCodes.h >>$@
	ls ${TM_LEVEL}/product/MercuryE/src/TagIdBuf.h >>$@
	# Fix syntax for rsync by stripping dir prefix (rsync wants relative paths)
	cat $@ |sed -e 's|^${TM_LEVEL}/||' |sort -u >$@.tmp && mv $@.tmp $@

synctowin: prereqs
	ssh $(WINSRV) mkdir -p $(WINDIR)/searchlight
	${RSYNC} $(SRCDIR)/ $(WINSRV):$(WINDIR)/searchlight

winmake:
	ssh $(WINSRV) 'cd $(WINDIR)/searchlight; make msbuild'

windist:
	ssh $(WINSRV) 'cd $(WINDIR)/searchlight; PRODUCT_NAME=$(PRODUCT_NAME) BRANCHNAME=$(BRANCHNAME) make buildinst-ce'

syncfromwin:
	$(RSYNC) '$(WINSRV):$(WINDIR)/searchlight/' $(SRCDIR)/

winclean:
	ssh $(WINSRV) 'cd $(WINDIR)/tm/product/toollink/phase2; make msclean'

