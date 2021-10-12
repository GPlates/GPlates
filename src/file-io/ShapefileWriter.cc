/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2009 Geological Survey of Norway
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

#include <iosfwd>
#include <vector>
#include <boost/optional.hpp>
#include <boost/scoped_ptr.hpp>
#include <QDebug>
#include <QFile>
#include <QMap>
#include <QString>

#include "FileInfo.h"
#include "OgrException.h"
#include "ShapefileUtils.h"
#include "feature-visitors/GeometryTypeFinder.h"
#include "feature-visitors/KeyValueDictionaryFinder.h"
#include "feature-visitors/PropertyValueFinder.h"
#include "feature-visitors/ToQvariantConverter.h"

#include "maths/MultiPointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolylineOnSphere.h"
#include "maths/PointOnSphere.h"
#include "model/ConstFeatureVisitor.h"
#include "model/PropertyName.h"
#include "model/ModelUtils.h"
#include "property-values/GeoTimeInstant.h"
#include "property-values/GmlLineString.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlPolygon.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlKeyValueDictionaryElement.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/XsDouble.h"
#include "property-values/XsInteger.h"
#include "property-values/XsString.h"
#include "qt-widgets/ShapefilePropertyMapper.h"
#include "utils/UnicodeStringUtils.h"

#include "OgrWriter.h"
#include "ShapefileWriter.h"


namespace
{
	GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type
	create_multi_point_from_points(
		const std::vector<GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type> &points)
	{
		std::vector<GPlatesMaths::PointOnSphere> vector_of_points;
		
		std::vector<GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type>::const_iterator
			 it = points.begin(),
			 end = points.end();
			 
		for (; it != end ; ++it)
		{
			vector_of_points.push_back(**it);
		}
		
		return GPlatesMaths::MultiPointOnSphere::create_on_heap(vector_of_points);
				
	}
	
	void
	replace_model_kvd(
		const GPlatesModel::FeatureHandle &feature_handle,
		const GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type kvd)
	{
	
		GPlatesModel::FeatureHandle &non_const_feature_handle = 
			const_cast<GPlatesModel::FeatureHandle&>(feature_handle);
			
		GPlatesModel::FeatureHandle::properties_iterator p_iter = non_const_feature_handle.properties_begin();
		GPlatesModel::FeatureHandle::properties_iterator p_iter_end = non_const_feature_handle.properties_end();

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
			if (q_prop_name == "shapefileAttributes")
			{
				//qDebug() << "Removing old kvd...";
				GPlatesModel::DummyTransactionHandle transaction(__FILE__, __LINE__);
				non_const_feature_handle.remove_top_level_property(p_iter, transaction);
				transaction.commit();
			}

		} // loop over properties in feature. 	
	
