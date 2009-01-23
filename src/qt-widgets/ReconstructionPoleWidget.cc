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

#include <iostream>

#include "ReconstructionPoleWidget.h"
#include "ViewportWindow.h"
#include "ApplyReconstructionPoleAdjustmentDialog.h"
#include "feature-visitors/TotalReconstructionSequencePlateIdFinder.h"
#include "feature-visitors/TotalReconstructionSequenceTimePeriodFinder.h"
#include "utils/MathUtils.h"
#include "view-operations/RenderedGeometryParameters.h"


GPlatesQtWidgets::ReconstructionPoleWidget::ReconstructionPoleWidget(
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
		GPlatesViewOperations::RenderedGeometryFactory &rendered_geom_factory,
		ViewportWindow &view_state,
		QWidget *parent_):
	QWidget(parent_),
	d_rendered_geom_collection(&rendered_geom_collection),
	d_rendered_geom_factory(&rendered_geom_factory),
	d_view_state_ptr(&view_state),
	d_dialog_ptr(new ApplyReconstructionPoleAdjustmentDialog(&view_state)),
	d_applicator_ptr(new AdjustmentApplicator(view_state, *d_dialog_ptr))
{
	setupUi(this);

	make_signal_slot_connections();

	create_child_rendered_layers();
}


void
GPlatesQtWidgets::ReconstructionPoleWidget::activate()
{
	d_is_active = true;
	draw_initial_geometries_at_activation();
}


void
GPlatesQtWidgets::ReconstructionPoleWidget::deactivate()
{
	d_is_active = false;
}


void
GPlatesQtWidgets::ReconstructionPoleWidget::draw_initial_geometries_at_activation()
{
	d_initial_geometries.clear();
	populate_initial_geometries();
	draw_dragged_geometries();
}


void
GPlatesQtWidgets::ReconstructionPoleWidget::start_new_drag(
		const GPlatesMaths::PointOnSphere &current_oriented_position)
{
	if ( ! d_accum_orientation) {
		d_accum_orientation.reset(new GPlatesGui::SimpleGlobeOrientation());
	}
	d_accum_orientation->set_new_handle_at_pos(current_oriented_position);
}


namespace
{
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
			// about the history, we just care about the maths).
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
GPlatesQtWidgets::ReconstructionPoleWidget::start_new_rotation_drag(
		const GPlatesMaths::PointOnSphere &current_oriented_position,
		const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport)
{
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
GPlatesQtWidgets::ReconstructionPoleWidget::update_drag_position(
		const GPlatesMaths::PointOnSphere &current_oriented_position)
{
	d_accum_orientation->move_handle_to_pos(current_oriented_position);

	draw_dragged_geometries();
	update_adjustment_fields();
}


void
GPlatesQtWidgets::ReconstructionPoleWidget::update_rotation_drag_position(
		const GPlatesMaths::PointOnSphere &current_oriented_position,
		const GPlatesMaths::PointOnSphere &oriented_centre_of_viewport)
{
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
GPlatesQtWidgets::ReconstructionPoleWidget::end_drag()
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
			boost::intrusive_ptr<GPlatesModel::FeatureHandle> current_feature)
	{
		using namespace GPlatesQtWidgets;

		if ( ! current_feature) {
			// There was a feature here, but it's been deleted.
			return;
		}

		trs_plate_id_finder.reset();
		trs_plate_id_finder.visit_feature_handle(*current_feature);

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
			trs_time_period_finder.visit_feature_handle(*current_feature);
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
			trs_time_period_finder.visit_feature_handle(*current_feature);
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

		std::vector<FeatureCollectionHandle::weak_ref>::const_iterator collections_iter =
				reconstruction.reconstruction_feature_collections().begin();
		std::vector<FeatureCollectionHandle::weak_ref>::const_iterator collections_end =
				reconstruction.reconstruction_feature_collections().end();
		for ( ; collections_iter != collections_end; ++collections_iter) {
			const FeatureCollectionHandle::weak_ref &current_collection = *collections_iter;
			if ( ! current_collection.is_valid()) {
				// FIXME:  Should we do anything about this? Or is this acceptable?
				// (If the collection is not valid, then presumably it has been
				// unloaded.  In which case, why hasn't the reconstruction been
				// recalculated?)
				continue;
			}

			FeatureCollectionHandle::features_iterator features_iter =
					current_collection->features_begin();
			FeatureCollectionHandle::features_iterator features_end =
					current_collection->features_end();
			for ( ; features_iter != features_end; ++features_iter) {
				boost::intrusive_ptr<FeatureHandle> current_feature = *features_iter;
				examine_trs(sequence_choices, trs_plate_id_finder,
						trs_time_period_finder, plate_id_of_interest,
						reconstruction_time, current_feature);
			}
		}
	}
}


void
GPlatesQtWidgets::ReconstructionPoleWidget::apply()
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
			d_view_state_ptr->reconstruction(), d_view_state_ptr->reconstruction_time());

	// The Applicator should be set before the dialog is set up.
	// Why, you ask?  Because when the dialog is set up, the first row in the sequence choices
	// table will be selected, which will send a signal which will trigger a slot in the
	// Applicator, which will not do anything useful unless the Applicator has been set.
	d_applicator_ptr->set(
			sequence_choices,
			d_accum_orientation->rotation(),
			d_view_state_ptr->reconstruction_time());
	d_dialog_ptr->setup_for_new_pole(
			*d_plate_id,
			d_view_state_ptr->reconstruction_time(),
			sequence_choices,
			d_accum_orientation->rotation());

	d_dialog_ptr->show();
}


