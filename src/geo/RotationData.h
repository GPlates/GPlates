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
 *   Hamish Law <hlaw@es.usyd.edu.au>
 */

#ifndef _GPLATES_ROTATIONDATA_H_
#define _GPLATES_ROTATIONDATA_H_

#include "GeologicalData.h"
#include "TimeWindow.h"
#include "maths/Rotation.h"

namespace GPlatesGeo
{
	/** 
	 * A GPlatesMaths::Rotation with a time window and associated fixed 
	 * and moving 'plates'. 
	 */
	class RotationData
	{
		public:
			RotationData(const GPlatesMaths::Rotation& rot, 
				const TimeWindow& window, GeologicalData& fixed, 
				GeologicalData& moving);

			virtual
			~RotationData() { }

			/**
			 * Interchange the fixed and moving 'plates'.
			 * @todo Yet to be implemented.
			 */
			void
			SwapFixedMoving() { }

			virtual void
			Accept(Visitor& visitor) const { visitor.Visit(*this); }

		private:
			GPlatesMaths::Rotation _rotation;
			TimeWindow _time_window;
			GeologicalData& _fixed;
			GeologicalData& _moving;
	};
}

#endif  // _GPLATES_ROTATIONDATA_H_
