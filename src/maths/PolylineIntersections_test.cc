/* $Id$ */

/**
 * @file 
 * This file contains automated regression tests for the function
 * @a PolylineIntersections::partition_intersecting_polylines.
 *
 * To execute these tests, move this file to "../gui/GPlatesApp.cc" (yes,
 * replacing the one which is already there), recompile and run GPlates.
 * Voila!
 *
 * Note that this file should not be compiled into GPlates @em unless it is
 * replacing "../gui/GPlatesApp.cc".
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2005, 2006 The University of Sydney, Australia
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

#include <iostream>
#include <vector>
#include <list>
#include <algorithm>  /* copy */
#include <iterator>  /* ostream_iterator */
#include <wx/wx.h>
#include <wx/cmdline.h>

#include "global/config.h"
#include "global/Exception.h"

#include "GPlatesApp.h"
#include "MainWindow.h"
#include "controls/Lifetime.h"
#include "controls/File.h"

#include "maths/PolylineIntersections.h"
#include "maths/PolylineEquivalencePredicates.h"
#include "maths/LatLonPointConversions.h"
#include "maths/HighPrecision.h"


class PointIsCoincident:
 public std::unary_function< GPlatesMaths::PointOnSphere, bool > {

 public:

	PointIsCoincident(
	 const GPlatesMaths::PointOnSphere &p) :
	 d_p(p) {  }

	bool
	operator()(
	 const GPlatesMaths::PointOnSphere &other_p) const {

		return points_are_coincident(d_p, other_p);
	}

 private:

	const GPlatesMaths::PointOnSphere d_p;

};


bool
sets_of_polyline_are_undirected_equivalent(
 const std::list< GPlatesMaths::PolylineOnSphere > &s1,
 const std::list< GPlatesMaths::PolylineOnSphere > &s2) {

	if (s1.size() != s2.size()) {

		// There is no way the two sets can be equivalent.
		return false;
	}

	std::list< GPlatesMaths::PolylineOnSphere > s2_dup(s2);
	std::list< GPlatesMaths::PolylineOnSphere >::const_iterator
	 s1_iter = s1.begin(),
	 s1_end = s1.end();
	for ( ; s1_iter != s1_end; ++s1_iter) {

		GPlatesMaths::PolylineIsUndirectedEquivalentRef pred(*s1_iter);
		std::list< GPlatesMaths::PolylineOnSphere >::iterator found =
		 std::find_if(s2_dup.begin(), s2_dup.end(), pred);
		if (found != s2_dup.end()) {

			// The polyline pointed-to by 'found' must be
			// undirected-equivalent to the polyline pointed-to by
			// 's1_iter'.  So let's metaphorically cross it off the
			// list, and keep testing.
			s2_dup.erase(found);

		} else {

			// Couldn't find any polyline in 's2_dup' which was
			// undirected-equivalent to the polyline pointed-to by
			// 's1_iter'.  Therefore, the two sets of polylines
			// cannot be undirected-equivalent.
			return false;
		}
	}
	return true;
}


template< typename T, typename P >
bool
sets_of_T_are_equivalent_by_predicate(
 const std::list< T > &s1,
 const std::list< T > &s2) {

	if (s1.size() != s2.size()) {

		// There is no way the two sets can be equivalent.
		return false;
	}

	std::list< T > s2_dup(s2);
	typename std::list< T >::const_iterator
	 s1_iter = s1.begin(),
	 s1_end = s1.end();
	for ( ; s1_iter != s1_end; ++s1_iter) {

		P pred(*s1_iter);
		typename std::list< T >::iterator found =
		 std::find_if(s2_dup.begin(), s2_dup.end(), pred);
		if (found != s2_dup.end()) {

			s2_dup.erase(found);

		} else {

			return false;
		}
	}
	return true;
}


enum PointListType {

	POINT_TYPE = 0,
	EOP_TYPE,  // End Of Poly
	EOL_TYPE   // End Of List
};

struct Point {

	GPlatesMaths::real_t lat;
	GPlatesMaths::real_t lon;

	PointListType point_list_type;

};


inline
GPlatesMaths::PointOnSphere
Point_to_PointOnSphere(
 const Point &p) {

	using namespace GPlatesMaths;

	LatLonPoint llp(p.lat, p.lon);
	PointOnSphere pos = make_point_on_sphere(llp);

	return pos;
}


class PointOnSphereOstreamIterator: public std::iterator<
 std::output_iterator_tag, GPlatesMaths::PointOnSphere > {

 public:

	PointOnSphereOstreamIterator(
	 std::ostream &os,
	 const char *delim = "") :
	 d_os_ptr(&os),
	 d_delim(delim) {  }

	PointOnSphereOstreamIterator &
	operator*() {

		return *this;
	}

	void
	operator=(
	 const GPlatesMaths::PointOnSphere &p);

	PointOnSphereOstreamIterator &
	operator++() {

		return *this;
	}

	PointOnSphereOstreamIterator
	operator++(int) {

		return *this;
	}

 private:

	std::ostream *d_os_ptr;

	std::string d_delim;

};


void
PointOnSphereOstreamIterator::operator=(
 const GPlatesMaths::PointOnSphere &p) {

	using namespace GPlatesMaths;

	LatLonPoint llp = make_lat_lon_point(p);

	(*d_os_ptr)
	 << "("
	 << llp.latitude()
	 << ", "
	 << llp.longitude()
	 << ")       "
	 << HighPrecision< PointOnSphere >(p)
	 << d_delim;
}


namespace GPlatesMaths {

	std::ostream &
	operator<<(
	 std::ostream &os,
	 const PolylineOnSphere &p) {

		os << "PolylineOnSphere:\n";

		std::copy(p.vertex_begin(), p.vertex_end(),
		 PointOnSphereOstreamIterator(std::cout, "\n"));

		return os;
	}

}


template< typename C >
class PointOnSphereAppender {

 public:

	PointOnSphereAppender(
	 C &coll) :
	 d_coll_ptr(&coll) {  }

	void
	operator()(
	 const Point &p) {

		d_coll_ptr->push_back(Point_to_PointOnSphere(p));
	}

 private:

	C *d_coll_ptr;

};


GPlatesMaths::PolylineOnSphere
make_poly(
 const Point *array) {

	using namespace GPlatesMaths;

	std::vector< PointOnSphere > points;
	for ( ; array->point_list_type == POINT_TYPE; ++array) {

		points.push_back(Point_to_PointOnSphere(*array));
	}
	return PolylineOnSphere::create(points);
}


void
make_polys(
 std::list< GPlatesMaths::PolylineOnSphere > &polys,
 const Point *array) {

	using namespace GPlatesMaths;

	std::vector< PointOnSphere > points;
	for ( ; ; ++array) {

		switch (array->point_list_type) {

		 case POINT_TYPE:

			points.push_back(Point_to_PointOnSphere(*array));
			break;

		 case EOP_TYPE:
		 case EOL_TYPE:

			if (points.size() > 0) {

				polys.push_back(
				 PolylineOnSphere::create(points));
				points.clear();
			}
			break;

		}
		if (array->point_list_type == EOL_TYPE) break;
	}
}


void
make_points(
 std::list< GPlatesMaths::PointOnSphere > &points,
 const Point *array) {

	for ( ; array->point_list_type == POINT_TYPE; ++array) {

		points.push_back(Point_to_PointOnSphere(*array));
	}
}


#define NUM_ELEMS(a) (sizeof(a) / sizeof((a)[0]))

#define POINT(lat, lon)       { (lat), (lon), POINT_TYPE }
#define BREAK_BETWEEN_POLYS   { 0, 0, EOP_TYPE }
#define END_OF_LIST           { 0, 0, EOL_TYPE }


namespace TestResults {

	enum TestResult {

		PASS,
		FAIL,
		ERROR
	};
}


