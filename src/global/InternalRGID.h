/* $Id$ */

/**
 * \file 
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

#ifndef _GPLATES_GLOBAL_INTERNALRGID_H_
#define _GPLATES_GLOBAL_INTERNALRGID_H_

#include <iostream>

namespace GPlatesGlobal
{
	/**
	 * Instances of this class are used to represent rotation-group ids
	 * internally.  They are supposed to be more computationally efficient
	 * than the so-called "GPlates RGID", but they are never supposed to
	 * be visible outside GPlates calculations.
	 *
	 * Internal RGIDs do not survive between executions of GPlates:  each
	 * time GPlates powers up, it reads in the RGIDs of whichever input
	 * format it is reading, and internal RGIDs are allocated on a
	 * first-come-first-served basis.  The program will store a map of
	 * external RGIDs to internal RGIDs to avoid duplication, and a map
	 * of internal RGIDs to external RGIDs for output and display.
	 *
	 * The internal RGID of the globe will always be 0.
	 */
	class InternalRGID
	{
		public:
			InternalRGID(unsigned int i) : _id(i) {  }

			unsigned int id() const { return _id; }

		private:
			unsigned int _id;
	};


	inline bool
	operator==(InternalRGID i1, InternalRGID i2) {

		return (i1.id() == i2.id());
	}


	inline bool
	operator!=(InternalRGID i1, InternalRGID i2) {

		return (i1.id() != i2.id());
	}
}

#endif  // _GPLATES_GLOBAL_INTERNALRGID_H_
