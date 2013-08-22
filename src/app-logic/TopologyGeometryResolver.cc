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

#include "TopologyGeometryResolver.h"

#include "GeometryUtils.h"
#include "ReconstructedFeatureGeometry.h"
#include "Reconstruction.h"
#include "ReconstructionGeometryUtils.h"
#include "ResolvedTopologicalGeometry.h"
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
#include "property-values/GpmlTopologicalLine.h"
#include "property-values/GpmlTopologicalLineSection.h"
#include "property-values/GpmlTopologicalPoint.h"

#include "utils/GeometryCreationUtils.h"
#include "utils/Profile.h"
#include "utils/UnicodeStringUtils.h"

GPlatesAppLogic::TopologyGeometryResolver::TopologyGeometryResolver(
		std::vector<ResolvedTopologicalGeometry::non_null_ptr_type> &resolved_topological_geometries,
		const resolve_geometry_flags_type &resolve_geometry_flags,
		ReconstructHandle::type reconstruct_handle,
		const ReconstructionTreeCreator &reconstruction_tree_creator,
		const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree,
		boost::optional<const std::vector<ReconstructHandle::type> &> topological_sections_reconstruct_handles) :
	d_resolved_topological_geometries(resolved_topological_geometries),
	d_resolve_geometry_flags(resolve_geometry_flags),
	d_reconstruct_handle(reconstruct_handle),
	d_reconstruction_tree_creator(reconstruction_tree_creator),
	d_reconstruction_tree(reconstruction_tree),
	d_topological_sections_reconstruct_handles(topological_sections_reconstruct_handles),
	d_reconstruction_params(reconstruction_tree->get_reconstruction_time())
{  
}


