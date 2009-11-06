/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007, 2008 The University of Sydney, Australia
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

#include <queue>
#include <QLocale>

#include "ClickGeometry.h"

#include "global/InternalInconsistencyException.h"
#include "gui/ProximityTests.h"
#include "maths/ProximityCriteria.h"
#include "maths/PointOnSphere.h"
#include "model/FeatureHandle.h"
#include "model/ReconstructedFeatureGeometry.h"
#include "qt-widgets/GlobeCanvas.h"
#include "qt-widgets/ViewportWindow.h"
#include "qt-widgets/FeaturePropertiesDialog.h"
#include "view-operations/RenderedGeometryCollection.h"
#include "view-operations/RenderedGeometryProximity.h"
#include "view-operations/RenderedGeometryVisitor.h"
#include "view-operations/RenderedReconstructionGeometry.h"

namespace
{
	/**
	 * A rendered geometry visitor that adds reconstruction geometries to a sequence.
	 */
	class AddReconstructionGeometriesToFeatureTable :
			public GPlatesViewOperations::ConstRenderedGeometryVisitor
	{
	public:
		AddReconstructionGeometriesToFeatureTable(
				GPlatesGui::FeatureTableModel::geometry_sequence_type &recon_geom_seq) :
			d_recon_geom_seq(recon_geom_seq)
		{  }

		virtual
		void
		visit_rendered_reconstruction_geometry(
				const GPlatesViewOperations::RenderedReconstructionGeometry &rendered_recon_geom)
		{
			d_recon_geom_seq.push_back(rendered_recon_geom.get_reconstruction_geometry());
		}

	private:
		GPlatesGui::FeatureTableModel::geometry_sequence_type &d_recon_geom_seq;
	};

	/**
	 * Adds @a ReconstructionGeometry objects in the sorted proximity hits to the feature table model.
	 *
	 * Returns false if no hits to begin with or if none of the hits are @a ReconstructionGeometry objects
	 * - the latter shouldn't really happen but could if other rendered geometry types are put
	 * in the RECONSTRUCTION layer.	 
	 */
	bool
	add_clicked_reconstruction_geoms_to_feature_table_model(
			GPlatesGui::FeatureTableModel *clicked_table_model,
			const GPlatesViewOperations::sorted_rendered_geometry_proximity_hits_type &sorted_hits)
	{
		if (sorted_hits.empty())
		{
			return false;
		}

		// The sequence of ReconstructionGeometry's were going to add to.
		GPlatesGui::FeatureTableModel::geometry_sequence_type new_recon_geom_seq;

		// Used to add reconstruction geometries to clicked table model.
		AddReconstructionGeometriesToFeatureTable add_recon_geoms(new_recon_geom_seq);

		GPlatesViewOperations::sorted_rendered_geometry_proximity_hits_type::const_iterator sorted_iter;
		for (sorted_iter = sorted_hits.begin(); sorted_iter != sorted_hits.end(); ++sorted_iter)
		{
			GPlatesViewOperations::RenderedGeometry rendered_geom =
				sorted_iter->d_rendered_geom_layer->get_rendered_geometry(
				sorted_iter->d_rendered_geom_index);

			// If rendered geometry contains a reconstruction geometry then add that
			// to the clicked table model.
			rendered_geom.accept_visitor(add_recon_geoms);
		}

		if (new_recon_geom_seq.empty())
		{
			// None of the hits were ReconstructionGeometry objects.
			return false;
		}

		clicked_table_model->begin_insert_features(0, static_cast<int>(new_recon_geom_seq.size()) - 1);

		// The sequence of ReconstructionGeometry's were going to add to.
		GPlatesGui::FeatureTableModel::geometry_sequence_type &recon_geom_seq =
			clicked_table_model->geometry_sequence();

		// Add to the beginning of the current sequence.
		recon_geom_seq.insert(recon_geom_seq.begin(), new_recon_geom_seq.begin(), new_recon_geom_seq.end()); 

		clicked_table_model->end_insert_features();

		return true;
	}
}

