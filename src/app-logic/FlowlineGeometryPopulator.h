/* $Id: ReconstructedFeatureGeometryPopulator.h 9200 2010-08-11 03:48:32Z mchin $ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date: 2010-08-11 05:48:32 +0200 (on, 11 aug 2010) $
 * 
 * Copyright (C) 2010, 2011 Geological Survey of Norway
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

#ifndef GPLATES_APP_LOGIC_FLOWLINEGEOMETRYPOPULATOR_H
#define GPLATES_APP_LOGIC_FLOWLINEGEOMETRYPOPULATOR_H

#include <vector>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

#include "FlowlineUtils.h"
#include "ReconstructedFlowline.h"
#include "ReconstructionTree.h"
#include "ReconstructionTreeCreator.h"

#include "maths/FiniteRotation.h"
#include "maths/PointOnSphere.h"

#include "model/FeatureVisitor.h"
#include "model/types.h"

#include "property-values/GeoTimeInstant.h"

namespace GPlatesAppLogic
{
	namespace FlowlineUtils
	{
		class FlowlinePropertyFinder;
	}


	/**
	 * Reconstructs flowline features.
	 *
	 * Calculates flowlines from the flowline feature's seed points, and creates ReconstructedFlowlines which
	 * are added to the reconstruction geometry collection. 
	 */
	class FlowlineGeometryPopulator:
			public GPlatesModel::FeatureVisitor,
			private boost::noncopyable
	{
	public:
		FlowlineGeometryPopulator(
				std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &reconstructed_feature_geometries,
				const ReconstructionTreeCreator &reconstruction_tree_creator,
				const double &reconstruction_time);

		virtual
		~FlowlineGeometryPopulator()
		{  }

	protected:

		virtual
		bool
		initialise_pre_feature_properties(
				GPlatesModel::FeatureHandle &feature_handle);


		virtual
		void
		visit_gml_multi_point(
				GPlatesPropertyValues::GmlMultiPoint &gml_multi_point);


		virtual
		void
		visit_gml_point(
				GPlatesPropertyValues::GmlPoint &gml_point);

		virtual
		void
		visit_gpml_constant_value(
				GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value);

	private:

		/**
		 * Create a reconstructed feature geometry from @a present_day_seed_geometry, using the reconstruction
		 * plate id, and add it to the reconstruction geometry collection.
		 *
		 * We need to use this when we don't have enough information to reconstruct a flowline properly - for 
		 * example insufficient time information, or missing left/right plate ids. In such cases we still want to 
		 * display a seed point somewhere. Here we're using the reconstruction plate id to do a normal 
		 * reconstruction 
		 */
		void
		reconstruct_seed_geometry_with_recon_plate_id(
			const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &present_day_seed_geometry);

		/**
		 * Create a reconstructed flowline (incorporating both left- and right-hand parts) from the point given
		 * by @a present_day_seed_point_geometry, and add this to the reconstruction geometry collection.
		 *
		 * @a reconstructed_seed_point_geometry is required so that we can associate the flowline geometry with the
		 * @a present_day_seed_point_geometry. This is the reconstructed version of @a present_day_seed_point_geometry. 
		 */
		void
		create_flowline_geometry(
			const GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type &present_day_seed_point_geometry,
			const GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type &reconstructed_seed_point_geometry);



		/**
		 * The @a ReconstructedFeatureGeometry objects generated during reconstruction.
		 */
		std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &d_reconstructed_feature_geometries;

		/**
		 * The function to call (with a reconstruction time argument) to get a @a ReconstructionTree.
		 */
		ReconstructionTreeCreator d_reconstruction_tree_creator;

		const GPlatesPropertyValues::GeoTimeInstant d_recon_time;

		boost::scoped_ptr<FlowlineUtils::FlowlinePropertyFinder> d_flowline_property_finder;

		
		/**
		 * The (half) stage-pole rotations required for building up the flowlines.                                                                    
		 */
		std::vector<GPlatesMaths::FiniteRotation> d_left_rotations;
		std::vector<GPlatesMaths::FiniteRotation> d_right_rotations;
		
		/**
		 * Rotations for moving the seed point prior to building the rest of the flowline.                                                                    
		 */
		std::vector<GPlatesMaths::FiniteRotation> d_left_seed_point_rotations;
		std::vector<GPlatesMaths::FiniteRotation> d_right_seed_point_rotations;

	};
}

#endif  // GPLATES_APP_LOGIC_FLOWLINEGEOMETRYPOPULATOR_H
