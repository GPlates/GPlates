/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2006, 2007 The University of Sydney, Australia
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

#include <map>
#include <string>
#include <sstream>
#include "ReadErrorAccumulationDialog.h"

#define NUM_ELEMS(a) (sizeof(a) / sizeof((a)[0]))

namespace
{
	// Maps used for error text lookup.
	typedef std::map<GPlatesFileIO::ReadErrors::Description, QString> description_map_type;
	typedef description_map_type::const_iterator description_map_const_iterator;
	typedef std::map<GPlatesFileIO::ReadErrors::Result, QString> result_map_type;
	typedef result_map_type::const_iterator result_map_const_iterator;

	// Map of Filename -> Error collection for reporting all errors of a particular type for each file.
	typedef std::map<std::string, GPlatesFileIO::ReadErrorAccumulation::read_error_collection_type> errors_by_file_map_type;
	typedef errors_by_file_map_type::const_iterator errors_by_file_map_const_iterator;

	// Map of ReadErrors::Description -> Error collection for reporting all errors of a particular type for each error code.
	typedef std::map<GPlatesFileIO::ReadErrors::Description, GPlatesFileIO::ReadErrorAccumulation::read_error_collection_type> errors_by_type_map_type;
	typedef errors_by_type_map_type::const_iterator errors_by_type_map_const_iterator;

	struct ReadErrorDescription
	{
		GPlatesFileIO::ReadErrors::Description code;
		const char *short_text;
		const char *full_text;
	};
	
	struct ReadErrorResult
	{
		GPlatesFileIO::ReadErrors::Result code;
		const char *text;
	};
	
