/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007, 2008, 2009 Geological Survey of Norway (under the name "ShapefileReader.h")
 * Copyright (C) 2010 The University of Sydney, Australia (under the name "ShapefileReader.h")
 * Copyright (C) 2012, 2013, 2014, 2015 Geological Survey of Norway
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

#include "Gdal.h"
#include "GdalUtils.h"
#include "ErrorOpeningFileForReadingException.h"
#include "FeatureCollectionFileFormat.h"
#include "FeatureCollectionFileFormatConfigurations.h"
#include "FileLoadAbortedException.h"
#include "OgrReader.h"
#include "OgrUtils.h"
#include "PropertyMapper.h"
#include "ShapefileXmlReader.h"

#include "feature-visitors/PropertyValueFinder.h" 
#include "feature-visitors/ShapefileAttributeFinder.h"

#include "model/ChangesetHandle.h"
#include "model/Gpgim.h"
#include "model/GpgimFeatureClass.h"
#include "model/Model.h"
#include "model/ModelUtils.h"
#include "model/NotificationGuard.h"
#include "model/QualifiedXmlName.h"
#include "model/XmlAttributeName.h"
#include "model/XmlAttributeValue.h"

#include "property-values/GeoTimeInstant.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlPolygon.h"
#include "property-values/GmlTimeInstant.h"
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

#include "utils/Profile.h"
#include "utils/UnicodeStringUtils.h"


boost::shared_ptr< GPlatesFileIO::PropertyMapper> GPlatesFileIO::OgrReader::s_property_mapper;

namespace
{

	/**
	 * @brief recon_method_is_valid returns true if @a recon_method is "ByPlateID",
	 * "HalfStageRotation", "HalfStageRotationVersion2" or "HalfStageRotationVersion3".
	 *
	 * Note that currently (Feb 2014) only HalfStageRotationVersion2 gets exported by any part of GPlates. ByPlateID
	 * is considered as the default and is not explicitly exported in any of GPlates' export functionality, and
	 * features will not normally contain a gpml:reconstructionMethod property. In this case shapefile export
	 * will write an empty string as the reconstructionMethod. As this returns false from this function,
	 * no reconstructionMethod will be added to the feature, and so the normal by-plate-id reconstruction method
	 * will be used.
	 */
	bool
	recon_method_is_valid(
			const QString &recon_method)
	{
		if (recon_method == "ByPlateID" ||
			recon_method == "HalfStageRotation" ||
			recon_method == "HalfStageRotationVersion2" ||
			recon_method == "HalfStageRotationVersion3")
		{
			return true;
		}
		return false;
	}


	const GPlatesPropertyValues::GeoTimeInstant
	create_geo_time_instant(
			const double &time)
	{
		if (time < -998.9 && time > -1000.0)
		{
			// It's in the distant future, which is denoted in PLATES4 line-format
			// files using times like -999.0 or -999.9.
			return GPlatesPropertyValues::GeoTimeInstant::create_distant_future();
		}
		if (time > 998.9 && time < 1000.0)
		{
			// It's in the distant past, which is denoted in PLATES4 line-format files
			// using times like 999.0 or 999.9.
			return GPlatesPropertyValues::GeoTimeInstant::create_distant_past();
		}

		return GPlatesPropertyValues::GeoTimeInstant(time);
	}

	const GPlatesPropertyValues::GeoTimeInstant
	create_begin_geo_time_instant(
			const boost::optional<double> &time)
	{
		if (time)
		{
			return create_geo_time_instant(time.get());
		}

		return GPlatesPropertyValues::GeoTimeInstant::create_distant_past();
	}

	const GPlatesPropertyValues::GeoTimeInstant
	create_end_geo_time_instant(
			const boost::optional<double> &time)
	{
		if (time)
		{
			return create_geo_time_instant(time.get());
		}

		return GPlatesPropertyValues::GeoTimeInstant::create_distant_future();
	}



	/**
	 * Creates a gml line string from @a list_of_points and adds this to @a feature.
	 */
	void
	add_polyline_geometry_to_feature(
		const GPlatesModel::FeatureHandle::weak_ref &feature,
		const std::vector<GPlatesMaths::PointOnSphere> &list_of_points,
		const boost::optional<GPlatesModel::GpgimProperty::non_null_ptr_to_const_type> &property)
	{
		GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline =
			GPlatesMaths::PolylineOnSphere::create_on_heap(list_of_points);

		GPlatesPropertyValues::GmlLineString::non_null_ptr_type gml_line_string =
			GPlatesPropertyValues::GmlLineString::create(polyline);

		GPlatesPropertyValues::GmlOrientableCurve::non_null_ptr_type gml_orientable_curve =
			GPlatesModel::ModelUtils::create_gml_orientable_curve(gml_line_string);

		GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type property_value =
			GPlatesModel::ModelUtils::create_gpml_constant_value(gml_orientable_curve);

		boost::optional<GPlatesModel::PropertyName> property_name;
		if (property)
		{
			property_name = (*property)->get_property_name();
		}
		else
		{
			property_name.reset(GPlatesModel::PropertyName::create_gpml("unclassifiedGeometry"));
		}

		feature->add(
				GPlatesModel::TopLevelPropertyInline::create(
					*property_name,
					property_value));
	}

	/**
	 * Creates a gml polygon from @a list_of_points and adds this to @a feature.
	 */
	void
	add_polygon_geometry_to_feature(		
		const GPlatesModel::FeatureHandle::weak_ref &feature,
		const std::vector<GPlatesMaths::PointOnSphere> &exterior_ring,
		const std::list< std::vector<GPlatesMaths::PointOnSphere> > &interior_rings,
		const boost::optional<GPlatesModel::GpgimProperty::non_null_ptr_to_const_type> &property)
	{
		GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon =
			GPlatesMaths::PolygonOnSphere::create_on_heap(exterior_ring, interior_rings);

		GPlatesPropertyValues::GmlPolygon::non_null_ptr_type gml_polygon =
			GPlatesPropertyValues::GmlPolygon::create(polygon);


		GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type property_value =
			GPlatesModel::ModelUtils::create_gpml_constant_value(gml_polygon);

		boost::optional<GPlatesModel::PropertyName> property_name;
		if (property)
		{
			property_name = (*property)->get_property_name();
		}
		else
		{
			property_name.reset(GPlatesModel::PropertyName::create_gpml("unclassifiedGeometry"));
		}

		feature->add(
				GPlatesModel::TopLevelPropertyInline::create(
					*property_name,
					property_value));
	}

