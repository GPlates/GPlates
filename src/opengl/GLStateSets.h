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

#ifndef GPLATES_OPENGL_GLSTATESETS_H
#define GPLATES_OPENGL_GLSTATESETS_H

#include <vector>
#include <boost/mpl/assert.hpp>
#include <boost/mpl/contains.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/optional.hpp>
#include <boost/shared_array.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/variant.hpp>
#include <opengl/OpenGL1.h>

#include "GLBuffer.h"
#include "GLDepthRange.h"
#include "GLFrameBufferObject.h"
#include "GLMatrix.h"
#include "GLProgramObject.h"
#include "GLStateSet.h"
#include "GLTexture.h"
#include "GLVertexArray.h"
#include "GLViewport.h"

#include "global/CompilerWarnings.h"

#include "maths/types.h"

// BOOST_MPL_ASSERT contains old-style casts.
DISABLE_GCC_WARNING("-Wold-style-cast")


namespace GPlatesOpenGL
{
	class GLCapabilities;

	//
	// NOTE: These classes are ordered alphabetically.
	//


	/**
	 * Used to set the active texture unit.
	 */
	struct GLActiveTextureStateSet :
			public GLStateSet
	{
		explicit
		GLActiveTextureStateSet(
				const GLCapabilities &capabilities,
				GLenum active_texture);

		virtual
		void
		apply_state(
				const GLCapabilities &capabilities,
				const GLStateSet &current_state_set,
				const GLState &current_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				const GLState &current_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				const GLState &current_state) const;


		GLenum d_active_texture;
	};

	/**
	 * Used to bind a buffer object.
	 */
	struct GLBindBufferStateSet :
			public GLStateSet
	{
		//! Binds a buffer object.
		GLBindBufferStateSet(
				GLenum target,
				boost::optional<GLBuffer::shared_ptr_type> buffer) :
			d_target(target),
			d_buffer(buffer),
			d_buffer_resource(0)
		{
			if (buffer)
			{
				d_buffer_resource = buffer.get()->get_resource_handle();
			}
		}

		virtual
		void
		apply_state(
				const GLCapabilities &capabilities,
				const GLStateSet &current_state_set,
				const GLState &current_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				const GLState &current_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				const GLState &current_state) const;


		GLenum d_target;
		boost::optional<GLBuffer::shared_ptr_type> d_buffer;
		GLuint d_buffer_resource;
	};

	/**
	 * Used to bind a framebuffer object.
	 */
	struct GLBindFrameBufferObjectStateSet :
			public GLStateSet
	{
		/**
		 * Binds a framebuffer object, or unbinds (if @a frame_buffer_object is boost::none).
		 */
		explicit
		GLBindFrameBufferObjectStateSet(
				boost::optional<GLFrameBufferObject::shared_ptr_to_const_type> frame_buffer_object) :
			d_frame_buffer_object(frame_buffer_object)
		{  }

		virtual
		void
		apply_state(
				const GLCapabilities &capabilities,
				const GLStateSet &current_state_set,
				const GLState &current_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				const GLState &current_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				const GLState &current_state) const;


		boost::optional<GLFrameBufferObject::shared_ptr_to_const_type> d_frame_buffer_object;
	};

	/**
	 * Used to bind a shader program object.
	 */
	struct GLBindProgramObjectStateSet :
			public GLStateSet
	{
		/**
		 * Binds a shader program object, or unbinds (if @a program_object is boost::none).
		 */
		explicit
		GLBindProgramObjectStateSet(
				boost::optional<GLProgramObject::shared_ptr_to_const_type> program_object) :
			d_program_object(program_object)
		{  }

		virtual
		void
		apply_state(
				const GLCapabilities &capabilities,
				const GLStateSet &current_state_set,
				const GLState &current_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				const GLState &current_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				const GLState &current_state) const;


		boost::optional<GLProgramObject::shared_ptr_to_const_type> d_program_object;
	};

	/**
	 * Used to bind a texture to a texture unit.
	 */
	struct GLBindTextureStateSet :
			public GLStateSet
	{
		//! Binds a texture object.
		GLBindTextureStateSet(
				const GLCapabilities &capabilities,
				GLenum texture_target,
				GLenum texture_unit,
				boost::optional<GLTexture::shared_ptr_type> texture_object);

		virtual
		void
		apply_state(
				const GLCapabilities &capabilities,
				const GLStateSet &current_state_set,
				const GLState &current_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				const GLState &current_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				const GLState &current_state) const;


		GLenum d_texture_target;
		GLenum d_texture_unit;
		boost::optional<GLTexture::shared_ptr_type> d_texture_object;
	};

