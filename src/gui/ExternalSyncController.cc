/**
 * \file
 * $Revision: 8672 $
 * $Date: 2010-06-10 07:00:38 +0200 (to, 10 jun 2010) $
 *
 * Copyright (C) 2010, 2011 Geological Survey of Norway
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

#include <iostream>
#include "boost/optional.hpp"
#include <QDebug>
#include <QMessageBox>
#include <QProcess>
#include <QStringList>

#include "ExternalSyncController.h"

#include "AnimationController.h"
#include "ViewportZoom.h"

#include "app-logic/ApplicationState.h"

#include "maths/LatLonPoint.h"
#include "maths/UnitVector3D.h"

#include "presentation/ViewState.h"

#include "qt-widgets/ReconstructionViewWidget.h"
#include "qt-widgets/SceneView.h"
#include "qt-widgets/ViewportWindow.h"


//#define SYNCDEBUG


namespace
{
	

	void
	debug_send_message(
		const QString &message)
	{
#ifdef SYNCDEBUG
			std::cout << "GPlates sent the message: " << message.toStdString().c_str() << std::endl;
#endif
	}

	void
	debug_receive_message(
		const QString &message)
	{
#ifdef SYNCDEBUG
		std::cout << "GPlates received the message: " << message.toStdString().c_str() << std::endl;
#endif
	}

    /**
     * Ensures @a string_list has at least 2 items and removes the first item.
     *
     * Used to extract filenames from a list of strings such as OPENSHAPFILE <filename1> <filename2>
     */
	QStringList
	get_filenames_from_argument_list(
		const QStringList &string_list)
	{
		if (string_list.size() < 2)
		{
			return QStringList();
		}

		QStringList result_list(string_list);

		result_list.removeFirst();

		return result_list;
	}



    /**
     * Extracts llp from @a string_list when @a string_list is of the form
     * VIEWPORTCENTRE <lat> <lon>
     */
	boost::optional<GPlatesMaths::LatLonPoint>
	get_centre_from_argument_list(
		const QStringList &string_list)
	{
		if (string_list.size() < 3)
		{
			return boost::none;
		}
		QString lat_as_string = string_list.at(1);
		QString lon_as_string = string_list.at(2);

		bool lat_ok, lon_ok;
		double lat = lat_as_string.toDouble(&lat_ok);
		double lon = lon_as_string.toDouble(&lon_ok);
		if (lat_ok && lon_ok)
		{
			boost::optional<GPlatesMaths::LatLonPoint> llp;
			try
			{
				llp = GPlatesMaths::LatLonPoint(lat,lon);
				return boost::optional<GPlatesMaths::LatLonPoint>(llp);
			}
			catch(...)
			{
				return boost::none;
			}
		}

		// To satisfy MSVC
		return boost::none;

	}

    /**
     * Extracts time from @a string_list when @a string_list is of the form
     * TIME <time>
     */
	boost::optional<double>
	get_time_from_argument_list(
		const QStringList &string_list)
	{
		if (string_list.size() < 2)
		{
			return boost::none;
		}
		QString time_as_string = string_list.at(1);

		bool ok;
		double time = time_as_string.toDouble(&ok);

		if (!ok)
		{
			return boost::none;
		}

		if (time < 0)
		{
			return boost::none;
		}

		return boost::optional<double>(time);

	}

    /**
     * Extracts zoom from @a string_list when @a string_list is of the form
     * ZOOM <zoom>
     */
	boost::optional<double>
	get_zoom_from_argument_list(
		const QStringList &string_list)
	{
		if (string_list.size() < 2)
		{
			return boost::none;
		}
		QString zoom_as_string = string_list.at(1);

		bool ok;
		double zoom = zoom_as_string.toDouble(&ok);

		if (!ok)
		{
			return boost::none;
		}

		if (zoom < 0)
		{
			return boost::none;
		}

		if (zoom < 100.)
		{
			zoom = 100.;
		}
		if (zoom > 10000)
		{
			zoom = 10000.;
		}

		return boost::optional<double>(zoom);
	}

    /**
     * Extracts a rotation from @a string_list when @a string_list is of the form
     * ORIENTATION <lat> <lon> <angle>
     *
     * <lat> <lon> and <angle> represent a rotation around <lat><lon> by <angle>
     */
	boost::optional<GPlatesMaths::Rotation>
	get_orientation_from_argument_list(
		const QStringList &string_list)
	{
		if (string_list.size() < 4)
		{
			return boost::none;
		}

		QString lat_as_string = string_list.at(1);
		QString lon_as_string = string_list.at(2);
		QString angle_as_string = string_list.at(3);

		bool lat_ok;
		double lat = lat_as_string.toDouble(&lat_ok);

		bool lon_ok;
		double lon = lon_as_string.toDouble(&lon_ok);

		bool angle_ok;
		double angle = angle_as_string.toDouble(&angle_ok);

		if (lat_ok && lon_ok && angle_ok)
		{
			GPlatesMaths::LatLonPoint llp(lat,lon);
			GPlatesMaths::PointOnSphere point = GPlatesMaths::make_point_on_sphere(llp);
			return GPlatesMaths::Rotation::create(point.position_vector(),angle);
		}

		return boost::none;
	}
}

