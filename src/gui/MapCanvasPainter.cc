/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
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

#include "Colour.h"
#include "ColourScheme.h"
#include "Map.h"
#include "MapCanvasPainter.h"
#include "MapProjection.h"
#include "maths/EllipseGenerator.h"
#include "maths/GreatCircle.h"
#include "maths/LatLonPoint.h"
#include "maths/MathsUtils.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolylineIntersections.h"
#include "maths/PolylineOnSphere.h"
#include "maths/Rotation.h"
#include "view-operations/RenderedDirectionArrow.h"
#include "view-operations/RenderedEllipse.h"
#include "view-operations/RenderedMultiPointOnSphere.h"
#include "view-operations/RenderedPointOnSphere.h"
#include "view-operations/RenderedPolylineOnSphere.h"
#include "view-operations/RenderedPolygonOnSphere.h"
#include "view-operations/RenderedSmallCircle.h"
#include "view-operations/RenderedSmallCircleArc.h"
#include "view-operations/RenderedString.h"

const float GPlatesGui::MapCanvasPainter::POINT_SIZE_ADJUSTMENT = 1.0f;
const float GPlatesGui::MapCanvasPainter::LINE_WIDTH_ADJUSTMENT = 1.0f;

const double TWO_PI = 2. * GPlatesMaths::PI;

// Variables for drawing velocity arrows.
const float GLOBE_TO_MAP_FACTOR = 180.;
const float MAP_VELOCITY_SCALE_FACTOR = 3.0;
const double ARROWHEAD_BASE_HEIGHT_RATIO = 0.5;

// Threshold in degrees for breaking up arcs into smaller arcs.
// const double RADIAN_STEP = GPlatesMaths::PI / 72.0;
// const double DISTANCE_THRESHOLD = std::cos(RADIAN_STEP);

// Tolerance for comparison of dot products. 
const double DOT_PRODUCT_THRESHOLD = 5.4e-7;

// Threshold in scene space for breaking up lines into smaller lines. 
const double SCREEN_THRESHOLD = 5.;

// Number of segments for drawing small-circles, small-circle-arcs, ellipses.
const unsigned int NUM_SEGMENTS = 256; 

// Angular increment for drawing small-circles, small-circle-arcs, ellipses.
const double ANGLE_INCREMENT = 2. * GPlatesMaths::PI / NUM_SEGMENTS;

namespace
{
	using namespace GPlatesMaths;

	struct CoincidentGreatCirclesException{};

	/**
	 * Determines the colour of a RenderedGeometry type using a ColourScheme
	 */
	template <class T>
	inline
	boost::optional<GPlatesGui::Colour>
	get_colour_of_rendered_geometry(
			const T &geom,
			boost::shared_ptr<GPlatesGui::ColourScheme> colour_scheme)
	{
		return geom.get_colour().get_colour(colour_scheme);
	}

	void
	draw_filled_triangle(
		const QPointF &v1,
		const QPointF &v2,
		const QPointF &v3)
	{
		glBegin(GL_TRIANGLES);
		glVertex2d(v1.x(),v1.y());
		glVertex2d(v2.x(),v2.y());
		glVertex2d(v3.x(),v3.y());
		glEnd();
	}

