/* $Id$ */

/**
* \file 
* File specific comments.
*
* Most recent change:
*   $Date$
* 
* Copyright (C) 2009, 2010, 2012, 2013, 2014 Geological Survey of Norway
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

#include <QMessageBox>
#include <QVariant>

#include "boost/foreach.hpp"

#include "feature-visitors/KeyValueDictionaryFinder.h"
#include "feature-visitors/ToQvariantConverter.h"
#include "model/GpgimProperty.h"
#include "model/GpgimStructuralType.h"
#include "property-values/Enumeration.h"
#include "property-values/GeoTimeInstant.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/StructuralType.h"
#include "property-values/XsInteger.h"
#include "property-values/XsDouble.h"
#include "OgrUtils.h"
#include "ShapefileXmlWriter.h"

namespace
{
    double
    get_time_from_time_instant(
        const GPlatesPropertyValues::GmlTimeInstant &time_instant)
    {

        if (time_instant.get_time_position().is_real()) {
            return time_instant.get_time_position().value();
        }
        else if (time_instant.get_time_position().is_distant_past()) {
            return 999.;
        }
        else if (time_instant.get_time_position().is_distant_future()) {
            return -999.;
        }
        else {
            return 0;
        }
    }
}

QString
GPlatesFileIO::OgrUtils::get_type_qstring_from_qvariant(
	const QVariant &variant)
{
	switch (variant.type())
	{
	case QVariant::Int:
		return QString("integer");
		break;
	case QVariant::Double:
		return QString("double");
		break;
	case QVariant::String:
		return QString("string");
		break;
	default:
		return QString();
	}
}

QString
GPlatesFileIO::OgrUtils::make_ogr_xml_filename(
		const QFileInfo &file_info)
{

	QString ogr_xml_filename = file_info.absoluteFilePath() + ".gplates.xml";

	return ogr_xml_filename;
}


void
GPlatesFileIO::OgrUtils::save_attribute_map_as_xml_file(
		const QString &filename,
		const QMap<QString,QString> &model_to_attribute_map)
{
	GPlatesFileIO::ShapefileXmlWriter writer;
	if (!writer.write_file(filename,model_to_attribute_map))
	{
		QMessageBox::warning(0,QObject::tr("ShapefileXmlWriter"),
			QObject::tr("Cannot write to file %1.")
			.arg(filename));
	};
}

const GPlatesFileIO::OgrUtils::feature_map_type &
GPlatesFileIO::OgrUtils::build_feature_map()
{

	// The data for the following map has been taken from 
	// the "build_feature_map_type" map in PlatesLineFormatReader.cc 
	
	//
	// FIXME: we should get this information from a common source, rather than having two independent sources.  
	static feature_map_type map;
	map["AR"] = "AseismicRidge";
	map["BA"] = "Bathymetry";
	map["BS"] = "Basin";
	map["CB"] = "PassiveContinentalBoundary";
	map["CF"] = "ContinentalFragment";
	map["CM"] = "PassiveConinentalBoundary";
	map["CO"] = "PassiveContinentalBoundary";
	map["CR"] = "Craton";
	map["CS"] = "Coastline";
	map["EC"] = "ExtendedContinentalCrust";
	map["FT"] = "Fault";
	map["FZ"] = "FractureZone";
	map["GR"] = "OldPlatesGridMark";
	map["GV"] = "Gravimetry";
	map["HF"] = "HeatFlow";
	map["HS"] = "HotSpot";
	map["HT"] = "HotSpotTrail";
	map["IA"] = "IslandArc";
	map["IC"] = "Isochron";
	map["IM"] = "Isochron";
	map["IP"] = "SedimentThickness";
	map["IR"] = "IslandArc";

	// -might- be Ice Shelf, might be Isochron. We don't know.
	// It appears IS covers IC and IM.
	//
	// Update 2012/9/4: Maria Seton requested "IS" result in an isochron instead of unclassified feature.
	map["IS"] = "Isochron";

	map["LI"] = "GeologicalLineation";
	map["MA"] = "Magnetics";
	map["NF"] = "gpmlFault";
	map["N1"] = "NavdatSampleMafic";
	map["N2"] = "NavdatSampleIntermediate";
	map["N3"] = "NavdatSampleFelsicLow";
	map["N4"] = "NavdatSampleFelsicHigh";
	map["OB"] = "OrogenicBelt";
	map["OP"] = "BasicRockUnit";
	map["OR"] = "OrogenicBelt";
	map["PB"] = "InferredPaleoBoundary";
	map["PA"] = "MagneticAnomalyIdentification";
	map["PC"] = "MagneticAnomalyIdentification";
	map["PL"] = "Pluton";
	map["PO"] = "PoliticalBoundary";
	map["PM"] = "MagneticAnomalyIdentification";
	map["RA"] = "IslandArc";
	map["RF"] = "Fault";
	map["RI"] = "MidOceanRidge";
	map["SM"] = "Seamount";
	map["SS"] = "Fault";
	map["SU"] = "Suture";
	map["TB"] = "TerraneBoundary";
	map["TC"] = "TransitionalCrust";
	map["TF"] = "Transform";
	map["TH"] = "Fault";
	map["TO"] = "Topography";
	map["TR"] = "SubductionZone";
	map["UN"] = "UnclassifiedFeature";
	map["VO"] = "Volcano";
	map["VP"] = "LargeIgneousProvince";
	map["XR"] = "MidOceanRidge";
	map["XT"] = "SubductionZone";

	return map;
}

void
GPlatesFileIO::OgrUtils::create_default_kvd_from_collection(
	const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection,
	boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type> &default_key_value_dictionary)
{
	if (feature_collection.is_valid())
	{
		GPlatesModel::FeatureCollectionHandle::const_iterator
			iter = feature_collection->begin(), 
			end = feature_collection->end();

		while ((iter != end) && !default_key_value_dictionary)
		{
			// FIXME: Replace this kvd-finder with the new PropertyValueFinder.
			GPlatesFeatureVisitors::KeyValueDictionaryFinder finder;
			finder.visit_feature(iter);
			if (finder.number_of_found_dictionaries() != 0)
			{
				GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type found_kvd =
					*(finder.found_key_value_dictionaries_begin());
				default_key_value_dictionary = found_kvd->clone();
			}

			++iter;
		}

	}		
}

void
GPlatesFileIO::OgrUtils::add_plate_id_to_kvd(
	const GPlatesModel::FeatureHandle::const_weak_ref &feature,
	GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type kvd)
{
	GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement> &
			elements = kvd->elements();

	static const GPlatesModel::PropertyName plate_id_property_name =
		GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");

	// Shapefile attribute field names are limited to 10 characters in length
	// and should not contain spaces.
	GPlatesPropertyValues::XsString::non_null_ptr_type key =
		GPlatesPropertyValues::XsString::create("PLATEID1");

	// Set up a default plate-id value, which we'll use if the feature doesn't have a plate-id.
	GPlatesModel::integer_plate_id_type plate_id_value = 0;

	// If we found a plate id, add it. 
	boost::optional<GPlatesPropertyValues::GpmlPlateId::non_null_ptr_to_const_type> recon_plate_id =
			GPlatesFeatureVisitors::get_property_value<GPlatesPropertyValues::GpmlPlateId>(
					feature, plate_id_property_name);
	if (recon_plate_id)
    {
		plate_id_value = recon_plate_id.get()->get_value();
    }

	GPlatesPropertyValues::XsInteger::non_null_ptr_type value =
		GPlatesPropertyValues::XsInteger::create(plate_id_value);

	GPlatesPropertyValues::GpmlKeyValueDictionaryElement::non_null_ptr_type element =
			GPlatesPropertyValues::GpmlKeyValueDictionaryElement::create(
					key,
					value,
					GPlatesPropertyValues::StructuralType::create_xsi("integer"));
	elements.push_back(element);
}

void
GPlatesFileIO::OgrUtils::add_reconstruction_fields_to_kvd(
	GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type kvd,
	const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
	const double &reconstruction_time)
{
	GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement> &
			elements = kvd->elements();

	// There should always be an anchor plate and a reconstruction time, so
	// default values are not appropriate here.

	// Anchor plate.

	// (Shapefile attribute fields are limited to 10 characters in length)
	GPlatesPropertyValues::XsString::non_null_ptr_type key = 
		GPlatesPropertyValues::XsString::create("ANCHOR");
	GPlatesPropertyValues::XsInteger::non_null_ptr_type anchor_value = 
		GPlatesPropertyValues::XsInteger::create(reconstruction_anchor_plate_id);	

	GPlatesPropertyValues::GpmlKeyValueDictionaryElement::non_null_ptr_type anchor_element =
			GPlatesPropertyValues::GpmlKeyValueDictionaryElement::create(
					key,
					anchor_value,
					GPlatesPropertyValues::StructuralType::create_xsi("integer"));
	elements.push_back(anchor_element);	

	// Reconstruction time.
	key = GPlatesPropertyValues::XsString::create("TIME");
	GPlatesPropertyValues::XsDouble::non_null_ptr_type time_value = 
		GPlatesPropertyValues::XsDouble::create(reconstruction_time);	

	GPlatesPropertyValues::GpmlKeyValueDictionaryElement::non_null_ptr_type time_element =
			GPlatesPropertyValues::GpmlKeyValueDictionaryElement::create(
					key,
					time_value,
					GPlatesPropertyValues::StructuralType::create_xsi("double"));
	elements.push_back(time_element);
}

void
GPlatesFileIO::OgrUtils::add_referenced_files_to_kvd(
	GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type kvd,
	const referenced_files_collection_type &referenced_files)
{
	GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement> &
			elements = kvd->elements();

	// Referenced files. 

	// Attribute field names will have the form "FILE1", "FILE2" etc...
	QString file_string("FILE");

	int file_count = 1;
	referenced_files_collection_type::const_iterator file_iter;
	for (file_iter = referenced_files.begin();
		file_iter != referenced_files.end();
		++file_iter, ++file_count)
	{
		const GPlatesFileIO::File::Reference *file = *file_iter;

		QString count_string = QString("%1").arg(file_count);
		QString field_name = file_string + count_string;

		// Some files might not actually exist yet if the user created a new
		// feature collection internally and hasn't saved it to file yet.
		if (!GPlatesFileIO::file_exists(file->get_file_info()))
		{
			continue;
		}

		QString filename = file->get_file_info().get_display_name(false/*use_absolute_path_name*/);

		GPlatesPropertyValues::XsString::non_null_ptr_type key = 
			GPlatesPropertyValues::XsString::create(GPlatesUtils::make_icu_string_from_qstring(field_name));
		GPlatesPropertyValues::XsString::non_null_ptr_type file_value = 
			GPlatesPropertyValues::XsString::create(GPlatesUtils::make_icu_string_from_qstring(filename));

		GPlatesPropertyValues::GpmlKeyValueDictionaryElement::non_null_ptr_type element =
				GPlatesPropertyValues::GpmlKeyValueDictionaryElement::create(
						key,
						file_value,
						GPlatesPropertyValues::StructuralType::create_xsi("string"));
		elements.push_back(element);
	}

}

