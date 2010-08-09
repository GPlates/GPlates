/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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

#ifndef GPLATES_FILEIO_TEMPORARYFILEREGISTRY_H
#define GPLATES_FILEIO_TEMPORARYFILEREGISTRY_H

#include <vector>
#include <QString>

#include "utils/Singleton.h"


namespace GPlatesFileIO
{
	/**
	 * A singleton that collects filenames of files to be deleted when the
	 * application exits.
	 *
	 * Use QTemporaryFile if you need a temporary file that deletes itself when the
	 * QTemporaryFile object is destroyed. Use this registry if you want the
	 * lifetime of the temporary file to be the application's lifetime, not the
	 * lifetime of any particular object.
	 */
	class TemporaryFileRegistry :
			public GPlatesUtils::Singleton<TemporaryFileRegistry>
	{
		GPLATES_SINGLETON_CONSTRUCTOR_DEF(TemporaryFileRegistry)

	public:

		~TemporaryFileRegistry();

		/**
		 * Registers @a filename as a temporary file, that will be deleted when the
		 * application exits.
		 */
		void
		add_file(
				const QString &filename);

	private:

		std::vector<QString> d_filenames;
	};
}

#endif  // GPLATES_FILEIO_TEMPORARYFILEREGISTRY_H
