/* $Id$ */

/**
 * \file 
 * Contains the definition of the class ReconstructionGraph.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_RECONSTRUCTIONGRAPH_H
#define GPLATES_APP_LOGIC_RECONSTRUCTIONGRAPH_H

#include <map>
#include <boost/intrusive/slist.hpp>
#include <boost/optional.hpp>
#include <boost/pool/object_pool.hpp>

#include "maths/FiniteRotation.h"
#include "maths/Real.h"

#include "model/types.h"

#include "property-values/GeoTimeInstant.h"

#include "utils/ReferenceCount.h"


namespace GPlatesAppLogic
{
	/**
	 * A reconstruction graph represents a plate circuit rotation hierarchy.
	 *
	 * A reconstruction *graph* can contain cycles due to crossovers (when a moving plate switches
	 * fixed plates at a particular time) because each edge represents a total reconstruction *sequence*
	 * (which contains a pole over a range of times).
	 *
	 * By specifying a reconstruction time and an anchor plate ID, you can create a ReconstructionTree
	 * rooted at the anchor plate and taking the path through the reconstruction graph at the
	 * reconstruction time. However, in contrast to a ReconstructionGraph, a ReconstructTree is *acyclic*
	 * and only takes one of the possible paths through a crossover (when the reconstruction time
	 * matches a moving plate's crossover time).
	 *
	 * NOTE: A @a ReconstructionGraph should be created using a @a ReconstructionGraphBuilder.
	 */
	class ReconstructionGraph :
			public GPlatesUtils::ReferenceCount<ReconstructionGraph>
	{
	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<ReconstructionGraph> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const ReconstructionGraph> non_null_ptr_to_const_type;

		//
		// NOTE: ReconstructionGraphBuilder should be used to create ReconstructionGraph.
		//


		// Some setup needed for an intrusive list of pole samples.
		typedef boost::intrusive::slist_base_hook<
				// Turn off safe linking (the default link mode) because it asserts if we destroy
				// a pole sample before removing it from a list (eg, a list inside an edge).
				// Well, we actually destroy all edges before pole samples, so this never actually happens.
				// But since we're destroying the entire graph in one go, we don't want the list destructors
				// to go to the extra (unnecessary) effort of setting each hook in list to default hook state...
				//
				// NOTE: This should be changed back to 'safe_link' if debugging a problem though...
				boost::intrusive::link_mode<boost::intrusive::normal_link> >
						pole_sample_list_base_hook_type;

		/**
		 * Represents the finite rotation value of a pole at a specific time instant.
		 */
		class PoleSample :
				// Using an intrusive list avoids extra memory allocations and is generally faster...
				public pole_sample_list_base_hook_type
		{
		public:

			const GPlatesPropertyValues::GeoTimeInstant &
			get_time_instant() const
			{
				return d_time_instant;
			}

			/**
			 * Return the total rotation from @a get_time_instant to present day of the pole's fixed/moving plate pair.
			 */
			const GPlatesMaths::FiniteRotation &
			get_finite_rotation() const
			{
				return d_finite_rotation;
			}

		private:

			friend class ReconstructionGraphBuilder;
			friend class boost::object_pool<PoleSample>;  // Access to Pole constructor.

			PoleSample(
					const GPlatesPropertyValues::GeoTimeInstant &time_instant,
					const GPlatesMaths::FiniteRotation &finite_rotation) :
				d_time_instant(time_instant),
				d_finite_rotation(finite_rotation)
			{  }

			GPlatesPropertyValues::GeoTimeInstant d_time_instant;
			GPlatesMaths::FiniteRotation d_finite_rotation;
		};

		/**
		 * Typedef for a list of pole samples.
		 *
		 * Note: Using a *singly*-linked list since don't need to do insertions (saves a small amount of memory).
		 */
		typedef boost::intrusive::slist<PoleSample,
				boost::intrusive::base_hook<pole_sample_list_base_hook_type>,
				boost::intrusive::cache_last<true>/*enable push_back() and back()*/>
						pole_sample_list_type;


		// Some setup needed for an intrusive list of plate *incoming* edges.
		class PlateIncomingEdgeTag;
		typedef boost::intrusive::slist_base_hook<
				boost::intrusive::tag<PlateIncomingEdgeTag>,
				// Turn off safe linking (the default link mode) because it asserts if we destroy
				// an edge before removing it from a list (eg, a list inside a plate).
				// Well, we actually destroy all plates before edges, so this never actually happens.
				// But since we're destroying the entire graph in one go, we don't want the list destructors
				// to go to the extra (unnecessary) effort of setting each hook in list to default hook state...
				//
				// NOTE: This should be changed back to 'safe_link' if debugging a problem though...
				boost::intrusive::link_mode<boost::intrusive::normal_link> >
						plate_incoming_edge_list_base_hook_type;

		// Some setup needed for an intrusive list of plate *outgoing* edges.
		class PlateOutgoingEdgeTag;
		typedef boost::intrusive::slist_base_hook<
				boost::intrusive::tag<PlateOutgoingEdgeTag>,
				// Turn off safe linking (the default link mode) because it asserts if we destroy
				// an edge before removing it from a list (eg, a list inside a plate).
				// Well, we actually destroy all plates before edges, so this never actually happens.
				// But since we're destroying the entire graph in one go, we don't want the list destructors
				// to go to the extra (unnecessary) effort of setting each hook in list to default hook state...
				//
				// NOTE: This should be changed back to 'safe_link' if debugging a problem though...
				boost::intrusive::link_mode<boost::intrusive::normal_link> >
						plate_outgoing_edge_list_base_hook_type;

		class Plate;

		/**
		 * Represents the relative rotation from a fixed @a Plate to a moving @a Plate.
		 *
		 * These are the edges in the graph.
		 */
		class Edge :
				// Using intrusive lists avoids extra memory allocations and is generally faster...
				public plate_incoming_edge_list_base_hook_type,
				public plate_outgoing_edge_list_base_hook_type
		{
		public:

			const Plate &
			get_fixed_plate() const
			{
				return *d_fixed_plate;
			}

			const Plate &
			get_moving_plate() const
			{
				return *d_moving_plate;
			}

			/**
			 * Return the sequence of pole time samples.
			 *
			 * These are ordered from youngest to oldest (same as in a rotation feature or file).
			 *
			 * Note: This is guaranteed to be at least two time samples.
			 */
			const pole_sample_list_type &
			get_pole() const
			{
				return d_pole;
			}

			/**
			 * Return the time of the *oldest* pole sample.
			 */
			const GPlatesPropertyValues::GeoTimeInstant &
			get_begin_time() const
			{
				return d_pole.back().get_time_instant();
			}

			/**
			 * Return the time of the *youngest* pole sample.
			 */
			const GPlatesPropertyValues::GeoTimeInstant &
			get_end_time() const
			{
				return d_pole.front().get_time_instant();
			}

		private:

			friend class ReconstructionGraphBuilder;
			friend class boost::object_pool<Edge>;  // Access to Edge constructor.

			Edge(
					Plate *fixed_plate,
					Plate *moving_plate) :
				d_fixed_plate(fixed_plate),
				d_moving_plate(moving_plate)
			{  }

			Plate *d_fixed_plate;
			Plate *d_moving_plate;
			pole_sample_list_type d_pole;
		};


		/**
		 * Typedef for a list of edges going *into* a plate (edge direction is from fixed plate to moving plate).
		 *
		 * In this sense the plate is the *moving* plate of the edges.
		 *
		 * Note: Using a *singly*-linked list since don't need to do insertions (saves a small amount of memory).
		 */
		typedef boost::intrusive::slist<Edge, boost::intrusive::base_hook<plate_incoming_edge_list_base_hook_type> >
				plate_incoming_edge_list_type;

		/**
		 * Typedef for a list of edges going *out* of a plate (edge direction is from fixed plate to moving plate).
		 *
		 * In this sense the plate is the *fixed* plate of the edges.
		 *
		 * Note: Using a *singly*-linked list since don't need to do insertions (saves a small amount of memory).
		 */
		typedef boost::intrusive::slist<Edge, boost::intrusive::base_hook<plate_outgoing_edge_list_base_hook_type> >
				plate_outgoing_edge_list_type;

		/**
		 * Represents a plate (ID).
		 *
		 * These are the nodes in the graph.
		 */
		class Plate
		{
		public:

			GPlatesModel::integer_plate_id_type
			get_plate_id() const
			{
				return d_plate_id;
			}
			
			/**
			 * List of edges going *into* this plate (edge direction is from fixed plate to moving plate).
			 *
			 * This plate is the *moving* plate of these edges.
			 */
			const plate_incoming_edge_list_type &
			get_incoming_edges() const
			{
				return d_incoming_edges;
			}

			/**
			 * List of edges going *out* of this plate (edge direction is from fixed plate to moving plate).
			 *
			 * This plate is the *fixed* plate of these edges.
			 */
			const plate_outgoing_edge_list_type &
			get_outgoing_edges() const
			{
				return d_outgoing_edges;
			}

		private:

			friend class ReconstructionGraphBuilder;
			friend class boost::object_pool<Plate>;  // Access to Plate constructor.

			explicit
			Plate(
					GPlatesModel::integer_plate_id_type plate_id) :
				d_plate_id(plate_id)
			{  }

			GPlatesModel::integer_plate_id_type d_plate_id;
			plate_incoming_edge_list_type d_incoming_edges;
			plate_outgoing_edge_list_type d_outgoing_edges;
		};


		/**
		 * Return the @a Plate associated with the specified plate ID.
		 *
		 * This is typically used to obtain the anchor @a Plate when creating a @a ReconstructionTree.
		 * Then the graph can be traversed from there through the @a Edge and @a Plate objects.
		 */
		boost::optional<const Plate &>
		get_plate(
				GPlatesModel::integer_plate_id_type plate_id) const
		{
			plate_map_type::const_iterator plate_iter = d_plate_map.find(plate_id);
			if (plate_iter == d_plate_map.end())
			{
				return boost::none;
			}

			return *plate_iter->second;
		}

	private:

		friend class ReconstructionGraphBuilder;

		static
		non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new ReconstructionGraph());
		}

		ReconstructionGraph()
		{  }


		//! Typedef for mapping plate IDs to @a Plate objects.
		typedef std::map<GPlatesModel::integer_plate_id_type, Plate *> plate_map_type;

		// Storage for the pole samples, edges and plates.
		boost::object_pool<PoleSample> d_pole_sample_pool;
		boost::object_pool<Edge> d_edge_pool;
		boost::object_pool<Plate> d_plate_pool;

		plate_map_type d_plate_map;
	};
}

#endif  // GPLATES_APP_LOGIC_RECONSTRUCTIONGRAPH_H
