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
#include <utility>
#include <vector>
#include <boost/foreach.hpp>
#include <boost/none.hpp>
#include <boost/optional.hpp>
#include <QDebug>

#include "TopologyInternalUtils.h"

#include "AppLogicUtils.h"
#include "GeometryUtils.h"
#include "ReconstructedFeatureGeometry.h"
#include "ReconstructionFeatureProperties.h"
#include "ReconstructionGeometryFinder.h"
#include "ReconstructionGeometryUtils.h"
#include "ReconstructionTree.h"
#include "ResolvedTopologicalBoundary.h"
#include "ResolvedTopologicalLine.h"
#include "TopologyReconstructedFeatureGeometry.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "maths/ConstGeometryOnSphereVisitor.h"
#include "maths/FiniteRotation.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolylineOnSphere.h"

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

#include "utils/Profile.h"
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
			// Nothing to do - we don't create topological sections for multi-points.

// The caller will be able to detect this error since they'll end up with no topological section.
#if 0
			qWarning() << "WARNING: GpmlTopologicalSection not created for gml:MultiPoint.";
			qWarning() << "FeatureId = " << GPlatesUtils::make_qstring_from_icu_string(
					d_geometry_property.handle_weak_ref()->feature_id().get());
			qWarning() << "PropertyName = " << GPlatesUtils::make_qstring_from_icu_string(
					(*d_geometry_property)->get_property_name().get_name());
