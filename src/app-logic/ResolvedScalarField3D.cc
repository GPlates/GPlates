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

#include "ResolvedScalarField3D.h"

#include "ScalarField3DLayerProxy.h"
#include "ReconstructionGeometryVisitor.h"

#include "model/WeakObserverVisitor.h"


GPlatesAppLogic::ResolvedScalarField3D::ResolvedScalarField3D(
		GPlatesModel::FeatureHandle &feature_handle,
		const double &reconstruction_time,
		const ScalarField3DLayerProxy::non_null_ptr_type &scalar_field_layer_proxy) :
	ReconstructionGeometry(reconstruction_time),
	WeakObserverType(feature_handle),
	d_reconstruction_time(reconstruction_time),
	d_scalar_field_layer_proxy(scalar_field_layer_proxy)
{
}


void
GPlatesAppLogic::ResolvedScalarField3D::accept_visitor(
		ConstReconstructionGeometryVisitor &visitor) const
{
	visitor.visit(GPlatesUtils::get_non_null_pointer(this));
}


void
GPlatesAppLogic::ResolvedScalarField3D::accept_visitor(
		ReconstructionGeometryVisitor &visitor)
{
	visitor.visit(GPlatesUtils::get_non_null_pointer(this));
}


void
GPlatesAppLogic::ResolvedScalarField3D::accept_weak_observer_visitor(
		GPlatesModel::WeakObserverVisitor<GPlatesModel::FeatureHandle> &visitor)
{
	visitor.visit_resolved_scalar_field_3d(*this);
}
