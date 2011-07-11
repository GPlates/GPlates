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

#include <cstddef> // For std::size_t
#include <vector>
#include <boost/foreach.hpp>
#include <QDebug>

#include "TopologyBoundaryResolver.h"

#include "GeometryUtils.h"
#include "ReconstructionGeometryUtils.h"
#include "ReconstructedFeatureGeometry.h"
#include "Reconstruction.h"
#include "ResolvedTopologicalBoundary.h"
#include "TopologyInternalUtils.h"
#include "TopologyUtils.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "maths/GeometryOnSphere.h"

#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlPiecewiseAggregation.h"
#include "property-values/GpmlTopologicalPolygon.h"
#include "property-values/GpmlTopologicalSection.h"
#include "property-values/GpmlTopologicalLineSection.h"
#include "property-values/GpmlTopologicalIntersection.h"
#include "property-values/GpmlTopologicalPoint.h"

#include "utils/GeometryCreationUtils.h"
#include "utils/Profile.h"
#include "utils/UnicodeStringUtils.h"


// Create a ReconstructedFeatureGeometry for the rotated reference points in each
// topological polygon.
// NOTE: This is no longer needed since the rotated reference points are no longer
// used to choose which partitioned segments contribute to the plate boundary as
// so the user no longer needs to see these points on the globe.
#if 0
#define CREATE_RFG_FOR_ROTATED_REFERENCE_POINTS
#endif


GPlatesAppLogic::TopologyBoundaryResolver::TopologyBoundaryResolver(
		std::vector<ResolvedTopologicalBoundary::non_null_ptr_type> &resolved_topological_boundaries,
		const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree,
		boost::optional<const std::vector<ReconstructHandle::type> &> topological_sections_reconstruct_handles) :
	d_resolved_topological_boundaries(resolved_topological_boundaries),
	d_reconstruction_tree(reconstruction_tree),
	d_topological_sections_reconstruct_handles(topological_sections_reconstruct_handles),
	d_reconstruction_params(reconstruction_tree->get_reconstruction_time())
{  
	d_num_topologies = 0;
}


bool
GPlatesAppLogic::TopologyBoundaryResolver::initialise_pre_feature_properties(
		GPlatesModel::FeatureHandle &feature_handle)
{
	// super short-cut for features without boundary list properties
	if (!TopologyUtils::is_topological_closed_plate_boundary_feature(feature_handle))
	{ 
		// Quick-out: No need to continue.
		return false; 
	}

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

	// Now visit each of the properties in turn.
	return true;
}


void
GPlatesAppLogic::TopologyBoundaryResolver::visit_gpml_constant_value(
		GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	gpml_constant_value.value()->accept_visitor(*this);
}


