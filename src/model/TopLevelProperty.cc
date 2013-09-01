/* $Id$ */

/**
 * \file 
 * Contains the implementation of the class TopLevelProperty.
 *
 * Most recent change:
 *   $Date$
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

#include <typeinfo>

#include "TopLevelProperty.h"

#include "BubbleUpRevisionHandler.h"
#include "FeatureHandle.h"


void
GPlatesModel::TopLevelProperty::set_xml_attributes(
		const xml_attributes_type &xml_attributes)
{
	BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().xml_attributes = xml_attributes;
	revision_handler.commit();
}


std::ostream &
GPlatesModel::operator<<(
		std::ostream &os,
		const TopLevelProperty &top_level_prop)
{
	return top_level_prop.print_to(os);
}

