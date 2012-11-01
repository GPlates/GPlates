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
#include <QDebug>

#include "ActionButtonBox.h"
#include "ApplyReconstructionPoleAdjustmentDialog.h"
#include "ModifyReconstructionPoleWidget.h"
#include "QtWidgetUtils.h"
#include "ViewportWindow.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/LayerProxyUtils.h"
#include "app-logic/ReconstructLayerProxy.h"
#include "app-logic/ReconstructionGeometryUtils.h"
#include "app-logic/ReconstructionTree.h"

#include "feature-visitors/TotalReconstructionSequencePlateIdFinder.h"
#include "feature-visitors/TotalReconstructionSequenceTimePeriodFinder.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "gui/FeatureFocus.h"

#include "presentation/ReconstructionGeometryRenderer.h"
#include "presentation/ViewState.h"

#include "view-operations/RenderedGeometryFactory.h"
#include "view-operations/RenderedGeometryParameters.h"


GPlatesQtWidgets::ModifyReconstructionPoleWidget::ModifyReconstructionPoleWidget(
		GPlatesPresentation::ViewState &view_state,
		ViewportWindow &viewport_window,
		QAction *clear_action,
		QWidget *parent_):
	TaskPanelWidget(parent_),
	d_application_state_ptr(&view_state.get_application_state()),
	d_rendered_geom_collection(&view_state.get_rendered_geometry_collection()),
	d_dialog_ptr(new ApplyReconstructionPoleAdjustmentDialog(&viewport_window)),
	d_applicator_ptr(new AdjustmentApplicator(view_state, *d_dialog_ptr)),
	d_should_constrain_latitude(false),
	d_should_display_children(false),
	d_is_active(false),
	d_view_state_ptr(&view_state)
{
	setupUi(this);

	// Set up the action button box showing the reset button.
	ActionButtonBox *action_button_box = new ActionButtonBox(1, 16, this);
	action_button_box->add_action(clear_action);
#ifndef Q_WS_MAC
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

	// Activate both rendered layers.
	d_initial_geom_layer_ptr->set_active();
	d_dragged_geom_layer_ptr->set_active();

	set_focus(d_view_state_ptr->get_feature_focus());
	draw_initial_geometries_at_activation();
}


void
GPlatesQtWidgets::ModifyReconstructionPoleWidget::deactivate()
{
	// Disable the task panel widget.
	setEnabled(false);

	d_is_active = false;

	// Deactivate both rendered layers.
	d_initial_geom_layer_ptr->set_active(false);
	d_dragged_geom_layer_ptr->set_active(false);
}


void
GPlatesQtWidgets::ModifyReconstructionPoleWidget::draw_initial_geometries_at_activation()
{
	d_reconstructed_feature_geometries.clear();
	populate_initial_geometries();
	draw_dragged_geometries();
}


namespace
{
	/**
	 * Return the closest point on the equator to @a p.
	 *
	 * This result point will be at the same longitude as @a p.
	 *
	 * If @p is at the North Pole or South Pole, there is no unique "closest" point to @a p on
	 * the equator, so boost::none will be returned.
	 */
	const boost::optional<GPlatesMaths::PointOnSphere>
	get_closest_point_on_equator(
			const GPlatesMaths::PointOnSphere &p)
	{
		using namespace GPlatesMaths;

		if (abs(p.position_vector().z()) >= 1.0) {
			// The point is on the North Pole or South Pole.  Hence, there is no unique
			// "closest" point on the equator.
			return boost::none;
		}
		// Else, the point is _not_ on the North Pole or South Pole, meaning there *is* a
		// unique "closest" point on the equator.  Hence, we can proceed.

		// Since the point is on neither the North Pole nor South Pole, it lies on a
		// well-defined small-circle of latitude (ie, a small-circle of latitude with a
		// non-zero radius).
		//
		// Equivalently, since the point is on neither the North Pole nor South Pole, the
		// z-coord of the unit-vector must lie in the range [-1, 1].  Hence, at least one
		// of the x- and y-coords must be non-zero, which means that the following radius
		// result will be greater than zero.
		const double &x = p.position_vector().x().dval();
		const double &y = p.position_vector().y().dval();
		double radius_of_small_circle = std::sqrt(x*x + y*y);

		// Since the radius is greater than zero, the following fraction is well-defined.
		double one_on_radius = 1.0 / radius_of_small_circle;

		return PointOnSphere(UnitVector3D(one_on_radius * x, one_on_radius * y, 0.0));
	}
	

