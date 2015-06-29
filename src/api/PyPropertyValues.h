/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2015 The University of Sydney, Australia
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

#ifndef GPLATES_API_PYPROPERTYVALUES_H
#define GPLATES_API_PYPROPERTYVALUES_H

#include "global/python.h"

#include "property-values/GmlDataBlock.h"


#if !defined(GPLATES_NO_PYTHON)

namespace GPlatesApi
{
	/**
	 * Create a @a GmlDataBlock containing one or more scalar types and their associated scalar values.
	 *
	 * @a scalar_type_to_values_mapping_object can be any of the following:
	 *   (1) a Python 'dict' mapping each 'ScalarType' to a sequence of float, or
	 *   (2) a Python sequence of ('ScalarType', sequence of float) tuples.
	 *
	 * This will raise Python ValueError if @a scalar_type_to_values_mapping_object is empty, or
	 * if each scalar type is not mapped to the same number of scalar values.
	 *
	 * The error message @a type_error_string should contain something like:
	 *
	 *  "Expected a 'dict' or a sequence of (scalar type, sequence of scalar values) 2-tuples"
	 */
	const GPlatesPropertyValues::GmlDataBlock::non_null_ptr_type
	create_gml_data_block(
			boost::python::object scalar_type_to_values_mapping_object,
			const char *type_error_string);


	/**
	 * Create a Python 'dict' from a @a GmlDataBlock.
	 *
	 * The returned dictionary maps each 'ScalarType' to a list of floating-point scalar values.
	 */
	boost::python::dict
	create_dict_from_gml_data_block(
			const GPlatesPropertyValues::GmlDataBlock &gml_data_block);

	/**
	 * Create a Python 'dict' from a sequene of 'GmlDataBlockCoordinateList::non_null_ptr_to_const_type'.
	 *
	 * The returned dictionary maps each 'ScalarType' to a list of floating-point scalar values.
	 */
	template <typename GmlDataBlockCoordinateListsIterType>
	boost::python::dict
	create_dict_from_gml_data_block_coordinate_lists(
			GmlDataBlockCoordinateListsIterType gml_data_block_coordinate_lists_begin,
			GmlDataBlockCoordinateListsIterType gml_data_block_coordinate_lists_end);
}

//
// Implementation
//

namespace GPlatesApi
{
	template <typename GmlDataBlockCoordinateListsIterType>
	boost::python::dict
	create_dict_from_gml_data_block_coordinate_lists(
			GmlDataBlockCoordinateListsIterType gml_data_block_coordinate_lists_begin,
			GmlDataBlockCoordinateListsIterType gml_data_block_coordinate_lists_end)
	{
		boost::python::dict scalar_values_dict;

		// Iterate over the 'GmlDataBlockCoordinateList::non_null_ptr_to_const_type's.
		GmlDataBlockCoordinateListsIterType gml_data_block_coordinate_lists_iter =
				gml_data_block_coordinate_lists_begin;
		for ( ;
			gml_data_block_coordinate_lists_iter != gml_data_block_coordinate_lists_end;
			++gml_data_block_coordinate_lists_iter)
		{
			GPlatesPropertyValues::GmlDataBlockCoordinateList::non_null_ptr_to_const_type
					gml_data_block_coordinate_list = *gml_data_block_coordinate_lists_iter;

			boost::python::list scalar_values_list_object;

			// Add the scalar values to a Python list.
			const GPlatesPropertyValues::GmlDataBlockCoordinateList::coordinates_type &scalar_values =
					gml_data_block_coordinate_list->get_coordinates();
			const unsigned int num_scalar_values = scalar_values.size();
			for (unsigned int v = 0; v < num_scalar_values; ++v)
			{
				scalar_values_list_object.append(scalar_values[v]);
			}

			const GPlatesPropertyValues::ValueObjectType &scalar_type =
					gml_data_block_coordinate_list->get_value_object_type();

			// Map the scalar type to the scalar values.
			scalar_values_dict[scalar_type] = scalar_values_list_object;
		}

		return scalar_values_dict;
	}
}

#endif   // GPLATES_NO_PYTHON

#endif // GPLATES_API_PYPROPERTYVALUES_H
