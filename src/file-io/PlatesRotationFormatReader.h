/* $Id$ */

/**
 * @file
 * Contains the definition of the class PlatesRotationFormatReader.
 *
 * Most recent change:
 *   $Date$
 *
 * Copyright (C) 2007 James Boyden <jboy@jboy.id.au>
 * Copyright (C) 2007, 2009 The University of Sydney, Australia
 * Copyright (C) 2007 Geological Survey of Norway
 *
 * This file is derived from the header file "PlatesRotationFormatReader.hh",
 * which is part of the ReconTreeViewer software:
 *  http://sourceforge.net/projects/recontreeviewer/
 *
 * ReconTreeViewer is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License, version 2, as published
 * by the Free Software Foundation.
 *
 * ReconTreeViewer is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef GPLATES_FILEIO_PLATESROTATIONFORMATREADER_H
#define GPLATES_FILEIO_PLATESROTATIONFORMATREADER_H

#include "File.h"
#include "FileInfo.h"
#include "ErrorOpeningFileForReadingException.h"
#include "ReadErrorAccumulation.h"
#include "model/ModelInterface.h"
#include "model/FeatureCollectionHandle.h"

namespace GPlatesFileIO
{
	/**
	 * A PLATES rotation-format reader is used to read the contents of a PLATES rotation-format
	 * file and parse it into the contents of a feature collection.
	 */
	class PlatesRotationFormatReader
	{
	public:
		/**
		 * Read the PLATES rotation-format file specified by @a fileinfo.
		 *
		 * If the file cannot be opened for reading, an exception of type
		 * ErrorOpeningFileForReadingException will be thrown.
		 */
		static
		File::shared_ref
		read_file(
				const FileInfo &filename,
				GPlatesModel::ModelInterface &model,
				ReadErrorAccumulation &read_errors);
	};
}
#endif  // GPLATES_FILEIO_PLATESROTATIONFORMATREADER_H
