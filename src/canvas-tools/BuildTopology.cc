/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date: 2009-02-06 15:36:27 -0800 (Fri, 06 Feb 2009) $
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

#include <queue>
#include <QLocale>
#include <QDebug>
#include <QString>

#include "BuildTopology.h"

#include "gui/ProximityTests.h"
#include "qt-widgets/GlobeCanvas.h"
#include "qt-widgets/ViewportWindow.h"
#include "qt-widgets/BuildTopologyWidget.h"
#include "maths/LatLonPointConversions.h"
#include "model/FeatureHandle.h"
#include "model/ReconstructedFeatureGeometry.h"
#include "global/InternalInconsistencyException.h"
#include "utils/UnicodeStringUtils.h"
#include "utils/GeometryCreationUtils.h"
#include "property-values/XsString.h"
#include "feature-visitors/PropertyValueFinder.h"

GPlatesCanvasTools::BuildTopology::BuildTopology(
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
				GPlatesGui::Globe &globe_,
				GPlatesQtWidgets::GlobeCanvas &globe_canvas_,
				const GPlatesQtWidgets::ViewportWindow &view_state_,
				GPlatesGui::FeatureTableModel &clicked_table_model_,	
				GPlatesGui::FeatureTableModel &segments_table_model_,	
				GPlatesQtWidgets::BuildTopologyWidget &build_topology_widget,
				GPlatesQtWidgets::BuildTopologyWidget::GeometryType geom_type,
				GPlatesGui::FeatureFocus &feature_focus):
	CanvasTool(globe_, globe_canvas_),
	d_rendered_geom_collection(&rendered_geom_collection),
	d_view_state_ptr(&view_state_),
	d_clicked_table_model_ptr(&clicked_table_model_),
	d_segments_table_model_ptr(&segments_table_model_),
	d_build_topology_widget_ptr(&build_topology_widget),
	d_default_geom_type(geom_type),
	d_feature_focus_ptr(&feature_focus)
{
}


	

void
GPlatesCanvasTools::BuildTopology::handle_activation()
{
	// FIXME:  We may have to adjust the message if we are using a Map View.
	if (d_build_topology_widget_ptr->geometry_type() ==
			GPlatesQtWidgets::BuildTopologyWidget::PLATEPOLYGON) {
		d_view_state_ptr->status_message(QObject::tr(
				"Click on features to choose segments for the boundary."
				" Ctrl+drag to re-orient the globe."));
	} else {
		d_view_state_ptr->status_message(QObject::tr(
				"Click to draw a new vertex."
				" Ctrl+drag to reorient the globe."));
	}

	// Activate rendered layer.
	d_rendered_geom_collection->set_main_layer_active(
		GPlatesViewOperations::RenderedGeometryCollection::TOPOLOGY_TOOL_LAYER);



	d_build_topology_widget_ptr->activate();
	

#if 0
// FIXME: something is wrong with connect's arguments below, but I can't see what?
// This connection was moved to ViewportWindow, and uses the canvas_tool_adapter 
// to come back to this tool ... it seems so round about, but it works.

 	// If the user creates a new feature with the BuildTopologyWidget, 
	// then we need to create, and append property values to the feature
 	QObject::connect( 
		&( d_build_topology_widget_ptr->create_feature_dialog() ),
 		SIGNAL(feature_created(GPlatesModel::FeatureHandle::weak_ref)),
 		this,
 		SLOT( append_property_values_to_feature( GPlatesModel::FeatureHandle::weak_ref ) )
		);
#endif
}


void
GPlatesCanvasTools::BuildTopology::handle_deactivation()
{
	d_build_topology_widget_ptr->deactivate();
}


void
GPlatesCanvasTools::BuildTopology::handle_left_click(
		const GPlatesMaths::PointOnSphere &click_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_click_pos_on_globe,
		bool is_on_globe)
{
	const GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(
		oriented_click_pos_on_globe);

	// send the click point to the widget
	d_build_topology_widget_ptr->set_click_point( llp.latitude(), llp.longitude() );

	//
	// From ClickGeometry
	//
	double proximity_inclusion_threshold =
			globe_canvas().current_proximity_inclusion_threshold(click_pos_on_globe);
	
	// What did the user click on just now?
	std::priority_queue<GPlatesGui::ProximityTests::ProximityHit> sorted_hits;

	GPlatesGui::ProximityTests::find_close_rfgs(sorted_hits, view_state().reconstruction(),
			oriented_click_pos_on_globe, proximity_inclusion_threshold);
	
	// Give the user some useful feedback in the status bar.
	if (sorted_hits.size() == 0) {
		d_view_state_ptr->status_message(tr("Clicked %1 geometries.").arg(sorted_hits.size()));
	} else if (sorted_hits.size() == 1) {
		d_view_state_ptr->status_message(tr("Clicked %1 geometry.").arg(sorted_hits.size()));
	} else {
		d_view_state_ptr->status_message(tr("Clicked %1 geometries.").arg(sorted_hits.size()));
	}

	// Clear the 'Clicked' FeatureTableModel, ready to be populated (or not).
	d_clicked_table_model_ptr->clear();

	if (sorted_hits.size() == 0) {
		d_view_state_ptr->status_message(tr("Clicked 0 geometries."));
		// User clicked on empty space! Clear the currently focused feature.
		d_feature_focus_ptr->unset_focus();
		emit no_hits_found();
		return;
	}

	// Populate the 'Clicked' FeatureTableModel.
	d_clicked_table_model_ptr->begin_insert_features(0, static_cast<int>(sorted_hits.size()) - 1);
	while ( ! sorted_hits.empty())
	{
		d_clicked_table_model_ptr->geometry_sequence().push_back(
				sorted_hits.top().d_recon_geometry);
		sorted_hits.pop();
	}
	d_clicked_table_model_ptr->end_insert_features();
	d_view_state_ptr->highlight_first_clicked_feature_table_row();
	emit sorted_hits_updated();

#if 0  
// FIXME: remove this 
	// It seems it's not necessary to set the feature focus here, 
	// as it's already being set elsewhere.
	// Update the focused feature.

	GPlatesModel::ReconstructionGeometry *rg = sorted_hits.top().d_recon_geometry.get();
	// We use a dynamic cast here (despite the fact that dynamic casts are generally considered
	// bad form) because we only care about one specific derivation.  There's no "if ... else
	// if ..." chain, so I think it's not super-bad form.  (The "if ... else if ..." chain
	// would imply that we should be using polymorphism -- specifically, the double-dispatch of
	// the Visitor pattern -- rather than updating the "if ... else if ..." chain each time a
	// new derivation is added.)
	GPlatesModel::ReconstructedFeatureGeometry *rfg =
			dynamic_cast<GPlatesModel::ReconstructedFeatureGeometry *>(rg);
	if (rfg) {
		GPlatesModel::FeatureHandle::weak_ref feature_ref = rfg->feature_ref();
		if ( ! feature_ref.is_valid()) {
			// FIXME:  Replace this exception with a problem-specific exception which
			// doesn't contain a string.
			throw GPlatesGlobal::InternalInconsistencyException(__FILE__, __LINE__,
					"Invalid FeatureHandle::weak_ref returned from proximity tests.");
		}
		d_feature_focus_ptr->set_focus(feature_ref, rfg);
	}
#endif
}