#endif
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


	/**
	 * Used to find feature IDs of all topological sections referenced by topological geometries/networks.
	 */
	class FindTopologicalSectionsReferenced :
			public GPlatesModel::ConstFeatureVisitor
	{
	public:

		explicit
		FindTopologicalSectionsReferenced(
				std::set<GPlatesModel::FeatureId> &topological_sections_referenced,
				boost::optional<GPlatesAppLogic::TopologyGeometry::Type> topology_geometry_type = boost::none,
				boost::optional<double> reconstruction_time = boost::none) :
			d_topological_sections_referenced(topological_sections_referenced),
			d_topology_geometry_type(topology_geometry_type),
			d_reconstruction_time(reconstruction_time)
		{  }


		virtual
		bool
		initialise_pre_feature_properties(
				const GPlatesModel::FeatureHandle &feature_handle)
		{
			// If we have a reconstruction time then make sure this feature is defined at that time.
			if (d_reconstruction_time)
			{
				GPlatesAppLogic::ReconstructionFeatureProperties reconstruction_params;
				reconstruction_params.visit_feature(feature_handle.reference());
				if (!reconstruction_params.is_feature_defined_at_recon_time(d_reconstruction_time.get()))
				{
					return false;
				}
			}

			// Now visit each of the properties in turn.
			return true;
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
			const GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlTimeWindow> &time_windows =
					gpml_piecewise_aggregation.time_windows();

			// NOTE: If there's only one time window then we do not check its time period against the
			// current reconstruction time (if checking reconstruction time).
			// This mirrors the compromise implemented in TopologyNetworkResolver and TopologyGeometryResolver
			// (see 'visit_gpml_piecewise_aggregation()' in those classes for more details).
			if (time_windows.size() == 1)
			{
				time_windows.front().get()->time_dependent_value()->accept_visitor(*this);
				return;
			}

			GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlTimeWindow>::const_iterator iter = time_windows.begin();
			GPlatesModel::RevisionedVector<GPlatesPropertyValues::GpmlTimeWindow>::const_iterator end = time_windows.end();
			for ( ; iter != end; ++iter) 
			{
				const GPlatesPropertyValues::GpmlTimeWindow &time_window = *iter->get();

				// If we have a reconstruction time and the time window period contains it then visit the time window.
				// The time periods should be mutually exclusive - if we happen to be in
				// two time periods then we're probably right on the boundary between the two
				// in which case we'll only visit the first time window encountered to mirror what
				// the topology geometry/network resolvers do.
				if (d_reconstruction_time)
				{
					if (time_window.valid_time()->contains(d_reconstruction_time.get()))
					{
						time_window.time_dependent_value()->accept_visitor(*this);
						return;
					}
				}
				else
				{
					// We don't have a reconstruction time so we'll visit all time windows.
					time_window.time_dependent_value()->accept_visitor(*this);
				}
			}
		}

		virtual
		void
		visit_gpml_topological_network(
				const GPlatesPropertyValues::GpmlTopologicalNetwork &gpml_topological_network)
		{
			// Filter based on topology geometry type (if requested).
			if (d_topology_geometry_type &&
				d_topology_geometry_type.get() != GPlatesAppLogic::TopologyGeometry::NETWORK)
			{
				return;
			}

			// Loop over all the boundary sections.
			GPlatesPropertyValues::GpmlTopologicalNetwork::boundary_sections_seq_type boundary_sections =
					gpml_topological_network.get_boundary_sections();

			// Loop over all the boundary sections.
			GPlatesPropertyValues::GpmlTopologicalNetwork::boundary_sections_seq_type::iterator boundary_iter =
					boundary_sections.begin();
			GPlatesPropertyValues::GpmlTopologicalNetwork::boundary_sections_seq_type::iterator boundary_end =
					boundary_sections.end();
			for ( ; boundary_iter != boundary_end; ++boundary_iter)
			{
				GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type topological_section =
						boundary_iter->get_source_section();

				topological_section->accept_visitor(*this);
			}

			// Loop over all the interior geometries.
			GPlatesPropertyValues::GpmlTopologicalNetwork::interior_geometry_seq_type interior_geometries =
					gpml_topological_network.get_interior_geometries();

			// Loop over all the interior geometries.
			GPlatesPropertyValues::GpmlTopologicalNetwork::interior_geometry_seq_type::const_iterator interior_iter =
					interior_geometries.begin();
			GPlatesPropertyValues::GpmlTopologicalNetwork::interior_geometry_seq_type::const_iterator interior_end =
					interior_geometries.end();
			for ( ; interior_iter != interior_end; ++interior_iter)
			{
				const GPlatesPropertyValues::GpmlTopologicalNetwork::Interior &interior = *interior_iter;

				// Add the feature ID of the interior geometry.
				d_topological_sections_referenced.insert(
						interior.get_source_geometry()->get_feature_id());
			}
		}

		virtual
		void
		visit_gpml_topological_line(
				const GPlatesPropertyValues::GpmlTopologicalLine &gpml_topological_line)
		{
			// Filter based on topology geometry type (if requested).
			if (d_topology_geometry_type &&
				d_topology_geometry_type.get() != GPlatesAppLogic::TopologyGeometry::LINE)
			{
				return;
			}

			// Loop over all the sections.
			GPlatesPropertyValues::GpmlTopologicalLine::sections_seq_type sections =
					gpml_topological_line.get_sections();

			GPlatesPropertyValues::GpmlTopologicalLine::sections_seq_type::const_iterator sections_iter = sections.begin();
			GPlatesPropertyValues::GpmlTopologicalLine::sections_seq_type::const_iterator sections_end = sections.end();
			for ( ; sections_iter != sections_end; ++sections_iter)
			{
				GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_to_const_type topological_section =
						sections_iter->get_source_section();

				topological_section->accept_visitor(*this);
			}
		}

		virtual
		void
		visit_gpml_topological_polygon(
				const GPlatesPropertyValues::GpmlTopologicalPolygon &gpml_topological_polygon)
		{
			// Filter based on topology geometry type (if requested).
			if (d_topology_geometry_type &&
				d_topology_geometry_type.get() != GPlatesAppLogic::TopologyGeometry::BOUNDARY)
			{
				return;
			}

			// Loop over all the exterior sections.
			GPlatesPropertyValues::GpmlTopologicalPolygon::sections_seq_type exterior_sections =
					gpml_topological_polygon.get_exterior_sections();
			GPlatesPropertyValues::GpmlTopologicalPolygon::sections_seq_type::const_iterator exterior_sections_iter =
					exterior_sections.begin();
			GPlatesPropertyValues::GpmlTopologicalPolygon::sections_seq_type::const_iterator exterior_sections_end =
					exterior_sections.end();
			for ( ; exterior_sections_iter != exterior_sections_end; ++exterior_sections_iter)
			{
				GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_to_const_type topological_section =
						exterior_sections_iter->get_source_section();

				topological_section->accept_visitor(*this);
			}
		}

		virtual
		void
		visit_gpml_topological_line_section(
				const GPlatesPropertyValues::GpmlTopologicalLineSection &gpml_toplogical_line_section)
		{
			// Add the feature ID of the line section geometry.
			d_topological_sections_referenced.insert(
					gpml_toplogical_line_section.get_source_geometry()->get_feature_id());
		}

		virtual
		void
		visit_gpml_topological_point(
				const GPlatesPropertyValues::GpmlTopologicalPoint &gpml_toplogical_point)
		{
			// Add the feature ID of the point geometry.
			d_topological_sections_referenced.insert(
					gpml_toplogical_point.get_source_geometry()->get_feature_id());
		}

	private:
		std::set<GPlatesModel::FeatureId> &d_topological_sections_referenced;
		boost::optional<GPlatesAppLogic::TopologyGeometry::Type> d_topology_geometry_type;
		boost::optional<double> d_reconstruction_time;
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
// These errors never really get fixed in the topology datasets so might as well stop spamming the log.
// Better to write a pyGPlates script to detect these types of errors as a post-process.
#if 0
			// If we found no RFG then it probably means the reconstruction time is outside the age range
			// of the referenced features. This is ok - it's not necessarily an error.
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
				if (visitor.is_feature_defined_at_recon_time(reconstruction_time))
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
#endif

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
// These errors never really get fixed in the topology datasets so might as well stop spamming the log.
// Better to write a pyGPlates script to detect these types of errors as a post-process.
#if 0
					qWarning() 
						<< "Multiple features for feature-id = "
						<< GPlatesUtils::make_qstring_from_icu_string(
								feature_ref.get()->feature_id().get() );
#endif

					return boost::none;
				}
			}

