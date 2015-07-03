/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2005, 2006, 2008 The University of Sydney, Australia
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

#include <map>
#include <vector>
#include <iterator>  /* std::distance */
#include <algorithm>  /* std::find */
#include "PolylineIntersections.h"
#include "UnableToIntersectEquivalentGreatCirclesException.h"
#include "UnableToExtendPointlikeArcException.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"


namespace {

	/**
	 * Determine whether the list @a l contains (a copy of) the element
	 * @a e.
	 *
	 * This is a useful utility function.
	 */
	template< typename T >
	inline
	bool
	list_contains_elem(
	 const std::list< T > &l,
	 const T &e) {

		return (std::find(l.begin(), l.end(), e) != l.end());
	}


	using GPlatesMaths::PolylineIntersections::Graph;
	using GPlatesMaths::PointOnSphere;
	using GPlatesMaths::PolylineOnSphere;
	using GPlatesMaths::GreatCircleArc;

	typedef std::list< GreatCircleArc > ArcList;
	typedef std::list< ArcList::iterator > ArcListIterList;


	/**
	 * This struct represents the node (aka "vertex", in the graph-theory
	 * sense) of an intersection of two arc-lists.
	 *
	 * It contains the geometrical position of the point of intersection,
	 * as well as two iterators which describe the local network around the
	 * node.
	 */
	struct IntersectionNode {

		IntersectionNode(
		 const PointOnSphere &point,
		 ArcList::const_iterator first_after_on_arcs1,
		 ArcList::const_iterator first_after_on_arcs2) :
		 d_point(point),
		 d_first_after_on_arcs1(first_after_on_arcs1),
		 d_first_after_on_arcs2(first_after_on_arcs2) {  }

		/*
		 * The position of the point of intersection.
		 */
		const PointOnSphere d_point;

		/*
		 * These two iterators each serve a dual purpose:  They
		 * represent both the "end" iterator (the "next after last"
		 * iterator) of the list of arcs *before* the intersection, and
		 * the "begin" iterator of the list of arcs *after* the
		 * intersection.
		 *
		 * Since the 'splice_point_into_arc' function below promises to
		 * maintain the validity of iterators whose arcs experience the
		 * indignity of having a point spliced into them, these
		 * iterators will remain valid even if the arcs at which they
		 * point are subsequently butchered.
		 *
		 * FIXME:  The only things which need to be double-checked are
		 * what happens when the arcs subsequently intersect at
		 * endpoints-only, and what happens when the arcs subsequently
		 * overlap...
		 */
		const ArcList::const_iterator d_first_after_on_arcs1;
		const ArcList::const_iterator d_first_after_on_arcs2;

	};

	typedef std::list< IntersectionNode > IntersectionNodeList;

	/**
	 * Used to compare list iterators so they can be used in a std::map.
	 */
	struct IntersectionNodeMapIteratorCompare
	{
		bool
		operator()(
				const IntersectionNodeList::iterator &lhs,
				const IntersectionNodeList::iterator &rhs) const
		{
			return &*lhs < &*rhs;
		}
	};

	/**
	 * Typedef for a map to look up Graph intersections given a
	 * intersection node list iterator.
	 *
	 * This is used to share intersections between arcs in the first and
	 * second arc sequences.
	 */
	typedef std::map<
			IntersectionNodeList::iterator,
			Graph::intersection_ptr_type,
			IntersectionNodeMapIteratorCompare> intersection_node_map_type;


	/**
	 * This class instantiates to a function object which determines
	 * whether a PointOnSphere is coincident with the 'd_point' member of
	 * an IntersectionNode instance.
	 *
	 * See Josuttis99, Chapter 8 "STL Function Objects", and in particular
	 * section 8.2.4 "User-Defined Function Objects for Function Adapters",
	 * for more information about @a std::unary_function.
	 */
	class PointIsCoincident:
	 public std::unary_function< IntersectionNode, bool > {

	 public:

		explicit
		PointIsCoincident(
		 const IntersectionNode &node) :
		 d_node(node) {  }

		bool
		operator()(
		 const IntersectionNode &ii) const {

			 // The first two tests are added to avoid an assertion failure in
			 // 'add_intersection2_into_graph()' that occurs due to numerical precision when
			 // the intersection points are *not* quite coincident (within epsilon) but an arc
			 // of either polyline *is* coincident (within epsilon) of an intersection point.
			 // This causes two *distinct* intersection points to have the same arc before the
			 // intersection point (and same arc after).
			return ii.d_first_after_on_arcs1 == d_node.d_first_after_on_arcs1 ||
				ii.d_first_after_on_arcs2 == d_node.d_first_after_on_arcs2 ||
				points_are_coincident(d_node.d_point, ii.d_point);
		}

	 private:

		const IntersectionNode d_node;

	};


	/**
	 * Determine whether the list of intersection-nodes @a l contains an
	 * intersection-node whose position is coincident with @a node.
	 */
	inline
	bool
	list_contains_coincident_point(
	 const IntersectionNodeList &l,
	 const IntersectionNode &node) {

		PointIsCoincident predicate(node);
		return (std::find_if(l.begin(), l.end(), predicate) != l.end());
	}


	/**
	 * Append (a copy of) the intersection-node @a node to @a node_list.
	 */
	void
	append_to_intersection_node_list(
	 const IntersectionNode &node,
	 IntersectionNodeList &node_list) {

		// Avoid a duplication of nodes in the list.
		if ( ! list_contains_coincident_point(node_list, node)) {

			/*
			 * Duplication of nodes can arise because we're too
			 * lazy to invent a rigorous and correct system to
			 * avoid duplication of endpoint intersections.
			 *
			 * Picture, for example, two '\/'s touching
			 * vertex-to-vertex to form an 'X'; the vertex of the
			 * first '\/' is already in the list, now we've found
			 * the vertex of the second '\/'); we don't need to
			 * append the point of intersection again.  The same
			 * problem can occur when the polylines form a '-|-'
			 * intersection.
			 *
			 * (The second scenario occurs when the point of
			 * intersection coincides with a vertex of *one* of the
			 * polylines; the first scenario occurs when the point
			 * of intersection coincides with a vertex of *both*
			 * polylines.)
			 *
			 * For this reason, it is likely that if a node is
			 * going to be a duplicate of one already in the list,
			 * we are on the next polyline segment along, and thus,
			 * the node already in the list is the
			 * most-recently-appended node in the list.  Since the
			 * 'list_contains_elem' function iterates from the
			 * beginning of the list to the end, it makes sense to
			 * make our searches as short as possible, by inserting
			 * new nodes at the front of the list.
			 */
			node_list.push_front(node);
		}
	}


	/**
	 * This class instantiates to a function object which determines
	 * whether an ArcList iterator equals the 'd_first_after_on_arcs1'
	 * member of an IntersectionNode instance.
	 *
	 * See Josuttis99, Chapter 8 "STL Function Objects", and in particular
	 * section 8.2.4 "User-Defined Function Objects for Function Adapters",
	 * for more information about @a std::unary_function.
	 */
	class ArcListIterEqualsFirstAfterOnArcs1:
	 public std::unary_function< IntersectionNode, bool > {

	 public:

		explicit
		ArcListIterEqualsFirstAfterOnArcs1(
		 ArcList::const_iterator iter) :
		 d_iter(iter) {  }

		bool
		operator()(
		 const IntersectionNode &ii) const {

			return (d_iter == ii.d_first_after_on_arcs1);
		}

	 private:

		const ArcList::const_iterator d_iter;

	};


	/**
	 * This class instantiates to a function object which determines
	 * whether an ArcList iterator equals the 'd_first_after_on_arcs2'
	 * member of an IntersectionNode instance.
	 *
	 * See Josuttis99, Chapter 8 "STL Function Objects", and in particular
	 * section 8.2.4 "User-Defined Function Objects for Function Adapters",
	 * for more information about @a std::unary_function.
	 */
	class ArcListIterEqualsFirstAfterOnArcs2:
	 public std::unary_function< IntersectionNode, bool > {

	 public:

		explicit
		ArcListIterEqualsFirstAfterOnArcs2(
		 ArcList::const_iterator iter) :
		 d_iter(iter) {  }

		bool
		operator()(
		 const IntersectionNode &ii) const {

			return (d_iter == ii.d_first_after_on_arcs2);
		}

	 private:

		const ArcList::const_iterator d_iter;

	};


	/**
	 * This class instantiates to a function object which determines
	 * whether a GreatCircleArc is equivalent to another GreatCircleArc
	 * when the directedness of the arcs is ignored.
	 *
	 * See Josuttis99, Chapter 8 "STL Function Objects", and in particular
	 * section 8.2.4 "User-Defined Function Objects for Function Adapters",
	 * for more information about @a std::unary_function.
	 */
	class ArcIsUndirectedEquivalent:
	 public std::unary_function< GreatCircleArc, bool > {

	 public:

		explicit
		ArcIsUndirectedEquivalent(
		 const GreatCircleArc &arc) :
		 d_arc(arc) {  }

		bool
		operator()(
		 const GreatCircleArc &other_arc) const;

	 private:

		const GreatCircleArc d_arc;

	};


