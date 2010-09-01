/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008, 2010 The University of Sydney, Australia
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

#include <QString>
#include <QStringList>
#include <QValidator>

#include "FeaturePropertiesDialog.h"

#include "model/FeatureType.h"
#include "model/FeatureId.h"
#include "model/FeatureRevision.h"
#include "model/GPGIMInfo.h"

#include "presentation/ViewState.h"

#include "utils/UnicodeStringUtils.h"


namespace
{
	class FeatureTypeValidator :
			public QValidator
	{
	public:

		FeatureTypeValidator(
				QObject *parent_ = NULL) :
			QValidator(parent_)
		{  }

		virtual
		QValidator::State
		validate(
				QString &input,
				int &pos) const
		{
			QStringList tokens = input.split(":");
			if (tokens.count() > 2)
			{
				return QValidator::Invalid;
			}

			if (tokens.count() == 2)
			{
				// We'll only let the user enter gpml, gml and xsi namespaces.
				const QString &namespace_alias = tokens.at(0);
				if (namespace_alias == "gpml" ||
					namespace_alias == "gml" ||
					namespace_alias == "xsi")
				{
					if (tokens.at(1).length() > 0)
					{
						return QValidator::Acceptable;
					}
					else
					{
						return QValidator::Intermediate;
					}
				}
				else
				{
					return QValidator::Intermediate;
				}
			}
			else
			{
				return QValidator::Intermediate;
			}
		}
	};
}


GPlatesQtWidgets::FeaturePropertiesDialog::FeaturePropertiesDialog(
		GPlatesPresentation::ViewState &view_state_,
		QWidget *parent_):
	QDialog(parent_, Qt::Window),
	d_query_feature_properties_widget(new GPlatesQtWidgets::QueryFeaturePropertiesWidget(
			view_state_, this)),
	d_edit_feature_properties_widget(new GPlatesQtWidgets::EditFeaturePropertiesWidget(
			view_state_, this)),
	d_view_feature_geometries_widget(new GPlatesQtWidgets::ViewFeatureGeometriesWidget(
			view_state_, this))
{
	setupUi(this);
	
	// Set up the tab widget. Note we have to delete the 'dummy' tab set up by the Designer.
	tabwidget_query_edit->clear();
	tabwidget_query_edit->addTab(d_query_feature_properties_widget,
			QIcon(":/gnome_edit_find_16.png"), tr("&Query Properties"));
	tabwidget_query_edit->addTab(d_edit_feature_properties_widget,
			QIcon(":/gnome_gtk_edit_16.png"), tr("&Edit Properties"));
	tabwidget_query_edit->addTab(d_view_feature_geometries_widget,
			QIcon(":/gnome_stock_edit_points_16.png"), tr("View &Coordinates"));
	tabwidget_query_edit->setCurrentIndex(0);

	// Handle tab changes.
	QObject::connect(tabwidget_query_edit, SIGNAL(currentChanged(int)),
			this, SLOT(handle_tab_change(int)));
	
	// Handle focus changes.
	QObject::connect(&view_state_.get_feature_focus(), 
			SIGNAL(focus_changed(GPlatesGui::FeatureFocus &)),
			this,
			SLOT(display_feature(GPlatesGui::FeatureFocus &)));
	QObject::connect(&view_state_.get_feature_focus(),
			SIGNAL(focused_feature_modified(GPlatesGui::FeatureFocus &)),
			this,
			SLOT(display_feature(GPlatesGui::FeatureFocus &)));

	// Handle feature type changes.
	QObject::connect(
			combobox_feature_type->lineEdit(),
			SIGNAL(editingFinished()),
			this,
			SLOT(handle_feature_type_changed()));

	populate_feature_types();
	combobox_feature_type->setValidator(new FeatureTypeValidator(this));
	
	// Refresh display - since the feature ref is invalid at this point,
	// the dialog should lock everything down that might otherwise cause problems.
	refresh_display();
}


void
GPlatesQtWidgets::FeaturePropertiesDialog::display_feature(
		GPlatesGui::FeatureFocus &feature_focus)
{
	d_feature_ref = feature_focus.focused_feature();
	d_focused_rg = feature_focus.associated_reconstruction_geometry();

	refresh_display();
}


