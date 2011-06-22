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

#include <boost/optional.hpp>

#include "ReconstructMethodMotionPath.h"

#include "MotionPathGeometryPopulator.h"
#include "MotionPathUtils.h"
#include "ReconstructUtils.h"

#include "maths/MultiPointOnSphere.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolylineOnSphere.h"

#include "model/FeatureVisitor.h"
#include "model/types.h"

#include "property-values/GmlLineString.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlPolygon.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlPlateId.h"


namespace GPlatesAppLogic
{
	namespace
	{
		/**
		 * Finds the present day geometries of a MotionPath feature.
		 *
		 * Present day geometries probably don't make too much sense for motion paths but
		 * we'll add points and multipoints since they are what is currently used to seed
		 * motion paths.
		 */
		class GetPresentDayGeometries :
				public GPlatesModel::FeatureVisitor
		{
		public:
			GetPresentDayGeometries(
					std::vector<ReconstructMethodInterface::Geometry> &present_day_geometries) :
				d_present_day_geometries(present_day_geometries)
			{  }

		private:
			virtual
			void
			visit_gml_multi_point(
					GPlatesPropertyValues::GmlMultiPoint &gml_multi_point)
			{
				d_present_day_geometries.push_back(
						ReconstructMethodInterface::Geometry(
								*current_top_level_propiter(),
								gml_multi_point.multipoint()));
			}

			virtual
			void
			visit_gml_point(
					GPlatesPropertyValues::GmlPoint &gml_point)
			{
				d_present_day_geometries.push_back(
						ReconstructMethodInterface::Geometry(
								*current_top_level_propiter(),
								gml_point.point()));
			}

			virtual
			void
			visit_gpml_constant_value(
				GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
			{
				gpml_constant_value.value()->accept_visitor(*this);
			}


			std::vector<ReconstructMethodInterface::Geometry> &d_present_day_geometries;
		};
	}
}


bool
GPlatesAppLogic::ReconstructMethodMotionPath::can_reconstruct_feature(
		const GPlatesModel::FeatureHandle::const_weak_ref &feature_weak_ref)
{
	MotionPathUtils::DetectMotionPathFeatures visitor;
	visitor.visit_feature(feature_weak_ref);

	return visitor.has_motion_track_features();
}


void
GPlatesAppLogic::ReconstructMethodMotionPath::get_present_day_geometries(
		std::vector<Geometry> &present_day_geometries,
		const GPlatesModel::FeatureHandle::weak_ref &feature_weak_ref) const
{
	GetPresentDayGeometries visitor(present_day_geometries);

	visitor.visit_feature(feature_weak_ref);
}


void
GPlatesAppLogic::ReconstructMethodMotionPath::reconstruct_feature(
		std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &reconstructed_feature_geometries,
		const GPlatesModel::FeatureHandle::weak_ref &feature_weak_ref,
		const ReconstructParams &/*reconstruct_params*/,
		const ReconstructionTreeCreator &reconstruction_tree_creator,
		const double &reconstruction_time)
{
	MotionPathGeometryPopulator visitor(
			reconstructed_feature_geometries,
			reconstruction_tree_creator,
			reconstruction_time);

	visitor.visit_feature(feature_weak_ref);
}
