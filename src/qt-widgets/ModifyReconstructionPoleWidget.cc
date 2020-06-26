/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008, 2009, 2011 The University of Sydney, Australia
 * (as "ReconstructionPoleWidget.cc")
 * Copyright (C) 2010 Geological Survey of Norway
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

#include <iostream>
#include <vector>
#include <boost/optional.hpp>
#include <QDebug>
#include <QtGlobal>

#include "ActionButtonBox.h"
#include "ApplyReconstructionPoleAdjustmentDialog.h"
#include "ModifyReconstructionPoleWidget.h"
#include "MovePoleWidget.h"
#include "QtWidgetUtils.h"
#include "ViewportWindow.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/LayerProxyUtils.h"
#include "app-logic/Reconstruction.h"
#include "app-logic/ReconstructionGeometryUtils.h"
#include "app-logic/ReconstructionLayerProxy.h"
#include "app-logic/ReconstructionTree.h"

#include "feature-visitors/TotalReconstructionSequencePlateIdFinder.h"
#include "feature-visitors/TotalReconstructionSequenceTimePeriodFinder.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "gui/FeatureFocus.h"

#include "presentation/ReconstructionGeometryRenderer.h"
#include "presentation/ViewState.h"
#include "presentation/VisualLayer.h"
#include "presentation/VisualLayerParams.h"
#include "presentation/VisualLayers.h"

#include "view-operations/RenderedGeometryCollection.h"
#include "view-operations/RenderedGeometryFactory.h"
#include "view-operations/RenderedGeometryParameters.h"
#include "view-operations/RenderedGeometryUtils.h"


namespace
{
	/**
	 * Return the closest point on the horizon to @a point_within_horizon.
	 *
	 * If @a point_within_horizon is either coincident with the centre of the
	 * viewport, or (somehow) antipodal to the centre of the viewport, boost::none will be
	 * returned.
	 */
	const boost::optional<GPlatesMaths::PointOnSphere>
	get_closest_point_on_horizon(
			const GPlatesMaths::PointOnSphere &point_within_horizon,
			const GPlatesMaths::PointOnSphere &center_of_viewport)
	{
		using namespace GPlatesMaths;

		if (collinear(point_within_horizon.position_vector(),
				center_of_viewport.position_vector()))
		{
			// The point (which is meant to be) within the horizon is either coincident
			// with the centre of the viewport, or (somehow) antipodal to the centre of
			// the viewport (which should not be possible, but right now, we don't care
			// about the story, we just care about the maths).
			//
			// Hence, it's not mathematically possible to calculate a closest point on
			// the horizon.
			return boost::none;
		}
		Vector3D cross_result =
				cross(point_within_horizon.position_vector(),
						center_of_viewport.position_vector());
		// Since the two unit-vectors are non-collinear, we can assume the cross-product is
		// a non-zero vector.
		UnitVector3D normal_to_plane = cross_result.get_normalisation();

		Vector3D point_on_horizon =
				cross(center_of_viewport.position_vector(), normal_to_plane);
		// Since both the center-of-viewport and normal-to-plane are unit-vectors, and they
		// are (by definition) perpendicular, we will assume the result is of unit length.
		return PointOnSphere(point_on_horizon.get_normalisation());
	}


	/**
	 * Return the closest point on the equator (of pole @a pole) to @a point.
	 *
	 * The equator is the great circle with rotation axis @a pole.
	 *
	 * If @point is at @a pole or its antipodal, there is no unique "closest" point to @a point on
	 * the equator, so boost::none will be returned.
	 */
	const boost::optional<GPlatesMaths::PointOnSphere>
	get_closest_point_on_equator_of_pole(
			const GPlatesMaths::PointOnSphere &point,
			const GPlatesMaths::PointOnSphere &pole)
	{
		using namespace GPlatesMaths;

		Vector3D cross_point_and_pole = cross(point.position_vector(), pole.position_vector());
		if (cross_point_and_pole.magSqrd() == 0)
		{
			// The point is at 'pole' or its antipodal.  Hence, there is no unique
			// "closest" point on the equator.
			return boost::none;
		}
		// Else, the point is _not_ at 'pole' or its antipodal, meaning there *is* a
		// unique "closest" point on the equator.  Hence, we can proceed.

		// Move the point to the great circle 'equator'.
		return PointOnSphere(UnitVector3D(cross(
				pole.position_vector(),
				cross_point_and_pole.get_normalisation())));
	}

	void
	add_child_edges_to_collection(
		const GPlatesAppLogic::ReconstructionTree::Edge &edge,
		std::vector<GPlatesModel::integer_plate_id_type> &child_plate_id_collection)
	{	
		GPlatesAppLogic::ReconstructionTree::edge_list_type::const_iterator it;
		GPlatesAppLogic::ReconstructionTree::edge_list_type::const_iterator
			it_begin = edge.get_child_edges().begin();
		GPlatesAppLogic::ReconstructionTree::edge_list_type::const_iterator
			it_end = edge.get_child_edges().end();

		for(it = it_begin; it != it_end ; it++)
		{
			child_plate_id_collection.push_back(it->get_moving_plate());
			add_child_edges_to_collection(*it,child_plate_id_collection);
		}		
	}

	void
	add_children_to_geometry_collection(
		std::vector<GPlatesModel::integer_plate_id_type> &child_plate_id_collection,
		const GPlatesModel::integer_plate_id_type plate_id,
		const GPlatesAppLogic::ReconstructionTree &tree)
	{
		boost::optional<const GPlatesAppLogic::ReconstructionTree::Edge &> edge = tree.get_edge(plate_id);
		if (!edge)
		{
			// We didn't find the edge. That'ok, we might just not have a rotation file loaded. 
			//qDebug() << "Empty edge container for plate id " << plate_id;
			return;
		}

		add_child_edges_to_collection(edge.get(), child_plate_id_collection);
	}