	/**
	 * This table is sourced from http://trac.gplates.org/wiki/ReadErrorMessages .
	 */
	static ReadErrorDescription description_table[] = {
		// Error descriptions for PLATES line-format files:
		{ GPlatesFileIO::ReadErrors::InvalidPlatesRegionNumber,
				QT_TR_NOOP("Error reading 'Region Number'"),
				QT_TR_NOOP("Error reading 'Region Number' from header.") },
		{ GPlatesFileIO::ReadErrors::InvalidPlatesReferenceNumber,
				QT_TR_NOOP("Error reading 'Reference Number'"),
				QT_TR_NOOP("Error reading 'Reference Number' from header.") },
		{ GPlatesFileIO::ReadErrors::InvalidPlatesStringNumber,
				QT_TR_NOOP("Error reading 'String Number'"),
				QT_TR_NOOP("Error reading 'String Number' from header.") },
		{ GPlatesFileIO::ReadErrors::InvalidPlatesGeographicDescription,
				QT_TR_NOOP("Error reading 'Geographic Description'"),
				QT_TR_NOOP("Error reading 'Geographic Description' from header.") },
		{ GPlatesFileIO::ReadErrors::InvalidPlatesPlateIdNumber,
				QT_TR_NOOP("Error reading 'Plate Id'"),
				QT_TR_NOOP("Error reading 'Plate Id' from header.") },
		{ GPlatesFileIO::ReadErrors::InvalidPlatesAgeOfAppearance,
				QT_TR_NOOP("Error reading 'Age Of Appearance'"),
				QT_TR_NOOP("Error reading 'Age Of Appearance' from header.") },
		{ GPlatesFileIO::ReadErrors::InvalidPlatesAgeOfDisappearance,
				QT_TR_NOOP("Error reading 'Age Of Disappearance'"),
				QT_TR_NOOP("Error reading 'Age Of Disappearance' from header.") },
		{ GPlatesFileIO::ReadErrors::InvalidPlatesDataTypeCode,
				QT_TR_NOOP("Error reading 'Data Type Code'"),
				QT_TR_NOOP("Error reading 'Data Type Code' from header.") },
		{ GPlatesFileIO::ReadErrors::InvalidPlatesDataTypeCodeNumber,
				QT_TR_NOOP("Error reading 'Data Type Number'"),
				QT_TR_NOOP("Error reading 'Data Type Number' from header.") },
		{ GPlatesFileIO::ReadErrors::InvalidPlatesDataTypeCodeNumberAdditional,
				QT_TR_NOOP("Error reading 'Data Type Letter'"),
				QT_TR_NOOP("Error reading 'Data Type Letter' from header.") },
		{ GPlatesFileIO::ReadErrors::InvalidPlatesConjugatePlateIdNumber,
				QT_TR_NOOP("Error reading 'Conjugate Plate Id'"),
				QT_TR_NOOP("Error reading 'Conjugate Plate Id' from header.") },
		{ GPlatesFileIO::ReadErrors::InvalidPlatesColourCode,
				QT_TR_NOOP("Error reading 'Colour Code'"),
				QT_TR_NOOP("Error reading 'Colour Code' from header.") },
		{ GPlatesFileIO::ReadErrors::InvalidPlatesNumberOfPoints,
				QT_TR_NOOP("Error reading 'Number Of Points'"),
				QT_TR_NOOP("Error reading 'Number Of Points' from header.") },
		{ GPlatesFileIO::ReadErrors::UnknownPlatesDataTypeCode,
				QT_TR_NOOP("Unrecognized 'Data Type Code'"),
				QT_TR_NOOP("Unrecognized 'Data Type Code' in the header.") },
		{ GPlatesFileIO::ReadErrors::MissingPlatesPolylinePoint,
				QT_TR_NOOP("Point not found"),
				QT_TR_NOOP("A point was expected, but not found.") },
		{ GPlatesFileIO::ReadErrors::MissingPlatesHeaderSecondLine,
				QT_TR_NOOP("Missing second header line"),
				QT_TR_NOOP("The second line of the header was not found.") },
		{ GPlatesFileIO::ReadErrors::InvalidPlatesPolylinePoint,
				QT_TR_NOOP("Malformed point"),
				QT_TR_NOOP("A point was not of '<latitude> <longtitude> <plotter code>' form.") },
		{ GPlatesFileIO::ReadErrors::InvalidPlatesPolylinePlotterCode,
				QT_TR_NOOP("Invalid plotter code"),
				QT_TR_NOOP("The plotter code was invalid (neither 'draw to' nor 'skip to').") },
		{ GPlatesFileIO::ReadErrors::InvalidPlatesPolylineLatitude,
				QT_TR_NOOP("Invalid latitude"),
				QT_TR_NOOP("The latitude of the point was not in the range [-90, 90].") },
		{ GPlatesFileIO::ReadErrors::InvalidPlatesPolylineLongitude,
				QT_TR_NOOP("Invalid longitude"),
				QT_TR_NOOP("The longitude of the point was not in the range [-360, 360].") },
		{ GPlatesFileIO::ReadErrors::AdjacentSkipToPlotterCodes,
				QT_TR_NOOP("Adjacent 'skip to' codes"),
				QT_TR_NOOP("A 'skip to' plotter code followed immediately after another 'skip to' plotter code.") },
		{ GPlatesFileIO::ReadErrors::AmbiguousPlatesIceShelfCode,
				QT_TR_NOOP("Data type code 'IS' is deprecated"),
				QT_TR_NOOP("The data type code 'IS' is no longer used for Isochron (Cenozoic). Use 'IC' instead.") },
		{ GPlatesFileIO::ReadErrors::MoreThanOneDistinctPoint,
				QT_TR_NOOP("More than one point"),
				QT_TR_NOOP("A single distinct point was expected, but more were encountered.") },
		
		// Error descriptions for PLATES rotation-format files:
		{ GPlatesFileIO::ReadErrors::CommentMovingPlateIdAfterNonCommentSequence,
				QT_TR_NOOP("Moving plate ID will be changed"),
				QT_TR_NOOP("GPlates will need to change the moving plate ID from 999 to some other value.") },
		{ GPlatesFileIO::ReadErrors::ErrorReadingFixedPlateId,
				QT_TR_NOOP("Error reading fixed plate ID"),
				QT_TR_NOOP("Error reading the fixed plate ID.") },
		{ GPlatesFileIO::ReadErrors::ErrorReadingGeoTime,
				QT_TR_NOOP("Error reading geological time"),
				QT_TR_NOOP("Error reading the geological time.") },
		{ GPlatesFileIO::ReadErrors::ErrorReadingMovingPlateId,
				QT_TR_NOOP("Error reading moving plate ID"),
				QT_TR_NOOP("Error reading the moving plate ID.") },
		{ GPlatesFileIO::ReadErrors::ErrorReadingPoleLatitude,
				QT_TR_NOOP("Error reading latitude"),
				QT_TR_NOOP("Error reading the pole latitude coordinate.") },
		{ GPlatesFileIO::ReadErrors::ErrorReadingPoleLongitude,
				QT_TR_NOOP("Error reading longitude"),
				QT_TR_NOOP("Error reading the pole longitude coordinate.") },
		{ GPlatesFileIO::ReadErrors::ErrorReadingRotationAngle,
				QT_TR_NOOP("Error reading rotation angle"),
				QT_TR_NOOP("Error reading the rotation angle. ") },
		{ GPlatesFileIO::ReadErrors::MovingPlateIdEqualsFixedPlateId,
				QT_TR_NOOP("Identical plate IDs"),
				QT_TR_NOOP("The moving plate ID is identical to the fixed plate ID.") },
		{ GPlatesFileIO::ReadErrors::NoCommentFound,
				QT_TR_NOOP("No comment found"),
				QT_TR_NOOP("No comment string was found at the end of the line.") },
		{ GPlatesFileIO::ReadErrors::NoExclMarkToStartComment,
				QT_TR_NOOP("No exclamation mark found"),
				QT_TR_NOOP("No exclamation mark was found before the start of the comment string.") },
		{ GPlatesFileIO::ReadErrors::SamePlateIdsButDuplicateGeoTime,
				QT_TR_NOOP("Duplicate geo-time"),
				QT_TR_NOOP("Consecutive poles had the same plate IDs and identical geo-times.") },
		{ GPlatesFileIO::ReadErrors::SamePlateIdsButEarlierGeoTime,
				QT_TR_NOOP("Overlapping geo-times"),
				QT_TR_NOOP("Consecutive poles had the same plate IDs and overlapping geo-times.") },

		
		// Error descriptions for GPML format files:
		{ GPlatesFileIO::ReadErrors::DuplicateProperty,
				QT_TR_NOOP("Duplicate property"),
				QT_TR_NOOP("More than one instance of a property was found where no more than one was expected.") },
		{ GPlatesFileIO::ReadErrors::NecessaryPropertyNotFound,
				QT_TR_NOOP("Necessary property not found"),
				QT_TR_NOOP("A property which is not optional was not found.") },
		{ GPlatesFileIO::ReadErrors::UnknownValueType,
				QT_TR_NOOP("Unknown value type"),
				QT_TR_NOOP("A GPML Template Type was used where the type is not known to GPlates.") },
		{ GPlatesFileIO::ReadErrors::BadOrMissingTargetForValueType,
				QT_TR_NOOP("Bad or missing target for value type"),
				QT_TR_NOOP("A GPML Template Type was used but the value could not be resolved.") },
		{ GPlatesFileIO::ReadErrors::InvalidBoolean,
				QT_TR_NOOP("Invalid boolean"),
				QT_TR_NOOP("A boolean (true/false) value was expected, but the supplied value could not be interpreted as a boolean.") },
		{ GPlatesFileIO::ReadErrors::InvalidDouble,
				QT_TR_NOOP("Invalid double"),
				QT_TR_NOOP("A double (high precision decimal number) value was expected, but the supplied value could not be interpreted as a double.") },
		{ GPlatesFileIO::ReadErrors::InvalidGeoTime,
				QT_TR_NOOP("Invalid geological time"),
				QT_TR_NOOP("The supplied value could not be interpreted as a geological time.") },
		{ GPlatesFileIO::ReadErrors::InvalidInt,
				QT_TR_NOOP("Invalid integer"),
				QT_TR_NOOP("An integer value was expected, but the supplied value could not be interpreted as an integer.") },
		{ GPlatesFileIO::ReadErrors::InvalidLatLonPoint,
				QT_TR_NOOP("Invalid lat,lon point"),
				QT_TR_NOOP("A lat,lon point was encountered outside the valid range for latitude and longitude.") },
		{ GPlatesFileIO::ReadErrors::InvalidLong,
				QT_TR_NOOP("Invalid long integer"),
				QT_TR_NOOP("A long integer value was expected, but the supplied value could not be interpreted as a long integer.") },
		{ GPlatesFileIO::ReadErrors::InvalidPointsInPolyline,
				QT_TR_NOOP("Invalid points in polyline"),
				QT_TR_NOOP("The points of the polyline are invalid (No specific error message is available).") },
		{ GPlatesFileIO::ReadErrors::InsufficientDistinctPointsInPolyline,
				QT_TR_NOOP("Insufficient distinct points in polyline"),
				QT_TR_NOOP("Polylines must be defined with at least two distinct points.") },
		{ GPlatesFileIO::ReadErrors::AntipodalAdjacentPointsInPolyline,
				QT_TR_NOOP("Antipodal adjacent points in polyline"),
				QT_TR_NOOP("Segments of a polyline cannot be defined between two points which are antipodal.") },
		{ GPlatesFileIO::ReadErrors::InvalidPointsInPolygon,
				QT_TR_NOOP("Invalid points in polygon"),
				QT_TR_NOOP("The points of the polygon are invalid (No specific error message is available).") },
		{ GPlatesFileIO::ReadErrors::InvalidPolygonEndPoint,
				QT_TR_NOOP("Invalid polygon end point"),
				QT_TR_NOOP("GML Polygons' terminating point must be identical to their starting point.") },
		{ GPlatesFileIO::ReadErrors::InsufficientPointsInPolygon,
				QT_TR_NOOP("Insufficient points in polygon"),
				QT_TR_NOOP("GML Polygons must be defined with at least four points (which includes the identical start and end point).") },
		{ GPlatesFileIO::ReadErrors::InsufficientDistinctPointsInPolygon,
				QT_TR_NOOP("Insufficient distinct points in polygon"),
				QT_TR_NOOP("Polygons must be defined with at least three distinct points.") },
		{ GPlatesFileIO::ReadErrors::AntipodalAdjacentPointsInPolygon,
				QT_TR_NOOP("Antipodal adjacent points in polygon"),
				QT_TR_NOOP("Segments of a polygon cannot be defined between two points which are antipodal.") },
		{ GPlatesFileIO::ReadErrors::InvalidString,
				QT_TR_NOOP("Invalid string"),
				QT_TR_NOOP("A text string was encountered which included XML elements.") },
		{ GPlatesFileIO::ReadErrors::InvalidUnsignedInt,
				QT_TR_NOOP("Invalid unsigned integer"),
				QT_TR_NOOP("An unsigned (positive) integer value was expected, but the supplied value could not be interpreted as an unsigned integer.") },
		{ GPlatesFileIO::ReadErrors::InvalidUnsignedLong,
				QT_TR_NOOP("Invalid unsigned long integer"),
				QT_TR_NOOP("An unsigned (positive) long integer value was expected, but the supplied value could not be interpreted as an unsigned long integer.") },
		{ GPlatesFileIO::ReadErrors::MissingNamespaceAlias,
				QT_TR_NOOP("Missing XML namespace alias"),
				QT_TR_NOOP("An XML namespace alias was referred to which has not been defined at the start of the FeatureCollection element.") },
		{ GPlatesFileIO::ReadErrors::NonUniqueStructuralElement,
				QT_TR_NOOP("Multiple structural elements encountered"),
				QT_TR_NOOP("A single property containing multiple structural elements was encountered, where only one is allowed.") },
		{ GPlatesFileIO::ReadErrors::StructuralElementNotFound,
				QT_TR_NOOP("Structural element not found"),
				QT_TR_NOOP("A structural element was expected inside a property, but was not found.") },
		{ GPlatesFileIO::ReadErrors::TooManyChildrenInElement,
				QT_TR_NOOP("Too many children in element"),
				QT_TR_NOOP("Found more child elements than were expected.") },
		{ GPlatesFileIO::ReadErrors::UnexpectedEmptyString,
				QT_TR_NOOP("Unexpected empty string"),
				QT_TR_NOOP("A blank string was encountered where a non-empty text value was expected.") },
		{ GPlatesFileIO::ReadErrors::UnrecognisedChildFound,
				QT_TR_NOOP("Unrecognised child found"),
				QT_TR_NOOP("An unrecognised XML child element was encountered.") },
		{ GPlatesFileIO::ReadErrors::ConstantValueOnNonTimeDependentProperty,
				QT_TR_NOOP("ConstantValue on non-time-dependent property"),
				QT_TR_NOOP("A gpml:ConstantValue wrapper was found around a property value that was not expected to have time-dependency.") },
		// GPlatesFileIO::ReadErrors::DuplicateIdentityProperty FIXME: unused.
		// GPlatesFileIO::ReadErrors::DuplicateRevisionProperty FIXME: unused.
		{ GPlatesFileIO::ReadErrors::UnrecognisedFeatureCollectionElement,
				QT_TR_NOOP("Unrecognised feature collection element"),
				QT_TR_NOOP("An element inside the gml:FeatureCollection was unrecognised.") },
		{ GPlatesFileIO::ReadErrors::UnrecognisedFeatureType,
				QT_TR_NOOP("Unrecognised feature type"),
				QT_TR_NOOP("An unrecognised type of feature was encountered.") },
		{ GPlatesFileIO::ReadErrors::IncorrectRootElementName,
				QT_TR_NOOP("Incorrect root element name"),
				QT_TR_NOOP("The document root element was not a 'gml:FeatureCollection'.") },
		{ GPlatesFileIO::ReadErrors::MissingVersionAttribute,
				QT_TR_NOOP("Missing version attribute"),
				QT_TR_NOOP("No information about which version of GPML this document uses was found.") },
		{ GPlatesFileIO::ReadErrors::IncorrectVersionAttribute,
				QT_TR_NOOP("Incorrect version attribute"),
				QT_TR_NOOP("The document is not using a known version of GPML.") },
		{ GPlatesFileIO::ReadErrors::ParseError,
				QT_TR_NOOP("Parse Error"),
				QT_TR_NOOP("Malformed XML was encountered.") },
		{ GPlatesFileIO::ReadErrors::UnexpectedNonEmptyAttributeList,
				QT_TR_NOOP("Unexpected attributes found"),
				QT_TR_NOOP("XML attributes were encountered on a Feature element where none were expected.") },


		// The following descriptions are related to ESRI shapefile input errors:
		{ GPlatesFileIO::ReadErrors::NoLayersFoundInFile,
				QT_TR_NOOP("No layers found."),
				QT_TR_NOOP("No layers were found in the shapefile.") },
		{ GPlatesFileIO::ReadErrors::MultipleLayersInFile,
				QT_TR_NOOP("Multiple layers found."),
				QT_TR_NOOP("Multiple layers were found in the shapefile.") },
		{ GPlatesFileIO::ReadErrors::ErrorReadingShapefileLayer,
				QT_TR_NOOP("Error reading layer."),
				QT_TR_NOOP("There was an error reading a shapefile layer.") },
		{ GPlatesFileIO::ReadErrors::NoFeaturesFoundInShapefile,
				QT_TR_NOOP("No features found."),
				QT_TR_NOOP("No features were found in the shapefile.") },
		{ GPlatesFileIO::ReadErrors::ErrorReadingShapefileGeometry,
				QT_TR_NOOP("Error reading geometry."),
				QT_TR_NOOP("There was an error reading a shapefile geometry.") },
		{ GPlatesFileIO::ReadErrors::TwoPointFiveDGeometryDetected,
				QT_TR_NOOP("Geometry-type 2.5D"),
				QT_TR_NOOP("The shapefile is of geometry-type 2.5D") },
		{ GPlatesFileIO::ReadErrors::LessThanTwoPointsInLineString,
				QT_TR_NOOP("Less than two points"),
				QT_TR_NOOP("The line geometry had less than two points") },
		{ GPlatesFileIO::ReadErrors::InteriorRingsInShapefile,
				QT_TR_NOOP("Polygon had interior rings."),
				QT_TR_NOOP("A polygon had interior rings.") },
		{ GPlatesFileIO::ReadErrors::UnsupportedGeometryType,
				QT_TR_NOOP("Unsupported geometry type found."),
				QT_TR_NOOP("An unsupported geometry type was found.") },
		{ GPlatesFileIO::ReadErrors::NoLatitudeShapeData,
				QT_TR_NOOP("Latitude less than 1e-38."),
				QT_TR_NOOP("A latitude value less than 1e-38 was found.") },
		{ GPlatesFileIO::ReadErrors::NoLongitudeShapeData,
				QT_TR_NOOP("Longitude less than 1e-38."),
				QT_TR_NOOP("A longitude value less than 1e-38 was found.") },
		{ GPlatesFileIO::ReadErrors::InvalidShapefileLatitude,
				QT_TR_NOOP("Invalid latitude."),
				QT_TR_NOOP("An invalid latitude was found.") },
		{ GPlatesFileIO::ReadErrors::InvalidShapefileLongitude,
				QT_TR_NOOP("Invalid longitude."),
				QT_TR_NOOP("An invalid longitude was found.") },
		{ GPlatesFileIO::ReadErrors::NoPlateIdFound,
				QT_TR_NOOP("No Plate-id field."),
				QT_TR_NOOP("No Plate-id field was found for this file.") },
		{ GPlatesFileIO::ReadErrors::InvalidShapefilePlateIdNumber,
				QT_TR_NOOP("Invalid Plate-id."),
				QT_TR_NOOP("An invalid Plate-id was found.") },
		{ GPlatesFileIO::ReadErrors::UnrecognisedShapefileFeatureType,
				QT_TR_NOOP("Unrecognised feature type."),
				QT_TR_NOOP("Unrecognised feature type found.") },
		{ GPlatesFileIO::ReadErrors::InvalidShapefileAgeOfAppearance,
				QT_TR_NOOP("Invalid age of appearance."),
				QT_TR_NOOP("An invalid age of appearance was found.") },
		{ GPlatesFileIO::ReadErrors::InvalidShapefileAgeOfDisappearance,
				QT_TR_NOOP("Invalid age of disappearance."),
				QT_TR_NOOP("An invalid age of disappearance was found.") },
		{ GPlatesFileIO::ReadErrors::InvalidShapefilePoint,
				QT_TR_NOOP("Invalid point."),
				QT_TR_NOOP("An invalid point geometry was found.") },
		{ GPlatesFileIO::ReadErrors::InvalidShapefileMultiPoint,
				QT_TR_NOOP("Invalid multi-point."),
				QT_TR_NOOP("An invalid multi-point geometry was found.") },
		{ GPlatesFileIO::ReadErrors::InvalidShapefilePolyline,
				QT_TR_NOOP("Invalid polyline."),
				QT_TR_NOOP("An invalid polyline geometry was found.") },
		{ GPlatesFileIO::ReadErrors::InvalidShapefilePolygon,
				QT_TR_NOOP("Invalid polygon."),
				QT_TR_NOOP("An invalid polygon geometry was found.") },

		// Errors relating to raster files
		{ GPlatesFileIO::ReadErrors::InsufficientTextureMemory,
				QT_TR_NOOP("Insufficient texture memory."),
				QT_TR_NOOP("There was insufficient memory to load the requested raster.") },
		{ GPlatesFileIO::ReadErrors::ErrorGeneratingTexture,
				QT_TR_NOOP("Error generating texture."),
				QT_TR_NOOP("There was an error generating an openGL texture.") },

		// Errors relating to GDAL-readable Raster files
		{ GPlatesFileIO::ReadErrors::ErrorReadingGDALBand,
				QT_TR_NOOP("Error reading GDAL band."),
				QT_TR_NOOP("Error reading GDAL band.") },

		// Errors relating to QImage-readable image files
		{ GPlatesFileIO::ReadErrors::ErrorReadingQImageFile,
				QT_TR_NOOP("Error reading QImage file."),
				QT_TR_NOOP("Error reading QImage file.") },

		// Errors relating to time-dependent raster file sets
		{ GPlatesFileIO::ReadErrors::NoRasterSetsFound,
				QT_TR_NOOP("No raster sets found."),
				QT_TR_NOOP("No suitable raster files were found in the selected folder.") },
		{ GPlatesFileIO::ReadErrors::MultipleRasterSetsFound,
				QT_TR_NOOP("Multiple raster sets found."),
				QT_TR_NOOP("More than one suitable raster file set was found in the selected folder.") },

		// Generic file-related error descriptions:
		{ GPlatesFileIO::ReadErrors::ErrorOpeningFileForReading,
				QT_TR_NOOP("Error opening file"),
				QT_TR_NOOP("Error opening the file for reading.") },
		{ GPlatesFileIO::ReadErrors::FileIsEmpty,
				QT_TR_NOOP("File is empty"),
				QT_TR_NOOP("The file contains no data.") },
	};
	
