/* $Id$ */

/**
 * \file 
 * Contains template specialisations for the templated BasicHandle class.
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

#include "BasicHandle.h"
#include "FeatureRevision.h"
#include "RevisionAwareIterator.h"

namespace GPlatesModel
{
	template<>
	void
	BasicHandle<FeatureHandle>::remove_child_parent_pointers()
	{
		// Do nothing.
	}

	template<>
	container_size_type
	BasicHandle<FeatureHandle>::actual_add(
			GPlatesGlobal::PointerTraits<TopLevelProperty>::non_null_ptr_type new_child)
	{
		// Same as the generic case, except that we make a clone first.
		// This is because we can't allow direct modification of TopLevelProperty
		// objects in the model.
		// We also don't set the parent of TLP because it doesn't have one.
		return current_revision()->add(new_child->deep_clone());
	}


	template<>
	void
	BasicHandle<FeatureHandle>::set_child_active(
			const_iterator iter,
			bool active)
	{
		// Do nothing, as TopLevelProperty objects don't have active flag.
	}


	template<>
	void
	BasicHandle<FeatureHandle>::set_children_active(
			bool active)
	{
		// Do nothing, as TopLevelProperty objects don't have active flag.
	}


	template<>
	void
	BasicHandle<FeatureStoreRootHandle>::notify_parent_of_modification()
	{
		// Do nothing, as the parent of FeatureStoreRootHandle is the Model,
		// which does not need to be notified when a modification occurs.
	}


	template<>
	Model *
	BasicHandle<FeatureStoreRootHandle>::model_ptr()
	{
		// The parent of the feature store root is the model itself.
		return d_parent_ptr;
	}


	template<>
	const Model *
	BasicHandle<FeatureStoreRootHandle>::model_ptr() const
	{
		// The parent of the feature store root is the model itself.
		return d_parent_ptr;
	}


	template<>
	void
	BasicHandle<FeatureHandle>::flush_children_pending_notifications()
	{
		// Do nothing, as TopLevelProperty objects don't emit notifications.
	}

}

