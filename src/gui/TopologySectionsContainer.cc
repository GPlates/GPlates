/* $Id$ */

/**
 * \file
 * $Revision$
 * $Date$
 *
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
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

#include "TopologySectionsContainer.h"

#include "app-logic/TopologyInternalUtils.h"


GPlatesGui::TopologySectionsContainer::TopologySectionsContainer():
	d_insertion_point(0)
{  }


GPlatesGui::TopologySectionsContainer::size_type
GPlatesGui::TopologySectionsContainer::size() const
{
	return d_container.size();
}

const GPlatesGui::TopologySectionsContainer::TableRow &
GPlatesGui::TopologySectionsContainer::at(
		const size_type index) const
{
	return d_container.at(index);
}


void
GPlatesGui::TopologySectionsContainer::insert(
		const TableRow &entry)
{
	// All new entries get inserted at the insertion point.
	size_type index = insertion_point();
	d_container.insert(d_container.begin() + index, entry);
	// Which will naturally bump the insertion point down one row.
	move_insertion_point(insertion_point() + 1);
	// Emit signals
	emit entries_inserted(
			index,
			1,
			d_container.begin() + index,
			d_container.begin() + index + 1);
	emit container_changed(*this);
}


void
GPlatesGui::TopologySectionsContainer::update_at(
		const size_type index,
		const TableRow &entry)
{
	if (index >= d_container.size()) {
		return;
	}

	d_container.at(index) = entry;
	// Emit signals.
	emit entry_modified(index);
	emit container_changed(*this);
}


void
GPlatesGui::TopologySectionsContainer::remove_at(
		const size_type index)
{
	if (index >= d_container.size()) {
		return;
	}
	
	d_container.erase(d_container.begin() + index);
	
	// Adjust Insertion Point if necessary.
	if (insertion_point() > index) {
		move_insertion_point(insertion_point() - 1);
	}
	// Emit signals
	emit entry_removed(index);
	emit container_changed(*this);
}


GPlatesGui::TopologySectionsContainer::size_type
GPlatesGui::TopologySectionsContainer::insertion_point() const
{
	return d_insertion_point;
}


void
GPlatesGui::TopologySectionsContainer::move_insertion_point(
		size_type new_index)
{
#if 0
	// Note: It is possible to leave the new insertion index as the size
	// of the container but it's also equivalent to having it as index zero.
	// This is because the topology sections form a polygon and hence form
	// a cycle so it only matters where the sections are relative to each other
	// in the cycle.
	// But to make clients work easier we will set the insertion point to zero
	// in this case so that the insertion point always points to an existing
	// section in the container (rather than one past the last element).
	if (new_index == d_container.size()) 
	{
		new_index = 0;
	}
#endif
	
	if (new_index != d_insertion_point) {
		// Do the move.
		d_insertion_point = new_index;
		// Emit signals.
		emit insertion_point_moved(new_index);
		emit container_changed(*this);
	}// else, no need to move, and no need to emit signals.
}


void
GPlatesGui::TopologySectionsContainer::reset_insertion_point()
{
	// Set to last data item +1 - new data will be appended to the vector.
	move_insertion_point(d_container.size());
}

void
GPlatesGui::TopologySectionsContainer::update_table_from_container()
{
	// Emit signals.
	emit do_update();
	emit container_changed(*this);
}

void
GPlatesGui::TopologySectionsContainer::clear()
{
	// Get rid of underlying data.
	d_container.clear();
	d_insertion_point = 0;
	// Emit signals.
	emit cleared();
	emit insertion_point_moved(0);
	emit container_changed(*this);
}

void
GPlatesGui::TopologySectionsContainer::set_focus_feature_at_index( 
		size_type index )
{
	// Emit signals.
	emit focus_feature_at_index( index );
	emit container_changed(*this);
}

void
GPlatesGui::TopologySectionsContainer::set_container_ptr_in_table( 
	GPlatesGui::TopologySectionsContainer *ptr)
{
	// Emit signals.
	emit container_change( ptr );
	emit container_changed(*this);
}

namespace
{
	/**
	 * "Resolves" the target of a PropertyDelegate to a FeatureHandle::properties_iterator.
	 * Ideally, a PropertyDelegate would be able to uniquely identify a particular property,
	 * regardless of how many times that property appears inside a Feature or how many
	 * in-line properties (an idea which is now deprecated) that property might have.
	 *
	 * In reality, we need a way to go from FeatureId+PropertyName to a properties_iterator,
	 * and we need one now. This function exists to grab the first properties_iterator belonging
	 * to the FeatureHandle (which in turn can be resolved with the @a resolve_feature_id
	 * function above) which matches the supplied PropertyName.
	 *
	 * It returns an invalid iterator if no match is found.
	 */
	GPlatesModel::FeatureHandle::iterator
	find_properties_iterator(
			const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
			const GPlatesModel::PropertyName &property_name)
	{
		if ( ! feature_ref.is_valid())
		{
			// Return invalid properties iterator.
			return GPlatesModel::FeatureHandle::iterator();
		}

		// Iterate through the top level properties; look for the first name that matches.
		GPlatesModel::FeatureHandle::iterator it = feature_ref->begin();
		GPlatesModel::FeatureHandle::iterator end = feature_ref->end();
		for ( ; it != end; ++it)
		{
			// Elements of this properties vector can be NULL pointers.  (See the comment in
			// "model/FeatureRevision.h" for more details.)
			if (/*it.is_valid() &&*/ (*it)->property_name() == property_name)
			{
				return it;
			}
		}

		// No match.
		// Return invalid properties iterator.
		return GPlatesModel::FeatureHandle::iterator();
	}
}


GPlatesGui::TopologySectionsContainer::TableRow::TableRow(
		const GPlatesModel::FeatureId &feature_id,
		const GPlatesModel::PropertyName &geometry_property_name,
		bool reverse_order) :
	d_feature_id(feature_id),
	d_feature_ref(
			GPlatesAppLogic::TopologyInternalUtils::resolve_feature_id(feature_id)),
	// NOTE: 'd_geometry_property' must be declared/initialised after 'd_feature_ref'
	// since it is initialised from it.
	d_geometry_property(
			find_properties_iterator(d_feature_ref, geometry_property_name)),
	d_reverse(reverse_order)
{
}

GPlatesGui::TopologySectionsContainer::TableRow::TableRow(
		const GPlatesModel::FeatureHandle::iterator &geometry_property,
		bool reverse_order) :
	d_feature_id(
			geometry_property.is_still_valid()
					? geometry_property.handle_weak_ref()->feature_id()
					: GPlatesModel::FeatureId()),
	d_feature_ref(
			geometry_property.is_still_valid()
					? geometry_property.handle_weak_ref()
					: GPlatesModel::FeatureHandle::weak_ref()),
	d_geometry_property(geometry_property),
	d_reverse(reverse_order)
{
}
