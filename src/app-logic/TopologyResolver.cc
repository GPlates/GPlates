/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date: 2008-08-15 02:13:48 -0700 (Fri, 15 Aug 2008) $
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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
#include <QDebug>

#include "TopologyResolver.h"

#include "ReconstructionGeometryUtils.h"
#include "TopologyInternalUtils.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "maths/GeometryOnSphere.h"

#include "model/ReconstructedFeatureGeometry.h"
#include "model/Reconstruction.h"
#include "model/ResolvedTopologicalGeometry.h"

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
#define CREATE_RFG_FOR_ROTATED_REFERENCE_POINTS


GPlatesAppLogic::TopologyResolver::TopologyResolver(
			const double &recon_time,
			GPlatesModel::Reconstruction &recon) :
	d_recon_ptr(&recon),
	d_reconstruction_params(recon_time)
{  
	d_num_topologies = 0;
}


bool
GPlatesAppLogic::TopologyResolver::initialise_pre_feature_properties(
		GPlatesModel::FeatureHandle &feature_handle)
{
	// super short-cut for features without boundary list properties
	static QString type("TopologicalClosedPlateBoundary");
	if ( type != GPlatesUtils::make_qstring_from_icu_string(
			feature_handle.feature_type().get_name() ) ) 
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
GPlatesAppLogic::TopologyResolver::visit_gpml_constant_value(
		GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	gpml_constant_value.value()->accept_visitor(*this);
}


void
GPlatesAppLogic::TopologyResolver::visit_gpml_piecewise_aggregation(
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
GPlatesAppLogic::TopologyResolver::visit_gpml_time_window(
		GPlatesPropertyValues::GpmlTimeWindow &gpml_time_window)
{
	gpml_time_window.time_dependent_value()->accept_visitor(*this);
	gpml_time_window.valid_time()->accept_visitor(*this);
}


void
GPlatesAppLogic::TopologyResolver::visit_gpml_topological_polygon(
		GPlatesPropertyValues::GpmlTopologicalPolygon &gpml_topological_polygon)
{
	PROFILE_FUNC();

	// Prepare for a new topological polygon.
	d_resolved_boundary.reset();

	//
	// Visit the topological sections to gather needed information and store
	// it in internally in 'd_resolved_boundary'.
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
	// Now create the ResolvedTopologicalGeometry.
	//
	create_resolved_topology_geometry();
}


void
GPlatesAppLogic::TopologyResolver::record_topological_sections(
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
GPlatesAppLogic::TopologyResolver::visit_gpml_topological_line_section(
		GPlatesPropertyValues::GpmlTopologicalLineSection &gpml_toplogical_line_section)
{  
	const GPlatesModel::FeatureId source_feature_id =
			gpml_toplogical_line_section.get_source_geometry()->feature_id();

	ResolvedBoundary::Section section(source_feature_id);

	record_topological_section_reconstructed_geometry(
			section,
			*gpml_toplogical_line_section.get_source_geometry());
	
	// Set reverse flag.
	section.d_use_reverse = gpml_toplogical_line_section.get_reverse_order();

	// Record start intersection information.
	if (gpml_toplogical_line_section.get_start_intersection())
	{
		const GPlatesMaths::PointOnSphere reference_point =
				*gpml_toplogical_line_section.get_start_intersection()->reference_point()->point();

		section.d_start_intersection = ResolvedBoundary::Intersection(reference_point);
	}

	// Record end intersection information.
	if (gpml_toplogical_line_section.get_end_intersection())
	{
		const GPlatesMaths::PointOnSphere reference_point =
				*gpml_toplogical_line_section.get_end_intersection()->reference_point()->point();

		section.d_end_intersection = ResolvedBoundary::Intersection(reference_point);
	}

	// Add to internal sequence.
	d_resolved_boundary.d_sections.push_back(section);
}


void
GPlatesAppLogic::TopologyResolver::visit_gpml_topological_point(
		GPlatesPropertyValues::GpmlTopologicalPoint &gpml_toplogical_point)
{  
	const GPlatesModel::FeatureId source_feature_id =
			gpml_toplogical_point.get_source_geometry()->feature_id();

	ResolvedBoundary::Section section(source_feature_id);

	record_topological_section_reconstructed_geometry(
			section,
			*gpml_toplogical_point.get_source_geometry());

	// No other information to collect since this topological section is a point and
	// hence cannot intersect with neighbouring sections.

	// Add to internal sequence.
	d_resolved_boundary.d_sections.push_back(section);
}


void
GPlatesAppLogic::TopologyResolver::record_topological_section_reconstructed_geometry(
		ResolvedBoundary::Section &section,
		const GPlatesPropertyValues::GpmlPropertyDelegate &geometry_delegate)
{
	// Get the reconstructed geometry of the topological section's delegate.
	boost::optional<GPlatesModel::ReconstructedFeatureGeometry::non_null_ptr_type> source_rfg =
			TopologyInternalUtils::find_reconstructed_feature_geometry(
					geometry_delegate, d_recon_ptr);

	if (!source_rfg)
	{
		qDebug() << "ERROR: Failed to retrieve GpmlTopologicalSection "
				"reconstructed feature geometry - skipping line section.";
		debug_output_topological_section_feature_id(section.d_source_feature_id);
		return;
	}

	// Store the RFG.
	section.d_source_rfg = source_rfg;

	// Store the RFG's unclipped geometry.
	section.d_subsegment_geom = (*source_rfg)->geometry();
}


void
GPlatesAppLogic::TopologyResolver::validate_topological_section_intersections()
{
	// Iterate over our internal sequence of sections that we built up by
	// visiting the topological sections of a topological polygon.
	ResolvedBoundary::section_seq_type::const_iterator section_iter =
			d_resolved_boundary.d_sections.begin();
	const ResolvedBoundary::section_seq_type::const_iterator section_end =
			d_resolved_boundary.d_sections.end();
	for (std::size_t section_index = 0;
		section_iter != section_end;
		++section_iter, ++section_index)
	{
		validate_topological_section_intersection(section_index);
	}
}


void
GPlatesAppLogic::TopologyResolver::validate_topological_section_intersection(
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
GPlatesAppLogic::TopologyResolver::process_topological_section_intersections()
{
	// Iterate over our internal sequence of sections that we built up by
	// visiting the topological sections of a topological polygon.
	ResolvedBoundary::section_seq_type::const_iterator section_iter =
			d_resolved_boundary.d_sections.begin();
	const ResolvedBoundary::section_seq_type::const_iterator section_end =
			d_resolved_boundary.d_sections.end();
	for (std::size_t section_index = 0;
		section_iter != section_end;
		++section_iter, ++section_index)
	{
		process_topological_section_intersection(section_index);
	}
}


void
GPlatesAppLogic::TopologyResolver::process_topological_section_intersection(
		const std::size_t current_section_index)
{
	const std::size_t num_sections = d_resolved_boundary.d_sections.size();

	ResolvedBoundary::Section &current_section =
			d_resolved_boundary.d_sections[current_section_index];

	if (current_section.d_start_intersection)
	{
		//
		// NOTE: We don't get the start intersection geometry from the GpmlTopologicalIntersection
		// - instead we get the geometry from the previous section in the topological polygon's
		// list of sections.
		//
		// This assumes that the start intersection geometry is that of the previous section's
		// which is currently the case.
		//
		// By doing this we minimise the number of polyline intersection tests to the number
		// of topological sections in the topological polygon rather than twice this number
		// - and this helps speed up the code since approximately 60% of the cpu time spent
		// resolving topologies is spent in the polyline intersection code.
		//

		const std::size_t prev_section_index = (current_section_index == 0)
				? num_sections - 1
				: current_section_index - 1;

		ResolvedBoundary::Section &prev_section =
				d_resolved_boundary.d_sections[prev_section_index];

		if (!prev_section.d_end_intersection)
		{
			// The previous section did not have an end intersection which means the
			// topological polygon was not created in a valid state.
			// We'll just handle this by ignoring the intersection and keeping the
			// current section geometries as they are.
			qDebug() << "ERROR: Expected previous GpmlTopologicalSection to have "
					"end intersection - ignoring intersection.";
			debug_output_topological_section_feature_id(prev_section.d_source_feature_id);
			return;
		}

		// If we were unable to retrieve the reconstructed geometries for the
		// previous and current sections then we can't do an intersection.
		if (!prev_section.d_subsegment_geom)
		{
			qDebug() << "ERROR: Don't have topological section geometry for intersection - "
					"ignoring intersection.";
			debug_output_topological_section_feature_id(prev_section.d_source_feature_id);
			return;
		}
		if (!current_section.d_subsegment_geom)
		{
			qDebug() << "ERROR: Don't have topological section geometry for intersection - "
					"ignoring intersection.";
			debug_output_topological_section_feature_id(current_section.d_source_feature_id);
			return;
		}

		// Get the rotation used to rotate the previous section's reference point.
		// NOTE: we use the section itself as the reference feature rather than the
		// feature stored in the gpml:endIntersection.
		const boost::optional<GPlatesMaths::FiniteRotation> prev_section_rotation =
				TopologyInternalUtils::get_finite_rotation(
						(*prev_section.d_source_rfg)->get_feature_ref(),
						d_recon_ptr->reconstruction_tree());
		if (prev_section_rotation)
		{
			// Reconstruct the reference point.
			prev_section.d_end_intersection->d_reconstructed_reference_point =
					*prev_section_rotation * prev_section.d_end_intersection->d_reference_point;
		}
		else
		{
			qDebug() << "ERROR: No 'reconstructionPlateId' rotation found - using unrotated point.";
			debug_output_topological_section_feature_id(prev_section.d_source_feature_id);
			// No rotation was found so just use the unrotated point.
			prev_section.d_end_intersection->d_reconstructed_reference_point =
					prev_section.d_end_intersection->d_reference_point;
		}

		// Get the rotation used to rotate the next section's reference point.
		// NOTE: we use the section itself as the reference feature rather than the
		// feature stored in the gpml:startIntersection.
		const boost::optional<GPlatesMaths::FiniteRotation> current_section_rotation =
				TopologyInternalUtils::get_finite_rotation(
						(*current_section.d_source_rfg)->get_feature_ref(),
						d_recon_ptr->reconstruction_tree());
		if (current_section_rotation)
		{
			// Reconstruct the reference point.
			current_section.d_start_intersection->d_reconstructed_reference_point =
					*current_section_rotation * current_section.d_start_intersection->d_reference_point;
		}
		else
		{
			qDebug() << "ERROR: No 'reconstructionPlateId' rotation found - using unrotated point.";
			debug_output_topological_section_feature_id(current_section.d_source_feature_id);
			// No rotation was found so just use the unrotated point.
			current_section.d_start_intersection->d_reconstructed_reference_point =
					current_section.d_start_intersection->d_reference_point;
		}

		// Intersect the previous section with the current section and find the intersected
		// segments that are closest to the respective rotated reference points.
		boost::tuple<
				boost::optional<GPlatesMaths::PointOnSphere>/*intersection point*/, 
				GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type/*first_section_closest_segment*/,
				GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type/*second_section_closest_segment*/>
			closest_segments =
				TopologyInternalUtils::intersect_topological_sections(
						prev_section.d_subsegment_geom.get(),
						*prev_section.d_end_intersection->d_reconstructed_reference_point,
						current_section.d_subsegment_geom.get(),
						*current_section.d_start_intersection->d_reconstructed_reference_point);

		// Store the closest intersected segments back into the sequence of sections.
		// The current segment might even get clipped again when it intersects with the next
		// section (which would happen when we visit the next topological section).
		prev_section.d_subsegment_geom = boost::get<1>(closest_segments);
		current_section.d_subsegment_geom = boost::get<2>(closest_segments);
	}

	// NOTE: We don't need to look at the end intersection because the next topological
	// section that we visit will have this current section as its start intersection and
	// hence the intersection of the current section and the next section will be taken care of.
	//
	// This assumes that the next section's start intersection will refer to the
	// current section if the current section's end intersection refers to the next section.
	// Like a doubly-linked list. This should be true with the current topology tools.
}


void
GPlatesAppLogic::TopologyResolver::create_resolved_topology_geometry()
{
	PROFILE_FUNC();

	// The points to create the plate polygon with.
	std::vector<GPlatesMaths::PointOnSphere> polygon_points;

#if defined(CREATE_RFG_FOR_ROTATED_REFERENCE_POINTS)
	// The rotated reference points for any intersecting sections.
	std::vector<GPlatesMaths::PointOnSphere> rotated_reference_points;
#endif

	// Sequence of subsegments of resolved topology used when creating ResolvedTopologicalGeometry.
	std::vector<GPlatesModel::ResolvedTopologicalGeometry::SubSegment> output_subsegments;

	// Iterate over the sections of the resolved boundary and construct
	// the resolved polygon boundary and its subsegments.
	ResolvedBoundary::section_seq_type::const_iterator section_iter =
			d_resolved_boundary.d_sections.begin();
	const ResolvedBoundary::section_seq_type::const_iterator section_end =
			d_resolved_boundary.d_sections.end();
	for ( ; section_iter != section_end; ++section_iter)
	{
		const ResolvedBoundary::Section &section = *section_iter;

		// If we were unable to retrieve the reconstructed section geometry then
		// skip the current section - it will not be part of the polygon boundary.
		if (!section.d_source_rfg || !section.d_subsegment_geom)
		{
			continue;
		}

		// Get the subsegment feature reference.
		const GPlatesModel::FeatureHandle::weak_ref subsegment_feature_ref =
				(*section.d_source_rfg)->get_feature_ref();
		const GPlatesModel::FeatureHandle::const_weak_ref subsegment_feature_const_ref(
				GPlatesModel::FeatureHandle::get_const_weak_ref(subsegment_feature_ref));

		// Create a subsegment structure that'll get used when
		// creating the resolved topological geometry.
		const GPlatesModel::ResolvedTopologicalGeometry::SubSegment output_subsegment(
				section.d_subsegment_geom.get(),
				subsegment_feature_const_ref,
				section.d_use_reverse);
		output_subsegments.push_back(output_subsegment);

		// Append the subsegment geometry to the plate polygon points.
		TopologyInternalUtils::get_geometry_points(
				*section.d_subsegment_geom.get(), polygon_points, section.d_use_reverse);

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
	GPlatesModel::ResolvedTopologicalGeometry::non_null_ptr_type rtg_ptr =
		GPlatesModel::ResolvedTopologicalGeometry::create(
			*plate_polygon,
			*current_top_level_propiter()->collection_handle_ptr(),
			*current_top_level_propiter(),
			output_subsegments.begin(),
			output_subsegments.end(),
			d_reconstruction_params.get_recon_plate_id(),
			d_reconstruction_params.get_time_of_appearance());

	ReconstructionGeometryUtils::add_reconstruction_geometry_to_reconstruction(
			rtg_ptr, d_recon_ptr);

#if defined(CREATE_RFG_FOR_ROTATED_REFERENCE_POINTS)
	//
	// Create the RFG for the rotated reference points.
	//
	if (!rotated_reference_points.empty())
	{
		GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type rotated_reference_points_geom = 
				GPlatesMaths::MultiPointOnSphere::create_on_heap(rotated_reference_points);

		GPlatesModel::ReconstructedFeatureGeometry::non_null_ptr_type rotated_reference_points_rfg =
			GPlatesModel::ReconstructedFeatureGeometry::create(
				rotated_reference_points_geom,
				*current_top_level_propiter()->collection_handle_ptr(),
				*current_top_level_propiter());

		ReconstructionGeometryUtils::add_reconstruction_geometry_to_reconstruction(
				rotated_reference_points_rfg, d_recon_ptr);
	}
#endif // if defined(CREATE_RFG_FOR_ROTATED_REFERENCE_POINTS)
}


void
GPlatesAppLogic::TopologyResolver::debug_output_topological_section_feature_id(
		const GPlatesModel::FeatureId &section_feature_id)
{
	qDebug() << "Topological polygon feature_id=";
	qDebug() << GPlatesUtils::make_qstring_from_icu_string(
			d_currently_visited_feature->feature_id().get());
	qDebug() << "Topological section referencing feature_id=";
	qDebug() << GPlatesUtils::make_qstring_from_icu_string(section_feature_id.get());
}
