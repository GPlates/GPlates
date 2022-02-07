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

#include <algorithm>
#include <set>

#include <QDir>
#include <QStringList>

#include "qt-widgets/HellingerModel.h"
#include "utils/ComponentManager.h"

#include "ErrorOpeningFileForReadingException.h"
#include "HellingerReader.h"

namespace
{
	const int MIN_NUM_FIELDS = 5;

	bool
	plate_index_represents_an_enabled_pick(
			const GPlatesQtWidgets::HellingerPlateIndex &plate_index)
	{
		return ((plate_index == GPlatesQtWidgets::PLATE_ONE_PICK_TYPE) ||
				(plate_index == GPlatesQtWidgets::PLATE_TWO_PICK_TYPE) ||
				(plate_index == GPlatesQtWidgets::PLATE_THREE_PICK_TYPE));
	}

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

	/**
	 * @brief angle_ok
	 * @param s
	 * @param angle
	 * @return
	 */
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
	 * @param lat- latitude, initial guess
	 * @param lon - longitude, initial guess
	 * @param rho - angle, initial guess
	 * @return  true if the fields lat,lon,rho can be parsed correctly from the QString line.
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

	/**
	 * @brief boolean_line_ok - returns true of @a line consists of
	 * upper- or lower-case "y" or "n", returns false otherwise.
	 * @param line
	 * @param result - on return, contains true if @a line was "y" or "Y", false if it was "n" or "N"
	 */
	bool
	boolean_line_ok(const QString &line, bool &result)
	{
		QString stripped_line = line;
		stripped_line.replace(" ","");
		if ((stripped_line == "y") ||
				(stripped_line == "Y")){
			result = true;
			return true;
		}
		else if ((stripped_line == "n") ||
				 (stripped_line == "N"))
		{
			result = false;
			return true;
		}
		return false;
	}

	std::set<GPlatesQtWidgets::HellingerPlateIndex>
	create_two_way_plate_index_set()
	{
		std::set<GPlatesQtWidgets::HellingerPlateIndex> indices;
		indices.insert(GPlatesQtWidgets::PLATE_ONE_PICK_TYPE);
		indices.insert(GPlatesQtWidgets::PLATE_TWO_PICK_TYPE);
		indices.insert(GPlatesQtWidgets::DISABLED_PLATE_ONE_PICK_TYPE);
		indices.insert(GPlatesQtWidgets::DISABLED_PLATE_TWO_PICK_TYPE);
		return indices;
	}

	std::set<GPlatesQtWidgets::HellingerPlateIndex>
	create_three_way_plate_index_set()
	{
		std::set<GPlatesQtWidgets::HellingerPlateIndex> indices;
		indices.insert(GPlatesQtWidgets::PLATE_ONE_PICK_TYPE);
		indices.insert(GPlatesQtWidgets::PLATE_TWO_PICK_TYPE);
		indices.insert(GPlatesQtWidgets::PLATE_THREE_PICK_TYPE);
		indices.insert(GPlatesQtWidgets::DISABLED_PLATE_ONE_PICK_TYPE);
		indices.insert(GPlatesQtWidgets::DISABLED_PLATE_TWO_PICK_TYPE);
		indices.insert(GPlatesQtWidgets::DISABLED_PLATE_THREE_PICK_TYPE);
		return indices;
	}

