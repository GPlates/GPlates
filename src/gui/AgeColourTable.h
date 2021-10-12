/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2009 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_AGECOLOURTABLE_H
#define GPLATES_GUI_AGECOLOURTABLE_H

#include <vector>

#include "ColourTable.h"


namespace GPlatesAppLogic
{
	class Reconstruct;
}

namespace GPlatesPropertyValues
{
	class GeoTimeInstant;
}

namespace GPlatesGui 
{
	class AgeColourTable:
			public ColourTable
	{
	public:
		static
		AgeColourTable *
		Instance();

		virtual
		~AgeColourTable()
		{  }

		virtual
		ColourTable::const_iterator
		lookup(
				const GPlatesModel::ReconstructionGeometry &reconstruction_geometry) const;

		ColourTable::const_iterator
		lookup_by_age(
				const GPlatesPropertyValues::GeoTimeInstant &geo_time) const;

		void 
		set_reconstruct_state(
				const GPlatesAppLogic::Reconstruct &reconstruct);

		void 
		set_colour_scale_factor(
				int factor);

	protected:
		/**
		 * Private constructor to enforce singleton design.
		 */
		AgeColourTable();

	private:
		/**
		 * The singleton instance.
		 */
		static AgeColourTable *d_instance;

		std::vector<GPlatesGui::Colour> *d_colours;		

		const GPlatesAppLogic::Reconstruct *d_reconstruct;

		int d_colour_scale_factor;
	};
}

#endif  /* GPLATES_GUI_AGECOLOURTABLE_H */
