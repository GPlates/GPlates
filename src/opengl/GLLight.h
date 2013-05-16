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

#ifndef GPLATES_OPENGL_GLLIGHT_H
#define GPLATES_OPENGL_GLLIGHT_H

#include <boost/optional.hpp>

#include "GLMatrix.h"
#include "GLProgramObject.h"
#include "GLTexture.h"
#include "GLUtils.h"

#include "gui/MapProjection.h"
#include "gui/SceneLightingParameters.h"

#include "maths/UnitVector3D.h"

#include "utils/ReferenceCount.h"
#include "utils/SubjectObserverToken.h"


namespace GPlatesOpenGL
{
	class GLRenderer;

	/**
	 * A directional light that encodes light direction for both the 3D globe view and the 2D map views.
	 *
	 * For the 3D globe, the light direction is constant in world-space.
	 * But for the map views the light direction is constant in the map space and hence varies across
	 * the globe in world-space (when the light direction projected back onto the globe).
	 * Therefore the light direction is encoded as a function of position-on-sphere by using
	 * a hardware cube map that is indexed by the 3D position-on-sphere to give the 3D light direction
	 * for each rasterised pixel when rendering a raster with lighting.
	 */
	class GLLight :
			public GPlatesUtils::ReferenceCount<GLLight>
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLLight.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLLight> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLLight.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLLight> non_null_ptr_to_const_type;


		/**
		 * Returns true if lighting is supported on the runtime system.
		 *
		 * This requires vertex/fragment shader programs.
		 */
		static
		bool
		is_supported(
				GLRenderer &renderer);


		/**
		 * Creates a @a GLLight object.
		 *
		 * If @a map_projection is specified then the raster is rendered using the specified
		 * 2D map projection, otherwise it's rendered to the 3D globe.
		 *
		 * @param scene_lighting_params are the initial parameters for lighting.
		 * @param view_orientation is the initial orientation of view direction:
		 *        - for the 3D globe view this is the view direction relative to the globe,
		 *        - for the 2D map views this is the 2D view rotation of the 2D map-space about the
		 *          centre of the screen (ignoring translation).
		 * @param map_projection is used to convert the light direction from map-space to globe world-space.
		 */
		static
		non_null_ptr_type
		create(
				GLRenderer &renderer,
				const GPlatesGui::SceneLightingParameters &scene_lighting_params = GPlatesGui::SceneLightingParameters(),
				const GLMatrix &view_orientation = GLMatrix::IDENTITY,
				boost::optional<GPlatesGui::MapProjection::non_null_ptr_to_const_type> map_projection = boost::none);


		/**
		 * Returns a subject token that clients can observe to see if they need to update themselves.
		 *
		 * This includes any change to the scene lighting parameters, any change to the view
		 * orientation (*if* light is attached to view) and any change to map projection.
		 */
		const GPlatesUtils::SubjectToken &
		get_subject_token() const
		{
			return d_subject_token;
		}


		/**
		 * Updates internal state due to changes in these parameters.
		 *
		 * @a view_orientation is the orientation of the view direction relative to the globe
		 * (in 3D globe views) or relative to the unrotated map (in 2D map views).
		 */
		void
		set_scene_lighting(
				GLRenderer &renderer,
				const GPlatesGui::SceneLightingParameters &scene_lighting_params,
				const GLMatrix &view_orientation = GLMatrix::IDENTITY,
				boost::optional<GPlatesGui::MapProjection::non_null_ptr_to_const_type> map_projection = boost::none);


		/**
		 * Returns the scene lighting parameters used by this light.
		 */
		const GPlatesGui::SceneLightingParameters &
		get_scene_lighting_parameters() const
		{
			return d_scene_lighting_params;
		}

		/**
		 * Returns the current view orientation.
		 *
		 * The reverse of this orientation is used to transform light from view-space to
		 * world-space if the light is attached to view-space.
		 */
		const GLMatrix &
		get_view_orientation() const
		{
			return d_view_orientation;
		}

		/**
		 * Returns the map projection if view used for light is a 2D map view (not the 3D globe view).
		 *
		 * If this returns true then use the hardware cube map texture returned by
		 * @a get_map_view_light_direction_cube_map_texture to get the light direction as a function
		 * of position-on-globe.
		 * Otherwise just use the constant light direction specified by @a get_scene_lighting_parameters.
		 */
		boost::optional<GPlatesGui::MapProjection::non_null_ptr_to_const_type>
		get_map_projection() const
		{
			return d_map_projection;
		}


