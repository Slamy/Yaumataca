set -e
SCRIPT=$(realpath "$0")
SCRIPTPATH=$(dirname "$SCRIPT")
cd $SCRIPTPATH/..

cmake -S test -B build_unittest 
cmake --build build_unittest -j
cd build_unittest
./unittest

echo " --- Finished ---"
