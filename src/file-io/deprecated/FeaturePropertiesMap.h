/* $Id$ */

/**
 * @file 
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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

#ifndef GPLATES_FILEIO_FEATUREPROPERTIESMAP_H
#define GPLATES_FILEIO_FEATUREPROPERTIESMAP_H

#include <map>
#include <QString>

#include "GpmlPropertyReaderUtils.h"

#include "model/FeatureType.h"
#include "utils/Singleton.h"

namespace GPlatesFileIO
{
	/**
	 * This class encapsulates a mapping from a (fully qualified) feature
	 * type name to a mapping from the properties allowed in the feature to
	 * creation functions for the properties.
	 *
	 *   feature type name  ----->  ( property p  ----->  creation_function for p )
	 */
	class FeaturePropertiesMap :
			public GPlatesUtils::Singleton<FeaturePropertiesMap>
	{
		GPLATES_SINGLETON_CONSTRUCTOR_DECL( FeaturePropertiesMap )

	private:

		typedef std::map<
				GPlatesModel::FeatureType, 
				GpmlPropertyReaderUtils::PropertyCreatorMap> 
			feature_properties_map_type;

	public:

		typedef feature_properties_map_type::const_iterator const_iterator;

		const_iterator
		find(
				const GPlatesModel::FeatureType &key) const;

		const_iterator
		begin() const;

		const_iterator
		end() const;

		/**
		 * Returns whether @a property_name is a valid property of @a feature_type.
		 */
		bool
		is_valid_property(
				const GPlatesModel::FeatureType &feature_type,
				const GPlatesModel::PropertyName &property_name) const;

	private:

		feature_properties_map_type d_map;
	};
}

#endif // GPLATES_FILEIO_FEATUREPROPERTIESMAP_H
