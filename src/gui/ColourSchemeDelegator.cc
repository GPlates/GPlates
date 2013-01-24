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
			const GPlatesAppLogic::ReconstructionGeometry &reconstruction_geometry)
	{
		boost::optional<GPlatesModel::FeatureHandle::weak_ref> feature_ref =
				GPlatesAppLogic::ReconstructionGeometryUtils::get_feature_ref(
						&reconstruction_geometry);
		if (!feature_ref)
		{
			return NULL;
		}
		else
		{
			return feature_ref.get()->parent_ptr();
		}
	}
}


GPlatesGui::ColourSchemeDelegator::ColourSchemeDelegator(
		const ColourSchemeContainer &colour_scheme_container) :
	d_colour_scheme_container(colour_scheme_container),
	d_global_colour_scheme(
			std::make_pair(
					ColourSchemeCategory::PLATE_ID,
					colour_scheme_container.begin(ColourSchemeCategory::PLATE_ID)->first))
			// Default colour scheme is whichever is the first plate ID colour scheme.
{
	QObject::connect(
			&d_colour_scheme_container,
			SIGNAL(colour_scheme_edited(
					GPlatesGui::ColourSchemeCategory::Type,
					GPlatesGui::ColourSchemeContainer::id_type)),
			this,
			SLOT(handle_colour_scheme_edited(
					GPlatesGui::ColourSchemeCategory::Type,
					GPlatesGui::ColourSchemeContainer::id_type)));
}


void
GPlatesGui::ColourSchemeDelegator::set_colour_scheme(
		ColourSchemeCategory::Type category,
		ColourSchemeContainer::id_type id,
		GPlatesModel::FeatureCollectionHandle::const_weak_ref feature_collection)
{
	colour_scheme_handle colour_scheme = std::make_pair(category, id);
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
		else
		{
			// Overwrite existing entry.
			result.first->second = colour_scheme;
		}

		//colour_scheme_handle inserted = d_special_colour_schemes[feature_collection];
	}
	
	Q_EMIT changed();
}


void
GPlatesGui::ColourSchemeDelegator::unset_colour_scheme(
		GPlatesModel::FeatureCollectionHandle::const_weak_ref feature_collection)
{
	colour_schemes_map_type::iterator iter = d_special_colour_schemes.find(feature_collection);
	if (iter != d_special_colour_schemes.end())
	{
		d_special_colour_schemes.erase(iter);
	}

	Q_EMIT changed();
}


boost::optional<GPlatesGui::ColourSchemeDelegator::colour_scheme_handle>
GPlatesGui::ColourSchemeDelegator::get_colour_scheme(
		GPlatesModel::FeatureCollectionHandle::const_weak_ref feature_collection) const
{
	if (!feature_collection)
	{
		return d_global_colour_scheme;
	}
	else
	{
		typedef colour_schemes_map_type::const_iterator iterator_type;
		iterator_type special_colour_schemes_iter = d_special_colour_schemes.find(feature_collection);

		if (special_colour_schemes_iter == d_special_colour_schemes.end())
		{
			return boost::none;
		}
		else
		{
			return special_colour_schemes_iter->second;
		}
	}
}


boost::optional<GPlatesGui::Colour>
GPlatesGui::ColourSchemeDelegator::get_colour(
		const GPlatesAppLogic::ReconstructionGeometry &reconstruction_geometry) const
{
	// Shortcut if there are no feature collections have special colour schemes.
	if (d_special_colour_schemes.empty())
	{
		return apply_colour_scheme(d_global_colour_scheme, reconstruction_geometry);
	}

	// Work out which feature collection the reconstruction geometry came from, if any.
	GPlatesModel::FeatureCollectionHandle *feature_collection =
		get_feature_collection_from_reconstruction_geometry(reconstruction_geometry);

	// If the reconstruction geometry has no associated feature collection, use the
	// global colour scheme.
	if (!feature_collection)
	{
		return apply_colour_scheme(d_global_colour_scheme, reconstruction_geometry);
	}

	GPlatesModel::FeatureCollectionHandle::const_weak_ref feature_collection_ref =
		feature_collection->reference();

	typedef colour_schemes_map_type::const_iterator iterator_type;
	iterator_type iter = d_special_colour_schemes.find(feature_collection_ref);
	if (iter == d_special_colour_schemes.end())
	{
		return apply_colour_scheme(d_global_colour_scheme, reconstruction_geometry);
	}
	else
	{
		return apply_colour_scheme(iter->second, reconstruction_geometry);
	}
}


boost::optional<GPlatesGui::Colour>
GPlatesGui::ColourSchemeDelegator::apply_colour_scheme(
		const colour_scheme_handle &colour_scheme,
		const GPlatesAppLogic::ReconstructionGeometry &reconstruction_geometry) const
{
	ColourScheme::non_null_ptr_type colour_scheme_ptr = d_colour_scheme_container.get(
			colour_scheme.first, colour_scheme.second).colour_scheme_ptr;
	return colour_scheme_ptr->get_colour(reconstruction_geometry);
}


void
GPlatesGui::ColourSchemeDelegator::handle_colour_scheme_edited(
		GPlatesGui::ColourSchemeCategory::Type category,
		GPlatesGui::ColourSchemeContainer::id_type id)
{
	// We'll emit the changed signal if the colour scheme edited is the global
	// colour scheme, or one of the special feature collection colour schemes.
	if (d_global_colour_scheme.first == category &&
		d_global_colour_scheme.second == id)
	{
		Q_EMIT changed();
		return;
	}

	for (colour_schemes_map_type::const_iterator iter = d_special_colour_schemes.begin();
		iter != d_special_colour_schemes.end(); ++iter)
	{
		const colour_scheme_handle &curr = iter->second;
		if (curr.first == category &&
			curr.second == id)
		{
			Q_EMIT changed();
			return;
		}
	}
}
