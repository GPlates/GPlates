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

#ifndef GPLATES_OPENGL_GLSCALARFIELD3D_H
#define GPLATES_OPENGL_GLSCALARFIELD3D_H

#include <cstddef> // For std::size_t
#include <vector>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <QString>

#include "GLCubeSubdivision.h"
#include "GLFrameBufferObject.h"
#include "GLLight.h"
#include "GLProgramObject.h"
#include "GLShaderObject.h"
#include "GLTexture.h"

#include "file-io/ScalarField3DFileFormatReader.h"

#include "gui/ColourPalette.h"

#include "utils/ReferenceCount.h"
#include "utils/SubjectObserverToken.h"


namespace GPlatesOpenGL
{
	class GLRenderer;

	/**
	 * A 3D sub-surface scalar field represented as a cube map of concentric depth layers.
	 */
	class GLScalarField3D :
			public GPlatesUtils::ReferenceCount<GLScalarField3D>
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLScalarField3D.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLScalarField3D> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLScalarField3D.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLScalarField3D> non_null_ptr_to_const_type;

		/**
		 * Typedef for an opaque object that caches a particular render of this scalar field.
		 */
		typedef boost::shared_ptr<void> cache_handle_type;


		/**
		 * Returns true if rendering (ray-tracing) of 3D scalar fields are supported on the runtime system.
		 *
		 * This essentially requires graphics hardware supporting OpenGL 3.0.
		 */
		static
		bool
		is_supported(
				GLRenderer &renderer);


		/**
		 * Creates a @a GLScalarField3D object.
		 *
		 * @a scalar_field_filename is the cube map file containing the source of scalar field data.
		 *
		 * @a light determines the light direction and other parameters to use when rendering the scalar field.
		 */
		static
		non_null_ptr_type
		create(
				GLRenderer &renderer,
				const QString &scalar_field_filename,
				const GLLight::non_null_ptr_type &light);

		/**
		 * Set the iso-value of the iso-surface to render.
		 *
		 * The default value is zero.
		 */
		void
		set_iso_value(
				GLRenderer &renderer,
				float iso_value);

		/**
		 * Set the colour palette.
		 *
		 * The colour palette is expected to have the input range [0,1].
		 */
		void
		set_colour_palette(
				GLRenderer &renderer,
				const GPlatesGui::ColourPalette<double>::non_null_ptr_to_const_type &colour_palette);

		/**
		 * Optional test variables to use during shader program development.
		 *
		 * These variables are in the range [0,1] and come from the scalar field visual layer GUI.
		 * They are passed directly to uniform shader variables in the shader program(s).
		 *
		 * @a test_variables can be any size but typically only the first [0,N-1] variables are used
		 * where N is on the order of ten. Extra variables, that don't exist in the shader, will emit
		 * a warning (but that can be ignored during testing).
		 */
		void
		set_shader_test_variables(
				GLRenderer &renderer,
				const std::vector<float> &test_variables);


		/**
		 * Change to a new scalar field of the same dimensions as the current internal scalar field.
		 *
		 * This method is useful for time-dependent scalar fields with the same dimensions.
		 *
		 * Returns false if the scalar field to be loaded from @a scalar_field_filename has different
		 * dimensions than the current internal scalar field.
		 * In this case you'll need to create a new @a GLScalarField.
		 */
		bool
		change_scalar_field(
				GLRenderer &renderer,
				const QString &scalar_field_filename);


		/**
		 * Returns a subject token that clients can observe to see if they need to update themselves
		 * (such as any cached data we render for them) by getting us to re-render.
		 */
		const GPlatesUtils::SubjectToken &
		get_subject_token() const;


