/* $Id: FeatureCollection.cc 11961 2011-07-07 03:49:38Z mchin $ */

/**
 * \file 
 * $Revision: 11961 $
 * $Date: 2011-07-07 13:49:38 +1000 (Thu, 07 Jul 2011) $
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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
#include <vector>
#include <boost/foreach.hpp>

#include "PyFeature.h"
#include "PyCoregistrationLayerProxy.h"

#if !defined(GPLATES_NO_PYTHON)

#include "app-logic/AppLogicFwd.h"
#include "app-logic/ReconstructedFeatureGeometry.h"
#include "data-mining/DataMiningUtils.h"
#include "opengl/GLContext.h"
#include "opengl/GLRenderer.h"
#include "presentation/Application.h"
#include "qt-widgets/ReconstructionViewWidget.h"
#include "qt-widgets/GlobeAndMapWidget.h"


bp::list
GPlatesApi::PyCoregistrationLayerProxy::get_all_seed_features()
{
	bp::list result;
	std::set<GPlatesModel::FeatureHandle*> feature_set;
	using namespace GPlatesDataMining;
	
	BOOST_FOREACH(
			const GPlatesModel::FeatureHandle::weak_ref f, 
			d_proxy->get_seed_features())
	{
		if(f.is_valid() && feature_set.find(f.handle_ptr()) == feature_set.end())
		{
			feature_set.insert(f.handle_ptr());
			result.append(GPlatesApi::Feature(f));
		}
	}
	return result;
}


bp::list
GPlatesApi::PyCoregistrationLayerProxy::get_associations()
{
	bp::list ret;
	const GPlatesDataMining::CoRegConfigurationTable& table = 
		d_proxy->get_current_coregistration_configuration_table();
	
	GPlatesDataMining::CoRegConfigurationTable::const_iterator it = table.begin();
	for( ; it != table.end(); it++)
	{
		const QByteArray arr = GPlatesDataMining::to_string(*it).toUtf8();
		ret.append(bp::str(arr.data()));
	}
	return ret;
}


bp::list
GPlatesApi::PyCoregistrationLayerProxy::get_coregistration_data()
{
	return get_coregistration_data(
			GPlatesPresentation::Application::instance().get_application_state().get_current_reconstruction_time());
}


bp::list
GPlatesApi::PyCoregistrationLayerProxy::get_coregistration_data(
		float time)
{
	bp::list ret;
	GPlatesOpenGL::GLContext::non_null_ptr_type gl_context =
		GPlatesPresentation::Application::instance().get_main_window().
		reconstruction_view_widget().globe_and_map_widget().get_active_gl_context();

	// Make sure the context is currently active.
	gl_context->make_current();

	// Start a begin_render/end_render scope.
	// NOTE: Before calling this, OpenGL should be in the default OpenGL state.
	GPlatesOpenGL::GLRenderer::non_null_ptr_type renderer = gl_context->create_renderer();
	GPlatesOpenGL::GLRenderer::RenderScope render_scope(*renderer);
	boost::optional<GPlatesAppLogic::CoRegistrationData::non_null_ptr_type> coregistration_data = 
		d_proxy->get_coregistration_data(*renderer, time);
	if(coregistration_data)
	{
		std::vector<std::vector<QString> > table;
		(*coregistration_data)->data_table().to_qstring_table(table);
		BOOST_FOREACH(const std::vector<QString>& row, table)
		{
			bp::list data_row;
			BOOST_FOREACH(const QString& cell, row)
			{
				const QByteArray data_array = cell.toUtf8();
				data_row.append(bp::str(data_array.data()));
			}
			ret.append(data_row);
		}
	}
	return ret;
}


using namespace GPlatesApi;

bp::list (PyCoregistrationLayerProxy::*get_current_coreg_data)(float) = 
	&PyCoregistrationLayerProxy::get_coregistration_data;

bp::list (PyCoregistrationLayerProxy::*get_coreg_data)() = 
	&PyCoregistrationLayerProxy::get_coregistration_data;

void
export_coregistration_layer_proxy()
{
	using namespace boost::python;

	class_<PyCoregistrationLayerProxy>("CoregistrationLayerProxy", no_init /* for now, disable creation */)
		.def("get_all_seed_features",	&PyCoregistrationLayerProxy::get_all_seed_features)
		.def("get_associations",		&PyCoregistrationLayerProxy::get_associations)
		.def("get_coregistration_data", get_current_coreg_data)
		.def("get_coregistration_data", get_coreg_data)
		;

}
#endif   //GPLATES_NO_PYTHON


