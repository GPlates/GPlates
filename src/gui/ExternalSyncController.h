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

#ifndef GPLATES_GUI_EXTERNALSYNCCONTROLLER_H
#define GPLATES_GUI_EXTERNALSYNCCONTROLLER_H

#include "boost/optional.hpp"

#include <QProcess>
#include <QThread>

#include "maths/LatLonPoint.h"
#include "maths/Rotation.h"

//#define SYNCDEBUG


namespace GPlatesMaths
{
	class Rotation;
}

namespace GPlatesPresentation
{
    class ViewState;
}

namespace GPlatesQtWidgets
{
    class ReconstructionViewWidget;
    class ViewportWindow;
}

namespace GPlatesGui
{
	class AnimationController;
	class ViewportZoom;

   /**
     * Thread for monitoring stdIn
     */
    class StdInThread :
		public QThread
    {
       Q_OBJECT;
    public:

	StdInThread()
	{};

	void
	run();

    signals:

	void
	std_in_string_read(
	    QString str);
    };

    class ExternalSyncController :
	    public QObject
    {
	Q_OBJECT;

    public:

	explicit
	ExternalSyncController(
		const bool &gplates_is_master,
	    GPlatesQtWidgets::ViewportWindow *viewport_window_ptr,
	    GPlatesPresentation::ViewState *view_state_ptr);

	virtual
	~ExternalSyncController();




	/**
	 * Enable everything, that is to say:
	 *   read and write to std io;
	 *   respond to view, time and file messages;
	 *   send view and time message.
	 */
	void
	enable_external_syncing();

	void
	disable_external_syncing();

	/**
	 * Switch on auto-sync of time signals.
	 */
	void
	enable_time_commands();

	void
	enable_view_commands();

	void
	enable_file_commands();

	void
	disable_time_commands();

	void
	disable_view_commands();

	void
	disable_file_commands();

	void
	start_external_process(
	    const QString &process);

	/**
	 * Continuously send and receive/process projection-centre and zoom signals from std in/out                                                              
	 */
	void
	auto_sync_view(
	    bool sync);

	/**
	 * Continuously send and receive/process time signals from std in/out                                                              
	 */
	void
	auto_sync_time(
	    bool sync);

	void
	start_thread();

    /**
     * Sync the time of the external app with the gplates time.
     */
	void
	sync_external_time();


    /**
     * Sync the view (orienation and zoom) of the external app to that of gplates.
     */
    void
	sync_external_view();

    /**
     * Sync the view (orienation and zoom) of gplates to that of the external app.
     */
	void
	sync_gplates_view();

    /**
     * Sync the time of gplates to that of the external app.
     */
	void
	sync_gplates_time();

	void
	connect_message_signals();

	void
	send_external_command(
		QString &command);

	signals:

	/**
	 * Emitted when the external process finishes. This can be
	 * used by the sync dialog to enable the "Launch" button, for example. 
	 */
	void
	process_finished();

    private slots:

	void
	handle_command_received(
		QString str);

	void
	send_external_time_command(
		double time);

	void
	send_external_camera_command(
		double lat, double lon);

	void
	send_external_orientation_command(
		GPlatesMaths::Rotation &rotation);

	void
	send_external_zoom_command(
		double zoom);

	void
	handle_process_finished(
		int exit_code,
		QProcess::ExitStatus exit_status);

	void
	handle_process_started();

	void
	handle_process_error(
		QProcess::ProcessError error);

	void
	read_process_output();

    private:


	/**
	 * This parses the command string and farms it out to the various
	 * other "process..." methods as appropriate.
	 */
	void
	process_external_command(
		const QString &command_string);

	void
	process_time_command(
		const QStringList &commands);

	void
	process_viewport_centre_command(
		const QStringList &commands);

	void
	process_orientation_command(
		const QStringList &commands);

	void
	process_zoom_command(
		const QStringList &commands);

	void
	process_gain_focus_command();

	void
	process_open_file_command(
		const QStringList &commands);



	double
	get_time();

	boost::optional<GPlatesMaths::LatLonPoint>
	get_projection_centre();

	double
	get_zoom();

	boost::optional<GPlatesMaths::Rotation>
	get_orientation();

	void
	set_time(
		const double &time);

	void
	set_projection_centre(
		const GPlatesMaths::LatLonPoint &llp);

	void
	set_zoom(
		const double &zoom);

	void
	set_orientation(
		const GPlatesMaths::Rotation &rotation);

	/**
	 * Thread for monitoring stdin.
	 */
	StdInThread *d_std_in_thread_ptr;

	/**
     * Process for launching external app.
	 */
	QProcess *d_process;

	/**
	 * Need the viewport window to access:
	 *   ReconstructionViewWidget (set camera viewpoint)
	 *   ViewState's ViewportZoom (set zoom)
	 *   AnimationController (set time)
	 */
	GPlatesQtWidgets::ViewportWindow *d_viewport_window_ptr;

	GPlatesGui::AnimationController *d_animation_controller_ptr;
	GPlatesQtWidgets::ReconstructionViewWidget *d_reconstruction_view_widget_ptr;
	GPlatesPresentation::ViewState *d_view_state_ptr;

	bool d_should_sync_view;
	bool d_should_sync_time;

	double d_most_recent_time;
	GPlatesMaths::LatLonPoint d_most_recent_llp;
	double d_most_recent_zoom;
	GPlatesMaths::Rotation d_most_recent_orientation;

	/**
	 * True if gplates launches the external program, and therefore controls synchronisation. 
	 *
	 * Our communication method is dependent on this mode - if gplates controls the external program, then 
	 * this is run as a QProcess, and we need to use process->write() etc.
	 * If GPlates is launched from the external program, then that takes care of the process, and GPlates just
	 * needs to read/write from std:in/out.
	 *
	 */
	const bool d_gplates_is_master;

	/**
	 * Whether or not we should send signals. 
	 *
	 * Normally this is true, but if we have just received a signal from the external program then we should disable
	 * the sending of signals until the external command has been acted on.  This should avoid any nasty
	 * loops we might get.
	 */
	 bool d_should_send_output;

    };

}

#endif // GPLATES_GUI_EXTERNALSYNCCONTROLLER_H
