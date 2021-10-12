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

#include <cmath> // std::floor

#include <QChar>
#include <QDebug>
#include <QString>
#include <QTextStream>

#include "ErrorOpeningFileForReadingException.h"
#include "File.h"
#include "FileInfo.h"
#include "GmapReader.h"
#include "ReadErrorAccumulation.h"
#include "ReadErrors.h"

#include "model/Model.h"
#include "model/ModelInterface.h"
#include "model/ModelUtils.h"

#include "property-values/GmlPoint.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/XsDouble.h"
#include "property-values/XsString.h"

namespace
{
	// The initial time-of-apperance, time-of-disappearance will be set to be the
	// sample age +/- DELTA_AGE.
	// 
	// DELTA_AGE is in My.
	const float DELTA_AGE = 5.; 
 
	struct VirtualGeomagneticPole
	{
		QString header;
		float inclination;
		float declination;
		float a95;
		float site_latitude;
		float site_longitude;
		float vgp_latitude;
		float vgp_longitude;
		float dp;
		float dm;
		float age;
		boost::optional<GPlatesModel::integer_plate_id_type> plate_id;
	};
	
	void
	display_vgp(
		const VirtualGeomagneticPole &vgp)
	{
		qDebug();
		qDebug() << "Found complete VGP:";
		qDebug() << "	" << "Header:			" << vgp.header;
		qDebug() << "	" << "Inclination:		" << vgp.inclination;
		qDebug() << "	" << "Declination:		" << vgp.declination;
		qDebug() << "	" << "A95:				" << vgp.a95;
		qDebug() << "	" << "Site latitude:	" << vgp.site_latitude;
		qDebug() << "	" << "Site longitude:	" << vgp.site_longitude;
		qDebug() << "	" << "VGP latitude:		" << vgp.vgp_latitude;
		qDebug() << "	" << "VGP longitude:	" << vgp.vgp_longitude;
		qDebug() << "	" << "dp:				" << vgp.dp;
		qDebug() << "	" << "dm:				" << vgp.dm;
		qDebug() << "	" << "age:				" << vgp.age;
		if (vgp.plate_id)
		{
			qDebug() << "	" << "plate id:			" << *vgp.plate_id;
		}
	}
	
	void
	append_name_to_feature(
		const GPlatesModel::FeatureHandle::weak_ref &feature,
		const QString &description)
	{
		GPlatesPropertyValues::XsString::non_null_ptr_type gml_name = 
			GPlatesPropertyValues::XsString::create(UnicodeString(description.toStdString().c_str()));
		GPlatesModel::ModelUtils::append_property_value_to_feature(
			gml_name, 
			GPlatesModel::PropertyName::create_gml("name"), 
			feature);
	}	
	
	void
	append_site_geometry_to_feature(
		const GPlatesModel::FeatureHandle::weak_ref &feature,
		const float &latitude,
		const float &longitude)
	{
		GPlatesMaths::LatLonPoint llp(latitude,longitude);
		GPlatesMaths::PointOnSphere point = GPlatesMaths::make_point_on_sphere(llp);
		
		const GPlatesModel::PropertyValue::non_null_ptr_type gml_point =
			GPlatesPropertyValues::GmlPoint::create(point);

		GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type property_value =
			GPlatesModel::ModelUtils::create_gpml_constant_value(
			gml_point, 
			GPlatesPropertyValues::TemplateTypeParameterType::create_gml("Point"));

		GPlatesModel::ModelUtils::append_property_value_to_feature(property_value,
			GPlatesModel::PropertyName::create_gpml("averageSampleSitePosition"), feature);
	}
	
	void
	append_inclination_to_feature(
		const GPlatesModel::FeatureHandle::weak_ref &feature,
		const float &inclination)	
	{
		GPlatesPropertyValues::XsDouble::non_null_ptr_type gpml_inclination = 
			GPlatesPropertyValues::XsDouble::create(inclination);
		GPlatesModel::ModelUtils::append_property_value_to_feature(
			gpml_inclination, 
			GPlatesModel::PropertyName::create_gpml("averageInclination"), 
			feature);
	}	
	
	void
	append_declination_to_feature(
		const GPlatesModel::FeatureHandle::weak_ref &feature,
		const float &declination)	
	{
		GPlatesPropertyValues::XsDouble::non_null_ptr_type gpml_declination = 
			GPlatesPropertyValues::XsDouble::create(declination);
		GPlatesModel::ModelUtils::append_property_value_to_feature(
			gpml_declination, 
			GPlatesModel::PropertyName::create_gpml("averageDeclination"), 
			feature);
	}	
	