bool
GPlatesAppLogic::TopologyGeometryResolver::initialise_pre_feature_properties(
		GPlatesModel::FeatureHandle &feature_handle)
{
	// NOTE: We don't test for topological feature types anymore.
	// If a feature has a topological polygon or topological line property then it will
	// get resolved, otherwise no reconstruction geometries will be generated.
	// We're not testing feature type because we're introducing the ability for any feature type
	// to allow a topological (or static) geometry property.
	// This will mean that some features, in a feature collection, that contain non-topological
	// geometries will be unnecessarily visited (but at least nothing meaningful will happen).

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
GPlatesAppLogic::TopologyGeometryResolver::visit_gpml_constant_value(
		GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	gpml_constant_value.value()->accept_visitor(*this);
}


void
GPlatesAppLogic::TopologyGeometryResolver::visit_gpml_piecewise_aggregation(
		GPlatesPropertyValues::GpmlPiecewiseAggregation &gpml_piecewise_aggregation)
{
	std::vector<GPlatesPropertyValues::GpmlTimeWindow> time_windows =
			gpml_piecewise_aggregation.get_time_windows();

	std::vector<GPlatesPropertyValues::GpmlTimeWindow>::iterator iter = time_windows.begin();
	std::vector<GPlatesPropertyValues::GpmlTimeWindow>::iterator end = time_windows.end();

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

	gpml_piecewise_aggregation.set_time_windows(time_windows);
}


void
GPlatesAppLogic::TopologyGeometryResolver::visit_gpml_time_window(
		GPlatesPropertyValues::GpmlTimeWindow &gpml_time_window)
{
	gpml_time_window.get_time_dependent_value()->accept_visitor(*this);
	gpml_time_window.get_valid_time()->accept_visitor(*this);
}


void
GPlatesAppLogic::TopologyGeometryResolver::visit_gpml_topological_polygon(
		GPlatesPropertyValues::GpmlTopologicalPolygon &gpml_topological_polygon)
{
	// Only resolve topological boundaries (polygons) if we've been requested to.
	if (!d_resolve_geometry_flags.test(RESOLVE_BOUNDARY))
	{
		return;
	}

	PROFILE_FUNC();

	// Prepare for a new topological polygon.
	d_resolved_geometry.reset();

	// Visiting a topological polygon property.
	d_current_resolved_geometry_type = RESOLVE_BOUNDARY;

	//
	// Visit the topological sections to gather needed information and store
	// it internally in 'd_resolved_geometry'.
	//
	GPlatesPropertyValues::GpmlTopologicalPolygon::sections_seq_type exterior_sections =
			gpml_topological_polygon.get_exterior_sections();
	record_topological_sections(exterior_sections.begin(), exterior_sections.end());
	gpml_topological_polygon.set_exterior_sections(exterior_sections);

	//
	// Now iterate over our internal structure 'd_resolved_geometry' and
	// intersect neighbouring sections that require it and
	// generate the resolved boundary subsegments.
	//
	process_resolved_boundary_topological_section_intersections();

	//
	// Now iterate over the intersection results and assign boundary sub-segments to
	// each section.
	//
	assign_segments();

	//
	// Now create the resolved topological boundary.
	//
	create_resolved_topological_boundary();

	// Finished visiting topological polygon property.
	d_current_resolved_geometry_type = boost::none;
}


void
GPlatesAppLogic::TopologyGeometryResolver::visit_gpml_topological_line(
		GPlatesPropertyValues::GpmlTopologicalLine &gpml_topological_line)
{
	// Only resolve topological lines if we've been requested to.
	if (!d_resolve_geometry_flags.test(RESOLVE_LINE))
	{
		return;
	}

	PROFILE_FUNC();

	// Prepare for a new topological line.
	d_resolved_geometry.reset();

	// Visiting a topological line property.
	d_current_resolved_geometry_type = RESOLVE_LINE;

	//
	// Visit the topological sections to gather needed information and store
	// it internally in 'd_resolved_geometry'.
	//
	GPlatesPropertyValues::GpmlTopologicalLine::sections_seq_type sections =
			gpml_topological_line.get_sections();
	record_topological_sections(sections.begin(), sections.end());
	gpml_topological_line.set_sections(sections);

	//
	// Now iterate over our internal structure 'd_resolved_geometry' and
	// intersect neighbouring sections that require it and
	// generate the resolved line subsegments.
	//
	process_resolved_line_topological_section_intersections();

	//
	// Now iterate over the intersection results and assign sub-segments to
	// each section.
	//
	assign_segments();

	//
	// Now create the resolved topological line.
	//
	create_resolved_topological_line();

	// Finished visiting topological line property.
	d_current_resolved_geometry_type = boost::none;
}


template <typename TopologicalSectionsIterator>
void
GPlatesAppLogic::TopologyGeometryResolver::record_topological_sections(
		const TopologicalSectionsIterator &sections_begin,
		const TopologicalSectionsIterator &sections_end)
{
	// loop over all the sections
	for (TopologicalSectionsIterator sections_iter = sections_begin;
		sections_iter != sections_end;
		++sections_iter)
	{
		GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type topological_section =
				sections_iter->get_source_section();

		topological_section->accept_visitor(*this);
	}
}


void
GPlatesAppLogic::TopologyGeometryResolver::visit_gpml_topological_line_section(
		GPlatesPropertyValues::GpmlTopologicalLineSection &gpml_topological_line_section)
{  
	const GPlatesModel::FeatureId source_feature_id =
			gpml_topological_line_section.source_geometry()->get_feature_id();

	boost::optional<ResolvedGeometry::Section> section =
			record_topological_section_reconstructed_geometry(
					source_feature_id,
					*gpml_topological_line_section.source_geometry());
	if (!section)
	{
		// Return without adding topological section to the list of sections.
		return;
	}

	// Set reverse flag.
	section->d_use_reverse = gpml_topological_line_section.get_reverse_order();

	// Add to internal sequence.
	d_resolved_geometry.d_sections.push_back(*section);
}


void
GPlatesAppLogic::TopologyGeometryResolver::visit_gpml_topological_point(
		GPlatesPropertyValues::GpmlTopologicalPoint &gpml_toplogical_point)
{  
	const GPlatesModel::FeatureId source_feature_id =
			gpml_toplogical_point.source_geometry()->get_feature_id();

	boost::optional<ResolvedGeometry::Section> section =
			record_topological_section_reconstructed_geometry(
					source_feature_id,
					*gpml_toplogical_point.source_geometry());
	if (!section)
	{
		// Return without adding topological section to the list of sections.
		return;
	}

	// No other information to collect since this topological section is a point and
	// hence cannot intersect with neighbouring sections.

	// Add to internal sequence.
	d_resolved_geometry.d_sections.push_back(*section);
}


boost::optional<GPlatesAppLogic::TopologyGeometryResolver::ResolvedGeometry::Section>
GPlatesAppLogic::TopologyGeometryResolver::record_topological_section_reconstructed_geometry(
		const GPlatesModel::FeatureId &source_feature_id,
		const GPlatesPropertyValues::GpmlPropertyDelegate &geometry_delegate)
{
	// Get the reconstructed geometry of the topological section's delegate.
	// The referenced RGs must be in our sequence of reconstructed/resolved topological sections.
	// If we need to restrict the topological section RGs to specific reconstruct handles...
	boost::optional<const std::vector<ReconstructHandle::type> &> topological_sections_reconstruct_handles;
	if (d_topological_sections_reconstruct_handles)
	{
		topological_sections_reconstruct_handles = d_topological_sections_reconstruct_handles.get();
	}
	// Find the topological section reconstruction geometry.
	boost::optional<ReconstructionGeometry::non_null_ptr_type> source_rg =
			TopologyInternalUtils::find_topological_reconstruction_geometry(
					geometry_delegate,
					d_reconstruction_tree->get_reconstruction_time(),
					topological_sections_reconstruct_handles);
	if (!source_rg)
	{
		// If no RG was found then it's possible that the current reconstruction time is
		// outside the age range of the feature this section is referencing.
		// This is ok - it's not necessarily an error.
		// We just won't add it to the list of sections. This means either:
		//  - rubber banding will occur between the two sections adjacent to this section
		//    since this section is now missing, or
		//  - one of the adjacent sections did not exist until just now (because of its age range)
		//    and now it is popping in to replace the current section which is disappearing (an
		//    example of this is a bunch of sections that are mid-ocean ridge features that do not
		//    overlap in time and represent different geometries, from isochrons, of the same ridge).
		return boost::none;
	}

	//
	// Currently, topological sections can only be reconstructed feature geometries
	// (for resolved lines) and/or resolved topological *lines* (for resolved boundaries).
	//

	// See if topological section is a reconstructed feature geometry (or any of its derived types).
	boost::optional<ReconstructedFeatureGeometry *> source_rfg =
			ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
					ReconstructedFeatureGeometry>(source_rg.get());
	if (source_rfg)
	{
		// Store the feature id and reconstruction geometry.
		return ResolvedGeometry::Section(
				source_feature_id,
				source_rfg.get(),
				source_rfg.get()->reconstructed_geometry());
	}

	if (d_current_resolved_geometry_type == RESOLVE_BOUNDARY)
	{
		// See if topological section is a resolved topological geometry.
		boost::optional<ResolvedTopologicalGeometry *> source_rtg =
				ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
						ResolvedTopologicalGeometry>(source_rg.get());
		if (source_rtg)
		{
			// See if resolved topological geometry is a line (not a boundary).
			boost::optional<ResolvedTopologicalGeometry::resolved_topology_line_ptr_type> resolved_line_geometry =
					source_rtg.get()->resolved_topology_line();
			if (resolved_line_geometry)
			{
				// Store the feature id and reconstruction geometry.
				return ResolvedGeometry::Section(
						source_feature_id,
						source_rtg.get(),
						resolved_line_geometry.get());
			}
		}
	}

	// If we got here then either (1) the user created a malformed GPML file somehow (eg, with a script)
	// or (2) it's a program error (because the topology build/edit tools should only currently allow
	// the user to add topological sections that are reconstructed static geometries
	// (or resolved topological *lines* when resolving boundaries).
	// We'll assume (1) and emit an error message rather than asserting/aborting.
	if (d_current_resolved_geometry_type == RESOLVE_BOUNDARY)
	{
		qWarning() << "Ignoring topological section, for resolved boundary, that is not a regular feature or topological *line*.";
	}
	else
	{
		qWarning() << "Ignoring topological section, for resolved line, that is not a regular feature.";
	}

	return boost::none;
}


void
GPlatesAppLogic::TopologyGeometryResolver::process_resolved_boundary_topological_section_intersections()
{
	// Iterate over our internal sequence of sections that we built up by
	// visiting the topological sections of a topological geometry property.
	const std::size_t num_sections = d_resolved_geometry.d_sections.size();

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
		process_resolved_boundary_topological_section_intersection(1/*section_index*/, true/*two_sections*/);
		return;
	}

	// Iterate over the sections and process intersections between each section
	// and its previous neighbour.
	for (std::size_t section_index = 0; section_index < num_sections; ++section_index)
	{
		process_resolved_boundary_topological_section_intersection(section_index);
	}
}