	/**
	 * This table is sourced from http://trac.gplates.org/wiki/ReadErrorMessages .
	 */
	static ReadErrorResult result_table[] = {
		// Error results for PLATES line-format files:
		{ GPlatesFileIO::ReadErrors::UnclassifiedFeatureCreated,
				QT_TR_NOOP("Because the 'Data Type Code' was not known, Unclassified Features will be created.") },
		{ GPlatesFileIO::ReadErrors::FeatureDiscarded,
				QT_TR_NOOP("The feature was discarded due to errors encountered when parsing.") },
		{ GPlatesFileIO::ReadErrors::NoGeometryCreatedByMovement,
				QT_TR_NOOP("No new geometry was created by the 'pen movement'.") },

		// Error results for PLATES rotation-format files:
		{ GPlatesFileIO::ReadErrors::EmptyCommentCreated,
				QT_TR_NOOP("An empty comment was created.") },
		{ GPlatesFileIO::ReadErrors::ExclMarkInsertedAtCommentStart,
				QT_TR_NOOP("An exclamation mark was inserted to start the comment.") },
		{ GPlatesFileIO::ReadErrors::MovingPlateIdChangedToMatchEarlierSequence,
				QT_TR_NOOP("The moving plate ID of the pole was changed to match the earlier sequence.") },
		{ GPlatesFileIO::ReadErrors::NewOverlappingSequenceBegun,
				QT_TR_NOOP("A new sequence was begun which overlaps.") },
		{ GPlatesFileIO::ReadErrors::PoleDiscarded,
				QT_TR_NOOP("The pole was discarded.") },

		// Error results from GPML format files:
		// GPlatesFileIO::ReadErrors::ElementIgnored FIXME: unused.
		// GPlatesFileIO::ReadErrors::PropertyIgnored FIXME: unused.
		{ GPlatesFileIO::ReadErrors::ParsingStoppedPrematurely,
				QT_TR_NOOP("Parsing the file was stopped prematurely.") },
		{ GPlatesFileIO::ReadErrors::ElementNameChanged,
				QT_TR_NOOP("The name of the element was changed.") },
		{ GPlatesFileIO::ReadErrors::AssumingCorrectVersion,
				QT_TR_NOOP("The correct version will be assumed.") },
		{ GPlatesFileIO::ReadErrors::FeatureNotInterpreted,
				QT_TR_NOOP("The feature was not interpreted.") },
		{ GPlatesFileIO::ReadErrors::AttributesIgnored,
				QT_TR_NOOP("The attributes were ignored.") },

		// The following results apply to ESRI shapefile input errors:
		{ GPlatesFileIO::ReadErrors::MultipleLayersIgnored,
				QT_TR_NOOP("Only the first layer was read.") },
		{ GPlatesFileIO::ReadErrors::GeometryFlattenedTo2D,
				QT_TR_NOOP("The geometry has been flattened to geometry-type 2D") },
		{ GPlatesFileIO::ReadErrors::GeometryIgnored,
				QT_TR_NOOP("The geometry was ignored") },
		{ GPlatesFileIO::ReadErrors::OnlyExteriorRingRead,
				QT_TR_NOOP("Only the exterior ring was read.") },
		{ GPlatesFileIO::ReadErrors::NoPlateIdLoadedForFile,
				QT_TR_NOOP("No Plate-ids have been loaded for this file.") },
		{ GPlatesFileIO::ReadErrors::NoPlateIdLoadedForFeature,
				QT_TR_NOOP("No Plate-id was read for this feature.") },
		{ GPlatesFileIO::ReadErrors::AttributeIgnored,
				QT_TR_NOOP("The attribute was not mapped to a model property.") },
		{ GPlatesFileIO::ReadErrors::UnclassifiedShapefileFeatureCreated,
				QT_TR_NOOP("An unclassifiedFeature was created.") },
				
		// The following apply to time-dependent raster file sets
		{ GPlatesFileIO::ReadErrors::NoRasterSetsLoaded,
				QT_TR_NOOP("No raster file set was loaded.") },
		{ GPlatesFileIO::ReadErrors::OnlyFirstRasterSetLoaded,
				QT_TR_NOOP("Only the first raster file set was loaded.") },
				
		// Generic file-related errors:
		{ GPlatesFileIO::ReadErrors::FileNotLoaded,
				QT_TR_NOOP("The file was not loaded.") },
	};

	
	/**
	 * Converts the short error messages from the statically defined description
	 * table to a STL map.
	 */
	const description_map_type &
	build_short_description_map()
	{
		static description_map_type map;
		
		ReadErrorDescription *begin = description_table;
		ReadErrorDescription *end = begin + NUM_ELEMS(description_table);
		for (; begin != end; ++begin) {
			map[begin->code] = QObject::tr(begin->short_text);
		}
		
		return map;
	}

