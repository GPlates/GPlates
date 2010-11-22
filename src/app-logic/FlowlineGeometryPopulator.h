/* $Id: ReconstructedFeatureGeometryPopulator.h 9200 2010-08-11 03:48:32Z mchin $ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date: 2010-08-11 05:48:32 +0200 (on, 11 aug 2010) $
 * 
 * Copyright (C) 2010 Geological Survey of Norway
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
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

#include "FlowlineUtils.h"
#include "ReconstructedFlowline.h"
#include "ReconstructionFeatureProperties.h"
#include "ReconstructionTree.h"

#include "maths/FiniteRotation.h"
#include "maths/PointOnSphere.h"

#include "model/FeatureVisitor.h"
#include "model/types.h"

#include "property-values/GeoTimeInstant.h"

namespace GPlatesAppLogic
{

		/**
		* Determines if there are any flowline features in the collection.
		*/
		class DetectFlowlineFeatures:
			public GPlatesModel::ConstFeatureVisitor
		{
		public:
			DetectFlowlineFeatures() :
				d_found_flowline_features(false)
			{  }


			bool
			has_flowline_features() const
			{
				return d_found_flowline_features;
			}


			virtual
			void
			visit_feature_handle(
					const GPlatesModel::FeatureHandle &feature_handle)
			{
				if (d_found_flowline_features)
				{
					// We've already found a flowline feature so just return.
					return;
				}

				static const GPlatesModel::FeatureType flowline_feature_type = 
					GPlatesModel::FeatureType::create_gpml("Flowline");

				if (feature_handle.feature_type() == flowline_feature_type)
				{
					d_found_flowline_features = true;
				}

				// NOTE: We don't actually want to visit the feature's properties
				// so we're not calling 'visit_feature_properties()'.
			}

		private:



			bool d_found_flowline_features;
		};


	class ReconstructionGeometryCollection;

	/**
	 * Reconstructs flowline features
	 */
	class FlowlineGeometryPopulator:
			public GPlatesModel::FeatureVisitor,
			private boost::noncopyable
	{
	public:


		FlowlineGeometryPopulator(
				ReconstructionGeometryCollection &reconstruction_geometry_collection);

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
		finalise_post_feature_properties(
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

		void
		process_point(
			const GPlatesMaths::PointOnSphere &point);

		ReconstructionGeometryCollection &d_reconstruction_geometry_collection;

		ReconstructionTree::non_null_ptr_to_const_type d_reconstruction_tree;

		const GPlatesPropertyValues::GeoTimeInstant d_recon_time;

		FlowlineUtils::FlowlinePropertyFinder d_flowline_property_finder;

		std::vector<GPlatesMaths::FiniteRotation> d_rotations;

	};
}

#endif  // GPLATES_APP_LOGIC_FLOWLINEGEOMETRYPOPULATOR_H
