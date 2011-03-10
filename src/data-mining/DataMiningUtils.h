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

#ifndef GPLATESDATAMINING_DATAMININGUTILS_H
#define GPLATESDATAMINING_DATAMININGUTILS_H

#include <boost/optional.hpp>
#include <boost/tuple/tuple.hpp>
#include <QString>
#include <QVariant>
#include <vector>

#include "OpaqueData.h"

#include "model/FeatureHandle.h"

namespace GPlatesAppLogic
{
	class ReconstructedFeatureGeometry;
}

namespace GPlatesDataMining
{
	namespace DataMiningUtils
	{
		/*
		* TODO: comments
		*/
		boost::optional< double > 
		minimum(
				const std::vector< double >& input);

		/*
		* TODO: comments
		*/
		void
		convert_to_double_vector(
				std::vector<OpaqueData>::const_iterator begin,
				std::vector<OpaqueData>::const_iterator end,
				std::vector<double>& result);

		/*
		* TODO: comments
		*/
		double
		shortest_distance(
				const std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry*>& seed_geos,
				const GPlatesAppLogic::ReconstructedFeatureGeometry* geo);
		
		/*
		* TODO: comments
		*/
		OpaqueData
		get_property_value_by_name(
				GPlatesModel::FeatureHandle::const_weak_ref feature_ref,
				QString name);

		/*
		* TODO: comments
		*/
		OpaqueData
		convert_qvariant_to_Opaque_data(
				const QVariant& data);

		/*
		* TODO: comments
		*/
		OpaqueData
		get_shape_file_value_by_name(
				GPlatesModel::FeatureHandle::const_weak_ref feature_ref,
				QString name);

		static std::vector<
				boost::tuple<
						std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry*>,
						const GPlatesAppLogic::ReconstructedFeatureGeometry* > > RFG_INDEX_VECTOR;
	}
}
#endif
