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
#include <boost/range/iterator_range.hpp>

#include "AddNewLayerDialog.h"

#include "presentation/ViewState.h"
#include "presentation/VisualLayerRegistry.h"


GPlatesQtWidgets::AddNewLayerDialog::AddNewLayerDialog(
		GPlatesAppLogic::ApplicationState &application_state,
		GPlatesPresentation::ViewState &view_state,
		QWidget *parent_):
	QDialog(parent_, Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
	d_application_state(application_state),
	d_view_state(view_state)
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
	GPlatesPresentation::VisualLayerType::Type visual_layer_type =
		static_cast<GPlatesPresentation::VisualLayerType::Type>(
				layer_type_combobox->itemData(index).value<unsigned int>());
	const GPlatesPresentation::VisualLayerRegistry &visual_layer_registry =
		d_view_state.get_visual_layer_registry();
	visual_layer_registry.create_visual_layer(visual_layer_type);

	accept();
}


void
GPlatesQtWidgets::AddNewLayerDialog::handle_combobox_index_changed(
		int index)
{
	GPlatesPresentation::VisualLayerType::Type visual_layer_type =
		static_cast<GPlatesPresentation::VisualLayerType::Type>(
				layer_type_combobox->itemData(index).value<unsigned int>());
	const GPlatesPresentation::VisualLayerRegistry &visual_layer_registry =
		d_view_state.get_visual_layer_registry();
	layer_description_label->setText(visual_layer_registry.get_description(visual_layer_type));
}


void
GPlatesQtWidgets::AddNewLayerDialog::populate_combobox()
{
	const GPlatesPresentation::VisualLayerRegistry &visual_layer_registry =
		d_view_state.get_visual_layer_registry();
	typedef GPlatesPresentation::VisualLayerRegistry::visual_layer_type_seq_type visual_layer_type_seq_type;
	visual_layer_type_seq_type visual_layer_types = visual_layer_registry.get_visual_layer_types_in_order();

	// Simulating BOOST_REVERSE_FOREACH with a reversed iterator range since BOOST_REVERSE_FOREACH
	// was introduced in version 1.36 and our minimum requirement is 1.34.
	BOOST_FOREACH(
			visual_layer_type_seq_type::value_type visual_layer_type,
			boost::make_iterator_range(visual_layer_types.rbegin(), visual_layer_types.rend()))
	{
		const QString &layer_name = visual_layer_registry.get_name(visual_layer_type);
		const QIcon &layer_icon = visual_layer_registry.get_icon(visual_layer_type);
		QVariant qv(static_cast<unsigned int>(visual_layer_type));
		layer_type_combobox->addItem(layer_icon, layer_name, qv);
	}

	layer_type_combobox->setCurrentIndex(0);
}

