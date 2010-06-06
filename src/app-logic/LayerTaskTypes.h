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
 
#ifndef GPLATES_APP_LOGIC_LAYERTASKTYPES_H
#define GPLATES_APP_LOGIC_LAYERTASKTYPES_H


namespace GPlatesAppLogic
{
	class ApplicationState;
	class LayerTaskRegistry;

	namespace LayerTaskTypes
	{
		/**
		 * Register the layer tasks in @a task_types with @a layer_task_registry.
		 *
		 * One of the registered layers will also be set as the default layer that will be
		 * used when no layers can be found to process a feature collection.
		 * This default catch-all layer task will be the reconstruct layer task as it
		 * is the most common layer task and it is also a primary layer task (ie, can be
		 * used to automatically create a layer when a file is first loaded).
		 *
		 * NOTE: any new @a LayerTask derived types needs to have a registration entry
		 * added inside @a register_layer_task_types.
		 */
		void
		register_layer_task_types(
				LayerTaskRegistry &layer_task_registry,
				const ApplicationState &application_state);
	}
}

#endif // GPLATES_APP_LOGIC_LAYERTASKTYPES_H