	bool
	ArcIsUndirectedEquivalent::operator()(
	 const GreatCircleArc &other_arc) const {

		const PointOnSphere
		 &arc1_start = d_arc.start_point(),
		 &arc1_end = d_arc.end_point(),
		 &arc2_start = other_arc.start_point(),
		 &arc2_end = other_arc.end_point();

		return ((points_are_coincident(arc1_start, arc2_start) &&
			 points_are_coincident(arc1_end, arc2_end)) ||
			(points_are_coincident(arc1_start, arc2_end) &&
			 points_are_coincident(arc1_end, arc2_start)));
	}


	/**
	 * Determine whether the arc-list @a l contains a great-circle-arc
	 * which is undirected-equivalent to @a arc.
	 */
	inline
	bool
	list_contains_undirected_equiv_arc(
	 const ArcList &l,
	 const GreatCircleArc &arc) {

		ArcIsUndirectedEquivalent predicate(arc);
		return (std::find_if(l.begin(), l.end(), predicate) != l.end());
	}


	/**
	 * Return the next iterator after @a i.
	 */
	inline
	ArcList::iterator
	next_iter_after(
	 ArcList::iterator i) {

		return ++i;
	}


	/**
	 * Splice @a point into the arc pointed-to by @a iter (a non-"end"
	 * iterator of @a arc_list), effectively replacing the one arc with
	 * two.
	 *
	 * This function will throw an exception if @a point is coincident-with
	 * or antipodal-to either endpoint of the arc pointed-to by @a iter.
	 *
	 * Other points of note:
	 *
	 *  - @a point doesn't need to lie on the arc pointed-to by @a iter.
	 *
	 *  - The caller must ensure that @a iter points to a non-"end"
	 *    location in @a arc_list; otherwise, the behaviour is undefined.
	 *
	 *  - @a iter will @em not be invalidated by this operation.  Any other
	 *    iterators, which point to the arc at which @a iter points, will
	 *    likewise remain valid.
	 *
	 *  - This function is strongly exception-safe (that is, if it
	 *    terminates due to an exception, program state will remain
	 *    unchanged) and exception-neutral (that is, any exceptions are
	 *    propagated to the caller).
	 */
	void
	splice_point_into_arc(
	 const PointOnSphere &point,
	 ArcList::iterator arc_iter,
	 ArcList &arc_list) {

		ArcList new_arcs;

		GreatCircleArc replacement_arc1 =
		 GreatCircleArc::create(arc_iter->start_point(), point);
		GreatCircleArc replacement_arc2 =
		 GreatCircleArc::create(point, arc_iter->end_point());

		new_arcs.push_back(replacement_arc2);

		/*
		 * From here onward won't throw as long as GreatCircleArc
		 * assignment doesn't throw.
		 */

		// First, replace the arc at which 'arc_iter' points using the
		// arc's assignment operator, leaving 'arc_iter' valid.
		*arc_iter = replacement_arc1;

		// In case you can't read STL, this next line basically says:
		// "Splice the contents of 'new_arcs' in between 'arc_iter' and
		// the next iterator after 'arc_iter'."
		arc_list.splice(next_iter_after(arc_iter), new_arcs);
	}


	/**
	 * If necessary, partition the arc pointed-at by @a arc_iter (an
	 * iterator pointing to a non-"end" location in @a arc_list) at the
	 * point of intersection @a inter_point.
	 *
	 * The partitioning of the arc will result in the arc being split in
	 * two, with a new vertex appearing at the point of intersection.  It
	 * is "not necessary" to partition the arc if there is already a vertex
	 * at the point of intersection, ie, if the point of intersection is
	 * coincident with an endpoint of the arc.
	 *
	 * This function does not attempt to be strongly exception-safe (since
	 * any parameters it might happen to alter are assumed to be local to
	 * the enclosing function 'partition_intersecting_geometries', and hence
	 * will be destroyed anyway if an exception is thrown), but it *is*
	 * exception-neutral.
	 */
	void
	partition_arc_if_necessary(
	 ArcList::iterator arc_iter,
	 ArcList &arc_list,
	 const PointOnSphere &inter_point) {

		/*
		 * Recall that we have ensured that the *polylines* are not
		 * touching endpoint-to-endpoint (since that shouldn't be
		 * counted as an intersection), but we haven't evaluated
		 * whether or not the point of intersection was coincident with
		 * either endpoint of *just this arc*.
		 */
		if (points_are_coincident(arc_iter->start_point(), inter_point)
		    ||
		    points_are_coincident(arc_iter->end_point(), inter_point)) {

			// The point of intersection is coincident with one of
			// the arc endpoints, so there's no need to partition
			// the arc.

		} else {

			// Note that 'splice_point_into_arc' guarantees that
			// 'arc_iter' will remain valid.
			splice_point_into_arc(inter_point, arc_iter, arc_list);
		}
	}


	/**
	 * Return the first iterator after the point of intersection.
	 *
	 * This function is useful during the creation of instances of
	 * IntersectionNode.  It is assumed that @a iter points to either the
	 * arc which contains @a inter_point as a start-point, or an arc before
	 * that arc in the arc-list.  It will return the iterator pointing to
	 * the arc which contains @a inter_point as a start-point.
	 */
	inline
	ArcList::iterator
	return_first_after_intersection(
	 ArcList::iterator iter,
	 ArcList::iterator end,  // This is a crude hack to handle end-of-list.
	 const PointOnSphere &inter_point) {

		while ( ! points_are_coincident(iter->start_point(),
						inter_point)) {

			// Not this arc.  Try the next one...
			if (++iter == end) {

				// This is a crude hack to handle end-of-list.
				// FIXME:  Do this better.  Kill this function?
				break;
			}
		}
		return iter;
	}


	/**
	 * Handle the situation in which the arcs pointed-to by @a iter1 and
	 * @a iter2 have been found to intersect at the point @a inter_point.
	 *
	 * This "handling" may include partitioning the arcs at the point of
	 * intersection, as well as keeping track of the point of intersection
	 * by appending it to the list @a inter_nodes.
	 *
	 * This function does not attempt to be strongly exception-safe (since
	 * any parameters it might happen to alter are assumed to be local to
	 * the enclosing function 'partition_intersecting_geometries', and hence
	 * will be destroyed anyway if an exception is thrown), but it *is*
	 * exception-neutral.
	 */
	void
	handle_intersection(
	 ArcList &arcs1,
	 ArcList &arcs2,
	 ArcList::iterator &iter1,
	 ArcList::iterator &iter2,
	 IntersectionNodeList &inter_nodes,
	 const PointOnSphere &inter_point) {

		const PointOnSphere &arc1_start_pt = iter1->start_point();
		const PointOnSphere &arc1_end_pt = iter1->end_point();
		const PointOnSphere &arc2_start_pt = iter2->start_point();
		const PointOnSphere &arc2_end_pt = iter2->end_point();

		/*
		 * Remember the "one exception" described in the comment of
		 * 'partition_intersecting_geometries':  If the polylines are
		 * touching endpoint-to-endpoint, this will not be counted as
		 * an intersection.
		 */
		if ((iter1 == arcs1.begin() &&
		     iter2 == arcs2.begin() &&
		     points_are_coincident(arc1_start_pt, arc2_start_pt)) ||
		    (iter1 == arcs1.begin() &&
		     next_iter_after(iter2) == arcs2.end() &&
		     points_are_coincident(arc1_start_pt, arc2_end_pt)) ||
		    (next_iter_after(iter1) == arcs1.end() &&
		     iter2 == arcs2.begin() &&
		     points_are_coincident(arc1_end_pt, arc2_start_pt)) ||
		    (next_iter_after(iter1) == arcs1.end() &&
		     next_iter_after(iter2) == arcs2.end() &&
		     points_are_coincident(arc1_end_pt, arc2_end_pt))) {

			/*
			 * Observe that since there can be at most one point of
			 * intersection of any two non-overlapping GCAs, if the
			 * endpoints are coincident, they must be the point of
			 * intersection.
			 * 
			 * Thus, if the above if-expression evaluated to true,
			 * the polylines are touching endpoint-to-endpoint, and
			 * furthermore, this endpoint-to-endpoint touching is
			 * the point of intersection.
			 */
			return;
		}

		partition_arc_if_necessary(iter1, arcs1, inter_point);
		partition_arc_if_necessary(iter2, arcs2, inter_point);

		ArcList::iterator first_after_on_arc_1 =
		 return_first_after_intersection(iter1,
		  /* this is a crude hack */  arcs1.end(),
		  inter_point);
		ArcList::iterator first_after_on_arc_2 =
		 return_first_after_intersection(iter2,
		  /* this is a crude hack */  arcs2.end(),
		  inter_point);

		append_to_intersection_node_list(IntersectionNode(inter_point,
		  first_after_on_arc_1, first_after_on_arc_2), inter_nodes);
	}


