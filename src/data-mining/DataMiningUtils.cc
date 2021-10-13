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
#include "OpaqueDataToDouble.h"
#include "DataMiningUtils.h"

//Data-mining temporary code
bool enable_data_mining = false;

using namespace GPlatesDataMining;

boost::optional< double > 
DataMiningUtils::minimum(
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


void
DataMiningUtils::convert_to_double_vector(
		std::vector<OpaqueData>::const_iterator begin,
		std::vector<OpaqueData>::const_iterator end,
		std::vector<double>& result)
{
	boost::optional<double> tmp = boost::none;
	for(; begin != end; begin++)
	{
		tmp = boost::apply_visitor(
				ConvertOpaqueDataToDouble(),
				*begin);
		if(tmp)
		{
			result.push_back(*tmp);
		}
	}
}



