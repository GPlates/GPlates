/**
 * \file
 * $Revision: 8672 $
 * $Date: 2010-06-10 07:00:38 +0200 (to, 10 jun 2010) $
 *
 * Copyright (C) 2010 Geological Survey of Norway
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

#include <QDebug>
#include <QFile>
#include <QTextStream>

#include "ErrorOpeningFileForReadingException.h"
#include "SymbolFileReader.h"

#include "gui/Symbol.h"

namespace
{
	boost::optional<GPlatesGui::feature_type_symbol_pair_type>
	read_line(
			const QString &line)
	{
		if (line.isEmpty())
		{
			return boost::none;
		}


		// Treat #.... as comment line.
		if (line.at(0) == QChar('#'))
		{
			return boost::none;
		}

		QStringList list = line.split(' ');

		// Demand at least 3 items in the line
		if (list.size() < 3)
		{
			return boost::none;
		}

		// First string: should I demand gpml: at the start?
		GPlatesModel::FeatureType feature_type =
				GPlatesModel::FeatureType::create_gpml(list[0]);

		// Second string
		boost::optional<GPlatesGui::Symbol::SymbolType> symbol_type =
				GPlatesGui::get_symbol_type_from_string(list[1]);

		if (!symbol_type)
		{
			return boost::none;
		}

		// Third string: size
		bool ok;
		int size_as_int = list[2].toInt(&ok);

		if (!ok)
		{
			size_as_int = 1;
		}

		// Fourth string: FILLED or UNFILLED
		bool filled = true;
		if (list.size() >= 4)
		{

			QString fill_string = list[3];
			if (fill_string == "UNFILLED")
			{
				filled = false;
			}
		}

		return std::make_pair<GPlatesModel::FeatureType,GPlatesGui::Symbol>
				(feature_type,
				 GPlatesGui::Symbol(*symbol_type,size_as_int,filled));

	}
}


void
GPlatesFileIO::SymbolFileReader::read_file(
		const QString &filename,
		GPlatesGui::symbol_map_type &symbol_map)
{

	QFile file(filename);

	if ( ! file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		throw ErrorOpeningFileForReadingException(GPLATES_EXCEPTION_SOURCE, filename);
	}

	//Only allow the contents of one file at a time, so clear the map now.
	symbol_map.clear();

	QTextStream input(&file);

	while(!input.atEnd())
	{
		boost::optional<GPlatesGui::feature_type_symbol_pair_type> feature_symbol_pair =
				read_line(input.readLine());

		if (feature_symbol_pair)
		{
			symbol_map.insert(*feature_symbol_pair);
		}
	}

}
