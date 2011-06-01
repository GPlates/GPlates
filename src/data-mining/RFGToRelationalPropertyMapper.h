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
#ifndef GPLATESDATAMINING_RFGTORELATIONALPROPERTYMAPPER_H
#define GPLATESDATAMINING_RFGTORELATIONALPROPERTYMAPPER_H

#include <vector>
#include <QDebug>

#include <boost/foreach.hpp>

#include "CoRegMapper.h"
#include "DualGeometryVisitor.h"
#include "Types.h"
#include "DataMiningUtils.h"

namespace GPlatesDataMining
{
	class RFGToRelationalPropertyMapper : public CoRegMapper
	{
	public:
		RFGToRelationalPropertyMapper(
				const AttributeType attr_type,
				CoRegMapper::RFGVector seeds):
			d_attr_type(attr_type),
			d_seed_geos(seeds)
		{ }

		void
		process(
				CoRegMapper::MapperInDataset::const_iterator input_begin,
				CoRegMapper::MapperInDataset::const_iterator input_end,
				CoRegMapper::MapperOutDataset &output)
		{
			unsigned dis = 0;
			switch(d_attr_type)
			{
				case DISTANCE_ATTRIBUTE:
					for(;input_begin != input_end; input_begin++)
					{
						output.push_back(boost::make_tuple(
								OpaqueData(DataMiningUtils::shortest_distance(d_seed_geos,input_begin->second)),
								input_begin->second));
					}
					break;
				case PRESENCE_ATTRIBUTE:
					output.push_back(boost::make_tuple(
							OpaqueData((input_begin != input_end)),
							CoRegMapper::RFGVector()));
					break;
				case NUMBER_OF_PRESENCE_ATTRIBUTE:
					dis = static_cast<unsigned>(std::distance(input_begin,input_end));
					output.push_back(boost::make_tuple(OpaqueData(dis),CoRegMapper::RFGVector()));
					break;
				case CO_REGISTRATION_ATTRIBUTE:
				case SHAPE_FILE_ATTRIBUTE:
				default:
					break;
			}
			return;
		}

		virtual
		~RFGToRelationalPropertyMapper(){ }			

	protected:
		const AttributeType d_attr_type;
		CoRegMapper::RFGVector d_seed_geos;
	};
}
#endif //GPLATESDATAMINING_RFGTORELATIONALPROPERTYMAPPER_H





