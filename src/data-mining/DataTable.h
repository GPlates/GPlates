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

	typedef std::vector< QString > TableHeader;
	typedef boost::shared_ptr<DataRow> DataRowSharedPtr;

	class DataRow
	{
	public:
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
			ret = d_data[column_index];			
		}

		void
		append_cell(
				const OpaqueData& val);

		void
		append(
				std::size_t len,
				const OpaqueData& val)
		{
			d_data.insert(d_data.end(), len, val);
		}
		
		OpaqueData&
		operator[](std::size_t index)
		{
			return d_data[index];
		}

		size_t
		size()
		{
			return d_data.size();
		}

	protected:
		std::vector< OpaqueData > d_data;
	};

	class DataTable : 
		public std::vector<DataRowSharedPtr>
	{
	public:
		const TableHeader &
		table_header() const
		{
			return d_table_header;
		}

		void
		set_table_header(
				TableHeader& header)		
		{
			d_table_header = header;
		}

		double
		reconstruction_time() const
		{
			return d_reconstruction_time;
		}

		void
		set_reconstruction_time(
					double& new_time)
		{
			d_reconstruction_time = new_time;
		}

		void 
		export_as_CSV(
				const QString& filename) const;

		std::size_t
		data_index() const
		{
			return d_data_index;
		}

		void
		set_data_index(std::size_t idx)
		{
			d_data_index = idx;
		}


	protected:
		TableHeader d_table_header;
		double d_reconstruction_time;
		std::size_t d_data_index;

	};

	std::ostream &
	operator<<(
			std::ostream& os,
			const DataTable& table);
}

#endif














