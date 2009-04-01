/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
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

#include <boost/shared_ptr.hpp>
#include <boost/cast.hpp>
#include <boost/none.hpp>
#include <boost/bind.hpp>
#include <iterator>

#include "GeometryBuilder.h"
#include "InternalGeometryBuilder.h"

#include "maths/PointOnSphere.h"

#include "global/GPlatesAssert.h"
#include "global/AssertionFailureException.h"

namespace GPlatesViewOperations
{
	namespace GeometryBuilderInternal
	{
		class UndoImplInterface
		{
		public:
			virtual
			~UndoImplInterface()
			{  }

			virtual
			void
			accept_undo_visitor(
					GPlatesViewOperations::GeometryBuilder *) = 0;
		};

		class InsertPointUndoImpl :
			public UndoImplInterface
		{
		public:
			InsertPointUndoImpl(
					GeometryBuilder::PointIndex point_index) :
			d_point_index(point_index)
			{  }

			virtual
			void
			accept_undo_visitor(
					GPlatesViewOperations::GeometryBuilder* geometry_builder)
			{
				geometry_builder->visit_undo_operation(*this);
			}

			GeometryBuilder::PointIndex d_point_index;
		};

		class RemovePointUndoImpl :
			public UndoImplInterface
		{
		public:
			RemovePointUndoImpl(
					GeometryBuilder::PointIndex point_index,
					const GPlatesMaths::PointOnSphere &point) :
			d_point_index(point_index),
			d_point(point)
			{  }

			virtual
			void
			accept_undo_visitor(
					GPlatesViewOperations::GeometryBuilder* geometry_builder)
			{
				geometry_builder->visit_undo_operation(*this);
			}

			GeometryBuilder::PointIndex d_point_index;
			GPlatesMaths::PointOnSphere d_point;
		};

		class MovePointUndoImpl :
			public UndoImplInterface
		{
		public:
			MovePointUndoImpl(
					GeometryBuilder::PointIndex point_index,
					const GPlatesMaths::PointOnSphere &old_point) :
			d_point_index(point_index),
			d_old_point(old_point)
			{  }

			virtual
			void
			accept_undo_visitor(
					GPlatesViewOperations::GeometryBuilder* geometry_builder)
			{
				geometry_builder->visit_undo_operation(*this);
			}


			GeometryBuilder::PointIndex d_point_index;
			GPlatesMaths::PointOnSphere d_old_point;
		};

		class SetGeometryTypeUndoImpl :
			public UndoImplInterface
		{
		public:
			SetGeometryTypeUndoImpl(
					GeometryType::Value prev_geom_type) :
			d_prev_geom_type(prev_geom_type)
			{  }

			virtual
			void
			accept_undo_visitor(
					GPlatesViewOperations::GeometryBuilder* geometry_builder)
			{
				geometry_builder->visit_undo_operation(*this);
			}

			GeometryType::Value d_prev_geom_type;
		};

		class ClearAllGeometriesUndoImpl :
			public UndoImplInterface
		{
		public:
			ClearAllGeometriesUndoImpl(
					GeometryBuilder::GeometryIndex prev_current_geom_index) :
			d_prev_current_geom_index(prev_current_geom_index)
			{  }

			virtual
			void
			accept_undo_visitor(
					GPlatesViewOperations::GeometryBuilder* geometry_builder)
			{
				geometry_builder->visit_undo_operation(*this);
			}

			//! Typedef for a sequence of geometries.
			typedef std::vector<InternalGeometryBuilder::point_seq_type>
				geometry_seq_type;

			GeometryBuilder::GeometryIndex d_prev_current_geom_index;
			geometry_seq_type d_geometry_seq;
		};

		class InsertGeometryUndoImpl :
			public UndoImplInterface
		{
		public:
			InsertGeometryUndoImpl(	
					GeometryBuilder::GeometryIndex geom_index) :
			d_geom_index(geom_index)
			{  }

			virtual
			void
			accept_undo_visitor(
					GPlatesViewOperations::GeometryBuilder* geometry_builder)
			{
				geometry_builder->visit_undo_operation(*this);
			}

			GeometryBuilder::GeometryIndex d_geom_index;
		};