	void
	append_a95_to_feature(
		const GPlatesModel::FeatureHandle::weak_ref &feature,
		const float &a95)	
	{
		GPlatesPropertyValues::XsDouble::non_null_ptr_type gpml_a95 = 
			GPlatesPropertyValues::XsDouble::create(a95);
		GPlatesModel::ModelUtils::append_property_value_to_feature(
			gpml_a95, 
			//FIXME: Temporary name until role of a95/alpha95 is clarified.
			GPlatesModel::PropertyName::create_gpml("poleA95"), 
			feature);
	}
	
	void
	append_age_to_feature(
		const GPlatesModel::FeatureHandle::weak_ref &feature,
		const float &age)
	{
		GPlatesPropertyValues::XsDouble::non_null_ptr_type gpml_age = 
			GPlatesPropertyValues::XsDouble::create(age);
		GPlatesModel::ModelUtils::append_property_value_to_feature(
			gpml_age, 
			GPlatesModel::PropertyName::create_gpml("averageAge"), 
			feature);
			
		// In addition to setting the paleomag-specific age, we will set 
		// time of appearance and disappearance based on an interval around 
		// the sample age. Here I'm setting it to +/- 10My around the sample
		// age. We may later want to control this globally (e.g. set all paleomag
		// features to be visible for all times, or set them all to be visible
		// +/- x years from their sample age. 
		
		const GPlatesPropertyValues::GeoTimeInstant geo_time_instant_begin(age+DELTA_AGE);
		const GPlatesPropertyValues::GeoTimeInstant geo_time_instant_end(age-DELTA_AGE);

		GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type gml_valid_time = 
			GPlatesModel::ModelUtils::create_gml_time_period(geo_time_instant_begin,
			geo_time_instant_end);
		GPlatesModel::ModelUtils::append_property_value_to_feature(gml_valid_time, 
			GPlatesModel::PropertyName::create_gml("validTime"),
			feature);			
		
	}
	
	void
	append_vgp_position_to_feature(
		const GPlatesModel::FeatureHandle::weak_ref &feature,
		const float &vgp_latitude,
		const float &vgp_longitude)	
	{
	
		GPlatesMaths::LatLonPoint llp(vgp_latitude,vgp_longitude);
		GPlatesMaths::PointOnSphere point = GPlatesMaths::make_point_on_sphere(llp);

		const GPlatesModel::PropertyValue::non_null_ptr_type gml_point =
			GPlatesPropertyValues::GmlPoint::create(point);	
			
		GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type property_value =
			GPlatesModel::ModelUtils::create_gpml_constant_value(
				gml_point, 
				GPlatesPropertyValues::TemplateTypeParameterType::create_gml("Point"));			
			
	
		GPlatesModel::ModelUtils::append_property_value_to_feature(
			property_value, 
			GPlatesModel::PropertyName::create_gpml("polePosition"), 
			feature);		
	}
 
	void
	append_plate_id_to_feature(
		const GPlatesModel::FeatureHandle::weak_ref &feature,
		const GPlatesModel::integer_plate_id_type &plate_id)
	{
		GPlatesPropertyValues::GpmlPlateId::non_null_ptr_type gpml_plate_id = 
			GPlatesPropertyValues::GpmlPlateId::create(plate_id);
		GPlatesModel::ModelUtils::append_property_value_to_feature(
			GPlatesModel::ModelUtils::create_gpml_constant_value(gpml_plate_id,
			GPlatesPropertyValues::TemplateTypeParameterType::create_gpml("plateId")),
			GPlatesModel::PropertyName::create_gpml("reconstructionPlateId"),
			feature);	
	}
	
	void
	append_dm_to_feature(
		const GPlatesModel::FeatureHandle::weak_ref &feature,
		const float &dm)
	{
		GPlatesPropertyValues::XsDouble::non_null_ptr_type gpml_dm = 
			GPlatesPropertyValues::XsDouble::create(dm);
		GPlatesModel::ModelUtils::append_property_value_to_feature(
			gpml_dm, 
			GPlatesModel::PropertyName::create_gpml("poleDm"), 
			feature);	
	}
	
	void
	append_dp_to_feature(
		const GPlatesModel::FeatureHandle::weak_ref &feature,
		const float &dp)
	{
		GPlatesPropertyValues::XsDouble::non_null_ptr_type gpml_dp = 
			GPlatesPropertyValues::XsDouble::create(dp);
		GPlatesModel::ModelUtils::append_property_value_to_feature(
			gpml_dp, 
			GPlatesModel::PropertyName::create_gpml("poleDp"), 
			feature);
	}	
 
