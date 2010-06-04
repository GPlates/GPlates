/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#include "ViewState.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/PaleomagWorkflow.h"
#include "app-logic/PlateVelocityWorkflow.h"

#include "file-io/CptReader.h"
#include "file-io/ReadErrorAccumulation.h"

#include "gui/ColourRawRaster.h"
#include "gui/ColourSchemeContainer.h"
#include "gui/ColourSchemeDelegator.h"
#include "gui/ColourPaletteAdapter.h"
#include "gui/CptColourPalette.h"
#include "gui/FeatureFocus.h"
#include "gui/GeometryFocusHighlight.h"
#include "gui/MapTransform.h"
#include "gui/RasterColourPalette.h"
#include "gui/RenderSettings.h"
#include "gui/Texture.h"
#include "gui/VGPRenderSettings.h"
#include "gui/ViewportProjection.h"
#include "gui/ViewportZoom.h"

#include "maths/Real.h"

#include "property-values/Georeferencing.h"
#include "property-values/RawRaster.h"
#include "property-values/RawRasterUtils.h"

#include "utils/VirtualProxy.h"

#include "view-operations/ReconstructView.h"
#include "view-operations/RenderedGeometryCollection.h"


GPlatesPresentation::ViewState::ViewState(
		GPlatesAppLogic::ApplicationState &application_state) :
	d_application_state(application_state),
	d_other_view_state(NULL), // FIXME: remove this when refactored
	d_rendered_geometry_collection(
			new GPlatesViewOperations::RenderedGeometryCollection()),
	d_colour_scheme_container(
			new GPlatesGui::ColourSchemeContainer(*this)),
	d_colour_scheme(
			new GPlatesGui::ColourSchemeDelegator(*d_colour_scheme_container)),
	d_viewport_zoom(
			new GPlatesGui::ViewportZoom()),
	d_viewport_projection(
			new GPlatesGui::ViewportProjection(GPlatesGui::ORTHOGRAPHIC)),
	d_geometry_focus_highlight(
			new GPlatesGui::GeometryFocusHighlight(*d_rendered_geometry_collection)),
	d_feature_focus(
			new GPlatesGui::FeatureFocus(application_state)),
	d_comp_mesh_point_layer(
			d_rendered_geometry_collection->create_child_rendered_layer_and_transfer_ownership(
					GPlatesViewOperations::RenderedGeometryCollection::COMPUTATIONAL_MESH_LAYER,
					0.175f)),
	d_comp_mesh_arrow_layer(
			d_rendered_geometry_collection->create_child_rendered_layer_and_transfer_ownership(
					GPlatesViewOperations::RenderedGeometryCollection::COMPUTATIONAL_MESH_LAYER,
					0.175f)),
	d_paleomag_layer(
			d_rendered_geometry_collection->create_child_rendered_layer_and_transfer_ownership(
					GPlatesViewOperations::RenderedGeometryCollection::PALEOMAG_LAYER,
					0.175f)),					
	d_plate_velocity_workflow(
			new GPlatesAppLogic::PlateVelocityWorkflow(
					application_state, d_comp_mesh_point_layer, d_comp_mesh_arrow_layer)),
	d_plate_velocity_unregister(
			GPlatesAppLogic::FeatureCollectionWorkflow::register_and_create_auto_unregister_handle(
					d_plate_velocity_workflow.get(),
					application_state.get_feature_collection_file_state())),
	d_paleomag_workflow(
			new GPlatesAppLogic::PaleomagWorkflow(
					application_state, this, d_paleomag_layer)),
	d_paleomag_unregister(
			GPlatesAppLogic::FeatureCollectionWorkflow::register_and_create_auto_unregister_handle(
					d_paleomag_workflow.get(),
					application_state.get_feature_collection_file_state())),
	d_reconstruct_view(
			new GPlatesViewOperations::ReconstructView(
					*d_plate_velocity_workflow, *d_paleomag_workflow,*d_rendered_geometry_collection)),
	d_render_settings(
			new GPlatesGui::RenderSettings()),
	d_map_transform(
			new GPlatesGui::MapTransform()),
	d_main_viewport_min_dimension(0),
	d_vgp_render_settings(
			new GPlatesGui::VGPRenderSettings()),
	d_texture(
			new GPlatesGui::ProxiedTexture()),
	d_raw_raster(
			GPlatesPropertyValues::UninitialisedRawRaster::create()),
	d_georeferencing(
			GPlatesPropertyValues::Georeferencing::create()),
	d_is_raster_colour_map_invalid(false)
{
	// Call the operations in ReconstructView whenever a reconstruction is generated.
	get_application_state().set_reconstruction_hook(d_reconstruct_view.get());

	connect_to_viewport_zoom();

	// Connect to signals from FeatureFocus.
	connect_to_feature_focus();

	// Setup RenderedGeometryCollection.
	setup_rendered_geometry_collection();
}


