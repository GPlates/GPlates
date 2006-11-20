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

#ifndef _GPLATES_GEO_POINTDATA_H_
#define _GPLATES_GEO_POINTDATA_H_

#include "DrawableData.h"
#include "maths/PointOnSphere.h"

namespace GPlatesGeo
{
	/** 
	 * A PointOnSphere augmented with GeneralisedData.
	 */
	class PointData : public DrawableData
	{
		public:
			PointData(const DataType_t &,
			          const RotationGroupId_t &,
			          const TimeWindow &,
				  const std::string &first_header_line,
				  const std::string &second_header_line,
			          const Attributes_t &,
			          const GPlatesMaths::PointOnSphere &);

			virtual void
			Accept(Visitor& visitor) const { visitor.Visit(*this); }

			GPlatesMaths::PointOnSphere
			GetPointOnSphere() const { return _point; }

			void
			Draw();

			void
			RotateAndDraw(const GPlatesMaths::FiniteRotation &);

		protected:
			virtual GPlatesMaths::real_t
			proximity(const GPlatesMaths::PointOnSphere &pos) 
			 const {

				return GPlatesMaths::dot(
				 _point.position_vector(), pos.position_vector());
			}

		private:
			/** 
			 * The location of the data.
			 */
			GPlatesMaths::PointOnSphere _point;
	};
}

#endif  // _GPLATES_GEO_POINTDATA_H_
