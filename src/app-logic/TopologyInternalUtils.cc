/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
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
#include <deque>
#include <vector>
#include <boost/none.hpp>
#include <boost/optional.hpp>
#include <QDebug>

#include "TopologyInternalUtils.h"

#include "ReconstructedFeatureGeometryFinder.h"
#include "ReconstructionGeometryUtils.h"
#include "ReconstructionTree.h"

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

#include "property-values/GpmlConstantValue.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlPropertyDelegate.h"
#include "property-values/GpmlTopologicalIntersection.h"
#include "property-values/GpmlTopologicalLineSection.h"
#include "property-values/GpmlTopologicalPoint.h"
#include "property-values/XsString.h"

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
				const GPlatesModel::FeatureHandle::iterator &geometry_property,
				bool reverse_order,
				const boost::optional<GPlatesModel::FeatureHandle::iterator> &start_geometry_property,
				const boost::optional<GPlatesModel::FeatureHandle::iterator> &end_geometry_property,
				const boost::optional<GPlatesMaths::PointOnSphere> &present_day_reference_point)
		{
			if (!geometry_property.is_still_valid())
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
		GPlatesModel::FeatureHandle::iterator d_geometry_property;
		bool d_reverse_order;
		boost::optional<GPlatesModel::FeatureHandle::iterator> d_start_geometry_property;
		boost::optional<GPlatesModel::FeatureHandle::iterator> d_end_geometry_property;
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
					d_geometry_property.handle_weak_ref()->feature_id().get());
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
				const GPlatesModel::FeatureHandle::iterator &adjacent_geometry_property)
		{
			if (!adjacent_geometry_property.is_still_valid())
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
		GPlatesModel::FeatureHandle::iterator d_adjacent_geometry_property;
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
					d_adjacent_geometry_property.handle_weak_ref()->feature_id().get());
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
					d_adjacent_geometry_property.handle_weak_ref()->feature_id().get());
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
					GPlatesPropertyValues::TemplateTypeParameterType::create_gpml("plateId" );

			// Feature id of feature used to lookup plate id for reconstructing reference point.
			// This is the feature that contains the adjacent geometry properties iterator.
			const GPlatesModel::FeatureId &reference_point_feature_id =
					d_adjacent_geometry_property.handle_weak_ref()->feature_id();

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
	 * Determines which polyline route to follow through the intersection graph.
	 * The route starts at an intersection point and either traverses previous polylines
	 * or next polylines.
	 * And the traversal follows polylines partitioned from either the first or second
	 * section that was intersected.
	 */
	enum IntersectionGraphTraversal
	{
		PREVIOUS_POLYLINE1,
		PREVIOUS_POLYLINE2,
		NEXT_POLYLINE1,
		NEXT_POLYLINE2
	};

	/**
	 * Iterates through the intersection graph starting at @a intersection by
	 * following the next polyline guided by @a intersection_graph_traversal
	 * and concatenates the polyline segments into a single segment.
	 *
	 * Returns false if there are no intersected segments after @a intersection.
	 */
	boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>
	concatenate_partitioned_polylines_starting_at_intersection(
			const GPlatesMaths::PolylineIntersections::Graph::Intersection *intersection,
			const IntersectionGraphTraversal intersection_graph_traversal)
	{
		// Data member pointers to the data members of Graph::Intersection and
		// Graph::PartitionedPolyline that we will follow to traverse the intersection
		// graph as requested by the caller.
		const GPlatesMaths::PolylineIntersections::Graph::PartitionedPolyline *
			GPlatesMaths::PolylineIntersections::Graph::Intersection::*next_partitioned_polyline_ptr = NULL;
		const GPlatesMaths::PolylineIntersections::Graph::Intersection *
			GPlatesMaths::PolylineIntersections::Graph::PartitionedPolyline::*next_intersection_ptr = NULL;

		switch (intersection_graph_traversal)
		{
		case PREVIOUS_POLYLINE1:
			next_partitioned_polyline_ptr =
				&GPlatesMaths::PolylineIntersections::Graph::Intersection::prev_partitioned_polyline1;
			next_intersection_ptr =
				&GPlatesMaths::PolylineIntersections::Graph::PartitionedPolyline::prev_intersection;
			break;
		case PREVIOUS_POLYLINE2:
			next_partitioned_polyline_ptr =
				&GPlatesMaths::PolylineIntersections::Graph::Intersection::prev_partitioned_polyline2,
			next_intersection_ptr =
				&GPlatesMaths::PolylineIntersections::Graph::PartitionedPolyline::prev_intersection;
			break;
		case NEXT_POLYLINE1:
			next_partitioned_polyline_ptr =
				&GPlatesMaths::PolylineIntersections::Graph::Intersection::next_partitioned_polyline1;
			next_intersection_ptr =
				&GPlatesMaths::PolylineIntersections::Graph::PartitionedPolyline::next_intersection;
			break;
		case NEXT_POLYLINE2:
			next_partitioned_polyline_ptr =
				&GPlatesMaths::PolylineIntersections::Graph::Intersection::next_partitioned_polyline2;
			next_intersection_ptr =
				&GPlatesMaths::PolylineIntersections::Graph::PartitionedPolyline::next_intersection;
			break;
		}

		// Gather the polyline segments.
		std::deque<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> polyline_seq;
		do
		{
			// Get the polyline following the current intersection in the graph.
			const GPlatesMaths::PolylineIntersections::Graph::PartitionedPolyline *
					next_partitioned_polyline = (intersection->*next_partitioned_polyline_ptr);

			if (!next_partitioned_polyline)
			{
				break;
			}

			// We need to push the polylines in the correct order so that when they're
			// concatenated they are joined end1->start2->end2->start3 etc.
			if (intersection_graph_traversal == NEXT_POLYLINE1 ||
				intersection_graph_traversal == NEXT_POLYLINE2)
			{
				polyline_seq.push_back(next_partitioned_polyline->polyline);
			}
			else
			{
				polyline_seq.push_front(next_partitioned_polyline->polyline);
			}

			// Move to the following intersection in the graph.
			intersection = (next_partitioned_polyline->*next_intersection_ptr);
		}
		while (intersection);

		if (polyline_seq.empty())
		{
			return boost::none;
		}

		// If there's only one polyline then there's no need to concatenate.
		if (polyline_seq.size() == 1)
		{
			return polyline_seq.front();
		}

		return GPlatesMaths::concatenate_polylines(polyline_seq.begin(), polyline_seq.end());
	}
}


