#! /bin/sh

# This script is not intended for use by users of official GPlates releases.
#
# The purpose of this script is to "bootstrap" the build-system after a fresh
# checkout from the GPlates source-code repository, auto-generating a variety
# of configuration and compilation scripts.
#
# Users of official GPlates source-code releases should not need to run this
# script, since the build-system in an official source-code release should
# already be bootstrapped.  In fact, running this script could render the
# build-system inoperable, if the correct versions of the correct development
# tools are not installed.
#
# This is the first convenience script which should be run after a fresh
# checkout.  It should be followed by the script "build-everything.sh".

set -x
aclocal -I config \
	&& libtoolize --force --copy \
	&& autoheader \
	&& automake --gnu --add-missing --copy \
	&& autoconf
