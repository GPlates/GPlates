/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
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
 
#ifndef GPLATES_MODEL_GPGIMINFO_H
#define GPLATES_MODEL_GPGIMINFO_H

#include <map>
#include <set>
#include <QString>

#include "PropertyName.h"
#include "FeatureType.h"


namespace GPlatesModel
{
	// The following is just code that used to live in CreateFeatureDialog.
	// It is, in no way, a complete GPGIM-querying thing.

	namespace GPGIMInfo
	{
		typedef std::map<PropertyName, QString> geometric_property_name_map_type;

		/**
		 * Returns a map of geometric property names to human-readable names.
		 */
		const geometric_property_name_map_type &
		get_geometric_property_name_map();

		/**
		 * Returns the human-readable name for a geometric @a property_name.
		 */
		QString
		get_geometric_property_name(
				const PropertyName &property_name);



		typedef std::map<PropertyName, bool> geometric_property_timedependency_map_type;

		/**
		 * Returns a map of geometric property names to a boolean value indicating
		 * whether the property should have a time-dependent wrapper.
		 */
		const geometric_property_timedependency_map_type &
		get_geometric_property_timedependency_map();

		/**
		 * Returns whether the given geometric @a property_name should have a
		 * time-dependent wrapper.
		 */
		bool
		expects_time_dependent_wrapper(
				const PropertyName &property_name);



		typedef std::multimap<FeatureType, PropertyName> feature_geometric_property_map_type;

		/**
		 * Returns a map of feature types to property names, indicating what geometric
		 * properties can be associated with each feature type.
		 */
		const feature_geometric_property_map_type &
		get_feature_geometric_property_map();

		/**
		 * Returns whether the @a property_name is a valid geometric property for the
		 * given @a feature_type.
		 */
		bool
		is_valid_geometric_property(
				const FeatureType &feature_type,
				const PropertyName &property_name);



		typedef std::set<FeatureType> feature_set_type;

		/**
		 * Returns a set of feature types.
		 *
		 * Topological feature types are returned if @a topological is true; otherwise
		 * non-topological feature types are returned.
		 */
		const feature_set_type &
		get_feature_set(
				bool topological);

		/**
		 * Returns true if @a feature_type is a topological feature type.
		 */
		bool
		is_topological(
				const FeatureType &feature_type);
	}
}

#endif	// GPLATES_MODEL_GPGIMINFO_H