	void
	add_child_edges_to_collection(
		GPlatesAppLogic::ReconstructionTreeEdge::non_null_ptr_type edge,
		std::vector<GPlatesModel::integer_plate_id_type> &child_plate_id_collection)
	{	
		std::vector<GPlatesAppLogic::ReconstructionTreeEdge::non_null_ptr_type>::iterator it;
		std::vector<GPlatesAppLogic::ReconstructionTreeEdge::non_null_ptr_type>::iterator 
			it_begin = edge->children_in_built_tree().begin();
		std::vector<GPlatesAppLogic::ReconstructionTreeEdge::non_null_ptr_type>::iterator 
			it_end = edge->children_in_built_tree().end();

		for(it = it_begin; it != it_end ; it++)
		{
			child_plate_id_collection.push_back((*it)->moving_plate());
			add_child_edges_to_collection(*it,child_plate_id_collection);
		}		
	}

	void
	add_children_to_geometry_collection(
		std::vector<GPlatesModel::integer_plate_id_type> &child_plate_id_collection,
		const GPlatesModel::integer_plate_id_type plate_id,
		const GPlatesAppLogic::ReconstructionTree &tree)
	{
		GPlatesAppLogic::ReconstructionTree::edge_refs_by_plate_id_map_const_range_type 
			edges = tree.find_edges_whose_moving_plate_id_match(plate_id);

		if (edges.first == edges.second)
		{
			// We haven't found any edges. That'ok, we might just not have a
			// rotation file loaded. 
			//qDebug() << "Empty edge container for plate id " << plate_id;
			return;
		}

		// We shouldn't have more than one edge - even in a cross-over situation, one
		// of the edges will already have been selected for use in the tree	
 		if (std::distance(edges.first,edges.second) > 1)
		{
			qDebug() << "More than one edge found for plate id " << plate_id;
			return;			
		}

		GPlatesAppLogic::ReconstructionTree::edge_refs_by_plate_id_map_const_iterator
			it = edges.first;

		for (; it != edges.second ; ++it)
		{
			add_child_edges_to_collection(it->second,child_plate_id_collection);
		}

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
}


void
GPlatesQtWidgets::ModifyReconstructionPoleWidget::start_new_drag(
		const GPlatesMaths::PointOnSphere &current_oriented_position)
{
	if ( ! d_accum_orientation) {
		d_accum_orientation.reset(new GPlatesGui::SimpleGlobeOrientation());
	}
	if (d_should_constrain_latitude) {
		d_drag_start = get_closest_point_on_equator(current_oriented_position);
		if (d_drag_start) {
			d_accum_orientation->set_new_handle_at_pos(*d_drag_start);
		}
		// Else, the drag-start was at the North Pole or South Pole, so there was no unique
		// "closest" point to the equator; don't try to do anything until the drag leaves
		// the poles.
	} else {
		d_accum_orientation->set_new_handle_at_pos(current_oriented_position);
	}
}


namespace
{
	/**
	 * Return the closest point on the horizon to @a oriented_point_within_horizon.
	 *
	 * If @a oriented_point_within_horizon is either coincident with the centre of the
	 * viewport, or (somehow) antipodal to the centre of the viewport, boost::none will be
	 * returned.
	 */
	const boost::optional<GPlatesMaths::PointOnSphere>
	get_closest_point_on_horizon(
			const GPlatesMaths::PointOnSphere &oriented_point_within_horizon,
			const GPlatesMaths::PointOnSphere &oriented_center_of_viewport)
	{
		using namespace GPlatesMaths;

		if (collinear(oriented_point_within_horizon.position_vector(),
					oriented_center_of_viewport.position_vector())) {
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
				cross(oriented_point_within_horizon.position_vector(),
						oriented_center_of_viewport.position_vector());
		// Since the two unit-vectors are non-collinear, we can assume the cross-product is
		// a non-zero vector.
		UnitVector3D normal_to_plane = cross_result.get_normalisation();

		Vector3D point_on_horizon =
				cross(oriented_center_of_viewport.position_vector(), normal_to_plane);
		// Since both the center-of-viewport and normal-to-plane are unit-vectors, and they
		// are (by definition) perpendicular, we will assume the result is of unit length.
		return PointOnSphere(point_on_horizon.get_normalisation());
	}
}


void
GPlatesQtWidgets::ModifyReconstructionPoleWidget::start_new_rotation_drag(
		const GPlatesMaths::PointOnSphere &current_oriented_position,
		const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport)
{
	if (d_should_constrain_latitude) {
		// Rotation-dragging of the plate is disabled when the "Constrain Latitude"
		// checkbox is checked:  When you're constraining the latitude, you're also
		// implicitly constraining the plate orientation, since you want the VGP pole
		// position to remain in its current position.
		//
		// Hence, nothing to do in this function.
		return;
	}

	boost::optional<GPlatesMaths::PointOnSphere> point_on_horizon =
			get_closest_point_on_horizon(current_oriented_position, oriented_centre_of_viewport);
	if ( ! point_on_horizon) {
		// The mouse position could not be converted to a point on the horizon.  Presumably
		// the it was at the centre of the viewport.  Hence, nothing to be done.
		return;
	}
	if ( ! d_accum_orientation) {
		d_accum_orientation.reset(new GPlatesGui::SimpleGlobeOrientation());
	}
	d_accum_orientation->set_new_handle_at_pos(*point_on_horizon);
}


void
GPlatesQtWidgets::ModifyReconstructionPoleWidget::update_drag_position(
		const GPlatesMaths::PointOnSphere &current_oriented_position)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_accum_orientation != NULL,
			GPLATES_ASSERTION_SOURCE);

