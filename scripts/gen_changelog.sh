#!/bin/sh

DIRS=". .."
AUX_PATH=scripts/aux
AUX_FILES="boilerplate cvs2cl.pl usermap"
DEST="ChangeLog-CVS"

# Find files
for dir in $DIRS; do
	if test -f $dir/configure.in; then
		cd $dir
	fi
done
if ! test -f ./configure.in; then
	echo "In a bad directory (can't find configure.in in \"$DIRS\")"
	exit 1
fi
for file in $AUX_FILES; do
	if ! test -f $AUX_PATH/$file; then
		echo "Can't see \"$AUX_PATH/$file\""
		exit 1
	fi
done

###################################################################
OPTIONS="--revisions --day-of-week --tags"

# --utc                    Show times in GMT/UTC instead of local time
# -S, --separate-header    Blank line between each header and log message
# --no-wrap                Don't auto-wrap log message (recommend -S also)


rm -f $DEST
$AUX_PATH/cvs2cl.pl --stdout -U $AUX_PATH/usermap -I $DEST $OPTIONS \
	-g "-z3" --header $AUX_PATH/boilerplate \
	| sed "s/\.  /\. /g" > $DEST
chmod 0644 $DEST
echo "-------------"
echo "Committing..."
cvs -z3 commit -m "Updated $DEST." $DEST
echo "-------------"