	/**
	 * Determine whether the arcs pointed-to by @a iter1 and @a iter2
	 * "overlap" at their endpoints "only": if they do, handle this
	 * situation and return true; if not, return false.
	 *
	 * The arcs have been found to lie on the same great-circle (hence the
	 * use of the term "overlap"), but this function will determine whether
	 * they "only" "overlap" at their endpoints (ie, the only contact is
	 * endpoint-to-endpoint in some arrangement).
	 *
	 * If the arcs @em are touching endpoint-to-endpoint, this will be
	 * treated as an intersection.
	 *
	 * This function does not attempt to be strongly exception-safe (since
	 * any parameters it might happen to alter are assumed to be local to
	 * the enclosing function 'partition_intersecting_geometries', and hence
	 * will be destroyed anyway if an exception is thrown), but it *is*
	 * exception-neutral.
	 */
	bool
	handle_possible_endpoint_only_overlap(
	 ArcList &arcs1,
	 ArcList &arcs2,
	 ArcList::iterator &iter1,
	 ArcList::iterator &iter2,
	 IntersectionNodeList &inter_nodes) {

		const GreatCircleArc &arc1 = *iter1;
		const GreatCircleArc &arc2 = *iter2;

		const PointOnSphere &arc1_start_pt = arc1.start_point();
		const PointOnSphere &arc1_end_pt = arc1.end_point();
		const PointOnSphere &arc2_start_pt = arc2.start_point();
		const PointOnSphere &arc2_end_pt = arc2.end_point();

		if (unit_vectors_are_parallel(arc1.rotation_axis(), arc2.rotation_axis())) {

			/*
			 * The axes of the arcs are parallel, which means the
			 * arcs are "rotating" in the same direction.
			 *
			 * Hence, the arrangements which would cause the arcs
			 * to be touching endpoint-to-endpoint-only are:
			 *
			 *             |        | -- direction of rotation axis
			 *         X--arc1->X--arc2->
			 * and:
			 *             |        |
			 *         X--arc2->X--arc1->
			 */
			if (points_are_coincident(arc1_end_pt, arc2_start_pt)) {

				const PointOnSphere &point_of_intersection =
				 arc1_end_pt;
				handle_intersection(arcs1, arcs2, iter1, iter2,
				 inter_nodes, point_of_intersection);

				return true;
			}
			if (points_are_coincident(arc2_end_pt, arc1_start_pt)) {

				const PointOnSphere &point_of_intersection =
				 arc2_end_pt;
				handle_intersection(arcs1, arcs2, iter1, iter2,
				 inter_nodes, point_of_intersection);

				return true;
			}

			// Else, the arcs are not touching
			// endpoint-to-endpoint only.
			return false;

		} else {

			/*
			 * The axes of the arcs are anti-parallel, which means
			 * the arcs are "rotating" in the opposite direction.
			 *
			 * Hence, the arrangements which would cause the arcs
			 * to be touching endpoint-to-endpoint-only are:
			 *
			 *            |
			 *        X--arc1-><-arc2--X
			 *                     |
			 * or:
			 *                     |
			 *        <-arc2--XX--arc1->
			 *            |
			 */
			if (points_are_coincident(arc1_end_pt, arc2_end_pt)) {

				const PointOnSphere &point_of_intersection =
				 arc1_end_pt;
				handle_intersection(arcs1, arcs2, iter1, iter2,
				 inter_nodes, point_of_intersection);

				return true;
			}
			if (points_are_coincident(arc1_start_pt,
						  arc2_start_pt)) {

				const PointOnSphere &point_of_intersection =
				 arc1_start_pt;
				handle_intersection(arcs1, arcs2, iter1, iter2,
				 inter_nodes, point_of_intersection);

				return true;
			}

			// Else, the arcs are not touching
			// endpoint-to-endpoint only.
			return false;
		}
	}


	/**
	 * If necessary, partition the arc pointed-at by @a iter_at_longer_arc
	 * (an iterator pointing to a non-"end" location in
	 * @a arcs_containing_longer_arc) at the points of intersection
	 * @a earlier_inter_point and @a later_inter_point; regardless of
	 * whether or not partitioning was necessary, return an iterator
	 * pointing at the arc in @a arcs_containing_longer_arc delimited by
	 * @a earlier_inter_point and a later_inter_point.
	 *
	 * @a earlier_inter_point is the intersection point which is closer to
	 * the start of the arc; @a later_inter_point is closer to the end of
	 * the arc.
	 *
	 * The partitioning of the arc will result in the arc being split in
	 * two or three, with a new vertex appearing at each point of
	 * intersection.  It is "not necessary" to partition the arc at either
	 * of the points of intersection if there is already a vertex at that
	 * point, ie, if the point of intersection is coincident with an
	 * endpoint of the arc.
	 *
	 * This function does not attempt to be strongly exception-safe (since
	 * any parameters it might happen to alter are assumed to be local to
	 * the enclosing function 'partition_intersecting_geometries', and hence
	 * will be destroyed anyway if an exception is thrown), but it *is*
	 * exception-neutral.
	 */
	ArcList::iterator
	partition_longer_arc_if_necessary(
	 ArcList::iterator iter_at_longer_arc,
	 ArcList &arcs_containing_longer_arc,
	 const PointOnSphere &earlier_inter_point,
	 const PointOnSphere &later_inter_point) {

		ArcList::iterator iter_at_overlapping_arc_in_longer;

		if (points_are_coincident(iter_at_longer_arc->start_point(),
					  earlier_inter_point)) {

			// The earlier point of intersection is coincident with
			// the start-point of the longer-arc, so there's no
			// need to partition the longer-arc at the earlier
			// point of intersection.

			iter_at_overlapping_arc_in_longer = iter_at_longer_arc;

		} else {

			splice_point_into_arc(earlier_inter_point,
			 iter_at_longer_arc, arcs_containing_longer_arc);

			iter_at_overlapping_arc_in_longer =
			 next_iter_after(iter_at_longer_arc);

		}

		if (points_are_coincident(
		     iter_at_overlapping_arc_in_longer->end_point(),
		     later_inter_point)) {

			// The later point of intersection is coincident with
			// the end-point of the longer-arc, so there's no need
			// to partition the longer-arc at the later point of
			// intersection.

		} else {

			splice_point_into_arc(later_inter_point,
			 iter_at_overlapping_arc_in_longer,
			 arcs_containing_longer_arc);
		}

		return iter_at_overlapping_arc_in_longer;
	}


	/**
	 * If necsessary, partition the arc pointed-at by @a iter_at_longer_arc
	 * (an iterator pointing to a non-"end" location in
	 * @a arcs_containing_longer_arc) at the sub-arc of overlap defined by
	 * @a iter_at_defining_arc (an iterator pointing to a non-"end"
	 * location in @a arcs_containing_defining_arc); regardless of whether
	 * or not partitioning was necessary, return an iterator pointing at
	 * the arc in @a arcs_containing_longer_arc delimited by the endpoints
	 * of the defining arc.
	 *
	 * The partitioning of the longer arc will result in it being split in
	 * two or three, with a new vertex appearing at each endpoint of the
	 * defining arc.  It is "not necessary" to partition the longer arc at
	 * either of the endpoints of the defining arc if there is already a
	 * vertex at that point, ie, if the endpoint of the defining arc is
	 * coincident with an endpoint of the longer arc.
	 *
	 * This function does not attempt to be strongly exception-safe (since
	 * any parameters it might happen to alter are assumed to be local to
	 * the enclosing function 'partition_intersecting_geometries', and hence
	 * will be destroyed anyway if an exception is thrown), but it *is*
	 * exception-neutral.
	 */
	ArcList::iterator
	partition_overlap_of_one_defining_arc_if_necessary(
	 ArcList::iterator &iter_at_defining_arc,
	 ArcList &arcs_containing_defining_arc,
	 ArcList::iterator &iter_at_longer_arc,
	 ArcList &arcs_containing_longer_arc) {

		GreatCircleArc &defining_arc = *iter_at_defining_arc;
		GreatCircleArc &longer_arc = *iter_at_longer_arc;

		ArcList::iterator iter_at_overlapping_arc_in_longer;

		if (unit_vectors_are_parallel(defining_arc.rotation_axis(), longer_arc.rotation_axis())) {

			/*
			 * The arcs will be:
			 *
			 *                       |
			 *               X-defining_arc-->
			 *               ^               ^
			 *                       |
			 *            X--o--longer_arc---o-->
			 *               ^               ^
			 * earlier_inter_point       later_inter_point
			 */

			const PointOnSphere
			 &earlier_inter_point = defining_arc.start_point(),
			 &later_inter_point = defining_arc.end_point();

			iter_at_overlapping_arc_in_longer =
			 partition_longer_arc_if_necessary(iter_at_longer_arc,
			  arcs_containing_longer_arc, earlier_inter_point,
			  later_inter_point);

		} else {

			/*
			 * The arcs will be:
			 *
			 *               <--defining_arc-X
			 *               ^       |       ^
			 *
			 *                       |
			 *            X--o--longer_arc---o-->
			 *               ^               ^
			 * earlier_inter_point       later_inter_point
			 */

			const PointOnSphere
			 &earlier_inter_point = defining_arc.end_point(),
			 &later_inter_point = defining_arc.start_point();

			iter_at_overlapping_arc_in_longer =
			 partition_longer_arc_if_necessary(iter_at_longer_arc,
			  arcs_containing_longer_arc, earlier_inter_point,
			  later_inter_point);
		}

		return iter_at_overlapping_arc_in_longer;
	}


