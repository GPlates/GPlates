/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2019 The University of Sydney, Australia
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

#include <new>

#include "ReconstructionGraphBuilder.h"


GPlatesAppLogic::ReconstructionGraphBuilder::ReconstructionGraphBuilder(
		bool extend_total_reconstruction_poles_to_distant_past_) :
	d_reconstruction_graph(ReconstructionGraph::create()),
	d_extend_total_reconstruction_poles_to_distant_past(extend_total_reconstruction_poles_to_distant_past_)
{
}


void
GPlatesAppLogic::ReconstructionGraphBuilder::insert_total_reconstruction_sequence(
		GPlatesModel::integer_plate_id_type fixed_plate_id,
		GPlatesModel::integer_plate_id_type moving_plate_id,
		const total_reconstruction_pole_type &pole)
{
	// We need at least two pole time samples to have a valid sequence.
	if (pole.size() < 2)
	{
		return;
	}

	//
	// Create the fixed plate if it doesn't already exist.
	//
	ReconstructionGraph::Plate *fixed_plate = NULL;
	std::pair<ReconstructionGraph::plate_map_type::iterator, bool> fixed_plate_insert_result =
			d_reconstruction_graph->d_plate_map.insert(
					ReconstructionGraph::plate_map_type::value_type(fixed_plate_id, NULL));
	if (fixed_plate_insert_result.second)
	{
		// Successfully inserted 'fixed_plate_id' so create a fixed plate object.
		fixed_plate = d_reconstruction_graph->d_plate_pool.construct(fixed_plate_id);
		if (fixed_plate == NULL)
		{
			throw std::bad_alloc();
		}
		// Initialise the map item to point to the new plate.
		fixed_plate_insert_result.first->second = fixed_plate;
	}
	else
	{
		fixed_plate = fixed_plate_insert_result.first->second;
	}

	//
	// Create the moving plate if it doesn't already exist.
	//
	ReconstructionGraph::Plate *moving_plate = NULL;
	std::pair<ReconstructionGraph::plate_map_type::iterator, bool> moving_plate_insert_result =
			d_reconstruction_graph->d_plate_map.insert(
					ReconstructionGraph::plate_map_type::value_type(moving_plate_id, NULL));
	if (moving_plate_insert_result.second)
	{
		// Successfully inserted 'moving_plate_id' so create a moving plate object.
		moving_plate = d_reconstruction_graph->d_plate_pool.construct(moving_plate_id);
		if (moving_plate == NULL)
		{
			throw std::bad_alloc();
		}
		// Initialise the map item to point to the new plate.
		moving_plate_insert_result.first->second = moving_plate;
	}
	else
	{
		moving_plate = moving_plate_insert_result.first->second;
	}

	//
	// Create a new Edge between the fixed and moving plates.
	//
	// Note that there can be more than one edge between the same fixed and moving plates.
	// This happens when a fixed/moving rotation sequence is split into two (or more) sequences
	// (such as splitting across two rotation files, one for 0-250Ma and the other for 250-410Ma).
	ReconstructionGraph::Edge *edge = d_reconstruction_graph->d_edge_pool.construct(
			fixed_plate, moving_plate);
	if (edge == NULL)
	{
		throw std::bad_alloc();
	}

	// Add the total reconstruction pole samples to the edge.
	total_reconstruction_pole_type::const_iterator pole_iter = pole.begin();
	total_reconstruction_pole_type::const_iterator pole_end = pole.end();
	for ( ; pole_iter != pole_end; ++pole_iter)
	{
		ReconstructionGraph::PoleSample *pole_sample = d_reconstruction_graph->d_pole_sample_pool.construct(
				pole_iter->first, pole_iter->second);
		if (pole_sample == NULL)
		{
			throw std::bad_alloc();
		}

		edge->d_pole.push_back(*pole_sample);
	}

	// Add the edge to the fixed and moving plates.
	//
	// The order is not currently important since it also doesn't currently matter for when a
	// ReconstructionTree is generated, even at crossovers (where a moving plate changes its
	// fixed plate at a specific time). Although it's possible we might want to order the
	// incoming edges by time to ensure only the fixed plate of the younger sequence is visited
	// but that's not the case currently (and I don't think it's really necessary).
	//
	// Note: If we did make the change we'd also need to change reverse (moving->fixed) as well as
	// forward (fixed->moving) propagation through the graph, with the latter (forward) actually
	// being more tricky (because we'd have to check the associated incoming edges of the
	// outgoing (forward) edge's moving plate to make sure the outgoing edge *is* the highest
	// priority edge in the set of incoming edges).
	fixed_plate->d_outgoing_edges.push_front(*edge);
	moving_plate->d_incoming_edges.push_front(*edge);
}