		//qDebug() << "Adding new kvd...";
		GPlatesModel::WeakReference<GPlatesModel::FeatureHandle> 			feature_weak_ref(non_const_feature_handle);
		GPlatesModel::ModelUtils::append_property_value_to_feature(
			kvd,
			GPlatesModel::PropertyName::create_gpml("shapefileAttributes"),
			feature_weak_ref);	
	
	}

	QVariant
	get_qvariant_from_element(
		const GPlatesPropertyValues::GpmlKeyValueDictionaryElement &element)
	{
		GPlatesFeatureVisitors::ToQvariantConverter converter;

		element.value()->accept_visitor(converter);

		if (converter.found_values_begin() != converter.found_values_end())
		{
			return *converter.found_values_begin();
		}
		else
		{
			return QVariant();
		}
	}

	void
	write_kvd(
		GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type kvd)
	{
		std::vector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement>::const_iterator it =
			kvd->elements().begin();

		for (; it != kvd->elements().end(); ++it)
		{
			qDebug() << "Key: " << 
				GPlatesUtils::make_qstring_from_icu_string((*it).key()->value().get()) << 
				", Value: " << 
				get_qvariant_from_element(*it);
		}
	}

	void
	write_kvd(
		GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type kvd)
	{
		std::vector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement>::const_iterator it =
			kvd->elements().begin();

		for (; it != kvd->elements().end(); ++it)
		{
			qDebug() << "Key: " << 
				GPlatesUtils::make_qstring_from_icu_string((*it).key()->value().get()) << 
				", Value: " << 
				get_qvariant_from_element(*it);
		}
	}

	double
	get_time_from_time_period(
			const GPlatesPropertyValues::GmlTimeInstant &time_instant
	)
	{

		if (time_instant.time_position().is_real()) {
			return time_instant.time_position().value();
		}
		else if (time_instant.time_position().is_distant_past()) {
			return 999.;
		} 
		else if (time_instant.time_position().is_distant_future()) {
			return -999.;
		} 
		else {
			return 0;
		}
	}


	std::vector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement>::iterator 
	find_element_by_key(
		const QString &key,
		GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type dictionary)
	{

		std::vector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement>::iterator
			it = dictionary->elements().begin(),
			end = dictionary->elements().end();

		for (; it != end ; ++it)
		{
			QString key_string = GPlatesUtils::make_qstring_from_icu_string(it->key()->value().get());
			if (key == key_string)
			{
				break;
			}
		}
		return it;
	}

	void
	fill_kvd_with_plate_id(
		GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type dictionary,
		const QMap< QString,QString > &model_to_shapefile_map,
		const GPlatesModel::FeatureHandle::const_weak_ref &feature)
	{
		static const GPlatesModel::PropertyName plate_id_property_name =
			GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");

		const GPlatesPropertyValues::GpmlPlateId *recon_plate_id;

		if (GPlatesFeatureVisitors::get_property_value(feature,plate_id_property_name,recon_plate_id))
		{
			// The feature has a reconstruction plate ID.
			//qDebug() << "fill_kvd: found plate-id " << recon_plate_id->value();	

			GPlatesPropertyValues::XsInteger::non_null_ptr_type value = 
				GPlatesPropertyValues::XsInteger::create(recon_plate_id->value());		

			QMap <QString,QString>::const_iterator it = model_to_shapefile_map.find(
				ShapefileAttributes::model_properties[ShapefileAttributes::PLATEID]);

			if (it != model_to_shapefile_map.end())
			{

				QString element_key = it.value();

				//qDebug() << "Element key: " << element_key;

				std::vector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement>::iterator element = 
					find_element_by_key(element_key,dictionary);

				if (element != dictionary->elements().end())
				{
					// We've found an element corresponding to the plate-id; replace it with 
					// a new element containing the value extracted from the feature. 
					GPlatesPropertyValues::XsString::non_null_ptr_type key = element->key();
					GPlatesPropertyValues::TemplateTypeParameterType type = element->value_type();

					GPlatesPropertyValues::GpmlKeyValueDictionaryElement new_element(
						key,
						value,
						type);

					*element = new_element;			
				}
			}
		}
	}

	void
	fill_kvd_with_feature_type(
		GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type dictionary,
		const QMap< QString,QString > &model_to_shapefile_map,
		const GPlatesModel::FeatureHandle::const_weak_ref &feature)
	{
		static const GPlatesFileIO::ShapefileUtils::feature_map_type &feature_map = 
			GPlatesFileIO::ShapefileUtils::build_feature_map();

		if ( ! feature.is_valid()) {
			return;
		}

		QString feature_type_model_qstring = GPlatesUtils::make_qstring_from_icu_string(
			feature->feature_type().get_name());

		QString feature_type_key = feature_map.key(feature_type_model_qstring);

		GPlatesModel::PropertyValue::non_null_ptr_type feature_type_value =
			GPlatesPropertyValues::XsString::create(
				GPlatesUtils::make_icu_string_from_qstring(feature_type_key));

		QMap <QString,QString>::const_iterator it = model_to_shapefile_map.find(
			ShapefileAttributes::model_properties[ShapefileAttributes::FEATURE_TYPE]);

		if (it != model_to_shapefile_map.end())
		{

			QString element_key = it.value();

			//qDebug() << "Element key: " << element_key;

			std::vector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement>::iterator element = 
				find_element_by_key(element_key,dictionary);

			if (element != dictionary->elements().end())
			{
				// We've found an element corresponding to the feature type; replace it with 
				// a new element containing the value extracted from the feature. 
				GPlatesPropertyValues::XsString::non_null_ptr_type key = element->key();
				GPlatesPropertyValues::TemplateTypeParameterType type = element->value_type();

				GPlatesPropertyValues::GpmlKeyValueDictionaryElement new_element(
					key,
					feature_type_value,
					type);

				*element = new_element;			
			}
		}

	}

	void
	fill_kvd_with_begin_and_end_time(
		GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type dictionary,
		const QMap< QString,QString > &model_to_shapefile_map,
		const GPlatesModel::FeatureHandle::const_weak_ref &feature)
	{
		static const GPlatesModel::PropertyName valid_time_property_name =
			GPlatesModel::PropertyName::create_gml("validTime");

		const GPlatesPropertyValues::GmlTimePeriod *time_period;

		if (GPlatesFeatureVisitors::get_property_value(
			feature,valid_time_property_name,time_period))
		{

			double begin_time = get_time_from_time_period(*(time_period->begin()));
			double end_time = get_time_from_time_period(*(time_period->end()));

			GPlatesPropertyValues::XsDouble::non_null_ptr_type begin_value = 
				GPlatesPropertyValues::XsDouble::create(begin_time);

			GPlatesPropertyValues::XsDouble::non_null_ptr_type end_value = 
				GPlatesPropertyValues::XsDouble::create(end_time);

			QMap <QString,QString>::const_iterator it = model_to_shapefile_map.find(
				ShapefileAttributes::model_properties[ShapefileAttributes::BEGIN]);

			if (it != model_to_shapefile_map.end())
			{

				QString element_key = it.value();

				//qDebug() << "Element key: " << element_key;

				std::vector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement>::iterator element = 
					find_element_by_key(element_key,dictionary);

				if (element != dictionary->elements().end())
				{
					// We've found an element corresponding to the begin time; replace it with 
					// a new element containing the value extracted from the feature. 
					GPlatesPropertyValues::XsString::non_null_ptr_type key = element->key();
					GPlatesPropertyValues::TemplateTypeParameterType type = element->value_type();

					GPlatesPropertyValues::GpmlKeyValueDictionaryElement new_element(
						key,
						begin_value,
						type);

					*element = new_element;			
				}
			}

			it = model_to_shapefile_map.find(
				ShapefileAttributes::model_properties[ShapefileAttributes::END]);

			if (it != model_to_shapefile_map.end())
			{

				QString element_key = it.value();

				//qDebug() << "Element key: " << element_key;

				std::vector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement>::iterator element = 
					find_element_by_key(element_key,dictionary);

				if (element != dictionary->elements().end())
				{
					// We've found an element corresponding to the end time; replace it with 
					// a new element containing the value extracted from the feature. 
					GPlatesPropertyValues::XsString::non_null_ptr_type key = element->key();
					GPlatesPropertyValues::TemplateTypeParameterType type = element->value_type();

					GPlatesPropertyValues::GpmlKeyValueDictionaryElement new_element(
						key,
						end_value,
						type);

					*element = new_element;			
				}
			}
		}

	}

	void
	fill_kvd_with_name(
		GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type dictionary,
		const QMap< QString,QString > &model_to_shapefile_map,
		const GPlatesModel::FeatureHandle::const_weak_ref &feature)
	{

		static const GPlatesModel::PropertyName name_property_name = 
			GPlatesModel::PropertyName::create_gml("name");

		const GPlatesPropertyValues::XsString *name;

		if (GPlatesFeatureVisitors::get_property_value(
				feature,name_property_name,name)) {

			GPlatesModel::PropertyValue::non_null_ptr_type name_value =
						name->clone();

			QMap <QString,QString>::const_iterator it = model_to_shapefile_map.find(
				ShapefileAttributes::model_properties[ShapefileAttributes::NAME]);

			if (it != model_to_shapefile_map.end())
			{

				QString element_key = it.value();

				//qDebug() << "Element key: " << element_key;

				std::vector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement>::iterator element = 
					find_element_by_key(element_key,dictionary);

				if (element != dictionary->elements().end())
				{
					// We've found an element corresponding to the name; replace it with 
					// a new element containing the value extracted from the feature. 
					GPlatesPropertyValues::XsString::non_null_ptr_type key = element->key();
					GPlatesPropertyValues::TemplateTypeParameterType type = element->value_type();

					GPlatesPropertyValues::GpmlKeyValueDictionaryElement new_element(
						key,
						name_value,
						type);

					*element = new_element;			
				}
			}
		}

	}

	void
	fill_kvd_with_description(
		GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type dictionary,
		const QMap< QString,QString > &model_to_shapefile_map,
		const GPlatesModel::FeatureHandle::const_weak_ref &feature)
	{

		static const GPlatesModel::PropertyName description_property_name = 
			GPlatesModel::PropertyName::create_gml("description");

		const GPlatesPropertyValues::XsString *description;

		if (GPlatesFeatureVisitors::get_property_value(
			feature,description_property_name,description)) {

				GPlatesModel::PropertyValue::non_null_ptr_type description_value =
					description->clone();

			QMap <QString,QString>::const_iterator it = model_to_shapefile_map.find(
				ShapefileAttributes::model_properties[ShapefileAttributes::DESCRIPTION]);

			if (it != model_to_shapefile_map.end())
			{

				QString element_key = it.value();

				//qDebug() << "Element key: " << element_key;

				std::vector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement>::iterator element = 
					find_element_by_key(element_key,dictionary);

				if (element != dictionary->elements().end())
				{
					// We've found an element corresponding to the description; replace it with 
					// a new element containing the value extracted from the feature. 
					GPlatesPropertyValues::XsString::non_null_ptr_type key = element->key();
					GPlatesPropertyValues::TemplateTypeParameterType type = element->value_type();

					GPlatesPropertyValues::GpmlKeyValueDictionaryElement new_element(
						key,
						description_value,
						type);

					*element = new_element;			
				}
			}
		}

	}

	void
	create_default_kvd_from_map(
		boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type> &default_key_value_dictionary,	
		const QMap< QString,QString > &model_to_shapefile_map)
	{
		std::vector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement> elements;

		// Add a plate ID entry.
		QMap<QString,QString>::const_iterator it = 
			model_to_shapefile_map.find(ShapefileAttributes::model_properties[ShapefileAttributes::PLATEID]);

		GPlatesPropertyValues::XsString::non_null_ptr_type key = 
			GPlatesPropertyValues::XsString::create(
				GPlatesUtils::make_icu_string_from_qstring(*it));

		GPlatesPropertyValues::XsInteger::non_null_ptr_type plate_id_value = 
			GPlatesPropertyValues::XsInteger::create(0);

		GPlatesPropertyValues::GpmlKeyValueDictionaryElement plate_id_element(
					key,
					plate_id_value,
					GPlatesPropertyValues::TemplateTypeParameterType::create_xsi("integer"));
		elements.push_back(plate_id_element);		

		// Add a feature type entry.
		it = model_to_shapefile_map.find(ShapefileAttributes::model_properties[ShapefileAttributes::FEATURE_TYPE]);
		key = GPlatesPropertyValues::XsString::create(
			GPlatesUtils::make_icu_string_from_qstring(*it));
		GPlatesPropertyValues::XsString::non_null_ptr_type type_value = 
			GPlatesPropertyValues::XsString::create(UnicodeString());

		GPlatesPropertyValues::GpmlKeyValueDictionaryElement type_element(
			key,
			type_value,
			GPlatesPropertyValues::TemplateTypeParameterType::create_xsi("string"));
		elements.push_back(type_element);	

		// Add a time-of-appearance entry.
		it = model_to_shapefile_map.find(ShapefileAttributes::model_properties[ShapefileAttributes::BEGIN]);
		key = GPlatesPropertyValues::XsString::create(
				GPlatesUtils::make_icu_string_from_qstring(*it));
		GPlatesPropertyValues::XsDouble::non_null_ptr_type begin_value = 
			GPlatesPropertyValues::XsDouble::create(0.0);

		GPlatesPropertyValues::GpmlKeyValueDictionaryElement begin_element(
					key,
					begin_value,
					GPlatesPropertyValues::TemplateTypeParameterType::create_xsi("double"));
		elements.push_back(begin_element);	

		// Add a time-of-disappearance entry.
		it = model_to_shapefile_map.find(ShapefileAttributes::model_properties[ShapefileAttributes::END]);
		key = GPlatesPropertyValues::XsString::create(
				GPlatesUtils::make_icu_string_from_qstring(*it));
		GPlatesPropertyValues::XsDouble::non_null_ptr_type end_value = 
			GPlatesPropertyValues::XsDouble::create(0.0);

		GPlatesPropertyValues::GpmlKeyValueDictionaryElement end_element(
					key,
					end_value,
					GPlatesPropertyValues::TemplateTypeParameterType::create_xsi("double"));
		elements.push_back(end_element);	

		// Add a name entry.
		it = model_to_shapefile_map.find(ShapefileAttributes::model_properties[ShapefileAttributes::NAME]);
		key = GPlatesPropertyValues::XsString::create(
			GPlatesUtils::make_icu_string_from_qstring(*it));
		GPlatesPropertyValues::XsString::non_null_ptr_type name_value = 
			GPlatesPropertyValues::XsString::create(UnicodeString());

		GPlatesPropertyValues::GpmlKeyValueDictionaryElement name_element(
			key,
			name_value,
			GPlatesPropertyValues::TemplateTypeParameterType::create_xsi("string"));
		elements.push_back(name_element);	

		// Add a description entry.
		it = model_to_shapefile_map.find(ShapefileAttributes::model_properties[ShapefileAttributes::DESCRIPTION]);
		key = GPlatesPropertyValues::XsString::create(
			GPlatesUtils::make_icu_string_from_qstring(*it));
		GPlatesPropertyValues::XsString::non_null_ptr_type description_value = 
			GPlatesPropertyValues::XsString::create(UnicodeString());

		GPlatesPropertyValues::GpmlKeyValueDictionaryElement description_element(
			key,
			description_value,
			GPlatesPropertyValues::TemplateTypeParameterType::create_xsi("string"));
		elements.push_back(description_element);	

		// Add them all to the default kvd.
		default_key_value_dictionary.reset(GPlatesPropertyValues::GpmlKeyValueDictionary::create(elements));
	}

	void
	fill_kvd(
		GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type dictionary,
		const QMap< QString,QString > &model_to_shapefile_map,
		const GPlatesModel::FeatureHandle &feature_handle)
	{
		GPlatesModel::FeatureHandle::const_weak_ref feature = feature_handle.reference();

		fill_kvd_with_plate_id(dictionary,model_to_shapefile_map,feature);
		fill_kvd_with_feature_type(dictionary,model_to_shapefile_map,feature);
		fill_kvd_with_begin_and_end_time(dictionary,model_to_shapefile_map,feature);
		fill_kvd_with_name(dictionary,model_to_shapefile_map,feature);
		fill_kvd_with_description(dictionary,model_to_shapefile_map,feature);

	}
	
	void
	create_default_model_to_shapefile_map(
		const GPlatesFileIO::FileInfo &file_info)
	{
		QMap< QString, QString > model_to_shapefile_map;
		model_to_shapefile_map.insert(
			ShapefileAttributes::model_properties[ShapefileAttributes::PLATEID],
			ShapefileAttributes::default_attributes[ShapefileAttributes::PLATEID]);
		model_to_shapefile_map.insert(
			ShapefileAttributes::model_properties[ShapefileAttributes::FEATURE_TYPE],
			ShapefileAttributes::default_attributes[ShapefileAttributes::FEATURE_TYPE]);
		model_to_shapefile_map.insert(
			ShapefileAttributes::model_properties[ShapefileAttributes::BEGIN],
			ShapefileAttributes::default_attributes[ShapefileAttributes::BEGIN]);
		model_to_shapefile_map.insert(
			ShapefileAttributes::model_properties[ShapefileAttributes::END],
			ShapefileAttributes::default_attributes[ShapefileAttributes::END]);
		model_to_shapefile_map.insert(
			ShapefileAttributes::model_properties[ShapefileAttributes::DESCRIPTION],
			ShapefileAttributes::default_attributes[ShapefileAttributes::DESCRIPTION]);
		model_to_shapefile_map.insert(
			ShapefileAttributes::model_properties[ShapefileAttributes::NAME],
			ShapefileAttributes::default_attributes[ShapefileAttributes::NAME]);

		file_info.set_model_to_shapefile_map(model_to_shapefile_map);
	}


	void
	create_default_kvd_from_collection(
			const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection,
			boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type> &default_key_value_dictionary)
	{
		if (feature_collection.is_valid())
		{
			GPlatesModel::FeatureCollectionHandle::features_const_iterator
				iter = feature_collection->features_begin(), 
				end = feature_collection->features_end();

			while ((iter != end) && !default_key_value_dictionary)
			{
				// FIXME: Replace this kvd-finder with the new PropertyValueFinder.
				GPlatesFeatureVisitors::KeyValueDictionaryFinder finder;
				finder.visit_feature(iter);
				if (finder.number_of_found_dictionaries() != 0)
				{
					GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type found_kvd =
						*(finder.found_key_value_dictionaries_begin());
					default_key_value_dictionary = GPlatesPropertyValues::GpmlKeyValueDictionary::create(
						found_kvd->elements());
				}

				++iter;
			}
			
		}		
	}

	void
	write_point_geometries(
		GPlatesFileIO::OgrWriter *ogr_writer,
		const std::vector<GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type> &point_geometries,
		const boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type> &key_value_dictionary)
	{
		if (point_geometries.size() > 1)
		{
		// We have more than one point in the feature, so we should handle this as a multi-point.
			GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point =
				create_multi_point_from_points(point_geometries);

			ogr_writer->write_multi_point_feature(multi_point,key_value_dictionary);
		}
		else
		{
			std::vector<GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type>::const_iterator 
				iter = point_geometries.begin(),
				end = point_geometries.end();

			for ( ; iter != end ; ++iter)
			{
				ogr_writer->write_point_feature(*iter,key_value_dictionary);
			}

		}
	}

	void
	write_multi_point_geometries(
		GPlatesFileIO::OgrWriter *ogr_writer,
		const std::vector<GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type> &multi_point_geometries,
		const boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type> &key_value_dictionary)
	{
		std::vector<GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type>::const_iterator 
			iter = multi_point_geometries.begin(),
			end = multi_point_geometries.end();

		for ( ; iter != end ; ++iter)
		{
			ogr_writer->write_multi_point_feature(*iter,key_value_dictionary);
		}
	}

	void
	write_polyline_geometries(
		GPlatesFileIO::OgrWriter *ogr_writer,
		const std::vector<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> &polyline_geometries,
		const boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type> &key_value_dictionary)
	{
		ogr_writer->write_polyline_feature(polyline_geometries,key_value_dictionary);	
	}

	void
	write_polygon_geometries(
		GPlatesFileIO::OgrWriter *ogr_writer,
		const std::vector<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> &polygon_geometries,
		const boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type> &key_value_dictionary)
	{
		ogr_writer->write_polygon_feature(polygon_geometries,key_value_dictionary);
	}
}

