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

#include "GsmlConst.h"
#include "model/FeatureCollectionHandle.h"

namespace GPlatesFileIO
{
	using GPlatesModel::FeatureCollectionHandle;
	class GsmlFeatureHandler
	{
	public:
		void
		handle_feature_memeber(
				FeatureCollectionHandle::weak_ref fc,
				QByteArray&);
		virtual 
		~GsmlFeatureHandler() { }
	protected:
		/*
		 * Override this function in subclass 
		 * to change the behavior of GsmlFeatureHandler.                                                                  
		*/
		virtual
		void
		handle_gsml_feature(
				const QString& feature_type_str,
				FeatureCollectionHandle::weak_ref fc,
				QBuffer& xml_data);
	};

	class GsmlFeatureHandlerFactory
	{
		/*
		 * Give user an opportunity to use different GsmlFeatureHandler.
		 * Change the factory if you want to equip a different GsmlFeatureHandler.                                                                  
		*/
	public:
		static
		boost::shared_ptr<GsmlFeatureHandler>
		get_instance()
		{
			return boost::shared_ptr<GsmlFeatureHandler>(new GsmlFeatureHandler());
		}
	};
}

#endif  // GPLATES_FILEIO_GSMLFEATUREHANDLERS_H


