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

#include "CgalUtils.h"
#include "GeometryUtils.h"
#include "ReconstructionGeometryUtils.h"
#include "ReconstructedFeatureGeometry.h"
#include "Reconstruction.h"
#include "ResolvedTopologicalBoundary.h"
#include "ResolvedTopologicalNetwork.h"
#include "TopologyInternalUtils.h"
#include "TopologyUtils.h"

#include "feature-visitors/PropertyValueFinder.h"
#include "feature-visitors/GeometryTypeFinder.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "maths/GeometryOnSphere.h"

#include "model/FeatureHandleWeakRefBackInserter.h"

#include "property-values/XsBoolean.h"
#include "property-values/XsDouble.h"
#include "property-values/XsString.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlPiecewiseAggregation.h"
#include "property-values/GpmlTopologicalInterior.h"
#include "property-values/GpmlTopologicalPolygon.h"
#include "property-values/GpmlTopologicalSection.h"
#include "property-values/GpmlTopologicalLineSection.h"
#include "property-values/GpmlTopologicalIntersection.h"
#include "property-values/GpmlTopologicalPoint.h"

#include "utils/GeometryCreationUtils.h"
#include "utils/UnicodeStringUtils.h"
#include "utils/Profile.h"


GPlatesAppLogic::TopologyNetworkResolver::TopologyNetworkResolver(
		std::vector<resolved_topological_network_non_null_ptr_type> &resolved_topological_networks,
		const reconstruction_tree_non_null_ptr_to_const_type &reconstruction_tree,
		boost::optional<const std::vector<ReconstructHandle::type> &> topological_sections_reconstruct_handles) :
	d_resolved_topological_networks(resolved_topological_networks),
	d_reconstruction_tree(reconstruction_tree),
	d_topological_sections_reconstruct_handles(topological_sections_reconstruct_handles),
	d_reconstruction_params(reconstruction_tree->get_reconstruction_time())
{  
	d_num_topologies = 0;
}

void
GPlatesAppLogic::TopologyNetworkResolver::finalise_post_feature_properties(
		GPlatesModel::FeatureHandle &feature_handle)
{
	// If we visited a GpmlTopologicalPolygon property then create:
	// ResolvedTopologicalNetwork.

	if (d_topological_polygon_feature_iterator)
	{
		create_resolved_topology_network(feature_handle);
	}
}

