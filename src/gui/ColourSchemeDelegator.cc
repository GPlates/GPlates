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

#include <utility>

#include "ColourSchemeDelegator.h"

#include "app-logic/ReconstructionGeometryUtils.h"
#include "model/FeatureHandle.h"
#include "model/WeakReferenceCallback.h"


namespace
{
	/**
	 * Removes a weak reference from the special colour schemes map automatically
	 * when the feature collection that it points to gets deactivated.
	 */
	class WeakReferenceRemover :
			public GPlatesModel::WeakReferenceCallback<const GPlatesModel::FeatureCollectionHandle>
	{
	public:

		typedef GPlatesGui::ColourSchemeDelegator::colour_schemes_map_type map_type;
		typedef map_type::iterator iterator_type;

		WeakReferenceRemover(
				map_type &map,
				iterator_type element) :
			d_map(map),
			d_element(element)
		{
		}

		void
		publisher_deactivated(
				const deactivated_event_type &event)
		{
			d_map.erase(d_element);
		}

	private:

		map_type &d_map;
		iterator_type d_element;
	};

	/**
	 * Returns the feature collection that holds the feature from which the
	 * @a reconstruction_geometry was created.
	 *
	 * Returns NULL if there is no such feature collection.
	 */
	GPlatesModel::FeatureCollectionHandle *
	get_feature_collection_from_reconstruction_geometry(
			const GPlatesModel::ReconstructionGeometry &reconstruction_geometry)
	{
		GPlatesModel::FeatureHandle::weak_ref feature_ref;
		if (!GPlatesAppLogic::ReconstructionGeometryUtils::get_feature_ref(
					&reconstruction_geometry, feature_ref))
		{
			return NULL;
		}
		else
		{
			return feature_ref->parent_ptr();
		}
	}
}


GPlatesGui::ColourSchemeDelegator::ColourSchemeDelegator(
		ColourScheme::non_null_ptr_type global_colour_scheme) :
	d_global_colour_scheme(global_colour_scheme)
{
}


void
GPlatesGui::ColourSchemeDelegator::set_colour_scheme(
		ColourScheme::non_null_ptr_type colour_scheme,
		GPlatesModel::FeatureCollectionHandle::const_weak_ref feature_collection)
{
	if (!feature_collection)
	{
		d_global_colour_scheme = colour_scheme;
	}
	else
	{
		typedef colour_schemes_map_type::iterator iterator_type;
		std::pair<iterator_type, bool> result = d_special_colour_schemes.insert(
				std::make_pair(feature_collection, colour_scheme));

		if (result.second)
		{
			// A new entry was inserted into the map. We'll need to attach a callback
			// to the weak-ref.
			const GPlatesModel::FeatureCollectionHandle::const_weak_ref &weak_ref = result.first->first;
			weak_ref.attach_callback(
					new WeakReferenceRemover(
						d_special_colour_schemes,
						result.first));
		}
	}
}


GPlatesGui::ColourScheme::non_null_ptr_type
GPlatesGui::ColourSchemeDelegator::get_colour_scheme(
		GPlatesModel::FeatureCollectionHandle::const_weak_ref feature_collection) const
{
	if (!feature_collection)
	{
		return d_global_colour_scheme;
	}
	else
	{
		colour_schemes_map_type::const_iterator special_colour_schemes_iter =
			d_special_colour_schemes.find(feature_collection);

		// Return the global colour scheme if there is no special colour scheme for
		// this feature collection.
		if (special_colour_schemes_iter == d_special_colour_schemes.end())
		{
			return d_global_colour_scheme;
		}
		else
		{
			return special_colour_schemes_iter->second;
		}
	}
}


boost::optional<GPlatesGui::Colour>
GPlatesGui::ColourSchemeDelegator::get_colour(
		const GPlatesModel::ReconstructionGeometry &reconstruction_geometry) const
{
	// Shortcut if there are no feature collections have special colour schemes.
	if (d_special_colour_schemes.empty())
	{
		return d_global_colour_scheme->get_colour(reconstruction_geometry);
	}

	GPlatesModel::FeatureCollectionHandle *feature_collection =
		get_feature_collection_from_reconstruction_geometry(reconstruction_geometry);

	if (!feature_collection)
	{
		return d_global_colour_scheme->get_colour(reconstruction_geometry);
	}

	GPlatesModel::FeatureCollectionHandle::const_weak_ref feature_collection_ref =
		feature_collection->reference();
	colour_schemes_map_type::const_iterator feature_collection_iter =
		d_special_colour_schemes.find(feature_collection_ref);
	if (feature_collection_iter == d_special_colour_schemes.end())
	{
		return d_global_colour_scheme->get_colour(reconstruction_geometry);
	}
	else
	{
		return feature_collection_iter->second->get_colour(reconstruction_geometry);
	}
}