GPlatesPresentation::ViewState::~ViewState()
{
	// boost::scoped_ptr destructor requires complete type.
}


GPlatesAppLogic::ApplicationState &
GPlatesPresentation::ViewState::get_application_state()
{
	return d_application_state;
}


GPlatesViewOperations::RenderedGeometryCollection &
GPlatesPresentation::ViewState::get_rendered_geometry_collection()
{
	return *d_rendered_geometry_collection;
}


GPlatesGui::FeatureFocus &
GPlatesPresentation::ViewState::get_feature_focus()
{
	return *d_feature_focus;
}


GPlatesGui::ViewportZoom &
GPlatesPresentation::ViewState::get_viewport_zoom()
{
	return *d_viewport_zoom;
}


GPlatesGui::ViewportProjection &
GPlatesPresentation::ViewState::get_viewport_projection()
{
	return *d_viewport_projection;
}


const GPlatesAppLogic::PlateVelocityWorkflow &
GPlatesPresentation::ViewState::get_plate_velocity_workflow() const
{
	return *d_plate_velocity_workflow;
}


GPlatesGui::ColourSchemeContainer &
GPlatesPresentation::ViewState::get_colour_scheme_container()
{
	return *d_colour_scheme_container;
}


GPlatesGlobal::PointerTraits<GPlatesGui::ColourScheme>::non_null_ptr_type
GPlatesPresentation::ViewState::get_colour_scheme()
{
	return d_colour_scheme;
}


GPlatesGlobal::PointerTraits<GPlatesGui::ColourSchemeDelegator>::non_null_ptr_type
GPlatesPresentation::ViewState::get_colour_scheme_delegator()
{
	return d_colour_scheme;
}


GPlatesGui::RenderSettings &
GPlatesPresentation::ViewState::get_render_settings()
{
	return *d_render_settings;
}


GPlatesGui::MapTransform &
GPlatesPresentation::ViewState::get_map_transform()
{
	return *d_map_transform;
}


int
GPlatesPresentation::ViewState::get_main_viewport_min_dimension()
{
	return d_main_viewport_min_dimension;
}


void
GPlatesPresentation::ViewState::set_main_viewport_min_dimension(
		int min_dimension)
{
	d_main_viewport_min_dimension = min_dimension;
}


void
GPlatesPresentation::ViewState::handle_zoom_change()
{
	d_rendered_geometry_collection->set_viewport_zoom_factor(d_viewport_zoom->zoom_factor());
}


void
GPlatesPresentation::ViewState::setup_rendered_geometry_collection()
{
	// Reconstruction rendered layer is always active.
	d_rendered_geometry_collection->set_main_layer_active(
		GPlatesViewOperations::RenderedGeometryCollection::RECONSTRUCTION_LAYER);

	d_rendered_geometry_collection->set_main_layer_active(
		GPlatesViewOperations::RenderedGeometryCollection::COMPUTATIONAL_MESH_LAYER);
		
	d_rendered_geometry_collection->set_main_layer_active(
		GPlatesViewOperations::RenderedGeometryCollection::SMALL_CIRCLE_TOOL_LAYER);	
		
	d_rendered_geometry_collection->set_main_layer_active(
		GPlatesViewOperations::RenderedGeometryCollection::PALEOMAG_LAYER);			
			

	// Activate the main rendered layer.
	// Specify which main rendered layers are orthogonal to each other - when
	// one is activated the others are automatically deactivated.
	GPlatesViewOperations::RenderedGeometryCollection::orthogonal_main_layers_type orthogonal_main_layers;
	orthogonal_main_layers.set(
			GPlatesViewOperations::RenderedGeometryCollection::DIGITISATION_LAYER);
	orthogonal_main_layers.set(
			GPlatesViewOperations::RenderedGeometryCollection::POLE_MANIPULATION_LAYER);
	orthogonal_main_layers.set(
			GPlatesViewOperations::RenderedGeometryCollection::GEOMETRY_FOCUS_HIGHLIGHT_LAYER);
	orthogonal_main_layers.set(
			GPlatesViewOperations::RenderedGeometryCollection::MEASURE_DISTANCE_LAYER);
	orthogonal_main_layers.set(
			GPlatesViewOperations::RenderedGeometryCollection::TOPOLOGY_TOOL_LAYER);

	d_rendered_geometry_collection->set_orthogonal_main_layers(orthogonal_main_layers);
}


void
GPlatesPresentation::ViewState::connect_to_viewport_zoom()
{
	// Handle zoom changes.
	QObject::connect(d_viewport_zoom.get(), SIGNAL(zoom_changed()),
			this, SLOT(handle_zoom_change()));
}


