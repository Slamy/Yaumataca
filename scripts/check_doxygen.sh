set -e
SCRIPT=$(realpath "$0")
SCRIPTPATH=$(dirname "$SCRIPT")
cd $SCRIPTPATH/..

doxygen > /dev/null || echo "Failed!"

echo " --- Finished ---"
