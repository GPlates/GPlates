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
#include "global/python.h"
#include "presentation/Application.h"
#include "qt-widgets/ReconstructionViewWidget.h"
#include "qt-widgets/SceneView.h"
#include "maths/InvalidLatLonException.h"

#if !defined(GPLATES_NO_PYTHON)
namespace bp = boost::python;

namespace
{
	class ViewportWindow
	{
	public:
		ViewportWindow() :
			d_viewport(GPlatesPresentation::Application::instance()->get_viewport_window())
		{  }

		void
		status_message(const bp::object &obj)
		{
			if(!GPlatesApi::PythonUtils::is_main_thread())
			{
				boost::function<void ()> f = boost::bind(
						&ViewportWindow::status_message,
						this,
						obj);
				return GPlatesApi::PythonUtils::run_in_main_thread(f);
			}

			GPlatesApi::PythonInterpreterLocker interpreter_locker;
			d_viewport.status_message(QString(bp::extract<const char*>(obj))); 
		}

		void
		set_camera(double lat, double lon)
		{
			if(!GPlatesApi::PythonUtils::is_main_thread())
			{
				boost::function<void ()> f = boost::bind(
						&ViewportWindow::set_camera,
						this,
						lat, lon);
				return GPlatesApi::PythonUtils::run_in_main_thread(f);
			}

			try
			{
				GPlatesMaths::LatLonPoint center(lat,lon);
				d_viewport.reconstruction_view_widget().active_view().set_camera_viewpoint(center);
			}
			catch(GPlatesMaths::InvalidLatLonException& ex)
			{
				std::stringstream ss;
				ex.write(ss);
				qWarning() << ss;
			}
		}

		void
		move_camera_up()
		{
			if(!GPlatesApi::PythonUtils::is_main_thread())
			{
				boost::function<void ()> f = boost::bind(
						&ViewportWindow::move_camera_up,
						this);
				return GPlatesApi::PythonUtils::run_in_main_thread(f);
			}

			d_viewport.reconstruction_view_widget().active_view().move_camera_up();
		}

		void
		move_camera_down()
		{
			if(!GPlatesApi::PythonUtils::is_main_thread())
			{
				boost::function<void ()> f = boost::bind(
					&ViewportWindow::move_camera_down,
					this);
				return GPlatesApi::PythonUtils::run_in_main_thread(f);
			}

			d_viewport.reconstruction_view_widget().active_view().move_camera_down();
		}

		void
		move_camera_left()
		{
			if(!GPlatesApi::PythonUtils::is_main_thread())
			{
				boost::function<void ()> f = boost::bind(
					&ViewportWindow::move_camera_left,
					this);
				return GPlatesApi::PythonUtils::run_in_main_thread(f);
			}

			d_viewport.reconstruction_view_widget().active_view().move_camera_left();
		}

		void
		move_camera_right()
		{
			if(!GPlatesApi::PythonUtils::is_main_thread())
			{
				boost::function<void ()> f = boost::bind(
					&ViewportWindow::move_camera_right,
					this);
				return GPlatesApi::PythonUtils::run_in_main_thread(f);
			}

			d_viewport.reconstruction_view_widget().active_view().move_camera_right();
		}

		void
		rotate_camera_clockwise() 
		{
			if(!GPlatesApi::PythonUtils::is_main_thread())
			{
				boost::function<void ()> f = boost::bind(
					&ViewportWindow::rotate_camera_clockwise,
					this);
				return GPlatesApi::PythonUtils::run_in_main_thread(f);
			}

			d_viewport.reconstruction_view_widget().active_view().rotate_camera_clockwise();
		}

		void
		rotate_camera_anticlockwise() 
		{
			if(!GPlatesApi::PythonUtils::is_main_thread())
			{
				boost::function<void ()> f = boost::bind(
					&ViewportWindow::rotate_camera_anticlockwise,
					this);
				return GPlatesApi::PythonUtils::run_in_main_thread(f);
			}

			d_viewport.reconstruction_view_widget().active_view().rotate_camera_anticlockwise();
		}


		void
		reset_camera_orientation()
		{
			if(!GPlatesApi::PythonUtils::is_main_thread())
			{
				boost::function<void ()> f = boost::bind(
					&ViewportWindow::reset_camera_orientation,
					this);
				return GPlatesApi::PythonUtils::run_in_main_thread(f);
			}

			d_viewport.reconstruction_view_widget().active_view().reset_camera_orientation();
		}

		void
		zoom_in(double num_levels)
		{
			if(!GPlatesApi::PythonUtils::is_main_thread())
			{
				boost::function<void ()> f = boost::bind(
					&ViewportWindow::zoom_in,
					this,
					num_levels);
				return GPlatesApi::PythonUtils::run_in_main_thread(f);
			}
			GPlatesPresentation::Application::instance()->get_view_state().get_viewport_zoom().zoom_in(num_levels);
		}


		void
		zoom_out(double num_levels)
		{
			if(!GPlatesApi::PythonUtils::is_main_thread())
			{
				boost::function<void ()> f = boost::bind(
					&ViewportWindow::zoom_out,
					this,
					num_levels);
				return GPlatesApi::PythonUtils::run_in_main_thread(f);
			}

			GPlatesPresentation::Application::instance()->get_view_state().get_viewport_zoom().zoom_out(num_levels);
		}


		void
		reset_zoom()
		{
			if(!GPlatesApi::PythonUtils::is_main_thread())
			{
				boost::function<void ()> f = boost::bind(
					&ViewportWindow::reset_zoom,
					this);
				return GPlatesApi::PythonUtils::run_in_main_thread(f);
			}

			GPlatesPresentation::Application::instance()->get_view_state().get_viewport_zoom().reset_zoom();
		}


		void
		set_zoom_percent(double new_zoom_percent)
		{
			if(!GPlatesApi::PythonUtils::is_main_thread())
			{
				boost::function<void ()> f = boost::bind(
					&ViewportWindow::set_zoom_percent,
					this,
					new_zoom_percent);
				return GPlatesApi::PythonUtils::run_in_main_thread(f);
			}

			GPlatesPresentation::Application::instance()->get_view_state().get_viewport_zoom().set_zoom_percent(new_zoom_percent);
		}

	private:
		GPlatesQtWidgets::ViewportWindow &d_viewport;
	};
}

	
void
export_main_window()
{
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
		;
}
#endif







