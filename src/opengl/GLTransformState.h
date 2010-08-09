/* $Id$ */


/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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
 
#ifndef GPLATES_OPENGL_GLTRANSFORMSTATE_H
#define GPLATES_OPENGL_GLTRANSFORMSTATE_H

#include <stack>
#include <boost/optional.hpp>
#include <opengl/OpenGL.h>

#include "GLIntersectPrimitives.h"
#include "GLMatrix.h"
#include "GLViewport.h"

#include "maths/Vector3D.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	class GLTransform;

	/**
	 * Shadow the OpenGL transform state so that we can query
	 * the frustum clip planes in model space (for visibility culling) and
	 * the projected size of objects in screen space (used for level-of-detail selection).
	 *
	 * Any OpenGL function that can affect the view frustum
	 * - and even the windows coordinates (such as glViewport) -
	 * is shadowed here.
	 * Note: glDepthRange is absent as it only affects the mapping of normalized device coordinate
	 * z to fixed-point depth buffer (eg, mapping to a 24-bit depth buffer).
	 */
	class GLTransformState :
			public GPlatesUtils::ReferenceCount<GLTransformState>
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLTransformState.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLTransformState> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLTransformState.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLTransformState> non_null_ptr_to_const_type;


		/**
		 * Creates a @a GLTransformState object.
		 */
		static
		non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new GLTransformState());
		}


		/**
		 * Sets the current matrix mode.
		 *
		 * NOTE: This does not call OpenGL directly - just provides a familiar interface.
		 *
		 * @a mode must be one of GL_MODELVIEW or GL_PROJECTION.
		 *
		 * NOTE: GL_TEXTURE is *not* included here because:
		 * - it is bound to the currently active texture unit unlike GL_MODELVIEW and GL_PROJECTION,
		 * - it does not normally follow a hierarchy of transformations like GL_MODELVIEW tends to,
		 * - it is infrequently used when rendering drawables.
		 * So for these reasons it is better to set it in a @a GLStateSet by doing the following
		 * in GLStateSet::enter_state_set():
		 * - call glMatrixMode(GL_TEXTURE),
		 * - explicitly set the active texture unit to the desired texture unit number,
		 * - call glLoadMatrix() to load the texture matrix,
		 * and doing the following in GLStateSet::leave_state_set():
		 * - call glMatrixMode(GL_MODELVIEW) // to restore the default matrix mode
		 */
		void
		gl_matrix_mode(
				GLenum mode);


		/**
		 * Performs function of similarly named OpenGL function.
		 *
		 * NOTE: This does not call OpenGL directly - just provides a familiar interface.
		 */
		void
		gl_push_matrix();


		/**
		 * Performs function of similarly named OpenGL function.
		 *
		 * NOTE: This does not call OpenGL directly - just provides a familiar interface.
		 */
		void
		gl_pop_matrix();


		/**
		 * Performs function of similarly named OpenGL function.
		 *
		 * NOTE: This does not call OpenGL directly - just provides a familiar interface.
		 */
		void
		gl_load_identity();


		/**
		 * Performs function of similarly named OpenGL function.
		 *
		 * NOTE: This does not call OpenGL directly - just provides a familiar interface.
		 */
		void
		gl_load_matrix(
				const GLMatrix &matrix);


		/**
		 * Performs function of similarly named OpenGL function (including post-multiplication).
		 *
		 * NOTE: This does not call OpenGL directly - just provides a familiar interface.
		 */
		void
		gl_mult_matrix(
				const GLMatrix &matrix);


		/**
		 * Performs function of similarly named OpenGL function.
		 *
		 * NOTE: This does not call OpenGL directly - just provides a familiar interface.
		 *
		 * NOTE: This method must be called at least once before some other methods can be called
		 * (such as @a glu_project, @a glu_un_project and @a get_min_pixel_size_on_unit_sphere).
		 */
		void
		gl_viewport(
				const GLViewport &viewport);


		/**
		 * Returns the matrix currently at the top of the GL_MODELVIEW stack.
		 *
		 * This is the equivalent of calling:
		 *   glGetDoublev(GL_MODELVIEW_MATRIX, matrix);
		 * ...in OpenGL.
		 */
		GLMatrix::non_null_ptr_to_const_type
		get_current_model_view_matrix() const;

		/**
		 * Returns the matrix currently at the top of the GL_PROJECTION stack.
		 *
		 * This is the equivalent of calling:
		 *   glGetDoublev(GL_PROJECTION_MATRIX, matrix);
		 * ...in OpenGL.
		 */
		GLMatrix::non_null_ptr_to_const_type
		get_current_projection_matrix() const;


		/**
		 * Returns the current viewport or false if @a gl_viewport has not been called yet.
		 *
		 * This is the equivalent of calling:
		 * glGetIntegerv(GL_VIEWPORT, viewport);
		 */
		boost::optional<GLViewport>
		get_current_viewport() const;


		/**
		 * Convenience function performs same as similarly named OpenGL function
		 * except you don't need to perform a (potentially costly round-trip) retrieval
		 * of the current model-view and projection matrices (since they are shadowed).
		 *
		 * @throws PreconditionViolationError if @a gl_viewport has never been called
		 *         on 'this' object.
		 */
		int
		glu_project(
				double objx,
				double objy,
				double objz,
				GLdouble *winx,
				GLdouble *winy,
				GLdouble *winz) const;

		/**
		 * Convenience function performs same as similarly named OpenGL function
		 * except you don't need to perform a (potentially costly round-trip) retrieval
		 * of the current model-view and projection matrices (since they are shadowed).
		 *
		 * @throws PreconditionViolationError if @a gl_viewport has never been called
		 *         on 'this' object.
		 */
		int
		glu_un_project(
				double winx,
				double winy,
				double winz,
				GLdouble *objx,
				GLdouble *objy,
				GLdouble *objz) const;


		/**
		 * Returns an estimate of the minimum size of a viewport pixel when projected onto
		 * the unit sphere using the current model-view-projection transform.
		 *
		 * This assumes the globe is a sphere of radius one centred at the origin in model space.
		 *
		 * Currently this is done by sampling the corners of the view frustum and the middle
		 * of each of the four sides of the view frustum and the centre.
		 *
		 * This method is reasonably expensive but should be fine since it's only
		 * called once per raster per render scene.
		 *
		 * Returned result is in the range (0, Pi] where Pi is the distance between north and
		 * south poles on the unit sphere.
		 *
		 * @throws PreconditionViolationError if @a gl_viewport has never been called
		 *         on 'this' object.
		 */
		double
		get_min_pixel_size_on_unit_sphere() const;


		/**
		 * An array of the six frustum planes that bound the viewing volume.
		 */
		struct FrustumPlanes
		{
			/**
			 * The left, right, bottom, top, near and far frustum planes.
			 *
			 * NOTE: The plane normals point towards the *inside* of the view frustum
			 * volume and hence the view frustum is defined by the intersection of the
			 * positive half-spaces of these planes.
			 *
			 * NOTE: These planes do *not* have *unit* vector normals.
			 */
			GLIntersect::Plane planes[6];
		};

		/**
		 * Returns the *six* frustum planes represented by the current model-view and
		 * projection matrices.
		 *
		 * These frustum planes are in model-space (before any model-view or projection
		 * transformations are applied) also called object-space.
		 *
		 * The returned planes can be used for frustum culling (culling objects not
		 * visible inside the current view frustum).
		 */
		const FrustumPlanes &
		get_current_frustum_planes_in_model_space() const;

	private:
		//! Typedef for a stack of matrices.
		typedef std::stack<GLMatrix::non_null_ptr_type> matrix_stack_type;


		GLenum d_current_matrix_mode;

		matrix_stack_type d_model_view_matrix_stack;
		matrix_stack_type d_projection_matrix_stack;

		/**
		 * Points to the current matrix stack as determined by the current matrix mode.
		 */
		matrix_stack_type *d_current_matrix_stack;

		/**
		 * The most recent call to @a gl_viewport sets this otherwise it's undefined.
		 */
		boost::optional<GLViewport> d_current_viewport;

		/**
		 * The frustum planes of the current model-view and projection matrices.
		 */
		mutable FrustumPlanes d_current_frustum_planes;

		/**
		 * Whether @a d_current_frustum_planes is valid for the current model-view and
		 * projection matrices.
		 */
		mutable bool d_current_frustum_planes_valid;


		/**
		 * Constructor.
		 *
		 * The initial state matches the default OpenGL transform state
		 * (that is, the state when an OpenGL context is first created).
		 *
		 * For example, the initial matrix mode is GL_MODELVIEW.
		 *
		 * NOTE: The initial shadowed viewport is undefined until it is explicitly set
		 * with the @a gl_viewport method. Methods of this class that require the viewport
		 * to be defined will throw the @a PreconditionViolationError exception if it's
		 * not defined.
		 * @a gl_viewport should really be the first thing called anyway, for example,
		 * by creating a @a GLViewportNode at the top of the render graph.
		 */
		GLTransformState();


		/**
		 * Projects a windows coordinate onto the unit sphere in model space
		 * using the current model-view-projection transform and the current viewport.
		 *
		 * The returned vector is the intersection of the window coordinate (screen pixel)
		 * projected onto the unit sphere, or returns false if misses the globe.
		 */
		boost::optional<GPlatesMaths::Vector3D>
		project_window_coords_onto_unit_sphere(
				const double &window_x,
				const double &window_y) const;

		/**
		 * Returns frustum planes corresponding to identity model-view and projection matrices.
		 */
		static
		FrustumPlanes
		initialise_frustum_planes();
	};
}

#endif // GPLATES_OPENGL_GLTRANSFORMSTATE_H
