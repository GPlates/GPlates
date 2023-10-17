/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2021 The University of Sydney, Australia
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

#ifndef GPLATES_FILEIO_STANDALONEBUNDLE_H
#define GPLATES_FILEIO_STANDALONEBUNDLE_H

#include <boost/optional.hpp>
#include <QString>


namespace GPlatesFileIO
{
	/**
	 * A standalone bundle can be used for GPlates or pyGPlates, and means the bundle can be
	 * relocated to another folder (or another computer) and still function.
	 *
	 * This is because the dependency libraries (eg, GDAL) are installed into the bundle
	 * along with the GPlates executable (or the pyGPlates module library).
	 *
	 * Other inclusions are the GDAL and PROJ library data
	 * (eg, 'proj.db' is used by the proj library and needs to be found at runtime).
	 *
	 * Another inclusion is the Python standard library (for GPlates only).
	 */
	namespace StandaloneBundle
	{
		/**
		 * Initialise so that queries on the standalone bundle can be made.
		 *
		 * This also tells GDAL and PROJ where to find the bundled PROJ data (eg, 'proj.db').
		 *
		 * GPlates knows where the bundle is (via the Qt library).
		 * NOTE: QApplication should be initialised *before* this function is called since we rely on
		 *       QCoreApplication::applicationDirPath() to find the runtime directory of the gplates executable.
		 *
		 * In order for pyGPlates to know where its bundle is it must specify its runtime location here
		 * (it does this just after it is first imported into an external Python interpreter).
		 */
		void
		initialise(
#if !defined(GPLATES_PYTHON_EMBEDDING)  // compiling pygplates (not gplates)
			QString pygplates_import_directory
#endif
		);

		//
		// The following data directory queries return none if GPlates (or pyGPlates) was not compiled
		// as standalone (ie, GPLATES_INSTALL_STANDALONE is not defined) or the standalone bundle has
		// not yet been created (ie, if we've built GPlates with GPLATES_INSTALL_STANDALONE but not yet
		// installed it, eg, with "cmake --install . --prefix ~/gplates/", or not yet packaged it, eg, with "cpack").
		//

		/**
		 * Return the location of the Proj resource data in the standalone bundle.
		 */
		boost::optional<QString>
		get_proj_data_directory();

		/**
		 * Return the location of the GDAL resource data in the standalone bundle.
		 */
		boost::optional<QString>
		get_gdal_data_directory();

		/**
		 * Return the location of the GDAL plugins in the standalone bundle.
		 */
		boost::optional<QString>
		get_gdal_plugins_directory();

		/**
		 * Return the location of the Python standard library in the standalone bundle.
		 *
		 * Note: This is only used for gplates (not pygplates since that's imported by an external
		 * non-embedded Python interpreter that has its own Python standard library).
		 */
		boost::optional<QString>
		get_python_standard_library_directory();
	}
}

#endif // GPLATES_FILEIO_STANDALONEBUNDLE_H
