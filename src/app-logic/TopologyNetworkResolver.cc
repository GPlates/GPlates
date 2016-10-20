/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date: 2008-08-15 02:13:48 -0700 (Fri, 15 Aug 2008) $
 * 
 * Copyright (C) 2008, 2010 The University of Sydney, Australia
 * Copyright (C) 2008, 2009, 2013 California Institute of Technology 
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

#include <cstddef> // For std::size_t
#include <map>
#include <vector>
#include <boost/foreach.hpp>
#include <QDebug>

#include "TopologyNetworkResolver.h"

#if defined (CGAL_MACOS_COMPILER_WORKAROUND)
#	ifdef NDEBUG
#		define HAVE_NDEBUG
#		undef NDEBUG
#	endif
#endif

#include "ResolvedTriangulationNetwork.h"

#if defined (CGAL_MACOS_COMPILER_WORKAROUND)
#	ifdef HAVE_NDEBUG
#		define NDEBUG
#	endif
#endif

#include "GeometryUtils.h"
#include "ReconstructedFeatureGeometry.h"
#include "Reconstruction.h"
#include "ReconstructionGeometryUtils.h"
#include "ResolvedTopologicalLine.h"
#include "ResolvedTopologicalNetwork.h"
#include "TopologyInternalUtils.h"
#include "TopologyUtils.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "global/AssertionFailureException.h"
#include "global/CompilerWarnings.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "maths/GeometryOnSphere.h"

#include "model/FeatureHandleWeakRefBackInserter.h"

#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlPiecewiseAggregation.h"
#include "property-values/GpmlTopologicalLineSection.h"
#include "property-values/GpmlTopologicalNetwork.h"
#include "property-values/GpmlTopologicalPoint.h"
#include "property-values/GpmlTopologicalPolygon.h"
#include "property-values/GpmlTopologicalSection.h"
#include "property-values/XsBoolean.h"
#include "property-values/XsDouble.h"
#include "property-values/XsString.h"

#include "utils/GeometryCreationUtils.h"
#include "utils/Profile.h"
#include "utils/UnicodeStringUtils.h"


GPlatesAppLogic::TopologyNetworkResolver::TopologyNetworkResolver(
		std::vector<ResolvedTopologicalNetwork::non_null_ptr_type> &resolved_topological_networks,
		const double &reconstruction_time,
		ReconstructHandle::type reconstruct_handle,
		boost::optional<const std::vector<ReconstructHandle::type> &> topological_geometry_reconstruct_handles,
		const TopologyNetworkParams &topology_network_params,
		boost::optional<std::set<GPlatesModel::FeatureId> &> topological_sections_referenced) :
	d_resolved_topological_networks(resolved_topological_networks),
	d_reconstruction_time(reconstruction_time),
	d_reconstruct_handle(reconstruct_handle),
	d_topological_geometry_reconstruct_handles(topological_geometry_reconstruct_handles),
	d_topology_network_params(topology_network_params),
	d_topological_sections_referenced(topological_sections_referenced)
{  
}

bool
GPlatesAppLogic::TopologyNetworkResolver::initialise_pre_feature_properties(
		GPlatesModel::FeatureHandle &feature_handle)
{
	// NOTE: We don't test for topological network feature types anymore.
	// If a feature has a topological *network* property then it will
	// get resolved, otherwise no reconstruction geometries will be generated.
	// We're not testing feature type because that enables us to introduce a new feature type
	// that has a topological network property without requiring us to add the new feature type
	// to the list of feature types we would need to check.

	// Keep track of the feature we're visiting - used for debug/error messages.
	d_currently_visited_feature = feature_handle.reference();

	// Collect some reconstruction properties from the feature such as reconstruction
	// plate ID and time of appearance/disappearance.
	d_reconstruction_params.visit_feature(d_currently_visited_feature);

	// If the feature is not defined at the reconstruction time then don't visit the properties.
	if ( ! d_reconstruction_params.is_feature_defined_at_recon_time(d_reconstruction_time))
	{
		return false;
	}

	
#if 0 // Not currently using being used...

	// visit a few specific props
	//
	{
		static const GPlatesModel::PropertyName property_name =
			GPlatesModel::PropertyName::create_gpml("networkShapeFactor");
		boost::optional<GPlatesPropertyValues::XsDouble::non_null_ptr_to_const_type> property_value =
				GPlatesFeatureVisitors::get_property_value<GPlatesPropertyValues::XsDouble>(
						feature_handle.reference(),
						property_name);
		if (!property_value)
		{
			d_shape_factor = 0.125;
		}
		else
		{
			d_shape_factor = property_value.get()->value();
		}
	}
	// 
	{
		static const GPlatesModel::PropertyName property_name =
			GPlatesModel::PropertyName::create_gpml("networkMaxEdge");
		boost::optional<GPlatesPropertyValues::XsDouble::non_null_ptr_to_const_type> property_value =
				GPlatesFeatureVisitors::get_property_value<GPlatesPropertyValues::XsDouble>(
						feature_handle.reference(),
						property_name);
		if (!property_value)
		{
			d_max_edge = 300000000.0;
		}
		else
		{
			d_max_edge = property_value.get()->value();
// FIXME: this is to account for older files with values like 3.0 
			if (d_max_edge < 10000)
			{
				d_max_edge *= 100000000;
			}
		}
	}

#endif // ...not currently using being used.


	// Now visit each of the properties in turn.
	return true;
}


void
GPlatesAppLogic::TopologyNetworkResolver::visit_gpml_constant_value(
		GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	gpml_constant_value.value()->accept_visitor(*this);
}


void
GPlatesAppLogic::TopologyNetworkResolver::visit_gpml_piecewise_aggregation(
		GPlatesPropertyValues::GpmlPiecewiseAggregation &gpml_piecewise_aggregation)
{
	std::vector<GPlatesPropertyValues::GpmlTimeWindow> &time_windows = gpml_piecewise_aggregation.time_windows();

	// NOTE: If there's only one tine window then we do not check its time period against the
	// current reconstruction time.
	// This is because GPML files created with old versions of GPlates set the time period,
	// of the sole time window, to match that of the 'feature's time period (in the topology
	// build/edit tools) - newer versions set it to *all* time (distant past/future) - in fact
	// newer versions just use a GpmlConstantValue instead of GpmlPiecewiseAggregation because
	// the topology tools cannot yet create time-dependent topology (section) lists.
	// With old versions if the user expanded the 'feature's time period *after* building/editing
	// the topology then the *un-adjusted* time window time period will be incorrect and hence
	// we need to ignore it here.
	// Those old versions were around 4 years ago (prior to GPlates 1.3) - so we really shouldn't
	// be seeing any old topologies.
	// Actually I can see there are some currently in the sample data for GPlates 2.0.
	// So as a compromise we'll ignore the reconstruction time if there's only one time window
	// (a single time window shouldn't really have any time constraints on it anyway)
	// and respect the reconstruction time if there's more than one time window
	// (since multiple time windows need non-overlapping time constraints).
	// This is especially true now that pyGPlates will soon be able to generate time-dependent
	// topologies (where the reconstruction time will need to be respected otherwise multiple
	// networks from different time periods will get created instead of just one of them).
	if (time_windows.size() == 1)
	{
		visit_gpml_time_window(time_windows.front());
		return;
	}

	BOOST_FOREACH(GPlatesPropertyValues::GpmlTimeWindow &time_window, time_windows)
	{
		// If the time window period contains the current reconstruction time then visit.
		// The time periods should be mutually exclusive - if we happen to be in
		// two time periods then we're probably right on the boundary between the two
		// in which case we'll only visit the first time window encountered.
		if (time_window.valid_time()->contains(d_reconstruction_time))
		{
			visit_gpml_time_window(time_window);
			return;
		}
	}
}