void
GPlatesFileIO::OgrUtils::add_reconstruction_files_to_kvd(
		GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type kvd,
		const GPlatesFileIO::OgrUtils::referenced_files_collection_type &reconstruction_files)
{
	GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement> &
			elements = kvd->elements();

	// Attribute field names will have the form "RECONFILE1", "RECONFILE2" etc...
	QString file_string("RECONFILE");

	int file_count = 1;
	referenced_files_collection_type::const_iterator file_iter;
	for (file_iter = reconstruction_files.begin();
		file_iter != reconstruction_files.end();
		++file_iter, ++file_count)
	{
		const GPlatesFileIO::File::Reference *file = *file_iter;

		QString count_string = QString("%1").arg(file_count);
		QString field_name = file_string + count_string;

		// Some files might not actually exist yet if the user created a new
		// feature collection internally and hasn't saved it to file yet.
		if (!GPlatesFileIO::file_exists(file->get_file_info()))
		{
			continue;
		}

		QString filename = file->get_file_info().get_display_name(false/*use_absolute_path_name*/);

		GPlatesPropertyValues::XsString::non_null_ptr_type key =
			GPlatesPropertyValues::XsString::create(GPlatesUtils::make_icu_string_from_qstring(field_name));
		GPlatesPropertyValues::XsString::non_null_ptr_type file_value =
			GPlatesPropertyValues::XsString::create(GPlatesUtils::make_icu_string_from_qstring(filename));

		GPlatesPropertyValues::GpmlKeyValueDictionaryElement::non_null_ptr_type element =
				GPlatesPropertyValues::GpmlKeyValueDictionaryElement::create(
						key,
						file_value,
						GPlatesPropertyValues::StructuralType::create_xsi("string"));
		elements.push_back(element);
	}
}


