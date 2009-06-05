/* $Id: FeatureHandleToOldId.cc 3399 2008-07-25 01:59:27Z mturner $ */

/**
 * \file
 * File specific comments.
 *
 * Most recent change:
 *   $Date: 2008-07-24 18:59:27 -0700 (Thu, 24 Jul 2008) $
 *
 * Copyright (C) 2006, 2007 The University of Sydney, Australia
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

#include <string>
#include <sstream>
#include <cstdlib>
#include <iostream>
#include <unicode/unistr.h>


#include "FeatureHandleToOldId.h"

#include "utils/UnicodeStringUtils.h"

#include "model/types.h"
#include "model/FeatureHandle.h"
#include "feature-visitors/ValueFinder.h"

std::string
GPlatesUtils::get_old_id( const GPlatesModel::FeatureHandle &feature )
{
	// start with a nice empty string
	std::string id = "";

	// Create a visitor to check for old plates header property 
	static const GPlatesModel::PropertyName prop_name = 
		GPlatesModel::PropertyName::create_gpml("oldPlatesHeader");

	GPlatesFeatureVisitors::ValueFinder finder(prop_name);

	finder.visit_feature_handle(feature);

	if (finder.found_values_begin() != finder.found_values_end())
	{
		std::string old_id = *finder.found_values_begin();
		// std::cout << "finder old_id " << old_id << std::endl;
		id.append( old_id );
	}
	return id;
}

std::string
GPlatesUtils::get_old_id( GPlatesModel::FeatureHandle::weak_ref ref)
{
	// start with a nice empty string
	std::string id = "";

	// Create a visitor to check for old plates header property 
	static const GPlatesModel::PropertyName prop_name = 
		GPlatesModel::PropertyName::create_gpml("oldPlatesHeader");

	GPlatesFeatureVisitors::ValueFinder finder(prop_name);

	finder.visit_feature_handle( *ref );

	if (finder.found_values_begin() != finder.found_values_end())
	{
		std::string old_id = *finder.found_values_begin();
		// std::cout << "finder old_id " << old_id << std::endl;
		id.append( old_id );
	}
	return id;
}



