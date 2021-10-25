/* $Id: ModifyGeometryWidget.h 10789 2011-01-19 08:58:33Z elau $ */

/**
 * \file Displays lat/lon points of geometry being modified by a canvas tool.
 *
 * $Revision: 10789 $
 * $Date: 2011-01-19 09:58:33 +0100 (on, 19 jan 2011) $
 *
 * Copyright (C) 2011 Geological Survey of Norway
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

#ifndef GPLATES_QTWIDGETS_SMALLCIRCLEWIDGET_H
#define GPLATES_QTWIDGETS_SMALLCIRCLEWIDGET_H

#include <QDebug>
#include <QWidget>

#include "maths/SmallCircle.h"
#include "SmallCircleWidgetUi.h"
#include "TaskPanelWidget.h"

namespace GPlatesAppLogic
{
	class ApplicationState;
}

namespace GPlatesCanvasTools
{
	class CreateSmallCircle;
}

namespace GPlatesMaths
{
	class LatLonPoint;
}

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesViewOperations
{
	class RenderedGeometryLayer;
}

namespace GPlatesQtWidgets
{
	class CreateSmallCircleDialog;

	class SmallCircleWidget :
			public TaskPanelWidget,
			protected Ui_SmallCircleWidget
	{
		Q_OBJECT

	public:

		typedef std::vector<GPlatesMaths::SmallCircle> small_circle_collection_type;

		explicit
		SmallCircleWidget(
				GPlatesPresentation::ViewState &view_state,
				QWidget *parent_ = NULL);

		void
		handle_activation();

		void
		set_centre(
				const GPlatesMaths::LatLonPoint &centre);

		small_circle_collection_type&
		small_circle_collection()
		{
			return d_small_circles;
		};

		void
		update_small_circle_layer();

		/**
		 * Update the centre part of the current_circles group box.
		 */
		void
		update_current_centre(
				const GPlatesMaths::PointOnSphere &current_centre);

		
		void
		update_current_radius(
				const GPlatesMaths::Real &radius_in_radians);

		/**
		 * Update the radii of the current_circles group box from small circles collection d_small_circles.
		 *
		 * An optional extra radius can be provided - so that we can add the radius of the current canvas circle.
		 */
		void
		update_radii(
				boost::optional<double> current_radius = boost::none);

		void
		update_circles(
				small_circle_collection_type &small_circle_collection);

	Q_SIGNALS:
		
		/**
		 * For triggering a reconstruction.
		 */
		void
		feature_created();


		void
		clear_geometries();


	private:

		void
		set_default_states();

		/**
		 * Override the QWidget's hideEvent so that we can
		 * close the associated (non-modal) CalculateSmallCircle dialog as well.
		 */
		virtual
		void
		hideEvent(
				QHideEvent *);

		void
		update_buttons();

		void
		update_current_circles();



	private Q_SLOTS:

		void
		handle_create_feature();

		void
		handle_clear();

		void
		handle_specify();


	private:

		GPlatesAppLogic::ApplicationState *d_application_state_ptr;
		CreateSmallCircleDialog *d_create_small_circle_dialog_ptr;
		GPlatesViewOperations::RenderedGeometryLayer *d_small_circle_layer;

		small_circle_collection_type d_small_circles;

	};
}

#endif // GPLATES_QTWIDGETS_SMALLCIRCLEWIDGET_H
