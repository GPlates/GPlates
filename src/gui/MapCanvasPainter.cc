/* $Id: MapCanvasPainter.cc 4818 2009-02-13 10:05:30Z rwatson $ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date: 2009-02-13 11:05:30 +0100 (fr, 13 feb 2009) $
 * 
 * Copyright (C) 2009 The Geological Survey of Norway
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


#include <limits>
#include <stack>

#include <QDebug>

#include <QPolygonF>

#include "gui/MapCanvasPainter.h"
#include "gui/MapProjection.h"
#include "gui/PlatesColourTable.h"
#include "maths/GreatCircle.h"
#include "maths/LatLonPointConversions.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolylineIntersections.h"
#include "maths/PolylineOnSphere.h"
#include "view-operations/RenderedMultiPointOnSphere.h"
#include "view-operations/RenderedPointOnSphere.h"
#include "view-operations/RenderedPolylineOnSphere.h"
#include "view-operations/RenderedPolygonOnSphere.h"
#include "view-operations/RenderedString.h"

int GPlatesGui::MapCanvasPainter::s_num_vertices_drawn = 0;

const float GPlatesGui::MapCanvasPainter::POINT_SIZE_ADJUSTMENT = 1.5f;
const float GPlatesGui::MapCanvasPainter::LINE_SIZE_ADJUSTMENT = 1.5f;

// Threshold in degrees for breaking up arcs into smaller arcs.
const double RADIAN_STEP = GPlatesMaths::PI/72.0;
// const double DISTANCE_THRESHOLD = std::cos(RADIAN_STEP);

// Tolerance for comparison of dot products. 
const double DOT_PRODUCT_THRESHOLD = 5.4e-7;

// Threshold in scene space for breaking up lines into smaller lines. 
const double SCREEN_THRESHOLD = 5.;

namespace
{
	using namespace GPlatesMaths;

	struct CoincidentGreatCirclesException{};

	double
	distance_between_qpointfs(
		const QPointF &p1,
		const QPointF &p2)
	{
		QPointF difference = p1 - p2;

		return sqrt(difference.x()*difference.x() + difference.y()*difference.y());
	}

	double
	angle_between_vectors(
			const Vector3D &v1,
			const Vector3D &v2)
	{
		// Get the angle from the dot product of v1 and v2.
		GPlatesMaths::real_t dp = GPlatesMaths::dot(v1.get_normalisation(),
				v2.get_normalisation());
		GPlatesMaths::real_t angle = GPlatesMaths::acos(dp);
	
		// Don't care about the direction here.
		return angle.dval();
	}

	/**
	 * Returns true if the polyline segment joining point1 and point2 crosses, or begins and/or
	 * ends on, the great_circle. 
	 */
	bool
	segment_crosses_great_circle(
		const PointOnSphere &point1,
		const PointOnSphere &point2,
		const GreatCircle &great_circle)
	{

		real_t d1 = dot(point1.position_vector(),great_circle.axisvector());
		real_t d2 = dot(point2.position_vector(),great_circle.axisvector());
		bool sign1 = d1 > 0;
		bool sign2 = d2 > 0; 

		//std::cerr << "sign1: " << sign1 << " sign2: " << sign2 << " d1: " << d1 << " d2: " << d2 << std::endl;
#if 0
		if ((sign1 != sign2) || (d1 == 0) || (d2 == 0))
		{
			std::cerr << "Crossing or touching boundary circle" << std::endl;
		}
#endif
		return ((sign1 != sign2) || (d1 < DOT_PRODUCT_THRESHOLD) || (d2 < DOT_PRODUCT_THRESHOLD));
	}

	bool
	segment_crosses_boundary(
		const PointOnSphere &point1,
		const PointOnSphere &point2,
		const GreatCircle &great_circle,
		const PointOnSphere &central_pos)
	{
		real_t x1 = dot(point1.position_vector(),great_circle.axisvector());
		real_t x2 = dot(point2.position_vector(),great_circle.axisvector());

		real_t y1 = dot(point1.position_vector(),central_pos.position_vector());
		real_t y2 = dot(point2.position_vector(),central_pos.position_vector());

		bool sign1 = x1 > 0;
		bool sign2 = x2 > 0; 

		return ( ( (sign1 != sign2) || 
				(x1 == 0) || 
				(x2 == 0) ) && 
				(y2/x2 > (y2-y1)/(x2-x1)) );
	}

	/**
	 * Returns true if @a point lies on the "rear" great circle arc of @a great_circle.
	 * FIXME: what sort of tolerance should I use here?
	 */
	bool
	point_lies_on_boundary(
		const PointOnSphere &point,
		const GreatCircle &great_circle,
		const PointOnSphere &front_point)
	{
		return ((dot(point.position_vector(),great_circle.axisvector()) < DOT_PRODUCT_THRESHOLD)&&
				 (dot(point.position_vector(),front_point.position_vector()) < 0));
	}

	/**
	 * Returns true if @a point lies on @a great_circle.
	 */
	bool
	point_lies_on_circle(
		const PointOnSphere &point,
		const GreatCircle &great_circle)
	{
		return  (dot(point.position_vector(),great_circle.axisvector()) <  DOT_PRODUCT_THRESHOLD);
	}
	 
	// This is largely taken from the function in 
	// PolylineIntersection's anonymous namespace. 
	std::pair<PointOnSphere,PointOnSphere>
	calculate_intersections_of_great_circles(
		const GreatCircle &circle1,
		const GreatCircle &circle2)
	{
		if (collinear(circle1.axisvector(),circle2.axisvector()))
		{
			throw CoincidentGreatCirclesException();
		}
		Vector3D v = cross(circle1.axisvector(), circle2.axisvector());
		UnitVector3D normalised_v = v.get_normalisation();

		PointOnSphere inter_point1(normalised_v);
		PointOnSphere inter_point2( -normalised_v);

		return std::make_pair(inter_point1, inter_point2);		
	}

	PointOnSphere
	calculate_intersection_of_segment_with_great_circle(
		const PointOnSphere &point1,
		const PointOnSphere &point2,
		const GreatCircle &circle)
	{
		if (collinear(point1.position_vector(),point2.position_vector()))
		{
			throw CoincidentGreatCirclesException();
		}
		GreatCircle segment_circle(point1,point2);
		std::pair<PointOnSphere,PointOnSphere> intersections =
			calculate_intersections_of_great_circles(segment_circle,circle);

		// We now have 2 (antipodal) points. 
		// I'm assuming here that exactly one of them lies on the gca between point1 and point2.
		// (If that's not the case, we shouldn't have got here...)
		//
		// I'm going to find which one is our desired point by checking the dot product of 
		// each of the gca-points with one of the intersection points.
		// If both dot-products are positive, then this intersection point lies on the gca.
		// If both dot-products are negative, then this intersection point does not lie on the gca.
		// If one is positive and one is negative, then we can check the sum of the two dot products:
		//		If the sum is positive, the intersection point must lie on the gca. 
		// (It should not be possible for the sum to be zero). 
		real_t sum_of_dot_products = dot(point1.position_vector(),intersections.first.position_vector()) + 
									 dot(point2.position_vector(),intersections.first.position_vector());

		if (sum_of_dot_products > 0)
		{
			return intersections.first;
		}
		else
		{
			return intersections.second;
		}

	}