GPlatesFileIO::ShapefileWriter::ShapefileWriter(
	const FileInfo &file_info,
	const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection_ref)
{
	// Check what types of geometries exist in the feature collection.

	GPlatesFeatureVisitors::GeometryTypeFinder finder;

	GPlatesModel::FeatureCollectionHandle::features_const_iterator 
		iter = feature_collection_ref->features_begin(),
		end = feature_collection_ref->features_end();

	for ( ; iter != end ; ++iter)
	{
		finder.visit_feature(iter);
	}

	// Set up an appropriate OgrWriter.
	d_ogr_writer.reset(new OgrWriter(file_info.get_qfileinfo().filePath(),finder.has_found_multiple_geometries()));

	// The file_info might not have a model_to_shapefile_map - the feature collection
	// might have originated from a plates file, for example. If we don't have one,
	// create a default map.
	if (file_info.get_model_to_shapefile_map().isEmpty())
	{
		create_default_model_to_shapefile_map(file_info);

#if 0
	// Disable exporting of xml file for the moment, until I arrange things so that
	// multiple-layer shapefiles each have their own xml mapping file.  

		// Export the newly created map as an shp.gplates.xml file.
		QString shapefile_xml_filename = ShapefileUtils::make_shapefile_xml_filename(file_info.get_qfileinfo());

		// FIXME: If we have multiple layers, we should really export the appropriately named xml file
		// for each layer. 
		ShapefileUtils::save_attribute_map_as_xml_file(shapefile_xml_filename,
			file_info.get_model_to_shapefile_map());
#endif
	}

	d_model_to_shapefile_map = file_info.get_model_to_shapefile_map();

	// Look for a key value dictionary, and store it as the default. 
	create_default_kvd_from_collection(feature_collection_ref,d_default_key_value_dictionary);

	// We didn't find one, so make one from the model-to-attribute-map. 
	if (!d_default_key_value_dictionary)
	{
		create_default_kvd_from_map(d_default_key_value_dictionary,
			d_model_to_shapefile_map);
	}

}


