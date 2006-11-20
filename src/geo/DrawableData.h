/* $Id$ */

/**
 * @file 
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

#ifndef _GPLATES_GEO_DRAWABLEDATA_H_
#define _GPLATES_GEO_DRAWABLEDATA_H_

#include <string>
#include "GeologicalData.h"
#include "maths/FiniteRotation.h"

namespace GPlatesGeo
{
	/**
	 * Being a child of this class signifies that the child
	 * is "drawable", ie. it can be displayed on the screen.
	 */
	class DrawableData : public GeologicalData
	{
		public:
			virtual void
			Draw() = 0;

			virtual void
			RotateAndDraw(const GPlatesMaths::FiniteRotation &) = 0;

			const std::string &
			FirstHeaderLine() const {

				return _first_header_line;
			}

			const std::string &
			SecondHeaderLine() const {

				return _second_header_line;
			}

			bool
			ShouldBePainted() const {

				return _should_be_painted;
			}

			void
			SetShouldBePainted(bool should_be_painted) {

				_should_be_painted = should_be_painted;
			}

			static GPlatesMaths::real_t
			ProximityToPointOnSphere(
			 const DrawableData *data,
			 const GPlatesMaths::PointOnSphere &pos) {

				return data->proximity(pos);
			}

		protected:
			DrawableData(const DataType_t& dt,
			             const RotationGroupId_t& rg,
			             const TimeWindow& tw,
				     const std::string &first_header_line,
				     const std::string &second_header_line,
			             const Attributes_t& attrs) :
			 GeologicalData(dt, rg, tw, attrs),
			 _first_header_line(first_header_line),
			 _second_header_line(second_header_line),
			 _should_be_painted(true) {  }

			virtual GPlatesMaths::real_t
			proximity(const GPlatesMaths::PointOnSphere &pos) 
			 const = 0;

		private:

			std::string _first_header_line, _second_header_line;

			/*
			 * Should this item of data be painted when the globe
			 * and its contents are painted onto the screen?  This
			 * is a hack to enable items to be temporarily made
			 * invisible.
			 */
			bool _should_be_painted;
	};
}

#endif  /* _GPLATES_GEO_DRAWABLEDATA_H_ */