	/**
	 * Creates a feature of type specified in @a feature_creation_pair, and adds it to @a collection. 
	 *
	 * @return a feature handle to the created feature. 
	 */
	const GPlatesModel::FeatureHandle::weak_ref
	create_feature(
		const GPlatesModel::FeatureType &feature_type,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
		const QString &feature_type_qstring,
		boost::optional<GPlatesUtils::UnicodeString> &feature_id)
	{

		if (feature_id)
		{
			return GPlatesModel::FeatureHandle::create(
					collection,
					feature_type,
					GPlatesModel::FeatureId(*feature_id));
		}
		else
		{
			return GPlatesModel::FeatureHandle::create(
					collection,
					feature_type);
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
	append_conjugate_plate_id_to_feature(
		const GPlatesModel::FeatureHandle::weak_ref &feature,
		int conjugate_plate_id_as_int)
	{
		GPlatesPropertyValues::GpmlPlateId::non_null_ptr_type conjugate_plate_id = 
			GPlatesPropertyValues::GpmlPlateId::create(conjugate_plate_id_as_int);

		feature->add(
				GPlatesModel::TopLevelPropertyInline::create(
					GPlatesModel::PropertyName::create_gpml("conjugatePlateId"),
					conjugate_plate_id));
	}

	void
	append_left_plate_id_to_feature(
			const GPlatesModel::FeatureHandle::weak_ref &feature,
			int left_plate_id_as_int)
	{
		GPlatesPropertyValues::GpmlPlateId::non_null_ptr_type left_plate_id =
				GPlatesPropertyValues::GpmlPlateId::create(left_plate_id_as_int);

		feature->add(
					GPlatesModel::TopLevelPropertyInline::create(
						GPlatesModel::PropertyName::create_gpml("leftPlate"),
						left_plate_id));
	}

	void
	append_right_plate_id_to_feature(
			const GPlatesModel::FeatureHandle::weak_ref &feature,
			int right_plate_id_as_int)
	{
		GPlatesPropertyValues::GpmlPlateId::non_null_ptr_type right_plate_id =
				GPlatesPropertyValues::GpmlPlateId::create(right_plate_id_as_int);

		feature->add(
					GPlatesModel::TopLevelPropertyInline::create(
						GPlatesModel::PropertyName::create_gpml("rightPlate"),
						right_plate_id));
	}

	void
	append_recon_method_to_feature(
			const GPlatesModel::FeatureHandle::weak_ref &feature,
			const QString &recon_method)
	{
		GPlatesPropertyValues::Enumeration::non_null_ptr_type recon_method_property_value =
				GPlatesPropertyValues::Enumeration::create(
						GPlatesPropertyValues::EnumerationType::create_gpml("ReconstructionMethodEnumeration"),
						GPlatesUtils::make_icu_string_from_qstring(recon_method));

		feature->add(
					GPlatesModel::TopLevelPropertyInline::create(
						GPlatesModel::PropertyName::create_gpml("reconstructionMethod"),
						recon_method_property_value));
	}

	void
	append_spreading_asymmetry_to_feature(
			const GPlatesModel::FeatureHandle::weak_ref &feature,
			double spreading_asymmetry)
	{
		GPlatesPropertyValues::XsDouble::non_null_ptr_type spreading_asymmetry_property_value =
				GPlatesPropertyValues::XsDouble::create(spreading_asymmetry);

		feature->add(
					GPlatesModel::TopLevelPropertyInline::create(
						GPlatesModel::PropertyName::create_gpml("spreadingAsymmetry"),
						spreading_asymmetry_property_value));
	}

	void
	append_plate_id_to_feature(
		const GPlatesModel::FeatureHandle::weak_ref &feature,
		int plate_id_as_int)
	{
		GPlatesPropertyValues::GpmlPlateId::non_null_ptr_type plate_id = 
			GPlatesPropertyValues::GpmlPlateId::create(plate_id_as_int);
		feature->add(
				GPlatesModel::TopLevelPropertyInline::create(
					GPlatesModel::PropertyName::create_gpml("reconstructionPlateId"),
					GPlatesModel::ModelUtils::create_gpml_constant_value(plate_id)));
	}

	void
	append_geo_times_to_feature(
		const GPlatesModel::FeatureHandle::weak_ref &feature,
		const boost::optional<double> &age_of_appearance,
		const boost::optional<double> &age_of_disappearance)
	{

		const GPlatesPropertyValues::GeoTimeInstant geo_time_instant_begin = create_begin_geo_time_instant(age_of_appearance);

		const GPlatesPropertyValues::GeoTimeInstant geo_time_instant_end = create_end_geo_time_instant(age_of_disappearance);

		GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type gml_valid_time = 
			GPlatesModel::ModelUtils::create_gml_time_period(geo_time_instant_begin,
			geo_time_instant_end);
		feature->add(
				GPlatesModel::TopLevelPropertyInline::create(
					GPlatesModel::PropertyName::create_gml("validTime"),
					gml_valid_time));
	}

	void
	append_geometry_import_time_to_feature(
		const GPlatesModel::FeatureHandle::weak_ref &feature,
		double geometry_import_time)
	{
		const GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_type geometry_import_time_property_value =
				GPlatesModel::ModelUtils::create_gml_time_instant(
						create_geo_time_instant(geometry_import_time));
		feature->add(
				GPlatesModel::TopLevelPropertyInline::create(
					GPlatesModel::PropertyName::create_gpml("geometryImportTime"),
					geometry_import_time_property_value));
	}

	void
	append_name_to_feature(
		const GPlatesModel::FeatureHandle::weak_ref &feature,
		QString name)
	{
		GPlatesPropertyValues::XsString::non_null_ptr_type gml_name = 
			GPlatesPropertyValues::XsString::create(GPlatesUtils::UnicodeString(name.toStdString().c_str()));
		feature->add(
				GPlatesModel::TopLevelPropertyInline::create(
					GPlatesModel::PropertyName::create_gml("name"),
					gml_name));
	}

	void
	append_description_to_feature(
		const GPlatesModel::FeatureHandle::weak_ref &feature,
		QString description)
	{
		GPlatesPropertyValues::XsString::non_null_ptr_type gml_description = 
			GPlatesPropertyValues::XsString::create(GPlatesUtils::UnicodeString(description.toStdString().c_str()));
		feature->add(
				GPlatesModel::TopLevelPropertyInline::create(
					GPlatesModel::PropertyName::create_gml("description"),
					gml_description));
	}
	

	/**
	 * Removes properties with the property names:
	 *		reconstructionPlateId,
	 *		validTime,
	 *		description
	 *		name
	 *		conjugatePlateId
	 *		reconstructionMethod
	 *		leftPlate
	 *		rightPlate
	 *		spreadingAsymmetry
	 *		geometryImportTime
	 *	from the feature given by @a feature_handle.
	 *
	 * This is used when re-mapping model properties from shapefile attributes.
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
		property_name_list << QString("conjugatePlateId");
		property_name_list << QString("reconstructionMethod");
		property_name_list << QString("leftPlate");
		property_name_list << QString("rightPlate");
		property_name_list << QString("spreadingAsymmetry");
		property_name_list << QString("geometryImportTime");

		GPlatesModel::FeatureHandle::iterator p_iter = feature->begin();
		GPlatesModel::FeatureHandle::iterator p_iter_end = feature->end();

		for ( ; p_iter != p_iter_end ; ++p_iter)
		{
			GPlatesModel::PropertyName property_name = (*p_iter)->get_property_name();
			QString q_prop_name = GPlatesUtils::make_qstring_from_icu_string(property_name.get_name());
			if (property_name_list.contains(q_prop_name))
			{
				feature->remove(p_iter);
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
		const QMap<QString,QString> &model_to_attribute_map,
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
					GPlatesFileIO::ReadErrors::NoPlateIdCreatedForFeature));

			}

		}


		boost::optional<double> age_of_appearance;
		boost::optional<double> age_of_disappearance;

		it = model_to_attribute_map.find(
				ShapefileAttributes::model_properties[ShapefileAttributes::BEGIN]);
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

		it = model_to_attribute_map.find(
				ShapefileAttributes::model_properties[ShapefileAttributes::END]);
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

		append_geo_times_to_feature(feature,age_of_appearance,age_of_disappearance);


		it = model_to_attribute_map.find(
				ShapefileAttributes::model_properties[ShapefileAttributes::NAME]);
		if (it != model_to_attribute_map.constEnd())
		{
			attribute = get_qvariant_from_finder(it.value(),feature);
			QString name = attribute.toString();
			append_name_to_feature(feature,name);
		}

		it = model_to_attribute_map.find(
				ShapefileAttributes::model_properties[ShapefileAttributes::DESCRIPTION]);
		if (it != model_to_attribute_map.constEnd())
		{
			attribute = get_qvariant_from_finder(it.value(),feature);
			QString description = attribute.toString();
			append_description_to_feature(feature,description);
		}


		
		it = model_to_attribute_map.find(
				ShapefileAttributes::model_properties[ShapefileAttributes::CONJUGATE_PLATE_ID]);

		if (it != model_to_attribute_map.constEnd())
		{
			attribute = get_qvariant_from_finder(it.value(),feature);
			bool ok;
			int conjugate_plate_id_as_int = attribute.toInt(&ok);
			if (ok){
				append_conjugate_plate_id_to_feature(feature,conjugate_plate_id_as_int);
			}
			else{
				read_errors.d_warnings.push_back(
					GPlatesFileIO::ReadErrorOccurrence(
					source,
					location,
					GPlatesFileIO::ReadErrors::InvalidShapefilePlateIdNumber,
					GPlatesFileIO::ReadErrors::NoConjugatePlateIdCreatedForFeature));
			}

		}		

		it = model_to_attribute_map.find(
				ShapefileAttributes::model_properties[ShapefileAttributes::RECONSTRUCTION_METHOD]);

		if (it != model_to_attribute_map.constEnd())
		{
			attribute = get_qvariant_from_finder(it.value(),feature);

			QString recon_method = attribute.toString();

			if ( recon_method_is_valid(recon_method)){
				append_recon_method_to_feature(feature,recon_method);
			}
			else{
				if (!recon_method.isEmpty()) // suppress warning messages for empty strings.
				{
					read_errors.d_warnings.push_back(
								GPlatesFileIO::ReadErrorOccurrence(
									source,
									location,
									GPlatesFileIO::ReadErrors::InvalidShapefileReconstructionMethod,
									GPlatesFileIO::ReadErrors::AttributeIgnored));
				}
			}

		}

		it = model_to_attribute_map.find(
					ShapefileAttributes::model_properties[ShapefileAttributes::SPREADING_ASYMMETRY]);

		if (it != model_to_attribute_map.constEnd())
		{
			attribute = get_qvariant_from_finder(it.value(),feature);
			bool ok;
			double spreading_asymmetry = attribute.toDouble(&ok);
			if (ok){
				append_spreading_asymmetry_to_feature(feature,spreading_asymmetry);
			}
			else{
				read_errors.d_warnings.push_back(
					GPlatesFileIO::ReadErrorOccurrence(
					source,
					location,
					GPlatesFileIO::ReadErrors::InvalidShapefileSpreadingAsymmetry,
					GPlatesFileIO::ReadErrors::AttributeIgnored));
			}

		}

		it = model_to_attribute_map.find(
				ShapefileAttributes::model_properties[ShapefileAttributes::LEFT_PLATE]);

		if (it != model_to_attribute_map.constEnd())
		{
			attribute = get_qvariant_from_finder(it.value(),feature);
			bool ok;
			int left_plate_id_as_int = attribute.toInt(&ok);
			if (ok){
				append_left_plate_id_to_feature(feature,left_plate_id_as_int);
			}
			else{
				read_errors.d_warnings.push_back(
					GPlatesFileIO::ReadErrorOccurrence(
					source,
					location,
					GPlatesFileIO::ReadErrors::InvalidShapefilePlateIdNumber,
					GPlatesFileIO::ReadErrors::NoLeftPlateIdCreatedForFeature));
			}

		}


		it = model_to_attribute_map.find(
			ShapefileAttributes::model_properties[ShapefileAttributes::RIGHT_PLATE]);

		if (it != model_to_attribute_map.constEnd())
		{
			attribute = get_qvariant_from_finder(it.value(),feature);
			bool ok;
			int right_plate_id_as_int = attribute.toInt(&ok);
			if (ok){
				append_right_plate_id_to_feature(feature,right_plate_id_as_int);
			}
			else{
				read_errors.d_warnings.push_back(
							GPlatesFileIO::ReadErrorOccurrence(
								source,
								location,
								GPlatesFileIO::ReadErrors::InvalidShapefilePlateIdNumber,
								GPlatesFileIO::ReadErrors::NoRightPlateIdCreatedForFeature));
			}

		}

		it = model_to_attribute_map.find(
					ShapefileAttributes::model_properties[ShapefileAttributes::GEOMETRY_IMPORT_TIME]);

		if (it != model_to_attribute_map.constEnd())
		{
			attribute = get_qvariant_from_finder(it.value(),feature);
			bool ok;
			double geometry_import_time = attribute.toDouble(&ok);
			if (ok){
				append_geometry_import_time_to_feature(feature,geometry_import_time);
			}
			else{
				read_errors.d_warnings.push_back(
					GPlatesFileIO::ReadErrorOccurrence(
					source,
					location,
					GPlatesFileIO::ReadErrors::InvalidShapefileGeometryImportTime,
					GPlatesFileIO::ReadErrors::AttributeIgnored));
			}
		}
	}

	/**
	 * Uses the @a model_to_attribute_map to create model properties from the 
	 * shapefile-attributes key-value-dictionary, for each feature in @a file's 
	 * feature collection. 
	 */
	void
	remap_feature_collection(
		GPlatesFileIO::File::Reference &file,
		const QMap< QString,QString > &model_to_attribute_map,
		GPlatesFileIO::ReadErrorAccumulation &read_errors)
	{
		QString filename = file.get_file_info().get_qfileinfo().filePath();

		GPlatesModel::FeatureCollectionHandle::weak_ref collection = file.get_feature_collection();

		GPlatesModel::FeatureCollectionHandle::iterator it = collection->begin();
		GPlatesModel::FeatureCollectionHandle::iterator it_end = collection->end();
		int count = 0;
		for ( ; it != it_end ; ++it)
		{
			boost::shared_ptr<GPlatesFileIO::DataSource> source(
				new GPlatesFileIO::LocalFileDataSource(filename, GPlatesFileIO::DataFormats::Shapefile));
			boost::shared_ptr<GPlatesFileIO::LocationInDataSource> location(
				new GPlatesFileIO::LineNumber(count));
			GPlatesModel::FeatureHandle::weak_ref feature = (*it)->reference();
			remove_old_properties(feature);
			map_attributes_to_properties(feature,model_to_attribute_map,read_errors,source,location);
			count++;
		} 
	}
	
	/**
	 * Fills the QMap<QString,QString> @a model_to_attribute_map with default field names from the
	 * list of default_attribute_names defined in "PropertyMapper.h"
	 */
	void
	fill_attribute_map_with_default_values(
			QMap< QString, QString > &model_to_attribute_map)
	{
		for (unsigned int i = 0; i < ShapefileAttributes::NUM_PROPERTIES; ++i)
		{
			model_to_attribute_map.insert(
				ShapefileAttributes::model_properties[i],
				ShapefileAttributes::default_attribute_field_names[i]);
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
		// If there's no property mapper then fill the mapping with default values.
		// If there's no property mapper then 'set_property_mapper()' was never called which
		// can happen when we not using GPlates as a GUI (eg, importing pygplates into a python
		// interpreter) - in this case we don't want to abort due to lack of a shapefile mapping.
		if (mapper == NULL)
		{
			fill_attribute_map_with_default_values(model_to_attribute_map);
			return true;
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


	/**
	 * Loads a model-to-attribute map from the specified file reference object.
	 */
	void
	load_model_to_attribute_map_from_file_reference(
			QMap<QString, QString> &model_to_attribute_map,
			GPlatesFileIO::File::Reference &file_ref)
	{
		if (!file_ref.get_feature_collection().is_valid())
		{
			qWarning() << "ERROR: Unable to load a model-to-attribute mapping from file: invalid feature collection.";
			return;
		}

		model_to_attribute_map =
				GPlatesFileIO::FeatureCollectionFileFormat::OGRConfiguration::get_model_to_attribute_map(
						*file_ref.get_feature_collection());
	}


	/**
	 * Stores the specified model-to-attribute map in the feature collection of the
	 * specified file reference object.
	 */
	void
	store_model_to_attribute_map_in_file_reference(
			const QMap<QString, QString> &model_to_attribute_map,
			GPlatesFileIO::File::Reference &file_ref)
	{
		// Get a reference to the existing model-to-attribute map in the feature collection of the file.
		QMap<QString, QString> &existing_model_to_attribute_map =
				GPlatesFileIO::FeatureCollectionFileFormat::OGRConfiguration::get_model_to_attribute_map(
						*file_ref.get_feature_collection());

		// Overwrite the existing map.
		existing_model_to_attribute_map = model_to_attribute_map;
	}
}


GPlatesFileIO::OgrReader::OgrReader():
	d_num_layers(0),
	d_data_source_ptr(NULL),
	d_geometry_ptr(NULL),
	d_feature_ptr(NULL),
	d_layer_ptr(NULL),
	d_feature_type_string("UnclassifiedFeature"),
	d_total_geometries(0),
	d_loaded_geometries(0),
	d_total_features(0),
	d_current_coordinate_transformation(GPlatesPropertyValues::CoordinateTransformation::create())
{
	GdalUtils::register_all_drivers();
}

GPlatesFileIO::OgrReader::~OgrReader()
{
	try
	{
		if (d_data_source_ptr)
		{
			GdalUtils::close_vector(d_data_source_ptr);
		}
	}
	catch (...)
	{
	}
}


bool
GPlatesFileIO::OgrReader::check_file_format(
	ReadErrorAccumulation &read_error)
{
	if (!d_data_source_ptr){
		// we should not be here
		return false;
	}

	boost::shared_ptr<GPlatesFileIO::DataSource> e_source(
		new GPlatesFileIO::LocalFileDataSource(d_filename, GPlatesFileIO::DataFormats::Shapefile));
  	boost::shared_ptr<GPlatesFileIO::LocationInDataSource> e_location(
				new GPlatesFileIO::LineNumber(0));

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
				GPlatesFileIO::ReadErrors::ErrorReadingOgrLayer,
				GPlatesFileIO::ReadErrors::FileNotLoaded));
		return false;
	}
	d_feature_ptr = d_layer_ptr->GetNextFeature();
	if (d_feature_ptr == NULL){
		read_error.d_failures_to_begin.push_back(
			GPlatesFileIO::ReadErrorOccurrence(
				e_source,
				e_location,
				GPlatesFileIO::ReadErrors::NoFeaturesFoundInOgrFile,
				GPlatesFileIO::ReadErrors::FileNotLoaded));
		return false;
	}

	d_layer_ptr->ResetReading();
		
	OGRFeature::DestroyFeature(d_feature_ptr);
	return true;
}

bool
GPlatesFileIO::OgrReader::open_file(
		const QString &filename,
		ReadErrorAccumulation &read_errors)
{
	d_data_source_ptr = GdalUtils::open_vector(filename, false/*update*/, &read_errors);
	if	(d_data_source_ptr == NULL)
	{
		return false;
	}

	d_filename = filename;
	return true;
}



void
GPlatesFileIO::OgrReader::read_features(
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

	static const OgrUtils::feature_map_type &feature_map = OgrUtils::build_feature_map();

	while ((d_feature_ptr = d_layer_ptr->GetNextFeature()) != NULL){
	
		boost::shared_ptr<GPlatesFileIO::LocationInDataSource> e_location(
				new GPlatesFileIO::LineNumber(feature_number));

		d_geometry_ptr = d_feature_ptr->GetGeometryRef();
		if (d_geometry_ptr == NULL){
			read_errors.d_recoverable_errors.push_back(
				GPlatesFileIO::ReadErrorOccurrence(
					e_source,
					e_location,
					GPlatesFileIO::ReadErrors::ErrorReadingOgrGeometry,
					GPlatesFileIO::ReadErrors::FeatureIgnored));
			++feature_number;
			OGRFeature::DestroyFeature(d_feature_ptr);
			continue;
		}

		get_attributes();


		// Check if we have a shapefile attribute corresponding to the Feature Type.
		QMap<QString,QString>::const_iterator it = 
			d_model_to_attribute_map.find(ShapefileAttributes::model_properties[ShapefileAttributes::FEATURE_TYPE]);


		if ((it != d_model_to_attribute_map.constEnd()) && d_field_names.contains(it.value())) {
		
			int index = d_field_names.indexOf(it.value());
	
			// d_field_names should be the same size as d_attributes, but check that we
			// don't try to go beyond the bounds of d_attributes. If somehow we are trying to do 
			// this, then we just get an unclassifiedFeature created.
			if ((index >= 0) && (index < static_cast<int>(d_attributes.size())))
			{

				QString feature_string = d_attributes[index].toString();
				if (GPlatesFileIO::OgrUtils::feature_type_field_is_gpgim_type(d_model_to_attribute_map))
				{
					// We've loosened the GPGIM loading constraints to allow any feature type
					// (even if it's not defined in the GPGIM). So there's no need to check it's in the GPGIM.
					// It still has to be in "<namespace_alias>:<name>" format though (but that's checked below).
					d_feature_type_string = feature_string;
				}
				else
				{
					OgrUtils::feature_map_const_iterator result = feature_map.find(feature_string);
					if (result != feature_map.end()) {
						d_feature_type_string = *result;
					} else {
						read_errors.d_warnings.push_back(GPlatesFileIO::ReadErrorOccurrence(e_source, e_location,
						GPlatesFileIO::ReadErrors::UnrecognisedOgrFeatureType,
						GPlatesFileIO::ReadErrors::UnclassifiedOgrFeatureCreated));
					}
				}
			}
		}



		it = d_model_to_attribute_map.find(
			ShapefileAttributes::model_properties[ShapefileAttributes::FEATURE_ID]);

		if ((it != d_model_to_attribute_map.constEnd()) && d_field_names.contains(it.value())) {

			int index = d_field_names.indexOf(it.value());

			// d_field_names should be the same size as d_attributes, but check that we
			// don't try to go beyond the bounds of d_attributes. If somehow we are trying to do 
			// this, then we just get a feature without a feature_id.
			if ((index >= 0) && (index < static_cast<int>(d_attributes.size())))
			{

				QString feature_id = d_attributes[index].toString();

				// FIXME: should we check here that the provided string is of valid feature-id form,
				// rather than just checking if it's not empty? 
				if (feature_id.isEmpty())
				{
					d_feature_id.reset();
				}
				else
				{
					d_feature_id.reset(GPlatesUtils::make_icu_string_from_qstring(feature_id));
				}
			}
		}		


		boost::optional<GPlatesModel::FeatureType> feature_type =
				GPlatesModel::convert_qstring_to_qualified_xml_name<GPlatesModel::FeatureType>(d_feature_type_string);
		if (!feature_type)
		{
			// For some reason we didn't get a valid feature type. Make an unclassified feature.
			feature_type.reset(GPlatesModel::FeatureType::create_gpml("UnclassifiedFeature"));
			read_errors.d_warnings.push_back(GPlatesFileIO::ReadErrorOccurrence(e_source, e_location,
																				GPlatesFileIO::ReadErrors::UnrecognisedOgrFeatureType,
																				GPlatesFileIO::ReadErrors::UnclassifiedOgrFeatureCreated));
		}
		// Now we have a feature type (in gpml form), even though it may still be the default "UnclassifiedFeature".
		// Get the default geometry property for that feature type, and the possible structural types (e.g. point/multipint etc)
		// for that default geometry property.

		boost::optional<GPlatesModel::GpgimFeatureClass::non_null_ptr_to_const_type> feature_class =
				GPlatesModel::Gpgim::instance().get_feature_class(*feature_type);
		boost::optional<GPlatesModel::GpgimProperty::non_null_ptr_to_const_type> default_geometry_feature_property;
		GPlatesModel::GpgimProperty::structural_type_seq_type default_structural_types;

		if (feature_class)
		{
			 default_geometry_feature_property = (*feature_class)->get_default_geometry_feature_property();
		}
		else
		{
			// TODO: We didn't get a valid feature class. What can we do here?
			// I guess we have to bail out and flag up the issue with read-errors, and skip to the next feature.
			read_errors.d_warnings.push_back(
						GPlatesFileIO::ReadErrorOccurrence(
							e_source,
							e_location,
							GPlatesFileIO::ReadErrors::UnrecognisedOgrFeatureType,
							GPlatesFileIO::ReadErrors::FeatureIgnored));
			++feature_number;
			OGRFeature::DestroyFeature(d_feature_ptr);
			continue;
		}

		d_type = d_geometry_ptr->getGeometryType();
		OGRwkbGeometryType flattened_type = wkb_flatten(d_type);

		if( d_type != flattened_type){
			read_errors.d_warnings.push_back(
						GPlatesFileIO::ReadErrorOccurrence(
							e_source,
							e_location,
							GPlatesFileIO::ReadErrors::TwoPointFiveDGeometryDetected,
							GPlatesFileIO::ReadErrors::GeometryFlattenedTo2D));
		}




		if (default_geometry_feature_property)
		{
			default_structural_types = (*default_geometry_feature_property)->get_structural_types();
		}

		// If we don't have a default, the default_structural_types container will be empty.
		if (OgrUtils::wkb_type_belongs_to_structural_types(flattened_type,default_structural_types))
		{
			// We need to send the raw ogr type here so that we can determine if we need to handle multipolyines, multipolygons and the like.
			handle_geometry(*feature_type,flattened_type,default_geometry_feature_property,collection,read_errors,e_source,e_location);
		}
		else
		{
			// We should get here either if:
			//		we didn't have a default property, or
			//		the structural type from OGR didn't match the possible structural types of the default property.
			//
			// So in this case we want to try any remaining properties and see if we get a match between property
			// structural type and OGR structural type.
			boost::optional<GPlatesPropertyValues::StructuralType> structural_type_of_ogr_geom =
					OgrUtils::get_structural_type_of_wkb_type(flattened_type);

			bool found_matching_property = false;
			if (structural_type_of_ogr_geom)
			{
				GPlatesModel::GpgimFeatureClass::gpgim_property_seq_type properties;
				(*feature_class)->get_feature_properties(properties);


				BOOST_FOREACH(GPlatesModel::GpgimProperty::non_null_ptr_to_const_type property, properties)
				{
					if (property->get_structural_type(*structural_type_of_ogr_geom))
					{
						found_matching_property = true;
						boost::optional<GPlatesModel::GpgimProperty::non_null_ptr_to_const_type> optional_property;
						optional_property.reset(property);
						handle_geometry(*feature_type,flattened_type,optional_property,collection,read_errors,e_source,e_location);
						break;
					}
				}
			}
			if (!found_matching_property)
			{
				// We can't match the OGR geometry with the feature's required geometry.
				read_errors.d_warnings.push_back(
							GPlatesFileIO::ReadErrorOccurrence(
								e_source,
								e_location,
								GPlatesFileIO::ReadErrors::UnableToMatchOgrGeometryWithFeature,
								GPlatesFileIO::ReadErrors::FeatureIgnored));
			}
		}

		OGRFeature::DestroyFeature(d_feature_ptr);
		feature_number++;
	} // while

}

const GPlatesModel::FeatureHandle::weak_ref
GPlatesFileIO::OgrReader::create_polygon_feature_from_list(
	const GPlatesModel::FeatureType &feature_type,
	const GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
	const std::vector<GPlatesMaths::PointOnSphere> &exterior_ring,
	const std::list< std::vector<GPlatesMaths::PointOnSphere> > &interior_rings,
	const boost::optional<GPlatesModel::GpgimProperty::non_null_ptr_to_const_type> &property)
{
	GPlatesModel::FeatureHandle::weak_ref feature = create_feature(feature_type,collection,d_feature_type_string,d_feature_id);

	GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere =
		GPlatesMaths::PolygonOnSphere::create_on_heap(exterior_ring, interior_rings);

	GPlatesPropertyValues::GmlPolygon::non_null_ptr_type gml_polygon =
		GPlatesPropertyValues::GmlPolygon::create(polygon_on_sphere);

	GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type property_value =
		GPlatesModel::ModelUtils::create_gpml_constant_value(gml_polygon);

	boost::optional<GPlatesModel::PropertyName> property_name;
	if (property)
	{
		property_name = (*property)->get_property_name();
	}
	else
	{
		property_name.reset(GPlatesModel::PropertyName::create_gpml("unclassifiedGeometry"));
	}

	feature->add(
			GPlatesModel::TopLevelPropertyInline::create(
				*property_name,
				property_value));

	return feature;

}

const GPlatesModel::FeatureHandle::weak_ref
GPlatesFileIO::OgrReader::create_line_feature_from_list(
	const GPlatesModel::FeatureType &feature_type,
	const GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
	const std::vector<GPlatesMaths::PointOnSphere> &list_of_points,
	const boost::optional<GPlatesModel::GpgimProperty::non_null_ptr_to_const_type> &property)
{
	GPlatesModel::FeatureHandle::weak_ref feature = create_feature(feature_type,collection,d_feature_type_string,d_feature_id);

	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline =
		GPlatesMaths::PolylineOnSphere::create_on_heap(list_of_points);

	GPlatesPropertyValues::GmlLineString::non_null_ptr_type gml_line_string =
		GPlatesPropertyValues::GmlLineString::create(polyline);

	GPlatesPropertyValues::GmlOrientableCurve::non_null_ptr_type gml_orientable_curve =
		GPlatesModel::ModelUtils::create_gml_orientable_curve(gml_line_string);
			
	GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type property_value =
			GPlatesModel::ModelUtils::create_gpml_constant_value(gml_orientable_curve);

	boost::optional<GPlatesModel::PropertyName> property_name;
	if (property)
	{
		property_name = (*property)->get_property_name();
	}
	else
	{
		property_name.reset(GPlatesModel::PropertyName::create_gpml("unclassifiedGeometry"));
	}

	feature->add(
			GPlatesModel::TopLevelPropertyInline::create(
				*property_name,
				property_value));

	return feature;
}


const GPlatesModel::FeatureHandle::weak_ref
GPlatesFileIO::OgrReader::create_point_feature_from_point_on_sphere(
	const GPlatesModel::FeatureType &feature_type,
	const GPlatesModel::FeatureCollectionHandle::weak_ref &collection, 
	const GPlatesMaths::PointOnSphere &point,
	const boost::optional<GPlatesModel::GpgimProperty::non_null_ptr_to_const_type> &property)
{

	GPlatesModel::FeatureHandle::weak_ref feature = create_feature(feature_type,collection,d_feature_type_string,d_feature_id);

	const GPlatesModel::PropertyValue::non_null_ptr_type gml_point =
		GPlatesPropertyValues::GmlPoint::create(point);

	GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type property_value =
		GPlatesModel::ModelUtils::create_gpml_constant_value(gml_point);

	boost::optional<GPlatesModel::PropertyName> property_name;
	if (property)
	{
		property_name = (*property)->get_property_name();
	}
	else
	{
		property_name.reset(GPlatesModel::PropertyName::create_gpml("unclassifiedGeometry"));
	}
	feature->add(
			GPlatesModel::TopLevelPropertyInline::create(
				*property_name,
				property_value));

	return feature;

}

const GPlatesModel::FeatureHandle::weak_ref
GPlatesFileIO::OgrReader::create_multi_point_feature_from_list(
	const GPlatesModel::FeatureType &feature_type,
	const GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
	const std::vector<GPlatesMaths::PointOnSphere> &list_of_points,
	const boost::optional<GPlatesModel::GpgimProperty::non_null_ptr_to_const_type> &property)
{

	GPlatesModel::FeatureHandle::weak_ref feature = create_feature(feature_type,collection,d_feature_type_string,d_feature_id);

	GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere =
		GPlatesMaths::MultiPointOnSphere::create_on_heap(list_of_points);

	GPlatesPropertyValues::GmlMultiPoint::non_null_ptr_type gml_multi_point =
		GPlatesPropertyValues::GmlMultiPoint::create(multi_point_on_sphere);

	GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type property_value =
		GPlatesModel::ModelUtils::create_gpml_constant_value(gml_multi_point);

	boost::optional<GPlatesModel::PropertyName> property_name;
	if (property)
	{
		property_name = (*property)->get_property_name();
	}
	else
	{
		property_name.reset(GPlatesModel::PropertyName::create_gpml("unclassifiedGeometry"));
	}

	feature->add(
			GPlatesModel::TopLevelPropertyInline::create(
				*property_name,
				property_value));

	return feature;
}

void 
GPlatesFileIO::OgrReader::get_field_names(
	ReadErrorAccumulation &read_errors)
{
	boost::shared_ptr<GPlatesFileIO::DataSource> e_source(
		new GPlatesFileIO::LocalFileDataSource(d_filename, GPlatesFileIO::DataFormats::Shapefile));

	boost::shared_ptr<GPlatesFileIO::LocationInDataSource> e_location(
		new GPlatesFileIO::LineNumber(0));
				
	d_field_names.clear();
	if (!d_feature_ptr){
		return;
	}
	OGRFeatureDefn *feature_def_ptr = d_layer_ptr->GetLayerDefn();
	int num_fields = feature_def_ptr->GetFieldCount();
	int count;

	for ( count=0; count< num_fields; count++){
		OGRFieldDefn *field_def_ptr = feature_def_ptr->GetFieldDefn(count);
		d_field_names.push_back(QString::fromAscii(field_def_ptr->GetNameRef()));
	}

}

/**
 * @brief GPlatesFileIO::OgrReader::get_attributes
 * Fills the member variable d_attributes with QVariant forms of the imported file's attributes.
 * Note that OgrReader was written initially to support ESRI shapefiles. While shapefiles
 * can store a variety of field types in the dbf file
 * (see for example http://www.dbase.com/Knowledgebase/INT/db7_file_fmt.htm)
 * the OGR driver supports only Integer, Real, String and Date.
 * (see http://www.gdal.org/ogr/drv_shapefile.html )
 *
 * TODO: Since we are now attempting to support other OGR-supported formats, we may need/want to
 * extend the field types recognised here and store them in the model appropriately.
 *
 * Docs for GMT5 for example (http://gmt.soest.hawaii.edu/5/GMT_Docs.pdf) state that
 *
 * "Available datatypes should largely follow the shapefile (DB3) specification, including string, integer,
 *	double, datetime, and logical (boolean)."
 *
 */
void 
GPlatesFileIO::OgrReader::get_attributes()
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
		QVariant value_variant;
		if (field_def_ptr->GetType()==OFTInteger){
			value_variant =  d_feature_ptr->IsFieldSet(count) ?
							QVariant(d_feature_ptr->GetFieldAsInteger(count)) :
							QVariant(QVariant::Int);
		}
		else if (field_def_ptr->GetType()==OFTReal){
			value_variant =  d_feature_ptr->IsFieldSet(count) ?
							QVariant(d_feature_ptr->GetFieldAsDouble(count)) :
							QVariant(QVariant::Double);
		}
		else if (field_def_ptr->GetType()==OFTDate)
		{
			// Store this as a string. It's possible to extract the various year/month/day
			// fields separately if it becomes necessary.
			value_variant =  d_feature_ptr->IsFieldSet(count) ?
							QVariant(d_feature_ptr->GetFieldAsString(count)) :
							QVariant(QVariant::String);
		}
		else
		{ // If string or other type.
			value_variant =  d_feature_ptr->IsFieldSet(count) ?
							QVariant(d_feature_ptr->GetFieldAsString(count)) :
							QVariant(QVariant::String);
		}

		d_attributes.push_back(value_variant);
	}
}

