/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2007, 2008 The University of Sydney, Australia
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
 
#ifndef GPLATES_QTWIDGETS_QUERYFEATUREPROPERTIESWIDGET_H
#define GPLATES_QTWIDGETS_QUERYFEATUREPROPERTIESWIDGET_H

#include <QWidget>

#include "QueryFeaturePropertiesWidgetUi.h"

#include "gui/FeatureFocus.h"
#include "model/FeatureHandle.h"


namespace GPlatesAppLogic
{
	class ApplicationState;
}

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{
	class QueryFeaturePropertiesWidget: 
			public QWidget,
			protected Ui_QueryFeaturePropertiesWidget 
	{
		Q_OBJECT
		
	public:
		explicit
		QueryFeaturePropertiesWidget(
				GPlatesPresentation::ViewState &view_state_,
				QWidget *parent_ = NULL);

		virtual
		~QueryFeaturePropertiesWidget()
		{  }

		/**
		 * The parameter is a QString to enable us to pass the string "indeterminate".
		 */
		void
		set_euler_pole(
				const QString &point_position);

		void
		set_angle(
				const double &angle);

		void
		set_plate_id(
				unsigned long plate_id);

		void
		set_root_plate_id(
				unsigned long plate_id);

		void
		set_reconstruction_time(
				const double &recon_time);

		QTreeWidget &
		property_tree() const;

		void
		load_data_if_necessary();
		
	public Q_SLOTS:

		/**
		 * Updates the dialog to redisplay the geometry of the current Feature.
		 *
		 * Called when the current reconstruction time changes.
		 */
		void
		refresh_display();

		/**
		 * Updates the query widget to display properties of the given feature.
		 * Called by FeaturePropertiesDialog after the weak_ref is checked for validity.
		 */
		void
		display_feature(
				GPlatesModel::FeatureHandle::weak_ref feature_ref,
				GPlatesAppLogic::ReconstructionGeometry::maybe_null_ptr_to_const_type focused_rg);
		
	Q_SIGNALS:

	private:
		/**
		 * This is the view state which is used to obtain the reconstruction root.
		 */
		GPlatesAppLogic::ApplicationState *d_application_state_ptr;

		/**
		 * This is the feature we are displaying. Make sure to check this ref is_valid()!
		 */
		GPlatesModel::FeatureHandle::weak_ref d_feature_ref;

		/**
		 * The @a ReconstructionGeometry associated with the feature that is in focus.
		 */
		GPlatesAppLogic::ReconstructionGeometry::maybe_null_ptr_to_const_type d_focused_rg;

		bool d_need_load_data;
	};
}

#endif  // GPLATES_QTWIDGETS_QUERYFEATUREPROPERTIESWIDGET_H
