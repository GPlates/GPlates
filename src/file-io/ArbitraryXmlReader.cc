/* $Id$ */

/**
 * @file 
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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
#include <QXmlQuery>
#include "ArbitraryXmlReader.h"

void
GPlatesFileIO::ArbitraryXmlReader::read_file(
		File::Reference &file_ref,
		boost::shared_ptr<ArbitraryXmlProfile> profile,
		GPlatesModel::ModelInterface &model,
		ReadErrorAccumulation &read_errors)
{
	SetErrorAccumulation setter(&read_errors,this);
	profile->populate(file_ref);
}


void
GPlatesFileIO::ArbitraryXmlReader::read_xml_data(
		File::Reference &file_ref,
		boost::shared_ptr<ArbitraryXmlProfile> profile,
		GPlatesModel::ModelInterface &model,
		QByteArray& data,
		ReadErrorAccumulation &read_errors)
{
// qDebug() << "GPlatesFileIO::ArbitraryXmlReader::read_xml_data()";
	SetErrorAccumulation setter(&read_errors,this);
	profile->populate(
			data,
			file_ref.get_feature_collection());
// qDebug() << "GPlatesFileIO::ArbitraryXmlReader::read_xml_data() END";
}


int
GPlatesFileIO::ArbitraryXmlReader::count_features(
		File::Reference &file_ref,
		boost::shared_ptr<ArbitraryXmlProfile> profile,
		GPlatesModel::ModelInterface &model,
		QByteArray& data,
		ReadErrorAccumulation &read_errors)
{
// qDebug() << "GPlatesFileIO::ArbitraryXmlReader::count_features()";
	SetErrorAccumulation setter(&read_errors,this);
	return profile->count_features(
			data,
			file_ref.get_feature_collection());
}