	/**
	 * Used to bind a vertex array object.
	 */
	struct GLBindVertexArrayStateSet :
			public GLStateSet
	{
		//! Binds a vertex array object (none and 0 mean unbind).
		GLBindVertexArrayStateSet(
				boost::optional<GLVertexArray::shared_ptr_type> array,
				// Array resource handle associated with the current OpenGL context...
				GLuint array_resource) :
			d_array(array),
			d_array_resource(array_resource)
		{  }

		virtual
		void
		apply_state(
				const GLCapabilities &capabilities,
				const GLStateSet &current_state_set,
				const GLState &current_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				const GLState &current_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				const GLState &current_state) const;


		boost::optional<GLVertexArray::shared_ptr_type> d_array;
		//! Array resource handle associated with the current OpenGL context.
		GLuint d_array_resource;
	};

	/**
	 * Used to set the alpha blend equation.
	 */
	struct GLBlendEquationStateSet :
			public GLStateSet
	{
		explicit
		GLBlendEquationStateSet(
				const GLCapabilities &capabilities,
				GLenum mode);

		GLBlendEquationStateSet(
				const GLCapabilities &capabilities,
				GLenum modeRGB,
				GLenum modeAlpha);

		virtual
		void
		apply_state(
				const GLCapabilities &capabilities,
				const GLStateSet &current_state_set,
				const GLState &current_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				const GLState &current_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				const GLState &current_state) const;


		GLenum d_mode_RGB;
		GLenum d_mode_A;

		//! If the RGB and A components have separate equations.
		bool d_separate_equations;
	};

	/**
	 * Used to set the alpha blend function.
	 */
	struct GLBlendFuncStateSet :
			public GLStateSet
	{
		GLBlendFuncStateSet(
				GLenum sfactor,
				GLenum dfactor) :
			d_src_factor_RGB(sfactor),
			d_dst_factor_RGB(dfactor),
			d_src_factor_A(sfactor),
			d_dst_factor_A(dfactor),
			d_separate_factors(false)
		{  }

		GLBlendFuncStateSet(
				const GLCapabilities &capabilities,
				GLenum sfactorRGB,
				GLenum dfactorRGB,
				GLenum sfactorAlpha,
				GLenum dfactorAlpha);

		virtual
		void
		apply_state(
				const GLCapabilities &capabilities,
				const GLStateSet &current_state_set,
				const GLState &current_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				const GLState &current_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				const GLState &current_state) const;


		GLenum d_src_factor_RGB;
		GLenum d_dst_factor_RGB;

		GLenum d_src_factor_A;
		GLenum d_dst_factor_A;

		//! If the RGB and A components have separate factors.
		bool d_separate_factors;
	};

	/**
	 * Used to set the clear color.
	 */
	struct GLClearColorStateSet :
			public GLStateSet
	{
		GLClearColorStateSet(
				GLclampf red,
				GLclampf green,
				GLclampf blue,
				GLclampf alpha) :
			d_red(red),
			d_green(green),
			d_blue(blue),
			d_alpha(alpha)
		{  }

		virtual
		void
		apply_state(
				const GLCapabilities &capabilities,
				const GLStateSet &current_state_set,
				const GLState &current_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				const GLState &current_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				const GLState &current_state) const;


		GPlatesMaths::real_t d_red;
		GPlatesMaths::real_t d_green;
		GPlatesMaths::real_t d_blue;
		GPlatesMaths::real_t d_alpha;
	};

	/**
	 * Used to set the clear depth.
	 */
	struct GLClearDepthStateSet :
			public GLStateSet
	{
		explicit
		GLClearDepthStateSet(
				GLclampd depth) :
			d_depth(depth)
		{  }

		virtual
		void
		apply_state(
				const GLCapabilities &capabilities,
				const GLStateSet &current_state_set,
				const GLState &current_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				const GLState &current_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				const GLState &current_state) const;


		GPlatesMaths::real_t d_depth;
	};