void
GPlatesPresentation::ViewState::connect_to_feature_focus()
{
	// If the focused feature is modified, we may need to reconstruct to update the view.
	// FIXME:  If the FeatureFocus emits the 'focused_feature_modified' signal, the view will
	// be reconstructed twice -- once here, and once as a result of the 'set_focus' slot in the
	// GeometryFocusHighlight below.
	QObject::connect(
			&get_feature_focus(),
			SIGNAL(focused_feature_modified(GPlatesGui::FeatureFocus &)),
			&get_application_state(),
			SLOT(reconstruct()));

	// Connect the geometry-focus highlight to the feature focus.
	QObject::connect(
			&get_feature_focus(),
			SIGNAL(focus_changed(
					GPlatesGui::FeatureFocus &)),
			d_geometry_focus_highlight.get(),
			SLOT(set_focus(
					GPlatesGui::FeatureFocus &)));

	QObject::connect(
			&get_feature_focus(),
			SIGNAL(focused_feature_modified(
					GPlatesGui::FeatureFocus &)),
			d_geometry_focus_highlight.get(),
			SLOT(set_focus(
					GPlatesGui::FeatureFocus &)));
}


GPlatesGui::VGPRenderSettings &
GPlatesPresentation::ViewState::get_vgp_render_settings()
{
	return *d_vgp_render_settings;
}


GPlatesGui::ProxiedTexture &
GPlatesPresentation::ViewState::get_texture()
{
	return *d_texture;
}


GPlatesGlobal::PointerTraits<GPlatesPropertyValues::RawRaster>::non_null_ptr_type
GPlatesPresentation::ViewState::get_raw_raster()
{
	return d_raw_raster;
}


void
GPlatesPresentation::ViewState::set_raw_raster(
		GPlatesGlobal::PointerTraits<GPlatesPropertyValues::RawRaster>::non_null_ptr_type raw_raster)
{
	d_raw_raster = raw_raster;
}


namespace
{
	boost::optional<GPlatesPropertyValues::Rgba8RawRaster::non_null_ptr_type>
	colour_raw_raster_using_default(
			GPlatesPropertyValues::RawRaster &raw_raster)
	{
		// Get statistics.
		GPlatesPropertyValues::RasterStatistics *statistics_ptr =
			GPlatesPropertyValues::RawRasterUtils::get_raster_statistics(raw_raster);
		if (!statistics_ptr)
		{
			return boost::none;
		}
		GPlatesPropertyValues::RasterStatistics &statistics = *statistics_ptr;
		if (!statistics.mean || !statistics.standard_deviation)
		{
			return boost::none;
		}
		double mean = *statistics.mean;
		double std_dev = *statistics.standard_deviation;

		// Create default raster colour palette.
		GPlatesGui::DefaultRasterColourPalette::non_null_ptr_type rgba8_palette =
			GPlatesGui::DefaultRasterColourPalette::create(mean, std_dev);

		// Convert the non-RGBA8 RawRaster into a RGBA8 RawRaster.
		return GPlatesGui::ColourRawRaster::colour_raw_raster<double>(
				raw_raster, rgba8_palette);
	}


	boost::optional<GPlatesPropertyValues::Rgba8RawRaster::non_null_ptr_type>
	colour_raw_raster_using_cpt(
			GPlatesPropertyValues::RawRaster &raw_raster,
			const QString &cpt_filename)
	{
		// FIXME: For now, we just suppress the read errors.
		GPlatesFileIO::ReadErrorAccumulation read_errors;

		// For real-valued rasters, we must use a regular CPT file to colour it.
		// For int-valued rasters, we can use either a regular or categorical CPT file
		// (we can use IntegerCptReader, which abstracts away the difference for us).
		GPlatesPropertyValues::RawRasterUtils::RawRasterTypeInfoVisitor visitor;
		raw_raster.accept_visitor(visitor);

		if (visitor.is_floating_point())
		{
			// Attempt to read the CPT file.
			GPlatesFileIO::RegularCptReader cpt_reader;
			GPlatesGui::RegularCptColourPalette::maybe_null_ptr_type rgba8_palette_opt =
				cpt_reader.read_file(cpt_filename, read_errors);
			if (!rgba8_palette_opt)
			{
				return boost::none;
			}

			// Colour the raster using our colour palette.
			GPlatesGui::RegularCptColourPalette::non_null_ptr_type rgba8_palette = rgba8_palette_opt.get();
			GPlatesGui::ColourPalette<double>::non_null_ptr_type adapted_palette =
				GPlatesGui::ColourPaletteAdapter
					<GPlatesMaths::Real, double, GPlatesGui::RealToBuiltInConverter<double> >::create(
						rgba8_palette);
			return GPlatesGui::ColourRawRaster::colour_raw_raster<double>(
					raw_raster, adapted_palette);
		}
		else if (visitor.is_integer())
		{
			// FIXME: Differentiate between signed/unsigned integer rasters.

			// Attempt to read the CPT file.
			GPlatesFileIO::IntegerCptReader<int> cpt_reader;
			GPlatesGui::ColourPalette<int>::maybe_null_ptr_type rgba8_palette_opt =
				cpt_reader.read_file(cpt_filename, read_errors);
			if (!rgba8_palette_opt)
			{
				return boost::none;
			}

			// Colour the raster using our colour palette.
			GPlatesGui::ColourPalette<int>::non_null_ptr_type rgba8_palette = rgba8_palette_opt.get();
			return GPlatesGui::ColourRawRaster::colour_raw_raster<int>(
					raw_raster, rgba8_palette);
		}
		else
		{
			return boost::none;
		}
	}
}


