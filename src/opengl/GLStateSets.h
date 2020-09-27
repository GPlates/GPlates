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
#include <opengl/OpenGL1.h>

#include "GLBuffer.h"
#include "GLCapabilities.h"
#include "GLFramebuffer.h"
#include "GLMatrix.h"
#include "GLProgram.h"
#include "GLRenderbuffer.h"
#include "GLSampler.h"
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
	 * Used to bind a framebuffer object to GL_DRAW_FRAMEBUFFER and/or GL_READ_FRAMEBUFFER targets.
	 */
	struct GLBindFramebufferStateSet :
			public GLStateSet
	{
		/**
		 * Binds a framebuffer object, or unbinds (if boost::none).
		 */
		explicit
		GLBindFramebufferStateSet(
				boost::optional<GLFramebuffer::shared_ptr_type> draw_framebuffer,
				boost::optional<GLFramebuffer::shared_ptr_type> read_framebuffer,
				// Draw framebuffer resource handle associated with the current OpenGL context...
				GLuint draw_framebuffer_resource,
				// Read framebuffer resource handle associated with the current OpenGL context...
				GLuint read_framebuffer_resource) :
			d_draw_framebuffer(draw_framebuffer),
			d_read_framebuffer(read_framebuffer),
			d_draw_framebuffer_resource(draw_framebuffer_resource),
			d_read_framebuffer_resource(read_framebuffer_resource)
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


		boost::optional<GLFramebuffer::shared_ptr_type> d_draw_framebuffer;
		boost::optional<GLFramebuffer::shared_ptr_type> d_read_framebuffer;

		GLuint d_draw_framebuffer_resource;
		GLuint d_read_framebuffer_resource;
	};

	/**
	 * Used to bind a renderbuffer object.
	 */
	struct GLBindRenderbufferStateSet :
			public GLStateSet
	{
		//! Binds a renderbuffer object.
		GLBindRenderbufferStateSet(
				GLenum target,
				boost::optional<GLRenderbuffer::shared_ptr_type> renderbuffer) :
			d_target(target),
			d_renderbuffer(renderbuffer),
			d_renderbuffer_resource(0)
		{
			if (renderbuffer)
			{
				d_renderbuffer_resource = renderbuffer.get()->get_resource_handle();
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
		boost::optional<GLRenderbuffer::shared_ptr_type> d_renderbuffer;
		GLuint d_renderbuffer_resource;
	};

	/**
	 * Used to bind a sampler to a texture unit.
	 */
	struct GLBindSamplerStateSet :
			public GLStateSet
	{
		//! Binds a sampler object.
		GLBindSamplerStateSet(
				const GLCapabilities &capabilities,
				GLuint unit,
				boost::optional<GLSampler::shared_ptr_type> sampler);

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


		GLuint d_unit;
		boost::optional<GLSampler::shared_ptr_type> d_sampler;
		GLuint d_sampler_resource;
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
				boost::optional<GLTexture::shared_ptr_type> texture);

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
		boost::optional<GLTexture::shared_ptr_type> d_texture;
		GLuint d_texture_resource;
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
	 * Used for glBlendColor.
	 */
	struct GLBlendColorStateSet :
			public GLStateSet
	{
		GLBlendColorStateSet(
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
	 * Used to set the alpha blend equation.
	 */
	struct GLBlendEquationStateSet :
			public GLStateSet
	{
		static const GLenum DEFAULT_MODE;

		// Same RGB and alpha modes.
		explicit
		GLBlendEquationStateSet(
				GLenum mode) :
			d_mode_RGB(mode),
			d_mode_alpha(mode)
		{  }

		// Different RGB and alpha modes.
		GLBlendEquationStateSet(
				GLenum mode_RGB,
				GLenum mode_alpha) :
			d_mode_RGB(mode_RGB),
			d_mode_alpha(mode_alpha)
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


		GLenum d_mode_RGB;
		GLenum d_mode_alpha;
	};

	/**
	 * Used to set the alpha blend function.
	 */
	struct GLBlendFuncStateSet :
			public GLStateSet
	{
		struct Func
		{
			GLenum src;
			GLenum dst;

			bool
			operator==(
					const Func &rhs) const
			{
				return src == rhs.src && dst == rhs.dst;
			}

			bool
			operator!=(
					const Func &rhs) const
			{
				return !(*this == rhs);
			}
		};

		static const Func DEFAULT_FUNC;

		// Same RGB and alpha modes.
		GLBlendFuncStateSet(
				const Func &func) :
			d_RGB_func(func),
			d_alpha_func(func)
		{  }

		// Different RGB and alpha modes.
		GLBlendFuncStateSet(
				const Func &RGB_func,
				const Func &alpha_func) :
			d_RGB_func(RGB_func),
			d_alpha_func(alpha_func)
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


		Func d_RGB_func;
		Func d_alpha_func;
	};

	/**
	 * Used to set the clamp color (used by 'glReadPixels').
	 */
	struct GLClampColorStateSet :
			public GLStateSet
	{
		GLClampColorStateSet(
				GLenum target,
				GLenum clamp) :
			d_target(target),
			d_clamp(clamp)
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
		GLenum d_clamp;
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
		struct Mask
		{
			bool operator==(
					const Mask &rhs) const
			{
				return red == rhs.red && green == rhs.green && blue == rhs.blue && alpha == rhs.alpha;
			}

			bool operator!=(
					const Mask &rhs) const
			{
				return !(*this == rhs);
			}

			GLboolean red;
			GLboolean green;
			GLboolean blue;
			GLboolean alpha;
		};

		static const Mask DEFAULT_MASK;


		// glColorMask
		GLColorMaskStateSet(
				const GLCapabilities &capabilities,
				const Mask &mask) :
			d_masks(capabilities.gl_max_draw_buffers, mask),
			d_all_masks_equal(true)
		{  }

		// glColorMaski
		GLColorMaskStateSet(
				const GLCapabilities &capabilities,
				const std::vector<Mask> &masks) :
			d_masks(masks),
			d_all_masks_equal(false)
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

		std::vector<Mask> d_masks;
		bool d_all_masks_equal;
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
		//! Constructor to set all depth ranges to the same parameters.
		explicit
		GLDepthRangeStateSet(
				const GLCapabilities &capabilities,
				GLclampd n,
				GLclampd f) :
			d_n(n),
			d_f(f)
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

		GPlatesMaths::real_t d_n;
		GPlatesMaths::real_t d_f;
	};

	/**
	 * Used to set the draw buffers in default framebuffer.
	 */
	struct GLDrawBuffersStateSet :
			public GLStateSet
	{
		explicit
		GLDrawBuffersStateSet(
				const std::vector<GLenum> &draw_buffers,
				GLenum default_draw_buffer) :
			d_draw_buffers(draw_buffers),
			d_default_draw_buffer(default_draw_buffer)
		{  }

		explicit
		GLDrawBuffersStateSet(
				GLenum draw_buffer,
				GLenum default_draw_buffer) :
			d_draw_buffers(1, draw_buffer),
			d_default_draw_buffer(default_draw_buffer)
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

		std::vector<GLenum> d_draw_buffers;
		GLenum d_default_draw_buffer;
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
	 * Used to enable/disable indexed capabilities.
	 */
	struct GLEnableIndexedStateSet :
			public GLStateSet
	{
		// Enable all indices (or disable all indices) in the specified capability.
		GLEnableIndexedStateSet(
				GLenum cap,
				bool enable,
				GLuint num_capability_indices) :
			d_cap(cap),
			d_indices(num_capability_indices, enable),
			d_all_indices_equal(true)
		{  }

		// Enable/disable each index individually in the specified capability.
		GLEnableIndexedStateSet(
				GLenum cap,
				const std::vector<bool> &enable_indices) :
			d_cap(cap),
			d_indices(enable_indices),
			d_all_indices_equal(false)
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

		GLenum d_cap;
		std::vector<bool> d_indices;
		bool d_all_indices_equal;
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
	 * Used for glPixelStoref/glPixelStorei.
	 */
	struct GLPixelStoreStateSet :
			public GLStateSet
	{
		/**
		 * Specify *float* parameter value.
		 *
		 * Note: We map GLfloat to a GLint since there are actually no parameters of type GLfloat.
		 *       They're all boolean or integer.
		 */
		GLPixelStoreStateSet(
				GLenum pname,
				GLfloat param);

		/**
		 * Specify *integer* parameter value.
		 */
		GLPixelStoreStateSet(
				GLenum pname,
				GLint param);

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

		static
		GLint
		get_default(
				GLenum pname);

		GLenum d_pname;
		GLint d_param;
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
	 * Used for glPrimitiveRestartIndex.
	 */
	struct GLPrimitiveRestartIndexStateSet :
			public GLStateSet
	{
		explicit
		GLPrimitiveRestartIndexStateSet(
				GLuint index) :
			d_index(index)
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


		GLuint d_index;
	};

	/**
	 * Used to set the read buffer in default framebuffer.
	 */
	struct GLReadBufferStateSet :
			public GLStateSet
	{
		explicit
		GLReadBufferStateSet(
				GLenum read_buffer,
				GLenum default_read_buffer) :
			d_read_buffer(read_buffer),
			d_default_read_buffer(default_read_buffer)
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

		GLenum d_read_buffer;
		GLenum d_default_read_buffer;
	};

	/**
	 * Used for glSampleCoverage.
	 */
	struct GLSampleCoverageStateSet :
			public GLStateSet
	{
		explicit
		GLSampleCoverageStateSet(
				GLclampf value,
				GLboolean invert) :
			d_value(value),
			d_invert(invert)
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

		GPlatesMaths::real_t d_value;
		GLboolean d_invert;
	};

	/**
	 * Used for glSampleMaski.
	 */
	struct GLSampleMaskStateSet :
			public GLStateSet
	{
		static const GLbitfield DEFAULT_MASK;

		explicit
		GLSampleMaskStateSet(
				const std::vector<GLbitfield> &masks) :
			d_masks(masks)
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

		std::vector<GLbitfield> d_masks;
	};

	/**
	 * Used to set the scissor rectangle(s).
	 */
	struct GLScissorStateSet :
			public GLStateSet
	{
		GLScissorStateSet(
				const GLViewport &scissor_rectangle,
				const GLViewport &default_scissor_rectangle) :
			d_scissor_rectangle(scissor_rectangle),
			d_default_scissor_rectangle(default_scissor_rectangle)
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


		GLViewport d_scissor_rectangle;
		GLViewport d_default_scissor_rectangle;
	};

	/**
	 * Used to set the stencil function.
	 */
	struct GLStencilFuncStateSet :
			public GLStateSet
	{
		struct Func
		{
			GLenum func;
			GLint ref;
			GLuint mask;

			bool
			operator==(
					const Func &rhs) const
			{
				return func == rhs.func && ref == rhs.ref && mask == rhs.mask;
			}

			bool
			operator!=(
					const Func &rhs) const
			{
				return !(*this == rhs);
			}
		};

		static const Func DEFAULT_FUNC;

		// Same front and back.
		GLStencilFuncStateSet(
				const Func &func) :
			d_front_func(func),
			d_back_func(func)
		{  }

		// Different front and back.
		GLStencilFuncStateSet(
				const Func &front_func,
				const Func &back_func) :
			d_front_func(front_func),
			d_back_func(back_func)
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


		Func d_front_func;
		Func d_back_func;
	};

	/**
	 * Used to set the stencil mask.
	 */
	struct GLStencilMaskStateSet :
			public GLStateSet
	{
		static const GLuint DEFAULT_MASK;

		// Same front and back.
		explicit
		GLStencilMaskStateSet(
				GLuint mask) :
			d_front_mask(mask),
			d_back_mask(mask)
		{  }

		// Different front and back.
		GLStencilMaskStateSet(
				GLuint front_mask,
				GLuint back_mask) :
			d_front_mask(front_mask),
			d_back_mask(back_mask)
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


		GLuint d_front_mask;
		GLuint d_back_mask;
	};

	/**
	 * Used to set the stencil operation.
	 */
	struct GLStencilOpStateSet :
			public GLStateSet
	{
		struct Op
		{
			GLenum sfail;
			GLenum dpfail;
			GLenum dppass;

			bool
			operator==(
					const Op &rhs) const
			{
				return sfail == rhs.sfail && dpfail == rhs.dpfail && dppass == rhs.dppass;
			}

			bool
			operator!=(
					const Op &rhs) const
			{
				return !(*this == rhs);
			}
		};

		static const Op DEFAULT_OP;

		// Same front and back.
		GLStencilOpStateSet(
				const Op &op) :
			d_front_op(op),
			d_back_op(op)
		{  }

		// Different front and back.
		GLStencilOpStateSet(
				const Op &front_op,
				const Op &back_op) :
			d_front_op(front_op),
			d_back_op(back_op)
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

		Op d_front_op;
		Op d_back_op;
	};

	/**
	 * Used to bind/use a program object.
	 */
	struct GLUseProgramStateSet :
			public GLStateSet
	{
		/**
		 * Binds a shader program object, or unbinds (if @a program is boost::none).
		 */
		explicit
		GLUseProgramStateSet(
				boost::optional<GLProgram::shared_ptr_type> program) :
			d_program(program),
			d_program_resource(0)
		{
			if (program)
			{
				d_program_resource = program.get()->get_resource_handle();
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


		boost::optional<GLProgram::shared_ptr_type> d_program;
		GLuint d_program_resource;
	};

	/**
	 * Used to set the viewport.
	 */
	struct GLViewportStateSet :
			public GLStateSet
	{
		GLViewportStateSet(
				const GLViewport &viewport,
				const GLViewport &default_viewport) :
			d_viewport(viewport),
			d_default_viewport(default_viewport)
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


		GLViewport d_viewport;
		GLViewport d_default_viewport;
	};
}

#endif // GPLATES_OPENGL_GLSTATESETS_H
