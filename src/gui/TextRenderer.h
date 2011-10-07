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
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
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

#include "gui/Colour.h"

#include "utils/ReferenceCount.h"
#include "utils/non_null_intrusive_ptr.h"


namespace GPlatesGui
{
	class TextRenderer :
			public GPlatesUtils::ReferenceCount<TextRenderer>
	{
	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<TextRenderer> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const TextRenderer> non_null_ptr_to_const_type;

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
				const QFont &font = QFont(),
				float scale = 1.0f) const = 0;
	};
}

#endif  // GPLATES_GUI_TEXTRENDERER_H
