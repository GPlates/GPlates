/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006 The GPlates Consortium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <algorithm>  /* std::distance */
#include "GmlOrientableCurve.h"
#include "maths/LatLonPointConversions.h"
#include "maths/PolylineOnSphere.h"


struct TrailingCoordinateException {

	/**
	 * We want a size type large enough to hold the 'size' of an STL container.
	 */
	typedef unsigned long size_type;

	template<typename C>
	TrailingCoordinateException(
			const double &trailing_coord,
			const size_type &dimension,
			const C &container):
		d_trailing_coord(trailing_coord),
		d_dimension(dimension),
		d_container_length(static_cast<size_type>(container.size())) {  }

	/**
	 * This is the value of the trailing coordinate.
	 */
	double d_trailing_coord;

	/**
	 * This is the dimension of the coordinate input.
	 *
	 * For example: 2 for (lat, lon); 3 for (lat, lon, z-value); etc.
	 */
	size_type d_dimension;

	/**
	 * This is the length of the container.
	 */
	size_type d_container_length;
};


struct InvalidCoordinateInput {

	/**
	 * We want a size type large enough to hold the 'size' of an STL container.
	 */
	typedef unsigned long size_type;

	/**
	 * This is the type of the coordinate: latitude, longitude, etc.
	 */
	enum CoordinateType {
		LATITUDE,
		LONGITUDE
	};

	/**
	 * The template type @a I must be an input iterator.
	 */
	template<typename I>
	InvalidCoordinateInput(
			const GPlatesMaths::real_t &invalid_coordinate_value,
			I iter,
			I begin,
			CoordinateType coordinate_type):
		d_invalid_coordinate_value(invalid_coordinate_value),
		d_index(static_cast<size_type>(std::distance(begin, iter))),
		d_coordinate_type(coordinate_type) {  }

	/**
	 * This is the value of the invalid coordinate.
	 */
	GPlatesMaths::real_t d_invalid_coordinate_value;

	/**
	 * This is the index of the invalid longitude value in the collection.
	 */
	size_type d_index;

	/**
	 * This is the type of the coordinate: latitude, longitude, etc.
	 */
	CoordinateType d_coordinate_type;
};


namespace {

	/**
	 * Populate @a pos_vector with GPlatesMaths::PolylineOnSphere instances created from the
	 * longitude and latitude coordinates in @a gml_pos_list.
	 *
	 * Note that this function expects the longitude of a point to precede the latitude of the
	 * point.  The sequence of coordinates will be interpreted as: lon, lat, lon, lat, ...
	 *
	 * Note that any pre-existing elements in @a pos_vector will be removed during the course
	 * of the function (as if @a pos_vector had been cleared at the commencement of the
	 * function).
	 *
	 * This function is strongly exception-safe and exception neutral.
	 */
	void
	populate_point_on_sphere_vector_from_gml_pos_list(
			std::vector<GPlatesMaths::PointOnSphere> &pos_vector,
			const std::vector<double> &gml_pos_list)
	{
		using namespace ::GPlatesMaths;

		static const unsigned long dimension = 2;

		// Create a tmp, so that this function can easily be strongly exception-safe.
		std::vector<GPlatesMaths::PointOnSphere> tmp_pos_vector;

		// First, verify that the collection is of an even length (since each consecutive
		// pairing of doubles is a (lon, lat) point coordinate -- note that I'm assuming
		// point coordinates are specified as (lon, lat) based upon the Ron Lake GML book).
		if ((gml_pos_list.size() % dimension) != 0) {
			// Uh oh, it's an odd-length collection!
			const double &trailing_coord = *(gml_pos_list.end() - 1);
			throw TrailingCoordinateException(trailing_coord, dimension, gml_pos_list);
		}

		// Now that we have confirmed that the collection is of an even length, let's turn
		// all the (lon, lat) point coordinates into GPlatesMaths::PointOnSpheres.

		tmp_pos_vector.reserve(gml_pos_list.size() / dimension);

		// Note that, because we have already confirmed that the length of the collection
		// is even, we don't need to worry about the undefined behaviour described in
		// Figure 7.2 at the end of section 7.2.5 "Random Access Iterators" in Josuttis.
		std::vector<double>::const_iterator iter = gml_pos_list.begin();
		std::vector<double>::const_iterator end = gml_pos_list.end();
		for ( ; iter != end; iter += dimension) {
			real_t lon = *iter;
			real_t lat = *(iter + 1);
			if ( ! LatLonPoint::isValidLon(lon)) {
				throw InvalidCoordinateInput(lon, iter, gml_pos_list.begin(),
						InvalidCoordinateInput::LONGITUDE);
			}
			if ( ! LatLonPoint::isValidLat(lat)) {
				throw InvalidCoordinateInput(lat, iter + 1, gml_pos_list.begin(),
						InvalidCoordinateInput::LATITUDE);
			}
			LatLonPoint llp(lat, lon);
			tmp_pos_vector.push_back(
					LatLonPointConversions::convertLatLonPointToPointOnSphere(llp));
		}

		// Now that all the exception-throwing parts are behind us, swap the contents of
		// the vectors to modify the program state.
		pos_vector.swap(tmp_pos_vector);
	}

}


boost::intrusive_ptr<GPlatesModel::GmlOrientableCurve>
GPlatesModel::GmlOrientableCurve::create(
		const std::vector<double> &gml_pos_list)
{
	using namespace ::GPlatesMaths;

	std::vector<PointOnSphere> pos_vector;
	::populate_point_on_sphere_vector_from_gml_pos_list(pos_vector, gml_pos_list);
	boost::intrusive_ptr<PolylineOnSphere> polyline_ptr(PolylineOnSphere::create_on_heap(pos_vector));
	boost::intrusive_ptr<GmlOrientableCurve> orientable_curve_ptr(new GmlOrientableCurve(polyline_ptr));

	return orientable_curve_ptr;
}


// We define this constructor in this source file rather than in the header file, because the
// constructor needs access to the function 'intrusive_ptr_add_ref(PolylineOnSphere *)', which is
// declared in "maths/PolylineOnSphere.h", which we would rather not include in the header.
GPlatesModel::GmlOrientableCurve::GmlOrientableCurve(
		boost::intrusive_ptr<GPlatesMaths::PolylineOnSphere> polyline):
	PropertyValue(),
	d_polyline(polyline)
{  }


// We define this constructor in this source file rather than in the header file, because the
// constructor needs access to the function 'intrusive_ptr_add_ref(PolylineOnSphere *)', which is
// declared in "maths/PolylineOnSphere.h", which we would rather not include in the header.
GPlatesModel::GmlOrientableCurve::GmlOrientableCurve(
		const GmlOrientableCurve &other):
	PropertyValue(),
	d_polyline(other.d_polyline)
{  }


// We define this destructor in this source file rather than in the header file, because the
// destructor needs access to the function 'intrusive_ptr_release(PolylineOnSphere *)', which is
// declared in "maths/PolylineOnSphere.h", which we would rather not include in the header.
GPlatesModel::GmlOrientableCurve::~GmlOrientableCurve()
{  }