#if 1
	/**
	 * Returns the map coordinates (in the coordinate frame of the QGraphicsScene MapCanvas) of the 
	 * point-on-sphere @a pos, using the map projection @a projection.
	 */
	QPointF
	get_scene_coords_from_pos(
		const PointOnSphere &pos,
		const GPlatesGui::MapProjection &projection)
	{
		LatLonPoint llp = make_lat_lon_point(pos);
		double lat = llp.latitude();
		double lon = llp.longitude();
		projection.forward_transform(lon,lat);
		return QPointF(lon,lat);
	}
#else
	/**
	* Returns the map coordinates (in the coordinate frame of the QGraphicsScene MapCanvas) of the 
	* point-on-sphere @a pos, using the map projection @a projection.
	*/
	QPointF
		get_scene_coords_from_pos(
		const PointOnSphere &pos,
		const GPlatesGui::MapProjection &projection)
	{
		double lat,lon;
		projection.forward_transform(pos,lon,lat);
		return QPointF(lon,lat);
	}
#endif

	inline
	void
	draw_point_on_sphere(
		const PointOnSphere &point_on_sphere,
		const GPlatesGui::MapProjection &projection)
	{
		QPointF point = get_scene_coords_from_pos(point_on_sphere,projection);

		glVertex2d(point.x(),point.y());
	}

	void
	draw_string(
		const GPlatesViewOperations::RenderedString &rendered_string,
		const GPlatesGui::MapProjection &projection,
		const GPlatesGui::TextRenderer &text_renderer)
	{
		QPointF point = get_scene_coords_from_pos(
				rendered_string.get_point_on_sphere(),
				projection);
		
		// render drop shadow
		if (rendered_string.get_shadow_colour())
		{
			text_renderer.render_text(
					point.x(),
					point.y(),
					0.0,
					rendered_string.get_string(),
					*rendered_string.get_shadow_colour(),
					rendered_string.get_x_offset() + 1, // right 1px
					rendered_string.get_y_offset() - 1, // down 1px
					rendered_string.get_font());
		}

		// render main text
		text_renderer.render_text(
				point.x(),
				point.y(),
				0.0,
				rendered_string.get_string(),
				rendered_string.get_colour(),
				rendered_string.get_x_offset(),
				rendered_string.get_y_offset(),
				rendered_string.get_font());
	}

	void
	change_side_if_necessary(
		const QPointF &previous_point,
		QPointF &point)
	{
		double last_x_value = previous_point.x();
		if (  ((last_x_value < 0) && (point.x() > 0)) ||
			((last_x_value > 0) && (point.x() < 0)) )
		{
			point.setX(-point.x());
		}
	}


	/**
	 * Draw polygon or polyline geometries on the map. 
	 */ 
	template < typename GeometryIterator >
	void
	draw_geometry(
		GeometryIterator &begin,
		GeometryIterator &end,
		const GPlatesGui::MapProjection &projection)
	{
		GeometryIterator iter = begin;

		// Grab some information from the projection, which we'll use later when we test for boundary-crossing.
		GPlatesMaths::GreatCircle great_circle = projection.boundary_great_circle();
		GPlatesMaths::LatLonPoint central_llp = projection.central_llp();
		GPlatesMaths::PointOnSphere central_pos = GPlatesMaths::make_point_on_sphere(central_llp);

		// Transform the first point in the polyline. 
		QPointF first_point = get_scene_coords_from_pos(iter->start_point(),projection);
	
		double prev_x = first_point.x(), prev_y = first_point.y();

		glBegin(GL_LINE_STRIP);
		glVertex2d(first_point.x(),first_point.y());

		for (; iter != end ; ++iter)
		{
			const PointOnSphere &start_point = iter->start_point();
			const PointOnSphere &end_point = iter->end_point();

			// Get, and transform to scene coordinates, the next point on the line. 
			QPointF start_qpoint = get_scene_coords_from_pos(start_point,projection);
			QPointF end_qpoint = get_scene_coords_from_pos(end_point,projection);

			//if (iter->dot_of_endpoints().dval() < DISTANCE_THRESHOLD) {
			if (distance_between_qpointfs(start_qpoint,end_qpoint) > SCREEN_THRESHOLD) {

				glEnd();
				draw_arc(*iter,projection,great_circle,central_pos);
				
				QPointF end_of_arc = get_scene_coords_from_pos(iter->end_point(),projection);
				glBegin(GL_LINE_STRIP);
				glVertex2d(end_of_arc.x(),end_of_arc.y());

				prev_x = end_of_arc.x();
				prev_y = end_of_arc.y();
				continue;
			}

			// Make sure we don't have identical consecutive points.
			if (collinear(start_point.position_vector(),end_point.position_vector()))
			{				
				continue;
			}

			if (segment_crosses_great_circle(start_point,end_point,great_circle))
			{

				// At this stage our segment may cross, or begin or end on, the *front* of the 
				// boundary great circle. 

				// Now we need to check if any one of several unusual cases occur:
				// 1. The first point, but not the second point, of the segment lies on the boundary line;
				// 2. The second point, but not the first point, of the segment lies on the boundary line;
				// 3. Both the first and second points of the segment lie on the boundary line. 
				//
				// Case (1) is treated by checking the side on which the second point lies, and 
				// making sure that the first point is placed at the correct boundary.
				//
				// Case (2) is treated by checking the side on which the first point lies, and
				// making sure that the second point is drawn at the correct boundary. 
				//
				// Case (3) is treated by checking the side on which the first point lies, and
				// making sure that the second point is drawn at the correct boundary.
				//
				// So cases (2) and (3) are treated the same, and can be tested for by a single condition,
				// i.e. that the second point lies on the boundary line. 
				//
				// For all of these cases, we don't need to find an intersection point.

				bool first_lies_on_circle = point_lies_on_circle(start_point,great_circle);
				bool second_lies_on_circle = point_lies_on_circle(end_point,great_circle);

				double previous_lon = prev_x;
				double previous_lat = prev_y;
				QPointF previous_point(prev_x,prev_y);

				if (first_lies_on_circle || second_lies_on_circle)
				{
					bool first_lies_on_boundary = point_lies_on_boundary(start_point,great_circle,central_pos);
					bool second_lies_on_boundary = point_lies_on_boundary(end_point,great_circle,central_pos);

					if (second_lies_on_boundary)
					{
						change_side_if_necessary(previous_point,end_qpoint);

					}
					else if (first_lies_on_boundary)
					{
						// We only get here if the second point *doesn't* lie on the boundary.
						// So we check for the sign of second point, and do a "moveTo" if necessary.

						if (((previous_lon < 0) && (end_qpoint.x() > 0)) ||
							((previous_lon > 0) && (end_qpoint.x() < 0)))
						{	
							glEnd();
							glBegin(GL_LINE_STRIP);
							glVertex2d(-previous_lon,previous_lat);
						}
					}
				}
				else
				{
					// Neither of our 2 points lies on the boundary, so check for the intersection point.
					GPlatesMaths::PointOnSphere intersection = 
						calculate_intersection_of_segment_with_great_circle(start_point,end_point,great_circle);

					// This intersection point may lie on the "front" part of the map, rather than
					// on the boundary great circle arc.
					//
					// If it lies on the edge of the map, then the dot-product of the intersection
					// point with the central llp will be negative. 
					if (GPlatesMaths::dot(central_pos.position_vector(),intersection.position_vector()) < 0)
					{
						// We have an intersection with the edge of the map.
						// We need to split the line up into 2 segments: one which joins
						// the previous point to the edge; and a second which joins the opposite
						// edge to the current point.

						// Transform the intersection point to scene coordinates. 
						QPointF intersection_point = get_scene_coords_from_pos(intersection,projection);

						// The proj4 library may return a scene coordinate at the left-edge, or the right-edge.
						// We need to determine which, so that we can draw the 2 new segments correctly.
						// Our map projection is centred in scene coordinates, so the x-coordinate of the first edge should 
						// be the same sign as previous_lon. If it isn't, we can reverse the sign of it.
						change_side_if_necessary(previous_point,intersection_point);

						// Draw to the edge.
						glVertex2d(intersection_point.x(),intersection_point.y());

						// Move to the opposite edge.
						glEnd();
						glBegin(GL_LINE_STRIP);
						glVertex2d(-intersection_point.x(),intersection_point.y());
					}
				}
			} // if (segment_crosses_great_circle)

			// If the segment didn't cross the boundary, we just draw the line. 
			// If the segment did cross the boundary, we need to finish off the second of the two new segments. 
			// So either way, we need to draw a line to current_lon, current_lat. 
			prev_x = end_qpoint.x();
			prev_y = end_qpoint.y();
			glVertex2d(end_qpoint.x(),end_qpoint.y());

		} // for loop.
		glEnd();
	}
	
	void
	draw_segment(
		const PointOnSphere &start_point_on_sphere,
		const PointOnSphere &end_point_on_sphere,
		const GPlatesGui::MapProjection &projection,
		const GreatCircle &great_circle,
		const PointOnSphere &central_pos)
	{

		glBegin(GL_LINES);
			QPointF start_qpoint = get_scene_coords_from_pos(start_point_on_sphere,projection);
			glVertex2d(start_qpoint.x(),start_qpoint.y());

			QPointF end_qpoint = get_scene_coords_from_pos(end_point_on_sphere,projection);

			if (segment_crosses_great_circle(start_point_on_sphere,end_point_on_sphere,great_circle))
			{

				// The segment may cross, or begin and/or end on, the *front* of the 
				// boundary great circle. 

				// Now we need to check if any one of several unusual cases occur:
				// 1. The first point, but not the second point, of the segment lies on the boundary line;
				// 2. The second point, but not the first point, of the segment lies on the boundary line;
				// 3. Both the first and second points of the segment lie on the boundary line. 
				//
				// Case (1) is treated by checking the side on which the second point lies, and 
				// making sure that the first point is placed at the correct boundary.
				//
				// Case (2) is treated by checking the side on which the first point lies, and
				// making sure that the second point is drawn at the correct boundary. 
				//
				// Case (3) is treated by checking the side on which the first point lies, and
				// making sure that the second point is drawn at the correct boundary.
				//
				// So cases (2) and (3) are treated the same, and can be tested for by a single condition,
				// i.e. that the second point lies on the boundary line. 
				//
				// For all of these cases, we don't need to find an intersection point.

				bool first_lies_on_circle = point_lies_on_circle(start_point_on_sphere,great_circle);
				bool second_lies_on_circle = point_lies_on_circle(end_point_on_sphere,great_circle);

				if (first_lies_on_circle || second_lies_on_circle)
				{
					bool first_lies_on_boundary = 
						point_lies_on_boundary(start_point_on_sphere,great_circle,central_pos);
					bool second_lies_on_boundary = 
						point_lies_on_boundary(end_point_on_sphere,great_circle,central_pos);

					if (second_lies_on_boundary)
					{
						change_side_if_necessary(start_qpoint,end_qpoint);

					}
					else if (first_lies_on_boundary)
					{
						// We only get here if the second point *doesn't* lie on the boundary.
						// So we check for the sign of second point.

						if (((start_qpoint.x() < 0) && (end_qpoint.x() > 0)) ||
							((start_qpoint.x() > 0) && (end_qpoint.x() < 0)))
						{	
							glEnd();
							glBegin(GL_LINES);
							glVertex2d(-start_qpoint.x(),start_qpoint.y());
						}
					}
				}
				else
				{

					// Neither of our 2 points lies on the boundary, so check for the intersection point.
					GPlatesMaths::PointOnSphere intersection = 
						calculate_intersection_of_segment_with_great_circle(
								start_point_on_sphere,
								end_point_on_sphere,
								great_circle);

					// This intersection point may lie on the "front" part of the map, rather than
					// on the boundary great circle arc.
					//
					// If it lies on the edge of the map, then the dot-product of the intersection
					// point with the central llp will be negative. 
					if (GPlatesMaths::dot(central_pos.position_vector(),intersection.position_vector()) < 0)
					{
						// We have an intersection with the edge of the map. We'll split the line up 
						// into 2 segments: one which joins the previous point to the edge; and a second 
						// which joins the opposite edge to the current point.

						// Transform the intersection point to scene coordinates. 
						QPointF intersection_qpoint = get_scene_coords_from_pos(intersection,projection);

						// The proj4 library may return a scene coordinate at the left-edge, or the right-edge.
						// We need to determine which, so that we can draw the 2 new segments correctly.
						// Our map projection is centred in scene coordinates, so the x-coordinate of the first edge should 
						// be the same sign as previous_lon. If it isn't, we can reverse the sign of it.
						change_side_if_necessary(start_qpoint,intersection_qpoint);

						// Draw to the edge.
						glVertex2d(intersection_qpoint.x(),intersection_qpoint.y());

						// Move to the opposite edge.
						glEnd();
						glBegin(GL_LINES);
						glVertex2d(-intersection_qpoint.x(),intersection_qpoint.y());
					}
				}
			} // if (segment_crosses_great_circle)

			// If the segment didn't cross the boundary, we just draw the line. 
			// If the segment did cross the boundary, we need to finish off the second of the two new segments. 
			// So either way, we need to draw a line to the second point. 
			glVertex2d(end_qpoint.x(),end_qpoint.y());
		glEnd();
	}

	void
	draw_segment_without_edge_checking(
		const PointOnSphere &pos1,
		const PointOnSphere &pos2,
		const GPlatesGui::MapProjection &projection,
		const GreatCircle &great_circle,
		const PointOnSphere &central_pos)
	{
		// A quick implementation without any checks for edge-crossing yet. 
		glBegin(GL_LINES);
			QPointF p1 = get_scene_coords_from_pos(pos1,projection);
			glVertex2d(p1.x(),p1.y());

			QPointF p2 = get_scene_coords_from_pos(pos2,projection);
			glVertex2d(p2.x(),p2.y());
		glEnd();
	}


	template< typename GeometryIterator >
	void
	draw_arc(
		GeometryIterator iter,
		const GPlatesGui::MapProjection &projection,
		const GreatCircle &great_circle,
		const PointOnSphere &central_pos)
	{
		// Draw a curved line on the map by splitting the arc into smaller segments, so 
		// that each segment is no longer than SCREEN_THRESHOLD screen pixels.
		GPlatesMaths::Vector3D start_pt = GPlatesMaths::Vector3D(iter.start_point().position_vector());
		GPlatesMaths::Vector3D end_pt = GPlatesMaths::Vector3D(iter.end_point().position_vector());


		QPointF start_point = get_scene_coords_from_pos(iter.start_point(),projection);
		QPointF end_point = get_scene_coords_from_pos(iter.end_point(),projection);

		double distance = distance_between_qpointfs(start_point,end_point);

		if (distance < SCREEN_THRESHOLD)
		{ 
			return;
		}

		//std::cerr << "drawing smaller sections in scene space" << std::endl;

		int number_of_segments = static_cast<int>(distance/SCREEN_THRESHOLD + 1);
		double fraction_increment = 1./number_of_segments;

		GPlatesMaths::UnitVector3D segment_start = start_pt.get_normalisation();
		GPlatesMaths::PointOnSphere segment_start_pos =
				GPlatesMaths::PointOnSphere(segment_start);

		double fraction_along_arc = 0.;

		for (int i = 0; i < number_of_segments; ++i)
		{
			GPlatesMaths::UnitVector3D segment_end = 
				GPlatesMaths::Vector3D(start_pt + fraction_along_arc*(end_pt - start_pt)).get_normalisation();
			
			GPlatesMaths::PointOnSphere segment_end_pos =
				GPlatesMaths::PointOnSphere(segment_end);

			draw_segment(segment_start_pos,segment_end_pos,projection,great_circle,central_pos);

			segment_start_pos = segment_end_pos;
			fraction_along_arc += fraction_increment;
		}
		draw_segment(segment_start_pos,iter.end_point(),projection,great_circle,central_pos);

	}
