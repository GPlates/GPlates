/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007, 2008, 2009 Geological Survey of Norway
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
#include <boost/optional.hpp>
#include <QDebug>
#include <QMessageBox>
#include <QString>
#include <QStringList>
#include <QVariant>

#include "ErrorOpeningFileForReadingException.h"
#include "FileLoadAbortedException.h"
#include "PropertyMapper.h"
#include "ShapefileReader.h"
#include "ShapefileUtils.h"
#include "ShapefileXmlReader.h"

#include "feature-visitors/PropertyValueFinder.h" 
#include "feature-visitors/ShapefileAttributeFinder.h"

#include "model/Model.h"
#include "model/ModelUtils.h"
#include "model/QualifiedXmlName.h"
#include "model/XmlAttributeName.h"
#include "model/XmlAttributeValue.h"

#include "property-values/GeoTimeInstant.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlPolygon.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlKeyValueDictionary.h"
#include "property-values/GpmlKeyValueDictionaryElement.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/XsInteger.h"
#include "property-values/XsDouble.h"
#include "property-values/XsString.h"

#include "maths/LatLonPoint.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/PolylineOnSphere.h"
#include "maths/PolygonOnSphere.h"



boost::shared_ptr< GPlatesFileIO::PropertyMapper> GPlatesFileIO::ShapefileReader::s_property_mapper;

QMap<QString,QString> GPlatesFileIO::ShapefileReader::s_model_to_attribute_map;

QStringList GPlatesFileIO::ShapefileReader::s_field_names;
	
namespace
{


	/**
	 * Creates a gml line string from @a list_of_points and adds this to @a feature.
	 */
	void
	add_polyline_geometry_to_feature(
		const GPlatesModel::FeatureHandle::weak_ref &feature,
		std::list<GPlatesMaths::PointOnSphere> &list_of_points)
	{
		GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline =
			GPlatesMaths::PolylineOnSphere::create_on_heap(list_of_points);

		GPlatesPropertyValues::GmlLineString::non_null_ptr_type gml_line_string =
			GPlatesPropertyValues::GmlLineString::create(polyline);

		GPlatesPropertyValues::GmlOrientableCurve::non_null_ptr_type gml_orientable_curve =
			GPlatesModel::ModelUtils::create_gml_orientable_curve(gml_line_string);

		GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type property_value =
			GPlatesModel::ModelUtils::create_gpml_constant_value(
			gml_orientable_curve, 
			GPlatesPropertyValues::TemplateTypeParameterType::create_gml("OrientableCurve"));

		GPlatesModel::ModelUtils::append_property_value_to_feature(property_value,
			//					GPlatesModel::PropertyName::create_gpml("centerLineOf"), feature);
			GPlatesModel::PropertyName::create_gpml("unclassifiedGeometry"), feature);

	}

	/**
	* Creates a gml polygon from @a list_of_points and adds this to @a feature.
	*/
	void
	add_polygon_geometry_to_feature(
		const GPlatesModel::FeatureHandle::weak_ref &feature,
		std::list<GPlatesMaths::PointOnSphere> &list_of_points)
	{
		GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon =
			GPlatesMaths::PolygonOnSphere::create_on_heap(list_of_points);

		GPlatesPropertyValues::GmlPolygon::non_null_ptr_type gml_polygon =
			GPlatesPropertyValues::GmlPolygon::create(polygon);


		GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type property_value =
			GPlatesModel::ModelUtils::create_gpml_constant_value(
			gml_polygon, 
			GPlatesPropertyValues::TemplateTypeParameterType::create_gml("Polygon"));

		// Anything that's got a polygon geometry is going to get an "unclassifiedGeometry" property name. 
		GPlatesModel::ModelUtils::append_property_value_to_feature(property_value,
			GPlatesModel::PropertyName::create_gpml("unclassifiedGeometry"), feature);

	}

	/**
	 * Creates a feature of type specified in @a feature_creation_pair, and adds it to @a collection. 
	 *
	 * @return a feature handle to the created feature. 
	 */
	const GPlatesModel::FeatureHandle::weak_ref
	create_feature(
		GPlatesModel::ModelInterface &model,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
		QString &feature_type_qstring,
		boost::optional<UnicodeString> &feature_id)
	{


		GPlatesModel::FeatureType feature_type = 
			GPlatesModel::FeatureType::create_gpml(feature_type_qstring);

		if (feature_id)
		{
			return model->create_feature(feature_type,GPlatesModel::FeatureId(*feature_id),collection);		
		}
		else
		{
			return model->create_feature(feature_type,collection);		
		}
	}

	/**
	 * Returns a QVariant representing the @a shapefile_property_name from the 
	 * @a feature_handle's shapefile-attribute key-value-dictionary.
	 * 
	 */
	QVariant
	get_qvariant_from_finder(
		QString shapefile_property_name,
		const GPlatesModel::FeatureHandle::weak_ref &feature)
	{
		GPlatesFeatureVisitors::ShapefileAttributeFinder finder(shapefile_property_name);

		finder.visit_feature(feature);
		if (finder.found_qvariants_begin() != finder.found_qvariants_end())
		{
			return *(finder.found_qvariants_begin());
		}
		else
		{
			return QVariant();
		}

	}
		

	void
	append_plate_id_to_feature(
		const GPlatesModel::FeatureHandle::weak_ref &feature,
		int plate_id_as_int)
	{
		GPlatesPropertyValues::GpmlPlateId::non_null_ptr_type plate_id = 
			GPlatesPropertyValues::GpmlPlateId::create(plate_id_as_int);
		GPlatesModel::ModelUtils::append_property_value_to_feature(
			GPlatesModel::ModelUtils::create_gpml_constant_value(plate_id,
			GPlatesPropertyValues::TemplateTypeParameterType::create_gpml("plateId")),
			GPlatesModel::PropertyName::create_gpml("reconstructionPlateId"),
			feature);
	}

