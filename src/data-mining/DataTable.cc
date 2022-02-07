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

#include "global/CompilerWarnings.h"


void
GPlatesDataMining::DataRow::append_cell(
		const OpaqueData &val)
{
	d_data.push_back(val);
}


void
GPlatesDataMining::DataTable::export_as_CSV(
		const QString& filename) const
{
	std::vector<GPlatesGui::CsvExport::LineDataType> vector_table;

	vector_table.push_back(d_table_header);
	to_qstring_table(vector_table);
	
	GPlatesGui::CsvExport::ExportOptions opt;
	opt.delimiter = ',';
	GPlatesGui::CsvExport::export_data(
			filename, 
			opt, 
			vector_table);
}

void
GPlatesDataMining::DataTable::to_qstring_table(
		std::vector<std::vector<QString> >& table) const
{
	const_iterator iter = begin(), iter_end = end();

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
		table.push_back(line);
	}
	return;
}


std::ostream &
GPlatesDataMining::operator<<(
		std::ostream& os,
		const DataTable& table)
{
	DataTable::const_iterator iter = table.begin(), iter_end = table.end();

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