void
GPlatesFileIO::OgrReader::add_attributes_to_feature(
	const GPlatesModel::FeatureHandle::weak_ref &feature,
	GPlatesFileIO::ReadErrorAccumulation &read_errors,
	const boost::shared_ptr<GPlatesFileIO::DataSource> &source,
	const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> &location)
{
	int n = static_cast<int>(d_attributes.size());
	
	// Can there be zero attributes? I dunno. 
	if (n == 0) return;

	// The key-value dictionary elements.
	std::vector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement::non_null_ptr_type> dictionary_elements;

	// If for any reason we've found more attributes than we have field names, only 
	// go as far as the number of field names. 
	if (n > d_field_names.size())
	{
		n = d_field_names.size();
	}

	for (int count = 0; count < n ; count++){
		QString fieldname = d_field_names[count];
		QVariant attribute = d_attributes[count];

		QVariant::Type type_ = attribute.type();

		// Make an XsString property for the attribute field name. 
		GPlatesPropertyValues::XsString::non_null_ptr_type key = 
			GPlatesPropertyValues::XsString::create(
				GPlatesUtils::make_icu_string_from_qstring(fieldname));

		bool ok;
		// Add the attribute to the dictionary.
		switch(type_){
			case QVariant::Int:
			{
				int i = attribute.toInt(&ok);
				if (ok)
				{
					GPlatesPropertyValues::XsInteger::non_null_ptr_type value = 
						GPlatesPropertyValues::XsInteger::create(i);
					GPlatesPropertyValues::GpmlKeyValueDictionaryElement::non_null_ptr_type element =
							GPlatesPropertyValues::GpmlKeyValueDictionaryElement::create(
									key,
									value,
									GPlatesPropertyValues::StructuralType::create_xsi("integer"));
					dictionary_elements.push_back(element);
				}
			}
			break;
			case QVariant::Double:
			{
				double d = attribute.toDouble(&ok);
				if (ok)
				{
					GPlatesPropertyValues::XsDouble::non_null_ptr_type value = 
						GPlatesPropertyValues::XsDouble::create(d);
					GPlatesPropertyValues::GpmlKeyValueDictionaryElement::non_null_ptr_type element =
							GPlatesPropertyValues::GpmlKeyValueDictionaryElement::create(
									key,
									value,
									GPlatesPropertyValues::StructuralType::create_xsi("double"));
					dictionary_elements.push_back(element);
				}
			}
			break;
			case QVariant::String:
			default:
				GPlatesPropertyValues::XsString::non_null_ptr_type value = 
					GPlatesPropertyValues::XsString::create(
							GPlatesUtils::make_icu_string_from_qstring(attribute.toString()));
				GPlatesPropertyValues::GpmlKeyValueDictionaryElement::non_null_ptr_type element =
						GPlatesPropertyValues::GpmlKeyValueDictionaryElement::create(
								key,
								value,
								GPlatesPropertyValues::StructuralType::create_xsi("string"));
				dictionary_elements.push_back(element);
				break;
		}

	} // loop over number of attributes

	// Create a key-value dictionary.
	GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type dictionary = 
		GPlatesPropertyValues::GpmlKeyValueDictionary::create(dictionary_elements);

	// Add the dictionary to the feature.
	feature->add(
			GPlatesModel::TopLevelPropertyInline::create(
				GPlatesModel::PropertyName::create_gpml("shapefileAttributes"),
				dictionary));

	// Map the shapefile attributes to model properties.
	map_attributes_to_properties(feature,d_model_to_attribute_map,read_errors,source,location);

}


