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
 
#ifndef GPLATES_QTWIDGETS_COLOURSCHEMEINFO_H
#define GPLATES_QTWIDGETS_COLOURSCHEMEINFO_H

#include <QString>

#include "gui/ColourScheme.h"


namespace GPlatesQtWidgets
{
	/**
	 * ColourSchemeInfo holds information about a loaded colour scheme.
	 * 
	 * This is a convenience structure to share information between the
	 * ColouringDialog and PaletteSelectionWidget.
	 */
	struct ColourSchemeInfo
	{
		ColourSchemeInfo(
				GPlatesGui::ColourScheme::non_null_ptr_type colour_scheme_ptr_,
				QString label_text_,
				QString tooltip_text_,
				bool is_built_in_) :
			colour_scheme_ptr(colour_scheme_ptr_),
			label_text(label_text_),
			tooltip_text(tooltip_text_),
			is_built_in(is_built_in_)
		{
		}

		/**
		 * A pointer to the colour scheme.
		 */
		GPlatesGui::ColourScheme::non_null_ptr_type colour_scheme_ptr;

		/**
		 * The text used to label the preview of the colour scheme.
		 */
		QString label_text;

		/**
		 * The text used as the tooltip over the preview of the colour scheme.
		 */
		QString tooltip_text;

		/**
		 * True if the colour scheme is a built-in scheme. Built-in schemes cannot
		 * be removed from the dialog.
		 */
		bool is_built_in;
	};
}

#endif  // GPLATES_QTWIDGETS_COLOURSCHEMEINFO_H