GPlatesCanvasTools::ClickGeometry::ClickGeometry(
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
		const GPlatesQtWidgets::ViewportWindow &view_state_,
		GPlatesGui::FeatureTableModel &clicked_table_model_,
		GPlatesQtWidgets::FeaturePropertiesDialog &fp_dialog_,
		GPlatesGui::FeatureFocus &feature_focus_):
	d_rendered_geom_collection(&rendered_geom_collection),
	d_view_state_ptr(&view_state_),
	d_clicked_table_model_ptr(&clicked_table_model_),
	d_fp_dialog_ptr(&fp_dialog_),
	d_feature_focus_ptr(&feature_focus_)
{
}

void
GPlatesCanvasTools::ClickGeometry::handle_activation()
{
	set_status_bar_message(QObject::tr(
		"Click a geometry to choose a feature."
		" Shift+click to query immediately."
		" Ctrl+drag to re-orient the globe."));

	// Activate the geometry focus hightlight layer.
	d_rendered_geom_collection->set_main_layer_active(
		GPlatesViewOperations::RenderedGeometryCollection::GEOMETRY_FOCUS_HIGHLIGHT_LAYER);
}

void
GPlatesCanvasTools::ClickGeometry::handle_left_click(
		const GPlatesMaths::PointOnSphere &point_on_sphere,
		bool is_on_earth,
		double proximity_inclusion_threshold)
{
	handle_left_click(point_on_sphere,
				proximity_inclusion_threshold,
				*d_view_state_ptr,
				*d_clicked_table_model_ptr,
				*d_feature_focus_ptr,
				*d_rendered_geom_collection);
}

void
GPlatesCanvasTools::ClickGeometry::handle_left_click(
		const GPlatesMaths::PointOnSphere &point_on_sphere,
		double proximity_inclusion_threshold,
		const GPlatesQtWidgets::ViewportWindow &view_state,
		GPlatesGui::FeatureTableModel &clicked_table_model,
		GPlatesGui::FeatureFocus &feature_focus,
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection)
{

	
	// What did the user click on just now?
	GPlatesViewOperations::sorted_rendered_geometry_proximity_hits_type sorted_hits;

	// Test for proximity to the RenderedGeometry objects in the reconstruction layer.
	// These RenderedGeometry objects each contain a ReconstructionGeometry.
	// If the reconstruction main layer is inactive or parts of it are inactive (ie, child
	// layers) then they don't get tested.
	// Only what's visible gets tested which is what we want.
	GPlatesMaths::ProximityCriteria criteria(point_on_sphere, proximity_inclusion_threshold);
	GPlatesViewOperations::test_proximity(
			sorted_hits,
			rendered_geometry_collection,
			criteria);
	
	// Give the user some useful feedback in the status bar.
	if (sorted_hits.size() == 0) {
		set_status_bar_message(QObject::tr("Clicked %1 geometries.").arg(sorted_hits.size()));
	} else if (sorted_hits.size() == 1) {
		set_status_bar_message(QObject::tr("Clicked %1 geometry.").arg(sorted_hits.size()));
	} else {
		set_status_bar_message(QObject::tr("Clicked %1 geometries.").arg(sorted_hits.size()));
	}

	// Clear the 'Clicked' FeatureTableModel, ready to be populated (or not).
	clicked_table_model.clear();

	// Populate the 'Clicked' FeatureTableModel.
	if (!add_clicked_reconstruction_geoms_to_feature_table_model(&clicked_table_model, sorted_hits))
	{
		// User clicked on empty space (or a rendered geometry that isn't a feature in the reconstruction layer)!
		// Clear the currently focused feature.
		feature_focus.unset_focus();

		return;
	}

	view_state.highlight_first_clicked_feature_table_row();

}

void
GPlatesCanvasTools::ClickGeometry::handle_shift_left_click(
		const GPlatesMaths::PointOnSphere &point_on_sphere,
		bool is_on_earth,
		double proximity_inclusion_threshold)
{
	handle_left_click(
			point_on_sphere,
			is_on_earth,
			proximity_inclusion_threshold);

	// If there is a feature focused, we'll assume that the user wants to look at it in detail.
	if (d_feature_focus_ptr->is_valid()) {
		fp_dialog().choose_query_widget_and_open();
	}
}

