/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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
#include <QObject>
#include <QtGlobal>

#include "ReadErrorMessages.h"


//
// NOTE: This code was refactored out of ReadErrorAccumulationDialog so that
// it could also be accessed from the command-line interface (non-GUI) part of GPlates.
//


#define NUM_ELEMS(a) (sizeof(a) / sizeof((a)[0]))


namespace
{
	// Maps used for error text lookup.
	typedef std::map<GPlatesFileIO::ReadErrors::Description, QString> description_map_type;
	typedef description_map_type::const_iterator description_map_const_iterator;
	typedef std::map<GPlatesFileIO::ReadErrors::Result, QString> result_map_type;
	typedef result_map_type::const_iterator result_map_const_iterator;

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
		{ GPlatesFileIO::ReadErrors::NoValidGeometriesInPlatesFeature,
				QT_TR_NOOP("No valid geometries found in feature"),
				QT_TR_NOOP("This might be caused by all geometry points having pen-down ('3') codes.") },
		{ GPlatesFileIO::ReadErrors::InvalidMultipointGeometry,
				QT_TR_NOOP("Invalid multipoint geometry"),
				QT_TR_NOOP("A geometry expected to be a multipoint had an invalid geometry.") },				
		
		// Error descriptions for PLATES rotation-format files:
		{ GPlatesFileIO::ReadErrors::CommentMovingPlateIdAfterNonCommentSequence,
				QT_TR_NOOP("Detected commented-out pole within a sequence"),
				QT_TR_NOOP("The commented-out pole has the same fixed plate ID as the previous pole.") },
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
		{ GPlatesFileIO::ReadErrors::InvalidPoleLatitude,
				QT_TR_NOOP("Invalid latitude"),
				QT_TR_NOOP("The latitude of the pole was not in the range [-90, 90].") },
		{ GPlatesFileIO::ReadErrors::ErrorReadingPoleLongitude,
				QT_TR_NOOP("Error reading longitude"),
				QT_TR_NOOP("Error reading the pole longitude coordinate.") },
		{ GPlatesFileIO::ReadErrors::InvalidPoleLongitude,
				QT_TR_NOOP("Invalid longitude"),
				QT_TR_NOOP("The longitude of the pole was not in the range [-360, 360].") },
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
		{ GPlatesFileIO::ReadErrors::InvalidEnumerationValue,
				QT_TR_NOOP("Invalid enumeration value"),
				QT_TR_NOOP("The enumeration value is not in the list of supported values for the enumeration type.") },
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
		{ GPlatesFileIO::ReadErrors::UnexpectedStructuralElement,
				QT_TR_NOOP("Expected structural element not found"),
				QT_TR_NOOP("A structural element was found, but was not of the expected structural type.") },
		{ GPlatesFileIO::ReadErrors::UnexpectedPropertyStructuralElement,
				QT_TR_NOOP("Expected property structural element not found"),
				QT_TR_NOOP("A property's structural element was found, but was not one of its expected structural types.") },
		{ GPlatesFileIO::ReadErrors::PropertyNameNotRecognisedInFeatureType,
				QT_TR_NOOP("Property name does not belong to the feature type"),
				QT_TR_NOOP("A property name was found, but was not in the list of names associated with the feature's type.") },
		{ GPlatesFileIO::ReadErrors::TimeDependentPropertyStructuralElementNotFound,
				QT_TR_NOOP("Time-dependent property structural element not found"),
				QT_TR_NOOP("The property value is missing a time-dependent wrapper.") },
		{ GPlatesFileIO::ReadErrors::TimeDependentPropertyStructuralElementFound,
				QT_TR_NOOP("Time-dependent property structural element found"),
				QT_TR_NOOP("The property value should not have a time-dependent wrapper.") },
		{ GPlatesFileIO::ReadErrors::IncorrectTimeDependentPropertyStructuralElementFound,
				QT_TR_NOOP("Property structural element has incorrect time-dependent wrapper type"),
				QT_TR_NOOP("The property value has an unexpected type of time-dependent wrapper.") },
		{ GPlatesFileIO::ReadErrors::TooManyChildrenInElement,
				QT_TR_NOOP("Too many children in element"),
				QT_TR_NOOP("Found more child elements than were expected.") },
		{ GPlatesFileIO::ReadErrors::UnexpectedEmptyString,
				QT_TR_NOOP("Unexpected empty string"),
				QT_TR_NOOP("A blank string was encountered where a non-empty text value was expected.") },
		{ GPlatesFileIO::ReadErrors::UnrecognisedChildFound,
				QT_TR_NOOP("Unrecognised child found"),
				QT_TR_NOOP("An unrecognised XML child element was encountered.") },
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
		{ GPlatesFileIO::ReadErrors::MalformedVersionAttribute,
				QT_TR_NOOP("Malformed version attribute"),
				QT_TR_NOOP("The document GPML version string is malformed.") },
		{ GPlatesFileIO::ReadErrors::PartiallySupportedVersionAttribute,
				QT_TR_NOOP("Partially supported GPML version"),
				QT_TR_NOOP("The document was generated by a more recent version of GPlates.") },
		{ GPlatesFileIO::ReadErrors::ParseError,
				QT_TR_NOOP("Parse Error"),
				QT_TR_NOOP("Malformed XML was encountered.") },
		{ GPlatesFileIO::ReadErrors::UnexpectedNonEmptyAttributeList,
				QT_TR_NOOP("Unexpected attributes found"),
				QT_TR_NOOP("XML attributes were encountered on a Feature element where none were expected.") },
		{ GPlatesFileIO::ReadErrors::DuplicateRasterBandName,
				QT_TR_NOOP("Duplicate raster band name found"),
				QT_TR_NOOP("The list of band names in a raster Feature element contained duplicates.") },