void
GPlatesAppLogic::TopologyBoundaryResolver::visit_gpml_piecewise_aggregation(
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
GPlatesAppLogic::TopologyBoundaryResolver::visit_gpml_time_window(
		GPlatesPropertyValues::GpmlTimeWindow &gpml_time_window)
{
	gpml_time_window.time_dependent_value()->accept_visitor(*this);
	gpml_time_window.valid_time()->accept_visitor(*this);
}


void
GPlatesAppLogic::TopologyBoundaryResolver::visit_gpml_topological_polygon(
		GPlatesPropertyValues::GpmlTopologicalPolygon &gpml_topological_polygon)
{
	PROFILE_FUNC();

	// Prepare for a new topological polygon.
	d_resolved_boundary.reset();

	//
	// Visit the topological sections to gather needed information and store
	// it internally in 'd_resolved_boundary'.
	//
	record_topological_sections(gpml_topological_polygon.sections());

	//
	// See if the topological section 'start' and 'end' intersections are consistent.
	//
	validate_topological_section_intersections();

	//
	// Now iterate over our internal structure 'd_resolved_boundary' and
	// intersect neighbouring sections that require it and
	// generate the resolved boundary subsegments.
	//
	process_topological_section_intersections();

	//
	// Now iterate over the intersection results and assign boundary segments to
	// each section.
	//
	assign_boundary_segments();

	//
	// Now create the ResolvedTopologicalBoundary.
	//
	create_resolved_topology_boundary();
}


void
GPlatesAppLogic::TopologyBoundaryResolver::record_topological_sections(
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
GPlatesAppLogic::TopologyBoundaryResolver::visit_gpml_topological_line_section(
		GPlatesPropertyValues::GpmlTopologicalLineSection &gpml_toplogical_line_section)
{  
	const GPlatesModel::FeatureId source_feature_id =
			gpml_toplogical_line_section.get_source_geometry()->feature_id();

	boost::optional<ResolvedBoundary::Section> section =
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

		section->d_start_intersection = ResolvedBoundary::Intersection(reference_point);
	}

	// Record end intersection information.
	if (gpml_toplogical_line_section.get_end_intersection())
	{
		const GPlatesMaths::PointOnSphere reference_point =
				*gpml_toplogical_line_section.get_end_intersection()->reference_point()->point();

		section->d_end_intersection = ResolvedBoundary::Intersection(reference_point);
	}

	// Add to internal sequence.
	d_resolved_boundary.d_sections.push_back(*section);
}


void
GPlatesAppLogic::TopologyBoundaryResolver::visit_gpml_topological_point(
		GPlatesPropertyValues::GpmlTopologicalPoint &gpml_toplogical_point)
{  
	const GPlatesModel::FeatureId source_feature_id =
			gpml_toplogical_point.get_source_geometry()->feature_id();

	boost::optional<ResolvedBoundary::Section> section =
			record_topological_section_reconstructed_geometry(
					source_feature_id,
					*gpml_toplogical_point.get_source_geometry());
	if (!section)
	{
		// Return without adding topological section to the list of boundary sections.
		return;
	}

	// No other information to collect since this topological section is a point and
	// hence cannot intersect with neighbouring sections.

	// Add to internal sequence.
	d_resolved_boundary.d_sections.push_back(*section);
}


boost::optional<GPlatesAppLogic::TopologyBoundaryResolver::ResolvedBoundary::Section>
GPlatesAppLogic::TopologyBoundaryResolver::record_topological_section_reconstructed_geometry(
		const GPlatesModel::FeatureId &source_feature_id,
		const GPlatesPropertyValues::GpmlPropertyDelegate &geometry_delegate)
{
	// Get the reconstructed geometry of the topological section's delegate.
	// The referenced RFGs must be in our sequence of reconstructed topological boundary sections.
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
	if (!source_rfg)
	{
		// If no RFG was found then it's possible that the current reconstruction time is
		// outside the age range of the feature this section is referencing.
		// This is ok - it's not necessarily an error.
		// We just won't add it to the list of boundary sections. This means either:
		//  - rubber banding will occur between the two sections adjacent to this section
		//    since this section is now missing, or
		//  - one of the adjacent sections did not exist until just now (because of its age range)
		//    and now it is popping in to replace the current section which is disappearing (an
		//    example of this is a bunch of sections that are mid-ocean ridge features that do not
		//    overlap in time and represent different geometries, from isochrons, of the same ridge).
		return boost::none;
	}

	// Store the feature id and RFG.
	return ResolvedBoundary::Section(source_feature_id, *source_rfg);
}


void
GPlatesAppLogic::TopologyBoundaryResolver::validate_topological_section_intersections()
{
	// Iterate over our internal sequence of sections that we built up by
	// visiting the topological sections of a topological polygon.
	const std::size_t num_sections = d_resolved_boundary.d_sections.size();
	for (std::size_t section_index = 0; section_index < num_sections; ++section_index)
	{
		validate_topological_section_intersection(section_index);
	}
}


void
GPlatesAppLogic::TopologyBoundaryResolver::validate_topological_section_intersection(
		const std::size_t current_section_index)
{
	const std::size_t num_sections = d_resolved_boundary.d_sections.size();

	const ResolvedBoundary::Section &current_section =
			d_resolved_boundary.d_sections[current_section_index];

	// If the current section has a 'start' intersection then the previous section
	// should have an 'end' intersection.
	if (current_section.d_start_intersection)
	{
		const std::size_t prev_section_index = (current_section_index == 0)
				? num_sections - 1
				: current_section_index - 1;
		const ResolvedBoundary::Section &prev_section =
				d_resolved_boundary.d_sections[prev_section_index];

		if (!prev_section.d_end_intersection)
		{
			qDebug() << "ERROR: Validate failure for GpmlTopologicalPolygon.";
			qDebug() << "If a GpmlTopologicalSection has a start intersection then "
					"the previous GpmlTopologicalSection should have an end intersection.";
			debug_output_topological_section_feature_id(prev_section.d_source_feature_id);
		}
	}

	// If the current section has an 'end' intersection then the next section
	// should have a 'start' intersection.
	if (current_section.d_end_intersection)
	{
		const std::size_t next_section_index = (current_section_index == num_sections - 1)
				? 0
				: current_section_index + 1;
		const ResolvedBoundary::Section &next_section =
				d_resolved_boundary.d_sections[next_section_index];

		if (!next_section.d_start_intersection)
		{
			qDebug() << "ERROR: Validate failure for GpmlTopologicalPolygon.";
			qDebug() << "If a GpmlTopologicalSection has an end intersection then "
					"the next GpmlTopologicalSection should have a start intersection.";
			debug_output_topological_section_feature_id(next_section.d_source_feature_id);
		}
	}
}


void
GPlatesAppLogic::TopologyBoundaryResolver::process_topological_section_intersections()
{
	// Iterate over our internal sequence of sections that we built up by
	// visiting the topological sections of a topological polygon.
	const std::size_t num_sections = d_resolved_boundary.d_sections.size();

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
		process_topological_section_intersection(1/*section_index*/, true/*two_sections*/);
		return;
	}

	// Iterate over the sections and process intersections between each section
	// and its previous neighbour.
	for (std::size_t section_index = 0; section_index < num_sections; ++section_index)
	{
		process_topological_section_intersection(section_index);
	}
}