bool
GPlatesAppLogic::TopologyNetworkResolver::initialise_pre_feature_properties(
		GPlatesModel::FeatureHandle &feature_handle)
{
	// super short-cut for features without network properties
	if (!TopologyUtils::is_topological_network_feature(feature_handle))
	{ 
		// Quick-out: No need to continue.
		return false; 
	}

	// Reset the visited GpmlTopologicalPolygon property.
	d_topological_polygon_feature_iterator = boost::none;

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
			GPlatesModel::PropertyName::create_gpml("shapeFactor");
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
			GPlatesModel::PropertyName::create_gpml("maxEdge");
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
GPlatesAppLogic::TopologyNetworkResolver::visit_gpml_topological_polygon(
		GPlatesPropertyValues::GpmlTopologicalPolygon &gpml_topological_polygon)
{
	PROFILE_FUNC();

	// set the visit mode
	d_is_visit_interior = false;

	// Prepare for a new topological polygon.
	d_resolved_network.reset();

	// Record the visited property iterator since we are now creating the
	// resolved topological network *outside* the feature property iteration visitation
	// (ie, 'current_top_level_propiter()' cannot later be called once we finished visiting).
	d_topological_polygon_feature_iterator = current_top_level_propiter();

	//
	// Visit the topological sections to gather needed information and store
	// it internally in 'd_resolved_network', and 'd_resolved_network'
	//
	record_topological_sections( gpml_topological_polygon.sections() );

	//
	// See if the topological section 'start' and 'end' intersections are consistent.
	//
	validate_topological_section_intersections_boundary();

	//
	// Now iterate over our internal structure 'd_resolved_network' and
	// intersect neighbouring sections that require it and
	// generate the resolved boundary subsegments.
	//
	process_topological_section_intersections_boundary();

	//
	// Now iterate over the intersection results and assign boundary segments to
	// each section.
	//
	assign_boundary_segments_boundary();

	// NOTE: unlike TopologyBoundaryResolver,
	// the final creation step is called from finalise_post_feature_properties()

}

void
GPlatesAppLogic::TopologyNetworkResolver::visit_gpml_topological_interior(
		GPlatesPropertyValues::GpmlTopologicalInterior &gpml_topological_interior)
{

	// set the visit mode
	d_is_visit_interior = true;

	// Visit the topological sections to gather needed information 
	record_topological_sections( gpml_topological_interior.sections() );

	validate_topological_section_intersections_interior();

	// NOTE: interior sections do not get intersected with each other
	// do not call: process_topological_section_intersections_interior();
	// left here for reference if we want to change things ...

	assign_boundary_segments_interior();
}


void
GPlatesAppLogic::TopologyNetworkResolver::record_topological_sections(
		std::vector<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type> &
				sections)
{
	// loop over all the sections
	std::vector<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type>::iterator iter =
			sections.begin();
	std::vector<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type>::iterator end =
			sections.end();
	for ( ; iter != end; ++iter)
	{
		GPlatesPropertyValues::GpmlTopologicalSection *topological_section = iter->get();

		topological_section->accept_visitor(*this);
	}
}


void
GPlatesAppLogic::TopologyNetworkResolver::visit_gpml_topological_line_section(
		GPlatesPropertyValues::GpmlTopologicalLineSection &gpml_toplogical_line_section)
{  
	const GPlatesModel::FeatureId source_feature_id =
			gpml_toplogical_line_section.get_source_geometry()->feature_id();

	boost::optional<ResolvedNetwork::Section> section =
			record_topological_section_reconstructed_geometry(
					source_feature_id,
					*gpml_toplogical_line_section.get_source_geometry());
	if (!section)
	{
		// Return without adding topological section to the list of boundary sections.
		return;
	}

	// Set reverse flag.
	section->d_use_reverse = gpml_toplogical_line_section.get_reverse_order();

	// Record start intersection information.
	if (gpml_toplogical_line_section.get_start_intersection())
	{
		const GPlatesMaths::PointOnSphere reference_point =
				*gpml_toplogical_line_section.get_start_intersection()->reference_point()->point();

		section->d_start_intersection = ResolvedNetwork::Intersection(reference_point);
	}

	// Record end intersection information.
	if (gpml_toplogical_line_section.get_end_intersection())
	{
		const GPlatesMaths::PointOnSphere reference_point =
				*gpml_toplogical_line_section.get_end_intersection()->reference_point()->point();

		section->d_end_intersection = ResolvedNetwork::Intersection(reference_point);
	}

	// Add to internal sequence.
	if (d_is_visit_interior)
		d_resolved_network.d_sections_interior.push_back(*section);
	else
		d_resolved_network.d_sections.push_back(*section);
}


void
GPlatesAppLogic::TopologyNetworkResolver::visit_gpml_topological_point(
		GPlatesPropertyValues::GpmlTopologicalPoint &gpml_toplogical_point)
{  
	const GPlatesModel::FeatureId source_feature_id =
			gpml_toplogical_point.get_source_geometry()->feature_id();

	boost::optional<ResolvedNetwork::Section> section =
			record_topological_section_reconstructed_geometry(
					source_feature_id,
					*gpml_toplogical_point.get_source_geometry());
	if (!section)
	{
		// Return without adding topological section to the list of boundary sections.
		return;
	}

	// set source geom type
	section->d_is_point = true;

    // Test for feature type 
	static const GPlatesModel::FeatureType test_type =
		GPlatesModel::FeatureType::create_gpml("PolygonCentroidPoint");

	// this will become a seed point , not a point in the triangulation
	if ( section->d_source_rfg->get_feature_ref()->feature_type() == test_type )
	{
		section->d_is_seed_point = true;
	}

	// No other information to collect since this topological section is a point and
	// hence cannot intersect with neighbouring sections.

	// Add to internal sequence.
	if (d_is_visit_interior)
		d_resolved_network.d_sections_interior.push_back(*section);
	else
		d_resolved_network.d_sections.push_back(*section);
}


//////////////////////////////////////////////////

boost::optional<GPlatesAppLogic::TopologyNetworkResolver::ResolvedNetwork::Section>
GPlatesAppLogic::TopologyNetworkResolver::record_topological_section_reconstructed_geometry(
		const GPlatesModel::FeatureId &source_feature_id,
		const GPlatesPropertyValues::GpmlPropertyDelegate &geometry_delegate)
{
	// Get the reconstructed geometry of the topological section's delegate.
	// The referenced RFGs must be in our sequence of reconstructed topological sections.
	// If we need to restrict the topological section RFGs to specific reconstruct handles...
	boost::optional<const std::vector<ReconstructHandle::type> &> topological_sections_reconstruct_handles;
	if (d_topological_sections_reconstruct_handles)
	{
		topological_sections_reconstruct_handles = d_topological_sections_reconstruct_handles.get();
	}
	// Find the topological section RFG.
	boost::optional<ReconstructedFeatureGeometry::non_null_ptr_type> source_rfg =
			TopologyInternalUtils::find_reconstructed_feature_geometry(
					geometry_delegate,
					topological_sections_reconstruct_handles);

	// If no RFG was found then it's possible that the current reconstruction time is
	// outside the age range of the feature this section is referencing.
	// This is ok - it's not necessarily an error.
	// If we're currently processing boundary sections (as opposed to interior sections) then
	// we just won't add it to the list of boundary sections. For boundary sections this means either:
	//  - rubber banding will occur between the two sections adjacent to this section
	//    since this section is now missing, or
	//  - one of the adjacent sections did not exist until just now (because of its age range)
	//    and now it is popping in to replace the current section which is disappearing (an
	//    example of this is a bunch of sections that are mid-ocean ridge features that do not
	//    overlap in time and represent different geometries, from isochrons, of the same ridge).
	if (!source_rfg)
	{
		return boost::none;
	}

	// Store the feature id and RFG.
	return ResolvedNetwork::Section(source_feature_id, *source_rfg);
}

///////////
// VALIDATE sections 
///////////

void
GPlatesAppLogic::TopologyNetworkResolver::validate_topological_section_intersections_boundary()
{
	// Iterate over our internal sequence of sections that we built up by
	// visiting the topological sections of a topological polygon.
	const std::size_t num_sections = d_resolved_network.d_sections.size();
	for (std::size_t section_index = 0; section_index < num_sections; ++section_index)
	{
		validate_topological_section_intersection_boundary(section_index);
	}
}

void
GPlatesAppLogic::TopologyNetworkResolver::validate_topological_section_intersection_boundary(
		const std::size_t current_section_index)
{
	const std::size_t num_sections = d_resolved_network.d_sections.size();

	const ResolvedNetwork::Section &current_section =
			d_resolved_network.d_sections[current_section_index];

	// If the current section has a 'start' intersection then the previous section
	// should have an 'end' intersection.
	if (current_section.d_start_intersection)
	{
		const std::size_t prev_section_index = (current_section_index == 0)
				? num_sections - 1
				: current_section_index - 1;
		const ResolvedNetwork::Section &prev_section =
				d_resolved_network.d_sections[prev_section_index];

		if (!prev_section.d_end_intersection)
		{
			qDebug() << "ERROR: Validate failure for GpmlTopologicalPolygon.";
			qDebug() << "If a GpmlTopologicalSection has a start intersection then "
					"the previous GpmlTopologicalSection should have an end intersection.";
			debug_output_topological_section(prev_section);
		}
	}

	// If the current section has an 'end' intersection then the next section
	// should have a 'start' intersection.
	if (current_section.d_end_intersection)
	{
		const std::size_t next_section_index = (current_section_index == num_sections - 1)
				? 0
				: current_section_index + 1;
		const ResolvedNetwork::Section &next_section =
				d_resolved_network.d_sections[next_section_index];

		if (!next_section.d_start_intersection)
		{
			qDebug() << "ERROR: Validate failure for GpmlTopologicalPolygon.";
			qDebug() << "If a GpmlTopologicalSection has an end intersection then "
					"the next GpmlTopologicalSection should have a start intersection.";
			debug_output_topological_section(next_section);
		}
	}
}


void
GPlatesAppLogic::TopologyNetworkResolver::validate_topological_section_intersections_interior()
{
	// Iterate over our internal sequence of sections that we built up by
	// visiting the topological sections of a topological polygon.
	const std::size_t num_sections = d_resolved_network.d_sections_interior.size();
	for (std::size_t section_index = 0; section_index < num_sections; ++section_index)
	{
		validate_topological_section_intersection_interior(section_index);
	}
}

void
GPlatesAppLogic::TopologyNetworkResolver::validate_topological_section_intersection_interior(
		const std::size_t current_section_index)
{
	const std::size_t num_sections = d_resolved_network.d_sections_interior.size();

	const ResolvedNetwork::Section &current_section =
			d_resolved_network.d_sections_interior[current_section_index];

	// If the current section has a 'start' intersection then the previous section
	// should have an 'end' intersection.
	if (current_section.d_start_intersection)
	{
		const std::size_t prev_section_index = (current_section_index == 0)
				? num_sections - 1
				: current_section_index - 1;
		const ResolvedNetwork::Section &prev_section =
				d_resolved_network.d_sections_interior[prev_section_index];

		if (!prev_section.d_end_intersection)
		{
			qDebug() << "ERROR: Validate failure for GpmlTopologicalPolygon.";
			qDebug() << "If a GpmlTopologicalSection has a start intersection then "
					"the previous GpmlTopologicalSection should have an end intersection.";
			debug_output_topological_section(prev_section);
		}
	}

	// If the current section has an 'end' intersection then the next section
	// should have a 'start' intersection.
	if (current_section.d_end_intersection)
	{
		const std::size_t next_section_index = (current_section_index == num_sections - 1)
				? 0
				: current_section_index + 1;
		const ResolvedNetwork::Section &next_section =
				d_resolved_network.d_sections_interior[next_section_index];

		if (!next_section.d_start_intersection)
		{
			qDebug() << "ERROR: Validate failure for GpmlTopologicalPolygon.";
			qDebug() << "If a GpmlTopologicalSection has an end intersection then "
					"the next GpmlTopologicalSection should have a start intersection.";
			debug_output_topological_section(next_section);
		}
	}
}

//////////
// PROCESS INTERSECTIONS
/////////

void
GPlatesAppLogic::TopologyNetworkResolver::process_topological_section_intersections_boundary()
{
	// Iterate over our internal sequence of sections that we built up by
	// visiting the topological sections of a topological polygon.
	const std::size_t num_sections = d_resolved_network.d_sections.size();

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

	const std::size_t num_sections = d_resolved_network.d_sections.size();

	ResolvedNetwork::Section &current_section =
			d_resolved_network.d_sections[current_section_index];

	//
	// NOTE: We don't get the start intersection geometry from the GpmlTopologicalIntersection
	// - instead we get the geometry from the previous section in the topological polygon's
	// list of sections whose valid time ranges include the current reconstruction time.
	//

	const std::size_t prev_section_index = (current_section_index == 0)
			? num_sections - 1
			: current_section_index - 1;

	ResolvedNetwork::Section &prev_section =
			d_resolved_network.d_sections[prev_section_index];

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
	if (prev_section.d_source_rfg.get() == current_section.d_source_rfg.get())
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


void
GPlatesAppLogic::TopologyNetworkResolver::process_topological_section_intersections_interior()
{
	// Iterate over our internal sequence of sections that we built up by
	// visiting the topological sections of a topological polygon.
	const std::size_t num_sections = d_resolved_network.d_sections_interior.size();

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
		process_topological_section_intersection_interior(1/*section_index*/, true/*two_sections*/);
		return;
	}

	// Iterate over the sections and process intersections between each section
	// and its previous neighbour.
	for (std::size_t section_index = 0; section_index < num_sections; ++section_index)
	{
		process_topological_section_intersection_interior(section_index);
	}
}

void
GPlatesAppLogic::TopologyNetworkResolver::process_topological_section_intersection_interior(
		const std::size_t current_section_index,
		const bool two_sections)
{
	//
	// Intersect the current section with the previous section.
	//

	const std::size_t num_sections = d_resolved_network.d_sections_interior.size();

	ResolvedNetwork::Section &current_section =
			d_resolved_network.d_sections_interior[current_section_index];

	//
	// NOTE: We don't get the start intersection geometry from the GpmlTopologicalIntersection
	// - instead we get the geometry from the previous section in the topological polygon's
	// list of sections whose valid time ranges include the current reconstruction time.
	//

	const std::size_t prev_section_index = (current_section_index == 0)
			? num_sections - 1
			: current_section_index - 1;

	ResolvedNetwork::Section &prev_section =
			d_resolved_network.d_sections_interior[prev_section_index];

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
	if (prev_section.d_source_rfg.get() == current_section.d_source_rfg.get())
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
GPlatesAppLogic::TopologyNetworkResolver::assign_boundary_segments_boundary()
{
	// Make sure all the boundary segments have been found.
	// It is an error in the code (not in the data) if this is not the case.
	const std::size_t num_sections = d_resolved_network.d_sections.size();
	for (std::size_t section_index = 0; section_index < num_sections; ++section_index)
	{
		assign_boundary_segment_boundary(section_index);
	}
}

void
GPlatesAppLogic::TopologyNetworkResolver::assign_boundary_segment_boundary(
		const std::size_t section_index)
{
	ResolvedNetwork::Section &section = d_resolved_network.d_sections[section_index];

	// See if the reverse flag has been set by intersection processing - this
	// happens if the visible section intersected both its neighbours otherwise it just
	// returns the flag we passed it.
	section.d_use_reverse = section.d_intersection_results.get_reverse_flag(section.d_use_reverse);

	section.d_final_boundary_segment_unreversed_geom =
			section.d_intersection_results.get_unreversed_boundary_segment(section.d_use_reverse);
}

void
GPlatesAppLogic::TopologyNetworkResolver::assign_boundary_segments_interior()
{
	// Make sure all the boundary segments have been found.
	// It is an error in the code (not in the data) if this is not the case.
	const std::size_t num_sections = d_resolved_network.d_sections_interior.size();
	for (std::size_t section_index = 0; section_index < num_sections; ++section_index)
	{
		assign_boundary_segment_interior(section_index);
	}
}

void
GPlatesAppLogic::TopologyNetworkResolver::assign_boundary_segment_interior(
		const std::size_t section_index)
{
	ResolvedNetwork::Section &section = d_resolved_network.d_sections_interior[section_index];

	// See if the reverse flag has been set by intersection processing - this
	// happens if the visible section intersected both its neighbours otherwise it just
	// returns the flag we passed it.
	section.d_use_reverse = section.d_intersection_results.get_reverse_flag(section.d_use_reverse);

	section.d_final_boundary_segment_unreversed_geom =
			section.d_intersection_results.get_unreversed_boundary_segment(section.d_use_reverse);
}


// Final Creation Step
void
GPlatesAppLogic::TopologyNetworkResolver::create_resolved_topology_network(
		GPlatesModel::FeatureHandle &feature_handle)
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

	// Points per section; temporary list used in the for loop below 
	std::vector<GPlatesMaths::PointOnSphere> section_points;

	// Points from a set of non-intersecting lines
	std::vector<GPlatesMaths::PointOnSphere> group_of_related_points;

	// Points from multple single point sections 
	std::vector<GPlatesMaths::PointOnSphere> scattered_points;

	// seed points filled below , per section
	std::vector<GPlatesMaths::PointOnSphere> all_seed_points;

	// Sequence of boundary subsegments of resolved topology boundary.
	std::vector<ResolvedTopologicalBoundarySubSegment> boundary_subsegments;

	// Sequence of subsegments of resolved topology used when creating ResolvedTopologicalNetwork.
	// See the code in:
	// GPlatesAppLogic::TopologyUtils::query_resolved_topology_networks_for_interpolation
	std::vector<ResolvedTopologicalNetwork::Node> output_nodes;

	// Any interior section that is a polygon - these are regions that are inside the network but are
	// not part of the network (ie, not triangulated) and hence are effectively outside the network.
	std::vector<ResolvedTopologicalNetwork::InteriorPolygon> interior_polygons;

// qDebug() << "\ncreate_resolved_topology_network: Loop over BOUNDARY d_resolved_network.d_sections\n";
	//
	// Iterate over the sections of the resolved boundary and construct
	// the resolved polygon boundary and its subsegments.
	//
	ResolvedNetwork::section_seq_type::const_iterator section_iter =
			d_resolved_network.d_sections.begin();
	ResolvedNetwork::section_seq_type::const_iterator section_end =
			d_resolved_network.d_sections.end();
	for ( ; section_iter != section_end; ++section_iter)
	{
		const ResolvedNetwork::Section &section = *section_iter;

		# if 0
		debug_output_topological_section(section);
		#endif

		// clear out the old list of points
		section_points.clear();

		// It's possible for a valid segment to not contribute to the boundary
		// of the plate polygon. This can happen if it contributes zero-length
		// to the plate boundary which happens when both its neighbouring
		// boundary sections intersect it at the same point.
		if (!section.d_final_boundary_segment_unreversed_geom)
		{
			continue; // to next section in topology network
		}

		// Get the subsegment feature reference.
		const GPlatesModel::FeatureHandle::weak_ref subsegment_feature_ref =
				section.d_source_rfg->get_feature_ref();
		const GPlatesModel::FeatureHandle::const_weak_ref subsegment_feature_const_ref(subsegment_feature_ref);

		// Create a subsegment structure that'll get used when
		// creating the boundary of the resolved topological geometry.
		const ResolvedTopologicalBoundarySubSegment boundary_subsegment(
				section.d_final_boundary_segment_unreversed_geom.get(),
				subsegment_feature_const_ref,
				section.d_use_reverse);
		boundary_subsegments.push_back(boundary_subsegment);

		// Create a subsegment structure that'll get used when
		// creating the resolved topological geometry.
		const ResolvedTopologicalNetwork::Node output_node(
				section.d_final_boundary_segment_unreversed_geom.get(),
				subsegment_feature_const_ref);
		output_nodes.push_back(output_node);

		//
		// First, check if this is a seed point type section 
		//
		if ( section.d_is_seed_point ) 
		{
			// Save the subsegment geom as a seed point
			GPlatesAppLogic::GeometryUtils::get_geometry_points(
				*section.d_final_boundary_segment_unreversed_geom.get(),
				all_seed_points,
				section.d_use_reverse);

			continue; // to next section in topology
		}
		else
		{
			// Append the subsegment geometry to the total network points.
			GPlatesAppLogic::GeometryUtils::get_geometry_points(
					*section.d_final_boundary_segment_unreversed_geom.get(),
					all_network_points,
					section.d_use_reverse);

			//
			// Determine the subsegments orginal geometry type 
			//
			GPlatesFeatureVisitors::GeometryTypeFinder geometry_type_finder;
 			geometry_type_finder.visit_feature( subsegment_feature_ref );

//qDebug() << "geom_type_finder: points :" << geometry_type_finder.num_point_geometries_found();
//qDebug() << "geom_type_finder: mpoints:" << geometry_type_finder.num_multi_point_geometries_found();
//qDebug() << "geom_type_finder: lines  :" << geometry_type_finder.num_polyline_geometries_found();
//qDebug() << "geom_type_finder: polygon:" << geometry_type_finder.num_polygon_geometries_found();

			//
			// Determine how to add the subsegment's points to the triangulation
			//

			// Check for single point
			if ( geometry_type_finder.num_point_geometries_found() > 0)
			{
				// this is probably one of a collection of points; 
				// save and add to constrained triangulation later
				GPlatesAppLogic::GeometryUtils::get_geometry_points(
					*section.d_final_boundary_segment_unreversed_geom.get(),
					//scattered_points,
					boundary_points,
					section.d_use_reverse);
				//qDebug() << "section_points.size(): " << section_points.size();

				continue; // to next section of the topology
			}
			// Check multipoint 
			else if ( geometry_type_finder.num_multi_point_geometries_found() > 0)
			{
				// this is a single multi point feature section
				GPlatesAppLogic::GeometryUtils::get_geometry_points(
						*section.d_final_boundary_segment_unreversed_geom.get(),
						//section_points,
						boundary_points,
						section.d_use_reverse);
				//qDebug() << "section_points.size(): " << section_points.size();

#if 0
				// 2D + C
				// add multipoint with all connections between points contrained 
				GPlatesAppLogic::CgalUtils::insert_points_into_constrained_delaunay_triangulation_2(
					*constrained_delaunay_triangulation_2, 
					section_points.begin(), 
					section_points.end(),
					true);
#endif

				continue; // to next section of the topology
			}
			// Check for polyline geometry
			else if ( geometry_type_finder.num_polyline_geometries_found() > 0)
			{
				if ( !section.d_start_intersection && !section.d_start_intersection )
				{
					//qDebug() << "if ( !section.d_start_intersection && !section.d_start_intersection )";

					// this is probably an isolated non-intersecting line on the boundary
					// save and add to constrained triangulation later
					GPlatesAppLogic::GeometryUtils::get_geometry_points(
						*section.d_final_boundary_segment_unreversed_geom.get(),
						//group_of_related_points,
						boundary_points,
						section.d_use_reverse);
					//qDebug() << "non intersect section_points.size(): " << section_points.size();
				}
				else
				{
					// this is a single line feature, possibly clipped by intersections
					GPlatesAppLogic::GeometryUtils::get_geometry_points(
							*section.d_final_boundary_segment_unreversed_geom.get(),
							//section_points,
							boundary_points,
							section.d_use_reverse);
					//qDebug() << "section_points.size(): " << section_points.size();

#if 0
					// 2D + C
					// add as a contrained line segment ; do not contrain begin and end
					GPlatesAppLogic::CgalUtils::insert_points_into_constrained_delaunay_triangulation_2(
						*constrained_delaunay_triangulation_2, 
						section_points.begin(), 
						section_points.end(),
						false);
#endif
				}

				continue; // to next section of the topology
			}
			// Check for polygon geometry
			else if ( geometry_type_finder.num_polygon_geometries_found() > 0)
			{
				// this is a single polygon feature
				GPlatesAppLogic::GeometryUtils::get_geometry_points(
						*section.d_final_boundary_segment_unreversed_geom.get(),
						//section_points,
						boundary_points,
						section.d_use_reverse);
				//qDebug() << "section_points.size(): " << section_points.size();
				
#if 0
				// 2D + C
				// add as a contrained line segment ; do contrain begin and end
				GPlatesAppLogic::CgalUtils::insert_points_into_constrained_delaunay_triangulation_2(
					*constrained_delaunay_triangulation_2, 
					section_points.begin(), 
					section_points.end(),
					true);
#endif

				continue; // to next section of the topology
			}

		} // end of if (seed) / else ( add geom to triangulation )

	} // end of loop over sections

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

// qDebug() << "\ncreate_resolved_topology_network: Loop over INTERIOR d_resolved_network.d_sections_interior\n";

	// 
	// Iterate over the sections of the interior 
	//
	section_iter = d_resolved_network.d_sections_interior.begin();
	section_end = d_resolved_network.d_sections_interior.end();
	for ( ; section_iter != section_end; ++section_iter)
	{
		const ResolvedNetwork::Section &section = *section_iter;

		#if 0
		debug_output_topological_section(section);
		#endif

		// clear out the old list of points
		section_points.clear();

		// It's possible for a valid segment to not contribute to the boundary
		// of the plate polygon. This can happen if it contributes zero-length
		// to the plate boundary which happens when both its neighbouring
		// boundary sections intersect it at the same point.
		if (!section.d_final_boundary_segment_unreversed_geom)
		{
			continue; // to next section in topology network
		}

		// Get the subsegment feature reference.
		const GPlatesModel::FeatureHandle::weak_ref subsegment_feature_ref =
				section.d_source_rfg->get_feature_ref();
		const GPlatesModel::FeatureHandle::const_weak_ref subsegment_feature_const_ref(subsegment_feature_ref);

		// Create a subsegment structure that'll get used when
		// creating the resolved topological geometry.
		const ResolvedTopologicalNetwork::Node output_node(
				section.d_final_boundary_segment_unreversed_geom.get(),
				subsegment_feature_const_ref);
		output_nodes.push_back(output_node);

		//
		// First, check if this is a seed point type section 
		//
		if ( section.d_is_seed_point ) 
		{
			// Save the subsegment geom as a seed point
			GPlatesAppLogic::GeometryUtils::get_geometry_points(
				*section.d_final_boundary_segment_unreversed_geom.get(),
				all_seed_points,
				section.d_use_reverse);

			continue; // to next section in topology
		}
		else
		{
			// Append the subsegment geometry to the total network points.
			GPlatesAppLogic::GeometryUtils::get_geometry_points(
					*section.d_final_boundary_segment_unreversed_geom.get(),
					all_network_points,
					section.d_use_reverse);

			// Keep track of any interior polygon regions.
			// These will be needed for calculating velocities since they are not part of the triangulation
			// generated (velocities will be calculated in the normal manner for static polygons).
			GPlatesFeatureVisitors::GeometryTypeFinder geometry_type_finder;
			section.d_source_rfg->reconstructed_geometry()->accept_visitor(geometry_type_finder);
			if (geometry_type_finder.num_polygon_geometries_found() > 0)
			{
				// Create a subsegment structure that'll get used when
				// creating the resolved topological geometry.
				const ResolvedTopologicalNetwork::InteriorPolygon interior_polygon(section.d_source_rfg);
				interior_polygons.push_back(interior_polygon);
			}
			geometry_type_finder.clear();

			//
			// Determine the subsegments orginal geometry type 
			//
 			geometry_type_finder.visit_feature( subsegment_feature_ref );

//qDebug() << "geom_type_finder: points :" << geometry_type_finder.num_point_geometries_found();
//qDebug() << "geom_type_finder: mpoints:" << geometry_type_finder.num_multi_point_geometries_found();
//qDebug() << "geom_type_finder: lines  :" << geometry_type_finder.num_polyline_geometries_found();
//qDebug() << "geom_type_finder: polygon:" << geometry_type_finder.num_polygon_geometries_found();

			// Check for single point
			if ( geometry_type_finder.num_point_geometries_found() > 0)
			{
				// this is probably one of a collection of points; 
				// save and add to constrained triangulation later
				GPlatesAppLogic::GeometryUtils::get_geometry_points(
					*section.d_final_boundary_segment_unreversed_geom.get(),
					scattered_points,
					section.d_use_reverse);
				//qDebug() << "section_points.size(): " << section_points.size();

				continue; // to next section of the topology
			}
			// Check multipoint 
			else if ( geometry_type_finder.num_multi_point_geometries_found() > 0)
			{
				// this is a single multi point feature section
				GPlatesAppLogic::GeometryUtils::get_geometry_points(
						*section.d_final_boundary_segment_unreversed_geom.get(),
						section_points,
						section.d_use_reverse);
				//qDebug() << "section_points.size(): " << section_points.size();

				// 2D + C
				// add multipoint with all connections between points contrained 
				GPlatesAppLogic::CgalUtils::insert_points_into_constrained_delaunay_triangulation_2(
					*constrained_delaunay_triangulation_2, 
					section_points.begin(), 
					section_points.end(),
					true);

				continue; // to next section of the topology
			}
			// Check for polyline geometry
			else if ( geometry_type_finder.num_polyline_geometries_found() > 0)
			{
				if ( !section.d_start_intersection && !section.d_start_intersection )
				{
					//qDebug() << "if ( !section.d_start_intersection && !section.d_start_intersection )";

					// this is probably an isolated non-intersecting line on the boundary
					// or inside the region
					// save and add to constrained triangulation later
					GPlatesAppLogic::GeometryUtils::get_geometry_points(
						*section.d_final_boundary_segment_unreversed_geom.get(),
						section_points,
						section.d_use_reverse);
					//qDebug() << "non intersect section_points.size(): " << section_points.size();

					// 2D + C
					// add as a contrained line segment ; do not contrain begin and end
					GPlatesAppLogic::CgalUtils::insert_points_into_constrained_delaunay_triangulation_2(
						*constrained_delaunay_triangulation_2, 
						section_points.begin(), 
						section_points.end(),
						false);
				}
				else
				{
					// this is a single line feature, possibly clipped by intersections
					GPlatesAppLogic::GeometryUtils::get_geometry_points(
							*section.d_final_boundary_segment_unreversed_geom.get(),
							section_points,
							section.d_use_reverse);
					//qDebug() << "section_points.size(): " << section_points.size();

					// 2D + C
					// add as a contrained line segment ; do not contrain begin and end
					GPlatesAppLogic::CgalUtils::insert_points_into_constrained_delaunay_triangulation_2(
						*constrained_delaunay_triangulation_2, 
						section_points.begin(), 
						section_points.end(),
						false);
				}

				continue; // to next section of the topology
			}
			// Check for polygon geometry
			else if ( geometry_type_finder.num_polygon_geometries_found() > 0)
			{
				// this is a single polygon feature
				GPlatesAppLogic::GeometryUtils::get_geometry_points(
						*section.d_final_boundary_segment_unreversed_geom.get(),
						section_points,
						section.d_use_reverse);
				//qDebug() << "section_points.size(): " << section_points.size();
				
				// 2D + C
				// add as a contrained line segment ; do contrain begin and end
				GPlatesAppLogic::CgalUtils::insert_points_into_constrained_delaunay_triangulation_2(
					*constrained_delaunay_triangulation_2, 
					section_points.begin(), 
					section_points.end(),
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
						all_seed_points.push_back( section_points.back() );
					}
				}
				else
				{
					qDebug() << "====> property rigidBlock = NOT FOUND";
				}
#endif

				continue; // to next section of the topology
			}

		} // end of if (seed) / else ( add geom to triangulation )

	} // end of loop over sections


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
					feature_handle,
					*d_topological_polygon_feature_iterator,
					output_nodes.begin(),
					output_nodes.end(),
					boundary_subsegments.begin(),
					boundary_subsegments.end(),
					boundary_polygon.get(),
					interior_polygons.begin(),
					interior_polygons.end(),
					d_reconstruction_params.get_recon_plate_id(),
					d_reconstruction_params.get_time_of_appearance());

	d_resolved_topological_networks.push_back(network);
}



