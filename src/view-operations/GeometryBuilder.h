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

#ifndef GPLATES_VIEWOPERATIONS_GEOMETRYBUILDER_H
#define GPLATES_VIEWOPERATIONS_GEOMETRYBUILDER_H

#include <boost/any.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <vector>
#include <QObject>

#include "InternalGeometryBuilder.h"
#include "maths/PointOnSphere.h"

namespace GPlatesViewOperations
{
	namespace GeometryBuilderInternal
	{
		class InsertPointUndoImpl;
		class RemovePointUndoImpl;
		class MovePointUndoImpl;
		class SetGeometryTypeUndoImpl;
		class ClearAllGeometriesUndoImpl;
		class InsertGeometryUndoImpl;
	}

	class GeometryBuilder :
		public QObject
	{
		Q_OBJECT

	public:
		/**
		* This typedef is used wherever geometry (of some unknown type) is expected.
		*
		* It is a boost::optional because creation of geometry may fail for various reasons.
		*/
		typedef boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>
			geometry_opt_ptr_type;

		//! Geometry index.
		typedef unsigned int GeometryIndex;

		//! Point index within a geometry.
		typedef unsigned int PointIndex;

		//! Const iterator over geometry points.
		typedef InternalGeometryBuilder::point_seq_const_iterator_type
			point_const_iterator_type;

		/**
		 * Opaque undo memento returned by public modifying operations.
		 *
		 * To undo one of these operations you can pass the memento to the @a undo method.
		 * This type encodes the operation and any data needed to undo it.
		 *
		 * Being a memento it is only understand by the implementation of this class.
		 */
		typedef boost::any UndoOperation;

		GeometryBuilder();

		//////////////////////////////////////////////////////////////////////////
		// Interface for modifying the geometry state (with support for undo/redo).
		//////////////////////////////////////////////////////////////////////////

		/**
		 * Specifies the type of geometry the user wants to build.
		 *
		 * The geometry(s) might not satisfy the conditions though.
		 * For example there might be too few points for a polygon.
		 * Each internal geometry might be a different type until
		 * the conditions are met (eg, enough points added).
		 *
		 * This method generates signals.
		 *
		 * @param geom_type type of geometry to build.
		 */
		UndoOperation
		set_geometry_type_to_build(
				GeometryType::Value geom_type);

		/**
		 * Clears and removes all geometry(s).
		 *
		 * This effectively removes all geometries.
		 *
		 * This method generates signals.
		 */
		UndoOperation
		clear_all_geometries();

		/**
		 * Sets the geometry and type.
		 *
		 * If there are any existing internal geometries then they are
		 * cleared first.
		 *
		 * This method generates signals.
		 */
		template <typename ForwardPointIter>
		UndoOperation
		set_geometry(
				GeometryType::Value geom_type,
				ForwardPointIter geom_points_begin,
				ForwardPointIter geom_points_end);

		/**
		 * Insert a point into the current geometry.
		 *
		 * Currently we have only one internal geometry.
		 * In the future we may support multiple internal geometries...
		 * ...for example, deleting a line segment on a polyline would result in two polylines
		 * in which case the current geometry could then be either of the two polylines.
		 *
		 * If no geometry currently exists then this method will create one.
		 *
		 * This method generates signals.
		 */
		UndoOperation
		insert_point_into_current_geometry(
				PointIndex point_index,
				const GPlatesMaths::PointOnSphere &oriented_pos_on_globe);

		/**
		 * Remove a point from the current geometry.
		 *
		 * As noted in @a insert_point_into_current_geometry we currently have
		 * only one internal geometry.
		 *
		 * If point being removed is the last point in the current geometry then
		 * the geometry is removed. Since we currently cannot have more than one internal
		 * geometry this will leave us with no geometries.
		 *
		 * This method generates signals.
		 */
		UndoOperation
		remove_point_from_current_geometry(
				PointIndex point_index);

		/**
		 * Moves a point in the current geometry.
		 *
		 * The @a is_intermediate_move flag is simply passed to the
		 * @a moved_point_in_current_geometry signal. Nothing is done
		 * with it internally. For example, it can be used to signal the
		 * user is dragging a point with the mouse but has not yet released
		 * the mouse button.
		 *
		 * This method generates signals.
		 */
		UndoOperation
		move_point_in_current_geometry(
				PointIndex point_index,
				const GPlatesMaths::PointOnSphere &new_oriented_pos_on_globe,
				bool is_intermediate_move = false);

		/**
		 * Undo a previous operation.
		 *
		 * The type of operation and any data associated with it are
		 * encoded in the opaque type @a UndoOperation.
		 *
		 * @param undo_operation the operation that needs undoing.
		 */
		void
		undo(
				UndoOperation &undo_operation);

		//////////////////////////////////////////////////////////////////////////
		// Interface used by signal observers for querying geometry state.
		//////////////////////////////////////////////////////////////////////////

		/**
		 * Returns true if there are any internal geometries in this builder.
		 */
		bool
		has_geometry() const
		{
			return get_num_geometries() > 0;
		}

		/**
		 * The number of internal geometries.
		 *
		 * There can be more than one geometry for things such as GML multi-curve
		 * but since that's not currently implemented we have at most one geometry.
		 */
		unsigned int
		get_num_geometries() const;

		/**
		 * The type of geometry we're trying to build.
		 *
		 * This is not necessarily the type of the current geometry ( see
		 * @a set_geometry_type_to_build).
		 */
		GeometryType::Value
		get_geometry_build_type() const;

		/**
		 * The actual type of the geometry at the current geometry index.
		 *
		 * This may differ from the type set in @a set_geometry_type_to_build.
		 *
		 * @throws PreconditionViolationError if there's currently no geometry in this builder.
		 * Check first with @a has_geometry if you're unsure.
		 */
		GeometryType::Value
		get_actual_type_of_current_geometry() const;

		/**
		 * The actual type of the geometry at the specified geometry index.
		 *
		 * This may differ from the type set in @a set_geometry_type_to_build.
		 *
		 * @throws PreconditionViolationError if there's currently no geometry in this builder
		 * at index @a geom_index.
		 */
		GeometryType::Value
		get_actual_type_of_geometry(
				GeometryIndex geom_index) const;

		/**
		 * The current geometry that operations are being directed at.
		 *
		 * Currently this is always zero since we have only one internal geometry.
		 * In the future we may support multiple geometries (eg, deleting a line segment on
		 * a polyline would result in two polylines - in which case the current geometry index
		 * could then be either of the two - we would then most likely have a tool to select
		 * which geometry is the current geometry - that is the geometry that subsequent
		 * operations would apply to).
		 */
		GeometryIndex
		get_current_geometry_index() const;
	
		/**
		 * Number of points/vertices in the current geometry.
		 *
		 * Returns zero if there's currently no geometry in this builder.
		 */
		unsigned int
		get_num_points_in_current_geometry() const;
	
		/**
		 * Number of points/vertices in the geometry at index @a geom_index.
		 *
		 * @throws PreconditionViolationError if there's currently no geometry in this builder
		 * at index @a geom_index.
		 */
		unsigned int
		get_num_points_in_geometry(
				GeometryIndex geom_index) const;

		/**
		 * Begin iterator to GPlatesMaths::PointOnSphere points/vertices
		 * of geometry at index @a geom_index.
		 *
		 * @throws PreconditionViolationError if there's currently no geometry in this builder
		 * at index @a geom_index.
		 */
		point_const_iterator_type
		get_geometry_point_begin(
				GeometryIndex geom_index) const;

		/**
		 * End iterator to GPlatesMaths::PointOnSphere points/vertices
		 * of geometry at index @a geom_index.
		 *
		 * @throws PreconditionViolationError if there's currently no geometry in this builder
		 * at index @a geom_index.
		 */
		point_const_iterator_type
		get_geometry_point_end(
				GeometryIndex geom_index) const;

		/**
		 * Returns point/vertex of geometry at index @a geom_index and
		 * point at index @a point_index within that geometry.
		 *
		 * @throws PreconditionViolationError if there's currently no geometry in this builder
		 * at index @a geom_index or if there's no point at index @a point_index.
		 */
		const GPlatesMaths::PointOnSphere &
		get_geometry_point(
				GeometryIndex geom_index,
				PointIndex point_index) const;

		//////////////////////////////////////////////////////////////////////////
		// Interface returns final geometry on sphere that has been built.
		//////////////////////////////////////////////////////////////////////////

		/**
		 * Returns geometry built or NULL if no geometries currently in this builder.
		 *
		 * Currently the build operations prevent more than one internal geometry
		 * from being built even though the interface currently supports
		 * multiple geometries. In the future we may have GeometryOnSphere classes
		 * that can contain multiple geometries internally such as a multi-curve
		 * containing multiple PolylineOnSphere's.
		 * Even so this method will only ever return a single geometry
		 * whether that's a wrapper over multiple geometries or not.
		 */
		geometry_opt_ptr_type
		get_geometry_on_sphere();

		//////////////////////////////////////////////////////////////////////////
		// Only GeometryBuilder implementation classes call these methods.
		//////////////////////////////////////////////////////////////////////////

		void
		visit_undo_operation(
				GeometryBuilderInternal::InsertPointUndoImpl &);

		void
		visit_undo_operation(
				GeometryBuilderInternal::RemovePointUndoImpl &);

		void
		visit_undo_operation(
				GeometryBuilderInternal::MovePointUndoImpl &);

		void
		visit_undo_operation(
				GeometryBuilderInternal::SetGeometryTypeUndoImpl &);

		void
		visit_undo_operation(
				GeometryBuilderInternal::ClearAllGeometriesUndoImpl &);

		void
		visit_undo_operation(
				GeometryBuilderInternal::InsertGeometryUndoImpl &);

	signals:
		// NOTE: all signals/slots should use namespace scope for all arguments
		//       otherwise differences between signals and slots will cause Qt
		//       to not be able to connect them at runtime.

		/**
		 * Geometry modifications have started.
		 *
		 * This signal can be followed by zero or more other signals
		 * and then the @a stopped_updating_geometry signal.
		 */
		void
		started_updating_geometry();

		/**
		 * Geometry modifications have started.
		 *
		 * Same as above except this signal is not emitted if the geometry operations
		 * consist only of one or more intermediate move operations.
		 * This can significantly reduce the number of signals received when the
		 * user is dragging vertices.
		 */
		void
		started_updating_geometry_excluding_intermediate_moves();

		/**
		 * Geometry modifications have stopped.
		 *
		 * This signal is preceeded by the @a started_updating_geometry signal
		 * and zero or more other signals.
		 * This signal could be the only signal you register to receive if
		 * you're only interested in knowing that the geometry was updated
		 * but don't care about the details (eg, if you query the entire geometry
		 * state every time you receive this signal).
		 */
		void
		stopped_updating_geometry();

		/**
		 * Geometry modifications have stopped.
		 *
		 * Same as above except this signal is not emitted if the geometry operations
		 * consist only of one or more intermediate move operations.
		 * This can significantly reduce the number of signals received when the
		 * user is dragging vertices.
		 */
		void
		stopped_updating_geometry_excluding_intermediate_moves();

		/**
		 * The actual type of geometry at @a geometry_index has changed
		 * to @a geometry_type.
		 */
		void
		changed_actual_geometry_type(
				GPlatesViewOperations::GeometryBuilder::GeometryIndex geometry_index,
				GPlatesViewOperations::GeometryType::Value geometry_type);
		
		/**
		 * Geometry was inserted at @a geometry_index.
		 *
		 * Geometries with indices @a geometry_index or above should now be
		 * incremented by one.
		 * Inserted geometry is not necessarily empty - if it's not empty you'll
		 * need to query the type of points contained within (ie, you won't
		 * get any @a inserted_point_into_current_geometry signals).
		 */
		void
		inserted_geometry(
				GPlatesViewOperations::GeometryBuilder::GeometryIndex geometry_index);
		
		/**
		 * Geometry was removed at @a geometry_index.
		 *
		 * Geometries with indices greater than @a geometry_index should now be
		 * decremented by one.
		 * Removed geometry is not necessarily empty - if it's not empty
		 * you won't receive any @a removed_point_from_current_geometry signals.
		 */
		void
		removed_geometry(
				GPlatesViewOperations::GeometryBuilder::GeometryIndex geometry_index);
		
		/**
		 * The current geometry index, at which all operations are currently directed,
		 * has changed to @a current_geometry_index.
		 */
		void
		changed_current_geometry_index(
				GPlatesViewOperations::GeometryBuilder::GeometryIndex current_geometry_index);

		/**
		 * The point @a inserted_point was inserted into the current geometry
		 * at index @a point_index.
		 */
		void
		inserted_point_into_current_geometry(
				GPlatesViewOperations::GeometryBuilder::PointIndex point_index,
				const GPlatesMaths::PointOnSphere &inserted_point);

		/**
		 * The point at index @a point_index was removed from the current geometry.
		 */
		void
		removed_point_from_current_geometry(
				GPlatesViewOperations::GeometryBuilder::PointIndex point_index);

		/**
		 * The point at index @a point_index was removed from the current geometry.
		 *
		 * The @a is_intermediate_move flag is simply the same named flag passed to
		 * @a move_point_in_current_geometry.
		 */
		void
		moved_point_in_current_geometry(
				GPlatesViewOperations::GeometryBuilder::PointIndex point_index,
				const GPlatesMaths::PointOnSphere &new_point_position,
				bool is_intermediate_move);

	private:
		/**
		 * Convenience structure for ensuring matching begin/end_update_geometry calls.
		 */
		struct UpdateGuard :
			public boost::noncopyable
		{
			UpdateGuard(
					GeometryBuilder &,
					bool is_intermediate_move = false);

			~UpdateGuard();

			GeometryBuilder &d_geometry_builder;
			bool d_is_intermediate_move;
		};

		/**
		 * Typedef for pointer to @a InternalGeometryBuilder.
		 */
		typedef boost::shared_ptr<InternalGeometryBuilder> geometry_builder_ptr_type;

		/**
		 * Typedef for sequence of @a InternalGeometryBuilder pointers.
		 */
		typedef std::vector<geometry_builder_ptr_type> geometry_builder_seq_type;

		/**
		 * Typedef for a sequence of @a UndoOperation objects.
		 */
		typedef std::vector<UndoOperation> undo_operation_seq_type;

		/**
		 * Value of geometry we're trying to build.
		 */
		GeometryType::Value d_geometry_build_type;

		/**
		 * Sequence of geometries.
		 *
		 * Until a GeometryOnSphere type that supports multiple internal geometries
		 * comes along this sequence will never contain more than on geometry.
		 */
		geometry_builder_seq_type d_geometry_builder_seq;

		/**
		 * Index of geometry that's currently being edited/built.
		 *
		 * Until a GeometryOnSphere type that supports multiple internal geometries
		 * comes along this will always be zero.
		 */
		GeometryIndex d_current_geometry_index;

		/**
		 * Used by @a begin_update_geometry and @a end_update_geometry to
		 * keep track of the nested call depth.
		 */
		int d_update_geometry_depth;

		static const GeometryIndex DEFAULT_GEOMETRY_INDEX = 0;

		void
		begin_update_geometry(
				bool is_intermedate_move);

		void
		end_update_geometry(
				bool is_intermedate_move);

		const InternalGeometryBuilder&
		get_current_geometry_builder() const;

		InternalGeometryBuilder&
		get_current_geometry_builder();

		template <typename ForwardPointIter>
		UndoOperation
		insert_geometry(
				GeometryIndex,
				ForwardPointIter geom_points_begin,
				ForwardPointIter geom_points_end);

		UndoOperation
		insert_geometry(
				GeometryIndex);

		UndoOperation
		insert_geometry(
				geometry_builder_ptr_type geometry_ptr,
				GeometryIndex geom_index);

		void
		remove_geometry(
				GeometryIndex);

		/**
		 * Combines multiple undo operations into one.
		 */
		UndoOperation
		create_composite_undo_operation(
				undo_operation_seq_type undo_operations);
	};

