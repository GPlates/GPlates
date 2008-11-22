/* $Id$ */

/**
 * @file 
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006, 2007 The University of Sydney, Australia
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

#include <map>

#include "FileInfo.h"
#include "ErrorOpeningFileForReadingException.h"
#include "ReadErrorAccumulation.h"
#include "model/ModelInterface.h"
#include "model/FeatureCollectionHandle.h"

namespace GPlatesFileIO {

	//typedef std::map<std::string, GPlatesModel::FeatureId> old_id_to_new_id_map_type;
	//typedef old_id_to_new_id_map_type::const_iterator old_id_to_new_id_map_const_iterator;

	class PlatesLineFormatReader
	{
	public:
		/**
		 * Read the PLATES line-format file specified by @a fileinfo.
		 *
		 * If the file cannot be opened for reading, an exception of type
		 * ErrorOpeningFileForReadingException will be thrown.
		 */
		static 
		void
		read_file(
				FileInfo &fileinfo,
				GPlatesModel::ModelInterface &model,
				ReadErrorAccumulation &read_errors);


	private:

		//old_id_to_new_id_map_type d_id_map;

	};
	
}

#endif // GPLATES_FILEIO_PLATESLINEFORMATREADER_H
