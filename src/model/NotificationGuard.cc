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
		Model *model_ptr) :
	d_model_ptr(model_ptr),
	d_guard_released(false)
{
	if (model_ptr)
	{
		model_ptr->increment_notification_guard_count();
	}
}


GPlatesModel::NotificationGuard::~NotificationGuard()
{
	release_guard();
}


void
GPlatesModel::NotificationGuard::release_guard()
{
	if (!d_guard_released)
	{
		if (d_model_ptr)
		{
			d_model_ptr->decrement_notification_guard_count();
		}

		d_guard_released = true;
	}
}


void
GPlatesModel::NotificationGuard::acquire_guard()
{
	if (d_guard_released)
	{
		if (d_model_ptr)
		{
			d_model_ptr->increment_notification_guard_count();
		}

		d_guard_released = false;
	}
}
