/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2014 The University of Sydney, Australia
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

#ifndef GPLATES_QT_WIDGETS_MOVEPOLEWIDGET_H
#define GPLATES_QT_WIDGETS_MOVEPOLEWIDGET_H

#include <boost/optional.hpp>
#include <QWidget>

#include "MovePoleWidgetUi.h"
#include "TaskPanelWidget.h"

#include "maths/PointOnSphere.h"


namespace GPlatesQtWidgets
{
	class MovePoleWidget :
			public TaskPanelWidget, 
			protected Ui_MovePoleWidget
	{
		Q_OBJECT

	public:

		explicit
		MovePoleWidget(
				QWidget *parent_ = NULL);

		~MovePoleWidget();

		virtual
		void
		handle_activation()
		{  }

		/**
		 * Returns pole (if enabled).
		 */
		boost::optional<GPlatesMaths::PointOnSphere>
		get_pole() const
		{
			return d_pole;
		}

		/**
		 * Sets pole (also enables/disables pole).
		 */
		void
		set_pole(
				boost::optional<GPlatesMaths::PointOnSphere> pole = boost::none);

	Q_SIGNALS:
		// NOTE: all signals/slots should use namespace scope for all arguments
		//       otherwise differences between signals and slots will cause Qt
		//       to not be able to connect them at runtime.

		//! Emitted when the pole has changed (including enabled/disabled).
		void
		pole_changed(
				boost::optional<GPlatesMaths::PointOnSphere> pole);

	public Q_SLOTS:

		void
		activate();

		void
		deactivate();

	private Q_SLOTS:

		void
		react_enable_pole_check_box_changed();

		void
		react_latitude_spinbox_changed();

		void
		react_longitude_spinbox_changed();

		void
		react_north_pole_pushbutton_clicked();

	private:

		boost::optional<GPlatesMaths::PointOnSphere> d_pole;


		void
		make_signal_slot_connections();
	};
}

#endif // GPLATES_QT_WIDGETS_MOVEPOLEWIDGET_H
