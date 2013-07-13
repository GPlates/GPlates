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

#include <QFile>
#include <QStringList>

#include "qt-widgets/HellingerModel.h"

#include "HellingerWriter.h"


void
GPlatesFileIO::HellingerWriter::write_pick_file(
		const QString &filename,
		GPlatesQtWidgets::HellingerModel &hellinger_model,
		bool export_disabled_picks)
{
	QFile file(filename);
	QTextStream out(&file);

	// TODO: Refactor this. Probably cleaner to grab the raw data from the model and convert to string as we go.
	if (file.open(QIODevice::WriteOnly))
	{
		QStringList load_data = hellinger_model.get_data_as_string();
		QString pick_state;
		if (!load_data.isEmpty())
		{
			int a = 0;
			for (int i=0; i < (load_data.size()/6); i++)
			{
				bool should_export = true;
				bool ok;
				if (!load_data.at(a+5).toInt(&ok))
				{
					if (!export_disabled_picks)
					{
						should_export = false;
					}
					if (load_data.at(a+1).toInt() == GPlatesQtWidgets::MOVING_PICK_TYPE)
					{

						pick_state = QString("%1").arg(GPlatesQtWidgets::DISABLED_MOVING_PICK_TYPE);

					}
					else if (load_data.at(a+1).toInt() == GPlatesQtWidgets::FIXED_PICK_TYPE)
					{

						pick_state = QString("%1").arg(GPlatesQtWidgets::DISABLED_FIXED_PICK_TYPE);
					}
				}
				else if (load_data.at(a+5).toInt(&ok))
				{
					pick_state = load_data.at(a+1);
				}
				if (should_export)
				{
					out << pick_state<<" "<<load_data.at(a)<<" "<<load_data.at(a+2)
						<<" "<<load_data.at(a+3)<<" "<<load_data.at(a+4)<<"\n";
//					qDebug() << pick_state<<" "<<load_data.at(a)<<" "<<load_data.at(a+2)
//							 <<" "<<load_data.at(a+3)<<" "<<load_data.at(a+4)<<"\n";
				}
				a=a+6;
			}
		}
	}
}

void GPlatesFileIO::HellingerWriter::write_com_file(
		const QString &filename,
		GPlatesQtWidgets::HellingerModel &hellinger_model)
{
	// NOTE: We may want to set up a more informative .com file structure, but as this would mess up use of these files in users'
	// FORTRAN routines, leave things as they are for now.
	// Later we can export to one of 2 versions (or both):
	//		- legacy .com file for FORTRAN compliance
	//		- GPlates .com (or some other suitable extension) for use with GPlates. Here we would have free reign
	//			on the format, content etc.
	boost::optional<GPlatesQtWidgets::HellingerComFileStructure> com_struct = hellinger_model.get_com_file();
	if (com_struct)
	{
		QFile file(filename);
		QTextStream out(&file);

		if (file.open(QIODevice::WriteOnly))
		{
			// Pick file name
			out << com_struct->d_pick_file << '\n';

			// Initial guess: lat, lon, rho
			out << com_struct->d_lat << " " << com_struct->d_lon << " " << com_struct->d_rho << '\n';

			// Search radius
			out << com_struct->d_search_radius << '\n';

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
			out << com_struct->d_data_filename << '\n';
			out << com_struct->d_up_filename << '\n';
			out << com_struct->d_down_filename << '\n';

		}
	}
}
