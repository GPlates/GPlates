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
 
#ifndef GPLATES_QTWIDGETS_TIMECONTROLWIDGET_H
#define GPLATES_QTWIDGETS_TIMECONTROLWIDGET_H

#include <QWidget>
#include <QDockWidget>
#include "ui_TimeControlWidgetUi.h"


namespace GPlatesGui
{
	class AnimationController;
}


namespace GPlatesQtWidgets
{
	/**
	 * This widget resides inside the AwesomeBar at the top of the ReconstructionViewWidget.
	 *
	 * It offers basic control over the current reconstruction time.
	 */
	class TimeControlWidget: 
			public QWidget,
			protected Ui_TimeControlWidget
	{
		Q_OBJECT
		
	public:
	
		static
		QDockWidget *
		create_as_qdockwidget(
				GPlatesGui::AnimationController &animation_controller);
	
		explicit
		TimeControlWidget(
				GPlatesGui::AnimationController &animation_controller,
				QWidget *parent_);

		virtual
		~TimeControlWidget()
		{  }

	Q_SIGNALS:

		/**
		 * Emitted when the user has entered a new time value in the spinbox.
		 * The ReconstructionViewWidget listens for this signal so it can give
		 * the globe keyboard focus again after editing.
		 */
		void
		editing_finished();
	
	public Q_SLOTS:
		
		/**
		 * Focuses the spinbox and highlights text, ready to be replaced.
		 */
		void
		activate_time_spinbox();

		/**
		 * Sets whether you want the << / >> buttons shown or hidden.
		 * Defaults to false.
		 */
		void
		show_step_buttons(
				bool show_);

		/**
		 * Sets whether you want the "Time:" label shown or hidden.
		 * Defaults to true.
		 */
		void
		show_label(
				bool show_);

	private Q_SLOTS:
		
		void
		handle_time_spinbox_editing_finished();

		void
		handle_view_time_changed(
				double new_time);

	private:

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