	/**
	 * Converts the full error messages from the statically defined description
	 * table to a STL map.
	 */
	const description_map_type &
	build_full_description_map()
	{
		static description_map_type map;
		
		ReadErrorDescription *begin = description_table;
		ReadErrorDescription *end = begin + NUM_ELEMS(description_table);
		for (; begin != end; ++begin) {
			map[begin->code] = QObject::tr(begin->full_text);
		}
		
		return map;
	}
	
	/**
	 * Converts the statically defined result table to a STL map.
	 */
	const result_map_type &
	build_result_map()
	{
		static result_map_type map;
		
		ReadErrorResult *begin = result_table;
		ReadErrorResult *end = begin + NUM_ELEMS(result_table);
		for (; begin != end; ++begin) {
			map[begin->code] = QObject::tr(begin->text);
		}
		
		return map;
	}

	/**
	 * Takes a read_error_collection_type and creates a map of read_error_collection_types,
	 * grouped by filename.
	 */
	void
	group_read_errors_by_file(
			errors_by_file_map_type &errors_by_file,
			const GPlatesFileIO::ReadErrorAccumulation::read_error_collection_type &errors)
	{
		GPlatesFileIO::ReadErrorAccumulation::read_error_collection_const_iterator it = errors.begin();
		GPlatesFileIO::ReadErrorAccumulation::read_error_collection_const_iterator end = errors.end();
		for (; it != end; ++it) {
			// Add the error to the map based on filename.
			std::ostringstream source;
			it->d_data_source->write_full_name(source);
			
			errors_by_file[source.str()].push_back(*it);
		}
	}