	QPointF
	get_scene_coords_from_llp(
		const LatLonPoint &llp,
		const GPlatesGui::MapProjection &projection)
	{
		double lat = llp.latitude();
		double lon = llp.longitude();
		projection.forward_transform(lon,lat);
		return QPointF(lon,lat);		
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
	
	double
	distance_between_qpointfs(
		const QPointF &p1,
		const QPointF &p2)
	{
		QPointF difference = p1 - p2;

		return sqrt(difference.x()*difference.x() + difference.y()*difference.y());
	}

	void
	draw_arrowhead(
		const QPointF &start_qpoint,
		const QPointF &end_qpoint,
		const double arrowhead_size)
	{
		// A vector in the direction start_point->end_point. This will be the direction of the arrowhead.
		QPointF distance_vector = end_qpoint - start_qpoint;

		// The length of the distance vector
		real_t d = sqrt(distance_vector.x()*distance_vector.x() + distance_vector.y()*distance_vector.y());
		if (d == 0.)
		{ 
			return;
		}

		// Unit vector in the direction of the arrowhead, then scaled up to the arrowhead size.
		distance_vector*= (arrowhead_size/d.dval());

		// A vector perpendicular to the arrow direction, for forming the base of the triangle.
		QPointF perpendicular_vector(-distance_vector.y(),distance_vector.x());

		QPointF base_qpoint = end_qpoint - distance_vector;
		QPointF corner1 = base_qpoint + perpendicular_vector*ARROWHEAD_BASE_HEIGHT_RATIO;
		QPointF corner2 = base_qpoint - perpendicular_vector*ARROWHEAD_BASE_HEIGHT_RATIO;

		draw_filled_triangle(end_qpoint,corner1,corner2);

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

		real_t d1 = dot(point1.position_vector(),great_circle.axis_vector());
		real_t d2 = dot(point2.position_vector(),great_circle.axis_vector());
		bool sign1 = d1 > 0;
		bool sign2 = d2 > 0; 

		//std::cerr << "sign1: " << sign1 << " sign2: " << sign2 << " d1: " << d1 << " d2: " << d2 << std::endl;
#if 0
		if ((sign1 != sign2) || (d1 == 0) || (d2 == 0))
		{
			std::cerr << "Crossing or touching boundary circle" << std::endl;
		}
#endif
		return ((sign1 != sign2) || (abs(d1) < DOT_PRODUCT_THRESHOLD) || (abs(d2) < DOT_PRODUCT_THRESHOLD));
	}

	bool
	segment_crosses_boundary(
		const PointOnSphere &point1,
		const PointOnSphere &point2,
		const GreatCircle &great_circle,
		const PointOnSphere &central_pos)
	{
		real_t x1 = dot(point1.position_vector(),great_circle.axis_vector());
		real_t x2 = dot(point2.position_vector(),great_circle.axis_vector());

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
		return ((abs(dot(point.position_vector(),great_circle.axis_vector())) < DOT_PRODUCT_THRESHOLD)&&
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
		return  (abs(dot(point.position_vector(),great_circle.axis_vector())) <  DOT_PRODUCT_THRESHOLD);
	}
	 
	// This is largely taken from the function in 
	// PolylineIntersection's anonymous namespace. 
	std::pair<PointOnSphere,PointOnSphere>
	calculate_intersections_of_great_circles(
		const GreatCircle &circle1,
		const GreatCircle &circle2)
	{
		if (collinear(circle1.axis_vector(),circle2.axis_vector()))
		{
			throw CoincidentGreatCirclesException();
		}
		Vector3D v = cross(circle1.axis_vector(), circle2.axis_vector());
		UnitVector3D normalised_v = v.get_normalisation();

		PointOnSphere inter_point1(normalised_v);
		PointOnSphere inter_point2( -normalised_v);

		return std::make_pair(inter_point1, inter_point2);		
	}

	/**
	 * Returns the intersection point (point-on-sphere) of the great circle arc between
	 * @a point1 and  @point2 with the great circle @a circle.                                                                     
	 */
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
		const GPlatesGui::TextRenderer &text_renderer,
		boost::shared_ptr<GPlatesGui::ColourScheme> colour_scheme,
		float scale)
	{
		QPointF point = get_scene_coords_from_pos(
				rendered_string.get_point_on_sphere(),
				projection);
		
		// render drop shadow
		boost::optional<GPlatesGui::Colour> shadow_colour = rendered_string.get_shadow_colour().get_colour(
				colour_scheme);
		if (shadow_colour)
		{
			text_renderer.render_text(
					point.x(),
					point.y(),
					0.0,
					rendered_string.get_string(),
					*shadow_colour,
					rendered_string.get_x_offset() + 1, // right 1px
					rendered_string.get_y_offset() - 1, // down 1px
					rendered_string.get_font(),
					scale);
		}

		// render main text
		boost::optional<GPlatesGui::Colour> colour = get_colour_of_rendered_geometry(rendered_string, colour_scheme);
		if (colour)
		{
			text_renderer.render_text(
					point.x(),
					point.y(),
					0.0,
					rendered_string.get_string(),
					*colour,
					rendered_string.get_x_offset(),
					rendered_string.get_y_offset(),
					rendered_string.get_font(),
					scale);
		}
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
	
	/**
	 * Draw the great-circle-arc between @a start_point_on_sphere and @a end_point_on_sphere on the map.
	 *
	 * This will check, and correct for, edge conditions     
	 *
	 * Note that this function should not used for general polyline/polygon drawing, as it would 
	 * project each vertex (except for the first and last vertices) in the polyline/polygon twice.                                                                
	 */
	void
	draw_segment(
		const PointOnSphere &start_point_on_sphere,
		const PointOnSphere &end_point_on_sphere,
		const GPlatesGui::MapProjection &projection,
		const GreatCircle &great_circle,
		const PointOnSphere &central_pos,
		const boost::optional<double> &arrowhead_size = boost::none)
	{

		glBegin(GL_LINES);
			QPointF start_qpoint = get_scene_coords_from_pos(start_point_on_sphere,projection);
			QPointF last_start_qpoint = start_qpoint;
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
						// Set our new start-point in case we need to
						// draw an arrowhead.
						last_start_qpoint.setX(-intersection_qpoint.x());
						last_start_qpoint.setY(intersection_qpoint.y());
					}
				}
			} // if (segment_crosses_great_circle)

