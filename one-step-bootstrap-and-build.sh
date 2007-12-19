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

# all parameters passed to ./configure in build-everything.sh

set -x
sh bootstrap.sh
sh build-everything.sh "$@"
( cd src/qt-widgets && make ViewportWindowUi.h InformationDialogUi.h )
( cd src/qt-widgets && make SpecifyFixedPlateDialogUi.h )
( cd src/qt-widgets && make AnimateDialogUi.h QueryFeaturePropertiesDialogUi.h )
( cd src/qt-widgets && make AboutDialogUi.h ReconstructionViewWidgetUi.h )
( cd src/qt-widgets && make ReadErrorAccumulationDialogUi.h )
( cd src/qt-widgets && make ManageFeatureCollectionsDialogUi.h ManageFeatureCollectionsActionWidgetUi.h )
( cd src/qt-widgets && make SetCameraViewpointDialogUi.h )
make