bool
GPlatesFileIO::OgrReader::transform_and_check_coords(
		double &x,
		double &y,
		ReadErrorAccumulation &read_errors,
		const boost::shared_ptr<GPlatesFileIO::DataSource> &source,
		const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> &location)
{

	if (!d_current_coordinate_transformation->transform_in_place(&x,&y))
	{
		qWarning() << "Failed to transform coordinates";
		return false;
	}

	if (x < SHAPE_NO_DATA){
		read_errors.d_recoverable_errors.push_back(
					GPlatesFileIO::ReadErrorOccurrence(
						source,
						location,
						GPlatesFileIO::ReadErrors::NoLongitudeShapeData,
						GPlatesFileIO::ReadErrors::GeometryIgnored));
		return false;
	}
	if (y < SHAPE_NO_DATA){
		read_errors.d_recoverable_errors.push_back(
					GPlatesFileIO::ReadErrorOccurrence(
						source,
						location,
						GPlatesFileIO::ReadErrors::NoLatitudeShapeData,
						GPlatesFileIO::ReadErrors::GeometryIgnored));
		return false;
	}

	if (!GPlatesMaths::LatLonPoint::is_valid_latitude(y)){
		read_errors.d_recoverable_errors.push_back(
					GPlatesFileIO::ReadErrorOccurrence(
						source,
						location,
						GPlatesFileIO::ReadErrors::InvalidOgrLatitude,
						GPlatesFileIO::ReadErrors::GeometryIgnored));
		// Increase precision to make sure numbers like 90.00000190700007 (an actual value in a Shapefile)
		// don't get printed as 90.0.
		qDebug() << "Invalid latitude: " << qSetRealNumberPrecision(16) << y;
		return false;
	}

	if (!GPlatesMaths::LatLonPoint::is_valid_longitude(x)){
		read_errors.d_recoverable_errors.push_back(
					GPlatesFileIO::ReadErrorOccurrence(
						source,
						location,
						GPlatesFileIO::ReadErrors::InvalidOgrLongitude,
						GPlatesFileIO::ReadErrors::GeometryIgnored));
		// Increase precision to make sure numbers very slightly less/greater than -360.0/360.0
		// don't get printed -360.0/360.0.
		qDebug() << "Invalid longitude: " << qSetRealNumberPrecision(16) << x;
		return false;
	}

	return true;
}


