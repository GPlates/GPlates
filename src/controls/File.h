/**
 * @file 
 * $Id$
 * 
 * Copyright (C) 2003 The GPlates Consortium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
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
