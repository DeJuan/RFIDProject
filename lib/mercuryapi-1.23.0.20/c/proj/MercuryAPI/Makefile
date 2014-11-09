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
VCBUILD8 ?= echo VCBUILD
else
MSBUILD2 ?= "/cygdrive/c/WINDOWS/Microsoft.NET/Framework/v2.0.50727/MSBuild.exe"
MSBUILD3 ?= "/cygdrive/c/WINDOWS/Microsoft.NET/Framework/v3.5/MSBuild.exe"
MSBUILD4 ?= "/cygdrive/c/WINDOWS/Microsoft.NET/Framework/v4.0.30319/MSBuild.exe"
VCBUILD8 ?= "/cygdrive/c/Program Files/Microsoft Visual Studio 8/vc/vcpackages/vcbuild.exe"
endif


# Add path to VCBuild
VCBUILDPATH ?= "/cygdrive/c/Program Files/Microsoft Visual Studio 8/VC/vcpackages"

default: Release/MercuryAPI.lib

Release/MercuryAPI.lib: MercuryAPI.sln MercuryAPI.vcproj
	$(MSBUILD2) /t:Rebuild /p:Configuration=Release MercuryAPI.sln
clean:
	rm -fr Release/MercuryAPI.dll



# Support for C projects is kind of screwed up if you haven't
# installed a full-blown Visual Studio.
#
# MSBuild can't find VCBuild: error MSB3428: Could not load the Visual
# C++ component "VCBuild.exe". To fix this, 1) install the .NET
# Framework 2.0 SDK, 2) install Microsoft Visual Studio 2005 or 3) add
# the location of the component to the system path if it is installed
# elsewhere.
#
# http://www.netframeworkdev.com/msbuild/msbuild-cannot-find-vcbuild-other-sdk-madness-24439.shtml
# confirms that this situation sucks.
#
# Running VCBuild directly (using the command printed out by MSBuild
# on a working, Visual Studio-enabled machine) works better, but still
# runs into: fatal error C1902: Program database manager mismatch;
# plese check your installation
#
# http://social.msdn.microsoft.com/forums/en-US/vcgeneral/thread/ce48212e-9731-4539-b6fd-8cc92195c69f/
# says this is a bug in the compiler when run from a non-user account
# and points to a hotfix for Visual Studio.
#
# All this works fine from Harry's laptop (which has VS2005
# installed), so we should just get Visual Studio installed on the
# winbuild server.

# "/cygdrive/c/WINDOWS/Microsoft.NET/Framework/v2.0.50727/MSBuild.exe" /t:Build /p:Configuration=Release MercuryAPI.sln /p:VCToolPath="/cygdrive/c/Program Files/Microsoft Visual Studio 8/VC/vcpackages"

# C:\Program Files\Microsoft Visual Studio 8\Common7\IDE\..\..\vc\vcpackages\vcbuild.exe /rebuild c:\svn\TEST\c\proj\MercuryAPI\MercuryAPI.vcproj "Release|Win32" 
