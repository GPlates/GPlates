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
( cd src/qt-widgets && make AnimateDialogUi.h QueryFeaturePropertiesWidgetUi.h )
( cd src/qt-widgets && make AboutDialogUi.h ReconstructionViewWidgetUi.h )
( cd src/qt-widgets && make ReadErrorAccumulationDialogUi.h )
( cd src/qt-widgets && make ManageFeatureCollectionsDialogUi.h )
( cd src/qt-widgets && make ManageFeatureCollectionsActionWidgetUi.h )
( cd src/qt-widgets && make SetCameraViewpointDialogUi.h EulerPoleDialogUi.h )
( cd src/qt-widgets && make EditFeaturePropertiesWidgetUi.h )
( cd src/qt-widgets && make EditTimePeriodWidgetUi.h EditOldPlatesHeaderWidgetUi.h )
( cd src/qt-widgets && make AddPropertyDialogUi.h )
( cd src/qt-widgets && make CreateFeatureDialogUi.h DigitisationWidgetUi.h )
( cd src/qt-widgets && make PlateClosureWidgetUi.h )
( cd src/qt-widgets && make EditStringWidgetUi.h EditEnumerationWidgetUi.h )
( cd src/qt-widgets && make EditTimeInstantWidgetUi.h EditPlateIdWidgetUi.h )
( cd src/qt-widgets && make EditGeometryWidgetUi.h EditGeometryActionWidgetUi.h )
( cd src/qt-widgets && make EditIntegerWidgetUi.h EditDoubleWidgetUi.h )
( cd src/qt-widgets && make EditBooleanWidgetUi.h EditPolarityChronIdWidgetUi.h )
( cd src/qt-widgets && make EditAngleWidgetUi.h )
( cd src/qt-widgets && make ExportCoordinatesDialogUi.h )
( cd src/qt-widgets && make FeatureSummaryWidgetUi.h ReconstructionPoleWidgetUi.h )
( cd src/qt-widgets && make FeaturePropertiesDialogUi.h ViewFeatureGeometriesWidgetUi.h )
( cd src/qt-widgets && make ShapefileAttributeMapperDialogUi.h )
( cd src/qt-widgets && make ShapefileAttributeRemapperDialogUi.h )
( cd src/qt-widgets && make ShapefileAttributeWidgetUi.h )
( cd src/qt-widgets && make TaskPanelUi.h ApplyReconstructionPoleAdjustmentDialogUi.h )
make