#if 0
	template< typename GeometryIterator >
	void
	draw_arc(
		GeometryIterator iter,
		const GPlatesGui::MapProjection &projection,
		const GreatCircle &great_circle,
		const PointOnSphere &central_pos)
	{
		// Draw a curved line on the map by splitting the arc into smaller segments, so 
		// that each segment is no longer than RADIAN_STEP radians of arc.

		GPlatesMaths::Vector3D start_pt = GPlatesMaths::Vector3D(iter.start_point().position_vector());
		GPlatesMaths::Vector3D end_pt = GPlatesMaths::Vector3D(iter.end_point().position_vector());

		double angle = angle_between_vectors(start_pt,end_pt);

		if (angle < RADIAN_STEP)
		{ 
			return;
		}

		int number_of_segments = static_cast<int>(angle/RADIAN_STEP + 1);
		double fraction_increment = 1./number_of_segments;

		GPlatesMaths::UnitVector3D segment_start = start_pt.get_normalisation();
		GPlatesMaths::PointOnSphere segment_start_pos =
				GPlatesMaths::PointOnSphere(segment_start);

		double fraction_along_arc = 0.;

		for (int i = 0; i < number_of_segments; ++i)
		{
			GPlatesMaths::UnitVector3D segment_end = 
				GPlatesMaths::Vector3D(start_pt + fraction_along_arc*(end_pt - start_pt)).get_normalisation();
			
			GPlatesMaths::PointOnSphere segment_end_pos =
				GPlatesMaths::PointOnSphere(segment_end);

			draw_segment(segment_start_pos,segment_end_pos,projection,great_circle,central_pos);

			segment_start_pos = segment_end_pos;
			fraction_along_arc += fraction_increment;
		}
		draw_segment(segment_start_pos,iter.end_point(),projection,great_circle,central_pos);
	}
