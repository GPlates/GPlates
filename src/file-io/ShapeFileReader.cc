/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007, Geological Survey of Norway
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
#include <fstream>

#include <QString>
#include <QVariant>

#include "ogrsf_frmts.h"

#include "model/XmlAttributeName.h"
#include "model/XmlAttributeValue.h"

#include "property-values/GmlPoint.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/XsInteger.h"
#include "property-values/XsDouble.h"
#include "property-values/XsString.h"

#include "ErrorOpeningFileForReadingException.h"
#include "ShapeFileReader.h"


GPlatesFileIO::ShapeFileReader::ShapeFileReader():
	d_num_layers(0),
	d_data_source_ptr(NULL),
	d_geometry_ptr(NULL),
	d_feature_ptr(NULL),
	d_layer_ptr(NULL)
{
	OGRRegisterAll();
}

GPlatesFileIO::ShapeFileReader::~ShapeFileReader()
{
	if(d_data_source_ptr){
		OGRDataSource::DestroyDataSource(d_data_source_ptr);
	}
}

namespace
{
	/**
	 * This function is intended to replace the OGR macro 'wkbFlatten'.
	 *
	 * We need to replace OGR's 'wkbFlatten' because it uses an old-style cast, which causes
	 * G++ to complain.  'wkbFlatten' is #defined in the OGR header file "ogr_core.h".
	 * 
	 * Beware:  Copying code out of header files sucks.  As time passes, we'll need to verify
	 * that the code in this function still corresponds to the code in the macro.
	 */
	inline
	OGRwkbGeometryType
	wkb_flatten(
			OGRwkbGeometryType type)
	{
		// The definition of 'wkbFlatten' is currently:
		// #define wkbFlatten(x)  ((OGRwkbGeometryType) ((x) & (~wkb25DBit)))

		// The symbol 'wkb25DBit' is a macro constant which is #defined in "ogr_core.h".
		// Note that it's a little questionable to put the ~ operator *inside* the parens
		// (since this could result in unintended expression evaluation due to operator
		// precedence), but we'll copy OGR so that we'll get exactly the same behaviour,
		// unintended expression evaluation and all.
		return static_cast<OGRwkbGeometryType>(type & (~wkb25DBit));
	}
}

bool
GPlatesFileIO::ShapeFileReader::check_file_format(
	ReadErrorAccumulation &read_error)
{
	if (!d_data_source_ptr){
	// we should not be here
		return false;
	}

	boost::shared_ptr<GPlatesFileIO::DataSource> e_source(
		new GPlatesFileIO::LocalFileDataSource(d_filename, GPlatesFileIO::DataFormats::Shapefile));
	boost::shared_ptr<GPlatesFileIO::LocationInDataSource> e_location(
				new GPlatesFileIO::LineNumberInFile(0));

	d_num_layers = d_data_source_ptr->GetLayerCount();

	if (d_num_layers == 0){
		read_error.d_failures_to_begin.push_back(
			GPlatesFileIO::ReadErrorOccurrence(
				e_source,
				e_location,
				GPlatesFileIO::ReadErrors::NoLayersFoundInFile,
				GPlatesFileIO::ReadErrors::FileNotLoaded));

		return false;
	}
	if (d_num_layers > 1){
		read_error.d_warnings.push_back(
			GPlatesFileIO::ReadErrorOccurrence(
				e_source,
				e_location,
				GPlatesFileIO::ReadErrors::MultipleLayersInFile,
				GPlatesFileIO::ReadErrors::MultipleLayersIgnored));
	}

	d_layer_ptr = d_data_source_ptr->GetLayer(0);
	if (d_layer_ptr == NULL){
		read_error.d_failures_to_begin.push_back(
			GPlatesFileIO::ReadErrorOccurrence(
				e_source,
				e_location,
				GPlatesFileIO::ReadErrors::ErrorReadingShapefileLayer,
				GPlatesFileIO::ReadErrors::FileNotLoaded));
		return false;
	}
	d_feature_ptr = d_layer_ptr->GetNextFeature();
	if (d_feature_ptr == NULL){
		read_error.d_failures_to_begin.push_back(
			GPlatesFileIO::ReadErrorOccurrence(
				e_source,
				e_location,
				GPlatesFileIO::ReadErrors::NoFeaturesFoundInShapefile,
				GPlatesFileIO::ReadErrors::FileNotLoaded));
		return false;
	}

	d_layer_ptr->ResetReading();

	d_geometry_ptr = d_feature_ptr->GetGeometryRef();
	if (d_geometry_ptr == NULL){
		read_error.d_failures_to_begin.push_back(
			GPlatesFileIO::ReadErrorOccurrence(
				e_source,
				e_location,
				GPlatesFileIO::ReadErrors::ErrorReadingShapefileGeometry,
				GPlatesFileIO::ReadErrors::FileNotLoaded));
		return false;
	}
	d_type = d_geometry_ptr->getGeometryType();
	OGRwkbGeometryType flat_type;

	flat_type = wkb_flatten(d_type);

	if( d_type != flat_type){
		read_error.d_warnings.push_back(
			GPlatesFileIO::ReadErrorOccurrence(
				e_source,
				e_location,
				GPlatesFileIO::ReadErrors::TwoPointFiveDGeometryDetected,
				GPlatesFileIO::ReadErrors::GeometryFlattenedTo2D));
	}

	return true;
}

