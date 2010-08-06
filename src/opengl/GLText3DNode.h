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
 
#ifndef GPLATES_OPENGL_GLTEXT3DNODE_H
#define GPLATES_OPENGL_GLTEXT3DNODE_H

#include <QFont>
#include <QString>
#include <opengl/OpenGL.h>

#include "GLDrawable.h"
#include "GLRenderGraphNode.h"

#include "gui/Colour.h"
#include "gui/TextRenderer.h"


namespace GPlatesOpenGL
{
	class GLTransformState;

	/**
	 * A render graph node for drawing text.
	 */
	class GLText3DNode :
			public GLRenderGraphNode
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLText3DNode.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLText3DNode> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLText3DNode.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLText3DNode> non_null_ptr_to_const_type;


		/**
		 * Creates a @a GLText3DNode object.
		 */
		static
		non_null_ptr_type
		create(
				const GPlatesGui::TextRenderer::ptr_to_const_type &text_renderer,
				double x,
				double y,
				double z,
				const QString &string,
				const GPlatesGui::Colour &colour,
				int x_offset = 0,
				int y_offset = 0,
				const QFont &font = QFont(),
				float scale = 1.0f)
		{
			return non_null_ptr_type(
					new GLText3DNode(
							text_renderer, x, y, z, string, colour, x_offset, y_offset, font, scale));
		}


		/**
		 * Returns a text drawable.
		 */
		GLDrawable::non_null_ptr_to_const_type
		get_drawable(
				const GLTransformState &transform_state) const;


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
		GPlatesGui::TextRenderer::ptr_to_const_type d_text_renderer;

		double d_x;
		double d_y;
		double d_z;
		QString d_string;
		GPlatesGui::Colour d_colour;
		int d_x_offset;
		int d_y_offset;
		QFont d_font;
		float d_scale;


		//! Constructor.
		GLText3DNode(
				const GPlatesGui::TextRenderer::ptr_to_const_type &text_renderer,
				double x,
				double y,
				double z,
				const QString &string,
				const GPlatesGui::Colour &colour,
				int x_offset,
				int y_offset,
				const QFont &font,
				float scale);
	};
}

#endif // GPLATES_OPENGL_GLTEXT3DNODE_H
