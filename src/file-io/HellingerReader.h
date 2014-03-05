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

#include "ReadErrorAccumulation.h"

namespace GPlatesQtWidgets
{
	class HellingerModel;
}

namespace GPlatesFileIO
{


	/**
	 * @brief The HellingerReader class
	 */
	class HellingerReader
	{
	public:
		HellingerReader();

		static
		void
		read_pick_file(
				const QString &filename,
				GPlatesQtWidgets::HellingerModel &hellinger_model,
				ReadErrorAccumulation &read_errors);

		static
		void
		read_com_file(
				const QString &filename,
				GPlatesQtWidgets::HellingerModel& hellinger_model,
				ReadErrorAccumulation &read_errors);

		// TODO: Implement. Reading the error ellipse is done by the HellingerModel class.
		static
		void
		read_error_ellipse(
				const QString &filename,
				GPlatesQtWidgets::HellingerModel& hellinger_model);
	};

}
#endif // GPLATES_FILEIO_PICKFILEREADER_H
