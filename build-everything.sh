#! /bin/bash

# This script is not intended for use by users of official GPlates releases.
#
# The purpose of this script is to "build" the whole program from the source,
# using specific compiler settings to verify the correctness and portability
# of GPlates source-code.  This script is intended for GPlates developers.
#
# Non-developers should not run this script, since the stricter compiler
# settings provide no benefit to end-users, and may cause compilation to fail.
# Instead, end-users should run the command " ./configure && make ".
#
# This is the second convenience script which should be run after a fresh
# checkout, following the script "bootstrap.sh".

set -x
./configure --enable-dev "$@" && \
make
