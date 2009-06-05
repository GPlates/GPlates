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

#include "CreateTopologyWidget.h"
#include "ViewportWindow.h"
#include "ApplyReconstructionPoleAdjustmentDialog.h"
#include "feature-visitors/TotalReconstructionSequencePlateIdFinder.h"
#include "feature-visitors/TotalReconstructionSequenceTimePeriodFinder.h"
#include "utils/MathUtils.h"
#include "view-operations/RenderedGeometryParameters.h"


GPlatesQtWidgets::CreateTopologyWidget::CreateTopologyWidget(
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
		GPlatesViewOperations::RenderedGeometryFactory &rendered_geom_factory,
		ViewportWindow &view_state,
		QWidget *parent_):
	QWidget(parent_),
	d_rendered_geom_collection(&rendered_geom_collection),
	d_rendered_geom_factory(&rendered_geom_factory),
	d_view_state_ptr(&view_state)
	// d_dialog_ptr(new ApplyReconstructionPoleAdjustmentDialog(&view_state)),
	// d_applicator_ptr(new AdjustmentApplicator(view_state, *d_dialog_ptr))
{
	setupUi(this);

	make_signal_slot_connections();

	create_child_rendered_layers();
}


void
GPlatesQtWidgets::CreateTopologyWidget::activate()
{
	d_is_active = true;
	draw_initial_geometries_at_activation();
}


void
GPlatesQtWidgets::CreateTopologyWidget::deactivate()
{
	d_is_active = false;
}


void
GPlatesQtWidgets::CreateTopologyWidget::draw_initial_geometries_at_activation()
{
	d_initial_geometries.clear();
	populate_initial_geometries();
	draw_dragged_geometries();
}





void
GPlatesQtWidgets::CreateTopologyWidget::apply()
{
}


void
GPlatesQtWidgets::CreateTopologyWidget::reset()
{
	reset_adjustment();
	draw_initial_geometries();
}


void
GPlatesQtWidgets::CreateTopologyWidget::reset_adjustment()
{
	// Update the fields in the TaskPanel pane.
}


void
GPlatesQtWidgets::CreateTopologyWidget::set_focus(
		GPlatesModel::FeatureHandle::weak_ref feature_ref,
		GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type focused_geometry)
{
	if ( ! focused_geometry) {
		// No RFG, so nothing we can do.

#if 0
		// Clear the plate ID and the plate ID field.
		d_plate_id = boost::none;
		reset_adjustment();
		d_initial_geometries.clear();
		field_moving_plate->clear();
#endif
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
#if 0
		QLocale locale_;
		// We need this static-cast because apparently QLocale's 'toString' member function
		// doesn't have an overload for 'unsigned long', so the compiler complains about
		// ambiguity.
		field_moving_plate->setText(locale_.toString(static_cast<unsigned>(*d_plate_id)));
#endif
	} else {
#if 0
		// Clear the plate ID field.
		field_moving_plate->clear();
#endif
	}
}


void
GPlatesQtWidgets::CreateTopologyWidget::handle_reconstruction_time_change(
		double new_time)
{
	if (d_is_active) {
		d_initial_geometries.clear();
		populate_initial_geometries();
		draw_dragged_geometries();
	}
}


void
GPlatesQtWidgets::CreateTopologyWidget::populate_initial_geometries()
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
		std::cerr << "No initial geometries found CreateTopologyWidget::populate_initial_geometries!"
				<< std::endl;
	}
}


void
GPlatesQtWidgets::CreateTopologyWidget::draw_initial_geometries()
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

	const GPlatesGui::Colour &white_colour = GPlatesGui::Colour::get_white();

	geometry_collection_type::const_iterator iter = d_initial_geometries.begin();
	geometry_collection_type::const_iterator end = d_initial_geometries.end();
	for ( ; iter != end; ++iter)
	{
		// Create rendered geometry.
		const GPlatesViewOperations::RenderedGeometry rendered_geometry =
			d_rendered_geom_factory->create_rendered_geometry_on_sphere(
					*iter, 
					white_colour,
					GPlatesViewOperations::RenderedLayerParameters::DIGITISATION_POINT_SIZE_HINT,
					GPlatesViewOperations::RenderedLayerParameters::DIGITISATION_LINE_WIDTH_HINT);

		// Add to pole manipulation layer.
		d_initial_geom_layer_ptr->add_rendered_geometry(rendered_geometry);
	}
}


void
GPlatesQtWidgets::CreateTopologyWidget::draw_dragged_geometries()
{
#if 0
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

	geometry_collection_type::const_iterator iter = d_initial_geometries.begin();
	geometry_collection_type::const_iterator end = d_initial_geometries.end();
	for ( ; iter != end; ++iter)
	{
		// Create rendered geometry.
		const GPlatesViewOperations::RenderedGeometry rendered_geometry =
			d_rendered_geom_factory->create_rendered_geometry_on_sphere(
					d_accum_orientation->orient_geometry(*iter),
					silver_colour);

		// Add to pole manipulation layer.
		d_dragged_geom_layer_ptr->add_rendered_geometry(rendered_geometry);
	}
#endif
}


void
GPlatesQtWidgets::CreateTopologyWidget::update_adjustment_fields()
{
#if 0
	if ( ! d_accum_orientation) {
		// No idea why the boost::optional is boost::none here, but let's not crash!
		// FIXME:  Complain about this.
		return;
	}
	ApplyReconstructionPoleAdjustmentDialog::fill_in_fields_for_rotation(
			field_adjustment_lat, field_adjustment_lon, spinbox_adjustment_angle,
			d_accum_orientation->rotation());
#endif
}


void
GPlatesQtWidgets::CreateTopologyWidget::clear_and_reset_after_reconstruction()
{
#if 0
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
#endif 
}

void
GPlatesQtWidgets::CreateTopologyWidget::make_signal_slot_connections()
{
#if 0
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
#endif
}

void
GPlatesQtWidgets::CreateTopologyWidget::create_child_rendered_layers()
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
				GPlatesViewOperations::RenderedGeometryCollection::CREATE_TOPOLOGY_LAYER);

	// Create a rendered layer to draw the initial geometries.
	// NOTE: this must be created second to get drawn on top.
	d_dragged_geom_layer_ptr =
		d_rendered_geom_collection->create_child_rendered_layer_and_transfer_ownership(
				GPlatesViewOperations::RenderedGeometryCollection::CREATE_TOPOLOGY_LAYER);

	// In both cases above we store the returned object as a data member and it
	// automatically destroys the created layer for us when 'this' object is destroyed.

	// Activate both layers.
	d_initial_geom_layer_ptr->set_active();
	d_dragged_geom_layer_ptr->set_active();
}
