/* $Id$ */
 
/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#include "FeatureCollectionWorkflow.h"



GPlatesAppLogic::FeatureCollectionWorkflow::FeatureCollectionWorkflow() :
	d_file_state(NULL)
{
}


void
GPlatesAppLogic::FeatureCollectionWorkflow::register_workflow(
		FeatureCollectionFileState &file_state)
{
	// Return if already registered.
	if (d_file_state)
	{
		return;
	}

	d_file_state = &file_state;
	d_file_state->register_workflow(this);
}


void
GPlatesAppLogic::FeatureCollectionWorkflow::unregister_workflow()
{
	// Since this is called from a destructor we cannot let any exceptions escape.
	// If one is thrown we just have to lump it and continue on.
	try
	{
		if (d_file_state)
		{
			d_file_state->unregister_workflow(this);
			d_file_state = NULL;
		}
	}
	catch (...)
	{
	}
}
