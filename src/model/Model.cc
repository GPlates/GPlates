/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 The University of Sydney, Australia
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

#include "Model.h"

#include "ChangesetHandle.h"
#include "FeatureStoreRootHandle.h"


GPlatesModel::Model::Model():
	d_root(FeatureStoreRootHandle::create()),
	d_current_changeset_handle(NULL),
	d_notification_guard_count(0)
{
	d_root->set_parent_ptr(this, 0);
}


GPlatesModel::Model::~Model()
{
}


const GPlatesModel::WeakReference<GPlatesModel::FeatureStoreRootHandle>
GPlatesModel::Model::root()
{
	return d_root->reference();
}


const GPlatesModel::WeakReference<const GPlatesModel::FeatureStoreRootHandle>
GPlatesModel::Model::root() const
{
	return d_root->reference();
}


GPlatesModel::ChangesetHandle *
GPlatesModel::Model::current_changeset_handle()
{
	return d_current_changeset_handle;
}


const GPlatesModel::ChangesetHandle *
GPlatesModel::Model::current_changeset_handle() const
{
	return d_current_changeset_handle;
}


bool
GPlatesModel::Model::has_notification_guard() const
{
	return d_notification_guard_count;
}


void
GPlatesModel::Model::increment_notification_guard_count()
{
	++d_notification_guard_count;
}


void
GPlatesModel::Model::decrement_notification_guard_count()
{
	--d_notification_guard_count;

	if (d_notification_guard_count == 0)
	{
		// Increment/decrement count around the flushed notifications so that other notification guards
		// inside the called notification observers will not also try to flush pending notifications
		// resulting in infinite recursion.
		++d_notification_guard_count;

		d_root->flush_pending_notifications();

		--d_notification_guard_count;
	}
}


void
GPlatesModel::Model::register_changeset_handle(
		ChangesetHandle *changeset_handle)
{
	if (!d_current_changeset_handle)
	{
		d_current_changeset_handle = changeset_handle;
	}
}


void
GPlatesModel::Model::unregister_changeset_handle(
		ChangesetHandle *changeset_handle)
{
	if (d_current_changeset_handle == changeset_handle)
	{
		d_current_changeset_handle = NULL;
	}
}

