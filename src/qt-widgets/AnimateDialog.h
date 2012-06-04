/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2007, 2008, 2009 The University of Sydney, Australia
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
 
#ifndef GPLATES_QTWIDGETS_ANIMATEDIALOG_H
#define GPLATES_QTWIDGETS_ANIMATEDIALOG_H

#include <QDialog>

#include "AnimateDialogUi.h"

#include "GPlatesDialog.h"


namespace GPlatesGui
{
	class AnimationController;
}


namespace GPlatesQtWidgets
{
	class AnimateDialog: 
			public GPlatesDialog,
			protected Ui_AnimateDialog 
	{
		Q_OBJECT
		
	public:
		explicit
		AnimateDialog(
				GPlatesGui::AnimationController &animation_controller,
				QWidget *parent_ = NULL);

		virtual
		~AnimateDialog()
		{  }

		const double &
		view_time() const;

	public slots:
		void
		set_start_time_value_to_view_time();

		void
		set_end_time_value_to_view_time();

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
		react_start_time_spinbox_changed(
				double new_val);

		void
		react_end_time_spinbox_changed(
				double new_val);

		void
		react_time_increment_spinbox_changed(
				double new_val);

		void
		react_current_time_spinbox_changed(
				double new_val);

		void
		handle_start_time_changed(
				double new_val);

		void
		handle_end_time_changed(
				double new_val);

		void
		handle_time_increment_changed(
				double new_val);

		void
		handle_current_time_changed(
				double new_val);

		/**
		 * (Re)sets checkboxes according to animation controller state.
		 */
		void
		handle_options_changed();

		void
		handle_animation_started();

		void
		handle_animation_paused();

		void
		set_current_time_from_slider(
				int slider_pos);

	private:
		/**
		 * This is the animation controller, which holds the state of any
		 * animation set up in the application.
		 * This allows us to control the same animation from both
		 * AnimateDialog and AnimateControlWidget.
		 */
		GPlatesGui::AnimationController *d_animation_controller_ptr;

		/**
		 * Updates button label & icon.
		 */
		void
		set_start_button_state(
				bool animation_is_playing);

		int
		ma_to_slider_units(
				const double &ma);

		double
		slider_units_to_ma(
				const int &slider_pos);

		void
		recalculate_slider();

	};
}

#endif
