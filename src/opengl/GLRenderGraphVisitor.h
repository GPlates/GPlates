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
 
#ifndef GPLATES_OPENGL_GLRENDERGRAPHVISITOR_H
#define GPLATES_OPENGL_GLRENDERGRAPHVISITOR_H

#include <boost/noncopyable.hpp>

#include "utils/CopyConst.h"
#include "utils/non_null_intrusive_ptr.h"


namespace GPlatesOpenGL
{
	class GLRenderGraph;

	// Forward declaration of template visitor class.
	template <class GLRenderGraphType>
	class GLRenderGraphVisitorBase;


	/**
	 * Typedef for visitor over a non-const @a GLRenderGraph.
	 */
	typedef GLRenderGraphVisitorBase<GLRenderGraph>
			GLRenderGraphVisitor;

	/**
	 * Typedef for visitor over a const @a GLRenderGraph.
	 */
	typedef GLRenderGraphVisitorBase<const GLRenderGraph>
			ConstGLRenderGraphVisitor;


	// Forward declarations of visitable concrete render graph types.
	class GLMultiResolutionRasterNode;
	class GLMultiResolutionReconstructedRasterNode;
	class GLRenderGraphDrawableNode;
	class GLRenderGraphInternalNode;
	class GLText3DNode;
	class GLViewportNode;


	/**
	 * This class defines an abstract interface for a Visitor to visit a render graph.
	 *
	 * See the Visitor pattern (p.331) in Gamma95 for more information on the design and
	 * operation of this class.  This class corresponds to the abstract Visitor class in the
	 * pattern structure.
	 *
	 * @par Notes on the class implementation:
	 *  - All the virtual member "visit" functions have (empty) definitions for convenience, so
	 *    that derivations of this abstract base need only override the "visit" functions which
	 *    interest them.
	 */
	template <class GLRenderGraphType>
	class GLRenderGraphVisitorBase :
			private boost::noncopyable
	{
	public:
		//! Typedef for @a GLRenderGraph of appropriate const-ness.
		typedef GLRenderGraphType render_graph_type;

		//! Typedef for @a GLRenderGraphInternalNode of appropriate const-ness.
		typedef typename GPlatesUtils::CopyConst<
				GLRenderGraphType, GLRenderGraphInternalNode>::type
						render_graph_internal_node_type;

		//! Typedef for @a GLViewportNode of appropriate const-ness.
		typedef typename GPlatesUtils::CopyConst<
				GLRenderGraphType, GLViewportNode>::type
						viewport_node_type;

		//! Typedef for @a GLRenderGraphDrawableNode of appropriate const-ness.
		typedef typename GPlatesUtils::CopyConst<
				GLRenderGraphType, GLRenderGraphDrawableNode>::type
						render_graph_drawable_node_type;

		//! Typedef for @a GLMultiResolutionRasterNode of appropriate const-ness.
		typedef typename GPlatesUtils::CopyConst<
				GLRenderGraphType, GLMultiResolutionRasterNode>::type
						multi_resolution_raster_node_type;

		//! Typedef for @a GLMultiResolutionReconstructedRasterNode of appropriate const-ness.
		typedef typename GPlatesUtils::CopyConst<
				GLRenderGraphType, GLMultiResolutionReconstructedRasterNode>::type
						multi_resolution_reconstructed_raster_node_type;

		//! Typedef for @a GLText3DNode of appropriate const-ness.
		typedef typename GPlatesUtils::CopyConst<
				GLRenderGraphType, GLText3DNode>::type
						text_3d_node_type;


		// We'll make this function pure virtual so that the class is abstract.  The class
		// *should* be abstract, but wouldn't be unless we did this, since all the virtual
		// member functions have (empty) definitions.
		virtual
		~GLRenderGraphVisitorBase() = 0;


		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<render_graph_type> &)
		{  }


		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<render_graph_internal_node_type> &)
		{  }


		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<viewport_node_type> &)
		{  }


		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<render_graph_drawable_node_type> &)
		{  }


		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<multi_resolution_raster_node_type> &)
		{  }


		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<multi_resolution_reconstructed_raster_node_type> &)
		{  }


		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<text_3d_node_type> &)
		{  }
	};


	template <class GLRenderGraphType>
	inline
	GLRenderGraphVisitorBase<GLRenderGraphType>::~GLRenderGraphVisitorBase()
	{  }
}

#endif // GPLATES_OPENGL_GLRENDERGRAPHVISITOR_H