bool
GPlatesFileIO::ShapeFileReader::open_file(
		const QString &filename)
{
	char* fname = qstrdup(filename.toLatin1());
	
	d_data_source_ptr = OGRSFDriverRegistrar::Open(fname);
	if	(d_data_source_ptr == NULL){
		return false;
	}
	d_filename = filename;
	return true;
}



const GPlatesModel::FeatureCollectionHandle::weak_ref
GPlatesFileIO::ShapeFileReader::read_features(
	GPlatesModel::ModelInterface &model,
	ReadErrorAccumulation &read_errors)
{

	GPlatesModel::FeatureCollectionHandle::weak_ref collection
		= model.create_feature_collection();

	if (!d_layer_ptr){
		// we shouldn't really be here
		std::cerr << "null d_layer_ptr in read_features" << std::endl;
	}

	int feature_number = 0; // for error reporting.

	boost::shared_ptr<GPlatesFileIO::DataSource> e_source(
		new GPlatesFileIO::LocalFileDataSource(d_filename, GPlatesFileIO::DataFormats::Shapefile));

	while ((d_feature_ptr = d_layer_ptr->GetNextFeature()) != NULL){
	
		boost::shared_ptr<GPlatesFileIO::LocationInDataSource> e_location(
				new GPlatesFileIO::LineNumberInFile(feature_number));

		d_geometry_ptr = d_feature_ptr->GetGeometryRef();
		if (d_geometry_ptr == NULL){
			read_errors.d_recoverable_errors.push_back(
				GPlatesFileIO::ReadErrorOccurrence(
					e_source,
					e_location,
					GPlatesFileIO::ReadErrors::ErrorReadingShapefileGeometry,
					GPlatesFileIO::ReadErrors::GeometryIgnored));
			continue;
		}

		d_type = d_geometry_ptr->getGeometryType();
		switch (d_type){
		case wkbPoint:
			{
				handle_point(model,collection,read_errors,e_source,e_location);
			}
			break;
		case wkbMultiPoint:
			{
				handle_multi_point(model,collection,read_errors,e_source,e_location);
			}
			break;
		case wkbLineString:
			{
				handle_linestring(model,collection,read_errors,e_source,e_location);
			}
			break;
		case wkbMultiLineString:
			{
				handle_multi_linestring(model,collection,read_errors,e_source,e_location);
			}
			break;
		case wkbPolygon:
			{
				handle_polygon(model,collection,read_errors,e_source,e_location);
			}
			break;
		case wkbMultiPolygon:
			{
				handle_multi_polygon(model,collection,read_errors,e_source,e_location);
			}
			break;
		default:
			{
				std::cerr << "Unsupported geometry type." << std::endl;
				read_errors.d_recoverable_errors.push_back(
				GPlatesFileIO::ReadErrorOccurrence(
					e_source,
					e_location,
					GPlatesFileIO::ReadErrors::UnsupportedGeometryType,
					GPlatesFileIO::ReadErrors::GeometryIgnored));
			}
		} // switch

		OGRFeature::DestroyFeature(d_feature_ptr);
		feature_number++;
	} // while

	return collection;
}


