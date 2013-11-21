/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2009, 2010 Geological Survey of Norway
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

#include "model/ModelUtils.h"

#include "property-values/GmlPoint.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/XsDouble.h"
#include "property-values/XsString.h"

#include "utils/Profile.h"
#include "utils/UnicodeStringUtils.h"


namespace
{
	// The initial time-of-apperance, time-of-disappearance may be set to be the
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
		boost::optional<GPlatesModel::integer_plate_id_type> plate_id;
		float age;

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
		//qDebug() << "	" << "dp:				" << vgp.dp;
		//qDebug() << "	" << "dm:				" << vgp.dm;
		if (vgp.plate_id)
		{
			qDebug() << "	" << "plate id:			" << *vgp.plate_id;
		}		
		qDebug() << "	" << "age:				" << vgp.age;

	}
	
	void
	append_name_to_feature(
		const GPlatesModel::FeatureHandle::weak_ref &feature,
		const QString &description)
	{
		GPlatesPropertyValues::XsString::non_null_ptr_type gml_name = 
			GPlatesPropertyValues::XsString::create(GPlatesUtils::UnicodeString(description.toStdString().c_str()));
		feature->add(
				GPlatesModel::TopLevelPropertyInline::create(
					GPlatesModel::PropertyName::create_gml("name"),
					gml_name));
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
			GPlatesModel::ModelUtils::create_gpml_constant_value(gml_point);

		feature->add(
				GPlatesModel::TopLevelPropertyInline::create(
					GPlatesModel::PropertyName::create_gpml("averageSampleSitePosition"),
					property_value));
	}
	
	void
	append_inclination_to_feature(
		const GPlatesModel::FeatureHandle::weak_ref &feature,
		const float &inclination)	
	{
		GPlatesPropertyValues::XsDouble::non_null_ptr_type gpml_inclination = 
			GPlatesPropertyValues::XsDouble::create(inclination);
		feature->add(
				GPlatesModel::TopLevelPropertyInline::create(
					GPlatesModel::PropertyName::create_gpml("averageInclination"),
					gpml_inclination));
	}	
	
	void
	append_declination_to_feature(
		const GPlatesModel::FeatureHandle::weak_ref &feature,
		const float &declination)	
	{
		GPlatesPropertyValues::XsDouble::non_null_ptr_type gpml_declination = 
			GPlatesPropertyValues::XsDouble::create(declination);
		feature->add(
				GPlatesModel::TopLevelPropertyInline::create(
					GPlatesModel::PropertyName::create_gpml("averageDeclination"),
					gpml_declination));
	}	
	
	void
	append_a95_to_feature(
		const GPlatesModel::FeatureHandle::weak_ref &feature,
		const float &a95)	
	{
		GPlatesPropertyValues::XsDouble::non_null_ptr_type gpml_a95 = 
			GPlatesPropertyValues::XsDouble::create(a95);
		feature->add(
				GPlatesModel::TopLevelPropertyInline::create(
					//FIXME: Temporary name until role of a95/alpha95 is clarified.
					GPlatesModel::PropertyName::create_gpml("poleA95"),
					gpml_a95));
	}
	
	void
	append_age_to_feature(
		const GPlatesModel::FeatureHandle::weak_ref &feature,
		const float &age)
	{
		GPlatesPropertyValues::XsDouble::non_null_ptr_type gpml_age = 
			GPlatesPropertyValues::XsDouble::create(age);
		feature->add(
				GPlatesModel::TopLevelPropertyInline::create(
					GPlatesModel::PropertyName::create_gpml("averageAge"),
					gpml_age));
 
// VGP visibilty is now set via the UI, and we no longer need to
// provide a begin/end time for the feature. 
#if 0
		const GPlatesPropertyValues::GeoTimeInstant geo_time_instant_begin(age+DELTA_AGE);
		const GPlatesPropertyValues::GeoTimeInstant geo_time_instant_end(age-DELTA_AGE);

		GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type gml_valid_time = 
			GPlatesModel::ModelUtils::create_gml_time_period(geo_time_instant_begin,
			geo_time_instant_end);
		feature->add(
				GPlatesModel::TopLevelPropertyInline::create(
					GPlatesModel::PropertyName::create_gml("validTime"),
					gml_valid_time));
#endif
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
			GPlatesModel::ModelUtils::create_gpml_constant_value(gml_point);

		feature->add(
				GPlatesModel::TopLevelPropertyInline::create(
					GPlatesModel::PropertyName::create_gpml("polePosition"),
					property_value));
	}
 
	void
	append_plate_id_to_feature(
		const GPlatesModel::FeatureHandle::weak_ref &feature,
		const GPlatesModel::integer_plate_id_type &plate_id)
	{
		GPlatesPropertyValues::GpmlPlateId::non_null_ptr_type gpml_plate_id = 
			GPlatesPropertyValues::GpmlPlateId::create(plate_id);
		feature->add(
				GPlatesModel::TopLevelPropertyInline::create(
					GPlatesModel::PropertyName::create_gpml("reconstructionPlateId"),
					GPlatesModel::ModelUtils::create_gpml_constant_value(gpml_plate_id)));
	}
	
	void
	append_dm_to_feature(
		const GPlatesModel::FeatureHandle::weak_ref &feature,
		const float &dm)
	{
		GPlatesPropertyValues::XsDouble::non_null_ptr_type gpml_dm = 
			GPlatesPropertyValues::XsDouble::create(dm);
		feature->add(
				GPlatesModel::TopLevelPropertyInline::create(
					GPlatesModel::PropertyName::create_gpml("poleDm"),
					gpml_dm));
	}
	
	void
	append_dp_to_feature(
		const GPlatesModel::FeatureHandle::weak_ref &feature,
		const float &dp)
	{
		GPlatesPropertyValues::XsDouble::non_null_ptr_type gpml_dp = 
			GPlatesPropertyValues::XsDouble::create(dp);
		feature->add(
				GPlatesModel::TopLevelPropertyInline::create(
					GPlatesModel::PropertyName::create_gpml("poleDp"),
					gpml_dp));
	}	
 
	void
	create_vgp_feature(
		GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
		const VirtualGeomagneticPole &vgp)
	{
		GPlatesModel::FeatureType feature_type = 
			GPlatesModel::FeatureType::create_gpml("VirtualGeomagneticPole");

		GPlatesModel::FeatureHandle::weak_ref feature =
			GPlatesModel::FeatureHandle::create(collection, feature_type);
			
		// Many of the following "append_" functions don't save us so much space/code now,
		// and could be directly replaced by their "feature->add(...)" functions. 
		append_name_to_feature(feature,vgp.header);	
		append_site_geometry_to_feature(feature,vgp.site_latitude,vgp.site_longitude);		
		append_inclination_to_feature(feature,vgp.inclination);
		append_declination_to_feature(feature,vgp.declination);
		append_a95_to_feature(feature,vgp.a95);
		append_vgp_position_to_feature(feature,vgp.vgp_latitude,vgp.vgp_longitude);
		append_age_to_feature(feature,vgp.age);

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
		GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
		const QString &header_line,
		QTextStream &input,
		const boost::shared_ptr<GPlatesFileIO::DataSource> &source,
		unsigned int &line_number,
		GPlatesFileIO::ReadErrorAccumulation &errors)
	{
		//qint64 last_good_position = input.pos();
		
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

		// Line 8 was formerly interpreted as dp , the semi-major axis (degrees).
		// Currently the content of this field is not used.
		boost::optional<float> dp = check_format_and_return_value(next_line);
		if (!dp)
		{
			throw GPlatesFileIO::ReadErrors::GmapFieldFormatError;
		}		
		vgp.dp = *dp;
		
		next_line = input.readLine();
		++line_number;

		// Line 9 (formerly dm) will now be interpreted as plate_id
		boost::optional<float> plate_id = check_format_and_return_value(next_line);
		if (!plate_id)
		{
			throw GPlatesFileIO::ReadErrors::GmapFieldFormatError;
		}
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
		
		next_line = input.readLine();
		++line_number;

		// Line 10, age, My
		boost::optional<float> age = check_format_and_return_value(next_line);
		if (!age)
		{
			throw GPlatesFileIO::ReadErrors::GmapFieldFormatError;
		}
		vgp.age = *age;
		//last_good_position = input.pos();

		// If we've come this far, we should have enough information to create the feature.

		//display_vgp(vgp);

		create_vgp_feature(collection,vgp);
	}
	
 }
 
 
