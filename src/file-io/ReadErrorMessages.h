/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#ifndef GPLATES_FILE_IO_READERRORMESSAGES_H
#define GPLATES_FILE_IO_READERRORMESSAGES_H

#include <QString>

#include "ReadErrors.h"


namespace GPlatesFileIO
{
	namespace ReadErrorMessages
	{
		/**
		 * Converts a ReadErrors::Description enum to a translated QString (short form).
		 */
		const QString &
		get_short_description_as_string(
				ReadErrors::Description code);


		/**
		 * Converts a ReadErrors::Description enum to a translated QString (full text).
		 */
		const QString &
		get_full_description_as_string(
				ReadErrors::Description code);


		/**
		 * Converts a ReadErrors::Result enum to a translated QString.
		 */
		const QString &
		get_result_as_string(
				ReadErrors::Result code);
	}
}

#endif // GPLATES_FILE_IO_READERRORMESSAGES_H