#endif

	void
	display_vertex(
		const PointOnSphere &point)
	{
		std::cerr << "Vertex: " << point.position_vector() << std::endl;
	}
	
	void
	display_vertex(
		const PointOnSphere &point,
		const GPlatesGui::MapProjection &projection)
	{
		std::cerr << "Vertex: " << point.position_vector() << std::endl;
		qDebug() << get_scene_coords_from_pos(point,projection);
		qDebug();
	}	

}


void
GPlatesGui::MapCanvasPainter::visit_rendered_multi_point_on_sphere(
			const GPlatesViewOperations::RenderedMultiPointOnSphere &rendered_multi_point_on_sphere)	
{
	if (!d_render_settings.show_multipoints)
	{
		return;
	}
	
	GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere =
		rendered_multi_point_on_sphere.get_multi_point_on_sphere();

	const GPlatesGui::Colour colour = rendered_multi_point_on_sphere.get_colour();

	glColor3fv(colour);
	glPointSize(rendered_multi_point_on_sphere.get_point_size_hint() * POINT_SIZE_ADJUSTMENT);

	GPlatesMaths::MultiPointOnSphere::const_iterator 
		iter = multi_point_on_sphere->begin(),
		end = multi_point_on_sphere->end();

	glBegin(GL_POINTS);
	for (; iter != end ; ++iter)
	{
		draw_point_on_sphere(*iter,d_canvas_ptr->projection());

		s_num_vertices_drawn ++;
	}
	glEnd();
}