boost::optional<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type>
GPlatesAppLogic::TopologyInternalUtils::create_gpml_topological_section(
		const GPlatesModel::FeatureHandle::iterator &geometry_property,
		bool reverse_order,
		const boost::optional<GPlatesModel::FeatureHandle::iterator> &start_geometry_property,
		const boost::optional<GPlatesModel::FeatureHandle::iterator> &end_geometry_property,
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
		const GPlatesModel::FeatureHandle::iterator &adjacent_geometry_property,
		const GPlatesMaths::PointOnSphere &present_day_reference_point)
{
	CreateTopologicalIntersectionPropertyValue create_topological_intersection_property_value(
			present_day_reference_point);

	return create_topological_intersection_property_value.create_gpml_topological_intersection(
			adjacent_geometry_property);
}


boost::optional<GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type>
GPlatesAppLogic::TopologyInternalUtils::create_geometry_property_delegate(
		const GPlatesModel::FeatureHandle::iterator &geometry_property,
		const QString &property_value_type)
{
	if (!geometry_property.is_still_valid())
	{
		// Return invalid iterator.
		return boost::none;
	}

	// Feature id obtained from geometry property iterator.
	const GPlatesModel::FeatureId &feature_id =
			geometry_property.handle_weak_ref()->feature_id();

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
			qDebug() 
				<< "ERROR: missing feature for id ="
				<< GPlatesUtils::make_qstring_from_icu_string( feature_id.get() );
		}
		else
		{
			qDebug() 
				<< "ERROR: multiple features for id ="
				<< GPlatesUtils::make_qstring_from_icu_string( feature_id.get() );
		}

		// Return null feature reference.
		return GPlatesModel::FeatureHandle::weak_ref();
	}

	return back_ref_targets.front();
}


