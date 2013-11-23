/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

#ifndef GPLATES_API_PYFEATURECOLLECTIONFILEFORMATREGISTRY_H
#define GPLATES_API_PYFEATURECOLLECTIONFILEFORMATREGISTRY_H

#include <QString>

#include "file-io/FeatureCollectionFileFormatRegistry.h"

#include "model/FeatureCollectionHandle.h"


#if !defined(GPLATES_NO_PYTHON)

namespace GPlatesApi
{
	/**
	 * Read a feature collection from the specified file.
	 *
	 * This interface is exposed so other API functions can use it in their implementation.
	 */
	GPlatesModel::FeatureCollectionHandle::non_null_ptr_type
	read_feature_collection(
			const GPlatesFileIO::FeatureCollectionFileFormat::Registry &registry,
			const QString &filename);

	/**
	 * Write a feature collection to the specified file.
	 *
	 * This interface is exposed so other API functions can use it in their implementation.
	 */
	void
	write_feature_collection(
			const GPlatesFileIO::FeatureCollectionFileFormat::Registry &registry,
			GPlatesModel::FeatureCollectionHandle::non_null_ptr_type feature_collection,
			const QString &filename);
}

#endif   // GPLATES_NO_PYTHON

#endif // GPLATES_API_PYFEATURECOLLECTIONFILEFORMATREGISTRY_H
