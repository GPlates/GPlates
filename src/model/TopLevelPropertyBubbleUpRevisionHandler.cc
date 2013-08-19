/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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


#include "TopLevelPropertyBubbleUpRevisionHandler.h"

#include "Model.h"


GPlatesModel::TopLevelPropertyBubbleUpRevisionHandler::TopLevelPropertyBubbleUpRevisionHandler(
		const TopLevelProperty::non_null_ptr_type &top_level_property) :
	d_model(top_level_property->get_model()),
	d_top_level_property(top_level_property),
	d_revision(top_level_property->create_bubble_up_revision(d_transaction)),
	d_committed(false)
{
}


GPlatesModel::TopLevelPropertyBubbleUpRevisionHandler::~TopLevelPropertyBubbleUpRevisionHandler()
{
	// Since this is a destructor we cannot let any exceptions escape.
	// If one is thrown we just have to lump it and continue on.
	try
	{
		commit();
	}
	catch (...)
	{
	}
}


void
GPlatesModel::TopLevelPropertyBubbleUpRevisionHandler::commit()
{
	if (d_committed)
	{
		return;
	}

	d_committed = true;

	// Committing the transaction switches over to the new revision.
	d_transaction.commit();

	// Emit the model events if either there's no model (ie, not attached to model), or
	// we are attached to the model but the model notification guard is not currently active
	// (if it's active then the events will be re-determined and emitted when the notification
	// guard is released).
	if (!d_model ||
		!d_model->has_notification_guard())
	{
		// TODO: Emit model events.
	}
}
