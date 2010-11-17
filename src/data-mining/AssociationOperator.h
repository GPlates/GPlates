/* $Id$ */

/**
 * \file 
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
#ifndef GPLATESDATAMINING_ASSOCIATIONOPERATOR_H
#define GPLATESDATAMINING_ASSOCIATIONOPERATOR_H

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
	enum AssociationOperatorType
	{
		REGION_OF_INTEREST,
		SEED_ITSELF,
		FEATURE_ID_LIST,
		INSIDE
	};

	struct AssociationOperatorParameters
	{
		double d_time;
		AssociationOperatorType d_associator_type;
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
			AssociationOperatorParameters d_associator_cfg;
		};
		
		virtual
		~AssociationOperator(){}

		virtual
		void
		execute(
				const GPlatesModel::FeatureHandle::const_weak_ref& seed,
				const GPlatesModel::FeatureCollectionHandle::const_weak_ref& target_collection,
				const FeatureGeometryMap& seed_map,
				const FeatureGeometryMap& target_map) = 0;

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
#endif