GPlatesGui::ExternalSyncController::ExternalSyncController(
	const bool &gplates_is_master,
    GPlatesQtWidgets::ViewportWindow *viewport_window_ptr,
    GPlatesPresentation::ViewState *view_state_ptr):
    d_std_in_thread_ptr(new StdInThread()),
    d_process(new QProcess(this)),
    d_viewport_window_ptr(viewport_window_ptr),
    d_animation_controller_ptr(&view_state_ptr->get_animation_controller()),
    d_reconstruction_view_widget_ptr(&viewport_window_ptr->reconstruction_view_widget()),
    d_view_state_ptr(view_state_ptr),
    d_should_sync_view(false),
    d_should_sync_time(false),
	d_most_recent_time(0.),
	d_most_recent_llp(GPlatesMaths::LatLonPoint(0.,0.)),
	d_most_recent_zoom(100.),
	d_most_recent_orientation(GPlatesMaths::Rotation::create(GPlatesMaths::UnitVector3D(0.,0.,1.),0.)),
	d_gplates_is_master(gplates_is_master),
	d_should_send_output(true)
{
}

GPlatesGui::ExternalSyncController::~ExternalSyncController()
{
    if (d_std_in_thread_ptr)
    {
	d_std_in_thread_ptr->terminate();
	d_std_in_thread_ptr->wait();
	delete d_std_in_thread_ptr;
    }

    if (d_process)
    {
	d_process->terminate();
	d_process->waitForFinished();
	delete d_process;
    }
}

void
GPlatesGui::ExternalSyncController::process_external_command(
	const QString &command_string)
{

	debug_receive_message(command_string);

    // Prevents any data going to output from GPlates while we're processing input.
    d_should_send_output = false;

	static const QString TIME_COMMAND_STRING = "TIME";
	static const QString VIEW_CENTRE_COMMAND_STRING = "PROJECTIONCENTRE";
	static const QString ZOOM_COMMAND_STRING = "DISTANCE";
	static const QString GAIN_FOCUS_COMMAND_STRING = "GAINFOCUS";
	static const QString ORIENTATION_COMMAND_STRING = "ORIENTATION";
	static const QString OPEN_FILE_COMMAND_STRING = "OPENSHAPEFILE";


	QStringList command_string_parts = command_string.split(" ",QString::SkipEmptyParts);

	if (command_string_parts.isEmpty())
	{
		return;
	}

	QString command = command_string_parts.at(0);

	if (command == TIME_COMMAND_STRING)
	{
		process_time_command(command_string_parts);
	}
	else if (command == VIEW_CENTRE_COMMAND_STRING)
	{
#if 0
		process_viewport_centre_command(command_string_parts);
#endif
	}
	else if (command == ZOOM_COMMAND_STRING)
	{
		process_zoom_command(command_string_parts);
	}
	else if (command == GAIN_FOCUS_COMMAND_STRING)
	{
		process_gain_focus_command();
	}
	else if (command == OPEN_FILE_COMMAND_STRING)
	{
		process_open_file_command(command_string_parts);
	}
	else if (command == ORIENTATION_COMMAND_STRING)
	{
		process_orientation_command(command_string_parts);
	}

    // Allow output data again.
	d_should_send_output = true;
}