GPlatesModel::FeatureHandle::weak_ref
GPlatesFileIO::ShapeFileReader::create_line_feature_from_list(
	GPlatesModel::ModelInterface &model,
	GPlatesModel::FeatureCollectionHandle::weak_ref collection,
	std::list<GPlatesMaths::PointOnSphere> &list_of_points)
{

	GPlatesModel::FeatureType feature_type("gpml:UnclassifiedFeature");


	GPlatesModel::FeatureHandle::weak_ref feature_handle =
			model.create_feature(feature_type,collection);

	GPlatesMaths::PolylineOnSphere::non_null_ptr_type polyline =
		GPlatesMaths::PolylineOnSphere::create_on_heap(list_of_points);

	GPlatesPropertyValues::GmlLineString::non_null_ptr_type gml_line_string =
		GPlatesPropertyValues::GmlLineString::create(polyline);

	GPlatesModel::ModelUtils::append_property_value_to_feature(gml_line_string,
		"gml:centerLineOf",feature_handle);

	return feature_handle;
}


GPlatesModel::FeatureHandle::weak_ref
GPlatesFileIO::ShapeFileReader::create_point_feature_from_pair(
	GPlatesModel::ModelInterface &model, 
	GPlatesModel::FeatureCollectionHandle::weak_ref collection, 
	std::pair<double,double> &point)
{
	GPlatesModel::FeatureType feature_type("gpml:UnclassifiedFeature");

	GPlatesModel::FeatureHandle::weak_ref feature_handle =
			model.create_feature(feature_type,collection);

	const GPlatesModel::PropertyValue::non_null_ptr_type gmlpoint =
		GPlatesPropertyValues::GmlPoint::create(point);

	GPlatesModel::ModelUtils::append_property_value_to_feature(
		gmlpoint,
		"gml:position",
		feature_handle);

	return feature_handle;

}


GPlatesModel::FeatureHandle::weak_ref
GPlatesFileIO::ShapeFileReader::create_point_feature_from_point_on_sphere(
	GPlatesModel::ModelInterface &model, 
	GPlatesModel::FeatureCollectionHandle::weak_ref collection, 
	GPlatesMaths::PointOnSphere &point)
{
	GPlatesModel::FeatureType feature_type("gpml:UnclassifiedFeature");

	GPlatesModel::FeatureHandle::weak_ref feature_handle =
			model.create_feature(feature_type,collection);

	const GPlatesModel::PropertyValue::non_null_ptr_type gmlpoint =
		GPlatesPropertyValues::GmlPoint::create(point);

	GPlatesModel::ModelUtils::append_property_value_to_feature(
		gmlpoint,
		"gml:position",
		feature_handle);

	return feature_handle;

}


void 
GPlatesFileIO::ShapeFileReader::get_field_names(
	ReadErrorAccumulation &read_errors)
{
	boost::shared_ptr<GPlatesFileIO::DataSource> e_source(
		new GPlatesFileIO::LocalFileDataSource(d_filename, GPlatesFileIO::DataFormats::Shapefile));

	boost::shared_ptr<GPlatesFileIO::LocationInDataSource> e_location(
		new GPlatesFileIO::LineNumberInFile(0));
				
	d_field_names.clear();
	if(!d_feature_ptr){
		return;
	}
	OGRFeatureDefn *feature_def_ptr = d_layer_ptr->GetLayerDefn();
	int num_fields = feature_def_ptr->GetFieldCount();
	int count;
	bool plateid_field_found = false;
	for(count=0; count< num_fields; count++)
	{
		OGRFieldDefn *field_def_ptr = feature_def_ptr->GetFieldDefn(count);
		d_field_names[count] = QString::fromLocal8Bit(field_def_ptr->GetNameRef());
		if (strcmp(field_def_ptr->GetNameRef(),"PLATEID1")==0){
			plateid_field_found = true;
		}
	}

	if (!plateid_field_found){
		read_errors.d_warnings.push_back(
			GPlatesFileIO::ReadErrorOccurrence(
				e_source,
				e_location,
				GPlatesFileIO::ReadErrors::NoPlateIdFound,
				GPlatesFileIO::ReadErrors::NoPlateIdLoadedForFile));
	}
}