		class CompositeUndoImpl :
			public UndoImplInterface
		{
		public:
			/**
			 * Typedef for a sequence of @a UndoOperation objects.
			 */
			typedef std::vector<GeometryBuilder::UndoOperation> undo_operation_seq_type;

			CompositeUndoImpl(
					undo_operation_seq_type undo_operation_seq) :
			d_undo_operation_seq(undo_operation_seq)
			{  }

			virtual
			void
			accept_undo_visitor(
					GPlatesViewOperations::GeometryBuilder* geometry_builder)
			{
				// Call each undo operation on the GeometryBuilder.
				// Undo in the reverse order.
				std::for_each(
						d_undo_operation_seq.rbegin(),
						d_undo_operation_seq.rend(),
						boost::bind(
								&GeometryBuilder::undo,
								geometry_builder,
								_1)
						);
			}

			undo_operation_seq_type d_undo_operation_seq;
		};

		typedef boost::shared_ptr<UndoImplInterface> UndoImpl;
	}
}

GPlatesViewOperations::GeometryBuilder::GeometryBuilder() :
d_geometry_build_type(GeometryType::NONE),
d_current_geometry_index(DEFAULT_GEOMETRY_INDEX),
d_update_geometry_depth(0)
{
	// No methods that emit signals can be called here in constructor because the
	// signals will not yet be connected to anything and will be lost.
}

GPlatesViewOperations::GeometryBuilder::UndoOperation
GPlatesViewOperations::GeometryBuilder::clear_all_geometries()
{
	// This gets put in all public methods that modify geometry state.
	// It checks for geometry type changes and emits begin_update/end_update signals.
	UpdateGuard update_guard(*this);

	// Remove all geometries and set current geometry index to default index.

	// We'll be copying the removed geometries into our undo operation.
	std::auto_ptr<GeometryBuilderInternal::ClearAllGeometriesUndoImpl> undo_operation(
			new GeometryBuilderInternal::ClearAllGeometriesUndoImpl(
					d_current_geometry_index));

	if (d_current_geometry_index != DEFAULT_GEOMETRY_INDEX)
	{
		d_current_geometry_index = DEFAULT_GEOMETRY_INDEX;

		emit changed_current_geometry_index(DEFAULT_GEOMETRY_INDEX);
	}

	// Resize space for geometries to be copied to undo operation.
	undo_operation->d_geometry_seq.resize(d_geometry_builder_seq.size());

	// Traverse geometries in reverse order to make less work for our clients
	// (because they'll be erasing at the end of their geometry sequence).
	for (int geom_index = boost::numeric_cast<int>(d_geometry_builder_seq.size());
		--geom_index >= 0;
		)
	{
		// Transfer geometry to undo operation before removing it.
		undo_operation->d_geometry_seq[geom_index].swap(
				d_geometry_builder_seq[geom_index]->get_point_seq());

		remove_geometry(geom_index);
	}

	return boost::any( GeometryBuilderInternal::UndoImpl(undo_operation.release()) );
}

GPlatesViewOperations::GeometryBuilder::UndoOperation
GPlatesViewOperations::GeometryBuilder::set_geometry_type_to_build(
		GeometryType::Value geom_type)
{
	// This gets put in all public methods that modify geometry state.
	// It checks for geometry type changes and emits begin_update/end_update signals.
	UpdateGuard update_guard(*this);

	const GeometryType::Value prev_geom_build_type = d_geometry_build_type;

	d_geometry_build_type = geom_type;

	// Iterate through all geometries.
	for (geometry_builder_seq_type::size_type geom_index = 0;
		geom_index < d_geometry_builder_seq.size();
		++geom_index)
	{
		geometry_builder_ptr_type geometry_builder_ptr = d_geometry_builder_seq[geom_index];

		geometry_builder_ptr->set_desired_geometry_type(d_geometry_build_type);

		++geom_index;
	}

	return boost::any( GeometryBuilderInternal::UndoImpl(
			new GeometryBuilderInternal::SetGeometryTypeUndoImpl(prev_geom_build_type)) );
}

GPlatesViewOperations::GeometryBuilder::UndoOperation
GPlatesViewOperations::GeometryBuilder::insert_point_into_current_geometry(
		PointIndex point_index,
		const GPlatesMaths::PointOnSphere &oriented_pos_on_globe)
{
	// This gets put in all public methods that modify geometry state.
	// It checks for geometry type changes and emits begin_update/end_update signals.
	UpdateGuard update_guard(*this);

	// If we don't have any geometries then create one.
	if (d_geometry_builder_seq.empty())
	{
		insert_geometry(0);
	}

	InternalGeometryBuilder &geometry = get_current_geometry_builder();

	GPlatesGlobal::Assert(point_index <= geometry.get_point_seq_const().size(),
		GPlatesGlobal::AssertionFailureException(__FILE__, __LINE__));

	// Insert point into current geometry builder.
	InternalGeometryBuilder::point_seq_type::iterator insert_iter =
		geometry.get_point_seq().begin();
	std::advance(insert_iter, point_index);
	geometry.get_point_seq().insert(insert_iter, oriented_pos_on_globe);

	emit inserted_point_into_current_geometry(point_index, oriented_pos_on_globe);

	return boost::any( GeometryBuilderInternal::UndoImpl(
			new GeometryBuilderInternal::InsertPointUndoImpl(point_index)) );
}

