/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#ifndef GPLATES_OPENGL_GLSTATESETSTORE_H
#define GPLATES_OPENGL_GLSTATESETSTORE_H

#include "GLStateSets.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ObjectPool.h"
#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	/**
	 * Manages allocation of derived @a GLStateSet classes using a separate object pool for each type.
	 */
	class GLStateSetStore :
			public GPlatesUtils::ReferenceCount<GLStateSetStore>
	{
	public:
		typedef GPlatesUtils::non_null_intrusive_ptr<GLStateSetStore> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLStateSetStore> non_null_ptr_to_const_type;


		/**
		 * Creates a @a GLStateSetStore object.
		 */
		static
		non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new GLStateSetStore());
		}

		//
		// The @a GLStateSet object pools - one for each derived type.
		//

		// Alphabetically ordered...
		GPlatesUtils::ObjectPool<GLActiveTextureStateSet> active_texture_state_sets;
		GPlatesUtils::ObjectPool<GLAlphaFuncStateSet> alpha_func_state_sets;
		GPlatesUtils::ObjectPool<GLBindBufferObjectStateSet> bind_buffer_object_state_sets;
		GPlatesUtils::ObjectPool<GLBindFrameBufferObjectStateSet> bind_frame_buffer_object_state_sets;
		GPlatesUtils::ObjectPool<GLBindProgramObjectStateSet> bind_program_object_state_sets;
		GPlatesUtils::ObjectPool<GLBindTextureStateSet> bind_texture_state_sets;
		GPlatesUtils::ObjectPool<GLBindVertexArrayObjectStateSet> bind_vertex_array_object_state_sets;
		GPlatesUtils::ObjectPool<GLBlendFuncStateSet> blend_func_state_sets;
		GPlatesUtils::ObjectPool<GLClearColorStateSet> clear_color_state_sets;
		GPlatesUtils::ObjectPool<GLClearDepthStateSet> clear_depth_state_sets;
		GPlatesUtils::ObjectPool<GLClearStencilStateSet> clear_stencil_state_sets;
		GPlatesUtils::ObjectPool<GLClientActiveTextureStateSet> client_active_texture_state_sets;
		GPlatesUtils::ObjectPool<GLColorMaskStateSet> color_mask_state_sets;
		GPlatesUtils::ObjectPool<GLColorPointerStateSet> color_pointer_state_sets;
		GPlatesUtils::ObjectPool<GLCullFaceStateSet> cull_face_state_sets;
		GPlatesUtils::ObjectPool<GLDepthFuncStateSet> depth_func_state_sets;
		GPlatesUtils::ObjectPool<GLDepthMaskStateSet> depth_mask_state_sets;
		GPlatesUtils::ObjectPool<GLDepthRangeStateSet> depth_range_state_sets;
		GPlatesUtils::ObjectPool<GLEnableClientStateStateSet> enable_client_state_state_sets;
		GPlatesUtils::ObjectPool<GLEnableClientTextureStateStateSet> enable_client_texture_state_state_sets;
		GPlatesUtils::ObjectPool<GLEnableStateSet> enable_state_sets;
		GPlatesUtils::ObjectPool<GLEnableTextureStateSet> enable_texture_state_sets;
		GPlatesUtils::ObjectPool<GLEnableVertexAttribArrayStateSet> enable_vertex_attrib_array_state_sets;
		GPlatesUtils::ObjectPool<GLFrontFaceStateSet> front_face_state_sets;
		GPlatesUtils::ObjectPool<GLHintStateSet> hint_state_sets;
		GPlatesUtils::ObjectPool<GLLineWidthStateSet> line_width_state_sets;
		GPlatesUtils::ObjectPool<GLLoadMatrixStateSet> load_matrix_state_sets;
		GPlatesUtils::ObjectPool<GLLoadTextureMatrixStateSet> load_texture_matrix_state_sets;
		GPlatesUtils::ObjectPool<GLMatrixModeStateSet> matrix_mode_state_sets;
		GPlatesUtils::ObjectPool<GLPointSizeStateSet> point_size_state_sets;
		GPlatesUtils::ObjectPool<GLPolygonModeStateSet> polygon_mode_state_sets;
		GPlatesUtils::ObjectPool<GLPolygonOffsetStateSet> polygon_offset_state_sets;
		GPlatesUtils::ObjectPool<GLNormalPointerStateSet> normal_pointer_state_sets;
		GPlatesUtils::ObjectPool<GLScissorStateSet> scissor_state_sets;
		GPlatesUtils::ObjectPool<GLStencilMaskStateSet> stencil_mask_state_sets;
		GPlatesUtils::ObjectPool<GLTexCoordPointerStateSet> tex_coord_pointer_state_sets;
		GPlatesUtils::ObjectPool<GLTexGenStateSet> tex_gen_state_sets;
		GPlatesUtils::ObjectPool<GLTexEnvStateSet> tex_env_state_sets;
		GPlatesUtils::ObjectPool<GLVertexAttribPointerStateSet> vertex_attrib_array_state_sets;
		GPlatesUtils::ObjectPool<GLVertexPointerStateSet> vertex_pointer_state_sets;
		GPlatesUtils::ObjectPool<GLViewportStateSet> viewport_state_sets;

	private:
		//! Constructor.
		GLStateSetStore()
		{  }
	};
}

#endif // GPLATES_OPENGL_GLSTATESETSTORE_H
