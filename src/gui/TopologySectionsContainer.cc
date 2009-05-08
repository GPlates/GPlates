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

#include "TopologySectionsContainer.h"


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
	emit entries_inserted(index, 1);
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
	emit entries_modified(index, index);
}


void
GPlatesGui::TopologySectionsContainer::remove_at(
		const size_type index)
{
qDebug() << "GPlatesGui::TopologySectionsContainer::remove_at() i=" << index;

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
	if (new_index > d_container.size()) {
		new_index = d_container.size();
	}
	
	if (new_index != d_insertion_point) {
		// Do the move.
		d_insertion_point = new_index;
		// Emit signals.
qDebug() << "GPlatesGui::TopologySectionsContainer::move_insertion_point() i=" << new_index;
		emit insertion_point_moved(new_index);
	}// else, no need to move, and no need to emit signals.
}


void
GPlatesGui::TopologySectionsContainer::reset_insertion_point()
{
	// Set to last data item +1 - new data will be appended to the vector.
	move_insertion_point(d_container.size());
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
}


