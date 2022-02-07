/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#include "ScribeExportRegistry.h"

#include "ScribeExceptions.h"

#include "global/GPlatesAssert.h"


boost::optional<const GPlatesScribe::ExportClassType &>
GPlatesScribe::ExportRegistry::get_class_type(
		const std::string &class_id_name) const
{
	// Look up the class id name.
	class_id_name_to_type_map_type::const_iterator class_type_iter =
			d_class_id_name_to_type_map.find(class_id_name);
	if (class_type_iter == d_class_id_name_to_type_map.end())
	{
		return boost::none;
	}

	const ExportClassType &class_type = *class_type_iter->second;

	return class_type;
}


boost::optional<const GPlatesScribe::ExportClassType &>
GPlatesScribe::ExportRegistry::get_class_type(
		const std::type_info &class_type_info) const
{
	// Look up the class type info.
	class_type_info_to_type_map_type::const_iterator class_type_iter =
			d_class_type_info_to_type_map.find(&class_type_info);
	if (class_type_iter == d_class_type_info_to_type_map.end())
	{
		return boost::none;
	}

	const ExportClassType &class_type = *class_type_iter->second;

	return class_type;
}