void
GPlatesAppLogic::TopologyNetworkResolver::visit_gpml_time_window(
		GPlatesPropertyValues::GpmlTimeWindow &gpml_time_window)
{
	gpml_time_window.time_dependent_value()->accept_visitor(*this);
}

void
GPlatesAppLogic::TopologyNetworkResolver::visit_gpml_topological_network(
		GPlatesPropertyValues::GpmlTopologicalNetwork &gpml_topological_network)
{
	PROFILE_FUNC();

	// Prepare for a new topological network.
	d_resolved_network.reset();

	//
	// Visit the topological boundary sections and topological interiors to gather needed
	// information and store it internally in 'd_resolved_network'.
	//
	record_topological_boundary_sections(gpml_topological_network);
	record_topological_interior_geometries(gpml_topological_network);

	//
	// Now iterate over our internal structure 'd_resolved_network' and
	// intersect neighbouring sections that require it and
	// generate the resolved boundary subsegments.
	//
	process_topological_boundary_section_intersections();

	//
	// Now iterate over the intersection results and assign boundary segments to
	// each section.
	//
	assign_boundary_segments();

	//
	// Now create the resolved topological network.
	//
	create_resolved_topology_network();
}


void
GPlatesAppLogic::TopologyNetworkResolver::record_topological_boundary_sections(
		GPlatesPropertyValues::GpmlTopologicalNetwork &gpml_topological_network)
{
	// Loop over all the boundary sections.
	GPlatesPropertyValues::GpmlTopologicalNetwork::boundary_sections_const_iterator iter =
			gpml_topological_network.boundary_sections_begin();
	GPlatesPropertyValues::GpmlTopologicalNetwork::boundary_sections_const_iterator end =
			gpml_topological_network.boundary_sections_end();
	for ( ; iter != end; ++iter)
	{
		const GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type &
				topological_section = *iter;

		topological_section->accept_visitor(*this);
	}
}


void
GPlatesAppLogic::TopologyNetworkResolver::visit_gpml_topological_line_section(
		GPlatesPropertyValues::GpmlTopologicalLineSection &gpml_toplogical_line_section)
{  
	// Get the reconstruction geometry referenced by the topological line property delegate.
	boost::optional<ReconstructionGeometry::non_null_ptr_type> topological_reconstruction_geometry =
			find_topological_reconstruction_geometry(
					*gpml_toplogical_line_section.get_source_geometry());
	if (!topological_reconstruction_geometry)
	{
		// If no RG was found then it's possible that the current reconstruction time is
		// outside the age range of the feature this section is referencing.
		// This is ok - it's not necessarily an error - we just won't add it to the list.
		// This means either:
		//  - rubber banding will occur between the two sections adjacent to this section
		//    since this section is now missing, or
		//  - one of the adjacent sections did not exist until just now (because of its age range)
		//    and now it is popping in to replace the current section which is disappearing (an
		//    example of this is a bunch of sections that are mid-ocean ridge features that do not
		//    overlap in time and represent different geometries, from isochrons, of the same ridge).
		return;
	}

	boost::optional<ResolvedNetwork::BoundarySection> boundary_section =
			record_topological_boundary_section_reconstructed_geometry(
					gpml_toplogical_line_section.get_source_geometry()->feature_id(),
					topological_reconstruction_geometry.get());
	if (!boundary_section)
	{
		// Return without adding topological section to the list of boundary sections.
		return;
	}

	// Set reverse flag.
	boundary_section->d_use_reverse = gpml_toplogical_line_section.get_reverse_order();

	// Add to boundary section sequence.
	// NOTE: Topological sections only exist for the network *boundary*.
	// The interior geometries are not topological sections.
	d_resolved_network.d_boundary_sections.push_back(boundary_section.get());
}


void
GPlatesAppLogic::TopologyNetworkResolver::visit_gpml_topological_point(
		GPlatesPropertyValues::GpmlTopologicalPoint &gpml_toplogical_point)
{  
	// Get the reconstruction geometry referenced by the topological point property delegate.
	boost::optional<ReconstructionGeometry::non_null_ptr_type> topological_reconstruction_geometry =
			find_topological_reconstruction_geometry(
					*gpml_toplogical_point.get_source_geometry());
	if (!topological_reconstruction_geometry)
	{
		// If no RG was found then it's possible that the current reconstruction time is
		// outside the age range of the feature this section is referencing.
		// This is ok - it's not necessarily an error - we just won't add it to the list.
		// This means either:
		//  - rubber banding will occur between the two sections adjacent to this section
		//    since this section is now missing, or
		//  - one of the adjacent sections did not exist until just now (because of its age range)
		//    and now it is popping in to replace the current section which is disappearing (an
		//    example of this is a bunch of sections that are mid-ocean ridge features that do not
		//    overlap in time and represent different geometries, from isochrons, of the same ridge).
		return;
	}

	// If the topological point is a deprecated seed feature then ignore it.
	// These features were previously used to detect rigid blocks since they were placed anywhere
	// inside a polygon interior geometry and a point-in-polygon test used.
	// Now all interior geometries that are polygons are automatically rigid blocks
	// (all points in the polygon have the same plate ID and hence it can only be a rigid block).
	// Previously the seed point was also used by CGAL to avoid Delaunay mesh refinement inside
	// these rigid blocks, but we no longer use mesh refinement. If mesh refinement gets added back
	// later then we should use another means of generating the seed point rather than requiring
	// the user to do it via the topology tools (perhaps by algorithmically finding an arbitrary
	// point inside the rigid block polygon).
	if (is_deprecated_seed_geometry(topological_reconstruction_geometry.get()))
	{
		// Ignore it - it's not an actual interior geometry that contribute to the Delaunay triangulation.
		return;
	}
	// ...else topological point is not a deprecated seed geometry.

	boost::optional<ResolvedNetwork::BoundarySection> boundary_section =
			record_topological_boundary_section_reconstructed_geometry(
					gpml_toplogical_point.get_source_geometry()->feature_id(),
					topological_reconstruction_geometry.get());
	if (!boundary_section)
	{
		// Return without adding topological section to the list of boundary sections.
		return;
	}

	// No other information to collect since this topological section is a point and
	// hence cannot intersect with neighbouring sections.

	// Add to boundary section sequence.
	// NOTE: Topological sections only exist for the network *boundary*.
	// The interior geometries are not topological sections.
	d_resolved_network.d_boundary_sections.push_back(boundary_section.get());
}


void
GPlatesAppLogic::TopologyNetworkResolver::record_topological_interior_geometries(
		GPlatesPropertyValues::GpmlTopologicalNetwork &gpml_topological_network)
{
	// Loop over all the interior geometries.
	GPlatesPropertyValues::GpmlTopologicalNetwork::interior_geometries_const_iterator iter =
			gpml_topological_network.interior_geometries_begin();
	GPlatesPropertyValues::GpmlTopologicalNetwork::interior_geometries_const_iterator end =
			gpml_topological_network.interior_geometries_end();
	for ( ; iter != end; ++iter)
	{
		record_topological_interior_geometry(*iter);
	}
}