	if (d_should_constrain_latitude) {
		if ( ! d_drag_start) {
			// We haven't set the drag start yet.  The mouse pointer must have been at
			// either the North Pole or South Pole.  The first thing we should try to
			// do is start the drag now.
			d_drag_start = get_closest_point_on_equator(current_oriented_position);
			if (d_drag_start) {
				d_accum_orientation->set_new_handle_at_pos(*d_drag_start);
			}
			// Else, the drag-start was at the North Pole or South Pole, so there was
			// no unique "closest" point to the equator; don't try to do anything until
			// the drag leaves the poles.
		} else {
			boost::optional<GPlatesMaths::PointOnSphere> drag_update =
					get_closest_point_on_equator(current_oriented_position);
			if (drag_update) {
				d_accum_orientation->move_handle_to_pos(*drag_update);
			}
			// Else, the drag-update was at the North Pole or South Pole, so there was
			// no unique "closest" point to the equator; don't try to do anything until
			// the drag leaves the poles.
		}
	} else {
		d_accum_orientation->move_handle_to_pos(current_oriented_position);
	}

	draw_dragged_geometries();
	update_adjustment_fields();
}


void
GPlatesQtWidgets::ModifyReconstructionPoleWidget::update_rotation_drag_position(
		const GPlatesMaths::PointOnSphere &current_oriented_position,
		const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport)
{
	if (d_should_constrain_latitude) {
		// Rotation-dragging of the plate is disabled when the "Constrain Latitude"
		// checkbox is checked:  When you're constraining the latitude, you're also
		// implicitly constraining the plate orientation, since you want the VGP pole
		// position to remain in its current position.
		//
		// Hence, nothing to do in this function.
		return;
	}

	if ( ! d_accum_orientation) {
		// We must be in the middle of a non-drag.  Perhaps the user tried to drag at the
		// centre of the viewport, for instance.
		return;
	}

	boost::optional<GPlatesMaths::PointOnSphere> point_on_horizon =
			get_closest_point_on_horizon(current_oriented_position, oriented_centre_of_viewport);
	if ( ! point_on_horizon) {
		// The mouse position could not be converted to a point on the horizon.  Presumably
		// the it was at the centre of the viewport.  Hence, nothing to be done.
		return;
	}
	d_accum_orientation->move_handle_to_pos(*point_on_horizon);

	draw_dragged_geometries();
	update_adjustment_fields();
}