boost::optional<GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type>
GPlatesAppLogic::TopologyInternalUtils::find_reconstructed_feature_geometry(
		const GPlatesPropertyValues::GpmlPropertyDelegate &geometry_delegate,
		const boost::optional<const ReconstructionTree &> &reconstruction_tree,
		const boost::optional<const std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &> &
				restrict_reconstructed_feature_geometries)
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

	// Find the RFGs, optionally in the reconstruction_tree, for the feature ref and target property.
	ReconstructedFeatureGeometryFinder rfg_finder(
			property_name,
			reconstruction_tree ? &reconstruction_tree.get() : NULL); 
	rfg_finder.find_rfgs_of_feature(feature_ref);

	// Put found RFGs in a vector so two code paths (if/else) can generate results in same way.
	std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> found_rfgs;
	if (restrict_reconstructed_feature_geometries)
	{
		// Search the found RFGs for those contained in 'restrict_reconstructed_feature_geometries'.
		for (ReconstructedFeatureGeometryFinder::const_iterator found_rfg_iter = rfg_finder.found_rfgs_begin();
			found_rfg_iter != rfg_finder.found_rfgs_end();
			++found_rfg_iter)
		{
			const ReconstructedFeatureGeometry::non_null_ptr_type &found_rfg = *found_rfg_iter;

			if (std::find(
					restrict_reconstructed_feature_geometries->begin(),
					restrict_reconstructed_feature_geometries->end(),
					found_rfg) != restrict_reconstructed_feature_geometries->end())
			{
				found_rfgs.push_back(found_rfg);
			}
		}
	}
	else
	{
		found_rfgs.insert(
				found_rfgs.end(),
				rfg_finder.found_rfgs_begin(),
				rfg_finder.found_rfgs_end());
	}

// FIXME: MULTIPLE GEOM

	// If we found no RFG (optionally referencing 'reconstruction_tree') that is reconstructed from
	// 'geometry_property' then it probably means the reconstruction time is
	// outside the age range of the feature containing 'geometry_property'.
	// This is ok - it's not necessarily an error.
	if (found_rfgs.size() == 0)
	{ 
		int num = found_rfgs.size();
		qDebug() << "ERROR: " << num << "Reconstruction Feature Geometries (RFGs) found for:";
		qDebug() << "  feature id =" 
			<< GPlatesUtils::make_qstring_from_icu_string( geometry_delegate.feature_id().get() );
		static const GPlatesModel::PropertyName prop = GPlatesModel::PropertyName::create_gml("name");
		const GPlatesPropertyValues::XsString *name;
        if ( GPlatesFeatureVisitors::get_property_value(feature_ref, prop, name) ) {
            qDebug() << "  feature name =" << GPlatesUtils::make_qstring( name->value() );
        } else {
            qDebug() << "  feature name = UNKNOWN";
        }
        qDebug() << "  property name =" << property_name_qstring;
        qDebug() << "  Unable to use any RFG.";
        return boost::none;
    }
    else if (found_rfgs.size() > 1)
    {
        // We should only return boost::none for the case == 0, as above.
        // For the case >1 we return the rfg_finder.found_rfgs_begin() as normally
        int num = found_rfgs.size();
        qDebug() << "WARNING: " << num << "Reconstruction Feature Geometries (RFGs) found for:";
        qDebug() << "  feature id =" 
			<< GPlatesUtils::make_qstring_from_icu_string( geometry_delegate.feature_id().get() );
        static const GPlatesModel::PropertyName prop = GPlatesModel::PropertyName::create_gml("name");
        const GPlatesPropertyValues::XsString *name;
        if ( GPlatesFeatureVisitors::get_property_value(feature_ref, prop, name) ) {
            qDebug() << "  feature name =" << GPlatesUtils::make_qstring( name->value() );
        } else {
            qDebug() << "  feature name = UNKNOWN";
        }
        qDebug() << "  property name =" << property_name_qstring;
        qDebug() << "  Using the first RFG found.";
    }

	// Return the first RFG found.
	return found_rfgs.front();
}