void
GPlatesAppLogic::TopologyNetworkResolver::record_topological_interior_geometry(
		const GPlatesPropertyValues::GpmlTopologicalNetwork::Interior &gpml_topological_interior)
{
	// Get the reconstruction geometry referenced by the topological interior property delegate.
	boost::optional<ReconstructionGeometry::non_null_ptr_type> topological_reconstruction_geometry =
			find_topological_reconstruction_geometry(
					*gpml_topological_interior.get_source_geometry());
	if (!topological_reconstruction_geometry)
	{
		// If no RG was found then it's possible that the current reconstruction time is
		// outside the age range of the feature this section is referencing.
		// This is ok - it's not necessarily an error - we just won't add it to the list.
		return;
	}

	// If the topological point is a deprecated seed feature then ignore it.
	// These features were previously used to detect rigid blocks since they were placed anywhere
	// inside a polygon interior geometry and a point-in-polygon test used.
	// Now all interior geometries that are polygons are automatically rigid blocks
	// (all points in the polygon have the same plate ID and hence it can only be a rigid block).
	// Previously the seed point was also used by CGAL to avoid Delaunay mesh refinement inside
	// these rigid blocks, but we no longer use mesh refinement. If mesh refinement gets added back
	// later then we should use another means of generating the seed point rather than requiring
	// the user to do it via the topology tools (perhaps by algorithmically finding an arbitrary
	// point inside the rigid block polygon).
	if (is_deprecated_seed_geometry(topological_reconstruction_geometry.get()))
	{
		// Ignore it - it's not an actual interior geometry that contribute to the Delaunay triangulation.
		return;
	}
	// ...else topological point is not a deprecated seed geometry.

	boost::optional<ResolvedNetwork::InteriorGeometry> interior_geometry =
			record_topological_interior_reconstructed_geometry(
					gpml_topological_interior.get_source_geometry()->feature_id(),
					topological_reconstruction_geometry.get());
	if (!interior_geometry)
	{
		// Return without adding topological interior to the list of interior geometries.
		return;
	}

	// Add to interior geometries sequence.
	// NOTE: The interior geometries are not topological sections because
	// they don't intersect with each other.
	d_resolved_network.d_interior_geometries.push_back(interior_geometry.get());
}


//////////////////////////////////////////////////


boost::optional<GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_type>
GPlatesAppLogic::TopologyNetworkResolver::find_topological_reconstruction_geometry(
		const GPlatesPropertyValues::GpmlPropertyDelegate &geometry_delegate)
{
	// If caller has requested the referenced topological sections.
	// Note that we add a topological section feature ID even if we cannot find
	// any topological section features that have that feature ID.
	if (d_topological_sections_referenced)
	{
		d_topological_sections_referenced->insert(geometry_delegate.feature_id());
	}

	// Get the reconstructed geometry of the geometry property delegate.
	// The referenced RGs must be in our sequence of reconstructed/resolved topological geometries.
	// If we need to restrict the topological RGs to specific reconstruct handles...
	boost::optional<const std::vector<ReconstructHandle::type> &> topological_geometry_reconstruct_handles;
	if (d_topological_geometry_reconstruct_handles)
	{
		topological_geometry_reconstruct_handles = d_topological_geometry_reconstruct_handles.get();
	}

	// Find the topological RG.
	return TopologyInternalUtils::find_topological_reconstruction_geometry(
			geometry_delegate,
			d_reconstruction_time,
			topological_geometry_reconstruct_handles);
}


bool
GPlatesAppLogic::TopologyNetworkResolver::is_deprecated_seed_geometry(
		const ReconstructionGeometry::non_null_ptr_type &reconstruction_geometry)
{
	//
	// See if the referenced feature geometry is a deprecated seed geometry.
	//
	// These seed features were previously used to detect rigid blocks since they were placed anywhere
	// inside a polygon interior geometry and a point-in-polygon test used.
	// Now all interior geometries that are polygons are automatically rigid blocks
	// (all points in the polygon have the same plate ID and hence it can only be a rigid block).
	//

	boost::optional<GPlatesModel::FeatureHandle::weak_ref> feature_ref =
			ReconstructionGeometryUtils::get_feature_ref(reconstruction_geometry);
	if (!feature_ref)
	{
		// Not a seed geometry.
		return false;
	}

    // Test for seed point feature type.
	static const GPlatesModel::FeatureType POLYGON_CENTROID_FEATURE_TYPE =
			GPlatesModel::FeatureType::create_gpml("PolygonCentroidPoint");
	if (feature_ref.get()->feature_type() != POLYGON_CENTROID_FEATURE_TYPE)
	{
		// Not a seed geometry.
		return false;
	}

	// See if it's a reconstructed feature geometry (or any of its derived types).
	boost::optional<ReconstructedFeatureGeometry *> rfg =
			ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
					ReconstructedFeatureGeometry *>(reconstruction_geometry);
	if (!rfg)
	{
		// Not a seed geometry.
		return false;
	}

	// This is a seed point, not a point in the triangulation.
	return true;
}


boost::optional<GPlatesAppLogic::TopologyNetworkResolver::ResolvedNetwork::BoundarySection>
GPlatesAppLogic::TopologyNetworkResolver::record_topological_boundary_section_reconstructed_geometry(
		const GPlatesModel::FeatureId &boundary_section_source_feature_id,
		const ReconstructionGeometry::non_null_ptr_type &boundary_section_source_rg)
{
	//
	// Currently, topological sections can only be reconstructed feature geometries
	// and resolved topological *lines* for resolved network boundaries.
	//

	// See if topological boundary section is a reconstructed feature geometry (or any of its derived types).
	boost::optional<ReconstructedFeatureGeometry *> boundary_section_source_rfg =
			ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
					ReconstructedFeatureGeometry *>(boundary_section_source_rg);
	if (boundary_section_source_rfg)
	{
		// Store the feature id and reconstruction geometry.
		return ResolvedNetwork::BoundarySection(
				boundary_section_source_feature_id,
				boundary_section_source_rfg.get(),
				boundary_section_source_rfg.get()->reconstructed_geometry());
	}

	// See if topological section is a resolved topological line.
	boost::optional<ResolvedTopologicalLine *> boundary_section_source_rtl =
			ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
					ResolvedTopologicalLine *>(boundary_section_source_rg);
	if (boundary_section_source_rtl)
	{
		// Store the feature id and reconstruction geometry.
		return ResolvedNetwork::BoundarySection(
				boundary_section_source_feature_id,
				boundary_section_source_rtl.get(),
				boundary_section_source_rtl.get()->resolved_topology_line());
	}

	// If we got here then either (1) the user created a malformed GPML file somehow (eg, with a script)
	// or (2) it's a program error (because the topology build/edit tools should only currently allow
	// the user to add topological sections that are reconstructed static geometries
	// (or resolved topological *lines* when resolving network boundaries).
	// We'll assume (1) and emit an error message rather than asserting/aborting.
	qWarning() << "Ignoring topological section, for resolved network boundary, that is not a "
			"regular feature or topological *line*.";

	return boost::none;
}


boost::optional<GPlatesAppLogic::TopologyNetworkResolver::ResolvedNetwork::InteriorGeometry>
GPlatesAppLogic::TopologyNetworkResolver::record_topological_interior_reconstructed_geometry(
		const GPlatesModel::FeatureId &interior_source_feature_id,
		const ReconstructionGeometry::non_null_ptr_type &interior_source_rg)
{
	//
	// Currently, interior geometries can only be reconstructed feature geometries
	// and resolved topological *lines* for resolved network boundaries.
	//

	// See if topological interior is a reconstructed feature geometry (or any of its derived types).
	boost::optional<ReconstructedFeatureGeometry *> interior_source_rfg =
			ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
					ReconstructedFeatureGeometry *>(interior_source_rg);
	if (interior_source_rfg)
	{
		// Store the feature id and reconstruction geometry.
		return ResolvedNetwork::InteriorGeometry(
				interior_source_feature_id,
				interior_source_rfg.get(),
				interior_source_rfg.get()->reconstructed_geometry());
	}

	// See if topological interior is a resolved topological line.
	boost::optional<ResolvedTopologicalLine *> interior_source_rtl =
			ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
					ResolvedTopologicalLine *>(interior_source_rg);
	if (interior_source_rtl)
	{
		// Store the feature id and reconstruction geometry.
		return ResolvedNetwork::InteriorGeometry(
				interior_source_feature_id,
				interior_source_rtl.get(),
				interior_source_rtl.get()->resolved_topology_line());
	}

	// If we got here then either (1) the user created a malformed GPML file somehow (eg, with a script)
	// or (2) it's a program error (because the topology build/edit tools should only currently allow
	// the user to add topological interiors that are reconstructed static geometries or
	// resolved topological *lines*.
	// We'll assume (1) and emit an error message rather than asserting/aborting.
	qWarning() << "Ignoring topological interior, for resolved network, that is not a "
			"regular feature or topological *line*.";

	return boost::none;
}