void
GPlatesGui::ExternalSyncController::process_time_command(
		const QStringList &commands)
{
	boost::optional<double> time = get_time_from_argument_list(commands);

	if (time)
	{
		if (d_should_sync_time)
		{
			// Use the animation controller - this will trigger the reconstruction and update the time widget.
			set_time(*time);
		}
		d_most_recent_time = *time;

	}
}

void
GPlatesGui::ExternalSyncController::process_viewport_centre_command(
	const QStringList &commands)
{
	boost::optional<GPlatesMaths::LatLonPoint> desired_centre = get_centre_from_argument_list(commands);
	if (desired_centre)
	{
		if (d_should_sync_view)
		{
			set_projection_centre(*desired_centre);
		}
		d_most_recent_llp = *desired_centre;
	}

}

void
GPlatesGui::ExternalSyncController::process_zoom_command(
	const QStringList &commands)
{
	boost::optional<double> zoom_percent = get_zoom_from_argument_list(commands);

	if (zoom_percent)
	{
		if (d_should_sync_view)
		{
			set_zoom(*zoom_percent);
		}
		d_most_recent_zoom = *zoom_percent;
	}

}

void
GPlatesGui::ExternalSyncController::process_gain_focus_command()
{
	// Hmmm. Nothing I do here seems to raise the window to the front. Focus, yes, but not raise it above the
    // calling application.
	d_viewport_window_ptr->showNormal();
	d_viewport_window_ptr->activateWindow();
	d_viewport_window_ptr->raise();
	d_viewport_window_ptr->setFocus();
}

void
GPlatesGui::ExternalSyncController::process_open_file_command(
	const QStringList &commands)
{
	QStringList filenames = get_filenames_from_argument_list(commands);
	d_viewport_window_ptr->load_feature_collections(filenames);
}

void
GPlatesGui::ExternalSyncController::process_orientation_command(
	const QStringList &commands)
{
	boost::optional<GPlatesMaths::Rotation> rotation = get_orientation_from_argument_list(commands);
	
	if (rotation)
	{
		if (d_should_sync_view)
		{
			d_reconstruction_view_widget_ptr->active_view().set_orientation(*rotation);
		}

		d_most_recent_orientation = *rotation;
	}
}

void
GPlatesGui::ExternalSyncController::send_external_time_command(
	double time)
{
    if (d_should_sync_time)
    {
		QString message = QString("TIME %1").arg(time);

		send_external_command(message);
    }
}

void
GPlatesGui::ExternalSyncController::send_external_camera_command(
	double lat, double lon)
{
    if (d_should_sync_view)
    {
		QString message = QString("PROJECTIONCENTRE %1 %2").arg(lat).arg(lon);

		send_external_command(message);
    }

}

void
GPlatesGui::ExternalSyncController::send_external_zoom_command(
	double zoom)
{
    if (d_should_sync_view)
    {
		QString message = QString("DISTANCE %1").arg(zoom);

		send_external_command(message);
    }

}

void
GPlatesGui::ExternalSyncController::send_external_orientation_command(
	GPlatesMaths::Rotation &rotation)
{
	if (d_should_sync_view)
	{
		GPlatesMaths::UnitVector3D uv = rotation.axis();
		GPlatesMaths::PointOnSphere p(uv);

		GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(p);

		QString message = QString("ORIENTATION %1 %2 %3").arg(llp.latitude()).arg(llp.longitude()).arg(rotation.angle().dval());

		send_external_command(message);
	}
}

