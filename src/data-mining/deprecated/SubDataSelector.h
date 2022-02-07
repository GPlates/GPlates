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

#ifndef GPLATESDATAMINING_SUBDATASELECTOR_H
#define GPLATESDATAMINING_SUBDATASELECTOR_H

#include "DataTable.h"
#include "CoRegConfigurationTable.h"
#include "Prospector.h"

namespace GPlatesDataMining
{

	class SubDataSelector :
		public Prospector
	{

	public:
		SubDataSelector(
				const CoRegConfigurationTable& matrix,
				GPlatesModel::FeatureHandle::const_weak_ref seed_feature,
				const FeatureGeometryMap& seed_geometry_map,
				const FeatureGeometryMap& target_geometry_map) :
			d_data_row(DataRowSharedPtr(new DataRow)),
			d_matrix(matrix),
			d_seed_feature(seed_feature),
			d_seed_geometry_map(seed_geometry_map),
			d_target_geometry_map(target_geometry_map)
		{

		}

		virtual
		void
		do_job();
			
		virtual
		~SubDataSelector()
		{ }
		protected:
			DataRowSharedPtr d_data_row;
			const CoRegConfigurationTable& d_matrix;
			GPlatesModel::FeatureHandle::const_weak_ref d_seed_feature;
			const FeatureGeometryMap& d_seed_geometry_map;
			const FeatureGeometryMap& d_target_geometry_map;
	};
}


#endif



