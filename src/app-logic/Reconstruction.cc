/* $Id$ */

/**
 * \file 
 * File specific comments.
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

#include "Reconstruction.h"

#include "ReconstructUtils.h"

#include "global/PreconditionViolationError.h"


GPlatesAppLogic::Reconstruction::Reconstruction(
		const double &reconstruction_time,
		const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree) :
	d_reconstruction_time(reconstruction_time),
	d_default_reconstruction_tree(reconstruction_tree)
{
}


GPlatesAppLogic::Reconstruction::Reconstruction(
		const double &reconstruction_time,
		GPlatesModel::integer_plate_id_type anchored_plate_id) :
	d_reconstruction_time(reconstruction_time),
	// Create an empty default reconstruction tree...
	d_default_reconstruction_tree(
			ReconstructUtils::create_reconstruction_tree(
					reconstruction_time,
					anchored_plate_id))
{
}


GPlatesAppLogic::Reconstruction::~Reconstruction()
{
	// Tell all ReconstructionGeometryCollection's, which currently point to this
	// Reconstruction instance, to set those pointers to NULL,
	// lest they become dangling pointers.

	reconstruction_tree_map_type::iterator reconstruction_geom_collection_iter =
			d_reconstruction_tree_map.begin();
	reconstruction_tree_map_type::iterator reconstruction_geom_collection_end =
			d_reconstruction_tree_map.end();
	for ( ;
		reconstruction_geom_collection_iter != reconstruction_geom_collection_end;
		++reconstruction_geom_collection_iter)
	{
		reconstruction_geom_collection_iter->second->set_reconstruction_ptr(NULL);
	}
}


void
GPlatesAppLogic::Reconstruction::add_reconstruction_geometries(
		const GPlatesAppLogic::ReconstructionGeometryCollection::non_null_ptr_to_const_type &
				reconstruction_geom_collection)
{
	if (d_reconstruction_time != reconstruction_geom_collection->get_reconstruction_time())
	{
		throw GPlatesGlobal::PreconditionViolationError(GPLATES_EXCEPTION_SOURCE);
	}

	// Map the reconstruction tree to the reconstruction geometry collection.
	d_reconstruction_tree_map.insert(
			std::make_pair(
					reconstruction_geom_collection->reconstruction_tree(),
					reconstruction_geom_collection));

	reconstruction_geom_collection->set_reconstruction_ptr(this);
}


GPlatesAppLogic::Reconstruction::ConstReconstructionGeometryIterator
GPlatesAppLogic::Reconstruction::ConstReconstructionGeometryIterator::create_begin(
		const Reconstruction &reconstruction,
		const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree)
{
	reconstruction_tree_map_type::const_iterator lower_bound_iter =
			reconstruction.d_reconstruction_tree_map.lower_bound(reconstruction_tree);

	if (lower_bound_iter == reconstruction.d_reconstruction_tree_map.end())
	{
		return ConstReconstructionGeometryIterator(
				&reconstruction,
				lower_bound_iter);
	}
	return ConstReconstructionGeometryIterator(
			&reconstruction,
			lower_bound_iter,
			lower_bound_iter->second->begin());
}


GPlatesAppLogic::Reconstruction::ConstReconstructionGeometryIterator
GPlatesAppLogic::Reconstruction::ConstReconstructionGeometryIterator::create_end(
		const Reconstruction &reconstruction,
		const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree)
{
	reconstruction_tree_map_type::const_iterator upper_bound_iter =
			reconstruction.d_reconstruction_tree_map.upper_bound(reconstruction_tree);

	if (upper_bound_iter == reconstruction.d_reconstruction_tree_map.end())
	{
		return ConstReconstructionGeometryIterator(
				&reconstruction,
				upper_bound_iter);
	}
	return ConstReconstructionGeometryIterator(
			&reconstruction,
			upper_bound_iter,
			upper_bound_iter->second->begin());
}


const GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type
GPlatesAppLogic::Reconstruction::ConstReconstructionGeometryIterator::operator*() const
{
	// The caller shouldn't be trying to dereference an invalid iterator
	// so we will assume valid internal iterators.
	return *d_reconstruction_geometry_collection_iterator.get();
}


GPlatesAppLogic::Reconstruction::ConstReconstructionGeometryIterator &
GPlatesAppLogic::Reconstruction::ConstReconstructionGeometryIterator::operator++()
{
	// The caller shouldn't be trying to increment an invalid iterator
	// so we will assume valid internal iterators.

	// If we've reached the end of one ReconstructionGeometryCollection then
	// move to the beginning of the next ReconsructionGeometryCollection.
	++d_reconstruction_geometry_collection_iterator.get();
	if (d_reconstruction_geometry_collection_iterator ==
		d_reconstruction_tree_map_iterator->second->end())
	{
		// See if we were at the last ReconstructionGeometryCollection in the map.
		++d_reconstruction_tree_map_iterator;
		if (d_reconstruction_tree_map_iterator !=
			d_reconstruction->d_reconstruction_tree_map.end())
		{
			d_reconstruction_geometry_collection_iterator =
					d_reconstruction_tree_map_iterator->second->begin();
		}
		else
		{
			d_reconstruction_geometry_collection_iterator = boost::none;
		}
	}

	return *this;
}


GPlatesAppLogic::Reconstruction::ConstReconstructionGeometryIterator::ConstReconstructionGeometryIterator(
		const Reconstruction *reconstruction,
		reconstruction_tree_map_type::const_iterator reconstruction_tree_map_iterator,
		boost::optional<GPlatesAppLogic::ReconstructionGeometryCollection::const_iterator>
				reconstruction_geometry_collection_iterator) :
	d_reconstruction(reconstruction),
	d_reconstruction_tree_map_iterator(reconstruction_tree_map_iterator),
	d_reconstruction_geometry_collection_iterator(reconstruction_geometry_collection_iterator)
{
}
