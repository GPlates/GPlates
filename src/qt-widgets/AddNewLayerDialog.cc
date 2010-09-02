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

#include <boost/foreach.hpp>
#include <QMetaType>

#include "AddNewLayerDialog.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/LayerTask.h"
#include "app-logic/LayerTaskRegistry.h"

#include "gui/LayerTaskTypeInfo.h"


namespace
{
	struct LayerTaskTypeInfo
	{
		LayerTaskTypeInfo()
		{  }

		LayerTaskTypeInfo(
				const GPlatesAppLogic::LayerTaskRegistry::LayerTaskType &layer_task_type_,
				GPlatesAppLogic::LayerTaskType::Type layer_enum_) :
			layer_task_type(layer_task_type_),
			layer_enum(layer_enum_)
		{  }

		GPlatesAppLogic::LayerTaskRegistry::LayerTaskType layer_task_type;
		GPlatesAppLogic::LayerTaskType::Type layer_enum;
	};
}


Q_DECLARE_METATYPE( LayerTaskTypeInfo )


GPlatesQtWidgets::AddNewLayerDialog::AddNewLayerDialog(
		GPlatesAppLogic::ApplicationState &application_state,
		QWidget *parent_):
	QDialog(parent_, Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
	d_application_state(application_state)
{
	setupUi(this);

	// ButtonBox signals.
	QObject::connect(
			main_buttonbox,
			SIGNAL(accepted()),
			this,
			SLOT(handle_accept()));
	QObject::connect(
			main_buttonbox,
			SIGNAL(rejected()),
			this,
			SLOT(reject()));

	// Combobox signals.
	QObject::connect(
			layer_type_combobox,
			SIGNAL(currentIndexChanged(int)),
			this,
			SLOT(handle_combobox_index_changed(int)));

	populate_combobox();
}


void
GPlatesQtWidgets::AddNewLayerDialog::handle_accept()
{
	// Create a new layer task and add it to the reconstruct graph.
	int index = layer_type_combobox->currentIndex();
	GPlatesAppLogic::LayerTaskRegistry::LayerTaskType layer_task_type =
		layer_type_combobox->itemData(index).value<LayerTaskTypeInfo>().layer_task_type;
	boost::shared_ptr<GPlatesAppLogic::LayerTask> layer_task = layer_task_type.create_layer_task();
	d_application_state.get_reconstruct_graph().add_layer(layer_task);

	accept();
}


void
GPlatesQtWidgets::AddNewLayerDialog::handle_combobox_index_changed(
		int index)
{
	GPlatesAppLogic::LayerTaskType::Type layer_enum =
		layer_type_combobox->itemData(index).value<LayerTaskTypeInfo>().layer_enum;
	layer_description_label->setText(
			GPlatesGui::LayerTaskTypeInfo::get_description(layer_enum));
}


void
GPlatesQtWidgets::AddNewLayerDialog::populate_combobox()
{
	GPlatesAppLogic::LayerTaskRegistry &layer_task_registry =
		d_application_state.get_layer_task_registry();
	std::vector<GPlatesAppLogic::LayerTaskRegistry::LayerTaskType> layer_task_types =
		layer_task_registry.get_all_layer_task_types();

	BOOST_FOREACH(const GPlatesAppLogic::LayerTaskRegistry::LayerTaskType &layer_task_type, layer_task_types)
	{
		boost::shared_ptr<GPlatesAppLogic::LayerTask> layer_task = layer_task_type.create_layer_task();
		GPlatesAppLogic::LayerTaskType::Type layer_enum = layer_task->get_layer_type();

		const QString &layer_name = GPlatesGui::LayerTaskTypeInfo::get_name(layer_enum);
		const QIcon &layer_icon = GPlatesGui::LayerTaskTypeInfo::get_icon(layer_enum);
		QVariant qv;
		qv.setValue(LayerTaskTypeInfo(layer_task_type, layer_enum));
		layer_type_combobox->addItem(layer_icon, layer_name, qv);
	}

	layer_type_combobox->setCurrentIndex(0);
}