		/**
		 * Renders the scalar field visible in the view frustum (determined by the current
		 * model-view/projection transforms of @a renderer).
		 *
		 * @a surface_occlusion_texture is a viewport-size 2D texture containing the RGBA rendering
		 * of the surface geometries/rasters on the *front* of the globe.
		 * If specified it will be used to early terminate ray-tracing for those pixels that are
		 * occluded by surface geometries/rasters (that have alpha of 1.0).
		 *
		 * @a depth_read_texture is a viewport-size 2D texture containing the screen-space depth
		 * (in range [-1,1] as opposed to window coordinate range [0,1]) of sub-surface structures
		 * already rendered.
		 * If specified it will be used to early terminate ray-tracing for those pixels that are
		 * depth-occluded by sub-surface structures.
		 * NOTE: This depth might not contain all sub-surface structures rendered so far so it
		 * shouldn't be used as a replacement for hardware depth testing (ie, hardware depth testing
		 * and depth writes should still both be enabled).
		 *
		 * The screen-space depth (in range [-1,1] as opposed to window coordinate range [0,1]) is
		 * always output by the fragment shader, during rendering, to the draw buffer at index 1
		 * (the scalar field colour is output to draw buffer index 0).
		 * By default this is ignored since default state of glDrawBuffers is NONE for indices >= 1.
		 * But if you attach a viewport-size floating-point texture to a GLFrameBufferObject,
		 * and call GLFrameBufferObject::gl_draw_buffers(), then depth will get written to the
		 * texture for those pixels on the rendered iso-surface (or cross-section).
		 */
		void
		render(
				GLRenderer &renderer,
				cache_handle_type &cache_handle,
				boost::optional<GLTexture::shared_ptr_to_const_type> surface_occlusion_texture = boost::none,
				boost::optional<GLTexture::shared_ptr_to_const_type> depth_read_texture = boost::none);

	private:

		/**
		 * The version of GLSL shading language to use in our shaders.
		 *
		 * We're currently leaving this as the default version (1.2) since, although we use OpenGL 3.0
		 * features (GLSL 1.3), we're not currently sure whether MacOS Snow Leopard officially
		 * supports OpenGL 3.0 (and associated GLSL 1.3) so we use the shader '#extension' mechanism
		 * where possible instead. This works for GL_EXT_texture_array but may need to up the GLSL version
		 * if start using other features that don't have GLSL '#extension's.
		 */
		static const GLShaderObject::ShaderVersion SHADER_VERSION = GLShaderObject::DEFAULT_SHADER_VERSION;

		/**
		 * The resolution of the 1D texture for converting depth radii to layer indices.
		 *
		 * This needs to be reasonably high since non-uniformly spaced depth layers will have
		 * mapping errors within a texel distance of each layer's depth due to the linear filtering,
		 * interpolating between the nearest texel centres surrounding a layer's depth, not being
		 * able to reproduce the non-linear (piecewise linear) mapping in that region.
		 */
		static const unsigned int DEPTH_RADIUS_TO_LAYER_RESOLUTION = 2048;

		/**
		 * The resolution of the 1D texture for converting scalar values (or gradient magnitudes) to colour.
		 *
		 * This needs to be reasonably high since non-uniformly spaced colour slices (in palette) will
		 * have mapping errors within a texel distance of each colour slice boundary due to the linear
		 * filtering, interpolating between the nearest texel centres surrounding a colour slice boundary,
		 * not being able to reproduce the non-linear (piecewise linear) mapping in that region.
		 */
		static const unsigned int COLOUR_PALETTE_RESOLUTION = 2048;


		/**
		 * Used to inform clients that we have been updated.
		 */
		mutable GPlatesUtils::SubjectToken d_subject_token;

		/**
		 * The light (direction) used during lighting.
		 */
		GLLight::non_null_ptr_type d_light;

		//! Keep track of changes to @a d_light.
		mutable GPlatesUtils::ObserverToken d_light_observer_token;

		//! The tile metadata resolution of the current scalar field.
		unsigned int d_tile_meta_data_resolution;

		//! The tile resolution of the current scalar field.
		unsigned int d_tile_resolution;

		//! The number of active tiles of the current scalar field.
		unsigned int d_num_active_tiles;

		//! The number of depth layers of the current scalar field.
		unsigned int d_num_depth_layers;