//////////
// PROCESS INTERSECTIONS
/////////

void
GPlatesAppLogic::TopologyNetworkResolver::process_topological_boundary_section_intersections()
{
	// Iterate over our internal sequence of sections that we built up by
	// visiting the topological sections of a topological polygon.
	const std::size_t num_sections = d_resolved_network.d_boundary_sections.size();

	// If there's only one section then don't try to intersect it with itself.
	if (num_sections < 2)
	{
		return;
	}

	// Special case treatment when there are exactly two sections.
	// In this case the two sections can intersect twice to form a closed polygon.
	// This is the only case where two adjacent sections are allowed to intersect twice.
	if (num_sections == 2)
	{
		// NOTE: We use index 1 instead of 0 to match similar code in the topology builder tool.
		// This makes a difference if the user builds a topology with two sections that only
		// intersect once (not something the user should be building) and means that the
		// same topology will be creating here as in the builder.
		process_topological_section_intersection_boundary(1/*section_index*/, true/*two_sections*/);
		return;
	}

	// Iterate over the sections and process intersections between each section
	// and its previous neighbour.
	for (std::size_t section_index = 0; section_index < num_sections; ++section_index)
	{
		process_topological_section_intersection_boundary(section_index);
	}
}

void
GPlatesAppLogic::TopologyNetworkResolver::process_topological_section_intersection_boundary(
		const std::size_t current_section_index,
		const bool two_sections)
{
	//
	// Intersect the current section with the previous section.
	//

	const std::size_t num_sections = d_resolved_network.d_boundary_sections.size();

	ResolvedNetwork::BoundarySection &current_section =
			d_resolved_network.d_boundary_sections[current_section_index];

	//
	// We get the start intersection geometry from previous section in the topological polygon's
	// list of sections whose valid time ranges include the current reconstruction time.
	//

	const std::size_t prev_section_index = (current_section_index == 0)
			? num_sections - 1
			: current_section_index - 1;

	ResolvedNetwork::BoundarySection &prev_section =
			d_resolved_network.d_boundary_sections[prev_section_index];

	// If both sections refer to the same geometry then don't intersect.
	// This can happen when the same geometry is added more than once to the topology
	// when it forms different parts of the plate polygon boundary - normally there
	// are other geometries in between but when building topologies it's possible to
	// add the geometry as first section, then add another geometry as second section,
	// then add the first geometry again as the third section and then add another
	// geometry as the fourth section - before the fourth section is added the
	// first and third sections are adjacent and they are the same geometry - and if
	// the topology build/edit tool creates the topology when only three sections are
	// added then we have to deal with it here in the boundary resolver.
	if (prev_section.d_source_rg.get() == current_section.d_source_rg.get())
	{
		return;
	}

	//
	// Process the actual intersection.
	//
	if (two_sections)
	{
		current_section.d_intersection_results.
				intersect_with_previous_section_allowing_two_intersections(
						prev_section.d_intersection_results);
	}
	else
	{
		current_section.d_intersection_results.intersect_with_previous_section(
				prev_section.d_intersection_results,
				prev_section.d_use_reverse);
	}

	// NOTE: We don't need to look at the end intersection because the next topological
	// section that we visit will have this current section as its start intersection and
	// hence the intersection of this current section and its next section will be
	// taken care of during that visit.
}


///////////
// ASSIGMENT of segments 
///////////

void
GPlatesAppLogic::TopologyNetworkResolver::assign_boundary_segments()
{
	// Make sure all the boundary segments have been found.
	// It is an error in the code (not in the data) if this is not the case.
	const std::size_t num_sections = d_resolved_network.d_boundary_sections.size();
	for (std::size_t section_index = 0; section_index < num_sections; ++section_index)
	{
		assign_boundary_segment_boundary(section_index);
	}
}

void
GPlatesAppLogic::TopologyNetworkResolver::assign_boundary_segment_boundary(
		const std::size_t section_index)
{
	ResolvedNetwork::BoundarySection &boundary_section =
			d_resolved_network.d_boundary_sections[section_index];

	// See if the reverse flag has been set by intersection processing - this
	// happens if the visible section intersected both its neighbours otherwise it just
	// returns the flag we passed it.
	boundary_section.d_use_reverse =
			boundary_section.d_intersection_results.get_reverse_flag(
					boundary_section.d_use_reverse);

	boundary_section.d_final_boundary_segment_unreversed_geom =
			boundary_section.d_intersection_results.get_unreversed_sub_segment(
					boundary_section.d_use_reverse);
}


