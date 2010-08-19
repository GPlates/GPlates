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
 
#ifndef GPLATES_OPENGL_GLMULTIRESOLUTIONRECONSTRUCTEDRASTERNODE_H
#define GPLATES_OPENGL_GLMULTIRESOLUTIONRECONSTRUCTEDRASTERNODE_H
#define GPLATES_OPENGL_GLMULTIRESOLUTIONRASTERNODE_H

#include <opengl/OpenGL.h>

#include "GLMultiResolutionReconstructedRaster.h"
#include "GLRenderGraphNode.h"
#include "GLRenderGraphVisitor.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	/**
	 * A render graph node for a multi-resolution reconstructed reconstructed_raster.
	 */
	class GLMultiResolutionReconstructedRasterNode :
			public GLRenderGraphNode
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLMultiResolutionReconstructedRasterNode.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLMultiResolutionReconstructedRasterNode> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLMultiResolutionReconstructedRasterNode.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLMultiResolutionReconstructedRasterNode> non_null_ptr_to_const_type;


		/**
		 * Creates a @a GLMultiResolutionReconstructedRasterNode object.
		 */
		static
		non_null_ptr_type
		create(
				const GLMultiResolutionReconstructedRaster::non_null_ptr_type &reconstructed_raster)
		{
			return non_null_ptr_type(new GLMultiResolutionReconstructedRasterNode(reconstructed_raster));
		}


		/**
		 * Returns multi-resolution reconstructed raster.
		 */
		GLMultiResolutionReconstructedRaster::non_null_ptr_type
		get_multi_resolution_reconstructed_raster() const
		{
			return d_reconstructed_raster;
		}


		/**
		 * Accept a ConstGLRenderGraphVisitor instance.
		 */
		virtual
		void
		accept_visitor(
				ConstGLRenderGraphVisitor &visitor) const
		{
			visitor.visit(GPlatesUtils::get_non_null_pointer(this));
		}


		/**
		 * Accept a GLRenderGraphVisitor instance.
		 */
		virtual
		void
		accept_visitor(
				GLRenderGraphVisitor &visitor)
		{
			visitor.visit(GPlatesUtils::get_non_null_pointer(this));
		}

	private:
		GLMultiResolutionReconstructedRaster::non_null_ptr_type d_reconstructed_raster;


		//! Constructor.
		explicit
		GLMultiResolutionReconstructedRasterNode(
				const GLMultiResolutionReconstructedRaster::non_null_ptr_type &raster) :
			d_reconstructed_raster(raster)
		{  }
	};
}

#endif // GPLATES_OPENGL_GLMULTIRESOLUTIONRECONSTRUCTEDRASTERNODE_H