	void
	append_geo_time_to_feature(
		const GPlatesModel::FeatureHandle::weak_ref &feature,
		double age_of_appearance,
		double age_of_disappearance)
	{
		const GPlatesPropertyValues::GeoTimeInstant geo_time_instant_begin(age_of_appearance);
		const GPlatesPropertyValues::GeoTimeInstant geo_time_instant_end(age_of_disappearance);

		GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type gml_valid_time = 
			GPlatesModel::ModelUtils::create_gml_time_period(geo_time_instant_begin,
			geo_time_instant_end);
		GPlatesModel::ModelUtils::append_property_value_to_feature(gml_valid_time, 
			GPlatesModel::PropertyName::create_gml("validTime"),
			feature);
	}

	void
	append_name_to_feature(
		const GPlatesModel::FeatureHandle::weak_ref &feature,
		QString name)
	{
		GPlatesPropertyValues::XsString::non_null_ptr_type gml_description = 
			GPlatesPropertyValues::XsString::create(UnicodeString(name.toStdString().c_str()));
		GPlatesModel::ModelUtils::append_property_value_to_feature(
			gml_description, 
			GPlatesModel::PropertyName::create_gml("name"), 
			feature);
	}

	void
	append_description_to_feature(
		const GPlatesModel::FeatureHandle::weak_ref &feature,
		QString description)
	{
		GPlatesPropertyValues::XsString::non_null_ptr_type gml_description = 
			GPlatesPropertyValues::XsString::create(UnicodeString(description.toStdString().c_str()));
		GPlatesModel::ModelUtils::append_property_value_to_feature(
			gml_description, 
			GPlatesModel::PropertyName::create_gml("description"), 
			feature);
	}

	/**
	 * Removes properties with the property names:
	 *		reconstructionPlateId,
	 *		validTime,
	 *		description  and
	 *		name
	 *	from the feature given by @a feature_handle.
	 */
	void
	remove_old_properties(
		const GPlatesModel::FeatureHandle::weak_ref &feature)
	{

		QStringList property_name_list;
		property_name_list << QString("reconstructionPlateId");
		property_name_list << QString("validTime"); 
		property_name_list << QString("description"); 
		property_name_list << QString("name"); 

		GPlatesModel::FeatureHandle::children_iterator p_iter = feature->children_begin();
		GPlatesModel::FeatureHandle::children_iterator p_iter_end = feature->children_end();

		for ( ; p_iter != p_iter_end ; ++p_iter)
		{
			if (!p_iter.is_valid())
			{
				continue;
			}
			if (*p_iter == NULL){
				continue;
			}
			GPlatesModel::PropertyName property_name = (*p_iter)->property_name();
			QString q_prop_name = GPlatesUtils::make_qstring_from_icu_string(property_name.get_name());
			if (property_name_list.contains(q_prop_name))
			{
				GPlatesModel::DummyTransactionHandle transaction(__FILE__, __LINE__);
				feature->remove_child(p_iter, transaction);
				transaction.commit();
			}

		} // loop over properties in feature. 

	}

		
	/** 
	 * Uses the @a model_to_attribute_map to create model properties from the 
	 * @a feature_handle's shapefile-attributes key-value-dictionary.
	 */ 
	void
	map_attributes_to_properties(
		const GPlatesModel::FeatureHandle::weak_ref &feature,
		QMap<QString,QString> model_to_attribute_map,
		GPlatesFileIO::ReadErrorAccumulation &read_errors,
		const boost::shared_ptr<GPlatesFileIO::DataSource> source,
		const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> location)
	{
		QVariant attribute;

		QMap<QString,QString>::const_iterator it = 
			model_to_attribute_map.find(ShapefileAttributes::model_properties[ShapefileAttributes::PLATEID]);

		if (it != model_to_attribute_map.constEnd())
		{
			attribute = get_qvariant_from_finder(it.value(),feature);
			bool ok;
			int plate_id_as_int = attribute.toInt(&ok);
			if (ok){
				append_plate_id_to_feature(feature,plate_id_as_int);
			}
			else{
				read_errors.d_warnings.push_back(
					GPlatesFileIO::ReadErrorOccurrence(
					source,
					location,
					GPlatesFileIO::ReadErrors::InvalidShapefilePlateIdNumber,
					GPlatesFileIO::ReadErrors::NoPlateIdLoadedForFeature));

			}

		}


		boost::optional<double> age_of_appearance;
		boost::optional<double> age_of_disappearance;

		it = model_to_attribute_map.find(ShapefileAttributes::model_properties[ShapefileAttributes::BEGIN]);
		if (it != model_to_attribute_map.constEnd())
		{
			attribute = get_qvariant_from_finder(it.value(),feature);
			bool ok;
			double age = attribute.toDouble(&ok);
			if (ok){
				age_of_appearance = age;
			}
			else{
				read_errors.d_warnings.push_back(
					GPlatesFileIO::ReadErrorOccurrence(
					source,
					location,
					GPlatesFileIO::ReadErrors::InvalidShapefileAgeOfAppearance,
					GPlatesFileIO::ReadErrors::AttributeIgnored));

			}
		}

		it = model_to_attribute_map.find(ShapefileAttributes::model_properties[ShapefileAttributes::END]);
		if (it != model_to_attribute_map.constEnd())
		{
			attribute = get_qvariant_from_finder(it.value(),feature);
			bool ok;
			double age = attribute.toDouble(&ok);
			if (ok){
				age_of_disappearance = age;
			}
			else{
				read_errors.d_warnings.push_back(
					GPlatesFileIO::ReadErrorOccurrence(
					source,
					location,
					GPlatesFileIO::ReadErrors::InvalidShapefileAgeOfDisappearance,
					GPlatesFileIO::ReadErrors::AttributeIgnored));
			}
		}

		it = model_to_attribute_map.find(ShapefileAttributes::model_properties[ShapefileAttributes::NAME]);
		if (it != model_to_attribute_map.constEnd())
		{
			attribute = get_qvariant_from_finder(it.value(),feature);
			QString name = attribute.toString();
			append_name_to_feature(feature,name);
		}

		it = model_to_attribute_map.find(ShapefileAttributes::model_properties[ShapefileAttributes::DESCRIPTION]);
		if (it != model_to_attribute_map.constEnd())
		{
			attribute = get_qvariant_from_finder(it.value(),feature);
			QString description = attribute.toString();
			append_description_to_feature(feature,description);
		}

		// FIXME : allow only one of the begin/end pair to be provided.
		if (age_of_appearance && age_of_disappearance){
			append_geo_time_to_feature(feature,*age_of_appearance,*age_of_disappearance);
		}
	}