// Final Creation Step
void
GPlatesAppLogic::TopologyNetworkResolver::create_resolved_topology_network()
{
	// 2D + INFO 
	// This vector holds extra info to pass to the delaunay triangulation.
	std::vector<ResolvedTriangulation::Network::DelaunayPoint> delaunay_points;

	// All the points on the boundary of the delaunay_triangulation_2_type
	std::vector<GPlatesMaths::PointOnSphere> boundary_points;

	// Sequence of boundary subsegments of resolved topology boundary.
	std::vector<ResolvedTopologicalGeometrySubSegment> boundary_subsegments;

	// A rigid block is any interior geometry that is a polygon.
	// These are polygon regions that are inside the network but are meant to be rigid and not deforming.
	// All points in the polygon will have the same plate ID so they can only really be rigid.
	std::vector<ResolvedTriangulation::Network::RigidBlock> rigid_blocks;

#if 0 // Not currently using being used...
	// 2D + constraints
	// This vector holds the geometry constraints to pass to the constrained delaunay triangulation.
	std::vector<ResolvedTriangulation::Network::ConstrainedDelaunayGeometry> constrained_delaunay_geometries;

	// Points from multple single point sections 
	std::vector<GPlatesMaths::PointOnSphere> scattered_points;
#endif // ...not currently using being used.


	// Iterate over the sections of the resolved boundary and construct
	// the resolved polygon boundary and its subsegments.
	ResolvedNetwork::boundary_section_seq_type::const_iterator boundary_sections_iter =
			d_resolved_network.d_boundary_sections.begin();
	ResolvedNetwork::boundary_section_seq_type::const_iterator boundary_sections_end =
			d_resolved_network.d_boundary_sections.end();
	for ( ; boundary_sections_iter != boundary_sections_end; ++boundary_sections_iter)
	{
		const ResolvedNetwork::BoundarySection &boundary_section = *boundary_sections_iter;

		// It's possible for a valid segment to not contribute to the boundary
		// of the network. This can happen if it contributes zero-length
		// to the network boundary which happens when both its neighbouring
		// boundary sections intersect it at the same point.
		if (!boundary_section.d_final_boundary_segment_unreversed_geom)
		{
			continue; // to next section in topology network
		}

		// Get the subsegment feature reference.
		boost::optional<GPlatesModel::FeatureHandle::weak_ref> boundary_subsegment_feature_ref =
				ReconstructionGeometryUtils::get_feature_ref(boundary_section.d_source_rg);
		// If the feature reference is invalid then skip the current section.
		if (!boundary_subsegment_feature_ref)
		{
			continue;
		}
		const GPlatesModel::FeatureHandle::const_weak_ref boundary_subsegment_feature_const_ref(
				boundary_subsegment_feature_ref.get());

		//
		// A network boundary section can come from a reconstructed feature geometry or a resolved topological *line*.
		//

		boost::optional<ReconstructedFeatureGeometry *> boundary_section_rfg =
				ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
						ReconstructedFeatureGeometry *>(boundary_section.d_source_rg);
		if (boundary_section_rfg)
		{
			// Add the boundary delaunay points from the reconstructed feature geometry.
			add_boundary_delaunay_points_from_reconstructed_feature_geometry(
					boundary_section_rfg.get(),
					*boundary_section.d_final_boundary_segment_unreversed_geom.get(),
					delaunay_points);
		}
		else // resolved topological *line* ...
		{
			boost::optional<ResolvedTopologicalLine *> boundary_section_rtl =
					ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
							ResolvedTopologicalLine *>(boundary_section.d_source_rg);

			// Skip the current boundary section if it's not a resolved topological *line*.
			if (!boundary_section_rtl)
			{
				continue;
			}

			// Add the boundary delaunay points from the resolved topological *line*.
			add_boundary_delaunay_points_from_resolved_topological_line(
					boundary_section_rtl.get(),
					*boundary_section.d_final_boundary_segment_unreversed_geom.get(),
					delaunay_points);
		}

		// Add the boundary polygon (constrained) points.
		add_boundary_points(boundary_section, boundary_points);

		// Create a subsegment structure that'll get used when
		// creating the boundary of the resolved topological network.
		const ResolvedTopologicalGeometrySubSegment boundary_subsegment(
				boundary_section.d_final_boundary_segment_unreversed_geom.get(),
				boundary_section.d_source_rg,
				boundary_subsegment_feature_const_ref,
				boundary_section.d_use_reverse);
		boundary_subsegments.push_back(boundary_subsegment);
	}

	// Create a polygon on sphere for the resolved boundary using 'boundary_points'.
	GPlatesUtils::GeometryConstruction::GeometryConstructionValidity boundary_polygon_validity;
	boost::optional<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> boundary_polygon =
			GPlatesUtils::create_polygon_on_sphere(
					boundary_points.begin(), boundary_points.end(), boundary_polygon_validity);

	// If we are unable to create a polygon (such as insufficient points) then
	// just return without creating a resolved topological geometry.
	if (boundary_polygon_validity != GPlatesUtils::GeometryConstruction::VALID)
	{
// These errors never really get fixed in the topology datasets so might as well stop spamming the log.
// Better to write a pyGPlates script to detect these types of errors as a post-process.
#if 0
		qDebug() << "ERROR: Failed to create a polygon boundary for a ResolvedTopologicalNetwork - "
				"probably has insufficient points for a polygon.";
		qDebug() << "Skipping creation for topological network feature_id=";
		qDebug() << GPlatesUtils::make_qstring_from_icu_string(
				d_currently_visited_feature->feature_id().get());
#endif

		return;
	}


#if 0 // Not currently using being used...
	// 2D + C
	// add boundary_points as contrained ; do contrain begin and end
	constrained_delaunay_geometries.push_back(
			ResolvedTriangulation::Network::ConstrainedDelaunayGeometry(
					boundary_polygon.get(),
					true)); // do constrain begin and end points
#endif // ...not currently using being used.


	// Iterate over the interior geometries.
	ResolvedNetwork::interior_geometry_seq_type::const_iterator interior_geometry_iter =
			d_resolved_network.d_interior_geometries.begin();
	ResolvedNetwork::interior_geometry_seq_type::const_iterator interior_geometry_end =
			d_resolved_network.d_interior_geometries.end();
	for ( ; interior_geometry_iter != interior_geometry_end; ++interior_geometry_iter)
	{
		const ResolvedNetwork::InteriorGeometry &interior_geometry = *interior_geometry_iter;

		// If the interior feature reference is invalid then skip the current interior geometry.
		if (!ReconstructionGeometryUtils::get_feature_ref(interior_geometry.d_source_rg))
		{
			continue;
		}

		//
		// Network interiors can come from a reconstructed feature geometry or a resolved topological *line*.
		//

		boost::optional<ReconstructedFeatureGeometry *> interior_rfg =
				ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
						ReconstructedFeatureGeometry *>(interior_geometry.d_source_rg);
		if (interior_rfg)
		{
			// Add the interior delaunay points from the reconstructed feature geometry.
			add_interior_delaunay_points_from_reconstructed_feature_geometry(
					interior_rfg.get(),
					*interior_geometry.d_geometry,
					delaunay_points,
					rigid_blocks);
		}
		else // resolved topological *line* ...
		{
			boost::optional<ResolvedTopologicalLine *> interior_rtl =
					ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
							ResolvedTopologicalLine *>(interior_geometry.d_source_rg);

			// Skip the current interior geometry if it's not a resolved topological *line*.
			if (!interior_rtl)
			{
				continue;
			}

			// Add the interior delaunay points from the resolved topological *line*.
			add_interior_delaunay_points_from_resolved_topological_line(
					interior_rtl.get(),
					delaunay_points);
		}

#if 0 // Not currently using being used...
		// Add the interior constrained delaunay points.
		add_interior_constrained_delaunay_points(
				interior_geometry,
				constrained_delaunay_geometries,
				scattered_points);
#endif // ...not currently using being used.
	}

	// Now that we've gathered all the triangulation information we can create the triangulation network.
	ResolvedTriangulation::Network::non_null_ptr_type triangulation_network =
			ResolvedTriangulation::Network::create(
					d_reconstruction_time,
					boundary_polygon.get(),
					delaunay_points.begin(),
					delaunay_points.end(),
					rigid_blocks.begin(),
					rigid_blocks.end(),
					d_topology_network_params);

	// Join adjacent deforming points that are spread along a deforming zone boundary.
	//
	// This was meant to be a temporary hack to be removed when resolved *line* topologies were
	// implemented. However, unfortunately it seems we need to keep this hack in place for any
	// old data files that use the old method.
	std::vector<ResolvedTopologicalGeometrySubSegment> joined_boundary_subsegments;
	TopologyInternalUtils::join_adjacent_deforming_points(
			joined_boundary_subsegments,
			boundary_subsegments,
			d_reconstruction_time);

	// Create the network RTN 
	const ResolvedTopologicalNetwork::non_null_ptr_type network =
			ResolvedTopologicalNetwork::create(
					d_reconstruction_time,
					triangulation_network,
					*(current_top_level_propiter()->handle_weak_ref()),
					*(current_top_level_propiter()),
					joined_boundary_subsegments.begin(),
					joined_boundary_subsegments.end(),
					d_reconstruction_params.get_recon_plate_id(),
					d_reconstruction_params.get_time_of_appearance(),
					d_reconstruct_handle/*identify where/when this RTN was resolved*/);

	d_resolved_topological_networks.push_back(network);
}


void
GPlatesAppLogic::TopologyNetworkResolver::add_boundary_delaunay_points_from_reconstructed_feature_geometry(
		const ReconstructedFeatureGeometry::non_null_ptr_type &boundary_section_rfg,
		const GPlatesMaths::GeometryOnSphere &boundary_section_geometry,
		std::vector<ResolvedTriangulation::Network::DelaunayPoint> &delaunay_points)
{
	// Get the reconstruction plate id.
	GPlatesModel::integer_plate_id_type boundary_section_plate_id = 0;
	if (boundary_section_rfg->reconstruction_plate_id())
	{
		boundary_section_plate_id = boundary_section_rfg->reconstruction_plate_id().get();
	}

	// The creator of reconstruction trees used when reconstructing the boundary section.
	const ReconstructionTreeCreator boundary_section_reconstruction_tree_creator =
			boundary_section_rfg->get_reconstruction_tree_creator();

	// Get the points for this boundary section.
	// The section reversal order doesn't matter here - we're just adding independent points.
	std::vector<GPlatesMaths::PointOnSphere> boundary_section_points;
	GeometryUtils::get_geometry_exterior_points(boundary_section_geometry, boundary_section_points);

	// Create the delaunay points for this boundary section.
	std::vector<GPlatesMaths::PointOnSphere>::const_iterator boundary_section_points_iter =
			boundary_section_points.begin();
	std::vector<GPlatesMaths::PointOnSphere>::const_iterator boundary_section_points_end =
			boundary_section_points.end();
	for ( ; boundary_section_points_iter != boundary_section_points_end; ++boundary_section_points_iter)
	{
		const ResolvedTriangulation::Network::DelaunayPoint delaunay_point(
				*boundary_section_points_iter,
				boundary_section_plate_id,
				boundary_section_reconstruction_tree_creator);
		delaunay_points.push_back(delaunay_point);
	}
}


