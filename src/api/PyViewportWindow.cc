/* $Id:  $ */

/**
 * \file 
 * $Revision: 7584 $
 * $Date: 2010-02-10 19:29:36 +1100 (Wed, 10 Feb 2010) $
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

#include <sstream>

#include "PythonUtils.h"
#include "PyFeature.h"

#include "global/CompilerWarnings.h"
#include "global/python.h"

#include "gui/AnimationController.h"
#include "gui/FeatureFocus.h"

#include "maths/InvalidLatLonException.h"
#include "maths/LatLonPoint.h"

#include "presentation/Application.h"

#include "qt-widgets/ReconstructionViewWidget.h"
#include "qt-widgets/SceneView.h"

#include "utils/FeatureUtils.h"


namespace bp = boost::python;

namespace GPlatesApi
{
	class ViewportWindow
	{
	public:
		ViewportWindow() :
			d_viewport(GPlatesPresentation::Application::instance().get_main_window()),
			d_scene_view(d_viewport.reconstruction_view_widget().active_view()),
			d_zoom(GPlatesPresentation::Application::instance().get_view_state().get_viewport_zoom())
		{  }

		void
		status_message(const char* msg)
		{
			DISPATCH_GUI_FUN<void>(boost::bind(&ViewportWindow::status_message, this, msg));

			d_viewport.status_message(QString::fromUtf8(msg)); 
		}

		void
		set_camera(double lat, double lon)
		{
			DISPATCH_GUI_FUN<void>(boost::bind(&ViewportWindow::set_camera,	this, lat, lon));

			try
			{
				GPlatesMaths::LatLonPoint center(lat,lon);
				d_scene_view.set_camera_viewpoint(center);
			}
			catch(GPlatesMaths::InvalidLatLonException& ex)
			{
				std::stringstream ss;
				ex.write(ss);
				qWarning() << ss.str().c_str();
			}
		}

		void
		move_camera_up()
		{
			DISPATCH_GUI_FUN<void>(boost::bind(&ViewportWindow::move_camera_up,this));

			d_scene_view.move_camera_up();
		}

		void
		move_camera_down()
		{
			DISPATCH_GUI_FUN<void>(boost::bind(&ViewportWindow::move_camera_down,this));

			d_scene_view.move_camera_down();
		}

		void
		move_camera_left()
		{
			DISPATCH_GUI_FUN<void>(boost::bind(&ViewportWindow::move_camera_left,this));

			d_scene_view.move_camera_left();
		}

		void
		move_camera_right()
		{
			DISPATCH_GUI_FUN<void>(boost::bind(&ViewportWindow::move_camera_right, this));

			d_scene_view.move_camera_right();
		}

		void
		rotate_camera_clockwise() 
		{
			DISPATCH_GUI_FUN<void>(boost::bind(&ViewportWindow::rotate_camera_clockwise, this));

			d_scene_view.rotate_camera_clockwise();
		}

		void
		rotate_camera_anticlockwise() 
		{
			DISPATCH_GUI_FUN<void>(boost::bind(&ViewportWindow::rotate_camera_anticlockwise, this));

			d_scene_view.rotate_camera_anticlockwise();
		}


		void
		reset_camera_orientation()
		{
			DISPATCH_GUI_FUN<void>(boost::bind(&ViewportWindow::reset_camera_orientation, this));

			d_scene_view.reset_camera_orientation();
		}

		void
		zoom_in(double num_levels)
		{
			DISPATCH_GUI_FUN<void>(boost::bind(&ViewportWindow::zoom_in, this, num_levels));
			
			d_zoom.zoom_in(num_levels);
		}


		void
		zoom_out(double num_levels)
		{			
			DISPATCH_GUI_FUN<void>(boost::bind(&ViewportWindow::zoom_out, this,	num_levels));

			d_zoom.zoom_out(num_levels);
		}


		void
		reset_zoom()
		{
			DISPATCH_GUI_FUN<void>(boost::bind(&ViewportWindow::reset_zoom,	this));

			d_zoom.reset_zoom();
		}


		void
		set_zoom_percent(double new_zoom_percent)
		{
			DISPATCH_GUI_FUN<void>(boost::bind(&ViewportWindow::set_zoom_percent,this,new_zoom_percent));

			d_zoom.set_zoom_percent(new_zoom_percent);
		}

		DISABLE_GCC_WARNING("-Wshadow")

		void
		set_focus(
				Feature feature)
		{
			DISPATCH_GUI_FUN<void>(boost::bind(&ViewportWindow::set_focus, this, feature));

			GPlatesPresentation::Application &app = GPlatesPresentation::Application::instance();
			//set focus
			app.get_view_state().get_feature_focus().set_focus(GPlatesModel::FeatureHandle::weak_ref(feature));

			boost::optional<GPlatesMaths::LatLonPoint> point = GPlatesGui::locate_focus();
			if(point)
				d_scene_view.set_camera_viewpoint(*point);
		}

		void
		set_focus_by_id(
				boost::python::object id)
		{
			DISPATCH_GUI_FUN<void>(boost::bind(&ViewportWindow::set_focus_by_id, this, id));
			
			GPlatesPresentation::Application &app = GPlatesPresentation::Application::instance();
			std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> files =
				app.get_application_state().get_feature_collection_file_state().get_loaded_files();
			
			BOOST_FOREACH(GPlatesAppLogic::FeatureCollectionFileState::file_reference& file_ref, files)
			{
				GPlatesModel::FeatureCollectionHandle::weak_ref fc = file_ref.get_file().get_feature_collection();
				BOOST_FOREACH(GPlatesModel::FeatureHandle::non_null_ptr_type feature, *fc)
				{
					if(feature->feature_id().get().qstring() == QString(boost::python::extract<const char*>(id)))
					{
						set_focus(Feature(feature->reference()));
					}
				}
			}
			qWarning() << QString("Cannot found the feature by id: %1").arg(QString(boost::python::extract<const char*>(id)));
		}

	private:
		GPlatesQtWidgets::ViewportWindow &d_viewport;
		GPlatesQtWidgets::SceneView & d_scene_view;
		GPlatesGui::ViewportZoom & d_zoom;
	};
}

	
void
export_main_window()
{
	using namespace GPlatesApi;

	bp::class_<ViewportWindow>("MainWindow")
		.def("set_status_message", &ViewportWindow::status_message)
		.def("set_camera", &ViewportWindow::set_camera)
		.def("move_camera_up", &ViewportWindow::move_camera_up)
		.def("move_camera_down", &ViewportWindow::move_camera_down)
		.def("move_camera_left", &ViewportWindow::move_camera_left)
		.def("move_camera_right", &ViewportWindow::move_camera_right)
		.def("rotate_camera_clockwise", &ViewportWindow::rotate_camera_clockwise)
		.def("rotate_camera_anticlockwise", &ViewportWindow::rotate_camera_anticlockwise)
		.def("reset_camera_orientation", &ViewportWindow::reset_camera_orientation)
		.def("zoom_in", &ViewportWindow::zoom_in)
		.def("zoom_out", &ViewportWindow::zoom_out)
		.def("reset_zoom", &ViewportWindow::reset_zoom)
		.def("set_zoom_percent", &ViewportWindow::set_zoom_percent)
		.def("set_focus", &ViewportWindow::set_focus_by_id)
		.def("set_focus", &ViewportWindow::set_focus)
		;
}