void
GPlatesFileIO::OgrUtils::add_standard_properties_to_kvd(
	const GPlatesModel::FeatureHandle::const_weak_ref &feature,
	GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type kvd)
{
    //FIXME: in each of the functions below, take the string used for the field name
    // from PropertyMapper.h
    add_plate_id_to_kvd(feature,kvd);
    add_feature_type_to_kvd(feature,kvd);
    add_begin_and_end_time_to_kvd(feature,kvd);
    add_name_to_kvd(feature,kvd);
    add_description_to_kvd(feature,kvd);
    add_feature_id_to_kvd(feature,kvd);
    add_conjugate_plate_id_to_kvd(feature,kvd);
	add_reconstruction_method_to_kvd(feature,kvd);
	add_left_plate_to_kvd(feature,kvd);
	add_right_plate_to_kvd(feature,kvd);
	add_spreading_asymmetry_to_kvd(feature,kvd);
	add_geometry_import_time_to_kvd(feature,kvd);
}

void
GPlatesFileIO::OgrUtils::add_feature_type_to_kvd(
	const GPlatesModel::FeatureHandle::const_weak_ref &feature,
	GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type kvd)
{
	// We'll now export both 2-letter and gpgim-style feature types.
	// The feature should always have a feature-type, even if it's
	// just "UnclassifiedFeature".

    static const GPlatesFileIO::OgrUtils::feature_map_type &feature_map =
        GPlatesFileIO::OgrUtils::build_feature_map();

    if ( ! feature.is_valid()) {
        return;
    }

	// Export 2-letter code type.
    QString feature_type_model_qstring = GPlatesUtils::make_qstring_from_icu_string(
        feature->feature_type().get_name());


    QString feature_type_key;
    if (feature_type_model_qstring == "UnclassifiedFeature")
    {
        feature_type_key = "";
    }
    else
    {
        feature_type_key = feature_map.key(feature_type_model_qstring);
    }


	GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement> &
			elements = kvd->elements();

    GPlatesModel::PropertyValue::non_null_ptr_type value =
        GPlatesPropertyValues::XsString::create(
            GPlatesUtils::make_icu_string_from_qstring(feature_type_key));


    GPlatesPropertyValues::XsString::non_null_ptr_type key =
            GPlatesPropertyValues::XsString::create("TYPE");

	GPlatesPropertyValues::GpmlKeyValueDictionaryElement::non_null_ptr_type two_letter_element =
			GPlatesPropertyValues::GpmlKeyValueDictionaryElement::create(
					key,
					value,
					GPlatesPropertyValues::StructuralType::create_xsi("string"));

	elements.push_back(two_letter_element);

	// Export the gpgim form to GPGIM_TYPE field.
	QString gpgim_feature_type = convert_qualified_xml_name_to_qstring(
				feature->feature_type());
	GPlatesPropertyValues::XsString::non_null_ptr_type gpgim_key =
			GPlatesPropertyValues::XsString::create("GPGIM_TYPE");
	GPlatesModel::PropertyValue::non_null_ptr_type gpgim_value =
		GPlatesPropertyValues::XsString::create(GPlatesUtils::make_icu_string_from_qstring(gpgim_feature_type));

	GPlatesPropertyValues::GpmlKeyValueDictionaryElement::non_null_ptr_type gpgim_element =
			GPlatesPropertyValues::GpmlKeyValueDictionaryElement::create(
					gpgim_key,
					gpgim_value,
					GPlatesPropertyValues::StructuralType::create_xsi("string"));

	elements.push_back(gpgim_element);
}

