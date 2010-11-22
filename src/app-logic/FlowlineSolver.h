/* $Id: FlowlinesSolver.h 8271 2010-05-03 13:30:36Z rwatson $ */

/**
* \file 
* File specific comments.
*
* Most recent change:
*   $Date: 2010-05-03 15:30:36 +0200 (ma, 03 mai 2010) $
* 
* Copyright (C) 2009, 2010 Geological Survey of Norway.
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

#ifndef GPLATES_APPLOGIC_FLOWLINESOLVER_H
#define GPLATES_APPLOGIC_FLOWLINESOLVER_H


#include "maths/PointOnSphere.h"
#include "model/FeatureVisitor.h"
#include "property-values/GpmlTimeSample.h"


namespace GPlatesAppLogic
{

	class ReconstructionGeometryCollection;

	class FlowlineSolver:
		public GPlatesModel::FeatureVisitor,
		public boost::noncopyable
	{
	public:

		typedef std::vector< std::list< GPlatesMaths::PointOnSphere > > lines_container_type;

		FlowlineSolver(
			const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &reconstruction_feature_collections,
			const double &reconstruction_time,
			const GPlatesModel::integer_plate_id_type anchor_plate,
			const GPlatesModel::integer_plate_id_type left_plate,
			const GPlatesModel::integer_plate_id_type right_plate,
			GPlatesAppLogic::ReconstructionGeometryCollection &flowlines_collection):
				d_reconstruction_feature_collections(reconstruction_feature_collections),
				d_reconstruction_time(reconstruction_time),
				d_anchor_plate_id(anchor_plate),
				d_left_plate_id(left_plate),
				d_right_plate_id(right_plate),
				d_flowlines_collection(&flowlines_collection)
		{ }


	virtual
	void
	visit_feature_handle(
		GPlatesModel::FeatureHandle &feature_handle);	
		
	virtual
	void
	visit_gml_point(
		GPlatesPropertyValues::GmlPoint &gml_point);
		
	virtual
	void
	visit_gml_multi_point(
		GPlatesPropertyValues::GmlMultiPoint &gml_multi_point);
		
	virtual
	void
	visit_gpml_constant_value(
		GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value);
		
	private:	

			
		// Required for creating new trees at other reconstruction times. There may be new 
		// mechanisms for doing this in the updated layer system....
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &d_reconstruction_feature_collections;
		
		double d_reconstruction_time;
		GPlatesModel::integer_plate_id_type d_anchor_plate_id;
		GPlatesModel::integer_plate_id_type d_left_plate_id;
		GPlatesModel::integer_plate_id_type d_right_plate_id;
		std::vector<GPlatesPropertyValues::GpmlTimeSample> d_time_samples;

		// For holding output geometries
		GPlatesAppLogic::ReconstructionGeometryCollection *d_flowlines_collection;			
	};
}

#endif // GPLATES_APPLOGIC_FLOWLINESOLVER_H