	/**
	 * @brief pick_fields_are_ok
	 * Returns true if it was possible to build a pick structure from @param fields.
	 * @param pick and @param int are filled on return.
	 */
	bool
	pick_fields_are_ok(
			const QStringList &fields,
			GPlatesQtWidgets::HellingerPick &pick,
			int &segment)
	{
		/* here we should check that the fields are sensible,
				  e.g.
					FIELD           POSSIBLE VALUES
					1               1, 2, 3, 31, 32, 33  (where an intial "3" indicates a commented out pick)
					2               integer > 0
					3               double in range -90 -> 90
					4               double in range -360->360
					5               double > 0.
															*/
		// Field 0: 1, 2, 3, 31, 32 or 33.

		static const std::set<GPlatesQtWidgets::HellingerPlateIndex> valid_plate_indices =
				GPlatesUtils::ComponentManager::instance().is_enabled(
							GPlatesUtils::ComponentManager::Component::hellinger_three_plate()) ?
					create_three_way_plate_index_set() : create_two_way_plate_index_set();
		bool plate_index_ok = false;
		bool ok;
		GPlatesQtWidgets::HellingerPlateIndex plate_index =
				static_cast<GPlatesQtWidgets::HellingerPlateIndex>(fields.at(0).toInt(&ok));
		if ((ok) && (valid_plate_indices.find(plate_index) != valid_plate_indices.end()))
		{
			plate_index_ok  = true;
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

		if(plate_index_ok  && segment_ok && lat_ok && lon_ok && uncert_ok)
		{
			bool enabled = plate_index_represents_an_enabled_pick(plate_index);
			pick = GPlatesQtWidgets::HellingerPick(static_cast<GPlatesQtWidgets::HellingerPlateIndex>(plate_index),
												   lat,lon,uncert,enabled);
			return true;
		}
		return false;
	}

	/**
	 * @brief test_first_line - 3-way pick files may have a single integer in the first line
	 * representing the total number of segments. Find and return this number, if it exists.
	 * @param line
	 * @return
	 */
	boost::optional<int>
	try_to_extract_nsegments_from_first_line(
			const QString &line)
	{
		if (line.split(" ", QString::SkipEmptyParts).size() == 1)
		{
			bool ok = false;
			int nlines = line.toInt(&ok);
			if (ok){
				return nlines;
			}
		}
		return boost::none;
	}

	void
	parse_pick_line(
			const QString &line,
			GPlatesQtWidgets::hellinger_model_type &pick_data)
	{
		QStringList fields = line.split(" ",QString::SkipEmptyParts);

		GPlatesQtWidgets::HellingerPick pick;
		int segment;
		if ((fields.size() >= MIN_NUM_FIELDS) && (pick_fields_are_ok(fields,pick,segment)))
		{
			pick_data.insert(GPlatesQtWidgets::hellinger_model_pair_type(segment,pick));
		}
		else
		{
			throw GPlatesFileIO::ReadErrors::HellingerPickFormatError;
		}
	}

	void
	parse_two_plate_com_line(
			const QString &line,
			GPlatesQtWidgets::HellingerComFileStructure &hellinger_com_file,
			unsigned int &line_number)
	{
		bool line_ok = false;
		switch(line_number){
		case 0: // string representing pick filename
			hellinger_com_file.d_pick_file = line;
			line_ok = true;
			break;
		case 1: // three space-separated numerical values representing initial guesses of lat,lon,angle
		{
			double lat = 0., lon = 0., rho = 0.;
			if ((line_ok = initial_guess_ok(line,lat,lon,rho)))
			{
				hellinger_com_file.d_estimate_12.d_lat = lat;
				hellinger_com_file.d_estimate_12.d_lon = lon;
				hellinger_com_file.d_estimate_12.d_angle = rho;
			}
			break;
		}
		case 2: // double representing search radius
		{
			double radius = line.toDouble(&line_ok);
			if (line_ok){
				hellinger_com_file.d_search_radius_degrees = radius;
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
			hellinger_com_file.d_error_ellipse_filename_12 = line;
			line_ok = true;
			break;
		}
		case 8: // string - filename for .up output
		{
			hellinger_com_file.d_upper_surface_filename_12 = line;
			line_ok = true;
			break;
		}
		case 9: // string - filename for .down output
		{
			hellinger_com_file.d_lower_surface_filename_12 = line;
			line_ok = true;
			break;
		}
		} // switch

		if (!line_ok)
		{
			throw GPlatesFileIO::ReadErrors::InvalidHellingerComFileFormat;
		}

	}


	void
	read_file_and_guess(
			QTextStream &stream,
			GPlatesQtWidgets::HellingerComFileStructure &hellinger_com_file,
			unsigned int &line_number)
	{
		QString line;
		double lat = 0., lon = 0., rho = 0.;
		bool line_ok = false;

		line = stream.readLine();
		if (!line.isEmpty())
		{
			hellinger_com_file.d_pick_file = line;
		}
		else
		{
			throw GPlatesFileIO::ReadErrors::InvalidHellingerComFileFormat;
		}
		++line_number;

		line = stream.readLine();
		if ((line_ok = initial_guess_ok(line,lat,lon,rho)))
		{
			GPlatesQtWidgets::HellingerPoleEstimate estimate(lat,lon,rho);
			hellinger_com_file.d_estimate_12 = estimate;
		}
		else
		{
			throw GPlatesFileIO::ReadErrors::InvalidHellingerComFileFormat;
		}
		++line_number;
	}

	void
	read_file_and_guesses(
			QTextStream &stream,
			GPlatesQtWidgets::HellingerComFileStructure &hellinger_com_file,
			unsigned int &line_number)
	{
		QString line;
		double lat = 0., lon = 0., rho = 0.;
		bool line_ok = false;

		line = stream.readLine();
		if (!line.isEmpty())
		{
			hellinger_com_file.d_pick_file = line;
		}
		else
		{
			throw GPlatesFileIO::ReadErrors::InvalidHellingerComFileFormat;
		}
		++line_number;

		line = stream.readLine();
		if ((line_ok = initial_guess_ok(line,lat,lon,rho)))
		{
			GPlatesQtWidgets::HellingerPoleEstimate estimate(lat,lon,rho);
			hellinger_com_file.d_estimate_12 = estimate;
		}
		else
		{
			throw GPlatesFileIO::ReadErrors::InvalidHellingerComFileFormat;
		}
		++line_number;

		line = stream.readLine();
		if ((line_ok = initial_guess_ok(line,lat,lon,rho)))
		{
			GPlatesQtWidgets::HellingerPoleEstimate estimate(lat,lon,rho);
			hellinger_com_file.d_estimate_13 = estimate;
		}
		else
		{
			throw GPlatesFileIO::ReadErrors::InvalidHellingerComFileFormat;
		}
		++line_number;
	}

	void
	read_search_and_grid_options(
			QTextStream &stream,
			GPlatesQtWidgets::HellingerComFileStructure &hellinger_com_file,
			unsigned int &line_number)
	{


		QString line = stream.readLine();

		bool line_ok = false;
		double search_radius_degrees = line.toDouble(&line_ok);
		if (line_ok){
			hellinger_com_file.d_search_radius_degrees = search_radius_degrees;
		}
		else
		{
			throw GPlatesFileIO::ReadErrors::InvalidHellingerComFileFormat;
		}
		++line_number;

		// The FORTRAN code (hellinger1) only performs a grid
		// search if "y" is the response (for command-line use) or parameter (for file use),
		// i.e. there is no initial default iteration of the grid search. So we
		// start from zero iterations.
		unsigned int number_of_grid_iterations = 0;

		bool content = false;
		while (!stream.atEnd())
		{
			line = stream.readLine();
			if (line.isEmpty() || !boolean_line_ok(line,content))
			{
				throw GPlatesFileIO::ReadErrors::InvalidHellingerComFileFormat;
			}
			if (!content)
			{
				break;
			}
			++number_of_grid_iterations;
			++line_number;
		}
		hellinger_com_file.d_number_of_grid_iterations = number_of_grid_iterations;
		hellinger_com_file.d_perform_grid_search = (number_of_grid_iterations > 0);

		qDebug() << "line_number: " << line_number;
		qDebug() << "iterations: " << number_of_grid_iterations;
	}

	void
	read_amoeba_iterations(
			QTextStream &stream,
			GPlatesQtWidgets::HellingerComFileStructure &hellinger_com_file,
			unsigned int &line_number)
	{
		QString line;
		bool content = false;

		// The FORTRAN code (hellinger3) runs amoeba once before asking
		// for further iterations; any "y" lines in the .com file
		// add to the initial iteration. So we start at 1.
		//
		unsigned int number_of_amoeba_iterations = 1;
		while (!stream.atEnd())
		{
			line = stream.readLine();
			if (line.isEmpty() || !boolean_line_ok(line,content))
			{
				throw GPlatesFileIO::ReadErrors::InvalidHellingerComFileFormat;
			}
			if (!content)
			{
				break;
			}
			++number_of_amoeba_iterations;
			++line_number;
		}
		hellinger_com_file.d_number_amoeba_iterations = number_of_amoeba_iterations;
		qDebug() << "line_number: " << line_number;
		qDebug() << "iterations: " << number_of_amoeba_iterations;
	}

	void
	read_confidence_and_kappa(
			QTextStream &stream,
			GPlatesQtWidgets::HellingerComFileStructure &hellinger_com_file,
			unsigned int &line_number)
	{
		QString line = stream.readLine();

		bool line_ok = false;
		double sig_level = line.toDouble(&line_ok);
		if (line_ok){
			hellinger_com_file.d_significance_level = sig_level;
		}
		else
		{
			throw GPlatesFileIO::ReadErrors::InvalidHellingerComFileFormat;
		}
		++line_number;

		line = stream.readLine();

		bool kappa = true;
		if ((line_ok = boolean_line_ok(line,kappa)))
		{
			hellinger_com_file.d_estimate_kappa = kappa;
		}
		else
		{
			throw GPlatesFileIO::ReadErrors::InvalidHellingerComFileFormat;
		}
		++line_number;
	}

	void
	read_output_filenames(
			const GPlatesQtWidgets::HellingerPlatePairType &pair_type,
			QTextStream &stream,
			GPlatesQtWidgets::HellingerComFileStructure &hellinger_com_file,
			unsigned int &line_number)
	{

	}

	/**
	 * @brief parse_two_plate_com_lines - parse the fields in a
	 * hellinger1 ".com" file and store them in @a hellinger_com_file
	 *
	 * @param stream
	 * @param hellinger_com_file
	 * @param line_number - on return, the line number which we last attempted to read,
	 * for error reporting.
	 */
	void
	parse_two_plate_com_lines(
			QTextStream &stream,
			GPlatesQtWidgets::HellingerComFileStructure &hellinger_com_file,
			unsigned int &line_number)

	{
		/*
		 * hellinger1 com files are assumed to have the following format:
		 * (Note that if line 4 is "y", then additional lines are read, and so the line numbers
		 * provided below - from "line 5" onwards - may be offset)
		 * line 1: (string) filename containing pick data
		 * line 2: (3 space-separated doubles) initial guess for the rotation pole (lat,lon,angle)
		 * line 3: (double) search radius in degrees. Used for grid search, but also to set the
		 * initial perturbation for an amoeba search.
		 * line 4: (single character, "n", "N", "y" or "Y") - whether or not to perform a grid search.
		 * If line 4 is "y" or "Y", the following lines will represent whether or not to perform an
		 * additional grid search at a reduced search radius, until a line contains "n" or "N".
		 * line 5: (double) confidence level for uncertainty calculation
		 * line 6: (single characteter, "n", "N", "y" or "Y") - whether or not to calculate "kappa"
		 * line 7: (single characteter, "n", "N", "y" or "Y") - whether or not to calculate output graphics
		 * lines 8-10 (string) - if line 7 is "y" or "Y", then lines 8-10 correspond to the output filenames
		 * for the error ellipse, the upper error bounds, and the lower error bounds respectively.
		 *
		 * Currently we ignore lines 6 onwards, as we always calculate kappa, and we always calculate
		 * "output graphics" files. Output graphics files are given names derived from the input
		 * pick filename, and with suffixes "_ellipse.dat", "_up.dat" and "_down.dat" respectively.
		 */
		read_file_and_guess(stream,hellinger_com_file,line_number);
		read_search_and_grid_options(stream,hellinger_com_file,line_number);
		read_confidence_and_kappa(stream,hellinger_com_file,line_number);
		read_output_filenames(GPlatesQtWidgets::PLATES_1_2_PAIR_TYPE,stream,hellinger_com_file,line_number);
	}


	void
	parse_three_plate_com_lines(
			QTextStream &stream,
			GPlatesQtWidgets::HellingerComFileStructure &hellinger_com_file,
			unsigned int &line_number)
	{
		read_file_and_guesses(stream,hellinger_com_file,line_number);
		read_amoeba_iterations(stream,hellinger_com_file,line_number);
		read_confidence_and_kappa(stream,hellinger_com_file,line_number);
		read_output_filenames(GPlatesQtWidgets::PLATES_1_2_PAIR_TYPE,stream,hellinger_com_file,line_number);
		read_output_filenames(GPlatesQtWidgets::PLATES_1_3_PAIR_TYPE,stream,hellinger_com_file,line_number);
		read_output_filenames(GPlatesQtWidgets::PLATES_2_3_PAIR_TYPE,stream,hellinger_com_file,line_number);

		qDebug() << "from com: no amoeba iterations: " << hellinger_com_file.d_number_amoeba_iterations;
	}

	/**
	 * @brief parse_filename_for_chron_string
	 * @param filepath
	 * @return the string lying between the last dash ("-") or last underscore ("_")  (whichever lies closest to the end)
	 * and the last full stop ("."). If the resultant string has a leading "C" character, this is stripped before returning.
	 */
	QString
	parse_filename_for_chron_string(
			QString filepath)
	{
		QFileInfo file_info(filepath);

		// completeBaseName gives us the filename upto the last "." and without the path.
		QString base_name = file_info.completeBaseName();

		int dash_index = base_name.lastIndexOf("-");
		int underscore_index = base_name.lastIndexOf("_");

		if ((dash_index == -1) && (underscore_index == -1))
		{
			return QString();
		}

		int index = qMax(dash_index,underscore_index);
		int l = base_name.length();

		QString chron_string = base_name.right(l-index-1);

		if (chron_string.startsWith('C'))
		{
			// Remove leading character
			chron_string.remove(0,1);
		}

#if 0
		qDebug() << "base_name: " << base_name;
		qDebug() << "indices: " << dash_index << ", " << underscore_index;
		qDebug() << "Chron string: " << chron_string;
		qDebug() << "Stripped chron string: " << chron_string;
#endif
		return chron_string;
	}

	/**
	 * @brief determine_com_file_type_from_third_line
	 * Distinguish two-way and three-way .com file based on the contents of the third line.
	 * In a two-way com file, the third line should be a single double (representing search radius)
	 * In a three-way com file, the third line should represent the initial guess for plate-pair
	 * 1 and 3, and should consist of 3 space-separated doubles.
	 * @param line
	 * @return
	 */
	GPlatesQtWidgets::HellingerFitType
	determine_com_file_type_from_third_line(
			const QString &line)
	{
		QStringList list = line.split(" ",QString::SkipEmptyParts);
		if (list.size() == 1)
		{
			return GPlatesQtWidgets::TWO_PLATE_FIT_TYPE;
		}
		else if (GPlatesUtils::ComponentManager::instance().is_enabled(
					 GPlatesUtils::ComponentManager::Component::hellinger_three_plate()) &&
				list.size() == 3)
		{
			return GPlatesQtWidgets::THREE_PLATE_FIT_TYPE;
		}
		else{
			throw GPlatesFileIO::ReadErrors::InvalidHellingerComFileFormat;
		}
	}

	/**
	 * @brief determine_fit_type - determines whether the file represented by
	 * @a stream is a 2-way or 3-way .com file, based solely on the form of the
	 * third line.
	 *
	 * @a stream is reset to the start of the file before returning.
	 *
	 * @param stream
	 * @return
	 */
	GPlatesQtWidgets::HellingerFitType
	determine_fit_type(
			QTextStream &stream)
	{

		QString line;

		// Skip to third line.
		line = stream.readLine();
		line = stream.readLine();
		line = stream.readLine();

		qDebug() << "3rd line: " << line;

		stream.seek(0);
		return determine_com_file_type_from_third_line(line);
	}

}





bool
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
	unsigned int valid_lines = 0;

	GPlatesQtWidgets::hellinger_model_type pick_data;

	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		read_errors.d_failures_to_begin.push_back(
					ReadErrorOccurrence(
						source,
						boost::shared_ptr<LocationInDataSource>(new LineNumber(line_number)),
						ReadErrors::ErrorOpeningFileForReading,
						ReadErrors::FileNotLoaded));
		return false;
	}

