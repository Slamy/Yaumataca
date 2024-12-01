set -e
SCRIPT=$(realpath "$0")
SCRIPTPATH=$(dirname "$SCRIPT")
cd $SCRIPTPATH/..

mkdir -p release
rm -f release/*.uf2 release/*.prg

# Create special build for 2 button mouse swap
rm -rf build_release
cmake -S . -B build_release -DCONFIG_SWAP2BUTTON=True
cmake --build build_release -j
cp build_release/yaumataca.uf2 release/yaumataca_2buttonswap.uf2

# Create special build with disabled Amiga WheelBusMouse
rm -rf build_release
cmake -S . -B build_release -DCONFIG_DISABLE_AMIGA_WHEELBUSMOUSE=True
cmake --build build_release -j
cp build_release/yaumataca.uf2 release/yaumataca_no_wheelbusmouse.uf2

# Create standard build
rm -rf build_release
cmake -S . -B build_release
cmake --build build_release -j
cp build_release/yaumataca.uf2 release/

# Build the C64 calibration tool
cl65 -O -t c64 c64tool/calibration.c -o release/calib.prg

# Package everything into a flat zip
rm -f *.zip
zip -j yaumataca_$(git describe --tags --dirty).zip release/*

echo " --- Finished ---"
