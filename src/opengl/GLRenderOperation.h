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
 
#ifndef GPLATES_OPENGL_GLRENDEROPERATION_H
#define GPLATES_OPENGL_GLRENDEROPERATION_H

#include "GLDrawable.h"
#include "GLTransform.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	/**
	 * Associates a drawable with enough information (such as OpenGL state, matrices)
	 * to render the drawable.
	 */
	class GLRenderOperation :
			public GPlatesUtils::ReferenceCount<GLRenderOperation>
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLRenderOperation.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLRenderOperation> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLRenderOperation.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLRenderOperation> non_null_ptr_to_const_type;


		/**
		 * Creates a @a GLRenderOperation object.
		 */
		static
		non_null_ptr_type
		create(
				const GLDrawable::non_null_ptr_to_const_type &drawable,
				const GLTransform::non_null_ptr_to_const_type &model_view_matrix,
				const GLTransform::non_null_ptr_to_const_type &projection_matrix)
		{
			return non_null_ptr_type(new GLRenderOperation(
					drawable, model_view_matrix, projection_matrix));
		}

		const GLDrawable::non_null_ptr_to_const_type &
		get_drawable() const
		{
			return d_drawable;
		}

		const GLTransform::non_null_ptr_to_const_type &
		get_model_view_matrix() const
		{
			return d_model_view_matrix;
		}

		const GLTransform::non_null_ptr_to_const_type &
		get_projection_matrix() const
		{
			return d_projection_matrix;
		}

	private:
		GLDrawable::non_null_ptr_to_const_type d_drawable;
		GLTransform::non_null_ptr_to_const_type d_model_view_matrix;
		GLTransform::non_null_ptr_to_const_type d_projection_matrix;


		//! Constructor.
		GLRenderOperation(
				const GLDrawable::non_null_ptr_to_const_type &drawable,
				const GLTransform::non_null_ptr_to_const_type &model_view_matrix,
				const GLTransform::non_null_ptr_to_const_type &projection_matrix) :
			d_drawable(drawable),
			d_model_view_matrix(model_view_matrix),
			d_projection_matrix(projection_matrix)
		{  }
	};
}


#endif // GPLATES_OPENGL_GLRENDEROPERATION_H
