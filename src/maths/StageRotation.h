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

#ifndef _GPLATES_MATHS_STAGEROTATION_H_
#define _GPLATES_MATHS_STAGEROTATION_H_

#include "UnitQuaternion3D.h"
#include "FiniteRotation.h"
#include "types.h"  /* real_t */


namespace GPlatesMaths
{
	using namespace GPlatesGlobal;


	/** 
	 * Represents a so-called "stage rotation" of plate tectonics.
	 *
	 * If a "finite rotation" represents the particular rotation used
	 * to transform a point on the sphere from the present back to a
	 * particular point in time, a stage rotation can be considered
	 * the delta between a pair of finite rotations.  It represents
	 * the change in rotation over the change in time.  [Note that
	 * the stage rotation itself is not a time-derivative, but it
	 * could be very easily used to calculate a time-derivative.]
	 *
	 * Alternately, if a finite rotation is considered as a point in a
	 * 4-dimensional rotation-space, a stage rotation is a displacement
	 * between two points.
	 */
	class StageRotation
	{
		public:
			/**
			 * Create a stage rotation consisting of the given
			 * unit quaternion and the given change in time.
			 * The change in time is in Millions of years.
			 */
			StageRotation(const UnitQuaternion3D &uq,
			              const real_t &time_delta)

			 : _quat(uq), _time_delta(time_delta) {  }


			UnitQuaternion3D
			quat() const { return _quat; }


			real_t
			timeDelta() const { return _time_delta; }

		private:
			UnitQuaternion3D _quat;
			real_t           _time_delta;  // Millions of years
	};

}

#endif  // _GPLATES_MATHS_STAGEROTATION_H_
