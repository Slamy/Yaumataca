set -e

# Compile first
cl65 -O -t c64 calibration.c -o calib.prg
ls -lh calib.prg