	/**
	 * Create intersection nodes for the overlapping arcs pointed-at by
	 * @a iter_at_overlap_arc_in_arcs1 and @a iter_at_overlap_arc_in_arcs2,
	 * and append these new nodes to @a inter_nodes.
	 *
	 * This function does not attempt to be strongly exception-safe (since
	 * any parameters it might happen to alter are assumed to be local to
	 * the enclosing function 'partition_intersecting_geometries', and hence
	 * will be destroyed anyway if an exception is thrown), but it *is*
	 * exception-neutral.
	 */
	void
	append_intersection_nodes_for_overlap(
	 const ArcList::iterator &iter_at_overlap_arc_in_arcs1,
	 const ArcList::iterator &iter_at_overlap_arc_in_arcs2,
	 IntersectionNodeList &inter_nodes) {

		const GreatCircleArc &arc1 = *iter_at_overlap_arc_in_arcs1;
		const GreatCircleArc &arc2 = *iter_at_overlap_arc_in_arcs2;

		/*
		 * Obviously (because the arcs are overlapping) the rotation
		 * axes must be "absolutely" parallel (ie, pointing along the
		 * same line, parallel down to a multiplication by -1), but do
		 * they point in the same direction or opposite directions? 
		 *
		 * We want to know whether the arcs are "rotating" in the same
		 * direction or opposite directions.
		 */
		if (unit_vectors_are_parallel(arc1.rotation_axis(), arc2.rotation_axis())) {

			// The arcs are rotating in the same direction.  Thus,
			// 'arc1.start_point()' will be the earlier point of
			// intersection for both polylines.
			append_to_intersection_node_list(
			 IntersectionNode(arc1.start_point(),
			  iter_at_overlap_arc_in_arcs1,
			  iter_at_overlap_arc_in_arcs2),
			 inter_nodes);
			append_to_intersection_node_list(
			 IntersectionNode(arc1.end_point(),
			  next_iter_after(iter_at_overlap_arc_in_arcs1),
			  next_iter_after(iter_at_overlap_arc_in_arcs2)),
			 inter_nodes);

		} else {

			// The arcs are rotating in opposite directions.  Thus,
			// 'arc1.start_point()' will be the earlier point of
			// intersection for 'arcs1' but the later point of
			// intersection for 'arcs2'.
			append_to_intersection_node_list(
			 IntersectionNode(arc1.start_point(),
			  iter_at_overlap_arc_in_arcs1,
			  next_iter_after(iter_at_overlap_arc_in_arcs2)),
			 inter_nodes);
			append_to_intersection_node_list(
			 IntersectionNode(arc1.end_point(),
			  next_iter_after(iter_at_overlap_arc_in_arcs1),
			  iter_at_overlap_arc_in_arcs2),
			 inter_nodes);
		}
	}