void
GPlatesFileIO::OgrUtils::add_begin_and_end_time_to_kvd(
	const GPlatesModel::FeatureHandle::const_weak_ref &feature,
	GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type kvd)
{
	GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement> &
			elements = kvd->elements();

    static const GPlatesModel::PropertyName valid_time_property_name =
        GPlatesModel::PropertyName::create_gml("validTime");

	// Set up default begin and end times in case we don't find any in the feature.
	double begin_time = 999.;

	double end_time = -999.;

	boost::optional<GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_to_const_type> time_period =
			GPlatesFeatureVisitors::get_property_value<GPlatesPropertyValues::GmlTimePeriod>(
					feature, valid_time_property_name);
    if (time_period)
    {
		begin_time = get_time_from_time_instant(*(time_period.get()->begin()));
		end_time = get_time_from_time_instant(*(time_period.get()->end()));
    }

	GPlatesPropertyValues::XsDouble::non_null_ptr_type begin_value =
		GPlatesPropertyValues::XsDouble::create(begin_time);

	GPlatesPropertyValues::XsDouble::non_null_ptr_type end_value =
		GPlatesPropertyValues::XsDouble::create(end_time);

	GPlatesPropertyValues::XsString::non_null_ptr_type begin_key =
			GPlatesPropertyValues::XsString::create("FROMAGE");

	GPlatesPropertyValues::GpmlKeyValueDictionaryElement::non_null_ptr_type begin_element =
			GPlatesPropertyValues::GpmlKeyValueDictionaryElement::create(
					begin_key,
					begin_value,
					GPlatesPropertyValues::StructuralType::create_xsi("double"));

	elements.push_back(begin_element);

	GPlatesPropertyValues::XsString::non_null_ptr_type end_key =
			GPlatesPropertyValues::XsString::create("TOAGE");

	GPlatesPropertyValues::GpmlKeyValueDictionaryElement::non_null_ptr_type end_element =
			GPlatesPropertyValues::GpmlKeyValueDictionaryElement::create(
					end_key,
					end_value,
					GPlatesPropertyValues::StructuralType::create_xsi("double"));

	elements.push_back(end_element);
}

