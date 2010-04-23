/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008, 2009 The University of Sydney, Australia
 *(as "ReconstructionPoleWidget.cc")
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

#include "ApplyReconstructionPoleAdjustmentDialog.h"
#include "ModifyReconstructionPoleWidget.h"
#include "ViewportWindow.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/PaleomagUtils.h"
#include "app-logic/ReconstructionGeometryUtils.h"
#include "feature-visitors/TotalReconstructionSequencePlateIdFinder.h"
#include "feature-visitors/TotalReconstructionSequenceTimePeriodFinder.h"
#include "gui/FeatureFocus.h"
#include "model/ReconstructionTree.h"
#include "presentation/ViewState.h"
#include "view-operations/RenderedGeometryFactory.h"
#include "view-operations/RenderedGeometryParameters.h"


GPlatesQtWidgets::ModifyReconstructionPoleWidget::ModifyReconstructionPoleWidget(
		GPlatesPresentation::ViewState &view_state,
		ViewportWindow &viewport_window,
		QWidget *parent_):
	QWidget(parent_),
	d_application_state_ptr(&view_state.get_application_state()),
	d_rendered_geom_collection(&view_state.get_rendered_geometry_collection()),
	d_dialog_ptr(new ApplyReconstructionPoleAdjustmentDialog(&viewport_window)),
	d_applicator_ptr(new AdjustmentApplicator(view_state, *d_dialog_ptr)),
	d_should_constrain_latitude(false),
	d_should_display_children(false),	
	d_view_state_ptr(&view_state)
{
	setupUi(this);

	make_signal_slot_connections(view_state);

	create_child_rendered_layers();
}


GPlatesQtWidgets::ModifyReconstructionPoleWidget::~ModifyReconstructionPoleWidget()
{
}


void
GPlatesQtWidgets::ModifyReconstructionPoleWidget::activate()
{
	d_is_active = true;
	draw_initial_geometries_at_activation();
}


void
GPlatesQtWidgets::ModifyReconstructionPoleWidget::deactivate()
{
	d_is_active = false;
}


