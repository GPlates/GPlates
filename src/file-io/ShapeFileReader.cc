/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007, 2008 Geological Survey of Norway
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
#include <QMessageBox>
#include <QString>
#include <QStringList>
#include <QVariant>

#include "PropertyMapper.h"
#include "ShapeFileReader.h"
#include "ShapefileXmlReader.h"
#include "ShapefileXmlWriter.h"

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

#include "maths/LatLonPointConversions.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/PolylineOnSphere.h"
#include "maths/PolygonOnSphere.h"

#include "qt-widgets/ShapefilePropertyMapper.h"

#include "ErrorOpeningFileForReadingException.h"

boost::shared_ptr< GPlatesFileIO::PropertyMapper> GPlatesFileIO::ShapeFileReader::s_property_mapper;

QMap<QString,QString> GPlatesFileIO::ShapeFileReader::s_model_to_attribute_map;

QStringList GPlatesFileIO::ShapeFileReader::s_field_names;
	
namespace
{
	QVariant
	get_qvariant_from_finder(
		QString shapefile_property_name,
		GPlatesModel::FeatureHandle::weak_ref feature_handle)
	{
		GPlatesFeatureVisitors::ShapefileAttributeFinder finder(shapefile_property_name);

		feature_handle->accept_visitor(finder);

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
		GPlatesModel::FeatureHandle::weak_ref feature_handle,
		int plate_id_as_int)
	{
		GPlatesPropertyValues::GpmlPlateId::non_null_ptr_type plate_id = 
			GPlatesPropertyValues::GpmlPlateId::create(plate_id_as_int);
		GPlatesModel::ModelUtils::append_property_value_to_feature(
			GPlatesModel::ModelUtils::create_gpml_constant_value(plate_id,
			GPlatesPropertyValues::TemplateTypeParameterType::create_gpml("plateId")),
			GPlatesModel::PropertyName::create_gpml("reconstructionPlateId"),
			feature_handle);
	}