	/**
	 * Takes a read_error_collection_type and creates a map of read_error_collection_types,
	 * grouped by error type (the ReadErrors::Description enum).
	 */
	void
	group_read_errors_by_type(
			errors_by_type_map_type &errors_by_type,
			const GPlatesFileIO::ReadErrorAccumulation::read_error_collection_type &errors)
	{
		GPlatesFileIO::ReadErrorAccumulation::read_error_collection_const_iterator it = errors.begin();
		GPlatesFileIO::ReadErrorAccumulation::read_error_collection_const_iterator end = errors.end();
		for (; it != end; ++it) {
			// Add the error to the map based on error code.
			errors_by_type[it->d_description].push_back(*it);
		}
	}
	
}


GPlatesQtWidgets::ReadErrorAccumulationDialog::ReadErrorAccumulationDialog(
		QWidget *parent_):
	QDialog(parent_),
	d_information_dialog(s_information_dialog_text, s_information_dialog_title, this)
{
	setupUi(this);
	clear();
	
	QObject::connect(button_help, SIGNAL(clicked()),
			this, SLOT(pop_up_help_dialog()));

	QObject::connect(button_expand_all, SIGNAL(clicked()),
			this, SLOT(expandAll()));
	QObject::connect(button_collapse_all, SIGNAL(clicked()),
			this, SLOT(collapseAll()));
	QObject::connect(button_clear, SIGNAL(clicked()),
			this, SLOT(clear_errors()));
}


void
GPlatesQtWidgets::ReadErrorAccumulationDialog::clear()
{
	label_problem->setText(tr("There are no warnings or errors for the currently-loaded files."));
	
	// Clear errors from the "By Error" tab.
	tree_widget_errors_by_type->clear();
	// Add the "Failures to Begin (n)" item.
	d_tree_type_failures_to_begin_ptr = new QTreeWidgetItem(tree_widget_errors_by_type);
	d_tree_type_failures_to_begin_ptr->setHidden(true);
	tree_widget_errors_by_type->addTopLevelItem(d_tree_type_failures_to_begin_ptr);
	// Add the "Terminating Errors (n)" item.
	d_tree_type_terminating_errors_ptr = new QTreeWidgetItem(tree_widget_errors_by_type);
	d_tree_type_terminating_errors_ptr->setHidden(true);
	tree_widget_errors_by_type->addTopLevelItem(d_tree_type_terminating_errors_ptr);
	// Add the "Recoverable Errors (n)" item.
	d_tree_type_recoverable_errors_ptr = new QTreeWidgetItem(tree_widget_errors_by_type);
	d_tree_type_recoverable_errors_ptr->setHidden(true);
	tree_widget_errors_by_type->addTopLevelItem(d_tree_type_recoverable_errors_ptr);
	// Add the "Warnings (n)" item.
	d_tree_type_warnings_ptr = new QTreeWidgetItem(tree_widget_errors_by_type);
	d_tree_type_warnings_ptr->setHidden(true);
	tree_widget_errors_by_type->addTopLevelItem(d_tree_type_warnings_ptr);

	// Clear errors from the "By Line" tab.
	tree_widget_errors_by_line->clear();
	// Add the "Failures to Begin (n)" item.
	d_tree_line_failures_to_begin_ptr = new QTreeWidgetItem(tree_widget_errors_by_line);
	d_tree_line_failures_to_begin_ptr->setHidden(true);
	tree_widget_errors_by_line->addTopLevelItem(d_tree_line_failures_to_begin_ptr);
	// Add the "Terminating Errors (n)" item.
	d_tree_line_terminating_errors_ptr = new QTreeWidgetItem(tree_widget_errors_by_line);
	d_tree_line_terminating_errors_ptr->setHidden(true);
	tree_widget_errors_by_line->addTopLevelItem(d_tree_line_terminating_errors_ptr);
	// Add the "Recoverable Errors (n)" item.
	d_tree_line_recoverable_errors_ptr = new QTreeWidgetItem(tree_widget_errors_by_line);
	d_tree_line_recoverable_errors_ptr->setHidden(true);
	tree_widget_errors_by_line->addTopLevelItem(d_tree_line_recoverable_errors_ptr);
	// Add the "Warnings (n)" item.
	d_tree_line_warnings_ptr = new QTreeWidgetItem(tree_widget_errors_by_line);
	d_tree_line_warnings_ptr->setHidden(true);
	tree_widget_errors_by_line->addTopLevelItem(d_tree_line_warnings_ptr);
}


