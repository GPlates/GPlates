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
 
#ifndef GPLATES_OPENGL_GLTEXTURETRANSFORMSTATE_H
#define GPLATES_OPENGL_GLTEXTURETRANSFORMSTATE_H

#include <boost/optional.hpp>
#include <opengl/OpenGL.h>

#include "GLMatrix.h"
#include "GLStateSet.h"


namespace GPlatesOpenGL
{
	/**
	 * Sets texture transform state such as glTexGen and the texture coordinate matrix.
	 */
	class GLTextureTransformState :
			public GLStateSet
	{
	public:
		typedef GPlatesUtils::non_null_intrusive_ptr<GLTextureTransformState> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLTextureTransformState> non_null_ptr_to_const_type;


		/**
		 * Creates a @a GLTextureTransformState object with no state.
		 *
		 * Call @a gl_enable_texture_2D, etc to initialise the state.
		 * For example:
		 *   blend_state->gl_enable_texture_2D(GL_TRUE);
		 */
		static
		non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new GLTextureTransformState());
		}


		/**
		 * Selects the current texture unit that the other 'gl_...' calls apply to.
		 *
		 * Like the other 'gl*()' methods in this class the same-named call to OpenGL
		 * is not made here (it is delayed until @a enter_state_set is called).
		 *
		 * The default texture unit is texture unit 0, in which case it is not necessary
		 * to call this method. The default is texture unit 0 regardless of the currently
		 * active unit for some other @a GLTextureTransformState object.
		 *
		 * If the runtime system doesn't support the GL_ARB_multitexture extension
		 * (and hence only supports one texture unit) then it is not necessary
		 * to call this function.
		 *
		 * If doesn't matter if you call this before or after the other 'gl_...' methods.
		 */
		void
		gl_active_texture_ARB(
				GLenum texture);


		/**
		 * Used to set state for a particular texture coordinate (GL_S, GL_T, GL_R or GL_Q).
		 */
		class TexGenCoordState
		{
		public:
			TexGenCoordState &
			gl_enable_texture_gen(
					GLboolean enable)
			{
				d_enable_texture_gen = enable;
				return *this;
			}

			TexGenCoordState &
			gl_tex_gen_mode(
					GLint param = GL_EYE_LINEAR)
			{
				d_tex_gen_mode = param;
				return *this;
			}

			TexGenCoordState &
			gl_object_plane(
					const GLdouble *param)
			{
				const Plane object_plane = { { { param[0], param[1], param[2], param[3] } } };
				d_object_plane = object_plane;
				return *this;
			}

			TexGenCoordState &
			gl_eye_plane(
					const GLdouble *param)
			{
				const Plane eye_plane = { { { param[0], param[1], param[2], param[3] } } };
				d_eye_plane = eye_plane;
				return *this;
			}

		private:
			struct Plane
			{
				union
				{
					GLdouble plane[4];

					struct
					{
						GLdouble x, y, z, w;
					};
				};
			};

			boost::optional<GLboolean> d_enable_texture_gen;
			boost::optional<GLint> d_tex_gen_mode;
			boost::optional<Plane> d_object_plane;
			boost::optional<Plane> d_eye_plane;

			friend class GLTextureTransformState;
		};


		/**
		 * Sets the texture generation state for a specific coordinate.
		 *
		 * @a tex_gen_coord_state must be one of GL_S, GL_T, GL_R, or GL_Q.
		 */
		GLTextureTransformState &
		set_tex_gen_coord_state(
				GLenum coord,
				const TexGenCoordState &tex_gen_coord_state);


		/**
		 * Sets the texture matrix (see GL_TEXTURE).
		 *
		 * This is done here instead of pushing/popping as is done
		 * in @a GLTransformState because there's a separate texture matrix
		 * for each texture unit and it seems more useful here anyway.
		 *
		 * NOTE: When leaving this state set the texture matrix will get
		 * set back to the identity matrix.
		 *
		 * NOTE: After entering and after leaving the matrix mode will be set
		 * to GL_MODELVIEW.
		 */
		GLTextureTransformState &
		gl_load_matrix(
				const GLMatrix &texture_matrix)
		{
			d_texture_matrix = texture_matrix;
			return *this;
		}


		virtual
		void
		enter_state_set() const;


		virtual
		void
		leave_state_set() const;

	private:
		//! The texture unit we are setting state for.
		GLenum d_active_texture_ARB;

		boost::optional<TexGenCoordState> d_tex_gen_s;
		boost::optional<TexGenCoordState> d_tex_gen_t;
		boost::optional<TexGenCoordState> d_tex_gen_r;
		boost::optional<TexGenCoordState> d_tex_gen_q;

		boost::optional<GLMatrix> d_texture_matrix;

		//! Constructor.
		GLTextureTransformState();


		void
		enter_tex_gen_state(
				GLenum coord,
				const TexGenCoordState &tex_gen_coord_state) const;

		void
		leave_tex_gen_state(
				GLenum coord,
				const TexGenCoordState &tex_gen_coord_state) const;
	};
}

#endif // GPLATES_OPENGL_GLTEXTURETRANSFORMSTATE_H
