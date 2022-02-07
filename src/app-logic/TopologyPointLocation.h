/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2016 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_TOPOLOGYPOINTLOCATION_H
#define GPLATES_APP_LOGIC_TOPOLOGYPOINTLOCATION_H

#include <utility>

#include <boost/optional.hpp>
#include <boost/ref.hpp>
#include <boost/variant.hpp>

#include "ResolvedTopologicalBoundary.h"
#include "ResolvedTopologicalNetwork.h"
#include "ResolvedTriangulationNetwork.h"


namespace GPlatesAppLogic
{
	/**
	 * Optional location of a point in a resolved topological boundary or network.
	 *
	 * This class is mainly to reduce memory usage since each point in each geometry,
	 * that is reconstructed using topologies, will store one of these for each time slot
	 * (in time history of topology reconstructions).
	 *
	 * Size is reduced (for 32-bit builds) to 12 bytes, from 20 bytes for a:
	 *     boost::optional<
	 *         boost::variant<
	 *             // resolved boundary...
	 *             ...,
	 *             // resolved network...
	 *             std::pair<
	 *                 ResolvedTriangulation::Network::point_location_type,
	 *                 ResolvedTopologicalNetwork::non_null_ptr_type>
	 *             >
	 *         >
	 *     >
	 */
	class TopologyPointLocation
	{
	public:

		//! Location in a network (delaunay face or rigid block).
		typedef std::pair<
				ResolvedTopologicalNetwork::non_null_ptr_type,
				ResolvedTriangulation::Network::point_location_type>
						network_location_type;


		//! Point is not located inside resolved boundaries/networks (ie, is outside all resolved boundaries/networks).
		TopologyPointLocation() :
			d_location(NoLocation())
		{  }

		//! Point located inside resolved boundary.
		explicit
		TopologyPointLocation(
				const ResolvedTopologicalBoundary::non_null_ptr_type &boundary) :
			d_location(BoundaryLocation(boundary))
		{  }

		//! Point located inside resolved network.
		TopologyPointLocation(
				const ResolvedTopologicalNetwork::non_null_ptr_type &network,
				const ResolvedTriangulation::Network::point_location_type &network_point_location) :
			d_location(boost::apply_visitor(ConstructNetworkVisitor(network), network_point_location))
		{  }


		//! Returns true if point is not located inside resolved boundaries/networks (ie, is outside all resolved boundaries/networks).
		bool
		not_located() const
		{
			return boost::apply_visitor(NoLocationVisitor(), d_location);
		}

		//! Returns resolved boundary that point is located in (otherwise returns none).
		boost::optional<ResolvedTopologicalBoundary::non_null_ptr_type>
		located_in_resolved_boundary() const
		{
			return boost::apply_visitor(BoundaryLocationVisitor(), d_location);
		}

		//! Returns resolved network location that point is located in (otherwise returns none).
		boost::optional<network_location_type>
		located_in_resolved_network() const
		{
			return boost::apply_visitor(NetworkLocationVisitor(), d_location);
		}

	private:

		struct NoLocation
		{  };

		struct BoundaryLocation
		{
			explicit
			BoundaryLocation(
					const ResolvedTopologicalBoundary::non_null_ptr_type &boundary_) :
				boundary(boundary_)
			{  }

			ResolvedTopologicalBoundary::non_null_ptr_type boundary;
		};

		struct NetworkDelaunayFaceLocation
		{
			NetworkDelaunayFaceLocation(
					const ResolvedTopologicalNetwork::non_null_ptr_type &network_,
					const ResolvedTriangulation::Delaunay_2::Face_handle &delauny_face_) :
				network(network_),
				delauny_face(delauny_face_)
			{  }

			ResolvedTopologicalNetwork::non_null_ptr_type network;
			ResolvedTriangulation::Delaunay_2::Face_handle delauny_face;
		};

		struct NetworkRigidBlockLocation
		{
			NetworkRigidBlockLocation(
					const ResolvedTopologicalNetwork::non_null_ptr_type &network_,
					const ResolvedTriangulation::Network::RigidBlock &rigid_block_) :
				network(network_),
				rigid_block(rigid_block_)
			{  }

			ResolvedTopologicalNetwork::non_null_ptr_type network;
			boost::reference_wrapper<const ResolvedTriangulation::Network::RigidBlock> rigid_block; // behaves like 'const RigidBlock &'
		};

		//! Typedef for location of a point.
		typedef boost::variant<
				NoLocation,
				BoundaryLocation,
				NetworkDelaunayFaceLocation,
				NetworkRigidBlockLocation
		> location_type;


		//! Construct a 'location_type' from a 'ResolvedTriangulation::Network::location_type'.
		struct ConstructNetworkVisitor :
				public boost::static_visitor<location_type>
		{
			explicit
			ConstructNetworkVisitor(
					const ResolvedTopologicalNetwork::non_null_ptr_type &network) :
				d_network(network)
			{  }

			location_type
			operator()(
					const ResolvedTriangulation::Delaunay_2::Face_handle &delaunay_face) const
			{
				return location_type(NetworkDelaunayFaceLocation(d_network, delaunay_face));
			}

			location_type
			operator()(
					const ResolvedTriangulation::Network::RigidBlock &rigid_block) const
			{
				return location_type(NetworkRigidBlockLocation(d_network, rigid_block));
			}

			ResolvedTopologicalNetwork::non_null_ptr_type d_network;
		};

		//! Returns true if point not located in resolved boundaries/networks.
		struct NoLocationVisitor :
				public boost::static_visitor<bool>
		{
			bool
			operator()(
					const NoLocation &) const
			{
				return true;
			}

			template <class LocationType>
			bool
			operator()(
					const LocationType &) const
			{
				return false;
			}
		};

		//! Returns resolved boundary that point is located in (otherwise returns none).
		struct BoundaryLocationVisitor :
				public boost::static_visitor< boost::optional<ResolvedTopologicalBoundary::non_null_ptr_type> >
		{
			boost::optional<ResolvedTopologicalBoundary::non_null_ptr_type>
			operator()(
					const BoundaryLocation &boundary_location) const
			{
				return boundary_location.boundary;
			}

			template <class LocationType>
			boost::optional<ResolvedTopologicalBoundary::non_null_ptr_type>
			operator()(
					const LocationType &) const
			{
				return boost::none;
			}
		};

		//! Returns resolved network location that point is located in (otherwise returns none).
		struct NetworkLocationVisitor :
				public boost::static_visitor< boost::optional<network_location_type> >
		{
			boost::optional<network_location_type>
			operator()(
					const NetworkDelaunayFaceLocation &network_delaunay_face_location) const
			{
				return network_location_type(
						network_delaunay_face_location.network,
						network_delaunay_face_location.delauny_face);
			}

			boost::optional<network_location_type>
			operator()(
					const NetworkRigidBlockLocation &network_rigid_block_location) const
			{
				return network_location_type(
						network_rigid_block_location.network,
						network_rigid_block_location.rigid_block);
			}

			template <class LocationType>
			boost::optional<network_location_type>
			operator()(
					const LocationType &) const
			{
				return boost::none;
			}
		};


		//! The location of the point.
		location_type d_location;
	};
}

#endif // GPLATES_APP_LOGIC_TOPOLOGYPOINTLOCATION_H