GPlatesViewOperations::GeometryBuilder::UndoOperation
GPlatesViewOperations::GeometryBuilder::remove_point_from_current_geometry(
		PointIndex point_index)
{
	// This gets put in all public methods that modify geometry state.
	// It checks for geometry type changes and emits begin_update/end_update signals.
	UpdateGuard update_guard(*this);

	InternalGeometryBuilder &geometry = get_current_geometry_builder();

	GPlatesGlobal::Assert(point_index < geometry.get_point_seq_const().size(),
		GPlatesGlobal::AssertionFailureException(__FILE__, __LINE__));

	InternalGeometryBuilder::point_seq_type::iterator erase_iter =
		geometry.get_point_seq().begin();

	std::advance(erase_iter, point_index);

	// Keep copy of point to be removed in case we need to undo.
	GPlatesMaths::PointOnSphere removed_point(*erase_iter);

	geometry.get_point_seq().erase(erase_iter);

	emit removed_point_from_current_geometry(point_index);

	// If no points left in geometry then remove it.
	if (geometry.get_point_seq().empty())
	{
		remove_geometry(get_current_geometry_index());
	}

	return boost::any( GeometryBuilderInternal::UndoImpl(
			new GeometryBuilderInternal::RemovePointUndoImpl(
					point_index, removed_point)) );
}

GPlatesViewOperations::GeometryBuilder::UndoOperation
GPlatesViewOperations::GeometryBuilder::move_point_in_current_geometry(
		PointIndex point_index,
		const GPlatesMaths::PointOnSphere &new_oriented_pos_on_globe,
		bool is_intermediate_move)
{
	// This gets put in all public methods that modify geometry state.
	// It checks for geometry type changes and emits begin_update/end_update signals.
	UpdateGuard update_guard(*this, is_intermediate_move);

	InternalGeometryBuilder &geometry = get_current_geometry_builder();

	GPlatesGlobal::Assert(point_index <= geometry.get_point_seq_const().size(),
		GPlatesGlobal::AssertionFailureException(__FILE__, __LINE__));

	// Move point in the current geometry builder.
	InternalGeometryBuilder::point_seq_type::iterator move_iter =
		geometry.get_point_seq().begin();
	std::advance(move_iter, point_index);

	GPlatesMaths::PointOnSphere old_oriented_pos_on_globe = *move_iter;
	*move_iter = new_oriented_pos_on_globe;

	emit moved_point_in_current_geometry(
			point_index, new_oriented_pos_on_globe, is_intermediate_move);

	return boost::any( GeometryBuilderInternal::UndoImpl(
			new GeometryBuilderInternal::MovePointUndoImpl(
					point_index, old_oriented_pos_on_globe)) );
}

const GPlatesViewOperations::InternalGeometryBuilder&
GPlatesViewOperations::GeometryBuilder::get_current_geometry_builder() const
{
	GPlatesGlobal::Assert(d_current_geometry_index < d_geometry_builder_seq.size(),
		GPlatesGlobal::AssertionFailureException(__FILE__, __LINE__));

	return *d_geometry_builder_seq[d_current_geometry_index];
}

GPlatesViewOperations::GeometryBuilder::UndoOperation
GPlatesViewOperations::GeometryBuilder::insert_geometry(
		GeometryIndex geom_index)
{
	// Create new geometry builder.
	geometry_builder_ptr_type geometry_ptr(
		new InternalGeometryBuilder(this, d_geometry_build_type));

	return insert_geometry(geometry_ptr, geom_index);
}

