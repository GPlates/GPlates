/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#include <QHeaderView>

#include "ConfigGuiUtils.h"

#include "utils/ConfigInterface.h"
#include "gui/ConfigModel.h"
#include "gui/ConfigValueDelegate.h"

#include "qt-widgets/PreferencesDialog.h"

GPlatesQtWidgets::ConfigTableView *
GPlatesGui::ConfigGuiUtils::link_config_interface_to_table(
		GPlatesUtils::ConfigInterface &config,
		bool use_icons,
		QWidget *parent)
{
	// We allocate the memory for this new table widget, and give it the parent supplied
	// by the caller so that Qt will handle cleanup of said memory.
	GPlatesQtWidgets::ConfigTableView *tableview_ptr = new GPlatesQtWidgets::ConfigTableView(parent);
		
	// We also create a ConfigModel to act as the intermediary between ConfigBundle and
	// the table, and parent that to the table view widget so that it also gets cleaned
	// up when appropriate.
	ConfigModel *config_model_ptr = new ConfigModel(config, use_icons, tableview_ptr);
	
	// Tell the table to use the model we created.
	tableview_ptr->setModel(config_model_ptr);
	
	// Set some sensible defaults for the QTableView.
	tableview_ptr->verticalHeader()->hide();
	tableview_ptr->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
	tableview_ptr->horizontalHeader()->setStretchLastSection(true);
	tableview_ptr->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
	tableview_ptr->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

	// Install our custom ItemDelegate to let users set values normally,
	// but also include a "reset" button.
	ConfigValueDelegate *delegate = new ConfigValueDelegate(tableview_ptr);
	tableview_ptr->setItemDelegate(delegate);

	return tableview_ptr;
}


void
GPlatesGui::ConfigGuiUtils::link_widget_to_preference(
		QLineEdit *widget,
		GPlatesUtils::ConfigInterface &config,
		const QString &key,
		QAbstractButton *reset_button)
{
	ConfigGuiUtils::ConfigWidgetAdapter *adapter = new ConfigGuiUtils::ConfigWidgetAdapter(
			widget, config, key);
	// When the config key changes, update the widget.
	QObject::connect(adapter, SIGNAL(value_changed(const QString &)), widget, SLOT(setText(const QString &)));
	// When Enter is pressed or the QLineEdit loses input focus, update the key.
	// Wishlist functionality: A delayed commit using a QTimer linked to that widget
	//                         - see the LogDialog's filter box to see what I mean.
	//                         You could implement it as a tiny helper adapter QObject child.
	QObject::connect(widget, SIGNAL(editingFinished()), adapter, SLOT(handle_widget_editing_finished()));
	
	// Optional reset button.
	if (reset_button) {
		QObject::connect(reset_button, SIGNAL(clicked()), adapter, SLOT(handle_reset_clicked()));
	}
	
	// Do a one-off fake update so widget has correct value in it.
	adapter->handle_key_value_updated(key);
}


void
GPlatesGui::ConfigGuiUtils::link_widget_to_preference(
		QCheckBox *widget,
		GPlatesUtils::ConfigInterface &config,
		const QString &key,
		QAbstractButton *reset_button)
{
	ConfigGuiUtils::ConfigWidgetAdapter *adapter = new ConfigGuiUtils::ConfigWidgetAdapter(
			widget, config, key);
	// When the config key changes, update the widget.
	QObject::connect(adapter, SIGNAL(value_changed(bool)), widget, SLOT(setChecked(bool)));
	// When the widget changes, update the key.
	QObject::connect(widget, SIGNAL(clicked(bool)), adapter, SLOT(handle_widget_value_updated(bool)));

	// Optional reset button.
	if (reset_button) {
		QObject::connect(reset_button, SIGNAL(clicked()), adapter, SLOT(handle_reset_clicked()));
	}
	
	// Do a one-off fake update so widget has correct value in it.
	adapter->handle_key_value_updated(key);
}


void
GPlatesGui::ConfigGuiUtils::link_widget_to_preference(
		QSpinBox *widget,
		GPlatesUtils::ConfigInterface &config,
		const QString &key,
		QAbstractButton *reset_button)
{
	ConfigGuiUtils::ConfigWidgetAdapter *adapter = new ConfigGuiUtils::ConfigWidgetAdapter(
			widget, config, key);
	// When the config key changes, update the widget.
	QObject::connect(adapter, SIGNAL(value_changed(int)), widget, SLOT(setValue(int)));
	// When the widget changes, update the key.
	QObject::connect(widget, SIGNAL(valueChanged(int)), adapter, SLOT(handle_widget_value_updated(int)));

	// Optional reset button.
	if (reset_button) {
		QObject::connect(reset_button, SIGNAL(clicked()), adapter, SLOT(handle_reset_clicked()));
	}
	
	// Do a one-off fake update so widget has correct value in it.
	adapter->handle_key_value_updated(key);
}


