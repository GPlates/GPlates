/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
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

#ifndef GPLATESDATAMINING_COREGCONFIGURATIONTABLE_H
#define GPLATESDATAMINING_COREGCONFIGURATIONTABLE_H

#include <vector>
#include <map>

#include "Filter.h"
#include "ReducerTypes.h"
#include "DataTable.h"
#include "app-logic/FeatureCollectionFileState.h"
#include "model/FeatureHandle.h"

namespace GPlatesDataMining
{
	struct ConfigurationTableRow
	{
		GPlatesModel::FeatureCollectionHandle::const_weak_ref target_fc;
		FilterType filter_type;
		FilterCfg filter_cfg;
		QString attr_name;
		AttributeType attr_type;
		ReducerType reducer_type;
	};

	class CoRegConfigurationTable :
		public std::vector< ConfigurationTableRow > 
	{
	public:
		inline
		void
		set_seeds_file(
				std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> seed_files)
		{
			d_seed_files = seed_files;
		}

		void
		optimize();

	protected:
		bool
		is_seed_feature_collection(
				const ConfigurationTableRow& input_row);

	private:
		std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> d_seed_files;
	};

	inline
	std::ostream &
	operator<<(
			std::ostream &o,
			const ConfigurationTableRow &g)
	{
		o << g.filter_type << "\t";
		o << g.filter_cfg.d_ROI_range <<  "\t";
		o << g.attr_name.toStdString() <<  "\t";
		o << g.attr_type <<  "\t";
		o << g.reducer_type << "\t";
		return o;
	}
}
#endif //GPLATESDATAMINING_COREGCONFIGURATIONTABLE_H




