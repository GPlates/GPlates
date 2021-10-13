/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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
 
#ifndef GPLATES_QTWIDGETS_CHANGEGEOMETRYPROPERTYWIDGET_H
#define GPLATES_QTWIDGETS_CHANGEGEOMETRYPROPERTYWIDGET_H

#include <QWidget>
#include <QString>

#include "ChangeGeometryPropertyWidgetUi.h"

#include "model/FeatureHandle.h"
#include "model/FeatureType.h"


namespace GPlatesGui
{
	class FeatureFocus;
}


namespace GPlatesQtWidgets
{
	class ChooseGeometryPropertyWidget;

	/**
	 * The ChangeGeometryPropertyWidget is a helper widget for the
	 * ChangeFeatureTypeDialog; for each problematic geometry property detected by
	 * the ChangeFeatureTypeDialog, it will spawn one of these widgets, which is
	 * responsible for presenting the user with a choice of alternative geometry
	 * properties suitable for the new feature type.
	 */
	class ChangeGeometryPropertyWidget : 
			public QWidget,
			protected Ui_ChangeGeometryPropertyWidget
	{
		Q_OBJECT
		
	public:

		explicit
		ChangeGeometryPropertyWidget(
				const GPlatesGui::FeatureFocus &feature_focus,
				QWidget *parent_ = NULL);

		/**
		 * Causes the widget to present to the user a choice of alternative geometry
		 * properties suitable for the @a new_feature_type chosen for the given
		 * @a geometry_property of a particular @a feature_ref.
		 */
		void
		populate(
				const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
				const GPlatesModel::FeatureHandle::iterator &geometry_property,
				const GPlatesModel::FeatureType &new_feature_type);

		/**
		 * Change the geometry property to the user's choice, if the user has elected
		 * to change the geometry property.
		 *
		 * If the currently focused geometry is reassigned to a new property, this
		 * method sets @a new_focused_geometry_property to the new property.
		 */
		void
		process(
				GPlatesModel::FeatureHandle::iterator &new_focused_geometry_property);

	private slots:

		void
		handle_checkbox_state_changed(
				int state);

	private:

		const GPlatesGui::FeatureFocus &d_feature_focus;

		ChooseGeometryPropertyWidget *d_geometry_destinations_widget;
		QString d_default_explanatory_text;

		GPlatesModel::FeatureHandle::weak_ref d_feature_ref;
		GPlatesModel::FeatureHandle::iterator d_geometric_property;
	};
}

#endif  // GPLATES_QTWIDGETS_CHANGEGEOMETRYPROPERTYWIDGET_H