void
GPlatesFileIO::OgrReader::read_file(
		GPlatesFileIO::File::Reference &file_ref,
		const FeatureCollectionFileFormat::OGRConfiguration::shared_ptr_to_const_type &default_file_configuration,
		ReadErrorAccumulation &read_errors,
		bool &contains_unsaved_changes)
{
	PROFILE_FUNC();

	contains_unsaved_changes = false;

	const FileInfo &fileinfo = file_ref.get_file_info();

	QString absolute_path_filename = fileinfo.get_qfileinfo().absoluteFilePath();
	QString filename = fileinfo.get_qfileinfo().fileName();

	OgrReader reader;
	if (!reader.open_file(absolute_path_filename, read_errors))
	{
		throw ErrorOpeningFileForReadingException(GPLATES_EXCEPTION_SOURCE, filename);
	}

	if (!reader.check_file_format(read_errors)){
		throw ErrorOpeningFileForReadingException(GPLATES_EXCEPTION_SOURCE, filename);
	}

	reader.read_srs_and_set_transformation(file_ref,default_file_configuration);

	reader.get_field_names(read_errors);

	QString shapefile_xml_filename = OgrUtils::make_ogr_xml_filename(fileinfo.get_qfileinfo());

	reader.d_model_to_attribute_map.clear();

	if (!fill_attribute_map_from_xml_file(shapefile_xml_filename,reader.d_model_to_attribute_map))
	{
		// Set the last argument to false, because this is an initial mapping, not a re-mapping. 
		if (!fill_attribute_map_from_dialog(
				filename,
				reader.d_field_names,
				reader.d_model_to_attribute_map,
				s_property_mapper,
				false))
		{
			// The user has cancelled the mapper-dialog routine, so cancel the whole ogr loading procedure.
			throw FileLoadAbortedException(GPLATES_EXCEPTION_SOURCE, "File load aborted.",filename);
		}
		OgrUtils::save_attribute_map_as_xml_file(shapefile_xml_filename,reader.d_model_to_attribute_map);
	}

	// Store the model-to-attribute map so we can access it if the feature collection gets written back out.
	store_model_to_attribute_map_in_file_reference(reader.d_model_to_attribute_map, file_ref);

	GPlatesModel::FeatureCollectionHandle::weak_ref collection = file_ref.get_feature_collection();

	reader.read_features(collection,read_errors);


	//reader.display_feature_counts();
}


