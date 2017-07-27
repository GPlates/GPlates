/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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
#include <boost/foreach.hpp>

#include "CoRegConfigurationTable.h"
#include "RegionOfInterestFilter.h"

#include "global/GPlatesAssert.h"

#include "scribe/Scribe.h"


namespace
{
	using namespace GPlatesDataMining;
	
	bool
	compare_layer(
			const ConfigurationTableRow& row_1,
			const ConfigurationTableRow& row_2)
	{
		return row_1.target_layer < row_2.target_layer;
	}

	bool
	compare_filter_type(
			const ConfigurationTableRow& row_1,
			const ConfigurationTableRow& row_2)
	{
		return row_1.filter_cfg->filter_name() < row_2.filter_cfg->filter_name();
	}

	bool
	compare_filter(
			const ConfigurationTableRow& row_1,
			const ConfigurationTableRow& row_2)
	{
		return *row_2.filter_cfg < *row_1.filter_cfg;
	}
}

void
GPlatesDataMining::CoRegConfigurationTable::optimize()
{
	group_and_sort();
	d_optimized = true;
}


void
GPlatesDataMining::CoRegConfigurationTable::group_and_sort()
{
	for(std::size_t i=0; i<d_rows.size(); i++)//keep the original index.
		d_rows[i].index = i;

	//group by layer
	std::sort(begin(), end(), compare_layer);
	
	iterator it = begin(), it_end = end();
	while(it != it_end)
	{
		std::pair<iterator,iterator> bounds;
		bounds = std::equal_range(it, it_end, *it, compare_layer);
		it = bounds.second;
		iterator inner_it = bounds.first, inner_it_end =bounds.second;
		std::sort(inner_it, inner_it_end, compare_filter_type); // group by filter type
		while(inner_it != inner_it_end)
		{
			std::pair<iterator,iterator> inner_bounds;
			inner_bounds = std::equal_range(inner_it, inner_it_end, *inner_it, compare_filter_type);
			inner_it = inner_bounds.second;
			if(inner_bounds.first != inner_bounds.second)
			{
				std::sort(inner_bounds.first, inner_bounds.second, compare_filter); //sort by filter
			}
		}
	}
}


GPlatesDataMining::ConfigurationTableRow::ConfigurationTableRow() :
	attr_type(CO_REGISTRATION_GPML_ATTRIBUTE/*arbitrary*/),
	reducer_type(REDUCER_MIN/*arbitrary*/),
	raster_level_of_detail(0),
	raster_fill_polygons(false),
	index(0)
{
}


bool
GPlatesDataMining::ConfigurationTableRow::operator==(
		const ConfigurationTableRow &rhs) const
{
	return 
		target_layer == rhs.target_layer	&&
		*filter_cfg == *rhs.filter_cfg		&& // Dereference to use 'CoRegFilter::Config' equality operator.
		attr_name == rhs.attr_name			&&
		layer_name == rhs.layer_name		&&
		assoc_name == rhs.assoc_name		&&
		attr_type == rhs.attr_type			&&
		reducer_type == rhs.reducer_type	&&
		raster_level_of_detail == rhs.raster_level_of_detail	&&
		raster_fill_polygons == rhs.raster_fill_polygons		&&
		index == rhs.index;
}


GPlatesScribe::TranscribeResult
GPlatesDataMining::ConfigurationTableRow::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	boost::optional<GPlatesScribe::TranscribeContext<ConfigurationTableRow> &> transcribe_context =
			scribe.get_transcribe_context<ConfigurationTableRow>();
	if (!transcribe_context)
	{
		return GPlatesScribe::TRANSCRIBE_INCOMPATIBLE;
	}

	typedef GPlatesScribe::TranscribeContext<ConfigurationTableRow>::layer_seq_type layer_seq_type;
	const layer_seq_type &layers = transcribe_context->get_layers();

	// Transcribe the target layer (transcribe an index into the sequence of layers).
	if (scribe.is_saving())
	{
		// Find the input layer in our list of layers.
		layer_seq_type::const_iterator layer_iter = std::find(layers.begin(), layers.end(), target_layer);
		if (layer_iter != layers.end())
		{
			const unsigned int layer_index = layer_iter - layers.begin();
			scribe.save(TRANSCRIBE_SOURCE, layer_index, "target_layer");
		}
	}
	else // loading...
	{
		unsigned int layer_index;
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, layer_index, "target_layer"))
		{
			return scribe.get_transcribe_result();
		}

		if (layer_index >= layers.size())
		{
			return GPlatesScribe::TRANSCRIBE_INCOMPATIBLE;
		}

		target_layer = layers[layer_index];

		// The target layer might not have successfully loaded in the first place.
		if (!target_layer.is_valid())
		{
			return GPlatesScribe::TRANSCRIBE_INCOMPATIBLE;
		}
	}

	// We don't transcribe 'index' - that gets set once the config table is grouped and sorted.
	if (!scribe.transcribe(TRANSCRIBE_SOURCE, filter_cfg, "filter_cfg") ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, attr_name, "attr_name") ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, layer_name, "layer_name") ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, assoc_name, "assoc_name") ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, attr_type, "attr_type") ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, reducer_type, "reducer_type") ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, raster_level_of_detail, "raster_level_of_detail") ||
		!scribe.transcribe(TRANSCRIBE_SOURCE, raster_fill_polygons, "raster_fill_polygons"))
	{
		return scribe.get_transcribe_result();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesDataMining::CoRegConfigurationTable::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	const GPlatesScribe::ObjectTag rows_object_tag("rows");

	if (scribe.is_saving())
	{
		for (unsigned int row_index = 0; row_index < d_rows.size(); ++row_index)
		{
			scribe.save(TRANSCRIBE_SOURCE, d_rows[row_index], rows_object_tag[row_index]);
		}

		// Save number of rows.
		scribe.save(TRANSCRIBE_SOURCE, d_rows.size(), rows_object_tag.sequence_size());
	}
	else // loading...
	{
		// Load number of rows.
		unsigned int num_rows;
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, num_rows, rows_object_tag.sequence_size()))
		{
			return scribe.get_transcribe_result();
		}

		for (unsigned int row_index = 0; row_index < num_rows; ++row_index)
		{
			// If row fails to load then skip it and continue to the next row instead of failing.
			ConfigurationTableRow row;
			if (scribe.transcribe(TRANSCRIBE_SOURCE, row, rows_object_tag[row_index]))
			{
				push_back(row);
			}
		}

		// Optimise and group so that clients can properly compare two configuration tables for equality.
		optimize();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}
