/* $Id$ */


/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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
 
#ifndef GPLATES_FILE_IO_PLATESFORMATUTILS_H
#define GPLATES_FILE_IO_PLATESFORMATUTILS_H

#include "global/unicode.h"

#include "model/FeatureHandle.h"


namespace GPlatesFileIO
{
	namespace PlatesFormatUtils
	{
		/**
		 * Used in the data type code field of PLATES header to indicate an unknown or invalid type.
		 */
		const GPlatesUtils::UnicodeString INVALID_DATA_TYPE_CODE = "XX";


		/**
		 * Determines the PLATES4 header data type code from the specified feature.
		 *
		 * If the feature cannot be mapped to a plates data type then @a INVALID_DATA_TYPE_CODE
		 * is returned.
		 */
		GPlatesUtils::UnicodeString
		get_plates_data_type_code(
				const GPlatesModel::FeatureHandle::const_weak_ref &feature);
	}
}


#endif // GPLATES_FILE_IO_PLATESFORMATUTILS_H