void
GPlatesCanvasTools::BuildTopology::handle_shift_left_click(
		const GPlatesMaths::PointOnSphere &click_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_click_pos_on_globe,
		bool is_on_globe)
{
	const GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(
		oriented_click_pos_on_globe);

	// send the click point to the widget
	d_build_topology_widget_ptr->set_click_point( llp.latitude(), llp.longitude() );

	//
	// From ClickGeometry
	//
	double proximity_inclusion_threshold =
			globe_canvas().current_proximity_inclusion_threshold(click_pos_on_globe);
	
	// What did the user click on just now?
	std::priority_queue<GPlatesGui::ProximityTests::ProximityHit> sorted_hits;

	GPlatesGui::ProximityTests::find_close_rfgs(sorted_hits, view_state().reconstruction(),
			oriented_click_pos_on_globe, proximity_inclusion_threshold);
	
	// Give the user some useful feedback in the status bar.
	if (sorted_hits.size() == 0) {
		d_view_state_ptr->status_message(tr("Clicked %1 geometries.").arg(sorted_hits.size()));
	} else if (sorted_hits.size() == 1) {
		d_view_state_ptr->status_message(tr("Clicked %1 geometry.").arg(sorted_hits.size()));
	} else {
		d_view_state_ptr->status_message(tr("Clicked %1 geometries.").arg(sorted_hits.size()));
	}

	// Clear the 'Clicked' FeatureTableModel, ready to be populated (or not).
	d_clicked_table_model_ptr->clear();

	if (sorted_hits.size() == 0) {
		d_view_state_ptr->status_message(tr("Clicked 0 geometries."));
		// User clicked on empty space! Clear the currently focused feature.
		d_feature_focus_ptr->unset_focus();
		emit no_hits_found();
		return;
	}

	// Populate the 'Clicked' FeatureTableModel.
	d_clicked_table_model_ptr->begin_insert_features(0, static_cast<int>(sorted_hits.size()) - 1);
	while ( ! sorted_hits.empty())
	{
		d_clicked_table_model_ptr->geometry_sequence().push_back(
				sorted_hits.top().d_recon_geometry);
		sorted_hits.pop();
	}
	d_clicked_table_model_ptr->end_insert_features();
	d_view_state_ptr->highlight_first_clicked_feature_table_row();
	emit sorted_hits_updated();

	// Pass the click info to the widget 
	d_build_topology_widget_ptr->handle_shift_left_click(
		click_pos_on_globe, oriented_click_pos_on_globe, is_on_globe);

}


void
GPlatesCanvasTools::BuildTopology::handle_create_new_feature(
	GPlatesModel::FeatureHandle::weak_ref feature_ref)
{
qDebug() << "GPlatesCanvasTools::BuildTopology::handle_create_new_feature";
qDebug() << "feature_ref = " 
<< GPlatesUtils::make_qstring_from_icu_string( feature_ref->feature_id().get() );

static const GPlatesModel::PropertyName name_property_name =
	GPlatesModel::PropertyName::create_gml("name");
const GPlatesPropertyValues::XsString *name;
if ( GPlatesFeatureVisitors::get_property_value( *feature_ref, name_property_name, &name) )
{
	qDebug() << "name=" << GPlatesUtils::make_qstring( name->value() );
}

	// FIXME:
	// some users want the creation process to happen after selecting sections
	// do it this way:

	// append the new boundary prop value
	d_build_topology_widget_ptr->append_boundary_to_feature( feature_ref );

	// FIXME:
	// some users want the creation process to happen before selecting sections:

	// set the ref
	//d_build_topology_widget_ptr->set_topology_feature_ref( feature_ref );

	// FIXME:
	// be sure to synch with GPlatesQtWidgets::BuildTopologyWidget::activate() 
}





