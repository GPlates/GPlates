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


namespace GPlatesQtWidgets
{
	class ViewportWindow;

	class QueryFeaturePropertiesWidget: 
			public QWidget,
			protected Ui_QueryFeaturePropertiesWidget 
	{
		Q_OBJECT
		
	public:
		explicit
		QueryFeaturePropertiesWidget(
				const GPlatesQtWidgets::ViewportWindow &view_state_,
				GPlatesGui::FeatureFocus &feature_focus,
				QWidget *parent_ = NULL);

		virtual
		~QueryFeaturePropertiesWidget()
		{  }

		const GPlatesQtWidgets::ViewportWindow &
		view_state() const
		{
			return *d_view_state_ptr;
		}

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

	public slots:

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
				GPlatesModel::ReconstructionGeometry::maybe_null_ptr_type focused_rg);
		
	signals:

	private:
		/**
		 * This is the view state which is used to obtain the reconstruction root.
		 */
		const GPlatesQtWidgets::ViewportWindow *d_view_state_ptr;

		/**
		 * This is the feature we are displaying. Make sure to check this ref is_valid()!
		 */
		GPlatesModel::FeatureHandle::weak_ref d_feature_ref;

		/**
		 * The @a ReconstructionGeometry associated with the feature that is in focus.
		 */
		GPlatesModel::ReconstructionGeometry::maybe_null_ptr_type d_focused_rg;
	};
}

#endif  // GPLATES_QTWIDGETS_QUERYFEATUREPROPERTIESWIDGET_H