	void
	create_vgp_feature(
		GPlatesModel::ModelInterface &model,
		GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
		const VirtualGeomagneticPole &vgp)
	{
		GPlatesModel::FeatureType feature_type = 
			GPlatesModel::FeatureType::create_gpml("VirtualGeomagneticPole");

		GPlatesModel::FeatureHandle::weak_ref feature =
			model->create_feature(feature_type,collection);
			
		append_name_to_feature(feature,vgp.header);
		
		append_site_geometry_to_feature(feature,vgp.site_latitude,vgp.site_longitude);
			
		append_inclination_to_feature(feature,vgp.inclination);
		append_declination_to_feature(feature,vgp.declination);
		append_a95_to_feature(feature,vgp.a95);
		append_vgp_position_to_feature(feature,vgp.vgp_latitude,vgp.vgp_longitude);
		append_age_to_feature(feature,vgp.age);
		append_dm_to_feature(feature,vgp.dm);
		append_dp_to_feature(feature,vgp.dp);
		if (vgp.plate_id)
		{
			append_plate_id_to_feature(feature,*vgp.plate_id);
		}
		
	}
 
	/**
	 * Returns a non-empty boost::optional<float> if @a line, after trimming of white-space,
	 * begins and ends in the double-quote character, and if the string contained between 
	 * the quotes can be converted to a float 
	 */
	boost::optional<float>
	check_format_and_return_value(
		QString &line)
	{
		line = line.trimmed();
		if (!line.startsWith(QChar('"')) && !line.endsWith(QChar('"')))
		{
			return boost::none;
		}
		unsigned int l = line.length();
		line = line.remove(l-1,1);
		line = line.remove(0,1);
		
		bool ok = false;
		float string_as_float = line.toFloat(&ok);

		if (!ok)
		{
			return boost::none;
		}
		else
		{
			return boost::optional<float>(string_as_float);
		}
	}
	
	
 
 
	/**
	 *  Returns true if the @a line is identified as a GMAP vgp header line.
	 *
	 *  The line will be identified as a header line if it does not begin with a double quote.                                                                     
	 */
	bool
	line_is_header(
		const QString &line)
	{
		if (!line.isEmpty() && line.at(0) != QChar('"'))
		{
			return true;
		}
		return false;
	}
	
	void
	read_feature(
		GPlatesModel::ModelInterface &model,
		GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
		const QString &header_line,
		QTextStream &input,
		const boost::shared_ptr<GPlatesFileIO::DataSource> &source,
		unsigned int &line_number,
		GPlatesFileIO::ReadErrorAccumulation &errors)
	{
		qint64 last_good_position = input.pos();
		
		VirtualGeomagneticPole vgp;
		vgp.header = header_line;		
		
		QString next_line = input.readLine();
		++line_number;
		
		// Line 1, inclination, degreees.
		boost::optional<float> inclination = check_format_and_return_value(next_line);
		// FIXME: Check for valid range of inclination.
		if (!inclination)
		{
			throw GPlatesFileIO::ReadErrors::GmapFieldFormatError;
		}
		vgp.inclination = *inclination;
		
		next_line = input.readLine();
		++line_number;
		
		// Line 2, declination, degrees.
		boost::optional<float> declination = check_format_and_return_value(next_line);
		// FIXME: Check for valid range of declination.
		if (!declination)
		{	
			throw GPlatesFileIO::ReadErrors::GmapFieldFormatError;
		}
		vgp.declination = *declination;
				
		next_line = input.readLine();
		++line_number;
		
		// Line 3, a95, degrees.
		boost::optional<float> a95 = check_format_and_return_value(next_line);
		if (!a95)
		{	
			throw GPlatesFileIO::ReadErrors::GmapFieldFormatError;
		}
		vgp.a95 = *a95;
		
		next_line = input.readLine();
		++line_number;

		// Line 4, site latitude, degrees.
		boost::optional<float> site_latitude = check_format_and_return_value(next_line);
		if (!(site_latitude && GPlatesMaths::LatLonPoint::is_valid_latitude(*site_latitude)))
		{
			throw GPlatesFileIO::ReadErrors::GmapFieldFormatError;
		}		
		vgp.site_latitude = *site_latitude;
		
		next_line = input.readLine();
		++line_number;

		// Line 5, site longitude, degrees.
		boost::optional<float> site_longitude = check_format_and_return_value(next_line);
		if (!(site_longitude && GPlatesMaths::LatLonPoint::is_valid_longitude(*site_longitude)))
		{
			throw GPlatesFileIO::ReadErrors::GmapFieldFormatError;
		}	
		vgp.site_longitude = *site_longitude;
		
		next_line = input.readLine();
		++line_number;

		// Line 6, VGP latitude, degrees.
		boost::optional<float> vgp_latitude = check_format_and_return_value(next_line);
		if (!(vgp_latitude && GPlatesMaths::LatLonPoint::is_valid_latitude(*vgp_latitude)))
		{	
			throw GPlatesFileIO::ReadErrors::GmapFieldFormatError;
		}
		vgp.vgp_latitude = *vgp_latitude;
		
		next_line = input.readLine();
		++line_number;

		// Line 7, VGP longitude, degrees.
		boost::optional<float> vgp_longitude = check_format_and_return_value(next_line);
		if (!(vgp_longitude && GPlatesMaths::LatLonPoint::is_valid_longitude(*vgp_longitude)))
		{
			throw GPlatesFileIO::ReadErrors::GmapFieldFormatError;
		}
		vgp.vgp_longitude = *vgp_longitude;
	
		next_line = input.readLine();
		++line_number;

		// Line 8, dp (semi-major axis), degrees.
		boost::optional<float> dp = check_format_and_return_value(next_line);
		if (!dp)
		{
			throw GPlatesFileIO::ReadErrors::GmapFieldFormatError;
		}		
		vgp.dp = *dp;
		
		next_line = input.readLine();
		++line_number;

		// Line 9, dm (semi-minor axis), degrees.
		boost::optional<float> dm = check_format_and_return_value(next_line);
		if (!dm)
		{
			throw GPlatesFileIO::ReadErrors::GmapFieldFormatError;
		}		
		vgp.dm = *dm;
		
		next_line = input.readLine();
		++line_number;

		// Line 10, age, My
		boost::optional<float> age = check_format_and_return_value(next_line);
		if (!age)
		{
			throw GPlatesFileIO::ReadErrors::GmapFieldFormatError;
		}
		vgp.age = *age;
		last_good_position = input.pos();
		
		// Line 11, plate id. This is optional. 
		next_line = input.readLine();
		++line_number;
		
		boost::optional<float> plate_id = check_format_and_return_value(next_line);
		if (!plate_id)
		{
			// No plate id field; that's ok, but to keep track of line numbers for
			// error-reporting, reset the line number and text stream.
			--line_number;
			input.seek(last_good_position);
			vgp.plate_id = boost::none;
		}
		else {
			// We have a plate id field.
			float plate_id_as_float = *plate_id;
			if ((GPlatesMaths::Real(plate_id_as_float) !=
					GPlatesMaths::Real(std::floor(plate_id_as_float))))
			{
				// We did find a plate id, but it's not an integer.
				// Should we round it and use it, or complain?
				// Let's complain.
				throw GPlatesFileIO::ReadErrors::GmapFieldFormatError;
			}
			// The plate id is an integer.
			// Now, we'll cast it to an integer type for the rest of GPlates.
			GPlatesModel::integer_plate_id_type plate_id_as_integer =
					static_cast<GPlatesModel::integer_plate_id_type>(plate_id_as_float);
			vgp.plate_id = plate_id_as_integer;
		}

		// If we've come this far, we should have enough information to create the feature.

		//display_vgp(vgp);

		create_vgp_feature(model,collection,vgp);
	}
	
 }
 
 