		// The following descriptions are related to ESRI shapefile input errors:
		{ GPlatesFileIO::ReadErrors::NoLayersFoundInFile,
				QT_TR_NOOP("No layers found."),
				QT_TR_NOOP("No layers were found in the shapefile.") },
		{ GPlatesFileIO::ReadErrors::MultipleLayersInFile,
				QT_TR_NOOP("Multiple layers found."),
				QT_TR_NOOP("Multiple layers were found in the shapefile.") },
		{ GPlatesFileIO::ReadErrors::ErrorReadingOgrLayer,
				QT_TR_NOOP("Error reading layer."),
				QT_TR_NOOP("There was an error reading an OGR layer.") },
		{ GPlatesFileIO::ReadErrors::NoFeaturesFoundInOgrFile,
				QT_TR_NOOP("No features found."),
				QT_TR_NOOP("No features were found in the OGR file.") },
		{ GPlatesFileIO::ReadErrors::ErrorReadingOgrGeometry,
				QT_TR_NOOP("Error reading geometry."),
				QT_TR_NOOP("There was an error reading an OGR geometry.") },
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
		{ GPlatesFileIO::ReadErrors::InvalidOgrLatitude,
				QT_TR_NOOP("Invalid latitude."),
				QT_TR_NOOP("An invalid latitude was found.") },
		{ GPlatesFileIO::ReadErrors::InvalidOgrLongitude,
				QT_TR_NOOP("Invalid longitude."),
				QT_TR_NOOP("An invalid longitude was found.") },
		{ GPlatesFileIO::ReadErrors::NoPlateIdFound,
				QT_TR_NOOP("No Plate-id field."),
				QT_TR_NOOP("No Plate-id field was found for this file.") },
		{ GPlatesFileIO::ReadErrors::InvalidShapefilePlateIdNumber,
				QT_TR_NOOP("Invalid Plate-id."),
				QT_TR_NOOP("An invalid Plate-id was found.") },
		{ GPlatesFileIO::ReadErrors::UnrecognisedOgrFeatureType,
				QT_TR_NOOP("Unrecognised feature type."),
				QT_TR_NOOP("Unrecognised feature type found.") },
		{ GPlatesFileIO::ReadErrors::InvalidShapefileAgeOfAppearance,
				QT_TR_NOOP("Invalid age of appearance."),
				QT_TR_NOOP("An invalid age of appearance was found.") },
		{ GPlatesFileIO::ReadErrors::InvalidShapefileAgeOfDisappearance,
				QT_TR_NOOP("Invalid age of disappearance."),
				QT_TR_NOOP("An invalid age of disappearance was found.") },
		{ GPlatesFileIO::ReadErrors::InvalidShapefileConjugatePlateIdNumber,
				QT_TR_NOOP("Invalid conjugate Plate-id."),
				QT_TR_NOOP("An invalid conjugate Plate-id was found.") },
		{ GPlatesFileIO::ReadErrors::InvalidOgrPoint,
				QT_TR_NOOP("Invalid point."),
				QT_TR_NOOP("An invalid point geometry was found.") },
		{ GPlatesFileIO::ReadErrors::InvalidOgrMultiPoint,
				QT_TR_NOOP("Invalid multi-point."),
				QT_TR_NOOP("An invalid multi-point geometry was found.") },
		{ GPlatesFileIO::ReadErrors::InvalidOgrPolyline,
				QT_TR_NOOP("Invalid polyline."),
				QT_TR_NOOP("An invalid polyline geometry was found.") },
		{ GPlatesFileIO::ReadErrors::InvalidOgrPolygon,
				QT_TR_NOOP("Invalid polygon."),
				QT_TR_NOOP("An invalid polygon geometry was found.") },

