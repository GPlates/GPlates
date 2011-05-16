/* $Id: DataOperator.cc 10236 2010-11-17 01:53:09Z mchin $ */

/**
 * \file .
 * $Revision: 10236 $
 * $Date: 2010-11-17 12:53:09 +1100 (Wed, 17 Nov 2010) $
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

#include <boost/assign.hpp>

#include "DataOperator.h"
#include "DataMiningUtils.h"

using namespace GPlatesDataMining;

DataOperator::DataOperatorNameMap
DataOperator::d_data_operator_name_map = boost::assign::map_list_of
		("Min", DATA_OPERATOR_MIN)
		("Max", DATA_OPERATOR_MAX)
		("Lookup", DATA_OPERATOR_LOOKUP)
		("Vote", DATA_OPERATOR_VOTE)
		("Min Distance", DATA_OPERATOR_MIN_DISTANCE)
		("Presence", DATA_OPERATOR_RRESENCE)
		("NumberInROI", DATA_OPERATOR_NUM_IN_ROI)
		; 


boost::optional<
		GPlatesModel::TopLevelProperty::non_null_ptr_to_const_type
				>
DataOperator::get_property_by_name(
		GPlatesModel::FeatureHandle::const_weak_ref feature_ref,
		QString name)
{
	using namespace GPlatesModel;
	FeatureHandle::const_iterator it = feature_ref->begin();
	FeatureHandle::const_iterator it_end = feature_ref->end();
	for(; it != it_end; it++)
	{
		if((*it)->property_name().get_name() == GPlatesUtils::make_icu_string_from_qstring(name))
		{
			return *it;	
		}
	}
	return boost::none;
}

void
DataOperator::get_closest_features(
		const AssociationOperator::AssociatedCollection& association_collection,
		std::vector< GPlatesModel::FeatureHandle::const_weak_ref >& colsest_features)
{
	AssociationOperator::AssociatedCollection::FeatureDistanceMap::const_iterator 
		it = association_collection.d_associated_features.begin();
	AssociationOperator::AssociatedCollection::FeatureDistanceMap::const_iterator 
		it_end = association_collection.d_associated_features.end();
	
	boost::optional< double > tmp_distance = boost::none;
	
	for(; it != it_end; it++)
	{
		boost::optional < double > temp = DataMiningUtils::minimum(it->second);
		if(!temp)
		{
			continue;
		}

		if(!tmp_distance)
		{
			tmp_distance = temp;
			colsest_features.push_back(it->first);
		}
		else
		{
			if((*temp) > (*tmp_distance))
			{
				continue;
			}
			else if((*temp) < (*tmp_distance) )
			{
				colsest_features.clear();
			}
			tmp_distance = temp;
			colsest_features.push_back(it->first);
		}
	}
}

boost::optional< GPlatesModel::FeatureHandle::const_weak_ref >
DataOperator::get_closest_feature(
		const AssociationOperator::AssociatedCollection& association_collection)
{
	std::vector< GPlatesModel::FeatureHandle::const_weak_ref > closest_features;
	get_closest_features( association_collection, closest_features );

	if(closest_features.size() > 1)
	{
#ifdef _DEBUG
		qDebug() << closest_features.size() <<" eligible features have been found.";
		qDebug() << "And these features cannot be distinguished by distance.";
		qDebug() << "Just pick up the first feature in the collection .";
#endif
	}
	if(closest_features.size() < 1)
	{
#ifdef _DEBUG
		qDebug() << "No eligible features found.";
#endif
		return boost::none;
	}
	return boost::optional< GPlatesModel::FeatureHandle::const_weak_ref >(closest_features.at(0));
}


