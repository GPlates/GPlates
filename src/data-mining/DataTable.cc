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
#include "gui/CsvExport.h"

#include "DataTable.h"
#include "OpaqueDataToQString.h"

void
GPlatesDataMining::DataTable::export_as_CSV(
		const QString& filename) const
{
	const_iterator iter = begin();
	const_iterator iter_end = end();

	std::vector<GPlatesGui::CsvExport::LineDataType> vector_table;

	for(; iter != iter_end; iter++) //for each row
	{
		std::vector<QString> line;
		//get each cell
		for(unsigned i = 0; i < (*iter)->size(); i++)
		{
			OpaqueData o_data;
			(*iter)->get_cell(i, o_data);

			line.push_back(
					boost::apply_visitor(
							GPlatesDataMining::ConvertOpaqueDataToString(),
							o_data));
			
		}
		vector_table.push_back(line);
	}

	GPlatesGui::CsvExport::ExportOptions opt;
	opt.delimiter = ',';
	GPlatesGui::CsvExport::export_data(
			filename, 
			opt, 
			vector_table);
}


std::ostream &
GPlatesDataMining::operator<<(
		std::ostream& os,
		const DataTable& table)
{
	DataTable::const_iterator iter		 = table.begin();
	DataTable::const_iterator iter_end	 = table.end();

	for(; iter != iter_end; iter++) //for each row
	{
		//get each cell
		for(unsigned i = 0; i < (*iter)->size(); i++)
		{
			OpaqueData o_data;
			(*iter)->get_cell(i,o_data);

			os	<< "{ " 
				<< boost::apply_visitor(
						GPlatesDataMining::ConvertOpaqueDataToString(),
						o_data).toStdString() 
						<< " }";
		}
		os << std::endl;
	}
	return os;
}