	void
	display_collection(
		const std::vector<GPlatesModel::integer_plate_id_type> &collection)
	{
		std::vector<GPlatesModel::integer_plate_id_type>::const_iterator 
			it = collection.begin(),
			end = collection.end();

		qDebug() << "Plate id collection:";			
		for (; it != end ; ++it)
		{
			qDebug() << *it;
		}
	}

	void
	examine_trs(
			std::vector<GPlatesQtWidgets::ApplyReconstructionPoleAdjustmentDialog::PoleSequenceInfo> &
					sequence_choices,
			GPlatesFeatureVisitors::TotalReconstructionSequencePlateIdFinder &trs_plate_id_finder,
			GPlatesFeatureVisitors::TotalReconstructionSequenceTimePeriodFinder &trs_time_period_finder,
			GPlatesModel::integer_plate_id_type plate_id_of_interest,
			const double &reconstruction_time,
			GPlatesModel::FeatureCollectionHandle::iterator &current_feature)
	{
		using namespace GPlatesQtWidgets;

		trs_plate_id_finder.reset();
		trs_plate_id_finder.visit_feature(current_feature);

		// A valid TRS should have a fixed reference frame and a moving reference frame. 
		// Let's verify that this is a valid TRS.
		if ( ! (trs_plate_id_finder.fixed_ref_frame_plate_id() &&
				trs_plate_id_finder.moving_ref_frame_plate_id()))
		{
			// This feature was missing one (or both) of the plate IDs which a TRS is
			// supposed to have.  Skip this feature.
			return;
		}
		// Else, we know it found both of the required plate IDs.

		if (*trs_plate_id_finder.fixed_ref_frame_plate_id() ==
				*trs_plate_id_finder.moving_ref_frame_plate_id())
		{
			// The fixed ref-frame plate ID equals the moving ref-frame plate ID? 
			// Something strange is going on here.  Skip this feature.
			return;
		}

		// Dietmar has said that he doesn't want the table to include pole sequences for
		// which the plate ID of interest is the fixed ref-frame.  (2008-09-18)
#if 0
		if (*trs_plate_id_finder.fixed_ref_frame_plate_id() == plate_id_of_interest)
		{
			trs_time_period_finder.reset();
			trs_time_period_finder.visit_feature(current_feature);
			if ( ! (trs_time_period_finder.begin_time() && trs_time_period_finder.end_time()))
			{
				// No time samples were found.  Skip this feature.
				return;
			}

			// For now, let's _not_ include sequences which don't span this
			// reconstruction time.
			GPlatesPropertyValues::GeoTimeInstant current_time(reconstruction_time);
			if (trs_time_period_finder.begin_time()->is_strictly_later_than(current_time) ||
					trs_time_period_finder.end_time()->is_strictly_earlier_than(current_time))
			{
				return;
			}

			sequence_choices.push_back(
					ApplyReconstructionPoleAdjustmentDialog::PoleSequenceInfo(
							current_feature->reference(),
							*trs_plate_id_finder.fixed_ref_frame_plate_id(),
							*trs_plate_id_finder.moving_ref_frame_plate_id(),
							trs_time_period_finder.begin_time()->value(),
							trs_time_period_finder.end_time()->value(),
							true));
		}
#endif
		if (*trs_plate_id_finder.moving_ref_frame_plate_id() == plate_id_of_interest)
		{
			trs_time_period_finder.reset();
			trs_time_period_finder.visit_feature(current_feature);
			if ( ! (trs_time_period_finder.begin_time() && trs_time_period_finder.end_time()))
			{
				// No time samples were found.  Skip this feature.
				return;
			}

			// For now, let's _not_ include sequences which don't span this
			// reconstruction time.
			GPlatesPropertyValues::GeoTimeInstant current_time(reconstruction_time);
			if (trs_time_period_finder.begin_time()->is_strictly_later_than(current_time) ||
					trs_time_period_finder.end_time()->is_strictly_earlier_than(current_time))
			{
				return;
			}

			sequence_choices.push_back(
					ApplyReconstructionPoleAdjustmentDialog::PoleSequenceInfo(
							(*current_feature)->reference(),
							*trs_plate_id_finder.fixed_ref_frame_plate_id(),
							*trs_plate_id_finder.moving_ref_frame_plate_id(),
							trs_time_period_finder.begin_time()->value(),
							trs_time_period_finder.end_time()->value(),
							false));
		}
	}


