/* $Id$ */

/**
 * \file Class performs reconstructions and is based on Template Method pattern so can be extended.
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

#ifndef GPLATES_APP_LOGIC_RECONSTRUCTTEMPLATE_H
#define GPLATES_APP_LOGIC_RECONSTRUCTTEMPLATE_H

#include <vector>

#include "model/ModelInterface.h"
#include "model/FeatureCollectionHandle.h"
#include "model/Reconstruction.h"
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

namespace GPlatesAppLogic
{
	class ReconstructTemplate
	{
	public:
		/**
		 * Constructor.
		 * @param model the model interface passed to @begin_reconstruction and @a end_reconstruction.
		 */
		explicit
		ReconstructTemplate(
				GPlatesModel::ModelInterface &model);


		/**
		 * Create a reconstruction for the reconstruction time @a reconstruction_time,
		 * with root @a reconstruction_anchored_plate_id.
		 *
		 * The protected methods @a begin_reconstruction and @a end_reconstruction
		 * are called before and after the reconstruction is created respectively.
		 * The default behaviour is to do nothing.
		 * To override the default behaviour inherit from this class.
		 *
		 * The feature collections in @a reconstruction_features_collection are expected to
		 * contain reconstruction features (ie, total reconstruction sequences and absolute
		 * reference frames).
		 */
		GPlatesModel::Reconstruction::non_null_ptr_type
		reconstruct(
				const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
						reconstructable_features_collection,
				const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
						reconstruction_features_collection,
				const double &reconstruction_time,
				GPlatesModel::integer_plate_id_type reconstruction_anchored_plate_id);

	protected:
		/**
		 * Called by @a reconstruct before a reconstruction is created.
		 */
		virtual
		void
		begin_reconstruction(
				GPlatesModel::ModelInterface &model,
				const double &reconstruction_time,
				GPlatesModel::integer_plate_id_type reconstruction_anchored_plate_id)
		{  }


		/**
		 * Called by @a reconstruct after a reconstruction is created.
		 *
		 * The created reconstruction is passed as @a reconstruction.
		 *
		 * FIXME: When TopologyResolver is divided into two parts (see comment inside
		 * Reconstruct::create_reconstruction) remove it from argument list.
		 */
		virtual
		void
		end_reconstruction(
				GPlatesModel::ModelInterface &model,
				GPlatesModel::Reconstruction &reconstruction,
				const double &reconstruction_time,
				GPlatesModel::integer_plate_id_type reconstruction_anchored_plate_id,
				GPlatesFeatureVisitors::TopologyResolver &topology_resolver)
		{  }

	private:
		GPlatesModel::ModelInterface d_model;
	};
}

#endif // GPLATES_APP_LOGIC_RECONSTRUCTTEMPLATE_H
