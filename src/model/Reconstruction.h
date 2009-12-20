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

#ifndef GPLATES_MODEL_RECONSTRUCTION_H
#define GPLATES_MODEL_RECONSTRUCTION_H

#include <vector>
#include "ReconstructionGeometry.h"
#include "ReconstructionTree.h"
#include "FeatureCollectionHandle.h"
#include "utils/ReferenceCount.h"


namespace GPlatesModel
{
	/**
	 * This class represents a plate-tectonic reconstruction at a particular geological
	 * time-instant.
	 */
	class Reconstruction :
			public GPlatesUtils::ReferenceCount<Reconstruction>
	{
	public:
		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<Reconstruction,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<Reconstruction,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<const Reconstruction,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const Reconstruction,
				GPlatesUtils::NullIntrusivePointerHandler>
				non_null_ptr_to_const_type;

		/**
		 * The type of the collection of RFGs.
		 */
		typedef std::vector<ReconstructionGeometry::non_null_ptr_type> geometry_collection_type;

		~Reconstruction();

		/**
		 * Create a new Reconstruction instance.
		 */
		static
		const non_null_ptr_type
		create(
				ReconstructionTree::non_null_ptr_type reconstruction_tree_ptr_,
				const std::vector<FeatureCollectionHandle::weak_ref> &reconstruction_feature_collections_)
		{
			non_null_ptr_type ptr(new Reconstruction(reconstruction_tree_ptr_,
							reconstruction_feature_collections_),
					GPlatesUtils::NullIntrusivePointerHandler());
			return ptr;
		}

		/**
		 * Access the reconstructed geometries.
		 */
		const geometry_collection_type &
		geometries() const
		{
			return d_geometries;
		}

		/**
		 * Access the reconstructed geometries.
		 */
		geometry_collection_type &
		geometries()
		{
			return d_geometries;
		}

		/**
		 * Access the reconstruction tree.
		 */
		const ReconstructionTree &
		reconstruction_tree() const
		{
			return *d_reconstruction_tree_ptr;
		}

		/**
		 * Access the reconstruction tree.
		 */
		ReconstructionTree &
		reconstruction_tree()
		{
			return *d_reconstruction_tree_ptr;
		}

		/**
		 * Access the feature collections containing the reconstruction features used to
		 * create this reconstruction.
		 */
		const std::vector<FeatureCollectionHandle::weak_ref> &
		reconstruction_feature_collections() const
		{
			return d_reconstruction_feature_collections;
		}

	private:

		/**
		 * The reconstructed geometries.
		 */
		geometry_collection_type d_geometries;

		/**
		 * The plate-reconstruction hierarchy of total reconstruction poles which was used
		 * to reconstruct the geometries.
		 */
		ReconstructionTree::non_null_ptr_type d_reconstruction_tree_ptr;

		/**
		 * Access the feature collections containing the reconstruction features used to
		 * create this reconstruction.
		 */
		std::vector<FeatureCollectionHandle::weak_ref> d_reconstruction_feature_collections;

		/**
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		explicit
		Reconstruction(
				ReconstructionTree::non_null_ptr_type reconstruction_tree_ptr_,
				const std::vector<FeatureCollectionHandle::weak_ref> &reconstruction_feature_collections_):
			d_reconstruction_tree_ptr(reconstruction_tree_ptr_),
			d_reconstruction_feature_collections(reconstruction_feature_collections_)
		{  }

		// This constructor should never be defined, because we don't want to allow
		// copy-construction.
		Reconstruction(
				const Reconstruction &other);

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All "assignment" should really only be assignment of one
		// intrusive-pointer to another.
		Reconstruction &
		operator=(
				const Reconstruction &);
	};
}

#endif  // GPLATES_MODEL_RECONSTRUCTION_H