void
GPlatesFileIO::OgrReader::set_property_mapper(
	boost::shared_ptr< PropertyMapper > property_mapper)
{
	s_property_mapper = property_mapper;
}

void
GPlatesFileIO::OgrReader::handle_geometry(
		const GPlatesModel::FeatureType &feature_type,
		const OGRwkbGeometryType &type,
		const boost::optional<GPlatesModel::GpgimProperty::non_null_ptr_to_const_type> &property,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
		ReadErrorAccumulation &read_errors,
		const boost::shared_ptr<GPlatesFileIO::DataSource> &e_source,
		const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> &e_location)
{
	switch (type){
	case wkbPoint:
		handle_point(feature_type,property,collection,read_errors,e_source,e_location);
		break;
	case wkbMultiPoint:
		handle_multi_point(feature_type,property,collection,read_errors,e_source,e_location);
		break;
	case wkbLineString:
		handle_linestring(feature_type,property,collection,read_errors,e_source,e_location);
		break;
	case wkbMultiLineString:
		handle_multi_linestring(feature_type,property,collection,read_errors,e_source,e_location);
		break;
	case wkbPolygon:
		handle_polygon(feature_type,property,collection,read_errors,e_source,e_location);
		break;
	case wkbMultiPolygon:
		handle_multi_polygon(feature_type,property,collection,read_errors,e_source,e_location);
		break;
	default:
		read_errors.d_recoverable_errors.push_back(
					GPlatesFileIO::ReadErrorOccurrence(
						e_source,
						e_location,
						GPlatesFileIO::ReadErrors::UnsupportedGeometryType,
						GPlatesFileIO::ReadErrors::GeometryIgnored));
	} // switch
}

void
GPlatesFileIO::OgrReader::handle_point(
		const GPlatesModel::FeatureType &feature_type,
		const boost::optional<GPlatesModel::GpgimProperty::non_null_ptr_to_const_type> &property,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
		ReadErrorAccumulation &read_errors,
		const boost::shared_ptr<GPlatesFileIO::DataSource> &source,
		const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> &location)
{
	OGRPoint *ogr_point = static_cast<OGRPoint*>(d_geometry_ptr);
	double x = ogr_point->getX();
	double y = ogr_point->getY();
	if (transform_and_check_coords(x,y,read_errors,source,location)){
		GPlatesMaths::LatLonPoint llp(y,x);
				GPlatesMaths::PointOnSphere point = GPlatesMaths::make_point_on_sphere(llp);
				try {
					GPlatesModel::FeatureHandle::weak_ref feature =
							create_point_feature_from_point_on_sphere(feature_type,collection,point,property);
					add_attributes_to_feature(feature,read_errors,source,location);
					d_loaded_geometries++;
				}
				catch (std::exception &exc)
				{
					qWarning() << exc.what();

					read_errors.d_recoverable_errors.push_back(
						GPlatesFileIO::ReadErrorOccurrence(
						source,
						location,
						GPlatesFileIO::ReadErrors::InvalidOgrPoint,
						GPlatesFileIO::ReadErrors::GeometryIgnored));
				}
				catch (...)
				{
					qWarning() << "GPlatesFileIO::OgrReader::handle_point: Unknown error";

					read_errors.d_recoverable_errors.push_back(
						GPlatesFileIO::ReadErrorOccurrence(
						source,
						location,
						GPlatesFileIO::ReadErrors::InvalidOgrPoint,
						GPlatesFileIO::ReadErrors::GeometryIgnored));
				}
			}
			d_total_geometries++;
}

