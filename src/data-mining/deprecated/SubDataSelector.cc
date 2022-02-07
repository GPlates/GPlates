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
#include <QDebug>
#include "SubDataSelector.h"
#include "DataSelector.h"

using namespace GPlatesDataMining;

void
SubDataSelector::do_job()
{
	qDebug() << "SubDataSelector are doing its job.";
	CoRegConfigurationTable::const_iterator inner_it = d_matrix.begin();
	CoRegConfigurationTable::const_iterator inner_it_end = d_matrix.end();
	 
	std::multimap< 
			GPlatesModel::FeatureCollectionHandle::const_weak_ref, 
			boost::shared_ptr< const AssociationOperator::AssociatedCollection > 
				> associated_data_cache;

	for(; inner_it != inner_it_end; inner_it++)//for each row in data association matrix
	{
		
		boost::shared_ptr< const AssociationOperator::AssociatedCollection > 
			associated_data_ptr = 
				DataSelector::retrieve_associated_data_from_cache(
						inner_it->association_parameters,
						inner_it->target_feature_collection_handle,
						associated_data_cache);

		if(!associated_data_ptr)
		{
			boost::scoped_ptr< AssociationOperator > association_operator( 
					AssociationOperatorFactory::create(
							inner_it->association_operator_type,
							inner_it->association_parameters));

			association_operator->execute(
					d_seed_feature,
					inner_it->target_feature_collection_handle,
					d_seed_geometry_map,
					d_target_geometry_map);

			associated_data_ptr = association_operator->get_associated_collection_ptr();
			DataSelector::insert_associated_data_into_cache(
					associated_data_ptr,
					inner_it->target_feature_collection_handle,
					associated_data_cache);
		}

		boost::scoped_ptr< DataOperator > data_operator(
				DataOperatorFactory::create(
						inner_it->data_operator_type,
						inner_it->data_operator_parameters));
		
		data_operator->get_data(
				*associated_data_ptr,
				inner_it->attribute_name,
				*d_data_row);
	}
	qDebug() << "subdataselector done the job!!!!!!!!!!!!!!!!";
	return;
}