	/**
	 * Used to set the clear stencil.
	 */
	struct GLClearStencilStateSet :
			public GLStateSet
	{
		explicit
		GLClearStencilStateSet(
				GLint stencil) :
			d_stencil(stencil)
		{  }

		virtual
		void
		apply_state(
				const GLCapabilities &capabilities,
				const GLStateSet &current_state_set,
				const GLState &current_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				const GLState &current_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				const GLState &current_state) const;


		GLint d_stencil;
	};

	/**
	 * Used to set the color mask.
	 */
	struct GLColorMaskStateSet :
			public GLStateSet
	{
		GLColorMaskStateSet(
				GLboolean red,
				GLboolean green,
				GLboolean blue,
				GLboolean alpha) :
			d_red(red),
			d_green(green),
			d_blue(blue),
			d_alpha(alpha)
		{  }

		virtual
		void
		apply_state(
				const GLCapabilities &capabilities,
				const GLStateSet &current_state_set,
				const GLState &current_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				const GLState &current_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				const GLState &current_state) const;


		GLboolean d_red;
		GLboolean d_green;
		GLboolean d_blue;
		GLboolean d_alpha;
	};

	/**
	 * Used glCullFace.
	 */
	struct GLCullFaceStateSet :
			public GLStateSet
	{
		explicit
		GLCullFaceStateSet(
				GLenum mode) :
			d_mode(mode)
		{  }

		virtual
		void
		apply_state(
				const GLCapabilities &capabilities,
				const GLStateSet &current_state_set,
				const GLState &current_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				const GLState &current_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				const GLState &current_state) const;


		GLenum d_mode;
	};

	/**
	 * Used to set the depth test function.
	 */
	struct GLDepthFuncStateSet :
			public GLStateSet
	{
		explicit
		GLDepthFuncStateSet(
				GLenum func) :
			d_depth_func(func)
		{  }

		virtual
		void
		apply_state(
				const GLCapabilities &capabilities,
				const GLStateSet &current_state_set,
				const GLState &current_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				const GLState &current_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				const GLState &current_state) const;


		GLenum d_depth_func;
	};

	/**
	 * Used to set the depth mask.
	 */
	struct GLDepthMaskStateSet :
			public GLStateSet
	{
		explicit
		GLDepthMaskStateSet(
				GLboolean flag) :
			d_flag(flag)
		{  }

		virtual
		void
		apply_state(
				const GLCapabilities &capabilities,
				const GLStateSet &current_state_set,
				const GLState &current_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				const GLState &current_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				const GLState &current_state) const;


		GLboolean d_flag;
	};

	/**
	 * Used to set the depth range.
	 */
	struct GLDepthRangeStateSet :
			public GLStateSet
	{
		//! Typedef for a sequence of depth ranges.
		typedef std::vector<GLDepthRange> depth_range_seq_type;

		//! Constructor to set all depth ranges to the same parameters.
		explicit
		GLDepthRangeStateSet(
				const GLCapabilities &capabilities,
				const GLDepthRange &depth_range);

		//! Constructor to set depth ranges individually.
		explicit
		GLDepthRangeStateSet(
				const GLCapabilities &capabilities,
				const depth_range_seq_type &all_depth_ranges);

		virtual
		void
		apply_state(
				const GLCapabilities &capabilities,
				const GLStateSet &current_state_set,
				const GLState &current_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				const GLState &current_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				const GLState &current_state) const;


	private:
		//! Contains 'GLCapabilities::Viewport::gl_max_viewports' depth ranges.
		depth_range_seq_type d_depth_ranges;

		//! Is true if all depth ranges in @a d_depth_ranges are the same.
		bool d_all_depth_ranges_are_the_same;

		static GLDepthRange DEFAULT_DEPTH_RANGE;

		void
		apply_state(
				const GLCapabilities &capabilities) const;
	};

	/**
	 * Used to enable/disable capabilities.
	 */
	struct GLEnableStateSet :
			public GLStateSet
	{
		GLEnableStateSet(
				GLenum cap,
				bool enable) :
			d_cap(cap),
			d_enable(enable)
		{  }

		virtual
		void
		apply_state(
				const GLCapabilities &capabilities,
				const GLStateSet &current_state_set,
				const GLState &current_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				const GLState &current_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				const GLState &current_state) const;

		//! Utility function to return the default for the specified capability.
		static
		bool
		get_default(
				GLenum cap);

		GLenum d_cap;
		bool d_enable;
	};

