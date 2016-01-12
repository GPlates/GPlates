/* $Id$ */

/**
 * \file 
 * File specific comments.
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

#include <iostream>

#include "GpmlMeasure.h"

#include "model/BubbleUpRevisionHandler.h"


const GPlatesPropertyValues::StructuralType
GPlatesPropertyValues::GpmlMeasure::STRUCTURAL_TYPE = GPlatesPropertyValues::StructuralType::create_gpml("measure");


void
GPlatesPropertyValues::GpmlMeasure::set_quantity(
		const double &q)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().quantity = q;
	revision_handler.commit();
}


void
GPlatesPropertyValues::GpmlMeasure::set_quantity_xml_attributes(
		const std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> &qxa)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().quantity_xml_attributes = qxa;
	revision_handler.commit();
}


std::ostream &
GPlatesPropertyValues::GpmlMeasure::print_to(
		std::ostream &os) const
{
	return os << get_current_revision<Revision>().quantity;
}

