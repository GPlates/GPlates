/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
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

#include <cmath>
#include <QFile>
#include <QIODevice>
#include <QString>

#include "DigitiseGeometry.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/PlanetaryParameters.h"
#include "app-logic/ProjectDocumentRegistry.h"
#include "app-logic/ProjectMetadata.h"

#include "maths/MathsUtils.h"
#include "maths/Rotation.h"
#include "maths/UnitVector3D.h"
#include "maths/Vector3D.h"

#include "presentation/Application.h"

#include "view-operations/AddPointGeometryOperation.h"
#include "view-operations/GeometryBuilder.h"
#include "view-operations/RenderedGeometryCollection.h"


namespace
{
	// Used when a Primary Project Document exists but says nothing about resolution, or there is
	// no Primary Project Document at all - matches the project template's own default_km, so
	// Shift-clicking still does something sane rather than nothing.
	const double FALLBACK_RESOLUTION_KM = 500.0;


	/**
	 * The furthest a Shift-clicked vertex should land from the previous one, in kilometres.
	 *
	 * Reads the gplates.resolution front matter of the Primary Project Document. A geometry being
	 * digitised has no feature type yet - the user picks that when the feature is created - so the
	 * project default is the only thing that can apply, and per-feature-type overrides do not.
	 * This is read per click rather than cached, since the document can change while the tool is
	 * active.
	 */
	double
	resolution_kilometres(
			GPlatesAppLogic::ApplicationState &application_state)
	{
		const QString primary_document_path =
				application_state.get_project_document_registry().primary_document_path();
		if (!primary_document_path.isEmpty())
		{
			QFile primary_document(primary_document_path);
			if (primary_document.open(QIODevice::ReadOnly))
			{
				const GPlatesAppLogic::ProjectMetadata metadata =
						GPlatesAppLogic::ProjectMetadataParser::parse(
								QString::fromUtf8(primary_document.readAll()));
				if (metadata.default_resolution_km)
				{
					return metadata.default_resolution_km.get();
				}
			}
		}
		return FALLBACK_RESOLUTION_KM;
	}
}


GPlatesCanvasTools::DigitiseGeometry::DigitiseGeometry(
		const status_bar_callback_type &status_bar_callback,
		GPlatesMaths::GeometryType::Value geom_type,
		GPlatesViewOperations::GeometryBuilder &geometry_builder,
		GPlatesCanvasTools::GeometryOperationState &geometry_operation_state,
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
		GPlatesViewOperations::RenderedGeometryCollection::MainLayerType main_rendered_layer_type,
		GPlatesGui::CanvasToolWorkflows &canvas_tool_workflows,
		const GPlatesViewOperations::QueryProximityThreshold &query_proximity_threshold) :
	CanvasTool(status_bar_callback),
	d_default_geom_type(geom_type),
	d_geometry_builder(geometry_builder),
	d_add_point_geometry_operation(
		   new GPlatesViewOperations::AddPointGeometryOperation(
					geom_type,
					geometry_builder,
					geometry_operation_state,
					rendered_geometry_collection,
					main_rendered_layer_type,
					canvas_tool_workflows,
					query_proximity_threshold))
{  }


GPlatesCanvasTools::DigitiseGeometry::~DigitiseGeometry()
{
	// boost::scoped_ptr destructor needs complete type.
}

void
GPlatesCanvasTools::DigitiseGeometry::handle_activation()
{
	// In addition to adding points - our dual responsibility is to change
	// the type of geometry the builder is attempting to build.
	//
	// Set type to build - ignore returned undo operation (this is handled
	// at a higher level).
	d_geometry_builder.set_geometry_type_to_build(d_default_geom_type);

	// Activate our AddPointGeometryOperation - it will add points to the specified GeometryBuilder
	// and add RenderedGeometry objects to the specified main render layer.
	d_add_point_geometry_operation->activate();

	if (d_default_geom_type == GPlatesMaths::GeometryType::MULTIPOINT)
	{
		set_status_bar_message(QT_TR_NOOP("Click to draw a new point."));
	}
	else
	{
		set_status_bar_message(QT_TR_NOOP("Click to draw a new vertex."));
	}
}

void
GPlatesCanvasTools::DigitiseGeometry::handle_deactivation()
{
	// Deactivate our AddPointGeometryOperation.
	d_add_point_geometry_operation->deactivate();
}


void
GPlatesCanvasTools::DigitiseGeometry::handle_left_click(
		const GPlatesMaths::PointOnSphere &point_on_sphere,
		bool is_on_earth,
		double proximity_inclusion_threshold)
{
	d_add_point_geometry_operation->add_point(
			point_on_sphere,
			proximity_inclusion_threshold);
}


void
GPlatesCanvasTools::DigitiseGeometry::handle_shift_left_click(
		const GPlatesMaths::PointOnSphere &point_on_sphere,
		bool is_on_earth,
		double proximity_inclusion_threshold)
{
	// With no previous vertex there is nothing to measure from, so this is an ordinary first click.
	const GPlatesViewOperations::GeometryBuilder::GeometryIndex geometry_index =
			d_geometry_builder.get_current_geometry_index();
	const unsigned int num_points =
			d_geometry_builder.get_num_geometries() > 0
					? d_geometry_builder.get_num_points_in_geometry(geometry_index)
					: 0;
	if (num_points == 0)
	{
		handle_left_click(point_on_sphere, is_on_earth, proximity_inclusion_threshold);
		return;
	}

	const GPlatesMaths::PointOnSphere previous_point =
			d_geometry_builder.get_geometry_point(geometry_index, num_points - 1);

	GPlatesAppLogic::ApplicationState &application_state =
			GPlatesPresentation::Application::instance().get_application_state();

	const double resolution_km = resolution_kilometres(application_state);
	const double radius_km =
			application_state.get_planetary_parameters().effective_radius_kilometres();
	if (radius_km <= 0 || resolution_km <= 0)
	{
		handle_left_click(point_on_sphere, is_on_earth, proximity_inclusion_threshold);
		return;
	}

	// Distance on a sphere is an angle: the arc length divided by the radius.
	const double max_angle_radians = resolution_km / radius_km;
	const double click_angle_radians =
			std::acos(
					GPlatesMaths::dot(
							previous_point.position_vector(),
							point_on_sphere.position_vector()).dval());

	// Written this way so a NaN angle falls through to a plain click rather than clamping.
	if (!(click_angle_radians > max_angle_radians))
	{
		// Already within the resolution, so the click stands as it is. Clamping here would move
		// the vertex away from where the user clicked for no benefit.
		handle_left_click(point_on_sphere, is_on_earth, proximity_inclusion_threshold);
		return;
	}

	// Rotate the previous point towards the click, by the largest angle the project allows. The
	// axis is perpendicular to both, which is the plane the great circle between them lies in.
	const GPlatesMaths::Vector3D axis =
			GPlatesMaths::cross(
					previous_point.position_vector(),
					point_on_sphere.position_vector());
	if (axis.magSqrd() <= 0.0)
	{
		// The click is on the previous point, or exactly opposite it, so there is no unique great
		// circle to move along. Nothing sensible to clamp to.
		handle_left_click(point_on_sphere, is_on_earth, proximity_inclusion_threshold);
		return;
	}

	const GPlatesMaths::Rotation rotation =
			GPlatesMaths::Rotation::create(
					GPlatesMaths::UnitVector3D(axis.get_normalisation()),
					max_angle_radians);

	handle_left_click(rotation * previous_point, is_on_earth, proximity_inclusion_threshold);
}
