/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date: 2010-08-11 05:48:32 +0200 (on, 11 aug 2010) $
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

#ifndef GPLATES_APP_LOGIC_SMALLCIRCLEGEOMETRYPOPULATOR_H
#define GPLATES_APP_LOGIC_SMALLCIRCLEGEOMETRYPOPULATOR_H

#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

#include "ReconstructedFeatureGeometry.h"
#include "ReconstructionTreeCreator.h"

#include "maths/PointOnSphere.h"

#include "model/FeatureVisitor.h"
#include "model/types.h"

#include "property-values/GeoTimeInstant.h"
#include "property-values/GmlTimePeriod.h"

namespace GPlatesAppLogic
{

	/**
	 * Creates small circle geometries
	 */
	class SmallCircleGeometryPopulator:
			public GPlatesModel::FeatureVisitor,
			private boost::noncopyable
	{
	public:


		SmallCircleGeometryPopulator(
				std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &reconstructed_feature_geometries,
				const ReconstructionTreeCreator &reconstruction_tree_creator,
				const double &reconstruction_time);

		virtual
		~SmallCircleGeometryPopulator()
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
		visit_gml_point(
				GPlatesPropertyValues::GmlPoint &gml_point);

		virtual
		void
		visit_gml_time_period(
				GPlatesPropertyValues::GmlTimePeriod &gml_time_period);

		virtual
		void
		visit_gpml_constant_value(
				GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value);

		virtual
		void
		visit_gpml_plate_id(
				GPlatesPropertyValues::GpmlPlateId &gpml_plate_id);

		virtual
		void
		visit_gpml_measure(
				GPlatesPropertyValues::GpmlMeasure &gpml_measure);



	private:


		/**
		 * The @a ReconstructedFeatureGeometry objects generated during reconstruction.
		 */
		std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &d_reconstructed_feature_geometries;

		/**
		 * Used to get a @a ReconstructionTree.
		 */
		ReconstructionTreeCreator d_reconstruction_tree_creator;

		const GPlatesPropertyValues::GeoTimeInstant d_reconstruction_time;

		boost::optional<GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type> d_centre;
		boost::optional<double> d_radius_in_degrees;

		// We need to provide an iterator-to-geometry-property to the various ReconstructedGeometry creation functions.
		boost::optional<GPlatesModel::FeatureHandle::iterator> d_geometry_iterator;

		boost::optional<GPlatesModel::integer_plate_id_type> d_reconstruction_plate_id;

		bool d_feature_is_defined_at_recon_time;

	};
}

#endif  // GPLATES_APP_LOGIC_SMALLCIRCLEGEOMETRYPOPULATOR_H