	/**
	 * This finds all the TRSes (total reconstruction sequences) in the supplied reconstruction
	 * whose fixed or moving ref-frame plate ID matches our plate ID of interest.
	 *
	 * The two vectors @a trses_with_plate_id_as_fixed and @a trses_with_plate_id_as_moving
	 * will be populated with the matches.
	 */
	void
	find_trses(
			std::vector<GPlatesQtWidgets::ApplyReconstructionPoleAdjustmentDialog::PoleSequenceInfo> &sequence_choices,
			GPlatesFeatureVisitors::TotalReconstructionSequencePlateIdFinder &trs_plate_id_finder,
			GPlatesFeatureVisitors::TotalReconstructionSequenceTimePeriodFinder &trs_time_period_finder,
			GPlatesModel::integer_plate_id_type plate_id_of_interest,
			const GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree,
			const GPlatesAppLogic::Reconstruction &reconstruction)
	{
		using namespace GPlatesModel;

		// Find the reconstruction feature collections used to create the reconstruction tree.
		// They could come from any of the reconstruction layer outputs (likely only one layer but could be more).
		boost::optional<const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &> reconstruction_feature_collections;
		std::vector<GPlatesAppLogic::ReconstructionLayerProxy::non_null_ptr_type> reconstruction_layer_outputs;
		if (reconstruction.get_active_layer_outputs<GPlatesAppLogic::ReconstructionLayerProxy>(reconstruction_layer_outputs))
		{
			// Iterate over the reconstruction layers.
			for (unsigned int reconstruction_layer_index = 0;
				reconstruction_layer_index < reconstruction_layer_outputs.size();
				++reconstruction_layer_index)
			{
				const GPlatesAppLogic::ReconstructionLayerProxy::non_null_ptr_type &
						reconstruction_layer_output = reconstruction_layer_outputs[reconstruction_layer_index];

				if (reconstruction_layer_output->get_reconstruction_tree() == reconstruction_tree)
				{
					reconstruction_feature_collections = reconstruction_layer_output->get_current_reconstruction_feature_collections();
					break;
				}
			}
		}
		if (!reconstruction_feature_collections)
		{
			return;
		}

		std::vector<FeatureCollectionHandle::weak_ref>::const_iterator collections_iter =
				reconstruction_feature_collections->begin();
		std::vector<FeatureCollectionHandle::weak_ref>::const_iterator collections_end =
				reconstruction_feature_collections->end();
		for ( ; collections_iter != collections_end; ++collections_iter)
		{
			const FeatureCollectionHandle::weak_ref &current_collection = *collections_iter;
			if ( ! current_collection.is_valid())
			{
				// FIXME:  Should we do anything about this? Or is this acceptable?
				// (If the collection is not valid, then presumably it has been
				// unloaded.  In which case, why hasn't the reconstruction been
				// recalculated?)
				continue;
			}

			FeatureCollectionHandle::iterator features_iter = current_collection->begin();
			FeatureCollectionHandle::iterator features_end = current_collection->end();
			for ( ; features_iter != features_end; ++features_iter)
			{
				examine_trs(
						sequence_choices,
						trs_plate_id_finder,
						trs_time_period_finder,
						plate_id_of_interest,
						reconstruction.get_reconstruction_time(),
						features_iter);
			}
		}
	}
}


GPlatesQtWidgets::ModifyReconstructionPoleWidget::ModifyReconstructionPoleWidget(
		MovePoleWidget &move_pole_widget,
		GPlatesPresentation::ViewState &view_state,
		ViewportWindow &viewport_window,
		QAction *clear_action,
		QWidget *parent_):
	TaskPanelWidget(parent_),
	d_application_state_ptr(&view_state.get_application_state()),
	d_move_pole_widget(move_pole_widget),
	d_rendered_geom_collection(&view_state.get_rendered_geometry_collection()),
	d_dialog_ptr(new ApplyReconstructionPoleAdjustmentDialog(&viewport_window)),
	d_applicator_ptr(new AdjustmentApplicator(view_state, *d_dialog_ptr)),
	d_should_display_children(false),
	d_is_active(false),
	d_view_state_ptr(&view_state)
{
	setupUi(this);

	// Set up the action button box showing the reset button.
	ActionButtonBox *action_button_box = new ActionButtonBox(1, 16, this);
	action_button_box->add_action(clear_action);
#ifndef Q_OS_MAC
	action_button_box->setFixedHeight(button_apply->sizeHint().height());
#endif
	QtWidgetUtils::add_widget_to_placeholder(
			action_button_box,
			action_button_box_placeholder_widget);

	make_signal_slot_connections(view_state);

	create_child_rendered_layers();

	// Disable the task panel widget.
	// It will get enabled when the Manipulate Pole canvas tool is activated.
	// This prevents the user from interacting with the task panel widget if the
	// canvas tool happens to be disabled at startup.
	setEnabled(false);
}


GPlatesQtWidgets::ModifyReconstructionPoleWidget::~ModifyReconstructionPoleWidget()
{
}


void
GPlatesQtWidgets::ModifyReconstructionPoleWidget::activate()
{
	// Enable the task panel widget.
	setEnabled(true);

	d_is_active = true;

	// Activate rendered layers.
	d_initial_geom_layer_ptr->set_active();
	d_dragged_geom_layer_ptr->set_active();
	d_adjustment_pole_layer_ptr->set_active();

	set_focus(d_view_state_ptr->get_feature_focus());
	draw_initial_geometries_at_activation();
	draw_adjustment_pole();
}


void
GPlatesQtWidgets::ModifyReconstructionPoleWidget::deactivate()
{
	// Disable the task panel widget.
	setEnabled(false);

	d_is_active = false;

	// Deactivate rendered layers.
	d_initial_geom_layer_ptr->set_active(false);
	d_dragged_geom_layer_ptr->set_active(false);
	d_adjustment_pole_layer_ptr->set_active(false);
}


void
GPlatesQtWidgets::ModifyReconstructionPoleWidget::draw_initial_geometries_at_activation()
{
	draw_initial_geometries();
	draw_dragged_geometries();
}


void
GPlatesQtWidgets::ModifyReconstructionPoleWidget::start_new_drag(
		const GPlatesMaths::PointOnSphere &current_position)
{
	if ( ! d_drag_orientation)
	{
		d_drag_orientation = AccumOrientation();
	}

	if (d_move_pole_widget.get_pole())
	{
		d_drag_start = get_closest_point_on_equator_of_pole(
				current_position,
				d_move_pole_widget.get_pole().get());
		if (d_drag_start)
		{
			d_drag_orientation->start_drag(d_drag_start.get());
		}
		// Else, the drag-start was at the adjustment pole location (or its antipodal), so there was no
		// unique "closest" point to the equator; don't try to do anything until the drag leaves the poles.
	}
	else
	{
		d_drag_orientation->start_drag(current_position);
	}
}


