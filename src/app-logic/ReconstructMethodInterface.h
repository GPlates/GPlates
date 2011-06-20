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

#ifndef GPLATES_APP_LOGIC_RECONSTRUCTMETHODINTERFACE_H
#define GPLATES_APP_LOGIC_RECONSTRUCTMETHODINTERFACE_H

#include <vector>
#include <boost/function.hpp>
#include <boost/optional.hpp>

#include "ReconstructedFeatureGeometry.h"
#include "ReconstructionTree.h"
#include "ReconstructionTreeCreator.h"

#include "maths/GeometryOnSphere.h"

#include "model/FeatureHandle.h"

#include "utils/ReferenceCount.h"


namespace GPlatesAppLogic
{
	class ReconstructParams;

	/**
	 * Registry for information required to find and create @a ReconstructMethodInterface objects.
	 */
	class ReconstructMethodInterface :
			public GPlatesUtils::ReferenceCount<ReconstructMethodInterface>
	{
	public:
		// Convenience typedefs for a shared pointer to a @a ReconstructMethodInterface.
		typedef GPlatesUtils::non_null_intrusive_ptr<ReconstructMethodInterface> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const ReconstructMethodInterface> non_null_ptr_to_const_type;


		//! Typedef for a geometry type.
		typedef GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry_type;


		/**
		 * Associates a present day or resolved geometry with its geometry property iterator.
		 */
		struct Geometry
		{
			Geometry(
					GPlatesModel::FeatureHandle::iterator property_iterator_,
					geometry_type geometry_) :
				property_iterator(property_iterator_),
				geometry(geometry_)
			{  }

			GPlatesModel::FeatureHandle::iterator property_iterator;
			geometry_type geometry;
		};


		virtual
		~ReconstructMethodInterface()
		{  }


		/**
		 * The same as @a get_resolved_geometries with a reconstruction time of zero
		 * except there *must* be one geometry for *each* geometry property in the specified
		 * feature that is reconstructable when @a reconstruct_feature is called - but they do
		 * not have to be returned in any particular order.
		 *
		 * So this means if the geometry is *not* active at present day it is still returned.
		 * And this means it could return a different result than @a get_resolved_geometries (with a time of zero).
		 * TODO: May need to revisit this.
		 */
		virtual
		void
		get_present_day_geometries(
				std::vector<Geometry> &present_day_geometries,
				const GPlatesModel::FeatureHandle::weak_ref &feature_weak_ref) const = 0;


#if 0
		/**
		 * Returns the resolved geometries for the geometry properties of the feature specified,
		 * at the specified reconstruction time.
		 *
		 * NOTE: Unlike @a get_present_day_geometries, this method can return fewer geometries
		 * than there are geometry feature properties that can be reconstructed.
		 * If a geometry property is time-dependent and is not active at @a reconstruction_time
		 * then a corresponding resolved geometry will not be returned.
		 */
		virtual
		void
		get_resolved_geometries(
				std::vector<Geometry> &resolved_geometries,
				const GPlatesModel::FeatureHandle::weak_ref &feature_weak_ref,
				const double &reconstruction_time) const = 0;
#endif


		/**
		 * Reconstructs the specified feature at the specified reconstruction time and returns
		 * one more more reconstructed feature geometries.
		 *
		 * NOTE: Only features that exist at the specified @a reconstruction_time will be reconstructed.
		 *
		 * The reconstructed feature geometries are appended to @a reconstructed_feature_geometries.
		 *
		 * Note that @a reconstruction_tree_creator can be used to get reconstruction trees at times
		 * other than @a reconstruction_time.
		 * This is useful for reconstructing flowlines since the function might be hooked up
		 * to a reconstruction tree cache.
		 * NOTE: Calling 'set_default_reconstruction_time()' or 'set_default_anchor_plate_id'
		 * can result in a thrown exception. These defaults are managed by the caller and
		 * should not be altered.
		 */
		virtual
		void
		reconstruct_feature(
				std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &reconstructed_feature_geometries,
				const GPlatesModel::FeatureHandle::weak_ref &feature_weak_ref,
				const ReconstructParams &reconstruct_params,
				const ReconstructionTreeCreator &reconstruction_tree_creator,
				const double &reconstruction_time) = 0;
	};
}

#endif // GPLATES_APP_LOGIC_RECONSTRUCTMETHODINTERFACE_H
