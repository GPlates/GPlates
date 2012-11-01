/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date: 2008-08-15 02:13:48 -0700 (Fri, 15 Aug 2008) $
 * 
 * Copyright (C) 2008, 2010 The University of Sydney, Australia
 * Copyright (C) 2008, 2009 California Institute of Technology 
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

#include "global/CompilerWarnings.h"

PUSH_MSVC_WARNINGS
DISABLE_MSVC_WARNING( 4503 ) // For C:\CGAL-3.5\include\CGAL/Mesh_2/Clusters.h
#include <map>
POP_MSVC_WARNINGS

#include <cstddef> // For std::size_t
#include <vector>
#include <QDebug>

#include "TopologyNetworkResolver.h"

#if defined (CGAL_MACOS_COMPILER_WORKAROUND)
#	ifdef NDEBUG
#		define HAVE_NDEBUG
#		undef NDEBUG
#	endif
#endif

#include "CgalUtils.h"

#if defined (CGAL_MACOS_COMPILER_WORKAROUND)
#	ifdef HAVE_NDEBUG
#		define NDEBUG
#	endif
#endif

#include "GeometryUtils.h"
#include "ReconstructedFeatureGeometry.h"
#include "Reconstruction.h"
#include "ReconstructionGeometryUtils.h"
#include "ResolvedTopologicalGeometry.h"
#include "ResolvedTopologicalNetwork.h"
#include "TopologyInternalUtils.h"
#include "TopologyUtils.h"

#include "feature-visitors/PropertyValueFinder.h"
#include "feature-visitors/GeometryTypeFinder.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

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
		std::vector<resolved_topological_network_non_null_ptr_type> &resolved_topological_networks,
		ReconstructHandle::type reconstruct_handle,
		const reconstruction_tree_non_null_ptr_to_const_type &reconstruction_tree,
		boost::optional<const std::vector<ReconstructHandle::type> &> topological_geometry_reconstruct_handles) :
	d_resolved_topological_networks(resolved_topological_networks),
	d_reconstruct_handle(reconstruct_handle),
	d_reconstruction_tree(reconstruction_tree),
	d_topological_geometry_reconstruct_handles(topological_geometry_reconstruct_handles),
	d_reconstruction_params(reconstruction_tree->get_reconstruction_time())
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
	if ( ! d_reconstruction_params.is_feature_defined_at_recon_time())
	{
		return false;
	}

	
	// visit a few specific props
	//
	{
		static const GPlatesModel::PropertyName property_name =
			GPlatesModel::PropertyName::create_gpml("networkShapeFactor");
		const GPlatesPropertyValues::XsDouble *property_value = NULL;

		if (!GPlatesFeatureVisitors::get_property_value( 
			feature_handle.reference(), property_name, property_value))
		{
			d_shape_factor = 0.125;
		}
		else
		{
			d_shape_factor = property_value->value();
		}
	}
	// 
	{
		static const GPlatesModel::PropertyName property_name =
			GPlatesModel::PropertyName::create_gpml("networkMaxEdge");
		const GPlatesPropertyValues::XsDouble *property_value = NULL;

		if (!GPlatesFeatureVisitors::get_property_value( 
			feature_handle.reference(), property_name, property_value))
		{
			d_max_edge = 5.0;
		}
		else
		{
			d_max_edge = property_value->value();
		}
	}

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
	std::vector<GPlatesPropertyValues::GpmlTimeWindow>::iterator iter =
			gpml_piecewise_aggregation.time_windows().begin();

	std::vector<GPlatesPropertyValues::GpmlTimeWindow>::iterator end =
			gpml_piecewise_aggregation.time_windows().end();

	for ( ; iter != end; ++iter) 
	{
		// NOTE: We really should be checking the time period of each time window against the
		// current reconstruction time.
		// However we won't fix this just yet because GPML files created with old versions of GPlates
		// set the time period, of the sole time window, to match that of the 'feature's time period
		// (in the topology build/edit tools) - newer versions set it to *all* time (distant past/future).
		// If the user expands the 'feature's time period *after* building/editing the topology then
		// the *un-adjusted* time window time period will be incorrect and hence we need to ignore it.
		// By the way, the time window is a *sole* time window because the topology tools cannot yet
		// create time-dependent topology (section) lists.
		visit_gpml_time_window(*iter);
	}
}