void
GPlatesAppLogic::TopologyNetworkResolver::debug_output_topological_section(
		const ResolvedNetwork::Section &section)
{
	QString s;

	// get the fearture id
	const GPlatesModel::FeatureId &feature_id = section.d_source_feature_id;
	// get the fearture ref
	std::vector<GPlatesModel::FeatureHandle::weak_ref> back_ref_targets;
	feature_id.find_back_ref_targets(GPlatesModel::append_as_weak_refs(back_ref_targets));
	GPlatesModel::FeatureHandle::weak_ref feature_ref = back_ref_targets.front();

	// get the name
	s.append ( "SECTION name = '" );
	static const GPlatesModel::PropertyName prop = GPlatesModel::PropertyName::create_gml("name");
 	const GPlatesPropertyValues::XsString *name;
 	if ( GPlatesFeatureVisitors::get_property_value(feature_ref, prop, name) ) {
		s.append(GPlatesUtils::make_qstring( name->value() ));
 	} else {
 		s.append("UNKNOWN");
 	}
 	s.append("'; id = ");
	s.append ( GPlatesUtils::make_qstring_from_icu_string( feature_id.get() ) );

	qDebug() << s;
}

GPlatesAppLogic::TopologyNetworkResolver::ResolvedNetwork::Section::Section(
		const GPlatesModel::FeatureId &source_feature_id,
		const reconstructed_feature_geometry_non_null_ptr_type &source_rfg) :
	d_source_feature_id(source_feature_id),
	d_source_rfg(source_rfg),
	d_use_reverse(false),
	d_is_seed_point(false),
	d_is_point(false),
	d_is_line(false),
	d_is_polygon(false),
	d_intersection_results(source_rfg->reconstructed_geometry())
{
}