void
GPlatesGui::ExternalSyncController::enable_external_syncing()
{
    d_should_sync_view = true;
    d_should_sync_time = true;

    start_thread();
    connect_message_signals();

}

void
GPlatesGui::ExternalSyncController::handle_command_received(
	QString command_string)
{
	process_external_command(command_string);
}

void
GPlatesGui::ExternalSyncController::start_external_process(
    const QString &process_string)
{
    if(!(d_process->state() & QProcess::Running))
    {
		d_process->start(QString("\"%1\" --enable-gplates-commands").arg(process_string),
			QIODevice::ReadWrite |
			QIODevice::Text |
			QIODevice::Unbuffered);
#ifdef SYNCDEBUG
		std::cout << "Starting process: " << process_string.toStdString() << " from GPlates" << std::endl;
#endif
    }
}

void
GPlatesGui::ExternalSyncController::auto_sync_view(
    bool should_sync)
{
    d_should_sync_view = should_sync;
}

void
GPlatesGui::ExternalSyncController::auto_sync_time(
    bool should_sync)
{
    d_should_sync_time = should_sync;
}

double
GPlatesGui::ExternalSyncController::get_time()
{
    return d_view_state_ptr->get_application_state().get_current_reconstruction_time();
}

boost::optional<GPlatesMaths::LatLonPoint>
GPlatesGui::ExternalSyncController::get_projection_centre()
{
    return d_reconstruction_view_widget_ptr->active_view().camera_llp();
}

double
GPlatesGui::ExternalSyncController::get_zoom()
{
    return d_view_state_ptr->get_viewport_zoom().zoom_percent();
}

boost::optional<GPlatesMaths::Rotation>
GPlatesGui::ExternalSyncController::get_orientation()
{
	return d_reconstruction_view_widget_ptr->active_view().orientation();
}

void
GPlatesGui::ExternalSyncController::set_time(
	const double &time)
{
	d_animation_controller_ptr->set_view_time(time);
}

void
GPlatesGui::ExternalSyncController::set_projection_centre(
	const GPlatesMaths::LatLonPoint &llp)
{
	d_reconstruction_view_widget_ptr->active_view().set_camera_viewpoint(llp);
}

void
GPlatesGui::ExternalSyncController::set_orientation(
	const GPlatesMaths::Rotation &rotation)
{
	d_reconstruction_view_widget_ptr->active_view().set_orientation(rotation);
}

void
GPlatesGui::ExternalSyncController::set_zoom(
	const double &zoom)
{
	d_view_state_ptr->get_viewport_zoom().set_zoom_percent(zoom);
}

void
GPlatesGui::ExternalSyncController::enable_time_commands()
{
    d_should_sync_time = true;
}

void
GPlatesGui::ExternalSyncController::enable_view_commands()
{
    d_should_sync_view = true;
}

void
GPlatesGui::ExternalSyncController::start_thread()
{
    if (d_std_in_thread_ptr && !d_std_in_thread_ptr->isRunning())
    {
		d_std_in_thread_ptr->start();
    }
}

void
GPlatesGui::ExternalSyncController::connect_message_signals()
{
    QObject::connect(
	    d_std_in_thread_ptr,
	    SIGNAL(std_in_string_read(QString)),
	    this,
	    SLOT(handle_command_received(QString)));

    QObject::connect(
	    d_animation_controller_ptr,
	    SIGNAL(send_time_to_stdout(double)),
	    this,
	    SLOT(send_external_time_command(double)));

	// Camera commands now superseded by orientation commands. 
#if 0
    QObject::connect(
	    d_reconstruction_view_widget_ptr,
	    SIGNAL(send_camera_pos_to_stdout(double, double)),
	    this,
	    SLOT(send_external_camera_command(double,double)));
#endif

	QObject::connect(
		d_reconstruction_view_widget_ptr,
		SIGNAL(send_orientation_to_stdout(GPlatesMaths::Rotation &)),
		this,
		SLOT(send_external_orientation_command(GPlatesMaths::Rotation &)));

    QObject::connect(
	    &d_view_state_ptr->get_viewport_zoom(),
	    SIGNAL(send_zoom_to_stdout(double)),
	    this,
	    SLOT(send_external_zoom_command(double)));

	QObject::connect(
		d_process,
		SIGNAL(finished(int,QProcess::ExitStatus)),
		this,
		SLOT(handle_process_finished(int, QProcess::ExitStatus)));

	QObject::connect(
		d_process,
		SIGNAL(error(QProcess::ProcessError)),
		this,
		SLOT(handle_process_error(QProcess::ProcessError)));

	QObject::connect(
		d_process,
		SIGNAL(started()),
		this,
		SLOT(handle_process_started()));

	QObject::connect(
		d_process,
		SIGNAL(readyReadStandardOutput()),
		this,
		SLOT(read_process_output()));

}