void
GPlatesFileIO::OgrReader::handle_multi_point(
			const GPlatesModel::FeatureType &feature_type,
			const boost::optional<GPlatesModel::GpgimProperty::non_null_ptr_to_const_type> &property,
			const GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			ReadErrorAccumulation &read_errors,
			const boost::shared_ptr<GPlatesFileIO::DataSource> &source,
			const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> &location)
{
		OGRMultiPoint *multi = static_cast<OGRMultiPoint*>(d_geometry_ptr);
		int num_geometries = multi->getNumGeometries();

		if (num_geometries == 0)
		{
			read_errors.d_recoverable_errors.push_back(
				GPlatesFileIO::ReadErrorOccurrence(
					source,
					location,
					GPlatesFileIO::ReadErrors::NoGeometriesFoundInMultiGeometry,
					GPlatesFileIO::ReadErrors::FeatureIgnored));
			return;
		}

		d_total_geometries += num_geometries;

		std::vector<GPlatesMaths::PointOnSphere> list_of_points;
		list_of_points.reserve(num_geometries);

		for(int count = 0; count < num_geometries; count++)
		{
			OGRPoint* ogr_point = static_cast<OGRPoint*>(multi->getGeometryRef(count));
			double x = ogr_point->getX();
			double y = ogr_point->getY();

			if (transform_and_check_coords(x,y,read_errors,source,location)){
				GPlatesMaths::LatLonPoint llp(y,x);
				GPlatesMaths::PointOnSphere p = GPlatesMaths::make_point_on_sphere(llp);
				list_of_points.push_back(p);

			}
		} // loop over geometries

		if (!list_of_points.empty())
		{
			try {
				GPlatesModel::FeatureHandle::weak_ref feature = create_multi_point_feature_from_list(feature_type,collection,list_of_points,property);
				add_attributes_to_feature(feature,read_errors,source,location);
				d_loaded_geometries++;
			}
			catch (std::exception &exc)
			{
				qWarning() << "GPlatesFileIO::OgrReader::handle_multi_point: " << exc.what();
				read_errors.d_recoverable_errors.push_back(
					GPlatesFileIO::ReadErrorOccurrence(
						source,
						location,
						GPlatesFileIO::ReadErrors::InvalidOgrMultiPoint,
						GPlatesFileIO::ReadErrors::GeometryIgnored));
			}
			catch(...)
			{
				qWarning() << "GPlatesFileIO::OgrReader::handle_multi_point: Unknown error";
				read_errors.d_recoverable_errors.push_back(
					GPlatesFileIO::ReadErrorOccurrence(
						source,
						location,
						GPlatesFileIO::ReadErrors::InvalidOgrMultiPoint,
						GPlatesFileIO::ReadErrors::GeometryIgnored));
			}
		}
}

void
GPlatesFileIO::OgrReader::handle_linestring(
			const GPlatesModel::FeatureType &feature_type,
			const boost::optional<GPlatesModel::GpgimProperty::non_null_ptr_to_const_type> &property,
			const GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			ReadErrorAccumulation &read_errors,
			const boost::shared_ptr<GPlatesFileIO::DataSource> &source,
			const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> &location
			)
{
	std::vector<GPlatesMaths::PointOnSphere> feature_points;
	OGRLineString *linestring = static_cast<OGRLineString*>(d_geometry_ptr);
	int num_points = linestring->getNumPoints();
	feature_points.reserve(num_points);
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
	double x,y;
	for (count = 0; count < num_points; count ++){
		x = linestring->getX(count);
		y = linestring->getY(count);
		if (transform_and_check_coords(x,y,read_errors,source,location)){
			GPlatesMaths::LatLonPoint llp(y,x);
			feature_points.push_back(GPlatesMaths::make_point_on_sphere(llp));
		}
		else{
			return;
		}

	}

	try {
		GPlatesModel::FeatureHandle::weak_ref feature = create_line_feature_from_list(feature_type,collection,feature_points,property);
		add_attributes_to_feature(feature,read_errors,source,location);
		d_loaded_geometries++;
	}
	catch (std::exception &exc)
	{
		qWarning() << "GPlatesFileIO::OgrReader::handle_linestring: " << exc.what();

		read_errors.d_recoverable_errors.push_back(
			GPlatesFileIO::ReadErrorOccurrence(
			source,
			location,
			GPlatesFileIO::ReadErrors::InvalidOgrPolyline,
			GPlatesFileIO::ReadErrors::GeometryIgnored));
	}
	catch (...)
	{
		qWarning() << "GPlatesFileIO::OgrReader::handle_linestring: Unknown error";

		read_errors.d_recoverable_errors.push_back(
			GPlatesFileIO::ReadErrorOccurrence(
			source,
			location,
			GPlatesFileIO::ReadErrors::InvalidOgrPolyline,
			GPlatesFileIO::ReadErrors::GeometryIgnored));
	}

}

void
GPlatesFileIO::OgrReader::handle_multi_linestring(
			const GPlatesModel::FeatureType &feature_type,
			const boost::optional<GPlatesModel::GpgimProperty::non_null_ptr_to_const_type> &property,
			const GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			ReadErrorAccumulation &read_errors,
			const boost::shared_ptr<GPlatesFileIO::DataSource> &source,
			const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> &location)
{

	OGRMultiLineString *multi = static_cast<OGRMultiLineString*>(d_geometry_ptr);
	int num_geometries = multi->getNumGeometries();
	if (num_geometries == 0)
	{
		read_errors.d_recoverable_errors.push_back(
			GPlatesFileIO::ReadErrorOccurrence(
				source,
				location,
				GPlatesFileIO::ReadErrors::NoGeometriesFoundInMultiGeometry,
				GPlatesFileIO::ReadErrors::FeatureIgnored));
		return;
	}
	d_total_geometries += num_geometries;

#if 0
	qDebug() << "handle_multi_linestring";
	qDebug() << "num geometries: " << num_geometries;
#endif
		
	GPlatesModel::FeatureHandle::weak_ref feature = create_feature(feature_type,collection,d_feature_type_string,d_feature_id);
	add_attributes_to_feature(feature,read_errors,source,location);	

	for (int multiCount = 0; multiCount < num_geometries ; multiCount++){

		std::vector<GPlatesMaths::PointOnSphere> feature_points;
		OGRLineString *linestring = static_cast<OGRLineString*>(multi->getGeometryRef(multiCount));

		int num_points = linestring->getNumPoints();
		feature_points.reserve(num_points);
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
		double x,y;

		for (count = 0; count < num_points; count++){
			x = linestring->getX(count);
			y = linestring->getY(count);
			if (transform_and_check_coords(x,y,read_errors,source,location)){
				GPlatesMaths::LatLonPoint llp(y,x);
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
				add_polyline_geometry_to_feature(feature,feature_points,property);
				d_loaded_geometries++;
			}
			catch (std::exception &exc)
			{
				qWarning() << "GPlatesFileIO::OgrReader::handle_multi_linestring: " << exc.what();

				read_errors.d_recoverable_errors.push_back(
					GPlatesFileIO::ReadErrorOccurrence(
					source,
					location,
					GPlatesFileIO::ReadErrors::InvalidOgrPolyline,
					GPlatesFileIO::ReadErrors::GeometryIgnored));
			}
			catch (...)
			{
				qWarning() << "GPlatesFileIO::OgrReader::handle_multi_linestring: Unknown error";

				read_errors.d_recoverable_errors.push_back(
					GPlatesFileIO::ReadErrorOccurrence(
					source,
					location,
					GPlatesFileIO::ReadErrors::InvalidOgrPolyline,
					GPlatesFileIO::ReadErrors::GeometryIgnored));
			}
		}
	} // for loop over multilines
}


void
GPlatesFileIO::OgrReader::handle_polygon(
			const GPlatesModel::FeatureType &feature_type,
			const boost::optional<GPlatesModel::GpgimProperty::non_null_ptr_to_const_type> &property,
			const GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			ReadErrorAccumulation &read_errors,
			const boost::shared_ptr<GPlatesFileIO::DataSource> &source,
			const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> &location)
{
	OGRPolygon *polygon = static_cast<OGRPolygon*>(d_geometry_ptr);
	d_total_geometries++;

	// Read the exterior ring points.
	std::vector<GPlatesMaths::PointOnSphere> exterior_ring_points;
	OGRLinearRing *exterior_ring = polygon->getExteriorRing();
	add_ring_to_points_list(exterior_ring, exterior_ring_points, read_errors, source, location);

	// If there are no points in the exterior ring then we don't create a polygon feature.
	if (exterior_ring_points.empty())
	{
		return;
	}

	std::list< std::vector<GPlatesMaths::PointOnSphere> > interior_rings;

	// Read the points in the interior rings.
	int num_interior_rings = polygon->getNumInteriorRings();
	for (int ring_count = 0; ring_count < num_interior_rings; ring_count++)
	{
		//std::cerr << "Interior ring: " << ring_count << std::endl;

		std::vector<GPlatesMaths::PointOnSphere> interior_ring_points;
		OGRLinearRing *interior_ring = polygon->getInteriorRing(ring_count);
		add_ring_to_points_list(interior_ring, interior_ring_points, read_errors, source, location);

		// Only add interior ring if it contains points.
		if (!interior_ring_points.empty())
		{
			interior_rings.push_back(interior_ring_points);
		}

	} // loop over interior rings


	try
	{
		GPlatesModel::FeatureHandle::weak_ref feature = create_polygon_feature_from_list(
				feature_type, collection, exterior_ring_points, interior_rings, property);
		add_attributes_to_feature(feature, read_errors, source, location);
		d_loaded_geometries++;
	}
	catch (std::exception &exc)
	{
		qWarning() << "GPlatesFileIO::OgrReader::handle_polygon: " << exc.what();

		read_errors.d_recoverable_errors.push_back(
			GPlatesFileIO::ReadErrorOccurrence(
			source,
			location,
			GPlatesFileIO::ReadErrors::InvalidOgrPolygon,
			GPlatesFileIO::ReadErrors::GeometryIgnored));
	}
	catch (...)
	{
		qWarning() << "GPlatesFileIO::OgrReader::handle_polygon: Unknown error";

		read_errors.d_recoverable_errors.push_back(
			GPlatesFileIO::ReadErrorOccurrence(
			source,
			location,
			GPlatesFileIO::ReadErrors::InvalidOgrPolygon,
			GPlatesFileIO::ReadErrors::GeometryIgnored));
	}
}