void
GPlatesFileIO::OgrUtils::add_name_to_kvd(
	const GPlatesModel::FeatureHandle::const_weak_ref &feature,
	GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type kvd)
{
	GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement> &
			elements = kvd->elements();

    static const GPlatesModel::PropertyName name_property_name =
        GPlatesModel::PropertyName::create_gml("name");

	// Default string
	GPlatesPropertyValues::XsString::non_null_ptr_type value =
			GPlatesPropertyValues::XsString::create("");

	boost::optional<GPlatesPropertyValues::XsString::non_null_ptr_to_const_type> name =
			GPlatesFeatureVisitors::get_property_value<GPlatesPropertyValues::XsString>(
					feature, name_property_name);
    if (name)
    {
		value = name.get()->clone();
    }

	GPlatesPropertyValues::XsString::non_null_ptr_type key =
			GPlatesPropertyValues::XsString::create("NAME");

	GPlatesPropertyValues::GpmlKeyValueDictionaryElement::non_null_ptr_type element =
			GPlatesPropertyValues::GpmlKeyValueDictionaryElement::create(
					key,
					value,
					GPlatesPropertyValues::StructuralType::create_xsi("string"));

	elements.push_back(element);
}

void
GPlatesFileIO::OgrUtils::add_description_to_kvd(
	const GPlatesModel::FeatureHandle::const_weak_ref &feature,
	GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type kvd)
{
	GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement> &
			elements = kvd->elements();

	static const GPlatesModel::PropertyName desc_property_name =
		GPlatesModel::PropertyName::create_gml("description");

	// Default string
	GPlatesPropertyValues::XsString::non_null_ptr_type value =
			GPlatesPropertyValues::XsString::create("");

	boost::optional<GPlatesPropertyValues::XsString::non_null_ptr_to_const_type> description =
			GPlatesFeatureVisitors::get_property_value<GPlatesPropertyValues::XsString>(
					feature, desc_property_name);
	if (description)
	{
		value = description.get()->clone();
	}

	GPlatesPropertyValues::XsString::non_null_ptr_type key =
			GPlatesPropertyValues::XsString::create("DESCR");

	GPlatesPropertyValues::GpmlKeyValueDictionaryElement::non_null_ptr_type element =
			GPlatesPropertyValues::GpmlKeyValueDictionaryElement::create(
					key,
					value,
					GPlatesPropertyValues::StructuralType::create_xsi("string"));

	elements.push_back(element);
}

void
GPlatesFileIO::OgrUtils::add_feature_id_to_kvd(
	const GPlatesModel::FeatureHandle::const_weak_ref &feature,
	GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type kvd)
{
	GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement> &
			elements = kvd->elements();

	// There should always be a feature-id,so a default value is not appropriate here.

    GPlatesModel::PropertyValue::non_null_ptr_type feature_id_value =
        GPlatesPropertyValues::XsString::create(feature->feature_id().get());

    GPlatesPropertyValues::XsString::non_null_ptr_type feature_id_key =
            GPlatesPropertyValues::XsString::create("FEATURE_ID");

	GPlatesPropertyValues::GpmlKeyValueDictionaryElement::non_null_ptr_type element =
			GPlatesPropertyValues::GpmlKeyValueDictionaryElement::create(
					feature_id_key,
					feature_id_value,
					GPlatesPropertyValues::StructuralType::create_xsi("string"));

    elements.push_back(element);

}

void
GPlatesFileIO::OgrUtils::add_conjugate_plate_id_to_kvd(
	const GPlatesModel::FeatureHandle::const_weak_ref &feature,
	GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type kvd)
{
	GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement> &
			elements = kvd->elements();

	static const GPlatesModel::PropertyName property_name =
		GPlatesModel::PropertyName::create_gpml("conjugatePlateId");

	GPlatesPropertyValues::XsString::non_null_ptr_type key =
		GPlatesPropertyValues::XsString::create("PLATEID2");

	// Set up a default plate-id value, which we'll use if the feature doesn't have a plate-id.
	GPlatesModel::integer_plate_id_type plate_id_value = 0;

	// If we found a plate id, add it.
	boost::optional<GPlatesPropertyValues::GpmlPlateId::non_null_ptr_to_const_type> plate_id =
			GPlatesFeatureVisitors::get_property_value<GPlatesPropertyValues::GpmlPlateId>(
					feature, property_name);
	if (plate_id)
	{
		plate_id_value = plate_id.get()->get_value();
	}

	GPlatesPropertyValues::XsInteger::non_null_ptr_type value =
		GPlatesPropertyValues::XsInteger::create(plate_id_value);

	GPlatesPropertyValues::GpmlKeyValueDictionaryElement::non_null_ptr_type element =
			GPlatesPropertyValues::GpmlKeyValueDictionaryElement::create(
					key,
					value,
					GPlatesPropertyValues::StructuralType::create_xsi("integer"));
	elements.push_back(element);
}



