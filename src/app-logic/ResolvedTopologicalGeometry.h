/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_RESOLVEDTOPOLOGICALGEOMETRY_H
#define GPLATES_APP_LOGIC_RESOLVEDTOPOLOGICALGEOMETRY_H

#include <cstddef>
#include <iterator>
#include <vector>
#include <boost/operators.hpp>
#include <boost/optional.hpp>

#include "ReconstructionGeometry.h"
#include "ReconstructionGeometryVisitor.h"
#include "ReconstructionTree.h"
#include "ReconstructionTreeCreator.h"

#include "maths/GeometryOnSphere.h"

#include "model/FeatureHandle.h"
#include "model/types.h"
#include "model/WeakObserver.h"
#include "model/WeakObserverVisitor.h"

#include "property-values/GeoTimeInstant.h"


namespace GPlatesAppLogic
{
	/**
	 * Abstract base class for @a ResolvedTopologicalBoundary and @a ResolvedTopologicalLine.
	 */
	class ResolvedTopologicalGeometry :
			public ReconstructionGeometry,
			public GPlatesModel::WeakObserver<GPlatesModel::FeatureHandle>
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a ResolvedTopologicalGeometry.
		typedef GPlatesUtils::non_null_intrusive_ptr<ResolvedTopologicalGeometry> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a non-const @a ResolvedTopologicalGeometry.
		typedef GPlatesUtils::non_null_intrusive_ptr<const ResolvedTopologicalGeometry> non_null_ptr_to_const_type;

		//! A convenience typedef for boost::intrusive_ptr<ResolvedTopologicalGeometry>.
		typedef boost::intrusive_ptr<ResolvedTopologicalGeometry> maybe_null_ptr_type;

		//! A convenience typedef for boost::intrusive_ptr<const ResolvedTopologicalGeometry>.
		typedef boost::intrusive_ptr<const ResolvedTopologicalGeometry> maybe_null_ptr_to_const_type;

		//! A convenience typedef for the WeakObserver base class of this class.
		typedef GPlatesModel::WeakObserver<GPlatesModel::FeatureHandle> WeakObserverType;

		//! A convenience typedef for the geometry of this @a ResolvedTopologicalGeometry.
		typedef GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type resolved_topology_geometry_ptr_type;


		virtual
		~ResolvedTopologicalGeometry()
		{  }

		/**
		 * Access the resolved topology geometry.
		 *
		 * This is a polygon for @a ResolvedTopologicalBoundary subclass or a polyline for
		 * @a ResolvedTopologicalLine subclass.
		 */
		virtual
		const resolved_topology_geometry_ptr_type
		resolved_topology_geometry() const = 0;

		/**
		 * Get a non-null pointer to a const @a ResolvedTopologicalGeometry which points to this
		 * instance.
		 */
		const non_null_ptr_to_const_type
		get_non_null_pointer_to_const() const
		{
			return GPlatesUtils::get_non_null_pointer(this);
		}

		/**
		 * Get a non-null pointer to a @a ResolvedTopologicalGeometry which points to this
		 * instance.
		 */
		const non_null_ptr_type
		get_non_null_pointer()
		{
			return GPlatesUtils::get_non_null_pointer(this);
		}

		/**
		 * Access the ReconstructionTree that was used to reconstruct this ReconstructionGeometry.
		 */
		ReconstructionTree::non_null_ptr_to_const_type
		get_reconstruction_tree() const
		{
			return d_reconstruction_tree;
		}

		/**
		 * Gets the reconstruction tree creator that uses the same anchor plate and reconstruction
		 * features as used to create the tree returned by @a get_reconstruction_tree.
		 */
		ReconstructionTreeCreator
		get_reconstruction_tree_creator() const
		{
			return d_reconstruction_tree_creator;
		}

		/**
		 * Return whether this RTG references @a that_feature_handle.
		 *
		 * This function will not throw.
		 */
		bool
		references(
				const GPlatesModel::FeatureHandle &that_feature_handle) const
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
		GPlatesModel::FeatureHandle *
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
		 * Return a weak-ref to the feature whose resolved topological geometry this RTG
		 * contains, or an invalid weak-ref, if this pointer is not valid to be
		 * dereferenced.
		 */
		const GPlatesModel::FeatureHandle::weak_ref
		get_feature_ref() const
		{
			return is_valid()
					? feature_handle_ptr()->reference()
					: GPlatesModel::FeatureHandle::weak_ref();
		}

