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
#include <QDataStream>
#include <QString>

#include "GLFramebuffer.h"
#include "GLMultiResolutionRaster.h"
#include "GLProgram.h"
#include "GLRenderbuffer.h"
#include "GLScalarFieldDepthLayersSource.h"
#include "GLVertexArray.h"
#include "OpenGL.h"  // For Class GL and the OpenGL constants/typedefs

#include "file-io/ReadErrors.h"
#include "file-io/ScalarField3DFileFormat.h"

#include "property-values/CoordinateTransformation.h"
#include "property-values/Georeferencing.h"

#include "utils/ReferenceCount.h"


namespace GPlatesFileIO
{
	struct ReadErrorAccumulation;
}

namespace GPlatesOpenGL
{
	class GLMatrix;

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
				GL &gl,
				const QString &scalar_field_filename,
				const GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type &georeferencing,
				const GPlatesPropertyValues::CoordinateTransformation::non_null_ptr_to_const_type &coordinate_transformation,
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
				GL &gl,
				GPlatesFileIO::ReadErrorAccumulation *read_errors);

	private:

		QString d_scalar_field_filename;
		GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type d_georeferencing;
		GPlatesPropertyValues::CoordinateTransformation::non_null_ptr_to_const_type d_coordinate_transformation;
		depth_layer_seq_type d_depth_layers;

		boost::optional<GLScalarFieldDepthLayersSource::non_null_ptr_type> d_depth_layers_source;
		boost::optional<GLMultiResolutionRaster::non_null_ptr_type> d_multi_resolution_raster;
		unsigned int d_cube_face_dimension;

		//! Tile colour buffer.
		GLRenderbuffer::shared_ptr_type d_tile_colour_buffer;

		//! Tile stencil buffer.
		GLRenderbuffer::shared_ptr_type d_tile_stencil_buffer;

		//! Tile framebuffer object.
		GLFramebuffer::shared_ptr_type d_tile_framebuffer;

		//! Full-screen quad to render tile mask.
		GLVertexArray::shared_ptr_type d_full_screen_quad;

		//! Shader program to render the tile mask.
		GLProgram::shared_ptr_type d_render_tile_mask_program;


		//! Constructor.
		GLScalarField3DGenerator(
				GL &gl,
				const QString &scalar_field_filename,
				const GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type &georeferencing,
				const GPlatesPropertyValues::CoordinateTransformation::non_null_ptr_to_const_type &coordinate_transformation,
				unsigned int depth_layer_width,
				unsigned int depth_layer_height,
				const depth_layer_seq_type &depth_layers,
				GPlatesFileIO::ReadErrorAccumulation *read_errors);

		bool
		initialise_multi_resolution_raster(
				GL &gl,
				GPlatesFileIO::ReadErrorAccumulation *read_errors);

		void
		initialise_cube_face_dimension(
				GL &gl);

		void
		report_recoverable_error(
				GPlatesFileIO::ReadErrorAccumulation *read_errors,
				GPlatesFileIO::ReadErrors::Description description);

		void
		report_failure_to_begin(
				GPlatesFileIO::ReadErrorAccumulation *read_errors,
				GPlatesFileIO::ReadErrors::Description description);

		void
		create_tile_framebuffer(
				GL &gl);

		void
		compile_link_shader_program(
				GL &gl);

		void
		generate_scalar_field_depth_tile(
				GL &gl,
				QDataStream &out,
				unsigned int depth_layer_index,
				const GLMatrix &view_projection_transform,
				const std::vector<GLMultiResolutionRaster::tile_handle_type> &source_raster_tile_handles,
				unsigned int tile_resolution,
				double &tile_scalar_min,
				double &tile_scalar_max,
				double &scalar_min,
				double &scalar_max,
				double &scalar_sum,
				double &scalar_sum_squares,
				double &gradient_magnitude_min,
				double &gradient_magnitude_max,
				double &gradient_magnitude_sum,
				double &gradient_magnitude_sum_squares);

		void
		generate_scalar_field_tile_mask(
				GL &gl,
				unsigned int tile_resolution,
				std::vector<GPlatesFileIO::ScalarField3DFileFormat::MaskDataSample> &mask_data_array);
	};
}

#endif // GPLATES_OPENGL_GLSCALARFIELD3DGENERATOR_H
