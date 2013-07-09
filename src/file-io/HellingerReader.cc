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


#include <QStringList>

#include "qt-widgets/HellingerModel.h"

#include "ErrorOpeningFileForReadingException.h"
#include "HellingerReader.h"

namespace
{
            const int MIN_NUM_FIELDS = 5;
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
					const QString s, double &latitude)
			{

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
				if((latitude_ok(fields.at(0),lat) &&
					longitude_ok(fields.at(1),lon) &&
					angle_ok(fields.at(2),rho)))
				{
					return true;
				}
				return false;
			}

			bool
			boolean_line_ok(const QString &line, bool result)
			{
				return false;
			}

            bool
            fields_are_ok(
                    const QStringList &fields)
            {
                /* here we should check that the fields are sensible,
                  e.g.
                    FIELD           POSSIBLE VALUES
                    1               1, 2, 31, 32  (where the "3" indicated a commented out pick)
                    2               integer > 0
                    3               double in range -90 -> 90
                    4               double in range -360->360
                    5               double > 0.
                                                            */
                if ((fields.at(0).toInt()== 1 || fields.at(0).toInt()==2 || fields.at(0).toInt()==31 ||
                     fields.at(0).toInt()== 32)&&(fields.at(1).toInt()>0)&&
                        ((fields.at(2).toDouble() <= 90)&&(fields.at(2).toDouble()>=-90))&&
                        ((fields.at(3).toDouble() <= 360)&&(fields.at(3).toDouble()>=-360))&&
                        (fields.at(4).toDouble() > 0))
                {
                    return true;
                }
                else
                {
                    return false;
                }
            }

			void
			parse_line(
				const QString &line,
				GPlatesQtWidgets::HellingerModel &hellinger_model)
			{
				QStringList fields = line.split(" ",QString::SkipEmptyParts);
				if ((fields.size() >= MIN_NUM_FIELDS) && (fields_are_ok(fields)))
				{
					hellinger_model.add_pick(fields);
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
				GPlatesQtWidgets::hellinger_com_file_struct &hellinger_com_file = hellinger_model.get_hellinger_com_file_struct();
				bool line_ok = false;
				switch(line_number){
				case 0: // string representing pick filename
					hellinger_com_file.d_pick_file = line;
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
					if (boolean_line_ok(line,grid_search))
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
					break;
				}
				case 8: // string - filename for .up output
				{
					hellinger_com_file.d_up_filename = line;
					break;
				}
				case 9: // string - filename for .down output
				{
					hellinger_com_file.d_down_filename = line;
					break;
				}
				} // switch


            }


}


#if 0
void
GPlatesFileIO::PickFileReader::read_file(
        File::Reference &file_ref,
        GPlatesModel::ModelInterface &model,
        ReadErrorAccumulation &read_errors,
        const GPlatesModel::integer_plate_id_type &moving_plate_id,
        const GPlatesModel::integer_plate_id_type &fixed_plate_id,
        const double &chron_time)
{
    const FileInfo &fileinfo = file_ref.get_file_info();

    // By placing all changes to the model under the one changeset, we ensure that
    // feature revision ids don't get changed from what was loaded from file no
    // matter what we do to the features.
    GPlatesModel::ChangesetHandle changeset(
            model.access_model(),
            "open " + fileinfo.get_qfileinfo().fileName().toStdString());

    QString filename = fileinfo.get_qfileinfo().absoluteFilePath();

    QFile file(filename);

    if ( ! file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        throw ErrorOpeningFileForReadingException(GPLATES_EXCEPTION_SOURCE, filename);
    }

    QTextStream input(&file);

    boost::shared_ptr<DataSource> source(
        new GPlatesFileIO::LocalFileDataSource(filename, DataFormats::Gmap));

    GPlatesModel::FeatureCollectionHandle::weak_ref collection = file_ref.get_feature_collection();

    // For error reporting.
    unsigned int line_number = 0;

    while(!input.atEnd())
    {
        QString line = input.readLine();
        try
        {
            read_feature(model, collection, line, input, source, line_number, read_errors);
        }
        catch (GPlatesFileIO::ReadErrors::Description error)
        {
            const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> location(
                        new GPlatesFileIO::LineNumber(line_number));
            read_errors.d_recoverable_errors.push_back(GPlatesFileIO::ReadErrorOccurrence(
                                                           source, location, error, GPlatesFileIO::ReadErrors::GmapFeatureIgnored));
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
#endif

void
GPlatesFileIO::HellingerReader::read_configuration_file(
	const QString &filename, ReadErrorAccumulation &read_errors)
{

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
			parse_line(line,hellinger_model);
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

}