		/**
		 * Access the topological geometry feature property used to generate
		 * the resolved topological geometry.
		 */
		const GPlatesModel::FeatureHandle::iterator
		property() const
		{
			return d_property_iterator;
		}

		/**
		 * Access the cached plate ID, if it exists.
		 *
		 * Note that it's possible for a @a ResolvedTopologicalGeometry to be created without
		 * a plate ID -- for example, if no plate ID is found amongst the properties of the
		 * feature whose topological geometry was resolved.
		 */
		const boost::optional<GPlatesModel::integer_plate_id_type> &
		plate_id() const
		{
			return d_plate_id;
		}

		/**
		 * Return the cached time of formation of the feature.
		 */
		const boost::optional<GPlatesPropertyValues::GeoTimeInstant> &
		time_of_formation() const
		{
			return d_time_of_formation;
		}


		/**
		 * Accept a ConstReconstructionGeometryVisitor instance.
		 */
		virtual
		void
		accept_visitor(
				ConstReconstructionGeometryVisitor &visitor) const
		{
			visitor.visit(this->get_non_null_pointer_to_const());
		}


		/**
		 * Accept a ReconstructionGeometryVisitor instance.
		 */
		virtual
		void
		accept_visitor(
				ReconstructionGeometryVisitor &visitor)
		{
			visitor.visit(this->get_non_null_pointer());
		}

		/**
		 * Accept a WeakObserverVisitor instance.
		 */
		virtual
		void
		accept_weak_observer_visitor(
				GPlatesModel::WeakObserverVisitor<GPlatesModel::FeatureHandle> &visitor)
		{
			visitor.visit_resolved_topological_geometry(*this);
		}

	protected:

		/**
		 * Instantiate a resolved topological geometry with an optional reconstruction
		 * plate ID and an optional time of formation.
		 */
		ResolvedTopologicalGeometry(
				const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree_,
				const ReconstructionTreeCreator &reconstruction_tree_creator,
				GPlatesModel::FeatureHandle &feature_handle,
				GPlatesModel::FeatureHandle::iterator property_iterator_,
				boost::optional<GPlatesModel::integer_plate_id_type> plate_id_,
				boost::optional<GPlatesPropertyValues::GeoTimeInstant> time_of_formation_,
				boost::optional<ReconstructHandle::type> reconstruct_handle_):
			ReconstructionGeometry(reconstruction_tree_->get_reconstruction_time(), reconstruct_handle_),
			WeakObserverType(feature_handle),
			d_reconstruction_tree(reconstruction_tree_),
			d_reconstruction_tree_creator(reconstruction_tree_creator),
			d_property_iterator(property_iterator_),
			d_plate_id(plate_id_),
			d_time_of_formation(time_of_formation_)
		{  }

	private:

		/**
		 * The reconstruction tree used to reconstruct us.
		 */
		ReconstructionTree::non_null_ptr_to_const_type d_reconstruction_tree;

		/**
		 * Used to create reconstruction trees similar that the tree used to reconstruction 'this'
		 * reconstruction geometry (the only difference being the reconstruction time).
		 */
		ReconstructionTreeCreator d_reconstruction_tree_creator;

		/**
		 * This is an iterator to the (topological-geometry-valued) property from which
		 * this RTG was derived.
		 */
		GPlatesModel::FeatureHandle::iterator d_property_iterator;

		/**
		 * The cached plate ID, if it exists.
		 *
		 * Note that it's possible for a @a ResolvedTopologicalGeometry to be created without
		 * a plate ID -- for example, if no plate ID is found amongst the properties of the
		 * feature whose topological geometry was resolved.
		 *
		 * The plate ID is used when colouring feature geometries by plate ID.  It's also
		 * of interest to a user who has clicked on the feature geometry.
		 */
		boost::optional<GPlatesModel::integer_plate_id_type> d_plate_id;

		/**
		 * The cached time of formation of the feature, if it exists.
		 *
		 * This is cached so that it can be used to calculate the age of the feature at any
		 * particular reconstruction time.  The age of the feature is used when colouring
		 * feature geometries by age.
		 */
		boost::optional<GPlatesPropertyValues::GeoTimeInstant> d_time_of_formation;
	};
}

#endif  // GPLATES_APP_LOGIC_RESOLVEDTOPOLOGICALGEOMETRY_H
