/* $Id: AssociationOperator.h 8676 2010-06-11 05:40:59Z mchin $ */

/**
 * \file 
 * $Revision: 8676 $
 * $Date:  $
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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
#ifndef GPLATESDATAMINING_REGIONOFINTERESTASSOCIATIONOPERATOR_H
#define GPLATESDATAMINING_REGIONOFINTERESTASSOCIATIONOPERATOR_H

#include <vector>
#include <bitset>

#include <QDebug>

#include "AssociationOperator.h"

#include <boost/variant.hpp>
#include <boost/variant/get.hpp>
#include <boost/tuple/tuple.hpp>

#include "maths/ProximityHitDetail.h"
#include "maths/ProximityCriteria.h"
#include "maths/ConstGeometryOnSphereVisitor.h"


#include "model/FeatureHandle.h"

#include "feature-visitors/GeometryFinder.h"

namespace GPlatesDataMining
{
	/*
	*Comments....
	*/
	class RegionOfInterestAssociationOperator :
		public AssociationOperator
	{
	public:
		
		friend class AssociationOperatorFactory;
		
		virtual	
		void
		execute(
				const GPlatesModel::FeatureHandle::const_weak_ref& seed,					/*In*/
				const GPlatesModel::FeatureCollectionHandle::const_weak_ref& association_target,	/*In*/
				const FeatureGeometryMap& seed_map,		/*In*/
				const FeatureGeometryMap& target_map);

	protected:
		void
		associate(
				GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type seed_geo,
				GPlatesModel::FeatureHandle::non_null_ptr_to_const_type target_feature,
				const FeatureGeometryMap& target_map);

		FeatureGeometryMap d_feature_geometry_map;

		AssociationOperatorParameters d_cfg;

		RegionOfInterestAssociationOperator(
				AssociationOperatorParameters cfg) :
			d_cfg(cfg)
		{
			d_cfg.d_associator_type = REGION_OF_INTEREST;
			d_dataset->d_associator_cfg = d_cfg;
		}

		RegionOfInterestAssociationOperator();

		typedef
		std::vector< boost::tuple< 
				GPlatesModel::FeatureHandle::const_weak_ref, 
				std::vector<double>/*distance*/ > > FilterInputSequenceType;
	};

}
#endif