	/**
	 * Handle the possible overlap of the arc pointed-at by @a iter1 (an
	 * iterator pointing to a non-"end" location in @a arcs1) and the arc
	 * pointed-at by @a iter2 (an iterator pointing to a non-"end" location
	 * in @a arcs2).
	 *
	 * The arcs are "possibly overlapping" because they have been found to
	 * lie on the same great-circle.  This means that they may have zero
	 * intersections, one intersection (ie, endpoint-to-endpoint touching)
	 * or an infinite number of intersections (ie, overlap).
	 *
	 * This function does not attempt to be strongly exception-safe (since
	 * any parameters it might happen to alter are assumed to be local to
	 * the enclosing function 'partition_intersecting_geometries', and hence
	 * will be destroyed anyway if an exception is thrown), but it *is*
	 * exception-neutral.
	 */
	void
	handle_possible_overlap(
	 ArcList &arcs1,
	 ArcList &arcs2,
	 ArcList::iterator &iter1,
	 ArcList::iterator &iter2,
	 IntersectionNodeList &inter_nodes,
	 ArcListIterList &overlap_arcs_in_1,
	 ArcListIterList &overlap_arcs_in_2) {

		/*
		 * Observe that there's still a chance the arcs are touching
		 * endpoint-to-endpoint only, which would mean a unique point
		 * of intersection, which would mean a call to
		 * 'handle_intersection'.
		 *
		 * Let's test for this trivial case first, since attempting to
		 * create an overlap arc from two coincident points would
		 * result in an exception being thrown back at us.
		 */
		if (handle_possible_endpoint_only_overlap(arcs1, arcs2, iter1,
		    iter2, inter_nodes)) {

			// The arcs did indeed touch endpoint-to-endpoint only.
			// This was treated as an intersection, and processed
			// accordingly.  Nothing more to do here.
			return;
		}

		const GreatCircleArc &arc1 = *iter1;
		const GreatCircleArc &arc2 = *iter2;

		bool arc2_start_point_lies_on_arc1 =
		 arc2.start_point().lies_on_gca(arc1);

		bool arc2_end_point_lies_on_arc1 =
		 arc2.end_point().lies_on_gca(arc1);

		bool arc1_start_point_lies_on_arc2 =
		 arc1.start_point().lies_on_gca(arc2);

		bool arc1_end_point_lies_on_arc2 =
		 arc1.end_point().lies_on_gca(arc2);

		/*
		 * Note that the order of these tests is important.  We have to
		 * test for the both-endpoints-of-one-arc-lie-on-the-other-arc
		 * cases before we test for the mixed cases, so that there's no
		 * chance of picking coincident endpoints in situations when
		 * the two arcs start or end at the same point.
		 *
		 * Don't follow?  The POWER of Asky Art COMPELS YOU!
		 *
		 * Say we have arc1:
		 *   \---------\
		 *
		 * and arc2:
		 *   /---------/
		 *
		 * Now, the arcs could share one endpoint:
		 *   X--------\--/
		 *
		 * Or two endpoints:
		 *   X---------X
		 *
		 * In such cases, if we arbitrarily picked one endpoint of arc1
		 * and one of arc2, there's a chance we could pick coincident
		 * points, which don't correctly represent the extent of the
		 * overlap.  Thus, we test each arc individually before testing
		 * the arcs together.
		 */
		if (arc2_start_point_lies_on_arc1 &&
		    arc2_end_point_lies_on_arc1) {

			// 'arc2' defines the extent of the overlap.
			ArcList::iterator &iter_at_defining_arc = iter2;
			ArcList &arcs_containing_defining_arc = arcs2;

			ArcList::iterator &iter_at_longer_arc = iter1;
			ArcList &arcs_containing_longer_arc = arcs1;

			// First, partition the longer arc if necessary.
			ArcList::iterator iter_at_overlapping_arc_in_longer =
			 partition_overlap_of_one_defining_arc_if_necessary(
			  iter_at_defining_arc, arcs_containing_defining_arc,
			  iter_at_longer_arc, arcs_containing_longer_arc);

			// Now, update the list of intersection-nodes.
			ArcList::iterator &iter_at_overlapping_arc_in_arcs1 =
			 iter_at_overlapping_arc_in_longer;
			ArcList::iterator &iter_at_overlapping_arc_in_arcs2 =
			 iter_at_defining_arc;

			append_intersection_nodes_for_overlap(
			 iter_at_overlapping_arc_in_arcs1,
			 iter_at_overlapping_arc_in_arcs2,
			 inter_nodes);

			// And finally, the lists of overlaps.
			overlap_arcs_in_1.push_back(
			 iter_at_overlapping_arc_in_arcs1);
			overlap_arcs_in_2.push_back(
			 iter_at_overlapping_arc_in_arcs2);

		} else if (arc1_start_point_lies_on_arc2 &&
			   arc1_end_point_lies_on_arc2) {

			// 'arc1' defines the extent of the overlap.
			ArcList::iterator &iter_at_defining_arc = iter1;
			ArcList &arcs_containing_defining_arc = arcs1;

			ArcList::iterator &iter_at_longer_arc = iter2;
			ArcList &arcs_containing_longer_arc = arcs2;

			ArcList::iterator iter_at_overlapping_arc_in_longer =
			 partition_overlap_of_one_defining_arc_if_necessary(
			  iter_at_defining_arc, arcs_containing_defining_arc,
			  iter_at_longer_arc, arcs_containing_longer_arc);

			// Now, update the list of intersection-nodes.
			ArcList::iterator &iter_at_overlapping_arc_in_arcs1 =
			 iter_at_defining_arc;
			ArcList::iterator &iter_at_overlapping_arc_in_arcs2 =
			 iter_at_overlapping_arc_in_longer;

			append_intersection_nodes_for_overlap(
			 iter_at_overlapping_arc_in_arcs1,
			 iter_at_overlapping_arc_in_arcs2,
			 inter_nodes);

			// And finally, the lists of overlaps.
			overlap_arcs_in_1.push_back(
			 iter_at_overlapping_arc_in_arcs1);
			overlap_arcs_in_2.push_back(
			 iter_at_overlapping_arc_in_arcs2);

		} else if (arc1_start_point_lies_on_arc2 &&
			   arc2_start_point_lies_on_arc1) {

			/*
			 * arc1:     ------->
			 * arc2: <-------
			 *
			 * (The actual direction of arc1 is unimportant; only
			 * the relative directions of arc1 and arc2, as well as
			 * their relative positions, are important.)
			 */
			PointOnSphere ip_on_arc1 = arc2.start_point();
			PointOnSphere ip_on_arc2 = arc1.start_point();

			// Note that 'splice_point_into_arc' guarantees that
			// 'iter1' and 'iter2' will remain valid.
			splice_point_into_arc(ip_on_arc1, iter1, arcs1);
			splice_point_into_arc(ip_on_arc2, iter2, arcs2);

			/*
			 * arc1:     ----o--->
			 * arc2: <---o----
			 */

			ArcList::iterator iter_at_overlapping_arc_in_arcs1 =
			 iter1;
			ArcList::iterator iter_at_overlapping_arc_in_arcs2 =
			 iter2;

			append_to_intersection_node_list(
			 IntersectionNode(ip_on_arc2,
			  iter_at_overlapping_arc_in_arcs1,
			  next_iter_after(iter_at_overlapping_arc_in_arcs2)),
			 inter_nodes);
			append_to_intersection_node_list(
			 IntersectionNode(ip_on_arc1,
			  next_iter_after(iter_at_overlapping_arc_in_arcs1),
			  iter_at_overlapping_arc_in_arcs2),
			 inter_nodes);

			overlap_arcs_in_1.push_back(
			 iter_at_overlapping_arc_in_arcs1);
			overlap_arcs_in_2.push_back(
			 iter_at_overlapping_arc_in_arcs2);

		} else if (arc1_start_point_lies_on_arc2 &&
			   arc2_end_point_lies_on_arc1) {

			/*
			 * arc1:     ------->
			 * arc2: ------->
			 *
			 * (The actual direction of arc1 is unimportant; only
			 * the relative directions of arc1 and arc2, as well as
			 * their relative positions, are important.)
			 */
			PointOnSphere ip_on_arc1 = arc2.end_point();
			PointOnSphere ip_on_arc2 = arc1.start_point();

			// Note that 'splice_point_into_arc' guarantees that
			// 'iter1' and 'iter2' will remain valid.
			splice_point_into_arc(ip_on_arc1, iter1, arcs1);
			splice_point_into_arc(ip_on_arc2, iter2, arcs2);

			/*
			 * arc1:     ---o--->
			 * arc2: ----o-->
			 */

			ArcList::iterator iter_at_overlapping_arc_in_arcs1 =
			 iter1;
			ArcList::iterator iter_at_overlapping_arc_in_arcs2 =
			 next_iter_after(iter2);

			append_to_intersection_node_list(
			 IntersectionNode(ip_on_arc2,
			  iter_at_overlapping_arc_in_arcs1,
			  iter_at_overlapping_arc_in_arcs2),
			 inter_nodes);
			append_to_intersection_node_list(
			 IntersectionNode(ip_on_arc1,
			  next_iter_after(iter_at_overlapping_arc_in_arcs1),
			  next_iter_after(iter_at_overlapping_arc_in_arcs2)),
			 inter_nodes);

			overlap_arcs_in_1.push_back(
			 iter_at_overlapping_arc_in_arcs1);
			overlap_arcs_in_2.push_back(
			 iter_at_overlapping_arc_in_arcs2);
  
		} else if (arc1_end_point_lies_on_arc2 &&
			   arc2_start_point_lies_on_arc1) {

			/*
			 * arc1: ------->
			 * arc2:     ------->
			 *
			 * (The actual direction of arc1 is unimportant; only
			 * the relative directions of arc1 and arc2, as well as
			 * their relative positions, are important.)
			 */
			PointOnSphere ip_on_arc1 = arc2.start_point();
			PointOnSphere ip_on_arc2 = arc1.end_point();

			// Note that 'splice_point_into_arc' guarantees that
			// 'iter1' and 'iter2' will remain valid.
			splice_point_into_arc(ip_on_arc1, iter1, arcs1);
			splice_point_into_arc(ip_on_arc2, iter2, arcs2);

			/*
			 * arc1: ----o-->
			 * arc2:     ---o--->
			 */

			ArcList::iterator iter_at_overlapping_arc_in_arcs1 =
			 next_iter_after(iter1);
			ArcList::iterator iter_at_overlapping_arc_in_arcs2 =
			 iter2;

			append_to_intersection_node_list(
			 IntersectionNode(ip_on_arc1,
			  iter_at_overlapping_arc_in_arcs1,
			  iter_at_overlapping_arc_in_arcs2),
			 inter_nodes);
			append_to_intersection_node_list(
			 IntersectionNode(ip_on_arc2,
			  next_iter_after(iter_at_overlapping_arc_in_arcs1),
			  next_iter_after(iter_at_overlapping_arc_in_arcs2)),
			 inter_nodes);

			overlap_arcs_in_1.push_back(
			 iter_at_overlapping_arc_in_arcs1);
			overlap_arcs_in_2.push_back(
			 iter_at_overlapping_arc_in_arcs2);

		} else if (arc1_end_point_lies_on_arc2 &&
			   arc2_end_point_lies_on_arc1) {

			/*
			 * arc1: ------->
			 * arc2:     <-------
			 *
			 * (The actual direction of arc1 is unimportant; only
			 * the relative directions of arc1 and arc2, as well as
			 * their relative positions, are important.)
			 */
			PointOnSphere ip_on_arc1 = arc2.end_point();
			PointOnSphere ip_on_arc2 = arc1.end_point();

			// Note that 'splice_point_into_arc' guarantees that
			// 'iter1' and 'iter2' will remain valid.
			splice_point_into_arc(ip_on_arc1, iter1, arcs1);
			splice_point_into_arc(ip_on_arc2, iter2, arcs2);

			/*
			 * arc1: ----o-->
			 * arc2:     <--o----
			 */

			ArcList::iterator iter_at_overlapping_arc_in_arcs1 =
			 next_iter_after(iter1);
			ArcList::iterator iter_at_overlapping_arc_in_arcs2 =
			 next_iter_after(iter2);

			append_to_intersection_node_list(
			 IntersectionNode(ip_on_arc1,
			  iter_at_overlapping_arc_in_arcs1,
			  next_iter_after(iter_at_overlapping_arc_in_arcs2)),
			 inter_nodes);
			append_to_intersection_node_list(
			 IntersectionNode(ip_on_arc2,
			  next_iter_after(iter_at_overlapping_arc_in_arcs1),
			  iter_at_overlapping_arc_in_arcs2),
			 inter_nodes);

			overlap_arcs_in_1.push_back(
			 iter_at_overlapping_arc_in_arcs1);
			overlap_arcs_in_2.push_back(
			 iter_at_overlapping_arc_in_arcs2);
		}
		// Else, no overlap.
	}


	/**
	 * Calculate the two unique (antipodal) points at which these two great-circle arcs would
	 * intersect if they were "extended" to whole great-circles.
	 *
	 * It is assumed that these arcs lie on distinct great-circles; if they do indeed lie on
	 * equivalent great-circles, an UnableToIntersectEquivalentGreatCirclesException will be
	 * thrown.
	 *
	 * It is also assumed that neither of the arcs are pointlike; if either arc @em is
	 * pointlike, an UnableToExtendPointlikeArcException will be thrown.
	 */
	const std::pair<PointOnSphere, PointOnSphere>
	calculate_intersections_of_extended_arcs(
			const GreatCircleArc &arc1,
			const GreatCircleArc &arc2)
	{
		if (arcs_lie_on_same_great_circle(arc1, arc2)) {
			throw GPlatesMaths::UnableToIntersectEquivalentGreatCirclesException(
					GPLATES_EXCEPTION_SOURCE, arc1, arc2);
		}
		if (arc1.is_zero_length()) {
			throw GPlatesMaths::UnableToExtendPointlikeArcException(GPLATES_EXCEPTION_SOURCE,
					arc1);
		}
		if (arc2.is_zero_length()) {
			throw GPlatesMaths::UnableToExtendPointlikeArcException(GPLATES_EXCEPTION_SOURCE,
					arc2);
		}

		GPlatesMaths::Vector3D v = cross(arc1.rotation_axis(), arc2.rotation_axis());
		GPlatesMaths::UnitVector3D normalised_v = v.get_normalisation();

		PointOnSphere inter_point1(normalised_v);
		PointOnSphere inter_point2( -normalised_v);

		return std::make_pair(inter_point1, inter_point2);
	}


