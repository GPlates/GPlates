#! /bin/bash
# $Id$

DIRS=". ./src ../src ../../src"

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

echo -n "Looking for source code... "
file_list=`find . -name *.cc -or -name *.h`
file_count=`echo $file_list | wc -w`
echo "ok, found $file_count files"

echo
echo
cat $file_list | grep "Author: [a-z]\+" | cut -d$ -f2 | cut -d' ' -f2 \
	| sort | uniq -c | sort -n

echo "     ^  ^------- Programmer"
echo "     \`-------------- Number of files they were last to edit"
echo