void
GPlatesQtWidgets::ModifyReconstructionPoleWidget::draw_initial_geometries_at_activation()
{
	d_initial_geometries.clear();
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
		GPlatesModel::ReconstructionTreeEdge::non_null_ptr_type edge,
		std::vector<GPlatesModel::integer_plate_id_type> &child_plate_id_collection
		)
	{	
		std::vector<GPlatesModel::ReconstructionTreeEdge::non_null_ptr_type>::iterator it;
		std::vector<GPlatesModel::ReconstructionTreeEdge::non_null_ptr_type>::iterator 
			it_begin = edge->children_in_built_tree().begin();
		std::vector<GPlatesModel::ReconstructionTreeEdge::non_null_ptr_type>::iterator 
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
		GPlatesAppLogic::ApplicationState *application_state_ptr
		)
	{
		GPlatesModel::ReconstructionTree &tree =
				application_state_ptr->get_current_reconstruction().reconstruction_tree();
		GPlatesModel::ReconstructionTree::edge_refs_by_plate_id_map_const_range_type 
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

		GPlatesModel::ReconstructionTree::edge_refs_by_plate_id_map_const_iterator
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
			const GPlatesModel::FeatureHandle::weak_ref &current_feature)
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
							current_feature,
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
			const GPlatesModel::Reconstruction &reconstruction,
			const double &reconstruction_time)
	{
		using namespace GPlatesModel;

		std::vector<FeatureHandle::weak_ref>::const_iterator features_iter =
				reconstruction.reconstruction_tree().get_reconstruction_features().begin();
		std::vector<FeatureHandle::weak_ref>::const_iterator features_end =
				reconstruction.reconstruction_tree().get_reconstruction_features().end();
		for ( ; features_iter != features_end; ++features_iter) {
			const FeatureHandle::weak_ref &current_feature = *features_iter;
			if ( ! current_feature.is_valid()) {
				// FIXME:  Should we do anything about this? Or is this acceptable?
				// (If the handle is not valid, then presumably it belongs to a feature
				// collection that has been unloaded.  In which case, why hasn't the
				// reconstruction been recalculated?)
				continue;
			}

			examine_trs(sequence_choices, trs_plate_id_finder,
					trs_time_period_finder, plate_id_of_interest,
					reconstruction_time, current_feature);
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

	if ( ! d_plate_id) {
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
			d_application_state_ptr->get_current_reconstruction(),
			d_application_state_ptr->get_current_reconstruction_time());

	// The Applicator should be set before the dialog is set up.
	// Why, you ask?  Because when the dialog is set up, the first row in the sequence choices
	// table will be selected, which will send a signal which will trigger a slot in the
	// Applicator, which will not do anything useful unless the Applicator has been set.
	d_applicator_ptr->set(
			sequence_choices,
			d_accum_orientation->rotation(),
			d_application_state_ptr->get_current_reconstruction_time());
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
	d_initial_geometries.clear();
	draw_initial_geometries();
	draw_dragged_geometries();
}

void
GPlatesQtWidgets::ModifyReconstructionPoleWidget::set_focus(
		GPlatesGui::FeatureFocus &feature_focus)
{
	const GPlatesModel::ReconstructionGeometry::maybe_null_ptr_type focused_geometry =
			feature_focus.associated_reconstruction_geometry();

	if ( ! focused_geometry) {
		// No RG, so nothing we can do.

		// Clear the plate ID and the plate ID field.
		d_plate_id = boost::none;
		reset_adjustment();
		d_initial_geometries.clear();
		field_moving_plate->clear();
		// This is to clear the rendered geometries if the feature geometry
		// disappears when this tool is still active (eg, when a
		// feature collection is unloaded and its features should disappear).
		draw_initial_geometries();
		return;
	}

	// We're only interested in ReconstructedFeatureGeometry's (ResolvedTopologicalBoundary's,
	// for instance, are used to assign plate ids to regular features so we probably only
	// want to look at geometries of regular features).
	const GPlatesModel::ReconstructedFeatureGeometry *rfg = NULL;
	if (!GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type(
			focused_geometry, rfg))
	{
		return;
	}

	if (d_plate_id == rfg->reconstruction_plate_id()) {
		// The plate ID hasn't changed, so there's nothing to do.
		return;
	}
	reset_adjustment();
	d_initial_geometries.clear();
	d_plate_id = rfg->reconstruction_plate_id();
	if (d_plate_id) {
		QLocale locale_;
		// We need this static-cast because apparently QLocale's 'toString' member function
		// doesn't have an overload for 'unsigned long', so the compiler complains about
		// ambiguity.
		field_moving_plate->setText(locale_.toString(static_cast<unsigned>(*d_plate_id)));
	} else {
		// Clear the plate ID field.
		field_moving_plate->clear();
	}
}


void
GPlatesQtWidgets::ModifyReconstructionPoleWidget::handle_reconstruction()
{
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
	if ( ! d_plate_id) {
		return;
	}
	
	std::vector<GPlatesModel::integer_plate_id_type> plate_id_collection;
	plate_id_collection.push_back(*d_plate_id);

	if (d_should_display_children)
	{
		add_children_to_geometry_collection(plate_id_collection,*d_plate_id, d_application_state_ptr);
	}

#if 0
	display_collection(plate_id_collection);	
#endif

	GPlatesModel::Reconstruction::geometry_collection_type::iterator iter =
			d_application_state_ptr->get_current_reconstruction().geometries().begin();
	GPlatesModel::Reconstruction::geometry_collection_type::iterator end =
			d_application_state_ptr->get_current_reconstruction().geometries().begin();
	for ( ; iter != end; ++iter) {
		GPlatesModel::ReconstructionGeometry *rg = iter->get();

		// We're only interested in ReconstructedFeatureGeometry's (ResolvedTopologicalBoundary's,
		// for instance, are used to assign plate ids to regular features so we probably only
		// want to look at geometries of regular features).
		GPlatesModel::ReconstructedFeatureGeometry *rfg = NULL;
		if (GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type(
				rg, rfg))
		{
			// It's an RFG, so let's look at its reconstruction plate ID property (if
			// there is one).
			if (rfg->reconstruction_plate_id()) {
				// OK, so the RFG *does* have a reconstruction plate ID.
				GPlatesModel::integer_plate_id_type rfg_plate_id = *(rfg->reconstruction_plate_id());
				if (std::find(plate_id_collection.begin(),plate_id_collection.end(),rfg_plate_id) != plate_id_collection.end()){
					d_initial_geometries.push_back(std::make_pair(rfg->geometry(),rfg->get_non_null_pointer_to_const()));
				}
			}
		}
	}

	if (d_initial_geometries.empty()) {
		// That's pretty strange.  We expected at least one geometry here, or else, what's
		// the user dragging?
		std::cerr << "No initial geometries found ModifyReconstructionPoleWidget::populate_initial_geometries!"
				<< std::endl;
	}
	
}


void
GPlatesQtWidgets::ModifyReconstructionPoleWidget::draw_initial_geometries()
{
	static const GPlatesModel::PropertyName vgp_prop_name =
		GPlatesModel::PropertyName::create_gpml("polePosition");

	d_initial_geometries.clear();
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

	const GPlatesGui::ColourProxy &white_colour = GPlatesGui::Colour::get_white();

	geometry_collection_type::const_iterator iter = d_initial_geometries.begin();
	geometry_collection_type::const_iterator end = d_initial_geometries.end();
	for ( ; iter != end; ++iter)
	{
		// Create rendered geometry.
		const GPlatesViewOperations::RenderedGeometry rendered_geometry =
			GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
					(*iter).first,
					white_colour,
					GPlatesViewOperations::RenderedLayerParameters::POLE_MANIPULATION_POINT_SIZE_HINT,
					GPlatesViewOperations::RenderedLayerParameters::POLE_MANIPULATION_LINE_WIDTH_HINT);

		// Add to pole manipulation layer.
		d_initial_geom_layer_ptr->add_rendered_geometry(rendered_geometry);



		///////////////////////////////////////////////////////////////////
		// FIXME: The following is part of the paleomag workarounds.
		if (iter->second->is_valid())
		{
		
			if ((*(*iter).second->property())->property_name() == vgp_prop_name)
			{

				boost::optional<GPlatesMaths::Rotation> optional_rotation;
				GPlatesAppLogic::PaleomagUtils::VgpRenderer vgp_renderer(
					d_application_state_ptr->get_current_reconstruction(),
					optional_rotation,
					d_initial_geom_layer_ptr,
					white_colour,
					d_view_state_ptr);

				vgp_renderer.visit_feature(
					iter->second->feature_handle_ptr()->reference());			
			}
		}
		// End of paleomag workaround.
		//////////////////////////////////////////////////////////////////////

	}
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

	static const GPlatesModel::PropertyName vgp_prop_name =
		GPlatesModel::PropertyName::create_gpml("polePosition");

	// Clear all dragged geometry RenderedGeometry's before adding new ones.
	d_dragged_geom_layer_ptr->clear_rendered_geometries();

	const GPlatesGui::ColourProxy &silver_colour = GPlatesGui::Colour::get_silver();

	geometry_collection_type::const_iterator iter = d_initial_geometries.begin();
	geometry_collection_type::const_iterator end = d_initial_geometries.end();
	for ( ; iter != end; ++iter)
	{
		// Create rendered geometry.
		const GPlatesViewOperations::RenderedGeometry rendered_geometry =
			GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
					d_accum_orientation->orient_geometry((*iter).first),
					silver_colour,
					GPlatesViewOperations::RenderedLayerParameters::POLE_MANIPULATION_POINT_SIZE_HINT,
					GPlatesViewOperations::RenderedLayerParameters::POLE_MANIPULATION_LINE_WIDTH_HINT);

		// Add to pole manipulation layer.
		d_dragged_geom_layer_ptr->add_rendered_geometry(rendered_geometry);
		
	
		///////////////////////////////////////////////////////////////////////////
		// FIXME: The following is part of the paleomag workarounds.
		if (iter->second->is_valid())
		{
			if ((*(*iter).second->property())->property_name() == vgp_prop_name)
			{

				boost::optional<GPlatesMaths::Rotation> optional_rotation;
				optional_rotation.reset(d_accum_orientation->rotation());
				GPlatesAppLogic::PaleomagUtils::VgpRenderer vgp_renderer(
					d_application_state_ptr->get_current_reconstruction(),
					optional_rotation,
					d_dragged_geom_layer_ptr,
					silver_colour,
					d_view_state_ptr);

				vgp_renderer.visit_feature(
					iter->second->feature_handle_ptr()->reference());
			}	
		}
		// End of paleomag workaround.	
		////////////////////////////////////////////////////////////////////////////		

	}
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

	// The user wants to reset the adjustment (to zero).
	QObject::connect(button_reset_adjustment, SIGNAL(clicked()),
		this, SLOT(reset()));

	// Communication between the Apply ... Adjustment dialog and the Adjustment Applicator.
	QObject::connect(d_dialog_ptr.get(), SIGNAL(pole_sequence_choice_changed(int)),
		d_applicator_ptr.get(), SLOT(handle_pole_sequence_choice_changed(int)));
	QObject::connect(d_dialog_ptr.get(), SIGNAL(pole_sequence_choice_cleared()),
		d_applicator_ptr.get(), SLOT(handle_pole_sequence_choice_cleared()));
	QObject::connect(d_dialog_ptr.get(), SIGNAL(accepted()),
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
				GPlatesViewOperations::RenderedGeometryCollection::POLE_MANIPULATION_LAYER);

	// Create a rendered layer to draw the initial geometries.
	// NOTE: this must be created second to get drawn on top.
	d_dragged_geom_layer_ptr =
		d_rendered_geom_collection->create_child_rendered_layer_and_transfer_ownership(
				GPlatesViewOperations::RenderedGeometryCollection::POLE_MANIPULATION_LAYER);

	// In both cases above we store the returned object as a data member and it
	// automatically destroys the created layer for us when 'this' object is destroyed.

	// Activate both layers.
	d_initial_geom_layer_ptr->set_active();
	d_dragged_geom_layer_ptr->set_active();
}
