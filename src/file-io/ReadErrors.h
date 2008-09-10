/* $Id$ */

/**
 * \file 
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007, 2008 The University of Sydney, Australia
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
			ErrorReadingRotationAngle,
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

			// The following are specific to GPlates 8 hydrid PLATES line-format
			MissingPlatepolygonBoundaryFeature,
			InvalidPlatepolygonBoundaryFeature,

			// The following apply to ESRI Shapefile import
			NoLayersFoundInFile,
			MultipleLayersInFile,
			ErrorReadingShapefileLayer,
			NoFeaturesFoundInShapefile,
			ErrorReadingShapefileGeometry,
			TwoPointFiveDGeometryDetected,
			LessThanTwoPointsInLineString,
			InteriorRingsInShapefile,
			UnsupportedGeometryType,
			NoLatitudeShapeData,
			NoLongitudeShapeData,
			InvalidShapefileLatitude,
			InvalidShapefileLongitude,
			NoPlateIdFound,
			InvalidShapefilePlateIdNumber,
			InvalidShapefileAgeOfAppearance,
			InvalidShapefileAgeOfDisappearance,
			
			// The following relate to raster files.
			InsufficientTextureMemory,

			// The following relate to GDAL-readable Raster files.
			ErrorReadingGDALBand,

			// The following relate to QImage-readable image files.
			ErrorReadingQImageFile,

			// The following relate to time-dependent raster file sets.
			NoRasterSetsFound,
			MultipleRasterSetsFound,

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
			InvalidString,
			InvalidUnsignedInt,
			InvalidUnsignedLong,
			MissingNamespaceAlias,
			NonUniqueStructuralElement,
			StructuralElementNotFound,
			TooManyChildrenInElement,
			UnexpectedEmptyString,
			UnrecognisedChildFound,
			ConstantValueOnNonTimeDependentProperty,
			DuplicateIdentityProperty,
			DuplicateRevisionProperty,
			UnrecognisedFeatureCollectionElement,
			UnrecognisedFeatureType,
			IncorrectRootElementName,
			MissingVersionAttribute,
			IncorrectVersionAttribute,
			ParseError,
			UnexpectedNonEmptyAttributeList,

			// The following are generic to all local files
			ErrorOpeningFileForReading,
			FileIsEmpty
		};

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

			// The following are specific to ESRI shapefile reading.
			MultipleLayersIgnored,
			GeometryFlattenedTo2D,
			GeometryIgnored,
			OnlyExteriorRingRead,
			NoPlateIdLoadedForFile,
			NoPlateIdLoadedForFeature,
			AttributeIgnored,

			// The following relate to time-dependent raster file sets.
			NoRasterSetsLoaded,
			OnlyFirstRasterSetLoaded,
			
			// The following are specific to GPML reading.
			ElementIgnored,
			PropertyIgnored,
			ParsingStoppedPrematurely,
			ElementNameChanged,
			AssumingCorrectVersion,
			FeatureNotInterpreted,
			AttributesIgnored,

			// The following are generic to all local files
			FileNotLoaded
		};
	}
}

#endif  // GPLATES_FILEIO_READERRORS_H
