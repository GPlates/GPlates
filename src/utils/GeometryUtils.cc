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
#include "feature-visitors/GeometryFinder.h"
#include "feature-visitors/GeometryTypeFinder.h"

#include "model/ModelUtils.h"

#include "utils/GeometryUtils.h"

namespace{
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
			d_reverse_points(reverse_points)
		{  }


		virtual
		void
		visit_point_on_sphere(
				GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_on_sphere);


		virtual
		void
		visit_multi_point_on_sphere(
				GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere);


		virtual
		void
		visit_polygon_on_sphere(
				GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere);


		virtual
		void
		visit_polyline_on_sphere(
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere);

	private:
		//! Sequence of points to append to when visiting geometry on spheres.
		std::vector<GPlatesMaths::PointOnSphere> &d_point_seq;

		//! Whether to reverse the visiting geometry points before appending.
		bool d_reverse_points;
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
				bool reverse_points);

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
				GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_on_sphere);


		virtual
		void
		visit_multi_point_on_sphere(
				GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere);

		virtual
		void
		visit_polygon_on_sphere(
				GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere);


		virtual
		void
		visit_polyline_on_sphere(
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere);
	};


	void
	GetGeometryOnSpherePoints::visit_point_on_sphere(
			GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_on_sphere)
	{
		d_point_seq.push_back(*point_on_sphere);
	}

	void
	GetGeometryOnSpherePoints::visit_multi_point_on_sphere(
			GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere)
	{
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

	void
	GetGeometryOnSpherePoints::visit_polygon_on_sphere(
			GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
	{
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

	void
	GetGeometryOnSpherePoints::visit_polyline_on_sphere(
			GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
	{
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

	std::pair<
			GPlatesMaths::PointOnSphere/*start point*/,
			GPlatesMaths::PointOnSphere/*end point*/>
	GetGeometryOnSphereEndPoints::get_geometry_end_points(
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

	void
	GetGeometryOnSphereEndPoints::visit_point_on_sphere(
			GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_on_sphere)
	{
		d_start_point = *point_on_sphere;
		d_end_point = *point_on_sphere;
	}

	void
	GetGeometryOnSphereEndPoints::visit_multi_point_on_sphere(
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

	void
	GetGeometryOnSphereEndPoints::visit_polygon_on_sphere(
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

	void
	GetGeometryOnSphereEndPoints::visit_polyline_on_sphere(
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
}

void
GPlatesUtils::GeometryUtils::get_geometry_points(
		const GPlatesMaths::GeometryOnSphere &geometry_on_sphere,
		std::vector<GPlatesMaths::PointOnSphere> &points,
		bool reverse_points)
{
	GetGeometryOnSpherePoints get_geometry_on_sphere_points(points, reverse_points);

	geometry_on_sphere.accept_visitor(get_geometry_on_sphere_points);
}

std::pair<
GPlatesMaths::PointOnSphere/*start point*/,
GPlatesMaths::PointOnSphere/*end point*/
>
GPlatesUtils::GeometryUtils::get_geometry_end_points(
		const GPlatesMaths::GeometryOnSphere &geometry_on_sphere,
		bool reverse_points)
{
	GetGeometryOnSphereEndPoints get_geometry_on_sphere_end_points;

	return get_geometry_on_sphere_end_points.get_geometry_end_points(
		geometry_on_sphere, reverse_points);
}

boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type>
GPlatesUtils::GeometryUtils::create_geometry_property_value(
		point_seq_type::const_iterator begin, 
		point_seq_type::const_iterator end,
		GPlatesViewOperations::GeometryType::Value type)
{
	switch(type)
	{
		case GPlatesViewOperations::GeometryType::POLYLINE:
			return boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type>(
						GPlatesPropertyValues::GmlLineString::create(	
								GPlatesMaths::PolylineOnSphere::create_on_heap(
										begin, 
										end)));
			break;

		case GPlatesViewOperations::GeometryType::MULTIPOINT:
			return boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type>(
						GPlatesPropertyValues::GmlMultiPoint::create(	
								GPlatesMaths::MultiPointOnSphere::create_on_heap(
										begin, 
										end)));	
			break;

		case GPlatesViewOperations::GeometryType::POLYGON:
			return  boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type>(
						GPlatesPropertyValues::GmlPolygon::create(	
								GPlatesMaths::PolygonOnSphere::create_on_heap(
										begin, 
										end)));	
			break;

		case GPlatesViewOperations::GeometryType::POINT:
			return boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type>(
						GPlatesPropertyValues::GmlPoint::create(*begin));	
			break;

		default:
			return boost::none;
			break;
	}
}


void
GPlatesUtils::GeometryUtils::remove_geometry_properties_from_feature(
		const GPlatesModel::FeatureHandle::weak_ref &feature_ref)
{
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