void
GPlatesGui::MapCanvasPainter::visit_rendered_point_on_sphere(
			const GPlatesViewOperations::RenderedPointOnSphere &rendered_point_on_sphere)
{
	if (!d_render_settings.show_points)
	{
		return;
	}

	const GPlatesGui::Colour colour = rendered_point_on_sphere.get_colour();

	glColor3fv(colour);
	glPointSize(rendered_point_on_sphere.get_point_size_hint() * POINT_SIZE_ADJUSTMENT);

	glBegin(GL_POINTS);
		draw_point_on_sphere(rendered_point_on_sphere.get_point_on_sphere(),d_canvas_ptr->projection());
	glEnd();

	s_num_vertices_drawn ++;
}


void
GPlatesGui::MapCanvasPainter::visit_rendered_polygon_on_sphere(
			const GPlatesViewOperations::RenderedPolygonOnSphere &rendered_polygon_on_sphere)
{
	if (!d_render_settings.show_polygons)
	{
		return;
	}
	
	GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere =
			rendered_polygon_on_sphere.get_polygon_on_sphere();


	GPlatesMaths::PolygonOnSphere::const_iterator iter = polygon_on_sphere->begin();
	GPlatesMaths::PolygonOnSphere::const_iterator end = polygon_on_sphere->end();	

	const GPlatesGui::Colour colour = rendered_polygon_on_sphere.get_colour();

	glColor3fv(colour);

	draw_geometry(iter,end,d_canvas_ptr->projection());

}

void
GPlatesGui::MapCanvasPainter::visit_rendered_polyline_on_sphere(
			const GPlatesViewOperations::RenderedPolylineOnSphere &rendered_polyline_on_sphere)
{
	if (!d_render_settings.show_lines)
	{
		return;
	}

	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere =
			rendered_polyline_on_sphere.get_polyline_on_sphere();

	GPlatesMaths::PolylineOnSphere::const_iterator iter = polyline_on_sphere->begin();
	GPlatesMaths::PolylineOnSphere::const_iterator end = polyline_on_sphere->end();

	const GPlatesGui::Colour colour = rendered_polyline_on_sphere.get_colour();

	glColor3fv(colour);

	draw_geometry(iter,end,d_canvas_ptr->projection());

}

void
GPlatesGui::MapCanvasPainter::visit_rendered_string(
		const GPlatesViewOperations::RenderedString &rendered_string)
{
	if (!d_render_settings.show_strings)
	{
		return;
	}

	if (d_text_renderer_ptr)
	{
		draw_string(rendered_string, d_canvas_ptr->projection(), *d_text_renderer_ptr);
	}
}

