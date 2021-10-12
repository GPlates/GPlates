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
#include <QDebug>

#include "CgalUtils.h"
#include "GeometryUtils.h"
#include "ReconstructionGeometryCollection.h"
#include "ReconstructionGeometryUtils.h"
#include "ReconstructedFeatureGeometry.h"
#include "Reconstruction.h"
#include "ResolvedTopologicalNetwork.h"
#include "TopologyInternalUtils.h"
#include "TopologyNetworkResolver.h"
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
#include "utils/UnicodeStringUtils.h"


GPlatesAppLogic::TopologyNetworkResolver::TopologyNetworkResolver(
			ReconstructionGeometryCollection &reconstruction_geometry_collection) :
	d_reconstruction_geometry_collection(reconstruction_geometry_collection),
	d_reconstruction_params(reconstruction_geometry_collection.get_reconstruction_time())
{  
	d_num_topologies = 0;
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
	// Prepare for a new topological polygon.
	d_resolved_network.reset();

	//
	// Visit the topological sections to gather needed information and store
	// it internally in 'd_resolved_network'.
	//
	record_topological_sections(gpml_topological_polygon.sections());

	//
	// Now create the ResolvedTopologicalNetwork.
	//
	create_resolved_topology_network();
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

	ResolvedNetwork::Section section(source_feature_id);

	record_topological_section_reconstructed_geometry(
			section,
			*gpml_toplogical_line_section.get_source_geometry());

	// Add to internal sequence.
	d_resolved_network.d_sections.push_back(section);
}


void
GPlatesAppLogic::TopologyNetworkResolver::visit_gpml_topological_point(
		GPlatesPropertyValues::GpmlTopologicalPoint &gpml_toplogical_point)
{  
	const GPlatesModel::FeatureId source_feature_id =
			gpml_toplogical_point.get_source_geometry()->feature_id();

	ResolvedNetwork::Section section(source_feature_id);

	record_topological_section_reconstructed_geometry(
			section,
			*gpml_toplogical_point.get_source_geometry());

	// Add to internal sequence.
	d_resolved_network.d_sections.push_back(section);
}


void
GPlatesAppLogic::TopologyNetworkResolver::record_topological_section_reconstructed_geometry(
		ResolvedNetwork::Section &section,
		const GPlatesPropertyValues::GpmlPropertyDelegate &geometry_delegate)
{
	// Get the reconstructed geometry of the topological section's delegate.
	// The referenced features must have been reconstructed using the
	// reconstruction tree referenced by our destination reconstruction geometry collection.
	boost::optional<ReconstructedFeatureGeometry::non_null_ptr_type> source_rfg =
			TopologyInternalUtils::find_reconstructed_feature_geometry(
					geometry_delegate,
					*d_reconstruction_geometry_collection.reconstruction_tree());

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
	section.d_geometry = (*source_rfg)->geometry();
}


void
GPlatesAppLogic::TopologyNetworkResolver::create_resolved_topology_network()
{
	// The triangulation for the topological network.
	boost::shared_ptr<CgalUtils::cgal_delaunay_triangulation_type> delaunay_triangulation(
			new CgalUtils::cgal_delaunay_triangulation_type());

	// Sequence of subsegments of resolved topology used when creating ResolvedTopologicalNetwork.
	std::vector<ResolvedTopologicalNetwork::Node> output_nodes;

	// Iterate over the sections of the resolved network and construct
	// the resolved network.
	ResolvedNetwork::section_seq_type::const_iterator section_iter =
			d_resolved_network.d_sections.begin();
	const ResolvedNetwork::section_seq_type::const_iterator section_end =
			d_resolved_network.d_sections.end();
	for ( ; section_iter != section_end; ++section_iter)
	{
		const ResolvedNetwork::Section &section = *section_iter;

		// If we were unable to retrieve the reconstructed section geometry then
		// skip the current section - it will not be part of the network.
		if (!section.d_source_rfg || !section.d_geometry)
		{
			continue;
		}

		// Get the node feature reference.
		const GPlatesModel::FeatureHandle::const_weak_ref node_feature_const_ref =
				(*section.d_source_rfg)->get_feature_ref();

		// Get the section geometry.
		std::vector<GPlatesMaths::PointOnSphere> node_points;
		GPlatesAppLogic::GeometryUtils::get_geometry_points(
				*section.d_geometry.get(), node_points);

		// Create a subsegment structure that'll get used when
		// creating the resolved topological geometry.
		const ResolvedTopologicalNetwork::Node output_node(
				section.d_geometry.get(),
				node_feature_const_ref);
		output_nodes.push_back(output_node);

		// Add the node's points to the triangulation.
		CgalUtils::insert_points_into_delaunay_triangulation(
				*delaunay_triangulation, node_points.begin(), node_points.end());
	}

	// Create the network.
	const ResolvedTopologicalNetwork::non_null_ptr_type network =
			ResolvedTopologicalNetwork::create(
					d_reconstruction_geometry_collection.reconstruction_tree(),
					delaunay_triangulation,
					*(current_top_level_propiter()->handle_weak_ref()),
					*(current_top_level_propiter()),
					output_nodes.begin(),
					output_nodes.end(),
					d_reconstruction_params.get_recon_plate_id(),
					d_reconstruction_params.get_time_of_appearance());

	d_reconstruction_geometry_collection.add_reconstruction_geometry(network);
}


void
GPlatesAppLogic::TopologyNetworkResolver::debug_output_topological_section_feature_id(
		const GPlatesModel::FeatureId &section_feature_id)
{
	qDebug() << "Topological network feature_id=";
	qDebug() << GPlatesUtils::make_qstring_from_icu_string(
			d_currently_visited_feature->feature_id().get());
	qDebug() << "Topological section referencing feature_id=";
	qDebug() << GPlatesUtils::make_qstring_from_icu_string(section_feature_id.get());
}
