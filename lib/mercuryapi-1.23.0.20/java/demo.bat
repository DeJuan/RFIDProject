@echo off
REM
REM wrapper batch file to run demo program with the right classpath and
REM library path.

set dir="%~dp0%\"
java -Djava.library.path=%dir% -cp %dir%/mercuryapi.jar;%dir%/ltkjava-1.0.0.6.jar;%dir%/demo.jar samples.demo %*
