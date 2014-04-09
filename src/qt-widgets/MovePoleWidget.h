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

#include <utility>
#include <boost/optional.hpp>
#include <QWidget>

#include "MovePoleWidgetUi.h"
#include "TaskPanelWidget.h"

#include "app-logic/ReconstructedFeatureGeometry.h"

#include "maths/PointOnSphere.h"


namespace GPlatesGui
{
	class FeatureFocus;
}

namespace GPlatesPresentation
{
	class ViewState;
}

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
				GPlatesPresentation::ViewState &view_state,
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
		set_focus();

		void
		handle_reconstruction();

		void
		react_enable_pole_check_box_changed();

		void
		react_latitude_spinbox_changed();

		void
		react_longitude_spinbox_changed();

		void
		react_north_pole_pushbutton_clicked();

		void
		react_stage_pole_pushbutton_clicked();

		void
		react_keep_stage_pole_constrained_checkbox_changed();

	private:

		GPlatesGui::FeatureFocus &d_feature_focus;

		boost::optional<GPlatesMaths::PointOnSphere> d_pole;


		void
		make_signal_slot_connections(
				GPlatesPresentation::ViewState &view_state);

		void
		update_stage_pole_moving_fixed_plate_ids();

		boost::optional<GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_to_const_type>
		get_focused_feature_geometry() const;

		boost::optional<
				std::pair<
						GPlatesModel::integer_plate_id_type/*moving*/,
						GPlatesModel::integer_plate_id_type/*fixed*/> >
		get_stage_pole_plate_pair(
				const GPlatesAppLogic::ReconstructedFeatureGeometry &rfg) const;

		boost::optional<GPlatesMaths::PointOnSphere>
		get_stage_pole_location() const;
	};
}

#endif // GPLATES_QT_WIDGETS_MOVEPOLEWIDGET_H
