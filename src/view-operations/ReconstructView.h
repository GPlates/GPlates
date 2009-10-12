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

#ifndef GPLATES_VIEW_OPERATIONS_RECONSTRUCTVIEW_H
#define GPLATES_VIEW_OPERATIONS_RECONSTRUCTVIEW_H

#include "app-logic/Reconstruct.h"


namespace GPlatesAppLogic
{
	class PlateVelocities;
}

namespace GPlatesGui
{
	class ColourTable;
}

namespace GPlatesViewOperations
{
	class RenderedGeometryCollection;
}

namespace GPlatesViewOperations
{
	/**
	 * Handles tasks that need to be performed whenever a reconstruction is generated.
	 *
	 * These tasks include some app logic tasks such as solving plate velocities and
	 * some view related tasks such as rendering RFGs.
	 */
	class ReconstructView :
			public GPlatesAppLogic::Reconstruct::Hook
	{
	public:
		ReconstructView(
				GPlatesAppLogic::PlateVelocities &plate_velocities,
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
				const GPlatesGui::ColourTable &colour_table) :
			d_plate_velocities(plate_velocities),
			d_rendered_geom_collection(rendered_geom_collection),
			d_colour_table(colour_table)
		{  }


		/**
		 * Called after a reconstruction is created.
		 */
		virtual
		void
		end_reconstruction(
				GPlatesModel::ModelInterface &model,
				GPlatesModel::Reconstruction &reconstruction,
				const double &reconstruction_time,
				GPlatesModel::integer_plate_id_type reconstruction_anchored_plate_id,
				const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
						reconstructable_features_collection,
				const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
						reconstruction_features_collection,
				GPlatesFeatureVisitors::TopologyResolver &topology_resolver);

	private:
		GPlatesAppLogic::PlateVelocities &d_plate_velocities;
		GPlatesViewOperations::RenderedGeometryCollection &d_rendered_geom_collection;
		const GPlatesGui::ColourTable &d_colour_table;
	};
}

#endif // GPLATES_VIEW_OPERATIONS_RECONSTRUCTVIEW_H
