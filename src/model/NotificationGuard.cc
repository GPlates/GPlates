/* $Id$ */

/**
 * \file 
 * Contains the implementation of the class NotificationGuard.
 *
 * Most recent change:
 *   $Date$
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

#include "NotificationGuard.h"

#include "Model.h"


GPlatesModel::NotificationGuard::NotificationGuard(
		Model &model) :
	d_model(model),
	d_guard_released(true)
{
	acquire_guard();
}


GPlatesModel::NotificationGuard::NotificationGuard(
		boost::optional<Model &> model) :
	d_model(model),
	d_guard_released(true)
{
	acquire_guard();
}


GPlatesModel::NotificationGuard::~NotificationGuard()
{
	// Since this is a destructor we cannot let any exceptions escape.
	// If one is thrown we just have to lump it and continue on.
	try
	{
		release_guard();
	}
	catch (...)
	{
	}
}


void
GPlatesModel::NotificationGuard::release_guard()
{
	if (!d_guard_released)
	{
		if (d_model)
		{
			d_model->decrement_notification_guard_count();
		}

		d_guard_released = true;
	}
}


void
GPlatesModel::NotificationGuard::acquire_guard()
{
	if (d_guard_released)
	{
		if (d_model)
		{
			d_model->increment_notification_guard_count();
		}

		d_guard_released = false;
	}
}
