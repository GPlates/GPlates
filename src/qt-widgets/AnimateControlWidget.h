/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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
 
#ifndef GPLATES_QTWIDGETS_ANIMATECONTROLWIDGET_H
#define GPLATES_QTWIDGETS_ANIMATECONTROLWIDGET_H

#include <QWidget>
#include <QDockWidget>
#include "AnimateControlWidgetUi.h"


namespace GPlatesGui
{
	class AnimationController;
}


namespace GPlatesQtWidgets
{
	/**
	 * This widget resides inside a QDockWidget. The dock is usually hidden
	 * from view, but can pop up and sit in the top dock slot if the user
	 * starts playing an animation.
	 *
	 * It offers basic control over the animation.
	 */
	class AnimateControlWidget: 
			public QWidget,
			protected Ui_AnimateControlWidget
	{
		Q_OBJECT
		
	public:
	
		static
		QDockWidget *
		create_as_qdockwidget(
				GPlatesGui::AnimationController &animation_controller);
	
		explicit
		AnimateControlWidget(
				GPlatesGui::AnimationController &animation_controller,
				QWidget *parent_);

		virtual
		~AnimateControlWidget()
		{  }
	
	public Q_SLOTS:

		/**
		 * Sets whether you want a single button for play and pause
		 * (the default), or two separate buttons.
		 */
		void
		use_combined_play_pause_button(
				bool combined)
		{
			button_play->setHidden(combined);
			button_pause->setHidden(combined);
			button_play_or_pause->setVisible(combined);
		}

		/**
		 * Sets whether you want the << / >> buttons shown or hidden.
		 * Defaults to true.
		 */
		void
		show_step_buttons(
				bool show_);

	private Q_SLOTS:
		
		void
		handle_play_or_pause_clicked();

		void
		handle_seek_beginning_clicked();

		void
		set_current_time_from_slider(
				int slider_pos);

		void
		handle_view_time_changed(
				double new_time);

		void
		handle_start_time_changed(
				double new_time);

		void
		handle_end_time_changed(
				double new_time);

		void
		handle_animation_started();

		void
		handle_animation_paused();

	private:

		int
		ma_to_slider_units(
				const double &ma);

		double
		slider_units_to_ma(
				const int &slider_pos);

		void
		recalculate_slider();

		void
		update_button_states();
		
		/**
		 * This is the animation controller, which holds the state of any
		 * animation set up in the application.
		 * This allows us to control the same animation from both
		 * AnimateDialog and AnimateControlWidget.
		 */
		GPlatesGui::AnimationController *d_animation_controller_ptr;

	};
}

#endif