void
GPlatesFileIO::OgrUtils::add_left_plate_to_kvd(
	const GPlatesModel::FeatureHandle::const_weak_ref &feature,
	GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type kvd)
{
	GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement> &
			elements = kvd->elements();

	static const GPlatesModel::PropertyName property_name =
		GPlatesModel::PropertyName::create_gpml("leftPlate");

	GPlatesPropertyValues::XsString::non_null_ptr_type key =
		GPlatesPropertyValues::XsString::create("L_PLATE");

	// Set up a default plate-id value, which we'll use if the feature doesn't have a plate-id.
	GPlatesModel::integer_plate_id_type plate_id_value = 0;

	// If we found a plate id, add it.
	boost::optional<GPlatesPropertyValues::GpmlPlateId::non_null_ptr_to_const_type> plate_id =
			GPlatesFeatureVisitors::get_property_value<GPlatesPropertyValues::GpmlPlateId>(
					feature, property_name);
	if (plate_id)
	{
		plate_id_value = plate_id.get()->get_value();
	}

	GPlatesPropertyValues::XsInteger::non_null_ptr_type value =
		GPlatesPropertyValues::XsInteger::create(plate_id_value);

	GPlatesPropertyValues::GpmlKeyValueDictionaryElement::non_null_ptr_type element =
			GPlatesPropertyValues::GpmlKeyValueDictionaryElement::create(
					key,
					value,
					GPlatesPropertyValues::StructuralType::create_xsi("integer"));
	elements.push_back(element);
}

void
GPlatesFileIO::OgrUtils::add_right_plate_to_kvd(
		const GPlatesModel::FeatureHandle::const_weak_ref &feature,
		GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type kvd)
{
	GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement> &
			elements = kvd->elements();

	static const GPlatesModel::PropertyName property_name =
		GPlatesModel::PropertyName::create_gpml("rightPlate");

	GPlatesPropertyValues::XsString::non_null_ptr_type key =
		GPlatesPropertyValues::XsString::create("R_PLATE");

	// Set up a default plate-id value, which we'll use if the feature doesn't have a plate-id.
	GPlatesModel::integer_plate_id_type plate_id_value = 0;

	// If we found a plate id, add it.
	boost::optional<GPlatesPropertyValues::GpmlPlateId::non_null_ptr_to_const_type> plate_id =
			GPlatesFeatureVisitors::get_property_value<GPlatesPropertyValues::GpmlPlateId>(
					feature, property_name);
	if (plate_id)
	{
		plate_id_value = plate_id.get()->get_value();
	}

	GPlatesPropertyValues::XsInteger::non_null_ptr_type value =
		GPlatesPropertyValues::XsInteger::create(plate_id_value);

	GPlatesPropertyValues::GpmlKeyValueDictionaryElement::non_null_ptr_type element =
			GPlatesPropertyValues::GpmlKeyValueDictionaryElement::create(
					key,
					value,
					GPlatesPropertyValues::StructuralType::create_xsi("integer"));
	elements.push_back(element);
}


void
GPlatesFileIO::OgrUtils::add_reconstruction_method_to_kvd(
	const GPlatesModel::FeatureHandle::const_weak_ref &feature,
	GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type kvd)
{
	GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement> &
			elements = kvd->elements();

	static const GPlatesModel::PropertyName reconstruction_method_property_name =
		GPlatesModel::PropertyName::create_gpml("reconstructionMethod");

	// Default string.
	GPlatesPropertyValues::XsString::non_null_ptr_type value = GPlatesPropertyValues::XsString::create("");

	// If we found a reconstruction method, add it.
	boost::optional<GPlatesPropertyValues::Enumeration::non_null_ptr_to_const_type> reconstruction_method =
			GPlatesFeatureVisitors::get_property_value<GPlatesPropertyValues::Enumeration>(
					feature, reconstruction_method_property_name);
	if (reconstruction_method)
	{
		value = GPlatesPropertyValues::XsString::create(reconstruction_method.get()->get_value().get());
	}
	GPlatesPropertyValues::XsString::non_null_ptr_type key =
		GPlatesPropertyValues::XsString::create("RECON_METH");

	GPlatesPropertyValues::GpmlKeyValueDictionaryElement::non_null_ptr_type element =
			GPlatesPropertyValues::GpmlKeyValueDictionaryElement::create(
					key,
					value,
					GPlatesPropertyValues::StructuralType::create_xsi("string"));
	elements.push_back(element);
}

