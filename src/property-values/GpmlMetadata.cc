/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2016 The University of Sydney, Australia
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

#include "GpmlMetadata.h"


const GPlatesPropertyValues::StructuralType
GPlatesPropertyValues::GpmlMetadata::STRUCTURAL_TYPE = GPlatesPropertyValues::StructuralType::create_gpml("GpmlMetadata");


void
GPlatesPropertyValues::GpmlMetadata::set_data(
		const GPlatesModel::FeatureCollectionMetadata &metadata)
{
	GPlatesModel::BubbleUpRevisionHandler revision_handler(this);
	revision_handler.get_revision<Revision>().metadata = metadata;
	revision_handler.commit();
}


std::ostream &
GPlatesPropertyValues::GpmlMetadata::print_to(
		std::ostream &os) const
{
	qWarning() << "TODO: implement this function.";
	os << "TODO: implement this function.";
	return  os;
}