void
GPlatesQtWidgets::ReconstructionPoleWidget::reset()
{
	reset_adjustment();
	draw_initial_geometries();
}


void
GPlatesQtWidgets::ReconstructionPoleWidget::reset_adjustment()
{
	d_accum_orientation.reset();

	// Update the "Adjustment" fields in the TaskPanel pane.
	field_adjustment_lat->clear();
	field_adjustment_lon->clear();
	spinbox_adjustment_angle->setValue(0.0);
}


void
GPlatesQtWidgets::ReconstructionPoleWidget::set_focus(
		GPlatesModel::FeatureHandle::weak_ref feature_ref,
		GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type focused_geometry)
{
	if ( ! focused_geometry) {
		// No RFG, so nothing we can do.

		// Clear the plate ID and the plate ID field.
		d_plate_id = boost::none;
		reset_adjustment();
		d_initial_geometries.clear();
		field_moving_plate->clear();
		return;
	}
	if (d_plate_id == focused_geometry->reconstruction_plate_id()) {
		// The plate ID hasn't changed, so there's nothing to do.
		return;
	}
	reset_adjustment();
	d_initial_geometries.clear();
	d_plate_id = focused_geometry->reconstruction_plate_id();
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
GPlatesQtWidgets::ReconstructionPoleWidget::handle_reconstruction_time_change(
		double new_time)
{
	if (d_is_active) {
		d_initial_geometries.clear();
		populate_initial_geometries();
		draw_dragged_geometries();
	}
}


void
GPlatesQtWidgets::ReconstructionPoleWidget::populate_initial_geometries()
{
	// If there's no plate ID of the currently-focused RFG, then there can be no other RFGs
	// with the same plate ID.
	if ( ! d_plate_id) {
		return;
	}

	GPlatesModel::Reconstruction::geometry_collection_type::iterator iter =
			d_view_state_ptr->reconstruction().geometries().begin();
	GPlatesModel::Reconstruction::geometry_collection_type::iterator end =
			d_view_state_ptr->reconstruction().geometries().end();
	for ( ; iter != end; ++iter) {
		GPlatesModel::ReconstructionGeometry *rg = iter->get();

		// We use a dynamic cast here (despite the fact that dynamic casts are generally
		// considered bad form) because we only care about one specific derivation.
		// There's no "if ... else if ..." chain, so I think it's not super-bad form.  (The
		// "if ... else if ..." chain would imply that we should be using polymorphism --
		// specifically, the double-dispatch of the Visitor pattern -- rather than updating
		// the "if ... else if ..." chain each time a new derivation is added.)
		GPlatesModel::ReconstructedFeatureGeometry *rfg =
				dynamic_cast<GPlatesModel::ReconstructedFeatureGeometry *>(rg);
		if (rfg) { 
			// It's an RFG, so let's look at its reconstruction plate ID property (if
			// there is one).
			if (rfg->reconstruction_plate_id()) {
				// OK, so the RFG *does* have a reconstruction plate ID.
				if (*(rfg->reconstruction_plate_id()) == *d_plate_id) {
					d_initial_geometries.push_back(rfg->geometry());
				}
			}
		}
	}

	if (d_initial_geometries.empty()) {
		// That's pretty strange.  We expected at least one geometry here, or else, what's
		// the user dragging?
		std::cerr << "No initial geometries found ReconstructionPoleWidget::populate_initial_geometries!"
				<< std::endl;
	}
}


void
GPlatesQtWidgets::ReconstructionPoleWidget::draw_initial_geometries()
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

	const GPlatesGui::Colour &white_colour = GPlatesGui::Colour::WHITE;

	geometry_collection_type::const_iterator iter = d_initial_geometries.begin();
	geometry_collection_type::const_iterator end = d_initial_geometries.end();
	for ( ; iter != end; ++iter)
	{
		// Create rendered geometry.
		const GPlatesViewOperations::RenderedGeometry rendered_geometry =
			d_rendered_geom_factory->create_rendered_geometry_on_sphere(
					*iter,
					white_colour,
					GPlatesViewOperations::RenderedLayerParameters::POLE_MANIPULATION_POINT_SIZE_HINT,
					GPlatesViewOperations::RenderedLayerParameters::POLE_MANIPULATION_LINE_WIDTH_HINT);

		// Add to pole manipulation layer.
		d_initial_geom_layer_ptr->add_rendered_geometry(rendered_geometry);
	}
}