// These errors never really get fixed in the topology datasets so might as well stop spamming the log.
// Better to write a pyGPlates script to detect these types of errors as a post-process.
#if 0
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
#endif
		}

		// Return the first RG found.
		return found_rgs.front();
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


void
GPlatesAppLogic::TopologyInternalUtils::find_topological_sections_referenced(
		std::set<GPlatesModel::FeatureId> &topological_sections_referenced,
		const GPlatesModel::FeatureHandle::weak_ref &topology_feature_ref,
		boost::optional<GPlatesAppLogic::TopologyGeometry::Type> topology_geometry_type,
		boost::optional<double> reconstruction_time)
{
	PROFILE_FUNC();

	FindTopologicalSectionsReferenced visitor(topological_sections_referenced, topology_geometry_type, reconstruction_time);
	visitor.visit_feature(topology_feature_ref);
}


void
GPlatesAppLogic::TopologyInternalUtils::find_topological_sections_referenced(
		std::set<GPlatesModel::FeatureId> &topological_sections_referenced,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &topology_feature_collection_ref,
		boost::optional<GPlatesAppLogic::TopologyGeometry::Type> topology_geometry_type,
		boost::optional<double> reconstruction_time)
{
	PROFILE_FUNC();

	FindTopologicalSectionsReferenced visitor(topological_sections_referenced, topology_geometry_type, reconstruction_time);
	AppLogicUtils::visit_feature_collection(topology_feature_collection_ref, visitor);
}


