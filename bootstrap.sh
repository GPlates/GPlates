#! /bin/sh

set -x
aclocal -I config \
	&& autoheader \
	&& automake --gnu --add-missing --copy \
	&& autoconf