		/**
		 * Returns the *world-space* light direction for use in lighting the 3D globe view.
		 *
		 * This includes conversion of light direction from view-space to world-space if the light
		 * is attached to the view.
		 */
		const GPlatesMaths::UnitVector3D &
		get_globe_view_light_direction(
				GLRenderer &renderer) const
		{
			return d_globe_view_light_direction;
		}


		/**
		 * Returns the ambient and diffuse lighting for the 2D map views when no surface normal mapping
		 * is used (ie, when the surface normal is constant across the map and perpendicular to the map).
		 *
		 * When the surface is normal mapped (ie, the surface normals vary across the map) then use
		 * @a get_map_view_light_direction_cube_map_texture to obtain the varying light direction
		 * in spherical globe space (the light.
		 */
		float
		get_map_view_constant_lighting(
				GLRenderer &renderer) const
		{
			return d_map_view_constant_lighting;
		}


		/**
		 * Returns the hardware cube map texture containing the *world-space* light direction(s) for
		 * the current 2D map view (with map projection specified in @a set_scene_lighting).
		 *
		 * The returned texture format is 8-bit RGBA with RGB containing the light direction(s)
		 * with components in the range [0,1] - which clients need to convert to [-1,1] before use.
		 *
		 * @a renderer is used if the cube map needs to be updated such as an updated light direction.
		 *
		 * NOTE: This is only really needed when surface normal maps are used because the surface
		 * normal (in the map view) is then no longer constant across the map.
		 * When it is constant across the map (ie, surface normal is perpendicular to the map) the
		 * lighting is constant across the map and can be calculated using @a get_map_view_constant_lighting.
		 *
		 * NOTE: You should use GL_TEXTURE_CUBE_MAP_ARB instead of GL_TEXTURE_2D when binding the
		 * returned texture for read access.
		 */
		GLTexture::shared_ptr_to_const_type
		get_map_view_light_direction_cube_map_texture(
				GLRenderer &renderer)
		{
			return d_map_view_light_direction_cube_texture;
		}

	private:

		/**
		 * Used to inform clients that we have been updated.
		 */
		mutable GPlatesUtils::SubjectToken d_subject_token;

		/**
		 * The parameters used to surface light the reconstructed raster.
		 */
		GPlatesGui::SceneLightingParameters d_scene_lighting_params;

		/**
		 * This is the orientation of the view direction relative to the globe (in 3D globe views)
		 * or relative to the unrotated map (in 2D map views).
		 *
		 * The reverse of this transform is used to convert light direction from view-space to world-space.
		 */
		GLMatrix d_view_orientation;

		/**
		 * The world-space light direction for the 3D globe view (includes conversion from view-space).
		 */
		GPlatesMaths::UnitVector3D d_globe_view_light_direction;

		/**
		 * The ambient+diffuse lighting for the 2D map views (includes conversion from view-space) when
		 * the normal mapping is *not* used (ie, surface is constant across map and perpendicular to map).
		 */
		float d_map_view_constant_lighting;

		/**
		 * The map projection if the light direction is (constant) in 2D map-space.
		 */
		boost::optional<GPlatesGui::MapProjection::non_null_ptr_to_const_type> d_map_projection;

		/**
		 * The dimension of the square faces of the light direction cube texture (for the 2D map views).
		 */
		unsigned int d_map_view_light_direction_cube_texture_dimension;

		/**
		 * The hardware cube map encoding the light direction(s) for a 2D map view.
		 */
		GLTexture::shared_ptr_type d_map_view_light_direction_cube_texture;

		/**
		 * Shader program to render light direction into cube texture for 2D map views.
		 */
		boost::optional<GLProgramObject::shared_ptr_type> d_render_map_view_light_direction_program_object;


		GLLight(
				GLRenderer &renderer,
				const GPlatesGui::SceneLightingParameters &scene_lighting_params,
				const GLMatrix &view_orientation,
				boost::optional<GPlatesGui::MapProjection::non_null_ptr_to_const_type> map_projection);

		void
		create_shader_programs(
				GLRenderer &renderer);

		void
		create_map_view_light_direction_cube_texture(
				GLRenderer &renderer);

		void
		update_map_view(
				GLRenderer &renderer);

		void
		update_globe_view(
				GLRenderer &renderer);
	};
}

#endif // GPLATES_OPENGL_GLLIGHT_H
