/* $Id$ */

/**
 * \file 
 * Transfers focused feature geometry changes made by a @a GeometryBuilder
 * to the feature containing the geometry.
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

#ifndef GPLATES_VIEWOPERATIONS_FOCUSEDFEATUREGEOMETRYMANIPULATOR_H
#define GPLATES_VIEWOPERATIONS_FOCUSEDFEATUREGEOMETRYMANIPULATOR_H

#include <QObject>

#include "GeometryBuilder.h"
#include "gui/FeatureFocus.h"
#include "maths/GeometryOnSphere.h"
#include "model/types.h"


namespace  GPlatesQtWidgets
{
	class ViewportWindow;
}

namespace GPlatesViewOperations
{
	class GeometryBuilder;

	/**
	 * Transfers focused feature geometry changes made by a @a GeometryBuilder
	 * to the feature containing the geometry.
	 */
	class FocusedFeatureGeometryManipulator :
		public QObject
	{
		Q_OBJECT

	public:
		FocusedFeatureGeometryManipulator(
				GeometryBuilder &focused_feature_geom_builder,
				GPlatesGui::FeatureFocus &feature_focus,
				GPlatesQtWidgets::ViewportWindow &viewport_window);

	public slots:
		// NOTE: all signals/slots should use namespace scope for all arguments
		//       otherwise differences between signals and slots will cause Qt
		//       to not be able to connect them at runtime.

		/**
		 * @a GeometryBuilder has done a group of one or more updates.
		 */
		void
		geometry_builder_stopped_updating_geometry();

		/**
		 * @a GeometryBuilder has moved a vertex.
		 */
		void
		move_point_in_current_geometry(
				GPlatesViewOperations::GeometryBuilder::PointIndex point_index,
				const GPlatesMaths::PointOnSphere &new_oriented_pos_on_globe,
				bool is_intermediate_move);

		/**
		 * Changed which reconstruction geometry is currently focused.
		 */
		void
		set_focus(
				GPlatesModel::FeatureHandle::weak_ref feature_ref,
				GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type focused_geometry);

	private:
		/**
		 * Convenience structure for calling @a begin_block_infinite_signal_slot_loop and @a end_block_infinite_signal_slot_loop.
		 */
		struct BlockInfiniteSignalSlotLoop
		{
			BlockInfiniteSignalSlotLoop(
					FocusedFeatureGeometryManipulator *geom_manipulator) :
			d_geom_manipulator(geom_manipulator)
			{
				d_geom_manipulator->begin_block_infinite_signal_slot_loop();
			}

			~BlockInfiniteSignalSlotLoop()
			{
				// Cannot allow exceptions to leave a destructor.
				// Exception won't happen here anyway.
				try
				{
					d_geom_manipulator->end_block_infinite_signal_slot_loop();
				}
				catch (...)
				{
				}
			}

			FocusedFeatureGeometryManipulator *d_geom_manipulator;
		};

		/**
		 * Used to set initial focused feature geometry and get final geometry.
		 */
		GeometryBuilder *d_focused_feature_geom_builder;

		/**
		 * Used to announce modifications of focused feature.
		 */
		GPlatesGui::FeatureFocus *d_feature_focus;

		/**
		 * Used to get access to current reconstruction tree.
		 */
		GPlatesQtWidgets::ViewportWindow *d_viewport_window;

		/**
		 * The feature which contains the geometry whose RFG is the currently-focused
		 * reconstruction geometry.
		 *
		 * Note that there might not be any such feature, in which case this would be an
		 * invalid weak-ref.
		 */
		GPlatesModel::FeatureHandle::weak_ref d_feature;

		/**
		 * The reconstruction geometry which is focused.
		 *
		 * Note that there may not be a focused reconstruction geometry, in which case this
		 * would be a null pointer.
		 */
		GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type d_focused_geometry;

		/**
		 * Is true if we've received an update signal from @a GeometryBuilder
		 * but have chosen to ignore it.
		 */
		bool d_ignore_geom_builder_update;

		/**
		 * Counts depth of nested calls to @a begin_block_infinite_signal_slot_loop and @a end_block_infinite_signal_slot_loop.
		 */
		int d_block_infinite_signal_slot_loop_depth;

		void
		connect_to_geometry_builder();

		void
		connect_to_feature_focus();

		/**
		 * Gets focused feature geometry and sets it in the @a GeometryBuilder. 
		 */
		void
		convert_geom_from_feature_to_builder();

		/**
		 * Gets geometry from @a GeometryBuilder and sets it in the focused feature.
		 */
		void
		convert_geom_from_builder_to_feature();

		/**
		 * Reconstructs the specified geometry forward or backward in time using
		 * current reconstruction tree and plate_id of currently focused feature.
		 * If @a reverse_reconstruct is true then reconstruct back to present day.
		 */
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
		reconstruct(
				GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry_on_sphere,
				bool reverse_reconstruct);

		/**
		 * Starts blocking of @a set_focus.
		 */
		void
		begin_block_infinite_signal_slot_loop()
		{
			++d_block_infinite_signal_slot_loop_depth;
		}

		/**
		 * Finishes blocking of @a set_focus.
		 */
		void
		end_block_infinite_signal_slot_loop()
		{
			--d_block_infinite_signal_slot_loop_depth;
		}

		bool
		is_infinite_signal_slot_loop_blocked() const
		{
			return d_block_infinite_signal_slot_loop_depth > 0;
		}

		/**
		 * Gets the plate id from the focused feature.
		 * Returns false if nothing found.
		 */
		bool
		get_plate_id_from_feature(
				GPlatesModel::integer_plate_id_type &plate_id);
	};
}

#endif // GPLATES_VIEWOPERATIONS_FOCUSEDFEATUREGEOMETRYMANIPULATOR_H