void 
GPlatesFileIO::ShapeFileReader::get_attributes()
{
	d_attributes.clear();
	if (!d_feature_ptr){
		return;
	}
	OGRFeatureDefn *feature_def_ptr = d_layer_ptr->GetLayerDefn();
	int num_fields = feature_def_ptr->GetFieldCount();
	int count;
	for (count=0; count< num_fields; count++){
		OGRFieldDefn *field_def_ptr = feature_def_ptr->GetFieldDefn(count);
		if(field_def_ptr->GetType()==OFTInteger)
			d_attributes.push_back(QVariant(d_feature_ptr->GetFieldAsInteger(count)));
		if(field_def_ptr->GetType()==OFTReal)
			d_attributes.push_back(QVariant(d_feature_ptr->GetFieldAsDouble(count)));
		if(field_def_ptr->GetType()==OFTString)
			d_attributes.push_back(QVariant(QObject::tr(d_feature_ptr->GetFieldAsString(count))));
	}
}

void
GPlatesFileIO::ShapeFileReader::add_attributes_to_feature(
	GPlatesModel::FeatureHandle::weak_ref feature_handle,
	GPlatesFileIO::ReadErrorAccumulation &read_errors,
	const boost::shared_ptr<GPlatesFileIO::DataSource> &source,
	const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> &location)
{

	bool plate_id_found = false;
	int n = d_attributes.size();
	for (int count = 0; count < n ; count++){
		QString fieldname = d_field_names[count];
		QVariant attribute = d_attributes[count];

		const char *sfieldname = qstrdup(fieldname.toLatin1());
		
		QVariant::Type type_ = attribute.type();
		switch(type_){
			case QVariant::Int:
				{
				int i = attribute.toInt();
				GPlatesPropertyValues::XsInteger::non_null_ptr_type value = GPlatesPropertyValues::XsInteger::create(i);
				GPlatesModel::ModelUtils::append_property_value_to_feature(value,sfieldname,"source","shapefile",feature_handle);
				}
				break;
			case QVariant::Double:
				{
				double d = attribute.toDouble();
				GPlatesPropertyValues::XsDouble::non_null_ptr_type value = GPlatesPropertyValues::XsDouble::create(d);
				GPlatesModel::ModelUtils::append_property_value_to_feature(value,sfieldname,"source","shapefile",feature_handle);
				}
				break;
			case QVariant::String:
				{
				std::string s = attribute.toString().toStdString();
				GPlatesPropertyValues::XsString::non_null_ptr_type value = GPlatesPropertyValues::XsString::create(UnicodeString(s.c_str()));
				GPlatesModel::ModelUtils::append_property_value_to_feature(value,sfieldname,"source","shapefile",feature_handle);	
				}
				break;
			default:
				break;
		}

		if (strcmp(sfieldname,"PLATEID1")==0){
			if(plate_id_found){
				std::cerr << "multiple plate ID attributes found" << std::endl;
			}
			else{
				if (type_ == QVariant::String){
					read_errors.d_warnings.push_back(
						GPlatesFileIO::ReadErrorOccurrence(
							source,
							location,
							GPlatesFileIO::ReadErrors::InvalidShapefilePlateIdNumber,
							GPlatesFileIO::ReadErrors::NoPlateIdLoadedForFeature));
				}
				else{
					GPlatesPropertyValues::GpmlPlateId::non_null_ptr_type plate_id = 
						GPlatesPropertyValues::GpmlPlateId::create(attribute.toInt());
					GPlatesModel::ModelUtils::append_property_value_to_feature(
						GPlatesModel::ModelUtils::create_gpml_constant_value(plate_id,"gpml:plateId"),
						"gpml:reconstructionPlateId",
						feature_handle);
				}
				plate_id_found = true;
			}

		} // end if (strcmp(sfieldname,"PLATEID")==0)


	} // for loop over number of attributes

	if (!plate_id_found){
		std::cerr << "No plate ID attribute found." << std::endl; 
	}


}


