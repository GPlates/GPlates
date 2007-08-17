/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2007 The University of Sydney, Australia
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
 
#ifndef GPLATES_GUI_ANIMATEDIALOG_H
#define GPLATES_GUI_ANIMATEDIALOG_H

#include <QDialog>
#include <QTimer>
#include "AnimateDialogUi.h"


namespace GPlatesGui
{
	class Document;


	class AnimateDialog: 
			public QDialog,
			protected Ui_AnimateDialog 
	{
		Q_OBJECT
		
	public:
		AnimateDialog(
				Document &viewport,
				QWidget *parent = NULL);

		virtual
		~AnimateDialog()
		{  }

		const double &
		viewport_time() const;
	public slots:
		void
		set_start_time_value_to_viewport_time()
		{
			widget_start_time->setValue(viewport_time());
		}

		void
		set_end_time_value_to_viewport_time()
		{
			widget_end_time->setValue(viewport_time());
		}

		void
		set_current_time_value_to_viewport_time()
		{
			widget_current_time->setValue(viewport_time());
		}

		void
		toggle_animation_playback_state();

		void
		rewind();

	signals:
		void
		current_time_changed(
				double new_value);

	private slots:
		void
		react_start_time_changed(
				double new_val);

		void
		react_end_time_changed(
				double new_val);

		void
		react_current_time_changed(
				double new_val);

		void
		react_animation_playback_step();

	private:
		/**
		 * This is the viewport which will be queried for the current viewport time
		 * whenever the user presses a "Use Viewport Time" button.
		 */
		Document *d_viewport_ptr;

		/**
		 * This QTimer instance triggers the frame updates during animation playback.
		 */
		QTimer d_timer;

		/**
		 * This is the increment applied to the current time in successive frames of the
		 * animation.
		 *
		 * This value is either greater than zero or less than zero.
		 *
		 * The user specifies the absolute value of this time increment in the "time
		 * increment" widget in the dialog.  The value in the "time increment" widget is
		 * constrained to be greater than zero.  The @a recalculate_increment function
		 * examines the value in the "time increment" dialog, and determines whether the
		 * value of this datum must be greater than zero or less than zero in order to
		 * successively increment the current-time from the start-time to the end-time.
		 */
		double d_time_increment;

		/**
		 * Modify the current time, if necessary, to ensure that it lies within the
		 * [closed, closed] range of the boundary times.
		 */
		void
		ensure_current_time_lies_within_bounds();

		/**
		 * Modify the boundary times, if necessary, to ensure that they contain the current
		 * time.
		 */
		void
		ensure_bounds_contain_current_time();

		void
		recalculate_slider();

		void
		start_animation_playback();

		void
		stop_animation_playback();

		/**
		 * Set the value of the member datum @a d_time_increment.
		 *
		 * This function examines the value in the "time increment" dialog, and determines
		 * whether the value of this datum must be greater than zero or less than zero in
		 * order to successively increment the current-time from the start-time to the
		 * end-time.
		 */
		void
		recalculate_increment();
	};
}

#endif
