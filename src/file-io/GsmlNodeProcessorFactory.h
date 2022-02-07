/* $Id$ */

/**
 * \file 
 * File specific comments.
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

#ifndef GPLATES_FILEIO_XMLNODEPROCESSORFACTORY_H
#define GPLATES_FILEIO_XMLNODEPROCESSORFACTORY_H

#include <QBuffer>
#include <vector>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

#include "model/FeatureCollectionHandle.h"

namespace GPlatesFileIO
{
	class GsmlPropertyHandlers;
	class GsmlNodeProcessor;

	class GsmlNodeProcessorFactory
	{
	public:
		explicit
		GsmlNodeProcessorFactory(
				GPlatesModel::FeatureHandle::weak_ref feature);

		void
		process_with_property_processors(
				const QString& feature_type,
				QByteArray& data);

		void
		process_with_property_processors(
				const QString& feature_type,
				QBuffer& buf);

	protected:
		GsmlNodeProcessorFactory();
		GsmlNodeProcessorFactory(
					const GsmlNodeProcessorFactory&);

		std::vector<boost::shared_ptr<GsmlNodeProcessor> >
		create_property_processors(
				const QString& feature_type);

	private:
		boost::shared_ptr<GsmlPropertyHandlers> d_property_handler;
	};
	
}

#endif  // GPLATES_FILEIO_XMLNODEPROCESSORFACTORY_H


