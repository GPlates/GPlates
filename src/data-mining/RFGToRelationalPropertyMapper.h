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
				const GPlatesAppLogic::ReconstructContext::ReconstructedFeature &reconstructed_seed_feature):
			d_attr_type(attr_type),
			d_reconstructed_seed_feature(reconstructed_seed_feature)
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
						const GPlatesAppLogic::ReconstructContext::ReconstructedFeature &reconstructed_target_feature =
								*input_begin;

						std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry*> target_geos;
						BOOST_FOREACH(
								const GPlatesAppLogic::ReconstructContext::Reconstruction &target_geo_recon,
								reconstructed_target_feature.get_reconstructions())
						{
							target_geos.push_back(target_geo_recon.get_reconstructed_feature_geometry().get());
						}

						std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry*> seed_geos;
						BOOST_FOREACH(
								const GPlatesAppLogic::ReconstructContext::Reconstruction &seed_geo_recon,
								d_reconstructed_seed_feature.get_reconstructions())
						{
							seed_geos.push_back(seed_geo_recon.get_reconstructed_feature_geometry().get());
						}

						output.push_back(
								boost::make_tuple(
										OpaqueData(DataMiningUtils::shortest_distance(seed_geos, target_geos)),
										reconstructed_target_feature));
					}
					break;

				case PRESENCE_ATTRIBUTE:
					output.push_back(boost::make_tuple(
							OpaqueData((input_begin != input_end)),
							// Not used...
							GPlatesAppLogic::ReconstructContext::ReconstructedFeature(
									GPlatesModel::FeatureHandle::weak_ref())));
					break;

				case NUMBER_OF_PRESENCE_ATTRIBUTE:
					dis = static_cast<unsigned>(std::distance(input_begin,input_end));
					output.push_back(boost::make_tuple(
							OpaqueData(dis),
							// Not used...
							GPlatesAppLogic::ReconstructContext::ReconstructedFeature(
									GPlatesModel::FeatureHandle::weak_ref())));
					break;

				case CO_REGISTRATION_GPML_ATTRIBUTE:
				case CO_REGISTRATION_SHAPEFILE_ATTRIBUTE:
				default:
					break;
			}
		}

		virtual
		~RFGToRelationalPropertyMapper(){ }			

	protected:
		const AttributeType d_attr_type;
		const GPlatesAppLogic::ReconstructContext::ReconstructedFeature &d_reconstructed_seed_feature;
	};
}
#endif //GPLATESDATAMINING_RFGTORELATIONALPROPERTYMAPPER_H