void
GPlatesAppLogic::TopologyNetworkResolver::add_boundary_delaunay_points_from_resolved_topological_line(
		const ResolvedTopologicalLine::non_null_ptr_type &boundary_section_rtl,
		const GPlatesMaths::GeometryOnSphere &boundary_section_geometry,
		std::vector<ResolvedTriangulation::Network::DelaunayPoint> &delaunay_points)
{
	//
	// FIXME: This function is very fragile and will most likely break in some cases.
	// FIXME: It needs to be done in a completely different way where we're not trying
	// FIXME: to match sub-segment points and all the assumptions about how the boundary section
	// FIXME: and its sub-segments are built (such as duplicate points at intersections, etc).
	//

	// Get the sub-segments of the boundary section so we can access their plate ids and reconstruction trees.
	const sub_segment_seq_type &sub_segments = boundary_section_rtl->get_sub_segment_sequence();

	// Get the points for the (potentially clipped) boundary section geometry.
	// Note that we do *not* take into account its reversal because we want to proceed in the same
	// order its sub-segment components (although each of those will need appropriate reversal).
	std::vector<GPlatesMaths::PointOnSphere> boundary_section_points;
	GeometryUtils::get_geometry_exterior_points(boundary_section_geometry, boundary_section_points);

	const std::vector<GPlatesMaths::PointOnSphere>::const_iterator boundary_section_points_begin =
			boundary_section_points.begin();
	const std::vector<GPlatesMaths::PointOnSphere>::const_iterator boundary_section_points_end =
			boundary_section_points.end();
	std::vector<GPlatesMaths::PointOnSphere>::const_iterator boundary_section_points_iter =
			boundary_section_points_begin;

	// Need to find first matching point between boundary section and its sub-segments.
	bool initialised_start_of_boundary_section = false;

	// Iterate over the sub-segments.
	const sub_segment_seq_type::const_iterator sub_segments_begin = sub_segments.begin();
	const sub_segment_seq_type::const_iterator sub_segments_end = sub_segments.end();
	sub_segment_seq_type::const_iterator sub_segments_iter = sub_segments_begin;
	for ( ; sub_segments_iter != sub_segments_end; ++sub_segments_iter)
	{
		const ResolvedTopologicalGeometrySubSegment &sub_segment = *sub_segments_iter;

		ReconstructionGeometry::non_null_ptr_to_const_type sub_segment_rg =
				sub_segment.get_reconstruction_geometry();

		// Get the sub-segment reconstruction plate id.
		boost::optional<GPlatesModel::integer_plate_id_type> sub_segment_plate_id =
				ReconstructionGeometryUtils::get_plate_id(sub_segment_rg);
		if (!sub_segment_plate_id)
		{
			sub_segment_plate_id = 0;
		}

		// The creator of reconstruction trees used when reconstructing the sub-segment recon geometry.
		boost::optional<ReconstructionTreeCreator> sub_segment_reconstruction_tree_creator =
				ReconstructionGeometryUtils::get_reconstruction_tree_creator(sub_segment_rg);

		// Sub-segments are either reconstructed feature geometries or resolved topological geometries
		// and both have reconstruction tree creators so this should always succeed.
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				sub_segment_reconstruction_tree_creator,
				GPLATES_ASSERTION_SOURCE);

		// Get the points for this sub-segment.
		// Note that we *do* take into account sub-segment reversal to keep the order of points
		// aligned with the (un-reversed) boundary section geometry.
		std::vector<GPlatesMaths::PointOnSphere> sub_segment_points;
		GeometryUtils::get_geometry_exterior_points(
				*sub_segment.get_geometry(),
				sub_segment_points,
				sub_segment.get_use_reverse());

		const std::vector<GPlatesMaths::PointOnSphere>::const_iterator sub_segment_points_begin =
				sub_segment_points.begin();
		const std::vector<GPlatesMaths::PointOnSphere>::const_iterator sub_segment_points_end =
				sub_segment_points.end();
		std::vector<GPlatesMaths::PointOnSphere>::const_iterator sub_segment_points_iter =
				sub_segment_points_begin;

		// Initialise the beginning of the boundary section to get things started.
		if (!initialised_start_of_boundary_section)
		{
			// The first boundary point might not match if it was clipped to an adjacent
			// network boundary section thus introducing a new (intersection) point that doesn't
			// exist in the original un-clipped boundary geometry (and its sub-segments).
			if (boundary_section_points_iter == boundary_section_points_begin &&
				*boundary_section_points_iter != *sub_segment_points_iter)
			{
				// Move to the second boundary section point.
				++boundary_section_points_iter;
				if (boundary_section_points_iter == boundary_section_points_end)
				{
					// Boundary section only contains one point but we still can't find a match.
					qWarning() << "TopologyNetworkResolver: Unexpected mis-match between a single "
						"point network boundary section and its component sub-segment.";

					// Output the single delaunay point.
					const ResolvedTriangulation::Network::DelaunayPoint delaunay_point(
							boundary_section_points.front()/*first point*/,
							sub_segment_plate_id.get(),
							sub_segment_reconstruction_tree_creator.get());
					delaunay_points.push_back(delaunay_point);

					return;
				}
			}

			// Iterate along the sub-segment until we find a point that matches the second
			// boundary section point. If we're at the first boundary section point this will
			// match the first sub-segment point (already tested above).
			while (*boundary_section_points_iter != *sub_segment_points_iter)
			{
				++sub_segment_points_iter;
				if (sub_segment_points_iter == sub_segment_points_end)
				{
					// Reached end of current sub-segment.
					break;
				}
			}

			// If we didn't find a matching point on the current sub-segment then keep looking
			// at the next sub-segment - it could be that the clipping of the boundary section
			// has essentially removed one or more of its sub-segments completely.
			if (sub_segment_points_iter == sub_segment_points_end)
			{
				continue;
			}

			// If we skipped the first boundary section point then go back and fill it in with
			// information from the current sub-segment.
			if (boundary_section_points_iter != boundary_section_points_begin)
			{
				const ResolvedTriangulation::Network::DelaunayPoint delaunay_point(
						boundary_section_points.front()/*first point*/,
						sub_segment_plate_id.get(),
						sub_segment_reconstruction_tree_creator.get());
				delaunay_points.push_back(delaunay_point);
			}

			initialised_start_of_boundary_section = true;
		}

		// For the 2nd, 3rd, etc sub-segments the sub-segment begin point matches the previous
		// sub-segment end point, but there might not be a duplicate point in the boundary section.
		if (sub_segments_iter != sub_segments_begin)
		{
			// Advance to the second sub-segment point if it doesn't match
			// (it's a duplicate of the previous sub-segment end point).
			if (*boundary_section_points_iter != *sub_segment_points_iter)
			{
				++sub_segment_points_iter;
				if (sub_segment_points_iter == sub_segment_points_end)
				{
					// There's only one point in the current sub-segment.
					// We'll stay at that one point even though it doesn't match because
					// it's possible it's the point past the end of the clipped boundary section
					// and hence it will get ignored.
					--sub_segment_points_iter;
				}
			}
		}

		// Keep adding delaunay points while we have matching points.
		while (*boundary_section_points_iter == *sub_segment_points_iter)
		{
			const ResolvedTriangulation::Network::DelaunayPoint delaunay_point(
					*boundary_section_points_iter,
					sub_segment_plate_id.get(),
					sub_segment_reconstruction_tree_creator.get());
			delaunay_points.push_back(delaunay_point);

			++boundary_section_points_iter;
			++sub_segment_points_iter;

			if (boundary_section_points_iter == boundary_section_points_end)
			{
				// Finished with all boundary section points.
				//
				// It's fine to finish the boundary section before finishing the sub-segments
				// because the boundary section is clipped (to its adjacent neighbours on the
				// network boundary) whereas the sub-segments represent the entire un-clipped
				// resolved line geometry.
				return;
			}

			if (sub_segment_points_iter == sub_segment_points_end)
			{
				// Finished with the current sub-segment.
				break;
			}
		}

		// If the entire sub-segment didn't match (and note that we can only get here if we've not
		// yet reached the end of the boundary section) then either:
		//
		//  (1) we've reached the last boundary section point and it intersects the next network
		//      boundary section neighbour and hence is a new point that doesn't exist in the
		//      un-clipped boundary geometry (consisting of its sub-segments), or
		//
		//  (2) there was an unexpected mismatch between the boundary section points and the
		//      sub-segment points, so we log a warning and return.
		//
		// For case (1) we assign the current sub-segment parameters (plate id, etc) to the last point.
		//
		// FIXME: This could be problematic for resolved topological lines made from points
		// (rather than intersecting lines) because the intersection point is not on an existing
		// geometry (its between two point geometries) and hence the plate id is essentially undefined.
		//
		if (sub_segment_points_iter != sub_segment_points_end)
		{
			// Are we at the last boundary section point (it might be an intersection point).
			if (boundary_section_points_iter == boundary_section_points_end - 1)
			{
				const ResolvedTriangulation::Network::DelaunayPoint delaunay_point(
						boundary_section_points.back()/*last point*/,
						sub_segment_plate_id.get(),
						sub_segment_reconstruction_tree_creator.get());
				delaunay_points.push_back(delaunay_point);

				return;
			}
			else
			{
				qWarning() << "TopologyNetworkResolver: Unexpected mis-match between a network "
					"boundary section and its component sub-segments - "
					"some boundary section points may have incorrect plate ids / velocities.";

				// Add the remaining boundary section points using the current sub-segment parameters.
				for ( ;
					boundary_section_points_iter != boundary_section_points_end;
					++boundary_section_points_iter)
				{
					const ResolvedTriangulation::Network::DelaunayPoint delaunay_point(
							*boundary_section_points_iter,
							sub_segment_plate_id.get(),
							sub_segment_reconstruction_tree_creator.get());
					delaunay_points.push_back(delaunay_point);
				}

				return;
			}
		}
	}

	qWarning() << "TopologyNetworkResolver: Reached end of a network boundary section's "
		"sub-segments before reaching end of the boundary section - some boundary section points "
		"not using sub-segment plate ids - probably will give incorrect velocities.";

	// Resort to using the plate id and reconstruction trees of the boundary section itself
	// rather than its sub-segments. This will probably give incorrect velocities at these points.

	// Get the boundary section plate id.
	GPlatesModel::integer_plate_id_type boundary_section_plate_id = 0;
	if (boundary_section_rtl->plate_id())
	{
		boundary_section_plate_id = boundary_section_rtl->plate_id().get();
	}

	// It's possible we didn't even find a point matching the first boundary section point.
	if (!initialised_start_of_boundary_section)
	{
		const ResolvedTriangulation::Network::DelaunayPoint delaunay_point(
				boundary_section_points.front()/*first point*/,
				boundary_section_plate_id,
				boundary_section_rtl->get_reconstruction_tree_creator());
		delaunay_points.push_back(delaunay_point);

		initialised_start_of_boundary_section = true;

		// 'boundary_section_points_iter' points at the second boundary section point.
	}

	// Add the remaining boundary section points using the current sub-segment parameters.
	for ( ; boundary_section_points_iter != boundary_section_points_end; ++boundary_section_points_iter)
	{
		const ResolvedTriangulation::Network::DelaunayPoint delaunay_point(
				*boundary_section_points_iter,
				boundary_section_plate_id,
				boundary_section_rtl->get_reconstruction_tree_creator());
		delaunay_points.push_back(delaunay_point);
	}
}