boost::optional<GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type>
GPlatesAppLogic::TopologyInternalUtils::find_reconstructed_feature_geometry(
		const GPlatesModel::FeatureHandle::iterator &geometry_property,
		const ReconstructionTree &reconstruction_tree)
{
	/*
	if (!geometry_property.is_valid())
	{
		return boost::none;
	}
	*/

	// Get a reference to the feature containing the geometry property.
	const GPlatesModel::FeatureHandle::weak_ref &feature_ref =
			geometry_property.handle_weak_ref();

	// Find the RFGs, referencing 'reconstruction_tree', for the feature ref and geometry property.
	ReconstructedFeatureGeometryFinder rfg_finder(geometry_property, &reconstruction_tree); 
	rfg_finder.find_rfgs_of_feature(feature_ref);

	// It's currently proving too difficult to ensure only one RFG is created per geometry property
	// per reconstruction time.
	// FIXME: An overhaul of the method of finding RFGs from features is required as is
	// default reconstruction tree layers.
#if 0
	// Because we are searching using a geometry properties iterator we can only
	// find at most one RFG (unless the geometry got reconstructed twice and hence added to
	// the Reconstruction twice - in which case this is an programming bug).
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			rfg_finder.num_rfgs_found() <= 1,
			GPLATES_ASSERTION_SOURCE);
#endif

	// If we found no RFG (referencing 'reconstruction_tree') that is reconstructed from
	// 'geometry_property' then it probably means the reconstruction time is
	// outside the age range of the feature containing 'geometry_property'.
	// This is ok - it's not necessarily an error.
	if (rfg_finder.num_rfgs_found() == 0)
	{
		return boost::none;
	}

	// Get the only RFG found.
	const ReconstructedFeatureGeometry::non_null_ptr_type &rfg =
			*rfg_finder.found_rfgs_begin();

	// Return the RFG.
	return rfg;
}


boost::optional<GPlatesMaths::FiniteRotation>
GPlatesAppLogic::TopologyInternalUtils::get_finite_rotation(
		const GPlatesModel::FeatureHandle::weak_ref &reconstruction_plateid_feature,
		const ReconstructionTree &reconstruction_tree)
{
	if (!reconstruction_plateid_feature.is_valid())
	{
		return boost::none;
	}

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

	boost::optional<
		boost::tuple<
			/*intersection point*/
			GPlatesMaths::PointOnSphere,
			/*head_first_section*/
			boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>,
			/*tail_first_section*/
			boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>,
			/*head_second_section*/
			boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>,
			/*tail_second_section*/
			boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> > >
		intersected_segments = intersect_topological_sections(
				first_section_polyline,
				second_section_polyline);

	// If there were no intersections then return the original geometries.
	if (!intersected_segments)
	{
		// Return original geometries.
		return boost::make_tuple<optional_intersection_point_type>(
				boost::none,
				first_section_polyline,
				second_section_polyline);
	}

	// It's possible for the intersection to form a T-junction where
	// one polyline is divided into two (or even a V-junction where
	// both polylines meet at either of their endpoints).
	// If that happens then we don't need to test a polyline that is not divided
	// because it will be the closest segment to the reference point already.

	const GPlatesMaths::PointOnSphere &intersection_point = boost::get<0>(*intersected_segments);

	// We are guaranteed that at least one of head or tail is non-null.
	const boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> &
			head_first_section = boost::get<1>(*intersected_segments);
	const boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> &
			tail_first_section = boost::get<2>(*intersected_segments);

	// We are guaranteed that at least one of head or tail is non-null.
	const boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> &
			head_second_section = boost::get<3>(*intersected_segments);
	const boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> &
			tail_second_section = boost::get<4>(*intersected_segments);

	const GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type first_section_closest_segment =
			head_first_section && tail_first_section
					? find_closest_intersected_segment_to_reference_point(
							first_section_reference_point,
							*head_first_section,
							*tail_first_section)
					: (head_first_section ? *head_first_section : *tail_first_section);

	const GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type second_section_closest_segment =
			head_second_section && tail_second_section
					? find_closest_intersected_segment_to_reference_point(
							second_section_reference_point,
							*head_second_section,
							*tail_second_section)
					: (head_second_section ? *head_second_section : *tail_second_section);

	return boost::make_tuple(
			// The single intersection point...
			intersection_point,
			first_section_closest_segment,
			second_section_closest_segment);
}


boost::optional<
	boost::tuple<
		/*intersection point*/
		GPlatesMaths::PointOnSphere,
		/*head_first_section*/
		boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>,
		/*tail_first_section*/
		boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>,
		/*head_second_section*/
		boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>,
		/*tail_second_section*/
		boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> > >