GPlatesViewOperations::GeometryBuilder::UndoOperation
GPlatesViewOperations::GeometryBuilder::insert_geometry(
		geometry_builder_ptr_type geometry_ptr,
		GeometryIndex geom_index)
{
	GPlatesGlobal::Assert(geom_index <= d_geometry_builder_seq.size(),
		GPlatesGlobal::AssertionFailureException(__FILE__, __LINE__));

	// Determine where to insert new geometry into geometry sequence.
	geometry_builder_seq_type::iterator insert_iter = d_geometry_builder_seq.begin();
	std::advance(insert_iter, geom_index);

	// Insert new geometry.
	d_geometry_builder_seq.insert(insert_iter, geometry_ptr);

	emit inserted_geometry(geom_index);

	// If geometry was inserted before the current geometry then
	// change the current geometry index.
	if (d_geometry_builder_seq.size() > 1 && geom_index <= d_current_geometry_index)
	{
		++d_current_geometry_index;
		emit changed_current_geometry_index(d_current_geometry_index);
	}

	return boost::any( GeometryBuilderInternal::UndoImpl(
			new GeometryBuilderInternal::InsertGeometryUndoImpl(geom_index)) );
}

void
GPlatesViewOperations::GeometryBuilder::remove_geometry(
		GeometryIndex geom_index)
{
	GPlatesGlobal::Assert(geom_index < d_geometry_builder_seq.size(),
		GPlatesGlobal::AssertionFailureException(__FILE__, __LINE__));

	geometry_builder_seq_type::iterator erase_iter = d_geometry_builder_seq.begin();
	std::advance(erase_iter, geom_index);

	d_geometry_builder_seq.erase(erase_iter);

	emit removed_geometry(geom_index);

	// If geometry was erased at or before the current geometry then
	// change the current geometry index.
	if (geom_index <= d_current_geometry_index)
	{
		// If current geometry index is zero then leave it at zero
		// even though we might not have any geometries - as soon as
		// a point is added again then a new geometry will be created.
		if (d_current_geometry_index > 0)
		{
			--d_current_geometry_index;
			emit changed_current_geometry_index(d_current_geometry_index);
		}
	}
}

void
GPlatesViewOperations::GeometryBuilder::undo(
		UndoOperation &undo_memento)
{
	// This gets put in all public methods that modify geometry state.
	// It checks for geometry type changes and emits begin_update/end_update signals.
	UpdateGuard update_guard(*this);

	// Convert from opaque type to abstract UndoImplInterface.
	GeometryBuilderInternal::UndoImpl* undo_impl =
			boost::any_cast<GeometryBuilderInternal::UndoImpl>(&undo_memento);

	GPlatesGlobal::Assert(undo_impl != NULL,
		GPlatesGlobal::AssertionFailureException(__FILE__, __LINE__));

	// Perform the undo operation.
	(*undo_impl)->accept_undo_visitor(this);

	// Clear the client's undo data since we don't need it anymore.
	undo_impl->reset();
}

void
GPlatesViewOperations::GeometryBuilder::visit_undo_operation(
		GeometryBuilderInternal::InsertPointUndoImpl &insert_point_undo)
{
	const PointIndex inserted_point_index = insert_point_undo.d_point_index;

	// Ignore returned UndoOperation.
	remove_point_from_current_geometry(inserted_point_index);
}

void
GPlatesViewOperations::GeometryBuilder::visit_undo_operation(
		GeometryBuilderInternal::RemovePointUndoImpl &remove_point_undo)
{
	const PointIndex removed_point_index = remove_point_undo.d_point_index;
	const GPlatesMaths::PointOnSphere &removed_point = remove_point_undo.d_point;

	// Ignore returned UndoOperation.
	insert_point_into_current_geometry(removed_point_index, removed_point);
}

void
GPlatesViewOperations::GeometryBuilder::visit_undo_operation(
		GeometryBuilderInternal::MovePointUndoImpl &move_point_undo)
{
	const PointIndex moved_point_index = move_point_undo.d_point_index;
	const GPlatesMaths::PointOnSphere &old_point = move_point_undo.d_old_point;

	// Ignore returned UndoOperation.
	move_point_in_current_geometry(moved_point_index, old_point);
}

void
GPlatesViewOperations::GeometryBuilder::visit_undo_operation(
		GeometryBuilderInternal::SetGeometryTypeUndoImpl &set_geom_type_undo)
{
	const GeometryType::Value prev_geom_type = set_geom_type_undo.d_prev_geom_type;

	// Ignore returned UndoOperation.
	set_geometry_type_to_build(prev_geom_type);
}

