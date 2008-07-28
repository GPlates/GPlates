/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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

#include "ReconstructedFeatureGeometry.h"
#include "ReconstructionGeometryVisitor.h"
#include "global/IntrusivePointerZeroRefCountException.h"


const GPlatesModel::ReconstructedFeatureGeometry::non_null_ptr_type
GPlatesModel::ReconstructedFeatureGeometry::get_non_null_pointer()
{
	if (ref_count() == 0) {
		// How did this happen?  This should not have happened.
		//
		// Presumably, the programmer obtained the raw ReconstructedFeatureGeometry pointer
		// from inside a ReconstructedFeatureGeometry::non_null_ptr_type, and is invoking
		// this member function upon the instance indicated by the raw pointer, after all
		// ref-counting pointers have expired and the instance has actually been deleted.
		//
		// Regardless of how this happened, this is an error.
		throw GPlatesGlobal::IntrusivePointerZeroRefCountException(this, __FILE__, __LINE__);
	} else {
		// This instance is already managed by intrusive-pointers, so we can simply return
		// another intrusive-pointer to this instance.
		return non_null_ptr_type(
				this,
				GPlatesUtils::NullIntrusivePointerHandler());
	}
}


void
GPlatesModel::ReconstructedFeatureGeometry::accept_visitor(
		ReconstructionGeometryVisitor &visitor)
{
	visitor.visit_reconstructed_feature_geometry(this->get_non_null_pointer());
}
