/* $Id$ */

/**
 * @file
 * Contains the definition of the class PlatesLineFormatReader.
 *
 * Most recent change:
 *   $Date$
 *
 * Copyright (C) 2007 The University of Sydney, Australia
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

#ifndef GPLATES_FILEIO_PLATESLINEFORMATREADER_H
#define GPLATES_FILEIO_PLATESLINEFORMATREADER_H

#include <string>

#include "ErrorOpeningFileForReadingException.h"
#include "ReadErrorAccumulation.h"
#include "model/ModelInterface.h"
#include "model/FeatureCollectionHandle.h"

namespace GPlatesFileIO
{
	/**
	 * A PLATES line-format reader is used to read the contents of a PLATES line-format
	 * file and parse it into the contents of a feature collection.
	 */
	class PlatesLineFormatReader
	{
	public:
		/**
		 * Read the PLATES line-format file named @a filename.
		 *
		 * If the file cannot be opened for reading, an exception of type
		 * ErrorOpeningFileForReadingException will be thrown.
		 */
		static
		const GPlatesModel::FeatureCollectionHandle::weak_ref
		read_file(
				const std::string &filename,
				GPlatesModel::ModelInterface &model,
				ReadErrorAccumulation &read_errors);
	};
}
#endif  // GPLATES_FILEIO_PLATESLINEFORMATREADER_H
