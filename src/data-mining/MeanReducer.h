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
#ifndef GPLATESDATAMINING_MEANREDUCER_H
#define GPLATESDATAMINING_MEANREDUCER_H

#include <numeric>
#include <vector>
#include <QDebug>

#include "CoRegReducer.h"
#include "DataMiningUtils.h"

namespace GPlatesDataMining
{
	class MeanReducer : public CoRegReducer
	{
	protected:
		OpaqueData
		exec(
				CoRegReducer::ReducerInDataset::const_iterator first,
				CoRegReducer::ReducerInDataset::const_iterator last) 
		{
			std::vector<OpaqueData> data;
			extract_opaque_data(first, last, data);
			std::vector<double> buf;
			DataMiningUtils::convert_to_double_vector(data.begin(),data.end(), buf); 
			double ret = std::accumulate(buf.begin(), buf.end(), 0.0) / buf.size();
			return OpaqueData(ret);
		}
	};
}
#endif