	/**
	 * Used for glFrontFace.
	 */
	struct GLFrontFaceStateSet :
			public GLStateSet
	{
		explicit
		GLFrontFaceStateSet(
				GLenum dir) :
			d_dir(dir)
		{  }

		virtual
		void
		apply_state(
				const GLCapabilities &capabilities,
				const GLStateSet &current_state_set,
				const GLState &current_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				const GLState &current_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				const GLState &current_state) const;


		GLenum d_dir;
	};

	/**
	 * Used for glHint.
	 */
	struct GLHintStateSet :
			public GLStateSet
	{
		GLHintStateSet(
				GLenum target,
				GLenum hint) :
			d_target(target),
			d_hint(hint)
		{  }

		virtual
		void
		apply_state(
				const GLCapabilities &capabilities,
				const GLStateSet &current_state_set,
				const GLState &current_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				const GLState &current_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				const GLState &current_state) const;


		GLenum d_target;
		GLenum d_hint;
	};

	/**
	 * Used to set the line width.
	 */
	struct GLLineWidthStateSet :
			public GLStateSet
	{
		explicit
		GLLineWidthStateSet(
				GLfloat width) :
			d_width(width)
		{  }

		virtual
		void
		apply_state(
				const GLCapabilities &capabilities,
				const GLStateSet &current_state_set,
				const GLState &current_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				const GLState &current_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				const GLState &current_state) const;


		GPlatesMaths::real_t d_width;
	};

	/**
	 * Used to set the point size.
	 */
	struct GLPointSizeStateSet :
			public GLStateSet
	{
		explicit
		GLPointSizeStateSet(
				GLfloat size) :
			d_size(size)
		{  }

		virtual
		void
		apply_state(
				const GLCapabilities &capabilities,
				const GLStateSet &current_state_set,
				const GLState &current_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				const GLState &current_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				const GLState &current_state) const;


		GPlatesMaths::real_t d_size;
	};

	/**
	 * Used to set the polygon mode.
	 *
	 * NOTE: OpenGL 3.3 core requires 'face' (parameter of glPolygonMode) to be 'GL_FRONT_AND_BACK'.
	 */
	struct GLPolygonModeStateSet :
			public GLStateSet
	{
		explicit
		GLPolygonModeStateSet(
				GLenum mode) :
			d_mode(mode)
		{  }

		virtual
		void
		apply_state(
				const GLCapabilities &capabilities,
				const GLStateSet &current_state_set,
				const GLState &current_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				const GLState &current_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				const GLState &current_state) const;


		GLenum d_mode;
	};

	/**
	 * Used for glPolygonOffset.
	 */
	struct GLPolygonOffsetStateSet :
			public GLStateSet
	{
		GLPolygonOffsetStateSet(
				GLfloat factor,
				GLfloat units) :
			d_factor(factor),
			d_units(units)
		{  }

		virtual
		void
		apply_state(
				const GLCapabilities &capabilities,
				const GLStateSet &current_state_set,
				const GLState &current_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				const GLState &current_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				const GLState &current_state) const;


		GPlatesMaths::real_t d_factor;
		GPlatesMaths::real_t d_units;
	};

	/**
	 * Used to set the scissor rectangle(s).
	 */
	struct GLScissorStateSet :
			public GLStateSet
	{
		//! Typedef for a sequence of scissor rectangles.
		typedef std::vector<GLViewport> scissor_rectangle_seq_type;

		//! Constructor to set all scissor rectangles to the same parameters.
		GLScissorStateSet(
				const GLCapabilities &capabilities,
				const GLViewport &all_scissor_rectangles,
				const GLViewport &default_viewport);

		//! Constructor to set scissor rectangles individually.
		GLScissorStateSet(
				const GLCapabilities &capabilities,
				const scissor_rectangle_seq_type &all_scissor_rectangles,
				const GLViewport &default_viewport);

		virtual
		void
		apply_state(
				const GLCapabilities &capabilities,
				const GLStateSet &current_state_set,
				const GLState &current_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				const GLState &current_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				const GLState &current_state) const;


		/**
		 * Returns scissor rectangle at index @a viewport_index (default index is zero).
		 *
		 * If there's only one scissor rectangle set (see constructor) then all scissor rectangles
		 * are the same and it doesn't matter which index is chosen.
		 *
		 * NOTE: @a viewport_index must be less than 'context.get_capabilities().viewport.gl_max_viewports'.
		 */
		const GLViewport &
		get_scissor(
				const GLCapabilities &capabilities,
				unsigned int viewport_index = 0) const;

	private:
		//! Contains 'GLCapabilities::Viewport::gl_max_viewports' scissor rectangles.
		scissor_rectangle_seq_type d_scissor_rectangles;

		//! Is true if all scissor rectangles in @a d_scissor_rectangles are the same.
		bool d_all_scissor_rectangles_are_the_same;

		//! Default viewport of window currently attached to the OpenGL context.
		GLViewport d_default_viewport;

		void
		apply_state(
				const GLCapabilities &capabilities) const;
	};

