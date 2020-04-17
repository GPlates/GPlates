
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

#ifndef GPLATES_APP_LOGIC_RECONSTRUCTIONGRAPHBUILDER_H
#define GPLATES_APP_LOGIC_RECONSTRUCTIONGRAPHBUILDER_H

#include <map>
#include <boost/noncopyable.hpp>
#include <utility>
#include <vector>

#include "ReconstructionGraph.h"

#include "maths/FiniteRotation.h"

#include "model/types.h"

#include "property-values/GeoTimeInstant.h"


namespace GPlatesAppLogic
{
	/**
	 * Build a reconstruction graph by first inserting total reconstruction sequences and then building the graph.
	 */
	class ReconstructionGraphBuilder :
			private boost::noncopyable
	{
	public:

		//! Typedef for the value of a total reconstruction pole at a particular time instant.
		typedef std::pair<
				GPlatesPropertyValues::GeoTimeInstant,
				GPlatesMaths::FiniteRotation>
						total_reconstruction_pole_time_sample_type;

		//! Typedef for the value of a time-dependent total reconstruction pole (a sequence of time samples).
		typedef std::vector<total_reconstruction_pole_time_sample_type> total_reconstruction_pole_type;


		/**
		 * Create a @a ReconstructionGraphBuilder in order to build a @a ReconstructionGraph in order
		 * to create a @a ReconstructionTree at any reconstruction time.
		 *
		 * If @a extend_total_reconstruction_poles_to_distant_past is true then each moving plate
		 * sequence is extended back to the distant past such that any @a ReconstructionTree objects
		 * created from the @a ReconstructionGraph will not cause reconstructed geometries to snap
		 * back to their present day positions.
		 * Here the pole at the oldest time of the oldest fixed plate sequence of each moving plate is
		 * extended to the distant past such that pole at all older times match. For example, a
		 * moving plate 9 might move relative to plate 7 from 0-200Ma and relative to plate 8 from 200-400Ma,
		 * and so the pole 8->9 at 400Ma is extended back to the distant past (Infinity).
		 * By default we respect the time ranges in the input total reconstruction sequences.
		 */
		explicit
		ReconstructionGraphBuilder(
				bool extend_total_reconstruction_poles_to_distant_past_ = false);

		/**
		 * Insert a total reconstruction sequence for the specified fixed/moving plate pair.
		 *
		 * This incrementally builds the reconstruction graph internally.
		 *
		 * The time-dependent total reconstruction pole is specified with @a pole.
		 *
		 * Note: The total reconstruction sequence is ignored if it contains less than two pole time samples.
		 *       We need at least two enabled time samples in the total reconstruction sequence in order
		 *       to have a meaningful sequence (ie, something that's valid at times other than present day).
		 */
		void
		insert_total_reconstruction_sequence(
				GPlatesModel::integer_plate_id_type fixed_plate_id,
				GPlatesModel::integer_plate_id_type moving_plate_id,
				const total_reconstruction_pole_type &pole);

		/**
		 * Return the graph created from previous calls to @a insert_total_reconstruction_sequence.
		 *
		 * Subsequent calls to @a insert_total_reconstruction_sequence (if any) will then build
		 * a new graph that can be returned by another call to @a build_graph.
		 */
		ReconstructionGraph::non_null_ptr_to_const_type
		build_graph();

	private:
		ReconstructionGraph::non_null_ptr_type d_reconstruction_graph;
		bool d_extend_total_reconstruction_poles_to_distant_past;

		void
		extend_total_reconstruction_poles_to_distant_past();
	};
}

#endif // GPLATES_APP_LOGIC_RECONSTRUCTIONGRAPHBUILDER_H
