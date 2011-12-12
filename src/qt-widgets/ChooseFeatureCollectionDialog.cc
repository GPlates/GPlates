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

#include "ChooseFeatureCollectionDialog.h"

#include "ChooseFeatureCollectionWidget.h"
#include "QtWidgetUtils.h"


GPlatesQtWidgets::ChooseFeatureCollectionDialog::ChooseFeatureCollectionDialog(
		GPlatesAppLogic::FeatureCollectionFileState &file_state,
		GPlatesAppLogic::FeatureCollectionFileIO &file_io,
		QWidget *parent_) :
	QDialog(parent_, Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
	d_choose_widget(
			new ChooseFeatureCollectionWidget(
				file_state,
				file_io,
				this))
{
	setupUi(this);
	d_choose_widget->set_help_text(tr("Choose a feature collection for the cloned feature:"));
	QtWidgetUtils::add_widget_to_placeholder(
			d_choose_widget,
			placeholder_widget);

	QObject::connect(
			d_choose_widget,
			SIGNAL(item_activated()),
			this,
			SLOT(accept()));

	// ButtonBox signals.
	QObject::connect(
			main_buttonbox,
			SIGNAL(accepted()),
			this,
			SLOT(accept()));
	QObject::connect(
			main_buttonbox,
			SIGNAL(rejected()),
			this,
			SLOT(reject()));
}


boost::optional<std::pair<GPlatesAppLogic::FeatureCollectionFileState::file_reference, bool> >
GPlatesQtWidgets::ChooseFeatureCollectionDialog::get_file_reference(
		const GPlatesAppLogic::FeatureCollectionFileState::file_reference &initial)
{
	d_choose_widget->initialise();
	d_choose_widget->select_file_reference(initial);
	if (exec() == QDialog::Accepted)
	{
		try
		{
			return d_choose_widget->get_file_reference();
		}
		catch (std::exception &exc)
		{
			qWarning() << "GPlatesQtWidgets::ChooseFeatureCollectionDialog::get_file_reference: " << exc.what();
			return boost::none;
		}
		catch (...)
		{
			qWarning() << "GPlatesQtWidgets::ChooseFeatureCollectionDialog::get_file_reference: unknown error";
			return boost::none;
		}
	}
	else
	{
		return boost::none;
	}
}


boost::optional<std::pair<GPlatesAppLogic::FeatureCollectionFileState::file_reference, bool> >
GPlatesQtWidgets::ChooseFeatureCollectionDialog::get_file_reference(
		const GPlatesModel::FeatureCollectionHandle::weak_ref &initial)
{
	d_choose_widget->initialise();
	d_choose_widget->setFocus();
	d_choose_widget->select_feature_collection(initial);
	if (exec() == QDialog::Accepted)
	{
		try
		{
			return d_choose_widget->get_file_reference();
		}
		catch (std::exception &exc)
		{
			qWarning() << "GPlatesQtWidgets::ChooseFeatureCollectionDialog::get_file_reference: " << exc.what();
			return boost::none;
		}
		catch (...)
		{
			qWarning() << "GPlatesQtWidgets::ChooseFeatureCollectionDialog::get_file_reference: unknown error";
			return boost::none;
		}
	}
	else
	{
		return boost::none;
	}
}

boost::optional<std::pair<GPlatesAppLogic::FeatureCollectionFileState::file_reference, bool> >
GPlatesQtWidgets::ChooseFeatureCollectionDialog::get_file_reference()
{
	d_choose_widget->initialise();
	d_choose_widget->setFocus();

	if (exec() == QDialog::Accepted)
	{
		try
		{
			return d_choose_widget->get_file_reference();
		}
		catch (...)
		{
			return boost::none;
		}
	}
	else
	{
		return boost::none;
	}
}