bool
GPlatesPresentation::ViewState::update_texture_from_raw_raster()
{
	// See whether it's an Rgba8RawRaster.
	boost::optional<GPlatesPropertyValues::Rgba8RawRaster::non_null_ptr_type> rgba8_raster_opt =
		GPlatesPropertyValues::RawRasterUtils::try_rgba8_raster_cast(*d_raw_raster);
	if (!rgba8_raster_opt)
	{
		if (!d_raster_colour_map_filename.isEmpty())
		{
			// If there's a CPT file set, let's try to colour using it.
			rgba8_raster_opt = colour_raw_raster_using_cpt(
					*d_raw_raster, d_raster_colour_map_filename);
			if (rgba8_raster_opt)
			{
				d_is_raster_colour_map_invalid = false;
			}
			else
			{
				d_is_raster_colour_map_invalid = true;

				// Colour using the default raster colour palette instead.
				rgba8_raster_opt = colour_raw_raster_using_default(*d_raw_raster);
			}
		}
		else
		{
			// Otherwise, just colour using the default raster colour palette.
			rgba8_raster_opt = colour_raw_raster_using_default(*d_raw_raster);

			d_is_raster_colour_map_invalid = false;
		}

		if (!rgba8_raster_opt)
		{
			return false;
		}
	}

	// Image size.
	boost::optional<std::pair<unsigned int, unsigned int> > image_size =
		GPlatesPropertyValues::RawRasterUtils::get_raster_size(*d_raw_raster);
	if (!image_size)
	{
		return false;
	}
	QSize qsize(image_size->first, image_size->second);

	// Feed the bytes into Texture.
	GPlatesGui::Texture &texture = *get_texture();
	GPlatesGui::rgba8_t *rgba8_buf = (*rgba8_raster_opt)->data().get();
	GPlatesPropertyValues::InMemoryRaster::ColourFormat format =
		GPlatesPropertyValues::InMemoryRaster::RgbaFormat;
	texture.generate_raster(
			reinterpret_cast<unsigned char *>(rgba8_buf),
			qsize,
			format);
	texture.set_enabled(true);
	
	return true;
}


GPlatesGlobal::PointerTraits<GPlatesPropertyValues::Georeferencing>::non_null_ptr_type
GPlatesPresentation::ViewState::get_georeferencing()
{
	return d_georeferencing;
}


void
GPlatesPresentation::ViewState::update_texture_extents()
{
	// Convert to QRectF for Texture.
	// We'll nuke the Texture class later, but for now, we still need to talk to it.
	boost::optional<std::pair<unsigned int, unsigned int> > size =
		GPlatesPropertyValues::RawRasterUtils::get_raster_size(*d_raw_raster);
	if (size)
	{
		boost::optional<GPlatesPropertyValues::Georeferencing::lat_lon_extents_type> extents =
			d_georeferencing->lat_lon_extents(size->first, size->second);
		if (extents)
		{
			// Sanity check because the current raster rendering code is quite picky.
			if (GPlatesMaths::Real(extents->left) > GPlatesMaths::Real(extents->right))
			{
				return;
			}

			QRectF extents_as_rect(
					QPointF(extents->left, extents->top),
					QPointF(extents->right, extents->bottom));
			get_texture()->set_extent(extents_as_rect);
		}
	}
}


const QString &
GPlatesPresentation::ViewState::get_raster_filename() const
{
	return d_raster_filename;
}


void
GPlatesPresentation::ViewState::set_raster_filename(
		const QString &filename)
{
	d_raster_filename = filename;
}


const QString &
GPlatesPresentation::ViewState::get_raster_colour_map_filename() const
{
	return d_raster_colour_map_filename;
}


void
GPlatesPresentation::ViewState::set_raster_colour_map_filename(
		const QString &filename)
{
	d_raster_colour_map_filename = filename;
}

