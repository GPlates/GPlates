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
#include "feature-visitors/TotalReconstructionSequencePlateIdFinder.h"
#include "feature-visitors/TotalReconstructionSequenceRotationInserter.h"
#include "utils/MathUtils.h"


GPlatesQtWidgets::ReconstructionPoleWidget::ReconstructionPoleWidget(
		GPlatesQtWidgets::ViewportWindow &view_state,
		QWidget *parent_):
	QWidget(parent_),
	d_view_state_ptr(&view_state)
{
	setupUi(this);
	
	// Reset button to change the current adjustment to an identity rotation.
	QObject::connect(button_adjustment_reset, SIGNAL(clicked()),
			this, SLOT(handle_reset_adjustment()));
}


void
GPlatesQtWidgets::ReconstructionPoleWidget::start_new_drag(
		const GPlatesMaths::PointOnSphere &current_position)
{
	d_accum_orientation.reset(new GPlatesGui::SimpleGlobeOrientation());
	d_accum_orientation->set_new_handle_at_pos(current_position);
}


namespace
{
	const QString
	format_axis_as_lat_lon_point(
			const GPlatesMaths::UnitVector3D &axis)
	{
		using namespace GPlatesMaths;

		PointOnSphere pos(axis);
		LatLonPoint llp = make_lat_lon_point(pos);
		QLocale locale;
		QString str = QObject::tr("(%1 ; %2)")
				.arg(locale.toString(llp.latitude(), 'f', 1))
				.arg(locale.toString(llp.longitude(), 'f', 1));
		return str;
	}
}


void
GPlatesQtWidgets::ReconstructionPoleWidget::update_drag_position(
		const GPlatesMaths::PointOnSphere &current_position)
{
	d_accum_orientation->move_handle_to_pos(current_position);
	if ( ! d_initial_geometry) {
		// That's pretty strange.  We expected a geometry here, or else, what's the user
		// dragging?
		std::cerr << "No initial geometry in ReconstructionPoleWidget::update_drag_position!" << std::endl;
		return;
	}
	GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type non_null_geom_ptr =
			GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type(d_initial_geometry.get(),
					GPlatesUtils::NullIntrusivePointerHandler());

	GlobeCanvas &canvas = d_view_state_ptr->globe_canvas();
	GPlatesGui::RenderedGeometryLayers &layers = canvas.globe().rendered_geometry_layers();

	layers.mouse_movement_layer().clear();
	GPlatesGui::PlatesColourTable::const_iterator silver_colour = &GPlatesGui::Colour::SILVER;
	layers.mouse_movement_layer().push_back(
			GPlatesGui::RenderedGeometry(
					d_accum_orientation->orient_geometry(non_null_geom_ptr),
					silver_colour));
	canvas.update_canvas();

	// Update the "Adjustment" fields in the TaskPanel pane.
	double rot_angle_in_rads = d_accum_orientation->rotation_angle().dval();
	double rot_angle_in_degs = GPlatesUtils::convert_rad_to_deg(rot_angle_in_rads);
	spinbox_adjustment_angle->setValue(rot_angle_in_degs);

	const GPlatesMaths::UnitVector3D &rot_axis = d_accum_orientation->rotation_axis();
	lineedit_adjustment_pole->setText(format_axis_as_lat_lon_point(rot_axis));
}