void
GPlatesAppLogic::TopologyNetworkResolver::visit_gpml_time_window(
		GPlatesPropertyValues::GpmlTimeWindow &gpml_time_window)
{
	gpml_time_window.time_dependent_value()->accept_visitor(*this);
	gpml_time_window.valid_time()->accept_visitor(*this);
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

	// See if the topological point references a seed feature.
	boost::optional<ResolvedNetwork::SeedGeometry> seed_geometry =
			find_seed_geometry(
					topological_reconstruction_geometry.get());
	if (seed_geometry)
	{
		// Add to the list of seed geometries and return.
		d_resolved_network.d_seed_geometries.push_back(seed_geometry.get());
		return;
	}
	// ...else topological point is not a seed geometry.

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

	// See if the topological interior references a seed feature.
	boost::optional<ResolvedNetwork::SeedGeometry> seed_geometry =
			find_seed_geometry(
					topological_reconstruction_geometry.get());
	if (seed_geometry)
	{
		// Add to the list of seed geometries and return.
		d_resolved_network.d_seed_geometries.push_back(seed_geometry.get());
		return;
	}
	// ...else topological interior is not a seed geometry.

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
			d_reconstruction_tree->get_reconstruction_time(),
			topological_geometry_reconstruct_handles);
}


boost::optional<GPlatesAppLogic::TopologyNetworkResolver::ResolvedNetwork::SeedGeometry>
GPlatesAppLogic::TopologyNetworkResolver::find_seed_geometry(
		const ReconstructionGeometry::non_null_ptr_type &reconstruction_geometry)
{
	//
	// See if the referenced feature geometry is meant to be used as a seed point for CGAL.
	// These features are used only so that CGAL knows it shouldn't mesh inside an interior polygon.
	//
	// FIXME: These features can currently be either boundary sections or interior geometries ?
	// If we could get them into the 'gpml:TopologicalNetwork' property somehow that would be best.
	//

	boost::optional<GPlatesModel::FeatureHandle::weak_ref> feature_ref =
			GPlatesAppLogic::ReconstructionGeometryUtils::get_feature_ref(reconstruction_geometry);
	if (!feature_ref)
	{
		// Not a seed geometry.
		return boost::none;
	}

    // Test for seed point feature type.
	static const GPlatesModel::FeatureType POLYGON_CENTROID_FEATURE_TYPE =
			GPlatesModel::FeatureType::create_gpml("PolygonCentroidPoint");
	if (feature_ref.get()->feature_type() != POLYGON_CENTROID_FEATURE_TYPE)
	{
		// Not a seed geometry.
		return boost::none;
	}

	// See if it's a reconstructed feature geometry (or any of its derived types).
	boost::optional<ReconstructedFeatureGeometry *> rfg =
			ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
					ReconstructedFeatureGeometry>(reconstruction_geometry);
	if (!rfg)
	{
		// Not a seed geometry.
		return boost::none;
	}

	// This is a seed point, not a point in the triangulation.
	return ResolvedNetwork::SeedGeometry(rfg.get()->reconstructed_geometry());
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
					ReconstructedFeatureGeometry>(boundary_section_source_rg);
	if (boundary_section_source_rfg)
	{
		// Store the feature id and reconstruction geometry.
		return ResolvedNetwork::BoundarySection(
				boundary_section_source_feature_id,
				boundary_section_source_rfg.get(),
				boundary_section_source_rfg.get()->reconstructed_geometry());
	}

	// See if topological section is a resolved topological geometry.
	boost::optional<ResolvedTopologicalGeometry *> boundary_section_source_rtg =
			ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
					ResolvedTopologicalGeometry>(boundary_section_source_rg);
	if (boundary_section_source_rtg)
	{
		// See if resolved topological geometry is a line (not a boundary).
		boost::optional<ResolvedTopologicalGeometry::resolved_topology_line_ptr_type>
				boundary_section_resolved_line_geometry =
						boundary_section_source_rtg.get()->resolved_topology_line();
		if (boundary_section_resolved_line_geometry)
		{
			// Store the feature id and reconstruction geometry.
			return ResolvedNetwork::BoundarySection(
					boundary_section_source_feature_id,
					boundary_section_source_rtg.get(),
					boundary_section_resolved_line_geometry.get());
		}
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
					ReconstructedFeatureGeometry>(interior_source_rg);
	if (interior_source_rfg)
	{
		// Store the feature id and reconstruction geometry.
		return ResolvedNetwork::InteriorGeometry(
				interior_source_feature_id,
				interior_source_rfg.get(),
				interior_source_rfg.get()->reconstructed_geometry());
	}

	// See if topological interior is a resolved topological geometry.
	boost::optional<ResolvedTopologicalGeometry *> interior_source_rtg =
			ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
					ResolvedTopologicalGeometry>(interior_source_rg);
	if (interior_source_rtg)
	{
		// See if resolved topological geometry is a line (not a boundary).
		boost::optional<ResolvedTopologicalGeometry::resolved_topology_line_ptr_type>
				interior_resolved_line_geometry =
						interior_source_rtg.get()->resolved_topology_line();
		if (interior_resolved_line_geometry)
		{
			// Store the feature id and reconstruction geometry.
			return ResolvedNetwork::InteriorGeometry(
					interior_source_feature_id,
					interior_source_rtg.get(),
					interior_resolved_line_geometry.get());
		}
	}

	// If we got here then either (1) the user created a malformed GPML file somehow (eg, with a script)
	// or (2) it's a program error (because the topology build/edit tools should only currently allow
	// the user to add topological interiors that are reconstructed static geometries
	// (or resolved topological *lines* when resolving network boundaries).
	// We'll assume (1) and emit an error message rather than asserting/aborting.
	qWarning() << "Ignoring topological interior, for resolved network boundary, that is not a "
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
	// The triangulation structs for the topological network.

	// 2D 
	boost::shared_ptr<CgalUtils::cgal_delaunay_triangulation_2_type> delaunay_triangulation_2(
			new CgalUtils::cgal_delaunay_triangulation_2_type() );

	// 2D + C
	boost::shared_ptr<GPlatesAppLogic::CgalUtils::cgal_constrained_delaunay_triangulation_2_type> 
		constrained_delaunay_triangulation_2(
			new GPlatesAppLogic::CgalUtils::cgal_constrained_delaunay_triangulation_2_type() );

	// 2D + C + Mesh
	boost::shared_ptr<GPlatesAppLogic::CgalUtils::cgal_constrained_mesher_2_type> 
		constrained_mesher(
			new GPlatesAppLogic::CgalUtils::cgal_constrained_mesher_2_type( 
				*constrained_delaunay_triangulation_2 ) );

	GPlatesAppLogic::CgalUtils::cgal_constrained_delaunay_mesh_size_criteria_2_type 
		constrained_criteria( 
			d_shape_factor,
			d_max_edge);
	constrained_mesher->set_criteria( constrained_criteria ); 

	// 3D
	boost::shared_ptr<GPlatesAppLogic::CgalUtils::cgal_triangulation_hierarchy_3_type> 
		triangulation_hierarchy_3(
			new GPlatesAppLogic::CgalUtils::cgal_triangulation_hierarchy_3_type());

	// Lists of points used to insert into the triangulations

	// All the points to create the cgal_delaunay_triangulation_2_type
	std::vector<GPlatesMaths::PointOnSphere> all_network_points;

	// All the points on the boundary of the cgal_delaunay_triangulation_2_type
	std::vector<GPlatesMaths::PointOnSphere> boundary_points;

	// Points from a set of non-intersecting lines
	std::vector<GPlatesMaths::PointOnSphere> group_of_related_points;

	// Points from multple single point sections 
	std::vector<GPlatesMaths::PointOnSphere> scattered_points;

	// Sequence of boundary subsegments of resolved topology boundary.
	std::vector<ResolvedTopologicalGeometrySubSegment> boundary_subsegments;

	// Sequence of subsegments of resolved topology used when creating ResolvedTopologicalNetwork.
	// See the code in:
	// GPlatesAppLogic::TopologyUtils::query_resolved_topology_networks_for_interpolation
	std::vector<ResolvedTopologicalNetwork::Node> output_nodes;

	// Any interior section that is a polygon - these are regions that are inside the network but are
	// not part of the network (ie, not triangulated) and hence are effectively outside the network.
	std::vector<ResolvedTopologicalNetwork::InteriorPolygon> interior_polygons;

// qDebug() << "\ncreate_resolved_topology_network: Loop over BOUNDARY d_resolved_network.d_boundary_sections\n";
	//
	// Iterate over the sections of the resolved boundary and construct
	// the resolved polygon boundary and its subsegments.
	//
	ResolvedNetwork::boundary_section_seq_type::const_iterator boundary_sections_iter =
			d_resolved_network.d_boundary_sections.begin();
	ResolvedNetwork::boundary_section_seq_type::const_iterator boundary_sections_end =
			d_resolved_network.d_boundary_sections.end();
	for ( ; boundary_sections_iter != boundary_sections_end; ++boundary_sections_iter)
	{
		const ResolvedNetwork::BoundarySection &boundary_section = *boundary_sections_iter;

		# if 0
		debug_output_topological_source_feature(boundary_section.d_source_feature_id);
		#endif

		// It's possible for a valid segment to not contribute to the boundary
		// of the network. This can happen if it contributes zero-length
		// to the network boundary which happens when both its neighbouring
		// boundary sections intersect it at the same point.
		if (!boundary_section.d_final_boundary_segment_unreversed_geom)
		{
			continue; // to next section in topology network
		}

		// Get the subsegment feature reference.
		boost::optional<GPlatesModel::FeatureHandle::weak_ref> subsegment_feature_ref =
				GPlatesAppLogic::ReconstructionGeometryUtils::get_feature_ref(boundary_section.d_source_rg);
		// If the feature reference is invalid then skip the current section.
		if (!subsegment_feature_ref)
		{
			continue;
		}
		const GPlatesModel::FeatureHandle::const_weak_ref subsegment_feature_const_ref(
				subsegment_feature_ref.get());

		// Create a subsegment structure that'll get used when
		// creating the boundary of the resolved topological geometry.
		const ResolvedTopologicalGeometrySubSegment boundary_subsegment(
				boundary_section.d_final_boundary_segment_unreversed_geom.get(),
				subsegment_feature_const_ref,
				boundary_section.d_use_reverse);
		boundary_subsegments.push_back(boundary_subsegment);

		// Create a subsegment structure that'll get used when
		// creating the resolved topological geometry.
		const ResolvedTopologicalNetwork::Node output_node(
				boundary_section.d_final_boundary_segment_unreversed_geom.get(),
				subsegment_feature_const_ref);
		output_nodes.push_back(output_node);

		// Append the subsegment geometry to the total network points.
		GPlatesAppLogic::GeometryUtils::get_geometry_points(
				*boundary_section.d_final_boundary_segment_unreversed_geom.get(),
				all_network_points,
				boundary_section.d_use_reverse);

		//
		// Determine the subsegments original geometry type 
		//
		// NOTE: 'GeometryTypeFinder' only works with regular features (not topological lines)
		// so instead we determine the type directly from the GeometryOnSphere itself rather
		// than visiting the section *feature*.
		//
		// FIXME: Should this be the section geometry before or after intersection-clipping?
		const GPlatesViewOperations::GeometryType::Value section_geometry_type =
				GeometryUtils::get_geometry_type(*boundary_section.d_source_geometry);

		//
		// Determine how to add the subsegment's points to the triangulation
		//

		// Check for single point
		if (section_geometry_type == GPlatesViewOperations::GeometryType::POINT)
		{
			// this is probably one of a collection of points; 
			// save and add to constrained triangulation later
			GPlatesAppLogic::GeometryUtils::get_geometry_points(
				*boundary_section.d_final_boundary_segment_unreversed_geom.get(),
				boundary_points,
				boundary_section.d_use_reverse);

			continue; // to next section of the topology
		}

		// Check multipoint 
		if (section_geometry_type == GPlatesViewOperations::GeometryType::MULTIPOINT)
		{
			// this is a single multi point feature section
			GPlatesAppLogic::GeometryUtils::get_geometry_points(
					*boundary_section.d_final_boundary_segment_unreversed_geom.get(),
					boundary_points,
					boundary_section.d_use_reverse);

			continue; // to next section of the topology
		}

		// Check for polyline geometry
		if (section_geometry_type == GPlatesViewOperations::GeometryType::POLYLINE)
		{
			// this is a single line feature, possibly clipped by intersections
			// NOTE: We cannot use the presence of gpml start and end intersection to
			// determine if a line is clipped or not (since they have been deprecated due
			// to the auto-intersection-reversal algorithm).
			GPlatesAppLogic::GeometryUtils::get_geometry_points(
					*boundary_section.d_final_boundary_segment_unreversed_geom.get(),
					boundary_points,
					boundary_section.d_use_reverse);

			continue; // to next section of the topology
		}

		// Check for polygon geometry
		if (section_geometry_type == GPlatesViewOperations::GeometryType::POLYGON)
		{
			// this is a single polygon feature
			GPlatesAppLogic::GeometryUtils::get_geometry_points(
					*boundary_section.d_final_boundary_segment_unreversed_geom.get(),
					boundary_points,
					boundary_section.d_use_reverse);

			continue; // to next section of the topology
		}

	} // end of loop over boundary sections

#if 0
qDebug() << "all_network_points.size(): " << all_network_points.size();
qDebug() << "boundary_points.size(): " << boundary_points.size();
#endif


	// 2D + C
	// add boundary_points as contrained ; do contrain begin and end
	GPlatesAppLogic::CgalUtils::insert_points_into_constrained_delaunay_triangulation_2(
		*constrained_delaunay_triangulation_2, 
		boundary_points.begin(), 
		boundary_points.end(),
		true);

	// Create a polygon on sphere for the resolved boundary using 'boundary_points'.
	GPlatesUtils::GeometryConstruction::GeometryConstructionValidity boundary_polygon_validity;
	boost::optional<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> boundary_polygon =
			GPlatesUtils::create_polygon_on_sphere(
					boundary_points.begin(), boundary_points.end(), boundary_polygon_validity);

	// If we are unable to create a polygon (such as insufficient points) then
	// just return without creating a resolved topological geometry.
	if (boundary_polygon_validity != GPlatesUtils::GeometryConstruction::VALID)
	{
		qDebug() << "ERROR: Failed to create a polygon boundary for a ResolvedTopologicalNetwork - "
				"probably has insufficient points for a polygon.";
		qDebug() << "Skipping creation for topological network feature_id=";
		qDebug() << GPlatesUtils::make_qstring_from_icu_string(
				d_currently_visited_feature->feature_id().get());
		return;
	}

// qDebug() << "\ncreate_resolved_topology_network: Loop over INTERIOR d_resolved_network.d_interior_geometries\n";

	// 
	// Iterate over the interior geometries.
	//
	ResolvedNetwork::interior_geometry_seq_type::const_iterator interior_geometry_iter =
			d_resolved_network.d_interior_geometries.begin();
	ResolvedNetwork::interior_geometry_seq_type::const_iterator interior_geometry_end =
			d_resolved_network.d_interior_geometries.end();
	for ( ; interior_geometry_iter != interior_geometry_end; ++interior_geometry_iter)
	{
		const ResolvedNetwork::InteriorGeometry &interior_geometry = *interior_geometry_iter;

		#if 0
		debug_output_topological_source_feature(interior_geometry.d_source_feature_id);
		#endif

		// Get the interior feature reference.
		boost::optional<GPlatesModel::FeatureHandle::weak_ref> interior_feature_ref =
				GPlatesAppLogic::ReconstructionGeometryUtils::get_feature_ref(interior_geometry.d_source_rg);
		// If the feature reference is invalid then skip the current section.
		if (!interior_feature_ref)
		{
			continue;
		}
		const GPlatesModel::FeatureHandle::const_weak_ref interior_feature_const_ref(
				interior_feature_ref.get());

		// Create a subsegment structure that'll get used when
		// creating the resolved topological geometry.
		const ResolvedTopologicalNetwork::Node output_node(
				interior_geometry.d_geometry,
				interior_feature_const_ref);
		output_nodes.push_back(output_node);

		// Append the interior geometry to the total network points.
		GPlatesAppLogic::GeometryUtils::get_geometry_points(
				*interior_geometry.d_geometry,
				all_network_points);

		// Keep track of any interior polygon regions.
		// These will be needed for calculating velocities since they are not part of the triangulation
		// generated (velocities will be calculated in the normal manner for static polygons).
		//
		// NOTE: Since currently only RFGs and resolved topological *lines* can be referenced by
		// networks it's only possible to have an interior polygon if it's an RFG - so we don't
		// need to worry about resolved topological geometries just yet.
		// See if interior is a reconstructed feature geometry (or any of its derived types).
		boost::optional<ReconstructedFeatureGeometry *> interior_rfg =
				ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
						ReconstructedFeatureGeometry>(interior_geometry.d_source_rg);
		if (interior_rfg)
		{
			// NOTE: 'GeometryTypeFinder' only works with regular features (not topological lines).
			GPlatesFeatureVisitors::GeometryTypeFinder geometry_type_finder;

			interior_rfg.get()->reconstructed_geometry()->accept_visitor(geometry_type_finder);
			if (geometry_type_finder.num_polygon_geometries_found() > 0)
			{
				const ResolvedTopologicalNetwork::InteriorPolygon interior_polygon(interior_rfg.get());
				interior_polygons.push_back(interior_polygon);
			}
		}

		//
		// Determine the interior geometry type.
		//
		// NOTE: 'GeometryTypeFinder' only works with regular features (not topological lines)
		// so instead we determine the type directly from the GeometryOnSphere itself rather
		// than visiting the interior *feature*.
		const GPlatesViewOperations::GeometryType::Value interior_geometry_type =
				GeometryUtils::get_geometry_type(*interior_geometry.d_geometry);

		// Check for single point
		if (interior_geometry_type == GPlatesViewOperations::GeometryType::POINT)
		{
			//std::vector<GPlatesMaths::PointOnSphere> interior_points;

			// this is probably one of a collection of points; 
			// save and add to constrained triangulation later
			GPlatesAppLogic::GeometryUtils::get_geometry_points(
				*interior_geometry.d_geometry,
				scattered_points);
			//qDebug() << "interior_points.size(): " << interior_points.size();

			continue; // to next interior geometry of the topology
		}

		// Check multipoint 
		if (interior_geometry_type == GPlatesViewOperations::GeometryType::MULTIPOINT)
		{
			std::vector<GPlatesMaths::PointOnSphere> interior_points;

			// this is a single multi point feature section
			GPlatesAppLogic::GeometryUtils::get_geometry_points(
					*interior_geometry.d_geometry,
					interior_points);
			//qDebug() << "interior_points.size(): " << interior_points.size();

			// 2D + C
			// add multipoint with all connections between points contrained 
			GPlatesAppLogic::CgalUtils::insert_points_into_constrained_delaunay_triangulation_2(
				*constrained_delaunay_triangulation_2, 
				interior_points.begin(), 
				interior_points.end(),
				true);

			continue; // to next interior geometry of the topology
		}

		// Check for polyline geometry
		if (interior_geometry_type == GPlatesViewOperations::GeometryType::POLYLINE)
		{
			std::vector<GPlatesMaths::PointOnSphere> interior_points;

			// this is a single line feature, possibly clipped by intersections
			// NOTE: We cannot use the presence of gpml start and end intersection to
			// determine if a line is clipped or not (since they have been deprecated due
			// to the auto-intersection-reversal algorithm).
			GPlatesAppLogic::GeometryUtils::get_geometry_points(
					*interior_geometry.d_geometry,
					interior_points);
			//qDebug() << "interior_points.size(): " << interior_points.size();

			// 2D + C
			// add as a contrained line segment ; do not contrain begin and end
			GPlatesAppLogic::CgalUtils::insert_points_into_constrained_delaunay_triangulation_2(
				*constrained_delaunay_triangulation_2, 
				interior_points.begin(), 
				interior_points.end(),
				false);

			continue; // to next interior geometry of the topology
		}

		// Check for polygon geometry
		if (interior_geometry_type == GPlatesViewOperations::GeometryType::POLYGON)
		{
			std::vector<GPlatesMaths::PointOnSphere> interior_points;

			// this is a single polygon feature
			GPlatesAppLogic::GeometryUtils::get_geometry_points(
					*interior_geometry.d_geometry,
					interior_points);
			//qDebug() << "interior_points.size(): " << interior_points.size();
			
			// 2D + C
			// add as a contrained line segment ; do contrain begin and end
			GPlatesAppLogic::CgalUtils::insert_points_into_constrained_delaunay_triangulation_2(
				*constrained_delaunay_triangulation_2, 
				interior_points.begin(), 
				interior_points.end(),
				true);

// FIXME: this idea should work, but it seems like rounding errors ( or somthing else ?) 
// causes the last vertex to shift from inside to outside the section polygon
// and the network flips between the whole region , or just the section polygon 
// during certain reconstruction ages ...  so just comment out for now ... 
// FIXME: probably should use CGAL to compute a centroid for the section polygon 
// and use that as a mesh seed point 
#if 0
			// check If the feature has the 'block' property, 

			// get the rigidBlock prop value
			static const GPlatesModel::PropertyName prop_name = 
				GPlatesModel::PropertyName::create_gpml("rigidBlock");
			const GPlatesPropertyValues::XsBoolean *prop_value;
			if ( GPlatesFeatureVisitors::get_property_value( 
				subsegment_feature_ref, prop_name, prop_value) )
			{
				bool is_rigid_block = prop_value->value();
				if ( is_rigid_block )
				{
					qDebug() << "====> property rigidBlock =" << is_rigid_block;
					// then add it's last vertex as a seed point 
					all_seed_points.push_back( interior_points.back() );
				}
			}
			else
			{
				qDebug() << "====> property rigidBlock = NOT FOUND";
			}
#endif

			continue; // to next interior geometry of the topology
		}

	} // end of loop over interior geometries


	// Now add all the group_of_related_points ; 
	if ( group_of_related_points.size() > 0)
	{
		GPlatesAppLogic::CgalUtils::insert_points_into_constrained_delaunay_triangulation_2(
			*constrained_delaunay_triangulation_2, 
			group_of_related_points.begin(), 
			group_of_related_points.end(),
			true); //do contrain begin and end
	}

	// Now add all the scattered_pointsl ; 
	if ( scattered_points.size() > 0)
	{
		GPlatesAppLogic::CgalUtils::insert_scattered_points_into_constrained_delaunay_triangulation_2(
			*constrained_delaunay_triangulation_2, 
			scattered_points.begin(), 
			scattered_points.end(),
			false); //do NOT contrain every point to ever other point 
	}



	// 2D 
	GPlatesAppLogic::CgalUtils::insert_points_into_delaunay_triangulation_2(
		*delaunay_triangulation_2, 
		all_network_points.begin(), 
		all_network_points.end() );

#if 0
	// 2D + C
	GPlatesAppLogic::CgalUtils::insert_points_into_constrained_delaunay_triangulation_2(
		*constrained_delaunay_triangulation_2, 
		all_network_points.begin(), 
		all_network_points.end(),
		true);
#endif

#if 0
	// 3D
	GPlatesAppLogic::CgalUtils::insert_points_into_delaunay_triangulation_hierarchy_3(
		*triangulation_hierarchy_3, all_network_points.begin(), all_network_points.end());
#endif
// NOTE : these are examples of how to add points to all the triangulation types
// 2D ; 2D + C ; 3D , using all_network_points ; save them for reference

	//
	// Add the seed points to the mesher
	//

	std::vector<GPlatesMaths::PointOnSphere> all_seed_points;

	// Iterate over the seed geometries.
	ResolvedNetwork::seed_geometry_seq_type::const_iterator seed_geometries_iter =
			d_resolved_network.d_seed_geometries.begin();
	ResolvedNetwork::seed_geometry_seq_type::const_iterator seed_geometries_end =
			d_resolved_network.d_seed_geometries.end();
	for ( ; seed_geometries_iter != seed_geometries_end; ++seed_geometries_iter)
	{
		const ResolvedNetwork::SeedGeometry &seed_geometry = *seed_geometries_iter;

		// Each point in the geometry contributes a seed point.
		// We're only expecting single point geometries though.
		GPlatesAppLogic::GeometryUtils::get_geometry_points(
				*seed_geometry.d_geometry,
				all_seed_points);
	}

	if (all_seed_points.size() > 0) 
	{
		GPlatesAppLogic::CgalUtils::insert_seed_points_into_constrained_mesh(
			*constrained_mesher, all_seed_points.begin(), all_seed_points.end());
	}

	// Mesh the data 
	constrained_mesher->refine_mesh();

#if 0
std::cout << "constrained_delaunay_triangulation_2 verts: " 
<< constrained_delaunay_triangulation_2->number_of_vertices() << std::endl;
#endif

	// make it conforming Delaunay
	CGAL::make_conforming_Delaunay_2(*constrained_delaunay_triangulation_2);

#if 0
std::cout << "Number of vertices after make_conforming_Delaunay_2: "
<< constrained_delaunay_triangulation_2->number_of_vertices() << std::endl;
#endif

	// then make it conforming Gabriel
	CGAL::make_conforming_Gabriel_2(*constrained_delaunay_triangulation_2);

#if 0
std::cout << "Number of vertices after make_conforming_Gabriel_2: "
<< constrained_delaunay_triangulation_2->number_of_vertices() << std::endl;
#endif
	
	// Create the network RTN 
	const ResolvedTopologicalNetwork::non_null_ptr_type network =
			ResolvedTopologicalNetwork::create(
					d_reconstruction_tree,
					delaunay_triangulation_2,
					constrained_delaunay_triangulation_2,
					*(current_top_level_propiter()->handle_weak_ref()),
					*(current_top_level_propiter()),
					output_nodes.begin(),
					output_nodes.end(),
					boundary_subsegments.begin(),
					boundary_subsegments.end(),
					boundary_polygon.get(),
					interior_polygons.begin(),
					interior_polygons.end(),
					d_reconstruction_params.get_recon_plate_id(),
					d_reconstruction_params.get_time_of_appearance(),
					d_reconstruct_handle/*identify where/when this RTN was resolved*/);

	d_resolved_topological_networks.push_back(network);
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
 	const GPlatesPropertyValues::XsString *name;
 	if ( GPlatesFeatureVisitors::get_property_value(feature_ref, prop, name) ) {
		s.append(GPlatesUtils::make_qstring( name->value() ));
 	} else {
 		s.append("UNKNOWN");
 	}
 	s.append("'; id = ");
	s.append ( GPlatesUtils::make_qstring_from_icu_string( source_feature_id.get() ) );

	qDebug() << s;
}

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

GPlatesAppLogic::TopologyNetworkResolver::ResolvedNetwork::SeedGeometry::SeedGeometry(
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry) :
	d_geometry(geometry)
{
}
