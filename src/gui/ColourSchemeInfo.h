/* $Id$ */

/**
 * @file 
 * Contains the defintion of the ColourSchemeContainer class.
 * 
 * Most recent change:
 *   $Date$
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

#ifndef GPLATES_GUI_COLOURSCHEMEINFO_H
#define GPLATES_GUI_COLOURSCHEMEINFO_H

#include <QString>

#include "ColourScheme.h"


namespace GPlatesGui
{
	/**
	 * A convenience structure for use by ColourSchemeContainer to hold
	 * information about a loaded colour scheme.
	 */
	struct ColourSchemeInfo
	{
		/**
		 * Constructs a ColourSchemeInfo.
		 */
		ColourSchemeInfo(
				ColourScheme::non_null_ptr_type colour_scheme_ptr_,
				QString short_description_,
				QString long_description_,
				bool is_built_in_);

		/**
		 * A pointer to the colour scheme.
		 */
		ColourScheme::non_null_ptr_type colour_scheme_ptr;

		/**
		 * A short, human-readable description of the colour scheme.
		 *
		 * This is used by the colouring dialog for the label underneath the colour
		 * scheme's preview.
		 */
		QString short_description;

		/**
		 * A longer, human-readable description of the colour scheme.
		 *
		 * This is used by the colouring dialog for the tooltip over the colour
		 * scheme's preview.
		 */
		QString long_description;

		/**
		 * True if the colour scheme is a built-in scheme. Built-in schemes are those
		 * that are present on application start-up by default. Built-in schemes
		 * cannot be unloaded.
		 */
		bool is_built_in;
	};
}

#endif  // GPLATES_GUI_COLOURSCHEMEINFO_H