void
GPlatesGui::ExternalSyncController::sync_external_time()
{
	d_should_sync_time = true;
	double time = get_time();
	send_external_time_command(time);
	d_most_recent_time = time;
	d_should_sync_time = false;
}

void
GPlatesGui::ExternalSyncController::sync_external_view()
{
	d_should_sync_view = true;
	boost::optional<GPlatesMaths::LatLonPoint> llp = get_projection_centre();
	if (llp)
	{
		send_external_camera_command(llp->latitude(),llp->longitude());
		d_most_recent_llp = *llp;
	}

	boost::optional<GPlatesMaths::Rotation> orientation = get_orientation();
	if (orientation)
	{
		send_external_orientation_command(*orientation);
		d_most_recent_orientation = *orientation;
	}

	double zoom = get_zoom();
	send_external_zoom_command(zoom);
	d_most_recent_zoom = zoom;

	d_should_sync_view = false;
}

void
GPlatesGui::ExternalSyncController::sync_gplates_time()
{
	set_time(d_most_recent_time);
}

void
GPlatesGui::ExternalSyncController::sync_gplates_view()
{
	set_orientation(d_most_recent_orientation);
	set_zoom(d_most_recent_zoom);
}

void
GPlatesGui::ExternalSyncController::handle_process_finished(
	int exit_code,
	QProcess::ExitStatus exit_status)
{
	// Emit here so that the dialog can update. 
	Q_EMIT process_finished();	
}

void
GPlatesGui::ExternalSyncController::handle_process_error(
	QProcess::ProcessError error)
{
	std::cout << "Error with external process started from GPlates." << std::endl;
	std::cout << "Error code: " << error << std::endl;
	d_process->waitForFinished();
	Q_EMIT process_finished();
}

void
GPlatesGui::ExternalSyncController::handle_process_started()
{
	// nothing to do here at the moment. 
}

void
GPlatesGui::ExternalSyncController::read_process_output()
{
	// What we do here only makes sense when gplates is controlling external software. 
	if (!d_gplates_is_master)
	{
		return;
	}

	if (!(d_process->state() & QProcess::Running))
	{
		return;
	}
	
	QString messages = QString(d_process->readAllStandardOutput()).trimmed();
	QStringList message_list = messages.split("\n",QString::SkipEmptyParts);

	QStringList::const_iterator 
		iter = message_list.begin(),
		end = message_list.end();

	for (; iter != end ; ++iter)
	{
		QString command = iter->toAscii();
		process_external_command(command);
	}
}

void
GPlatesGui::ExternalSyncController::send_external_command(
	QString &command)
{
	if (!d_should_send_output)
	{
		return;
	}

	debug_send_message(command);

	if (d_gplates_is_master)
	{
		// We send to the external process
		if (d_process->state() & QProcess::Running)
		{
			command += QString("\n");
			d_process->write(command.toAscii());
		}
	}
	else
	{
		// We send to std::out
		std::cout << command.toStdString() << std::endl << std::flush;
	}
}

void
GPlatesGui::StdInThread::run()
{
	std::string input_line;
	while(true)
	{
		std::getline(std::cin, input_line);
		Q_EMIT std_in_string_read(QString(input_line.c_str()));
	}
}
