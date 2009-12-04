/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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
#include <boost/none.hpp>
#include <boost/optional.hpp>
#include <QDebug>

#include "TopologyInternalUtils.h"

#include "ReconstructionGeometryUtils.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "maths/ConstGeometryOnSphereVisitor.h"
#include "maths/FiniteRotation.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolylineOnSphere.h"
#include "maths/PolylineIntersections.h"

#include "model/FeatureHandleWeakRefBackInserter.h"
#include "model/PropertyName.h"
#include "model/ReconstructedFeatureGeometryFinder.h"
#include "model/ReconstructionTree.h"

#include "property-values/GpmlConstantValue.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlPropertyDelegate.h"
#include "property-values/GpmlTopologicalIntersection.h"
#include "property-values/GpmlTopologicalLineSection.h"
#include "property-values/GpmlTopologicalPoint.h"

#include "utils/UnicodeStringUtils.h"


namespace
{
	/**
	 * Creates a @a GpmlTopologicalSection
	 */
	class CreateTopologicalSectionPropertyValue :
			public GPlatesModel::ConstFeatureVisitor
	{
	public:
		boost::optional<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type>
		create_gpml_topological_section(
				const GPlatesModel::FeatureHandle::properties_iterator &geometry_property,
				bool reverse_order,
				const boost::optional<GPlatesModel::FeatureHandle::properties_iterator> &start_geometry_property,
				const boost::optional<GPlatesModel::FeatureHandle::properties_iterator> &end_geometry_property,
				const boost::optional<GPlatesMaths::PointOnSphere> &present_day_reference_point)
		{
			if (!geometry_property.is_valid())
			{
				// Return invalid iterator.
				return boost::none;
			}

			// Initialise data members.
			d_geometry_property = geometry_property;
			d_reverse_order = reverse_order;
			d_start_geometry_property = start_geometry_property;
			d_end_geometry_property = end_geometry_property;
			d_present_day_reference_point = present_day_reference_point;

			d_topological_section = boost::none;

			(*geometry_property)->accept_visitor(*this);

			return d_topological_section;
		}

	private:
		GPlatesModel::FeatureHandle::properties_iterator d_geometry_property;
		bool d_reverse_order;
		boost::optional<GPlatesModel::FeatureHandle::properties_iterator> d_start_geometry_property;
		boost::optional<GPlatesModel::FeatureHandle::properties_iterator> d_end_geometry_property;
		boost::optional<GPlatesMaths::PointOnSphere> d_present_day_reference_point;

		boost::optional<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type> d_topological_section;


		virtual
		void
		visit_gml_line_string(
			const GPlatesPropertyValues::GmlLineString &/*gml_line_string*/)
		{
			static const QString property_value_type = "LineString";
			create_topological_line_section(property_value_type);
		}


		virtual
		void
		visit_gml_multi_point(
			const GPlatesPropertyValues::GmlMultiPoint &/*gml_multi_point*/)
		{
			qDebug() << "WARNING: GpmlTopologicalSection not created for gml:MultiPoint.";
			qDebug() << "FeatureId = " << GPlatesUtils::make_qstring_from_icu_string(
					d_geometry_property.collection_handle_ptr()->feature_id().get());
			qDebug() << "PropertyName = " << GPlatesUtils::make_qstring_from_icu_string(
					(*d_geometry_property)->property_name().get_name());
		}


		virtual
		void
		visit_gml_orientable_curve(
			const GPlatesPropertyValues::GmlOrientableCurve &gml_orientable_curve)
		{
			gml_orientable_curve.base_curve()->accept_visitor(*this);
		}


		virtual
		void
		visit_gml_point(
			const GPlatesPropertyValues::GmlPoint &/*gml_point*/)
		{
			static QString property_value_type = "Point";
			create_topological_point(property_value_type);
		}


		virtual
		void
		visit_gml_polygon(
			const GPlatesPropertyValues::GmlPolygon &/*gml_polygon*/)
		{
			static const QString property_value_type = "LinearRing";
			create_topological_line_section(property_value_type);
		}


		virtual
		void
		visit_gpml_constant_value(
			const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
		{
			gpml_constant_value.value()->accept_visitor(*this);
		}


		void
		create_topological_point(
				const QString &property_value_type)
		{
			boost::optional<GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type> geom_delegate =
					GPlatesAppLogic::TopologyInternalUtils::create_geometry_property_delegate(
							d_geometry_property, property_value_type);

			if (!geom_delegate)
			{
				return;
			}
			
			// Create a GpmlTopologicalPoint from the delegate.
			d_topological_section =
					GPlatesPropertyValues::GpmlTopologicalPoint::create(*geom_delegate);
		}


		void
		create_topological_line_section(
				const QString &property_value_type)
		{
			boost::optional<GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type> geom_delegate =
					GPlatesAppLogic::TopologyInternalUtils::create_geometry_property_delegate(
							d_geometry_property, property_value_type);

			if (!geom_delegate)
			{
				return;
			}

			// If there was a start intersection then create a topological intersection for it.
			boost::optional<GPlatesPropertyValues::GpmlTopologicalIntersection> start_intersection;
			if (d_start_geometry_property && d_present_day_reference_point)
			{
				start_intersection = GPlatesAppLogic::TopologyInternalUtils
						::create_gpml_topological_intersection(
								*d_start_geometry_property,
								*d_present_day_reference_point);
			}

			// If there was an end intersection then create a topological intersection for it.
			boost::optional<GPlatesPropertyValues::GpmlTopologicalIntersection> end_intersection;
			if (d_end_geometry_property && d_present_day_reference_point)
			{
				end_intersection = GPlatesAppLogic::TopologyInternalUtils
						::create_gpml_topological_intersection(
								*d_end_geometry_property,
								*d_present_day_reference_point);
			}

			// Create a GpmlTopologicalLineSection from the delegate.
			d_topological_section = GPlatesPropertyValues::GpmlTopologicalLineSection::create(
					*geom_delegate,
					start_intersection,
					end_intersection,
					d_reverse_order);
		}
	};


	/**
	 * Creates a @a GpmlTopologicalIntersection
	 */
	class CreateTopologicalIntersectionPropertyValue :
			public GPlatesModel::ConstFeatureVisitor
	{
	public:
		CreateTopologicalIntersectionPropertyValue(
				const GPlatesMaths::PointOnSphere &reference_point) :
			d_reference_point(reference_point)
		{  }


		boost::optional<GPlatesPropertyValues::GpmlTopologicalIntersection>
		create_gpml_topological_intersection(
				const GPlatesModel::FeatureHandle::properties_iterator &adjacent_geometry_property)
		{
			if (!adjacent_geometry_property.is_valid())
			{
				// Return false.
				return boost::none;
			}

			// Initialise data members.
			d_adjacent_geometry_property = adjacent_geometry_property;

			d_topological_intersection = boost::none;

			(*adjacent_geometry_property)->accept_visitor(*this);

			return d_topological_intersection;
		}

	private:
		GPlatesModel::FeatureHandle::properties_iterator d_adjacent_geometry_property;
		GPlatesMaths::PointOnSphere d_reference_point;

		boost::optional<GPlatesPropertyValues::GpmlTopologicalIntersection> d_topological_intersection;


		virtual
		void
		visit_gml_line_string(
			const GPlatesPropertyValues::GmlLineString &/*gml_line_string*/)
		{
			static const QString property_value_type = "LineString";
			create_intersection(property_value_type);
		}


		virtual
		void
		visit_gml_multi_point(
			const GPlatesPropertyValues::GmlMultiPoint &/*gml_multi_point*/)
		{
			qDebug() << "WARNING: GpmlTopologicalIntersection not created for gml:Point.";
			qDebug() << "FeatureId = " << GPlatesUtils::make_qstring_from_icu_string(
					d_adjacent_geometry_property.collection_handle_ptr()->feature_id().get());
			qDebug() << "PropertyName = " << GPlatesUtils::make_qstring_from_icu_string(
					(*d_adjacent_geometry_property)->property_name().get_name());
		}


		virtual
		void
		visit_gml_orientable_curve(
			const GPlatesPropertyValues::GmlOrientableCurve &gml_orientable_curve)
		{
			gml_orientable_curve.base_curve()->accept_visitor(*this);
		}


		virtual
		void
		visit_gml_point(
			const GPlatesPropertyValues::GmlPoint &/*gml_point*/)
		{
			qDebug() << "WARNING: GpmlTopologicalIntersection not created for gml:MultiPoint.";
			qDebug() << "FeatureId = " << GPlatesUtils::make_qstring_from_icu_string(
					d_adjacent_geometry_property.collection_handle_ptr()->feature_id().get());
			qDebug() << "PropertyName = " << GPlatesUtils::make_qstring_from_icu_string(
					(*d_adjacent_geometry_property)->property_name().get_name());
		}


		virtual
		void
		visit_gml_polygon(
			const GPlatesPropertyValues::GmlPolygon &/*gml_polygon*/)
		{
			static const QString property_value_type = "LinearRing";
			create_intersection(property_value_type);
		}


		virtual
		void
		visit_gpml_constant_value(
			const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
		{
			gpml_constant_value.value()->accept_visitor(*this);
		}


		void
		create_intersection(
				const QString &property_value_type)
		{
			//
			// Create geometry delegate.
			//

			boost::optional<GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type> geom_delegate =
					GPlatesAppLogic::TopologyInternalUtils::create_geometry_property_delegate(
							d_adjacent_geometry_property, property_value_type);

			// This only happens if 'd_adjacent_geometry_property' is invalid
			// but we've already checked for that - so this shouldn't happen.
			if (!geom_delegate)
			{
				return;
			}

			//
			// Create the reference point and plate id used to reconstruct the reference point.
			//

			// The reference gml point.
			GPlatesPropertyValues::GmlPoint::non_null_ptr_type reference_gml_point =
					GPlatesPropertyValues::GmlPoint::create(d_reference_point);

			static const GPlatesModel::PropertyName reference_point_plate_id_property_name =
					GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");

			static const GPlatesPropertyValues::TemplateTypeParameterType reference_point_value_type =
					GPlatesPropertyValues::TemplateTypeParameterType::create_gpml("PlateId" );

			// Feature id of feature used to lookup plate id for reconstructing reference point.
			// This is the feature that contains the adjacent geometry properties iterator.
			const GPlatesModel::FeatureId &reference_point_feature_id =
					d_adjacent_geometry_property.collection_handle_ptr()->feature_id();

			GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type plate_id_delegate = 
					GPlatesPropertyValues::GpmlPropertyDelegate::create( 
							reference_point_feature_id,
							reference_point_plate_id_property_name,
							reference_point_value_type);

			//
			// Create the GpmlTopologicalIntersection
			//

			d_topological_intersection = GPlatesPropertyValues::GpmlTopologicalIntersection(
					*geom_delegate,
					reference_gml_point,
					plate_id_delegate);
		}
	};


	/**
	 * Visits two @a GeometryOnSphere objects to see if they can be intersected and if so
	 * returns them as @a PolylineOnSphere objects for use in the @a PolylineIntersections code.
	 */
	class GetPolylineOnSpheresForIntersections :
			public GPlatesMaths::ConstGeometryOnSphereVisitor
	{
	public:
		GetPolylineOnSpheresForIntersections(
				GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type first_geometry,
				GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type second_geometry) :
			d_first_geometry(first_geometry),
			d_second_geometry(second_geometry),
			d_visit_phase(VISITING_FIRST_GEOM_TO_SEE_IF_INTERSECTABLE)
		{  }


		/**
		 * Returns geometries @a first_geometry and @a second_geometry as @a PolylineOnSphere
		 * geometries if they are intersectable, otherwise returns false.
		 *
		 * If either geometries is a point or multi-point then the geometries are
		 * not intersectable and false is returned.
		 */
		boost::optional<
			std::pair<
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type,
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> >
		get_polylines_for_intersection()
		{
			d_is_first_geom_intersectable = false;
			d_is_second_geom_intersectable = false;

			d_visit_phase = VISITING_FIRST_GEOM_TO_SEE_IF_INTERSECTABLE;
			d_first_geometry->accept_visitor(*this);

			d_visit_phase = VISITING_SECOND_GEOM_TO_SEE_IF_INTERSECTABLE;
			d_second_geometry->accept_visitor(*this);

			// The geometries can only be intersected if they are both intersectable
			// (such as a polyline or polygon).
			if (!(d_is_first_geom_intersectable && d_is_second_geom_intersectable))
			{
				return boost::none;
			}

			d_visit_phase = VISITING_FIRST_GEOM_TO_GET_POLYLINE;
			d_first_geometry->accept_visitor(*this);

			d_visit_phase = VISITING_SECOND_GEOM_TO_GET_POLYLINE;
			d_second_geometry->accept_visitor(*this);

			// Returned polylines to caller.
			return std::make_pair(*d_first_geom_as_polyline, *d_second_geom_as_polyline);
		}

	private:
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type d_first_geometry;
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type d_second_geometry;

		enum VisitPhase
		{
			VISITING_FIRST_GEOM_TO_SEE_IF_INTERSECTABLE,
			VISITING_SECOND_GEOM_TO_SEE_IF_INTERSECTABLE,
			VISITING_FIRST_GEOM_TO_GET_POLYLINE,
			VISITING_SECOND_GEOM_TO_GET_POLYLINE
		};

		VisitPhase d_visit_phase;
		bool d_is_first_geom_intersectable;
		bool d_is_second_geom_intersectable;

		boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> d_first_geom_as_polyline;
		boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> d_second_geom_as_polyline;


		virtual
		void
		visit_polygon_on_sphere(
				GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
		{
			switch (d_visit_phase)
			{
			case VISITING_FIRST_GEOM_TO_SEE_IF_INTERSECTABLE:
				d_is_first_geom_intersectable = true;
				break;
			case VISITING_SECOND_GEOM_TO_SEE_IF_INTERSECTABLE:
				d_is_second_geom_intersectable = true;
				break;
			case VISITING_FIRST_GEOM_TO_GET_POLYLINE:
				// This should happen rarely since most topological sections
				// will be polylines and not polygons - so the expense of conversion
				// should happen rarely if at all.
				d_first_geom_as_polyline = GPlatesMaths::PolylineOnSphere::create_on_heap(
						polygon_on_sphere->vertex_begin(),
						polygon_on_sphere->vertex_end());
				break;
			case VISITING_SECOND_GEOM_TO_GET_POLYLINE:
				// This should happen rarely since most topological sections
				// will be polylines and not polygons - so the expense of conversion
				// should happen rarely if at all.
				d_second_geom_as_polyline = GPlatesMaths::PolylineOnSphere::create_on_heap(
						polygon_on_sphere->vertex_begin(),
						polygon_on_sphere->vertex_end());
				break;
			}
		}

		virtual
		void
		visit_polyline_on_sphere(
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
		{
			switch (d_visit_phase)
			{
			case VISITING_FIRST_GEOM_TO_SEE_IF_INTERSECTABLE:
				d_is_first_geom_intersectable = true;
				break;
			case VISITING_SECOND_GEOM_TO_SEE_IF_INTERSECTABLE:
				d_is_second_geom_intersectable = true;
				break;
			case VISITING_FIRST_GEOM_TO_GET_POLYLINE:
				// Return reference to existing polyline - no conversion needed.
				d_first_geom_as_polyline = polyline_on_sphere;
				break;
			case VISITING_SECOND_GEOM_TO_GET_POLYLINE:
				// Return reference to existing polyline - no conversion needed.
				d_second_geom_as_polyline = polyline_on_sphere;
				break;
			}
		}
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
			d_reverse_points(reverse_points)
		{  }


		virtual
		void
		visit_point_on_sphere(
				GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_on_sphere)
		{
			d_point_seq.push_back(*point_on_sphere);
		}


		virtual
		void
		visit_multi_point_on_sphere(
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


		virtual
		void
		visit_polygon_on_sphere(
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


		virtual
		void
		visit_polyline_on_sphere(
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
}


boost::optional<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type>
GPlatesAppLogic::TopologyInternalUtils::create_gpml_topological_section(
		const GPlatesModel::FeatureHandle::properties_iterator &geometry_property,
		bool reverse_order,
		const boost::optional<GPlatesModel::FeatureHandle::properties_iterator> &start_geometry_property,
		const boost::optional<GPlatesModel::FeatureHandle::properties_iterator> &end_geometry_property,
		const boost::optional<GPlatesMaths::PointOnSphere> &present_day_reference_point)

{
	CreateTopologicalSectionPropertyValue create_topological_section_property_value;

	return create_topological_section_property_value.create_gpml_topological_section(
			geometry_property,
			reverse_order,
			start_geometry_property,
			end_geometry_property,
			present_day_reference_point);
}


boost::optional<GPlatesPropertyValues::GpmlTopologicalIntersection>
GPlatesAppLogic::TopologyInternalUtils::create_gpml_topological_intersection(
		const GPlatesModel::FeatureHandle::properties_iterator &adjacent_geometry_property,
		const GPlatesMaths::PointOnSphere &present_day_reference_point)
{
	CreateTopologicalIntersectionPropertyValue create_topological_intersection_property_value(
			present_day_reference_point);

	return create_topological_intersection_property_value.create_gpml_topological_intersection(
			adjacent_geometry_property);
}


boost::optional<GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type>
GPlatesAppLogic::TopologyInternalUtils::create_geometry_property_delegate(
		const GPlatesModel::FeatureHandle::properties_iterator &geometry_property,
		const QString &property_value_type)
{
	if (!geometry_property.is_valid())
	{
		// Return invalid iterator.
		return boost::none;
	}

	// Feature id obtained from geometry property iterator.
	const GPlatesModel::FeatureId &feature_id =
			geometry_property.collection_handle_ptr()->feature_id();

	// Property name obtained from geometry property iterator.
	const QString property_name = GPlatesUtils::make_qstring_from_icu_string(
			(*geometry_property)->property_name().get_name());

	const GPlatesModel::PropertyName prop_name =
			GPlatesModel::PropertyName::create_gpml(property_name);

	const GPlatesPropertyValues::TemplateTypeParameterType value_type =
			GPlatesPropertyValues::TemplateTypeParameterType::create_gml(
					property_value_type);

	return GPlatesPropertyValues::GpmlPropertyDelegate::create( 
			feature_id,
			prop_name,
			value_type);
}


GPlatesModel::FeatureHandle::weak_ref
GPlatesAppLogic::TopologyInternalUtils::resolve_feature_id(
		const GPlatesModel::FeatureId &feature_id)
{
	std::vector<GPlatesModel::FeatureHandle::weak_ref> back_ref_targets;
	feature_id.find_back_ref_targets(
			GPlatesModel::append_as_weak_refs(back_ref_targets));
	
	if (back_ref_targets.size() != 1)
	{
		// We didn't get exactly one feature with the feature id so something is
		// not right (user loaded same file twice or didn't load at all)
		// so print debug message and return null feature reference.
		if ( back_ref_targets.empty() )
		{
			qDebug() << "ERROR: No feature found for feature_id =";
			qDebug() <<
				GPlatesUtils::make_qstring_from_icu_string( feature_id.get() );
		}
		else
		{
			qDebug() << "ERROR: More than one feature found for feature_id =";
			qDebug() <<
				GPlatesUtils::make_qstring_from_icu_string( feature_id.get() );
			qDebug() << "ERROR: Unable to determine feature";
		}

		// Return null feature reference.
		return GPlatesModel::FeatureHandle::weak_ref();
	}

	return back_ref_targets.front();
}


boost::optional<GPlatesModel::ReconstructedFeatureGeometry::non_null_ptr_type>
GPlatesAppLogic::TopologyInternalUtils::find_reconstructed_feature_geometry(
		const GPlatesPropertyValues::GpmlPropertyDelegate &geometry_delegate,
		GPlatesModel::Reconstruction *reconstruction)
{
	const GPlatesModel::FeatureHandle::weak_ref feature_ref = resolve_feature_id(
			geometry_delegate.feature_id());

	if (!feature_ref.is_valid())
	{
		return boost::none;
	}

	// Create a property name from the target_propery.
	const QString property_name_qstring = GPlatesUtils::make_qstring_from_icu_string(
			geometry_delegate.target_property().get_name());
	const GPlatesModel::PropertyName property_name = GPlatesModel::PropertyName::create_gpml(
			property_name_qstring);

	// Find the RFGs, in the reconstruction, for the feature ref and target property.
	GPlatesModel::ReconstructedFeatureGeometryFinder rfg_finder(property_name, reconstruction); 
	rfg_finder.find_rfgs_of_feature(feature_ref);

	// If we found zero RFGs or more than one RFG then something is wrong so return false.
	if (rfg_finder.num_rfgs_found() != 1)
	{
		if (rfg_finder.num_rfgs_found() == 0)
		{
			qDebug() << "ERROR: No RFG found using property name for feature_id =";
			qDebug() <<
				GPlatesUtils::make_qstring_from_icu_string( geometry_delegate.feature_id().get() );
			qDebug() << "and property name =" << property_name_qstring;
		}
		else
		{
			qDebug() << "ERROR: More than one RFG found using property name for feature_id =";
			qDebug() <<
				GPlatesUtils::make_qstring_from_icu_string( geometry_delegate.feature_id().get() );
			qDebug() << "and property name =" << property_name_qstring;
			qDebug() << "ERROR: Unable to determine RFG";
		}

		return boost::none;
	}

	// Get the only RFG found.
	const GPlatesModel::ReconstructedFeatureGeometry::non_null_ptr_type &rfg =
			*rfg_finder.found_rfgs_begin();

	// Return the RFG.
	return rfg;
}


boost::optional<GPlatesModel::ReconstructedFeatureGeometry::non_null_ptr_type>
GPlatesAppLogic::TopologyInternalUtils::find_reconstructed_feature_geometry(
		const GPlatesModel::FeatureHandle::properties_iterator &geometry_property,
		GPlatesModel::Reconstruction *reconstruction)
{
	if (!geometry_property.is_valid())
	{
		return boost::none;
	}

	// Get a reference to the feature containing the geometry property.
	const GPlatesModel::FeatureHandle::weak_ref &feature_ref =
			geometry_property.collection_handle_ptr()->reference();

	// Find the RFGs, in the reconstruction, for the feature ref and geometry property.
	GPlatesModel::ReconstructedFeatureGeometryFinder rfg_finder(geometry_property, reconstruction); 
	rfg_finder.find_rfgs_of_feature(feature_ref);

	// If we found no RFG then something is wrong so return false.
	if (rfg_finder.num_rfgs_found() != 1)
	{
		// Since we are searching with a feature properties iterator we can find at most
		// one RFG and since we didn't find one when must have found zero.
		qDebug() << "ERROR: No RFG found using feature properties iterator for feature_id =";
		qDebug() <<
			GPlatesUtils::make_qstring_from_icu_string( feature_ref->feature_id().get() );
		qDebug() << "and property name ="
				<< GPlatesUtils::make_qstring_from_icu_string(
						(*geometry_property)->property_name().get_name());

		return boost::none;
	}

	// Get the only RFG found.
	const GPlatesModel::ReconstructedFeatureGeometry::non_null_ptr_type &rfg =
			*rfg_finder.found_rfgs_begin();

	// Return the RFG.
	return rfg;
}


boost::optional<GPlatesMaths::FiniteRotation>
GPlatesAppLogic::TopologyInternalUtils::get_finite_rotation(
		const GPlatesModel::FeatureHandle::weak_ref &reconstruction_plateid_feature,
		const GPlatesModel::ReconstructionTree &reconstruction_tree)
{
	// Get the plate id from the reference point feature.
	static const GPlatesModel::PropertyName plate_id_property_name =
			GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");

	const GPlatesPropertyValues::GpmlPlateId *recon_plate_id = NULL;
	if ( !GPlatesFeatureVisitors::get_property_value(
		reconstruction_plateid_feature, plate_id_property_name, recon_plate_id ) )
	{
		// There's no reconstruction plate id so return the unrotated point.
		return boost::none;
	}

	// The feature has a reconstruction plate ID.
	// Return the rotation.
	return reconstruction_tree.get_composed_absolute_rotation(recon_plate_id->value()).first;
}


boost::tuple<
		boost::optional<GPlatesMaths::PointOnSphere>/*intersection point*/, 
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type/*first_section_closest_segment*/,
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type/*second_section_closest_segment*/>
GPlatesAppLogic::TopologyInternalUtils::intersect_topological_sections(
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type first_section_geometry,
		const GPlatesMaths::PointOnSphere &first_section_reference_point,
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type second_section_geometry,
		const GPlatesMaths::PointOnSphere &second_section_reference_point)
{
	typedef boost::optional<GPlatesMaths::PointOnSphere> optional_intersection_point_type;

	// Get the two geometries as polylines if they are intersectable - that is if neither
	// geometry is a point or a multi-point.
	boost::optional<
		std::pair<
			GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type,
			GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> > polylines_for_intersection =
				get_polylines_for_intersection(first_section_geometry, second_section_geometry);

	// If the two geometries are not intersectable then just return the original geometries.
	if (!polylines_for_intersection)
	{
		// Return original geometries.
		return boost::make_tuple<optional_intersection_point_type>(
				boost::none, first_section_geometry, second_section_geometry);
	}

	const GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type first_section_polyline =
			polylines_for_intersection->first;
	const GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type second_section_polyline =
			polylines_for_intersection->second;

	return intersect_topological_sections(
			first_section_polyline,
			first_section_reference_point,
			second_section_polyline,
			second_section_reference_point);
}


boost::tuple<
		boost::optional<GPlatesMaths::PointOnSphere>/*intersection point*/, 
		GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type/*first_section_closest_segment*/,
		GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type/*second_section_closest_segment*/>
GPlatesAppLogic::TopologyInternalUtils::intersect_topological_sections(
		GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type first_section_polyline,
		const GPlatesMaths::PointOnSphere &first_section_reference_point,
		GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type second_section_polyline,
		const GPlatesMaths::PointOnSphere &second_section_reference_point)
{
	typedef boost::optional<GPlatesMaths::PointOnSphere> optional_intersection_point_type;

	// Intersect the two section polylines.
	std::list<GPlatesMaths::PointOnSphere> intersection_points;
	std::list<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> partitioned_lines;
	const int num_intersect = GPlatesMaths::PolylineIntersections::partition_intersecting_polylines(
		*first_section_polyline,
		*second_section_polyline,
		intersection_points,
		partitioned_lines);

	// If there were no intersections then return the original geometries.
	if (num_intersect == 0)
	{
		// Return original geometries.
		return boost::make_tuple<optional_intersection_point_type>(
				boost::none,
				first_section_polyline,
				second_section_polyline);
	}

	// FIXME: We currently don't handle more than one intersection.
	if (num_intersect >= 2)
	{
		std::cerr << "TopologyInternalUtils::intersect_topological_sections: " << std::endl
			<< "WARN: num_intersect=" << num_intersect << std::endl 
			<< "WARN: Unable to set boundary feature intersection relations!" << std::endl
			<< "WARN: Make sure boundary feature's only intersect once." << std::endl
			<< std::endl;
		std::cerr << std::endl;

		// Return original geometries.
		return boost::make_tuple<optional_intersection_point_type>(
				boost::none,
				first_section_polyline,
				second_section_polyline);
	}

	//
	// There was a single intersection.
	//

	// pair of polyline lists from intersection
	std::pair<
		std::list<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>,
		std::list<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>
	> intersected_polyline_parts;

	// unambiguously identify partitioned lines:
	// intersected_polyline_parts.first.front is the head of first_section_polyline
	// intersected_polyline_parts.first.back is the tail of first_section_polyline
	// intersected_polyline_parts.second.front is the head of second_section_polyline
	// intersected_polyline_parts.second.back is the tail of second_section_polyline
	intersected_polyline_parts = GPlatesMaths::PolylineIntersections::identify_partitioned_polylines(
			*first_section_polyline,
			*second_section_polyline,
			intersection_points,
			partitioned_lines);

	const GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type first_section_closest_segment =
			find_closest_intersected_segment_to_reference_point(
					first_section_reference_point,
					intersected_polyline_parts.first.front(),
					intersected_polyline_parts.first.back());

	const GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type second_section_closest_segment =
			find_closest_intersected_segment_to_reference_point(
					second_section_reference_point,
					intersected_polyline_parts.second.front(),
					intersected_polyline_parts.second.back());

	return boost::make_tuple(
			intersection_points.front()/*the single intersection point*/,
			first_section_closest_segment,
			second_section_closest_segment);
}


boost::optional<
	std::pair<
		GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type,
		GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> >
GPlatesAppLogic::TopologyInternalUtils::get_polylines_for_intersection(
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type first_section_geometry,
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type second_section_geometry)
{
	GetPolylineOnSpheresForIntersections get_polylines(
			first_section_geometry, second_section_geometry);

	return get_polylines.get_polylines_for_intersection();
}


GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type
GPlatesAppLogic::TopologyInternalUtils::find_closest_intersected_segment_to_reference_point(
		const GPlatesMaths::PointOnSphere &section_reference_point,
		GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type first_intersected_segment,
		GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type second_intersected_segment)
{
	// Setup up proximity test parameters.
	static GPlatesMaths::real_t closeness_inclusion_threshold = 0.9;
	static const GPlatesMaths::real_t cit_sqrd =
		closeness_inclusion_threshold * closeness_inclusion_threshold;
	static const GPlatesMaths::real_t latitude_exclusion_threshold = sqrt(1.0 - cit_sqrd);

	// Determine closeness to first intersected segment.
	GPlatesMaths::real_t closeness_first_segment = -1/*least close*/;
	const bool is_reference_point_close_to_first_segment = first_intersected_segment->is_close_to(
		section_reference_point,
		closeness_inclusion_threshold,
		latitude_exclusion_threshold,
		closeness_first_segment);

	// Determine closeness to second intersected segment.
	GPlatesMaths::real_t closeness_second_segment = -1/*least close*/;
	const bool is_reference_point_close_to_second_segment = second_intersected_segment->is_close_to(
		section_reference_point,
		closeness_inclusion_threshold,
		latitude_exclusion_threshold,
		closeness_second_segment);

	// Make sure that the reference point is close to one of the segments.
	if ( !is_reference_point_close_to_first_segment &&
		!is_reference_point_close_to_second_segment ) 
	{
		std::cerr << "TopologyInternalUtils::find_closest_intersected_segment_to_reference_point: "
			<< std::endl
			<< "WARN: reference point not close to anything!" << std::endl
			<< "WARN: Arbitrarily selecting one of the intersected segments!" 
			<< std::endl;

		// FIXME: do something better than just arbitrarily select one of the segments as closest.
		return first_intersected_segment;
	}

	// Return the closest segment.
	return (closeness_first_segment > closeness_second_segment)
			? first_intersected_segment
			: second_intersected_segment;
}


void
GPlatesAppLogic::TopologyInternalUtils::get_geometry_points(
		const GPlatesMaths::GeometryOnSphere &geometry_on_sphere,
		std::vector<GPlatesMaths::PointOnSphere> &points,
		bool reverse_points)
{
	GetGeometryOnSpherePoints get_geometry_on_sphere_points(points, reverse_points);

	geometry_on_sphere.accept_visitor(get_geometry_on_sphere_points);
}


std::pair<
		GPlatesMaths::PointOnSphere/*start point*/,
		GPlatesMaths::PointOnSphere/*end point*/>
GPlatesAppLogic::TopologyInternalUtils::get_geometry_end_points(
		const GPlatesMaths::GeometryOnSphere &geometry_on_sphere,
		bool reverse_points)
{
	GetGeometryOnSphereEndPoints get_geometry_on_sphere_end_points;

	return get_geometry_on_sphere_end_points.get_geometry_end_points(
			geometry_on_sphere, reverse_points);
}


bool
GPlatesAppLogic::TopologyInternalUtils::include_only_reconstructed_feature_geometries(
		const GPlatesModel::ReconstructionGeometry::non_null_ptr_type &recon_geom)
{
	// We only return true if the reconstruction geometry is a reconstructed feature geometry.
	GPlatesModel::ReconstructedFeatureGeometry *rfg_ptr = NULL;
	return GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type(
			recon_geom, rfg_ptr);
}