void
GPlatesFileIO::OgrUtils::add_spreading_asymmetry_to_kvd(
		const GPlatesModel::FeatureHandle::const_weak_ref &feature,
		GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type kvd)
{
	GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement> &
			elements = kvd->elements();

	static const GPlatesModel::PropertyName spreading_asymmetry_property_name =
			GPlatesModel::PropertyName::create_gpml("spreadingAsymmetry");

	// Default string.
	GPlatesPropertyValues::XsDouble::non_null_ptr_type value = GPlatesPropertyValues::XsDouble::create(0.);

	// If we found a spreading asymmetry, add it.
	boost::optional<GPlatesPropertyValues::XsDouble::non_null_ptr_to_const_type> spreading_asymmetry =
			GPlatesFeatureVisitors::get_property_value<GPlatesPropertyValues::XsDouble>(
					feature, spreading_asymmetry_property_name);
	if (spreading_asymmetry)
	{
		value = GPlatesPropertyValues::XsDouble::create(spreading_asymmetry.get()->get_value());
	}
	GPlatesPropertyValues::XsString::non_null_ptr_type key =
			GPlatesPropertyValues::XsString::create("SPREAD_ASY");

	GPlatesPropertyValues::GpmlKeyValueDictionaryElement::non_null_ptr_type element =
			GPlatesPropertyValues::GpmlKeyValueDictionaryElement::create(
					key,
					value,
					GPlatesPropertyValues::StructuralType::create_xsi("double"));
	elements.push_back(element);
}

void
GPlatesFileIO::OgrUtils::add_geometry_import_time_to_kvd(
		const GPlatesModel::FeatureHandle::const_weak_ref &feature,
		GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type kvd)
{
	GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement> &
			elements = kvd->elements();

	static const GPlatesModel::PropertyName geometry_import_time_property_name =
			GPlatesModel::PropertyName::create_gpml("geometryImportTime");

	// Default value.
	double geometry_import_time = 0.0;

	boost::optional<GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_to_const_type> time_instant =
			GPlatesFeatureVisitors::get_property_value<GPlatesPropertyValues::GmlTimeInstant>(
					feature, geometry_import_time_property_name);
    if (time_instant)
    {
		geometry_import_time = get_time_from_time_instant(*time_instant.get());
    }

	GPlatesPropertyValues::XsDouble::non_null_ptr_type value =
			GPlatesPropertyValues::XsDouble::create(geometry_import_time);

	GPlatesPropertyValues::XsString::non_null_ptr_type key =
			GPlatesPropertyValues::XsString::create("IMPORT_AGE");

	GPlatesPropertyValues::GpmlKeyValueDictionaryElement::non_null_ptr_type element =
			GPlatesPropertyValues::GpmlKeyValueDictionaryElement::create(
					key,
					value,
					GPlatesPropertyValues::StructuralType::create_xsi("double"));

	elements.push_back(element);
}

bool
GPlatesFileIO::OgrUtils::feature_type_field_is_gpgim_type(
		const model_to_attribute_map_type &model_to_attribute_map)
{
	model_to_attribute_map_type::ConstIterator it =
			model_to_attribute_map.find(
				ShapefileAttributes::model_properties[
					ShapefileAttributes::FEATURE_TYPE]);
	if (it == model_to_attribute_map.end())
	{
		// We don't have an entry for feature_type, so we can return false.
		return false;
	}
	if (it.value() == "GPGIM_TYPE")
	{
		// We do have an entry for feature_type, and it's GPGIM_TYPE
		return true;
	}
	else
	{
		return false;
	}
}

