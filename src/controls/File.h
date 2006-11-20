/**
 * @file 
 * $Id$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006 The University of Sydney, Australia
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

#ifndef _GPLATES_CONTROLS_FILE_H_
#define _GPLATES_CONTROLS_FILE_H_

#include <string>
#include "global/types.h"

namespace GPlatesControls
{
	namespace File
	{
		/**
		 * Open a native GPlates data file.
		 */
		void
		OpenData(const std::string& filepath);

		/**
		 * Load a PLATES rotation file.
		 */
		void
		LoadRotation(const std::string& filepath);

		/**
		 * Import a non-native data file.
		 */
		void ImportData(const std::string &filepath);

		/**
		 * Write the current data to a GPML file.
		 */
		void
		SaveData(const std::string& filepath);
		
		/**
		 * Exit GPlates.
		 */
		void
		Quit(const GPlatesGlobal::integer_t& exit_status);
	}
}

#endif  /* _GPLATES_CONTROLS_FILE_H_ */
