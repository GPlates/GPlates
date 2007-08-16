/* $Id$ */

/**
 * \file 
 * Contains the definitions of the member functions of the class PlatesLineFormatReader.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007 The University of Sydney, Australia
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

#include <fstream>
#include "PlatesLineFormatReader.h"
#include "PlatesBoundaryParser.h"
#include "PlatesPostParseTranslator.h"

const GPlatesModel::FeatureCollectionHandle::weak_ref
GPlatesFileIO::PlatesLineFormatReader::read_file(
		const std::string &filename,
		GPlatesModel::ModelInterface &model,
		ReadErrorAccumulation &read_errors)
{
	PlatesParser::PlatesDataMap plates_data;

	std::ifstream input(filename.c_str());
	if ( ! input) {
		throw ErrorOpeningFileForReadingException(filename);
	}
	PlatesParser::ReadInPlateBoundaryData(filename.c_str(), input, 
			plates_data, read_errors);
	return PlatesPostParseTranslator::get_features_from_plates_data(
			model, plates_data, filename, read_errors);
}