	/**
	 * Handle the possible intersection of the arc pointed-at by @a iter1
	 * (an iterator pointing to a non-"end" location in @a arcs1) and the
	 * arc pointed-at by @a iter2 (an iterator pointing to a non-"end"
	 * location in @a arcs2).
	 *
	 * The arcs are "possibly intersecting" because they have been found to
	 * lie on distinct great-circles.  This means that they may have zero
	 * or one intersections; the intersection test requires that they lie
	 * on distinct great-circles.
	 *
	 * This function does not attempt to be strongly exception-safe (since
	 * any parameters it might happen to alter are assumed to be local to
	 * the enclosing function 'partition_intersecting_geometries', and hence
	 * will be destroyed anyway if an exception is thrown), but it *is*
	 * exception-neutral.
	 */
	void
	handle_possible_intersection(
	 ArcList &arcs1,
	 ArcList &arcs2,
	 ArcList::iterator &iter1,
	 ArcList::iterator &iter2,
	 IntersectionNodeList &inter_nodes) {

		const GreatCircleArc &arc1 = *iter1;
		const GreatCircleArc &arc2 = *iter2;

		/*
		 * Since (as we have already established) 'arc1' and 'arc2' do
		 * not lie on the same great-circle, the distinct great-circles
		 * on which they lie must have two unique (antipodal) points of
		 * intersection.  So we now want to determine whether either of
		 * these points of intersection lies on both arcs, in which
		 * case, the arcs intersect.
		 */
		std::pair< PointOnSphere, PointOnSphere >
		 possible_inter_points =
		  calculate_intersections_of_extended_arcs(arc1, arc2);

		if (possible_inter_points.first.lies_on_gca(arc1) &&
		    possible_inter_points.first.lies_on_gca(arc2)) {

			const PointOnSphere &point_of_intersection =
			 possible_inter_points.first;
			handle_intersection(arcs1, arcs2, iter1, iter2,
			 inter_nodes, point_of_intersection);

		} else if (possible_inter_points.second.lies_on_gca(arc1) &&
			   possible_inter_points.second.lies_on_gca(arc2)) {

			const PointOnSphere &point_of_intersection =
			 possible_inter_points.second;
			handle_intersection(arcs1, arcs2, iter1, iter2,
			 inter_nodes, point_of_intersection);
		}
		// Else, no intersection.
	}


	/**
	 * Handle the possible intersection or overlap of the arc pointed-at by
	 * @a iter1 (an iterator pointing to a non-"end" location in @a arcs1)
	 * and the arc pointed-at by @a iter2 (an iterator pointing to a
	 * non-"end" location in @a arcs2).
	 *
	 * The arcs are "possibly intersecting or overlapping" because they
	 * have been found to lie sufficiently close that intersection or
	 * overlap is not out of the question.
	 *
	 * This function does not attempt to be strongly exception-safe (since
	 * any parameters it might happen to alter are assumed to be local to
	 * the enclosing function 'partition_intersecting_geometries', and hence
	 * will be destroyed anyway if an exception is thrown), but it *is*
	 * exception-neutral.
	 */
	void
	handle_possible_overlap_or_intersection(
	 ArcList &arcs1,
	 ArcList &arcs2,
	 ArcList::iterator &iter1,
	 ArcList::iterator &iter2,
	 IntersectionNodeList &inter_nodes,
	 ArcListIterList &overlap_arcs_in_1,
	 ArcListIterList &overlap_arcs_in_2) {

		const GreatCircleArc &arc1 = *iter1;
		const GreatCircleArc &arc2 = *iter2;

		/*
		 * We need to distinguish between the case when the arcs lie on
		 * the same great-circle and the case when the arcs lie on
		 * distinct great-circles.  In the former case, the arcs may
		 * have zero intersections, one intersection (ie, endpoint-to-
		 * endpoint touching) or an infinite number of intersections
		 * (ie, overlap); in the latter case, the arcs may have zero or
		 * one intersections, but the intersection test requires that
		 * they lie on distinct great-circles.
		 */
		if (arcs_lie_on_same_great_circle(arc1, arc2)) {

			// There's a chance the arcs might overlap or intersect
			// at a unique point of intersection.
			handle_possible_overlap(arcs1, arcs2, iter1, iter2,
			 inter_nodes, overlap_arcs_in_1, overlap_arcs_in_2);

		} else {

			// There's no chance the arcs could overlap, but
			// there's still a chance the arcs might intersect at a
			// unique point of intersection.
			handle_possible_intersection(arcs1, arcs2, iter1, iter2,
			 inter_nodes);
		}
	}


	/**
	 * Given the sequence of non-intersecting arcs delimited by the
	 * iterators @a begin and @a end, create and return a partitioned polyline1.
	 *
	 * This function does not attempt to be strongly exception-safe (since
	 * any parameters it might happen to alter are assumed to be local to
	 * the enclosing function, and hence will be destroyed anyway if
	 * an exception is thrown), but it *is* exception-neutral.
	 */
	PolylineOnSphere::non_null_ptr_to_const_type
	create_polyline_from_arc_seq(
	 const ArcList::iterator &begin,
	 const ArcList::iterator &end) {

		ArcList::iterator arc_seq_iter = begin;
		ArcList::iterator arc_seq_end = end;

		std::vector< PointOnSphere >::size_type num_arcs =
		 std::distance(arc_seq_iter, arc_seq_end);

		std::vector< PointOnSphere > points;
		points.reserve(num_arcs + 1);

		points.push_back(arc_seq_iter->start_point());
		for ( ; arc_seq_iter != arc_seq_end; ++arc_seq_iter) {

			points.push_back(arc_seq_iter->end_point());
		}

		return PolylineOnSphere::create_on_heap(points);
	}


	void
	add_partitioned_polyline1_to_graph(
		ArcList::iterator arc_seq_begin,
		ArcList::iterator arc_seq_end,
		std::list< ArcList::iterator > &overlap_arcs_in_1,
		Graph::intersection_ptr_seq_type &graph_intersections,
		Graph::partitioned_polyline_ptr_seq_type &graph_partitioned_polylines1)
	{
		// Create a partitioned polyline1.
		PolylineOnSphere::non_null_ptr_to_const_type polyline1 =
				create_polyline_from_arc_seq(arc_seq_begin, arc_seq_end);

		// All arcs between arc_seq_begin and arc_seq_end should be either overlapping
		// or not. So we can choose any arc - choose the first one.
		const bool is_overlapping = list_contains_elem(overlap_arcs_in_1, arc_seq_begin);

		// Add the partitioned polyline1 to the graph.
		Graph::partitioned_polyline_ptr_type partitioned_polyline(
				new Graph::PartitionedPolyline(polyline1, is_overlapping));
		graph_partitioned_polylines1.push_back(partitioned_polyline);

		// If there's a previous intersection then link it and the partitioned polyline.
		if (!graph_intersections.empty()) {
			graph_intersections.back()->next_partitioned_polyline1 = partitioned_polyline.get();
			partitioned_polyline->prev_intersection = graph_intersections.back().get();
		}
	}


	void
	add_intersection1_to_graph(
		Graph::intersection_ptr_seq_type &graph_intersections1,
		Graph::partitioned_polyline_ptr_seq_type &graph_partitioned_polylines1,
		intersection_node_map_type &intersection_node_map,
		IntersectionNodeList::iterator inter_node_iter)
	{
		// Create an intersection.
		Graph::intersection_ptr_type intersection(
				new Graph::Intersection(inter_node_iter->d_point));

		if (!graph_partitioned_polylines1.empty()) {

			// Link the current intersection and the previous partitioned polyline.
			intersection->prev_partitioned_polyline1 = graph_partitioned_polylines1.back().get();
			graph_partitioned_polylines1.back()->next_intersection = intersection.get();
		}
		
		// Keep track of intersections so they can be shared with arc sequence 2 later.
		intersection_node_map[inter_node_iter] = intersection;

		// Add the intersection to the graph.
		graph_intersections1.push_back(intersection);
	}


	void
	add_partitioned_polyline2_to_graph(
		ArcList::iterator arc_seq_begin,
		ArcList::iterator arc_seq_end,
		std::list< ArcList::iterator > &overlap_arcs_in_2,
		Graph::intersection_ptr_seq_type &graph_intersections,
		Graph::partitioned_polyline_ptr_seq_type &graph_partitioned_polylines2)
	{
		// Create a partitioned polyline2.
		PolylineOnSphere::non_null_ptr_to_const_type polyline2 =
				create_polyline_from_arc_seq(arc_seq_begin, arc_seq_end);

		// All arcs between arc_seq_begin and arc_seq_end should be either overlapping
		// or not. So we can choose any arc - choose the first one.
		const bool is_overlapping = list_contains_elem(overlap_arcs_in_2, arc_seq_begin);

		// Add the partitioned polyline2 to the graph.
		Graph::partitioned_polyline_ptr_type partitioned_polyline(
				new Graph::PartitionedPolyline(polyline2, is_overlapping));
		graph_partitioned_polylines2.push_back(partitioned_polyline);

		// If there's a previous intersection then link it and the partitioned polyline.
		if (!graph_intersections.empty()) {
			graph_intersections.back()->next_partitioned_polyline2 = partitioned_polyline.get();
			partitioned_polyline->prev_intersection = graph_intersections.back().get();
		}
	}


