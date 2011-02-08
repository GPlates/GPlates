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

#include "AssociationOperator.h"
#include "AssociationOperatorFactory.h"
#include "DataOperator.h"
#include "DataOperatorFactory.h"
#include "DataTable.h"

#include "app-logic/FeatureCollectionFileState.h"

#include "model/FeatureHandle.h"

namespace GPlatesDataMining
{
	/*
	 * TODO:
	*/
	struct ConfigurationTableRow
	{
		GPlatesModel::FeatureCollectionHandle::const_weak_ref target_feature_collection_handle;
		AssociationOperatorType association_operator_type;
		AssociationOperatorParameters association_parameters;
		QString attribute_name;
		AttributeType attr_type;
		DataOperatorType data_operator_type;
		DataOperatorParameters data_operator_parameters;
	};

	/*
	 * TODO:
	*/
	class CoRegConfigurationTable :
		public std::vector< ConfigurationTableRow > 
	{
	public:
		inline
		AssociationOperatorType
		associate_operator_type(
				iterator iter)
		{
			return iter->association_operator_type;
		}

		inline
		const QString&
		export_path() const
		{
			return d_export_path;
		}

		inline
		QString&
		export_path() 
		{
			return d_export_path;
		}

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
		//Put export path here temporarily. 
		//Eventually, the export will be done in export dialog.
		QString d_export_path;

		std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> d_seed_files;
	};
}
#endif //GPLATESDATAMINING_COREGCONFIGURATIONTABLE_H




