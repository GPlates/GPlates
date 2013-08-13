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
#include <opengl/OpenGL.h>

#include "GLBufferImpl.h"
#include "GLBufferObject.h"
#include "GLDepthRange.h"
#include "GLFrameBufferObject.h"
#include "GLMatrix.h"
#include "GLProgramObject.h"
#include "GLStateSet.h"
#include "GLTexture.h"
#include "GLVertexArrayObject.h"
#include "GLVertexBufferObject.h"
#include "GLViewport.h"

#include "global/CompilerWarnings.h"

#include "maths/types.h"

// BOOST_MPL_ASSERT contains old-style casts.
DISABLE_GCC_WARNING("-Wold-style-cast")


namespace GPlatesOpenGL
{
	class GLCapabilities;

	namespace Implementation
	{
		/**
		 * Utility class for tracking state changes of vertex attribute pointers (both generic and non-generic).
		 *
		 * Also tracks state changes from buffer objects to/from client memory arrays.
		 */
		class GLVertexAttributeBuffer

		{
		public:
			//! Binds to a vertex buffer object.
			GLVertexAttributeBuffer(
					GLint offset,
					const GLBufferObject::shared_ptr_to_const_type &buffer_object) :
				d_offset(offset),
				d_buffer_variant(buffer_object),
				d_buffer(buffer_object),
				d_pointer_to_apply(NULL)
			{  }

			//! No binding to a vertex buffer object (using client memory array).
			GLVertexAttributeBuffer(
					GLint offset,
					const GLBufferImpl::shared_ptr_to_const_type &buffer_impl) :
				d_offset(offset),
				d_buffer_variant(buffer_impl),
				d_buffer(buffer_impl),
				d_pointer_to_apply(NULL)
			{  }


			/**
			 * Returns true if a buffer pointer state change is necessary.
			 *
			 * For buffer objects a change is switching buffer objects (or switching from
			 * using client memory arrays, ie, buffer object zero).
			 *
			 * For client memory arrays a change is switching away from a buffer object.
			 *
			 * NOTE: This also updates the buffer pointer to apply (see @a get_buffer_pointer_to_apply).
			 */
			bool
			has_changed_state(
					const GLVertexAttributeBuffer &last_applied_buffer) const;

			/**
			 * Returns true if a buffer pointer state change from the default state is necessary.
			 *
			 * NOTE: This also updates the buffer pointer to apply (see @a get_buffer_pointer_to_apply).
			 */
			bool
			has_changed_from_default_state() const;

			/**
			 * Returns true if a buffer pointer state change to the default state is necessary.
			 *
			 * NOTE: This also updates the buffer pointer to apply (see @a get_buffer_pointer_to_apply).
			 */
			bool
			has_changed_to_default_state() const;

			/**
			 * Binds the current buffer.
			 *
			 * If the current buffer is a client memory buffer then ensures no buffer objects are bound.
			 * If the current buffer is a buffer object then ensures it is currently bound.
			 */
			void
			bind_buffer(
					const GLCapabilities &capabilities,
					GLState &last_applied_state) const;

			/**
			 * Unbinds the current buffer.
			 *
			 * Regardless of whether the current buffer is a client memory buffer or a buffer object
			 * this method ensures no buffer objects are bound (returns to default client memory arrays).
			 */
			void
			unbind_buffer(
					const GLCapabilities &capabilities,
					GLState &last_applied_state) const;

			/**
			 * Returns the buffer pointer (to an attribute array with offset added) that needs to be applied.
			 *
			 * NOTE: For buffer objects the returned pointer is actually just the offset.
			 */
			const GLvoid *
			get_buffer_pointer_to_apply() const
			{
				return d_pointer_to_apply;
			}

			/**
			 * Call this when you've just specified a vertex array pointer.
			 */
			void
			applied_buffer_pointer_to_opengl() const
			{
				// Keep the buffer allocation observer up-to-date with the latest buffer allocation.
				d_buffer->update_buffer_allocation_observer(d_buffer_allocation_observer);
			}

		private:
			//! Typedef for a variant of @a GLBufferObject or @a GLBufferImpl.
			typedef boost::variant<GLBufferObject::shared_ptr_to_const_type, GLBufferImpl::shared_ptr_to_const_type> buffer_type;

