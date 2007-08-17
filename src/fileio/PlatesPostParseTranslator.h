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

#ifndef GPLATES_FILEIO_PLATESPOSTPARSETRANSLATOR
#define GPLATES_FILEIO_PLATESPOSTPARSETRANSLATOR

#include "model/ModelInterface.h"
#include "PlatesBoundaryParser.h"
#include "PlatesRotationParser.h"
#include "ReadErrorAccumulation.h"

#include <vector>

namespace GPlatesFileIO
{
	/**
	 * Contains the function that translates from the data structure
	 * output by the PLATES parser into the internal GPlates data
	 * structure.
	 */
	namespace PlatesPostParseTranslator
	{
		GPlatesModel::FeatureCollectionHandle::weak_ref
		get_features_from_plates_data(
				GPlatesModel::ModelInterface &model,
				const PlatesParser::PlatesDataMap &map,
				const std::string &filename,
				ReadErrorAccumulation &errors);

		GPlatesModel::FeatureCollectionHandle::weak_ref
		get_rotation_sequences_from_plates_data(
				GPlatesModel::ModelInterface &model,
				const PlatesParser::PlatesRotationData &list,
				const std::string &filename,
				ReadErrorAccumulation &errors);
	}
}

#endif  /* GPLATES_FILEIO_PLATESPOSTPARSETRANSLATOR */