void
GPlatesAppLogic::TopologyGeometryResolver::process_resolved_boundary_topological_section_intersection(
		const std::size_t current_section_index,
		const bool two_sections)
{
	//
	// Intersect the current section with the previous section.
	//

	const std::size_t num_sections = d_resolved_geometry.d_sections.size();

	ResolvedGeometry::Section &current_section =
			d_resolved_geometry.d_sections[current_section_index];

	//
	// We get the start intersection geometry the previous section in the topological geometry's
	// list of sections whose valid time ranges include the current reconstruction time.
	//

	// Topological *boundaries* form a closed loop of sections so handle wraparound.
	const std::size_t prev_section_index = (current_section_index == 0)
			? num_sections - 1
			: current_section_index - 1;

	ResolvedGeometry::Section &prev_section =
			d_resolved_geometry.d_sections[prev_section_index];

	// If both sections refer to the same geometry then don't intersect.
	// This can happen when the same geometry is added more than once to the topology
	// when it forms different parts of the resolved topological geometry - normally there
	// are other geometries in between but when building topologies it's possible to
	// add the geometry as first section, then add another geometry as second section,
	// then add the first geometry again as the third section and then add another
	// geometry as the fourth section - before the fourth section is added the
	// first and third sections are adjacent and they are the same geometry - and if
	// the topology build/edit tool creates the topology when only three sections are
	// added then we have to deal with it here in the topology resolver.
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


void
GPlatesAppLogic::TopologyGeometryResolver::process_resolved_line_topological_section_intersections()
{
	// Iterate over our internal sequence of sections that we built up by
	// visiting the topological sections of a topological geometry property.
	const std::size_t num_sections = d_resolved_geometry.d_sections.size();

	// If there's only one section then don't try to intersect it with itself.
	if (num_sections < 2)
	{
		return;
	}

	// Resolved topological *lines* do not form a closed loop like boundaries.
	// So there's no need to treat the special case of two topological sections forming a closed loop.

	// Iterate over the sections and process intersections between each section
	// and its previous neighbour.
	for (std::size_t section_index = 0; section_index < num_sections; ++section_index)
	{
		process_resolved_line_topological_section_intersection(section_index);
	}
}


void
GPlatesAppLogic::TopologyGeometryResolver::process_resolved_line_topological_section_intersection(
		const std::size_t current_section_index)
{
	//
	// Intersect the current section with the previous section.
	//

	ResolvedGeometry::Section &current_section =
			d_resolved_geometry.d_sections[current_section_index];

	//
	// We get the start intersection geometry from the previous section in the topological geometry's
	// list of sections whose valid time ranges include the current reconstruction time.
	//

	// Topological *lines* don't form a closed loop of sections so we don't handle wraparound.
	// For the first section there is no previous section so there's no intersection to process.
	if (current_section_index == 0)
	{
		return;
	}

	const std::size_t prev_section_index = current_section_index - 1;

	ResolvedGeometry::Section &prev_section =
			d_resolved_geometry.d_sections[prev_section_index];

	// If both sections refer to the same geometry then don't intersect.
	// This can happen when the same geometry is added more than once to the topology
	// when it forms different parts of the resolved topological geometry - normally there
	// are other geometries in between but when building topologies it's possible to
	// add the geometry as first section, then add another geometry as second section,
	// then add the first geometry again as the third section and then add another
	// geometry as the fourth section - before the fourth section is added the
	// first and third sections are adjacent and they are the same geometry - and if
	// the topology build/edit tool creates the topology when only three sections are
	// added then we have to deal with it here in the topology resolver.
	if (prev_section.d_source_rg.get() == current_section.d_source_rg.get())
	{
		return;
	}

	//
	// Process the actual intersection.
	//
	current_section.d_intersection_results.intersect_with_previous_section(
			prev_section.d_intersection_results,
			prev_section.d_use_reverse);

	// NOTE: We don't need to look at the end intersection because the next topological
	// section that we visit will have this current section as its start intersection and
	// hence the intersection of this current section and its next section will be
	// taken care of during that visit.
}


void
GPlatesAppLogic::TopologyGeometryResolver::assign_segments()
{
	// Make sure all the boundary segments have been found.
	// It is an error in the code (not in the data) if this is not the case.
	const std::size_t num_sections = d_resolved_geometry.d_sections.size();
	for (std::size_t section_index = 0; section_index < num_sections; ++section_index)
	{
		assign_segment(section_index);
	}
}


void
GPlatesAppLogic::TopologyGeometryResolver::assign_segment(
		const std::size_t section_index)
{
	ResolvedGeometry::Section &section = d_resolved_geometry.d_sections[section_index];

	// See if the reverse flag has been set by intersection processing - this
	// happens if the visible section intersected both its neighbours otherwise it just
	// returns the flag we passed it.
	section.d_use_reverse = section.d_intersection_results.get_reverse_flag(section.d_use_reverse);

	section.d_final_segment_unreversed_geom =
			section.d_intersection_results.get_unreversed_sub_segment(section.d_use_reverse);
}


void
GPlatesAppLogic::TopologyGeometryResolver::create_resolved_topological_boundary()
{
	PROFILE_FUNC();

	// The points to create the plate polygon with.
	std::vector<GPlatesMaths::PointOnSphere> polygon_points;

	// Sequence of subsegments of resolved topology used when creating ResolvedTopologicalGeometry.
	std::vector<ResolvedTopologicalGeometrySubSegment> output_subsegments;

	// Iterate over the sections of the resolved boundary and construct
	// the resolved polygon boundary and its subsegments.
	ResolvedGeometry::section_seq_type::const_iterator section_iter =
			d_resolved_geometry.d_sections.begin();
	const ResolvedGeometry::section_seq_type::const_iterator section_end =
			d_resolved_geometry.d_sections.end();
	for ( ; section_iter != section_end; ++section_iter)
	{
		const ResolvedGeometry::Section &section = *section_iter;

		// It's possible for a valid segment to not contribute to the boundary
		// of the plate polygon. This can happen if it contributes zero-length
		// to the plate boundary which happens when both its neighbouring
		// boundary sections intersect it at the same point.
		if (!section.d_final_segment_unreversed_geom)
		{
			continue;
		}

		// Get the subsegment feature reference.
		boost::optional<GPlatesModel::FeatureHandle::weak_ref> subsegment_feature_ref =
				GPlatesAppLogic::ReconstructionGeometryUtils::get_feature_ref(section.d_source_rg);
		// If the feature reference is invalid then skip the current section.
		if (!subsegment_feature_ref)
		{
			continue;
		}
		const GPlatesModel::FeatureHandle::const_weak_ref subsegment_feature_const_ref(
				subsegment_feature_ref.get());

		// Create a subsegment structure that'll get used when
		// creating the resolved topological geometry.
		const ResolvedTopologicalGeometrySubSegment output_subsegment(
				section.d_final_segment_unreversed_geom.get(),
				section.d_source_rg,
				subsegment_feature_const_ref,
				section.d_use_reverse);
		output_subsegments.push_back(output_subsegment);

		// Append the subsegment geometry to the plate polygon points.
		GPlatesAppLogic::GeometryUtils::get_geometry_points(
				*section.d_final_segment_unreversed_geom.get(),
				polygon_points,
				section.d_use_reverse);
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
		qDebug() << "ERROR: Failed to create a ResolvedTopologicalGeometry - probably has "
				"insufficient points for a polygon.";
		qDebug() << "Skipping creation for topological polygon feature_id=";
		qDebug() << GPlatesUtils::make_qstring_from_icu_string(
				d_currently_visited_feature->feature_id().get());
		return;
	}

	//
	// Create the RTG for the plate polygon.
	//
	ResolvedTopologicalGeometry::non_null_ptr_type rtb_ptr =
		ResolvedTopologicalGeometry::create(
			d_reconstruction_tree,
			d_reconstruction_tree_creator,
			*plate_polygon,
			*(current_top_level_propiter()->handle_weak_ref()),
			*(current_top_level_propiter()),
			output_subsegments.begin(),
			output_subsegments.end(),
			d_reconstruction_params.get_recon_plate_id(),
			d_reconstruction_params.get_time_of_appearance(),
			d_reconstruct_handle/*identify where/when this RTG was resolved*/);

	d_resolved_topological_geometries.push_back(rtb_ptr);
}


void
GPlatesAppLogic::TopologyGeometryResolver::create_resolved_topological_line()
{
	PROFILE_FUNC();

	// The points to create the resolved line with.
	std::vector<GPlatesMaths::PointOnSphere> resolved_line_points;

	// Sequence of subsegments of resolved topology used when creating ResolvedTopologicalGeometry.
	std::vector<ResolvedTopologicalGeometrySubSegment> output_subsegments;

	// Iterate over the sections of the resolved line and construct
	// the resolved polyline and its subsegments.
	ResolvedGeometry::section_seq_type::const_iterator section_iter =
			d_resolved_geometry.d_sections.begin();
	const ResolvedGeometry::section_seq_type::const_iterator section_end =
			d_resolved_geometry.d_sections.end();
	for ( ; section_iter != section_end; ++section_iter)
	{
		const ResolvedGeometry::Section &section = *section_iter;

		// It's possible for a valid segment to not contribute to the resolved line.
		// This can happen if it contributes zero-length to the resolved line which happens when
		// both its neighbouring sections intersect it at the same point.
		if (!section.d_final_segment_unreversed_geom)
		{
			continue;
		}

		// Get the subsegment feature reference.
		boost::optional<GPlatesModel::FeatureHandle::weak_ref> subsegment_feature_ref =
				GPlatesAppLogic::ReconstructionGeometryUtils::get_feature_ref(section.d_source_rg);
		// If the feature reference is invalid then skip the current section.
		if (!subsegment_feature_ref)
		{
			continue;
		}
		const GPlatesModel::FeatureHandle::const_weak_ref subsegment_feature_const_ref(
				subsegment_feature_ref.get());

		// Create a subsegment structure that'll get used when
		// creating the resolved topological geometry.
		const ResolvedTopologicalGeometrySubSegment output_subsegment(
				section.d_final_segment_unreversed_geom.get(),
				section.d_source_rg,
				subsegment_feature_const_ref,
				section.d_use_reverse);
		output_subsegments.push_back(output_subsegment);

		// Append the subsegment geometry to the resolved line points.
		GPlatesAppLogic::GeometryUtils::get_geometry_points(
				*section.d_final_segment_unreversed_geom.get(),
				resolved_line_points,
				section.d_use_reverse);
	}

	// Create a polyline on sphere for the resolved line using 'resolved_line_points'.
	GPlatesUtils::GeometryConstruction::GeometryConstructionValidity polyline_validity;
	boost::optional<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> resolved_line_geometry =
			GPlatesUtils::create_polyline_on_sphere(
					resolved_line_points.begin(), resolved_line_points.end(), polyline_validity);

	// If we are unable to create a polyline (such as insufficient points) then
	// just return without creating a resolved topological geometry.
	if (polyline_validity != GPlatesUtils::GeometryConstruction::VALID)
	{
		qDebug() << "ERROR: Failed to create a ResolvedTopologicalGeometry - probably has "
				"insufficient points for a polyline.";
		qDebug() << "Skipping creation for topological line feature_id=";
		qDebug() << GPlatesUtils::make_qstring_from_icu_string(
				d_currently_visited_feature->feature_id().get());
		return;
	}

	//
	// Create the RTG for the resolved line.
	//
	ResolvedTopologicalGeometry::non_null_ptr_type rtl_ptr =
		ResolvedTopologicalGeometry::create(
			d_reconstruction_tree,
			d_reconstruction_tree_creator,
			*resolved_line_geometry,
			*(current_top_level_propiter()->handle_weak_ref()),
			*(current_top_level_propiter()),
			output_subsegments.begin(),
			output_subsegments.end(),
			d_reconstruction_params.get_recon_plate_id(),
			d_reconstruction_params.get_time_of_appearance(),
			d_reconstruct_handle/*identify where/when this RTG was resolved*/);

	d_resolved_topological_geometries.push_back(rtl_ptr);
}


void
GPlatesAppLogic::TopologyGeometryResolver::debug_output_topological_section_feature_id(
		const GPlatesModel::FeatureId &section_feature_id)
{
	qDebug() << "Topological geometry feature_id=";
	qDebug() << GPlatesUtils::make_qstring_from_icu_string(
			d_currently_visited_feature->feature_id().get());
	qDebug() << "Topological section referencing feature_id=";
	qDebug() << GPlatesUtils::make_qstring_from_icu_string(section_feature_id.get());
}
