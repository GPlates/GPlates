/* $Id: Filter.h 10236 2010-11-17 01:53:09Z mchin $ */

/**
 * \file 
 * $Revision: 10236 $
 * $Date: 2010-11-17 12:53:09 +1100 (Wed, 17 Nov 2010) $
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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
#ifndef GPLATESDATAMINING_COREGMAPPER_H
#define GPLATESDATAMINING_COREGMAPPER_H
#include <boost/version.hpp>

#if BOOST_VERSION > 103600
#include <boost/unordered_map.hpp>
#endif

#include <boost/tuple/tuple.hpp>
#include <vector>

#include "OpaqueData.h"

namespace GPlatesAppLogic
{
	class ReconstructedFeatureGeometry;
}

namespace GPlatesModel
{
	class FeatureHandle;
}

namespace GPlatesDataMining
{
	class CoRegMapper
	{
	public:
#if BOOST_VERSION > 103600
		typedef boost::unordered_map< 
				const GPlatesModel::FeatureHandle*,
				std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry*> 
									> MapperInDataset;

		typedef std::vector<boost::tuple<
				OpaqueData,
				std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry*> 
										> 
							>MapperOutDataset;
#else
		typedef std::map<
                                const GPlatesModel::FeatureHandle*,
                                std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry*>
                                                                        > MapperInDataset;

                typedef std::vector<boost::tuple<
                                OpaqueData,
                                std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry*>
                                                                                >
                                                        >MapperOutDataset;
#endif
		typedef std::vector<
			const GPlatesAppLogic::ReconstructedFeatureGeometry*> RFGVector;

		virtual
		void
		process(
				MapperInDataset::const_iterator first,
				MapperInDataset::const_iterator last,
				MapperOutDataset& output
		) = 0;

		virtual
		~CoRegMapper(){}
	};

	class DummyMapper : public CoRegMapper
	{
	public:
		void
		process(
				CoRegMapper::MapperInDataset::const_iterator first,
				CoRegMapper::MapperInDataset::const_iterator last,
				CoRegMapper::MapperOutDataset& output)
		{ }
		~DummyMapper(){ }
	};
}
#endif //GPLATESDATAMINING_COREGMAPPER_H





