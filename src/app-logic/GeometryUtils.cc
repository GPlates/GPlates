/* $Id$ */
 
/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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

#include <algorithm>
#include <iostream>
#include <cmath>
#include <vector>
#include <boost/utility/in_place_factory.hpp>
#include <QDebug>

#include "GeometryUtils.h"
#include "ReconstructedFeatureGeometryFinder.h"
#include "ReconstructionTree.h"

#include "feature-visitors/GeometryFinder.h"
#include "feature-visitors/GeometryTypeFinder.h"
#include "feature-visitors/PropertyValueFinder.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "maths/ConstGeometryOnSphereVisitor.h"
#include "maths/FiniteRotation.h"
#include "maths/PointOnSphere.h"
#include "maths/PolylineIntersections.h"

#include "model/FeatureHandleWeakRefBackInserter.h"
#include "model/FeatureVisitor.h"
#include "model/PropertyName.h"
#include "model/ModelUtils.h"
#include "model/NotificationGuard.h"

#include "property-values/GeoTimeInstant.h"
#include "property-values/GmlLineString.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlPolygon.h"
#include "property-values/GmlPoint.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlPropertyDelegate.h"
#include "property-values/GpmlTopologicalLineSection.h"
#include "property-values/GpmlTopologicalPoint.h"

#include "utils/UnicodeStringUtils.h"


namespace
{
	/**
	 * Determines the geometry type of a derived @a GeometryOnSphere.
	 */
	class GetGeometryOnSphereType :
			public GPlatesMaths::ConstGeometryOnSphereVisitor
	{
	public:
		GetGeometryOnSphereType() :
			d_geometry_on_sphere_type(GPlatesMaths::GeometryType::NONE)
		{  }


		GPlatesMaths::GeometryType::Value
		get_geometry_on_sphere_type() const
		{
			return d_geometry_on_sphere_type;
		}


		virtual
		void
		visit_point_on_sphere(
				GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type /*point_on_sphere*/)
		{
			d_geometry_on_sphere_type = GPlatesMaths::GeometryType::POINT;
		}


		virtual
		void
		visit_multi_point_on_sphere(
				GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type /*multi_point_on_sphere*/)
		{
			d_geometry_on_sphere_type = GPlatesMaths::GeometryType::MULTIPOINT;
		}


		virtual
		void
		visit_polygon_on_sphere(
				GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type /*polygon_on_sphere*/)
		{
			d_geometry_on_sphere_type = GPlatesMaths::GeometryType::POLYGON;
		}


		virtual
		void
		visit_polyline_on_sphere(
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type /*polyline_on_sphere*/)
		{
			d_geometry_on_sphere_type = GPlatesMaths::GeometryType::POLYLINE;
		}

	private:

		GPlatesMaths::GeometryType::Value d_geometry_on_sphere_type;
	};