GPlatesAppLogic::ReconstructionGraph::non_null_ptr_to_const_type
GPlatesAppLogic::ReconstructionGraphBuilder::build_graph()
{
	if (d_extend_total_reconstruction_poles_to_distant_past)
	{
		extend_total_reconstruction_poles_to_distant_past();
	}

	// The built reconstruction graph to return.
	ReconstructionGraph::non_null_ptr_type built_reconstruction_graph = d_reconstruction_graph;

	// Create a new empty reconstruction graph for the next build (if any).
	//
	// We don't want any subsequent insertions (of total reconstruction sequences) to
	// affect the returned graph.
	d_reconstruction_graph = ReconstructionGraph::create();

	return built_reconstruction_graph;
}


void
GPlatesAppLogic::ReconstructionGraphBuilder::extend_total_reconstruction_poles_to_distant_past()
{
	// Iterate over all plates in the graph.
	ReconstructionGraph::plate_map_type::const_iterator plate_iter = d_reconstruction_graph->d_plate_map.begin();
	ReconstructionGraph::plate_map_type::const_iterator plate_end = d_reconstruction_graph->d_plate_map.end();
	for( ; plate_iter != plate_end; ++plate_iter)
	{
		ReconstructionGraph::Plate &moving_plate = *plate_iter->second;

		const ReconstructionGraph::plate_incoming_edge_list_type &incoming_edges = moving_plate.get_incoming_edges();
		if (incoming_edges.empty())
		{
			// The root plate of the graph is typically the only plate that does not have incoming edges.
			continue;
		}

		// Iterate over the edges going *into* the moving plate and find the oldest pole of the oldest sequence.
		//
		// It's possible we could have one or more crossovers into our moving plate, and/or sequences where a fixed plate
		// is divided into multiple time ranges (eg, a fixed->moving pair with both 0-250Ma and 250-410Ma edges).

		ReconstructionGraph::plate_incoming_edge_list_type::const_iterator incoming_edges_iter = incoming_edges.begin();
		ReconstructionGraph::plate_incoming_edge_list_type::const_iterator incoming_edges_end = incoming_edges.end();

		// The first incoming edge.
		const ReconstructionGraph::Edge *oldest_incoming_edge = &*incoming_edges_iter;
		GPlatesPropertyValues::GeoTimeInstant oldest_incoming_edge_begin_time = oldest_incoming_edge->get_begin_time();

		// Iterate over the moving plate's remaining incoming edges (if any).
		for (++incoming_edges_iter; incoming_edges_iter != incoming_edges_end; ++incoming_edges_iter)
		{
			const ReconstructionGraph::Edge &incoming_edge = *incoming_edges_iter;

			const GPlatesPropertyValues::GeoTimeInstant &incoming_edge_begin_time = incoming_edge.get_begin_time();
			if (incoming_edge_begin_time.is_strictly_earlier_than(oldest_incoming_edge_begin_time))
			{
				oldest_incoming_edge_begin_time = incoming_edge_begin_time;
				oldest_incoming_edge = &incoming_edge;
			}
		}

		// Fixed plate of the oldest incoming edge.
		ReconstructionGraph::Plate &fixed_plate = *oldest_incoming_edge->d_fixed_plate;

		// Create a new edge between the fixed and moving plates, from the oldest pole time
		// back to the distant past.
		ReconstructionGraph::Edge *distant_past_edge = d_reconstruction_graph->d_edge_pool.construct(
				&fixed_plate, &moving_plate);
		if (distant_past_edge == NULL)
		{
			throw std::bad_alloc();
		}

		const ReconstructionGraph::PoleSample &oldest_pole_sample = oldest_incoming_edge->get_pole().back();

		// The youngest pole sample of new distant-past edge equals the oldest pole sample.
		// And its time instant is also equal.
		ReconstructionGraph::PoleSample *distant_past_edge_youngest_pole_sample = d_reconstruction_graph->d_pole_sample_pool.construct(
				oldest_pole_sample.get_time_instant(),
				oldest_pole_sample.get_finite_rotation());
		if (distant_past_edge_youngest_pole_sample == NULL)
		{
			throw std::bad_alloc();
		}
		distant_past_edge->d_pole.push_back(*distant_past_edge_youngest_pole_sample);

		// The oldest pole sample of new distant-past edge also equals its youngest pole sample.
		// But its time instant is the distant past.
		ReconstructionGraph::PoleSample *distant_past_edge_oldest_pole_sample = d_reconstruction_graph->d_pole_sample_pool.construct(
				GPlatesPropertyValues::GeoTimeInstant::create_distant_past(),
				oldest_pole_sample.get_finite_rotation());
		if (distant_past_edge_oldest_pole_sample == NULL)
		{
			throw std::bad_alloc();
		}
		distant_past_edge->d_pole.push_back(*distant_past_edge_oldest_pole_sample);

		// Add the distant-past edge to the fixed and moving plates.
		fixed_plate.d_outgoing_edges.push_front(*distant_past_edge);
		moving_plate.d_incoming_edges.push_front(*distant_past_edge);
	}
}
