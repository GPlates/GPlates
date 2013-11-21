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
 
#ifndef GPLATES_QTWIDGETS_CHANGEFEATURETYPEDIALOG_H
#define GPLATES_QTWIDGETS_CHANGEFEATURETYPEDIALOG_H

#include <vector>
#include <QDialog>
#include <QBoxLayout>
#include <QString>
#include <QTextEdit>
#include <QStringList>

#include "ChangeFeatureTypeDialogUi.h"

#include "model/FeatureHandle.h"


namespace GPlatesAppLogic
{
	class ApplicationState;
}

namespace GPlatesGui
{
	class FeatureFocus;
}

namespace GPlatesQtWidgets
{
	class ChangePropertyWidget;
	class ChooseFeatureTypeWidget;

	class ChangeFeatureTypeDialog : 
			public QDialog,
			protected Ui_ChangeFeatureTypeDialog 
	{
		Q_OBJECT
		
	public:

		explicit
		ChangeFeatureTypeDialog(
				GPlatesAppLogic::ApplicationState &application_state,
				GPlatesGui::FeatureFocus &feature_focus,
				QWidget *parent_ = NULL);

		/**
		 * Sets up the dialog to change the feature type for the given @a feature.
		 */
		void
		populate(
				const GPlatesModel::FeatureHandle::weak_ref &feature_ref);

	private Q_SLOTS:

		void
		handle_feature_type_changed(
				boost::optional<GPlatesModel::FeatureType> feature_type_opt);

		void
		change_feature_type();

	private:

		/**
		 * The InvalidPropertiesWidget shows a list of properties that are invalid for
		 * the new feature type with an explanatory message.
		 */
		class InvalidPropertiesWidget :
				public QWidget
		{
		public:

			InvalidPropertiesWidget(
					QWidget *parent_ = NULL);

			void
			populate(
					const QStringList &invalid_properties);

		private:

			QTextEdit *d_invalid_properties_textedit;
		};

		GPlatesAppLogic::ApplicationState &d_application_state;
		GPlatesGui::FeatureFocus &d_feature_focus;

		/**
		 * Allows the user to choose a new feature type.
		 */
		ChooseFeatureTypeWidget *d_new_feature_type_widget;

		/**
		 * The container holding all the ChangePropertyWidgets.
		 */
		QWidget *d_widget_container;

		/**
		 * The layout of d_widget_container.
		 */
		QBoxLayout *d_widget_container_layout;

		/**
		 * Displays invalid properties to the user.
		 */
		InvalidPropertiesWidget *d_invalid_properties_widget;

		/**
		 * A pool of ChangePropertyWidget instances, to save us from having to
		 * continuously destroy and create these objects.
		 */
		std::vector<ChangePropertyWidget *> d_change_property_widget_pool;

		/**
		 * The number of widgets active in d_change_property_widget_pool.
		 */
		unsigned int d_num_active_widgets;

		/**
		 * A handle to the feature that we're modifying.
		 */
		GPlatesModel::FeatureHandle::weak_ref d_feature_ref;
	};
}

#endif  // GPLATES_QTWIDGETS_CHANGEFEATURETYPEDIALOG_H
