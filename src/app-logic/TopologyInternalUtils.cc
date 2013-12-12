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
#include <boost/foreach.hpp>
#include <boost/none.hpp>
#include <boost/optional.hpp>
#include <QDebug>

#include "TopologyInternalUtils.h"

#include "ReconstructedFeatureGeometry.h"
#include "ReconstructionFeatureProperties.h"
#include "ReconstructionGeometryFinder.h"
#include "ReconstructionGeometryUtils.h"
#include "ReconstructionTree.h"
#include "ResolvedTopologicalGeometry.h"

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
#include "model/ModelUtils.h"

#include "property-values/GmlLineString.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlPolygon.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlPiecewiseAggregation.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlPropertyDelegate.h"
#include "property-values/GpmlTimeWindow.h"
#include "property-values/GpmlTopologicalLine.h"
#include "property-values/GpmlTopologicalLineSection.h"
#include "property-values/GpmlTopologicalNetwork.h"
#include "property-values/GpmlTopologicalPoint.h"
#include "property-values/GpmlTopologicalPolygon.h"
#include "property-values/XsString.h"

#include "utils/UnicodeStringUtils.h"


namespace
{
	/**
	 * Used to determine if a feature property is a topological geometry.
	 *
	 * This should be used to visit a single feature *property* (not a feature).
	 */
	class TopologicalGeometryPropertyValueType :
			public GPlatesModel::ConstFeatureVisitor
	{
	public:

		boost::optional<GPlatesPropertyValues::StructuralType>
		get_topological_geometry_property_value_type()
		{
			return d_topological_geometry_property_value_type;
		}


		virtual
		void
		visit_gpml_constant_value(
				const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
		{
			gpml_constant_value.value()->accept_visitor(*this);
		}

		virtual
		void
		visit_gpml_piecewise_aggregation(
				const GPlatesPropertyValues::GpmlPiecewiseAggregation &gpml_piecewise_aggregation)
		{
			GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlTimeWindow>::const_iterator begin =
					gpml_piecewise_aggregation.time_windows().begin();
			GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlTimeWindow>::const_iterator end =
					gpml_piecewise_aggregation.time_windows().end();

			// Only need to visit the first time window - all windows have the same template type.
			if (begin != end)
			{
				GPlatesPropertyValues::GpmlTimeWindow::non_null_ptr_to_const_type gpml_time_window = *begin;
				gpml_time_window->time_dependent_value()->accept_visitor(*this);
			}
		}

		virtual
		void
		visit_gpml_topological_network(
				const GPlatesPropertyValues::GpmlTopologicalNetwork &gpml_toplogical_network)
		{
			d_topological_geometry_property_value_type = gpml_toplogical_network.get_structural_type();
		}

		virtual
		void
		visit_gpml_topological_line(
				const GPlatesPropertyValues::GpmlTopologicalLine &gpml_topological_line)
		{
			d_topological_geometry_property_value_type = gpml_topological_line.get_structural_type();
		}

		virtual
		void
		visit_gpml_topological_polygon(
				const GPlatesPropertyValues::GpmlTopologicalPolygon &gpml_topological_polygon)
		{
			d_topological_geometry_property_value_type = gpml_topological_polygon.get_structural_type();
		}

	private:

		boost::optional<GPlatesPropertyValues::StructuralType> d_topological_geometry_property_value_type;
	};


	/**
	 * Creates a @a GpmlTopologicalSection
	 */
	class CreateTopologicalSectionPropertyValue :
			public GPlatesModel::ConstFeatureVisitor
	{
	public:

		CreateTopologicalSectionPropertyValue() :
			d_reverse_order(false),
			d_visited_topological_line(false)
		{  }

		boost::optional<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type>
		create_gpml_topological_section(
				const GPlatesModel::FeatureHandle::iterator &geometry_property,
				bool reverse_order)
		{
			if (!geometry_property.is_still_valid())
			{
				// Return invalid iterator.
				return boost::none;
			}

			// Initialise data members.
			d_geometry_property = geometry_property;
			d_reverse_order = reverse_order;

			d_topological_section = boost::none;
			d_visited_topological_line = false;

			(*geometry_property)->accept_visitor(*this);

			return d_topological_section;
		}

	private:
		GPlatesModel::FeatureHandle::iterator d_geometry_property;
		bool d_reverse_order;

		boost::optional<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type> d_topological_section;

		//! If GpmlTopologicalLine is in a piecewise aggregration then we only need to visit one time window.
		bool d_visited_topological_line;


		virtual
		void
		visit_gml_line_string(
			const GPlatesPropertyValues::GmlLineString &gml_line_string)
		{
			static const GPlatesPropertyValues::StructuralType property_value_type =
					GPlatesPropertyValues::StructuralType::create_gml("LineString");

			create_topological_line_section(gml_line_string.get_structural_type());
		}


		virtual
		void
		visit_gml_multi_point(
			const GPlatesPropertyValues::GmlMultiPoint &/*gml_multi_point*/)
		{
			qWarning() << "WARNING: GpmlTopologicalSection not created for gml:MultiPoint.";
			qWarning() << "FeatureId = " << GPlatesUtils::make_qstring_from_icu_string(
					d_geometry_property.handle_weak_ref()->feature_id().get());
			qWarning() << "PropertyName = " << GPlatesUtils::make_qstring_from_icu_string(
					(*d_geometry_property)->get_property_name().get_name());
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
			const GPlatesPropertyValues::GmlPoint &gml_point)
		{
			create_topological_point(gml_point.get_structural_type());
		}


		virtual
		void
		visit_gml_polygon(
			const GPlatesPropertyValues::GmlPolygon &/*gml_polygon*/)
		{
			static const GPlatesPropertyValues::StructuralType property_value_type =
					GPlatesPropertyValues::StructuralType::create_gml("LinearRing");

			create_topological_line_section(property_value_type);
		}


		virtual
		void
		visit_gpml_constant_value(
			const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
		{
			gpml_constant_value.value()->accept_visitor(*this);
		}


		virtual
		void
		visit_gpml_piecewise_aggregation(
				const GPlatesPropertyValues::GpmlPiecewiseAggregation &gpml_piecewise_aggregation)
		{
			GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlTimeWindow>::const_iterator iter =
					gpml_piecewise_aggregation.time_windows().begin();
			GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlTimeWindow>::const_iterator end =
					gpml_piecewise_aggregation.time_windows().end();

			for ( ; iter != end; ++iter) 
			{
				GPlatesPropertyValues::GpmlTimeWindow::non_null_ptr_to_const_type gpml_time_window = *iter;
				gpml_time_window->time_dependent_value()->accept_visitor(*this);

				// Break out early if (first) time window has a topological line property.
				// We only need to know there's a GpmlTopologicalLine present in order to reference it.
				if (d_visited_topological_line)
				{
					break;
				}
			}
		}


		virtual
		void
		visit_gpml_topological_line(
				const GPlatesPropertyValues::GpmlTopologicalLine &gpml_topological_line)
		{
			// FIXME: This might need to be "PiecewiseAggregation" instead of "TopologicalLine".
			// In any case the property *type* is not currently used by the topology resolver.
			create_topological_line_section(gpml_topological_line.get_structural_type());

			d_visited_topological_line = true;
		}


		void
		create_topological_point(
				const GPlatesPropertyValues::StructuralType &property_value_type)
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
				const GPlatesPropertyValues::StructuralType &property_value_type)
		{
			boost::optional<GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type> geom_delegate =
					GPlatesAppLogic::TopologyInternalUtils::create_geometry_property_delegate(
							d_geometry_property, property_value_type);

			if (!geom_delegate)
			{
				return;
			}

			// Create a GpmlTopologicalLineSection from the delegate.
			d_topological_section = GPlatesPropertyValues::GpmlTopologicalLineSection::create(
					*geom_delegate,
					d_reverse_order);
		}
	};


	/**
	 * Creates a @a GpmlTopologicalNetwork::Interior
	 */
	class CreateTopologicalNetworkInterior :
			public GPlatesModel::ConstFeatureVisitor
	{
	public:

		CreateTopologicalNetworkInterior() :
			d_visited_topological_line(false)
		{  }

		boost::optional<GPlatesPropertyValues::GpmlTopologicalNetwork::Interior>
		create_gpml_topological_network_interior(
				const GPlatesModel::FeatureHandle::iterator &geometry_property)
		{
			if (!geometry_property.is_still_valid())
			{
				// Return invalid iterator.
				return boost::none;
			}

			// Initialise data members.
			d_geometry_property = geometry_property;

			d_topological_interior = boost::none;
			d_visited_topological_line = false;

			(*geometry_property)->accept_visitor(*this);

			return d_topological_interior;
		}

	private:
		GPlatesModel::FeatureHandle::iterator d_geometry_property;

		boost::optional<GPlatesPropertyValues::GpmlTopologicalNetwork::Interior> d_topological_interior;

		//! If GpmlTopologicalLine is in a piecewise aggregration then we only need to visit one time window.
		bool d_visited_topological_line;


		virtual
		void
		visit_gml_line_string(
				const GPlatesPropertyValues::GmlLineString &gml_line_string)
		{
			create_topological_network_interior(gml_line_string.get_structural_type());
		}


		virtual
		void
		visit_gml_multi_point(
				const GPlatesPropertyValues::GmlMultiPoint &gml_multi_point)
		{
			create_topological_network_interior(gml_multi_point.get_structural_type());
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
				const GPlatesPropertyValues::GmlPoint &gml_point)
		{
			create_topological_network_interior(gml_point.get_structural_type());
		}


		virtual
		void
		visit_gml_polygon(
				const GPlatesPropertyValues::GmlPolygon &/*gml_polygon*/)
		{
			static const GPlatesPropertyValues::StructuralType property_value_type =
					GPlatesPropertyValues::StructuralType::create_gml("LinearRing");

			create_topological_network_interior(property_value_type);
		}


		virtual
		void
		visit_gpml_constant_value(
				const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
		{
			gpml_constant_value.value()->accept_visitor(*this);
		}


		virtual
		void
		visit_gpml_piecewise_aggregation(
				const GPlatesPropertyValues::GpmlPiecewiseAggregation &gpml_piecewise_aggregation)
		{
			GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlTimeWindow>::const_iterator iter =
					gpml_piecewise_aggregation.time_windows().begin();
			GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlTimeWindow>::const_iterator end =
					gpml_piecewise_aggregation.time_windows().end();

			for ( ; iter != end; ++iter) 
			{
				GPlatesPropertyValues::GpmlTimeWindow::non_null_ptr_to_const_type gpml_time_window = *iter;
				gpml_time_window->time_dependent_value()->accept_visitor(*this);

				// Break out early if (first) time window has a topological line property.
				// We only need to know there's a GpmlTopologicalLine present in order to reference it.
				if (d_visited_topological_line)
				{
					break;
				}
			}
		}


		virtual
		void
		visit_gpml_topological_line(
				const GPlatesPropertyValues::GpmlTopologicalLine &gpml_topological_line)
		{
			// FIXME: This might need to be "PiecewiseAggregation" instead of "TopologicalLine".
			// In any case the property *type* is not currently used by the topology resolver.
			create_topological_network_interior(gpml_topological_line.get_structural_type());

			d_visited_topological_line = true;
		}


		void
		create_topological_network_interior(
				const GPlatesPropertyValues::StructuralType &property_value_type)
		{
			boost::optional<GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type> geom_delegate =
					GPlatesAppLogic::TopologyInternalUtils::create_geometry_property_delegate(
							d_geometry_property, property_value_type);

			if (!geom_delegate)
			{
				return;
			}

			// Create a GpmlTopologicalNetwork::Interior from the delegate.
			d_topological_interior =
					GPlatesPropertyValues::GpmlTopologicalNetwork::Interior(*geom_delegate);
		}
	};


	boost::optional<GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_type>
	find_topological_section_reconstruction_geometry(
			const std::vector<GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_type> &found_rgs,
			const std::vector<GPlatesModel::FeatureHandle::weak_ref> &feature_refs,
			const GPlatesModel::PropertyName &property_name,
			const double &reconstruction_time)
	{
		// FIXME: MULTIPLE GEOM

		if (found_rgs.empty())
		{
			// If we found no RFG that is reconstructed from 'geometry_property' then it probably means
			// the reconstruction time is outside the age range of the feature containing 'geometry_property'.
			// This is ok - it's not necessarily an error.
			// In fact we only report an error if any of the referenced features exist at the current
			// reconstruction time because, with the addition of resolved *line* topologies, one use
			// case is emulating a time-dependent section list by using a single list where a subset
			// of the resolved line's sections emulate one physical section over time (because each section
			// in the subset represents that section for a small time period and these time periods do
			// not overlap) - as the time changes one section will disappear at the same time another
			// will appear to take its place - and in this case, at any particular time, not all sections
			// in the list will actually exist and we don't want to report this as an error.
			BOOST_FOREACH(const GPlatesModel::FeatureHandle::weak_ref &feature_ref, feature_refs)
			{
				GPlatesAppLogic::ReconstructionFeatureProperties visitor;
				visitor.visit_feature(feature_ref);
				if (visitor.is_feature_defined_at_recon_time())
				{
					qWarning() << "No topological sections were found at " << reconstruction_time << "Ma for:";
					qWarning() << "  feature id =" 
						<< GPlatesUtils::make_qstring_from_icu_string( feature_ref->feature_id().get() );
					static const GPlatesModel::PropertyName prop = GPlatesModel::PropertyName::create_gml("name");
					boost::optional<GPlatesPropertyValues::XsString::non_null_ptr_to_const_type> name =
							GPlatesFeatureVisitors::get_property_value<GPlatesPropertyValues::XsString>(
									feature_ref, prop);
					if (name)
					{
						qWarning() << "  feature name =" << GPlatesUtils::make_qstring( name.get()->get_value() );
					}
					else
					{
						qWarning() << "  feature name = UNKNOWN";
					}
					qWarning() << "  property name =" << property_name.get_name().qstring();
					qWarning() << "  Unable to use any topological section.";

					break;
				}
			}

			return boost::none;
		}
		else if (found_rgs.size() > 1)
		{
			// See if the same feature id is used by multiple features that generated found
			// reconstruction geometries. We allowed multiple features with same feature id up
			// until now on the chance that the found recon geoms would only come from one of the
			// features (thus making the search non-ambiguous). But if this is not the case then
			// we'll emit a warning and return no reconstruction geometries, thus forcing the user
			// to either avoid loading multiple features with the same feature id into GPlates or
			// suitably restrict the found reconstruction geometries (using reconstruct handles
			// that limit to a specific layer or file) such that the search ambiguity is removed.
			boost::optional<GPlatesModel::FeatureHandle::weak_ref> found_feature_ref;
			BOOST_FOREACH(const GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_type &found_rg, found_rgs)
			{
				boost::optional<GPlatesModel::FeatureHandle::weak_ref> feature_ref =
						GPlatesAppLogic::ReconstructionGeometryUtils::get_feature_ref(found_rg);
				if (!feature_ref)
				{
					continue;
				}

				// Find the feature ref of the first recon geom.
				if (!found_feature_ref)
				{
					found_feature_ref = feature_ref.get();
					continue;
				}

				// All reconstruction geometries should refer to the same feature.
				if (feature_ref.get() != found_feature_ref.get())
				{
					qWarning() 
						<< "Multiple features for feature-id = "
						<< GPlatesUtils::make_qstring_from_icu_string(
								feature_ref.get()->feature_id().get() );
					return boost::none;
				}
			}

			if (found_feature_ref)
			{
				// We should only return boost::none for the case == 0, as above.
				// For the case >1 we return first RG found as normally.
				int num = found_rgs.size();
				qWarning() << num << "Topological sections found for:";
				qWarning() << "  feature id =" 
					<< GPlatesUtils::make_qstring_from_icu_string( found_feature_ref.get()->feature_id().get() );
				static const GPlatesModel::PropertyName prop = GPlatesModel::PropertyName::create_gml("name");
				boost::optional<GPlatesPropertyValues::XsString::non_null_ptr_to_const_type> name =
						GPlatesFeatureVisitors::get_property_value<GPlatesPropertyValues::XsString>(
								found_feature_ref.get(), prop);
				if (name)
				{
					qWarning() << "  feature name =" << GPlatesUtils::make_qstring( name.get()->get_value() );
				}
				else
				{
					qWarning() << "  feature name = UNKNOWN";
				}
				qWarning() << "  property name =" << property_name.get_name().qstring();
				qWarning() << "  Using the first topological section found.";
			}
		}

		// Return the first RG found.
		return found_rgs.front();
	}


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


boost::optional<GPlatesPropertyValues::StructuralType>
GPlatesAppLogic::TopologyInternalUtils::get_topology_geometry_property_value_type(
		const GPlatesModel::FeatureHandle::const_iterator &property)
{
	TopologicalGeometryPropertyValueType visitor;
	(*property)->accept_visitor(visitor);

	return visitor.get_topological_geometry_property_value_type();
}


boost::optional<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type>
GPlatesAppLogic::TopologyInternalUtils::create_gpml_topological_section(
		const GPlatesModel::FeatureHandle::iterator &geometry_property,
		bool reverse_order)

{
	CreateTopologicalSectionPropertyValue create_topological_section_property_value;

	return create_topological_section_property_value.create_gpml_topological_section(
			geometry_property,
			reverse_order);
}


boost::optional<GPlatesPropertyValues::GpmlTopologicalNetwork::Interior>
GPlatesAppLogic::TopologyInternalUtils::create_gpml_topological_network_interior(
		const GPlatesModel::FeatureHandle::iterator &geometry_property)
{
	CreateTopologicalNetworkInterior create_topological_interior_property_value;

	return create_topological_interior_property_value.create_gpml_topological_network_interior(geometry_property);
}


boost::optional<GPlatesPropertyValues::GpmlPropertyDelegate::non_null_ptr_type>
GPlatesAppLogic::TopologyInternalUtils::create_geometry_property_delegate(
		const GPlatesModel::FeatureHandle::iterator &geometry_property,
		const GPlatesPropertyValues::StructuralType &property_value_type)
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
			(*geometry_property)->get_property_name().get_name());

	const GPlatesModel::PropertyName prop_name =
			GPlatesModel::PropertyName::create_gpml(property_name);

	return GPlatesPropertyValues::GpmlPropertyDelegate::create( 
			feature_id,
			prop_name,
			property_value_type);
}


GPlatesModel::FeatureHandle::weak_ref
GPlatesAppLogic::TopologyInternalUtils::resolve_feature_id(
		const GPlatesModel::FeatureId &feature_id)
{
	return GPlatesModel::ModelUtils::find_feature(feature_id);
}


boost::optional<GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_type>
GPlatesAppLogic::TopologyInternalUtils::find_topological_reconstruction_geometry(
		const GPlatesPropertyValues::GpmlPropertyDelegate &geometry_delegate,
		const double &reconstruction_time,
		boost::optional<const std::vector<ReconstructHandle::type> &> reconstruct_handles)
{
	// Find all features with the feature id specified by the geometry delegate.
	// Typically there should be only one feature since it's not generally a good idea to
	// load multiple features with the same feature id into GPlates because both features
	// will get found and it'll be ambiguous as to which one to use.
	//
	// However there are situations where this is can happen such as loading two different
	// topology datasets that happen to have the same feature ids (presumably because one GPML
	// file was copied to another and the second one modified to be different than the first).
	// In this case the user might load both files (creating two separate layers) and compare them.
	// Then if the user restricts the topological sections for each layer then, although two
	// features will be found with the same feature id (one from each layer), only one reconstruction
	// geometry will get found after restricting the topological sections using the reconstruct handles.
	//
	std::vector<GPlatesModel::FeatureHandle::weak_ref> resolved_features;
	geometry_delegate.get_feature_id().find_back_ref_targets(
			GPlatesModel::append_as_weak_refs(resolved_features));

	// We didn't get exactly one feature with the feature id so something is
	// not right (user loaded same file twice or didn't load at all)
	// so print debug message and return null feature reference.
	if (resolved_features.empty())
	{
		qWarning() 
			<< "Missing feature for feature-id = "
			<< GPlatesUtils::make_qstring_from_icu_string(geometry_delegate.get_feature_id().get());
		return boost::none;
	}

	// Create a property name from the target_propery.
	const QString property_name_qstring = GPlatesUtils::make_qstring_from_icu_string(
			geometry_delegate.get_target_property_name().get_name());
	const GPlatesModel::PropertyName property_name = GPlatesModel::PropertyName::create_gpml(
			property_name_qstring);

	// Find all the reconstruction geometries that reference the resolved features, and that
	// are restricted by the reconstruct handles.
	std::vector<ReconstructionGeometry::non_null_ptr_type> found_rgs;
	BOOST_FOREACH(const GPlatesModel::FeatureHandle::weak_ref &resolved_feature, resolved_features)
	{
		// Find the reconstruction geometries for the feature ref and target property.
		ReconstructionGeometryFinder rg_finder(
				property_name,
				reconstruct_handles); 
		rg_finder.find_rgs_of_feature(resolved_feature);

		found_rgs.insert(found_rgs.end(), rg_finder.found_rgs_begin(), rg_finder.found_rgs_end());
	}

	return ::find_topological_section_reconstruction_geometry(
			found_rgs,
			resolved_features,
			property_name,
			reconstruction_time);
}


boost::optional<GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_type>
GPlatesAppLogic::TopologyInternalUtils::find_topological_reconstruction_geometry(
		const GPlatesModel::FeatureHandle::iterator &geometry_property,
		const double &reconstruction_time,
		boost::optional<const std::vector<ReconstructHandle::type> &> reconstruct_handles)
{
	if (!geometry_property.is_still_valid())
	{
		return boost::none;
	}

	// Get a reference to the feature containing the geometry property.
	const GPlatesModel::FeatureHandle::weak_ref &feature_ref =
			geometry_property.handle_weak_ref();

	// Find the reconstruction geometries, referencing 'reconstruction_tree', for the feature ref
	// and geometry property.
	ReconstructionGeometryFinder rg_finder(
			geometry_property,
			reconstruct_handles); 
	rg_finder.find_rgs_of_feature(feature_ref);

	return ::find_topological_section_reconstruction_geometry(
			std::vector<ReconstructionGeometry::non_null_ptr_type>(
					rg_finder.found_rgs_begin(),
					rg_finder.found_rgs_end()),
			std::vector<GPlatesModel::FeatureHandle::weak_ref>(1, feature_ref),
			(*geometry_property)->get_property_name(),
			reconstruction_time);
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

	// If we can't get a reconstruction plate ID then we'll just use plate id zero (spin axis)
	// which can still give a non-identity rotation if the anchor plate id is non-zero.
	GPlatesModel::integer_plate_id_type reconstruction_plate_id = 0;
	boost::optional<GPlatesPropertyValues::GpmlPlateId::non_null_ptr_to_const_type> recon_plate_id =
		GPlatesFeatureVisitors::get_property_value<GPlatesPropertyValues::GpmlPlateId>(
				reconstruction_plateid_feature,
				plate_id_property_name);
	if (recon_plate_id)
	{
		reconstruction_plate_id = recon_plate_id.get()->get_value();
	}

	// The feature has a reconstruction plate ID.
	// Return the rotation.
	return reconstruction_tree.get_composed_absolute_rotation(reconstruction_plate_id).first;
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
		qWarning() << "TopologyInternalUtils::intersect_topological_sections: ";
		qWarning() << "	num_intersect=" << intersection_graph->intersections.size();
		qWarning() << "	Boundary feature intersection relations may not be correct!";
		qWarning() << "	Make sure boundary feature's only intersect once.";
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
		qWarning() << "TopologyInternalUtils::intersect_topological_sections_allowing_two_intersections: ";
		qWarning() << "	num_intersect=" << intersection_graph->intersections.size();
		qWarning() << "	Boundary feature intersection relations may not be correct!";
		qWarning() << "	Make sure a boundary with exactly two sections only intersects twice.";
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
	static GPlatesMaths::AngularExtent closeness_angular_extent_threshold =
			GPlatesMaths::AngularExtent::create_from_cosine(0.9);

	// Determine closeness to first intersected segment.
	GPlatesMaths::real_t closeness_first_segment = -1/*least close*/;
	const bool is_reference_point_close_to_first_segment = first_intersected_segment->is_close_to(
		section_reference_point,
		closeness_angular_extent_threshold,
		closeness_first_segment);

	// Determine closeness to second intersected segment.
	GPlatesMaths::real_t closeness_second_segment = -1/*least close*/;
	const bool is_reference_point_close_to_second_segment = second_intersected_segment->is_close_to(
		section_reference_point,
		closeness_angular_extent_threshold,
		closeness_second_segment);

	// Make sure that the reference point is close to one of the segments.
	if ( !is_reference_point_close_to_first_segment &&
		!is_reference_point_close_to_second_segment ) 
	{
		qWarning() << "TopologyInternalUtils::find_closest_intersected_segment_to_reference_point: ";
		qWarning() << "	reference point not close to anything!";
		qWarning() << "	Arbitrarily selecting one of the intersected segments!";

		// FIXME: do something better than just arbitrarily select one of the segments as closest.
		return first_intersected_segment;
	}

	// Return the closest segment.
	return (closeness_first_segment > closeness_second_segment)
			? first_intersected_segment
			: second_intersected_segment;
}


bool
GPlatesAppLogic::TopologyInternalUtils::can_use_as_resolved_line_topological_section(
		const ReconstructionGeometry::non_null_ptr_to_const_type &recon_geom)
{
	// We only return true if the reconstruction geometry is a reconstructed feature geometry.
	boost::optional<const ReconstructedFeatureGeometry *> rfg =
			ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
					const ReconstructedFeatureGeometry>(recon_geom);
	if (!rfg)
	{
		return false;
	}

	// TODO: Filter out reconstructed geometries that have been deformed by topological network layers.
	// These reconstructed geometries cannot supply topological sections (to topological network layers)
	// because these reconstructed geometries are deformed by the topological networks which in turn
	// would use the reconstructed geometries to build the topological networks - thus creating a
	// cyclic dependency (so the layer system excludes those reconstruct layers that are deformed).
	// Note that these reconstructed geometries also cannot supply topological sections to
	// topological 'geometry' layers, eg containing topological lines, because those resolved
	// topological lines can, in turn, be used as topological sections by topological networks -
	// so there's still a cyclic dependency (it's just a more round-about or indirect dependency).
	//
	// This is a bit tricky because it's not easy to find the layer than reconstructed the RFG
	// (so that we can query whether it's connected to topological layers) because RFGs should
	// not know which layer they were generated from because they might not have been generated from
	// a layer (eg, if generated by GPlates command-line tool) or simply just not created from a layer.
	//
	// It's not so bad though because even if we could detect this it would not prevent the
	// user from building a topology using an RFG and then subsequently connecting that RFG's layer
	// to a topology layer (to deform it). In this situation the RFG would disappear from the
	// topology's boundary (or interior) as soon as its layer was connected a topology layer.
	//
	// In any case it'll be up to the topology builder user to not used deformed geometries as
	// topological sections.

	return true;
}


bool
GPlatesAppLogic::TopologyInternalUtils::can_use_as_resolved_boundary_topological_section(
		const ReconstructionGeometry::non_null_ptr_to_const_type &recon_geom)
{
	// If can use as a topological section for a resolved line then can also use for resolved boundary.
	if (can_use_as_resolved_line_topological_section(recon_geom))
	{
		return true;
	}

	// See if RTG.
	boost::optional<const ResolvedTopologicalGeometry *> resolved_topological_geometry =
			ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
					const ResolvedTopologicalGeometry>(recon_geom);
	if (resolved_topological_geometry)
	{
		// Return true if an RTG with a polyline geometry (a resolved topological line).
		if (resolved_topological_geometry.get()->resolved_topology_line())
		{
			// NOTE: We don't need to check that the resolved line was not formed from
			// RFGs that were deformed - see 'can_use_as_resolved_line_topological_section()' -
			// because the layer system prevents layers containing deformed RFGs from being searched
			// for topological sections (hence the resolved lines won't find them).
			return true;
		}
	}

	return false;
}


bool
GPlatesAppLogic::TopologyInternalUtils::can_use_as_resolved_network_topological_section(
		const ReconstructionGeometry::non_null_ptr_to_const_type &recon_geom)
{
	return can_use_as_resolved_boundary_topological_section(recon_geom);
}
