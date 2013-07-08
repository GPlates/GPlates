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
            QStringList d_com_fields;
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

            bool
            fields_com_are_ok(
                    const QStringList &fields)
            {
                /*
                    FIELD      VALUE
                    0          double in range -90 -> 90 | LAT
                    1          double in range -360 -> 360 | LON
                    2          double > 0 | RHO
                */
                bool ok1;
                bool ok2;
                bool ok3;
                fields.at(0).toDouble(&ok1);
                fields.at(1).toDouble(&ok2);
                fields.at(2).toDouble(&ok3);
                if (ok1 && ok2 && ok3)
                {

                    if (((fields.at(0).toDouble()<=90)&&(fields.at(0).toDouble()>=-90))&&
                            ((fields.at(1).toDouble()<=360)&&(fields.at(1).toDouble()>=-360))&&
                            (fields.at(2).toDouble()>0))
                    {
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }
                else
                {
                    return false;
                }
            }


            bool
            bool_data_com_ok(
                    const QStringList &fields)
            {
               bool ok;
               fields.at(0).toDouble(&ok);
               if (ok)
               {
                   return true;
               }
               else
               {
                   return false;
               }
            }

            bool
            string_data_com_ok(
                    const QStringList &fields)
            {
                bool ok;
                fields.at(0).toDouble(&ok);
                if (ok)
                {
                    return false;
                }
                else
                {
                    if (!fields.at(0).isEmpty())
                    {
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }
            }

			void
			parse_line(
				const QString &line,
				GPlatesQtWidgets::HellingerModel &hellinger_model)
			{
                QStringList fields = line.split(" ",QString::SkipEmptyParts);
                if (fields.size() >= MIN_NUM_FIELDS)
                {
                    if (fields_are_ok(fields))
                    {
                            hellinger_model.add_pick(fields);
                    }
                    else
                    {
                        throw GPlatesFileIO::ReadErrors::HellingerPickFieldFormatError;
                    }
                }
                else
                {
                    throw GPlatesFileIO::ReadErrors::InvalidHellingerPickFileFormat;
                }
			}


            void
            parse_com_fields(
                    const QStringList &field,
                    GPlatesQtWidgets::HellingerModel &hellinger_model,
                    unsigned int &line_number,
                    GPlatesFileIO::ReadErrorAccumulation &read_errors,
                    QString path)
			{

				if (line_number == 0)
                {
					// Line 0 should contain a single string representing the pick filename.
					if (field.length() == 1)
					{
						d_com_fields<<field.at(0);
					}
					else
					{
						qDebug() << "Throwing";
						throw GPlatesFileIO::ReadErrors::HellingerPickFieldFormatError;
					}
                }
                else if (line_number == 1)
                {

                    if (!fields_com_are_ok(field))
                    {
                        throw GPlatesFileIO::ReadErrors::HellingerComFieldFormatError;
                    }
                    else
                    {
                        d_com_fields<<field.at(0)<<field.at(1)<<field.at(2);
                    }
                }
                else if ((line_number == 2) ||(line_number == 4))
                {
                    if (!bool_data_com_ok(field))
                    {
                        throw GPlatesFileIO::ReadErrors::HellingerComFieldFormatError;
                    }
                    else
                    {
                        d_com_fields<<field.at(0);
                    }
                }
                else if ((line_number == 3) || (line_number == 5) ||
                         (line_number == 6) || (line_number == 7) ||
                         (line_number == 8) || (line_number == 9))
                {
                    if (!string_data_com_ok(field))
                    {
                        throw GPlatesFileIO::ReadErrors::HellingerComFieldFormatError;
                    }
                    else if (line_number==9)
                    {
                        d_com_fields<<field.at(0);                       
                        if (d_com_fields.count()==MIN_NUM_COM_FIELDS)
                        {
                        GPlatesFileIO::HellingerReader::read_pick_file(path+"/"+d_com_fields.at(0),hellinger_model,read_errors);
                        hellinger_model.set_initial_guess(d_com_fields);
                        d_com_fields.clear();
                        }
                        else
                        {
                            throw GPlatesFileIO::ReadErrors::InvalidHellingerComFileFormat;
                            d_com_fields.clear();
                        }
                    }
                    else
                    {
                        d_com_fields<<field.at(0);
                    }
                }

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
				source, location, error, GPlatesFileIO::ReadErrors::PickIgnored));	
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

    unsigned int line_com_number = 0;

    QFileInfo file_info(file.fileName());
    QString path = file_info.absolutePath();

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        read_errors.d_failures_to_begin.push_back(
            ReadErrorOccurrence(
                source,
                boost::shared_ptr<LocationInDataSource>(new LineNumber(line_com_number)),
                ReadErrors::ErrorOpeningFileForReading,
                ReadErrors::FileNotLoaded));
        return;
    }
    QTextStream input(&file);
    QString line;
    while(!input.atEnd())
    {
        line = input.readLine();
        QStringList field = line.split(" ",QString::SkipEmptyParts);
        if (!field.isEmpty())
        {
            try
            {
                parse_com_fields(field,hellinger_model, line_com_number, read_errors, path);
            }
            catch (GPlatesFileIO::ReadErrors::Description error)
            {
				qDebug() << "Catching " << line_com_number;
                const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> location(
                    new LineNumber(line_com_number+1));
                read_errors.d_recoverable_errors.push_back(GPlatesFileIO::ReadErrorOccurrence(
                    source, location, error, GPlatesFileIO::ReadErrors::ComIgnored));
            }
            ++line_com_number;
        }
     }

}