void
GPlatesAppLogic::TopologyNetworkResolver::add_boundary_points(
		const ResolvedNetwork::BoundarySection &boundary_section,
		std::vector<GPlatesMaths::PointOnSphere> &boundary_points)
{
	//
	// Determine the subsegments original geometry type 
	//
	// FIXME: Should this be the section geometry before or after intersection-clipping?
	const GPlatesMaths::GeometryType::Value section_geometry_type =
			GeometryUtils::get_geometry_type(*boundary_section.d_source_geometry);

	// Check for single point
	if (section_geometry_type == GPlatesMaths::GeometryType::POINT)
	{
		// this is probably one of a collection of points; 
		// save and add to constrained triangulation later
		GeometryUtils::get_geometry_exterior_points(
				*boundary_section.d_final_boundary_segment_unreversed_geom.get(),
				boundary_points,
				boundary_section.d_use_reverse);

		return;
	}

	// Check multipoint 
	if (section_geometry_type == GPlatesMaths::GeometryType::MULTIPOINT)
	{
		// Get the points for this geom
		GeometryUtils::get_geometry_exterior_points(
				*boundary_section.d_final_boundary_segment_unreversed_geom.get(),
				boundary_points,
				boundary_section.d_use_reverse);

		return;
	}

	// Check for polyline geometry
	if (section_geometry_type == GPlatesMaths::GeometryType::POLYLINE)
	{
		// this is a single line feature, possibly clipped by intersections
		GeometryUtils::get_geometry_exterior_points(
				*boundary_section.d_final_boundary_segment_unreversed_geom.get(),
				boundary_points,
				boundary_section.d_use_reverse);

		return;
	}

	// Check for polygon geometry
	if (section_geometry_type == GPlatesMaths::GeometryType::POLYGON)
	{
		// this is a single polygon feature
		GeometryUtils::get_geometry_exterior_points(
				*boundary_section.d_final_boundary_segment_unreversed_geom.get(),
				boundary_points,
				boundary_section.d_use_reverse);

		return;
	}

	// Shouldn't be able to get here.
	GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
}


void
GPlatesAppLogic::TopologyNetworkResolver::add_interior_delaunay_points_from_reconstructed_feature_geometry(
		const ReconstructedFeatureGeometry::non_null_ptr_type &interior_rfg,
		const GPlatesMaths::GeometryOnSphere &interior_geometry,
		std::vector<ResolvedTriangulation::Network::DelaunayPoint> &delaunay_points,
		std::vector<ResolvedTriangulation::Network::RigidBlock> &rigid_blocks)
{
	// Keep track of any interior static polygon regions - these are rigid blocks.
	// These will be needed for calculating velocities since they are not part of the triangulation
	// generated (velocities will be calculated in the normal manner for static polygons).
	//
	// NOTE: Since currently only RFGs and resolved topological *lines* can be referenced by
	// networks it's only possible to have an interior *polygon* if it's an RFG - so we don't
	// need to worry about resolved topological geometries just yet.
	// See if interior is a reconstructed feature geometry (or any of its derived types).
	//
	// If it's a polygon then add it to the list of interior rigid blocks.
	if (GeometryUtils::get_polygon_on_sphere(*interior_rfg->reconstructed_geometry()))
	{
		rigid_blocks.push_back(
				ResolvedTriangulation::Network::RigidBlock(interior_rfg));
	}

	// Get the reconstruction plate id.
	GPlatesModel::integer_plate_id_type interior_geometry_plate_id = 0;
	if (interior_rfg->reconstruction_plate_id())
	{
		interior_geometry_plate_id = interior_rfg->reconstruction_plate_id().get();
	}

	// The creator of reconstruction trees used when reconstructing the interior geometry.
	const ReconstructionTreeCreator interior_geometry_reconstruction_tree_creator =
			interior_rfg->get_reconstruction_tree_creator();

	// Get the points for this interior geometry.
	std::vector<GPlatesMaths::PointOnSphere> interior_geometry_points;
	GeometryUtils::get_geometry_exterior_points(interior_geometry, interior_geometry_points);

	// Create the delaunay points for this interior geometry.
	std::vector<GPlatesMaths::PointOnSphere>::const_iterator interior_geometry_points_iter =
			interior_geometry_points.begin();
	std::vector<GPlatesMaths::PointOnSphere>::const_iterator interior_geometry_points_end =
			interior_geometry_points.end();
	for ( ; interior_geometry_points_iter != interior_geometry_points_end; ++interior_geometry_points_iter)
	{
		const ResolvedTriangulation::Network::DelaunayPoint delaunay_point(
				*interior_geometry_points_iter,
				interior_geometry_plate_id,
				interior_geometry_reconstruction_tree_creator);
		delaunay_points.push_back(delaunay_point);
	}
}


