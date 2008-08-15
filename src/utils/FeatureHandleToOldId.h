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

#ifndef GPLATES_UTILS_FEATUREHANDLETOOLDID_H
#define GPLATES_UTILS_FEATUREHANDLETOOLDID_H

#include <string>
#include <unicode/unistr.h>

#include "model/FeatureHandle.h"

namespace GPlatesUtils
{
	/**
	 * From a feature handle, generate an old plates id.
	 *
	 * @return A string identifier.
	 *
	 * @pre True.
	 *
	 * @post Return-value is a string identifier 
	 */
	std::string
	get_old_id( const GPlatesModel::FeatureHandle &feature );
}

#endif  // GPLATES_UTILS_FEATUREHANDLETOOLDID_H
