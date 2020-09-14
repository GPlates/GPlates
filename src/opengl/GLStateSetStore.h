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
	class GLContext;

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
		GPlatesUtils::ObjectPool<GLBindBufferStateSet> bind_buffer_state_sets;
		GPlatesUtils::ObjectPool<GLBindFramebufferStateSet> bind_framebuffer_state_sets;
		GPlatesUtils::ObjectPool<GLBindRenderbufferStateSet> bind_renderbuffer_state_sets;
		GPlatesUtils::ObjectPool<GLBindSamplerStateSet> bind_sampler_state_sets;
		GPlatesUtils::ObjectPool<GLBindTextureStateSet> bind_texture_state_sets;
		GPlatesUtils::ObjectPool<GLBindVertexArrayStateSet> bind_vertex_array_state_sets;
		GPlatesUtils::ObjectPool<GLBlendColorStateSet> blend_color_state_sets;
		GPlatesUtils::ObjectPool<GLBlendEquationStateSet> blend_equation_state_sets;
		GPlatesUtils::ObjectPool<GLBlendFuncStateSet> blend_func_state_sets;
		GPlatesUtils::ObjectPool<GLClampColorStateSet> clamp_color_state_sets;
		GPlatesUtils::ObjectPool<GLClearColorStateSet> clear_color_state_sets;
		GPlatesUtils::ObjectPool<GLClearDepthStateSet> clear_depth_state_sets;
		GPlatesUtils::ObjectPool<GLClearStencilStateSet> clear_stencil_state_sets;
		GPlatesUtils::ObjectPool<GLColorMaskStateSet> color_mask_state_sets;
		GPlatesUtils::ObjectPool<GLCullFaceStateSet> cull_face_state_sets;
		GPlatesUtils::ObjectPool<GLDepthFuncStateSet> depth_func_state_sets;
		GPlatesUtils::ObjectPool<GLDepthMaskStateSet> depth_mask_state_sets;
		GPlatesUtils::ObjectPool<GLDepthRangeStateSet> depth_range_state_sets;
		GPlatesUtils::ObjectPool<GLDrawBuffersStateSet> draw_buffers_state_sets;
		GPlatesUtils::ObjectPool<GLEnableStateSet> enable_state_sets;
		GPlatesUtils::ObjectPool<GLEnableIndexedStateSet> enable_indexed_state_sets;
		GPlatesUtils::ObjectPool<GLFrontFaceStateSet> front_face_state_sets;
		GPlatesUtils::ObjectPool<GLHintStateSet> hint_state_sets;
		GPlatesUtils::ObjectPool<GLLineWidthStateSet> line_width_state_sets;
		GPlatesUtils::ObjectPool<GLPixelStoreStateSet> pixel_store_state_sets;
		GPlatesUtils::ObjectPool<GLPointSizeStateSet> point_size_state_sets;
		GPlatesUtils::ObjectPool<GLPolygonModeStateSet> polygon_mode_state_sets;
		GPlatesUtils::ObjectPool<GLPolygonOffsetStateSet> polygon_offset_state_sets;
		GPlatesUtils::ObjectPool<GLPrimitiveRestartIndexStateSet> primitive_restart_index_state_sets;
		GPlatesUtils::ObjectPool<GLReadBufferStateSet> read_buffer_state_sets;
		GPlatesUtils::ObjectPool<GLSampleCoverageStateSet> sample_coverage_state_sets;
		GPlatesUtils::ObjectPool<GLSampleMaskStateSet> sample_mask_state_sets;
		GPlatesUtils::ObjectPool<GLScissorStateSet> scissor_state_sets;
		GPlatesUtils::ObjectPool<GLStencilFuncStateSet> stencil_func_state_sets;
		GPlatesUtils::ObjectPool<GLStencilMaskStateSet> stencil_mask_state_sets;
		GPlatesUtils::ObjectPool<GLStencilOpStateSet> stencil_op_state_sets;
		GPlatesUtils::ObjectPool<GLUseProgramStateSet> use_program_state_sets;
		GPlatesUtils::ObjectPool<GLViewportStateSet> viewport_state_sets;

	private:
		//! Constructor.
		GLStateSetStore()
		{  }
	};
}

#endif // GPLATES_OPENGL_GLSTATESETSTORE_H
