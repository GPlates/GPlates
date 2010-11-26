/* $Id: ReconstructedVirtualGeomagneticPole.cc 8651 2010-06-06 18:15:55Z jcannon $ */
 
/**
 * \file 
 * $Revision: 8651 $
 * $Date: 2010-06-06 20:15:55 +0200 (sø, 06 jun 2010) $
 * 
 * Copyright (C) 2010 Geological Survey of Norway
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

#include "ReconstructedFlowline.h"

#include "ReconstructionGeometryVisitor.h"

#include "model/WeakObserverVisitor.h"


void
GPlatesAppLogic::ReconstructedFlowline::accept_visitor(
		ConstReconstructionGeometryVisitor &visitor) const
{
	visitor.visit(GPlatesUtils::get_non_null_pointer(this));
}


void
GPlatesAppLogic::ReconstructedFlowline::accept_visitor(
		ReconstructionGeometryVisitor &visitor)
{
	visitor.visit(GPlatesUtils::get_non_null_pointer(this));
}


void
GPlatesAppLogic::ReconstructedFlowline::accept_weak_observer_visitor(
		GPlatesModel::WeakObserverVisitor<GPlatesModel::FeatureHandle> &visitor)
{
	visitor.visit_reconstructed_flowline(*this);
}

const GPlatesModel::FeatureHandle::weak_ref
GPlatesAppLogic::ReconstructedFlowline::get_feature_ref() const
{
	if (is_valid()) {
		return feature_handle_ptr()->reference();
	} else {
		return GPlatesModel::FeatureHandle::weak_ref();
	}
}