	void
	append_geo_time_to_feature(
		GPlatesModel::FeatureHandle::weak_ref feature_handle,
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
			feature_handle);
	}

	void
	append_name_to_feature(
		GPlatesModel::FeatureHandle::weak_ref feature_handle,
		QString name)
	{
		GPlatesPropertyValues::XsString::non_null_ptr_type gml_description = 
			GPlatesPropertyValues::XsString::create(UnicodeString(name.toStdString().c_str()));
		GPlatesModel::ModelUtils::append_property_value_to_feature(
			gml_description, 
			GPlatesModel::PropertyName::create_gml("name"), 
			feature_handle);
	}

	void
	append_description_to_feature(
		GPlatesModel::FeatureHandle::weak_ref feature_handle,
		QString description)
	{
		GPlatesPropertyValues::XsString::non_null_ptr_type gml_description = 
			GPlatesPropertyValues::XsString::create(UnicodeString(description.toStdString().c_str()));
		GPlatesModel::ModelUtils::append_property_value_to_feature(
			gml_description, 
			GPlatesModel::PropertyName::create_gml("description"), 
			feature_handle);
	}

	void
	remove_old_properties(
		GPlatesModel::FeatureHandle::weak_ref feature_handle)
	{

		QStringList property_name_list;
		property_name_list << QString("reconstructionPlateId");
		property_name_list << QString("validTime"); 
		property_name_list << QString("description"); 
		property_name_list << QString("name"); 

		GPlatesModel::FeatureHandle::properties_iterator p_iter = feature_handle->properties_begin();
		GPlatesModel::FeatureHandle::properties_iterator p_iter_end = feature_handle->properties_end();

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
				feature_handle->remove_top_level_property(p_iter, transaction);
				transaction.commit();
			}

		} // loop over properties in feature. 

	}

		


	void
	map_attributes_to_properties(
		GPlatesModel::FeatureHandle::weak_ref feature_handle,
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
			attribute = get_qvariant_from_finder(it.value(),feature_handle);
			bool ok;
			int plate_id_as_int = attribute.toInt(&ok);
			if (ok){
				append_plate_id_to_feature(feature_handle,plate_id_as_int);
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
			attribute = get_qvariant_from_finder(it.value(),feature_handle);
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
			attribute = get_qvariant_from_finder(it.value(),feature_handle);
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
			attribute = get_qvariant_from_finder(it.value(),feature_handle);
			QString name = attribute.toString();
			append_name_to_feature(feature_handle,name);
		}

		it = model_to_attribute_map.find(ShapefileAttributes::model_properties[ShapefileAttributes::DESCRIPTION]);
		if (it != model_to_attribute_map.constEnd())
		{
			attribute = get_qvariant_from_finder(it.value(),feature_handle);
			QString description = attribute.toString();
			append_description_to_feature(feature_handle,description);
		}

		// FIXME : allow only one of the begin/end pair to be provided.
		if (age_of_appearance && age_of_disappearance){
			append_geo_time_to_feature(feature_handle,*age_of_appearance,*age_of_disappearance);
		}



	}

	void
	remap_feature_collection(
		GPlatesFileIO::FileInfo file_info,
		QMap< QString,QString > model_to_attribute_map,
		GPlatesFileIO::ReadErrorAccumulation &read_errors)
	{

		boost::optional< GPlatesModel::FeatureCollectionHandle::weak_ref > collection =
			file_info.get_feature_collection();

		QString filename = file_info.get_qfileinfo().filePath();

		GPlatesModel::FeatureCollectionHandle::features_iterator it = (*collection)->features_begin();
		GPlatesModel::FeatureCollectionHandle::features_iterator it_end = (*collection)->features_end();
		int count = 0;
		for ( ; it != it_end ; ++it)
		{
			boost::shared_ptr<GPlatesFileIO::DataSource> source(
				new GPlatesFileIO::LocalFileDataSource(filename, GPlatesFileIO::DataFormats::Shapefile));
			boost::shared_ptr<GPlatesFileIO::LocationInDataSource> location(
				new GPlatesFileIO::LineNumberInFile(count));
			GPlatesModel::FeatureHandle::weak_ref feature_handle = (*it)->reference();
			remove_old_properties(feature_handle);
			map_attributes_to_properties(feature_handle,model_to_attribute_map,read_errors,source,location);
			count++;
		} 
	}


	/**
	 * Given a shapefile name in the form <name>.shp , this will produce a filename of the form
	 * <name>.gplates.xml
	 */
	QString
	make_shapefile_xml_filename(
		QFileInfo file_info)
	{

		QString shapefile_xml_filename = file_info.absoluteFilePath() + ".gplates.xml";

		return shapefile_xml_filename;
	}

	/**
	 * Fills the QMap<QString,QString> from the given xml file. 
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
	 * Writes the data in the QMap<QString,QString> to an xml file.
	 */
	void
	save_attribute_map_as_xml_file(
		QString filename,
		QMap<QString,QString> &model_to_attribute_map)
	{
		GPlatesFileIO::ShapefileXmlWriter writer;
		if (!writer.write_file(filename,&model_to_attribute_map))
		{
			QMessageBox::warning(0,QObject::tr("ShapefileXmlWriter"),
				QObject::tr("Cannot write to file %1.")
					.arg(filename));
		};
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


	typedef std::map<QString, std::pair<QString,QString> > feature_map_type;
	typedef feature_map_type::const_iterator feature_map_const_iterator;

	const feature_map_type &
	build_feature_map()
	{

		// The data for the following map has been taken from 
		// 1. (feature-type-to-two-letter-code) The "build_feature_map_type" map in PlatesLineFormatReader.cc 
		// 2. (geometry-type-to-feature-type) The various "create_*" functions in PlatesLinesFormatReader.cc
		//
		// FIXME: we should get this information from a common source, rather than having two independent sources.  
		static feature_map_type map;
		map["AR"] = std::make_pair("AseismicRidge","centerLineOf");
		map["BA"] = std::make_pair("Bathymetry","centerLineOf");
		map["BS"] = std::make_pair("Basin","outlineOf");
		map["CB"] = std::make_pair("PassiveContinentalBoundary","centerLineOf");
		map["CF"] = std::make_pair("ContinentalFragment","outlineOf");
		map["CM"] = std::make_pair("PassiveConinentalBoundary","centerLineOf");
		map["CO"] = std::make_pair("PassiveContinentalBoundary","centerLineOf");
		map["CR"] = std::make_pair("Craton","outlineOf");
		map["CS"] = std::make_pair("Coastline","centerLineOf");
		map["EC"] = std::make_pair("ExtendedContinentalCrust","centerLineOf");
		map["FT"] = std::make_pair("Fault","centerLineOf");
		map["FZ"] = std::make_pair("FractureZone","centerLineOf");
		map["GR"] = std::make_pair("OldPlatesGridMark","centerLineOf");
		map["GV"] = std::make_pair("Gravimetry","outlineOf");
		map["HF"] = std::make_pair("HeatFlow","outlineOf");
		map["HS"] = std::make_pair("HotSpot","position");
		map["HT"] = std::make_pair("HotSpotTrail","unclassifiedGeometry");
		map["IA"] = std::make_pair("IslandArc","outlineOf");
		map["IC"] = std::make_pair("Isochron","centerLineOf");
		map["IM"] = std::make_pair("Isochron","centerLineOf");
		map["IP"] = std::make_pair("SedimentThickness","outlineOf");
		map["IR"] = std::make_pair("IslandArc","centerLineOf");
		map["IS"] = std::make_pair("UnclassifiedFeature","centerLineOf");
		map["LI"] = std::make_pair("GeologicalLineation","centerLineOf");
		map["MA"] = std::make_pair("Magnetics","outlineOf");
		map["NF"] = std::make_pair("gpmlFault","centerLineOf");
		map["OB"] = std::make_pair("OrogenicBelt","centerLineOf");
		map["OP"] = std::make_pair("BasicRockUnit","outlineOf");
		map["OR"] = std::make_pair("OrogenicBelt","centerLineOf");
		map["PB"] = std::make_pair("InferredPaleoBoundary","centerLineOf");
		map["PC"] = std::make_pair("MagneticAnomalyIdentification","position");
		map["PM"] = std::make_pair("MagneticAnomalyIdentification","position");
		map["RA"] = std::make_pair("IslandArc","centerLineOf");
		map["RF"] = std::make_pair("Fault","centerLineOf");
		map["RI"] = std::make_pair("MidOceanRidge","centerLineOf");
		map["SM"] = std::make_pair("Seamount","unclassifiedGeometry");
		map["SS"] = std::make_pair("Fault","centerLineOf");
		map["SU"] = std::make_pair("Suture","centerLineOf");
		map["TB"] = std::make_pair("TerraneBoundary","centerLineOf");
		map["TC"] = std::make_pair("TransitionalCrust","outlineOf");
		map["TF"] = std::make_pair("Transform","centerLineOf");
		map["TH"] = std::make_pair("Fault","centerLineOf");
		map["TO"] = std::make_pair("Topography","outlineOf");
		map["TR"] = std::make_pair("SubductionZone","centerLineOf");
		map["UN"] = std::make_pair("UnclassifiedFeature","unclassifiedGeometry");
		map["VO"] = std::make_pair("Volcano","unclassifiedGeometry");
		map["VP"] = std::make_pair("LargeIgneousProvince","outlineOf");
		map["XR"] = std::make_pair("MidOceanRidge","centerLineOf");
		map["XT"] = std::make_pair("SubductionZone","centerLineOf");

		return map;
	}

}


