/* $Id$ */

/**
 * \file 
 * Contains the definition of the class Reconstruction.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007, 2008 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_RECONSTRUCTION_H
#define GPLATES_APP_LOGIC_RECONSTRUCTION_H

#include <iterator>  // std::iterator
#include <map>
#include <boost/operators.hpp>
#include <boost/optional.hpp>

#include "ReconstructionGeometryCollection.h"
#include "ReconstructionTree.h"

#include "maths/Real.h"

#include "model/FeatureCollectionHandle.h"
#include "model/types.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesAppLogic
{
	/**
	 * This class represents a plate-tectonic reconstruction at a particular geological
	 * time-instant.
	 */
	class Reconstruction :
			public GPlatesUtils::ReferenceCount<Reconstruction>
	{
	private:
		/**
		 * Typedef for a mapping of reconstruction trees to of @a ReconstructionGeometry objects.
		 */
		typedef std::multimap<
				ReconstructionTree::non_null_ptr_to_const_type,
				GPlatesAppLogic::ReconstructionGeometryCollection::non_null_ptr_to_const_type>
						reconstruction_tree_map_type;

	public:
		/**
		 * A convenience typedef for a shared pointer to non-const @a Reconstruction.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<Reconstruction> non_null_ptr_type;

		/**
		 * A convenience typedef for a shared pointer to const @a Reconstruction.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const Reconstruction> non_null_ptr_to_const_type;


		/**
		 * Forward iterator over all @a ReconstructionGeometry objects in the @a Reconstruction.
		 * Dereferencing iterator returns a 'ReconstructionGeometry::non_null_ptr_to_const_type'.
		 */
		class ConstReconstructionGeometryIterator :
				public std::iterator<std::forward_iterator_tag, ReconstructionGeometry::non_null_ptr_to_const_type>,
				public boost::equality_comparable<ConstReconstructionGeometryIterator>,
				public boost::incrementable<ConstReconstructionGeometryIterator>
		{
		public:
			/**
			 * Create a "begin" iterator over the reconstruction geometries associated with
			 * @a reconstruction_tree.
			 */
			static
			ConstReconstructionGeometryIterator
			create_begin(
					const Reconstruction &reconstruction,
					const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree);


			/**
			 * Create a "end" iterator over the reconstruction geometries associated with
			 * @a reconstruction_tree.
			 */
			static
			ConstReconstructionGeometryIterator
			create_end(
					const Reconstruction &reconstruction,
					const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree);


			/**
			 * Dereference operator.
			 * No 'operator->()' is provided since we're returning a temporary object.
			 */
			const ReconstructionGeometry::non_null_ptr_to_const_type
			operator*() const;


			/**
			 * Pre-increment operator.
			 * Post-increment operator provided by base class boost::incrementable.
			 */
			ConstReconstructionGeometryIterator &
			operator++();


			/**
			 * Equality comparison for @a ConstReconstructionGeometryIterator.
			 * Inequality operator provided by base class boost::equality_comparable.
			 */
			friend
			bool
			operator==(
					const ConstReconstructionGeometryIterator &lhs,
					const ConstReconstructionGeometryIterator &rhs)
			{
				return
					(lhs.d_reconstruction_tree_map_iterator ==
							rhs.d_reconstruction_tree_map_iterator) &&
					(lhs.d_reconstruction_geometry_collection_iterator ==
							rhs.d_reconstruction_geometry_collection_iterator);
			}

		private:
			const Reconstruction *d_reconstruction;
			reconstruction_tree_map_type::const_iterator d_reconstruction_tree_map_iterator;
			boost::optional<GPlatesAppLogic::ReconstructionGeometryCollection::const_iterator>
					d_reconstruction_geometry_collection_iterator;


			ConstReconstructionGeometryIterator(
					const Reconstruction *reconstruction,
					reconstruction_tree_map_type::const_iterator reconstruction_tree_map_iterator,
					boost::optional<GPlatesAppLogic::ReconstructionGeometryCollection::const_iterator>
							reconstruction_geometry_collection_iterator = boost::none);
		};

		/**
		 * The type used to const_iterate over the reconstruction geometries associated
		 * with a reconstruction tree.
		 *
		 * Dereferencing iterator returns a 'ReconstructionGeometry::non_null_ptr_to_const_type'.
		 */
		typedef ConstReconstructionGeometryIterator reconstruction_geometry_const_iterator;


		/**
		 * Forward iterator over all @a ReconstructionTree objects in the @a Reconstruction.
		 * Dereferencing iterator returns a 'ReconstructionTree::non_null_ptr_to_const_type'.
		 */
		class ConstReconstructionTreeIterator :
				public std::iterator<std::forward_iterator_tag, ReconstructionTree::non_null_ptr_to_const_type>,
				public boost::equality_comparable<ConstReconstructionTreeIterator>,
				public boost::incrementable<ConstReconstructionTreeIterator>
		{
		public:
			/**
			 * Create a "begin" iterator over the reconstruction trees in @a reconstruction.
			 */
			static
			ConstReconstructionTreeIterator
			create_begin(
					const Reconstruction &reconstruction)
			{
				return ConstReconstructionTreeIterator(reconstruction.d_reconstruction_tree_map.begin());
			}


			/**
			 * Create a "end" iterator over the reconstruction trees in @a reconstruction.
			 */
			static
			ConstReconstructionTreeIterator
			create_end(
					const Reconstruction &reconstruction)
			{
				return ConstReconstructionTreeIterator(reconstruction.d_reconstruction_tree_map.end());
			}


			/**
			 * Dereference operator.
			 * No 'operator->()' is provided since we're returning a temporary object.
			 */
			const ReconstructionTree::non_null_ptr_to_const_type
			operator*() const
			{
				return d_reconstruction_tree_map_iterator->first;
			}


			/**
			 * Pre-increment operator.
			 * Post-increment operator provided by base class boost::incrementable.
			 */
			ConstReconstructionTreeIterator &
			operator++()
			{
				++d_reconstruction_tree_map_iterator;
				return *this;
			}


			/**
			 * Equality comparison for @a ConstReconstructionTreeIterator.
			 * Inequality operator provided by base class boost::equality_comparable.
			 */
			friend
			bool
			operator==(
					const ConstReconstructionTreeIterator &lhs,
					const ConstReconstructionTreeIterator &rhs)
			{
				return lhs.d_reconstruction_tree_map_iterator == rhs.d_reconstruction_tree_map_iterator;
			}

		private:
			reconstruction_tree_map_type::const_iterator d_reconstruction_tree_map_iterator;


			ConstReconstructionTreeIterator(
					reconstruction_tree_map_type::const_iterator reconstruction_tree_map_iterator) :
				d_reconstruction_tree_map_iterator(reconstruction_tree_map_iterator)
			{  }
		};

		/**
		 * The type used to const_iterate over the reconstruction trees in a @a Reconstruction.
		 *
		 * Dereferencing iterator returns a 'ReconstructionTree::non_null_ptr_to_const_type'.
		 */
		typedef ConstReconstructionTreeIterator reconstruction_tree_const_iterator;


		~Reconstruction();


		/**
		 * Create a new blank Reconstruction instance with the default reconstruction tree
		 * being an empty reconstruction tree (ie, returns identity rotations for all plates).
		 */
		static
		const non_null_ptr_type
		create(
				const double &reconstruction_time,
				GPlatesModel::integer_plate_id_type anchored_plate_id)
		{
			return non_null_ptr_type(
					new Reconstruction(reconstruction_time, anchored_plate_id),
					GPlatesUtils::NullIntrusivePointerHandler());
		}


		/**
		 * Create a new blank Reconstruction instance with the default reconstruction tree
		 * as @a default_reconstruction_tree.
		 */
		static
		const non_null_ptr_type
		create(
				const double &reconstruction_time,
				const ReconstructionTree::non_null_ptr_to_const_type &default_reconstruction_tree)
		{
			return non_null_ptr_type(
					new Reconstruction(reconstruction_time, default_reconstruction_tree),
					GPlatesUtils::NullIntrusivePointerHandler());
		}


		/**
		 * Adds @a collection to this reconstruction and explicitly associates @a collection
		 * with @a collection's reconstruction tree such that calling
		 * @a begin_reconstruction_geometries or @a end_reconstruction_geometries with that
		 * reconstruction tree as an argument will iterate over the @a ReconstructionGeometry
		 * objects contained in @a collection (and potentially other collections with the same
		 * reconstruction tree).
		 *
		 * Also sets reconstruction pointer of @a collection to 'this'.
		 * When 'this' is destroyed it will set the reconstruction pointer of @a collection to NULL.
		 *
		 * @throws PreconditionViolationError if the reconstruction time of @a collection
		 * is not the same as the reconstruction time passed into the constructor.
		 */
		void
		add_reconstruction_geometries(
				const GPlatesAppLogic::ReconstructionGeometryCollection::non_null_ptr_to_const_type &collection);

		
		/**
		 * Returns the reconstruction tree used to reconstruct layers that are not explicitly
		 * connected to an input reconstruction tree layer.
		 */
		ReconstructionTree::non_null_ptr_to_const_type
		get_default_reconstruction_tree() const
		{
			return d_default_reconstruction_tree;
		}


		/**
		 * Sets the reconstruction tree used to reconstruct layers that are not explicitly
		 * connected to an input reconstruction tree layer.
		 *
		 * If this is never called then the default reconstruction tree the one generated
		 * in the constructor.
		 */
		void
		set_default_reconstruction_tree(
				const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree)
		{
			d_default_reconstruction_tree = reconstruction_tree;
		}
				

		/**
		 * Returns the "begin" reconstruction_tree_const_iterator to iterate over the
		 * sequence of @a ReconstructionTree::non_null_ptr_to_const_type objects in
		 * this reconstruction.
		 */
		reconstruction_tree_const_iterator
		begin_reconstruction_trees() const
		{
			return reconstruction_tree_const_iterator::create_begin(*this);
		}


		/**
		 * Returns the "end" reconstruction_tree_const_iterator to iterate over the
		 * sequence of @a ReconstructionTree::non_null_ptr_to_const_type objects in
		 * this reconstruction.
		 */
		reconstruction_tree_const_iterator
		end_reconstruction_trees() const
		{
			return reconstruction_tree_const_iterator::create_end(*this);
		}


		/**
		 * Returns the "begin" reconstruction_geometry_const_iterator to iterate over the
		 * sequence of @a ReconstructionGeometry::non_null_ptr_to_const_type
		 * associated with @a reconstruction_tree.
		 */
		reconstruction_geometry_const_iterator
		begin_reconstruction_geometries(
				ReconstructionTree::non_null_ptr_to_const_type reconstruction_tree) const
		{
			return reconstruction_geometry_const_iterator::create_begin(*this, reconstruction_tree);
		}


		/**
		 * Returns the "end" reconstruction_geometry_const_iterator to iterate over the
		 * sequence of @a ReconstructionGeometry::non_null_ptr_to_const_type
		 * associated with @a reconstruction_tree.
		 */
		reconstruction_geometry_const_iterator
		end_reconstruction_geometries(
				ReconstructionTree::non_null_ptr_to_const_type reconstruction_tree) const
		{
			return reconstruction_geometry_const_iterator::create_end(*this, reconstruction_tree);
		}


		/**
		 * Returns the reconstruction time used for all reconstruction trees and
		 * all reconstructed geometries.
		 */
		const double &
		get_reconstruction_time() const
		{
			return d_reconstruction_time.dval();
		}

	private:
		/**
		 * The reconstruction time at which all reconstructions are performed.
		 */
		GPlatesMaths::Real d_reconstruction_time;

		/**
		 * The reconstruction tree used to reconstruct layers that are not explicitly
		 * connected to an input reconstruction tree layer.
		 */
		ReconstructionTree::non_null_ptr_to_const_type d_default_reconstruction_tree;

		/**
		 * Mapping from each ReconstructionTree to its group of ReconstructionGeometryCollection's.
		 */
		reconstruction_tree_map_type d_reconstruction_tree_map;

		/**
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		Reconstruction(
				const double &reconstruction_time,
				GPlatesModel::integer_plate_id_type anchored_plate_id);

		/**
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		Reconstruction(
				const double &reconstruction_time,
				const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree);
	};
}

#endif  // GPLATES_APP_LOGIC_RECONSTRUCTION_H
