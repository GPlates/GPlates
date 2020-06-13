/* $Id: FeatureCollection.cc 11961 2011-07-07 03:49:38Z mchin $ */

/**
 * \file 
 * $Revision: 11961 $
 * $Date: 2011-07-07 13:49:38 +1000 (Thu, 07 Jul 2011) $
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
#include "PyOldFeature.h"
#include "global/python.h"
#include "gui/TopologySectionsContainer.h"
#include "feature-visitors/TopologySectionsFinder.h"
#include "model/FeatureHandle.h"


namespace bp=boost::python;

namespace GPlatesApi
{
	class TopologyTools
	{
	public:
		static
		bp::list
		sections_info(OldFeature feaure)
		{
			bp::list ret;
			GPlatesFeatureVisitors::TopologySectionsFinder topo_sections_finder;
			topo_sections_finder.visit_feature(GPlatesModel::FeatureHandle::weak_ref(feaure));
			GPlatesGui::TopologySectionsContainer::const_iterator 
				b_iter = topo_sections_finder.boundary_sections_begin(),
				b_end = topo_sections_finder.boundary_sections_end();

			for(;b_iter != b_end; b_iter++)
			{
				const QByteArray id = b_iter->get_feature_id().get().qstring().toUtf8();
				const QByteArray prop_name = (*b_iter->get_geometry_property())->get_property_name().get_name().qstring().toUtf8();
				ret.append(bp::make_tuple(bp::str(id.data()),bp::str(prop_name.data()),bp::str("boundary")));
			}

			GPlatesGui::TopologySectionsContainer::const_iterator 
				i_iter = topo_sections_finder.interior_sections_begin(),
				i_end = topo_sections_finder.interior_sections_end();
			for(;i_iter != i_end; i_iter++)
			{
				const QByteArray id = i_iter->get_feature_id().get().qstring().toUtf8();
				const QByteArray prop_name = (*i_iter->get_geometry_property())->get_property_name().get_name().qstring().toUtf8();
				ret.append(bp::make_tuple(bp::str(id.data()),bp::str(prop_name.data()),bp::str("interior")));
			}
			return ret;
		}
	};
}

void
export_topology_tools()
{
	using namespace bp;
	using namespace GPlatesApi;
	class_<TopologyTools>("TopologyTools")
		.def("sections_info", &TopologyTools::sections_info).staticmethod("sections_info");
}
