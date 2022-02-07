/* $Id$ */

/**
 * \file 
 * Contains the implementation of the class ChangesetHandle.
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

#include "ChangesetHandle.h"

#include "Model.h"


GPlatesModel::ChangesetHandle::ChangesetHandle(
		Model *model_ptr,
		const std::string &description_) :
	d_model_ptr(model_ptr),
	d_description(description_)
{
	if (model_ptr)
	{
		model_ptr->register_changeset_handle(this);
	}
}


GPlatesModel::ChangesetHandle::~ChangesetHandle()
{
	if (d_model_ptr)
	{
		d_model_ptr->unregister_changeset_handle(this);
	}
}


const std::string &
GPlatesModel::ChangesetHandle::description() const
{
	return d_description;
}


void
GPlatesModel::ChangesetHandle::add_handle(
		const void *handle)
{
	d_modified_handles.insert(handle);
}


bool
GPlatesModel::ChangesetHandle::has_handle(
		const void *handle)
{
	return d_modified_handles.find(handle) != d_modified_handles.end();
}

