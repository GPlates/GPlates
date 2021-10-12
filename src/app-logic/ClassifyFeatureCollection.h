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

#ifndef GPLATES_APP_LOGIC_CLASSIFYFEATURECOLLECTION_H
#define GPLATES_APP_LOGIC_CLASSIFYFEATURECOLLECTION_H

#include <bitset>

#include "file-io/File.h"

#include "model/FeatureCollectionHandle.h"


namespace GPlatesAppLogic
{
	namespace ClassifyFeatureCollection
	{
		/**
		 * The types in which a feature collection can be classified.
		 */
		enum ClassificationType
		{
			RECONSTRUCTABLE,
			RECONSTRUCTION,

			NUM_CLASSIFICATION_TYPES // Must be the last enum.
		};


		/**
		 * A std::bitset for testing multiple classification types for
		 * a single feature collection.
		 *
		 * Test with the enums values in @a ClassificationType using classifications_type::test().
		 */
		typedef std::bitset<NUM_CLASSIFICATION_TYPES> classifications_type;


		/**
		 * Returns classification type(s) of feature collection in @a file.
		 *
		 * First looks at the file format and if classification can be determined
		 * from that then the feature collection is not inspected.
		 * Otherwise the features in the feature collection are inspected to determine
		 * classification.
		 */
		classifications_type
		classify_feature_collection(
				const GPlatesFileIO::File &file);


		/**
		 * Returns the classification type(s) of @a feature_collection.
		 */
		classifications_type
		classify_feature_collection(
				const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection);
	}
}

#endif // GPLATES_APP_LOGIC_CLASSIFYFEATURECOLLECTION_H