void
GPlatesFileIO::GmapReader::read_file(
		File::Reference &file_ref,
		ReadErrorAccumulation &read_errors)
{
	PROFILE_FUNC();

	const FileInfo &fileinfo = file_ref.get_file_info();

	QString filename = fileinfo.get_qfileinfo().absoluteFilePath();
	
	QFile file(filename);
	
	if ( ! file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		throw ErrorOpeningFileForReadingException(GPLATES_EXCEPTION_SOURCE, filename);
	}	
	
	QTextStream input(&file);

	boost::shared_ptr<DataSource> source( 
		new GPlatesFileIO::LocalFileDataSource(filename, DataFormats::Gmap));

	GPlatesModel::FeatureCollectionHandle::weak_ref collection = file_ref.get_feature_collection();

	unsigned int line_number = 0;

	while(!input.atEnd())
	{
		QString header_line = input.readLine();
		if (line_is_header(header_line))
		{
			try
			{
				read_feature(collection, header_line, input, source, line_number, read_errors);
			} 
			catch (GPlatesFileIO::ReadErrors::Description error)
			{
				const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> location(
					new GPlatesFileIO::LineNumber(line_number));
				read_errors.d_recoverable_errors.push_back(GPlatesFileIO::ReadErrorOccurrence(
					source, location, error, GPlatesFileIO::ReadErrors::GmapFeatureIgnored));
			}
		}
		line_number++;
	}
	if (collection->begin() == collection->end())
	{
		const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> location(
			new GPlatesFileIO::LineNumber(0));
		read_errors.d_failures_to_begin.push_back(GPlatesFileIO::ReadErrorOccurrence(
					source, location, GPlatesFileIO::ReadErrors::NoFeaturesFoundInFile,
					GPlatesFileIO::ReadErrors::FileNotLoaded));
	}
}

