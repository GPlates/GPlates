/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006 The University of Sydney, Australia
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

#ifndef _GPLATES_GLOBAL_INTERNALRID_H_
#define _GPLATES_GLOBAL_INTERNALRID_H_

#include <iostream>

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
	operator==(const InternalRID &i1, const InternalRID &i2) {

		return (i1.ival() == i2.ival());
	}


	inline bool
	operator!=(const InternalRID &i1, const InternalRID &i2) {

		return (i1.ival() != i2.ival());
	}


	/**
	 * Although this operation doesn't strictly make sense for an
	 * InternalRID, it is provided to enable InternalRIDs to be used
	 * as keys in STL maps.
	 */
	inline bool
	operator<(const InternalRID &i1, const InternalRID &i2) {

		return (i1.ival() < i2.ival());
	}


	/**
	 * Although this operation doesn't strictly make sense for an
	 * InternalRID, it is provided to enable client code to work out
	 * the "highest" InternalRID in a collection.
	 */
	inline bool
	operator>(const InternalRID &i1, const InternalRID &i2) {

		return (i1.ival() > i2.ival());
	}


	/**
	 * This operation is useful for debugging.
	 */
	inline std::ostream &
	operator<<(std::ostream &os, const InternalRID &i) {

		os << i.ival();
		return os;
	}
}

#endif  // _GPLATES_GLOBAL_INTERNALRID_H_