	void
	add_intersection2_into_graph(
		Graph::intersection_ptr_seq_type &graph_intersections2,
		Graph::partitioned_polyline_ptr_seq_type &graph_partitioned_polylines2,
		intersection_node_map_type &intersection_node_map,
		IntersectionNodeList::iterator inter_node_iter)
	{
		// Share the intersection created when processing arc sequence 1.
		intersection_node_map_type::const_iterator intersection_node_map_iter =
				intersection_node_map.find(inter_node_iter);
		// The intersection should have already been added while traversing the first polyline.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				intersection_node_map_iter != intersection_node_map.end(),
				GPLATES_ASSERTION_SOURCE);
		Graph::intersection_ptr_type intersection = intersection_node_map_iter->second;

		if (!graph_partitioned_polylines2.empty()) {

			// Link the current intersection and the previous partitioned polyline.
			intersection->prev_partitioned_polyline2 = graph_partitioned_polylines2.back().get();
			graph_partitioned_polylines2.back()->next_intersection = intersection.get();
		}

		// Add the intersection to the graph.
		//
		// This is actually just a temporary list of intersections for arc sequence 2
		// so that we can link the next partitioned polyline2 to this intersection.
		// It doesn't end up getting added to the final graph because intersections
		// are shared and this list ends up being a replica of the list of arc sequence 1.
		graph_intersections2.push_back(intersection);
	}


	/**
	 * Having partitioned the lists of arcs @a arcs1 and @a arcs2,
	 * calculated all the intersection nodes in @a inter_nodes and kept
	 * track of overlap arcs in @a overlap_arcs_in_2, now we finally
	 * construct the partitioned polylines, and populate the output graph
	 * with them and the intersection ponts.
	 *
	 * It is assumed that this function is only invoked when intersections
	 * have been found, which will mean that @a inter_nodes is not empty.
	 *
	 * This function does not attempt to be strongly exception-safe (since
	 * any parameters it might happen to alter are assumed to be local to
	 * the enclosing function 'partition_intersecting_geometries', and hence
	 * will be destroyed anyway if an exception is thrown), but it *is*
	 * exception-neutral.
	 */
	boost::shared_ptr<const Graph>
	generate_partitioned_polyline_graph(
	 ArcList &arcs1,
	 ArcList &arcs2,
	 IntersectionNodeList &inter_nodes,
	 std::list< ArcList::iterator > &overlap_arcs_in_1,
	 std::list< ArcList::iterator > &overlap_arcs_in_2) {

		ArcList::iterator arc_seq_begin;
		ArcList::iterator arc_seq_end;

		IntersectionNodeList::iterator inter_node_iter;

		// We create Graph::Intersection's when processing the first sequence of arcs
		// but since intersections are shared with the second sequence we need to look
		// up the ones created for the first sequence when we process the second sequence.
		intersection_node_map_type intersection_node_map;

		//
		// Process the first great circle arc sequence
		//

		Graph::intersection_ptr_seq_type graph_intersections1;
		Graph::partitioned_polyline_ptr_seq_type graph_partitioned_polylines1;

		// 'arcs1.begin()' is always going to be the beginning of a
		// polyline arc sequence...
		arc_seq_begin = arcs1.begin();

		// See if there's an intersection at the start of the first arc of the first arc sequence.
		inter_node_iter = std::find_if(inter_nodes.begin(), inter_nodes.end(),
				ArcListIterEqualsFirstAfterOnArcs1(arc_seq_begin));
		if (inter_node_iter != inter_nodes.end()) {

			add_intersection1_to_graph(
					graph_intersections1, graph_partitioned_polylines1,
					intersection_node_map, inter_node_iter);
		}

		// Pre-increment 'iter' because 'arcs1.begin()' has already
		// been handled.
		ArcList::iterator
		 iter = ++(arcs1.begin()),
		 end = arcs1.end();
		for ( ; iter != end; ++iter) {

			// See if there's an intersection at the start of the current arc.
			inter_node_iter = std::find_if(inter_nodes.begin(), inter_nodes.end(),
					ArcListIterEqualsFirstAfterOnArcs1(iter));
			if (inter_node_iter != inter_nodes.end()) {

				// The arc pointed-at by 'iter' is the first
				// arc after an intersection.  Hence, it is
				// both the "end" of the previous sequence and
				// the "begin" of the next.
				arc_seq_end = iter;

				add_partitioned_polyline1_to_graph(
					arc_seq_begin, arc_seq_end, overlap_arcs_in_1,
					graph_intersections1, graph_partitioned_polylines1);

				add_intersection1_to_graph(
						graph_intersections1, graph_partitioned_polylines1,
						intersection_node_map, inter_node_iter);

				arc_seq_begin = iter;

			}
			// Else, no intersections here.
		}

		// 'arcs1.end()' is always going to be the end of a polyline
		// arc sequence.
		arc_seq_end = arcs1.end();

		add_partitioned_polyline1_to_graph(
			arc_seq_begin, arc_seq_end, overlap_arcs_in_1,
			graph_intersections1, graph_partitioned_polylines1);

		// See if there's an intersection at the end of the last arc of the first arc sequence.
		inter_node_iter = std::find_if(inter_nodes.begin(), inter_nodes.end(),
				ArcListIterEqualsFirstAfterOnArcs1(arc_seq_end));
		if (inter_node_iter != inter_nodes.end()) {

			add_intersection1_to_graph(
					graph_intersections1, graph_partitioned_polylines1,
					intersection_node_map, inter_node_iter);
		}

		//
		// Now process the second great circle arc sequence
		//

		Graph::intersection_ptr_seq_type graph_intersections2;
		Graph::partitioned_polyline_ptr_seq_type graph_partitioned_polylines2;

		// 'arcs2.begin()' is always going to be the beginning of a
		// polyline arc sequence...
		arc_seq_begin = arcs2.begin();

		// See if there's an intersection at the start of the first arc of the second arc sequence.
		inter_node_iter = std::find_if(inter_nodes.begin(), inter_nodes.end(),
				ArcListIterEqualsFirstAfterOnArcs2(arc_seq_begin));
		if (inter_node_iter != inter_nodes.end()) {

			add_intersection2_into_graph(
					graph_intersections2, graph_partitioned_polylines2,
					intersection_node_map, inter_node_iter);
		}

		// Pre-increment 'iter' because 'arcs2.begin()' has already
		// been handled.
		iter = ++(arcs2.begin());
		end = arcs2.end();
		for ( ; iter != end; ++iter) {

			// See if there's an intersection at the start of the current arc.
			inter_node_iter = std::find_if(inter_nodes.begin(), inter_nodes.end(),
					ArcListIterEqualsFirstAfterOnArcs2(iter));
			if (inter_node_iter != inter_nodes.end()) {

				// The arc pointed-at by 'iter' is the first
				// arc after an intersection.  Hence, it is
				// both the "end" of the previous sequence and
				// the "begin" of the next.
				arc_seq_end = iter;

				add_partitioned_polyline2_to_graph(
					arc_seq_begin, arc_seq_end, overlap_arcs_in_2,
					graph_intersections2, graph_partitioned_polylines2);

				add_intersection2_into_graph(
						graph_intersections2, graph_partitioned_polylines2,
						intersection_node_map, inter_node_iter);

				arc_seq_begin = iter;

			}
			// Else, no intersections here.
		}

		// 'arcs2.end()' is always going to be the end of a polyline
		// arc sequence.
		arc_seq_end = arcs2.end();

		add_partitioned_polyline2_to_graph(
			arc_seq_begin, arc_seq_end, overlap_arcs_in_2,
			graph_intersections2, graph_partitioned_polylines2);

		// See if there's an intersection at the end of the last arc of the second arc sequence.
		inter_node_iter = std::find_if(inter_nodes.begin(), inter_nodes.end(),
				ArcListIterEqualsFirstAfterOnArcs2(arc_seq_end));
		if (inter_node_iter != inter_nodes.end()) {

			add_intersection2_into_graph(
					graph_intersections2, graph_partitioned_polylines2,
					intersection_node_map, inter_node_iter);
		}

		/*
		 * Gather the intersection results and return them in a single Graph object.
		 */

		// NOTE: We ignore 'graph_intersections2' since it is the same as 'graph_intersections1'.
		return boost::shared_ptr<const Graph>(new Graph(
				graph_intersections1,
				graph_partitioned_polylines1,
				graph_partitioned_polylines2));
	}


	/**
	 * Find all points of intersection (with one exception; see
	 * below) of @a polyline1 and @a polyline2, and append them to
	 * 'intersections' in the returned @a Graph object; partition
	 * @a polyline1 and @a polyline2 at these points of intersection,
	 * and store these new, non-intersecting polylines in
	 * 'partitioned_polylines1' and 'partitioned_polylines2' in the
	 * returned @a Graph object; return false if no intersections found.
	 *
	 * The "one exception" mentioned above is this:  If the
	 * polylines touch endpoint-to-endpoint, this will not be
	 * counted as an intersection.
	 */
	template <typename GreatCircleArcForwardIter1, typename GreatCircleArcForwardIter2>
	boost::shared_ptr<const Graph>
	partition_intersecting_geometries(
	 GreatCircleArcForwardIter1 line_geometry1_begin,
	 GreatCircleArcForwardIter1 line_geometry1_end,
	 GreatCircleArcForwardIter2 line_geometry2_begin,
	 GreatCircleArcForwardIter2 line_geometry2_end) {

		std::list< IntersectionNode > inter_nodes;

		std::list< GreatCircleArc >
		 line_geometry1_arcs(line_geometry1_begin, line_geometry1_end),
		 line_geometry2_arcs(line_geometry2_begin, line_geometry2_end);

		/*
		 * Firstly, let's filter out any zero-length arcs in each list, since
		 * these contribute nothing useful, and with them out of the way, we
		 * can assume that all arcs have determinate rotation axes.
		 *
		 * UPDATE: It's still possible that new zero-length arcs get generated during
		 * intersection/overlap testing. Ideally we should never generate zero-length arcs
		 * since they can result in subtle bugs such as a zero length arc1 overlapping the start
		 * of a non-zero-length arc2 where the latter gets split into a zero-length and non-zero
		 * length arc (that matches arc2) which just causes an infinite loop.
		 * We currently get around this skipping zero-length arcs below.
		 */
		line_geometry1_arcs.remove_if(GPlatesMaths::ArcHasIndeterminateRotationAxis());
		line_geometry2_arcs.remove_if(GPlatesMaths::ArcHasIndeterminateRotationAxis());

		/*
		 * An overlap is caused when a non-trivial linear extent of an arc in
		 * the first polyline coincides with a non-trivial linear extent of an
		 * arc in the second polyline.
		 *
		 * Observe also that since the polylines are assumed to be
		 * non-self-intersecting, we can assume that an arc of overlap will
		 * never be re-intersected or re-overlapped at a later time, and thus
		 * that there is no danger of an arc of overlap being subdivided at a
		 * later time.
		 *
		 * (Additionally, since we're using lists for everything (and functions
		 * like 'splice_point_into_arc' are providing guarantees that they
		 * won't invalidate iterators), there's no danger of iterators being
		 * invalidated, so we can store and compare iterators instead of
		 * storing and comparing the arcs directly.)
		 *
		 * However, since it's possible for a polyline to be partitioned by an
		 * overlap in such a way that the arclist-iterator steps back to an arc
		 * before the arc of overlap, it's possible for an arc of overlap to be
		 * re-encountered on a later iteration.  Hence, we'll keep track of the
		 * arcs of overlap for each polyline.
		 */
		std::list< std::list< GreatCircleArc >::iterator > overlap_arcs_in_1;
		std::list< std::list< GreatCircleArc >::iterator > overlap_arcs_in_2;

		std::list< GreatCircleArc >::iterator
		 iter_outer = line_geometry1_arcs.begin(),
		 end_outer = line_geometry1_arcs.end();
		for ( ; iter_outer != end_outer; ++iter_outer) {

			if (list_contains_elem(overlap_arcs_in_1, iter_outer)) {

				/*
				 * 'iter_outer' points to an arc which has already been
				 * determined to be an arc of overlap.
				 *
				 * Since we're assuming the polylines are
				 * non-self-intersecting, we can assume that an arc of
				 * overlap will never be re-intersected or
				 * re-overlapped.
				 *
				 * Hence, there is no reason to continue with this arc.
				 */
				continue;
			}

			std::list< GreatCircleArc >::iterator
			 iter_inner = line_geometry2_arcs.begin(),
			 end_inner = line_geometry2_arcs.end();
			for ( ; iter_inner != end_inner; ++iter_inner) {

				if (list_contains_elem(overlap_arcs_in_2, iter_inner)) {

					/*
					 * 'iter_inner' points to an arc which has
					 * already been determined to be an arc of
					 * overlap.
					 *
					 * Since we're assuming the polylines are
					 * non-self-intersecting, we can assume that an
					 * arc of overlap will never be re-intersected
					 * or re-overlapped.
					 *
					 * Hence, there is no reason to continue with
					 * this arc.
					 */
					continue;
				}

				const GreatCircleArc &arc1 = *iter_outer;
				const GreatCircleArc &arc2 = *iter_inner;

				// If either arc is zero length then return early.
				// If the arcs do touch then we'll ignore it because the previous or next non-zero length
				// arc (relative to the zero-length arc) will also touch and that will take care of the intersection.
				// We generally want to ignore zero length arcs because they can cause subtle bugs and are
				// essentially just duplicate, adjacent polyline points that can be ignored.
				if (arc1.is_zero_length() ||
					arc2.is_zero_length())
				{
					continue;
				}

				// Inexpensively eliminate the no-hopers.
				if ( ! arcs_are_near_each_other(arc1, arc2)) {

					// There's no chance the arcs could touch.
					continue;
				}

				// Else, there's a chance the arcs might overlap or
				// intersect.
				handle_possible_overlap_or_intersection(line_geometry1_arcs,
				 line_geometry2_arcs, iter_outer, iter_inner, inter_nodes,
				 overlap_arcs_in_1, overlap_arcs_in_2);
			}
		}

		std::list< PointOnSphere >::size_type num_intersections_found = inter_nodes.size();
		if (num_intersections_found == 0) {

			// Return false since no intersections found.
			return boost::shared_ptr<const Graph>();
		}

		/*
		 * Build an intersection graph to return to the caller.
		 */
		return generate_partitioned_polyline_graph(
			line_geometry1_arcs, line_geometry2_arcs,
			inter_nodes,
			overlap_arcs_in_1, overlap_arcs_in_2);
	}

}