void
GPlatesAppLogic::TopologyNetworkResolver::add_interior_delaunay_points_from_resolved_topological_line(
		const ResolvedTopologicalLine::non_null_ptr_type &interior_rtl,
		std::vector<ResolvedTriangulation::Network::DelaunayPoint> &delaunay_points)
{
	// Get the sub-segments of the resolved line so we can access their plate ids and reconstruction trees.
	const sub_segment_seq_type &resolved_line_sub_segments = interior_rtl->get_sub_segment_sequence();

	// Iterate over the sub-segments.
	sub_segment_seq_type::const_iterator sub_segments_iter = resolved_line_sub_segments.begin();
	sub_segment_seq_type::const_iterator sub_segments_end = resolved_line_sub_segments.end();
	for ( ; sub_segments_iter != sub_segments_end; ++sub_segments_iter)
	{
		const ResolvedTopologicalGeometrySubSegment &sub_segment = *sub_segments_iter;

		ReconstructionGeometry::non_null_ptr_to_const_type sub_segment_rg =
				sub_segment.get_reconstruction_geometry();

		// Get the sub-segment reconstruction plate id.
		boost::optional<GPlatesModel::integer_plate_id_type> sub_segment_plate_id =
				ReconstructionGeometryUtils::get_plate_id(sub_segment_rg);
		if (!sub_segment_plate_id)
		{
			sub_segment_plate_id = 0;
		}

		// The creator of reconstruction trees used when reconstructing the sub-segment recon geometry.
		boost::optional<ReconstructionTreeCreator> sub_segment_reconstruction_tree_creator =
			ReconstructionGeometryUtils::get_reconstruction_tree_creator(sub_segment_rg);

		// Sub-segments are either reconstructed feature geometries or resolved topological geometries
		// and both have reconstruction tree creators so this should always succeed.
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				sub_segment_reconstruction_tree_creator,
				GPLATES_ASSERTION_SOURCE);

		// Get the points for this sub-segment.
		std::vector<GPlatesMaths::PointOnSphere> sub_segment_points;
		GeometryUtils::get_geometry_exterior_points(*sub_segment.get_geometry(), sub_segment_points);

		// Create the delaunay points for this sub-segment.
		std::vector<GPlatesMaths::PointOnSphere>::const_iterator sub_segment_points_iter =
				sub_segment_points.begin();
		std::vector<GPlatesMaths::PointOnSphere>::const_iterator sub_segment_points_end =
				sub_segment_points.end();
		for ( ; sub_segment_points_iter != sub_segment_points_end; ++sub_segment_points_iter)
		{
			const ResolvedTriangulation::Network::DelaunayPoint delaunay_point(
					*sub_segment_points_iter,
					sub_segment_plate_id.get(),
					sub_segment_reconstruction_tree_creator.get());
			delaunay_points.push_back(delaunay_point);
		}
	}
}


#if 0 // Not currently using being used...

void
GPlatesAppLogic::TopologyNetworkResolver::add_interior_constrained_delaunay_points(
		const ResolvedNetwork::InteriorGeometry &interior_geometry,
		std::vector<ResolvedTriangulation::Network::ConstrainedDelaunayGeometry> &constrained_delaunay_geometries,
		std::vector<GPlatesMaths::PointOnSphere> &scattered_points)
{
	const GPlatesMaths::GeometryType::Value interior_geometry_type =
			GeometryUtils::get_geometry_type(*interior_geometry.d_geometry);

	// Check for single point
	if (interior_geometry_type == GPlatesMaths::GeometryType::POINT)
	{
		// this is probably one of a collection of points; 
		// save and add to constrained triangulation later
		GeometryUtils::get_geometry_exterior_points(
				*interior_geometry.d_geometry,
				scattered_points);

		return;
	}

	// Check multipoint 
	if (interior_geometry_type == GPlatesMaths::GeometryType::MULTIPOINT)
	{
		// 2D + C
		// add multipoint with all connections between points constrained 
		constrained_delaunay_geometries.push_back(
				ResolvedTriangulation::Network::ConstrainedDelaunayGeometry(
						interior_geometry.d_geometry,
						true)); // do constrain begin and end points

		return;
	}

	// Check for polyline geometry
	if (interior_geometry_type == GPlatesMaths::GeometryType::POLYLINE)
	{
		// 2D + C
		// add as a constrained line segment ; do not constrain begin and end
		constrained_delaunay_geometries.push_back(
				ResolvedTriangulation::Network::ConstrainedDelaunayGeometry(
						interior_geometry.d_geometry,
						false)); // do not constrain begin and end points

		return;
	}

	// Check for polygon geometry
	if (interior_geometry_type == GPlatesMaths::GeometryType::POLYGON)
	{
		// 2D + C
		// add as a constrained line segment ; do constrain begin and end
		constrained_delaunay_geometries.push_back(
				ResolvedTriangulation::Network::ConstrainedDelaunayGeometry(
						interior_geometry.d_geometry,
						true)); // do constrain begin and end points
		return;
	}

	// Shouldn't be able to get here.
	GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
}


void
GPlatesAppLogic::TopologyNetworkResolver::debug_output_topological_source_feature(
		const GPlatesModel::FeatureId &source_feature_id)
{
	QString s;

	// get the fearture ref
	std::vector<GPlatesModel::FeatureHandle::weak_ref> back_ref_targets;
	source_feature_id.find_back_ref_targets(GPlatesModel::append_as_weak_refs(back_ref_targets));
	GPlatesModel::FeatureHandle::weak_ref feature_ref = back_ref_targets.front();

	// get the name
	s.append ( "SOURCE name = '" );
	static const GPlatesModel::PropertyName prop = GPlatesModel::PropertyName::create_gml("name");
	boost::optional<GPlatesPropertyValues::XsString::non_null_ptr_to_const_type> name =
			GPlatesFeatureVisitors::get_property_value<GPlatesPropertyValues::XsString>(feature_ref, prop);
 	if (name) {
		s.append(GPlatesUtils::make_qstring( name.get()->value() ));
 	} else {
 		s.append("UNKNOWN");
 	}
 	s.append("'; id = ");
	s.append ( GPlatesUtils::make_qstring_from_icu_string( source_feature_id.get() ) );

	qDebug() << s;
}

#endif // ...not currently using being used.


GPlatesAppLogic::TopologyNetworkResolver::ResolvedNetwork::BoundarySection::BoundarySection(
		const GPlatesModel::FeatureId &source_feature_id,
		const ReconstructionGeometry::non_null_ptr_type &source_rg,
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &source_geometry) :
	d_source_feature_id(source_feature_id),
	d_source_rg(source_rg),
	d_source_geometry(source_geometry),
	d_use_reverse(false),
	d_intersection_results(source_geometry)
{
}


GPlatesAppLogic::TopologyNetworkResolver::ResolvedNetwork::InteriorGeometry::InteriorGeometry(
		const GPlatesModel::FeatureId &source_feature_id,
		const ReconstructionGeometry::non_null_ptr_type &source_rg,
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry) :
	d_source_feature_id(source_feature_id),
	d_source_rg(source_rg),
	d_geometry(geometry)
{
}