bool
GPlatesFileIO::ShapefileWriter::initialise_pre_feature_properties(
		const GPlatesModel::FeatureHandle &feature_handle)
{
	if (!d_ogr_writer)
	{
		return false;
	}

	clear_accumulators();

	// Next, visit the feature properties to check which geometry types exist in the feature
	// and fill the relevant geometry containers.
	return true;
}


void
GPlatesFileIO::ShapefileWriter::finalise_post_feature_properties(
		const GPlatesModel::FeatureHandle &feature_handle)
{
	if (!d_key_value_dictionary)
	{
		// We haven't found shapefile attributes in this feature, so we'll create a set of attributes
		// from the feature's properties and the model_to_shapefile map. 
		//qDebug() << "Couldn't find shapefile-attributes - generating new ones from the feature";
		fill_kvd(d_default_key_value_dictionary.get(),
				d_model_to_shapefile_map,
				feature_handle);

		if (d_default_key_value_dictionary)
		{
			//qDebug() << "Default kvd exists";
	
			d_key_value_dictionary.reset(
				GPlatesPropertyValues::GpmlKeyValueDictionary::create( 
					(*d_default_key_value_dictionary)->elements()));
		}

		// Add the dictionary to the model.
		// Make the feature handle non-const so I can append to it.
		GPlatesModel::FeatureHandle &non_const_feature_handle = const_cast<GPlatesModel::FeatureHandle &>(feature_handle);
	
		GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type kvd =
			GPlatesPropertyValues::GpmlKeyValueDictionary::create( 
			(*d_default_key_value_dictionary)->elements());

		GPlatesModel::WeakReference<GPlatesModel::FeatureHandle>  feature_weak_ref(non_const_feature_handle);
		GPlatesModel::ModelUtils::append_property_value_to_feature(
			kvd,
			GPlatesModel::PropertyName::create_gpml("shapefileAttributes"),
			feature_weak_ref);	

	}
	else
	{
		// We do have a shapefile kvd. Update it from the model. 
		//qDebug() << "Found already existing kvd.";
		GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type dictionary =
			GPlatesPropertyValues::GpmlKeyValueDictionary::create((*d_key_value_dictionary)->elements());
			
		fill_kvd(dictionary,
				d_model_to_shapefile_map,feature_handle);
				
		d_key_value_dictionary.reset(dictionary);

		//FIXME: Update the model's kvd from the new kvd values. 
		replace_model_kvd(feature_handle,
						dictionary);
	}

	// If a feature contains different geometry types, the geometries will be exported to
	// the appropriate file of the shapefile set.  
	// This means that we're potentially splitting up a feature across different files.

	write_point_geometries(d_ogr_writer.get(),d_point_geometries,d_key_value_dictionary);
	write_multi_point_geometries(d_ogr_writer.get(),d_multi_point_geometries,d_key_value_dictionary);
	write_polyline_geometries(d_ogr_writer.get(),d_polyline_geometries,d_key_value_dictionary);
	write_polygon_geometries(d_ogr_writer.get(),d_polygon_geometries,d_key_value_dictionary);

}


