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

#ifndef GPLATES_FILEIO_GSMLFEATUREHANDLERS_H
#define GPLATES_FILEIO_GSMLFEATUREHANDLERS_H

#include <QString>
#include <QXmlQuery>
#include <QBuffer>

#include "model/FeatureCollectionHandle.h"

namespace GPlatesFileIO
{
	class GsmlFeatureHandlers
	{
	public:
		
		static
		GsmlFeatureHandlers*
		instance()
		{
			static GsmlFeatureHandlers* inst = 
				new GsmlFeatureHandlers(GPlatesModel::FeatureCollectionHandle::weak_ref());
			return inst;
		}

		inline
		void
		set_feature_collection(
				GPlatesModel::FeatureCollectionHandle::weak_ref fc)
		{
			d_feature_collection = fc;
		}

		inline
		void
		set_device(
				QIODevice* device)
		{
			d_device = device;
		}

		void
		handle_mapped_feature(
				QBuffer& xml_data);

		void
		handle_fault_feature(
				QBuffer&);

	protected:
		explicit
		GsmlFeatureHandlers(
				GPlatesModel::FeatureCollectionHandle::weak_ref fch):
			d_feature_collection(fch)
		{
		}
		GsmlFeatureHandlers();
		GsmlFeatureHandlers(const GsmlFeatureHandlers&);
		GPlatesModel::FeatureCollectionHandle::weak_ref d_feature_collection;
		QIODevice* d_device;
	};
}

#endif  // GPLATES_FILEIO_GSMLFEATUREHANDLERS_H