TestResults::TestResult
partition_and_verify(
 const Point input_polyline1_point_array[],
 const Point input_polyline2_point_array[],
 const Point expected_intersection_points_point_array[],
 const Point expected_partitioned_polylines_point_array[],
 const char funcname[]) {

	using namespace GPlatesMaths;

	PolylineOnSphere poly1 = make_poly(input_polyline1_point_array);
	PolylineOnSphere poly2 = make_poly(input_polyline2_point_array);

	std::list< PointOnSphere > expected_intersection_points;
	make_points(expected_intersection_points,
	 expected_intersection_points_point_array);

	std::list< PolylineOnSphere > expected_partitioned_polylines;
	make_polys(expected_partitioned_polylines,
	 expected_partitioned_polylines_point_array);

	std::list< PointOnSphere > actual_intersection_points;
	std::list< PolylineOnSphere > actual_partitioned_polylines;

	std::list< PointOnSphere >::size_type num_intersections =
	 PolylineIntersections::partition_intersecting_polylines(poly1, poly2,
	  actual_intersection_points, actual_partitioned_polylines);

	if (expected_intersection_points.size() != num_intersections) {

		std::cout << "+ " << funcname << ": FAIL" << std::endl;

		std::cout << "Polyline 1 --\n" << poly1 << std::endl;
		std::cout << "Polyline 2 --\n" << poly2 << std::endl;

		std::cout << "\nExpected Number of Intersections -- "
		 << expected_intersection_points.size();
		std::cout << "\nActual Number of Intersections -- "
		 << num_intersections << std::endl;

		std::cout << "\nExpected Intersection Points --\n";
		std::copy(expected_intersection_points.begin(),
		 expected_intersection_points.end(),
		 PointOnSphereOstreamIterator(std::cout, "\n"));

		std::cout << "\nActual Intersection Points --\n";
		std::copy(actual_intersection_points.begin(),
		 actual_intersection_points.end(),
		 PointOnSphereOstreamIterator(std::cout, "\n"));

		return TestResults::FAIL;
	}
	if ( ! sets_of_T_are_equivalent_by_predicate< PointOnSphere,
	        PointIsCoincident >(expected_intersection_points,
				    actual_intersection_points)) {

		std::cout << "+ " << funcname << ": FAIL" << std::endl;

		std::cout << "Polyline 1 --\n" << poly1 << std::endl;
		std::cout << "Polyline 2 --\n" << poly2 << std::endl;

		std::cout << "Expected Intersection Points --\n";
		std::copy(expected_intersection_points.begin(),
		 expected_intersection_points.end(),
		 PointOnSphereOstreamIterator(std::cout, "\n"));

		std::cout << "\nActual Intersection Points --\n";
		std::copy(actual_intersection_points.begin(),
		 actual_intersection_points.end(),
		 PointOnSphereOstreamIterator(std::cout, "\n"));

		return TestResults::FAIL;
	}
	if ( ! sets_of_polyline_are_undirected_equivalent(
	        expected_partitioned_polylines,
	        actual_partitioned_polylines)) {

		std::cout << "+ " << funcname << ": FAIL" << std::endl;

		std::cout << "Polyline 1 --\n" << poly1 << std::endl;
		std::cout << "Polyline 2 --\n" << poly2 << std::endl;

		std::cout << "Expected Partitioned Polylines --\n";
		std::copy(expected_partitioned_polylines.begin(),
		 expected_partitioned_polylines.end(),
		 std::ostream_iterator< PolylineOnSphere >(std::cout, "\n"));

		std::cout << "\nActual Partitioned Polylines --\n";
		std::copy(actual_partitioned_polylines.begin(),
		 actual_partitioned_polylines.end(),
		 std::ostream_iterator< PolylineOnSphere >(std::cout, "\n"));

		return TestResults::FAIL;
	}
	std::cout << "+ " << funcname << ": PASS" << std::endl;

	return TestResults::PASS;
}


