/* $Id$ */

/**
 * @file 
 * Base class for different implementations of rendering text in OpenGL
 * to reduce the dependency on a particular method of rendering text a la
 * the Adapter pattern in Design Patterns (Gamma, et al.)
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_TEXTRENDERER_H
#define GPLATES_GUI_TEXTRENDERER_H

#include <QString>
#include <QFont>
#include <boost/shared_ptr.hpp>

#include "gui/Colour.h"

namespace GPlatesGui
{
	class TextRenderer
	{

	public:

		typedef boost::shared_ptr<const TextRenderer> ptr_to_const_type;

		virtual
		~TextRenderer()
		{  }

		/**
		 * Renders @a string at position (@a x , @a y ) in window coordinates
		 * using a particular @a colour and @a font.
		 */
		virtual
		void
		render_text(
				int x,
				int y,
				const QString &string,
				const GPlatesGui::Colour &colour,
				const QFont &font) const = 0;

		/**
		 * Renders @a string at position (@a x , @a y , @a z ) in scene coordinates
		 * using a particular @a colour and @a font.
		 * No occlusion testing is performed.
		 */
		virtual
		void
		render_text(
				double x,
				double y,
				double z,
				const QString &string,
				const GPlatesGui::Colour &colour,
				int x_offset,
				int y_offset,
				const QFont &font) const = 0;

	};
}

#endif  // GPLATES_GUI_TEXTRENDERER_H