GPlatesFileIO::ShapeFileReader::ShapeFileReader():
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

GPlatesFileIO::ShapeFileReader::~ShapeFileReader()
{
	try {
		if(d_data_source_ptr){
			OGRDataSource::DestroyDataSource(d_data_source_ptr);
		}
	}
	catch (...) {
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
		
	OGRFeature::DestroyFeature(d_feature_ptr);
	return true;
}

bool
GPlatesFileIO::ShapeFileReader::open_file(
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
GPlatesFileIO::ShapeFileReader::read_features(
	GPlatesModel::ModelInterface &model,
	GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
	ReadErrorAccumulation &read_errors)
{

	if (!d_layer_ptr){
		// we shouldn't really be here
		std::cerr << "null d_layer_ptr in read_features" << std::endl;
	}

	d_total_features = d_layer_ptr->GetFeatureCount();

	int feature_number = 0; // for error reporting.

	boost::shared_ptr<GPlatesFileIO::DataSource> e_source(
		new GPlatesFileIO::LocalFileDataSource(d_filename, GPlatesFileIO::DataFormats::Shapefile));

	static const feature_map_type &feature_map = build_feature_map();
	d_feature_creation_pair = std::make_pair("UnclassifiedFeature",
			"unclassifiedGeometry");

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

				feature_map_const_iterator result = feature_map.find(feature_string);
				if (result != feature_map.end()) {
					d_feature_creation_pair = result->second;
				} else {
					read_errors.d_warnings.push_back(GPlatesFileIO::ReadErrorOccurrence(e_source, e_location, 
					GPlatesFileIO::ReadErrors::UnrecognisedShapefileFeatureType,
					GPlatesFileIO::ReadErrors::UnclassifiedShapefileFeatureCreated));
				}
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

GPlatesModel::FeatureHandle::weak_ref
GPlatesFileIO::ShapeFileReader::create_polygon_feature_from_list(
	GPlatesModel::ModelInterface &model,
	GPlatesModel::FeatureCollectionHandle::weak_ref collection,
	std::list<GPlatesMaths::PointOnSphere> &list_of_points)
{

	GPlatesModel::FeatureType feature_type = 
		GPlatesModel::FeatureType::create_gpml(d_feature_creation_pair.first);

	GPlatesModel::FeatureHandle::weak_ref feature_handle =
			model->create_feature(feature_type,collection);

	GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere =
		GPlatesMaths::PolygonOnSphere::create_on_heap(list_of_points);

	GPlatesPropertyValues::GmlPolygon::non_null_ptr_type gml_polygon =
		GPlatesPropertyValues::GmlPolygon::create(polygon_on_sphere);

	GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type property_value =
		GPlatesModel::ModelUtils::create_gpml_constant_value(
							gml_polygon, 
							GPlatesPropertyValues::TemplateTypeParameterType::create_gml("Polygon"));

	// Anything that's got a polygon geometry is going to get an "outlineOf" property name. 
	// What happens if the feature type expects a different geometry type?
	GPlatesModel::ModelUtils::append_property_value_to_feature(property_value,
//		GPlatesModel::PropertyName::create_gpml("outlineOf"), feature_handle);
		GPlatesModel::PropertyName::create_gpml("unclassifiedGeometry"), feature_handle);

	return feature_handle;

}

GPlatesModel::FeatureHandle::weak_ref
GPlatesFileIO::ShapeFileReader::create_line_feature_from_list(
	GPlatesModel::ModelInterface &model,
	GPlatesModel::FeatureCollectionHandle::weak_ref collection,
	std::list<GPlatesMaths::PointOnSphere> &list_of_points)
{

	GPlatesModel::FeatureType feature_type = 
		GPlatesModel::FeatureType::create_gpml(d_feature_creation_pair.first);

	GPlatesModel::FeatureHandle::weak_ref feature_handle =
			model->create_feature(feature_type,collection);

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

	// Anything with a polyline geometry is going to get a "centerLineOf" property name. 
	GPlatesModel::ModelUtils::append_property_value_to_feature(property_value,
//					GPlatesModel::PropertyName::create_gpml("centerLineOf"), feature_handle);
					GPlatesModel::PropertyName::create_gpml("unclassifiedGeometry"), feature_handle);

	return feature_handle;
}


GPlatesModel::FeatureHandle::weak_ref
GPlatesFileIO::ShapeFileReader::create_point_feature_from_pair(
	GPlatesModel::ModelInterface &model, 
	GPlatesModel::FeatureCollectionHandle::weak_ref collection, 
	std::pair<double,double> &point)
{
	GPlatesModel::FeatureType feature_type = 
		GPlatesModel::FeatureType::create_gpml(d_feature_creation_pair.first);

	GPlatesModel::FeatureHandle::weak_ref feature_handle =
			model->create_feature(feature_type,collection);

	const GPlatesModel::PropertyValue::non_null_ptr_type gml_point =
		GPlatesPropertyValues::GmlPoint::create(point);

	GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type property_value =
		GPlatesModel::ModelUtils::create_gpml_constant_value(
							gml_point, 
							GPlatesPropertyValues::TemplateTypeParameterType::create_gml("Point"));

	// What sort of gpml property name should a point have? 
	GPlatesModel::ModelUtils::append_property_value_to_feature(property_value,
		GPlatesModel::PropertyName::create_gpml("unclassifiedGeometry"), feature_handle);

	return feature_handle;

}


GPlatesModel::FeatureHandle::weak_ref
GPlatesFileIO::ShapeFileReader::create_point_feature_from_point_on_sphere(
	GPlatesModel::ModelInterface &model, 
	GPlatesModel::FeatureCollectionHandle::weak_ref collection, 
	GPlatesMaths::PointOnSphere &point)
{

	GPlatesModel::FeatureType feature_type =
		GPlatesModel::FeatureType::create_gpml(d_feature_creation_pair.first);

	GPlatesModel::FeatureHandle::weak_ref feature_handle =
			model->create_feature(feature_type,collection);

	const GPlatesModel::PropertyValue::non_null_ptr_type gml_point =
		GPlatesPropertyValues::GmlPoint::create(point);

	GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type property_value =
		GPlatesModel::ModelUtils::create_gpml_constant_value(
							gml_point, 
							GPlatesPropertyValues::TemplateTypeParameterType::create_gml("Point"));

	// What sort of gpml property name should a point have? 
	// I'm going to leave it as an unclassifiedGeometry for now. 
	GPlatesModel::ModelUtils::append_property_value_to_feature(property_value,
		GPlatesModel::PropertyName::create_gpml("unclassifiedGeometry"), feature_handle);

	return feature_handle;

}

GPlatesModel::FeatureHandle::weak_ref
GPlatesFileIO::ShapeFileReader::create_multi_point_feature_from_list(
	GPlatesModel::ModelInterface &model,
	GPlatesModel::FeatureCollectionHandle::weak_ref collection,
	std::list<GPlatesMaths::PointOnSphere> &list_of_points)
{

	GPlatesModel::FeatureType feature_type =
		GPlatesModel::FeatureType::create_gpml(d_feature_creation_pair.first);

	GPlatesModel::FeatureHandle::weak_ref feature_handle =
			model->create_feature(feature_type,collection);

	GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere =
		GPlatesMaths::MultiPointOnSphere::create_on_heap(list_of_points);

	GPlatesPropertyValues::GmlMultiPoint::non_null_ptr_type gml_multi_point =
		GPlatesPropertyValues::GmlMultiPoint::create(multi_point_on_sphere);

	GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type property_value =
		GPlatesModel::ModelUtils::create_gpml_constant_value(
							gml_multi_point, 
							GPlatesPropertyValues::TemplateTypeParameterType::create_gml("MultiPoint"));

	GPlatesModel::ModelUtils::append_property_value_to_feature(property_value,
		GPlatesModel::PropertyName::create_gpml("unclassifiedGeometry"), feature_handle);


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
GPlatesFileIO::ShapeFileReader::get_attributes()
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
GPlatesFileIO::ShapeFileReader::add_attributes_to_feature(
	GPlatesModel::FeatureHandle::weak_ref feature_handle,
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
		feature_handle);

	// Map the shapefile attributes to model properties.
	map_attributes_to_properties(feature_handle,s_model_to_attribute_map,read_errors,source,location);

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

void
GPlatesFileIO::ShapeFileReader::read_file(
		FileInfo &fileinfo,
		GPlatesModel::ModelInterface &model,
		ReadErrorAccumulation &read_errors)
{
	QString absolute_path_filename = fileinfo.get_qfileinfo().absoluteFilePath();
	QString filename = fileinfo.get_qfileinfo().fileName();

	ShapeFileReader reader;
	if (!reader.open_file(absolute_path_filename)){
		throw ErrorOpeningFileForReadingException(filename);
	}

	if (!reader.check_file_format(read_errors)){
		throw ErrorOpeningFileForReadingException(filename);
	}
	reader.get_field_names(read_errors);

	QString shapefile_xml_filename = make_shapefile_xml_filename(fileinfo.get_qfileinfo());

	s_model_to_attribute_map.clear();

	if (!fill_attribute_map_from_xml_file(shapefile_xml_filename,s_model_to_attribute_map))
	{
		// Set the last argument to false, because this is an initial mapping, not a re-mapping. 
		if (!fill_attribute_map_from_dialog(filename,s_field_names,s_model_to_attribute_map,s_property_mapper,false))
		{
			// The user has cancelled the mapper-dialog routine, so cancel the whole shapefile loading procedure.
			return;
		}
		save_attribute_map_as_xml_file(shapefile_xml_filename,s_model_to_attribute_map);
	}

	GPlatesModel::FeatureCollectionHandle::weak_ref collection
		= model->create_feature_collection();

	reader.read_features(model,collection,read_errors);


	//reader.display_feature_counts();

	fileinfo.set_feature_collection(collection);
	return;
}

void
GPlatesFileIO::ShapeFileReader::set_property_mapper(
	boost::shared_ptr< PropertyMapper > property_mapper)
{
	s_property_mapper = property_mapper;
}


void
GPlatesFileIO::ShapeFileReader::handle_point(
		GPlatesModel::ModelInterface &model,
		GPlatesModel::FeatureCollectionHandle::weak_ref collection,
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
					GPlatesModel::FeatureHandle::weak_ref feature_handle = create_point_feature_from_point_on_sphere(model,collection,p);
					add_attributes_to_feature(feature_handle,read_errors,source,location);
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
GPlatesFileIO::ShapeFileReader::handle_multi_point(
			GPlatesModel::ModelInterface &model,
			GPlatesModel::FeatureCollectionHandle::weak_ref collection,
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
				GPlatesModel::FeatureHandle::weak_ref feature_handle = create_multi_point_feature_from_list(model,collection,list_of_points);
				add_attributes_to_feature(feature_handle,read_errors,source,location);
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
		GPlatesModel::FeatureHandle::weak_ref feature_handle = create_line_feature_from_list(model,collection,feature_points);
		add_attributes_to_feature(feature_handle,read_errors,source,location);
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
GPlatesFileIO::ShapeFileReader::handle_multi_linestring(
			GPlatesModel::ModelInterface &model,
			GPlatesModel::FeatureCollectionHandle::weak_ref collection,
			ReadErrorAccumulation &read_errors,
			const boost::shared_ptr<GPlatesFileIO::DataSource> &source,
			const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> &location)
{
		OGRMultiLineString *multi = static_cast<OGRMultiLineString*>(d_geometry_ptr);
		int num_geometries = multi->getNumGeometries();
		d_total_geometries += num_geometries;
		for (int multiCount = 0; multiCount < num_geometries ; multiCount++){

			std::list<GPlatesMaths::PointOnSphere> feature_points;
			OGRLineString *linestring = static_cast<OGRLineString*>(multi->getGeometryRef(multiCount));
		
			int num_points = linestring->getNumPoints();
			if (num_points < 2){	
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
					GPlatesModel::FeatureHandle::weak_ref feature_handle = create_line_feature_from_list(model,collection,feature_points);				
					add_attributes_to_feature(feature_handle,read_errors,source,location);
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
GPlatesFileIO::ShapeFileReader::handle_polygon(
			GPlatesModel::ModelInterface &model,
			GPlatesModel::FeatureCollectionHandle::weak_ref collection,
			ReadErrorAccumulation &read_errors,
			const boost::shared_ptr<GPlatesFileIO::DataSource> &source,
			const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> &location)
{
	std::list<GPlatesMaths::PointOnSphere> feature_points;
	OGRPolygon *polygon = static_cast<OGRPolygon*>(d_geometry_ptr);
	d_total_geometries++;

	OGRLinearRing *ring = polygon->getExteriorRing();
	add_ring_to_points_list(ring,feature_points,read_errors,source,location);

	// polygon geometries are stored in the model as linestrings
	// Not any more! Now we have polygons... For now, all the rings will be stored as separate polygons. 

	if (!feature_points.empty()){
		GPlatesModel::FeatureHandle::weak_ref feature_handle = create_polygon_feature_from_list(model,collection,feature_points);				
		add_attributes_to_feature(feature_handle,read_errors,source,location);
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
				GPlatesModel::FeatureHandle::weak_ref feature_handle = create_polygon_feature_from_list(model,collection,feature_points);				
				add_attributes_to_feature(feature_handle,read_errors,source,location);
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
GPlatesFileIO::ShapeFileReader::handle_multi_polygon(
			GPlatesModel::ModelInterface &model,
			GPlatesModel::FeatureCollectionHandle::weak_ref collection,
			ReadErrorAccumulation &read_errors,
			const boost::shared_ptr<GPlatesFileIO::DataSource> &source,
			const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> &location)
{
	//std::cerr << "Multi-polygon" << std::endl;
	OGRMultiPolygon *multi = static_cast<OGRMultiPolygon*>(d_geometry_ptr);
	int num_geometries = multi->getNumGeometries();

	d_total_geometries += num_geometries;
	for (int multiCount = 0; multiCount < num_geometries ; multiCount++){
		//std::cerr << "Polygon number: " << multiCount << std::endl;
		std::list<GPlatesMaths::PointOnSphere> feature_points;
		OGRPolygon *polygon = static_cast<OGRPolygon*>(multi->getGeometryRef(multiCount));

		OGRLinearRing *ring = polygon->getExteriorRing();

		add_ring_to_points_list(ring,feature_points,read_errors,source,location);

		if (!feature_points.empty()){
			GPlatesModel::FeatureHandle::weak_ref feature_handle = create_polygon_feature_from_list(model,collection,feature_points);				
			add_attributes_to_feature(feature_handle,read_errors,source,location);
			d_loaded_geometries++;
		}

		int num_interior_rings = polygon->getNumInteriorRings();

		for (int ring_count = 0; ring_count < num_interior_rings; ring_count++)
		{
			//std::cerr << "Multi-polygon Interior ring: " << ring_count << std::endl;
			feature_points.clear();

			ring = polygon->getInteriorRing(ring_count);

			add_ring_to_points_list(ring,feature_points,read_errors,source,location);

			if (!feature_points.empty()){
				GPlatesModel::FeatureHandle::weak_ref feature_handle = create_polygon_feature_from_list(model,collection,feature_points);				
				add_attributes_to_feature(feature_handle,read_errors,source,location);
				d_loaded_geometries++;
			}

		} // loop over interior rings

	} // loop over multipolygons
}

void
GPlatesFileIO::ShapeFileReader::display_feature_counts()
{

	std::cerr << "feature/geometry count: " <<
			d_total_features << ", " <<
			d_loaded_geometries << ", " <<
			d_total_geometries << std::endl;

}


void
GPlatesFileIO::ShapeFileReader::add_ring_to_points_list(
	OGRLinearRing *ring, 
	std::list<GPlatesMaths::PointOnSphere> &feature_points,
	ReadErrorAccumulation &read_errors,
	const boost::shared_ptr<GPlatesFileIO::DataSource> &source,
	const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> &location)
{
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
GPlatesFileIO::ShapeFileReader::remap_shapefile_attributes(
	GPlatesFileIO::FileInfo &fileinfo,
	GPlatesModel::ModelInterface &model,
	ReadErrorAccumulation &read_errors)
{
	QString absolute_path_filename = fileinfo.get_qfileinfo().absoluteFilePath();
	QString filename = fileinfo.get_qfileinfo().fileName();

	ShapeFileReader reader;
	if (!reader.open_file(absolute_path_filename)){
		throw ErrorOpeningFileForReadingException(filename);
	}

	if (!reader.check_file_format(read_errors)){
		throw ErrorOpeningFileForReadingException(filename);
	}

	reader.get_field_names(read_errors);

	QString shapefile_xml_filename = make_shapefile_xml_filename(fileinfo.get_qfileinfo());

	s_model_to_attribute_map.clear();

	fill_attribute_map_from_xml_file(shapefile_xml_filename,s_model_to_attribute_map);

	// Set the last argument to true because we are remapping. 
	if (!fill_attribute_map_from_dialog(filename,s_field_names,s_model_to_attribute_map,s_property_mapper,true))
	{
		// The user has cancelled the mapper-dialog, so cancel the whole shapefile re-mapping procedure.
		return;
	}
	save_attribute_map_as_xml_file(shapefile_xml_filename,s_model_to_attribute_map);

	if (fileinfo.get_feature_collection())
	{
		remap_feature_collection(
								fileinfo,
								s_model_to_attribute_map,
								read_errors);
	}
}