void
GPlatesQtWidgets::ModifyReconstructionPoleWidget::start_new_rotation_drag(
		const GPlatesMaths::PointOnSphere &current_position,
		const GPlatesMaths::PointOnSphere &centre_of_viewport)
{
	if (d_move_pole_widget.get_pole())
	{
		// Rotation-dragging of the plate is disabled because there is a specific adjustment pole
		// location which constrains the motion.
		//
		// Hence, nothing to do in this function.
		return;
	}

	boost::optional<GPlatesMaths::PointOnSphere> point_on_horizon =
			get_closest_point_on_horizon(current_position, centre_of_viewport);
	if ( ! point_on_horizon)
	{
		// The mouse position could not be converted to a point on the horizon.  Presumably
		// the it was at the centre of the viewport.  Hence, nothing to be done.
		return;
	}
	if ( ! d_drag_orientation)
	{
		d_drag_orientation = AccumOrientation();
	}
	d_drag_orientation->start_drag(point_on_horizon.get());
}


void
GPlatesQtWidgets::ModifyReconstructionPoleWidget::update_drag_position(
		const GPlatesMaths::PointOnSphere &current_position)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_drag_orientation,
			GPLATES_ASSERTION_SOURCE);

	if (d_move_pole_widget.get_pole())
	{
		if ( ! d_drag_start)
		{
			// We haven't set the drag start yet.  The mouse pointer must have been at either the
			// adjustment pole location (or its antipodal). The first thing we should try to do is
			// start the drag now.
			d_drag_start = get_closest_point_on_equator_of_pole(
					current_position,
					d_move_pole_widget.get_pole().get());
			if (d_drag_start)
			{
				d_drag_orientation->start_drag(d_drag_start.get());
			}
			// Else, the drag-start was at the adjustment pole location (or its antipodal), so there
			// was no unique "closest" point to the equator; don't try to do anything until the drag
			// leaves the poles.
		}
		else
		{
			boost::optional<GPlatesMaths::PointOnSphere> drag_update =
					get_closest_point_on_equator_of_pole(
							current_position,
							d_move_pole_widget.get_pole().get());
			if (drag_update)
			{
				d_drag_orientation->update_drag(drag_update.get());
			}
			// Else, the drag-update was at the adjustment pole location (or its antipodal), so
			// there was no unique "closest" point to the equator; don't try to do anything until
			// the drag leaves the poles.
		}
	}
	else
	{
		d_drag_orientation->update_drag(current_position);
	}

	draw_dragged_geometries();
	update_adjustment_fields();
}


void
GPlatesQtWidgets::ModifyReconstructionPoleWidget::update_rotation_drag_position(
		const GPlatesMaths::PointOnSphere &current_position,
		const GPlatesMaths::PointOnSphere &centre_of_viewport)
{
	if (d_move_pole_widget.get_pole())
	{
		// Rotation-dragging of the plate is disabled because there is a specific adjustment pole
		// location which constrains the motion.
		//
		// Hence, nothing to do in this function.
		return;
	}

	if ( ! d_drag_orientation)
	{
		// We must be in the middle of a non-drag.  Perhaps the user tried to drag at the
		// centre of the viewport, for instance.
		return;
	}

	boost::optional<GPlatesMaths::PointOnSphere> point_on_horizon =
			get_closest_point_on_horizon(current_position, centre_of_viewport);
	if ( ! point_on_horizon)
	{
		// The mouse position could not be converted to a point on the horizon.  Presumably
		// the it was at the centre of the viewport.  Hence, nothing to be done.
		return;
	}
	d_drag_orientation->update_drag(point_on_horizon.get());

	draw_dragged_geometries();
	update_adjustment_fields();
}


void
GPlatesQtWidgets::ModifyReconstructionPoleWidget::end_drag()
{  }


void
GPlatesQtWidgets::ModifyReconstructionPoleWidget::apply()
{
	if ( ! d_drag_orientation)
	{
		// The user must have released the mouse button after a non-drag.  Perhaps the user
		// tried to drag at the centre of the viewport, for instance.
		return;
	}

	if ( ! d_plate_id || ! d_reconstruction_tree)
	{
		// Presumably the feature did not contain a reconstruction plate ID.
		// What do we do here?  Do we give it one, or do nothing?
		// For now, let's just do nothing.
		return;
	}

	// Now find all the TRSes (total reconstruction sequences) whose fixed or moving ref-frame
	// plate ID matches our plate ID of interest.
	std::vector<ApplyReconstructionPoleAdjustmentDialog::PoleSequenceInfo> sequence_choices;
	GPlatesFeatureVisitors::TotalReconstructionSequencePlateIdFinder trs_plate_id_finder;
	GPlatesFeatureVisitors::TotalReconstructionSequenceTimePeriodFinder trs_time_period_finder;

	find_trses(
			sequence_choices,
			trs_plate_id_finder,
			trs_time_period_finder,
			*d_plate_id,
			*d_reconstruction_tree,
			d_application_state_ptr->get_current_reconstruction());

	// The Applicator should be set before the dialog is set up.
	// Why, you ask?  Because when the dialog is set up, the first row in the sequence choices
	// table will be selected, which will send a signal which will trigger a slot in the
	// Applicator, which will not do anything useful unless the Applicator has been set.
	d_applicator_ptr->set(
			sequence_choices,
			d_drag_orientation->get_accumulated_orientation(),
			*d_reconstruction_tree);
	d_dialog_ptr->setup_for_new_pole(
			*d_plate_id,
			d_application_state_ptr->get_current_reconstruction_time(),
			sequence_choices,
			d_drag_orientation->get_accumulated_orientation());

	d_dialog_ptr->show();
}