			//! The offset into the buffer.
			GLint d_offset;

			//! The derived GLBuffer type (used to access methods existing only in derived classes).
			buffer_type d_buffer_variant;

			//! The base GLBuffer pointer.
			GLBuffer::shared_ptr_to_const_type d_buffer;

			//! Keeps track of the last applied buffer (array) pointer.
			mutable const GLvoid *d_pointer_to_apply;

			/**
			 * Keeps track of internal buffer (re)allocations (ie, calls to 'GLBuffer::gl_buffer_data').
			 *
			 * When these happen we need to re-specify vertex array pointers/bindings.
			 */
			mutable GLBuffer::buffer_allocation_observer_type d_buffer_allocation_observer;
		};
	}


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
				const GLStateSet &last_applied_state_set,
				GLState &last_applied_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;


		GLenum d_active_texture;
	};

	/**
	 * Used to set the alpha test function.
	 */
	struct GLAlphaFuncStateSet :
			public GLStateSet
	{
		GLAlphaFuncStateSet(
				GLenum func,
				GLclampf ref) :
			d_alpha_func(func),
			d_ref(ref)
		{  }

		virtual
		void
		apply_state(
				const GLCapabilities &capabilities,
				const GLStateSet &last_applied_state_set,
				GLState &last_applied_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;


		GLenum d_alpha_func;
		GPlatesMaths::real_t d_ref;
	};

	/**
	 * Used to bind a framebuffer object.
	 */
	struct GLBindBufferObjectStateSet :
			public GLStateSet
	{
		//! Binds a buffer object.
		GLBindBufferObjectStateSet(
				const GLBufferObject::shared_ptr_to_const_type &buffer_object,
				GLenum target) :
			d_buffer_object(buffer_object),
			d_buffer_object_resource(buffer_object->get_buffer_resource_handle()),
			d_target(target)
		{  }

		//! Specifies no bound buffer object (at the specified target).
		explicit
		GLBindBufferObjectStateSet(
				GLenum target) :
			d_target(target)
		{  }

		virtual
		void
		apply_state(
				const GLCapabilities &capabilities,
				const GLStateSet &last_applied_state_set,
				GLState &last_applied_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;


		boost::optional<GLBufferObject::shared_ptr_to_const_type> d_buffer_object;
		boost::optional<GLBufferObject::resource_handle_type> d_buffer_object_resource;
		GLenum d_target;
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
				const GLStateSet &last_applied_state_set,
				GLState &last_applied_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;


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
				const GLStateSet &last_applied_state_set,
				GLState &last_applied_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;


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
				const GLTexture::shared_ptr_to_const_type &texture_object,
				GLenum texture_unit,
				GLenum texture_target);

		//! Unbinds any texture object currently bound to the specified target and texture unit.
		GLBindTextureStateSet(
				const GLCapabilities &capabilities,
				GLenum texture_unit,
				GLenum texture_target);

		virtual
		void
		apply_state(
				const GLCapabilities &capabilities,
				const GLStateSet &last_applied_state_set,
				GLState &last_applied_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;


		boost::optional<GLTexture::shared_ptr_to_const_type> d_texture_object;
		GLenum d_texture_unit;
		GLenum d_texture_target;
	};

	/**
	 * Used to bind a vertex array object.
	 */
	struct GLBindVertexArrayObjectStateSet :
			public GLStateSet
	{
		/**
		 * Binds a vertex array object.
		 *
		 * @a current_resource_state represents the vertex array state of the vertex array object.
		 *
		 * @a current_default_state represents the vertex array state of the default
		 * vertex array object (resource handle zero).
		 * It is needed in case we are asked to switch *from* the object state *to* the default state.
		 *
		 * NOTE: We pass in @a resource_handle instead of requesting it internally.
		 * This is because 'GLVertexArrayObject::get_vertex_array_resource_handle()' is not a simple
		 * handle retrieval function but instead can create a new resource handle (for a specific
		 * OpenGL context). In essence it's a higher level function in the renderer framework and
		 * doesn't belong at this low level.
		 */
		GLBindVertexArrayObjectStateSet(
				GLVertexArrayObject::resource_handle_type resource_handle,
				const boost::shared_ptr<GLState> &current_resource_state,
				const boost::shared_ptr<GLState> &current_default_state,
				const GLVertexArrayObject::shared_ptr_to_const_type &vertex_array_object) :
			d_resource_handle(resource_handle),
			d_current_resource_state(current_resource_state),
			d_current_default_state(current_default_state),
			d_vertex_array_object(vertex_array_object)
		{  }

		/**
		 * Unbinds any vertex array object (switches to using the default vertex array object with
		 * resource handle zero).
		 *
		 * @a current_default_state represents the vertex array state of the default
		 * vertex array object (resource handle zero).
		 */
		explicit
		GLBindVertexArrayObjectStateSet(
				const boost::shared_ptr<GLState> &current_default_state) :
			d_current_resource_state(current_default_state),
			d_current_default_state(current_default_state)
		{  }

		virtual
		void
		apply_state(
				const GLCapabilities &capabilities,
				const GLStateSet &last_applied_state_set,
				GLState &last_applied_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;


		boost::optional<GLVertexArrayObject::resource_handle_type> d_resource_handle;

		/**
		 * Represents the current vertex array object state as seen by the underlying OpenGL.
		 *
		 * NOTE: This is *always* non-null - even for the default vertex array object (resource handle zero).
		 * Even the default vertex array object represents vertex array state.
		 * In which case @a d_current_resource_state and @a d_current_default_state are the same.
		 */
		mutable boost::shared_ptr<GLState> d_current_resource_state;

		/**
		 * Represents the current vertex array state for the default vertex array object with
		 * resource handle zero (as seen by the underlying OpenGL).
		 *
		 * NOTE: This is *always* non-null - even for the default vertex array object (resource handle zero).
		 */
		mutable boost::shared_ptr<GLState> d_current_default_state;

		// NOTE: This is *only* here for clients to retrieve - so they can know what
		// @a GLVertexArrayObject the resource handle came from - we don't actually use it in
		// the implementation of this class.
		// The resource handle is what is used to do the actual binding.
		//
		// NOTE: 'GLVertexArrayObject::get_vertex_array_resource_handle()' cannot be called on this
		// here since it requires a 'GLRenderer' (since it can create a new vertex array object
		// for the current OpenGL context and perform some buffer bindings - and we're way past
		// that at this level of the code).
		boost::optional<GLVertexArrayObject::shared_ptr_to_const_type> d_vertex_array_object;
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
				const GLStateSet &last_applied_state_set,
				GLState &last_applied_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;


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
				const GLStateSet &last_applied_state_set,
				GLState &last_applied_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;


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
				const GLStateSet &last_applied_state_set,
				GLState &last_applied_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;


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
				const GLStateSet &last_applied_state_set,
				GLState &last_applied_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;


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
				const GLStateSet &last_applied_state_set,
				GLState &last_applied_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;


		GLint d_stencil;
	};

	/**
	 * Used to set the client active texture unit.
	 */
	struct GLClientActiveTextureStateSet :
			public GLStateSet
	{
		explicit
		GLClientActiveTextureStateSet(
				const GLCapabilities &capabilities,
				GLenum client_active_texture);

		virtual
		void
		apply_state(
				const GLCapabilities &capabilities,
				const GLStateSet &last_applied_state_set,
				GLState &last_applied_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;


		GLenum d_client_active_texture;
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
				const GLStateSet &last_applied_state_set,
				GLState &last_applied_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;


		GLboolean d_red;
		GLboolean d_green;
		GLboolean d_blue;
		GLboolean d_alpha;
	};

	/**
	 * Used to set the vertex color array source.
	 */
	struct GLColorPointerStateSet :
			public GLStateSet
	{
		//! Binds to a vertex buffer object.
		GLColorPointerStateSet(
				GLint size,
				GLenum type,
				GLsizei stride,
				GLint offset,
				const GLBufferObject::shared_ptr_to_const_type &buffer_object) :
			d_buffer(offset, buffer_object),
			d_size(size),
			d_type(type),
			d_stride(stride)
		{  }

		//! No binding to a vertex buffer object (using client memory array).
		GLColorPointerStateSet(
				GLint size,
				GLenum type,
				GLsizei stride,
				GLint offset,
				const GLBufferImpl::shared_ptr_to_const_type &buffer_impl) :
			d_buffer(offset, buffer_impl),
			d_size(size),
			d_type(type),
			d_stride(stride)
		{  }

		virtual
		void
		apply_state(
				const GLCapabilities &capabilities,
				const GLStateSet &last_applied_state_set,
				GLState &last_applied_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;


	private:
		Implementation::GLVertexAttributeBuffer d_buffer;
		GLint d_size;
		GLenum d_type;
		GLsizei d_stride;
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
				const GLStateSet &last_applied_state_set,
				GLState &last_applied_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;


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
				const GLStateSet &last_applied_state_set,
				GLState &last_applied_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;


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
				const GLStateSet &last_applied_state_set,
				GLState &last_applied_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;


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
				const GLStateSet &last_applied_state_set,
				GLState &last_applied_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;


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
	 * Used to enable/disable client state vertex arrays.
	 */
	struct GLEnableClientStateStateSet :
			public GLStateSet
	{
		GLEnableClientStateStateSet(
				GLenum array,
				bool enable) :
			d_array(array),
			d_enable(enable)
		{  }

		virtual
		void
		apply_state(
				const GLCapabilities &capabilities,
				const GLStateSet &last_applied_state_set,
				GLState &last_applied_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;


		GLenum d_array;
		bool d_enable;
	};

	/**
	 * Used to enable/disable client state texture coordinate vertex arrays.
	 */
	struct GLEnableClientTextureStateStateSet :
			public GLStateSet
	{
		GLEnableClientTextureStateStateSet(
				GLenum texture_unit,
				bool enable) :
			d_texture_unit(texture_unit),
			d_enable(enable)
		{  }

		virtual
		void
		apply_state(
				const GLCapabilities &capabilities,
				const GLStateSet &last_applied_state_set,
				GLState &last_applied_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;


		GLenum d_texture_unit;
		bool d_enable;
	};

	/**
	 * Used to enable/disable capabilities (except texturing - use @a GLEnableTextureStateSet for that).
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
				const GLStateSet &last_applied_state_set,
				GLState &last_applied_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;

		//! Utilitiy function to return the default for the specified capability.
		static
		bool
		get_default(
				GLenum cap);

		GLenum d_cap;
		bool d_enable;
	};

	/**
	 * Used to enable/disable texturing.
	 */
	struct GLEnableTextureStateSet :
			public GLStateSet
	{
		GLEnableTextureStateSet(
				GLenum texture_unit,
				GLenum texture_target,
				bool enable) :
			d_texture_unit(texture_unit),
			d_texture_target(texture_target),
			d_enable(enable)
		{  }

		virtual
		void
		apply_state(
				const GLCapabilities &capabilities,
				const GLStateSet &last_applied_state_set,
				GLState &last_applied_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;


		GLenum d_texture_unit;
		GLenum d_texture_target;
		bool d_enable;
	};

	/**
	 * Used to enable/disable *generic* vertex attribute arrays.
	 */
	struct GLEnableVertexAttribArrayStateSet :
			public GLStateSet
	{
		GLEnableVertexAttribArrayStateSet(
				GLuint attribute_index,
				bool enable) :
			d_attribute_index(attribute_index),
			d_enable(enable)
		{  }

		virtual
		void
		apply_state(
				const GLCapabilities &capabilities,
				const GLStateSet &last_applied_state_set,
				GLState &last_applied_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;


		GLuint d_attribute_index;
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
				GLenum mode) :
			d_mode(mode)
		{  }

		virtual
		void
		apply_state(
				const GLCapabilities &capabilities,
				const GLStateSet &last_applied_state_set,
				GLState &last_applied_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;


		GLenum d_mode;
	};

	/**
	 * Used for glHint.
	 */
	struct GLHintStateSet :
			public GLStateSet
	{
		GLHintStateSet(
				GLenum target,
				GLenum mode) :
			d_target(target),
			d_mode(mode)
		{  }

		virtual
		void
		apply_state(
				const GLCapabilities &capabilities,
				const GLStateSet &last_applied_state_set,
				GLState &last_applied_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;


		GLenum d_target;
		GLenum d_mode;
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
				const GLStateSet &last_applied_state_set,
				GLState &last_applied_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;


		GPlatesMaths::real_t d_width;
	};

	/**
	 * Used to load a modelview or projection matrix.
	 */
	struct GLLoadMatrixStateSet :
			public GLStateSet
	{
		GLLoadMatrixStateSet(
				GLenum mode,
				const GLMatrix &matrix) :
			d_mode(mode),
			d_matrix(matrix)
		{  }

		virtual
		void
		apply_state(
				const GLCapabilities &capabilities,
				const GLStateSet &last_applied_state_set,
				GLState &last_applied_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;


		GLenum d_mode;
		GLMatrix d_matrix;
	};

	/**
	 * Used to load a texture matrix.
	 */
	struct GLLoadTextureMatrixStateSet :
			public GLStateSet
	{
		GLLoadTextureMatrixStateSet(
				GLenum texture_unit,
				const GLMatrix &matrix) :
			d_texture_unit(texture_unit),
			d_matrix(matrix)
		{  }

		virtual
		void
		apply_state(
				const GLCapabilities &capabilities,
				const GLStateSet &last_applied_state_set,
				GLState &last_applied_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;


		GLenum d_texture_unit;
		GLMatrix d_matrix;
	};

	/**
	 * Used to specify matrix mode.
	 */
	struct GLMatrixModeStateSet :
			public GLStateSet
	{
		explicit
		GLMatrixModeStateSet(
				GLenum mode) :
			d_mode(mode)
		{  }

		virtual
		void
		apply_state(
				const GLCapabilities &capabilities,
				const GLStateSet &last_applied_state_set,
				GLState &last_applied_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;


		GLenum d_mode;
	};

	/**
	 * Used to set the vertex normal array source.
	 */
	struct GLNormalPointerStateSet :
			public GLStateSet
	{
		//! Binds to a vertex buffer object.
		GLNormalPointerStateSet(
				GLenum type,
				GLsizei stride,
				GLint offset,
				const GLBufferObject::shared_ptr_to_const_type &buffer_object) :
			d_buffer(offset, buffer_object),
			d_type(type),
			d_stride(stride)
		{  }

		//! No binding to a vertex buffer object (using client memory array).
		GLNormalPointerStateSet(
				GLenum type,
				GLsizei stride,
				GLint offset,
				const GLBufferImpl::shared_ptr_to_const_type &buffer_impl) :
			d_buffer(offset, buffer_impl),
			d_type(type),
			d_stride(stride)
		{  }

		virtual
		void
		apply_state(
				const GLCapabilities &capabilities,
				const GLStateSet &last_applied_state_set,
				GLState &last_applied_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;


	private:
		Implementation::GLVertexAttributeBuffer d_buffer;
		GLenum d_type;
		GLsizei d_stride;
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
				const GLStateSet &last_applied_state_set,
				GLState &last_applied_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;


		GPlatesMaths::real_t d_size;
	};

	/**
	 * Used to set the polygon mode.
	 *
	 * NOTE: Caller must detect 'GL_FRONT_AND_BACK' and specify separate objects for 'GL_FRONT' and 'GL_BACK'.
	 */
	struct GLPolygonModeStateSet :
			public GLStateSet
	{
		GLPolygonModeStateSet(
				GLenum face,
				GLenum mode) :
			d_face(face),
			d_mode(mode)
		{  }

		virtual
		void
		apply_state(
				const GLCapabilities &capabilities,
				const GLStateSet &last_applied_state_set,
				GLState &last_applied_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;


		GLenum d_face;
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
				const GLStateSet &last_applied_state_set,
				GLState &last_applied_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;


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
				const GLStateSet &last_applied_state_set,
				GLState &last_applied_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;


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
				const GLStateSet &last_applied_state_set,
				GLState &last_applied_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;


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
				const GLStateSet &last_applied_state_set,
				GLState &last_applied_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;


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
				const GLStateSet &last_applied_state_set,
				GLState &last_applied_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;


		GLenum d_fail;
		GLenum d_zfail;
		GLenum d_zpass;
	};

	/**
	 * Used to set the vertex texture coordinate array source.
	 */
	struct GLTexCoordPointerStateSet :
			public GLStateSet
	{
		//! Binds to a vertex buffer object.
		GLTexCoordPointerStateSet(
				GLint size,
				GLenum type,
				GLsizei stride,
				GLint offset,
				const GLBufferObject::shared_ptr_to_const_type &buffer_object,
				GLenum texture_unit) :
			d_buffer(offset, buffer_object),
			d_size(size),
			d_type(type),
			d_stride(stride),
			d_texture_unit(texture_unit)
		{  }

		//! No binding to a vertex buffer object (using client memory array).
		GLTexCoordPointerStateSet(
				GLint size,
				GLenum type,
				GLsizei stride,
				GLint offset,
				const GLBufferImpl::shared_ptr_to_const_type &buffer_impl,
				GLenum texture_unit) :
			d_buffer(offset, buffer_impl),
			d_size(size),
			d_type(type),
			d_stride(stride),
			d_texture_unit(texture_unit)
		{  }

		virtual
		void
		apply_state(
				const GLCapabilities &capabilities,
				const GLStateSet &last_applied_state_set,
				GLState &last_applied_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;


	private:
		Implementation::GLVertexAttributeBuffer d_buffer;
		GLint d_size;
		GLenum d_type;
		GLsizei d_stride;

		GLenum d_texture_unit;
	};

	/**
	 * Used set texture coordinate generation state.
	 */
	struct GLTexGenStateSet :
			public GLStateSet
	{
		//! 'ParamType' should be one of GLint, GLfloat, GLdouble, std::vector<GLint>, std::vector<GLfloat> or std::vector<GLdouble>.
		template <typename ParamType>
		GLTexGenStateSet(
				GLenum texture_unit,
				GLenum coord,
				GLenum pname,
				ParamType param) :
			d_texture_unit(texture_unit),
			d_coord(coord),
			d_pname(pname),
			d_param(param)
		{
			BOOST_MPL_ASSERT((boost::mpl::contains<param_data_types, ParamType>));
		}

		virtual
		void
		apply_state(
				const GLCapabilities &capabilities,
				const GLStateSet &last_applied_state_set,
				GLState &last_applied_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;

	private:
		//! The valid variant types.
		typedef boost::mpl::vector<
				GLint, GLfloat, GLdouble,
				std::vector<GLint>, std::vector<GLfloat>, std::vector<GLdouble>
						> param_data_types;

		//! The boost::variant itself.
		typedef boost::make_variant_over<param_data_types>::type param_type;

		static const param_type DEFAULT_GEN_MODE;
		static const param_type DEFAULT_S_PLANE;
		static const param_type DEFAULT_T_PLANE;
		static const param_type DEFAULT_R_AND_Q_PLANE;

		GLenum d_texture_unit;
		GLenum d_coord;
		GLenum d_pname;
		param_type d_param;


		static
		param_type
		initialise_plane(
				const GLdouble &x,
				const GLdouble &y,
				const GLdouble &z,
				const GLdouble &w);

		//! Returns the default @a param_type for 'this' state.
		param_type
		get_default_param() const;
	};

	/**
	 * Used set texture environment state.
	 */
	struct GLTexEnvStateSet :
			public GLStateSet
	{
		//! 'ParamType' should be one of GLint, GLfloat, std::vector<GLint> or std::vector<GLfloat>.
		template <typename ParamType>
		GLTexEnvStateSet(
				GLenum texture_unit,
				GLenum target,
				GLenum pname,
				ParamType param) :
			d_texture_unit(texture_unit),
			d_target(target),
			d_pname(pname),
			d_param(param)
		{
			BOOST_MPL_ASSERT((boost::mpl::contains<param_data_types, ParamType>));
		}

		virtual
		void
		apply_state(
				const GLCapabilities &capabilities,
				const GLStateSet &last_applied_state_set,
				GLState &last_applied_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;

	private:
		//! The valid variant types.
		typedef boost::mpl::vector<GLint, GLfloat, std::vector<GLint>, std::vector<GLfloat> > param_data_types;

		//! The boost::variant itself.
		typedef boost::make_variant_over<param_data_types>::type param_type;

		GLenum d_texture_unit;
		GLenum d_target;
		GLenum d_pname;
		param_type d_param;

		//! Returns the default @a param_type for 'this' state.
		param_type
		get_default_param() const;
	};

	/**
	 * Used to set the *generic* vertex attribute array source.
	 */
	struct GLVertexAttribPointerStateSet :
			public GLStateSet
	{
		//! The vertex attribute API to use...
		enum VertexAttribAPIType
		{
			VERTEX_ATTRIB_POINTER, // Used for 'glVertexAttribPointer'
			VERTEX_ATTRIB_I_POINTER, // Used for 'glVertexAttribIPointer'
			VERTEX_ATTRIB_L_POINTER // Used for 'glVertexAttribLPointer'
		};

		//! Binds to a vertex buffer object.
		GLVertexAttribPointerStateSet(
				const GLCapabilities &capabilities,
				GLuint attribute_index,
				VertexAttribAPIType vertex_attrib_api,
				GLint size,
				GLenum type,
				// Only used for 'VERTEX_ATTRIB_POINTER', not 'VERTEX_ATTRIB_I_POINTER' or 'VERTEX_ATTRIB_L_POINTER'...
				boost::optional<GLboolean> normalized,
				GLsizei stride,
				GLint offset,
				const GLBufferObject::shared_ptr_to_const_type &buffer_object);

		//! No binding to a vertex buffer object (using client memory array).
		GLVertexAttribPointerStateSet(
				const GLCapabilities &capabilities,
				GLuint attribute_index,
				VertexAttribAPIType vertex_attrib_api,
				GLint size,
				GLenum type,
				// Only used for 'VERTEX_ATTRIB_POINTER', not 'VERTEX_ATTRIB_I_POINTER' or 'VERTEX_ATTRIB_L_POINTER'...
				boost::optional<GLboolean> normalized,
				GLsizei stride,
				GLint offset,
				const GLBufferImpl::shared_ptr_to_const_type &buffer_impl);

		virtual
		void
		apply_state(
				const GLCapabilities &capabilities,
				const GLStateSet &last_applied_state_set,
				GLState &last_applied_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;


	private:
		Implementation::GLVertexAttributeBuffer d_buffer;
		GLuint d_attribute_index;
		VertexAttribAPIType d_vertex_attrib_api;
		GLint d_size;
		GLenum d_type;
		// Is optional since only used for 'glVertexAttribPointer' but not
		// 'glVertexAttribIPointer' or 'glVertexAttribLPointer'...
		boost::optional<GLboolean> d_normalized;
		GLsizei d_stride;
	};

	/**
	 * Used to set the vertex position array source.
	 */
	struct GLVertexPointerStateSet :
			public GLStateSet
	{
		//! Binds to a vertex buffer object.
		GLVertexPointerStateSet(
				GLint size,
				GLenum type,
				GLsizei stride,
				GLint offset,
				const GLBufferObject::shared_ptr_to_const_type &buffer_object) :
			d_buffer(offset, buffer_object),
			d_size(size),
			d_type(type),
			d_stride(stride)
		{  }

		//! No binding to a vertex buffer object (using client memory array).
		GLVertexPointerStateSet(
				GLint size,
				GLenum type,
				GLsizei stride,
				GLint offset,
				const GLBufferImpl::shared_ptr_to_const_type &buffer_impl) :
			d_buffer(offset, buffer_impl),
			d_size(size),
			d_type(type),
			d_stride(stride)
		{  }

		virtual
		void
		apply_state(
				const GLCapabilities &capabilities,
				const GLStateSet &last_applied_state_set,
				GLState &last_applied_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;


	private:
		Implementation::GLVertexAttributeBuffer d_buffer;
		GLint d_size;
		GLenum d_type;
		GLsizei d_stride;
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
				const GLStateSet &last_applied_state_set,
				GLState &last_applied_state) const;

		virtual
		void
		apply_from_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;

		virtual
		void
		apply_to_default_state(
				const GLCapabilities &capabilities,
				GLState &last_applied_state) const;

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
