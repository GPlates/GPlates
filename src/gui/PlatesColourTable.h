/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Author$
 *   $Date$
 * 
 * Copyright (C) 2003 The GPlates Consortium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Authors:
 *   James Boyden <jboyden@geosci.usyd.edu.au>
 */

#ifndef _GPLATES_GUI_PLATESCOLOURTABLE_H_
#define _GPLATES_GUI_PLATESCOLOURTABLE_H_

#include "Colour.h"
#include "global/types.h"

namespace GPlatesGui
{
	class PlatesColourTable
	{
		public:
			typedef const Colour *const_iterator;

			static PlatesColourTable *Instance();

			~PlatesColourTable();

			const_iterator
			end() const { return NULL; }

			const_iterator
			lookup(const GPlatesGlobal::rid_t &id) const;

		protected:
			/**
			 * Private constructor to enforce singleton design.
			 */
			PlatesColourTable();

		private:
			static PlatesColourTable *_instance;

			Colour **_id_table;
			Colour  *_colours;
	};
}

#endif  /* _GPLATES_GUI_PLATESCOLOURTABLE_H_ */