void
GPlatesQtWidgets::ModifyReconstructionPoleWidget::reset()
{
	reset_adjustment();
	draw_initial_geometries();
}


void
GPlatesQtWidgets::ModifyReconstructionPoleWidget::reset_adjustment()
{
	d_drag_orientation = boost::none;

	// Update the "Adjustment" fields in the TaskPanel pane.
	field_adjustment_lat->clear();
	field_adjustment_lon->clear();
	spinbox_adjustment_angle->setValue(0.0);
}

void
GPlatesQtWidgets::ModifyReconstructionPoleWidget::change_highlight_children_checkbox_state(
	int new_checkbox_state)
{
	if (new_checkbox_state == Qt::Unchecked)
	{
		d_should_display_children = false;
	}
	else if (new_checkbox_state == Qt::Checked)
	{
		d_should_display_children = true;
	}

	// Ignore any other values of 'new_checkbox_state'.
	draw_initial_geometries();
	draw_dragged_geometries();
}

void
GPlatesQtWidgets::ModifyReconstructionPoleWidget::set_focus(
		GPlatesGui::FeatureFocus &feature_focus)
{
	boost::optional<GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_to_const_type>
			rfg = get_focused_feature_geometry();

	// Do the following if no focused RFG.
	if (!rfg)
	{
		// Clear the plate ID and the plate ID field.
		d_reconstruction_tree = boost::none;
		d_plate_id = boost::none;
		reset_adjustment();
		field_moving_plate->clear();
		// This is to clear the rendered geometries if the feature geometry
		// disappears when this tool is still active (eg, when a
		// feature collection is unloaded and its features should disappear).
		draw_initial_geometries();
		return;
	}

	d_reconstruction_tree = rfg.get()->get_reconstruction_tree();

	// Nothing to do if plate ID hasn't changed.
	if (d_plate_id != rfg.get()->reconstruction_plate_id())
	{
		reset_adjustment();
		d_plate_id = rfg.get()->reconstruction_plate_id();
		if (d_plate_id)
		{
			QLocale locale_;
			// We need this static-cast because apparently QLocale's 'toString' member function
			// doesn't have an overload for 'unsigned long', so the compiler complains about
			// ambiguity.
			field_moving_plate->setText(locale_.toString(static_cast<unsigned>(*d_plate_id)));
		}
		else
		{
			// Clear the plate ID field.
			field_moving_plate->clear();
		}

		// Since the plate id has changed the initial geometries will also have changed.
		draw_initial_geometries();
	}
	// Else if this tool is active then re-populate our RFGs according to the new focused RFG
	// (note that the focused RFG can change with reconstruction time for the same focused feature).
	// See 'handle_reconstruction()' for why this is done here instead of in 'handle_reconstruction()'.
	else if (d_is_active)
	{
		draw_initial_geometries();
		draw_dragged_geometries();
	}
}


void
GPlatesQtWidgets::ModifyReconstructionPoleWidget::handle_reconstruction()
{
	// NOTE: We no longer do anything here because the order in which Qt slots are called
	// causes a problem - specifically here we rely on the focused RFG getting updated
	// (for the new reconstruction time) in order to re-populate our geometries but that update
	// doesn't happen until after this slot is called.
	// However when the focused RFG changes (associated with same focused feature), due to the
	// new reconstruction time, then our 'set_focus()' slot is called and that happens after the
	// focused RFG has been updated. So we moved our code into 'set_focus()'.
}


void
GPlatesQtWidgets::ModifyReconstructionPoleWidget::handle_layer_modified()
{
	// Re-populate the visible RFGs when a layer is made visible/invisible.
	draw_initial_geometries();
	draw_dragged_geometries();
}


boost::optional<GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_to_const_type>
GPlatesQtWidgets::ModifyReconstructionPoleWidget::get_focused_feature_geometry() const
{
	GPlatesAppLogic::ReconstructionGeometry::maybe_null_ptr_to_const_type focused_geometry =
			d_view_state_ptr->get_feature_focus().associated_reconstruction_geometry();
	if (!focused_geometry)
	{
		return boost::none;
	}

	// We're only interested in ReconstructedFeatureGeometry's.
	return GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
			GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_to_const_type>(focused_geometry);
}


