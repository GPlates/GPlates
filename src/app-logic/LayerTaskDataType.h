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
 
#ifndef GPLATES_APP_LOGIC_LAYERTASKDATATYPE_H
#define GPLATES_APP_LOGIC_LAYERTASKDATATYPE_H

#include <boost/mpl/assert.hpp>
#include <boost/mpl/contains.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/variant.hpp>

#include "ReconstructionGeometryCollection.h"
#include "ReconstructionTree.h"

#include "model/FeatureCollectionHandle.h"


namespace GPlatesAppLogic
{
	/**
	 * The possible data types that can be input to or output from a layer.
	 *
	 * Currently the three possible types are:
	 * 1) feature collection - typically used as the first level of input
	 *    to layers in the graph,
	 * 2) reconstruction geometries - typically output by layers and can be used
	 *    as inputs to other connected layers,
	 * 3) reconstruction tree - typically output by a layer that converts rotation
	 *    features (total reconstruction sequences) to a rotation tree that is used
	 *    as input to other layers for reconstruction purposes.
	 *
	 * NOTE: Keep the feature collection weak ref as the first type because
	 * it is default-constructable (boost::variant default-constructs its first bounded type).
	 */
	typedef boost::mpl::vector<
			GPlatesModel::FeatureCollectionHandle::weak_ref,
			ReconstructionGeometryCollection::non_null_ptr_to_const_type,
			ReconstructionTree::non_null_ptr_to_const_type
	> layer_task_data_types;

	/**
	 * The variant data type that is input to or output from a layer.
	 * The bounded data types of this variant are specified in @a layer_task_data_types.
	 */
	typedef boost::make_variant_over<layer_task_data_types>::type layer_task_data_type;


	/**
	 * Convenience function for extracting a bounded variant type from @a layer_task_data.
	 *
	 * Examples of the template parameter LayerDataType are:
	 * - ReconstructionGeometryCollection::non_null_ptr_to_const_type
	 * - ReconstructionTree::non_null_ptr_to_const_type
	 */
	template <class LayerDataType>
	boost::optional<LayerDataType>
	get_layer_task_data(
			const layer_task_data_type &layer_task_data)
	{
		// Compile time error to make sure that the template parameter 'LayerDataType'
		// is one of the bounded types in the 'layer_task_data_type' variant.
#ifdef WIN32 // Old-style cast error in gcc...
		BOOST_MPL_ASSERT((boost::mpl::contains<layer_task_data_types, LayerDataType>));
#endif

		// Extract the data type that we are looking for from the variant.
		const LayerDataType *layer_data =
				boost::get<LayerDataType>(&layer_task_data);

		// If the data type output by the current layer is what we're looking for...
		if (layer_data)
		{
			return *layer_data;
		}

		return boost::none;
	}
}

#endif // GPLATES_APP_LOGIC_LAYERTASKDATATYPE_H
