/* $Id$ */

/**
* \file 
* File specific comments.
*
* Most recent change:
*   $Date$
* 
* Copyright (C) 2009, 2010, 2012 Geological Survey of Norway
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

#include "feature-visitors/KeyValueDictionaryFinder.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/XsInteger.h"
#include "property-values/XsDouble.h"
#include "OgrUtils.h"
#include "ShapefileXmlWriter.h"

namespace
{
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
				default_key_value_dictionary = GPlatesPropertyValues::GpmlKeyValueDictionary::create(
					found_kvd->elements());
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
	static const GPlatesModel::PropertyName plate_id_property_name =
		GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");

	const GPlatesPropertyValues::GpmlPlateId *recon_plate_id;


	// If we found a plate id, add it. 
	if (GPlatesFeatureVisitors::get_property_value(feature,plate_id_property_name,recon_plate_id))
    {
        // Shapefile attribute field names are limited to 10 characters in length
        // and should not contain spaces.
        GPlatesPropertyValues::XsString::non_null_ptr_type key =
            GPlatesPropertyValues::XsString::create("PLATE_ID");
        GPlatesPropertyValues::XsInteger::non_null_ptr_type plateid_value =
            GPlatesPropertyValues::XsInteger::create(recon_plate_id->value());

        GPlatesPropertyValues::GpmlKeyValueDictionaryElement element(
            key,
            plateid_value,
            GPlatesPropertyValues::StructuralType::create_xsi("integer"));
        kvd->elements().push_back(element);
    }
}

void
GPlatesFileIO::OgrUtils::add_reconstruction_fields_to_kvd(
	GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type kvd,
	const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
	const double &reconstruction_time)
{


	// Anchor plate.

	// (Shapefile attribute fields are limited to 10 characters in length)
	GPlatesPropertyValues::XsString::non_null_ptr_type key = 
		GPlatesPropertyValues::XsString::create("ANCHOR");
	GPlatesPropertyValues::XsInteger::non_null_ptr_type anchor_value = 
		GPlatesPropertyValues::XsInteger::create(reconstruction_anchor_plate_id);	

	GPlatesPropertyValues::GpmlKeyValueDictionaryElement anchor_element(
		key,
		anchor_value,
		GPlatesPropertyValues::StructuralType::create_xsi("integer"));
	kvd->elements().push_back(anchor_element);	

	// Reconstruction time.
	key = GPlatesPropertyValues::XsString::create("TIME");
	GPlatesPropertyValues::XsDouble::non_null_ptr_type time_value = 
		GPlatesPropertyValues::XsDouble::create(reconstruction_time);	

	GPlatesPropertyValues::GpmlKeyValueDictionaryElement time_element(
		key,
		time_value,
		GPlatesPropertyValues::StructuralType::create_xsi("double"));
	kvd->elements().push_back(time_element);	
}

void
GPlatesFileIO::OgrUtils::add_referenced_files_to_kvd(
	GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type kvd,
	const referenced_files_collection_type &referenced_files)
{
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

		GPlatesPropertyValues::GpmlKeyValueDictionaryElement element(
			key,
			file_value,
			GPlatesPropertyValues::StructuralType::create_xsi("string"));
		kvd->elements().push_back(element);	
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
}

void
GPlatesFileIO::OgrUtils::add_feature_type_to_kvd(
	const GPlatesModel::FeatureHandle::const_weak_ref &feature,
	GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type kvd)
{
    static const GPlatesFileIO::OgrUtils::feature_map_type &feature_map =
        GPlatesFileIO::OgrUtils::build_feature_map();

    if ( ! feature.is_valid()) {
        return;
    }

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


    GPlatesModel::PropertyValue::non_null_ptr_type value =
        GPlatesPropertyValues::XsString::create(
            GPlatesUtils::make_icu_string_from_qstring(feature_type_key));


    GPlatesPropertyValues::XsString::non_null_ptr_type key =
            GPlatesPropertyValues::XsString::create("TYPE");

    GPlatesPropertyValues::GpmlKeyValueDictionaryElement element(
                key,
                value,
                GPlatesPropertyValues::StructuralType::create_xsi("string"));

    kvd->elements().push_back(element);
}

void
GPlatesFileIO::OgrUtils::add_begin_and_end_time_to_kvd(
	const GPlatesModel::FeatureHandle::const_weak_ref &feature,
	GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type kvd)
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


        GPlatesPropertyValues::XsString::non_null_ptr_type begin_key =
                GPlatesPropertyValues::XsString::create("FROMAGE");

        GPlatesPropertyValues::GpmlKeyValueDictionaryElement begin_element(
                    begin_key,
                    begin_value,
                    GPlatesPropertyValues::StructuralType::create_xsi("double"));

        kvd->elements().push_back(begin_element);



        GPlatesPropertyValues::XsString::non_null_ptr_type end_key =
                GPlatesPropertyValues::XsString::create("TOAGE");

        GPlatesPropertyValues::GpmlKeyValueDictionaryElement end_element(
                    end_key,
                    end_value,
                    GPlatesPropertyValues::StructuralType::create_xsi("double"));

        kvd->elements().push_back(end_element);

    }
}

void
GPlatesFileIO::OgrUtils::add_name_to_kvd(
	const GPlatesModel::FeatureHandle::const_weak_ref &feature,
	GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type kvd)
{
    static const GPlatesModel::PropertyName name_property_name =
        GPlatesModel::PropertyName::create_gml("name");

    const GPlatesPropertyValues::XsString *name;

    if (GPlatesFeatureVisitors::get_property_value(
            feature,name_property_name,name))
    {

        GPlatesModel::PropertyValue::non_null_ptr_type value =
                    name->clone();


        GPlatesPropertyValues::XsString::non_null_ptr_type key =
                GPlatesPropertyValues::XsString::create("NAME");

        GPlatesPropertyValues::GpmlKeyValueDictionaryElement element(
                    key,
                    value,
                    GPlatesPropertyValues::StructuralType::create_xsi("string"));

        kvd->elements().push_back(element);

    }
}

void
GPlatesFileIO::OgrUtils::add_description_to_kvd(
	const GPlatesModel::FeatureHandle::const_weak_ref &feature,
	GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type kvd)
{
    static const GPlatesModel::PropertyName desc_property_name =
        GPlatesModel::PropertyName::create_gml("description");

    const GPlatesPropertyValues::XsString *desc;

    if (GPlatesFeatureVisitors::get_property_value(
            feature,desc_property_name,desc))
    {

        GPlatesModel::PropertyValue::non_null_ptr_type value =
                    desc->clone();


        GPlatesPropertyValues::XsString::non_null_ptr_type key =
                GPlatesPropertyValues::XsString::create("DESCR");

        GPlatesPropertyValues::GpmlKeyValueDictionaryElement element(
                    key,
                    value,
                    GPlatesPropertyValues::StructuralType::create_xsi("string"));

        kvd->elements().push_back(element);

    }
}

void
GPlatesFileIO::OgrUtils::add_feature_id_to_kvd(
	const GPlatesModel::FeatureHandle::const_weak_ref &feature,
	GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type kvd)
{
    GPlatesModel::PropertyValue::non_null_ptr_type feature_id_value =
        GPlatesPropertyValues::XsString::create(feature->feature_id().get());

    GPlatesPropertyValues::XsString::non_null_ptr_type feature_id_key =
            GPlatesPropertyValues::XsString::create("FEATURE_ID");

    GPlatesPropertyValues::GpmlKeyValueDictionaryElement element(
                feature_id_key,
                feature_id_value,
                GPlatesPropertyValues::StructuralType::create_xsi("string"));

    kvd->elements().push_back(element);

}

void
GPlatesFileIO::OgrUtils::add_conjugate_plate_id_to_kvd(
	const GPlatesModel::FeatureHandle::const_weak_ref &feature,
	GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type kvd)
{
    static const GPlatesModel::PropertyName conjugate_id_property_name =
        GPlatesModel::PropertyName::create_gpml("conjugatePlateId");

    const GPlatesPropertyValues::GpmlPlateId *conjugate_plate_id;

    // If we found a plate id, add it.
    if (GPlatesFeatureVisitors::get_property_value(feature,conjugate_id_property_name,conjugate_plate_id))
    {
        // Shapefile attribute field names are limited to 10 characters in length
        // and should not contain spaces.
        GPlatesPropertyValues::XsString::non_null_ptr_type key =
            GPlatesPropertyValues::XsString::create("PLATEID2");
        GPlatesPropertyValues::XsInteger::non_null_ptr_type value =
            GPlatesPropertyValues::XsInteger::create(conjugate_plate_id->value());

        GPlatesPropertyValues::GpmlKeyValueDictionaryElement element(
            key,
            value,
            GPlatesPropertyValues::StructuralType::create_xsi("integer"));
        kvd->elements().push_back(element);
    }
}
