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

#include <QString>
#include <map>
#include "model/FeatureType.h"
#include "PropertyCreationUtils.h"

namespace GPlatesFileIO {

	/**
	 * This class encapsulates a mapping from a (fully qualified) feature
	 * type name to a mapping from the properties allowed in the feature to
	 * creation functions for the properties.
	 *
	 *   feature type name  ----->  ( property p  ----->  creation_function for p )
	 */
	class FeaturePropertiesMap
	{
			typedef std::map<
					GPlatesModel::FeatureType, 
					PropertyCreationUtils::PropertyCreatorMap> 
				FeaturePropertiesMapType;

		public:
			typedef FeaturePropertiesMapType::const_iterator const_iterator;

			static
			FeaturePropertiesMap *
			instance();


			const_iterator
			find(const GPlatesModel::FeatureType &key) const
			{
				return d_map.find(key);
			}
			

			const_iterator
			end() const
			{
				return d_map.end();
			}


		private:
			FeaturePropertiesMapType d_map;
			static FeaturePropertiesMap *d_instance;

			FeaturePropertiesMap();
			FeaturePropertiesMap(
					const FeaturePropertiesMap &);
			FeaturePropertiesMap &
			operator=(const FeaturePropertiesMap &);
	};
}

#endif // GPLATES_FILEIO_FEATUREPROPERTIESMAP_H