void
GPlatesQtWidgets::ModifyReconstructionPoleWidget::populate_initial_geometries()
{
	// First clear the RFGs before we do anything else (even before we return early).
	d_visual_layer_reconstructed_feature_geometries.clear();

	// If there's no plate ID of the currently-focused RFG, then there can be no other RFGs
	// with the same plate ID.
	if ( ! d_plate_id || ! d_reconstruction_tree)
	{
		return;
	}

	std::vector<GPlatesModel::integer_plate_id_type> plate_id_collection;
	plate_id_collection.push_back(*d_plate_id);

	if (d_should_display_children)
	{
		add_children_to_geometry_collection(
				plate_id_collection, *d_plate_id, **d_reconstruction_tree);
	}

#if 0
	display_collection(plate_id_collection);	
#endif

	//
	// Iterate over all the *visible* reconstruction geometries that were reconstructed using the
	// same reconstruction tree as the focused feature geometry (and has a plate ID in plate collection).
	//

	GPlatesViewOperations::RenderedGeometryUtils::child_rendered_geometry_layer_reconstruction_geom_map_type
			child_rendered_geometry_layer_reconstruction_geom_map;
	if (GPlatesViewOperations::RenderedGeometryUtils::get_unique_reconstruction_geometries_in_reconstruction_child_layers(
			child_rendered_geometry_layer_reconstruction_geom_map,
			*d_rendered_geom_collection))
	{
		// Iterate over the child rendered geometry layers in the main rendered RECONSTRUCTION layer.
		GPlatesViewOperations::RenderedGeometryUtils::child_rendered_geometry_layer_reconstruction_geom_map_type::const_iterator
				child_rendered_geometry_layer_iter = child_rendered_geometry_layer_reconstruction_geom_map.begin();
		GPlatesViewOperations::RenderedGeometryUtils::child_rendered_geometry_layer_reconstruction_geom_map_type::const_iterator
				child_rendered_geometry_layer_end = child_rendered_geometry_layer_reconstruction_geom_map.end();
		for ( ;
			child_rendered_geometry_layer_iter != child_rendered_geometry_layer_end;
			++child_rendered_geometry_layer_iter)
		{
			const GPlatesViewOperations::RenderedGeometryCollection::child_layer_index_type
					child_rendered_geometry_layer_index = child_rendered_geometry_layer_iter->first;

			// Find the visual layer associated with the current child layer index.
			const GPlatesPresentation::VisualLayers &visual_layers = d_view_state_ptr->get_visual_layers();
			boost::weak_ptr<const GPlatesPresentation::VisualLayer> visual_layer =
					visual_layers.get_visual_layer_at_child_layer_index(child_rendered_geometry_layer_index);
			if (visual_layer.expired())
			{
				// Did not find the associated visual layer, so ignore.
				// This shouldn't happen though.
				// FIXME: Probably should assert.
				continue;
			}

			// The visible ReconstructionGeometry objects in the current rendered geometry layer.
			const GPlatesViewOperations::RenderedGeometryUtils::reconstruction_geom_seq_type &
					visible_reconstruction_geometries = child_rendered_geometry_layer_iter->second;

			// Matching RFGs in the current layer will go here.
			reconstructed_feature_geometry_collection_type matching_reconstructed_feature_geometries;

			// Narrow the visible ReconstructionGeometry objects down to visible ReconstructFeatureGeometry objects.
			std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry *> visible_reconstructed_feature_geometries;
			if (GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type_sequence(
				visible_reconstruction_geometries.begin(),
				visible_reconstruction_geometries.end(),
				visible_reconstructed_feature_geometries))
			{
				// Iterate over the RFGs.
				std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry *>::const_iterator rfg_iter =
					visible_reconstructed_feature_geometries.begin();
				std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry *>::const_iterator rfg_end =
					visible_reconstructed_feature_geometries.end();
				for (; rfg_iter != rfg_end; ++rfg_iter)
				{
					const GPlatesAppLogic::ReconstructedFeatureGeometry *rfg = *rfg_iter;

					// Make sure the current RFG was created from the same reconstruction tree as the focused geometry.
					if (rfg->get_reconstruction_tree() != *d_reconstruction_tree)
					{
						continue;
					}

					// It's an RFG, so let's look at its reconstruction plate ID property (if there is one).
					if (rfg->reconstruction_plate_id())
					{
						// OK, so the RFG *does* have a reconstruction plate ID.
						GPlatesModel::integer_plate_id_type rfg_plate_id = rfg->reconstruction_plate_id().get();
						if (std::find(plate_id_collection.begin(), plate_id_collection.end(), rfg_plate_id) !=
							plate_id_collection.end())
						{
							matching_reconstructed_feature_geometries.push_back(rfg->get_non_null_pointer_to_const());
						}
					}
				}
			}

			// NOTE: We only insert an entry into the map for layers that actually contain matching recon geoms.
			// This is important otherwise the drawing code will have to iterate over all available layers
			// and set up layer rendering even if there is nothing in those layers.
			if (!matching_reconstructed_feature_geometries.empty())
			{
				// Record the visual layer and its matching RFGs.
				d_visual_layer_reconstructed_feature_geometries[visual_layer].swap(matching_reconstructed_feature_geometries);
			}
		}
	}

	// NOTE: No longer emit warning since we could get here when the layer visibility is turned off.
#if 0
	if (d_reconstructed_feature_geometries.empty())
	{
		// That's pretty strange.  We expected at least one geometry here, or else, what's
		// the user dragging?
		qWarning() << "No initial geometries found ModifyReconstructionPoleWidget::populate_initial_geometries!";
	}
#endif
}


void
GPlatesQtWidgets::ModifyReconstructionPoleWidget::draw_initial_geometries()
{
	populate_initial_geometries();

	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block. This is so we can do multiple changes
	// without redrawing canvas after each change.
	// This should ideally be located at the highest level to capture one
	// user GUI interaction - the user performs an action and we update canvas once.
	// But since these guards can be nested it's probably a good idea to have it here too.
	GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;


	// Clear all initial geometry RenderedGeometry's before adding new ones.
	d_initial_geom_layer_ptr->clear_rendered_geometries();
	d_dragged_geom_layer_ptr->clear_rendered_geometries();

	// Use a white colour.
	draw_geometries(
			*d_initial_geom_layer_ptr,
			GPlatesGui::Colour::get_white());
}