void
GPlatesQtWidgets::ReadErrorAccumulationDialog::update()
{
	static const QIcon icon_error(":/gnome_dialog_error_16.png");
	static const QIcon icon_warning(":/gnome_dialog_warning_16.png");
	
	// Disabling screen updates to work around Qt slowness when >1000 warnings.
	// http://doc.trolltech.com/4.3/qwidget.html#updatesEnabled-prop
	// Not as huge a speedup as I hoped, but every little bit helps.
	setUpdatesEnabled(false);
	
	// Populate the Failures to Begin tree by type.
	populate_top_level_tree_by_type(d_tree_type_failures_to_begin_ptr, tr("Failure to Begin (%1)"),
			d_read_errors.d_failures_to_begin, icon_error);

	// Populate the Terminating Errors tree by type.
	populate_top_level_tree_by_type(d_tree_type_terminating_errors_ptr, tr("Terminating Errors (%1)"),
			d_read_errors.d_terminating_errors, icon_error);

	// Populate the Recoverable Errors tree by type.
	populate_top_level_tree_by_type(d_tree_type_recoverable_errors_ptr, tr("Recoverable Errors (%1)"),
			d_read_errors.d_recoverable_errors, icon_error);

	// Populate the Warnings tree by type.
	populate_top_level_tree_by_type(d_tree_type_warnings_ptr, tr("Warnings (%1)"),
			d_read_errors.d_warnings, icon_warning);

	// Populate the Failures to Begin tree by line.
	populate_top_level_tree_by_line(d_tree_line_failures_to_begin_ptr, tr("Failure to Begin (%1)"),
			d_read_errors.d_failures_to_begin, icon_error);

	// Populate the Terminating Errors tree by line.
	populate_top_level_tree_by_line(d_tree_line_terminating_errors_ptr, tr("Terminating Errors (%1)"),
			d_read_errors.d_terminating_errors, icon_error);

	// Populate the Recoverable Errors tree by line.
	populate_top_level_tree_by_line(d_tree_line_recoverable_errors_ptr, tr("Recoverable Errors (%1)"),
			d_read_errors.d_recoverable_errors, icon_error);

	// Populate the Warnings tree by line.
	populate_top_level_tree_by_line(d_tree_line_warnings_ptr, tr("Warnings (%1)"),
			d_read_errors.d_warnings, icon_warning);

	// Update labels.
	QString summary_str = build_summary_string();
	label_problem->setText(summary_str);

	// Re-enable screen updates after all items have been added.
	// Re-enabling implicitly calls update() on the widget.
	setUpdatesEnabled(true);
}


const QString GPlatesQtWidgets::ReadErrorAccumulationDialog::s_information_dialog_text = QObject::tr(
		"<html><body>\n"
		"Read errors fall into four categories: <ul> <li>failures to begin</li> "
		"<li>terminating errors</li> <li>recoverable errors</li> <li>warnings</li> </ul>\n"
		"\n"
		"<h3>Failure To Begin:</h3>\n"
		"<ul>\n"
		"<li> A failure to begin has occurred when GPlates is not even able to start reading "
		"data from the data source. </li>\n"
		"<li> Examples of failures to begin might include: the file cannot be located on disk "
		"or opened for reading; the database cannot be accessed; no network connection "
		"could be established. </li>\n"
		"<li> In the event of a failure to begin, GPlates will not be able to load any data "
		"from the data source. </li>\n"
		"</ul>\n"
		"<h3>Terminating Error:</h3>\n"
		"<ul>\n"
		"<li> A terminating error halts the reading of data in such a way that GPlates is "
		"unable to read any more data from the data source. </li>\n"
		"<li> Examples of terminating errors might include: a file-system error; a broken "
		"network connection. </li>\n"
		"<li> When a terminating error occurs, GPlates will retain the data it has already "
		"read, but will not be able to read any more data from the data source. </li>\n"
		"</ul>\n"
		"<h3>Recoverable Error:</h3>\n"
		"<ul>\n"
		"<li> A recoverable error is an error (generally an error in the data) from which "
		"GPlates is able to recover, although some amount of data had to be discarded "
		"because it was invalid or malformed in such a way that GPlates was unable to repair "
		"it. </li>\n"
		"<li> Examples of recoverable errors might include: when the wrong type of data "
		"encountered in a fixed-width attribute field (for instance, text encountered where "
		"an integer was expected). </li>\n"
		"<li> When a recoverable error occurs, GPlates will retain the data it has already "
		"successfully read; discard the invalid or malformed data (which will result in "
		"some data loss); and continue reading from the data source. GPlates will discard "
		"the smallest possible amount of data, and will inform you exactly what was discarded. "
		"</li>\n"
		"</ul>\n"
		"<h3>Warning:</h3>\n"
		"<ul>\n"
		"<li> A warning is a notification of a problem (generally a problem in the data) "
		"which required GPlates to modify the data in order to rectify the situation. "
		"<li> Examples of problems which might result in warnings include: data which are "
		"being imported into GPlates, which do not possess <i>quite</i> enough information "
		"for the needs of GPlates (such as total reconstruction poles in PLATES4 "
		"rotation-format files which have been commented-out by changing their moving plate "
		"ID to 999); an attribute field whose value is obviously incorrect, but which is easy "
		"for GPlates to repair (for instance, when the 'Number Of Points' field in a PLATES4 "
		"line-format polyline header does not match the actual number of points in the "
		"polyline). </li>\n"
		"<li> A warning will not have resulted in any data loss, but you may wish to "
		"investigate the problem, in order to verify that GPlates has 'corrected' the "
		"incorrect data in the way you would expect; and to be aware of incorrect data which "
		"other programs may handle differently. </li>\n"
		"</ul>\n"
		"<i>Please be aware that all software needs to respond to situations such as these; "
		"GPlates is simply informing you when these situations occur!<i>\n"
		"</body></html>\n");

