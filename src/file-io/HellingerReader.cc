/**
 * \file
 * File specific comments.
 *
 * Most recent change:
 *   $Date: 2012-02-29 17:05:33 +0100 (Wed, 29 Feb 2012) $
 *
 * Copyright (C) 2012, 2013 Geological Survey of Norway
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

#include <QDir>
#include <QStringList>

#include "qt-widgets/HellingerModel.h"

#include "ErrorOpeningFileForReadingException.h"
#include "HellingerReader.h"

namespace
{
            const int MIN_NUM_FIELDS = 5;
			// FIXME: Currently unused, can probably remove.
            const int MIN_NUM_COM_FIELDS = 12;

#if 0
            void
            create_pick_feature(
                GPlatesModel::ModelInterface &model,
                GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
                const Pick &pick)
            {
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
                // Read stuff from the line
                Pick pick;

                create_pick_feature(model,collection,pick);
            }

#endif

			bool
			latitude_ok(
					const QString &s, double &latitude)
			{
				bool ok;
				latitude = s.toDouble(&ok);
				return((ok) && GPlatesMaths::LatLonPoint::is_valid_latitude(latitude));
			}

			bool
			longitude_ok(
					const QString &s, double &longitude)
			{
				bool ok;
				longitude = s.toDouble(&ok);
				return ((ok) && GPlatesMaths::LatLonPoint::is_valid_longitude(longitude));
			}

			bool
			angle_ok(
					const QString &s, double &angle)
			{
				bool ok;
				angle = s.toDouble(&ok);
				return ((ok) && (angle <= 360.) && (angle >= -360.));
			}

			/**
			 * @brief initial_guess_ok
			 * @param line - string containing line to be parsed
			 * @param lat - latitude, initial guess
			 * @param lon - longitude, initial guess
			 * @param rho - angle, initial guess
			 * @return true if the fields lat,lon,rho can be parsed correctly from the QString line.
			 * lat,lon and rho are filled with their parsed values on successful return.
			 */
			bool
			initial_guess_ok(const QString &line,double &lat, double &lon, double &rho)
			{
				QStringList fields = line.split(' ',QString::SkipEmptyParts);
				if (fields.length() != 3){
					return false;
				}
				if((latitude_ok(fields.at(0),lat) &&
					longitude_ok(fields.at(1),lon) &&
					angle_ok(fields.at(2),rho)))
				{
					return true;
				}
				return false;
			}

			bool
			boolean_line_ok(const QString &line, bool &result)
			{
				if ((line == "y") ||
						(line == "Y")){
					result = true;
					return true;
				}
				else if ((line == "n") ||
						(line == "N"))
				{
					result = false;
					return true;
				}
				return false;
			}

            bool
			pick_fields_are_ok(
					const QStringList &fields,
					GPlatesQtWidgets::HellingerPick &pick,
					int &segment)
            {
                /* here we should check that the fields are sensible,
                  e.g.
                    FIELD           POSSIBLE VALUES
					1               1, 2, 31, 32  (where the "3" indicates a commented out pick)
                    2               integer > 0
                    3               double in range -90 -> 90
                    4               double in range -360->360
                    5               double > 0.
                                                            */
				// Field 0: 1, 2, 31 or 32.
				bool moving_fixed_ok = false;
				bool ok;
				int moving_or_fixed = fields.at(0).toInt(&ok);
				if ((ok) && (
							(moving_or_fixed == 1) ||
							(moving_or_fixed == 2) ||
							(moving_or_fixed == 31) ||
							(moving_or_fixed == 32)))
				{
					moving_fixed_ok = true;
				}

				// Field 1: integer > 0
				bool segment_ok = false;
				segment = fields.at(1).toInt(&ok);
				if ((ok) && (segment > 0)){
					segment_ok = true;
				}

				// Field 2: latitude
				bool lat_ok;
				double lat = fields.at(2).toDouble(&ok);
				if ((ok) && GPlatesMaths::LatLonPoint::is_valid_latitude(lat))
				{
					lat_ok = true;
				}

				// Field 3: longitude
				bool lon_ok;
				double lon = fields.at(3).toDouble(&ok);
				if ((ok) && GPlatesMaths::LatLonPoint::is_valid_longitude(lon))
				{
					lon_ok = true;
				}

				// Field 4: uncertainty
				bool uncert_ok;
				double uncert = fields.at(4).toDouble(&ok);
				if ((ok) && (uncert > 0.))
				{
					uncert_ok = true;
				}

				if(moving_fixed_ok && segment_ok && lat_ok && lon_ok && uncert_ok)
				{
					bool enabled = ((moving_or_fixed == GPlatesQtWidgets::FIXED_SEGMENT_TYPE) ||
									(moving_or_fixed == GPlatesQtWidgets::MOVING_SEGMENT_TYPE));
					pick = GPlatesQtWidgets::HellingerPick(static_cast<GPlatesQtWidgets::HellingerSegmentType>(moving_or_fixed),
														   lat,lon,uncert,enabled);
					return true;
				}
				return false;
            }

			void
			parse_pick_line(
				const QString &line,
				GPlatesQtWidgets::HellingerModel &hellinger_model)
			{
				QStringList fields = line.split(" ",QString::SkipEmptyParts);

				GPlatesQtWidgets::HellingerPick pick;
				int segment;
				if ((fields.size() >= MIN_NUM_FIELDS) && (pick_fields_are_ok(fields,pick,segment)))
				{
					hellinger_model.add_pick(pick,segment);
					//hellinger_model.add_pick(fields);
				}
				else
				{
					throw GPlatesFileIO::ReadErrors::HellingerPickFormatError;
				}
			}


            void
			parse_com_lines(
					const QString &line,
                    GPlatesQtWidgets::HellingerModel &hellinger_model,
                    unsigned int &line_number,
					GPlatesFileIO::ReadErrorAccumulation &read_errors)
			{
				GPlatesQtWidgets::HellingerComFileStructure &hellinger_com_file = hellinger_model.get_hellinger_com_file_struct();
				bool line_ok = false;
				switch(line_number){
				case 0: // string representing pick filename
					hellinger_com_file.d_pick_file = line;
					line_ok = true;
					break;
				case 1: // three space-separated numerical values representing initial guesses of lat,lon,angle
				{
					double lat,lon,rho;
					if ((line_ok = initial_guess_ok(line,lat,lon,rho)))
					{
						hellinger_com_file.d_lat = lat;
						hellinger_com_file.d_lon = lon;
						hellinger_com_file.d_rho = rho;
					}
					break;
				}
				case 2: // double representing search radius
				{
					double radius = line.toDouble(&line_ok);
					if (line_ok){
						hellinger_com_file.d_search_radius = radius;
					}
					break;
				}
				case 3: // y or n - whether or not a grid search is performed.
				{
					bool grid_search;
					if ((line_ok = boolean_line_ok(line,grid_search)))
					{
						hellinger_com_file.d_perform_grid_search = grid_search;
					}
					break;
				}
				case 4: // double - significance level
				{
					double sig_level = line.toDouble(&line_ok);
					if (line_ok){
						hellinger_com_file.d_significance_level = sig_level;
					}
					break;
				}
				case 5:	// y or n - whether or not to estimate kappa
				{
					bool kappa;
					if ((line_ok = boolean_line_ok(line,kappa)))
					{
						hellinger_com_file.d_estimate_kappa = kappa;
					}
					break;
				}
				case 6: // y or n - whether or not to "calculate graphics output"
				{
					bool graphics;
					if ((line_ok = boolean_line_ok(line,graphics)))
					{
						hellinger_com_file.d_generate_output_files = graphics;
					}
					break;
				}
				case 7: // string - filename for .dat output
				{
					hellinger_com_file.d_data_filename = line;
					line_ok = true;
					break;
				}
				case 8: // string - filename for .up output
				{
					hellinger_com_file.d_up_filename = line;
					line_ok = true;
					break;
				}
				case 9: // string - filename for .down output
				{
					hellinger_com_file.d_down_filename = line;
					line_ok = true;
					break;
				}
				} // switch

				if (!line_ok)
				{
					throw GPlatesFileIO::ReadErrors::InvalidHellingerComFileFormat;
				}

            }


}





