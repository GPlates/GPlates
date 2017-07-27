/**
 * \file
 * File specific comments.
 *
 * Most recent change:
 *   $Date: 2012-02-29 17:05:33 +0100 (Wed, 29 Feb 2012) $
 *
 * Copyright (C) 2012 Geological Survey of Norway
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

#ifndef GPLATES_FILEIO_PICKFILEREADER_H
#define GPLATES_FILEIO_PICKFILEREADER_H

#include "qt-widgets/HellingerModel.h"
#include "ReadErrorAccumulation.h"

namespace GPlatesFileIO
{


	/**
	 * @brief The HellingerReader class
	 */
	class HellingerReader
	{
	public:
		HellingerReader();

		/**
		 * @brief read_pick_file - Read and parse the contents of a text .pick file,
		 * putting the contents into @a hellinger_model.
		 * The file can be a 2-way or 3-way pick file. We do some guess work based on the
		 * presence of an initial n-value (in 3-way files) and on the range of indices
		 * in the first column (1 and 2 only => 2-way pick file; 1,2 and 3 => 3-way file).
		 *
		 * @param filename
		 * @param hellinger_model
		 * @param read_errors
		 */
		static
		bool
		read_pick_file(
				const QString &filename,
				GPlatesQtWidgets::HellingerModel &hellinger_model,
				ReadErrorAccumulation &read_errors);

		static
		bool
		read_com_file(
				const QString &filename,
				GPlatesQtWidgets::HellingerModel& hellinger_model,
				ReadErrorAccumulation &read_errors);

		static
		void
		read_error_ellipse(
				const QString &filename,
				GPlatesQtWidgets::HellingerModel& hellinger_model,
				const GPlatesQtWidgets::HellingerPlatePairType &type = GPlatesQtWidgets::PLATES_1_2_PAIR_TYPE);

		static
		void
		read_fit_results_from_temporary_fit_file(
				const QString &filename,
				GPlatesQtWidgets::HellingerModel& hellinger_model);
	};

}
#endif // GPLATES_FILEIO_PICKFILEREADER_H
