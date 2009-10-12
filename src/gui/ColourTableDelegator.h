/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
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

#ifndef GPLATES_GUI_COLOURTABLEDELEGATOR_H
#define GPLATES_GUI_COLOURTABLEDELEGATOR_H

#include "ColourTable.h"


namespace GPlatesModel
{
	class ReconstructionGeometry;
}

namespace GPlatesGui
{
	/**
	 * Keeps track of changing target colour tables - allows switching
	 * of the actual colour table implementation without having to change
	 * reference(s) to it (just refer to @a ColourTableDelegator instead).
	 */
	class ColourTableDelegator :
			public ColourTable
	{
	public:
		//! Set the initial target colour table.
		explicit
		ColourTableDelegator(
				ColourTable *colour_table) :
			d_colour_table(colour_table)
		{  }


		//! Change the target colour table.
		void
		set_target_colour_table(
				ColourTable *colour_table)
		{
			d_colour_table = colour_table;
		}


		//! Delegate colour lookup to target colour table.
		virtual
		const_iterator
		lookup(
				const GPlatesModel::ReconstructionGeometry &reconstruction_geometry) const
		{
			return d_colour_table->lookup(reconstruction_geometry);
		}

	private:
		ColourTable *d_colour_table;
	};
}

#endif // GPLATES_GUI_COLOURTABLEDELEGATOR_H
