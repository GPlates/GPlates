/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
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
 */

#ifndef GPLATES_STATE_LAYOUT_H
#define GPLATES_STATE_LAYOUT_H

#include <list>
#include <utility>  /* std::pair */
#include <queue>  /* std::priority_queue */

#include "geo/PointData.h"
#include "geo/LineData.h"
#include "maths/PointOnSphere.h"
#include "maths/PolylineOnSphere.h"


namespace GPlatesState
{
	/**
	 * Provides access to the current geographical layout of geo-data.
	 */
	class Layout
	{
		public:

		       /*
			* The next few structs/functions constitute a hackity
			* hack hack to get interactive (mouse-clicky) selection
			* working.
			*/

		       enum DatumType {

			       POINT_DATUM,
			       LINE_DATUM
		       };

		       /**
			* This struct is intended to be used as the value_type
			* argument to an STL priority-queue containing the
			* DrawableData which are close to a given test-point.
			*/
		       struct CloseDatum {  // Datum is the singular, not Data

			       CloseDatum(
				GPlatesGeo::DrawableData *datum,
				DatumType datum_type,
				const GPlatesMaths::real_t &closeness) :
				m_datum(datum),
				m_datum_type(datum_type),
				m_closeness(closeness) {  }

			       bool
			       operator<(
				const CloseDatum &other) const {

				       return (m_closeness.isPreciselyLessThan(
					        other.m_closeness.dval()));
			       }

			       GPlatesGeo::DrawableData *m_datum;

			       DatumType m_datum_type;

			       GPlatesMaths::real_t m_closeness;

		       };


		       /**
			* Populate the supplied priority-queue
			* @a sorted_results with pointers to all the
			* DrawableData which are "close" to @a test_point.
			*
			* The measure of how "close" to @a test_point a piece
			* of DrawableData must be to be included in the results
			* is provided by @a closeness_inclusion_threshold,
			* which is assumed to contain a value very close to
			* (but strictly less than) 1.  The closer the value of
			* @a closeness_inclusion_threshold to 1, the closer a
			* piece of DrawableData must be to @a test_point to be
			* considered "close enough".  A useful value might be
			* about 0.9997 or 0.9998.
			*
			* The results in @a sorted_results will be sorted
			* according to their closeness to @a test_point.
			*/
		       static
		       void
		       find_close_data(
			std::priority_queue< CloseDatum > &sorted_results,
			const GPlatesMaths::PointOnSphere &test_point,
			const GPlatesMaths::real_t
			 &closeness_inclusion_threshold);

		       /*
			* End of hackity-hacks.
			*/

			typedef std::pair< GPlatesGeo::PointData *,
			 GPlatesMaths::PointOnSphere > PointDataPos;

			typedef std::pair< GPlatesGeo::LineData *,
			 GPlatesMaths::PolylineOnSphere > LineDataPos;

			typedef std::list< PointDataPos > PointDataLayout;
			typedef std::list< LineDataPos > LineDataLayout;


			static PointDataLayout::iterator
			PointDataLayoutBegin() {

				EnsurePointDataLayoutExists();
				return _point_data_layout->begin();
			}


			static PointDataLayout::iterator
			PointDataLayoutEnd() {

				EnsurePointDataLayoutExists();
				return _point_data_layout->end();
			}


			static LineDataLayout::iterator
			LineDataLayoutBegin() {

				EnsureLineDataLayoutExists();
				return _line_data_layout->begin();
			}


			static LineDataLayout::iterator
			LineDataLayoutEnd() {

				EnsureLineDataLayoutExists();
				return _line_data_layout->end();
			}


			static void
			InsertPointDataPos(GPlatesGeo::PointData *data,
			 const GPlatesMaths::PointOnSphere &position) {

				EnsurePointDataLayoutExists();
				_point_data_layout->push_back(
				 PointDataPos(data, position));
			}


			static void
			InsertLineDataPos(GPlatesGeo::LineData *data,
			 const GPlatesMaths::PolylineOnSphere &position) {

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

#endif  /* GPLATES_STATE_LAYOUT_H */
