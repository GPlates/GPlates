#! /bin/bash
# $Id$

DIRS=". ./src ../src ../../src"
OUTPUT="valgrind.log"
SUPPS="../scripts/aux/gplates.supp"

# Get to source directory
for dir in $DIRS; do
	if test -f $dir/gplates_main.cc; then
		cd $dir
	fi
done
if ! test -f gplates_main.cc; then
	echo "In a bad directory (can't find gplates_main.cc in \"$DIRS\")"
	exit 1
fi
if ! test -x gplates; then
	echo "It seems that GPlates isn't yet compiled!"
	exit 1
fi

echo "Running with valgrind, output to \"$OUTPUT\"..."
valgrind --tool=memcheck -v --leak-check=yes --num-callers=8 \
	--suppressions=$SUPPS \
	./gplates 2> $OUTPUT
