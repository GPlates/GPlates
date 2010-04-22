/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008, 2009 The University of Sydney, Australia
 * 
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
 
#ifndef GPLATES_QTWIDGETS_MODIFYRECONSTRUCTIONPOLEWIDGET_H
#define GPLATES_QTWIDGETS_MODIFYRECONSTRUCTIONPOLEWIDGET_H

#include <vector>
#include <QWidget>
#include <boost/scoped_ptr.hpp>
#include <boost/optional.hpp>

#include "ModifyReconstructionPoleWidgetUi.h"
#include "gui/SimpleGlobeOrientation.h"
#include "model/FeatureHandle.h"
#include "model/ReconstructedFeatureGeometry.h"
#include "view-operations/RenderedGeometryCollection.h"
#include "maths/PointOnSphere.h"


namespace GPlatesAppLogic
{
	class Reconstruct;
}

namespace GPlatesGui
{
	class FeatureFocus;
}

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{
	class ViewportWindow;
	class ApplyReconstructionPoleAdjustmentDialog;
	class AdjustmentApplicator;
	

	class ModifyReconstructionPoleWidget:
			public QWidget, 
			protected Ui_ModifyReconstructionPoleWidget
	{
		Q_OBJECT
	public:

		// FIXME: This typedef has been changed to include an RFG pointer.
		// This is part of the paleomag workarounds.
		typedef std::vector<std::pair<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type,
			GPlatesModel::ReconstructedFeatureGeometry::non_null_ptr_to_const_type> >
				geometry_collection_type;

		ModifyReconstructionPoleWidget(
				GPlatesPresentation::ViewState &view_state,
				ViewportWindow &viewport_window,
				QWidget *parent_ = NULL);

		~ModifyReconstructionPoleWidget();

	public slots:

		void
		start_new_drag(
				const GPlatesMaths::PointOnSphere &current_oriented_position);

		void
		update_drag_position(
				const GPlatesMaths::PointOnSphere &current_oriented_position);

		void
		start_new_rotation_drag(
				const GPlatesMaths::PointOnSphere &current_oriented_position,
				const GPlatesMaths::PointOnSphere &oriented_center_of_viewport);

		void
		update_rotation_drag_position(
				const GPlatesMaths::PointOnSphere &current_oriented_position,
				const GPlatesMaths::PointOnSphere &oriented_center_of_viewport);

		void
		end_drag();

		void
		apply();

		void
		reset();

		void
		reset_adjustment();

		void
		change_constrain_latitude_checkbox_state(
			int new_checkbox_state);
				
		void
		change_highlight_children_checkbox_state(
			int new_checkbox_state);				

		void
		set_focus(
				GPlatesGui::FeatureFocus &feature_focus);

		void
		handle_reconstruction();

		void
		activate();

		void
		deactivate();

	protected:

		/**
		 * Find the geometries whose RFG has a plate ID which is equal to the plate ID of
		 * the currently-focused RFG (if there is one).
		 *
		 * These are called the "initial" geometries, because they will be moved around by
		 * dragging.
		 */
		void
		populate_initial_geometries();

		/**
		 * Draw the initial geometries, before they've been dragged.
		 */
		void
		draw_initial_geometries();

		/**
		 * Draw the initial geometries in "dragged" positions, as a result of the
		 * accumulated orientation.
		 */
		void
		draw_dragged_geometries();

		/**
		 * Update the "Adjustment" fields in the TaskPanel pane.
		 */
		void
		update_adjustment_fields();

	protected slots:

		/**
		 * Draw the initial geometries when the canvas tool is first activated.
		 */
		void
		draw_initial_geometries_at_activation();

		/**
		 * Clear geometries and reset the adjustment after a reconstruction.
		 *
		 * Or, in more detail:  Clear the initial/dragged geometries from the globe (since
		 * the plate is about to be reconstructed to that position anyway) and reset the
		 * adjustment (since the plate is now in the dragged position, so there's no
		 * difference between the plate position and the dragged position).
		 *
		 * This slot is intended to be invoked after (re-)reconstruction has occurred (as a
		 * result of the user clicking "OK" in the Apply Reconstruction Pole Adjustment
		 * dialog).
		 */
		void
		clear_and_reset_after_reconstruction();

	private:
		//! Manages reconstructions.
		GPlatesAppLogic::Reconstruct *d_reconstruct_ptr;

		/**
		 * Used to draw rendered geometries.
		 */
		GPlatesViewOperations::RenderedGeometryCollection *d_rendered_geom_collection;

		/**
		 * Rendered geometry layer to render initial geometries.
		 */
		GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type
			d_initial_geom_layer_ptr;

		/**
		 * Rendered geometry layer to render dragged geometries.
		 */
		GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type
			d_dragged_geom_layer_ptr;

		/**
		 * The dialog presented to the user, to enable him to complete the modification of
		 * reconstruction poles.
		 *
		 * This dialog forms the second phase of user-interaction (after dragging
		 * geometries around on the globe to calculate a reconstruction pole adjustment).
		 */
		boost::scoped_ptr<ApplyReconstructionPoleAdjustmentDialog> d_dialog_ptr;

		boost::scoped_ptr<AdjustmentApplicator> d_applicator_ptr;

		/**
		 * Whether or not the latitude of the dragging motion should be constrained.
		 */
		bool d_should_constrain_latitude;
		
		/**
		 * Whether or not the children of the selected plate id should be displayed during a drag                                                                     
		 */
		bool d_should_display_children;		

		/**
		 * The start-point of a latitude-constraining drag.
		 *
		 * This data member is used @em only by latitude-constraining drags.
		 */
		boost::optional<GPlatesMaths::PointOnSphere> d_drag_start;

		/**
		 * Whether or not this dialog is currently active.
		 *
		 * This is slightly hackish, but I don't think we want to invoke
		 * @a populate_initial_geometries every time the reconstruction time changes, even
		 * when this dialog is not active...
		 */
		bool d_is_active;

		/**
		 * This accumulates the rotation for us.
		 *
		 * Ignore the fact that it looks like it's a @em globe orientation.  That's just
		 * your eyes playing tricks on you.
		 *
		 * Since SimpleGlobeOrientation is non-copy-constructable and non-copy-assignable,
		 * we have to use a boost:scoped_ptr here instead of a boost::optional.
		 */
		boost::scoped_ptr<GPlatesGui::SimpleGlobeOrientation> d_accum_orientation;

		/**
		 * The reconstruction plate ID from the reconstructed feature geometry (RFG).
		 *
		 * Note that this could be 'boost::none' -- it's possible for an RFG to be created
		 * without a reconstruction plate ID.  (See the comments in class
		 * GPlatesModel::ReconstructedFeatureGeometry for details.)
		 */
		boost::optional<GPlatesModel::integer_plate_id_type> d_plate_id;

		/**
		 * The (initial) geometry of each of the RFGs whose plate IDs equal the plate ID of
		 * the currently-focused RFG (if there is one).
		 *
		 * As the user drags the geometries around to modify the total reconstruction pole,
		 * these geometries will be rotated to new positions on the globe by the
		 * accumulated rotation.
		 *
		 * Presumably, this container will be non-empty when the 'update_drag_position'
		 * slot is triggered, since if there were no geometry, what would the user be
		 * dragging?
		 */
		geometry_collection_type d_initial_geometries;
		
		/**
		 * View state for extracting VGP visibility settings.                                                                      
		 */
		GPlatesPresentation::ViewState *d_view_state_ptr;		

		void
		make_signal_slot_connections(
				GPlatesPresentation::ViewState &view_state);

		void
		create_child_rendered_layers();
	};
}

#endif  // GPLATES_QTWIDGETS_MODIFYRECONSTRUCTIONPOLEWIDGET_H