	/**
	 * Used to set the stencil function.
	 */
	struct GLStencilFuncStateSet :
			public GLStateSet
	{
		explicit
		GLStencilFuncStateSet(
				GLenum func,
				GLint ref,
				GLuint mask) :
			d_func(func),
			d_ref(ref),
			d_mask(mask)
		{  }

		virtual
		void
		apply_state(
				const GLCapabilities &capabilities,
				const GLStateSet &current_state_set,
				const GLState &current_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				const GLState &current_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				const GLState &current_state) const;


		GLenum d_func;
		GLint d_ref;
		GLuint d_mask;
	};

	/**
	 * Used to set the stencil mask.
	 */
	struct GLStencilMaskStateSet :
			public GLStateSet
	{
		explicit
		GLStencilMaskStateSet(
				GLuint stencil) :
			d_stencil(stencil)
		{  }

		virtual
		void
		apply_state(
				const GLCapabilities &capabilities,
				const GLStateSet &current_state_set,
				const GLState &current_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				const GLState &current_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				const GLState &current_state) const;


		GLuint d_stencil;
	};

	/**
	 * Used to set the stencil operation.
	 */
	struct GLStencilOpStateSet :
			public GLStateSet
	{
		explicit
		GLStencilOpStateSet(
				GLenum fail,
				GLenum zfail,
				GLenum zpass) :
			d_fail(fail),
			d_zfail(zfail),
			d_zpass(zpass)
		{  }

		virtual
		void
		apply_state(
				const GLCapabilities &capabilities,
				const GLStateSet &current_state_set,
				const GLState &current_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				const GLState &current_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				const GLState &current_state) const;


		GLenum d_fail;
		GLenum d_zfail;
		GLenum d_zpass;
	};

	/**
	 * Used to set the viewport.
	 */
	struct GLViewportStateSet :
			public GLStateSet
	{
		//! Typedef for a sequence of viewports.
		typedef std::vector<GLViewport> viewport_seq_type;

		//! Constructor to set all viewport to the same parameters.
		GLViewportStateSet(
				const GLCapabilities &capabilities,
				const GLViewport &all_viewports,
				const GLViewport &default_viewport);

		//! Constructor to set viewports individually.
		GLViewportStateSet(
				const GLCapabilities &capabilities,
				const viewport_seq_type &all_viewports,
				const GLViewport &default_viewport);

		virtual
		void
		apply_state(
				const GLCapabilities &capabilities,
				const GLStateSet &current_state_set,
				const GLState &current_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				const GLState &current_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				const GLState &current_state) const;

		/**
		 * Returns viewport at index @a viewport_index (default index is zero).
		 *
		 * If there's only one viewport set (see constructor) then all viewports are the same and
		 * it doesn't matter which index is chosen.
		 *
		 * NOTE: @a viewport_index must be less than 'context.get_capabilities().viewport.gl_max_viewports'.
		 */
		const GLViewport &
		get_viewport(
				const GLCapabilities &capabilities,
				unsigned int viewport_index = 0) const;

	private:
		//! Contains 'GLCapabilities::Viewport::gl_max_viewports' viewports.
		viewport_seq_type d_viewports;

		//! Is true if all viewports in @a viewports are the same.
		bool d_all_viewports_are_the_same;

		//! Default viewport of window currently attached to the OpenGL context.
		GLViewport d_default_viewport;

		void
		apply_state(
				const GLCapabilities &capabilities) const;
	};
}

#endif // GPLATES_OPENGL_GLSTATESETS_H