void
GPlatesGui::ConfigGuiUtils::link_widget_to_preference(
		QDoubleSpinBox *widget,
		GPlatesUtils::ConfigInterface &config,
		const QString &key,
		QAbstractButton *reset_button)
{
	ConfigGuiUtils::ConfigWidgetAdapter *adapter = new ConfigGuiUtils::ConfigWidgetAdapter(
			widget, config, key);
	// When the config key changes, update the widget.
	QObject::connect(adapter, SIGNAL(value_changed(double)), widget, SLOT(setValue(double)));
	// When the widget changes, update the key.
	QObject::connect(widget, SIGNAL(valueChanged(double)), adapter, SLOT(handle_widget_value_updated(double)));

	// Optional reset button.
	if (reset_button) {
		QObject::connect(reset_button, SIGNAL(clicked()), adapter, SLOT(handle_reset_clicked()));
	}
	
	// Do a one-off fake update so widget has correct value in it.
	adapter->handle_key_value_updated(key);
}




GPlatesGui::ConfigGuiUtils::ConfigWidgetAdapter::ConfigWidgetAdapter(
		QWidget *widget,
		GPlatesUtils::ConfigInterface &config,
		const QString &key):
	QObject(widget),
	d_widget_ptr(widget),
	d_config(config),
	d_key(key)
{
	connect(&config, SIGNAL(key_value_updated(QString)), this, SLOT(handle_key_value_updated(QString)));
}


void
GPlatesGui::ConfigGuiUtils::ConfigWidgetAdapter::handle_key_value_updated(
		QString key)
{
	// Early exit if it's not the key we're following.
	if (key != d_key) {
		return;
	}
	
	// Otherwise re-emit signals that are more useful to the various widgets.
	QVariant value = d_config.get_value(key);
	
	emit value_changed(value.toString());
	emit value_changed(value.toBool());
	emit value_changed(value.toInt());
	emit value_changed(value.toDouble());
}


void
GPlatesGui::ConfigGuiUtils::ConfigWidgetAdapter::handle_widget_value_updated(
		QString value)
{
	// With the LineEdit, CheckBox, SpinBoxes etc. we don't really have to convert or extract the value specially. Just set the key.
	// We do want to provide these specific overloads on type, though, because this is a slot and we want the
	// types of sender and receiver to match.
	d_config.set_value(d_key, QVariant(value));
}

void
GPlatesGui::ConfigGuiUtils::ConfigWidgetAdapter::handle_widget_value_updated(
		bool value)
{
	// With the LineEdit, CheckBox, SpinBoxes etc. we don't really have to convert or extract the value specially. Just set the key.
	// We do want to provide these specific overloads on type, though, because this is a slot and we want the
	// types of sender and receiver to match.
	d_config.set_value(d_key, QVariant(value));
}

void
GPlatesGui::ConfigGuiUtils::ConfigWidgetAdapter::handle_widget_value_updated(
		int value)
{
	// With the LineEdit, CheckBox, SpinBoxes etc. we don't really have to convert or extract the value specially. Just set the key.
	// We do want to provide these specific overloads on type, though, because this is a slot and we want the
	// types of sender and receiver to match.
	d_config.set_value(d_key, QVariant(value));
}

void
GPlatesGui::ConfigGuiUtils::ConfigWidgetAdapter::handle_widget_value_updated(
		double value)
{
	// With the LineEdit, CheckBox, SpinBoxes etc. we don't really have to convert or extract the value specially. Just set the key.
	// We do want to provide these specific overloads on type, though, because this is a slot and we want the
	// types of sender and receiver to match.
	d_config.set_value(d_key, QVariant(value));
}


void
GPlatesGui::ConfigGuiUtils::ConfigWidgetAdapter::handle_widget_editing_finished()
{
	// d_widget_ptr is a guarded QPointer that knows when the object is gone.
	if ( ! d_widget_ptr) {
		return;
	}
	
	// Obtain the correct new value from the widget.
	if (QLineEdit *lineedit = qobject_cast<QLineEdit *>(d_widget_ptr)) {
		handle_widget_value_updated(lineedit->text());
		
	} else {
		qWarning("ConfigGuiUtils::ConfigWidgetAdapter::handle_widget_editing_finished() : Used on a widget type that is not supported.");
	}
}


void
GPlatesGui::ConfigGuiUtils::ConfigWidgetAdapter::handle_reset_clicked()
{
	d_config.clear_value(d_key);
}



