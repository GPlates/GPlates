#! /bin/sh

# This script is not intended for use by users of official GPlates releases.
#
# The purpose of this script is to enable one-step bootstrap-and-building of
# GPlates, after a fresh checkout from the GPlates source-code repositiory,
# until the build problem described by ticket:1 ("Qt Ui-file build problem")
# is fixed.
#
# After executing this script, GPlates should be completely built; it should
# not be necessary to execute any other compilation scripts or commands.

set -x
sh bootstrap.sh
sh build-everything.sh
( cd src/gui-qt && make ViewportWindowUi.h InformationDialogUi.h )
( cd src/gui-qt && make ReconstructToTimeDialogUi.h SpecifyFixedPlateDialogUi.h )
( cd src/gui-qt && make AnimateDialogUi.h )
make
