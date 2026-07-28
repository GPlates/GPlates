/*
 * Copyright (C) 2026 The GPlates development team
 *
 * This file is part of GPlates.
 *
 * GPlates is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 */

#include <algorithm>
#include <boost/foreach.hpp>

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QVBoxLayout>

#include "PreferencesPaneActiveFeatureTypes.h"

#include "FeatureTypeDisplayPreferences.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/UserPreferences.h"

#include "model/Gpgim.h"
#include "model/QualifiedXmlName.h"


GPlatesQtWidgets::PreferencesPaneActiveFeatureTypes::PreferencesPaneActiveFeatureTypes(
		GPlatesAppLogic::ApplicationState &app_state,
		QWidget *parent_) :
	QWidget(parent_),
	d_preferences(app_state.get_user_preferences()),
	d_filter_line_edit(new QLineEdit(this)),
	d_feature_type_list(new QListWidget(this)),
	d_summary_label(new QLabel(this)),
	d_populating(false)
{
	QVBoxLayout *main_layout = new QVBoxLayout(this);

	QLabel *heading_label = new QLabel(tr("<b>Active Feature Types</b>"), this);
	main_layout->addWidget(heading_label);

	QLabel *description_label = new QLabel(
			tr("Choose which feature types are offered when creating or changing a feature type. "
				"This changes only the displayed choices; it does not modify existing feature data."),
			this);
	description_label->setWordWrap(true);
	main_layout->addWidget(description_label);

	d_filter_line_edit->setPlaceholderText(tr("Filter feature types..."));
	d_filter_line_edit->setClearButtonEnabled(true);
	main_layout->addWidget(d_filter_line_edit);

	d_feature_type_list->setAlternatingRowColors(true);
	main_layout->addWidget(d_feature_type_list, 1);

	QHBoxLayout *controls_layout = new QHBoxLayout;
	QPushButton *show_all_button = new QPushButton(tr("Show All"), this);
	QPushButton *hide_all_button = new QPushButton(tr("Hide All"), this);
	controls_layout->addWidget(show_all_button);
	controls_layout->addWidget(hide_all_button);
	controls_layout->addStretch();
	controls_layout->addWidget(d_summary_label);
	main_layout->addLayout(controls_layout);

	populate_feature_types();

	QObject::connect(
			d_feature_type_list,
			SIGNAL(itemChanged(QListWidgetItem *)),
			this,
			SLOT(handle_item_changed(QListWidgetItem *)));
	QObject::connect(
			d_filter_line_edit,
			SIGNAL(textChanged(QString)),
			this,
			SLOT(handle_filter_text_changed(QString)));
	QObject::connect(show_all_button, SIGNAL(clicked()), this, SLOT(show_all_feature_types()));
	QObject::connect(hide_all_button, SIGNAL(clicked()), this, SLOT(hide_all_feature_types()));
}


void
GPlatesQtWidgets::PreferencesPaneActiveFeatureTypes::populate_feature_types()
{
	d_populating = true;
	d_feature_type_list->clear();

	const QStringList hidden_feature_types = d_preferences.get_value(
			FeatureTypeDisplayPreferences::hidden_feature_types_key()).toStringList();

	GPlatesModel::Gpgim::feature_type_seq_type feature_types =
			GPlatesModel::Gpgim::instance().get_concrete_feature_types();
	std::stable_sort(feature_types.begin(), feature_types.end());

	BOOST_FOREACH(const GPlatesModel::FeatureType &feature_type, feature_types)
	{
		const QString qualified_name =
				GPlatesModel::convert_qualified_xml_name_to_qstring(feature_type);

		QListWidgetItem *item = new QListWidgetItem(feature_type.get_name().qstring());
		item->setData(Qt::UserRole, qualified_name);
		item->setToolTip(qualified_name);
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState(hidden_feature_types.contains(qualified_name)
				? Qt::Unchecked
				: Qt::Checked);
		d_feature_type_list->addItem(item);
	}

	d_populating = false;
	update_summary();
}


void
GPlatesQtWidgets::PreferencesPaneActiveFeatureTypes::handle_item_changed(
		QListWidgetItem *)
{
	if (d_populating)
	{
		return;
	}

	save_hidden_feature_types();
	update_summary();
}


void
GPlatesQtWidgets::PreferencesPaneActiveFeatureTypes::handle_filter_text_changed(
		const QString &text)
{
	for (int row = 0; row < d_feature_type_list->count(); ++row)
	{
		QListWidgetItem *item = d_feature_type_list->item(row);
		item->setHidden(!item->text().contains(text, Qt::CaseInsensitive) &&
				!item->data(Qt::UserRole).toString().contains(text, Qt::CaseInsensitive));
	}
}


void
GPlatesQtWidgets::PreferencesPaneActiveFeatureTypes::show_all_feature_types()
{
	set_all_feature_types_checked(true);
}


void
GPlatesQtWidgets::PreferencesPaneActiveFeatureTypes::hide_all_feature_types()
{
	set_all_feature_types_checked(false);
}


void
GPlatesQtWidgets::PreferencesPaneActiveFeatureTypes::set_all_feature_types_checked(
		bool checked)
{
	d_populating = true;
	for (int row = 0; row < d_feature_type_list->count(); ++row)
	{
		d_feature_type_list->item(row)->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
	}
	d_populating = false;

	save_hidden_feature_types();
	update_summary();
}


void
GPlatesQtWidgets::PreferencesPaneActiveFeatureTypes::save_hidden_feature_types()
{
	QStringList hidden_feature_types;
	for (int row = 0; row < d_feature_type_list->count(); ++row)
	{
		QListWidgetItem *item = d_feature_type_list->item(row);
		if (item->checkState() != Qt::Checked)
		{
			hidden_feature_types.append(item->data(Qt::UserRole).toString());
		}
	}

	d_preferences.set_value(
			FeatureTypeDisplayPreferences::hidden_feature_types_key(),
			hidden_feature_types);
}


void
GPlatesQtWidgets::PreferencesPaneActiveFeatureTypes::update_summary()
{
	int visible_feature_type_count = 0;
	for (int row = 0; row < d_feature_type_list->count(); ++row)
	{
		if (d_feature_type_list->item(row)->checkState() == Qt::Checked)
		{
			++visible_feature_type_count;
		}
	}

	d_summary_label->setText(
			tr("%1 of %2 shown")
					.arg(visible_feature_type_count)
					.arg(d_feature_type_list->count()));
}
