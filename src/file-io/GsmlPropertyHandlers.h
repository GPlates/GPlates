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

#ifndef GPLATES_FILEIO_GSMLPROPERTYHANDLERS_H
#define GPLATES_FILEIO_GSMLPROPERTYHANDLERS_H

#include <QString>
#include <QBuffer>

#include "model/FeatureHandle.h"

namespace GPlatesFileIO
{
	class GsmlPropertyHandlers
	{
	public:
		
		static
		GsmlPropertyHandlers*
		instance()
		{
			static GsmlPropertyHandlers* inst = 
				new GsmlPropertyHandlers(GPlatesModel::FeatureHandle::weak_ref());
			return inst;
		}

		void
		handle_geometry_property(
				QBuffer&);

		inline
		void
		set_feature(
				GPlatesModel::FeatureHandle::weak_ref f)
		{
			d_feature = f;
		}

		inline
		void
		set_device(
				QIODevice* device)
		{
			d_device = device;
		}

	protected:

		void
		process_geometries(
				QBuffer&,
				const QString&);

	protected:
		explicit
		GsmlPropertyHandlers(
				GPlatesModel::FeatureHandle::weak_ref fh):
			d_feature(fh)
		{
		}
		GsmlPropertyHandlers();
		GsmlPropertyHandlers(
				const GsmlPropertyHandlers&);
		GPlatesModel::FeatureHandle::weak_ref d_feature;
		QIODevice* d_device;
	};
	
}

#endif  // GPLATES_FILEIO_GSMLPROPERTYHANDLERS_H
