/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
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

#include <boost/foreach.hpp>
#include <QDir>
#include <QFile>
#include <QFileInfo>

#include "TemporaryFileRegistry.h"


namespace
{
	QString
	construct_tmp_directory_path()
	{
		QString result = QDir::tempPath();
		if (!result.endsWith("/")) /* Qt uses / for separator on all platforms */
		{
			result.append("/");
		}
		return result;
	}
}


GPlatesFileIO::TemporaryFileRegistry::~TemporaryFileRegistry()
{
	BOOST_FOREACH(const QString &filename, d_filenames)
	{
		QFile file(filename);
		file.remove();
	}
}


void
GPlatesFileIO::TemporaryFileRegistry::add_file(
		const QString &filename)
{
	d_filenames.push_back(filename);
}


QString
GPlatesFileIO::TemporaryFileRegistry::make_filename_in_tmp_directory(
		const QString &filename)
{
	static const QString TMP_DIRECTORY_PATH = construct_tmp_directory_path();
	return TMP_DIRECTORY_PATH + QFileInfo(filename).fileName();
}