	/**
	 * Uses the @a model_to_attribute_map to create model properties from the 
	 * shapefile-attributes key-value-dictionary, for each feature in @a file's 
	 * feature collection. 
	 */
	void
	remap_feature_collection(
		GPlatesFileIO::File &file,
		QMap< QString,QString > model_to_attribute_map,
		GPlatesFileIO::ReadErrorAccumulation &read_errors)
	{
		QString filename = file.get_file_info().get_qfileinfo().filePath();

		GPlatesModel::FeatureCollectionHandle::weak_ref collection = file.get_feature_collection();

		GPlatesModel::FeatureCollectionHandle::children_iterator it = collection->children_begin();
		GPlatesModel::FeatureCollectionHandle::children_iterator it_end = collection->children_end();
		int count = 0;
		for ( ; it != it_end ; ++it)
		{
			boost::shared_ptr<GPlatesFileIO::DataSource> source(
				new GPlatesFileIO::LocalFileDataSource(filename, GPlatesFileIO::DataFormats::Shapefile));
			boost::shared_ptr<GPlatesFileIO::LocationInDataSource> location(
				new GPlatesFileIO::LineNumberInFile(count));
			GPlatesModel::FeatureHandle::weak_ref feature = (*it)->reference();
			remove_old_properties(feature);
			map_attributes_to_properties(feature,model_to_attribute_map,read_errors,source,location);
			count++;
		} 
	}

	/**
	 * Fills the QMap<QString,QString> @a model_to_attribute_map from the given xml file
	 * @a filename.
	 */
	bool
	fill_attribute_map_from_xml_file(
		QString filename,
		QMap<QString,QString> &model_to_attribute_map)
	{
		QFileInfo file_info(filename);
		if (!file_info.exists()){
			return false;
		}

		GPlatesFileIO::ShapefileXmlReader xml_reader;

		if (!xml_reader.read_file(filename,&model_to_attribute_map)) {
			QMessageBox::warning(0, QObject::tr("ShapefileXmlReader"),
				QObject::tr("Parse error in file %1 at line %2, column %3:\n%4")
				.arg(filename)
				.arg(xml_reader.lineNumber())
				.arg(xml_reader.columnNumber())
				.arg(xml_reader.errorString()));
			return false;
		}
		
		return true;
	}