GPlatesAppLogic::TopologyInternalUtils::intersect_topological_sections(
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type first_section_geometry,
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type second_section_geometry)
{
	// Get the two geometries as polylines if they are intersectable - that is if neither
	// geometry is a point or a multi-point.
	boost::optional<
		std::pair<
			GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type,
			GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> > polylines_for_intersection =
				get_polylines_for_intersection(first_section_geometry, second_section_geometry);

	// If the two geometries are not intersectable then return false.
	if (!polylines_for_intersection)
	{
		return boost::none;
	}

	const GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type first_section_polyline =
			polylines_for_intersection->first;
	const GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type second_section_polyline =
			polylines_for_intersection->second;

	boost::optional<
		boost::tuple<
			/*intersection point*/
			GPlatesMaths::PointOnSphere,
			/*head_first_section*/
			boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>,
			/*tail_first_section*/
			boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>,
			/*head_second_section*/
			boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>,
			/*tail_second_section*/
			boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> > >
		intersected_segments = intersect_topological_sections(
				first_section_polyline,
				second_section_polyline);

	if (!intersected_segments)
	{
		return boost::none;
	}

	// This helps us convert PolylineOnSphere to GeometryOnSphere in the tuple.
	const GPlatesMaths::PointOnSphere &intersection = boost::get<0>(*intersected_segments);
	boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> head_first_section;
	boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> tail_first_section;
	boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> head_second_section;
	boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> tail_second_section;
	boost::tie(
			boost::tuples::ignore,
			head_first_section,
			tail_first_section,
			head_second_section,
			tail_second_section) =
					*intersected_segments;

	return boost::make_tuple(
			intersection,
			head_first_section,
			tail_first_section,
			head_second_section,
			tail_second_section);
}


boost::optional<
	boost::tuple<
		/*intersection point*/
		GPlatesMaths::PointOnSphere,
		/*head_first_section*/
		boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>,
		/*tail_first_section*/
		boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>,
		/*head_second_section*/
		boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>,
		/*tail_second_section*/
		boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> > >
GPlatesAppLogic::TopologyInternalUtils::intersect_topological_sections(
		GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type first_section_polyline,
		GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type second_section_polyline)
{
	// Intersect the two section polylines.
	boost::shared_ptr<const GPlatesMaths::PolylineIntersections::Graph> intersection_graph =
			GPlatesMaths::PolylineIntersections::partition_intersecting_geometries(
					*first_section_polyline,
					*second_section_polyline);

	// If there were no intersections then return false.
	if (!intersection_graph)
	{
		return boost::none;
	}

	// Produce a warning message if there is more than one intersection.
	if (intersection_graph->intersections.size() >= 2)
	{
		std::cerr << "TopologyInternalUtils::intersect_topological_sections: " << std::endl
			<< "WARN: num_intersect=" << intersection_graph->intersections.size() << std::endl 
			<< "WARN: Boundary feature intersection relations may not be correct!" << std::endl
			<< "WARN: Make sure boundary feature's only intersect once." << std::endl
			<< std::endl;
		std::cerr << std::endl;
	}

	//
	// We have at least one intersection - ideally we're only expecting one intersection.
	//

	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			!intersection_graph->intersections.empty(),
			GPLATES_ASSERTION_SOURCE);

	// If even there is more than one intersection we'll just consider the first
	// intersection.
	const GPlatesMaths::PolylineIntersections::Graph::Intersection *intersection =
			intersection_graph->intersections[0].get();

	// If we have an intersection then we should have at least one segment from each polyline.
	// This is our guarantee to the caller.
	// Usually will have two for each but one can happen if intersection is a T or V junction.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			intersection->prev_partitioned_polyline1 || intersection->next_partitioned_polyline1,
			GPLATES_ASSERTION_SOURCE);
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			intersection->prev_partitioned_polyline2 || intersection->next_partitioned_polyline2,
			GPLATES_ASSERTION_SOURCE);

	// See comment in header file about T-junctions and V-junctions to see why
	// the returned segments are optional.
	boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> head_first_section;
	boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> head_second_section;
	boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> tail_first_section;
	boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> tail_second_section;

	//
	// If there is more than one intersection then only the first intersection is
	// considered such that this function still divides the two input sections into
	// (up to) two segments each (instead of several segments each).
	// It does this by concatenating divided segments (of each section) before or after
	// the intersection point into a single segment.
	//

	if (intersection->prev_partitioned_polyline1)
	{
		// Concatenate any polylines (from the first section) before the intersection.
		head_first_section = concatenate_partitioned_polylines_starting_at_intersection(
				intersection, PREVIOUS_POLYLINE1);
	}

	if (intersection->prev_partitioned_polyline2)
	{
		// Concatenate any polylines (from the second section) before the intersection.
		head_second_section = concatenate_partitioned_polylines_starting_at_intersection(
				intersection, PREVIOUS_POLYLINE2);
	}

	if (intersection->next_partitioned_polyline1)
	{
		// Concatenate any polylines (from the first section) after the intersection.
		tail_first_section = concatenate_partitioned_polylines_starting_at_intersection(
				intersection, NEXT_POLYLINE1);
	}

	if (intersection->next_partitioned_polyline2)
	{
		// Concatenate any polylines (from the second section) after the intersection.
		tail_second_section = concatenate_partitioned_polylines_starting_at_intersection(
				intersection, NEXT_POLYLINE2);
	}

	return boost::make_tuple(
			// The single intersection point...
			intersection->intersection_point,
			head_first_section,
			tail_first_section,
			head_second_section,
			tail_second_section);
}


