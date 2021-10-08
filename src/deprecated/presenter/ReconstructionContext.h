/* $Id$ */

/**
 * \file
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007 The University of Sydney, Australia
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

#ifndef GPLATES_PRESENTER_RECONSTRUCTIONCONTEXT_H
#define GPLATES_PRESENTER_RECONSTRUCTIONCONTEXT_H

#include "ExposedPresenterObject.h"

namespace GPlatesPresenter
{
	/** 
	 * A ReconstructionContext is an ExposedPresenterObject
	 * which handles the management of data necessary to generate
	 * a ReconstructionObject.
	 */
	class ReconstructionContext : public virtual ExposedPresenterObject {

		public:

		ReconstructionContext()
		{
			// EMIT RECONSTRUCTION_CONTEXT_CREATED<>
		}

		~ReconstructionContext()
		{
			// EMIT RECONSTRUCTION_CONTEXT_DESTROYED<>
		}

		void
		add_feature_collection(
				GPlatesModel::FeatureCollection::weak_ref fc,
				UsageMask mask)
		{
			// EMIT RECONSTRUCTION_CONTEXT_FEATURE_COLLECTION_ADDED<>
		}

		void
		remove_feature_collection(
				GPlatesModel::FeatureCollection::weak_ref fc,
				UsageMask mask)
		{
			// EMIT RECONSTRUCTION_CONTEXT_FEATURE_COLLECTION_REMOVED<>
		}

		void
		set_time(
				unsigned long time)
		{
			// EMIT RECONSTRUCTION_CONTEXT_TIME_SET<>
		}

		void
		set_root(
				unsigned long root)
		{
			// EMIT RECONSTRUCTION_CONTEXT_ROOT_SET<>
		}

		bool
		is_dirty() const
		{
			return d_dirty;
		}

		Reconstruction::non_null_ptr_type
		reconstruction_instance() const
		{
			if (! d_reconstruction_instance) {
				// create a new instance
			}
			return d_reconstruction_instance;
		}

	};

}

#endif  // GPLATES_PRESENTER_RECONSTRUCTIONCONTEXT_H