		// Errors relating to raster files in general
		{ GPlatesFileIO::ReadErrors::InsufficientMemoryToLoadRaster,
				QT_TR_NOOP("Insufficient memory."),
				QT_TR_NOOP("There was insufficient memory to load the requested raster.\n"
					"Try loading a JPEG or netCDF/GMT gridded raster.\n"
					"These formats should not cause a memory allocation failure regardless of raster size.") },
		{ GPlatesFileIO::ReadErrors::ErrorGeneratingTexture,
				QT_TR_NOOP("Error generating texture."),
				QT_TR_NOOP("There was an error generating an OpenGL texture.") },
		{ GPlatesFileIO::ReadErrors::UnrecognisedRasterFileType,
				QT_TR_NOOP("Unrecognised raster file type."),
				QT_TR_NOOP("The raster file was of an unrecognised type.") },
		{ GPlatesFileIO::ReadErrors::ErrorReadingRasterFile,
				QT_TR_NOOP("Error reading raster file."),
				QT_TR_NOOP("An error was encountered while opening a raster file for reading.") },
		{ GPlatesFileIO::ReadErrors::ErrorReadingRasterBand,
				QT_TR_NOOP("Error reading raster band."),
				QT_TR_NOOP("An error was encountered while reading a band from a raster file.") },
		{ GPlatesFileIO::ReadErrors::InvalidRegionInRaster,
				QT_TR_NOOP("Invalid region in raster."),
				QT_TR_NOOP("The region requested from the raster exceeded the raster's boundaries.") },

		// Errors relating to GDAL-readable raster files
		{ GPlatesFileIO::ReadErrors::ErrorInSystemLibraries,
				QT_TR_NOOP("Error in system libraries."),
				QT_TR_NOOP("An error was encountered while using this system's version of GDAL to read the raster file. "
					"Upgrading GDAL or compiling GDAL from source may fix this error.") },

		// Errors relating to time-dependent raster file sets
		{ GPlatesFileIO::ReadErrors::NoRasterSetsFound,
				QT_TR_NOOP("No raster sets found."),
				QT_TR_NOOP("No suitable raster files were found in the selected folder.") },
		{ GPlatesFileIO::ReadErrors::MultipleRasterSetsFound,
				QT_TR_NOOP("Multiple raster sets found."),
				QT_TR_NOOP("More than one suitable raster file set was found in the selected folder.") },

		// Errors relating to importing 3D scalar field files
		{ GPlatesFileIO::ReadErrors::DepthLayerRasterIsNotNumerical,
				QT_TR_NOOP("Non-numerical depth layer found."),
				QT_TR_NOOP("Depth layer raster should contain numerical (non-RGB) values.") },

		// Errors relating to GMAP VGP files
		{ GPlatesFileIO::ReadErrors::GmapError,
				QT_TR_NOOP("Error reading GMAP file."),
				QT_TR_NOOP("Error reading GMAP file.") },	
		{ GPlatesFileIO::ReadErrors::GmapFieldFormatError,
				QT_TR_NOOP("Error reading GMAP field."),
				QT_TR_NOOP("There was an error reading a field in the GMAP file.") },

		// Errors relating to GMT CPT files
		{ GPlatesFileIO::ReadErrors::InvalidRegularCptLine,
				QT_TR_NOOP("Invalid regular CPT line."),
				QT_TR_NOOP("The line was not in a format expected in a regular CPT file.") },
		{ GPlatesFileIO::ReadErrors::InvalidCategoricalCptLine,
				QT_TR_NOOP("Invalid categorical CPT line."),
				QT_TR_NOOP("The line was not in a format expected in a categorical CPT file.") },
		{ GPlatesFileIO::ReadErrors::CptSliceNotMonotonicallyIncreasing,
				QT_TR_NOOP("CPT slice not monotonically increasing."),
				QT_TR_NOOP("The key or range of this line was not after the key or range of the previous line.") },
		{ GPlatesFileIO::ReadErrors::ColourModelChangedMidway,
				QT_TR_NOOP("Colour model changed midway."),
				QT_TR_NOOP("A comment to change the colour model to RGB or HSV was encountered after some lines had already been processed.") },
		{ GPlatesFileIO::ReadErrors::NoLinesSuccessfullyParsed,
				QT_TR_NOOP("No lines successfully parsed."),
				QT_TR_NOOP("No lines (except comments) could be parsed in the CPT file.") },
		{ GPlatesFileIO::ReadErrors::CptFileTypeNotDeduced,
				QT_TR_NOOP("CPT file type not deduced."),
				QT_TR_NOOP("The type of the CPT file (regular or categorical) could not be deduced.") },
		{ GPlatesFileIO::ReadErrors::UnrecognisedLabel,
				QT_TR_NOOP("Unrecognised Label"),
				QT_TR_NOOP("The label could not be parsed into the required data type.") },
		{ GPlatesFileIO::ReadErrors::PatternFillInLine,
				QT_TR_NOOP("Pattern fill in line."),
				QT_TR_NOOP("Pattern fills are not supported.") },