bool
GPlatesFileIO::ShapeFileReader::is_valid_shape_data(
		double lat,
		double lon,
		ReadErrorAccumulation &read_errors,
		const boost::shared_ptr<GPlatesFileIO::DataSource> &source,
		const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> &location)
{

	if (lat < SHAPE_NO_DATA){
		read_errors.d_recoverable_errors.push_back(
			GPlatesFileIO::ReadErrorOccurrence(
			source,
			location,
			GPlatesFileIO::ReadErrors::NoLatitudeShapeData,
			GPlatesFileIO::ReadErrors::GeometryIgnored));
		return false;
	}			
	if (lon < SHAPE_NO_DATA){
		read_errors.d_recoverable_errors.push_back(
			GPlatesFileIO::ReadErrorOccurrence(
			source,
			location,
			GPlatesFileIO::ReadErrors::NoLongitudeShapeData,
			GPlatesFileIO::ReadErrors::GeometryIgnored));
		return false;
	}

	if (!GPlatesMaths::LatLonPoint::is_valid_latitude(lat)){
		read_errors.d_recoverable_errors.push_back(
			GPlatesFileIO::ReadErrorOccurrence(
			source,
			location,
			GPlatesFileIO::ReadErrors::InvalidShapefileLatitude,
			GPlatesFileIO::ReadErrors::GeometryIgnored));
		return false;
	}

	if (!GPlatesMaths::LatLonPoint::is_valid_longitude(lon)){
		read_errors.d_recoverable_errors.push_back(
			GPlatesFileIO::ReadErrorOccurrence(
			source,
			location,
			GPlatesFileIO::ReadErrors::InvalidShapefileLongitude,
			GPlatesFileIO::ReadErrors::GeometryIgnored));
		return false;
	}

	return true;
}

const GPlatesModel::FeatureCollectionHandle::weak_ref
GPlatesFileIO::ShapeFileReader::read_file(
		FileInfo &fileinfo,
		GPlatesModel::ModelInterface &model,
		ReadErrorAccumulation &read_errors)
{
	QString filename = fileinfo.get_qfileinfo().absoluteFilePath();

	ShapeFileReader reader;
	if (!reader.open_file(filename)){
		throw ErrorOpeningFileForReadingException(filename);
	}

	if(!reader.check_file_format(read_errors)){
		throw ErrorOpeningFileForReadingException(filename);
	}

	reader.get_field_names(read_errors);

	GPlatesModel::FeatureCollectionHandle::weak_ref collection
		= model.create_feature_collection();

	collection = reader.read_features(model,read_errors);
	fileinfo.set_feature_collection(collection);
	return collection;
}

void
GPlatesFileIO::ShapeFileReader::handle_point(
		GPlatesModel::ModelInterface &model,
		GPlatesModel::FeatureCollectionHandle::weak_ref collection,
		ReadErrorAccumulation &read_errors,
		const boost::shared_ptr<GPlatesFileIO::DataSource> &source,
		const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> &location)
{
			std::cout << "point type" << std::endl;
			OGRPoint *point = static_cast<OGRPoint*>(d_geometry_ptr);
			double lat,lon;
			lat = point->getY();
			lon = point->getX();

			
			if (is_valid_shape_data(lat,lon,read_errors,source,location)){
				GPlatesMaths::LatLonPoint llp(lat,lon);
				GPlatesMaths::PointOnSphere p = GPlatesMaths::make_point_on_sphere(llp);
				GPlatesModel::FeatureHandle::weak_ref feature_handle = create_point_feature_from_point_on_sphere(model,collection,p);
				get_attributes();
				add_attributes_to_feature(feature_handle,read_errors,source,location);
			}

}

void
GPlatesFileIO::ShapeFileReader::handle_multi_point(
			GPlatesModel::ModelInterface &model,
			GPlatesModel::FeatureCollectionHandle::weak_ref collection,
			ReadErrorAccumulation &read_errors,
			const boost::shared_ptr<GPlatesFileIO::DataSource> &source,
			const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> &location)
{
		std::cout << "multi point type" << std::endl;
		OGRMultiPoint *multi = static_cast<OGRMultiPoint*>(d_geometry_ptr);
		int n = multi->getNumGeometries();
		for(int count = 0; count < n; count++)
		{
			OGRPoint* point = static_cast<OGRPoint*>(multi->getGeometryRef(count));
			double lat,lon;
			lat = point->getY();
			lon = point->getX();

			if (is_valid_shape_data(lat,lon,read_errors,source,location)){
				GPlatesMaths::LatLonPoint llp(lat,lon);
				GPlatesMaths::PointOnSphere p = GPlatesMaths::make_point_on_sphere(llp);
				GPlatesModel::FeatureHandle::weak_ref feature_handle = create_point_feature_from_point_on_sphere(model,collection,p);
				get_attributes();
				add_attributes_to_feature(feature_handle,read_errors,source,location);
			}
		} // for
}

