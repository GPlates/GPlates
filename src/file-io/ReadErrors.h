/* $Id$ */

/**
 * \file 
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007, 2008 The University of Sydney, Australia
 * Copyright (C) 2007, 2012 Geological Survey of Norway
 *
 * This file is part of GPlates.
 *
 * GPlates is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 *
 * GPlates is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef GPLATES_FILEIO_READERRORS_H
#define GPLATES_FILEIO_READERRORS_H

namespace GPlatesFileIO
{
	/**
	 * Please keep these entries in the same order as they appear on the GPlates
	 * Trac wiki page "ReadErrorMessages".
	 *
	 * A corresponding textual description will also need to be added to "ReadErrorMessages.cc".
	 */
	namespace ReadErrors
	{
		enum Description
		{
			// These are specific to PLATES rotation-format reading.
			/*
			 * These enumerators are derived from the enumeration
			 * 'PlatesRotationFormatReader::ReadErrors::Description' in the header file
			 * "PlatesRotationFormatReader.hh" in the ReconTreeViewer software:
			 *  http://sourceforge.net/projects/recontreeviewer/
			 */
			CommentMovingPlateIdAfterNonCommentSequence,
			ErrorReadingFixedPlateId,
			ErrorReadingGeoTime,
			ErrorReadingMovingPlateId,
			ErrorReadingPoleLatitude,
			ErrorReadingPoleLongitude,
			InvalidPoleLatitude,
			ErrorReadingRotationAngle,
			InvalidPoleLongitude,
			MovingPlateIdEqualsFixedPlateId,
			NoCommentFound,
			NoExclMarkToStartComment,
			SamePlateIdsButDuplicateGeoTime,
			SamePlateIdsButEarlierGeoTime,

			// The following are specific to PLATES line-format reading.
			InvalidPlatesRegionNumber,
			InvalidPlatesReferenceNumber,
			InvalidPlatesStringNumber, 
			InvalidPlatesGeographicDescription, 
			InvalidPlatesPlateIdNumber, 
			InvalidPlatesAgeOfAppearance, 
			InvalidPlatesAgeOfDisappearance, 
			InvalidPlatesDataTypeCode, 
			InvalidPlatesDataTypeCodeNumber, 
			InvalidPlatesDataTypeCodeNumberAdditional, 
			InvalidPlatesConjugatePlateIdNumber, 
			InvalidPlatesColourCode, 
			InvalidPlatesNumberOfPoints,
			UnknownPlatesDataTypeCode,
			MissingPlatesPolylinePoint,
			MissingPlatesHeaderSecondLine,
			InvalidPlatesPolylinePoint,
			InvalidPlatesPolylinePlotterCode,
			InvalidPlatesPolylineLatitude,
			InvalidPlatesPolylineLongitude,
			AdjacentSkipToPlotterCodes,
			AmbiguousPlatesIceShelfCode,
			MoreThanOneDistinctPoint,
			NoValidGeometriesInPlatesFeature,
			InvalidMultipointGeometry,

			// The following are specific to GPlates 8 hydrid PLATES line-format.
			MissingPlatepolygonBoundaryFeature,
			InvalidPlatepolygonBoundaryFeature,

			// The following apply to OGR-supported file format import.
			NoLayersFoundInFile,
			MultipleLayersInFile,
			ErrorReadingOgrLayer,
			NoFeaturesFoundInOgrFile,
			ErrorReadingOgrGeometry,
			TwoPointFiveDGeometryDetected,
			LessThanTwoPointsInLineString,
			InteriorRingsInShapefile,
			UnsupportedGeometryType,
			NoLatitudeShapeData,
			NoLongitudeShapeData,
			InvalidOgrLatitude,
			InvalidOgrLongitude,
			NoPlateIdFound,
			InvalidShapefilePlateIdNumber,
			UnrecognisedOgrFeatureType,
			InvalidShapefileAgeOfAppearance,
			InvalidShapefileAgeOfDisappearance,
			InvalidShapefileConjugatePlateIdNumber,
			InvalidOgrPoint,
			InvalidOgrMultiPoint,
			InvalidOgrPolyline,
			InvalidOgrPolygon,
			
			// The following relate to raster files in general.
			InsufficientMemoryToLoadRaster,
			ErrorGeneratingTexture,
			UnrecognisedRasterFileType,
			ErrorReadingRasterFile,
			ErrorReadingRasterBand,
			InvalidRegionInRaster,

			// The following relate to GDAL-readable raster files.
			ErrorInSystemLibraries,

			// The following relate to time-dependent raster file sets.
			NoRasterSetsFound,
			MultipleRasterSetsFound,

			// The following relate to importing 3D scalar field files.
			DepthLayerRasterIsNotNumerical,

			// The following apply to GPML import
			DuplicateProperty,
			NecessaryPropertyNotFound,
			UnknownValueType,
			BadOrMissingTargetForValueType,
			InvalidBoolean,
			InvalidDouble,
			InvalidGeoTime,
			InvalidInt,
			InvalidLatLonPoint,
			InvalidLong,
			InvalidPointsInPolyline,
			InsufficientDistinctPointsInPolyline,
			AntipodalAdjacentPointsInPolyline,
			InvalidPointsInPolygon,
			InvalidPolygonEndPoint,
			InsufficientPointsInPolygon,
			InsufficientDistinctPointsInPolygon,
			AntipodalAdjacentPointsInPolygon,
			InvalidEnumerationValue,
			InvalidString,
			InvalidUnsignedInt,
			InvalidUnsignedLong,
			MissingNamespaceAlias,
			NonUniqueStructuralElement,
			StructuralElementNotFound,
			UnexpectedStructuralElement,
			UnexpectedPropertyStructuralElement,
			PropertyNameNotRecognisedInFeatureType,
			TimeDependentPropertyStructuralElementNotFound,
			TimeDependentPropertyStructuralElementFound,
			IncorrectTimeDependentPropertyStructuralElementFound,
			TooManyChildrenInElement,
			UnexpectedEmptyString,
			UnrecognisedChildFound,
			DuplicateIdentityProperty,
			DuplicateRevisionProperty,
			UnrecognisedFeatureCollectionElement,
			UnrecognisedFeatureType,
			IncorrectRootElementName,
			MissingVersionAttribute,
			MalformedVersionAttribute,
			PartiallySupportedVersionAttribute,
			ParseError,
			UnexpectedNonEmptyAttributeList,
			DuplicateRasterBandName,

			// The following are specific to GMAP vgp files
			// FIXME: This is a generic GmapError, we should add more field-specific errors.
			GmapError,
			GmapFieldFormatError,

			// The following are specific to regular and categorical GMT CPT files.
			InvalidRegularCptLine,
			InvalidCategoricalCptLine,
			CptSliceNotMonotonicallyIncreasing,
			ColourModelChangedMidway,
			NoLinesSuccessfullyParsed,
			CptFileTypeNotDeduced,
			UnrecognisedLabel,
			PatternFillInLine,

			// The following are specific to Hellinger-fit-related file formats.
			HellingerPickFormatError,
			InvalidHellingerComFileFormat,

			// The following are generic to all local files
			ErrorOpeningFileForReading,
			FileIsEmpty,
			NoFeaturesFoundInFile
		}; // enum Description

		enum Result
		{
			// These are specific to PLATES rotation-format reading.
			/*
			 * These enumerators are derived from the enumeration
			 * 'PlatesRotationFormatReader::ReadErrors::Result' in the header file
			 * "PlatesRotationFormatReader.hh" in the ReconTreeViewer software:
			 *  http://sourceforge.net/projects/recontreeviewer/
			 */
			EmptyCommentCreated,
			ExclMarkInsertedAtCommentStart,
			MovingPlateIdChangedToMatchEarlierSequence,
			NewOverlappingSequenceBegun,
			PoleDiscarded,

			// The following are specific to PLATES line-format reading.
			UnclassifiedFeatureCreated,
			FeatureDiscarded,
			NoGeometryCreatedByMovement,

			// The following are specific to OGR-supported file format reading.
			MultipleLayersIgnored,
			GeometryFlattenedTo2D,
			GeometryIgnored,
			OnlyExteriorRingRead,
			NoPlateIdLoadedForFile,
			NoPlateIdLoadedForFeature,
			NoConjugatePlateIdLoadedForFeature,
			AttributeIgnored,
			UnclassifiedOgrFeatureCreated,

			// The following relate to time-dependent raster file sets.
			NoRasterSetsLoaded,
			OnlyFirstRasterSetLoaded,
			
			// The following are specific to GPML reading.
			ElementIgnored,
			ParsingStoppedPrematurely,
			ElementNameChanged,
			ElementNotNameChanged,
			AssumingCurrentVersion,
			PropertyConvertedToTimeDependent,
			PropertyConvertedFromTimeDependent,
			PropertyConvertedBetweenTimeDependentTypes,
			PropertyNotInterpreted,
			FeatureNotInterpreted,
			AttributesIgnored,
			
			// The following are specific to GMAP vgp files
			GmapFeatureIgnored,

			// The following are specific to regular and categorical GMT CPT files.
			CptLineIgnored,

			// The following are specific to hellinger-fit-related files.
			HellingerComFileNotImported,
			HellingerPickIgnored,

			// The following are generic to all local files
			FileNotLoaded,
			FileNotImported,
			NoAction
		}; // enum Result


		/**
		 * Enumeration of possible error categories, for a simple way to report how
		 * severe an accumulation of errors is (ReadErrorAccumulation::most_severe_error_type()).
		 * Order from least severe to most severe.
		 */
		enum Severity
		{
			NothingWrong = 0,
			Warning,
			RecoverableError,
			TerminatingError,
			FailureToBegin
		};
	} // namespace ReadErrors
}

#endif  // GPLATES_FILEIO_READERRORS_H