	/**
	 * Allows the user to perform the model-property-to-shapefile-attribute mapping
	 * via a dialog. Returns false if the user cancelled the dialog, otherwise returns true.
	 */
	bool
	fill_attribute_map_from_dialog(
		QString filename,
		QStringList &field_names,
		QMap<QString,QString> &model_to_attribute_map,
		boost::shared_ptr< GPlatesFileIO::PropertyMapper > mapper,
		bool remapping)
	{
		if (mapper == NULL){
			return false;
		}
		return (mapper->map_properties(filename,field_names,model_to_attribute_map,remapping));
	}


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


GPlatesFileIO::ShapefileReader::ShapefileReader():
	d_num_layers(0),
	d_data_source_ptr(NULL),
	d_geometry_ptr(NULL),
	d_feature_ptr(NULL),
	d_layer_ptr(NULL),
	d_total_geometries(0),
	d_loaded_geometries(0),
	d_total_features(0)
{
	OGRRegisterAll();
}

GPlatesFileIO::ShapefileReader::~ShapefileReader()
{
	try {
		if(d_data_source_ptr){OGRDataSource::DestroyDataSource(d_data_source_ptr);
		}
	}
	catch (...) {
	}
}


bool
GPlatesFileIO::ShapefileReader::check_file_format(
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

		qDebug() << "Null Geometry Ref";

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
		
	OGRFeature::DestroyFeature(d_feature_ptr);
	return true;
}

bool
GPlatesFileIO::ShapefileReader::open_file(
		const QString &filename)
{

	std::string fname = filename.toStdString();
	
	d_data_source_ptr = OGRSFDriverRegistrar::Open(fname.c_str());
	if	(d_data_source_ptr == NULL){
		return false;
	}
	d_filename = filename;
	return true;
}



void
GPlatesFileIO::ShapefileReader::read_features(
	GPlatesModel::ModelInterface &model,
	const GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
	ReadErrorAccumulation &read_errors)
{

	if (!d_layer_ptr){
		// we shouldn't really be here
		std::cerr << "null d_layer_ptr in read_features" << std::endl;
	}

	d_total_features = d_layer_ptr->GetFeatureCount();

	int feature_number = 0; // For error reporting.

	boost::shared_ptr<GPlatesFileIO::DataSource> e_source(
		new GPlatesFileIO::LocalFileDataSource(d_filename, GPlatesFileIO::DataFormats::Shapefile));

	static const ShapefileUtils::feature_map_type &feature_map = ShapefileUtils::build_feature_map();

	d_feature_type = "UnclassifiedFeature";
#if 0
	d_feature_creation_pair = std::make_pair("UnclassifiedFeature",
			"unclassifiedGeometry");
#endif
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

		get_attributes();

		// Check if we have a shapefile attribute corresponding to the Feature Type.
		QMap<QString,QString>::const_iterator it = 
			s_model_to_attribute_map.find(ShapefileAttributes::model_properties[ShapefileAttributes::FEATURE_TYPE]);

		if ((it != s_model_to_attribute_map.constEnd()) && s_field_names.contains(it.value())) {
		
			int index = s_field_names.indexOf(it.value());
	
			// d_field_names should be the same size as d_attributes, but check that we
			// don't try to go beyond the bounds of d_attributes. If somehow we are trying to do 
			// this, then we just get an unclassifiedFeature created.
			if ((index >= 0) && (index < static_cast<int>(d_attributes.size())))
			{

				QString feature_string = d_attributes[index].toString();

				ShapefileUtils::feature_map_const_iterator result = feature_map.find(feature_string);
				if (result != feature_map.end()) {
				//	d_feature_creation_pair = result->second;
					d_feature_type = *result;
				} else {
					read_errors.d_warnings.push_back(GPlatesFileIO::ReadErrorOccurrence(e_source, e_location, 
					GPlatesFileIO::ReadErrors::UnrecognisedShapefileFeatureType,
					GPlatesFileIO::ReadErrors::UnclassifiedShapefileFeatureCreated));
				}
			}
		}
		
		// Check if we have a shapefile attribute corresponding to the Feature ID.
		d_feature_id.reset();
		
		it = s_model_to_attribute_map.find(
			ShapefileAttributes::model_properties[ShapefileAttributes::FEATURE_ID]);

		if ((it != s_model_to_attribute_map.constEnd()) && s_field_names.contains(it.value())) {

			int index = s_field_names.indexOf(it.value());

			// d_field_names should be the same size as d_attributes, but check that we
			// don't try to go beyond the bounds of d_attributes. If somehow we are trying to do 
			// this, then we just get a feature without a feature_id.
			if ((index >= 0) && (index < static_cast<int>(d_attributes.size())))
			{

				QString feature_id = d_attributes[index].toString();

				d_feature_id.reset(GPlatesUtils::make_icu_string_from_qstring(feature_id));
				//*d_feature_id = GPlatesUtils::make_icu_string_from_qstring(feature_id);
			}
		}		
		

		d_type = d_geometry_ptr->getGeometryType();
		d_type = wkb_flatten(d_type);
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

}

const GPlatesModel::FeatureHandle::weak_ref
GPlatesFileIO::ShapefileReader::create_polygon_feature_from_list(
	GPlatesModel::ModelInterface &model,
	const GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
	std::list<GPlatesMaths::PointOnSphere> &list_of_points)
{

			
	GPlatesModel::FeatureHandle::weak_ref feature = create_feature(model,collection,d_feature_type,d_feature_id);

	GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere =
		GPlatesMaths::PolygonOnSphere::create_on_heap(list_of_points);

	GPlatesPropertyValues::GmlPolygon::non_null_ptr_type gml_polygon =
		GPlatesPropertyValues::GmlPolygon::create(polygon_on_sphere);

	GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type property_value =
		GPlatesModel::ModelUtils::create_gpml_constant_value(
							gml_polygon, 
							GPlatesPropertyValues::TemplateTypeParameterType::create_gml("Polygon"));

	// Anything that's got a polygon geometry is going to get an "unclassifiedGeometry" property name. 
	GPlatesModel::ModelUtils::append_property_value_to_feature(property_value,
		GPlatesModel::PropertyName::create_gpml("unclassifiedGeometry"), feature);

	return feature;

}

const GPlatesModel::FeatureHandle::weak_ref
GPlatesFileIO::ShapefileReader::create_line_feature_from_list(
	GPlatesModel::ModelInterface &model,
	const GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
	std::list<GPlatesMaths::PointOnSphere> &list_of_points)
{
	GPlatesModel::FeatureHandle::weak_ref feature = create_feature(model,collection,d_feature_type,d_feature_id);

	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline =
		GPlatesMaths::PolylineOnSphere::create_on_heap(list_of_points);

	GPlatesPropertyValues::GmlLineString::non_null_ptr_type gml_line_string =
		GPlatesPropertyValues::GmlLineString::create(polyline);

	GPlatesPropertyValues::GmlOrientableCurve::non_null_ptr_type gml_orientable_curve =
		GPlatesModel::ModelUtils::create_gml_orientable_curve(gml_line_string);
			
	GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type property_value =
			GPlatesModel::ModelUtils::create_gpml_constant_value(
							gml_orientable_curve, 
							GPlatesPropertyValues::TemplateTypeParameterType::create_gml("OrientableCurve"));

	GPlatesModel::ModelUtils::append_property_value_to_feature(property_value,
//					GPlatesModel::PropertyName::create_gpml("centerLineOf"), feature);
					GPlatesModel::PropertyName::create_gpml("unclassifiedGeometry"), feature);

	return feature;
}


const GPlatesModel::FeatureHandle::weak_ref
GPlatesFileIO::ShapefileReader::create_point_feature_from_pair(
	GPlatesModel::ModelInterface &model, 
	const GPlatesModel::FeatureCollectionHandle::weak_ref &collection, 
	std::pair<double,double> &point)
{
	GPlatesModel::FeatureHandle::weak_ref feature = create_feature(model,collection,d_feature_type,d_feature_id);

	const GPlatesModel::PropertyValue::non_null_ptr_type gml_point =
		GPlatesPropertyValues::GmlPoint::create(point);

	GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type property_value =
		GPlatesModel::ModelUtils::create_gpml_constant_value(
							gml_point, 
							GPlatesPropertyValues::TemplateTypeParameterType::create_gml("Point"));

	// What sort of gpml property name should a point have? 
	GPlatesModel::ModelUtils::append_property_value_to_feature(property_value,
		GPlatesModel::PropertyName::create_gpml("unclassifiedGeometry"), feature);

	return feature;

}


const GPlatesModel::FeatureHandle::weak_ref
GPlatesFileIO::ShapefileReader::create_point_feature_from_point_on_sphere(
	GPlatesModel::ModelInterface &model, 
	const GPlatesModel::FeatureCollectionHandle::weak_ref &collection, 
	GPlatesMaths::PointOnSphere &point)
{

	GPlatesModel::FeatureHandle::weak_ref feature = create_feature(model,collection,d_feature_type,d_feature_id);

	const GPlatesModel::PropertyValue::non_null_ptr_type gml_point =
		GPlatesPropertyValues::GmlPoint::create(point);

	GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type property_value =
		GPlatesModel::ModelUtils::create_gpml_constant_value(
							gml_point, 
							GPlatesPropertyValues::TemplateTypeParameterType::create_gml("Point"));

	// What sort of gpml property name should a point have? 
	// I'm going to leave it as an unclassifiedGeometry for now. 
	GPlatesModel::ModelUtils::append_property_value_to_feature(property_value,
		GPlatesModel::PropertyName::create_gpml("unclassifiedGeometry"), feature);

	return feature;

}

const GPlatesModel::FeatureHandle::weak_ref
GPlatesFileIO::ShapefileReader::create_multi_point_feature_from_list(
	GPlatesModel::ModelInterface &model,
	const GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
	std::list<GPlatesMaths::PointOnSphere> &list_of_points)
{

	GPlatesModel::FeatureHandle::weak_ref feature = create_feature(model,collection,d_feature_type,d_feature_id);

	GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere =
		GPlatesMaths::MultiPointOnSphere::create_on_heap(list_of_points);

	GPlatesPropertyValues::GmlMultiPoint::non_null_ptr_type gml_multi_point =
		GPlatesPropertyValues::GmlMultiPoint::create(multi_point_on_sphere);

	GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type property_value =
		GPlatesModel::ModelUtils::create_gpml_constant_value(
							gml_multi_point, 
							GPlatesPropertyValues::TemplateTypeParameterType::create_gml("MultiPoint"));

	GPlatesModel::ModelUtils::append_property_value_to_feature(property_value,
		GPlatesModel::PropertyName::create_gpml("unclassifiedGeometry"), feature);


	return feature;
}

void 
GPlatesFileIO::ShapefileReader::get_field_names(
	ReadErrorAccumulation &read_errors)
{
	boost::shared_ptr<GPlatesFileIO::DataSource> e_source(
		new GPlatesFileIO::LocalFileDataSource(d_filename, GPlatesFileIO::DataFormats::Shapefile));

	boost::shared_ptr<GPlatesFileIO::LocationInDataSource> e_location(
		new GPlatesFileIO::LineNumberInFile(0));
				
	s_field_names.clear();
	if (!d_feature_ptr){
		return;
	}
	OGRFeatureDefn *feature_def_ptr = d_layer_ptr->GetLayerDefn();
	int num_fields = feature_def_ptr->GetFieldCount();
	int count;

	for ( count=0; count< num_fields; count++){
		OGRFieldDefn *field_def_ptr = feature_def_ptr->GetFieldDefn(count);
		s_field_names.push_back(QString::fromAscii(field_def_ptr->GetNameRef()));
	}

}

void 
GPlatesFileIO::ShapefileReader::get_attributes()
{
	d_attributes.clear();
	if (!d_feature_ptr){
		return;
	}
	OGRFeatureDefn *feature_def_ptr = d_layer_ptr->GetLayerDefn();
	int num_fields = feature_def_ptr->GetFieldCount();
	int count;
	for (count=0; count < num_fields; count++){
		OGRFieldDefn *field_def_ptr = feature_def_ptr->GetFieldDefn(count);
		if (field_def_ptr->GetType()==OFTInteger){
			d_attributes.push_back(QVariant(d_feature_ptr->GetFieldAsInteger(count)));
		}
		else if (field_def_ptr->GetType()==OFTReal){
			d_attributes.push_back(QVariant(d_feature_ptr->GetFieldAsDouble(count)));
		}
		else if (field_def_ptr->GetType()==OFTString){
			QString temp_string;
			temp_string = QString(d_feature_ptr->GetFieldAsString(count));
			d_attributes.push_back(QVariant(d_feature_ptr->GetFieldAsString(count)));
		}
		else if (field_def_ptr->GetType()==OFTDate)
		{
			// Store this as a string. It's possible to extract the various year/month/day
			// fields separately. 
			QString temp_string;
			temp_string = QString(d_feature_ptr->GetFieldAsString(count));
			d_attributes.push_back(QVariant(d_feature_ptr->GetFieldAsString(count)));
		}
		else{
			// Any other attribute types are not handled at the moment...use an
			// empty string so that the size of d_attributes keeps in sync with the number
			// of fields.
			d_attributes.push_back(QVariant(QString()));
		}
	}
}

void
GPlatesFileIO::ShapefileReader::add_attributes_to_feature(
	const GPlatesModel::FeatureHandle::weak_ref &feature,
	GPlatesFileIO::ReadErrorAccumulation &read_errors,
	const boost::shared_ptr<GPlatesFileIO::DataSource> &source,
	const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> &location)
{
	int n = static_cast<int>(d_attributes.size());
	
	// Can there be zero attributes? I dunno. 
	if (n == 0) return;

	// Create a key-value dictionary. This is empty and needs to have elements pushed back onto its d_elements vector. 
	GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type dictionary = 
		GPlatesPropertyValues::GpmlKeyValueDictionary::create();

	// If for any reason we've found more attributes than we have field names, only 
	// go as far as the number of field names. 
	if (n > s_field_names.size())
	{
		n = s_field_names.size();
	}

	for (int count = 0; count < n ; count++){
		QString fieldname = s_field_names[count];
		QVariant attribute = d_attributes[count];

		QVariant::Type type_ = attribute.type();

		// Make an XsString property for the attribute field name. 
		GPlatesPropertyValues::XsString::non_null_ptr_type key = 
			GPlatesPropertyValues::XsString::create(
				GPlatesUtils::make_icu_string_from_qstring(fieldname));

		// Add the attribute to the dictionary.
		switch(type_){
			case QVariant::Int:
				{
				int i = attribute.toInt();
				GPlatesPropertyValues::XsInteger::non_null_ptr_type value = 
					GPlatesPropertyValues::XsInteger::create(i);
				GPlatesPropertyValues::GpmlKeyValueDictionaryElement element(
					key,
					value,
					GPlatesPropertyValues::TemplateTypeParameterType::create_xsi("integer"));
				dictionary->elements().push_back(element);
				}
				break;
			case QVariant::Double:
				{
				double d = attribute.toDouble();
				GPlatesPropertyValues::XsDouble::non_null_ptr_type value = 
					GPlatesPropertyValues::XsDouble::create(d);
				GPlatesPropertyValues::GpmlKeyValueDictionaryElement element(
					key,
					value,
					GPlatesPropertyValues::TemplateTypeParameterType::create_xsi("double"));
				dictionary->elements().push_back(element);
				}
				break;
			case QVariant::String:
				{
				GPlatesPropertyValues::XsString::non_null_ptr_type value = 
					GPlatesPropertyValues::XsString::create(
							GPlatesUtils::make_icu_string_from_qstring(attribute.toString()));
				GPlatesPropertyValues::GpmlKeyValueDictionaryElement element(
					key,
					value,
					GPlatesPropertyValues::TemplateTypeParameterType::create_xsi("string"));
				dictionary->elements().push_back(element);
				}
				break;
			default:
				break;
		}

	} // loop over number of attributes

	// Add the dictionary to the model.
	GPlatesModel::ModelUtils::append_property_value_to_feature(
		dictionary,
		GPlatesModel::PropertyName::create_gpml("shapefileAttributes"),
		feature);

	// Map the shapefile attributes to model properties.
	map_attributes_to_properties(feature,s_model_to_attribute_map,read_errors,source,location);

}


bool
GPlatesFileIO::ShapefileReader::is_valid_shape_data(
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

const GPlatesFileIO::File::shared_ref
GPlatesFileIO::ShapefileReader::read_file(
		const FileInfo &fileinfo,
		GPlatesModel::ModelInterface &model,
		ReadErrorAccumulation &read_errors)
{
	QString absolute_path_filename = fileinfo.get_qfileinfo().absoluteFilePath();
	QString filename = fileinfo.get_qfileinfo().fileName();

	ShapefileReader reader;
	if (!reader.open_file(absolute_path_filename)){
		throw ErrorOpeningFileForReadingException(GPLATES_EXCEPTION_SOURCE, filename);
	}

	if (!reader.check_file_format(read_errors)){
		throw ErrorOpeningFileForReadingException(GPLATES_EXCEPTION_SOURCE, filename);
	}
	reader.get_field_names(read_errors);

	QString shapefile_xml_filename = ShapefileUtils::make_shapefile_xml_filename(fileinfo.get_qfileinfo());

	s_model_to_attribute_map.clear();

	if (!fill_attribute_map_from_xml_file(shapefile_xml_filename,s_model_to_attribute_map))
	{
		// Set the last argument to false, because this is an initial mapping, not a re-mapping. 
		if (!fill_attribute_map_from_dialog(filename,s_field_names,s_model_to_attribute_map,s_property_mapper,false))
		{
			// The user has cancelled the mapper-dialog routine, so cancel the whole shapefile loading procedure.
			throw FileLoadAbortedException(GPLATES_EXCEPTION_SOURCE, "File load aborted.");
		}
		ShapefileUtils::save_attribute_map_as_xml_file(shapefile_xml_filename,s_model_to_attribute_map);
	}

	// Store the map in the feature collection's file-info
	fileinfo.set_model_to_shapefile_map(s_model_to_attribute_map);

	GPlatesModel::FeatureCollectionHandle::weak_ref collection
		= model->create_feature_collection();

	// Make sure feature collection gets unloaded when it's no longer needed.
	GPlatesModel::FeatureCollectionHandleUnloader::shared_ref collection_unloader =
			GPlatesModel::FeatureCollectionHandleUnloader::create(collection);

	reader.read_features(model,collection,read_errors);


	//reader.display_feature_counts();

	return File::create_loaded_file(collection_unloader, fileinfo);
}

void
GPlatesFileIO::ShapefileReader::set_property_mapper(
	boost::shared_ptr< PropertyMapper > property_mapper)
{
	s_property_mapper = property_mapper;
}


void
GPlatesFileIO::ShapefileReader::handle_point(
		GPlatesModel::ModelInterface &model,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
		ReadErrorAccumulation &read_errors,
		const boost::shared_ptr<GPlatesFileIO::DataSource> &source,
		const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> &location)
{
			OGRPoint *point = static_cast<OGRPoint*>(d_geometry_ptr);
			double lat,lon;
			lat = point->getY();
			lon = point->getX();

			
			if (is_valid_shape_data(lat,lon,read_errors,source,location)){
				GPlatesMaths::LatLonPoint llp(lat,lon);
				GPlatesMaths::PointOnSphere p = GPlatesMaths::make_point_on_sphere(llp);
				try {
					GPlatesModel::FeatureHandle::weak_ref feature = create_point_feature_from_point_on_sphere(model,collection,p);
					add_attributes_to_feature(feature,read_errors,source,location);
					d_loaded_geometries++;
				}
				catch (...)
				{
					read_errors.d_recoverable_errors.push_back(
						GPlatesFileIO::ReadErrorOccurrence(
							source,
							location,
							GPlatesFileIO::ReadErrors::InvalidShapefilePoint,
							GPlatesFileIO::ReadErrors::GeometryIgnored));
				}
			}
			d_total_geometries++;
}

void
GPlatesFileIO::ShapefileReader::handle_multi_point(
			GPlatesModel::ModelInterface &model,
			const GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			ReadErrorAccumulation &read_errors,
			const boost::shared_ptr<GPlatesFileIO::DataSource> &source,
			const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> &location)
{
		OGRMultiPoint *multi = static_cast<OGRMultiPoint*>(d_geometry_ptr);
		int num_geometries = multi->getNumGeometries();
		d_total_geometries += num_geometries;

		std::list<GPlatesMaths::PointOnSphere> list_of_points;

		for(int count = 0; count < num_geometries; count++)
		{
			OGRPoint* point = static_cast<OGRPoint*>(multi->getGeometryRef(count));
			double lat,lon;
			lat = point->getY();
			lon = point->getX();

			if (is_valid_shape_data(lat,lon,read_errors,source,location)){
				GPlatesMaths::LatLonPoint llp(lat,lon);
				GPlatesMaths::PointOnSphere p = GPlatesMaths::make_point_on_sphere(llp);
				list_of_points.push_back(p);

			}
		} // loop over geometries

		if (!list_of_points.empty())
		{
			try {
				GPlatesModel::FeatureHandle::weak_ref feature = create_multi_point_feature_from_list(model,collection,list_of_points);
				add_attributes_to_feature(feature,read_errors,source,location);
				d_loaded_geometries++;
			}
			catch(...)
			{
				read_errors.d_recoverable_errors.push_back(
					GPlatesFileIO::ReadErrorOccurrence(
						source,
						location,
						GPlatesFileIO::ReadErrors::InvalidShapefileMultiPoint,
						GPlatesFileIO::ReadErrors::GeometryIgnored));
			}
		}
}

void
GPlatesFileIO::ShapefileReader::handle_linestring(
			GPlatesModel::ModelInterface &model,
			const GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			ReadErrorAccumulation &read_errors,
			const boost::shared_ptr<GPlatesFileIO::DataSource> &source,
			const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> &location
			)
{
	std::list<GPlatesMaths::PointOnSphere> feature_points;			
	OGRLineString *linestring = static_cast<OGRLineString*>(d_geometry_ptr);
	int num_points = linestring->getNumPoints();
	d_total_geometries++;
	if (num_points < 2){	
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
		else{
			return;
		}

	}

	try {
		GPlatesModel::FeatureHandle::weak_ref feature = create_line_feature_from_list(model,collection,feature_points);
		add_attributes_to_feature(feature,read_errors,source,location);
		d_loaded_geometries++;
	}
	catch (...)
	{
		read_errors.d_recoverable_errors.push_back(
			GPlatesFileIO::ReadErrorOccurrence(
				source,
				location,
				GPlatesFileIO::ReadErrors::InvalidShapefilePolyline,
				GPlatesFileIO::ReadErrors::GeometryIgnored));
	}

}

void
GPlatesFileIO::ShapefileReader::handle_multi_linestring(
			GPlatesModel::ModelInterface &model,
			const GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			ReadErrorAccumulation &read_errors,
			const boost::shared_ptr<GPlatesFileIO::DataSource> &source,
			const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> &location)
{

	OGRMultiLineString *multi = static_cast<OGRMultiLineString*>(d_geometry_ptr);
	int num_geometries = multi->getNumGeometries();
	d_total_geometries += num_geometries;

#if 0
	qDebug() << "handle_multi_linestring";
	qDebug() << "num geometries: " << num_geometries;
#endif
		
	GPlatesModel::FeatureHandle::weak_ref feature = create_feature(model,collection,d_feature_type,d_feature_id);				
	add_attributes_to_feature(feature,read_errors,source,location);	

	for (int multiCount = 0; multiCount < num_geometries ; multiCount++){

		std::list<GPlatesMaths::PointOnSphere> feature_points;
		OGRLineString *linestring = static_cast<OGRLineString*>(multi->getGeometryRef(multiCount));

		int num_points = linestring->getNumPoints();
		if (num_points < 2){	
			// FIXME: May want to treat this as a warning, and accept the single-point line. 
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
			else{
				feature_points.clear();
				break;
			}
		}

		if (!feature_points.empty())
		{
			try {
				add_polyline_geometry_to_feature(feature,feature_points);
				d_loaded_geometries++;
			}
			catch (...)
			{
				read_errors.d_recoverable_errors.push_back(
					GPlatesFileIO::ReadErrorOccurrence(
					source,
					location,
					GPlatesFileIO::ReadErrors::InvalidShapefilePolyline,
					GPlatesFileIO::ReadErrors::GeometryIgnored));
			}
		}
	} // for loop over multilines
}


void
GPlatesFileIO::ShapefileReader::handle_polygon(
			GPlatesModel::ModelInterface &model,
			const GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			ReadErrorAccumulation &read_errors,
			const boost::shared_ptr<GPlatesFileIO::DataSource> &source,
			const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> &location)
{
	std::list<GPlatesMaths::PointOnSphere> feature_points;
	OGRPolygon *polygon = static_cast<OGRPolygon*>(d_geometry_ptr);
	d_total_geometries++;

	OGRLinearRing *ring = polygon->getExteriorRing();
	add_ring_to_points_list(ring,feature_points,read_errors,source,location);

	GPlatesModel::FeatureHandle::weak_ref feature = create_feature(model,collection,d_feature_type,d_feature_id);				
	add_attributes_to_feature(feature,read_errors,source,location);	

	if (!feature_points.empty()){
		try {
			add_polygon_geometry_to_feature(feature,feature_points);
			d_loaded_geometries++;
			d_loaded_geometries++;
		}
		catch (...)
		{
			read_errors.d_recoverable_errors.push_back(
				GPlatesFileIO::ReadErrorOccurrence(
				source,
				location,
				GPlatesFileIO::ReadErrors::InvalidShapefilePolygon,
				GPlatesFileIO::ReadErrors::GeometryIgnored));
		}

		d_loaded_geometries++;
	}

	int num_interior_rings = polygon->getNumInteriorRings();
	if (num_interior_rings == 0){
		return;
	}

	for (int ring_count = 0; ring_count < num_interior_rings; ring_count++)
	{
		//std::cerr << "Interior ring: " << ring_count << std::endl;
		feature_points.clear();

		ring = polygon->getInteriorRing(ring_count);
		add_ring_to_points_list(ring,feature_points,read_errors,source,location);

		if (!feature_points.empty()){
			try {
				add_polygon_geometry_to_feature(feature,feature_points);
				d_loaded_geometries++;
			}
			catch (...)
			{
				read_errors.d_recoverable_errors.push_back(
					GPlatesFileIO::ReadErrorOccurrence(
					source,
					location,
					GPlatesFileIO::ReadErrors::InvalidShapefilePolygon,
					GPlatesFileIO::ReadErrors::GeometryIgnored));
			}
		}

	} // loop over interior rings

}

void
GPlatesFileIO::ShapefileReader::handle_multi_polygon(
			GPlatesModel::ModelInterface &model,
			const GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			ReadErrorAccumulation &read_errors,
			const boost::shared_ptr<GPlatesFileIO::DataSource> &source,
			const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> &location)
{
	//std::cerr << "Multi-polygon" << std::endl;
	OGRMultiPolygon *multi = static_cast<OGRMultiPolygon*>(d_geometry_ptr);
	int num_geometries = multi->getNumGeometries();

	GPlatesModel::FeatureHandle::weak_ref feature = create_feature(model,collection,d_feature_type,d_feature_id);				
	add_attributes_to_feature(feature,read_errors,source,location);	

	d_total_geometries += num_geometries;

	for (int multiCount = 0; multiCount < num_geometries ; multiCount++){
		//std::cerr << "Polygon number: " << multiCount << std::endl;
		std::list<GPlatesMaths::PointOnSphere> feature_points;
		OGRPolygon *polygon = static_cast<OGRPolygon*>(multi->getGeometryRef(multiCount));

		OGRLinearRing *ring = polygon->getExteriorRing();

		add_ring_to_points_list(ring,feature_points,read_errors,source,location);

		if (!feature_points.empty()){
			try {	
				add_polygon_geometry_to_feature(feature,feature_points);
				d_loaded_geometries++;
			}
			catch (...)
			{
					read_errors.d_recoverable_errors.push_back(
						GPlatesFileIO::ReadErrorOccurrence(
							source,
							location,
							GPlatesFileIO::ReadErrors::InvalidShapefilePolygon,
							GPlatesFileIO::ReadErrors::GeometryIgnored));
			}
		}

		int num_interior_rings = polygon->getNumInteriorRings();

		for (int ring_count = 0; ring_count < num_interior_rings; ring_count++)
		{
			//std::cerr << "Multi-polygon Interior ring: " << ring_count << std::endl;
			feature_points.clear();

			ring = polygon->getInteriorRing(ring_count);

			add_ring_to_points_list(ring,feature_points,read_errors,source,location);

			if (!feature_points.empty()){
				try {
					add_polygon_geometry_to_feature(feature,feature_points);
					d_loaded_geometries++;
				}
				catch (...)
				{
					read_errors.d_recoverable_errors.push_back(
						GPlatesFileIO::ReadErrorOccurrence(
						source,
						location,
						GPlatesFileIO::ReadErrors::InvalidShapefilePolygon,
						GPlatesFileIO::ReadErrors::GeometryIgnored));
				}
			}

		} // loop over interior rings

	} // loop over multipolygons
}

void
GPlatesFileIO::ShapefileReader::display_feature_counts()
{

	std::cerr << "feature/geometry count: " <<
			d_total_features << ", " <<
			d_loaded_geometries << ", " <<
			d_total_geometries << std::endl;

}


void
GPlatesFileIO::ShapefileReader::add_ring_to_points_list(
	OGRLinearRing *ring, 
	std::list<GPlatesMaths::PointOnSphere> &feature_points,
	ReadErrorAccumulation &read_errors,
	const boost::shared_ptr<GPlatesFileIO::DataSource> &source,
	const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> &location)
{
	// Make sure we have a valid ring.
	if (ring == NULL)
	{
		return;
	}

	int num_points;
	int count;
	double lat,lon; 

	feature_points.clear();

	num_points = ring->getNumPoints();
 
	// FIXME: Check if the shapefile format demands that a polygon must have
	// at least 3 points, and if so, check for that here.  
	// For now we are storing and drawing them as line strings, so we *can* handle 
	// 2-point polygons OK. 
	if (num_points < 2){
		read_errors.d_recoverable_errors.push_back(
			GPlatesFileIO::ReadErrorOccurrence(
			source,
			location,
			GPlatesFileIO::ReadErrors::LessThanTwoPointsInLineString,
			GPlatesFileIO::ReadErrors::GeometryIgnored));
			return;
	}

	for (count = 0; count < num_points; count++){
		lon = ring->getX(count);
		lat = ring->getY(count);
		if (is_valid_shape_data(lat,lon,read_errors,source,location)){
			GPlatesMaths::LatLonPoint llp(lat,lon);
			feature_points.push_back(GPlatesMaths::make_point_on_sphere(llp));
		}
		else{
			// One of our points is invalid. We can't create a feature, so clear the std::list.
			feature_points.clear();
			return;
		}
	}

}

void
GPlatesFileIO::ShapefileReader::remap_shapefile_attributes(
	File &file,
	GPlatesModel::ModelInterface &model,
	ReadErrorAccumulation &read_errors)
{
	const FileInfo &file_info = file.get_file_info();

	QString absolute_path_filename = file_info.get_qfileinfo().absoluteFilePath();
	QString filename = file_info.get_qfileinfo().fileName();

	ShapefileReader reader;
	if (!reader.open_file(absolute_path_filename)){
		throw ErrorOpeningFileForReadingException(GPLATES_EXCEPTION_SOURCE, filename);
	}

	if (!reader.check_file_format(read_errors)){
		throw ErrorOpeningFileForReadingException(GPLATES_EXCEPTION_SOURCE, filename);
	}

	reader.get_field_names(read_errors);

	QString shapefile_xml_filename = ShapefileUtils::make_shapefile_xml_filename(
			file_info.get_qfileinfo());

	s_model_to_attribute_map.clear();

	fill_attribute_map_from_xml_file(shapefile_xml_filename,s_model_to_attribute_map);

	// Set the last argument to true because we are remapping. 
	if (!fill_attribute_map_from_dialog(filename,s_field_names,s_model_to_attribute_map,s_property_mapper,true))
	{
		// The user has cancelled the mapper-dialog, so cancel the whole shapefile re-mapping procedure.
		return;
	}
	ShapefileUtils::save_attribute_map_as_xml_file(shapefile_xml_filename,s_model_to_attribute_map);

	file_info.set_model_to_shapefile_map(s_model_to_attribute_map);

	remap_feature_collection(
							file,
							s_model_to_attribute_map,
							read_errors);
}
