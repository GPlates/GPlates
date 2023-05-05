/**
 * Copyright (C) 2023 The University of Sydney, Australia
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

#ifndef GPLATES_DATA_MINING_SCRIBEEXPORTPROPERTYVALUES_H
#define GPLATES_DATA_MINING_SCRIBEEXPORTPROPERTYVALUES_H

#include "GmlDataBlock.h"
#include "GmlDataBlockCoordinateList.h"


/**
 * Scribe export registered classes/types in the 'property-values' source sub-directory.
 *
 * See "ScribeExportRegistration.h" for more details.
 *
 *******************************************************************************
 * WARNING: Changing the string ids will break backward/forward compatibility. *
 *******************************************************************************
 */
#define SCRIBE_EXPORT_PROPERTY_VALUES \
		\
		((GPlatesPropertyValues::GmlDataBlock, \
			"GPlatesPropertyValues::GmlDataBlock")) \
		\
		((GPlatesPropertyValues::GmlDataBlockCoordinateList, \
			"GPlatesPropertyValues::GmlDataBlockCoordinateList")) \
		\


#endif // GPLATES_DATA_MINING_SCRIBEEXPORTPROPERTYVALUES_H