void
GPlatesViewOperations::GeometryBuilder::visit_undo_operation(
		GeometryBuilderInternal::ClearAllGeometriesUndoImpl &clear_all_geoms_undo)
{
	d_current_geometry_index = clear_all_geoms_undo.d_prev_current_geom_index;

	// If we're undoing a clear all geometries then there should be none initially.
	GPlatesGlobal::Assert(d_geometry_builder_seq.empty(),
		GPlatesGlobal::AssertionFailureException(__FILE__, __LINE__));

	// Iterate through all the geometries to be restored.
	geometry_builder_seq_type::size_type geom_index;
	for (geom_index = 0;
		geom_index < clear_all_geoms_undo.d_geometry_seq.size();
		++geom_index)
	{
		// Insert geometry and copy points to it.
		insert_geometry(
				geom_index,
				clear_all_geoms_undo.d_geometry_seq[geom_index].begin(),
				clear_all_geoms_undo.d_geometry_seq[geom_index].end());
	}
}

void
GPlatesViewOperations::GeometryBuilder::visit_undo_operation(
		GeometryBuilderInternal::InsertGeometryUndoImpl &insert_geom_undo)
{
	// If we're undoing a geometry insertion then there should be some geometry(s) initially.
	GPlatesGlobal::Assert(!d_geometry_builder_seq.empty(),
		GPlatesGlobal::AssertionFailureException(__FILE__, __LINE__));

	const GeometryIndex geom_index_to_remove = insert_geom_undo.d_geom_index;

	// Ignore returned undo operation.
	remove_geometry(geom_index_to_remove);
}

GPlatesViewOperations::GeometryType::Value
GPlatesViewOperations::GeometryBuilder::get_actual_type_of_current_geometry() const
{
	const GeometryIndex current_geom_index = get_current_geometry_index();

	return get_actual_type_of_geometry(current_geom_index);
}

GPlatesViewOperations::GeometryType::Value
GPlatesViewOperations::GeometryBuilder::get_actual_type_of_geometry(
		GeometryIndex geom_index) const
{
	GPlatesGlobal::Assert(geom_index < d_geometry_builder_seq.size(),
		GPlatesGlobal::AssertionFailureException(__FILE__, __LINE__));

	if (d_geometry_builder_seq.empty())
	{
		return GeometryType::NONE;
	}

	const InternalGeometryBuilder &geometry = get_current_geometry_builder();

	return geometry.update(), geometry.get_actual_geometry_type();
}

GPlatesViewOperations::GeometryBuilder::geometry_opt_ptr_type
GPlatesViewOperations::GeometryBuilder::get_geometry_on_sphere()
{
	// Until multiple geometries are supported (ie can be returned in a
	// single GeometryOnSphere type) then make sure have only zero or one geometry.
	GPlatesGlobal::Assert(d_geometry_builder_seq.size() <= 1,
		GPlatesGlobal::AssertionFailureException(__FILE__, __LINE__));

	// If we don't have any geometries then return none.
	if (d_geometry_builder_seq.empty())
	{
		return boost::none;
	}

	InternalGeometryBuilder &geometry = get_current_geometry_builder();

	return geometry.update(), geometry.get_geometry_on_sphere();
}

unsigned int
GPlatesViewOperations::GeometryBuilder::get_num_geometries() const
{
	return d_geometry_builder_seq.size();
}

const GPlatesMaths::PointOnSphere&
GPlatesViewOperations::GeometryBuilder::get_geometry_point(
		GeometryIndex geom_index,
		PointIndex point_index) const
{
	GPlatesGlobal::Assert(
		geom_index < d_geometry_builder_seq.size(),
		GPlatesGlobal::AssertionFailureException(__FILE__, __LINE__));

	GPlatesGlobal::Assert(
		point_index < d_geometry_builder_seq[geom_index]->get_point_seq_const().size(),
		GPlatesGlobal::AssertionFailureException(__FILE__, __LINE__));

	return d_geometry_builder_seq[geom_index]->get_point_seq_const()[point_index];
}

GPlatesViewOperations::GeometryBuilder::point_const_iterator_type
GPlatesViewOperations::GeometryBuilder::get_geometry_point_begin(
		GeometryIndex geom_index) const
{
	GPlatesGlobal::Assert(
		geom_index < d_geometry_builder_seq.size(),
		GPlatesGlobal::AssertionFailureException(__FILE__, __LINE__));

	return d_geometry_builder_seq[geom_index]->get_point_seq_const().begin();
}

