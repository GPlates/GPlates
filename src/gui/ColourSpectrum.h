/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_COLOURSPECTRUM_H
#define GPLATES_GUI_COLOURSPECTRUM_H

#include "Colour.h"
#include "model/types.h"
#include "model/Model.h"
#include "ColourTable.h"
#include "qt-widgets/ViewportWindow.h"
#include <vector>

namespace GPlatesGui 
{
	class ColourSpectrum
	{
	public:
		static
		ColourSpectrum *
		Instance();

		std::vector<GPlatesGui::Colour> &
		get_colour_spectrum();

	protected:
		/**
		 * Private constructor to enforce singleton design.
		 */
		ColourSpectrum();

	private:
		/**
		 * The singleton instance.
		 */
		static ColourSpectrum *d_instance;

		std::vector<GPlatesGui::Colour> d_colours;
	};
}

#endif  /* GPLATES_GUI_COLOURSPECTRUM_H */