const QString GPlatesQtWidgets::ReadErrorAccumulationDialog::s_information_dialog_title = QObject::tr(
		"Read error types");

void
GPlatesQtWidgets::ReadErrorAccumulationDialog::populate_top_level_tree_by_type(
		QTreeWidgetItem *tree_item_ptr,
		QString tree_item_text,
		const GPlatesFileIO::ReadErrorAccumulation::read_error_collection_type &errors,
		const QIcon &occurrence_icon)
{
	// Un-hide the top-level item if it has content to add, and update the text.
	if ( ! errors.empty()) {
		tree_item_ptr->setText(0, tree_item_text.arg(errors.size()));
		tree_item_ptr->setHidden(false);
		tree_item_ptr->setExpanded(true);
	}
	
	// Build map of Filename -> Error collection.
	errors_by_file_map_type errors_by_file;
	group_read_errors_by_file(errors_by_file, errors);
	
	// Iterate over map to add file errors of this type grouped by file.
	errors_by_file_map_const_iterator it = errors_by_file.begin();
	errors_by_file_map_const_iterator end = errors_by_file.end();
	for (; it != end; ++it) {
		build_file_tree_by_type(tree_item_ptr, it->second, occurrence_icon);
	}
}


void
GPlatesQtWidgets::ReadErrorAccumulationDialog::populate_top_level_tree_by_line(
		QTreeWidgetItem *tree_item_ptr,
		QString tree_item_text,
		const GPlatesFileIO::ReadErrorAccumulation::read_error_collection_type &errors,
		const QIcon &occurrence_icon)
{
	// Un-hide the top-level item if it has content to add, and update the text.
	if ( ! errors.empty()) {
		tree_item_ptr->setText(0, tree_item_text.arg(errors.size()));
		tree_item_ptr->setHidden(false);
		tree_item_ptr->setExpanded(true);
	}
	
	// Build map of Filename -> Error collection.
	errors_by_file_map_type errors_by_file;
	group_read_errors_by_file(errors_by_file, errors);
	
	// Iterate over map to add file errors of this type grouped by file.
	errors_by_file_map_const_iterator it = errors_by_file.begin();
	errors_by_file_map_const_iterator end = errors_by_file.end();
	for (; it != end; ++it) {
		build_file_tree_by_line(tree_item_ptr, it->second, occurrence_icon);
	}
}


void
GPlatesQtWidgets::ReadErrorAccumulationDialog::build_file_tree_by_type(
		QTreeWidgetItem *parent_item_ptr,
		const GPlatesFileIO::ReadErrorAccumulation::read_error_collection_type &errors,
		const QIcon &occurrence_icon)
{
	if (errors.empty()) {
		return;
	}
	// We must refer to the first entry to get the path info we need.
	const GPlatesFileIO::ReadErrorOccurrence &first_error = errors[0];
	
	QTreeWidgetItem *file_info_item = create_occurrence_file_info_item(first_error);
	
	file_info_item->addChild(create_occurrence_file_path_item(first_error));

	// Build map of Description (enum) -> Error collection.
	errors_by_type_map_type errors_by_type;
	group_read_errors_by_type(errors_by_type, errors);
	
	// Iterate over map to add file errors of this type grouped by description.
	errors_by_type_map_const_iterator it = errors_by_type.begin();
	errors_by_type_map_const_iterator end = errors_by_type.end();
	for (; it != end; ++it) {
		QTreeWidgetItem *summary_item = create_occurrence_type_summary_item(
				it->second[0], occurrence_icon, it->second.size());
		
		static const QIcon file_line_icon(":/gnome_edit_find_16.png");
		build_occurrence_line_list(summary_item, it->second, file_line_icon, false);
		
		file_info_item->addChild(summary_item);
		summary_item->setExpanded(false);
	}
	
	parent_item_ptr->addChild(file_info_item);
	file_info_item->setExpanded(true);	// setExpanded won't have effect until after addChild!
}


void
GPlatesQtWidgets::ReadErrorAccumulationDialog::build_file_tree_by_line(
		QTreeWidgetItem *parent_item_ptr,
		const GPlatesFileIO::ReadErrorAccumulation::read_error_collection_type &errors,
		const QIcon &occurrence_icon)
{
	if (errors.empty()) {
		return;
	}
	// We must refer to the first entry to get the path info we need.
	const GPlatesFileIO::ReadErrorOccurrence &first_error = errors[0];
	
	QTreeWidgetItem *file_info_item = create_occurrence_file_info_item(first_error);
	
	file_info_item->addChild(create_occurrence_file_path_item(first_error));

	build_occurrence_line_list(file_info_item, errors, occurrence_icon, true);
	
	parent_item_ptr->addChild(file_info_item);
	file_info_item->setExpanded(true);	// setExpanded won't have effect until after addChild!
}


void
GPlatesQtWidgets::ReadErrorAccumulationDialog::build_occurrence_line_list(
		QTreeWidgetItem *parent_item_ptr,
		const GPlatesFileIO::ReadErrorAccumulation::read_error_collection_type &errors,
		const QIcon &occurrence_icon,
		bool show_short_description)
{
	// Add all error occurrences for this file, for this error type.
	GPlatesFileIO::ReadErrorAccumulation::read_error_collection_const_iterator it = errors.begin();
	GPlatesFileIO::ReadErrorAccumulation::read_error_collection_const_iterator end = errors.end();
	for (; it != end; ++it) {
		// For each occurrence, add a Line node with Description and Result nodes as children.
		QTreeWidgetItem *location_item = create_occurrence_line_item(
				*it, occurrence_icon, show_short_description);
		
		location_item->addChild(create_occurrence_description_item(*it));
		location_item->addChild(create_occurrence_result_item(*it));

		parent_item_ptr->addChild(location_item);
		location_item->setExpanded(false);
	}
}



QTreeWidgetItem *
GPlatesQtWidgets::ReadErrorAccumulationDialog::create_occurrence_type_summary_item(
		const GPlatesFileIO::ReadErrorOccurrence &error,
		const QIcon &occurrence_icon,
		size_t quantity)
{
	// Create node with a summary of the error description and how many there are.
	QTreeWidgetItem *summary_item = new QTreeWidgetItem();
	summary_item->setText(0, QString("[%1] %2 (%3)")
			.arg(error.d_description)
			.arg(get_short_description_as_string(error.d_description))
			.arg(quantity) );
	summary_item->setIcon(0, occurrence_icon);
	
	return summary_item;
}

