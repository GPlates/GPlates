/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2009 Geological Survey of Norway
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
#include <cmath>
#include <cstring>

#include <QtOpenGL/qgl.h>
#include <QFileDialog>
#include <QObject>
#include <QString>

#include "Texture.h"

#include "RasterColourPalette.h"

#include "opengl/GLBlendState.h"
#include "opengl/GLCompositeStateSet.h"
#include "opengl/GLMultiResolutionRasterNode.h"
#include "opengl/GLTextureEnvironmentState.h"
#include "opengl/OpenGLBadAllocException.h"
#include "opengl/OpenGLException.h"

#include "maths/PointOnSphere.h"

#include "property-values/RawRasterUtils.h"


GPlatesGui::Texture::Texture() :
	d_georeferencing(
			GPlatesPropertyValues::Georeferencing::create()),
	d_raw_raster(
			GPlatesPropertyValues::UninitialisedRawRaster::create()),
	d_updated_georeferencing(false),
	d_updated_raster(false),
	d_enabled(false),
	d_is_loaded(false)
{ }


void
GPlatesGui::Texture::set_georeferencing(
		const GPlatesPropertyValues::Georeferencing::non_null_ptr_type &georeferencing)
{
	d_georeferencing = georeferencing;
	d_updated_georeferencing = true;
	emit texture_changed();
}


void
GPlatesGui::Texture::set_raster(
		const GPlatesPropertyValues::RawRaster::non_null_ptr_type &raw_raster)
{
	d_raw_raster = raw_raster;

	// Setup a default colour scheme for non-RGBA rasters...
	// This should work for all raster types.
	GPlatesPropertyValues::RasterStatistics *statistics_ptr =
		GPlatesPropertyValues::RawRasterUtils::get_raster_statistics(*d_raw_raster);
	if (statistics_ptr)
	{
		GPlatesPropertyValues::RasterStatistics &statistics = *statistics_ptr;
		if (!statistics.mean || !statistics.standard_deviation)
		{
			return;
		}
		double mean = *statistics.mean;
		double std_dev = *statistics.standard_deviation;
		GPlatesGui::DefaultRasterColourPalette::non_null_ptr_type rgba8_palette =
			GPlatesGui::DefaultRasterColourPalette::create(mean, std_dev);

		d_raster_colour_scheme =
				GPlatesGui::RasterColourScheme::create<double>("band name", rgba8_palette);
	}

	d_updated_raster = true;
	
	d_is_loaded = true;

	emit texture_changed();
}


void
GPlatesGui::Texture::paint(
		const GPlatesOpenGL::GLRenderGraphInternalNode::non_null_ptr_type &render_graph_parent_node,
		const GPlatesOpenGL::GLTextureResourceManager::shared_ptr_type &texture_resource_manager)
{
	if (!d_enabled)
	{
		return;
	}

	if (d_updated_raster)
	{
		if (d_multi_resolution_raster)
		{
			// We already have a raster so see if we can just update the raster data.
			// This will return false if the raster dimensions differ.
			if (!d_proxied_raster_source.get()->change_raster(d_raw_raster, d_raster_colour_scheme))
			{
				// The raster dimensions differ so create a new multi-resolution raster.
				d_proxied_raster_source = GPlatesOpenGL::GLProxiedRasterSource::create(
						d_raw_raster, d_raster_colour_scheme);
				d_multi_resolution_raster = GPlatesOpenGL::GLMultiResolutionRaster::create(
						d_georeferencing, d_proxied_raster_source.get(), texture_resource_manager);
			}
		}
		else
		{
			// Haven't created a multi-resolution raster yet so create one.
			d_proxied_raster_source = GPlatesOpenGL::GLProxiedRasterSource::create(
					d_raw_raster, d_raster_colour_scheme);
			d_multi_resolution_raster = GPlatesOpenGL::GLMultiResolutionRaster::create(
					d_georeferencing, d_proxied_raster_source.get(), texture_resource_manager);
		}

		d_updated_raster = false;
		d_updated_georeferencing = false;
	}
	else if (d_updated_georeferencing)
	{
		if (d_multi_resolution_raster)
		{
			// Just create a new multi-resolution raster if the georeferencing has been updated.
			d_proxied_raster_source = GPlatesOpenGL::GLProxiedRasterSource::create(
					d_raw_raster, d_raster_colour_scheme);
			d_multi_resolution_raster = GPlatesOpenGL::GLMultiResolutionRaster::create(
					d_georeferencing, d_proxied_raster_source.get(), texture_resource_manager);
		}

		d_updated_georeferencing = false;
	}

	if (!d_multi_resolution_raster)
	{
		return;
	}

	GPlatesOpenGL::GLMultiResolutionRasterNode::non_null_ptr_type multi_resolution_raster_node =
			GPlatesOpenGL::GLMultiResolutionRasterNode::create(d_multi_resolution_raster.get());

	GPlatesOpenGL::GLCompositeStateSet::non_null_ptr_type state_set =
			GPlatesOpenGL::GLCompositeStateSet::create();

	// Enable texturing and set the texture function.
	GPlatesOpenGL::GLTextureEnvironmentState::non_null_ptr_type tex_env_state =
			GPlatesOpenGL::GLTextureEnvironmentState::create();
	tex_env_state->gl_enable_texture_2D(GL_TRUE);
	tex_env_state->gl_tex_env_mode(GL_REPLACE);
	state_set->add_state_set(tex_env_state);

	// Enable alpha-blending in case texture has partial transparency.
	GPlatesOpenGL::GLBlendState::non_null_ptr_type blend_state =
			GPlatesOpenGL::GLBlendState::create();
	blend_state->gl_enable(GL_TRUE).gl_blend_func(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	state_set->add_state_set(blend_state);

	// Set the state on the multi-resolution raster node.
	multi_resolution_raster_node->set_state_set(state_set);

	render_graph_parent_node->add_child_node(multi_resolution_raster_node);
}


void
GPlatesGui::Texture::set_enabled(
		bool enabled)
{
	d_enabled = enabled;
	emit texture_changed();
}


void
GPlatesGui::Texture::toggle()
{
	d_enabled = !d_enabled;
	emit texture_changed();
}