	inline
	GeometryType::Value
	GeometryBuilder::get_geometry_build_type() const
	{
		return d_geometry_build_type;
	}

	inline
	InternalGeometryBuilder &
	GeometryBuilder::get_current_geometry_builder()
	{
		return const_cast<InternalGeometryBuilder &>(
				static_cast<const GeometryBuilder*>(this)->get_current_geometry_builder());
	}

	template <typename ForwardPointIter>
	GeometryBuilder::UndoOperation
	GeometryBuilder::set_geometry(
			GeometryType::Value geom_type,
			ForwardPointIter geom_points_begin,
			ForwardPointIter geom_points_end)
	{
		// This gets put in all public methods that modify geometry state.
		// It checks for geometry type changes and emits begin_update/end_update signals.
		UpdateGuard update_guard(*this);

		undo_operation_seq_type undo_operation_seq;

		// Clear any internal geometries first.
		UndoOperation clear_geom_undo_operation = clear_all_geometries();
		undo_operation_seq.push_back(clear_geom_undo_operation);

		// Set the type of geometry to build.
		UndoOperation set_geom_type_undo_operation = set_geometry_type_to_build(geom_type);
		undo_operation_seq.push_back(set_geom_type_undo_operation);

		// Insert our points into a new geometry.
		UndoOperation insert_geom_undo_operation = insert_geometry(
				0/*geom_index*/, geom_points_begin, geom_points_end);
		undo_operation_seq.push_back(insert_geom_undo_operation);

		return create_composite_undo_operation(undo_operation_seq);
	}

	template <typename ForwardPointIter>
	GeometryBuilder::UndoOperation
	GeometryBuilder::insert_geometry(
			GeometryIndex geom_index,
			ForwardPointIter geom_points_begin,
			ForwardPointIter geom_points_end)
	{
		// Create new geometry builder.
		geometry_builder_ptr_type geometry_ptr(
			new InternalGeometryBuilder(this, d_geometry_build_type));

		// Copy points into the geometry.
		geometry_ptr->get_point_seq().insert(
				geometry_ptr->get_point_seq().end(),
				geom_points_begin,
				geom_points_end);

		return insert_geometry(geometry_ptr, geom_index);
	}
}

#endif // GPLATES_VIEWOPERATIONS_GEOMETRYBUILDER_H
