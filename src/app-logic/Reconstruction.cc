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

#include "Reconstruction.h"

#include "global/PreconditionViolationError.h"


GPlatesAppLogic::Reconstruction::Reconstruction(
		const double &reconstruction_time,
		GPlatesModel::integer_plate_id_type anchor_plate_id,
		const ReconstructionLayerProxy::non_null_ptr_type &default_reconstruction_layer_proxy) :
	d_reconstruction_time(reconstruction_time),
	d_anchor_plate_id(anchor_plate_id),
	d_default_reconstruction_layer_proxy(default_reconstruction_layer_proxy)
{
}


GPlatesAppLogic::Reconstruction::Reconstruction(
		const double &reconstruction_time,
		GPlatesModel::integer_plate_id_type anchor_plate_id) :
	d_reconstruction_time(reconstruction_time),
	d_anchor_plate_id(anchor_plate_id),
	// Create a reconstruction layer proxy that performs identity rotations...
	d_default_reconstruction_layer_proxy(ReconstructionLayerProxy::create())
{
}
