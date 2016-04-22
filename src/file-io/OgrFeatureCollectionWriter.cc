/* $Id$ */

/**
 * \file
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 *
 * Copyright (C) 2009, 2010, 2011, 2012, 2013 Geological Survey of Norway
 * Copyright (C) 2010 The University of Sydney, Australia
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

#include <vector>
#include <boost/optional.hpp>
#include <boost/scoped_ptr.hpp>
#include <QDebug>
#include <QFile>
#include <QMap>
#include <QString>

#include "FeatureCollectionFileFormatConfigurations.h"
#include "FileInfo.h"
#include "OgrException.h"
#include "PropertyMapper.h"
#include "OgrUtils.h"
#include "feature-visitors/GeometryTypeFinder.h"
#include "feature-visitors/KeyValueDictionaryFinder.h"
#include "feature-visitors/PropertyValueFinder.h"
#include "feature-visitors/ToQvariantConverter.h"

#include "maths/MultiPointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolylineOnSphere.h"
#include "maths/PointOnSphere.h"
#include "model/FeatureVisitor.h"
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
#include "property-values/GpmlOldPlatesHeader.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/XsDouble.h"
#include "property-values/XsInteger.h"
#include "property-values/XsString.h"
#include "utils/UnicodeStringUtils.h"

#include "OgrWriter.h"
#include "OgrFeatureCollectionWriter.h"


namespace
{


	bool
	is_shapefile_format(
			const QFileInfo &qfileinfo)
	{
		QString suffix = qfileinfo.suffix();
		suffix = suffix.toLower();
		return (suffix == "shp");
	}

	bool
	is_ogrgmt_format(
			const QFileInfo &qfileinfo)
	{
		QString suffix = qfileinfo.suffix();
		suffix = suffix.toLower();
		return (suffix == "gmt");
	}

	/*!
	 * \brief get_key_string - if a key for the model property given by @param property_enum exists
	 * in @param model_to_shapefile_map, return the value for that key; otherwise return the default
	 * attribute name for that property.
	 */
	QString
	get_key_string(
			const model_to_attribute_map_type &model_to_shapefile_map,
			const ShapefileAttributes::ModelProperties &property_enum)
	{
		model_to_attribute_map_type::ConstIterator it =
				model_to_shapefile_map.find(ShapefileAttributes::model_properties[property_enum]);

		// If we didn't find an entry for property_enum, return the default attribute name.
		if (it == model_to_shapefile_map.end())
		{
			return ShapefileAttributes::default_attribute_field_names[property_enum];
		}


		return it.value();
	}

	/*!
	 * Returns an iterator to the kvd element of @param dictionary which corresponds
	 * to the key @key.
	 */
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

	/**
	 * Adds or replaces @a new_element to the kvd @a dictionary.
	 *
	 * If an element with a key corresponding to  @a key_string already exists in @a dictionary, that
	 * element is replaced by @a new_element.
	 */
	void
	add_or_replace_kvd_element(
			const GPlatesPropertyValues::GpmlKeyValueDictionaryElement &new_element,
			const QString &key_string,
			GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type dictionary)
	{

		std::vector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement>::iterator element =
				find_element_by_key(key_string,dictionary);

		if (element == dictionary->elements().end())
		{
			dictionary->elements().push_back(new_element);
		}
		else
		{
			*element = new_element;
		}
	}
	
	/*!
	 * \brief add_field_to_kvd - adds the entry given by key @a key_string and value @a value to kvd @a dictionary.
	 * If an entry with key @a key_string already exists, the value of the entry will be overwritten with @a value.
	 *
	 * \param key_string
	 * \param value
	 * \param type
	 * \param dictionary
	 */
	void
	add_field_to_kvd(
			const QString &key_string,
			GPlatesModel::PropertyValue::non_null_ptr_type value,
			const GPlatesPropertyValues::StructuralType &type,
			GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type dictionary)
	{
		GPlatesPropertyValues::XsString::non_null_ptr_type key =
				GPlatesPropertyValues::XsString::create(GPlatesUtils::make_icu_string_from_qstring(
															key_string));

		GPlatesPropertyValues::GpmlKeyValueDictionaryElement new_element(
					key,
					value,
					type);

		add_or_replace_kvd_element(new_element,key_string,dictionary);
	}

	void
	add_plate_id_key_to_kvd_if_missing(
			GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type dictionary,
			const QMap< QString, QString > &model_to_shapefile_map)
	{
		GPlatesPropertyValues::XsInteger::non_null_ptr_type value =
				// FIXME: should we be using some sort of easily recognisable value to
				// represent "no" plate-id, like -999?
				GPlatesPropertyValues::XsInteger::create(0);

		add_field_to_kvd(get_key_string(model_to_shapefile_map, ShapefileAttributes::PLATEID),
						 value,
						 GPlatesPropertyValues::StructuralType::create_xsi("integer"),
						 dictionary);
	}
	
	void
	add_begin_time_key_to_kvd_if_missing(
			GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type dictionary,
			const QMap< QString, QString > &model_to_shapefile_map)
	{

		GPlatesPropertyValues::XsDouble::non_null_ptr_type value =
				GPlatesPropertyValues::XsDouble::create(999.);

		add_field_to_kvd(get_key_string(model_to_shapefile_map, ShapefileAttributes::BEGIN),
						 value,
						 GPlatesPropertyValues::StructuralType::create_xsi("double"),
						 dictionary);
	}
	
	void
	add_end_time_key_to_kvd_if_missing(
			GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type dictionary,
			const QMap< QString, QString > &model_to_shapefile_map)
	{

		GPlatesPropertyValues::XsDouble::non_null_ptr_type value =
				GPlatesPropertyValues::XsDouble::create(-999.);

		add_field_to_kvd(get_key_string(model_to_shapefile_map, ShapefileAttributes::END),
						 value,
						 GPlatesPropertyValues::StructuralType::create_xsi("double"),
						 dictionary);
	}
	
	void
	add_name_key_to_kvd_if_missing(
			GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type dictionary,
			const QMap< QString, QString > &model_to_shapefile_map)
	{

		GPlatesPropertyValues::XsString::non_null_ptr_type value =
				GPlatesPropertyValues::XsString::create("");

		add_field_to_kvd(get_key_string(model_to_shapefile_map, ShapefileAttributes::NAME),
						 value,
						 GPlatesPropertyValues::StructuralType::create_xsi("string"),
						 dictionary);
	}
	
	void
	add_description_key_to_kvd_if_missing(
			GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type dictionary,
			const QMap< QString, QString > &model_to_shapefile_map)
	{

		GPlatesPropertyValues::XsString::non_null_ptr_type value =
				GPlatesPropertyValues::XsString::create("");

		add_field_to_kvd(get_key_string(model_to_shapefile_map, ShapefileAttributes::DESCRIPTION),
						 value,
						 GPlatesPropertyValues::StructuralType::create_xsi("string"),
						 dictionary);
	}
	
	void
	add_feature_type_key_to_kvd_if_missing(
			GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type dictionary,
			const QMap< QString, QString > &model_to_shapefile_map)
	{
		// We add the GPGIM_TYPE to the kvd in all cases.
		add_field_to_kvd("GPGIM_TYPE",
						 GPlatesPropertyValues::XsString::create(""),
						 GPlatesPropertyValues::StructuralType::create_xsi("string"),
						 dictionary);

		// Now we also add the 2-letter code field. First check if the mapped field name is
		// GPGIM_TYPE, in which case we'll use TYPE for the 2-letter code. If the mapped field
		// isn't GPGIM_TYPE, we assume the user has defined their own field name and that they
		// are using this to map the 2-letter code type.
		GPlatesPropertyValues::XsString::non_null_ptr_type value =
				GPlatesPropertyValues::XsString::create("");

		QString key_string;
		if (GPlatesFileIO::OgrUtils::feature_type_field_is_gpgim_type(model_to_shapefile_map))
		{
			key_string = get_key_string(model_to_shapefile_map, ShapefileAttributes::FEATURE_TYPE);
		}
		else
		{
			key_string = "TYPE";
		}

		add_field_to_kvd(key_string,
						 value,
						 GPlatesPropertyValues::StructuralType::create_xsi("string"),
						 dictionary);
	}
	
	void
	add_feature_id_key_to_kvd_if_missing(
			GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type dictionary,
			const QMap< QString, QString > &model_to_shapefile_map)
	{

		GPlatesPropertyValues::XsString::non_null_ptr_type value =
				GPlatesPropertyValues::XsString::create("");

		add_field_to_kvd(get_key_string(model_to_shapefile_map, ShapefileAttributes::FEATURE_ID),
						 value,
						 GPlatesPropertyValues::StructuralType::create_xsi("string"),
						 dictionary);
	}
	
	void
	add_conjugate_key_to_kvd_if_missing(
			GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type dictionary,
			const QMap< QString, QString > &model_to_shapefile_map)
	{
		GPlatesPropertyValues::XsInteger::non_null_ptr_type value =
				GPlatesPropertyValues::XsInteger::create(0);

		add_field_to_kvd(get_key_string(model_to_shapefile_map, ShapefileAttributes::CONJUGATE_PLATE_ID),
						 value,
						 GPlatesPropertyValues::StructuralType::create_xsi("integer"),
						 dictionary);
	}

	void
	add_left_plate_key_to_kvd_if_missing(
			GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type dictionary,
			const QMap< QString, QString > &model_to_shapefile_map)
	{
		GPlatesPropertyValues::XsInteger::non_null_ptr_type value =
				GPlatesPropertyValues::XsInteger::create(0);

		add_field_to_kvd(get_key_string(model_to_shapefile_map, ShapefileAttributes::LEFT_PLATE),
						 value,
						 GPlatesPropertyValues::StructuralType::create_xsi("integer"),
						 dictionary);
	}

	void
	add_right_plate_key_to_kvd_if_missing(
			GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type dictionary,
			const QMap< QString, QString > &model_to_shapefile_map)
	{
		GPlatesPropertyValues::XsInteger::non_null_ptr_type value =
				GPlatesPropertyValues::XsInteger::create(0);

		add_field_to_kvd(get_key_string(model_to_shapefile_map, ShapefileAttributes::RIGHT_PLATE),
						 value,
						 GPlatesPropertyValues::StructuralType::create_xsi("integer"),
						 dictionary);
	}
	
	void
	add_reconstruction_method_key_to_kvd_if_missing(
			GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type dictionary,
			const QMap< QString, QString > &model_to_shapefile_map)
	{
		GPlatesPropertyValues::XsString::non_null_ptr_type value =
				GPlatesPropertyValues::XsString::create("");

		add_field_to_kvd(get_key_string(model_to_shapefile_map, ShapefileAttributes::RECONSTRUCTION_METHOD),
						 value,
						 GPlatesPropertyValues::StructuralType::create_xsi("string"),
						 dictionary);
	}

	void
	add_spreading_asymmetry_key_to_kvd_if_missing(
			GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type dictionary,
			const QMap< QString, QString > &model_to_shapefile_map)
	{
		GPlatesPropertyValues::XsDouble::non_null_ptr_type value =
				GPlatesPropertyValues::XsDouble::create(0.);

		add_field_to_kvd(get_key_string(model_to_shapefile_map, ShapefileAttributes::SPREADING_ASYMMETRY),
						 value,
						 GPlatesPropertyValues::StructuralType::create_xsi("string"),
						 dictionary);
	}

	void
	add_region_to_kvd(
			const GPlatesPropertyValues::GpmlOldPlatesHeader *old_plates_header,
			GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type dictionary)
	{
		GPlatesPropertyValues::XsInteger::non_null_ptr_type value =
				GPlatesPropertyValues::XsInteger::create(old_plates_header->region_number());

		QString key_string("REGION_NO");

		add_field_to_kvd(key_string,
						 value,
						 GPlatesPropertyValues::StructuralType::create_xsi("integer"),
						 dictionary);

	}
	
	void
	add_reference_number_to_kvd(
			const GPlatesPropertyValues::GpmlOldPlatesHeader *old_plates_header,
			GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type dictionary)
	{
		GPlatesPropertyValues::XsInteger::non_null_ptr_type value =
				GPlatesPropertyValues::XsInteger::create(old_plates_header->reference_number());

		QString key_string("REF_NO");

		add_field_to_kvd(key_string,
						 value,
						 GPlatesPropertyValues::StructuralType::create_xsi("integer"),
						 dictionary);
	}
	
	void
	add_string_number_to_kvd(
			const GPlatesPropertyValues::GpmlOldPlatesHeader *old_plates_header,
			GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type dictionary)
	{
		GPlatesPropertyValues::XsInteger::non_null_ptr_type value =
				GPlatesPropertyValues::XsInteger::create(old_plates_header->string_number());

		QString key_string("STRING_NO");
		
		add_field_to_kvd(key_string,
						 value,
						 GPlatesPropertyValues::StructuralType::create_xsi("integer"),
						 dictionary);
	}
	
	void
	add_data_type_code_number_to_kvd(
			const GPlatesPropertyValues::GpmlOldPlatesHeader *old_plates_header,
			GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type dictionary)
	{
		GPlatesPropertyValues::XsInteger::non_null_ptr_type value =
				GPlatesPropertyValues::XsInteger::create(old_plates_header->data_type_code_number());

		QString key_string("TYPE_NO");

		add_field_to_kvd(key_string,
						 value,
						 GPlatesPropertyValues::StructuralType::create_xsi("integer"),
						 dictionary);

	}
	
	void
	add_data_type_code_number_additional_to_kvd(
			const GPlatesPropertyValues::GpmlOldPlatesHeader *old_plates_header,
			GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type dictionary)
	{
		GPlatesPropertyValues::XsString::non_null_ptr_type value =
				GPlatesPropertyValues::XsString::create(old_plates_header->data_type_code_number_additional());

		QString key_string("TYPE_NO_ADD");

		add_field_to_kvd(key_string,
						 value,
						 GPlatesPropertyValues::StructuralType::create_xsi("string"),
						 dictionary);
	}

	
	void
	add_colour_code_to_kvd(
			const GPlatesPropertyValues::GpmlOldPlatesHeader *old_plates_header,
			GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type dictionary)
	{
		GPlatesPropertyValues::XsInteger::non_null_ptr_type value =
				GPlatesPropertyValues::XsInteger::create(old_plates_header->colour_code());

		QString key_string("COLOUR");

		add_field_to_kvd(key_string,
						 value,
						 GPlatesPropertyValues::StructuralType::create_xsi("integer"),
						 dictionary);

	}
	
	void
	add_number_of_points_to_kvd(
			const GPlatesPropertyValues::GpmlOldPlatesHeader *old_plates_header,
			GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type dictionary)
	{
		GPlatesPropertyValues::XsInteger::non_null_ptr_type value =
				GPlatesPropertyValues::XsInteger::create(old_plates_header->number_of_points());

		QString key_string("NPOINTS");

		add_field_to_kvd(key_string,
						 value,
						 GPlatesPropertyValues::StructuralType::create_xsi("integer"),
						 dictionary);

	}

	void
	add_plates_header_values_to_kvd(
			GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type dictionary,
			const GPlatesModel::FeatureHandle &feature_handle)
	{
		static const GPlatesModel::PropertyName old_plates_header_property_name =
				GPlatesModel::PropertyName::create_gpml("oldPlatesHeader");

		boost::optional<GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_to_const_type> old_plates_header =
				GPlatesFeatureVisitors::get_property_value<GPlatesPropertyValues::GpmlOldPlatesHeader>(
						feature_handle.reference(), old_plates_header_property_name);
		if (old_plates_header)
		{
			add_region_to_kvd(old_plates_header.get().get(), dictionary);
			add_reference_number_to_kvd(old_plates_header.get().get(), dictionary);
			add_string_number_to_kvd(old_plates_header.get().get(), dictionary);
			add_data_type_code_number_to_kvd(old_plates_header.get().get(), dictionary);
			add_data_type_code_number_additional_to_kvd(old_plates_header.get().get(), dictionary);
			add_colour_code_to_kvd(old_plates_header.get().get(), dictionary);
			add_number_of_points_to_kvd(old_plates_header.get().get(), dictionary);
		}
	}

	/**
	 * If any of the default mapped fields are not present in the model-to-shapefile-map,
	 * they will be added.
	 *
	 * This allows newly added properties to be exported via the kvd, if these properties
	 * have corresponding entries in the default model-to-shapefile-map.
	 */
	void
	add_missing_fields_to_map(
			QMap< QString, QString> &model_to_shapefile_map,
			const GPlatesFileIO::FileInfo &file_info)
	{
		QMap <QString,QString>::const_iterator it;
		
		for (unsigned int i=0; i < ShapefileAttributes::NUM_PROPERTIES; ++i)
		{
			it = model_to_shapefile_map.find(
						ShapefileAttributes::model_properties[i]);
			if (it == model_to_shapefile_map.end())
			{
				model_to_shapefile_map.insert(
							ShapefileAttributes::model_properties[i],
							ShapefileAttributes::default_attribute_field_names[i]);
			}
		}
	}
	
	/*!
	 * Adds any of the standard shapefile attributes to the @param kvd, if they don't already exist in the kvd.
	 * The shapefile attribute names are taken from @param model_to_shapefile_map.
	 *
	 * Note that what is considered "standard" features for export may change.
	 *
	 * Default values are used for the "value" part of the kvd.
	 */
	void
	add_missing_keys_to_kvd(
			GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type kvd,
			QMap< QString, QString> &model_to_shapefile_map)
	{
		add_plate_id_key_to_kvd_if_missing(kvd,model_to_shapefile_map);
		add_begin_time_key_to_kvd_if_missing(kvd,model_to_shapefile_map);
		add_end_time_key_to_kvd_if_missing(kvd,model_to_shapefile_map);
		add_name_key_to_kvd_if_missing(kvd,model_to_shapefile_map);
		add_description_key_to_kvd_if_missing(kvd,model_to_shapefile_map);
		add_conjugate_key_to_kvd_if_missing(kvd,model_to_shapefile_map);
		add_feature_type_key_to_kvd_if_missing(kvd,model_to_shapefile_map);
		add_feature_id_key_to_kvd_if_missing(kvd,model_to_shapefile_map);
		add_reconstruction_method_key_to_kvd_if_missing(kvd,model_to_shapefile_map);
		add_left_plate_key_to_kvd_if_missing(kvd,model_to_shapefile_map);
		add_right_plate_key_to_kvd_if_missing(kvd,model_to_shapefile_map);
		add_spreading_asymmetry_key_to_kvd_if_missing(kvd,model_to_shapefile_map);
	}

	
	/*!
	 * \brief add_or_replace_model_kvd - Add @a kvd to the feature given by @a feature_handle.
	 * If a kvd with property name "shapefileAttributes" already exists, it will be removed and @a kvd
	 * will be added.
	 *
	 * \param feature_handle
	 * \param kvd
	 */
	void
	add_or_replace_model_kvd(
			const GPlatesModel::FeatureHandle &feature_handle,
			const GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type kvd)
	{

		GPlatesModel::FeatureHandle &non_const_feature_handle =
				const_cast<GPlatesModel::FeatureHandle&>(feature_handle);

		GPlatesModel::FeatureHandle::iterator p_iter = non_const_feature_handle.begin();
		GPlatesModel::FeatureHandle::iterator p_iter_end = non_const_feature_handle.end();

		for ( ; p_iter != p_iter_end ; ++p_iter)
		{
			GPlatesModel::PropertyName property_name = (*p_iter)->property_name();
			QString q_prop_name = GPlatesUtils::make_qstring_from_icu_string(property_name.get_name());
			if (q_prop_name == "shapefileAttributes")
			{
				non_const_feature_handle.remove(p_iter);
			}

		} // loop over properties in feature.

		//qDebug() << "Adding new kvd...";
		GPlatesModel::WeakReference<GPlatesModel::FeatureHandle> feature_weak_ref(non_const_feature_handle);
		feature_weak_ref->add(
					GPlatesModel::TopLevelPropertyInline::create(
						GPlatesModel::PropertyName::create_gpml("shapefileAttributes"),
						kvd));
	}


	double
	get_time_from_time_period(
			const GPlatesPropertyValues::GmlTimeInstant &time_instant)
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




	void
	fill_kvd_with_plate_id(
			GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type dictionary,
			const QMap< QString,QString > &model_to_shapefile_map,
			const GPlatesModel::FeatureHandle::const_weak_ref &feature)
	{
		static const GPlatesModel::PropertyName plate_id_property_name =
				GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");

		boost::optional<GPlatesPropertyValues::GpmlPlateId::non_null_ptr_to_const_type> recon_plate_id =
				GPlatesFeatureVisitors::get_property_value<GPlatesPropertyValues::GpmlPlateId>(
						feature, plate_id_property_name);
		if (recon_plate_id)
		{
			// The feature has a reconstruction plate ID.
			//qDebug() << "fill_kvd: found plate-id " << recon_plate_id->value();

			GPlatesPropertyValues::XsInteger::non_null_ptr_type value =
					GPlatesPropertyValues::XsInteger::create(recon_plate_id.get()->value());

			QMap <QString,QString>::const_iterator it = model_to_shapefile_map.find(
						ShapefileAttributes::model_properties[ShapefileAttributes::PLATEID]);

			if (it != model_to_shapefile_map.end())
			{
				QString key_string = it.value();

				GPlatesPropertyValues::XsString::non_null_ptr_type key =
						GPlatesPropertyValues::XsString::create(GPlatesUtils::make_icu_string_from_qstring(key_string));

				GPlatesPropertyValues::GpmlKeyValueDictionaryElement new_element(
							key,
							value,
							GPlatesPropertyValues::StructuralType::create_xsi("integer"));

				add_or_replace_kvd_element(new_element,key_string,dictionary);

			}
		}
	}
	
	void
	fill_kvd_with_conjugate_plate_id(
			GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type dictionary,
			const QMap< QString,QString > &model_to_shapefile_map,
			const GPlatesModel::FeatureHandle::const_weak_ref &feature)
	{
		static const GPlatesModel::PropertyName conjugate_plate_id_property_name =
				GPlatesModel::PropertyName::create_gpml("conjugatePlateId");

		boost::optional<GPlatesPropertyValues::GpmlPlateId::non_null_ptr_to_const_type> conjugate_plate_id =
				GPlatesFeatureVisitors::get_property_value<GPlatesPropertyValues::GpmlPlateId>(
						feature, conjugate_plate_id_property_name);
		if (conjugate_plate_id)
		{
			// The feature has a conjugate plate ID
			GPlatesPropertyValues::XsInteger::non_null_ptr_type value =
					GPlatesPropertyValues::XsInteger::create(conjugate_plate_id.get()->value());

			QMap <QString,QString>::const_iterator it = model_to_shapefile_map.find(
						ShapefileAttributes::model_properties[ShapefileAttributes::CONJUGATE_PLATE_ID]);

			if (it != model_to_shapefile_map.end())
			{
				QString key_string = it.value();

				GPlatesPropertyValues::XsString::non_null_ptr_type key =
						GPlatesPropertyValues::XsString::create(GPlatesUtils::make_icu_string_from_qstring(key_string));

				GPlatesPropertyValues::GpmlKeyValueDictionaryElement new_element(
							key,
							value,
							GPlatesPropertyValues::StructuralType::create_xsi("integer"));

				add_or_replace_kvd_element(new_element,key_string,dictionary);

			}
		}
	}

	void
	fill_kvd_with_left_plate_id(
			GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type dictionary,
			const QMap< QString,QString > &model_to_shapefile_map,
			const GPlatesModel::FeatureHandle::const_weak_ref &feature)
	{
		static const GPlatesModel::PropertyName left_plate_id_property_name =
				GPlatesModel::PropertyName::create_gpml("leftPlate");

		boost::optional<GPlatesPropertyValues::GpmlPlateId::non_null_ptr_to_const_type> left_plate_id =
				GPlatesFeatureVisitors::get_property_value<GPlatesPropertyValues::GpmlPlateId>(
						feature, left_plate_id_property_name);
		if (left_plate_id)
		{
			// The feature has a left plate ID
			GPlatesPropertyValues::XsInteger::non_null_ptr_type value =
					GPlatesPropertyValues::XsInteger::create(left_plate_id.get()->value());

			QMap <QString,QString>::const_iterator it = model_to_shapefile_map.find(
						ShapefileAttributes::model_properties[ShapefileAttributes::LEFT_PLATE]);

			if (it != model_to_shapefile_map.end())
			{
				QString key_string = it.value();

				GPlatesPropertyValues::XsString::non_null_ptr_type key =
						GPlatesPropertyValues::XsString::create(GPlatesUtils::make_icu_string_from_qstring(key_string));

				GPlatesPropertyValues::GpmlKeyValueDictionaryElement new_element(
							key,
							value,
							GPlatesPropertyValues::StructuralType::create_xsi("integer"));

				add_or_replace_kvd_element(new_element,key_string,dictionary);

			}
		}
	}
	void
	fill_kvd_with_right_plate_id(
			GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type dictionary,
			const QMap< QString,QString > &model_to_shapefile_map,
			const GPlatesModel::FeatureHandle::const_weak_ref &feature)
	{
		static const GPlatesModel::PropertyName right_plate_id_property_name =
				GPlatesModel::PropertyName::create_gpml("rightPlate");

		boost::optional<GPlatesPropertyValues::GpmlPlateId::non_null_ptr_to_const_type> right_plate_id =
				GPlatesFeatureVisitors::get_property_value<GPlatesPropertyValues::GpmlPlateId>(
						feature, right_plate_id_property_name);
		if (right_plate_id)
		{
			// The feature has a right plate ID
			GPlatesPropertyValues::XsInteger::non_null_ptr_type value =
					GPlatesPropertyValues::XsInteger::create(right_plate_id.get()->value());

			QMap <QString,QString>::const_iterator it = model_to_shapefile_map.find(
						ShapefileAttributes::model_properties[ShapefileAttributes::RIGHT_PLATE]);

			if (it != model_to_shapefile_map.end())
			{
				QString key_string = it.value();

				GPlatesPropertyValues::XsString::non_null_ptr_type key =
						GPlatesPropertyValues::XsString::create(GPlatesUtils::make_icu_string_from_qstring(key_string));

				GPlatesPropertyValues::GpmlKeyValueDictionaryElement new_element(
							key,
							value,
							GPlatesPropertyValues::StructuralType::create_xsi("integer"));

				add_or_replace_kvd_element(new_element,key_string,dictionary);

			}
		}
	}

	void
	fill_kvd_with_recon_method(
			GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type dictionary,
			const QMap< QString,QString > &model_to_shapefile_map,
			const GPlatesModel::FeatureHandle::const_weak_ref &feature)
	{

		static const GPlatesModel::PropertyName recon_method_property_name =
				GPlatesModel::PropertyName::create_gpml("reconstructionMethod");

		boost::optional<GPlatesPropertyValues::Enumeration::non_null_ptr_to_const_type> recon_method =
				GPlatesFeatureVisitors::get_property_value<GPlatesPropertyValues::Enumeration>(
						feature, recon_method_property_name);
		if (recon_method)
		{
			GPlatesModel::PropertyValue::non_null_ptr_type value = recon_method.get()->clone();

			QMap <QString,QString>::const_iterator it = model_to_shapefile_map.find(
						ShapefileAttributes::model_properties[ShapefileAttributes::RECONSTRUCTION_METHOD]);

			if (it != model_to_shapefile_map.end())
			{

				QString key_string = it.value();

				GPlatesPropertyValues::XsString::non_null_ptr_type key =
						GPlatesPropertyValues::XsString::create(GPlatesUtils::make_icu_string_from_qstring(key_string));

				GPlatesPropertyValues::GpmlKeyValueDictionaryElement new_element(
							key,
							value,
							GPlatesPropertyValues::StructuralType::create_xsi("string"));

				add_or_replace_kvd_element(new_element,key_string,dictionary);
			}
		}

	}

	void
	fill_kvd_with_spreading_asymmetry(
			GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type dictionary,
			const QMap< QString,QString > &model_to_shapefile_map,
			const GPlatesModel::FeatureHandle::const_weak_ref &feature)
	{

		static const GPlatesModel::PropertyName spreading_asymmetry_property_name =
				GPlatesModel::PropertyName::create_gpml("spreadingAsymmetry");

		boost::optional<GPlatesPropertyValues::XsDouble::non_null_ptr_to_const_type> spreading_asymmetry =
				GPlatesFeatureVisitors::get_property_value<GPlatesPropertyValues::XsDouble>(
						feature, spreading_asymmetry_property_name);
		if (spreading_asymmetry)
		{
			GPlatesModel::PropertyValue::non_null_ptr_type value = spreading_asymmetry.get()->clone();

			QMap <QString,QString>::const_iterator it = model_to_shapefile_map.find(
						ShapefileAttributes::model_properties[ShapefileAttributes::SPREADING_ASYMMETRY]);

			if (it != model_to_shapefile_map.end())
			{

				QString key_string = it.value();

				GPlatesPropertyValues::XsString::non_null_ptr_type key =
						GPlatesPropertyValues::XsString::create(GPlatesUtils::make_icu_string_from_qstring(key_string));

				GPlatesPropertyValues::GpmlKeyValueDictionaryElement new_element(
							key,
							value,
							GPlatesPropertyValues::StructuralType::create_xsi("double"));

				add_or_replace_kvd_element(new_element,key_string,dictionary);
			}
		}

	}

	void
	fill_kvd_with_feature_type(
			GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type dictionary,
			const QMap< QString,QString > &model_to_shapefile_map,
			const GPlatesModel::FeatureHandle::const_weak_ref &feature)
	{
		static const GPlatesFileIO::OgrUtils::feature_map_type &feature_map =
				GPlatesFileIO::OgrUtils::build_feature_map();

		if ( ! feature.is_valid()) {
			return;
		}

		// Export the gpgim form to GPGIM_TYPE field.
		QString gpgim_feature_type = convert_qualified_xml_name_to_qstring(
					feature->feature_type());
		GPlatesPropertyValues::XsString::non_null_ptr_type gpgim_key =
				GPlatesPropertyValues::XsString::create("GPGIM_TYPE");
		GPlatesModel::PropertyValue::non_null_ptr_type gpgim_value =
			GPlatesPropertyValues::XsString::create(GPlatesUtils::make_icu_string_from_qstring(gpgim_feature_type));

		GPlatesPropertyValues::GpmlKeyValueDictionaryElement gpgim_element(
					gpgim_key,
					gpgim_value,
					GPlatesPropertyValues::StructuralType::create_xsi("string"));

		add_or_replace_kvd_element(gpgim_element,QString("GPGIM_TYPE"),dictionary);

		// Export the two-letter form to either the mapped field name, or to TYPE if the
		// mapped field name is GPGIM_TYPE

		QString feature_type_model_qstring = GPlatesUtils::make_qstring_from_icu_string(
					feature->feature_type().get_name());

		QString two_letter_feature_type;

		if (feature_type_model_qstring == "UnclassifiedFeature")
		{
			two_letter_feature_type = "";
		}
		else
		{
			two_letter_feature_type = feature_map.key(feature_type_model_qstring);
		}

		GPlatesModel::PropertyValue::non_null_ptr_type value =
				GPlatesPropertyValues::XsString::create(
					GPlatesUtils::make_icu_string_from_qstring(two_letter_feature_type));

		QMap <QString,QString>::const_iterator it = model_to_shapefile_map.find(
					ShapefileAttributes::model_properties[ShapefileAttributes::FEATURE_TYPE]);

		if (it != model_to_shapefile_map.end())
		{
			QString key_string;
			if (GPlatesFileIO::OgrUtils::feature_type_field_is_gpgim_type(model_to_shapefile_map))
			{
					key_string = "TYPE";
			}
			else
			{
					key_string = it.value();
			}

			GPlatesPropertyValues::XsString::non_null_ptr_type key =
					GPlatesPropertyValues::XsString::create(GPlatesUtils::make_icu_string_from_qstring(key_string));

			GPlatesPropertyValues::GpmlKeyValueDictionaryElement new_element(
						key,
						value,
						GPlatesPropertyValues::StructuralType::create_xsi("string"));

			add_or_replace_kvd_element(new_element,key_string,dictionary);
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

		boost::optional<GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_to_const_type> time_period =
				GPlatesFeatureVisitors::get_property_value<GPlatesPropertyValues::GmlTimePeriod>(
						feature, valid_time_property_name);
		if (time_period)
		{

			double begin_time = get_time_from_time_period(*(time_period.get()->begin()));
			double end_time = get_time_from_time_period(*(time_period.get()->end()));

			GPlatesPropertyValues::XsDouble::non_null_ptr_type begin_value =
					GPlatesPropertyValues::XsDouble::create(begin_time);

			GPlatesPropertyValues::XsDouble::non_null_ptr_type end_value =
					GPlatesPropertyValues::XsDouble::create(end_time);

			QMap <QString,QString>::const_iterator it = model_to_shapefile_map.find(
						ShapefileAttributes::model_properties[ShapefileAttributes::BEGIN]);

			if (it != model_to_shapefile_map.end())
			{

				QString key_string = it.value();

				GPlatesPropertyValues::XsString::non_null_ptr_type key =
						GPlatesPropertyValues::XsString::create(GPlatesUtils::make_icu_string_from_qstring(key_string));

				GPlatesPropertyValues::GpmlKeyValueDictionaryElement new_element(
							key,
							begin_value,
							GPlatesPropertyValues::StructuralType::create_xsi("double"));

				add_or_replace_kvd_element(new_element,key_string,dictionary);
			}

			it = model_to_shapefile_map.find(
						ShapefileAttributes::model_properties[ShapefileAttributes::END]);

			if (it != model_to_shapefile_map.end())
			{

				QString key_string = it.value();

				GPlatesPropertyValues::XsString::non_null_ptr_type key =
						GPlatesPropertyValues::XsString::create(GPlatesUtils::make_icu_string_from_qstring(key_string));

				GPlatesPropertyValues::GpmlKeyValueDictionaryElement new_element(
							key,
							end_value,
							GPlatesPropertyValues::StructuralType::create_xsi("double"));

				add_or_replace_kvd_element(new_element,key_string,dictionary);
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

		boost::optional<GPlatesPropertyValues::XsString::non_null_ptr_to_const_type> name =
				GPlatesFeatureVisitors::get_property_value<GPlatesPropertyValues::XsString>(
						feature, name_property_name);
		if (name)
		{
			GPlatesModel::PropertyValue::non_null_ptr_type value = name.get()->clone();

			QMap <QString,QString>::const_iterator it = model_to_shapefile_map.find(
						ShapefileAttributes::model_properties[ShapefileAttributes::NAME]);

			if (it != model_to_shapefile_map.end())
			{

				QString key_string = it.value();

				GPlatesPropertyValues::XsString::non_null_ptr_type key =
						GPlatesPropertyValues::XsString::create(GPlatesUtils::make_icu_string_from_qstring(key_string));

				GPlatesPropertyValues::GpmlKeyValueDictionaryElement new_element(
							key,
							value,
							GPlatesPropertyValues::StructuralType::create_xsi("string"));

				add_or_replace_kvd_element(new_element,key_string,dictionary);
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

		boost::optional<GPlatesPropertyValues::XsString::non_null_ptr_to_const_type> description =
				GPlatesFeatureVisitors::get_property_value<GPlatesPropertyValues::XsString>(
						feature, description_property_name);
		if (description)
		{
			GPlatesModel::PropertyValue::non_null_ptr_type value = description.get()->clone();

			QMap <QString,QString>::const_iterator it = model_to_shapefile_map.find(
						ShapefileAttributes::model_properties[ShapefileAttributes::DESCRIPTION]);

			if (it != model_to_shapefile_map.end())
			{

				QString key_string = it.value();

				GPlatesPropertyValues::XsString::non_null_ptr_type key =
						GPlatesPropertyValues::XsString::create(GPlatesUtils::make_icu_string_from_qstring(key_string));

				GPlatesPropertyValues::GpmlKeyValueDictionaryElement new_element(
							key,
							value,
							GPlatesPropertyValues::StructuralType::create_xsi("string"));

				add_or_replace_kvd_element(new_element,key_string,dictionary);
			}
		}

	}
	
	void
	fill_kvd_with_feature_id(
			GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type dictionary,
			const QMap< QString,QString > &model_to_shapefile_map,
			const GPlatesModel::FeatureHandle::const_weak_ref &feature)
	{

		GPlatesModel::PropertyValue::non_null_ptr_type feature_id_value =
				GPlatesPropertyValues::XsString::create(feature->feature_id().get());

		QMap <QString,QString>::const_iterator it = model_to_shapefile_map.find(
					ShapefileAttributes::model_properties[ShapefileAttributes::FEATURE_ID]);

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
				GPlatesPropertyValues::StructuralType type = element->value_type();

				GPlatesPropertyValues::GpmlKeyValueDictionaryElement new_element(
							key,
							feature_id_value,
							type);

				*element = new_element;
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
					GPlatesPropertyValues::StructuralType::create_xsi("integer"));
		elements.push_back(plate_id_element);

		// Add a feature type entry.
		GPlatesPropertyValues::GpmlKeyValueDictionaryElement gpgim_type_element(
					GPlatesPropertyValues::XsString::create(GPlatesUtils::UnicodeString("GPGIM_TYPE")),
					GPlatesPropertyValues::XsString::create(GPlatesUtils::UnicodeString()),
					GPlatesPropertyValues::StructuralType::create_xsi("string"));
		elements.push_back(gpgim_type_element);

		QString type_key;
		if (GPlatesFileIO::OgrUtils::feature_type_field_is_gpgim_type(model_to_shapefile_map))
		{
			type_key = "TYPE";
		}
		else
		{
			it = model_to_shapefile_map.find(ShapefileAttributes::model_properties[ShapefileAttributes::FEATURE_TYPE]);

			type_key = *it;
		}

		key = GPlatesPropertyValues::XsString::create(
					GPlatesUtils::make_icu_string_from_qstring(type_key));

		GPlatesPropertyValues::XsString::non_null_ptr_type type_value =
				GPlatesPropertyValues::XsString::create(GPlatesUtils::UnicodeString());

		GPlatesPropertyValues::GpmlKeyValueDictionaryElement type_element(
					key,
					type_value,
					GPlatesPropertyValues::StructuralType::create_xsi("string"));
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
					GPlatesPropertyValues::StructuralType::create_xsi("double"));
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
					GPlatesPropertyValues::StructuralType::create_xsi("double"));
		elements.push_back(end_element);

		// Add a name entry.
		it = model_to_shapefile_map.find(ShapefileAttributes::model_properties[ShapefileAttributes::NAME]);
		key = GPlatesPropertyValues::XsString::create(
					GPlatesUtils::make_icu_string_from_qstring(*it));
		GPlatesPropertyValues::XsString::non_null_ptr_type name_value =
				GPlatesPropertyValues::XsString::create(GPlatesUtils::UnicodeString());

		GPlatesPropertyValues::GpmlKeyValueDictionaryElement name_element(
					key,
					name_value,
					GPlatesPropertyValues::StructuralType::create_xsi("string"));
		elements.push_back(name_element);

		// Add a description entry.
		it = model_to_shapefile_map.find(ShapefileAttributes::model_properties[ShapefileAttributes::DESCRIPTION]);
		key = GPlatesPropertyValues::XsString::create(
					GPlatesUtils::make_icu_string_from_qstring(*it));
		GPlatesPropertyValues::XsString::non_null_ptr_type description_value =
				GPlatesPropertyValues::XsString::create(GPlatesUtils::UnicodeString());

		GPlatesPropertyValues::GpmlKeyValueDictionaryElement description_element(
					key,
					description_value,
					GPlatesPropertyValues::StructuralType::create_xsi("string"));
		elements.push_back(description_element);
		
		
		// Add a feature-id entry.
		it = model_to_shapefile_map.find(ShapefileAttributes::model_properties[ShapefileAttributes::FEATURE_ID]);
		key = GPlatesPropertyValues::XsString::create(
					GPlatesUtils::make_icu_string_from_qstring(*it));
		GPlatesPropertyValues::XsString::non_null_ptr_type feature_id_value =
				GPlatesPropertyValues::XsString::create(GPlatesUtils::UnicodeString());

		GPlatesPropertyValues::GpmlKeyValueDictionaryElement feature_id_element(
					key,
					feature_id_value,
					GPlatesPropertyValues::StructuralType::create_xsi("string"));
		elements.push_back(feature_id_element);
		
		// Add a conjugate plate id entry.
		it = model_to_shapefile_map.find(ShapefileAttributes::model_properties[ShapefileAttributes::CONJUGATE_PLATE_ID]);
		key = GPlatesPropertyValues::XsString::create(
					GPlatesUtils::make_icu_string_from_qstring(*it));

		GPlatesPropertyValues::XsInteger::non_null_ptr_type conjugate_plate_id_value =
				GPlatesPropertyValues::XsInteger::create(0);

		GPlatesPropertyValues::GpmlKeyValueDictionaryElement conjugate_plate_id_element(
					key,
					conjugate_plate_id_value,
					GPlatesPropertyValues::StructuralType::create_xsi("integer"));
		elements.push_back(conjugate_plate_id_element);

		// Add a left plate id entry.
		it = model_to_shapefile_map.find(ShapefileAttributes::model_properties[ShapefileAttributes::LEFT_PLATE]);
		key = GPlatesPropertyValues::XsString::create(
					GPlatesUtils::make_icu_string_from_qstring(*it));

		GPlatesPropertyValues::XsInteger::non_null_ptr_type left_plate_id_value =
				GPlatesPropertyValues::XsInteger::create(0);

		GPlatesPropertyValues::GpmlKeyValueDictionaryElement left_plate_id_element(
					key,
					left_plate_id_value,
					GPlatesPropertyValues::StructuralType::create_xsi("integer"));
		elements.push_back(left_plate_id_element);

		// Add a right plate id entry.
		it = model_to_shapefile_map.find(ShapefileAttributes::model_properties[ShapefileAttributes::RIGHT_PLATE]);
		key = GPlatesPropertyValues::XsString::create(
					GPlatesUtils::make_icu_string_from_qstring(*it));

		GPlatesPropertyValues::XsInteger::non_null_ptr_type right_plate_id_value =
				GPlatesPropertyValues::XsInteger::create(0);

		GPlatesPropertyValues::GpmlKeyValueDictionaryElement right_plate_id_element(
					key,
					right_plate_id_value,
					GPlatesPropertyValues::StructuralType::create_xsi("integer"));
		elements.push_back(right_plate_id_element);

		// Add a reconstruction method entry
		it = model_to_shapefile_map.find(ShapefileAttributes::model_properties[ShapefileAttributes::RECONSTRUCTION_METHOD]);
		key = GPlatesPropertyValues::XsString::create(
					GPlatesUtils::make_icu_string_from_qstring(*it));

		GPlatesPropertyValues::XsString::non_null_ptr_type recon_method_value =
				GPlatesPropertyValues::XsString::create(GPlatesUtils::UnicodeString());

		GPlatesPropertyValues::GpmlKeyValueDictionaryElement recon_method_element(
					key,
					recon_method_value,
					GPlatesPropertyValues::StructuralType::create_xsi("string"));
		elements.push_back(recon_method_element);

		// Add a spreading asymmetry method entry
		it = model_to_shapefile_map.find(ShapefileAttributes::model_properties[ShapefileAttributes::SPREADING_ASYMMETRY]);
		key = GPlatesPropertyValues::XsString::create(
					GPlatesUtils::make_icu_string_from_qstring(*it));

		GPlatesPropertyValues::XsDouble::non_null_ptr_type spreading_asymmetry_value =
				GPlatesPropertyValues::XsDouble::create(0.);

		GPlatesPropertyValues::GpmlKeyValueDictionaryElement spreading_asymmetry_element(
					key,
					spreading_asymmetry_value,
					GPlatesPropertyValues::StructuralType::create_xsi("double"));
		elements.push_back(spreading_asymmetry_element);

		// Add them all to the default kvd.
		default_key_value_dictionary.reset(GPlatesPropertyValues::GpmlKeyValueDictionary::create(elements));
	}

	void
	fill_kvd_values_from_feature(
			GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type dictionary,
			QMap< QString,QString > &model_to_shapefile_map,
			const GPlatesModel::FeatureHandle &feature_handle)
	{
		GPlatesModel::FeatureHandle::const_weak_ref feature = feature_handle.reference();

		fill_kvd_with_feature_id(dictionary,model_to_shapefile_map,feature);
		fill_kvd_with_plate_id(dictionary,model_to_shapefile_map,feature);
		fill_kvd_with_feature_type(dictionary,model_to_shapefile_map,feature);
		fill_kvd_with_begin_and_end_time(dictionary,model_to_shapefile_map,feature);
		fill_kvd_with_name(dictionary,model_to_shapefile_map,feature);
		fill_kvd_with_description(dictionary,model_to_shapefile_map,feature);
		fill_kvd_with_conjugate_plate_id(dictionary,model_to_shapefile_map,feature);
		fill_kvd_with_recon_method(dictionary,model_to_shapefile_map,feature);
		fill_kvd_with_left_plate_id(dictionary,model_to_shapefile_map,feature);
		fill_kvd_with_right_plate_id(dictionary,model_to_shapefile_map,feature);
		fill_kvd_with_spreading_asymmetry(dictionary,model_to_shapefile_map,feature);
	}
	
	void
	create_default_model_to_shapefile_map(
			QMap< QString, QString > &model_to_shapefile_map)
	{
		for (unsigned int i = 0; i < ShapefileAttributes::NUM_PROPERTIES; ++i)
		{
			model_to_shapefile_map.insert(
						ShapefileAttributes::model_properties[i],
						ShapefileAttributes::default_attribute_field_names[i]);
		}
	}

	void
	write_point_geometries(
			GPlatesFileIO::OgrWriter *ogr_writer,
			const std::vector<GPlatesMaths::PointOnSphere> &point_geometries,
			const boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type> &key_value_dictionary)
	{
		if (point_geometries.empty())
		{
			return;
		}

		if (point_geometries.size() > 1)
		{
			// We have more than one point in the feature, so we should handle this as a multi-point.
			ogr_writer->write_multi_point_feature(
					GPlatesMaths::MultiPointOnSphere::create_on_heap(
							point_geometries.begin(),
							point_geometries.end()),
					key_value_dictionary);
		}
		else
		{
			ogr_writer->write_point_feature(point_geometries.front(), key_value_dictionary);
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
		if (polyline_geometries.empty())
		{
			return;
		}
		if (polyline_geometries.size() > 1)
		{
			ogr_writer->write_multi_polyline_feature(polyline_geometries,key_value_dictionary);
		}
		else
		{
			ogr_writer->write_polyline_feature(polyline_geometries.front(),key_value_dictionary);
		}

	}

	void
	write_polygon_geometries(
			GPlatesFileIO::OgrWriter *ogr_writer,
			const std::vector<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> &polygon_geometries,
			const boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type> &key_value_dictionary)
	{
		if (polygon_geometries.empty())
		{
			return;
		}
		if (polygon_geometries.size() > 1)
		{
			ogr_writer->write_multi_polygon_feature(polygon_geometries,key_value_dictionary);
		}
		else
		{
			ogr_writer->write_polygon_feature(polygon_geometries.front(),key_value_dictionary);
		}

	}
}

GPlatesFileIO::OgrFeatureCollectionWriter::OgrFeatureCollectionWriter(
		File::Reference &file_ref,
		const FeatureCollectionFileFormat::OGRConfiguration::shared_ptr_to_const_type &default_ogr_file_configuration)
{

	/**
	 * In this constructor we:
	 *		* create a new instance of an OgrWriter, telling it:
	 *			* the filename
	 *			* whether or not we have multiple geometry types (e.g. points AND polylines)
	 *			* whether or not we want to perform dateline wrapping.
	 *		* build a property-to-attribute-map, using one obtained from the feature collection configuration as
	 *		  a starting point, and adding any missing required fields to it.  We will use this property-to-attribute
	 *		  map in finalise_post_feature_properties when we actually write out the feature.
	 *		* build a kvd, using a kvd found in the feature collection as a starting point, and adding any missing
	 *		  required fields to it. This kvd is used as the starting point in finalise_post_feature_properties, for
	 *		  features which did not have a kvd.
	 *		  This lets us be sure that the kvd has the same form for all features, as it should be for shapefiles.
	 *		* Update the file configuration's property-to-attribute map, as it may have been modified in the above steps.
	 *
	 */

	const FileInfo &file_info = file_ref.get_file_info();
	const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection_ref =
			file_ref.get_feature_collection();

	// If there's an OGR file configuration then use it.
	boost::optional<FeatureCollectionFileFormat::OGRConfiguration::shared_ptr_type> ogr_file_configuration =
			FeatureCollectionFileFormat::copy_cast_configuration<
			FeatureCollectionFileFormat::OGRConfiguration>(
				file_ref.get_file_configuration());
	// Otherwise use the default OGR configuration.
	if (!ogr_file_configuration)
	{
		// We have to copy the default configuration since we're going to modify it.
		ogr_file_configuration =
				FeatureCollectionFileFormat::OGRConfiguration::shared_ptr_type(
					new FeatureCollectionFileFormat::OGRConfiguration(
						*default_ogr_file_configuration));
	}

	// Check what types of geometries exist in the feature collection.

	GPlatesFeatureVisitors::GeometryTypeFinder finder;

	GPlatesModel::FeatureCollectionHandle::const_iterator
			iter = feature_collection_ref->begin(),
			end = feature_collection_ref->end();

	for ( ; iter != end ; ++iter)
	{
		finder.visit_feature(iter);
	}

	// Set up an appropriate OgrWriter.
	d_ogr_writer.reset(
				new OgrWriter(
					file_info.get_qfileinfo().filePath(),
					finder.has_found_multiple_geometry_types(),
					// Should polyline/polygon geometries be wrapped/clipped to the dateline...
					ogr_file_configuration.get()->get_wrap_to_dateline()));

	// The file_info might not have a model_to_shapefile_map - the feature collection
	// might have originated from a plates file, for example. If we don't have one,
	// create a default map, using the names define in PropertyMapper.h

	d_model_to_shapefile_map =
			FeatureCollectionFileFormat::OGRConfiguration::get_model_to_attribute_map(
					*file_ref.get_feature_collection());


	if (d_model_to_shapefile_map.isEmpty())
	{
		create_default_model_to_shapefile_map(d_model_to_shapefile_map);
	}


	// New properties may have been added to features in the collection. If these properties are
	// "mappable", then we should add them to the model-to-shapefile map. Rather than checking
	// all features in the collection for the existence of any of these new properties (such a property
	// might only have been added to a single feature, for example), we can add any of the missing
	// mappable attributes to the model-to-attribute-map.
	//
	// (Note that this approach will not map *all* gplates properties to the shapefile. Such an approach
	// would, I think, require more powerful shapefile-attribute-mapping functionality, where the
	// user, on output, could specify any model properties and provide shapefile attribute names for them.
	// But hopefully catering only for the "core" properties will satisfy most use cases.).
	//
	// Alternative approaches to this might be to
	//  1. Respond to the addition of new properties to a feature at the feature-collection level; if
	// a user has added a new property to any feature in the collection, the feature collection gets to know
	// about this and can update the shapefile attributes.
	//  2. Always have a model-to-shapefile entry for all the mappable properties, even if the user has
	// not selected, at input, a mapping for a given property.

	add_missing_fields_to_map(d_model_to_shapefile_map,file_info);

	// Look for a key value dictionary, and store it as the default.
	//
	// FIXME: It might be nicer to store a single kvd definition at the collection level - such
	// as in the OgrConfiguration.  Here we are getting the kvd by grabbing the first one we come across
	// in the collection, and typically every feature in a collection would have the same kvd. (This is
	// not necessarily the case - a user can delete the kvd property from a feature. A user cannot however
	// add or remove fields from the kvd, so the form of the kvd - if it hasn't been deleted - should remain
	// the same).
	//
	// The only other place in GPlates where a kvd could be modified is here in the ogr export workflow.
	//
	// So grabbing the first (existing) kvd from a collection should give us an appropriate kvd in any case.
	OgrUtils::create_default_kvd_from_collection(feature_collection_ref,d_default_key_value_dictionary);
	

	if (d_default_key_value_dictionary)
	{
		add_missing_keys_to_kvd(*d_default_key_value_dictionary,d_model_to_shapefile_map);
	}
	else
		// We didn't find one, so make one from the model-to-attribute-map.
	{
		create_default_kvd_from_map(d_default_key_value_dictionary,
									d_model_to_shapefile_map);
	}

	// Export the newly created map as a .gplates.xml file.
	QString ogr_xml_filename = OgrUtils::make_ogr_xml_filename(file_info.get_qfileinfo());

	// FIXME: If we have multiple layers, then we will have multiple shapefiles, but only one xml mapping file.
	// We should change this so that we have a separate (and appropriately named) xml mapping file for each shapefile.
	//
	// Not exporting an individual mapping file for each layer isn't a disaster - it just means the user
	// will have to go through the mapping dialog the next time they load any of the newly created files.
	OgrUtils::save_attribute_map_as_xml_file(ogr_xml_filename,
											 d_model_to_shapefile_map);


	// Store the (potentially) modified model-to-shapefile map back to the feature collection in the
	// file reference - assign through the reference returned by 'get_model_to_attribute_map()'.
	FeatureCollectionFileFormat::OGRConfiguration::get_model_to_attribute_map(
			*file_ref.get_feature_collection()) =
					d_model_to_shapefile_map;

	// Store the file configuration in the file reference.
	FeatureCollectionFileFormat::Configuration::shared_ptr_to_const_type
			file_configuration = ogr_file_configuration.get();
	file_ref.set_file_info(file_info, file_configuration);
}


bool
GPlatesFileIO::OgrFeatureCollectionWriter::initialise_pre_feature_properties(
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
GPlatesFileIO::OgrFeatureCollectionWriter::finalise_post_feature_properties(
		const GPlatesModel::FeatureHandle &feature_handle)
{
	if (!d_key_value_dictionary)
	{
		// We haven't found a kvd, so create one based on the default.
		if (d_default_key_value_dictionary)
		{
			GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type dictionary =
					GPlatesPropertyValues::GpmlKeyValueDictionary::create(
						(*d_default_key_value_dictionary)->elements());

			// Fill the kvd. Any fields which don't have model properties will not have their
			// kvd element changed, so the default values in the default kvd will be used.
			fill_kvd_values_from_feature(dictionary,
										 d_model_to_shapefile_map,
										 feature_handle);

			// If we don't have a kvd, then we don't have any old-plates-header fields in it either.
			// So we'll add them in here.
			// This only adds in the "additional" header fields, i.e. ones that aren't already mapped
			// to the model through the attribute-mapping process.
			add_plates_header_values_to_kvd(dictionary,feature_handle);

			// Add the dictionary to the model.
			// Make the feature handle non-const so I can append to it.
			GPlatesModel::FeatureHandle &non_const_feature_handle = const_cast<GPlatesModel::FeatureHandle &>(feature_handle);

			GPlatesModel::WeakReference<GPlatesModel::FeatureHandle>  feature_weak_ref(non_const_feature_handle);
			feature_weak_ref->add(
						GPlatesModel::TopLevelPropertyInline::create(
							GPlatesModel::PropertyName::create_gpml("shapefileAttributes"),
							dictionary));

			d_key_value_dictionary.reset(dictionary);
		}
	}
	else
	{
		// We do have a shapefile kvd. The model may have changed (e.g. a user might
		// have edited the plate-id). The kvd won't have been updated yet to reflect those
		// changes, so we need to update it now. We create a new dictionary which we'll use
		// (once we've updated it) to replace the feature's kvd.
		GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type dictionary =
				GPlatesPropertyValues::GpmlKeyValueDictionary::create((*d_key_value_dictionary)->elements());

		add_missing_keys_to_kvd(dictionary,d_model_to_shapefile_map);

		fill_kvd_values_from_feature(dictionary,
									 d_model_to_shapefile_map,feature_handle);

		add_or_replace_model_kvd(feature_handle,
						  dictionary);

		d_key_value_dictionary.reset(dictionary);
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
GPlatesFileIO::OgrFeatureCollectionWriter::visit_gml_point(
		const GPlatesPropertyValues::GmlPoint &gml_point)
{
	d_point_geometries.push_back(*gml_point.point());
}

void
GPlatesFileIO::OgrFeatureCollectionWriter::visit_gml_multi_point(
		const GPlatesPropertyValues::GmlMultiPoint &gml_multi_point)
{
	d_multi_point_geometries.push_back(gml_multi_point.multipoint());
}

void
GPlatesFileIO::OgrFeatureCollectionWriter::visit_gml_line_string(
		const GPlatesPropertyValues::GmlLineString &gml_line_string)
{
	d_polyline_geometries.push_back(gml_line_string.polyline());
}

void
GPlatesFileIO::OgrFeatureCollectionWriter::visit_gml_orientable_curve(
		const GPlatesPropertyValues::GmlOrientableCurve &gml_orientable_curve)
{
	gml_orientable_curve.base_curve()->accept_visitor(*this);
}

void
GPlatesFileIO::OgrFeatureCollectionWriter::visit_gml_polygon(
		const GPlatesPropertyValues::GmlPolygon &gml_polygon)
{
	d_polygon_geometries.push_back(gml_polygon.polygon());
}

void
GPlatesFileIO::OgrFeatureCollectionWriter::visit_gpml_constant_value(
		const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	gpml_constant_value.value()->accept_visitor(*this);
}

void
GPlatesFileIO::OgrFeatureCollectionWriter::visit_gpml_key_value_dictionary(
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
GPlatesFileIO::OgrFeatureCollectionWriter::clear_accumulators()
{
	d_point_geometries.clear();
	d_multi_point_geometries.clear();
	d_polyline_geometries.clear();
	d_polygon_geometries.clear();

	d_key_value_dictionary.reset();
}
