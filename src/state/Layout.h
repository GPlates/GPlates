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

#ifndef _GPLATES_STATE_LAYOUT_H_
#define _GPLATES_STATE_LAYOUT_H_

#include <list>
#include <utility>  /* std::pair */

#include "geo/PointData.h"
#include "geo/LineData.h"
#include "maths/PointOnSphere.h"
#include "maths/PolyLineOnSphere.h"


namespace GPlatesState
{
	/**
	 * Provides access to the current geographical layout of geo-data.
	 */
	class Layout
	{
		public:
			typedef std::pair< const GPlatesGeo::PointData *,
			 GPlatesMaths::PointOnSphere > PointDataPos;

			typedef std::pair< const GPlatesGeo::LineData *,
			 GPlatesMaths::PolyLineOnSphere > LineDataPos;

			typedef std::list< PointDataPos > PointDataLayout;
			typedef std::list< LineDataPos > LineDataLayout;


			static PointDataLayout::const_iterator
			PointDataLayoutBegin() {

				EnsurePointDataLayoutExists();
				return _point_data_layout->begin();
			}


			static PointDataLayout::const_iterator
			PointDataLayoutEnd() {

				EnsurePointDataLayoutExists();
				return _point_data_layout->end();
			}


			static LineDataLayout::const_iterator
			LineDataLayoutBegin() {

				EnsureLineDataLayoutExists();
				return _line_data_layout->begin();
			}


			static LineDataLayout::const_iterator
			LineDataLayoutEnd() {

				EnsureLineDataLayoutExists();
				return _line_data_layout->end();
			}


			static void
			InsertPointDataPos(const GPlatesGeo::PointData *data,
			 const GPlatesMaths::PointOnSphere &position) {

				EnsurePointDataLayoutExists();
				_point_data_layout->push_back(
				 PointDataPos(data, position));
			}


			static void
			InsertLineDataPos(const GPlatesGeo::LineData *data,
			 const GPlatesMaths::PolyLineOnSphere &position) {

				EnsureLineDataLayoutExists();
				_line_data_layout->push_back(
				 LineDataPos(data, position));
			}


			static void
			Clear() {

				if (_point_data_layout) {

					delete _point_data_layout;
					_point_data_layout = NULL;
				}
				if (_line_data_layout) {

					delete _line_data_layout;
					_line_data_layout = NULL;
				}
			}

		private:

			static PointDataLayout *_point_data_layout;
			static LineDataLayout *_line_data_layout;


			/**
			 * Prohibit construction of this class.
			 */
			Layout();
			

			/**
			 * Prohibit copying of this class.
			 */
			Layout(const Layout&);


			static void
			EnsurePointDataLayoutExists() {

				if ( ! _point_data_layout) {

					_point_data_layout =
					 new PointDataLayout();
				}
			}


			static void
			EnsureLineDataLayoutExists() {

				if ( ! _line_data_layout) {

					_line_data_layout =
					 new LineDataLayout();
				}
			}
	};
}

#endif  /* _GPLATES_STATE_LAYOUT_H_ */
