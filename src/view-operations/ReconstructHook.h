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

#ifndef GPLATES_VIEW_OPERATIONS_RECONSTRUCTHOOK_H
#define GPLATES_VIEW_OPERATIONS_RECONSTRUCTHOOK_H

#include "utils/non_null_intrusive_ptr.h"
#include "utils/NullIntrusivePointerHandler.h"
#include "utils/ReferenceCount.h"

#include "model/types.h"


namespace GPlatesModel
{
	class ModelInterface;
	class Reconstruction;
}

namespace GPlatesFeatureVisitors
{
	class TopologyResolver;
}

namespace GPlatesViewOperations
{
	/**
	 * Base class for reconstruction hooks or callbacks.
	 * This is effectively a callback called by @a ReconstructionContext
	 * just before and just after a reconstruction is generated.
	 */
	class ReconstructHook :
			public GPlatesUtils::ReferenceCount<ReconstructHook>
	{
	public:
		typedef GPlatesUtils::non_null_intrusive_ptr<ReconstructHook,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;

		typedef boost::intrusive_ptr<ReconstructHook> maybe_null_ptr_type;
		
		virtual
		~ReconstructHook()
		{  }
		

		/**
		 * Callback hook before a reconstruction is created.
		 * Called by the @a ReconstructionContext that this object is directly or indirectly
		 * set on.
		 */
		virtual
		void
		pre_reconstruction_hook(
				GPlatesModel::ModelInterface &model,
				const double &reconstruction_time,
				GPlatesModel::integer_plate_id_type reconstruction_anchored_plate_id)
		{  }


		/**
		 * Callback hook after a reconstruction is created.
		 * Called by the @a ReconstructionContext that this object is directly or indirectly
		 * set on.
		 *
		 * FIXME: When TopologyResolver is divided into two parts (see comment inside
		 * GPlatesAppLogic::Reconstruct::create_reconstruction) remove it from argument list.
		 */
		virtual
		void
		post_reconstruction_hook(
				GPlatesModel::ModelInterface &model,
				const GPlatesModel::Reconstruction &reconstruction,
				const double &reconstruction_time,
				GPlatesModel::integer_plate_id_type reconstruction_anchored_plate_id,
				const GPlatesFeatureVisitors::TopologyResolver &topology_resolver)
		{  }
	};
}

#endif // GPLATES_VIEW_OPERATIONS_RECONSTRUCTHOOK_H
