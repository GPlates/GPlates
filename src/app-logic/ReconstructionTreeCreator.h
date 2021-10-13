/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_RECONSTRUCTIONTREECREATOR_H
#define GPLATES_APP_LOGIC_RECONSTRUCTIONTREECREATOR_H

#include <vector>

#include "ReconstructionTree.h"

#include "model/FeatureCollectionHandle.h"
#include "model/types.h"

#include "utils/ReferenceCount.h"


namespace GPlatesAppLogic
{
	/**
	 * Create and return a reconstruction tree for the reconstruction time @a time,
	 * with anchor plate id @a anchor_plate_id.
	 *
	 * The feature collections in @a reconstruction_features_collection are expected to
	 * contain reconstruction features (ie, total reconstruction sequences and absolute
	 * reference frames).
	 *
	 * If @a reconstruction_features_collection is empty then the returned @a ReconstructionTree
	 * will always give an identity rotation when queried for a composed absolute rotation.
	 */
	const ReconstructionTree::non_null_ptr_type
	create_reconstruction_tree(
			const double &reconstruction_time,
			GPlatesModel::integer_plate_id_type anchor_plate_id,
			const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
					reconstruction_features_collection =
							std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref>());


	class ReconstructionTreeCreatorImpl;

	/**
	 * A wrapper around an implementation for creating reconstruction trees.
	 *
	 * For example some implementations may cache reconstruction trees, others may delegate to a
	 * reconstruction layer proxy, but the interface for creating reconstruction trees remains the same.
	 */
	class ReconstructionTreeCreator
	{
	public:
		explicit
		ReconstructionTreeCreator(
				const GPlatesUtils::non_null_intrusive_ptr<ReconstructionTreeCreatorImpl> &impl);

		~ReconstructionTreeCreator();

		/**
		 * Returns the reconstruction tree for the specified time and anchored plate id.
		 */
		ReconstructionTree::non_null_ptr_to_const_type
		get_reconstruction_tree(
				const double &reconstruction_time,
				GPlatesModel::integer_plate_id_type anchor_plate_id) const;


		/**
		 * Returns the reconstruction tree for the specified time and the *default* anchored plate id.
		 */
		ReconstructionTree::non_null_ptr_to_const_type
		get_reconstruction_tree(
				const double &reconstruction_time) const;


		/**
		 * Returns the reconstruction tree for the specified anchored plate id and the *default* time.
		 */
		ReconstructionTree::non_null_ptr_to_const_type
		get_reconstruction_tree(
				GPlatesModel::integer_plate_id_type anchor_plate_id) const;


		/**
		 * Returns the reconstruction tree for the *default* time and the *default* anchored plate id.
		 */
		ReconstructionTree::non_null_ptr_to_const_type
		get_reconstruction_tree() const;


		/**
		 * Changes the default reconstruction time.
		 */
		void
		set_default_reconstruction_time(
				const double &reconstruction_time);


		/**
		 * Changes the default anchor plate id.
		 */
		void
		set_default_anchor_plate_id(
				GPlatesModel::integer_plate_id_type anchor_plate_id);

	private:
		GPlatesUtils::non_null_intrusive_ptr<ReconstructionTreeCreatorImpl> d_impl;
	};


	/**
	 * Returns @a ReconstructionTreeCreator that is implemented as a least-recently-used cache
	 * of reconstruction trees.
	 *
	 * This is useful to reuse reconstruction trees spanning different reconstruction times and
	 * anchor plate ids. It's also useful if you aren't reusing trees in which case using the
	 * default value of one cached tree means it won't get created each time you call it with
	 * the same parameters (reconstruction time and anchor plate id).
	 *
	 * NOTE: The reconstruction feature collection weak refs are copied internally.
	 *
	 * @throws @a PreconditionViolationError if @a reconstruction_tree_cache_size is zero.
	 */
	ReconstructionTreeCreator
	get_cached_reconstruction_tree_creator(
			const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &reconstruction_features_collection,
			const double &default_reconstruction_time = 0,
			GPlatesModel::integer_plate_id_type default_anchor_plate_id = 0,
			unsigned int reconstruction_tree_cache_size = 1);


	/**
	 * Base implementation class for @a ReconstructionTreeCreator.
	 *
	 * Useful if you need to implement a new implementation (eg, different than that provided
	 * by @a get_cached_reconstruction_tree_creator).
	 */
	class ReconstructionTreeCreatorImpl :
			public GPlatesUtils::ReferenceCount<ReconstructionTreeCreatorImpl>
	{
	public:
		typedef GPlatesUtils::non_null_intrusive_ptr<ReconstructionTreeCreatorImpl> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const ReconstructionTreeCreatorImpl> non_null_ptr_to_const_type;


		virtual
		~ReconstructionTreeCreatorImpl()
		{  }


		//! Returns the reconstruction tree for the specified time and anchored plate id.
		virtual
		ReconstructionTree::non_null_ptr_to_const_type
		get_reconstruction_tree(
				const double &reconstruction_time,
				GPlatesModel::integer_plate_id_type anchor_plate_id) = 0;


		//! Returns the reconstruction tree for the specified time and the *default* anchored plate id.
		virtual
		ReconstructionTree::non_null_ptr_to_const_type
		get_reconstruction_tree_default_anchored_plate_id(
				const double &reconstruction_time) = 0;


		//! Returns the reconstruction tree for the specified anchored plate id and the *default* time.
		virtual
		ReconstructionTree::non_null_ptr_to_const_type
		get_reconstruction_tree_default_reconstruction_time(
				GPlatesModel::integer_plate_id_type anchor_plate_id) = 0;


		//! Returns the reconstruction tree for the *default* time and the *default* anchored plate id.
		virtual
		ReconstructionTree::non_null_ptr_to_const_type
		get_reconstruction_tree_default_reconstruction_time_and_anchored_plate_id() = 0;


		//! Changes the default reconstruction time.
		virtual
		void
		set_default_reconstruction_time(
				const double &reconstruction_time) = 0;


		//! Changes the default anchor plate id.
		virtual
		void
		set_default_anchor_plate_id(
				GPlatesModel::integer_plate_id_type anchor_plate_id) = 0;
	};
}

#endif // GPLATES_APP_LOGIC_RECONSTRUCTIONTREECREATOR_H
