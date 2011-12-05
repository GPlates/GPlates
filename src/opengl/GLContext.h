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
 
#ifndef GPLATES_OPENGL_GLCONTEXT_H
#define GPLATES_OPENGL_GLCONTEXT_H

#include <utility>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_comparison.hpp>
#include <opengl/OpenGL.h>
#include <QGLWidget>

#include "GLBufferObject.h"
#include "GLCompiledDrawState.h"
#include "GLFrameBufferObject.h"
#include "GLProgramObject.h"
#include "GLShaderObject.h"
#include "GLStateSetKeys.h"
#include "GLStateSetStore.h"
#include "GLTexture.h"
#include "GLVertexArray.h"
#include "GLVertexArrayObject.h"

#include "global/PointerTraits.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ObjectCache.h"
#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	class GLStateStore;
	class GLRenderer;

	/**
	 * Represents the OpenGL graphics state, transform state, texture state
	 * and drawables as a graph.
	 */
	class GLContext :
			public GPlatesUtils::ReferenceCount<GLContext>
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLContext.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLContext> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLContext.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLContext> non_null_ptr_to_const_type;


		/**
		 * Used to delegate to the real OpenGL context.
		 */
		class Impl
		{
		public:
			virtual
			~Impl()
			{  }

			//! Make this context the current context.
			virtual
			void
			make_current() = 0;

			//! The width of the window currently attached to the OpenGL context.
			virtual
			unsigned int
			get_window_width() const = 0;

			//! The height of the window currently attached to the OpenGL context.
			virtual
			unsigned int
			get_window_height() const = 0;
		};

		/**
		 * A derivation of @a Impl for QGLWidget.
		 */
		class QGLWidgetImpl :
				public Impl
		{
		public:
			explicit
			QGLWidgetImpl(
					QGLWidget &qgl_widget) :
				d_qgl_widget(qgl_widget)
			{  }

			virtual
			void
			make_current()
			{
				d_qgl_widget.makeCurrent();
			}

			virtual
			unsigned int
			get_window_width() const
			{
				return d_qgl_widget.width();
			}

			virtual
			unsigned int
			get_window_height() const
			{
				return d_qgl_widget.height();
			}

		private:
			QGLWidget &d_qgl_widget;
			friend class GLContext;
		};


		/**
		 * OpenGL state that can be shared between contexts (such as texture objects, vertex buffer objects, etc).
		 *
		 * Note that while native vertex array objects in OpenGL cannot be shared across contexts,
		 * the @a GLVertexArrayObject wrapper can (because internally it creates a native vertex
		 * array object for each context that it encounters - that uses it). Although the resource
		 * manager for it is still in @a NonSharedState in order that each native resource be released
		 * back to the context it was created in (ie, released when that context is active).
		 */
		class SharedState
		{
		public:
			//! Constructor.
			SharedState();

			/**
			 * Returns the texture resource manager.
			 */
			const boost::shared_ptr<GLTexture::resource_manager_type> &
			get_texture_object_resource_manager() const
			{
				return d_texture_object_resource_manager;
			}

			/**
			 * Returns the buffer object resource manager.
			 *
			 * NOTE: Only use if the GL_ARB_vertex_buffer_object extension is supported.
			 *
			 * The returned resource manager can create an OpenGL object that supports the buffer object
			 * OpenGL extension - well it's actually the GL_ARB_vertex_buffer_object extension because
			 * its first use was for vertex buffers but it has since been extended to other objects
			 * (such as pixel buffers, texture buffers).
			 */
			const boost::shared_ptr<GLBufferObject::resource_manager_type> &
			get_buffer_object_resource_manager() const
			{
				return d_buffer_object_resource_manager;
			}

			/**
			 * Returns the shader object resource manager for the specified shader type.
			 *
			 * @a shader_type can be GL_VERTEX_SHADER_ARB, GL_FRAGMENT_SHADER_ARB or GL_GEOMETRY_SHADER_ARB.
			 *
			 * Note that the 'GL_ARB_shader_objects' extension must be supported and also, for the
			 * three shader types above, the following extensions must also be supported:
			 *  - GL_ARB_vertex_shader (for GL_VERTEX_SHADER_ARB)... this is also core in OpenGL 2.0,
			 *  - GL_ARB_fragment_shader (for GL_FRAGMENT_SHADER_ARB)... this is also core in OpenGL 2.0,
			 *  - GL_ARB_geometry_shader4 (for GL_GEOMETRY_SHADER_ARB)... this is also core in OpenGL 3.2.
			 */
			const boost::shared_ptr<GLShaderObject::resource_manager_type> &
			get_shader_object_resource_manager(
					GLenum shader_type) const;

			/**
			 * Returns the shader program object resource manager.
			 *
			 * Note that the 'GL_ARB_shader_objects' extension must be supported.
			 */
			const boost::shared_ptr<GLProgramObject::resource_manager_type> &
			get_program_object_resource_manager() const;

			/**
			 * Returns a texture, from an internal cache, that matches the specified parameters.
			 *
			 * NOTE: Other texture parameters (such as filtering, etc) are not specified here so
			 * you will probably want to explicitly set all that state in the texture object.
			 * WARNING: The default value for GL_TEXTURE_MIN_FILTER is GL_NEAREST_MIPMAP_LINEAR which
			 * accesses mipmaps (and you may not have specified mipmaps because you didn't want them).
			 *
			 * Use this when you need a texture object temporarily and want to promote
			 * resource sharing by returning it for others to use.
			 *
			 * The dimensions must be a power-of-two.
			 * @a height and @a depth are not used for 1D textures (and @a depth not used for 2D textures).
			 *
			 * NOTE: The returned texture will have its level zero initialised (memory allocated for image)
			 * but the image data will be unspecified if it's the first time the texture object is returned.
			 * If @a mipmapped is true then all mipmap levels will also be initialised.
			 *
			 * NOTE: When all shared_ptr copies of the returned shared_ptr are released (destroyed)
			 * then the object will be returned to the internal cache for re-use and *not* destroyed.
			 * This is due to a custom deleter placed in boost::shared_ptr by the object cache.
			 */
			GLTexture::shared_ptr_type
			acquire_texture(
					GLRenderer &renderer,
					GLenum target,
					GLint internalformat,
					GLsizei width,
					boost::optional<GLsizei> height = boost::none, // Only used for 2D/3D textures.
					boost::optional<GLsizei> depth = boost::none,  // Only used for 3D textures.
					GLint border = 0,
					bool mipmapped = false);

			/**
			 * Returns a vertex array from an internal cache.
			 *
			 * NOTE: Even though vertex arrays cannot be shared across OpenGL contexts, the
			 * @a GLVertexArray (and @a GLVertexArrayObject) wrapper can be shared.
			 *
			 * Use this when you need a vertex array temporarily and want to promote
			 * resource sharing by returning it for others to use.
			 *
			 * NOTE: 'clear()' is called on the returned vertex array (just before returning)
			 * since the vertex attribute arrays enabled by other clients are unknown.
			 *
			 * NOTE: When all shared_ptr copies of the returned shared_ptr are released (destroyed)
			 * then the object will be returned to the internal cache for re-use and *not* destroyed.
			 * This is due to a custom deleter placed in boost::shared_ptr by the object cache.
			 */
			GLVertexArray::shared_ptr_type
			acquire_vertex_array(
					GLRenderer &renderer);

			/**
			 * Creates a compiled draw state that can render a full-screen textured quad.
			 *
			 * The returned compiled draw state can be used to draw a full-screen quad in order apply a
			 * texture to the screen-space of a render target.
			 *
			 * NOTE: This method maintains a single instance of a full-screen quad that clients can use.
			 * Which is why it is returned as a shared pointer to 'const'.
			 */
			GLCompiledDrawState::non_null_ptr_to_const_type
			get_full_screen_2D_textured_quad(
					GLRenderer &renderer);

			/**
			 * Creates a compiled draw state that specifies no bound vertex element buffer, no bound
			 * vertex attribute arrays (vertex buffers) and no enabled client vertex attribute state.
			 *
			 * This also includes the *generic* vertex attribute arrays (if 'GL_ARB_vertex_shader' supported).
			 *
			 * This method also makes state change detection in the renderer more efficient when
			 * binding vertex arrays (implementation detail: because the individual immutable GLStateSet
			 * objects, that specify unbound/disabled state, are shared by multiple vertex arrays resulting
			 * in a simple pointer comparison versus a virtual function call with comparison logic).
			 *
			 * So this method is built into @a GLVertexArray.
			 */
			GLCompiledDrawState::non_null_ptr_to_const_type
			get_unbound_vertex_array_compiled_draw_state(
					GLRenderer &renderer);

		private:
			//! Typedef for a key made up of the parameters of @a acquire_texture.
			typedef boost::tuple<GLenum, GLint, GLsizei, boost::optional<GLsizei>, boost::optional<GLsizei>, GLint, bool> texture_key_type;

			//! Typedef for a texture cache.
			typedef GPlatesUtils::ObjectCache<GLTexture> texture_cache_type;

			//! Typedef for a mapping of texture parameters (key) to texture caches.
			typedef std::map<texture_key_type, texture_cache_type::shared_ptr_type> texture_cache_map_type;


			boost::shared_ptr<GLTexture::resource_manager_type> d_texture_object_resource_manager;
			boost::shared_ptr<GLBufferObject::resource_manager_type> d_buffer_object_resource_manager;
			boost::shared_ptr<GLShaderObject::resource_manager_type> d_vertex_shader_object_resource_manager;
			boost::shared_ptr<GLShaderObject::resource_manager_type> d_fragment_shader_object_resource_manager;
			// The *geometry* shader object resource manager is optional since it may not be possible
			// to even create it if we have an old 'GLEW.h' file (since geometry shaders only added
			// relatively recently to OpenGL in version 3.2).
			boost::optional<boost::shared_ptr<GLShaderObject::resource_manager_type> > d_geometry_shader_object_resource_manager;

			boost::shared_ptr<GLProgramObject::resource_manager_type> d_program_object_resource_manager;

			texture_cache_map_type d_texture_cache_map;

			/**
			 * Even though vertex arrays cannot be shared across OpenGL contexts, the @a GLVertexArray
			 * (and @a GLVertexArrayObject) wrapper can be shared.
			 */
			GPlatesUtils::ObjectCache<GLVertexArray>::shared_ptr_type d_vertex_array_cache;

			/**
			 * Used by the renderer to efficiently allocate @a GLState objects.
			 *
			 * It's optional because we can't create it until we have a valid OpenGL context.
			 * NOTE: Access using @a get_state_store.
			 */
			boost::optional<boost::shared_ptr<GLStateStore> > d_state_store;

			/**
			 * The compiled draw state representing a full screen textured quad.
			 */
			boost::optional<GLCompiledDrawState::non_null_ptr_to_const_type> d_full_screen_2D_textured_quad;

			/**
			 * The compiled draw state representing a vertex array with no bound vertex attribute
			 * arrays (to vertex buffer objects), no bound vertex element buffer object and all
			 * vertex attribute client state disabled.
			 *
			 * NOTE: By sharing a single compiled draw state amongst multiple client vertex arrays
			 * we gain efficiency in detecting state changes.
			 * See comment on @a get_unbound_vertex_array_compiled_draw_state.
			 */
			boost::optional<GLCompiledDrawState::non_null_ptr_to_const_type> d_unbound_vertex_array_compiled_draw_state;

			// To access @a d_state_set_store and @a d_state_set_keys without exposing to clients.
			friend class GLContext;


			texture_cache_type::shared_ptr_type
			get_texture_cache(
					const texture_key_type &texture_key);

			//! Create state store if not yet done - an OpenGL context must be valid.
			boost::shared_ptr<GLStateStore>
			get_state_store();
		};


		/**
		 * OpenGL state that *cannot* be shared between contexts (such as vertex array objects, framebuffer objects).
		 *
		 * Note that while framebuffer objects cannot be shared across contexts their targets
		 * (textures and render buffers) can be shared across contexts.
		 */
		class NonSharedState
		{
		public:
			//! Constructor.
			NonSharedState();


			/**
			 * Returns the framebuffer object resource manager.
			 *
			 * NOTE: Only use if the GL_EXT_framebuffer_object extension is supported.
			 *
			 * Use this if you need to create your own framebuffer objects.
			 */
			const boost::shared_ptr<GLFrameBufferObject::resource_manager_type> &
			get_frame_buffer_object_resource_manager() const
			{
				return d_frame_buffer_object_resource_manager;
			}

			/**
			 * Returns a framebuffer object from an internal cache.
			 *
			 * Use this when you need a framebuffer object temporarily and want to promote
			 * resource sharing by returning it for others to use.
			 *
			 * NOTE: 'gl_detach_all()' is called on the returned framebuffer object (just before returning)
			 * since the attachments made by other clients are unknown.
			 *
			 * NOTE: When all shared_ptr copies of the returned shared_ptr are released (destroyed)
			 * then the object will be returned to the internal cache for re-use and *not* destroyed.
			 * This is due to a custom deleter placed in boost::shared_ptr by the object cache.
			 */
			GLFrameBufferObject::shared_ptr_type
			acquire_frame_buffer_object(
					GLRenderer &renderer);

			/**
			 * Returns the vertex array object resource manager.
			 *
			 * NOTE: Only use if the GL_ARB_vertex_array_object extension is supported.
			 *
			 * Use this if you need to create your own vertex array objects.
			 *
			 * Note that even though @a GLVertexArray (and hence @a GLVertexArrayObject) objects
			 * are cached in @a SharedState the allocator for native vertex array objects (resources)
			 * is here (in @a NonSharedState). This ensures the native resources are deallocated
			 * in the context they were allocated in (ie, deallocated when the correct context is active).
			 */
			const boost::shared_ptr<GLVertexArrayObject::resource_manager_type> &
			get_vertex_array_object_resource_manager() const
			{
				return d_vertex_array_object_resource_manager;
			}

		private:
			boost::shared_ptr<GLFrameBufferObject::resource_manager_type> d_frame_buffer_object_resource_manager;
			GPlatesUtils::ObjectCache<GLFrameBufferObject>::shared_ptr_type d_frame_buffer_object_cache;

			boost::shared_ptr<GLVertexArrayObject::resource_manager_type> d_vertex_array_object_resource_manager;
		};


		/**
		 * Creates a @a GLContext object.
		 */
		static
		non_null_ptr_type
		create(
				const boost::shared_ptr<Impl> &context_impl)
		{
			return non_null_ptr_type(new GLContext(context_impl));
		}


		/**
		 * Creates a @a GLContext object that shares state with another context.
		 */
		static
		non_null_ptr_type
		create(
				const boost::shared_ptr<Impl> &context_impl,
				GLContext &shared_context)
		{
			return non_null_ptr_type(new GLContext(context_impl, shared_context.get_shared_state()));
		}


		/**
		 * Initialises this @a GLContext.
		 *
		 * NOTE: An OpenGL context must be current before this is called.
		 */
		void
		initialise();


		/**
		 * Sets this context as the active OpenGL context.
		 */
		void
		make_current()
		{
			d_context_impl->make_current();
		}


		/**
		 * Creates a renderer.
		 *
		 * This is the interface through which OpenGL is used to draw.
		 *
		 * Typically a renderer is created each time a frame needs drawing.
		 */
		GPlatesGlobal::PointerTraits<GLRenderer>::non_null_ptr_type
		create_renderer();


		/**
		 * Returns the OpenGL state that can be shared with other OpenGL contexts.
		 *
		 * You can compare the pointers returned by @a get_shared_state on two
		 * different contexts to see if they are sharing or not.
		 */
		boost::shared_ptr<const SharedState>
		get_shared_state() const
		{
			return d_shared_state;
		}

		/**
		 * Returns the OpenGL state that can be shared with other OpenGL contexts.
		 *
		 * You can compare the pointers returned by @a get_shared_state on two
		 * different contexts to see if they are sharing or not.
		 */
		boost::shared_ptr<SharedState>
		get_shared_state()
		{
			return d_shared_state;
		}


		/**
		 * Returns the OpenGL state that *cannot* be shared with other OpenGL contexts.
		 */
		boost::shared_ptr<const NonSharedState>
		get_non_shared_state() const
		{
			return d_non_shared_state;
		}

		/**
		 * Returns the OpenGL state that *cannot* be shared with other OpenGL contexts.
		 */
		boost::shared_ptr<NonSharedState>
		get_non_shared_state()
		{
			return d_non_shared_state;
		}


		/**
		 * All implementation-dependent API parameters.
		 */
		struct Parameters
		{
			//! Parameters related to viewports.
			struct Viewport
			{
				Viewport();

				//! GL_MAX_VIEWPORTS query result - defaults to one.
				GLuint gl_max_viewports;

				//! Maximum supported width of viewport - is at least as large as display being renderer to.
				GLuint gl_max_viewport_width;

				//! Maximum supported height of viewport - is at least as large as display being renderer to.
				GLuint gl_max_viewport_height;
			};

			//! Parameters related to the framebuffers.
			struct Framebuffer
			{
				Framebuffer();

				//! Is GL_EXT_framebuffer_object supported?
				bool gl_EXT_framebuffer_object;

				//! GL_MAX_COLOR_ATTACHMENTS query result - zero if GL_EXT_framebuffer_object not supported.
				GLuint gl_max_color_attachments;
			};

			struct Shader
			{
				Shader();

				//! Is GL_ARB_shader_objects supported?
				bool gl_ARB_shader_objects;

				//! Is GL_ARB_vertex_shader supported?
				bool gl_ARB_vertex_shader;

				//! Is GL_ARB_fragment_shader supported?
				bool gl_ARB_fragment_shader;

				//! Is GL_ARB_geometry_shader4 supported?
				bool gl_ARB_geometry_shader4;

				//! Is GL_EXT_gpu_shader4 supported?
				bool gl_EXT_gpu_shader4;

				//! Is GL_ARB_gpu_shader_fp64 supported?
				bool gl_ARB_gpu_shader_fp64;

				//! Is GL_ARB_vertex_attrib_64bit supported?
				bool gl_ARB_vertex_attrib_64bit;

				/**
				 * The maximum number of generic vertex attributes supported by the
				 * GL_ARB_vertex_shader extension (or zero if it's not supported).
				 */
				GLuint gl_max_vertex_attribs; // GL_MAX_VERTEX_ATTRIBS_ARB query result
			};

			//! Parameters related to textures.
			struct Texture
			{
				Texture();

				/**
				 * Simply GL_TEXTURE0.
				 *
				 * This is here solely so we can include <GL/glew.h>, which defines
				 * GL_TEXTURE0, in "GLContext.cc" and hence avoid problems caused by
				 * including <GL/glew.h> in header files (because <GL/glew.h> must be included
				 * before OpenGL headers which means before Qt headers which is difficult).
				 */
				static const GLenum gl_texture0; // GL_TEXTURE0

				/**
				 * The minimum texture size (dimension) that all OpenGL implementations
				 * are required to support - this is without texture borders.
				 */
				static const GLuint gl_min_texture_size = 64;

				/**
				 * The maximum texture size (dimension) this OpenGL implementation/driver will support.
				 * This is without texture borders and will be a power-of-two.
				 *
				 * NOTE: This doesn't necessarily mean it will be hardware accelerated but
				 * it probably will be, especially if we use standard formats like GL_RGBA8.
				 */
				GLuint gl_max_texture_size; // GL_MAX_TEXTURE_SIZE query result

				//! Is GL_ARB_texture_non_power_of_two supported?
				bool gl_ARB_texture_non_power_of_two;

				/**
				 * The maximum number of texture units supported by the
				 * GL_ARB_multitexture extension (or one if it's not supported).
				 *
				 * NOTE: This is the 'old style' number of texture units where number of texture
				 * coordinates and number of texture images is the same.
				 *
				 * NOTE: This value should be used when using the fixed-function pipeline.
				 * For fragment shaders you can use @a gl_max_texture_image_units and
				 * @a gl_max_texture_coords which are either the same as @a gl_max_texture_units or larger.
				 * But you can *not* use them for the fixed-function pipeline.
				 */
				GLuint gl_max_texture_units; // GL_MAX_TEXTURE_UNITS query result

				/**
				 * The maximum number of texture *image* units supported by the
				 * GL_ARB_fragment_shader extension (or @a gl_max_texture_units if it's not supported).
				 *
				 * NOTE: This is the 'new style' number of texture units where number of texture
				 * *image* units differs to the number of texture coordinates.
				 *
				 * NOTE: This can be used for fragment shaders (can also use @a gl_max_texture_units
				 * but it's less than or equal to @a gl_max_texture_image_units).
				 */
				GLuint gl_max_texture_image_units; // GL_MAX_TEXTURE_IMAGE_UNITS query result

				/**
				 * The maximum number of texture coordinates supported by the
				 * GL_ARB_fragment_shader extension (or @a gl_max_texture_units if it's not supported).
				 *
				 * NOTE: This is the 'new style' number of texture units where number of texture
				 * *image* units differs to the number of texture coordinates.
				 *
				 * NOTE: This can be used for fragment shaders (can also use @a gl_max_texture_units
				 * but it's less than or equal to @a gl_max_texture_coords).
				 */
				GLuint gl_max_texture_coords; // GL_MAX_TEXTURE_COORDS query result

				/**
				 * The maximum texture filtering anisotropy supported by the
				 * GL_EXT_texture_filter_anisotropic extension (or 1.0 if it's not supported).
				 */
				GLfloat gl_texture_max_anisotropy; // GL_TEXTURE_MAX_ANISOTROPY query result

				/**
				 * Is GL_EXT_texture_edge_clamp supported?
				 *
				 * This is the standard texture clamping in Direct3D - it's easier for hardware to implement
				 * since it avoids accessing the texture border colour (even in (bi)linear filtering mode).
				 */
				bool gl_EXT_texture_edge_clamp;

				//! Is GL_SGIS_texture_edge_clamp supported? Same as GL_EXT_texture_edge_clamp extension really.
				bool gl_SGIS_texture_edge_clamp;

				//! Is GL_EXT_texture3D supported?
				bool gl_EXT_texture3D;

				//! Is GL_EXT_texture_array supported?
				bool gl_EXT_texture_array;

				//! The number of texture array layers supported - is 1 if GL_EXT_texture_array not supported.
				GLuint gl_max_texture_array_layers;

				//! Is GL_EXT_texture_buffer_object supported?
				bool gl_EXT_texture_buffer_object;

				//! Is GL_ARB_texture_float supported?
				bool gl_ARB_texture_float;

				//! Is GL_ARB_texture_rg supported?
				bool gl_ARB_texture_rg;

				/**
				 * Is GL_ARB_color_buffer_float supported?
				 *
				 * This affects things other than floating-point textures (samplers or render-targets) but
				 * we put it with the texture parameters since it's most directly related to floating-point
				 * colour buffers (eg, floating-point textures attached to a framebuffer object).
				 *
				 * Unfortunately for Mac OSX 10.5 (Leopard) this is not supported.
				 * It is supported in Snow Leopard (10.6), and above, however.
				 */
				bool gl_ARB_color_buffer_float;
			};

			//! Parameters related to buffer objects.
			struct Buffer
			{
				Buffer();

				//! Is GL_ARB_vertex_buffer_object supported?
				bool gl_ARB_vertex_buffer_object;

				//! Is GL_ARB_vertex_array_object supported?
				bool gl_ARB_vertex_array_object;

				//! Is GL_ARB_pixel_buffer_object supported?
				bool gl_ARB_pixel_buffer_object;

				//! Is GL_ARB_map_buffer_range supported?
				bool gl_ARB_map_buffer_range;

				//! Is GL_APPLE_flush_buffer_range supported?
				bool gl_APPLE_flush_buffer_range;
			};

			Viewport viewport;
			Framebuffer framebuffer;
			Shader shader;
			Texture texture;
			Buffer buffer;
		};

		/**
		 * Function to return implementation-dependent API parameters.
		 *
		 * @throws PreconditionViolationError if @a initialise not yet called.
		 * This method is static only to avoid having to pass around a @a GLContext.
		 */
		static
		const Parameters &
		get_parameters();


		/**
		 * Call this before rendering a scene.
		 *
		 * NOTE: Only @a GLRenderer need call this.
		 */
		void
		begin_render();


		/**
		 * Call this after rendering a scene.
		 *
		 * NOTE: Only @a GLRenderer need call this.
		 */
		void
		end_render();

	private:
		/**
		 * For delegating to the real OpenGL context.
		 */
		boost::shared_ptr<Impl> d_context_impl;

		/**
		 * OpenGL state that can be shared with another context.
		 */
		boost::shared_ptr<SharedState> d_shared_state;

		/**
		 * OpenGL state that *cannot* be shared with another context.
		 */
		boost::shared_ptr<NonSharedState> d_non_shared_state;

		/**
		 * Is true if the GLEW library has been initialised (if @a initialise has been called).
		 */
		static bool s_initialised_GLEW;

		/**
		 * Implementation-dependent API parameters.
		 */
		static boost::optional<Parameters> s_parameters;


		//! Constructor.
		explicit
		GLContext(
				const boost::shared_ptr<Impl> &context_impl) :
			d_context_impl(context_impl),
			d_shared_state(new SharedState()),
			d_non_shared_state(new NonSharedState())
		{  }

		//! Constructor.
		GLContext(
				const boost::shared_ptr<Impl> &context_impl,
				const boost::shared_ptr<SharedState> &shared_state) :
			d_context_impl(context_impl),
			d_shared_state(shared_state),
			d_non_shared_state(new NonSharedState())
		{  }

		//! Initialise implementation-dependent API parameters.
		void
		initialise_parameters();

		void
		initialise_viewport_parameters(
				Parameters::Viewport &viewport_parameters);

		void
		initialise_framebuffer_parameters(
				Parameters::Framebuffer &framebuffer_parameters);

		void
		initialise_shader_parameters(
				Parameters::Shader &shader_parameters);

		void
		initialise_texture_parameters(
				Parameters::Texture &texture_parameters);

		void
		initialise_buffer_parameters(
				Parameters::Buffer &buffer_parameters);

		/**
		 * Deallocates OpenGL objects that has been released but not yet destroyed/deallocated.
		 *
		 * They are queued for deallocation so that it can be done at a time when the OpenGL
		 * context is known to be active and hence OpenGL calls can be made.
		 */
		void
		deallocate_queued_object_resources();

		/**
		 * Disable specific OpenGL extensions.
		 */
		void
		disable_opengl_extensions();
	};
}

#endif // GPLATES_OPENGL_GLCONTEXT_H
