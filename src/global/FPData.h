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

#ifndef _GPLATES_GLOBAL_FPDATA_H_
#define _GPLATES_GLOBAL_FPDATA_H_

namespace GPlatesGlobal
{
	/**
	 * This class is used to represent any static floating-point data
	 * used in the project.  By "static" is meant "will not change",
	 * ie. no arithmetic or other mathematical operations will be
	 * performed upon it.  For this reason, no arithmetic operators
	 * will be provided for it, and no mathematical functions (sin,
	 * cos, tan, sqrt, etc) will be implemented for it.
	 *
	 * It <i>will</i> have mathematical comparison operations and
	 * I/O operators, but this should be all it needs.
	 */
	class FPData
	{
			double _dval;

		public:
			FPData(double d) : _dval(d) {  }

			double dval() const { return _dval; }
	};


	inline bool
	operator==(FPData fpd1, FPData fpd2) {

		return (fpd1.dval() == fpd2.dval());
	}


	inline bool
	operator!=(FPData fpd1, FPData fpd2) {

		return (fpd1.dval() != fpd2.dval());
	}


	inline bool
	operator<=(FPData fpd1, FPData fpd2) {

		return (fpd1.dval() <= fpd2.dval());
	}


	inline bool
	operator>=(FPData fpd1, FPData fpd2) {

		return (fpd1.dval() >= fpd2.dval());
	}


	inline bool
	operator>(FPData fpd1, FPData fpd2) {

		return (fpd1.dval() > fpd2.dval());
	}


	inline bool
	operator<(FPData fpd1, FPData fpd2) {

		return (fpd1.dval() < fpd2.dval());
	}


	inline std::ostream &
	operator<<(std::ostream &os, FPData fpd) {

		os << fpd.dval();
		return os;
	}


	inline std::ostream &
	operator>>(std::ostream &os, FPData &fpd) {

		double d;
		os >> d;
		fpd = FPData(d);
		return os;
	}
}

#endif  // _GPLATES_GLOBAL_FPDATA_H_
