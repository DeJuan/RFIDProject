#!/bin/sh
#
# Wrapper script to run demo program with the right classpath and library path.
dir=$(dirname $0)
java -Djava.library.path=$dir -cp ${dir}/mercuryapi.jar::${dir}/ltkjava-1.0.0.6.jar:${dir}/demo.jar samples.demo "$@"