boost::optional<
	boost::tuple<
		/*first intersection point*/
		GPlatesMaths::PointOnSphere,
		/*optional second intersection point*/
		boost::optional<GPlatesMaths::PointOnSphere>,
		/*optional info if two intersections and middle segments form a cycle*/
		boost::optional<bool>,
		/*optional head_first_section*/
		boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>,
		/*optional middle_first_section*/
		boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>,
		/*optional tail_first_section*/
		boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>,
		/*optional head_second_section*/
		boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>,
		/*optional middle_second_section*/
		boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>,
		/*optional tail_second_section*/
		boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> > >
GPlatesAppLogic::TopologyInternalUtils::intersect_topological_sections_allowing_two_intersections(
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type first_section_geometry,
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type second_section_geometry)
{
	// Get the two geometries as polylines if they are intersectable - that is if neither
	// geometry is a point or a multi-point.
	boost::optional<
		std::pair<
			GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type,
			GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> > polylines_for_intersection =
				get_polylines_for_intersection(first_section_geometry, second_section_geometry);

	// If the two geometries are not intersectable then return false.
	if (!polylines_for_intersection)
	{
		return boost::none;
	}

	const GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type first_section_polyline =
			polylines_for_intersection->first;
	const GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type second_section_polyline =
			polylines_for_intersection->second;

	boost::optional<
		boost::tuple<
			/*first intersection point*/
			GPlatesMaths::PointOnSphere,
			/*optional second intersection point*/
			boost::optional<GPlatesMaths::PointOnSphere>,
			/*optional info if two intersections and middle segments form a cycle*/
			boost::optional<bool>,
			/*optional head_first_section*/
			boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>,
			/*optional middle_first_section*/
			boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>,
			/*optional tail_first_section*/
			boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>,
			/*optional head_second_section*/
			boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>,
			/*optional middle_second_section*/
			boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>,
			/*optional tail_second_section*/
			boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> > >
				intersected_segments = intersect_topological_sections_allowing_two_intersections(
						first_section_polyline,
						second_section_polyline);

	if (!intersected_segments)
	{
		return boost::none;
	}

	// This helps us convert PolylineOnSphere to GeometryOnSphere in the tuple.
	const GPlatesMaths::PointOnSphere &first_intersection = boost::get<0>(*intersected_segments);
	boost::optional<GPlatesMaths::PointOnSphere> second_intersection;
	boost::optional<bool> middle_segments_from_a_cycle;
	boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> head_first_section;
	boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> middle_first_section;
	boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> tail_first_section;
	boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> head_second_section;
	boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> middle_second_section;
	boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> tail_second_section;
	boost::tie(
			boost::tuples::ignore,
			second_intersection,
			middle_segments_from_a_cycle,
			head_first_section,
			middle_first_section,
			tail_first_section,
			head_second_section,
			middle_second_section,
			tail_second_section) =
					*intersected_segments;

	return boost::make_tuple(
			first_intersection,
			second_intersection,
			middle_segments_from_a_cycle,
			head_first_section,
			middle_first_section,
			tail_first_section,
			head_second_section,
			middle_second_section,
			tail_second_section);
}


