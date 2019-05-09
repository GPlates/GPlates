/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007, 2008, 2009, 2010 The University of Sydney, Australia
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
 
#include <iostream>

#include "GpmlOldPlatesHeader.h"


GPlatesUtils::UnicodeString
GPlatesPropertyValues::GpmlOldPlatesHeader::old_feature_id() const
{
	std::ostringstream oss;
	oss << "gplates";
	oss << "_";

	oss.width(2);
	oss.fill('0'); // NOTE: pad the field with ZERO
	oss << d_region_number;
	oss << "_";

	oss.width(2);
	oss.fill('0'); // NOTE: pad the field with ZERO
	oss << d_reference_number;
	oss << "_";

	oss.width(4);
	oss.fill('0'); // NOTE: pad the field with ZERO
	oss << d_string_number;
	oss << "_";

	// NOTE: this string may have spaces in it; that's acceptable 
	oss << GPlatesUtils::make_qstring( d_geographic_description ).toStdString();
	oss << "_";

	oss.width(3);
	oss.fill('0'); // NOTE: pad the field with ZERO in case the plate id is single digit
	oss << d_plate_id_number;
	oss << "_";

	oss.setf(std::ios::showpoint);
	oss.setf(std::ios::fixed);
	oss.precision(1); // old_id's only have 1 decimal of precision
	oss.width(6);
	oss.fill(' '); // NOTE: DO NOT pad the field ; old_id's have spaces around ages
	oss << d_age_of_appearance;
	oss << "_";

	oss.setf(std::ios::fixed);
	oss.setf(std::ios::showpoint);
	oss.precision(1); // old_id's only have 1 decimal of precision
	oss.width(6);
	oss.fill(' '); // NOTE: pad the field with SPACE ; old_id's have spaces around ages
	oss << d_age_of_disappearance;
	oss << "_";

	oss << GPlatesUtils::make_qstring( d_data_type_code ).toStdString();
	oss << "_";

	oss.width(4);
	oss.fill('0'); // NOTE: pad the field with ZERO
	oss << d_data_type_code_number;
	oss << "_";

	oss.width(3);
	oss.fill('0'); // NOTE: pad the field with ZERO in case the plate id is single digit
	oss << d_conjugate_plate_id_number;

	oss << "_";

	return GPlatesUtils::UnicodeString(oss.str().c_str());
}


std::ostream &
GPlatesPropertyValues::GpmlOldPlatesHeader::print_to(
		std::ostream &os) const
{
	return os << old_feature_id();
}

