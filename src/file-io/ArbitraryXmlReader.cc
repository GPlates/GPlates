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
		const File::Reference &file_ref,
		boost::shared_ptr<ArbitraryXmlProfile> profile,
		GPlatesModel::ModelInterface &model,
		ReadErrorAccumulation &read_errors)
{

	GPlatesModel::FeatureCollectionHandle::weak_ref fc = file_ref.get_feature_collection();
	profile->populate(file_ref,fc);
}


void
GPlatesFileIO::ArbitraryXmlReader::read_xml_data(
		const File::Reference &file_ref,
		boost::shared_ptr<ArbitraryXmlProfile> profile,
		QByteArray& data)
{
	GPlatesModel::FeatureCollectionHandle::weak_ref fc = file_ref.get_feature_collection();
	profile->populate(data,fc);
}