void
GPlatesFileIO::ShapefileWriter::visit_gml_point(
	const GPlatesPropertyValues::GmlPoint &gml_point)
{
	d_point_geometries.push_back(gml_point.point());
}

void
GPlatesFileIO::ShapefileWriter::visit_gml_multi_point(
	const GPlatesPropertyValues::GmlMultiPoint &gml_multi_point)
{
	d_multi_point_geometries.push_back(gml_multi_point.multipoint());
}

void
GPlatesFileIO::ShapefileWriter::visit_gml_line_string(
	const GPlatesPropertyValues::GmlLineString &gml_line_string)
{
	d_polyline_geometries.push_back(gml_line_string.polyline());
}

void
GPlatesFileIO::ShapefileWriter::visit_gml_orientable_curve(
	const GPlatesPropertyValues::GmlOrientableCurve &gml_orientable_curve)
{
	gml_orientable_curve.base_curve()->accept_visitor(*this);
}

void
GPlatesFileIO::ShapefileWriter::visit_gml_polygon(
	const GPlatesPropertyValues::GmlPolygon &gml_polygon)
{
	// FIXME: Do something about interior rings....
	d_polygon_geometries.push_back(gml_polygon.exterior());
}

void
GPlatesFileIO::ShapefileWriter::visit_gpml_constant_value(
	const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	gpml_constant_value.value()->accept_visitor(*this);
}

void
GPlatesFileIO::ShapefileWriter::visit_gpml_key_value_dictionary(
	const GPlatesPropertyValues::GpmlKeyValueDictionary &gpml_key_value_dictionary)
{
	if (d_key_value_dictionary)
	{
		// We seem to have a key_value_dictionary already. 
		// I'm going to ignore the one we're currently visiting.
		qDebug() << "Multiple key-value-dictionaries found in feature.";
		return;
	}

	// FIXME: Check that the dictionary's property name is shapefileAttributes.
	d_key_value_dictionary.reset(
		GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type(
		&gpml_key_value_dictionary,GPlatesUtils::NullIntrusivePointerHandler()));
}

void
GPlatesFileIO::ShapefileWriter::clear_accumulators()
{
	d_point_geometries.clear();
	d_multi_point_geometries.clear();
	d_polyline_geometries.clear();
	d_polygon_geometries.clear();

	d_key_value_dictionary.reset();
}