QTreeWidgetItem *
GPlatesQtWidgets::ReadErrorAccumulationDialog::create_occurrence_file_info_item(
		const GPlatesFileIO::ReadErrorOccurrence &error)
{
	static const QIcon file_icon(":/gnome_text_file_16.png");

	// Add the "filename.dat (format)" item.
	QTreeWidgetItem *file_item = new QTreeWidgetItem();
	std::ostringstream file_str;
	error.write_short_name(file_str);
	file_item->setText(0, QString::fromAscii(file_str.str().c_str()));
 	file_item->setIcon(0, file_icon);
	
	return file_item;
}

QTreeWidgetItem *
GPlatesQtWidgets::ReadErrorAccumulationDialog::create_occurrence_file_path_item(
		const GPlatesFileIO::ReadErrorOccurrence &error)
{
	static const QIcon path_icon(":/gnome_folder_16.png");

	// Add the full path item.
	QTreeWidgetItem *path_item = new QTreeWidgetItem();
	std::ostringstream path_str;
	error.d_data_source->write_full_name(path_str);
	path_item->setText(0, QString::fromAscii(path_str.str().c_str()));
 	path_item->setIcon(0, path_icon);
 	
 	return path_item;
}

QTreeWidgetItem *
GPlatesQtWidgets::ReadErrorAccumulationDialog::create_occurrence_line_item(
		const GPlatesFileIO::ReadErrorOccurrence &error,
		const QIcon &occurrence_icon,
		bool show_short_description)
{
	// Create node with a single line error occurrence, with a summary of the error description.
	QTreeWidgetItem *location_item = new QTreeWidgetItem();
	std::ostringstream location_str;
	error.d_location->write(location_str);
	if (show_short_description) {
		location_item->setText(0, QString("Line %1 [%2; %3] %4")
				.arg(QString::fromAscii(location_str.str().c_str()))
				.arg(error.d_description)
				.arg(error.d_result)
				.arg(get_short_description_as_string(error.d_description)) );
	} else {
		location_item->setText(0, QString("Line %1 [%2; %3]")
				.arg(QString::fromAscii(location_str.str().c_str()))
				.arg(error.d_description)
				.arg(error.d_result) );
	}
	location_item->setIcon(0, occurrence_icon);
	
	return location_item;
}

QTreeWidgetItem *
GPlatesQtWidgets::ReadErrorAccumulationDialog::create_occurrence_description_item(
		const GPlatesFileIO::ReadErrorOccurrence &error)
{
	static const QIcon description_icon(":/gnome_help_agent_16.png");

	// Create leaf node with full description.
	QTreeWidgetItem *description_item = new QTreeWidgetItem();
	description_item->setText(0, QString("[%1] %2")
			.arg(error.d_description)
			.arg(get_full_description_as_string(error.d_description)) );
	description_item->setIcon(0, description_icon);
	
	return description_item;
}

QTreeWidgetItem *
GPlatesQtWidgets::ReadErrorAccumulationDialog::create_occurrence_result_item(
		const GPlatesFileIO::ReadErrorOccurrence &error)
{
	static const QIcon result_icon(":/gnome_gtk_edit_16.png");

	// Create leaf node with result text.
	QTreeWidgetItem *result_item = new QTreeWidgetItem();
	result_item->setText(0, QString("[%1] %2")
			.arg(error.d_result)
			.arg(get_result_as_string(error.d_result)) );
	result_item->setIcon(0, result_icon);
	
	return result_item;
}



const QString
GPlatesQtWidgets::ReadErrorAccumulationDialog::build_summary_string()
{
	size_t num_failures = d_read_errors.d_failures_to_begin.size() +
			d_read_errors.d_terminating_errors.size();
	size_t num_recoverable_errors = d_read_errors.d_recoverable_errors.size();
	size_t num_warnings = d_read_errors.d_warnings.size();
	
	/*
	 * Firstly, work out what errors need to be summarised.
	 * The prefix of the sentence is affected by the quantity of the first error listed.
	 */
	QString prefix(tr("There were"));
	QString errors("");
	// Build sentence fragment for failures.
	if (num_failures > 0) {
		if (errors.isEmpty()) {
			errors.append(" ");
			if (num_failures <= 1) {
				prefix = tr("There was");
			}
		} else {
			errors.append(", ");
		}
		
		if (num_failures > 1) {
			errors.append(tr("%1 failures").arg(num_failures));
		} else {
			errors.append(tr("%1 failure").arg(num_failures));
		}
	}
	// Build sentence fragment for errors.
	if (num_recoverable_errors > 0) {
		if (errors.isEmpty()) {
			errors.append(" ");
			if (num_recoverable_errors <= 1) {
				prefix = tr("There was");
			}
		} else {
			errors.append(", ");
		}
		
		if (num_recoverable_errors > 1) {
			errors.append(tr("%1 errors").arg(num_recoverable_errors));
		} else {
			errors.append(tr("%1 error").arg(num_recoverable_errors));
		}
	}
	// Build sentence fragment for warnings.
	if (num_warnings > 0) {
		if (errors.isEmpty()) {
			errors.append(" ");
			if (num_warnings <= 1) {
				prefix = tr("There was");
			}
		} else {
			errors.append(", ");
		}
		
		if (num_warnings > 1) {
			errors.append(tr("%1 warnings").arg(num_warnings));
		} else {
			errors.append(tr("%1 warning").arg(num_warnings));
		}
	}
	// Build sentence fragment for no problems.
	if (errors.isEmpty()) {
		errors.append(tr(" no problems"));
	}
	
	// Finally, build the whole summary string.
	QString summary("");
	summary.append(prefix);
	summary.append(errors);
	summary.append(".");
	
	return summary;
}


const QString &
GPlatesQtWidgets::ReadErrorAccumulationDialog::get_short_description_as_string(
		GPlatesFileIO::ReadErrors::Description code)
{
	static const QString description_not_found = QObject::tr("(No error description found.)");
	static const description_map_type &map = build_short_description_map();
	
	description_map_const_iterator r = map.find(code);
	if (r != map.end()) {
		return r->second;
	} else {
		return description_not_found;
	}
}


const QString &
GPlatesQtWidgets::ReadErrorAccumulationDialog::get_full_description_as_string(
		GPlatesFileIO::ReadErrors::Description code)
{
	static const QString description_not_found = QObject::tr("(Text not found for error description code.)");
	static const description_map_type &map = build_full_description_map();
	
	description_map_const_iterator r = map.find(code);
	if (r != map.end()) {
		return r->second;
	} else {
		return description_not_found;
	}
}


const QString &
GPlatesQtWidgets::ReadErrorAccumulationDialog::get_result_as_string(
		GPlatesFileIO::ReadErrors::Result code)
{
	static const QString result_not_found = QObject::tr("(Text not found for error result code.)");
	static const result_map_type &map = build_result_map();
	
	result_map_const_iterator r = map.find(code);
	if (r != map.end()) {
		return r->second;
	} else {
		return result_not_found;
	}
}