void
GPlatesQtWidgets::ModifyReconstructionPoleWidget::draw_dragged_geometries()
{
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block. This is so we can do multiple changes
	// without redrawing canvas after each change.
	// This should ideally be located at the highest level to capture one
	// user GUI interaction - the user performs an action and we update canvas once.
	// But since these guards can be nested it's probably a good idea to have it here too.
	GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;

	// Clear all dragged geometry RenderedGeometry's before adding new ones.
	d_dragged_geom_layer_ptr->clear_rendered_geometries();

	// Be careful that the boost::optional is not boost::none.
	if ( ! d_drag_orientation)
	{
		return;
	}

	const GPlatesGui::Colour &silver_colour = GPlatesGui::Colour::get_silver();

	// Use a silver colour and rotate geometries in the RFGs.
	draw_geometries(
			*d_dragged_geom_layer_ptr,
			GPlatesGui::Colour::get_silver(),
			d_drag_orientation->get_accumulated_orientation());
}


void
GPlatesQtWidgets::ModifyReconstructionPoleWidget::draw_geometries(
		GPlatesViewOperations::RenderedGeometryLayer &rendered_geometry_layer,
		const GPlatesGui::Colour &colour,
		const boost::optional<GPlatesMaths::Rotation> &reconstruction_adjustment)
{
	// Iterate over the visual layers.
	// Each one is associated with a visual layer that has its own symboliser.
	visual_layer_reconstructed_feature_geometry_collection_map_type::const_iterator visual_layer_iter =
		d_visual_layer_reconstructed_feature_geometries.begin();
	visual_layer_reconstructed_feature_geometry_collection_map_type::const_iterator visual_layer_end =
		d_visual_layer_reconstructed_feature_geometries.end();
	for (; visual_layer_iter != visual_layer_end; ++visual_layer_iter)
	{
		boost::shared_ptr<const GPlatesPresentation::VisualLayer> visual_layer = visual_layer_iter->first.lock();
		if (!visual_layer)
		{
			// Visual layer no longer exists for some reason, so ignore it.
			continue;
		}

		const GPlatesPresentation::VisualLayerParams::non_null_ptr_to_const_type visual_layer_params =
				visual_layer->get_visual_layer_params();
		const GPlatesPresentation::ReconstructionGeometrySymboliser &reconstruction_geometry_symboliser =
				visual_layer_params->get_reconstruction_geometry_symboliser();

		GPlatesPresentation::ReconstructionGeometryRenderer::RenderParamsPopulator render_params_populator(
				d_view_state_ptr->get_rendered_geometry_parameters());
		visual_layer_params->accept_visitor(render_params_populator);

		GPlatesPresentation::ReconstructionGeometryRenderer::RenderParams render_params =
				render_params_populator.get_render_params();
		render_params.reconstruction_line_width_hint =
				GPlatesViewOperations::RenderedLayerParameters::POLE_MANIPULATION_LINE_WIDTH_HINT;
		render_params.reconstruction_point_size_hint =
				GPlatesViewOperations::RenderedLayerParameters::POLE_MANIPULATION_POINT_SIZE_HINT;
		// Ensure filled polygons are fully opaque (it's possible the layer has set a translucent opacity).
		render_params.fill_modulate_colour = GPlatesGui::Colour::get_white();

		// This creates the RenderedGeometry's from the ReconstructedFeatureGeometry's.
		GPlatesPresentation::ReconstructionGeometryRenderer reconstruction_geometry_renderer(
				render_params,
				d_view_state_ptr->get_render_settings(),
				reconstruction_geometry_symboliser,
				d_application_state_ptr->get_current_topological_sections(),
				colour,
				reconstruction_adjustment,
				boost::none);

		reconstruction_geometry_renderer.begin_render(rendered_geometry_layer);

		const reconstructed_feature_geometry_collection_type &reconstructed_feature_geometries =
				visual_layer_iter->second;
		reconstructed_feature_geometry_collection_type::const_iterator rfg_iter =
				reconstructed_feature_geometries.begin();
		reconstructed_feature_geometry_collection_type::const_iterator rfg_end =
				reconstructed_feature_geometries.end();
		for ( ; rfg_iter != rfg_end; ++rfg_iter)
		{
			// Visit the RFG with the renderer.
			(*rfg_iter)->accept_visitor(reconstruction_geometry_renderer);
		}

		reconstruction_geometry_renderer.end_render();
	}
}


void
GPlatesQtWidgets::ModifyReconstructionPoleWidget::draw_adjustment_pole()
{
	// Clear current pole rendered geometry first.
	d_adjustment_pole_layer_ptr->clear_rendered_geometries();

	// We should only be rendering the pole if it's currently enabled.
	if (d_move_pole_widget.get_pole())
	{
		// Render the pole as a very non-intrusive semi-transparent arrow with cross symbol.
		const GPlatesViewOperations::RenderedGeometry adjustment_pole_arrow_rendered_geom =
				GPlatesViewOperations::RenderedGeometryFactory::create_rendered_radial_arrow(
						d_move_pole_widget.get_pole().get(),
						0.3f/*arrow_projected_length*/,
						0.12f/*arrowhead_projected_size*/,
						0.5f/*ratio_arrowline_width_to_arrowhead_size*/,
						GPlatesGui::Colour(1, 1, 1, 0.5f)/*arrow_colour*/,
						GPlatesViewOperations::RenderedRadialArrow::SYMBOL_CIRCLE_WITH_CROSS/*symbol_type*/,
						10.0f/*symbol_size*/,
						GPlatesGui::Colour::get_white()/*symbol_colour*/);
		d_adjustment_pole_layer_ptr->add_rendered_geometry(adjustment_pole_arrow_rendered_geom);
	}
}


void
GPlatesQtWidgets::ModifyReconstructionPoleWidget::react_adjustment_pole_changed()
{
	if (d_is_active)
	{
		draw_adjustment_pole();
	}
}


