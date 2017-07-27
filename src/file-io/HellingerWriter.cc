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

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStringList>

#include "qt-widgets/HellingerModel.h"

#include "HellingerWriter.h"

namespace
{
	GPlatesQtWidgets::HellingerPlateIndex
	get_plate_index(
			const GPlatesQtWidgets::HellingerPlateIndex index,
			bool enabled)
	{
		if ((index == GPlatesQtWidgets::PLATE_ONE_PICK_TYPE) ||
				(index == GPlatesQtWidgets::DISABLED_PLATE_ONE_PICK_TYPE))
		{
			return enabled ? GPlatesQtWidgets::PLATE_ONE_PICK_TYPE : GPlatesQtWidgets::DISABLED_PLATE_ONE_PICK_TYPE;
		}
		else if ((index == GPlatesQtWidgets::PLATE_TWO_PICK_TYPE) ||
				 (index == GPlatesQtWidgets::DISABLED_PLATE_TWO_PICK_TYPE))
		{
			return enabled ? GPlatesQtWidgets::PLATE_TWO_PICK_TYPE : GPlatesQtWidgets::DISABLED_PLATE_TWO_PICK_TYPE;
		}
		else if ((index == GPlatesQtWidgets::PLATE_THREE_PICK_TYPE) ||
				 (index == GPlatesQtWidgets::DISABLED_PLATE_THREE_PICK_TYPE))
		{
			return enabled ? GPlatesQtWidgets::PLATE_THREE_PICK_TYPE : GPlatesQtWidgets::DISABLED_PLATE_THREE_PICK_TYPE;
		}
		return index;
	}

}

void
GPlatesFileIO::HellingerWriter::write_pick_file(
		QString &filename,
		GPlatesQtWidgets::HellingerModel &hellinger_model,
		bool export_disabled_picks,
		bool add_missing_pick_extension)
{
	static const QString extension("pick");

	if (add_missing_pick_extension)
	{
		QFileInfo file_info(filename);
		if (QString::compare(file_info.suffix(),extension,Qt::CaseInsensitive) != 0)
		{
			filename = file_info.absolutePath() + QDir::separator() + file_info.baseName() + "." + extension;
		}
	}


	QFile file(filename);
	QTextStream out(&file);


	if (file.open(QIODevice::WriteOnly))
	{
		GPlatesQtWidgets::hellinger_model_type::const_iterator it = hellinger_model.begin();


		for (; it != hellinger_model.end() ; ++it)
		{
			int segment = it->first;
			GPlatesQtWidgets::HellingerPick pick = it->second;

			bool enabled = pick.d_is_enabled;

			// Skip export of the pick if it's disabled, and the caller has requested that
			// disabled picks are not exported.
			if (!enabled && !export_disabled_picks)
			{
				continue;
			}

			QString line;

			GPlatesQtWidgets::HellingerPlateIndex index = get_plate_index(pick.d_segment_type,pick.d_is_enabled);
			line.append(QString::number(index));
			line.append(" ");

			line.append(QString::number(segment));
			line.append(" ");

			line.append(QString::number(pick.d_lat));
			line.append(" ");

			line.append(QString::number(pick.d_lon));
			line.append(" ");

			line.append(QString::number(pick.d_uncertainty));
			line.append("\n");

			out << line;

		}
		file.close();
	}
	else
	{
		qWarning() << "HellingerWriter: Failed to open file " << filename << "for writing.";
	}

}

void
GPlatesFileIO::HellingerWriter::write_com_file(
		QString &filename,
		GPlatesQtWidgets::HellingerModel &hellinger_model)
{
	// NOTE: We may want to set up a more informative .com file structure, but as this would mess up use of these files in users'
	// FORTRAN routines, leave things as they are for now.
	// Later we can export to one of 2 versions (or both):
	//		- legacy .com file for FORTRAN compliance
	//		- GPlates .com (or some other suitable extension) for use with GPlates. Here we would have free rein
	//			on the format, content etc.
	boost::optional<GPlatesQtWidgets::HellingerComFileStructure> com_struct = hellinger_model.get_com_file();
	if (com_struct)
	{
		static const QString com_extension("com");
		static const QString pick_extension("pick");

		QFileInfo file_info(filename);
		if (QString::compare(file_info.suffix(),com_extension,Qt::CaseInsensitive) != 0)
		{
			filename = file_info.absolutePath() + QDir::separator() + file_info.baseName() + "." + com_extension;
		}
		qDebug() << filename;
		QFile file(filename);
		QTextStream out(&file);

		//TODO: if we haven't specified a pick file name, export a pick file with the same root name as the provided .com filename.

		// Strip the pick file name down from the full path to the filename only. The .com file format expects the filename relative
		// to the location of the .com file.  The procedures here won't guarantee that (the user can choose a .com location anywhere).
		// TODO: To do this properly I think we need a dedicated export dialog to handle the various possibilities i.e.
		//   - export pick file alone, let user select any output path/filename.
		//   - export .com and pick file together: user selects names for both, but path of pick file is forced to be the same as the .com file.
		//   - export .com only (say the user wants to use an existing pick file but just wants to change the initial guess or something...)
		//		- but what do we do with the pick filepath in this situation??
		QFile pick_file(com_struct->d_pick_file);
		QFileInfo pick_fileinfo(pick_file);
#if 1
		// Use the existing pick filename if it exists, else use the .com filename as basis for pick filename.
		QString pick_filename = pick_fileinfo.fileName();

		if (pick_filename.isEmpty())
		{
			pick_filename = file_info.baseName() + "." + pick_extension;
		}
#else
		// Use .com filename as basis for pick filename.
		QString pick_filename = file_info.baseName() +"." + pick_extension;
#endif
		if (file.open(QIODevice::WriteOnly))
		{
			// Pick file name
			out << pick_filename << '\n';

			// Initial guess: lat, lon, rho
			out << com_struct->d_estimate_12.d_lat << " " << com_struct->d_estimate_12.d_lon << " " << com_struct->d_estimate_12.d_angle << '\n';

			// Search radius
			out << com_struct->d_search_radius_degrees << '\n';

			// Perform grid search
			QString grid_string = "n";
			if (com_struct->d_perform_grid_search)
			{
				grid_string = "y";
			}
			out << grid_string << '\n';

			// Significance level
			out << com_struct->d_significance_level << '\n';

			// Estimate kappa
			QString kappa_string = "n";
			if (com_struct->d_estimate_kappa)
			{
				kappa_string = "y";
			}
			out << kappa_string << '\n';

			// Output graphics
			QString graphics_string = "n";
			if (com_struct->d_generate_output_files)
			{
				graphics_string = "y";
			}
			out << graphics_string << '\n';

			// Dat file names
			out << com_struct->d_error_ellipse_filename_12 << '\n';
			out << com_struct->d_upper_surface_filename_12 << '\n';
			out << com_struct->d_lower_surface_filename_12 << '\n';

		}
		else
		{
			qWarning() << "HellingerWriter: Failed to open file " << filename << "for writing.";
		}
	}
}