GPlatesViewOperations::GeometryBuilder::point_const_iterator_type
GPlatesViewOperations::GeometryBuilder::get_geometry_point_end(
		GeometryIndex geom_index) const
{
	GPlatesGlobal::Assert(
		geom_index < d_geometry_builder_seq.size(),
		GPlatesGlobal::AssertionFailureException(__FILE__, __LINE__));

	return d_geometry_builder_seq[geom_index]->get_point_seq_const().end();
}

unsigned int
GPlatesViewOperations::GeometryBuilder::get_num_points_in_current_geometry() const
{
	if (d_geometry_builder_seq.empty())
	{
		return 0;
	}

	const InternalGeometryBuilder &geometry = get_current_geometry_builder();

	return geometry.get_point_seq_const().size();
}

unsigned int
GPlatesViewOperations::GeometryBuilder::get_num_points_in_geometry(
		GeometryIndex geom_index) const
{
	GPlatesGlobal::Assert(
		geom_index < d_geometry_builder_seq.size(),
		GPlatesGlobal::AssertionFailureException(__FILE__, __LINE__));

	const InternalGeometryBuilder &geometry = *d_geometry_builder_seq[geom_index];

	return geometry.get_point_seq_const().size();
}

GPlatesViewOperations::GeometryBuilder::GeometryIndex
GPlatesViewOperations::GeometryBuilder::get_current_geometry_index() const
{
	return d_current_geometry_index;
}

void
GPlatesViewOperations::GeometryBuilder::begin_update_geometry(
		bool is_intermediate_move)
{
	if (d_update_geometry_depth == 0)
	{
		emit started_updating_geometry();

		if (!is_intermediate_move)
		{
			// Some clients are only interested in knowing about operations that
			// are not intermediate moves. This significantly reduces the number
			// of notifications they get when the user is dragging vertices.
			emit started_updating_geometry_excluding_intermediate_moves();
		}
	}

	// Increment nested call depth.
	++d_update_geometry_depth;
}

void
GPlatesViewOperations::GeometryBuilder::end_update_geometry(
		bool is_intermediate_move)
{
	// Decrement nested call depth.
	--d_update_geometry_depth;

	// If we've not reached outermost end_update_geometry call then do nothing.
	if (d_update_geometry_depth > 0)
	{
		return;
	}

	// Iterate through our geometries and see if any actual geometry types
	// have changed.
	geometry_builder_seq_type::size_type geometry_index;
	for (geometry_index = 0;
		geometry_index < d_geometry_builder_seq.size();
		++geometry_index)
	{
		geometry_builder_ptr_type geom_builder_ptr = d_geometry_builder_seq[geometry_index];

		// Get initial geometry type.
		const GeometryType::Value initial_geom_type =
			geom_builder_ptr->get_actual_geometry_type();

		// Update geometry if it needs to be.
		geom_builder_ptr->update();

		// Get final geometry type.
		const GeometryType::Value final_geom_type =
			geom_builder_ptr->get_actual_geometry_type();

		// Emit a signal if they differ.
		if (final_geom_type != initial_geom_type)
		{
			emit changed_actual_geometry_type(geometry_index, final_geom_type);
		}
	}

	//
	// Notify observers that we've stopped updating geometry.
	//
	emit stopped_updating_geometry();

	if (!is_intermediate_move)
	{
		// Some clients are only interested in knowing about operations that
		// are not intermediate moves. This significantly reduces the number
		// of notifications they get when the user is dragging vertices.
		emit stopped_updating_geometry_excluding_intermediate_moves();
	}
}

GPlatesViewOperations::GeometryBuilder::UndoOperation
GPlatesViewOperations::GeometryBuilder::create_composite_undo_operation(
		undo_operation_seq_type undo_operation_seq)
{
	return boost::any( GeometryBuilderInternal::UndoImpl(
			new GeometryBuilderInternal::CompositeUndoImpl(undo_operation_seq)) );
}

GPlatesViewOperations::GeometryBuilder::UpdateGuard::UpdateGuard(
		GeometryBuilder &geometry_builder,
		bool is_intermediate_move) :
d_geometry_builder(geometry_builder),
d_is_intermediate_move(is_intermediate_move)
{
	d_geometry_builder.begin_update_geometry(d_is_intermediate_move);
}


GPlatesViewOperations::GeometryBuilder::UpdateGuard::~UpdateGuard()
{
	// Since this is a destructor we cannot let any exceptions escape.
	// If one is thrown we just have to lump it and continue on.
	try
	{
		d_geometry_builder.end_update_geometry(d_is_intermediate_move);
	}
	catch (...)
	{
	}
}
