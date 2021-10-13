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

#ifndef GPLATESDATAMINING_DATATABLE_H
#define GPLATESDATAMINING_DATATABLE_H

#include <vector>
#include <iostream>

#include <QDebug>
#include <QString>

#include <boost/shared_ptr.hpp>
#include <boost/optional.hpp>
#include <boost/variant.hpp>

#include "OpaqueData.h"

namespace GPlatesAppLogic
{
	class ReconstructedFeatureGeometry;
}

namespace GPlatesDataMining
{
	class DataRow;

	typedef std::vector< QString > TableDesc;
	typedef boost::shared_ptr<DataRow> DataRowSharedPtr;
	typedef boost::shared_ptr<OpaqueData>	   DataCellSharedPtr;

	/*
	* TODO: comments
	*/ 
	class DataRow
	{
	public:
		inline
		void
		get_cell(
				unsigned column_index,
				OpaqueData& ret) const
		{
			if(column_index >= d_data.size())
			{
				qWarning() << "The column index which is used to get table cell is invalid";
				return;
			}
			ret = *d_data[column_index];			
		}

		inline
		void
		append_cell(
				const OpaqueData& val)
		{
			DataCellSharedPtr ptr(new OpaqueData(val));
			d_data.push_back(ptr);
		}

		inline
		size_t
		size()
		{
			return d_data.size();
		}

		inline
		void
		set_seed_rfgs(
				const std::vector< const GPlatesAppLogic::ReconstructedFeatureGeometry* >& seed_RFGs)
		{
			d_seed_RFGs = seed_RFGs;
		}

		inline
		const std::vector< const GPlatesAppLogic::ReconstructedFeatureGeometry* >&
		seed_rfgs() const
		{
			return d_seed_RFGs;
		}

	protected:
		std::vector< DataCellSharedPtr > d_data;
		std::vector< const GPlatesAppLogic::ReconstructedFeatureGeometry* > d_seed_RFGs;
	};

	/*
	* TODO:
	*/
	class DataTable : 
		public std::vector<DataRowSharedPtr>
	{
	public:
		inline
		const TableDesc &
		get_table_desc() const
		{
			return d_table_desc;
		}

		inline
		void
		set_table_desc(
				TableDesc& desc)		
		{
			d_table_desc = desc;
		}

		inline
		double
		reconstruction_time() const
		{
			return d_reconstruction_time;
		}

		inline
		void
		set_reconstruction_time(
					double& new_time)
		{
			d_reconstruction_time = new_time;
		}

		void 
		export_as_CSV(
				const QString& filename) const;		


	protected:
		TableDesc d_table_desc;
		double d_reconstruction_time;

	};

	std::ostream &
	operator<<(
			std::ostream& os,
			const DataTable& table);
}

#endif