namespace
{
	/**
	 * This finds all the TRSes (total reconstruction sequences) in the supplied reconstruction
	 * whose fixed or moving ref-frame plate ID matches our plate ID of interest.
	 *
	 * The two vectors @a trses_with_plate_id_as_fixed and @a trses_with_plate_id_as_moving
	 * will be populated with the matches.
	 */
	void
	find_trses(
			std::vector<GPlatesModel::FeatureHandle::weak_ref> &trses_with_plate_id_as_fixed,
			std::vector<GPlatesModel::FeatureHandle::weak_ref> &trses_with_plate_id_as_moving,
			GPlatesFeatureVisitors::TotalReconstructionSequencePlateIdFinder &trs_plate_id_finder,
			GPlatesModel::integer_plate_id_type plate_id_of_interest,
			const GPlatesModel::Reconstruction &reconstruction)
	{
		using namespace GPlatesModel;

		std::vector<FeatureCollectionHandle::weak_ref>::const_iterator collections_iter =
				reconstruction.reconstruction_feature_collections().begin();
		std::vector<FeatureCollectionHandle::weak_ref>::const_iterator collections_end =
				reconstruction.reconstruction_feature_collections().end();
		for ( ; collections_iter != collections_end; ++collections_iter) {
			const FeatureCollectionHandle::weak_ref &current_collection = *collections_iter;
			if ( ! current_collection.is_valid()) {
				// FIXME:  Should we do anything about this?  Or is this acceptable?
				// (If the collection is not valid, then presumably it has been unloaded.
				// In which case, why hasn't the reconstruction been recalculated?)
				continue;
			}

			FeatureCollectionHandle::features_iterator features_iter =
					current_collection->features_begin();
			FeatureCollectionHandle::features_iterator features_end =
					current_collection->features_end();
			for ( ; features_iter != features_end; ++features_iter) {
				boost::intrusive_ptr<FeatureHandle> current_feature = *features_iter;
				if ( ! current_feature) {
					// There was a feature here, but it's been deleted.
					continue;
				}
				trs_plate_id_finder.visit_feature_handle(*current_feature);

				if ( ! (trs_plate_id_finder.fixed_ref_frame_plate_id() &&
						trs_plate_id_finder.moving_ref_frame_plate_id())) {
					// This feature was missing one (or both) of the required
					// plate IDs.  Skip it.
					continue;
				}
				// Else, we know it found both of the required plate IDs.

				if (*trs_plate_id_finder.fixed_ref_frame_plate_id() ==
						*trs_plate_id_finder.moving_ref_frame_plate_id()) {
					// The fixed ref-frame plate ID equals the moving ref-frame
					// plate ID?  Something strange is going on here.  Skip it.
					continue;
				}

				if (*trs_plate_id_finder.fixed_ref_frame_plate_id() == plate_id_of_interest) {
					trses_with_plate_id_as_fixed.push_back(current_feature->reference());
				}
				if (*trs_plate_id_finder.moving_ref_frame_plate_id() == plate_id_of_interest) {
					trses_with_plate_id_as_moving.push_back(current_feature->reference());
				}
			}
		}
	}


	void
	update_trses_with_rotation_adjustment(
			std::vector<GPlatesModel::FeatureHandle::weak_ref> &trses_to_be_updated,
			const GPlatesMaths::Rotation &rotation_adjustment,
			const double &reconstruction_time)
	{
		GPlatesFeatureVisitors::TotalReconstructionSequenceRotationInserter inserter(
				reconstruction_time, rotation_adjustment);

		std::vector<GPlatesModel::FeatureHandle::weak_ref>::iterator iter = trses_to_be_updated.begin();
		std::vector<GPlatesModel::FeatureHandle::weak_ref>::iterator end = trses_to_be_updated.end();
		for ( ; iter != end; ++iter) {
			GPlatesModel::FeatureHandle::weak_ref current_feature = *iter;
			if ( ! current_feature.is_valid()) {
				// Nothing we can do with this feature.
				continue;
			}

			inserter.visit_feature_handle(*current_feature);
		}
	}
}


void
GPlatesQtWidgets::ReconstructionPoleWidget::finalise()
{
	GlobeCanvas &canvas = d_view_state_ptr->globe_canvas();
	GPlatesGui::RenderedGeometryLayers &layers = canvas.globe().rendered_geometry_layers();
	layers.mouse_movement_layer().clear();

	if ( ! d_plate_id) {
		// Presumably the feature did not contain a reconstruction plate ID.
		// What do we do here?  Do we give it one, or do nothing?
		// For now, let's just do nothing.
		return;
	}
	// Now find all the TRSes (total reconstruction sequences) whose fixed or moving ref-frame
	// plate ID matches our plate ID of interest.
	std::vector<GPlatesModel::FeatureHandle::weak_ref> trses_with_plate_id_as_fixed;
	std::vector<GPlatesModel::FeatureHandle::weak_ref> trses_with_plate_id_as_moving;
	GPlatesFeatureVisitors::TotalReconstructionSequencePlateIdFinder trs_plate_id_finder;
	find_trses(trses_with_plate_id_as_fixed, trses_with_plate_id_as_moving,
			trs_plate_id_finder, *d_plate_id, d_view_state_ptr->reconstruction());

	update_trses_with_rotation_adjustment(trses_with_plate_id_as_fixed,
			d_accum_orientation->rotation().get_reverse(),
			d_view_state_ptr->reconstruction_time());
	update_trses_with_rotation_adjustment(trses_with_plate_id_as_moving,
			d_accum_orientation->rotation(),
			d_view_state_ptr->reconstruction_time());

	d_view_state_ptr->reconstruct();
}


void
GPlatesQtWidgets::ReconstructionPoleWidget::handle_reset_adjustment()
{  }


void
GPlatesQtWidgets::ReconstructionPoleWidget::set_focus(
		GPlatesModel::FeatureHandle::weak_ref feature_ref,
		GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type focused_geometry)
{
	if ( ! focused_geometry) {
		// No RFG, so nothing we can do.
		return;
	}
	d_initial_geometry = focused_geometry->geometry().get();
	d_plate_id = focused_geometry->reconstruction_plate_id();
}