boost::shared_ptr<const GPlatesMaths::PolylineIntersections::Graph>
GPlatesMaths::PolylineIntersections::partition_intersecting_geometries(
 const PolylineOnSphere &polyline1,
 const PolylineOnSphere &polyline2)
{
	return ::partition_intersecting_geometries(
			polyline1.begin(), polyline1.end(),
			polyline2.begin(), polyline2.end());
}


boost::shared_ptr<const GPlatesMaths::PolylineIntersections::Graph>
GPlatesMaths::PolylineIntersections::partition_intersecting_geometries(
 const PolygonOnSphere &polygon1,
 const PolygonOnSphere &polygon2)
{
	return ::partition_intersecting_geometries(
			polygon1.begin(), polygon1.end(),
			polygon2.begin(), polygon2.end());
}


boost::shared_ptr<const GPlatesMaths::PolylineIntersections::Graph>
GPlatesMaths::PolylineIntersections::partition_intersecting_geometries(
 const PolylineOnSphere &polyline,
 const PolygonOnSphere &polygon)
{
	return ::partition_intersecting_geometries(
			polyline.begin(), polyline.end(),
			polygon.begin(), polygon.end());
}


boost::shared_ptr<const GPlatesMaths::PolylineIntersections::Graph>
GPlatesMaths::PolylineIntersections::partition_intersecting_geometries(
 const PolygonOnSphere &polygon,
 const PolylineOnSphere &polyline)
{
	return ::partition_intersecting_geometries(
			polygon.begin(), polygon.end(),
			polyline.begin(), polyline.end());
}



bool
GPlatesMaths::PolylineIntersections::polyline_set_is_self_intersecting(
 const std::list< PolylineOnSphere::non_null_ptr_to_const_type > &polyline_set,
 std::list< PointOnSphere > &intersection_points,
 std::list< GreatCircleArc > &overlap_segments) {

	GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);

	// FIXME:  This is a dummy implementation.
	return false;
}


GPlatesMaths::PolylineIntersections::Graph::PartitionedPolyline::PartitionedPolyline(
		PolylineOnSphere::non_null_ptr_to_const_type polyline_,
		bool is_overlapping_) :
	polyline(polyline_),
	is_overlapping(is_overlapping_),
	prev_intersection(NULL),
	next_intersection(NULL)
{
}


GPlatesMaths::PolylineIntersections::Graph::Intersection::Intersection(
		const PointOnSphere &intersection_point_) :
	intersection_point(intersection_point_),
	prev_partitioned_polyline1(NULL),
	next_partitioned_polyline1(NULL),
	prev_partitioned_polyline2(NULL),
	next_partitioned_polyline2(NULL)
{
}


GPlatesMaths::PolylineIntersections::Graph::Graph(
		const intersection_ptr_seq_type &intersections_,
		const partitioned_polyline_ptr_seq_type &partitioned_polylines1_,
		const partitioned_polyline_ptr_seq_type &partitioned_polylines2_)
{
	// Need to do this to convert elements from
	// boost::shared_ptr<ElementType> to boost::shared_ptr<const ElementType>.
	intersections.assign(intersections_.begin(), intersections_.end());
 	partitioned_polylines1.assign(partitioned_polylines1_.begin(), partitioned_polylines1_.end());
 	partitioned_polylines2.assign(partitioned_polylines2_.begin(), partitioned_polylines2_.end());
}
