/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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
 
#include "ReconstructVisualLayerParams.h"


GPlatesPresentation::ReconstructVisualLayerParams::ReconstructVisualLayerParams(
		GPlatesAppLogic::LayerTaskParams &layer_task_params) :
	VisualLayerParams(layer_task_params),
	d_vgp_draw_circular_error(true)
{
}


bool
GPlatesPresentation::ReconstructVisualLayerParams::get_vgp_draw_circular_error() const
{
	return d_vgp_draw_circular_error;
}


void
GPlatesPresentation::ReconstructVisualLayerParams::set_vgp_draw_circular_error(
		bool draw)
{
	d_vgp_draw_circular_error = draw;
	emit_modified();
}