QVariant
GPlatesFileIO::OgrUtils::get_qvariant_from_kvd_element(
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

/**
 * Write kvd to debug output
 */
void
GPlatesFileIO::OgrUtils::write_kvd(
		GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type kvd)
{
	const GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement> &
			elements = kvd->elements();
	GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement>::const_iterator 
			iter = elements.begin(),
			end = elements.end();

	for (; iter != end; ++iter)
	{
		qDebug() << "Key: " <<
					GPlatesUtils::make_qstring_from_icu_string(iter->get()->key()->get_value().get()) <<
					", Value: " <<
					get_qvariant_from_kvd_element(*iter->get());
	}
}

/**
 * Write kvd to debug output
 */
void
GPlatesFileIO::OgrUtils::write_kvd(
		GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type kvd)
{
	const GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement> &
			elements = kvd->elements();
	GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement>::const_iterator 
			iter = elements.begin(),
			end = elements.end();

	for (; iter != end; ++iter)
	{
		qDebug() << "Key: " <<
					GPlatesUtils::make_qstring_from_icu_string(iter->get()->key()->get_value().get()) <<
					", Value: " <<
					get_qvariant_from_kvd_element(*iter->get());
	}
}


bool
GPlatesFileIO::OgrUtils::wkb_type_belongs_to_structural_types(
		const OGRwkbGeometryType &wkb_type,
		const GPlatesModel::GpgimProperty::structural_type_seq_type &structural_types)
{
	static const GPlatesPropertyValues::StructuralType point_type = GPlatesPropertyValues::StructuralType::create_gml("Point");
	static const GPlatesPropertyValues::StructuralType multi_point_type = GPlatesPropertyValues::StructuralType::create_gml("MultiPoint");
	static const GPlatesPropertyValues::StructuralType polyline_type = GPlatesPropertyValues::StructuralType::create_gml("LineString");
	static const GPlatesPropertyValues::StructuralType polygon_type = GPlatesPropertyValues::StructuralType::create_gml("Polygon");

	GPlatesPropertyValues::StructuralType wkb_structural_type = point_type;

	switch(wkb_type)
	{
	case wkbPoint:
		wkb_structural_type = point_type;
		break;
	case wkbMultiPoint:
		wkb_structural_type = multi_point_type;
		break;
	case wkbLineString:
		wkb_structural_type = polyline_type;
		break;
	case wkbMultiLineString:
		wkb_structural_type = polyline_type;
		break;
	case wkbPolygon:
		wkb_structural_type = polygon_type;
		break;
	case wkbMultiPolygon:
		wkb_structural_type = polygon_type;
		break;
	default: ;
	}

	BOOST_FOREACH(GPlatesModel::GpgimStructuralType::non_null_ptr_to_const_type type, structural_types)
	{
		if (type->get_structural_type() == wkb_structural_type)
		{
			return true;
		}
	}
	return false;
}


boost::optional<GPlatesPropertyValues::StructuralType>
GPlatesFileIO::OgrUtils::get_structural_type_of_wkb_type(
		const OGRwkbGeometryType &wkb_type)
{
	switch(wkb_type)
	{
	case wkbPoint:
		return GPlatesPropertyValues::StructuralType::create_gml("Point");
		break;
	case wkbMultiPoint:
		return GPlatesPropertyValues::StructuralType::create_gml("MultiPoint");
		break;
	case wkbLineString:
		return GPlatesPropertyValues::StructuralType::create_gml("LineString");
		break;
	case wkbMultiLineString:
		return GPlatesPropertyValues::StructuralType::create_gml("LineString");
		break;
	case wkbPolygon:
		return GPlatesPropertyValues::StructuralType::create_gml("Polygon");
		break;
	case wkbMultiPolygon:
		return GPlatesPropertyValues::StructuralType::create_gml("Polygon");
		break;
	default:
		return boost::none;
	}
}




void
GPlatesFileIO::OgrUtils::add_filename_sequence_to_kvd(
		const QString &root_attribute_name,
		const GPlatesFileIO::OgrUtils::referenced_files_collection_type &files,
		GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type &dictionary)
{
	using namespace GPlatesPropertyValues;

	GPlatesModel::RevisionedVector<GpmlKeyValueDictionaryElement> &elements = dictionary->elements();

	int file_count = 1;
	referenced_files_collection_type::const_iterator file_iter;
	for (file_iter = files.begin();
		file_iter != files.end();
		++file_iter, ++file_count)
	{
		const GPlatesFileIO::File::Reference *file = *file_iter;

		QString count_string = QString("%1").arg(file_count);
		QString field_name = root_attribute_name + count_string;

		// Some files might not actually exist yet if the user created a new
		// feature collection internally and hasn't saved it to file yet.
		if (!GPlatesFileIO::file_exists(file->get_file_info()))
		{
			continue;
		}

		QString filename = file->get_file_info().get_display_name(false/*use_absolute_path_name*/);

		XsString::non_null_ptr_type key = XsString::create(GPlatesUtils::make_icu_string_from_qstring(field_name));
		XsString::non_null_ptr_type file_value =
			XsString::create(GPlatesUtils::make_icu_string_from_qstring(filename));

		GpmlKeyValueDictionaryElement::non_null_ptr_type element =
				GpmlKeyValueDictionaryElement::create(
						key,
						file_value,
						StructuralType::create_xsi("string"));
		elements.push_back(element);
	}
}