void
GPlatesQtWidgets::FeaturePropertiesDialog::refresh_display()
{
	if ( ! d_feature_ref.is_valid()) {
		// Disable everything except the Close button.
		combobox_feature_type->setEnabled(false);
		tabwidget_query_edit->setEnabled(false);
		combobox_feature_type->lineEdit()->clear();
		return;
	}

	//PROFILE_FUNC();

	// Ensure everything is enabled.
	combobox_feature_type->setEnabled(true);
	tabwidget_query_edit->setEnabled(true);
	
	// Update our text fields at the top.
	combobox_feature_type->lineEdit()->setText(
			GPlatesUtils::make_qstring_from_icu_string(
				d_feature_ref->feature_type().build_aliased_name()));
	
	// Update our tabbed sub-widgets.
	d_query_feature_properties_widget->display_feature(d_feature_ref, d_focused_rg);
	d_edit_feature_properties_widget->edit_feature(d_feature_ref);
	d_view_feature_geometries_widget->edit_feature(d_feature_ref, d_focused_rg);
}

		
void
GPlatesQtWidgets::FeaturePropertiesDialog::choose_query_widget_and_open()
{
	tabwidget_query_edit->setCurrentWidget(d_query_feature_properties_widget);
	d_query_feature_properties_widget->load_data_if_necessary();
	pop_up();
}

void
GPlatesQtWidgets::FeaturePropertiesDialog::choose_edit_widget_and_open()
{
	tabwidget_query_edit->setCurrentWidget(d_edit_feature_properties_widget);
	pop_up();
}

void
GPlatesQtWidgets::FeaturePropertiesDialog::choose_geometries_widget_and_open()
{
	tabwidget_query_edit->setCurrentWidget(d_view_feature_geometries_widget);
	pop_up();
}


void
GPlatesQtWidgets::FeaturePropertiesDialog::pop_up()
{
	show();
	// In most cases, 'show()' is sufficient. However, selecting the menu entry
	// a second time, when the dialog is still open, should make the dialog 'active'
	// and return keyboard focus to it.
	activateWindow();
	// On platforms which do not keep dialogs on top of their parent, a call to
	// raise() may also be necessary to properly 're-pop-up' the dialog.
	raise();
}


void
GPlatesQtWidgets::FeaturePropertiesDialog::populate_feature_types()
{
	for (int i = 0; i != 2; ++i)
	{
		typedef GPlatesModel::GPGIMInfo::feature_set_type feature_set_type;
		const feature_set_type &list = GPlatesModel::GPGIMInfo::get_feature_set(
				static_cast<bool>(i)); // first time, non-topological; second time, topological.

		for (feature_set_type::const_iterator iter = list.begin();
				iter != list.end(); ++iter)
		{
			combobox_feature_type->addItem(
					GPlatesUtils::make_qstring_from_icu_string(
						iter->build_aliased_name()));
		}
	}
}


void
GPlatesQtWidgets::FeaturePropertiesDialog::setVisible(
		bool visible)
{
	if ( ! visible) {
		// We are closing. Ensure things are left tidy.
		d_edit_feature_properties_widget->commit_edit_widget_data();
		d_edit_feature_properties_widget->clean_up();
	}
	QDialog::setVisible(visible);
}



void
GPlatesQtWidgets::FeaturePropertiesDialog::handle_tab_change(
		int index)
{
	const QIcon icon = tabwidget_query_edit->tabIcon(index);
	if(index == 0)
	{
		d_query_feature_properties_widget->load_data_if_necessary();
	}
	setWindowIcon(icon);
}


void
GPlatesQtWidgets::FeaturePropertiesDialog::handle_feature_type_changed()
{
	if (d_feature_ref.is_valid())
	{
		try
		{
			GPlatesUtils::Parse<GPlatesModel::FeatureType> parse;
			const QString &feature_type = combobox_feature_type->currentText();
			d_feature_ref->set_feature_type(parse(feature_type));
		}
		catch (const GPlatesUtils::ParseError &)
		{
			// ignore
		}
	}
}

