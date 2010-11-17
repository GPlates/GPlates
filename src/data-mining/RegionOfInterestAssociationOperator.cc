/* $Id$ */

/**
 * \file .
 * $Revision$
 * $Date$
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

#include <boost/foreach.hpp>

#include "global/CompilerWarnings.h"
#include "utils/Profile.h"

#include "RegionOfInterestAssociationOperator.h"
#include "IsCloseEnoughChecker.h"
#include "DualGeometryVisitor.h"


// The BOOST_FOREACH macro in versions of boost before 1.37 uses the same local
// variable name in each instantiation. Nested BOOST_FOREACH macros therefore
// cause GCC to warn about shadowed declarations.
DISABLE_GCC_WARNING("-Wshadow")

void
GPlatesDataMining::RegionOfInterestAssociationOperator::execute(
		const GPlatesModel::FeatureHandle::const_weak_ref& seed,					
		const GPlatesModel::FeatureCollectionHandle::const_weak_ref& association_target,	
		const FeatureGeometryMap& seed_map,		
		const FeatureGeometryMap& target_map)
{
	using namespace GPlatesFeatureVisitors;
	using namespace GPlatesMaths;

	//get the seed geometries
	//Should we assume there is only one reconstructed geometry for each feature?
	//I don't want the assumption, although it is in use.
	FeatureGeometryMap::const_iterator geo_it = 
		seed_map.find( seed.handle_ptr() );	
	if(geo_it == seed_map.end()) 
	{
		qWarning() << "Cannot find geometry for seed.";
		return;
	}
	std::vector< GeometryOnSphere::non_null_ptr_to_const_type > geometries = geo_it->second;

	//for each geometry in seed feature
	BOOST_FOREACH(GeometryOnSphere::non_null_ptr_to_const_type& seed_geo, geometries)
	{
		//for each feature handle in target
		BOOST_FOREACH(const GPlatesModel::FeatureHandle::non_null_ptr_to_const_type &target_feature, *association_target)
			associate( seed_geo, target_feature, target_map );
	}
	return;
}

// See above
ENABLE_GCC_WARNING("-Wshadow")


void
GPlatesDataMining::RegionOfInterestAssociationOperator::associate(
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type seed_geo,
		GPlatesModel::FeatureHandle::non_null_ptr_to_const_type target_feature,
		const FeatureGeometryMap& target_map)
{
	PROFILE_FUNC();
	using namespace GPlatesFeatureVisitors;
	
	//TODO: create a function for this
	FeatureGeometryMap::const_iterator geo_it = 
		target_map.find( target_feature.get() );
	if(geo_it == target_map.end()) 
	{
		qWarning() << "Did not find any reconstructed geometry in target feature.";
		return;
	}
	std::vector< GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type > 
		geometries = geo_it->second;


	//for each geometry in target feature
	BOOST_FOREACH(GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type& target_geo, geometries)
	{
		//we have to do multiple dispatch here(more than double)
		IsCloseEnoughChecker checker(d_cfg.d_ROI_range, true);
		DualGeometryVisitor< IsCloseEnoughChecker > dual_visitor(*target_geo,*seed_geo,&checker);
		dual_visitor.apply();
		
		if( checker.is_close_enough() )
		{
			//we don't break the loop here, because we are also interested in the distance from other geometries.
			AssociatedCollection::FeatureDistanceMap::iterator feature_distance_map_iter = 
					d_dataset->d_associated_features.find(target_feature->reference());
		
			//We don't assume only one reconstructed geometry in each feature
			//so, find the feature in map, if it is new feature, create a entry
			//otherwise, push the distance back.
			if(feature_distance_map_iter == d_dataset->d_associated_features.end())
			{
				std::vector< double > distance_vec(1, *checker.distance() );

				AssociatedCollection::FeatureDistancePair 
					feature_distance_pair(
							target_feature->reference(),
							distance_vec);
				d_dataset->d_associated_features.insert( feature_distance_pair );
			}
			else
			{
				feature_distance_map_iter->second.push_back(*checker.distance());
			}
		}
	}
}