void
GPlatesFileIO::OgrReader::handle_multi_polygon(
			const GPlatesModel::FeatureType &feature_type,
			const boost::optional<GPlatesModel::GpgimProperty::non_null_ptr_to_const_type> &property,
			const GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			ReadErrorAccumulation &read_errors,
			const boost::shared_ptr<GPlatesFileIO::DataSource> &source,
			const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> &location)
{
	//std::cerr << "Multi-polygon" << std::endl;
	OGRMultiPolygon *multi = static_cast<OGRMultiPolygon*>(d_geometry_ptr);
	int num_geometries = multi->getNumGeometries();
	if (num_geometries == 0)
	{
		read_errors.d_recoverable_errors.push_back(
			GPlatesFileIO::ReadErrorOccurrence(
				source,
				location,
				GPlatesFileIO::ReadErrors::NoGeometriesFoundInMultiGeometry,
				GPlatesFileIO::ReadErrors::FeatureIgnored));
		return;
	}

	GPlatesModel::FeatureHandle::weak_ref feature = create_feature(feature_type,collection,d_feature_type_string,d_feature_id);
	add_attributes_to_feature(feature,read_errors,source,location);	

	d_total_geometries += num_geometries;

	for (int multiCount = 0; multiCount < num_geometries ; multiCount++)
	{
		//std::cerr << "Polygon number: " << multiCount << std::endl;
		OGRPolygon *polygon = static_cast<OGRPolygon*>(multi->getGeometryRef(multiCount));

		// Read the exterior ring points.
		std::vector<GPlatesMaths::PointOnSphere> exterior_ring_points;
		OGRLinearRing *exterior_ring = polygon->getExteriorRing();
		add_ring_to_points_list(exterior_ring, exterior_ring_points, read_errors, source, location);

		// If there are no points in the exterior ring then we don't add a polygon geometry.
		if (exterior_ring_points.empty())
		{
			continue;
		}

		std::list< std::vector<GPlatesMaths::PointOnSphere> > interior_rings;

		// Read the points in the interior rings.
		int num_interior_rings = polygon->getNumInteriorRings();
		for (int ring_count = 0; ring_count < num_interior_rings; ring_count++)
		{
			//std::cerr << "Interior ring: " << ring_count << std::endl;

			std::vector<GPlatesMaths::PointOnSphere> interior_ring_points;
			OGRLinearRing *interior_ring = polygon->getInteriorRing(ring_count);
			add_ring_to_points_list(interior_ring, interior_ring_points, read_errors, source, location);

			// Only add interior ring if it contains points.
			if (!interior_ring_points.empty())
			{
				interior_rings.push_back(interior_ring_points);
			}

		} // loop over interior rings

		try
		{	
			add_polygon_geometry_to_feature(feature, exterior_ring_points, interior_rings, property);
			d_loaded_geometries++;
		}
		catch (std::exception &exc)
		{
			qWarning() << "GPlatesFileIO::OgrReader::handle_multi_polygon: " << exc.what();

			read_errors.d_recoverable_errors.push_back(
				GPlatesFileIO::ReadErrorOccurrence(
				source,
				location,
				GPlatesFileIO::ReadErrors::InvalidOgrPolygon,
				GPlatesFileIO::ReadErrors::GeometryIgnored));
		}
		catch (...)
		{
			qWarning() << "GPlatesFileIO::OgrReader::handle_multi_polygon: Unknown error";

			read_errors.d_recoverable_errors.push_back(
				GPlatesFileIO::ReadErrorOccurrence(
				source,
				location,
				GPlatesFileIO::ReadErrors::InvalidOgrPolygon,
				GPlatesFileIO::ReadErrors::GeometryIgnored));
		}

	} // loop over multipolygons
}

void
GPlatesFileIO::OgrReader::display_feature_counts()
{

	std::cerr << "feature/geometry count: " <<
			d_total_features << ", " <<
			d_loaded_geometries << ", " <<
			d_total_geometries << std::endl;

}

void
GPlatesFileIO::OgrReader::read_srs_and_set_transformation(
		GPlatesFileIO::File::Reference &file_ref,
		const GPlatesFileIO::FeatureCollectionFileFormat::OGRConfiguration::shared_ptr_to_const_type
		&default_ogr_file_configuration)
{
	// d_current_coordinate_transform is initialised to an identity transformation in the initialiser list, and
	// is only overwritten here if we find an SRS which can be transformed to WGS84.

	if (!d_layer_ptr)
	{
		return;
	}

	const OGRSpatialReference *ogr_srs = d_layer_ptr->GetSpatialRef();
	if (ogr_srs)
	{
		d_source_srs = GPlatesPropertyValues::SpatialReferenceSystem::create(*ogr_srs);

		// Transformation from provided srs (source) to WGS84 (target, default)
		boost::optional<GPlatesPropertyValues::CoordinateTransformation::non_null_ptr_type> transform =
				GPlatesPropertyValues::CoordinateTransformation::create(*d_source_srs);

		if (transform)
		{
			d_current_coordinate_transformation = transform.get();
		}

		boost::optional<FeatureCollectionFileFormat::OGRConfiguration::shared_ptr_type> ogr_file_configuration =
				FeatureCollectionFileFormat::OGRConfiguration::shared_ptr_type(
					new FeatureCollectionFileFormat::OGRConfiguration(
						*default_ogr_file_configuration));

		if (ogr_file_configuration)
		{
			ogr_file_configuration.get()->set_original_file_srs(*d_source_srs);
		}

		FeatureCollectionFileFormat::Configuration::shared_ptr_to_const_type
				file_configuration = ogr_file_configuration.get();

		file_ref.set_file_info(file_ref.get_file_info(), file_configuration);

	}
}


void
GPlatesFileIO::OgrReader::add_ring_to_points_list(
	OGRLinearRing *ring, 
	std::vector<GPlatesMaths::PointOnSphere> &ring_points,
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
	double x,y;

	ring_points.clear();

	num_points = ring->getNumPoints();
 
	// TODO: check is this FIXME note relevant now...
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

	ring_points.reserve(num_points);

	for (count = 0; count < num_points; count++){
		x = ring->getX(count);
		y = ring->getY(count);
		if (transform_and_check_coords(x,y,read_errors,source,location)){
			GPlatesMaths::LatLonPoint llp(y,x);
			ring_points.push_back(GPlatesMaths::make_point_on_sphere(llp));
		}
		else{
			// One of our points is invalid. We can't create a feature, so clear the std::vector.
			ring_points.clear();
			return;
		}
	}

}


QStringList
GPlatesFileIO::OgrReader::read_field_names(
		GPlatesFileIO::File::Reference &file_ref,
		GPlatesModel::ModelInterface &model,
		ReadErrorAccumulation &read_errors)
{
	const FileInfo &fileinfo = file_ref.get_file_info();

	// By placing all changes to the model under the one changeset, we ensure that
	// feature revision ids don't get changed from what was loaded from file no
	// matter what we do to the features.
	GPlatesModel::ChangesetHandle changeset(
			model.access_model(),
			"read_field_names " + fileinfo.get_qfileinfo().fileName().toStdString());

	QString absolute_path_filename = fileinfo.get_qfileinfo().absoluteFilePath();
	QString filename = fileinfo.get_qfileinfo().fileName();

	OgrReader reader;
	if (!reader.open_file(absolute_path_filename, read_errors))
	{
		throw ErrorOpeningFileForReadingException(GPLATES_EXCEPTION_SOURCE, filename);
	}

	if (!reader.check_file_format(read_errors))
	{
		throw ErrorOpeningFileForReadingException(GPLATES_EXCEPTION_SOURCE, filename);
	}
	reader.get_field_names(read_errors);

	return reader.d_field_names;


}

void
GPlatesFileIO::OgrReader::remap_shapefile_attributes(
	GPlatesFileIO::File::Reference &file,
	GPlatesModel::ModelInterface &model,
	ReadErrorAccumulation &read_errors)
{
	// We want to merge model events across this scope so that only one model event
	// is generated instead of many in case we incrementally modify the features below.
	// Probably won't be modifying the model so much when loading but we should keep this anyway.
	GPlatesModel::NotificationGuard model_notification_guard(*model.access_model());

	const FileInfo &file_info = file.get_file_info();

	// Load the model-to-attribute map from the file's configuration.
	QMap<QString,QString> model_to_attribute_map;
	load_model_to_attribute_map_from_file_reference(model_to_attribute_map, file);

	// Save the model-to-attribute map to the mapping xml file.
	OgrUtils::save_attribute_map_as_xml_file(
			OgrUtils::make_ogr_xml_filename(file_info.get_qfileinfo()),
			model_to_attribute_map);

	remap_feature_collection(file, model_to_attribute_map, read_errors);
}

