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

#include <iostream>

namespace GPlatesGlobal
{
	/**
	 * Instances of this class are used to represent static floating-point 
	 * data used in the project.  By "static" is meant "will not change",
	 * ie. no arithmetic or other mathematical operations will be performed
	 * upon it;  any instance of this class is effectively nothing more
	 * than a storage-unit for fp data.
	 *
	 * For this reason, no arithmetic operators will be provided for it,
	 * and no mathematical functions (sin, cos, tan, sqrt, etc) will be
	 * implemented for it.
	 *
	 * It <i>will</i> have mathematical comparison operations and I/O
	 * operators, but this should be all it needs.  This class attempts 
	 * to avoid the problems associated with standard floating-point
	 * comparisons by providing "almost exact" comparisons instead of the
	 * "exact" comparisons provided by the standard floating-point types.
	 */
	class FPData
	{
			static const double Epsilon;
			static const double NegativeEpsilon;

			/*
			 * these functions are friends,
			 * to be able to access the above two members.
			 */
			friend bool operator==(FPData, FPData);
			friend bool operator!=(FPData, FPData);
			friend bool operator<=(FPData, FPData);
			friend bool operator>=(FPData, FPData);
			friend bool operator<(FPData, FPData);
			friend bool operator>(FPData, FPData);

			double _dval;

		public:
			FPData() : _dval(0.0) {  }

			FPData(double d) : _dval(d) {  }

			double dval() const { return _dval; }
	};


	inline bool
	operator==(FPData f1, FPData f2) {

		/*
		 * Allow difference between f1 and f2 to fall into a range
		 * instead of insisting upon an exact value.
		 * That range will be [-e, e].
		 */
		double d = f1.dval() - f2.dval();
		return (FPData::NegativeEpsilon <= d && d <= FPData::Epsilon);
	}


	inline bool
	operator!=(FPData f1, FPData f2) {

		/*
		 * Difference between f1 and f2 must lie *outside* range.
		 * This is necessary to maintain the logical invariant
		 * (a == b) iff not (a != b)
		 */
		double d = f1.dval() - f2.dval();
		return (FPData::NegativeEpsilon > d || d > FPData::Epsilon);
	}


	inline bool
	operator<=(FPData f1, FPData f2) {

		/*
		 * According to the logical invariant
		 * (a == b) implies (a <= b), the set of pairs (f1, f2) which
		 * cause this boolean comparison to evaluate to true must be
		 * a superset of the set of pairs which cause the equality
		 * comparison to return true.
		 */
		double d = f1.dval() - f2.dval();
		return (d <= FPData::Epsilon);
	}


	inline bool
	operator>=(FPData f1, FPData f2) {

		/*
		 * According to the logical invariant
		 * (a == b) implies (a >= b), the set of pairs (f1, f2) which
		 * cause this boolean comparison to evaluate to true must be
		 * a superset of the set of pairs which cause the equality
		 * comparison to return true.
		 */
		double d = f1.dval() - f2.dval();
		return (FPData::NegativeEpsilon <= d);
	}


	inline bool
	operator>(FPData f1, FPData f2) {

		/*
		 * (a > b) must be the logical inverse of (a <= b).
		 */
		double d = f1.dval() - f2.dval();
		return (d > FPData::Epsilon);
	}


	inline bool
	operator<(FPData f1, FPData f2) {

		/*
		 * (a < b) must be the logical inverse of (a >= b).
		 */
		double d = f1.dval() - f2.dval();
		return (FPData::NegativeEpsilon > d);
	}


	inline std::ostream &
	operator<<(std::ostream &os, FPData f) {

		os << f.dval();
		return os;
	}


	inline std::istream &
	operator>>(std::istream &is, FPData &f) {

		double d;
		is >> d;
		f = FPData(d);
		return is;
	}
}

#endif  // _GPLATES_GLOBAL_FPDATA_H_