void
GPlatesQtWidgets::ReconstructionPoleWidget::draw_dragged_geometries()
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

	const GPlatesGui::Colour &silver_colour = GPlatesGui::Colour::SILVER;

	geometry_collection_type::const_iterator iter = d_initial_geometries.begin();
	geometry_collection_type::const_iterator end = d_initial_geometries.end();
	for ( ; iter != end; ++iter)
	{
		// Create rendered geometry.
		const GPlatesViewOperations::RenderedGeometry rendered_geometry =
			d_rendered_geom_factory->create_rendered_geometry_on_sphere(
					d_accum_orientation->orient_geometry(*iter),
					silver_colour,
					GPlatesViewOperations::RenderedLayerParameters::POLE_MANIPULATION_POINT_SIZE_HINT,
					GPlatesViewOperations::RenderedLayerParameters::POLE_MANIPULATION_LINE_WIDTH_HINT);

		// Add to pole manipulation layer.
		d_dragged_geom_layer_ptr->add_rendered_geometry(rendered_geometry);
	}
}


void
GPlatesQtWidgets::ReconstructionPoleWidget::update_adjustment_fields()
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
GPlatesQtWidgets::ReconstructionPoleWidget::clear_and_reset_after_reconstruction()
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
GPlatesQtWidgets::ReconstructionPoleWidget::make_signal_slot_connections()
{
	// The user wants to apply the current adjustment.
	QObject::connect(button_apply, SIGNAL(clicked()),
		this, SLOT(apply()));

	// The user wants to reset the adjustment (to zero).
	QObject::connect(button_reset_adjustment, SIGNAL(clicked()),
		this, SLOT(reset()));

	// Communication between the Apply ... Adjustment dialog and the Adjustment Applicator.
	QObject::connect(d_dialog_ptr, SIGNAL(pole_sequence_choice_changed(int)),
		d_applicator_ptr, SLOT(handle_pole_sequence_choice_changed(int)));
	QObject::connect(d_dialog_ptr, SIGNAL(pole_sequence_choice_cleared()),
		d_applicator_ptr, SLOT(handle_pole_sequence_choice_cleared()));
	QObject::connect(d_dialog_ptr, SIGNAL(accepted()),
		d_applicator_ptr, SLOT(apply_adjustment()));

	// The user has agreed to apply the adjustment as described in the dialog.
	QObject::connect(d_applicator_ptr, SIGNAL(have_reconstructed()),
		this, SLOT(clear_and_reset_after_reconstruction()));
}

void
GPlatesQtWidgets::ReconstructionPoleWidget::create_child_rendered_layers()
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