void
GPlatesAppLogic::TopologyInternalUtils::find_topological_sections_referenced(
		std::set<GPlatesModel::FeatureId> &topological_sections_referenced,
		const std::vector<GPlatesModel::FeatureHandle::weak_ref> &topology_features,
		boost::optional<TopologyGeometry::Type> topology_geometry_type,
		boost::optional<double> reconstruction_time)
{
	PROFILE_FUNC();

	FindTopologicalSectionsReferenced visitor(topological_sections_referenced, topology_geometry_type, reconstruction_time);
	AppLogicUtils::visit_features(topology_features.begin(), topology_features.end(), visitor);
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
	// However there are situations where this can happen such as loading two different
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

	// If there are no features with the delegate feature id...
	if (resolved_features.empty())
	{
// These errors never really get fixed in the topology datasets so might as well stop spamming the log.
// Better to write a pyGPlates script to detect these types of errors as a post-process.
#if 0
		qWarning() 
			<< "Missing feature for feature-id = "
			<< GPlatesUtils::make_qstring_from_icu_string(geometry_delegate.get_feature_id().get());
#endif

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


bool
GPlatesAppLogic::TopologyInternalUtils::can_use_as_resolved_line_topological_section(
		const ReconstructionGeometry::non_null_ptr_to_const_type &recon_geom)
{
	// We only return true if the reconstruction geometry is a reconstructed feature geometry.
	boost::optional<const ReconstructedFeatureGeometry *> rfg =
			ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
					const ReconstructedFeatureGeometry *>(recon_geom);
	if (rfg)
	{
		// Filter out reconstructed geometries that have been reconstructed using topological boundaries/networks.
		//
		// These reconstructed geometries cannot supply topological sections because they were
		// reconstructed using topological boundaries/networks thus creating a cyclic dependency
		// (so the layer system excludes those reconstruct layers that reconstruct using topologies).
		//
		// Note that this still does not prevent the user from building a topology using an RFG and then
		// subsequently connecting that RFG's layer to a topology layer (thus turning it into a DFG).
		// In this situation the RFG would disappear from the topology's boundary (or interior) as soon as
		// its layer was connected a topology layer.
		// In this case it'll be up to the topology builder user to not use reconstructed geometries,
		// that have been reconstructed using topological boundaries/networks, as topological sections.
		boost::optional<const TopologyReconstructedFeatureGeometry *> trfg =
				ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
						const TopologyReconstructedFeatureGeometry *>(rfg.get());
		if (trfg)
		{
			return false;
		}

		return true;
	}

	return false;
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

	// See if RTL.
	boost::optional<const ResolvedTopologicalLine *> resolved_topological_line =
			ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
					const ResolvedTopologicalLine *>(recon_geom);
	if (resolved_topological_line)
	{
		// NOTE: We don't need to check that the resolved line was not formed from
		// RFGs that were deformed - see 'can_use_as_resolved_line_topological_section()' -
		// because the layer system prevents layers containing deformed RFGs from being searched
		// for topological sections (hence the resolved lines won't find them).
		return true;
	}

	return false;
}


bool
GPlatesAppLogic::TopologyInternalUtils::can_use_as_resolved_network_topological_section(
		const ReconstructionGeometry::non_null_ptr_to_const_type &recon_geom)
{
	// Return true if the reconstruction geometry is a reconstructed feature geometry,
	// but not reconstructed using topologies, and is reconstructed by plate ID or half-stage rotation.
	boost::optional<const ReconstructedFeatureGeometry *> rfg =
			ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
					const ReconstructedFeatureGeometry *>(recon_geom);
	if (rfg)
	{
		// Filter out reconstructed geometries that have not been reconstructed by plate ID or half-stage rotation.
		// These are the only supported types inside the deforming network code (in Delaunay vertices).
		if (rfg.get()->get_reconstruct_method_type() != ReconstructMethod::BY_PLATE_ID &&
			rfg.get()->get_reconstruct_method_type() != ReconstructMethod::HALF_STAGE_ROTATION)
		{
			return false;
		}

		// Filter out reconstructed geometries that have been reconstructed using topological boundaries/networks.
		//
		// See 'can_use_as_resolved_line_topological_section()' for details.
		boost::optional<const TopologyReconstructedFeatureGeometry *> trfg =
				ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
						const TopologyReconstructedFeatureGeometry *>(rfg.get());
		if (trfg)
		{
			return false;
		}

		return true;
	}

	// See if RTL.
	boost::optional<const ResolvedTopologicalLine *> resolved_topological_line =
			ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
					const ResolvedTopologicalLine *>(recon_geom);
	if (resolved_topological_line)
	{
		// Iterate over the sub-segments of the resolved line and make sure that each one is an RFG
		// that was reconstructed by plate ID or half-stage rotation.
		const sub_segment_seq_type &sub_segments = resolved_topological_line.get()->get_sub_segment_sequence();
		sub_segment_seq_type::const_iterator sub_segments_iter = sub_segments.begin();
		const sub_segment_seq_type::const_iterator sub_segments_end = sub_segments.end();
		for ( ; sub_segments_iter != sub_segments_end; ++sub_segments_iter)
		{
			const ResolvedTopologicalGeometrySubSegment::non_null_ptr_type &sub_segment = *sub_segments_iter;

			boost::optional<const ReconstructedFeatureGeometry *> sub_segment_rfg =
					ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
							const ReconstructedFeatureGeometry *>(sub_segment->get_reconstruction_geometry());
			if (!sub_segment_rfg)
			{
				return false;
			}

			// Filter out reconstructed geometries that have not been reconstructed by plate ID or half-stage rotation.
			// These are the only supported types inside the deforming network code (in Delaunay vertices).
			if (sub_segment_rfg.get()->get_reconstruct_method_type() != ReconstructMethod::BY_PLATE_ID &&
				sub_segment_rfg.get()->get_reconstruct_method_type() != ReconstructMethod::HALF_STAGE_ROTATION)
			{
				return false;
			}

			// NOTE: We don't need to check that the resolved line was not formed from
			// RFGs that were deformed - see 'can_use_as_resolved_line_topological_section()' -
			// because the layer system prevents layers containing deformed RFGs from being searched
			// for topological sections (hence the resolved lines won't find them).
		}

		return true;
	}

	return false;
}