TestResults::TestResult
test_no_intersection_both_of_length_one() {

	Point input_polyline1_point_array[] = {

		POINT(	0,	50	),
		POINT(	0,	70	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	30,	70	),
		POINT(	0,	90	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


TestResults::TestResult
test_no_intersection_both_of_length_one_on_same_great_circle_1() {

	Point input_polyline1_point_array[] = {

		POINT(	0,	50	),
		POINT(	0,	70	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	0,	80	),
		POINT(	0,	95	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


TestResults::TestResult
test_no_intersection_both_of_length_one_on_same_great_circle_2() {

	Point input_polyline1_point_array[] = {

		POINT(	0,	50	),
		POINT(	0,	70	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	0,	230	),
		POINT(	0,	250	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


/**
 * If we were working in a 2-D plane, the middle segments of polyline1 and
 * polyline2 would overlap, but because we're working on the surface of the
 * sphere, there is no intersection.
 */
TestResults::TestResult
test_no_intersection_due_to_spherical_geometry() {

	Point input_polyline1_point_array[] = {

		POINT(	40,	10	),
		POINT(	20,	30	),
		POINT(	20,	90	),
		POINT(	40,	110	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	10,	50	),
		POINT(	20,	40	),
		POINT(	20,	80	),
		POINT(	10,	70	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


TestResults::TestResult
test_intersection_both_of_length_one() {

	Point input_polyline1_point_array[] = {

		POINT(	0,	50	),
		POINT(	0,	90	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	30,	70	),
		POINT(	-30,	70	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		POINT(	0,	70	),
		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		POINT(	0,	50	),
		POINT(	0,	70	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	70	),
		POINT(	0,	90	),
		BREAK_BETWEEN_POLYS,
		POINT(	30,	70	),
		POINT(	0,	70	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	70	),
		POINT(	-30,	70	),
		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


TestResults::TestResult
test_intersection_coincident_with_one_vertex_lengths_one_two() {

	Point input_polyline1_point_array[] = {

		POINT(	10,	30	),
		POINT(	50,	30	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	30,	5	),
		POINT(	25,	30	),
		POINT(	35,	40	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		POINT(	25,	30	),
		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		POINT(	10,	30	),
		POINT(	25,	30	),
		BREAK_BETWEEN_POLYS,
		POINT(	25,	30	),
		POINT(	50,	30	),
		BREAK_BETWEEN_POLYS,
		POINT(	30,	5	),
		POINT(	25,	30	),
		BREAK_BETWEEN_POLYS,
		POINT(	25,	30	),
		POINT(	35,	40	),
		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


TestResults::TestResult
test_intersection_coincident_with_two_vertices_both_of_length_two() {

	Point input_polyline1_point_array[] = {

		POINT(	10,	30	),
		POINT(	25,	30	),
		POINT(	50,	30	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	30,	5	),
		POINT(	25,	30	),
		POINT(	35,	40	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		POINT(	25,	30	),
		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		POINT(	10,	30	),
		POINT(	25,	30	),
		BREAK_BETWEEN_POLYS,
		POINT(	25,	30	),
		POINT(	50,	30	),
		BREAK_BETWEEN_POLYS,
		POINT(	30,	5	),
		POINT(	25,	30	),
		BREAK_BETWEEN_POLYS,
		POINT(	25,	30	),
		POINT(	35,	40	),
		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


TestResults::TestResult
test_intersection_coincident_with_two_vertices_both_of_length_four() {

	Point input_polyline1_point_array[] = {

		POINT(	-5,	30	),
		POINT(	10,	25	),
		POINT(	25,	30	),
		POINT(	50,	25	),
		POINT(	65,	30	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	25,	-5	),
		POINT(	30,	5	),
		POINT(	25,	30	),
		POINT(	35,	40	),
		POINT(	35,	50	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		POINT(	25,	30	),
		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		POINT(	-5,	30	),
		POINT(	10,	25	),
		POINT(	25,	30	),
		BREAK_BETWEEN_POLYS,
		POINT(	25,	30	),
		POINT(	50,	25	),
		POINT(	65,	30	),
		BREAK_BETWEEN_POLYS,
		POINT(	25,	-5	),
		POINT(	30,	5	),
		POINT(	25,	30	),
		BREAK_BETWEEN_POLYS,
		POINT(	25,	30	),
		POINT(	35,	40	),
		POINT(	35,	50	),
		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


TestResults::TestResult
test_touching_end_to_end_no_intersection_both_of_length_one() {

	Point input_polyline1_point_array[] = {

		POINT(	20,	30	),
		POINT(	10,	50	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	20,	75	),
		POINT(	10,	50	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


TestResults::TestResult
test_touching_end_to_start_no_intersection_both_of_length_one() {

	Point input_polyline1_point_array[] = {

		POINT(	20,	30	),
		POINT(	10,	50	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	10,	50	),
		POINT(	20,	75	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


TestResults::TestResult
test_touching_start_to_end_no_intersection_both_of_length_one() {

	Point input_polyline1_point_array[] = {

		POINT(	10,	50	),
		POINT(	20,	30	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	20,	75	),
		POINT(	10,	50	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


TestResults::TestResult
test_touching_start_to_start_no_intersection_both_of_length_one() {

	Point input_polyline1_point_array[] = {

		POINT(	10,	50	),
		POINT(	20,	30	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	10,	50	),
		POINT(	20,	75	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


TestResults::TestResult
test_touching_end_to_end_no_intersection_both_of_length_two() {

	Point input_polyline1_point_array[] = {

		POINT(	20,	30	),
		POINT(	8,	42	),
		POINT(	10,	50	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	20,	75	),
		POINT(	0,	70	),
		POINT(	10,	50	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


TestResults::TestResult
test_touching_end_to_start_no_intersection_both_of_length_two() {

	Point input_polyline1_point_array[] = {

		POINT(	20,	30	),
		POINT(	8,	42	),
		POINT(	10,	50	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	10,	50	),
		POINT(	0,	70	),
		POINT(	20,	75	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


TestResults::TestResult
test_touching_start_to_end_no_intersection_both_of_length_two() {

	Point input_polyline1_point_array[] = {

		POINT(	10,	50	),
		POINT(	8,	42	),
		POINT(	20,	30	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	20,	75	),
		POINT(	0,	70	),
		POINT(	10,	50	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


TestResults::TestResult
test_touching_start_to_start_no_intersection_both_of_length_two() {

	Point input_polyline1_point_array[] = {

		POINT(	10,	50	),
		POINT(	8,	42	),
		POINT(	20,	30	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	10,	50	),
		POINT(	0,	70	),
		POINT(	20,	75	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


TestResults::TestResult
test_touching_end_to_mid_intersection_both_of_length_one() {

	Point input_polyline1_point_array[] = {

		POINT(	40,	50	),
		POINT(	10,	80	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	-10,	80	),
		POINT(	20,	80	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		POINT(	10,	80	),
		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		POINT(	40,	50	),
		POINT(	10,	80	),
		BREAK_BETWEEN_POLYS,
		POINT(	-10,	80	),
		POINT(	10,	80	),
		BREAK_BETWEEN_POLYS,
		POINT(	10,	80	),
		POINT(	20,	80	),
		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


TestResults::TestResult
test_touching_start_to_mid_intersection_both_of_length_one() {

	Point input_polyline1_point_array[] = {

		POINT(	10,	80	),
		POINT(	40,	50	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	-10,	80	),
		POINT(	20,	80	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		POINT(	10,	80	),
		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		POINT(	40,	50	),
		POINT(	10,	80	),
		BREAK_BETWEEN_POLYS,
		POINT(	-10,	80	),
		POINT(	10,	80	),
		BREAK_BETWEEN_POLYS,
		POINT(	10,	80	),
		POINT(	20,	80	),
		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


TestResults::TestResult
test_touching_end_to_mid_intersection_both_of_length_one_meet_perp() {

	Point input_polyline1_point_array[] = {

		POINT(	10,	50	),
		POINT(	10,	80	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	-10,	80	),
		POINT(	20,	80	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		POINT(	10,	80	),
		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		POINT(	10,	50	),
		POINT(	10,	80	),
		BREAK_BETWEEN_POLYS,
		POINT(	-10,	80	),
		POINT(	10,	80	),
		BREAK_BETWEEN_POLYS,
		POINT(	10,	80	),
		POINT(	20,	80	),
		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


TestResults::TestResult
test_touching_start_to_mid_intersection_both_of_length_one_meet_perp() {

	Point input_polyline1_point_array[] = {

		POINT(	10,	80	),
		POINT(	10,	50	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	-10,	80	),
		POINT(	20,	80	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		POINT(	10,	80	),
		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		POINT(	10,	50	),
		POINT(	10,	80	),
		BREAK_BETWEEN_POLYS,
		POINT(	-10,	80	),
		POINT(	10,	80	),
		BREAK_BETWEEN_POLYS,
		POINT(	10,	80	),
		POINT(	20,	80	),
		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


TestResults::TestResult
test_touching_end_to_mid_intersection_both_of_length_two_1() {

	Point input_polyline1_point_array[] = {

		POINT(	20,	10	),
		POINT(	40,	50	),
		POINT(	10,	80	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	-10,	80	),
		POINT(	20,	80	),
		POINT(	40,	100	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		POINT(	10,	80	),
		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		POINT(	20,	10	),
		POINT(	40,	50	),
		POINT(	10,	80	),
		BREAK_BETWEEN_POLYS,
		POINT(	-10,	80	),
		POINT(	10,	80	),
		BREAK_BETWEEN_POLYS,
		POINT(	10,	80	),
		POINT(	20,	80	),
		POINT(	40,	100	),
		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


TestResults::TestResult
test_touching_end_to_mid_intersection_both_of_length_two_2() {

	Point input_polyline1_point_array[] = {

		POINT(	20,	10	),
		POINT(	40,	50	),
		POINT(	10,	80	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	-30,	60	),
		POINT(	-10,	80	),
		POINT(	20,	80	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		POINT(	10,	80	),
		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		POINT(	20,	10	),
		POINT(	40,	50	),
		POINT(	10,	80	),
		BREAK_BETWEEN_POLYS,
		POINT(	-30,	60	),
		POINT(	-10,	80	),
		POINT(	10,	80	),
		BREAK_BETWEEN_POLYS,
		POINT(	10,	80	),
		POINT(	20,	80	),
		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


TestResults::TestResult
test_touching_end_to_mid_intersection_both_of_length_two_3() {

	Point input_polyline1_point_array[] = {

		POINT(	20,	10	),
		POINT(	40,	50	),
		POINT(	10,	80	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	40,	100	),
		POINT(	20,	80	),
		POINT(	-10,	80	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		POINT(	10,	80	),
		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		POINT(	20,	10	),
		POINT(	40,	50	),
		POINT(	10,	80	),
		BREAK_BETWEEN_POLYS,
		POINT(	-10,	80	),
		POINT(	10,	80	),
		BREAK_BETWEEN_POLYS,
		POINT(	10,	80	),
		POINT(	20,	80	),
		POINT(	40,	100	),
		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


TestResults::TestResult
test_touching_end_to_mid_intersection_both_of_length_two_4() {

	Point input_polyline1_point_array[] = {

		POINT(	20,	10	),
		POINT(	40,	50	),
		POINT(	10,	80	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	20,	80	),
		POINT(	-10,	80	),
		POINT(	-30,	60	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		POINT(	10,	80	),
		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		POINT(	20,	10	),
		POINT(	40,	50	),
		POINT(	10,	80	),
		BREAK_BETWEEN_POLYS,
		POINT(	-30,	60	),
		POINT(	-10,	80	),
		POINT(	10,	80	),
		BREAK_BETWEEN_POLYS,
		POINT(	10,	80	),
		POINT(	20,	80	),
		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


TestResults::TestResult
test_touching_end_to_mid_intersection_lengths_two_three() {

	Point input_polyline1_point_array[] = {

		POINT(	20,	10	),
		POINT(	40,	50	),
		POINT(	10,	80	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	40,	100	),
		POINT(	20,	80	),
		POINT(	-10,	80	),
		POINT(	-30,	60	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		POINT(	10,	80	),
		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		POINT(	20,	10	),
		POINT(	40,	50	),
		POINT(	10,	80	),
		BREAK_BETWEEN_POLYS,
		POINT(	-30,	60	),
		POINT(	-10,	80	),
		POINT(	10,	80	),
		BREAK_BETWEEN_POLYS,
		POINT(	10,	80	),
		POINT(	20,	80	),
		POINT(	40,	100	),
		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


TestResults::TestResult
test_touching_end_to_vertex_intersection_both_of_length_two() {

	Point input_polyline1_point_array[] = {

		POINT(	20,	10	),
		POINT(	40,	50	),
		POINT(	10,	80	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	20,	80	),
		POINT(	10,	80	),
		POINT(	-30,	60	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		POINT(	10,	80	),
		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		POINT(	20,	10	),
		POINT(	40,	50	),
		POINT(	10,	80	),
		BREAK_BETWEEN_POLYS,
		POINT(	-30,	60	),
		POINT(	10,	80	),
		BREAK_BETWEEN_POLYS,
		POINT(	10,	80	),
		POINT(	20,	80	),
		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


TestResults::TestResult
test_overlap_defined_by_polyline1_parallel_arcs() {

	Point input_polyline1_point_array[] = {

		POINT(	30,	50	),
		POINT(	0,	60	),
		POINT(	0,	85	),
		POINT(	20,	100	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	-30,	20	),
		POINT(	0,	40	),
		POINT(	0,	100	),
		POINT(	-30,	120	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		POINT(	0,	60	),
		POINT(	0,	85	),
		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		POINT(	30,	50	),
		POINT(	0,	60	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	60	),
		POINT(	0,	85	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	85	),
		POINT(	20,	100	),
		BREAK_BETWEEN_POLYS,
		POINT(	-30,	20	),
		POINT(	0,	40	),
		POINT(	0,	60	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	85	),
		POINT(	0,	100	),
		POINT(	-30,	120	),
		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


TestResults::TestResult
test_overlap_defined_by_polyline1_antiparallel_arcs() {

	Point input_polyline1_point_array[] = {

		POINT(	30,	50	),
		POINT(	0,	60	),
		POINT(	0,	85	),
		POINT(	20,	100	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	-30,	120	),
		POINT(	0,	100	),
		POINT(	0,	40	),
		POINT(	-30,	20	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		POINT(	0,	60	),
		POINT(	0,	85	),
		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		POINT(	30,	50	),
		POINT(	0,	60	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	60	),
		POINT(	0,	85	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	85	),
		POINT(	20,	100	),
		BREAK_BETWEEN_POLYS,
		POINT(	-30,	20	),
		POINT(	0,	40	),
		POINT(	0,	60	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	85	),
		POINT(	0,	100	),
		POINT(	-30,	120	),
		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


TestResults::TestResult
test_overlap_defined_by_polyline2_parallel_arcs() {

	Point input_polyline1_point_array[] = {

		POINT(	-30,	20	),
		POINT(	0,	40	),
		POINT(	0,	100	),
		POINT(	-30,	120	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	30,	50	),
		POINT(	0,	60	),
		POINT(	0,	85	),
		POINT(	20,	100	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		POINT(	0,	60	),
		POINT(	0,	85	),
		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		POINT(	30,	50	),
		POINT(	0,	60	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	60	),
		POINT(	0,	85	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	85	),
		POINT(	20,	100	),
		BREAK_BETWEEN_POLYS,
		POINT(	-30,	20	),
		POINT(	0,	40	),
		POINT(	0,	60	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	85	),
		POINT(	0,	100	),
		POINT(	-30,	120	),
		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


TestResults::TestResult
test_overlap_defined_by_polyline2_antiparallel_arcs() {

	Point input_polyline1_point_array[] = {

		POINT(	-30,	120	),
		POINT(	0,	100	),
		POINT(	0,	40	),
		POINT(	-30,	20	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	30,	50	),
		POINT(	0,	60	),
		POINT(	0,	85	),
		POINT(	20,	100	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		POINT(	0,	60	),
		POINT(	0,	85	),
		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		POINT(	30,	50	),
		POINT(	0,	60	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	60	),
		POINT(	0,	85	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	85	),
		POINT(	20,	100	),
		BREAK_BETWEEN_POLYS,
		POINT(	-30,	20	),
		POINT(	0,	40	),
		POINT(	0,	60	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	85	),
		POINT(	0,	100	),
		POINT(	-30,	120	),
		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


/**
 * Test that the intersection function does indeed handle duplicate polyline
 * arguments in the way it says it will: partitioning the two overlapping
 * polylines at every vertex and breaking the polylines down into their
 * component segments.
 */
TestResults::TestResult
test_same_polyline() {

	Point input_polyline1_point_array[] = {

		POINT(	30,	20	),
		POINT(	15,	40	),
		POINT(	30,	60	),
		POINT(	20,	80	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	30,	20	),
		POINT(	15,	40	),
		POINT(	30,	60	),
		POINT(	20,	80	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		POINT(	30,	20	),
		POINT(	15,	40	),
		POINT(	30,	60	),
		POINT(	20,	80	),
		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		POINT(	30,	20	),
		POINT(	15,	40	),
		BREAK_BETWEEN_POLYS,
		POINT(	15,	40	),
		POINT(	30,	60	),
		BREAK_BETWEEN_POLYS,
		POINT(	30,	60	),
		POINT(	20,	80	),
		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


/**
 * Test that the intersection function correctly handles the situation when a
 * segment of one polyline partially overlaps with the other.
 *
 * In this case, the end-point of polyline1 intersects with polyline2, and the
 * start-point of polyline2 intersects with polyline1.  The polylines are
 * pointing in the same direction.
 */
TestResults::TestResult
test_overlap_partial_both_of_length_one_1() {

	Point input_polyline1_point_array[] = {

		POINT(	0,	20	),
		POINT(	0,	50	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	0,	30	),
		POINT(	0,	60	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		POINT(	0,	30	),
		POINT(	0,	50	),
		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		POINT(	0,	20	),
		POINT(	0,	30	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	30	),
		POINT(	0,	50	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	50	),
		POINT(	0,	60	),
		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


/**
 * Test that the intersection function correctly handles the situation when a
 * segment of one polyline partially overlaps with the other.
 *
 * In this case, the end-point of polyline1 intersects with polyline2, and the
 * end-point of polyline2 intersects with polyline1.  The polylines are
 * pointing in opposite directions.
 */
TestResults::TestResult
test_overlap_partial_both_of_length_one_2() {

	Point input_polyline1_point_array[] = {

		POINT(	0,	20	),
		POINT(	0,	50	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	0,	60	),
		POINT(	0,	30	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		POINT(	0,	30	),
		POINT(	0,	50	),
		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		POINT(	0,	20	),
		POINT(	0,	30	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	30	),
		POINT(	0,	50	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	50	),
		POINT(	0,	60	),
		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


/**
 * Test that the intersection function correctly handles the situation when a
 * segment of one polyline partially overlaps with the other.
 *
 * In this case, the start-point of polyline1 intersects with polyline2, and
 * the end-point of polyline2 intersects with polyline1.  The polylines are
 * pointing in the same direction.
 */
TestResults::TestResult
test_overlap_partial_both_of_length_one_3() {

	Point input_polyline1_point_array[] = {

		POINT(	0,	50	),
		POINT(	0,	20	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	0,	60	),
		POINT(	0,	30	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		POINT(	0,	30	),
		POINT(	0,	50	),
		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		POINT(	0,	20	),
		POINT(	0,	30	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	30	),
		POINT(	0,	50	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	50	),
		POINT(	0,	60	),
		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


/**
 * Test that the intersection function correctly handles the situation when a
 * segment of one polyline partially overlaps with the other.
 *
 * In this case, the start-point of polyline1 intersects with polyline2, and
 * the start-point of polyline2 intersects with polyline1.  The polylines are
 * pointing in opposite directions.
 */
TestResults::TestResult
test_overlap_partial_both_of_length_one_4() {

	Point input_polyline1_point_array[] = {

		POINT(	0,	50	),
		POINT(	0,	20	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	0,	30	),
		POINT(	0,	60	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		POINT(	0,	30	),
		POINT(	0,	50	),
		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		POINT(	0,	20	),
		POINT(	0,	30	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	30	),
		POINT(	0,	50	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	50	),
		POINT(	0,	60	),
		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


/**
 * Test that the intersection function correctly handles the situation when the
 * middle segment of one three-segment polyline partially overlaps with the
 * middle segment of the other.
 *
 * In this case, the end-point of the polyline1 segment intersects with the
 * polyline2 segment, and the start-point of the polyline2 segment intersects
 * with the polyline1 segment.  The polyline segments are pointing in the same
 * direction.
 */
TestResults::TestResult
test_overlap_partial_both_of_length_three_middle_segment_to_middle_1() {

	Point input_polyline1_point_array[] = {

		POINT(	20,	10	),
		POINT(	0,	20	),
		POINT(	0,	70	),
		POINT(	20,	80	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	-20,	30	),
		POINT(	0,	40	),
		POINT(	0,	90	),
		POINT(	-20,	100	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		POINT(	0,	40	),
		POINT(	0,	70	),
		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		POINT(	20,	10	),
		POINT(	0,	20	),
		POINT(	0,	40	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	40	),
		POINT(	0,	70	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	70	),
		POINT(	20,	80	),
		BREAK_BETWEEN_POLYS,
		POINT(	-20,	30	),
		POINT(	0,	40	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	70	),
		POINT(	0,	90	),
		POINT(	-20,	100	),
		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


/**
 * Test that the intersection function correctly handles the situation when the
 * middle segment of one three-segment polyline partially overlaps with the
 * middle segment of the other.
 *
 * In this case, the end-point of the polyline1 segment intersects with the
 * polyline2 segment, and the end-point of the polyline2 segment intersects
 * with the polyline1 segment.  The polyline segments are pointing in opposite
 * directions.
 */
TestResults::TestResult
test_overlap_partial_both_of_length_three_middle_segment_to_middle_2() {

	Point input_polyline1_point_array[] = {

		POINT(	20,	10	),
		POINT(	0,	20	),
		POINT(	0,	70	),
		POINT(	20,	80	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	-20,	100	),
		POINT(	0,	90	),
		POINT(	0,	40	),
		POINT(	-20,	30	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		POINT(	0,	40	),
		POINT(	0,	70	),
		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		POINT(	20,	10	),
		POINT(	0,	20	),
		POINT(	0,	40	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	40	),
		POINT(	0,	70	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	70	),
		POINT(	20,	80	),
		BREAK_BETWEEN_POLYS,
		POINT(	-20,	30	),
		POINT(	0,	40	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	70	),
		POINT(	0,	90	),
		POINT(	-20,	100	),
		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


/**
 * Test that the intersection function correctly handles the situation when the
 * middle segment of one three-segment polyline partially overlaps with the
 * middle segment of the other.
 *
 * In this case, the start-point of the polyline1 segment intersects with the
 * polyline2 segment, and the end-point of the polyline2 segment intersects
 * with the polyline1 segment.  The polyline segments are pointing in the same
 * direction.
 */
TestResults::TestResult
test_overlap_partial_both_of_length_three_middle_segment_to_middle_3() {

	Point input_polyline1_point_array[] = {

		POINT(	20,	80	),
		POINT(	0,	70	),
		POINT(	0,	20	),
		POINT(	20,	10	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	-20,	100	),
		POINT(	0,	90	),
		POINT(	0,	40	),
		POINT(	-20,	30	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		POINT(	0,	40	),
		POINT(	0,	70	),
		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		POINT(	20,	10	),
		POINT(	0,	20	),
		POINT(	0,	40	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	40	),
		POINT(	0,	70	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	70	),
		POINT(	20,	80	),
		BREAK_BETWEEN_POLYS,
		POINT(	-20,	30	),
		POINT(	0,	40	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	70	),
		POINT(	0,	90	),
		POINT(	-20,	100	),
		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


/**
 * Test that the intersection function correctly handles the situation when the
 * middle segment of one three-segment polyline partially overlaps with the
 * middle segment of the other.
 *
 * In this case, the start-point of the polyline1 segment intersects with the
 * polyline2 segment, and the start-point of the polyline2 segment intersects
 * with the polyline1 segment.  The polyline segments are pointing in opposite
 * directions.
 */
TestResults::TestResult
test_overlap_partial_both_of_length_three_middle_segment_to_middle_4() {

	Point input_polyline1_point_array[] = {

		POINT(	20,	80	),
		POINT(	0,	70	),
		POINT(	0,	20	),
		POINT(	20,	10	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	-20,	30	),
		POINT(	0,	40	),
		POINT(	0,	90	),
		POINT(	-20,	100	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		POINT(	0,	40	),
		POINT(	0,	70	),
		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		POINT(	20,	10	),
		POINT(	0,	20	),
		POINT(	0,	40	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	40	),
		POINT(	0,	70	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	70	),
		POINT(	20,	80	),
		BREAK_BETWEEN_POLYS,
		POINT(	-20,	30	),
		POINT(	0,	40	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	70	),
		POINT(	0,	90	),
		POINT(	-20,	100	),
		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


TestResults::TestResult
test_multi_intersection_4() {

	Point input_polyline1_point_array[] = {

		POINT(	50,			40	),
		POINT(	30,			60	),
		POINT(	50,			80	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	30,			40	),
		POINT(	50,			60	),
		POINT(	30,			80	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		POINT(	41.930105189940669,	50	),
		POINT(	41.930105189940669,	70	),
		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		POINT(	50,			40	),
		POINT(	41.930105189940669,	50	),
		BREAK_BETWEEN_POLYS,
		POINT(	41.930105189940669,	50	),
		POINT(	30,			60	),
		POINT(	41.930105189940669,	70	),
		BREAK_BETWEEN_POLYS,
		POINT(	41.930105189940669,	70	),
		POINT(	50,			80	),
		BREAK_BETWEEN_POLYS,
		POINT(	30,			40	),
		POINT(	41.930105189940669,	50	),
		BREAK_BETWEEN_POLYS,
		POINT(	41.930105189940669,	50	),
		POINT(	50,			60	),
		POINT(	41.930105189940669,	70	),
		BREAK_BETWEEN_POLYS,
		POINT(	41.930105189940669,	70	),
		POINT(	30,			80	),
		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


TestResults::TestResult
test_multi_intersection_5() {

	Point input_polyline1_point_array[] = {

		POINT(	50,			20	),
		POINT(	50,			40	),
		POINT(	30,			60	),
		POINT(	50,			80	),
		POINT(	50,			100	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	30,			20	),
		POINT(	30,			40	),
		POINT(	50,			60	),
		POINT(	30,			80	),
		POINT(	30,			100	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		POINT(	41.930105189940669,	50	),
		POINT(	41.930105189940669,	70	),
		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		POINT(	50,			20	),
		POINT(	50,			40	),
		POINT(	41.930105189940669,	50	),
		BREAK_BETWEEN_POLYS,
		POINT(	41.930105189940669,	50	),
		POINT(	30,			60	),
		POINT(	41.930105189940669,	70	),
		BREAK_BETWEEN_POLYS,
		POINT(	41.930105189940669,	70	),
		POINT(	50,			80	),
		POINT(	50,			100	),
		BREAK_BETWEEN_POLYS,
		POINT(	30,			20	),
		POINT(	30,			40	),
		POINT(	41.930105189940669,	50	),
		BREAK_BETWEEN_POLYS,
		POINT(	41.930105189940669,	50	),
		POINT(	50,			60	),
		POINT(	41.930105189940669,	70	),
		BREAK_BETWEEN_POLYS,
		POINT(	41.930105189940669,	70	),
		POINT(	30,			80	),
		POINT(	30,			100	),
		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


TestResults::TestResult
test_multi_intersection_6() {

	Point input_polyline1_point_array[] = {

		POINT(	50,			20	),
		POINT(	50,			40	),
		POINT(	30,			60	),
		POINT(	50,			80	),
		POINT(	50,			100	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	30,			100	),
		POINT(	30,			80	),
		POINT(	50,			60	),
		POINT(	30,			40	),
		POINT(	30,			20	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		POINT(	41.930105189940669,	50	),
		POINT(	41.930105189940669,	70	),
		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		POINT(	50,			20	),
		POINT(	50,			40	),
		POINT(	41.930105189940669,	50	),
		BREAK_BETWEEN_POLYS,
		POINT(	41.930105189940669,	50	),
		POINT(	30,			60	),
		POINT(	41.930105189940669,	70	),
		BREAK_BETWEEN_POLYS,
		POINT(	41.930105189940669,	70	),
		POINT(	50,			80	),
		POINT(	50,			100	),
		BREAK_BETWEEN_POLYS,
		POINT(	30,			20	),
		POINT(	30,			40	),
		POINT(	41.930105189940669,	50	),
		BREAK_BETWEEN_POLYS,
		POINT(	41.930105189940669,	50	),
		POINT(	50,			60	),
		POINT(	41.930105189940669,	70	),
		BREAK_BETWEEN_POLYS,
		POINT(	41.930105189940669,	70	),
		POINT(	30,			80	),
		POINT(	30,			100	),
		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


/**
 * Test that the intersection function correctly handles the situation when one
 * polyline intersects multiple times with a single segment of the other.
 *
 * In this case, it is polyline2 intersecting multiple times with a single
 * segment of polyline1.
 */
TestResults::TestResult
test_multi_intersection_7() {

	Point input_polyline1_point_array[] = {

		POINT(	20,	10	),
		POINT(	0,	20	),
		POINT(	0,	120	),
		POINT(	-20,	130	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	-20,	20	),
		POINT(	-20,	40	),
		POINT(	20,	60	),
		POINT(	-20,	80	),
		POINT(	20,	100	),
		POINT(	20,	120	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		POINT(	0,	50	),
		POINT(	0,	70	),
		POINT(	0,	90	),
		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		POINT(	20,	10	),
		POINT(	0,	20	),
		POINT(	0,	50	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	50	),
		POINT(	0,	70	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	70	),
		POINT(	0,	90	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	90	),
		POINT(	0,	120	),
		POINT(	-20,	130	),
		BREAK_BETWEEN_POLYS,
		POINT(	-20,	20	),
		POINT(	-20,	40	),
		POINT(	0,	50	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	50	),
		POINT(	20,	60	),
		POINT(	0,	70	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	70	),
		POINT(	-20,	80	),
		POINT(	0,	90	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	90	),
		POINT(	20,	100	),
		POINT(	20,	120	),
		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


/**
 * This test case is the same as @a test_multi_intersection_7 except that the
 * direction of polyline1 has been reversed.
 */
TestResults::TestResult
test_multi_intersection_8() {

	Point input_polyline1_point_array[] = {

		POINT(	-20,	130	),
		POINT(	0,	120	),
		POINT(	0,	20	),
		POINT(	20,	10	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	-20,	20	),
		POINT(	-20,	40	),
		POINT(	20,	60	),
		POINT(	-20,	80	),
		POINT(	20,	100	),
		POINT(	20,	120	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		POINT(	0,	50	),
		POINT(	0,	70	),
		POINT(	0,	90	),
		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		POINT(	20,	10	),
		POINT(	0,	20	),
		POINT(	0,	50	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	50	),
		POINT(	0,	70	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	70	),
		POINT(	0,	90	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	90	),
		POINT(	0,	120	),
		POINT(	-20,	130	),
		BREAK_BETWEEN_POLYS,
		POINT(	-20,	20	),
		POINT(	-20,	40	),
		POINT(	0,	50	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	50	),
		POINT(	20,	60	),
		POINT(	0,	70	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	70	),
		POINT(	-20,	80	),
		POINT(	0,	90	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	90	),
		POINT(	20,	100	),
		POINT(	20,	120	),
		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


/**
 * This test case is the same as @a test_multi_intersection_7 except that the
 * geometries of polyline1 and polyline2 have been swapped.
 */
TestResults::TestResult
test_multi_intersection_9() {

	Point input_polyline1_point_array[] = {

		POINT(	-20,	20	),
		POINT(	-20,	40	),
		POINT(	20,	60	),
		POINT(	-20,	80	),
		POINT(	20,	100	),
		POINT(	20,	120	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	20,	10	),
		POINT(	0,	20	),
		POINT(	0,	120	),
		POINT(	-20,	130	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		POINT(	0,	50	),
		POINT(	0,	70	),
		POINT(	0,	90	),
		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		POINT(	20,	10	),
		POINT(	0,	20	),
		POINT(	0,	50	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	50	),
		POINT(	0,	70	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	70	),
		POINT(	0,	90	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	90	),
		POINT(	0,	120	),
		POINT(	-20,	130	),
		BREAK_BETWEEN_POLYS,
		POINT(	-20,	20	),
		POINT(	-20,	40	),
		POINT(	0,	50	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	50	),
		POINT(	20,	60	),
		POINT(	0,	70	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	70	),
		POINT(	-20,	80	),
		POINT(	0,	90	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	90	),
		POINT(	20,	100	),
		POINT(	20,	120	),
		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


/**
 * This test case is the same as @a test_multi_intersection_9 except that the
 * direction of polyline1 has been reversed.
 */
TestResults::TestResult
test_multi_intersection_10() {

	Point input_polyline1_point_array[] = {

		POINT(	20,	120	),
		POINT(	20,	100	),
		POINT(	-20,	80	),
		POINT(	20,	60	),
		POINT(	-20,	40	),
		POINT(	-20,	20	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	20,	10	),
		POINT(	0,	20	),
		POINT(	0,	120	),
		POINT(	-20,	130	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		POINT(	0,	50	),
		POINT(	0,	70	),
		POINT(	0,	90	),
		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		POINT(	20,	10	),
		POINT(	0,	20	),
		POINT(	0,	50	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	50	),
		POINT(	0,	70	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	70	),
		POINT(	0,	90	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	90	),
		POINT(	0,	120	),
		POINT(	-20,	130	),
		BREAK_BETWEEN_POLYS,
		POINT(	-20,	20	),
		POINT(	-20,	40	),
		POINT(	0,	50	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	50	),
		POINT(	20,	60	),
		POINT(	0,	70	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	70	),
		POINT(	-20,	80	),
		POINT(	0,	90	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	90	),
		POINT(	20,	100	),
		POINT(	20,	120	),
		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


/**
 * This test case is the same as @a test_multi_intersection_7 except that the
 * third point of intersection is now @em between the first two.
 */
TestResults::TestResult
test_multi_intersection_11() {

	Point input_polyline1_point_array[] = {

		POINT(	20,	10	),
		POINT(	0,	20	),
		POINT(	0,	120	),
		POINT(	-20,	130	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	-20,	20	),
		POINT(	-20,	40	),
		POINT(	20,	60	),
		POINT(	20,	100	),
		POINT(	-20,	80	),
		POINT(	-20,	50	),
		POINT(	10,	60	),
		POINT(	10,	70	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		POINT(	0,	50	),
		POINT(	0,	90	),
		POINT(	0,	56.74036781990506	),
		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		POINT(	20,	10	),
		POINT(	0,	20	),
		POINT(	0,	50	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	50	),
		POINT(	0,	56.74036781990506	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	56.740367819905060	),
		POINT(	0,	90	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	90	),
		POINT(	0,	120	),
		POINT(	-20,	130	),
		BREAK_BETWEEN_POLYS,
		POINT(	-20,	20	),
		POINT(	-20,	40	),
		POINT(	0,	50	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	50	),
		POINT(	20,	60	),
		POINT(	20,	100	),
		POINT(	0,	90	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	90	),
		POINT(	-20,	80	),
		POINT(	-20,	50	),
		POINT(	0,	56.740367819905060	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	56.740367819905060	),
		POINT(	10,	60	),
		POINT(	10,	70	),
		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


/**
 * This test case is the same as @a test_multi_intersection_11 except that the
 * direction of polyline1 has been reversed.
 */
TestResults::TestResult
test_multi_intersection_12() {

	Point input_polyline1_point_array[] = {

		POINT(	-20,	130	),
		POINT(	0,	120	),
		POINT(	0,	20	),
		POINT(	20,	10	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	-20,	20	),
		POINT(	-20,	40	),
		POINT(	20,	60	),
		POINT(	20,	100	),
		POINT(	-20,	80	),
		POINT(	-20,	50	),
		POINT(	10,	60	),
		POINT(	10,	70	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		POINT(	0,	50	),
		POINT(	0,	90	),
		POINT(	0,	56.74036781990506	),
		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		POINT(	20,	10	),
		POINT(	0,	20	),
		POINT(	0,	50	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	50	),
		POINT(	0,	56.74036781990506	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	56.740367819905060	),
		POINT(	0,	90	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	90	),
		POINT(	0,	120	),
		POINT(	-20,	130	),
		BREAK_BETWEEN_POLYS,
		POINT(	-20,	20	),
		POINT(	-20,	40	),
		POINT(	0,	50	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	50	),
		POINT(	20,	60	),
		POINT(	20,	100	),
		POINT(	0,	90	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	90	),
		POINT(	-20,	80	),
		POINT(	-20,	50	),
		POINT(	0,	56.740367819905060	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	56.740367819905060	),
		POINT(	10,	60	),
		POINT(	10,	70	),
		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


/**
 * This test case is the same as @a test_multi_intersection_11 except that the
 * geometries of polyline1 and polyline2 have been swapped.
 */
TestResults::TestResult
test_multi_intersection_13() {

	Point input_polyline1_point_array[] = {

		POINT(	-20,	20	),
		POINT(	-20,	40	),
		POINT(	20,	60	),
		POINT(	20,	100	),
		POINT(	-20,	80	),
		POINT(	-20,	50	),
		POINT(	10,	60	),
		POINT(	10,	70	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	20,	10	),
		POINT(	0,	20	),
		POINT(	0,	120	),
		POINT(	-20,	130	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		POINT(	0,	50	),
		POINT(	0,	90	),
		POINT(	0,	56.74036781990506	),
		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		POINT(	20,	10	),
		POINT(	0,	20	),
		POINT(	0,	50	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	50	),
		POINT(	0,	56.74036781990506	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	56.740367819905060	),
		POINT(	0,	90	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	90	),
		POINT(	0,	120	),
		POINT(	-20,	130	),
		BREAK_BETWEEN_POLYS,
		POINT(	-20,	20	),
		POINT(	-20,	40	),
		POINT(	0,	50	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	50	),
		POINT(	20,	60	),
		POINT(	20,	100	),
		POINT(	0,	90	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	90	),
		POINT(	-20,	80	),
		POINT(	-20,	50	),
		POINT(	0,	56.740367819905060	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	56.740367819905060	),
		POINT(	10,	60	),
		POINT(	10,	70	),
		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


/**
 * This test case is the same as @a test_multi_intersection_13 except that the
 * direction of polyline1 has been reversed.
 */
TestResults::TestResult
test_multi_intersection_14() {

	Point input_polyline1_point_array[] = {

		POINT(	10,	70	),
		POINT(	10,	60	),
		POINT(	-20,	50	),
		POINT(	-20,	80	),
		POINT(	20,	100	),
		POINT(	20,	60	),
		POINT(	-20,	40	),
		POINT(	-20,	20	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	20,	10	),
		POINT(	0,	20	),
		POINT(	0,	120	),
		POINT(	-20,	130	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		POINT(	0,	50	),
		POINT(	0,	90	),
		POINT(	0,	56.74036781990506	),
		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		POINT(	20,	10	),
		POINT(	0,	20	),
		POINT(	0,	50	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	50	),
		POINT(	0,	56.74036781990506	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	56.740367819905060	),
		POINT(	0,	90	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	90	),
		POINT(	0,	120	),
		POINT(	-20,	130	),
		BREAK_BETWEEN_POLYS,
		POINT(	-20,	20	),
		POINT(	-20,	40	),
		POINT(	0,	50	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	50	),
		POINT(	20,	60	),
		POINT(	20,	100	),
		POINT(	0,	90	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	90	),
		POINT(	-20,	80	),
		POINT(	-20,	50	),
		POINT(	0,	56.740367819905060	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	56.740367819905060	),
		POINT(	10,	60	),
		POINT(	10,	70	),
		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


/**
 * This case tests two different configurations of overlap between two
 * polylines: one overlap of identical segments and one overlap of a smaller
 * segment contained within a larger segment.
 */
TestResults::TestResult
test_multi_overlap_1() {

	Point input_polyline1_point_array[] = {

		POINT(	20,	10	),
		POINT(	0,	20	),
		POINT(	0,	40	),
		POINT(	20,	50	),
		POINT(	20,	65	),
		POINT(	0,	75	),
		POINT(	0,	85	),
		POINT(	20,	95	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	-20,	10	),
		POINT(	0,	20	),
		POINT(	0,	40	),
		POINT(	-20,	50	),
		POINT(	0,	60	),
		POINT(	0,	100	),
		POINT(	-20,	110	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		POINT(	0,	20	),
		POINT(	0,	40	),
		POINT(	0,	75	),
		POINT(	0,	85	),
		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		POINT(	20,	10	),
		POINT(	0,	20	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	20	),
		POINT(	0,	40	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	40	),
		POINT(	20,	50	),
		POINT(	20,	65	),
		POINT(	0,	75	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	75	),
		POINT(	0,	85	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	85	),
		POINT(	20,	95	),
		BREAK_BETWEEN_POLYS,
		POINT(	-20,	10	),
		POINT(	0,	20	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	40	),
		POINT(	-20,	50	),
		POINT(	0,	60	),
		POINT(	0,	75	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	85	),
		POINT(	0,	100	),
		POINT(	-20,	110	),
		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


/**
 * This test case is the same as @a test_multi_overlap_1 except that the
 * geometries of polyline1 and polyline2 have been swapped.
 */
TestResults::TestResult
test_multi_overlap_2() {

	Point input_polyline1_point_array[] = {

		POINT(	-20,	10	),
		POINT(	0,	20	),
		POINT(	0,	40	),
		POINT(	-20,	50	),
		POINT(	0,	60	),
		POINT(	0,	100	),
		POINT(	-20,	110	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	20,	10	),
		POINT(	0,	20	),
		POINT(	0,	40	),
		POINT(	20,	50	),
		POINT(	20,	65	),
		POINT(	0,	75	),
		POINT(	0,	85	),
		POINT(	20,	95	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		POINT(	0,	20	),
		POINT(	0,	40	),
		POINT(	0,	75	),
		POINT(	0,	85	),
		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		POINT(	20,	10	),
		POINT(	0,	20	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	20	),
		POINT(	0,	40	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	40	),
		POINT(	20,	50	),
		POINT(	20,	65	),
		POINT(	0,	75	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	75	),
		POINT(	0,	85	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	85	),
		POINT(	20,	95	),
		BREAK_BETWEEN_POLYS,
		POINT(	-20,	10	),
		POINT(	0,	20	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	40	),
		POINT(	-20,	50	),
		POINT(	0,	60	),
		POINT(	0,	75	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	85	),
		POINT(	0,	100	),
		POINT(	-20,	110	),
		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


/**
 * This case tests the situation when polyline1 overlaps twice with a single
 * segment of polyline2.
 */
TestResults::TestResult
test_multi_overlap_3() {

	Point input_polyline1_point_array[] = {

		POINT(	20,	15	),
		POINT(	0,	25	),
		POINT(	0,	40	),
		POINT(	20,	45	),
		POINT(	0,	50	),
		POINT(	0,	70	),
		POINT(	20,	80	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	-20,	10	),
		POINT(	0,	20	),
		POINT(	0,	90	),
		POINT(	-20,	95	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		POINT(	0,	25	),
		POINT(	0,	40	),
		POINT(	0,	50	),
		POINT(	0,	70	),
		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		POINT(	20,	15	),
		POINT(	0,	25	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	25	),
		POINT(	0,	40	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	40	),
		POINT(	20,	45	),
		POINT(	0,	50	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	50	),
		POINT(	0,	70	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	70	),
		POINT(	20,	80	),
		BREAK_BETWEEN_POLYS,
		POINT(	-20,	10	),
		POINT(	0,	20	),
		POINT(	0,	25	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	40	),
		POINT(	0,	50	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	70	),
		POINT(	0,	90	),
		POINT(	-20,	95	),
		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


/**
 * This test case is the same as @a test_multi_overlap_4 except that the
 * direction of polyline1 has been reversed.
 */
TestResults::TestResult
test_multi_overlap_4() {

	Point input_polyline1_point_array[] = {

		POINT(	20,	80	),
		POINT(	0,	70	),
		POINT(	0,	50	),
		POINT(	20,	45	),
		POINT(	0,	40	),
		POINT(	0,	25	),
		POINT(	20,	15	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	-20,	10	),
		POINT(	0,	20	),
		POINT(	0,	90	),
		POINT(	-20,	95	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		POINT(	0,	25	),
		POINT(	0,	40	),
		POINT(	0,	50	),
		POINT(	0,	70	),
		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		POINT(	20,	15	),
		POINT(	0,	25	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	25	),
		POINT(	0,	40	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	40	),
		POINT(	20,	45	),
		POINT(	0,	50	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	50	),
		POINT(	0,	70	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	70	),
		POINT(	20,	80	),
		BREAK_BETWEEN_POLYS,
		POINT(	-20,	10	),
		POINT(	0,	20	),
		POINT(	0,	25	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	40	),
		POINT(	0,	50	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	70	),
		POINT(	0,	90	),
		POINT(	-20,	95	),
		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


/**
 * This test case is the same as @a test_multi_overlap_3 except that the
 * geometries of polyline1 and polyline2 have been swapped.
 */
TestResults::TestResult
test_multi_overlap_5() {

	Point input_polyline1_point_array[] = {

		POINT(	-20,	10	),
		POINT(	0,	20	),
		POINT(	0,	90	),
		POINT(	-20,	95	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	20,	15	),
		POINT(	0,	25	),
		POINT(	0,	40	),
		POINT(	20,	45	),
		POINT(	0,	50	),
		POINT(	0,	70	),
		POINT(	20,	80	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		POINT(	0,	25	),
		POINT(	0,	40	),
		POINT(	0,	50	),
		POINT(	0,	70	),
		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		POINT(	20,	15	),
		POINT(	0,	25	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	25	),
		POINT(	0,	40	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	40	),
		POINT(	20,	45	),
		POINT(	0,	50	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	50	),
		POINT(	0,	70	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	70	),
		POINT(	20,	80	),
		BREAK_BETWEEN_POLYS,
		POINT(	-20,	10	),
		POINT(	0,	20	),
		POINT(	0,	25	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	40	),
		POINT(	0,	50	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	70	),
		POINT(	0,	90	),
		POINT(	-20,	95	),
		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


/**
 * This test case is the same as @a test_multi_overlap_5 except that the
 * direction of polyline1 has been reversed.
 */
TestResults::TestResult
test_multi_overlap_6() {

	Point input_polyline1_point_array[] = {

		POINT(	-20,	95	),
		POINT(	0,	90	),
		POINT(	0,	20	),
		POINT(	-20,	10	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	20,	15	),
		POINT(	0,	25	),
		POINT(	0,	40	),
		POINT(	20,	45	),
		POINT(	0,	50	),
		POINT(	0,	70	),
		POINT(	20,	80	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		POINT(	0,	25	),
		POINT(	0,	40	),
		POINT(	0,	50	),
		POINT(	0,	70	),
		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		POINT(	20,	15	),
		POINT(	0,	25	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	25	),
		POINT(	0,	40	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	40	),
		POINT(	20,	45	),
		POINT(	0,	50	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	50	),
		POINT(	0,	70	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	70	),
		POINT(	20,	80	),
		BREAK_BETWEEN_POLYS,
		POINT(	-20,	10	),
		POINT(	0,	20	),
		POINT(	0,	25	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	40	),
		POINT(	0,	50	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	70	),
		POINT(	0,	90	),
		POINT(	-20,	95	),
		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


/**
 * This case tests the situation when a single segment of polyline1 overlaps
 * partially with two adjacent, parallel segments of polyline2.
 *
 * Basically, it is the same as a "middle segment of polyline1 overlaps with
 * middle segment of polyline2; the polyline1 segment defines the extent of the
 * overlap" test, except that there is a polyline2 vertex in the middle of the
 * extent of the polyline1 segment overlap, meaning that the polyline1 segment
 * is now partially overlapping *two* adjacent polyline2 segments.
 */
TestResults::TestResult
test_multi_overlap_7() {

	Point input_polyline1_point_array[] = {

		POINT(	20,	20	),
		POINT(	0,	40	),
		POINT(	0,	80	),
		POINT(	20,	100	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	-20,	10	),
		POINT(	0,	30	),
		POINT(	0,	60	),
		POINT(	0,	90	),
		POINT(	-20,	110	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		POINT(	0,	40	),
		POINT(	0,	60	),
		POINT(	0,	80	),
		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		POINT(	20,	20	),
		POINT(	0,	40	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	40	),
		POINT(	0,	60	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	60	),
		POINT(	0,	80	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	80	),
		POINT(	20,	100	),
		BREAK_BETWEEN_POLYS,
		POINT(	-20,	10	),
		POINT(	0,	30	),
		POINT(	0,	40	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	80	),
		POINT(	0,	90	),
		POINT(	-20,	110	),
		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


/**
 * This test case is the same as @a test_multi_overlap_7 except that the
 * geometries of polyline1 and polyline2 have been swapped.
 */
TestResults::TestResult
test_multi_overlap_8() {

	Point input_polyline1_point_array[] = {

		POINT(	-20,	10	),
		POINT(	0,	30	),
		POINT(	0,	60	),
		POINT(	0,	90	),
		POINT(	-20,	110	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	20,	20	),
		POINT(	0,	40	),
		POINT(	0,	80	),
		POINT(	20,	100	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		POINT(	0,	40	),
		POINT(	0,	60	),
		POINT(	0,	80	),
		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		POINT(	20,	20	),
		POINT(	0,	40	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	40	),
		POINT(	0,	60	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	60	),
		POINT(	0,	80	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	80	),
		POINT(	20,	100	),
		BREAK_BETWEEN_POLYS,
		POINT(	-20,	10	),
		POINT(	0,	30	),
		POINT(	0,	40	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	80	),
		POINT(	0,	90	),
		POINT(	-20,	110	),
		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


TestResults::TestResult
test_multi_overlap_and_intersection_1() {

	Point input_polyline1_point_array[] = {

		POINT(	30,	10	),
		POINT(	0,	20	),
		POINT(	0,	120	),
		POINT(	-30,	150	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	-20,	0	),
		POINT(	-20,	30	),
		POINT(	20,	45	),
		POINT(	0,	55	),
		POINT(	0,	80	),
		POINT(	25,	100	),
		POINT(	25,	130	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		POINT(	0,	37.5	),
		POINT(	0,	55	),
		POINT(	0,	80	),
		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		POINT(	30,	10	),
		POINT(	0,	20	),
		POINT(	0,	37.5	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	37.5	),
		POINT(	0,	55	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	55	),
		POINT(	0,	80	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	80	),
		POINT(	0,	120	),
		POINT(	-30,	150	),
		BREAK_BETWEEN_POLYS,
		POINT(	-20,	0	),
		POINT(	-20,	30	),
		POINT(	0,	37.5	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	37.5	),
		POINT(	20,	45	),
		POINT(	0,	55	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	80	),
		POINT(	25,	100	),
		POINT(	25,	130	),
		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


TestResults::TestResult
test_multi_overlap_and_intersection_2() {

	Point input_polyline1_point_array[] = {

		POINT(	-30,	150	),
		POINT(	0,	120	),
		POINT(	0,	20	),
		POINT(	30,	10	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	-20,	0	),
		POINT(	-20,	30	),
		POINT(	20,	45	),
		POINT(	0,	55	),
		POINT(	0,	80	),
		POINT(	25,	100	),
		POINT(	25,	130	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		POINT(	0,	37.5	),
		POINT(	0,	55	),
		POINT(	0,	80	),
		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		POINT(	30,	10	),
		POINT(	0,	20	),
		POINT(	0,	37.5	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	37.5	),
		POINT(	0,	55	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	55	),
		POINT(	0,	80	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	80	),
		POINT(	0,	120	),
		POINT(	-30,	150	),
		BREAK_BETWEEN_POLYS,
		POINT(	-20,	0	),
		POINT(	-20,	30	),
		POINT(	0,	37.5	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	37.5	),
		POINT(	20,	45	),
		POINT(	0,	55	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,	80	),
		POINT(	25,	100	),
		POINT(	25,	130	),
		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


TestResults::TestResult
test_multi_overlap_and_intersection_3() {

	Point input_polyline1_point_array[] = {

		POINT(	50,			10	),
		POINT(	20,			20	),
		POINT(	20,			120	),
		POINT(	-10,			150	),
		END_OF_LIST
	};
	Point input_polyline2_point_array[] = {

		POINT(	0,			0	),
		POINT(	0,			30	),
		POINT(	40,			45	),
		POINT(	28.221837346670856,	51.40774512087615	),
		POINT(	28.622213518361956,	85.470675188875916	),
		POINT(	45,			100	),
		POINT(	45,			130	),
		END_OF_LIST
	};
	Point expected_intersection_points_point_array[] = {

		POINT(	25.788044187940816,	38.570743740648808	),
		POINT(	28.221837346670856,	51.40774512087615	),
		POINT(	28.622213518361956,	85.470675188875916	),
		END_OF_LIST
	};
	Point expected_partitioned_polylines_point_array[] = {

		POINT(	50,			10	),
		POINT(	20,			20	),
		POINT(	25.788044187940816,	38.570743740648808	),
		BREAK_BETWEEN_POLYS,
		POINT(	25.788044187940816,	38.570743740648808	),
		POINT(	28.221837346670856,	51.40774512087615	),
		BREAK_BETWEEN_POLYS,
		POINT(	28.221837346670856,	51.40774512087615	),
		POINT(	28.622213518361956,	85.470675188875916	),
		BREAK_BETWEEN_POLYS,
		POINT(	28.622213518361956,	85.470675188875916	),
		POINT(	20,			120	),
		POINT(	-10,			150	),
		BREAK_BETWEEN_POLYS,
		POINT(	0,			0	),
		POINT(	0,			30	),
		POINT(	25.788044187940816,	38.570743740648808	),
		BREAK_BETWEEN_POLYS,
		POINT(	25.788044187940816,	38.570743740648808	),
		POINT(	40,			45	),
		POINT(	28.221837346670856,	51.40774512087615	),
		BREAK_BETWEEN_POLYS,
		POINT(	28.622213518361956,	85.470675188875916	),
		POINT(	45,			100	),
		POINT(	45,			130	),
		END_OF_LIST
	};

	return
	 partition_and_verify(input_polyline1_point_array,
	  input_polyline2_point_array,
	  expected_intersection_points_point_array,
	  expected_partitioned_polylines_point_array,
	  __func__);
}


typedef TestResults::TestResult (*test_fn_t)();

test_fn_t ALL_TESTS[] = {

	test_no_intersection_both_of_length_one,
	test_no_intersection_both_of_length_one_on_same_great_circle_1,
	test_no_intersection_both_of_length_one_on_same_great_circle_2,
	test_no_intersection_due_to_spherical_geometry,

	test_intersection_both_of_length_one,
	test_intersection_coincident_with_one_vertex_lengths_one_two,
	test_intersection_coincident_with_two_vertices_both_of_length_two,
	test_intersection_coincident_with_two_vertices_both_of_length_four,

	test_touching_end_to_end_no_intersection_both_of_length_one,
	test_touching_end_to_start_no_intersection_both_of_length_one,
	test_touching_start_to_end_no_intersection_both_of_length_one,
	test_touching_start_to_start_no_intersection_both_of_length_one,
	test_touching_end_to_end_no_intersection_both_of_length_two,
	test_touching_end_to_start_no_intersection_both_of_length_two,
	test_touching_start_to_end_no_intersection_both_of_length_two,
	test_touching_start_to_start_no_intersection_both_of_length_two,

	test_touching_end_to_mid_intersection_both_of_length_one,
	test_touching_start_to_mid_intersection_both_of_length_one,
	test_touching_end_to_mid_intersection_both_of_length_one_meet_perp,
	test_touching_start_to_mid_intersection_both_of_length_one_meet_perp,
	test_touching_end_to_mid_intersection_both_of_length_two_1,
	test_touching_end_to_mid_intersection_both_of_length_two_2,
	test_touching_end_to_mid_intersection_both_of_length_two_3,
	test_touching_end_to_mid_intersection_both_of_length_two_4,
	test_touching_end_to_mid_intersection_lengths_two_three,
	test_touching_end_to_vertex_intersection_both_of_length_two,

	test_overlap_defined_by_polyline1_parallel_arcs,
	test_overlap_defined_by_polyline1_antiparallel_arcs,
	test_overlap_defined_by_polyline2_parallel_arcs,
	test_overlap_defined_by_polyline2_antiparallel_arcs,

	test_same_polyline,

	test_overlap_partial_both_of_length_one_1,
	test_overlap_partial_both_of_length_one_2,
	test_overlap_partial_both_of_length_one_3,
	test_overlap_partial_both_of_length_one_4,
	test_overlap_partial_both_of_length_three_middle_segment_to_middle_1,
	test_overlap_partial_both_of_length_three_middle_segment_to_middle_2,
	test_overlap_partial_both_of_length_three_middle_segment_to_middle_3,
	test_overlap_partial_both_of_length_three_middle_segment_to_middle_4,

	test_multi_intersection_4,
	test_multi_intersection_5,
	test_multi_intersection_6,
	test_multi_intersection_7,
	test_multi_intersection_8,
	test_multi_intersection_9,
	test_multi_intersection_10,
	test_multi_intersection_11,
	test_multi_intersection_12,
	test_multi_intersection_13,
	test_multi_intersection_14,

	test_multi_overlap_1,
	test_multi_overlap_2,
	test_multi_overlap_3,
	test_multi_overlap_4,
	test_multi_overlap_5,
	test_multi_overlap_6,
	test_multi_overlap_7,
	test_multi_overlap_8,

	test_multi_overlap_and_intersection_1,
	test_multi_overlap_and_intersection_2,
	test_multi_overlap_and_intersection_3,
};


inline
TestResults::TestResult
run_test(
 test_fn_t f) {

	TestResults::TestResult res = TestResults::ERROR;
	try {

		res = (*f)();

	} catch (const GPlatesGlobal::Exception &e) {

		std::cerr << "ERROR: Caught Exception: " << e << std::endl;
	}
	return res;
}


void
run_tests() {

	unsigned num_passes = 0, num_fails = 0, num_errors = 0;

	test_fn_t *iter = ALL_TESTS, *end = ALL_TESTS + NUM_ELEMS(ALL_TESTS);
	for ( ; iter != end; ++iter) {

		TestResults::TestResult res = run_test(*iter);
		switch (res) {

		 case TestResults::PASS:

			++num_passes;
			break;

		case TestResults::FAIL:

			++num_fails;
			break;

		case TestResults::ERROR:

			++num_errors;
			break;
		}
	}
	std::cout << "\nNumber of passes: " << num_passes;
	std::cout << "\nNumber of fails:  " << num_fails;
	std::cout << "\nNumber of errors: " << num_errors << std::endl;
}


bool
GPlatesGui::GPlatesApp::OnInit() {

	run_tests();
	std::exit(0);

	// Normal start-up
	return true;
}


int
GPlatesGui::GPlatesApp::OnExit ()
{
	return 0;
}
