#!/bin/bash
set -e

MUSASHI=/Volumes/TB4-4Tb/Projects/mister/musashi
SST=$MUSASHI/test/singlestep/sst_runner
SSTDIR=$MUSASHI/test/singlestep/unified

cd test/singlestep
gcc -O2 -o sst_runner sst_runner.c sst_loader.c ../../m68kcpu.c ../../m68kops.c ../../softfloat/softfloat.c -I../../ -I../../softfloat
cd ../../

echo "========================================"
echo "SST TESTS (SingleStepTests) - Both Sources"
echo "========================================"
"$SST" "$SSTDIR"/*/*.sst 2>&1 | grep -v "^DEBUG"