void
GPlatesQtWidgets::ModifyReconstructionPoleWidget::update_adjustment_fields()
{
	if ( ! d_drag_orientation)
	{
		// No idea why the boost::optional is boost::none here, but let's not crash!
		// FIXME:  Complain about this.
		return;
	}
	ApplyReconstructionPoleAdjustmentDialog::fill_in_fields_for_rotation(
			field_adjustment_lat, field_adjustment_lon, spinbox_adjustment_angle,
			d_drag_orientation->get_accumulated_orientation());
}


void
GPlatesQtWidgets::ModifyReconstructionPoleWidget::clear_and_reset_after_reconstruction()
{
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block. This is so we can do multiple changes
	// without redrawing canvas after each change.
	// This should ideally be located at the highest level to capture one
	// user GUI interaction - the user performs an action and we update canvas once.
	// But since these guards can be nested it's probably a good idea to have it here too.
	GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;

	// Clear all RenderedGeometry's.
	d_initial_geom_layer_ptr->clear_rendered_geometries();
	d_dragged_geom_layer_ptr->clear_rendered_geometries();

	reset_adjustment();
	draw_initial_geometries_at_activation();
}


void
GPlatesQtWidgets::ModifyReconstructionPoleWidget::make_signal_slot_connections(
		GPlatesPresentation::ViewState &view_state)
{
	// The user wants to apply the current adjustment.
	QObject::connect(button_apply, SIGNAL(clicked()),
		this, SLOT(apply()));

	// Communication between the Apply ... Adjustment dialog and the Adjustment Applicator.
	QObject::connect(d_dialog_ptr, SIGNAL(pole_sequence_choice_changed(int)),
		d_applicator_ptr.get(), SLOT(handle_pole_sequence_choice_changed(int)));
	QObject::connect(d_dialog_ptr, SIGNAL(pole_sequence_choice_cleared()),
		d_applicator_ptr.get(), SLOT(handle_pole_sequence_choice_cleared()));
	QObject::connect(d_dialog_ptr, SIGNAL(accepted()),
		d_applicator_ptr.get(), SLOT(apply_adjustment()));

	// The user has agreed to apply the adjustment as described in the dialog.
	QObject::connect(d_applicator_ptr.get(), SIGNAL(have_reconstructed()),
		this, SLOT(clear_and_reset_after_reconstruction()));

	// Connect the reconstruction pole widget to the feature focus.
	QObject::connect(
			&view_state.get_feature_focus(),
			SIGNAL(focus_changed(
					GPlatesGui::FeatureFocus &)),
			this,
			SLOT(set_focus(
					GPlatesGui::FeatureFocus &)));

	// The Reconstruction Pole widget needs to know when the reconstruction time changes.
	QObject::connect(d_application_state_ptr,
			SIGNAL(reconstructed(GPlatesAppLogic::ApplicationState &)),
			this,
			SLOT(handle_reconstruction()));
			
	// Respond to changes in the "Highlight children" checkbox.
	QObject::connect(checkbox_highlight_children, SIGNAL(stateChanged(int)),
			this, SLOT(change_highlight_children_checkbox_state(int)));			

	// Listen for pole changes due to the Move Pole widget (since we draw the pole location).
	QObject::connect(
			&d_move_pole_widget, SIGNAL(pole_changed(boost::optional<GPlatesMaths::PointOnSphere>)),
			this, SLOT(react_adjustment_pole_changed()));

	// Re-populate the visible RFGs when a layer is made visible/invisible.
	QObject::connect(
			&view_state.get_visual_layers(), SIGNAL(layer_modified(size_t)),
			this, SLOT(handle_layer_modified()));
}


void
GPlatesQtWidgets::ModifyReconstructionPoleWidget::create_child_rendered_layers()
{
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block. This is so we can do multiple changes
	// without redrawing canvas after each change.
	// This should ideally be located at the highest level to capture one
	// user GUI interaction - the user performs an action and we update canvas once.
	// But since these guards can be nested it's probably a good idea to have it here too.
	GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;

	// Create a rendered layer to draw the initial geometries.
	d_initial_geom_layer_ptr =
		d_rendered_geom_collection->create_child_rendered_layer_and_transfer_ownership(
				GPlatesViewOperations::RenderedGeometryCollection::POLE_MANIPULATION_CANVAS_TOOL_WORKFLOW_LAYER);

	// Create a rendered layer to draw the initial geometries.
	// NOTE: this must be created second to get drawn on top.
	d_dragged_geom_layer_ptr =
		d_rendered_geom_collection->create_child_rendered_layer_and_transfer_ownership(
				GPlatesViewOperations::RenderedGeometryCollection::POLE_MANIPULATION_CANVAS_TOOL_WORKFLOW_LAYER);

	// Create a rendered layer to draw the optional adjustment pole location.
	d_adjustment_pole_layer_ptr =
		d_rendered_geom_collection->create_child_rendered_layer_and_transfer_ownership(
				GPlatesViewOperations::RenderedGeometryCollection::POLE_MANIPULATION_CANVAS_TOOL_WORKFLOW_LAYER);

	// In the cases above we store the returned object as a data member and it
	// automatically destroys the created layer for us when 'this' object is destroyed.
}


void
GPlatesQtWidgets::ModifyReconstructionPoleWidget::handle_activation()
{
}


QString
GPlatesQtWidgets::ModifyReconstructionPoleWidget::get_clear_action_text() const
{
	return tr("Re&set Rotation");
}


bool
GPlatesQtWidgets::ModifyReconstructionPoleWidget::clear_action_enabled() const
{
	return true;
}


void
GPlatesQtWidgets::ModifyReconstructionPoleWidget::handle_clear_action_triggered()
{
	reset();
}