	QTextStream input(&file);

	QString line = input.readLine();
	boost::optional<int> number_of_segments = try_to_extract_nsegments_from_first_line(line);

	// Reset textstream if we didn't have a segment number as the first line.
	if (!number_of_segments){
		input.seek(0);
	}

	while(!input.atEnd())
	{
		line = input.readLine();
		if (line.simplified().isEmpty() || (line.at(0) == '#'))
		{
			continue;
		}
		try
		{
			parse_pick_line(line,pick_data);
			++valid_lines;
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

	GPlatesQtWidgets::HellingerFitType fit_type = hellinger_model.get_fit_type();

	if (number_of_segments)
	{
		if (fit_type == GPlatesQtWidgets::TWO_PLATE_FIT_TYPE)
		{
			// The number of segments is usually specified in the first line of 3-way pick files, but not 2-way files.
			// Warn the user, but continue with a two-way fit.
			qWarning() << "Hellinger: first line of two-way Hellinger pick file contains the number of segments.";
		}
		if (hellinger_model.number_of_segments() != *number_of_segments)
		{
			// Warn the user, but continue using the number of segments in the model.
			qWarning() << "Hellinger: number of segments specified does not match number of segments in file.";
		}
	}

	if(valid_lines > 0)
	{
		qDebug() << "validlines:" << valid_lines;
		hellinger_model.reset_model();
		hellinger_model.set_model_data(pick_data);
		return true;
	}

	return false;

}

bool
GPlatesFileIO::HellingerReader::read_com_file(
		const QString &filename,
		GPlatesQtWidgets::HellingerModel &hellinger_model,
		ReadErrorAccumulation &read_errors)
{

	boost::shared_ptr<DataSource> source(
				new LocalFileDataSource(
					filename,
					DataFormats::HellingerPick));

	unsigned int line_number = 0;

	QFile file(filename);

	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		read_errors.d_failures_to_begin.push_back(
					ReadErrorOccurrence(
						source,
						boost::shared_ptr<LocationInDataSource>(new LineNumber(line_number)),
						ReadErrors::ErrorOpeningFileForReading,
						ReadErrors::FileNotLoaded));
		return false;
	}

	QTextStream stream(&file);

	GPlatesQtWidgets::HellingerComFileStructure com_file_structure;

	try{

		GPlatesQtWidgets::HellingerFitType type =
				determine_fit_type(stream);

		qDebug() << "fit type: " << type;

		hellinger_model.set_fit_type(type);

		switch(type)
		{
		case GPlatesQtWidgets::TWO_PLATE_FIT_TYPE:
			parse_two_plate_com_lines(stream,com_file_structure,line_number);
			break;
		case GPlatesQtWidgets::THREE_PLATE_FIT_TYPE:
			parse_three_plate_com_lines(stream,com_file_structure,line_number);
			break;
		}
	}
	catch (GPlatesFileIO::ReadErrors::Description error)
	{
		const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> location(
					new GPlatesFileIO::LineNumber(line_number+1));
		read_errors.d_terminating_errors.push_back(GPlatesFileIO::ReadErrorOccurrence(
					source, location, error, GPlatesFileIO::ReadErrors::HellingerComFileNotImported));
		return false;
	}

	QString chron_string = parse_filename_for_chron_string(filename);

	hellinger_model.set_com_file_structure(com_file_structure);

	hellinger_model.set_chron_string(chron_string);

	return true;
}


void
GPlatesFileIO::HellingerReader::read_error_ellipse(
		const QString &filename,
		GPlatesQtWidgets::HellingerModel &hellinger_model,
		const GPlatesQtWidgets::HellingerPlatePairType &type)
{
	QFile data_file(filename);

	hellinger_model.clear_error_ellipse(type);
	if (data_file.open(QFile::ReadOnly))
	{
		QTextStream in(&data_file);
		in.readLine();
		while (!in.atEnd())
		{
			QString line = in.readLine();
			QStringList fields = line.split(" ",QString::SkipEmptyParts);
			GPlatesMaths::LatLonPoint llp(fields.at(1).toDouble(),fields.at(0).toDouble());
			hellinger_model.error_ellipse_points(type).push_back(llp);
		}
	}
}

void
GPlatesFileIO::HellingerReader::read_fit_results_from_temporary_fit_file(
		const QString &filename,
		GPlatesQtWidgets::HellingerModel &hellinger_model)
{
	int line_number = 0;
	QFile file(filename);
	if (!file.open(QFile::ReadOnly))
	{
		throw GPlatesFileIO::ReadErrors::HellingerFileError;
	}

	hellinger_model.clear_fit_results();

	QTextStream stream(&file);
	while(!stream.atEnd())
	{
		bool ok;
		QString line = stream.readLine();
		QStringList fields = line.split(" ",QString::SkipEmptyParts);

		if (fields.size() != 3)
		{
			throw GPlatesFileIO::ReadErrors::HellingerFileError;
		}
		double lat = fields.at(0).toDouble(&ok);
		double lon = fields.at(1).toDouble(&ok);
		double angle = fields.at(2).toDouble(&ok);

		if (!ok)
		{
			throw GPlatesFileIO::ReadErrors::HellingerFileError;
		}


		GPlatesQtWidgets::HellingerFitStructure fit(lat,lon,angle);
		switch(line_number)
		{
		case 0:
			hellinger_model.set_fit_12(fit);
			break;
		case 1:
			hellinger_model.set_fit_13(fit);
			break;
		case 2:
			hellinger_model.set_fit_23(fit);
			break;
		default:
			break;
		}
		++line_number;
	}
}
