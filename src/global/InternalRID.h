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

#ifndef _GPLATES_GLOBAL_INTERNALRID_H_
#define _GPLATES_GLOBAL_INTERNALRID_H_

namespace GPlatesGlobal
{
	/**
	 * Instances of this class will be used internally to represent the
	 * rotation ids of rotating objects.
	 *
	 * Currently, the state of plate ids / rotation ids in GPlates and
	 * GPML is a bit of a mess.  This class will be used internally to
	 * remove the kinematic calculations from this mess.
	 *
	 * The internal RID of the Earth will always be 0.
	 */
	class InternalRID
	{
		public:
			InternalRID(unsigned int i) : _ival(i) {  }

			// no default constructor

			unsigned int ival() const { return _ival; }

		private:
			unsigned int _ival;
	};


	inline bool
	operator==(InternalRID i1, InternalRID i2) {

		return (i1.ival() == i2.ival());
	}


	inline bool
	operator!=(InternalRID i1, InternalRID i2) {

		return (i1.ival() != i2.ival());
	}


	/**
	 * Although this operation doesn't strictly make sense for an
	 * InternalRID, it is provided to enable InternalRIDs to be used
	 * as keys in STL maps.
	 */
	inline bool
	operator<(InternalRID i1, InternalRID i2) {

		return (i1.ival() < i2.ival());
	}
}

#endif  // _GPLATES_GLOBAL_INTERNALRID_H_
