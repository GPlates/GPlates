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

#ifndef GPLATES_GUI_COLOURTABLE_H
#define GPLATES_GUI_COLOURTABLE_H

#include "Colour.h"


namespace GPlatesModel
{
	class ReconstructedFeatureGeometry;
}

namespace GPlatesGui
{
	class ColourTable
	{
	public:
		typedef const Colour *const_iterator;

		virtual
		~ColourTable()
		{  }
   
		const_iterator
		end() const
		{
			return NULL;
		}

		virtual
		const_iterator
		lookup(
				const GPlatesModel::ReconstructedFeatureGeometry &feature) const = 0;
	};
}

#endif  /* GPLATES_GUI_COLOURTABLE_H */
