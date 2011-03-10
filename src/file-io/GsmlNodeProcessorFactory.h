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

#include <vector>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

#include "GsmlNodeProcessor.h"

namespace GPlatesFileIO
{
	//hard code here temporarily
	const QString xsi_ns = 
		"declare namespace xsi=\"http://www.w3.org/2001/XMLSchema-instance\";";
	const QString gml_ns = 
		"declare namespace gml=\"http://www.opengis.net/gml\";";
	const QString wfs_ns = 
		"declare namespace wfs=\"http://www.opengis.net/wfs\";";
	const QString gsml_ns = 
		"declare namespace gsml=\"urn:cgi:xmlns:CGI:GeoSciML:2.0\";";
	const QString sa_ns = 
		"declare namespace sa=\"http://www.opengis.net/sampling/1.0\";";
	const QString om_ns = 
		"declare namespace om=\"http://www.opengis.net/om/1.0\";";
	const QString cgu_ns = 
		"declare namespace cgu=\"urn:cgi:xmlns:CGI:Utilities:1.0\";";
	const QString xlink_ns = 
		"declare namespace xlink=\"http://www.w3.org/1999/xlink\";";

	class GsmlFeatureHandlers;
	class GsmlPropertyHandlers;

	class GsmlNodeProcessorFactory
	{
	public:
		static
		std::vector<boost::shared_ptr<GsmlNodeProcessor> >
		get_feature_processors();

		static
		std::vector<boost::shared_ptr<GsmlNodeProcessor> >
		get_property_processors_for_mapped_feature();

	protected:
		static
		std::vector<boost::shared_ptr<GsmlNodeProcessor> >
		create_feature_processors();

		static
		std::vector<boost::shared_ptr<GsmlNodeProcessor> >
		create_property_processors_for_mapped_feature();
	};
	
}

#endif  // GPLATES_FILEIO_XMLNODEPROCESSORFACTORY_H