boost::optional<
	boost::tuple<
		/*first intersection point*/
		GPlatesMaths::PointOnSphere,
		/*optional second intersection point*/
		boost::optional<GPlatesMaths::PointOnSphere>,
		/*optional info if two intersections and middle segments form a cycle*/
		boost::optional<bool>,
		/*optional head_first_section*/
		boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>,
		/*optional middle_first_section*/
		boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>,
		/*optional tail_first_section*/
		boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>,
		/*optional head_second_section*/
		boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>,
		/*optional middle_second_section*/
		boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>,
		/*optional tail_second_section*/
		boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> > >
GPlatesAppLogic::TopologyInternalUtils::intersect_topological_sections_allowing_two_intersections(
		GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type first_section_polyline,
		GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type second_section_polyline)
{
	// Intersect the two section polylines.
	boost::shared_ptr<const GPlatesMaths::PolylineIntersections::Graph> intersection_graph =
			GPlatesMaths::PolylineIntersections::partition_intersecting_geometries(
					*first_section_polyline,
					*second_section_polyline);

	// If there were no intersections then return false.
	if (!intersection_graph)
	{
		return boost::none;
	}

	// Produce a warning message if there is more than two intersections.
	if (intersection_graph->intersections.size() >= 3)
	{
		std::cerr
			<< "TopologyInternalUtils::intersect_topological_sections_allowing_two_intersections: "
			<< std::endl
			<< "WARN: num_intersect=" << intersection_graph->intersections.size() << std::endl 
			<< "WARN: Boundary feature intersection relations may not be correct!" << std::endl
			<< "WARN: Make sure a boundary with exactly two sections only intersects twice."
			<< std::endl
			<< std::endl;
		std::cerr << std::endl;
	}

	//
	// We have at least one intersection - ideally we're only expecting one or two intersections.
	//

	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			!intersection_graph->intersections.empty(),
			GPLATES_ASSERTION_SOURCE);

	const GPlatesMaths::PolylineIntersections::Graph::Intersection *first_intersection =
			intersection_graph->intersections[0].get();

	// If we have an intersection then we should have at least one segment from each polyline.
	// This is our guarantee to the caller.
	// Usually will have two for each but one can happen if intersection is a T or V junction.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			first_intersection->prev_partitioned_polyline1 ||
					first_intersection->next_partitioned_polyline1,
			GPLATES_ASSERTION_SOURCE);
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			first_intersection->prev_partitioned_polyline2 ||
					first_intersection->next_partitioned_polyline2,
			GPLATES_ASSERTION_SOURCE);

	// See comment in header file about T-junctions and V-junctions to see why
	// the returned segments are optional.
	boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> head_first_section;
	boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> head_second_section;
	boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> middle_first_section;
	boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> middle_second_section;
	boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> tail_first_section;
	boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> tail_second_section;

	// If there's only one intersection then there is only head and tail segments -
	// no middle segments.
	if (intersection_graph->intersections.size() == 1)
	{
		if (first_intersection->prev_partitioned_polyline1)
		{
			head_first_section = first_intersection->prev_partitioned_polyline1->polyline;
		}
		if (first_intersection->prev_partitioned_polyline2)
		{
			head_second_section = first_intersection->prev_partitioned_polyline2->polyline;
		}
		if (first_intersection->next_partitioned_polyline1)
		{
			tail_first_section = first_intersection->next_partitioned_polyline1->polyline;
		}
		if (first_intersection->next_partitioned_polyline2)
		{
			tail_second_section = first_intersection->next_partitioned_polyline2->polyline;
		}

		return boost::make_tuple(
				// The single intersection point...
				first_intersection->intersection_point,
				boost::optional<GPlatesMaths::PointOnSphere>(), // no second intersection point
				boost::optional<bool>(), // no info about middle segments forming a cycle
				head_first_section,
				middle_first_section,
				tail_first_section,
				head_second_section,
				middle_second_section,
				tail_second_section);
	}

	//
	// If we get here then we have two or more intersections.
	//

	// If even there is more than two intersections we'll just consider the first two
	// intersections.
	const GPlatesMaths::PolylineIntersections::Graph::Intersection *
			second_intersection = intersection_graph->intersections[1].get();

	//
	// If there are more than two intersections then only the first two intersections are
	// considered such that this function still divides the two input sections into
	// (up to) three segments each (instead of several segments each).
	// It does this by concatenating divided segments (of each section) before the first
	// intersection point or after the second intersection point into a single segment.
	//

	// Determine if first intersection point is near the head or tail of each section.
	// This is necessary in order to find what we are defining as each section's middle segment
	// (in case there are more than two intersections).
	//
	// For example the middle segment could be the segment just after the first intersection
	// or the segment just before the last intersection depending on the direction of the
	// section's relative to each other.
	//
	// The head of a section ends at the first intersection if it has no previous
	// intersection when following that section's path through the intersection graph.
	const bool head_first_section_ends_at_first_intersection =
			!(first_intersection->prev_partitioned_polyline1 &&
					first_intersection->prev_partitioned_polyline1->prev_intersection);
	const bool head_second_section_ends_at_first_intersection =
			!(first_intersection->prev_partitioned_polyline2 &&
					first_intersection->prev_partitioned_polyline2->prev_intersection);

	// The middle segments form a cycle if the section directions oppose each other.
	const bool middle_segments_form_a_cycle =
			head_first_section_ends_at_first_intersection !=
					head_second_section_ends_at_first_intersection;

	if (head_first_section_ends_at_first_intersection)
	{
		if (first_intersection->prev_partitioned_polyline1)
		{
			head_first_section = first_intersection->prev_partitioned_polyline1->polyline;
		}

		// Since we have two intersections the middle segment should always exist.
		middle_first_section = first_intersection->next_partitioned_polyline1->polyline;

		if (second_intersection->next_partitioned_polyline1)
		{
			// Concatenate any polylines (from the first section) after the second intersection.
			tail_first_section = concatenate_partitioned_polylines_starting_at_intersection(
					second_intersection, NEXT_POLYLINE1);
		}
	}
	else
	{
		if (second_intersection->prev_partitioned_polyline1)
		{
			// Concatenate any polylines (from the first section) before the second intersection.
			head_first_section = concatenate_partitioned_polylines_starting_at_intersection(
					second_intersection, PREVIOUS_POLYLINE1);
		}

		// Since we have two intersections the middle segment should always exist.
		middle_first_section = second_intersection->next_partitioned_polyline1->polyline;

		if (first_intersection->next_partitioned_polyline1)
		{
			tail_first_section = first_intersection->next_partitioned_polyline1->polyline;
		}
	}

	if (head_second_section_ends_at_first_intersection)
	{
		if (first_intersection->prev_partitioned_polyline2)
		{
			head_second_section = first_intersection->prev_partitioned_polyline2->polyline;
		}

		// Since we have two intersections the middle segment should always exist.
		middle_second_section = first_intersection->next_partitioned_polyline2->polyline;

		if (second_intersection->next_partitioned_polyline2)
		{
			// Concatenate any polylines (from the second section) after the second intersection.
			tail_second_section = concatenate_partitioned_polylines_starting_at_intersection(
					second_intersection, NEXT_POLYLINE2);
		}
	}
	else
	{
		if (second_intersection->prev_partitioned_polyline2)
		{
			// Concatenate any polylines (from the first section) before the second intersection.
			head_second_section = concatenate_partitioned_polylines_starting_at_intersection(
					second_intersection, PREVIOUS_POLYLINE2);
		}

		// Since we have two intersections the middle segment should always exist.
		middle_second_section = second_intersection->next_partitioned_polyline2->polyline;

		if (first_intersection->next_partitioned_polyline2)
		{
			tail_second_section = first_intersection->next_partitioned_polyline2->polyline;
		}
	}

	return boost::make_tuple(
			// First intersection point
			first_intersection->intersection_point,
			// Second intersection point
			boost::optional<GPlatesMaths::PointOnSphere>(second_intersection->intersection_point),
			boost::optional<bool>(middle_segments_form_a_cycle),
			head_first_section,
			middle_first_section,
			tail_first_section,
			head_second_section,
			middle_second_section,
			tail_second_section);
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

bool
GPlatesAppLogic::TopologyInternalUtils::include_only_reconstructed_feature_geometries(
		const ReconstructionGeometry::non_null_ptr_to_const_type &recon_geom)
{
	// We only return true if the reconstruction geometry is a reconstructed feature geometry.
	return ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
			const ReconstructedFeatureGeometry>(recon_geom);
}