			// If the segment didn't cross the boundary, we just draw the line. 
			// If the segment did cross the boundary, we need to finish off the second of the two new segments. 
			// So either way, we need to draw a line to the second point. 
			glVertex2d(end_qpoint.x(),end_qpoint.y());
		glEnd();

		if(arrowhead_size)
		{
			draw_arrowhead(last_start_qpoint,end_qpoint,*arrowhead_size);
		}		
		
	}

	/**
	 * Draw the great-circle-arc between @a start_point_on_sphere and @a end_point_on_sphere on the map.
	 *
	 * This does not check for edge conditions. 
	 */
	void
	draw_segment_without_edge_checking(
		const PointOnSphere &pos1,
		const PointOnSphere &pos2,
		const GPlatesGui::MapProjection &projection,
		const GreatCircle &great_circle,
		const PointOnSphere &central_pos)
	{
		// A quick implementation without any checks for edge-crossing. 
		glBegin(GL_LINES);
			QPointF p1 = get_scene_coords_from_pos(pos1,projection);
			glVertex2d(p1.x(),p1.y());

			QPointF p2 = get_scene_coords_from_pos(pos2,projection);
			glVertex2d(p2.x(),p2.y());
		glEnd();
	}

	/**
	 *  Draw a curved line on the map by splitting the arc into smaller segments, so 
	 *  that each segment is no longer than SCREEN_THRESHOLD screen pixels.                                                                    
	 */
	template< typename GeometryIterator >
	void
	draw_arc(
		GeometryIterator iter,
		const GPlatesGui::MapProjection &projection,
		const GreatCircle &great_circle,
		const PointOnSphere &central_pos)
	{

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
	
	void
	draw_small_circle(
		const GPlatesViewOperations::RenderedSmallCircle &rendered_small_circle,
		const GPlatesGui::MapProjection &projection)
	{
		// FIXME: make this zoom dependent.	

		GreatCircle great_circle = projection.boundary_great_circle();
		LatLonPoint central_llp = projection.central_llp();
		PointOnSphere central_pos = make_point_on_sphere(central_llp);	
		
		PointOnSphere centre = rendered_small_circle.get_centre();		

		UnitVector3D axis = generate_perpendicular(centre.position_vector());
		// Get a point on the small circle by rotating the centre point by the radius angle.
		Rotation rot_from_centre = Rotation::create(axis, rendered_small_circle.get_radius_in_radians());
		 					
		PointOnSphere start_point = rot_from_centre*centre;
			
		Rotation rot = Rotation::create(centre.position_vector(),ANGLE_INCREMENT);
		
		for (unsigned int i = 0; i < NUM_SEGMENTS ; ++i)
		{
			PointOnSphere end_point = rot*start_point;
			draw_segment(start_point,end_point,projection,great_circle,central_pos);
			start_point = end_point;
		}
	}	
	
	void
	draw_small_circle_arc(
			const GPlatesViewOperations::RenderedSmallCircleArc &rendered_small_circle_arc,
			const GPlatesGui::MapProjection &projection)
	{
		GreatCircle great_circle = projection.boundary_great_circle();
		LatLonPoint central_llp = projection.central_llp();
		PointOnSphere central_pos = make_point_on_sphere(central_llp);	
		
		PointOnSphere centre = rendered_small_circle_arc.get_centre();
		PointOnSphere start_point = rendered_small_circle_arc.get_start_point();
		const double arc_length = rendered_small_circle_arc.get_arc_length_in_radians().dval();

		const double delta_angle = arc_length/NUM_SEGMENTS;

		Rotation rot = Rotation::create(centre.position_vector(),delta_angle);

		for (double angle = 0; angle < arc_length ; angle += delta_angle)
		{
			PointOnSphere end_point = rot*start_point;
			draw_segment(start_point,end_point,projection,great_circle,central_pos);
			start_point = end_point;
		}		
		
	}
	
	void
	draw_ellipse(
			const GPlatesViewOperations::RenderedEllipse &rendered_ellipse,
			const GPlatesGui::MapProjection &projection,
			const double &inverse_zoom_factor)
	{
	
		if ((rendered_ellipse.get_semi_major_axis_radians() == 0.)
			|| (rendered_ellipse.get_semi_minor_axis_radians() == 0.))
		{
			return;	
		}		
	
		// See comments in the GlobeRenderedGeometryLayerPainter for possibilities
		// of making the number of steps zoom-dependent.
			
		GreatCircle great_circle = projection.boundary_great_circle();
		LatLonPoint central_llp = projection.central_llp();
		PointOnSphere central_pos = make_point_on_sphere(central_llp);		
	
		GPlatesMaths::EllipseGenerator ellipse_generator(
			rendered_ellipse.get_centre(),
			rendered_ellipse.get_semi_major_axis_radians(),
			rendered_ellipse.get_semi_minor_axis_radians(),
			rendered_ellipse.get_axis());
			
		GPlatesMaths::UnitVector3D uv = ellipse_generator.get_point_on_ellipse(0);
		PointOnSphere previous_pos = PointOnSphere(uv);

		for (double angle = ANGLE_INCREMENT ; angle < TWO_PI ; angle += ANGLE_INCREMENT)
		{
			uv = ellipse_generator.get_point_on_ellipse(angle);
			PointOnSphere pos(uv);
			draw_segment(previous_pos,pos,projection,great_circle,central_pos);
			previous_pos = pos;
		}

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


}


void
GPlatesGui::MapCanvasPainter::visit_rendered_multi_point_on_sphere(
			const GPlatesViewOperations::RenderedMultiPointOnSphere &rendered_multi_point_on_sphere)	
{
	if (!d_render_settings.show_multipoints())
	{
		return;
	}

	boost::optional<Colour> colour = get_colour_of_rendered_geometry(rendered_multi_point_on_sphere, d_colour_scheme);
	if (colour)
	{
		GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere =
			rendered_multi_point_on_sphere.get_multi_point_on_sphere();

		glColor3fv(*colour);
		glPointSize(rendered_multi_point_on_sphere.get_point_size_hint() *
				POINT_SIZE_ADJUSTMENT * d_scale);

		GPlatesMaths::MultiPointOnSphere::const_iterator 
			iter = multi_point_on_sphere->begin(),
			end = multi_point_on_sphere->end();

		glBegin(GL_POINTS);
		for (; iter != end ; ++iter)
		{
			draw_point_on_sphere(*iter,d_map.projection());
		}
		glEnd();
	}
}


void
GPlatesGui::MapCanvasPainter::visit_rendered_point_on_sphere(
			const GPlatesViewOperations::RenderedPointOnSphere &rendered_point_on_sphere)
{
	if (!d_render_settings.show_points())
	{
		return;
	}

	boost::optional<Colour> colour = get_colour_of_rendered_geometry(rendered_point_on_sphere, d_colour_scheme);
	if (colour)
	{
		glColor3fv(*colour);
		glPointSize(rendered_point_on_sphere.get_point_size_hint() *
				POINT_SIZE_ADJUSTMENT * d_scale);

		glBegin(GL_POINTS);
			draw_point_on_sphere(rendered_point_on_sphere.get_point_on_sphere(),d_map.projection());
		glEnd();
	}
}


void
GPlatesGui::MapCanvasPainter::visit_rendered_polygon_on_sphere(
			const GPlatesViewOperations::RenderedPolygonOnSphere &rendered_polygon_on_sphere)
{
	if (!d_render_settings.show_polygons())
	{
		return;
	}
	
	boost::optional<Colour> colour = get_colour_of_rendered_geometry(rendered_polygon_on_sphere, d_colour_scheme);
	if (colour)
	{
		GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere =
				rendered_polygon_on_sphere.get_polygon_on_sphere();

		GPlatesMaths::PolygonOnSphere::const_iterator iter = polygon_on_sphere->begin();
		GPlatesMaths::PolygonOnSphere::const_iterator end = polygon_on_sphere->end();

		glColor3fv(*colour);
		glLineWidth(rendered_polygon_on_sphere.get_line_width_hint() *
				LINE_WIDTH_ADJUSTMENT * d_scale);

		draw_geometry(iter, end, d_map.projection());
	}
}

void
GPlatesGui::MapCanvasPainter::visit_rendered_polyline_on_sphere(
			const GPlatesViewOperations::RenderedPolylineOnSphere &rendered_polyline_on_sphere)
{
	if (!d_render_settings.show_lines())
	{
		return;
	}

	boost::optional<Colour> colour = get_colour_of_rendered_geometry(rendered_polyline_on_sphere, d_colour_scheme);
	if (colour)
	{
		GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere =
				rendered_polyline_on_sphere.get_polyline_on_sphere();

		GPlatesMaths::PolylineOnSphere::const_iterator iter = polyline_on_sphere->begin();
		GPlatesMaths::PolylineOnSphere::const_iterator end = polyline_on_sphere->end();

		glColor3fv(*colour);
		glLineWidth(rendered_polyline_on_sphere.get_line_width_hint() *
				LINE_WIDTH_ADJUSTMENT * d_scale);
		
		draw_geometry(iter, end, d_map.projection());
	}
}

void
GPlatesGui::MapCanvasPainter::visit_rendered_string(
		const GPlatesViewOperations::RenderedString &rendered_string)
{
	if (!d_render_settings.show_strings())
	{
		return;
	}

	if (d_text_renderer_ptr)
	{
		draw_string(
				rendered_string,
				d_map.projection(),
				*d_text_renderer_ptr,
				d_colour_scheme,
				d_scale);
	}
}

void
GPlatesGui::MapCanvasPainter::visit_rendered_small_circle(
		const GPlatesViewOperations::RenderedSmallCircle &rendered_small_circle)
{
	boost::optional<Colour> colour = get_colour_of_rendered_geometry(rendered_small_circle, d_colour_scheme);
	if (colour)
	{
		glColor3fv(*colour);
		glLineWidth(rendered_small_circle.get_line_width_hint() *
				LINE_WIDTH_ADJUSTMENT * d_scale);
		draw_small_circle(rendered_small_circle, d_map.projection());
	}
}

void
GPlatesGui::MapCanvasPainter::visit_rendered_small_circle_arc(
		const GPlatesViewOperations::RenderedSmallCircleArc &rendered_small_circle_arc)
{
	boost::optional<Colour> colour = get_colour_of_rendered_geometry(rendered_small_circle_arc, d_colour_scheme);
	if (colour)
	{
		glColor3fv(*colour);
		glLineWidth(rendered_small_circle_arc.get_line_width_hint() *
				LINE_WIDTH_ADJUSTMENT * d_scale);	
		draw_small_circle_arc(rendered_small_circle_arc, d_map.projection());
	}
}

void
GPlatesGui::MapCanvasPainter::visit_rendered_ellipse(
		const GPlatesViewOperations::RenderedEllipse &rendered_ellipse)
{
	boost::optional<Colour> colour = get_colour_of_rendered_geometry(rendered_ellipse, d_colour_scheme);
	if (colour)
	{
		glColor3fv(*colour);
		glLineWidth(rendered_ellipse.get_line_width_hint() *
				LINE_WIDTH_ADJUSTMENT * d_scale);	
		draw_ellipse(rendered_ellipse,
					d_map.projection(),
					d_inverse_zoom_factor);
	}
}

void
GPlatesGui::MapCanvasPainter::visit_rendered_direction_arrow(
	const GPlatesViewOperations::RenderedDirectionArrow &rendered_direction_arrow)
{
	if (!d_render_settings.show_arrows())
	{
		return;
	}

	boost::optional<Colour> colour = get_colour_of_rendered_geometry(rendered_direction_arrow, d_colour_scheme);
	
	if (colour)
	{
		const GPlatesMaths::PointOnSphere start_point_on_sphere =
			rendered_direction_arrow.get_start_position();

		const GPlatesMaths::Vector3D start_vector(start_point_on_sphere.position_vector());

		// Calculate position from start point along tangent direction to
		// end point off the globe. The length of the arrow in world space
		// is inversely proportional to the zoom or magnification.
		const GPlatesMaths::Vector3D end_vector = GPlatesMaths::Vector3D(start_vector) +
			d_inverse_zoom_factor * rendered_direction_arrow.get_arrow_direction() * MAP_VELOCITY_SCALE_FACTOR;

		const GPlatesMaths::Vector3D arrowline = end_vector -start_vector;
		const double arrowline_length = arrowline.magnitude().dval();

		// This will project the end point on the surface. 
		const GPlatesMaths::UnitVector3D normalised_end = end_vector.get_normalisation();

		const GPlatesMaths::PointOnSphere end_point_on_sphere =
			GPlatesMaths::PointOnSphere(normalised_end);

		glColor3fv(*colour);
		glLineWidth(rendered_direction_arrow.get_arrowline_width_hint() *
				LINE_WIDTH_ADJUSTMENT * d_scale);

		boost::optional<double> arrowhead_size;
		arrowhead_size.reset( 
			d_inverse_zoom_factor*rendered_direction_arrow.get_arrowhead_projected_size()*GLOBE_TO_MAP_FACTOR);

		const float min_ratio_arrowhead_to_arrowline = 
			rendered_direction_arrow.get_min_ratio_arrowhead_to_arrowline()*GLOBE_TO_MAP_FACTOR;

		// We want to keep the projected arrowhead size constant regardless of the
		// the length of the arrowline, except...
		//
		// ...if the ratio of arrowhead size to arrowline length is large enough then
		// we need to start scaling the arrowhead size by the arrowline length so
		// that the arrowhead disappears as the arrowline disappears.
		if (arrowhead_size > min_ratio_arrowhead_to_arrowline * arrowline_length)
		{
			arrowhead_size = min_ratio_arrowhead_to_arrowline * arrowline_length;
		}

		GPlatesMaths::GreatCircle great_circle = d_map.projection().boundary_great_circle();
		GPlatesMaths::LatLonPoint central_llp = d_map.projection().central_llp();
		GPlatesMaths::PointOnSphere central_pos = GPlatesMaths::make_point_on_sphere(central_llp);

		draw_segment(start_point_on_sphere, end_point_on_sphere, d_map.projection(),
				great_circle,central_pos,arrowhead_size);
	}
}