void
GPlatesFileIO::ShapeFileReader::handle_linestring(
			GPlatesModel::ModelInterface &model,
			GPlatesModel::FeatureCollectionHandle::weak_ref collection,
			ReadErrorAccumulation &read_errors,
			const boost::shared_ptr<GPlatesFileIO::DataSource> &source,
			const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> &location
			)
{


	std::list<GPlatesMaths::PointOnSphere> feature_points;			
	OGRLineString *linestring = static_cast<OGRLineString*>(d_geometry_ptr);
	int num_points = linestring->getNumPoints();
	if (num_points < 2){	
		std::cerr << "nPoints is " << num_points << ": ignoring feature" << std::endl;
		read_errors.d_recoverable_errors.push_back(
			GPlatesFileIO::ReadErrorOccurrence(
			source,
			location,
			GPlatesFileIO::ReadErrors::LessThanTwoPointsInLineString,
			GPlatesFileIO::ReadErrors::GeometryIgnored));
			return;
	}
	int count;
	double lon,lat;
	for (count = 0; count < num_points; count ++){
		lon = linestring->getX(count);
		lat = linestring->getY(count);
		if (is_valid_shape_data(lat,lon,read_errors,source,location)){
			GPlatesMaths::LatLonPoint llp(lat,lon);
			feature_points.push_back(GPlatesMaths::make_point_on_sphere(llp));
		}

	}

	GPlatesModel::FeatureHandle::weak_ref feature_handle = create_line_feature_from_list(model,collection,feature_points);
	get_attributes();
	add_attributes_to_feature(feature_handle,read_errors,source,location);
}

void
GPlatesFileIO::ShapeFileReader::handle_multi_linestring(
			GPlatesModel::ModelInterface &model,
			GPlatesModel::FeatureCollectionHandle::weak_ref collection,
			ReadErrorAccumulation &read_errors,
			const boost::shared_ptr<GPlatesFileIO::DataSource> &source,
			const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> &location)
{
		std::cerr << "Multi Line " << std::endl;
		OGRMultiLineString *multi = static_cast<OGRMultiLineString*>(d_geometry_ptr);
		int num_geom = multi->getNumGeometries();
		std::cerr << num_geom << " geometries" << std::endl;
		for (int multiCount = 0; multiCount < num_geom ; multiCount++){

			std::list<GPlatesMaths::PointOnSphere> feature_points;
			OGRLineString *linestring = static_cast<OGRLineString*>(multi->getGeometryRef(multiCount));
		
			int num_points = linestring->getNumPoints();
			if (num_points < 2){	
				std::cerr << "nPoints is " << num_points << ": ignoring feature" << std::endl;
				read_errors.d_recoverable_errors.push_back(
					GPlatesFileIO::ReadErrorOccurrence(
							source,
							location,
							GPlatesFileIO::ReadErrors::LessThanTwoPointsInLineString,
							GPlatesFileIO::ReadErrors::GeometryIgnored));
				continue;
			}
			int count;
			double lon,lat;
			for (count = 0; count < num_points; count++){
				lon = linestring->getX(count);
				lat = linestring->getY(count);
				if (is_valid_shape_data(lat,lon,read_errors,source,location)){
					GPlatesMaths::LatLonPoint llp(lat,lon);
					feature_points.push_back(GPlatesMaths::make_point_on_sphere(llp));
				}
			}

			GPlatesModel::FeatureHandle::weak_ref feature_handle = create_line_feature_from_list(model,collection,feature_points);				
			get_attributes();
			add_attributes_to_feature(feature_handle,read_errors,source,location);
		} // for loop over multilines
}

