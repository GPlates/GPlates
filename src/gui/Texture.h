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

#ifndef GPLATES_GUI_TEXTURE_H
#define GPLATES_GUI_TEXTURE_H

#include <boost/optional.hpp>
#include <QtOpenGL/qgl.h>
#include <QString>
#include <QObject>

#include "opengl/GLMultiResolutionRaster.h"
#include "opengl/GLRenderGraphInternalNode.h"
#include "opengl/GLResourceManager.h"

#include "property-values/Georeferencing.h"
#include "property-values/InMemoryRaster.h"
#include "property-values/RawRaster.h"

#include "utils/VirtualProxy.h"


namespace GPlatesGui
{
	class Texture :
			public QObject,
			public GPlatesPropertyValues::InMemoryRaster
	{
		Q_OBJECT

	public:
		//! Constructor.
		Texture();


		/**
		 * Return the georeferencing.
		 */ 
		const GPlatesPropertyValues::Georeferencing::non_null_ptr_type &
		get_georeferencing()
		{
			return d_georeferencing;
		}

		/**
		 * Set the coordinate range over which the texture will be mapped.
		 */
		void
		set_georeferencing(
				const GPlatesPropertyValues::Georeferencing::non_null_ptr_type &georeferencing);

		/**
		 * Specify raster data.
		 */
		void
		set_raster(
				const GPlatesPropertyValues::RawRaster::non_null_ptr_type &raw_raster);


		/**
		 * Adds multi-resolution raster to render graph.
		 */
		void
		paint(
				const GPlatesOpenGL::GLRenderGraphInternalNode::non_null_ptr_type &render_graph_parent_node,
				const GPlatesOpenGL::GLTextureResourceManager::shared_ptr_type &texture_resource_manager);

		/**
		 * Returns the value of d_enabled, which determines whether or not the texture
		 * is displayed. 
		 */ 
		bool
		is_enabled()
		{
			return d_enabled;
		}

		/**
		 * Sets the value of the boolean d_enabled, which determines whether or not the texture
		 * is displayed.
		 */
		void
		set_enabled(
				bool enabled);
	
		/**
		 * Toggles the state of the boolean d_enabled, which determines whether or not the texture
		 * is displayed.
		 */
		void
		toggle();

		bool
		is_loaded()
		{
			return d_is_loaded;
		}

	signals:

		//! Emitted when the raster is loaded or changed, and when the texture is enabled/disabled
		void
		texture_changed();

	private:
		/**
		 * The georeferencing.
		 */
		GPlatesPropertyValues::Georeferencing::non_null_ptr_type d_georeferencing;

		/**
		 * The raster data. 
		 */
		GPlatesPropertyValues::RawRaster::non_null_ptr_type d_raster;

		/**
		 * Rendering is done with this OpenGL raster.
		 */
		boost::optional<GPlatesOpenGL::GLMultiResolutionRaster::non_null_ptr_type> d_multi_resolution_raster;

		bool d_updated_georeferencing;
		bool d_updated_raster;

		/**
		 * Whether or not the texture is displayed.
		 */
		bool d_enabled;

		/**
		 * Whether a raster has been loaded.
		 */
		bool d_is_loaded;
	};


	/**
	 * Delays creation of the texture until first use.
	 */
	typedef GPlatesUtils::VirtualProxy<Texture> ProxiedTexture;
	
}

#endif // GPLATES_GUI_TEXTURE_H