void
GPlatesFileIO::HellingerReader::read_pick_file(
	const QString &filename, 
	GPlatesQtWidgets::HellingerModel& hellinger_model, 
	ReadErrorAccumulation &read_errors)
{
	QFile file(filename);
	boost::shared_ptr<DataSource> source(
		new LocalFileDataSource(
			filename,
			DataFormats::HellingerPick));

	unsigned int line_number = 0;

	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		read_errors.d_failures_to_begin.push_back(
			ReadErrorOccurrence(
				source,
				boost::shared_ptr<LocationInDataSource>(new LineNumber(line_number)),
				ReadErrors::ErrorOpeningFileForReading,
				ReadErrors::FileNotLoaded));
		return;
	}

	QTextStream input(&file);

	while(!input.atEnd())
	{
		QString line = input.readLine();
		try
		{
			parse_pick_line(line,hellinger_model);
		}
		catch (GPlatesFileIO::ReadErrors::Description error)
		{
			const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> location(
				new LineNumber(line_number));
			read_errors.d_recoverable_errors.push_back(GPlatesFileIO::ReadErrorOccurrence(
				source, location, error, GPlatesFileIO::ReadErrors::HellingerPickIgnored));
		}	
		++line_number;

	}
}

void
GPlatesFileIO::HellingerReader::read_com_file(
        const QString &filename,
        GPlatesQtWidgets::HellingerModel &hellinger_model,
        ReadErrorAccumulation &read_errors)
{
    QFile file(filename);
    boost::shared_ptr<DataSource> source(
        new LocalFileDataSource(
            filename,
            DataFormats::HellingerPick));

	unsigned int line_number = 0;

    QFileInfo file_info(file.fileName());

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        read_errors.d_failures_to_begin.push_back(
            ReadErrorOccurrence(
                source,
				boost::shared_ptr<LocationInDataSource>(new LineNumber(line_number)),
                ReadErrors::ErrorOpeningFileForReading,
                ReadErrors::FileNotLoaded));
        return;
    }
    QTextStream input(&file);
    QString line;
    while(!input.atEnd())
    {
        line = input.readLine();
		if (!line.isEmpty())
        {
            try
            {
				parse_com_lines(line,hellinger_model,line_number,read_errors);
            }
            catch (GPlatesFileIO::ReadErrors::Description error)
            {
                const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> location(
					new LineNumber(line_number+1));
                read_errors.d_recoverable_errors.push_back(GPlatesFileIO::ReadErrorOccurrence(
					source, location, error, GPlatesFileIO::ReadErrors::HellingerComFileNotImported));
				return;
            }

        }
		++line_number;
     }

	QString pick_file = file_info.absolutePath() + QDir::separator() + hellinger_model.get_hellinger_com_file_struct().d_pick_file;

	read_pick_file(pick_file,hellinger_model,read_errors);

}
