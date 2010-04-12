/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_RESOLVEDTOPOLOGICALNETWORK_H
#define GPLATES_MODEL_RESOLVEDTOPOLOGICALNETWORK_H

#include <cstddef>
#include <iterator>
#include <vector>
#include <boost/operators.hpp>
#include <boost/optional.hpp>

#include "FeatureHandle.h"
#include "ReconstructionGeometry.h"
#include "ResolvedTopologicalNetworkImpl.h"
#include "WeakObserver.h"
#include "types.h"

#include "maths/PolygonOnSphere.h"
#include "property-values/GeoTimeInstant.h"


namespace GPlatesModel
{
	class ResolvedTopologicalNetwork:
			public ReconstructionGeometry,
			public WeakObserver<FeatureHandle>
	{
	public:
		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<ResolvedTopologicalNetwork,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<ResolvedTopologicalNetwork,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<const ResolvedTopologicalNetwork,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const ResolvedTopologicalNetwork,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_to_const_type;

		/**
		 * A convenience typedef for boost::intrusive_ptr<ResolvedTopologicalNetwork>.
		 */
		typedef boost::intrusive_ptr<ResolvedTopologicalNetwork> maybe_null_ptr_type;

		/**
		 * A convenience typedef for the WeakObserver base class of this class.
		 */
		typedef WeakObserver<FeatureHandle> WeakObserverType;

		/**
		 * A convenience typedef for the geometry of this @a ResolvedTopologicalNetwork.
		 */
		typedef GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type
				resolved_topology_geometry_ptr_type;


		/**
		 * Create a ResolvedTopologicalNetwork instance.
		 */
		static
		const non_null_ptr_type
		create(
				resolved_topology_geometry_ptr_type resolved_topology_geometry_ptr,
				FeatureHandle &feature_handle,
				const ResolvedTopologicalNetworkImpl::non_null_ptr_type &network)
		{
			non_null_ptr_type ptr(
					new ResolvedTopologicalNetwork(
							resolved_topology_geometry_ptr, feature_handle, network),
					GPlatesUtils::NullIntrusivePointerHandler());
			return ptr;
		}

		virtual
		~ResolvedTopologicalNetwork()
		{  }

		/**
		 * Get a non-null pointer to a const ResolvedTopologicalNetwork which points to this
		 * instance.
		 *
		 * Since the ResolvedTopologicalNetwork constructors are private, it should never
		 * be the case that a ResolvedTopologicalNetwork instance has been constructed on
		 * the stack.
		 */
		const non_null_ptr_to_const_type
		get_non_null_pointer_to_const() const;

		/**
		 * Get a non-null pointer to a ResolvedTopologicalNetwork which points to this
		 * instance.
		 *
		 * Since the ResolvedTopologicalNetwork constructors are private, it should never
		 * be the case that a ResolvedTopologicalNetwork instance has been constructed on
		 * the stack.
		 */
		const non_null_ptr_type
		get_non_null_pointer();

		/**
		 * Return the internal @a ResolvedTopologicalNetworkImpl that contains the full topological network information.
		 *
		 * @a ResolvedTopologicalNetwork currently only represents a single triangle in
		 * a network's triangulation.
		 */
		const ResolvedTopologicalNetworkImpl &
		get_network() const
		{
			return *d_network_impl;
		}

		/**
		 * Return the internal @a ResolvedTopologicalNetworkImpl that contains the full topological network information.
		 *
		 * @a ResolvedTopologicalNetwork currently only represents a single triangle in
		 * a network's triangulation.
		 */
		ResolvedTopologicalNetworkImpl &
		get_network()
		{
			return *d_network_impl;
		}

		/**
		 * Return whether this RTN references @a that_feature_handle.
		 *
		 * This function will not throw.
		 */
		bool
		references(
				const FeatureHandle &that_feature_handle) const
		{
			return (feature_handle_ptr() == &that_feature_handle);
		}

		/**
		 * Return the pointer to the FeatureHandle.
		 *
		 * The pointer returned will be NULL if this instance does not reference a
		 * FeatureHandle; non-NULL otherwise.
		 *
		 * This function will not throw.
		 */
		FeatureHandle *
		feature_handle_ptr() const
		{
			return WeakObserverType::publisher_ptr();
		}

		/**
		 * Return whether this pointer is valid to be dereferenced (to obtain a
		 * FeatureHandle).
		 *
		 * This function will not throw.
		 */
		bool
		is_valid() const
		{
			return (feature_handle_ptr() != NULL);
		}

		/**
		 * Return a weak-ref to the feature whose resolved topological geometry this RTN
		 * contains, or an invalid weak-ref, if this pointer is not valid to be
		 * dereferenced.
		 */
		const FeatureHandle::weak_ref
		get_feature_ref() const;

		/**
		 * Access the topological polygon feature property used to generate
		 * the resolved topological geometry.
		 */
		const FeatureHandle::iterator
		property() const
		{
			return d_network_impl->property();
		}

		/**
		 * Access the resolved topology polygon geometry.
		 *
		 * This returns the same geometry as the base class @a geometry method does but
		 * returns it as a @a resolved_topology_geometry_ptr_type instead
		 * of a @a geometry_ptr_type.
		 */
		const resolved_topology_geometry_ptr_type
		resolved_topology_geometry() const;

		/**
		 * Access the cached plate ID, if it exists.
		 *
		 * Note that it's possible for a ResolvedTopologicalNetwork to be created without
		 * a plate ID -- for example, if no plate ID is found amongst the properties of the
		 * feature whose topological geometry was resolved.
		 */
		const boost::optional<integer_plate_id_type> &
		plate_id() const
		{
			return d_network_impl->plate_id();
		}

		/**
		 * Return the cached time of formation of the feature.
		 */
		const boost::optional<GPlatesPropertyValues::GeoTimeInstant> &
		time_of_formation() const
		{
			return d_network_impl->time_of_formation();
		}


		/**
		 * Accept a ConstReconstructionGeometryVisitor instance.
		 */
		virtual
		void
		accept_visitor(
				ConstReconstructionGeometryVisitor &visitor) const;

		/**
		 * Accept a ReconstructionGeometryVisitor instance.
		 */
		virtual
		void
		accept_visitor(
				ReconstructionGeometryVisitor &visitor);

		/**
		 * Accept a WeakObserverVisitor instance.
		 */
		virtual
		void
		accept_weak_observer_visitor(
				WeakObserverVisitor<FeatureHandle> &visitor);

	private:
		ResolvedTopologicalNetworkImpl::non_null_ptr_type d_network_impl;


		/**
		 * Instantiate a resolved topological network.
		 *
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		ResolvedTopologicalNetwork(
				resolved_topology_geometry_ptr_type resolved_topology_geometry_ptr,
				FeatureHandle &feature_handle,
				const ResolvedTopologicalNetworkImpl::non_null_ptr_type &network) :
			ReconstructionGeometry(resolved_topology_geometry_ptr),
			WeakObserverType(feature_handle),
			d_network_impl(network)
		{  }
	};
}

#endif // GPLATES_MODEL_RESOLVEDTOPOLOGICALNETWORK_H
