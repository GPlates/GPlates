/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#ifndef GPLATES_OPENGL_GLSCALARFIELD3DGENERATOR_H
#define GPLATES_OPENGL_GLSCALARFIELD3DGENERATOR_H

#include <vector>
#include <boost/optional.hpp>
#include <QString>

#include "GLMultiResolutionRaster.h"
#include "GLScalarFieldDepthLayersSource.h"
#include "GLTexture.h"

#include "file-io/ReadErrors.h"

#include "property-values/Georeferencing.h"

#include "utils/ReferenceCount.h"


namespace GPlatesFileIO
{
	struct ReadErrorAccumulation;
}

namespace GPlatesOpenGL
{
	class GLRenderer;

	/**
	 * Generates a 3D sub-surface scalar field from a sequence of concentric depth layer 2D rasters.
	 */
	class GLScalarField3DGenerator :
			public GPlatesUtils::ReferenceCount<GLScalarField3DGenerator>
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLScalarField3DGenerator.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLScalarField3DGenerator> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLScalarField3DGenerator.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLScalarField3DGenerator> non_null_ptr_to_const_type;


		/**
		 * A single depth layer contributing to the 3D scalar field.
		 */
		struct DepthLayer
		{
			// Note that @a depth_radius_ is normalised [0,1] sphere radius.
			DepthLayer(
					const QString &depth_raster_filename_,
					double depth_radius_) :
				depth_raster_filename(depth_raster_filename_),
				depth_radius(depth_radius_)
			{  }

			QString depth_raster_filename;
			double depth_radius;
		};

		typedef std::vector<DepthLayer> depth_layer_seq_type;


		/**
		 * Returns true if generation of 3D scalar fields is supported on the runtime system.
		 *
		 * This is less than that required to render 3D scalar fields (OpenGL 3.0) and is roughly OpenGL 2.0.
		 */
		static
		bool
		is_supported(
				GLRenderer &renderer);


		/**
		 * Creates a @a GLScalarField3DGenerator object.
		 *
		 * @a scalar_field_filename is name of the file to contain the generated scalar field.
		 * @a georeferencing all depth layer rasters have the same georeferencing.
		 * @a depth_layers the depth layer rasters used to generate the scalar field from.
		 *
		 * NOTE: The depth layers do not need to be sorted by depth - that will be handled by this function.
		 */
		static
		non_null_ptr_type
		create(
				GLRenderer &renderer,
				const QString &scalar_field_filename,
				const GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type &georeferencing,
				unsigned int depth_layer_width,
				unsigned int depth_layer_height,
				const depth_layer_seq_type &depth_layers,
				GPlatesFileIO::ReadErrorAccumulation *read_errors);


		/**
		 * Generate and write the scalar field to file.
		 *
		 * Returns false on failure.
		 */
		bool
		generate_scalar_field(
				GLRenderer &renderer,
				GPlatesFileIO::ReadErrorAccumulation *read_errors);

	private:

		QString d_scalar_field_filename;
		GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type d_georeferencing;
		depth_layer_seq_type d_depth_layers;

		boost::optional<GLScalarFieldDepthLayersSource::non_null_ptr_type> d_depth_layers_source;
		boost::optional<GLMultiResolutionRaster::non_null_ptr_type> d_multi_resolution_raster;
		unsigned int d_cube_face_dimension;


		//! Constructor.
		GLScalarField3DGenerator(
				GLRenderer &renderer,
				const QString &scalar_field_filename,
				const GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type &georeferencing,
				unsigned int depth_layer_width,
				unsigned int depth_layer_height,
				const depth_layer_seq_type &depth_layers,
				GPlatesFileIO::ReadErrorAccumulation *read_errors);

		bool
		initialise_multi_resolution_raster(
				GLRenderer &renderer,
				GPlatesFileIO::ReadErrorAccumulation *read_errors);

		void
		initialise_cube_face_dimension(
				GLRenderer &renderer);

		GLTexture::shared_ptr_type
		create_cube_tile_texture(
				GLRenderer &renderer,
				unsigned int tile_resolution);

		void
		report_recoverable_error(
				GPlatesFileIO::ReadErrorAccumulation *read_errors,
				GPlatesFileIO::ReadErrors::Description description);

		void
		report_failure_to_begin(
				GPlatesFileIO::ReadErrorAccumulation *read_errors,
				GPlatesFileIO::ReadErrors::Description description);
	};
}

#endif // GPLATES_OPENGL_GLSCALARFIELD3DGENERATOR_H
