/* $Id: Filter.h 10236 2010-11-17 01:53:09Z mchin $ */

/**
 * \file 
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
#ifndef GPLATESDATAMINING_FILTER_H
#define GPLATESDATAMINING_FILTER_H

#include <vector>
#include <bitset>

#include <QDebug>

#include <boost/variant.hpp>
#include <boost/variant/get.hpp>

#include "maths/ProximityHitDetail.h"
#include "maths/ProximityCriteria.h"
#include "maths/ConstGeometryOnSphereVisitor.h"

#include "model/FeatureHandle.h"

#include "feature-visitors/GeometryFinder.h"


namespace GPlatesDataMining
{
	enum FilterType
	{
		REGION_OF_INTEREST,
		SEED_ITSELF,
		FEATURE_ID_LIST,
		INSIDE
	};

	struct FilterCfg
	{
		double d_time;
		FilterType d_filter_type;
		double d_ROI_range;
		std::vector < QString > d_feature_id_list;
	};

	typedef std::map<
				const GPlatesModel::FeatureHandle*,
				std::vector< GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type >
					> FeatureGeometryMap;
	
	/*
	* TODO...
	* Comments....
	*/
	class AssociationOperator 
	{
	public:
		
		struct AssociatedCollection
		{
			typedef
			std::map<
					GPlatesModel::FeatureHandle::const_weak_ref,
					std::vector< double > > FeatureDistanceMap;

			typedef	std::pair<
					GPlatesModel::FeatureHandle::const_weak_ref,
					std::vector<double> > FeatureDistancePair;

			FeatureDistanceMap d_associated_features;
			double d_reconstruction_time;
			GPlatesModel::FeatureHandle::const_weak_ref d_seed;
			FilterCfg d_associator_cfg;
		};
		
		virtual
		~AssociationOperator(){}

		inline
		const AssociatedCollection&
		get_associated_collection()
		{
			return *d_dataset;
		}

		inline
		boost::shared_ptr< const AssociatedCollection>
		get_associated_collection_ptr()
		{
			return d_dataset;
		}

		virtual
		inline
		void
		set_time(
				double time)
		{
			d_dataset->d_reconstruction_time = time;
		}

	protected:
		AssociationOperator() :
			d_dataset(new AssociatedCollection)
		{ }
		boost::shared_ptr< AssociatedCollection> d_dataset;
	};
}
#endif //GPLATESDATAMINING_FILTER_H