		// Generic file-related error descriptions:
		{ GPlatesFileIO::ReadErrors::ErrorOpeningFileForReading,
				QT_TR_NOOP("Error opening file."),
				QT_TR_NOOP("Error opening the file for reading.") },
		{ GPlatesFileIO::ReadErrors::FileIsEmpty,
				QT_TR_NOOP("File is empty."),
				QT_TR_NOOP("The file contains no data.") },
		{ GPlatesFileIO::ReadErrors::NoFeaturesFoundInFile,
				QT_TR_NOOP("No features in file."),
				QT_TR_NOOP("The file contains no features.") }
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
				QT_TR_NOOP("GPlates disabled the pole as expected and then continued the sequence.") },
		{ GPlatesFileIO::ReadErrors::NewOverlappingSequenceBegun,
				QT_TR_NOOP("A new sequence was begun which overlaps.") },
		{ GPlatesFileIO::ReadErrors::PoleDiscarded,
				QT_TR_NOOP("The pole was discarded.") },

		// Error results from GPML format files:
		// GPlatesFileIO::ReadErrors::ElementIgnored FIXME: unused.
		{ GPlatesFileIO::ReadErrors::ParsingStoppedPrematurely,
				QT_TR_NOOP("Parsing the file was stopped prematurely.") },
		{ GPlatesFileIO::ReadErrors::ElementNameChanged,
				QT_TR_NOOP("The name of the element was changed.") },
		{ GPlatesFileIO::ReadErrors::ElementNotNameChanged,
				QT_TR_NOOP("The name of the element was not changed.") },
		{ GPlatesFileIO::ReadErrors::AssumingCurrentVersion,
				QT_TR_NOOP("The current version will be assumed.") },
		{ GPlatesFileIO::ReadErrors::PropertyConvertedToTimeDependent,
				QT_TR_NOOP("A time-dependent wrapper was added to the property.") },
		{ GPlatesFileIO::ReadErrors::PropertyConvertedFromTimeDependent,
				QT_TR_NOOP("The time-dependent wrapper was removed from the property.") },
		{ GPlatesFileIO::ReadErrors::PropertyConvertedBetweenTimeDependentTypes,
				QT_TR_NOOP("The type of time-dependent property wrapper was changed.") },
		{ GPlatesFileIO::ReadErrors::PropertyNotInterpreted,
				QT_TR_NOOP("The property was not interpreted.") },
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
		{ GPlatesFileIO::ReadErrors::NoConjugatePlateIdLoadedForFeature,
				QT_TR_NOOP("No conjugate Plate-id was read for this feature.") },
		{ GPlatesFileIO::ReadErrors::AttributeIgnored,
				QT_TR_NOOP("The attribute was not mapped to a model property.") },
		{ GPlatesFileIO::ReadErrors::UnclassifiedOgrFeatureCreated,
				QT_TR_NOOP("An unclassifiedFeature was created.") },
				
		// The following apply to time-dependent raster file sets
		{ GPlatesFileIO::ReadErrors::NoRasterSetsLoaded,
				QT_TR_NOOP("No raster file set was loaded.") },
		{ GPlatesFileIO::ReadErrors::OnlyFirstRasterSetLoaded,
				QT_TR_NOOP("Only the first raster file set was loaded.") },
				
		// The following apply to GMAP vgp files.		
		{ GPlatesFileIO::ReadErrors::GmapFeatureIgnored,
				QT_TR_NOOP("The GMAP feature was ignored.") },				
		
		// The following apply to GMT CPT files
		{ GPlatesFileIO::ReadErrors::CptLineIgnored,
				QT_TR_NOOP("CPT line was ignored.") },

		// Generic file-related errors:
		{ GPlatesFileIO::ReadErrors::FileNotLoaded,
				QT_TR_NOOP("The file was not loaded.") },
		{ GPlatesFileIO::ReadErrors::FileNotImported,
				QT_TR_NOOP("The file was not imported.") },
		{ GPlatesFileIO::ReadErrors::NoAction,
				QT_TR_NOOP("No action was taken.") },
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
}


const QString &
GPlatesFileIO::ReadErrorMessages::get_short_description_as_string(
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
GPlatesFileIO::ReadErrorMessages::get_full_description_as_string(
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
GPlatesFileIO::ReadErrorMessages::get_result_as_string(
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
