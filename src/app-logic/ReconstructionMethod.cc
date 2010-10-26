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

#include <map>
#include <boost/static_assert.hpp>

#include "ReconstructionMethod.h"


namespace
{
	typedef std::map<
		GPlatesAppLogic::ReconstructionMethod::Type,
		GPlatesPropertyValues::EnumerationContent
	> recon_method_map_type;

	recon_method_map_type
	build_recon_method_map()
	{
		const GPlatesPropertyValues::EnumerationContent RECON_METHOD_STRINGS[] = {
			GPlatesPropertyValues::EnumerationContent("ByPlateId"),
			GPlatesPropertyValues::EnumerationContent("HalfStageRotation")
		};
		const unsigned int num_types = static_cast<unsigned int>(GPlatesAppLogic::ReconstructionMethod::NUM_TYPES);
		BOOST_STATIC_ASSERT(num_types == (sizeof(RECON_METHOD_STRINGS) /
			sizeof(GPlatesPropertyValues::EnumerationContent)));

		// Convert the above array into a map.
		recon_method_map_type result;
		for (unsigned int i = 0; i != num_types; ++i)
		{
			result.insert(
					recon_method_map_type::value_type(
						static_cast<GPlatesAppLogic::ReconstructionMethod::Type>(i),
						RECON_METHOD_STRINGS[i]));
		}
		return result;
	}

	const recon_method_map_type &
	recon_method_map()
	{
		static const recon_method_map_type RECON_METHOD_MAP = build_recon_method_map();
		return RECON_METHOD_MAP;
	}
}


GPlatesPropertyValues::EnumerationContent
GPlatesAppLogic::ReconstructionMethod::get_enum_as_string(
		Type reconstruction_method)
{
	const recon_method_map_type &map = recon_method_map();
	recon_method_map_type::const_iterator iter = map.find(reconstruction_method);

	return iter->second;
}


boost::optional<GPlatesAppLogic::ReconstructionMethod::Type>
GPlatesAppLogic::ReconstructionMethod::get_string_as_enum(
		const GPlatesPropertyValues::EnumerationContent &string)
{
	const recon_method_map_type &map = recon_method_map();

	for (recon_method_map_type::const_iterator iter = map.begin();
			iter != map.end(); ++iter)
	{
		if (iter->second == string)
		{
			return iter->first;
		}
	}

	return boost::none;
}

