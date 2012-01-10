/* $Id: ReconstructMethodVirtualGeomagneticPole.h 11715 2011-06-07 09:30:12Z rwatson $ */

/**
 * \file 
 * $Revision: 11715 $
 * $Date: 2011-06-07 11:30:12 +0200 (ti, 07 jun 2011) $
 * 
 * Copyright (C) 2011 Geological Survey of Norway
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

#ifndef GPLATES_APP_LOGIC_RECONSTRUCTMETHODSMALLCIRCLE_H
#define GPLATES_APP_LOGIC_RECONSTRUCTMETHODSMALLCIRCLE_H

#include <boost/optional.hpp>

#include "ReconstructedFeatureGeometry.h"
#include "ReconstructMethodInterface.h"

#include "utils/non_null_intrusive_ptr.h"


namespace GPlatesAppLogic
{
	/**
	 * Reconstructs a virtual geomagnetic pole feature.
	 */
	class ReconstructMethodSmallCircle :
			public ReconstructMethodInterface
	{
	public:
		// Convenience typedefs for a shared pointer to a @a ReconstructMethodVirtualGeomagneticPole.
		typedef GPlatesUtils::non_null_intrusive_ptr<ReconstructMethodSmallCircle> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const ReconstructMethodSmallCircle> non_null_ptr_to_const_type;


		/**
		 * Returns true if can reconstruct the specified feature.
		 *
		 * Feature is expected to have a feature type of "SmallCircle".
		 */
		static
		bool
		can_reconstruct_feature(
				const GPlatesModel::FeatureHandle::const_weak_ref &feature_weak_ref);


		/**
		 * Creates a @a ReconstructMethodSmallCircle object.
		 */
		static
		ReconstructMethodSmallCircle::non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new ReconstructMethodSmallCircle());
		}


		/**
		 * Returns the present day geometries of the specified feature.
		 */
		virtual
		void
		get_present_day_geometries(
				std::vector<Geometry> &present_day_geometries,
				const GPlatesModel::FeatureHandle::weak_ref &feature_weak_ref) const;


		/**
		 * Reconstructs the specified feature at the specified reconstruction time and returns
		 * one more more reconstructed feature geometries.
		 */
		virtual
		void
		reconstruct_feature(
				std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &reconstructed_feature_geometries,
				const GPlatesModel::FeatureHandle::weak_ref &feature_weak_ref,
				const ReconstructHandle::type &reconstruct_handle,
				const ReconstructParams &reconstruct_params,
				const ReconstructionTreeCreator &reconstruction_tree_creator,
				const double &reconstruction_time);

		/**
		 * Reconstructs the specified geometry from present day to the specified reconstruction time -
		 * unless @a reverse_reconstruct is true in which case the geometry is assumed to be
		 * the reconstructed geometry (at the reconstruction time) and the returned geometry will
		 * then be the present day geometry.
		 *
		 * NOTE: The specified feature is called @a reconstruction_properties since its geometry(s)
		 * is not reconstructed - it is only used as a source of properties that determine how
		 * to perform the reconstruction (for example, a reconstruction plate ID).
		 */
		virtual
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
		reconstruct_geometry(
				const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry,
				const GPlatesModel::FeatureHandle::weak_ref &reconstruction_properties,
				const ReconstructionTreeCreator &reconstruction_tree_creator,
				const double &reconstruction_time,
				bool reverse_reconstruct);

	private:
		ReconstructMethodSmallCircle()
		{  }
	};
}

#endif // GPLATES_APP_LOGIC_RECONSTRUCTMETHODSMALLCIRCLE_H