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

#include "DualGeometryVisitor.h"
#include "DataOperatorTypes.h"

namespace
{
	using namespace GPlatesDataMining;
	double
	shortest_distance(
			const std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry*>& seed_geos,
			const GPlatesAppLogic::ReconstructedFeatureGeometry* geo)
	{
		double ret = DEFAULT_RADIUS_OF_EARTH * PI;
		BOOST_FOREACH(const GPlatesAppLogic::ReconstructedFeatureGeometry* seed, seed_geos)
		{
			//use (DEFAULT_RADIUS_OF_EARTH * PI) as range, so the distance can always be calculated.
			IsCloseEnoughChecker checker((DEFAULT_RADIUS_OF_EARTH * PI), true);
			DualGeometryVisitor< IsCloseEnoughChecker > dual_visitor(
					*(geo->geometry()),
					*(seed->geometry()),
					&checker);
			dual_visitor.apply();
			boost::optional<double> tmp = checker.distance();
			if(tmp && ret > *tmp)
			{
				ret = *tmp;
			}
		}
		return ret;
	}
}


namespace GPlatesDataMining
{
	using namespace GPlatesUtils;
	
	typedef std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry*> InputSequence;
	typedef std::vector<OpaqueData> OutputSequence;

	/*	
	*	TODO:
	*	Comments....
	*/
	class RFGToRelationalPropertyMapper
	{
	public:

		explicit
		RFGToRelationalPropertyMapper(
				const AttributeType attr_type,
				const std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry*>& seed_geos):
			d_attr_type(attr_type),
			d_seed_geos(seed_geos)
		{ }


		/*
		* TODO: comments....
		*/
		template< class OutputType, class OutputMode>
		int
		operator()(
				InputSequence::const_iterator input_begin,
				InputSequence::const_iterator input_end,
				FilterMapOutputHandler<OutputType, OutputMode> &handler)
		{
			int count = 0;
			switch(d_attr_type)
			{
				case DISTANCE_ATTRIBUTE:
					for(;input_begin != input_end; input_begin++)
					{
						handler.insert(
								OpaqueData(shortest_distance(d_seed_geos,*input_begin)));
						count++;
					}
					return count;

				case PRESENCE_ATTRIBUTE:
					handler.insert(
							OpaqueData((input_begin != input_end)));
					return 1; // for "Presence" attribute, the length of output is always 1.
				
				case NUMBER_OF_PRESENCE_ATTRIBUTE:
					handler.insert(
							OpaqueData(static_cast<unsigned>(std::distance(input_begin,input_end))));
					return 1; // for "Number of Presence" attribute, the length of output is always 1.
				
				default:
					return 0;
			}
		}


		virtual
		~RFGToRelationalPropertyMapper(){}			

	protected:
		const AttributeType d_attr_type;
		const std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry*>& d_seed_geos;
	};
}
#endif //GPLATESDATAMINING_RFGTORELATIONALPROPERTYMAPPER_H