void
GPlatesFileIO::ShapeFileReader::handle_polygon(
			GPlatesModel::ModelInterface &model,
			GPlatesModel::FeatureCollectionHandle::weak_ref collection,
			ReadErrorAccumulation &read_errors,
			const boost::shared_ptr<GPlatesFileIO::DataSource> &source,
			const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> &location)
{
	std::cerr << "Polygon" << std::endl;
	std::list<GPlatesMaths::PointOnSphere> feature_points;
	OGRPolygon *polygon = static_cast<OGRPolygon*>(d_geometry_ptr);
	if (polygon->getNumInteriorRings() != 0){
		std::cerr << "Interior rings detected, which will be ignored." << std::endl;
		read_errors.d_recoverable_errors.push_back(
			GPlatesFileIO::ReadErrorOccurrence(
			source,
			location,
			GPlatesFileIO::ReadErrors::InteriorRingsInShapefile,
			GPlatesFileIO::ReadErrors::OnlyExteriorRingRead));
	}

	OGRLinearRing *ring = polygon->getExteriorRing();
	int num_points = ring->getNumPoints();
	if (num_points < 2){
		std::cerr << "nPoints is " << num_points << ": ignoring feature" << std::endl;
		read_errors.d_recoverable_errors.push_back(
			GPlatesFileIO::ReadErrorOccurrence(
			source,
			location,
			GPlatesFileIO::ReadErrors::LessThanTwoPointsInLineString,
			GPlatesFileIO::ReadErrors::GeometryIgnored));
		return;
	}

	int count;
	double lat,lon;
	for (count = 0; count < num_points; count++){
		lon = ring->getX(count);
		lat = ring->getY(count);
		if (is_valid_shape_data(lat,lon,read_errors,source,location)){
			GPlatesMaths::LatLonPoint llp(lat,lon);
			feature_points.push_back(GPlatesMaths::make_point_on_sphere(llp));
		}
	}
	// for now polygon geometries are stored in the model as linestrings

	GPlatesModel::FeatureHandle::weak_ref feature_handle = create_line_feature_from_list(model,collection,feature_points);				
	get_attributes();
	add_attributes_to_feature(feature_handle,read_errors,source,location);
}

void
GPlatesFileIO::ShapeFileReader::handle_multi_polygon(
			GPlatesModel::ModelInterface &model,
			GPlatesModel::FeatureCollectionHandle::weak_ref collection,
			ReadErrorAccumulation &read_errors,
			const boost::shared_ptr<GPlatesFileIO::DataSource> &source,
			const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> &location)
{
	std::cerr << "MultiPolygon" << std::endl;
	OGRMultiPolygon *multi = static_cast<OGRMultiPolygon*>(d_geometry_ptr);
	int n = multi->getNumGeometries();
	std::cerr << n << " geometries" << std::endl;

	for (int multiCount = 0; multiCount < n ; multiCount++){
		std::list<GPlatesMaths::PointOnSphere> feature_points;
		OGRPolygon *polygon = static_cast<OGRPolygon*>(multi->getGeometryRef(multiCount));

		if (polygon->getNumInteriorRings() != 0){
			std::cerr << "Interior rings detected, which will be ignored." << std::endl;
			read_errors.d_warnings.push_back(
				GPlatesFileIO::ReadErrorOccurrence(
				source,
				location,
				GPlatesFileIO::ReadErrors::InteriorRingsInShapefile,
				GPlatesFileIO::ReadErrors::OnlyExteriorRingRead));
		}

		OGRLinearRing *ring = polygon->getExteriorRing();
		int num_points = ring->getNumPoints();

		if (num_points < 2){
			std::cerr << "nPoints is " << num_points << ": ignoring feature" << std::endl;
			read_errors.d_recoverable_errors.push_back(
				GPlatesFileIO::ReadErrorOccurrence(
				source,
				location,
				GPlatesFileIO::ReadErrors::LessThanTwoPointsInLineString,
				GPlatesFileIO::ReadErrors::GeometryIgnored));
			continue;
		}

		int count;
		double lon,lat;
		for (count = 0; count < num_points; count++){
			lon = ring->getX(count);
			lat = ring->getY(count);

			if (is_valid_shape_data(lat,lon,read_errors,source,location)){
				GPlatesMaths::LatLonPoint llp(lat,lon);
				feature_points.push_back(GPlatesMaths::make_point_on_sphere(llp));
			}

		}

		// for now polygon geometries are handled as linestrings.
		GPlatesModel::FeatureHandle::weak_ref feature_handle = create_line_feature_from_list(model,collection,feature_points);				
		get_attributes();
		add_attributes_to_feature(feature_handle,read_errors,source,location);			
	} // for loop over multipolygons
}

