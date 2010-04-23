/* $Id$ */
 
/**
 * \file 
 * $Revision$
 * $Date$
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

#include <boost/bind.hpp>

#include "LayerTaskTypes.h"

#include "LayerTaskRegistry.h"
#include "ReconstructionLayerTask.h"
#include "ReconstructLayerTask.h"


void
GPlatesAppLogic::LayerTaskTypes::register_layer_task_types(
		LayerTaskRegistry &layer_task_registry,
		const ApplicationState &application_state)
{
	layer_task_registry.register_layer_task_type(
			&ReconstructLayerTask::create_layer_task,
			&ReconstructLayerTask::can_process_feature_collection);

	layer_task_registry.register_layer_task_type(
			boost::bind(&ReconstructionLayerTask::create_layer_task, boost::cref(application_state)),
			&ReconstructionLayerTask::can_process_feature_collection);
}