	/**
	 * Retrieves points in a derived @a GeometryOnSphere.
	 *
	 * When a @a GeometryOnSphere is visited its points are appended to the
	 * sequence of points passed into the constructor.
	 */
	class GetGeometryOnSpherePoints :
			public GPlatesMaths::ConstGeometryOnSphereVisitor
	{
	public:
		GetGeometryOnSpherePoints(
				std::vector<GPlatesMaths::PointOnSphere> &points,
				bool reverse_points) :
			d_point_seq(points),
			d_reverse_points(reverse_points),
			d_geometry_type(GPlatesMaths::GeometryType::NONE)
		{  }


		GPlatesMaths::GeometryType::Value
		get_geometry_type() const
		{
			return d_geometry_type;
		}


		virtual
		void
		visit_point_on_sphere(
				GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_on_sphere)
		{
			d_geometry_type = GPlatesMaths::GeometryType::POINT;

			d_point_seq.push_back(*point_on_sphere);
		}


		virtual
		void
		visit_multi_point_on_sphere(
				GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere)
		{
			d_geometry_type = GPlatesMaths::GeometryType::MULTIPOINT;

			// Avoid excessive re-allocations when the number of points is large.
			d_point_seq.reserve(multi_point_on_sphere->number_of_points());

			if (d_reverse_points)
			{
				std::reverse_copy(
						multi_point_on_sphere->begin(),
						multi_point_on_sphere->end(),
						std::back_inserter(d_point_seq));
			}
			else
			{
				std::copy(
						multi_point_on_sphere->begin(),
						multi_point_on_sphere->end(),
						std::back_inserter(d_point_seq));
			}
		}


		virtual
		void
		visit_polygon_on_sphere(
				GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
		{
			d_geometry_type = GPlatesMaths::GeometryType::POLYGON;

			// Avoid excessive re-allocations when the number of points is large.
			d_point_seq.reserve(polygon_on_sphere->number_of_vertices());

			if (d_reverse_points)
			{
				std::reverse_copy(
						polygon_on_sphere->vertex_begin(),
						polygon_on_sphere->vertex_end(),
						std::back_inserter(d_point_seq));
			}
			else
			{
				std::copy(
						polygon_on_sphere->vertex_begin(),
						polygon_on_sphere->vertex_end(),
						std::back_inserter(d_point_seq));
			}

		}


		virtual
		void
		visit_polyline_on_sphere(
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
		{
			d_geometry_type = GPlatesMaths::GeometryType::POLYLINE;

			// Avoid excessive re-allocations when the number of points is large.
			d_point_seq.reserve(polyline_on_sphere->number_of_vertices());

			if (d_reverse_points)
			{
				std::reverse_copy(
						polyline_on_sphere->vertex_begin(),
						polyline_on_sphere->vertex_end(),
						std::back_inserter(d_point_seq));
			}
			else
			{
				std::copy(
						polyline_on_sphere->vertex_begin(),
						polyline_on_sphere->vertex_end(),
						std::back_inserter(d_point_seq));
			}
		}

	private:
		//! Sequence of points to append to when visiting geometry on spheres.
		std::vector<GPlatesMaths::PointOnSphere> &d_point_seq;

		//! Whether to reverse the visiting geometry points before appending.
		bool d_reverse_points;

		GPlatesMaths::GeometryType::Value d_geometry_type;
	};


	/**
	 * Retrieves the end points in a derived @a GeometryOnSphere.
	 */
	class GetGeometryOnSphereEndPoints :
			private GPlatesMaths::ConstGeometryOnSphereVisitor
	{
	public:
		/**
		 * Visits @a geometry_on_sphere and returns its start and end points.
		 */
		std::pair<
				GPlatesMaths::PointOnSphere/*start point*/,
				GPlatesMaths::PointOnSphere/*end point*/>
		get_geometry_end_points(
				const GPlatesMaths::GeometryOnSphere &geometry_on_sphere,
				bool reverse_points)
		{
			d_reverse_points = reverse_points;
			d_start_point = boost::none;
			d_end_point = boost::none;

			geometry_on_sphere.accept_visitor(*this);

			// All geometry types should have a start and end point.
			// If not then it means a new GeometryOnSphere derived was implemented
			// and this class needs to visit it.
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					d_start_point && d_end_point, GPLATES_ASSERTION_SOURCE);

			return std::make_pair(*d_start_point, *d_end_point);
		}

	private:
		//! Start point of visited geometry on sphere.
		boost::optional<GPlatesMaths::PointOnSphere> d_start_point;
		//! End point of visited geometry on sphere.
		boost::optional<GPlatesMaths::PointOnSphere> d_end_point;

		//! Whether to reverse the visiting geometry end points before returning them.
		bool d_reverse_points;


		virtual
		void
		visit_point_on_sphere(
				GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_on_sphere)
		{
			d_start_point = *point_on_sphere;
			d_end_point = *point_on_sphere;
		}


		virtual
		void
		visit_multi_point_on_sphere(
				GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere)
		{
			if (d_reverse_points)
			{
				d_start_point = *--multi_point_on_sphere->end();
				d_end_point = *multi_point_on_sphere->begin();
			}
			else
			{
				d_start_point = *multi_point_on_sphere->begin();
				d_end_point = *--multi_point_on_sphere->end();
			}
		}

		virtual
		void
		visit_polygon_on_sphere(
				GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
		{
			if (d_reverse_points)
			{
				d_start_point = *--polygon_on_sphere->vertex_end();
				d_end_point = *polygon_on_sphere->vertex_begin();
			}
			else
			{
				d_start_point = *polygon_on_sphere->vertex_begin();
				d_end_point = *--polygon_on_sphere->vertex_end();
			}
		}


		virtual
		void
		visit_polyline_on_sphere(
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
		{
			if (d_reverse_points)
			{
				d_start_point = *--polyline_on_sphere->vertex_end();
				d_end_point = *polyline_on_sphere->vertex_begin();
			}
			else
			{
				d_start_point = *polyline_on_sphere->vertex_begin();
				d_end_point = *--polyline_on_sphere->vertex_end();
			}
		}
	};


	/**
	 * Retrieves the bounding small circle of a derived @a GeometryOnSphere if appropriate for the type.
	 */
	class GetBoundingSmallCircle :
			public GPlatesMaths::ConstGeometryOnSphereVisitor
	{
	public:
		const boost::optional<const GPlatesMaths::BoundingSmallCircle &> &
		get_bounding_small_circle() const
		{
			return d_bounding_small_circle;
		}


		virtual
		void
		visit_point_on_sphere(
				GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_on_sphere)
		{
			// There is no bounding small circle for a point.
		}

		virtual
		void
		visit_multi_point_on_sphere(
				GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere)
		{
			d_bounding_small_circle = multi_point_on_sphere->get_bounding_small_circle();
		}

		virtual
		void
		visit_polygon_on_sphere(
				GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
		{
			d_bounding_small_circle = polygon_on_sphere->get_bounding_small_circle();
		}

		virtual
		void
		visit_polyline_on_sphere(
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
		{
			d_bounding_small_circle = polyline_on_sphere->get_bounding_small_circle();
		}

	private:
		boost::optional<const GPlatesMaths::BoundingSmallCircle &> d_bounding_small_circle;
	};


	/**
	 * Uses the points in a derived @a GeometryOnSphere object to create a multi-point.
	 */
	class ConvertGeometryToMultiPoint :
			public GPlatesMaths::ConstGeometryOnSphereVisitor
	{
	public:

		GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type
		get_multi_point() const
		{
			// Assert if we failed to visit and convert all geometry types.
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					d_multi_point,
					GPLATES_ASSERTION_SOURCE);

			return d_multi_point.get();
		}


		virtual
		void
		visit_point_on_sphere(
				GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_on_sphere)
		{
			const GPlatesMaths::PointOnSphere point_on_sphere_array[1] =
			{
				GPlatesMaths::PointOnSphere(*point_on_sphere)
			};

			d_multi_point = GPlatesMaths::MultiPointOnSphere::create_on_heap(
					point_on_sphere_array,
					point_on_sphere_array + 1);
		}

		virtual
		void
		visit_multi_point_on_sphere(
				GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere)
		{
			d_multi_point = multi_point_on_sphere;
		}

		virtual
		void
		visit_polygon_on_sphere(
				GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
		{
			d_multi_point = GPlatesMaths::MultiPointOnSphere::create_on_heap(
					polygon_on_sphere->vertex_begin(),
					polygon_on_sphere->vertex_end());
		}

		virtual
		void
		visit_polyline_on_sphere(
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
		{
			d_multi_point = GPlatesMaths::MultiPointOnSphere::create_on_heap(
					polyline_on_sphere->vertex_begin(),
					polyline_on_sphere->vertex_end());
		}

	private:
		boost::optional<GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type> d_multi_point;
	};


	/**
	 * Uses the points in a derived @a GeometryOnSphere object to create a polygon.
	 */
	class ConvertGeometryToPolygon :
			public GPlatesMaths::ConstGeometryOnSphereVisitor
	{
	public:
		const boost::optional<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> &
		get_polygon() const
		{
			return d_polygon;
		}


		virtual
		void
		visit_point_on_sphere(
				GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_on_sphere)
		{
			// Cannot form a polygon from a point.
		}

		virtual
		void
		visit_multi_point_on_sphere(
				GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere)
		{
			if (multi_point_on_sphere->number_of_points() >= 3)
			{
				d_polygon = GPlatesMaths::PolygonOnSphere::create_on_heap(
						multi_point_on_sphere->begin(),
						multi_point_on_sphere->end());
			}
		}

		virtual
		void
		visit_polygon_on_sphere(
				GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
		{
			d_polygon = polygon_on_sphere;
		}

		virtual
		void
		visit_polyline_on_sphere(
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
		{
			if (polyline_on_sphere->number_of_vertices() >= 3)
			{
				d_polygon = GPlatesMaths::PolygonOnSphere::create_on_heap(
						polyline_on_sphere->vertex_begin(),
						polyline_on_sphere->vertex_end());
			}
		}

	private:
		boost::optional<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> d_polygon;
	};


	/**
	 * Visits a property value to retrieve the geometry contained inside it.
	 */
	class GetGeometryFromPropertyVisitor : 
			public GPlatesModel::ConstFeatureVisitor
	{
	public:

		boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>
		get_geometry_from_property(
				const GPlatesModel::FeatureHandle::iterator &property,
				const double &reconstruction_time)
		{
			d_reconstruction_time = GPlatesPropertyValues::GeoTimeInstant(reconstruction_time);
			d_geometry = boost::none;

			(*property)->accept_visitor(*this);

			return d_geometry;
		}

		boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>
		get_geometry_from_property_value(
				const GPlatesModel::PropertyValue &property_value,
				const double &reconstruction_time)
		{
			d_reconstruction_time = GPlatesPropertyValues::GeoTimeInstant(reconstruction_time);
			d_geometry = boost::none;

			property_value.accept_visitor(*this);

			return d_geometry;
		}

	private:

		virtual
		void
		visit_gpml_constant_value(
				const gpml_constant_value_type &gpml_constant_value)
		{
			gpml_constant_value.get_value()->accept_visitor(*this);
		}

		virtual
		void
		visit_gpml_piecewise_aggregation(
				const gpml_piecewise_aggregation_type &gpml_piecewise_aggregation)
		{
			std::vector<GPlatesPropertyValues::GpmlTimeWindow>::const_iterator iter =
				gpml_piecewise_aggregation.get_time_windows().begin();
			std::vector<GPlatesPropertyValues::GpmlTimeWindow>::const_iterator end =
				gpml_piecewise_aggregation.get_time_windows().end();
			for ( ; iter != end; ++iter)
			{
				// If the time window covers our reconstruction time then visit.
				if (iter->get_valid_time()->contains(d_reconstruction_time.get()))
				{
					iter->get_time_dependent_value()->accept_visitor(*this);

					// Break out of loop since time windows should be non-overlapping.
					return;
				}
			}
		}

		virtual
		void
		visit_gml_line_string(
				const gml_line_string_type &gml_line_string)
		{
			d_geometry = gml_line_string.get_polyline();
		}

		virtual
		void
		visit_gml_multi_point(
				const gml_multi_point_type &gml_multi_point)
		{
			d_geometry = gml_multi_point.get_multipoint();
		}

		virtual
		void
		visit_gml_orientable_curve(
				const gml_orientable_curve_type &gml_orientable_curve)
		{
			gml_orientable_curve.get_base_curve()->accept_visitor(*this);
		}

		virtual
		void
		visit_gml_point(
				const gml_point_type &gml_point)
		{
			d_geometry = gml_point.get_point();
		}

		virtual
		void
		visit_gml_polygon(
				const gml_polygon_type &gml_polygon)
		{
			d_geometry = gml_polygon.get_exterior();
		}


		boost::optional<GPlatesPropertyValues::GeoTimeInstant> d_reconstruction_time;
		boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> d_geometry;
	};


	/**
	 * Visits a @a GeometryOnSphere and creates a suitable property value for it.
	 */
	class CreateGeometryProperty :
			public GPlatesMaths::ConstGeometryOnSphereVisitor
	{
	public:
		boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type>
		create_geometry_property(
				const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry)
		{
			d_geometry_property = boost::none;

			geometry->accept_visitor(*this);

			return d_geometry_property;
		}

	protected:
		virtual
		void
		visit_multi_point_on_sphere(
				GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere)
		{
			d_geometry_property =
					GPlatesAppLogic::GeometryUtils::create_multipoint_geometry_property_value(
							multi_point_on_sphere);
		}

		virtual
		void
		visit_point_on_sphere(
				GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_on_sphere)
		{
			d_geometry_property =
					GPlatesAppLogic::GeometryUtils::create_point_geometry_property_value(
							*point_on_sphere);
		}

		virtual
		void
		visit_polygon_on_sphere(
				GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
		{
			d_geometry_property =
					GPlatesAppLogic::GeometryUtils::create_polygon_geometry_property_value(
							polygon_on_sphere);
		}

		virtual
		void
		visit_polyline_on_sphere(
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
		{
			d_geometry_property =
					GPlatesAppLogic::GeometryUtils::create_polyline_geometry_property_value(
							polyline_on_sphere);
		}

	private:
		boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type> d_geometry_property;
	};
}


GPlatesMaths::GeometryType::Value
GPlatesAppLogic::GeometryUtils::get_geometry_type(
		const GPlatesMaths::GeometryOnSphere &geometry_on_sphere)
{
	GetGeometryOnSphereType visitor;
	geometry_on_sphere.accept_visitor(visitor);

	return visitor.get_geometry_on_sphere_type();
}


boost::optional<const GPlatesMaths::PointOnSphere &>
GPlatesAppLogic::GeometryUtils::get_point_on_sphere(
		const GPlatesMaths::GeometryOnSphere &geometry_on_sphere)
{
	boost::optional<const GPlatesMaths::PointOnSphere &> point_on_sphere;

	const GPlatesMaths::PointOnSphere *point_on_sphere_ptr =
			dynamic_cast<const GPlatesMaths::PointOnSphere *>(&geometry_on_sphere);
	if (point_on_sphere_ptr)
	{
		point_on_sphere = *point_on_sphere_ptr;
	}

	return point_on_sphere;
}

boost::optional<GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type>
GPlatesAppLogic::GeometryUtils::get_multi_point_on_sphere(
		const GPlatesMaths::GeometryOnSphere &geometry_on_sphere)
{
	boost::optional<GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type> multi_point_on_sphere;

	const GPlatesMaths::MultiPointOnSphere *multi_point_on_sphere_ptr =
			dynamic_cast<const GPlatesMaths::MultiPointOnSphere *>(&geometry_on_sphere);
	if (multi_point_on_sphere_ptr)
	{
		multi_point_on_sphere = boost::in_place(multi_point_on_sphere_ptr);
	}

	return multi_point_on_sphere;
}

boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>
GPlatesAppLogic::GeometryUtils::get_polyline_on_sphere(
		const GPlatesMaths::GeometryOnSphere &geometry_on_sphere)
{
	boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> polyline_on_sphere;

	const GPlatesMaths::PolylineOnSphere *polyline_on_sphere_ptr =
			dynamic_cast<const GPlatesMaths::PolylineOnSphere *>(&geometry_on_sphere);
	if (polyline_on_sphere_ptr)
	{
		polyline_on_sphere = boost::in_place(polyline_on_sphere_ptr);
	}

	return polyline_on_sphere;
}

boost::optional<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type>
GPlatesAppLogic::GeometryUtils::get_polygon_on_sphere(
		const GPlatesMaths::GeometryOnSphere &geometry_on_sphere)
{
	boost::optional<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> polygon_on_sphere;

	const GPlatesMaths::PolygonOnSphere *polygon_on_sphere_ptr =
			dynamic_cast<const GPlatesMaths::PolygonOnSphere *>(&geometry_on_sphere);
	if (polygon_on_sphere_ptr)
	{
		polygon_on_sphere = boost::in_place(polygon_on_sphere_ptr);
	}

	return polygon_on_sphere;
}


GPlatesMaths::GeometryType::Value
GPlatesAppLogic::GeometryUtils::get_geometry_points(
		const GPlatesMaths::GeometryOnSphere &geometry_on_sphere,
		std::vector<GPlatesMaths::PointOnSphere> &points,
		bool reverse_points)
{
	GetGeometryOnSpherePoints get_geometry_on_sphere_points(points, reverse_points);

	geometry_on_sphere.accept_visitor(get_geometry_on_sphere_points);

	return get_geometry_on_sphere_points.get_geometry_type();
}

std::pair<
GPlatesMaths::PointOnSphere/*start point*/,
GPlatesMaths::PointOnSphere/*end point*/
>
GPlatesAppLogic::GeometryUtils::get_geometry_end_points(
		const GPlatesMaths::GeometryOnSphere &geometry_on_sphere,
		bool reverse_points)
{
	GetGeometryOnSphereEndPoints get_geometry_on_sphere_end_points;

	return get_geometry_on_sphere_end_points.get_geometry_end_points(
		geometry_on_sphere, reverse_points);
}


boost::optional<const GPlatesMaths::BoundingSmallCircle &>
GPlatesAppLogic::GeometryUtils::get_geometry_bounding_small_circle(
		const GPlatesMaths::GeometryOnSphere &geometry_on_sphere)
{
	GetBoundingSmallCircle visitor;

	geometry_on_sphere.accept_visitor(visitor);

	return visitor.get_bounding_small_circle();
}


GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type
GPlatesAppLogic::GeometryUtils::convert_geometry_to_multi_point(
		const GPlatesMaths::GeometryOnSphere &geometry_on_sphere)
{
	ConvertGeometryToMultiPoint visitor;

	geometry_on_sphere.accept_visitor(visitor);

	return visitor.get_multi_point();
}


boost::optional<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type>
GPlatesAppLogic::GeometryUtils::convert_geometry_to_polygon(
		const GPlatesMaths::GeometryOnSphere &geometry_on_sphere)
{
	ConvertGeometryToPolygon visitor;

	geometry_on_sphere.accept_visitor(visitor);

	return visitor.get_polygon();
}


boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>
GPlatesAppLogic::GeometryUtils::get_geometry_from_property(
		const GPlatesModel::FeatureHandle::iterator &property,
		const double &reconstruction_time)
{
	GetGeometryFromPropertyVisitor visitor;
	return visitor.get_geometry_from_property(property, reconstruction_time);
}


boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>
GPlatesAppLogic::GeometryUtils::get_geometry_from_property_value(
		const GPlatesModel::PropertyValue &property_value,
		const double &reconstruction_time)
{
	GetGeometryFromPropertyVisitor visitor;
	return visitor.get_geometry_from_property_value(property_value, reconstruction_time);
}


boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type>
GPlatesAppLogic::GeometryUtils::create_geometry_property_value(
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry)
{
	CreateGeometryProperty create_geometry;
	return create_geometry.create_geometry_property(geometry);
}


GPlatesModel::PropertyValue::non_null_ptr_type
GPlatesAppLogic::GeometryUtils::create_point_geometry_property_value(
		const GPlatesMaths::PointOnSphere &point)
{
	return GPlatesPropertyValues::GmlPoint::create(point);
}


GPlatesModel::PropertyValue::non_null_ptr_type
GPlatesAppLogic::GeometryUtils::create_multipoint_geometry_property_value(
		const GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type &multipoint)
{
	return GPlatesPropertyValues::GmlMultiPoint::create(multipoint);
}


GPlatesModel::PropertyValue::non_null_ptr_type
GPlatesAppLogic::GeometryUtils::create_polyline_geometry_property_value(
		const GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type &polyline)
{
	const GPlatesPropertyValues::GmlLineString::non_null_ptr_type gml_line_string =
			GPlatesPropertyValues::GmlLineString::create(polyline);

	GPlatesModel::PropertyValue::non_null_ptr_type geometry_property =
			GPlatesModel::ModelUtils::create_gml_orientable_curve(gml_line_string);

	return geometry_property;
}


GPlatesModel::PropertyValue::non_null_ptr_type
GPlatesAppLogic::GeometryUtils::create_polygon_geometry_property_value(
		const GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type &polygon)
{
	return GPlatesPropertyValues::GmlPolygon::create(polygon);
}


void
GPlatesAppLogic::GeometryUtils::remove_geometry_properties_from_feature(
		const GPlatesModel::FeatureHandle::weak_ref &feature_ref)
{
	// Merge model events across this scope to avoid excessive number of model callbacks.
	GPlatesModel::NotificationGuard model_notification_guard(*feature_ref->model_ptr());

	// Iterate over the feature properties of the feature.
	GPlatesModel::FeatureHandle::iterator feature_properties_iter =
			feature_ref->begin();
	GPlatesModel::FeatureHandle::iterator feature_properties_end =
			feature_ref->end();
	while (feature_properties_iter != feature_properties_end)
	{
		// Increment iterator before we remove property.
		// I don't think this is currently necessary but it doesn't hurt.
		GPlatesModel::FeatureHandle::iterator current_feature_properties_iter =
				feature_properties_iter;
		++feature_properties_iter;

		if (GPlatesFeatureVisitors::is_geometry_property(
					*current_feature_properties_iter))
		{
			feature_ref->remove(current_feature_properties_iter);
			continue;
		}
	}
}
