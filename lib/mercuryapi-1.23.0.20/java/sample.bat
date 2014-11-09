@echo off
REM
REM wrapper batch file to run sample program with the right classpath and
REM library path.

set dir="%~dp0%\"
set class=%1%
set params=
SHIFT
SHIFT
:next
if "%0" == "" goto end
  set params=%params% %0
  SHIFT
  goto next
:end
java -Djava.library.path=%dir% -cp %dir%/mercuryapi.jar;%dir%/ltkjava-1.0.0.6.jar;%dir%/demo.jar samples.%class% %params%