void
GPlatesAppLogic::TopologyBoundaryResolver::process_topological_section_intersection(
		const std::size_t current_section_index,
		const bool two_sections)
{
	//
	// Intersect the current section with the previous section.
	//

	const std::size_t num_sections = d_resolved_boundary.d_sections.size();

	ResolvedBoundary::Section &current_section =
			d_resolved_boundary.d_sections[current_section_index];

	//
	// NOTE: We don't get the start intersection geometry from the GpmlTopologicalIntersection
	// - instead we get the geometry from the previous section in the topological polygon's
	// list of sections whose valid time ranges include the current reconstruction time.
	//

	const std::size_t prev_section_index = (current_section_index == 0)
			? num_sections - 1
			: current_section_index - 1;

	ResolvedBoundary::Section &prev_section =
			d_resolved_boundary.d_sections[prev_section_index];

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
GPlatesAppLogic::TopologyBoundaryResolver::assign_boundary_segments()
{
	// Make sure all the boundary segments have been found.
	// It is an error in the code (not in the data) if this is not the case.
	const std::size_t num_sections = d_resolved_boundary.d_sections.size();
	for (std::size_t section_index = 0; section_index < num_sections; ++section_index)
	{
		assign_boundary_segment(section_index);
	}
}


void
GPlatesAppLogic::TopologyBoundaryResolver::assign_boundary_segment(
		const std::size_t section_index)
{
	ResolvedBoundary::Section &section = d_resolved_boundary.d_sections[section_index];

	// See if the reverse flag has been set by intersection processing - this
	// happens if the visible section intersected both its neighbours otherwise it just
	// returns the flag we passed it.
	section.d_use_reverse = section.d_intersection_results.get_reverse_flag(section.d_use_reverse);

	section.d_final_boundary_segment_unreversed_geom =
			section.d_intersection_results.get_unreversed_boundary_segment(section.d_use_reverse);
}


// Final Creation Step
void
GPlatesAppLogic::TopologyBoundaryResolver::create_resolved_topology_boundary()
{
	PROFILE_FUNC();

	// The points to create the plate polygon with.
	std::vector<GPlatesMaths::PointOnSphere> polygon_points;

#if defined(CREATE_RFG_FOR_ROTATED_REFERENCE_POINTS)
	// The rotated reference points for any intersecting sections.
	std::vector<GPlatesMaths::PointOnSphere> rotated_reference_points;
#endif

	// Sequence of subsegments of resolved topology used when creating ResolvedTopologicalBoundary.
	std::vector<ResolvedTopologicalBoundary::SubSegment> output_subsegments;

	// Iterate over the sections of the resolved boundary and construct
	// the resolved polygon boundary and its subsegments.
	ResolvedBoundary::section_seq_type::const_iterator section_iter =
			d_resolved_boundary.d_sections.begin();
	const ResolvedBoundary::section_seq_type::const_iterator section_end =
			d_resolved_boundary.d_sections.end();
	for ( ; section_iter != section_end; ++section_iter)
	{
		const ResolvedBoundary::Section &section = *section_iter;

		// It's possible for a valid segment to not contribute to the boundary
		// of the plate polygon. This can happen if it contributes zero-length
		// to the plate boundary which happens when both its neighbouring
		// boundary sections intersect it at the same point.
		if (!section.d_final_boundary_segment_unreversed_geom)
		{
			continue;
		}

		// Get the subsegment feature reference.
		const GPlatesModel::FeatureHandle::weak_ref subsegment_feature_ref =
				section.d_source_rfg->get_feature_ref();
		const GPlatesModel::FeatureHandle::const_weak_ref subsegment_feature_const_ref(subsegment_feature_ref);

		// Create a subsegment structure that'll get used when
		// creating the resolved topological geometry.
		const ResolvedTopologicalBoundary::SubSegment output_subsegment(
				section.d_final_boundary_segment_unreversed_geom.get(),
				subsegment_feature_const_ref,
				section.d_use_reverse);
		output_subsegments.push_back(output_subsegment);

		// Append the subsegment geometry to the plate polygon points.
		GPlatesAppLogic::GeometryUtils::get_geometry_points(
				*section.d_final_boundary_segment_unreversed_geom.get(),
				polygon_points,
				section.d_use_reverse);

#if defined(CREATE_RFG_FOR_ROTATED_REFERENCE_POINTS)
		// If there are any intersections then record the rotated reference points
		// so we can create an RFG for them below.
		if (section.d_start_intersection &&
			section.d_start_intersection->d_reconstructed_reference_point)
		{
			rotated_reference_points.push_back(
					*section.d_start_intersection->d_reconstructed_reference_point);
		}
		if (section.d_end_intersection &&
			section.d_end_intersection->d_reconstructed_reference_point)
		{
			rotated_reference_points.push_back(
					*section.d_end_intersection->d_reconstructed_reference_point);
		}
#endif // if defined(CREATE_RFG_FOR_ROTATED_REFERENCE_POINTS)
	}

	// Create a polygon on sphere for the resolved boundary using 'polygon_points'.
	GPlatesUtils::GeometryConstruction::GeometryConstructionValidity polygon_validity;
	boost::optional<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> plate_polygon =
			GPlatesUtils::create_polygon_on_sphere(
					polygon_points.begin(), polygon_points.end(), polygon_validity);

	// If we are unable to create a polygon (such as insufficient points) then
	// just return without creating a resolved topological geometry.
	if (polygon_validity != GPlatesUtils::GeometryConstruction::VALID)
	{
		qDebug() << "ERROR: Failed to create a ResolvedTopologicalBoundary - probably has "
				"insufficient points for a polygon.";
		qDebug() << "Skipping creation for topological polygon feature_id=";
		qDebug() << GPlatesUtils::make_qstring_from_icu_string(
				d_currently_visited_feature->feature_id().get());
		return;
	}

	//
	// Create the RTB for the plate polygon.
	//
	ResolvedTopologicalBoundary::non_null_ptr_type rtb_ptr =
		ResolvedTopologicalBoundary::create(
			d_reconstruction_tree,
			*plate_polygon,
			*(current_top_level_propiter()->handle_weak_ref()),
			*(current_top_level_propiter()),
			output_subsegments.begin(),
			output_subsegments.end(),
			d_reconstruction_params.get_recon_plate_id(),
			d_reconstruction_params.get_time_of_appearance());

	d_resolved_topological_boundaries.push_back(rtb_ptr);

#if defined(CREATE_RFG_FOR_ROTATED_REFERENCE_POINTS)
	//
	// Create the RFG for the rotated reference points.
	//
	if (!rotated_reference_points.empty())
	{
		GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type rotated_reference_points_geom = 
				GPlatesMaths::MultiPointOnSphere::create_on_heap(rotated_reference_points);

		ReconstructedFeatureGeometry::non_null_ptr_type rotated_reference_points_rfg =
			ReconstructedFeatureGeometry::create(
				d_reconstruction_tree,
				rotated_reference_points_geom,
				*(current_top_level_propiter()->handle_weak_ref()),
				*(current_top_level_propiter()));

		d_resolved_topological_boundaries.push_back(rotated_reference_points_rfg);
	}
#endif // if defined(CREATE_RFG_FOR_ROTATED_REFERENCE_POINTS)
}


void
GPlatesAppLogic::TopologyBoundaryResolver::debug_output_topological_section_feature_id(
		const GPlatesModel::FeatureId &section_feature_id)
{
	qDebug() << "Topological polygon feature_id=";
	qDebug() << GPlatesUtils::make_qstring_from_icu_string(
			d_currently_visited_feature->feature_id().get());
	qDebug() << "Topological section referencing feature_id=";
	qDebug() << GPlatesUtils::make_qstring_from_icu_string(section_feature_id.get());
}