		//! Minimum depth layer radius (closest to Earth's core).
		float d_min_depth_layer_radius;

		//! Maximum depth layer radius (closest to Earth's surface).
		float d_max_depth_layer_radius;

		//! The radius of each depth layer - ordered from smaller to larger radii.
		std::vector<float> d_depth_layer_radii;

		//! The minimum scalar value across the entire scalar field.
		double d_scalar_min;

		//! The maximum scalar value across the entire scalar field.
		double d_scalar_max;

		//! The mean scalar value across the entire scalar field.
		double d_scalar_mean;

		//! The scalar standard deviation across the entire scalar field.
		double d_scalar_standard_deviation;

		//! The minimum gradient magnitude across the entire scalar field.
		double d_gradient_magnitude_min;

		//! The maximum gradient magnitude across the entire scalar field.
		double d_gradient_magnitude_max;

		//! The mean gradient magnitude across the entire scalar field.
		double d_gradient_magnitude_mean;

		//! The gradient magnitude standard deviation across the entire scalar field.
		double d_gradient_magnitude_standard_deviation;

		/**
		 * Texture array where each texel contains metadata for a tile (tile ID, max/min scalar value).
		 */
		GLTexture::shared_ptr_type d_tile_meta_data_texture_array;

		/**
		 * Texture array containing the field data (scalar value and gradient).
		 */
		GLTexture::shared_ptr_type d_field_data_texture_array;

		/**
		 * Texture array containing the mask data.
		 */
		GLTexture::shared_ptr_type d_mask_data_texture_array;

		/**
		 * 1D texture to map layer depth radii to layer indices (into texture array).
		 */
		GLTexture::shared_ptr_type d_depth_radius_to_layer_texture;

		/**
		 * 1D texture to map scalar values (or gradient magnitudes) to colour.
		 */
		GLTexture::shared_ptr_type d_colour_palette_texture;

		/**
		 * Shader program for rendering an iso-surface.
		 */
		boost::optional<GLProgramObject::shared_ptr_type> d_render_iso_surface_program_object;

		/**
		 * The scalar value to render iso-surface at.
		 */
		double d_current_scalar_iso_surface_value;

		/**
		 * The colour palette used to map scalar values (or gradient magnitudes) to colour.
		 */
		GPlatesGui::ColourPalette<double>::non_null_ptr_to_const_type d_current_colour_palette;


		//! Constructor.
		GLScalarField3D(
				GLRenderer &renderer,
				const QString &scalar_field_filename,
				const GLLight::non_null_ptr_type &light);

		void
		create_shader_programs(
				GLRenderer &renderer);

		boost::optional<GLProgramObject::shared_ptr_type>
		create_shader_program(
				GLRenderer &renderer,
				const QString &vertex_shader_source_file_name,
				const QString &fragment_shader_source_file_name);

		void
		create_tile_meta_data_texture_array(
				GLRenderer &renderer);
		void
		create_field_data_texture_array(
				GLRenderer &renderer);

		void
		create_mask_data_texture_array(
				GLRenderer &renderer);

		void
		create_depth_radius_to_layer_texture(
				GLRenderer &renderer);

		void
		create_colour_palette_texture(
				GLRenderer &renderer);

		void
		load_scalar_field(
				GLRenderer &renderer,
				const GPlatesFileIO::ScalarField3DFileFormat::Reader &scalar_field_reader);

		void
		load_depth_radius_to_layer_texture(
				GLRenderer &renderer);

		void
		load_colour_palette_texture(
				GLRenderer &renderer);

		/**
		 * Render a debug test scalar field into the depth layers of a texture array.
		 */
		void
		load_shader_generated_test_scalar_field(
				GLRenderer &renderer,
				const QString &scalar_field_filename);

		void
		write_scalar_field_to_file(
				GLRenderer &renderer,
				const QString &scalar_field_filename);
	};
}

#endif // GPLATES_OPENGL_GLSCALARFIELD3D_H
