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
#include "AssociationOperator.h"
#include "MinDataOperator.h"

using namespace GPlatesDataMining;

boost::optional< double > 
MinDataOperator::get_min(
		const std::vector< double >& input)
{
	boost::optional< double >  ret = boost::none;

	std::vector< double >::const_iterator it		= input.begin();
	std::vector< double >::const_iterator it_end	= input.end();

	for(; it != it_end; it++)
	{
		if(ret)
		{
			*ret = std::min( (*ret), (*it) );
		}	
		else
		{
			ret = *it;
		}
	}
	return ret;
}

boost::optional< double >
MinDataOperator::get_min_from_feature(
		const AssociationOperator::AssociatedCollection& input,
		const QString&	attr_name)
{
	AssociationOperator::AssociatedCollection::FeatureDistanceMap::const_iterator
		it = input.d_associated_features.begin();
	
	AssociationOperator::AssociatedCollection::FeatureDistanceMap::const_iterator
		it_end = input.d_associated_features.end();
	
	boost::optional< double > min_val = boost::none; 

	std::vector< double > tmp;		
	for(; it != it_end; it++)
	{
		boost::optional< GPlatesModel::TopLevelProperty::non_null_ptr_to_const_type > 
				property = get_property_by_name( it->first, attr_name);

		if(!property)
		{
			qWarning() << "Cannot find property with name: " << attr_name ;
			continue;
		}

		GetValueFromPropertyVisitor< double > visitor;
		(*property)->accept_visitor(visitor);
		
		if( get_min(visitor.get_data()) )
		{
			tmp.push_back( *get_min(visitor.get_data()) );
		}

	}
	return get_min(tmp);
}