void
GPlatesQtWidgets::ModifyReconstructionPoleWidget::end_drag()
{  }


namespace
{
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
				trs_plate_id_finder.moving_ref_frame_plate_id())) {
			// This feature was missing one (or both) of the plate IDs which a TRS is
			// supposed to have.  Skip this feature.
			return;
		}
		// Else, we know it found both of the required plate IDs.

		if (*trs_plate_id_finder.fixed_ref_frame_plate_id() ==
				*trs_plate_id_finder.moving_ref_frame_plate_id()) {
			// The fixed ref-frame plate ID equals the moving ref-frame plate ID? 
			// Something strange is going on here.  Skip this feature.
			return;
		}

		// Dietmar has said that he doesn't want the table to include pole sequences for
		// which the plate ID of interest is the fixed ref-frame.  (2008-09-18)
#if 0
		if (*trs_plate_id_finder.fixed_ref_frame_plate_id() == plate_id_of_interest) {
			trs_time_period_finder.reset();
			trs_time_period_finder.visit_feature(current_feature);
			if ( ! (trs_time_period_finder.begin_time() && trs_time_period_finder.end_time())) {
				// No time samples were found.  Skip this feature.
				return;
			}

			// For now, let's _not_ include sequences which don't span this
			// reconstruction time.
			GPlatesPropertyValues::GeoTimeInstant current_time(reconstruction_time);
			if (trs_time_period_finder.begin_time()->is_strictly_later_than(current_time) ||
					trs_time_period_finder.end_time()->is_strictly_earlier_than(current_time)) {
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
		if (*trs_plate_id_finder.moving_ref_frame_plate_id() == plate_id_of_interest) {
			trs_time_period_finder.reset();
			trs_time_period_finder.visit_feature(current_feature);
			if ( ! (trs_time_period_finder.begin_time() && trs_time_period_finder.end_time())) {
				// No time samples were found.  Skip this feature.
				return;
			}

			// For now, let's _not_ include sequences which don't span this
			// reconstruction time.
			GPlatesPropertyValues::GeoTimeInstant current_time(reconstruction_time);
			if (trs_time_period_finder.begin_time()->is_strictly_later_than(current_time) ||
					trs_time_period_finder.end_time()->is_strictly_earlier_than(current_time)) {
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
			std::vector<GPlatesQtWidgets::ApplyReconstructionPoleAdjustmentDialog::PoleSequenceInfo> &
					sequence_choices,
			GPlatesFeatureVisitors::TotalReconstructionSequencePlateIdFinder &trs_plate_id_finder,
			GPlatesFeatureVisitors::TotalReconstructionSequenceTimePeriodFinder &trs_time_period_finder,
			GPlatesModel::integer_plate_id_type plate_id_of_interest,
			const GPlatesAppLogic::ReconstructionTree &reconstruction_tree)
	{
		using namespace GPlatesModel;

		std::vector<FeatureCollectionHandle::weak_ref>::const_iterator collections_iter =
				reconstruction_tree.get_reconstruction_features().begin();
		std::vector<FeatureCollectionHandle::weak_ref>::const_iterator collections_end =
				reconstruction_tree.get_reconstruction_features().end();
		for ( ; collections_iter != collections_end; ++collections_iter) {
			const FeatureCollectionHandle::weak_ref &current_collection = *collections_iter;
			if ( ! current_collection.is_valid()) {
				// FIXME:  Should we do anything about this? Or is this acceptable?
				// (If the collection is not valid, then presumably it has been
				// unloaded.  In which case, why hasn't the reconstruction been
				// recalculated?)
				continue;
			}

			FeatureCollectionHandle::iterator features_iter = current_collection->begin();
			FeatureCollectionHandle::iterator features_end = current_collection->end();
			for ( ; features_iter != features_end; ++features_iter) {
				examine_trs(sequence_choices, trs_plate_id_finder,
						trs_time_period_finder, plate_id_of_interest,
						reconstruction_tree.get_reconstruction_time(), features_iter);
			}
		}
	}
}


void
GPlatesQtWidgets::ModifyReconstructionPoleWidget::apply()
{
	if ( ! d_accum_orientation) {
		// The user must have released the mouse button after a non-drag.  Perhaps the user
		// tried to drag at the centre of the viewport, for instance.
		return;
	}

	if ( ! d_plate_id || ! d_reconstruction_tree) {
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

	find_trses(sequence_choices, trs_plate_id_finder, trs_time_period_finder, *d_plate_id,
			 **d_reconstruction_tree);

	// The Applicator should be set before the dialog is set up.
	// Why, you ask?  Because when the dialog is set up, the first row in the sequence choices
	// table will be selected, which will send a signal which will trigger a slot in the
	// Applicator, which will not do anything useful unless the Applicator has been set.
	d_applicator_ptr->set(
			sequence_choices,
			d_accum_orientation->rotation(),
			*d_reconstruction_tree);
	d_dialog_ptr->setup_for_new_pole(
			*d_plate_id,
			d_application_state_ptr->get_current_reconstruction_time(),
			sequence_choices,
			d_accum_orientation->rotation());

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
	d_accum_orientation.reset();

	// Update the "Adjustment" fields in the TaskPanel pane.
	field_adjustment_lat->clear();
	field_adjustment_lon->clear();
	spinbox_adjustment_angle->setValue(0.0);
}


void
GPlatesQtWidgets::ModifyReconstructionPoleWidget::change_constrain_latitude_checkbox_state(
		int new_checkbox_state)
{
	if (new_checkbox_state == Qt::Unchecked) {
		d_should_constrain_latitude = false;
	}
	else if (new_checkbox_state == Qt::Checked) {
		d_should_constrain_latitude = true;
	}
	// Ignore any other values of 'new_checkbox_state'.
}

void
GPlatesQtWidgets::ModifyReconstructionPoleWidget::change_highlight_children_checkbox_state(
	int new_checkbox_state)
{
	if (new_checkbox_state == Qt::Unchecked) {
		d_should_display_children = false;
	}
	else if (new_checkbox_state == Qt::Checked) {
		d_should_display_children = true;
	}

	// Ignore any other values of 'new_checkbox_state'.
	d_reconstructed_feature_geometries.clear();
	draw_initial_geometries();
	draw_dragged_geometries();
}

void
GPlatesQtWidgets::ModifyReconstructionPoleWidget::set_focus(
		GPlatesGui::FeatureFocus &feature_focus)
{
	const GPlatesAppLogic::ReconstructionGeometry::maybe_null_ptr_to_const_type focused_geometry =
			feature_focus.associated_reconstruction_geometry();

	// We're only interested in ReconstructedFeatureGeometry's (resolved topologies are excluded
	// since they, in turn, reference reconstructed static geometries).
	// NOTE: ReconstructedVirtualGeomagneticPole's will also be included since they derive
	// from ReconstructedFeatureGeometry.
	boost::optional<const GPlatesAppLogic::ReconstructedFeatureGeometry *> rfg =
			focused_geometry ?
			GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
					const GPlatesAppLogic::ReconstructedFeatureGeometry>(focused_geometry) :
			boost::none;

	// Do the following if no RG or if RG is not RFG.
	if (!rfg)
	{
		// Clear the plate ID and the plate ID field.
		d_reconstruction_tree = boost::none;
		d_plate_id = boost::none;
		reset_adjustment();
		d_reconstructed_feature_geometries.clear();
		field_moving_plate->clear();
		// This is to clear the rendered geometries if the feature geometry
		// disappears when this tool is still active (eg, when a
		// feature collection is unloaded and its features should disappear).
		draw_initial_geometries();
		return;
	}

	d_reconstruction_tree = focused_geometry->reconstruction_tree();

	// Nothing to do if plate ID hasn't changed.
	if (d_plate_id != rfg.get()->reconstruction_plate_id())
	{
		reset_adjustment();
		d_reconstructed_feature_geometries.clear();
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
	}
}


void
GPlatesQtWidgets::ModifyReconstructionPoleWidget::handle_reconstruction()
{
	set_focus(d_view_state_ptr->get_feature_focus());
	if (d_is_active) {
		draw_initial_geometries();
		draw_dragged_geometries();
	}
}


void
GPlatesQtWidgets::ModifyReconstructionPoleWidget::populate_initial_geometries()
{
	// If there's no plate ID of the currently-focused RFG, then there can be no other RFGs
	// with the same plate ID.
	if ( ! d_plate_id || ! d_reconstruction_tree) {
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
	// Iterate over all the reconstruction geometries that were reconstructed using the same
	// reconstruction tree as the focused feature geometry.
	//

	// Get the layer outputs.
	const GPlatesAppLogic::Reconstruction::layer_output_seq_type &layer_outputs =
			d_application_state_ptr->get_current_reconstruction().get_active_layer_outputs();

	// Find those layer outputs that come from a reconstruct layer.
	std::vector<GPlatesAppLogic::ReconstructLayerProxy *> reconstruct_outputs;
	if (GPlatesAppLogic::LayerProxyUtils::get_layer_proxy_derived_type_sequence(
			layer_outputs.begin(), layer_outputs.end(), reconstruct_outputs))
	{
		// Iterate over the *reconstruct* layers because...
		// ...we're only interested in ReconstructedFeatureGeometry's (resolved topologies are excluded
		// since they, in turn, reference reconstructed static geometries).
		// NOTE: ReconstructedVirtualGeomagneticPole's will also be included since they derive
		// from ReconstructedFeatureGeometry.
		BOOST_FOREACH(
				GPlatesAppLogic::ReconstructLayerProxy *reconstruct_layer_proxy,
				reconstruct_outputs)
		{
			// Get the reconstructed feature geometries from the current layer.
			typedef std::vector<GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type> rfg_seq_type;
			rfg_seq_type reconstructed_feature_geometries;
			reconstruct_layer_proxy->get_reconstructed_feature_geometries(reconstructed_feature_geometries);

			// Iterate over the RFGs.
			for (rfg_seq_type::const_iterator rfg_iter = reconstructed_feature_geometries.begin();
				rfg_iter != reconstructed_feature_geometries.end();
				++rfg_iter)
			{
				const GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type &rfg = *rfg_iter;

				// Make sure the current RFG was created from the same reconstruction tree as
				// the focused geometry.
				if (rfg->reconstruction_tree() != *d_reconstruction_tree)
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
						d_reconstructed_feature_geometries.push_back(rfg->get_non_null_pointer_to_const());
					}
				}
			}
		}
	}

	if (d_reconstructed_feature_geometries.empty()) {
		// That's pretty strange.  We expected at least one geometry here, or else, what's
		// the user dragging?
		qWarning() << "No initial geometries found ModifyReconstructionPoleWidget::populate_initial_geometries!";
	}
	
}


void
GPlatesQtWidgets::ModifyReconstructionPoleWidget::draw_initial_geometries()
{
	
	d_reconstructed_feature_geometries.clear();
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

	const GPlatesGui::Colour &white_colour = GPlatesGui::Colour::get_white();

	// FIXME: Probably should use the same styling params used to draw
	// the original geometries rather than use some of the defaults.
	GPlatesPresentation::ReconstructionGeometryRenderer::RenderParams render_style_params;
	render_style_params.reconstruction_line_width_hint =
			GPlatesViewOperations::RenderedLayerParameters::POLE_MANIPULATION_LINE_WIDTH_HINT;
	render_style_params.reconstruction_point_size_hint =
			GPlatesViewOperations::RenderedLayerParameters::POLE_MANIPULATION_POINT_SIZE_HINT;

	// This creates the RenderedGeometry's from the ReconstructedFeatureGeomtries
	// using the colour 'white_colour'.
	//
	// Note that we don't have the feature_type_to_symbol_map available in this class
	// at the moment, so we can't pass it to the Renderer, so any symbols will just
	// get rendered as regular point-on-spheres.
	GPlatesPresentation::ReconstructionGeometryRenderer initial_geometry_renderer(
			render_style_params,
			white_colour,
			boost::none,
			boost::none);

	initial_geometry_renderer.begin_render();

	reconstructed_feature_geometry_collection_type::const_iterator rfg_iter =
			d_reconstructed_feature_geometries.begin();
	reconstructed_feature_geometry_collection_type::const_iterator rfg_end =
			d_reconstructed_feature_geometries.end();
	for ( ; rfg_iter != rfg_end; ++rfg_iter)
	{
		// Visit the RFG with the renderer.
		(*rfg_iter)->accept_visitor(initial_geometry_renderer);
	}

	initial_geometry_renderer.end_render(*d_initial_geom_layer_ptr);
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

	// Be careful that the boost::optional is not boost::none.
	if ( ! d_accum_orientation) {
		draw_initial_geometries();
		return;
	}

	// Clear all dragged geometry RenderedGeometry's before adding new ones.
	d_dragged_geom_layer_ptr->clear_rendered_geometries();

	const GPlatesGui::Colour &silver_colour = GPlatesGui::Colour::get_silver();

	// FIXME: Probably should use the same styling params used to draw
	// the original geometries rather than use some of the defaults.
	GPlatesPresentation::ReconstructionGeometryRenderer::RenderParams render_style_params;
	render_style_params.reconstruction_line_width_hint =
			GPlatesViewOperations::RenderedLayerParameters::POLE_MANIPULATION_LINE_WIDTH_HINT;
	render_style_params.reconstruction_point_size_hint =
			GPlatesViewOperations::RenderedLayerParameters::POLE_MANIPULATION_POINT_SIZE_HINT;

	// This creates the RenderedGeometry's from the ReconstructedFeatureGeomtries
	// using the colour 'silver_colour' and rotates geometries in the RFGs.
	GPlatesPresentation::ReconstructionGeometryRenderer dragged_geometry_renderer(
			render_style_params,
			silver_colour,
			d_accum_orientation->rotation(),
			boost::none);

	dragged_geometry_renderer.begin_render();

	reconstructed_feature_geometry_collection_type::const_iterator rfg_iter =
			d_reconstructed_feature_geometries.begin();
	reconstructed_feature_geometry_collection_type::const_iterator rfg_end =
			d_reconstructed_feature_geometries.end();
	for ( ; rfg_iter != rfg_end; ++rfg_iter)
	{
		// Visit the RFG with the renderer.
		(*rfg_iter)->accept_visitor(dragged_geometry_renderer);
	}

	dragged_geometry_renderer.end_render(*d_dragged_geom_layer_ptr);
}


void
GPlatesQtWidgets::ModifyReconstructionPoleWidget::update_adjustment_fields()
{
	if ( ! d_accum_orientation) {
		// No idea why the boost::optional is boost::none here, but let's not crash!
		// FIXME:  Complain about this.
		return;
	}
	ApplyReconstructionPoleAdjustmentDialog::fill_in_fields_for_rotation(
			field_adjustment_lat, field_adjustment_lon, spinbox_adjustment_angle,
			d_accum_orientation->rotation());
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

	// Respond to changes in the "Constrain Latitude" checkbox.
	QObject::connect(checkbox_constrain_latitude, SIGNAL(stateChanged(int)),
			this, SLOT(change_constrain_latitude_checkbox_state(int)));
			
	// Respond to changes in the "Highlight children" checkbox.
	QObject::connect(checkbox_highlight_children, SIGNAL(stateChanged(int)),
			this, SLOT(change_highlight_children_checkbox_state(int)));			
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

	// In both cases above we store the returned object as a data member and it
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