const GPlatesFileIO::File::shared_ref
GPlatesFileIO::GmapReader::read_file(
	const FileInfo &fileinfo,
	GPlatesModel::ModelInterface &model,
	ReadErrorAccumulation &read_errors)
{
	QString filename = fileinfo.get_qfileinfo().absoluteFilePath();
	
	QFile file(filename);
	
	if ( ! file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		throw ErrorOpeningFileForReadingException(GPLATES_EXCEPTION_SOURCE, filename);
	}	
	
	QTextStream input(&file);

	boost::shared_ptr<DataSource> source( 
		new GPlatesFileIO::LocalFileDataSource(filename, DataFormats::Gmap));
	GPlatesModel::FeatureCollectionHandle::weak_ref collection
		= model->create_feature_collection();

	// Make sure feature collection gets unloaded when it's no longer needed.
	GPlatesModel::FeatureCollectionHandleUnloader::shared_ref collection_unloader =
		GPlatesModel::FeatureCollectionHandleUnloader::create(collection);

	unsigned int line_number = 0;

	while(!input.atEnd())
	{
		QString header_line = input.readLine();
		if (line_is_header(header_line))
		{
			try {
				read_feature(model, collection, header_line, input, source, line_number, read_errors);
			} 
			catch (GPlatesFileIO::ReadErrors::Description error) {
				const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> location(
					new GPlatesFileIO::LineNumberInFile(line_number));
				read_errors.d_recoverable_errors.push_back(GPlatesFileIO::ReadErrorOccurrence(
					source, location, error, GPlatesFileIO::ReadErrors::GmapFeatureIgnored));
			}
		}
		line_number++;
	}
	if (collection->features_begin() == collection->features_end())
	{
		const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> location(
			new GPlatesFileIO::LineNumberInFile(0));
		read_errors.d_failures_to_begin.push_back(GPlatesFileIO::ReadErrorOccurrence(
					source, location, GPlatesFileIO::ReadErrors::NoFeaturesFoundInFile,
					GPlatesFileIO::ReadErrors::FileNotLoaded));
	}
	return File::create_loaded_file(collection_unloader, fileinfo);
}

 
