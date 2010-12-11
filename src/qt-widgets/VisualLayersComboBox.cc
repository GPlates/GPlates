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

#include <boost/shared_ptr.hpp>
#include <QVariant>
#include <QMetaType>
#include <QDebug>

#include "VisualLayersComboBox.h"

#include "presentation/VisualLayerRegistry.h"
#include "presentation/VisualLayers.h"

Q_DECLARE_METATYPE( boost::weak_ptr<GPlatesPresentation::VisualLayer> )


GPlatesQtWidgets::VisualLayersComboBox::VisualLayersComboBox(
		GPlatesPresentation::VisualLayers &visual_layers,
		GPlatesPresentation::VisualLayerRegistry &visual_layer_registry,
		const predicate_type &predicate,
		QWidget *parent_) :
	QComboBox(parent_),
	d_visual_layers(visual_layers),
	d_visual_layer_registry(visual_layer_registry),
	d_predicate(predicate)
{
	make_signal_slot_connections(&visual_layers);
	populate();
}


boost::weak_ptr<GPlatesPresentation::VisualLayer>
GPlatesQtWidgets::VisualLayersComboBox::get_selected_visual_layer() const
{
	if (currentIndex() == -1)
	{
		return boost::weak_ptr<GPlatesPresentation::VisualLayer>();
	}

	return itemData(currentIndex()).value<boost::weak_ptr<GPlatesPresentation::VisualLayer> >();
}


void
GPlatesQtWidgets::VisualLayersComboBox::set_selected_visual_layer(
		boost::weak_ptr<GPlatesPresentation::VisualLayer> visual_layer)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = visual_layer.lock())
	{
		for (int i = 0; i != count(); ++i)
		{
			boost::weak_ptr<GPlatesPresentation::VisualLayer> curr = itemData(i).value<
				boost::weak_ptr<GPlatesPresentation::VisualLayer> >();
			if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_curr = curr.lock())
			{
				if (locked_curr == locked_visual_layer)
				{
					setCurrentIndex(i);
					return;
				}
			}
		}
	}
}


void
GPlatesQtWidgets::VisualLayersComboBox::handle_visual_layers_changed()
{
	populate();
}


void
GPlatesQtWidgets::VisualLayersComboBox::handle_current_index_changed(
		int index)
{
	emit selected_visual_layer_changed(get_selected_visual_layer());
}


void
GPlatesQtWidgets::VisualLayersComboBox::make_signal_slot_connections(
		GPlatesPresentation::VisualLayers *visual_layers)
{
	// VisualLayers signals.
	QObject::connect(
			visual_layers,
			SIGNAL(changed()),
			this,
			SLOT(handle_visual_layers_changed()));

	// QComboBox signals.
	QObject::connect(
			this,
			SIGNAL(currentIndexChanged(int)),
			this,
			SLOT(handle_current_index_changed(int)));
}


void
GPlatesQtWidgets::VisualLayersComboBox::populate()
{
	// Remember which visual layer (if any) was selected before repopulating the combobox.
	boost::weak_ptr<GPlatesPresentation::VisualLayer> selected = get_selected_visual_layer();
	int index_to_select = -1;
	int curr_index = 0;

	// Suppress signal first.
	QObject::disconnect(
			this,
			SIGNAL(currentIndexChanged(int)),
			this,
			SLOT(handle_current_index_changed(int)));

	clear();

	for (size_t i = d_visual_layers.size(); i != 0; --i)
	{
		boost::weak_ptr<GPlatesPresentation::VisualLayer> curr = d_visual_layers.visual_layer_at(i - 1);
		if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_curr = curr.lock())
		{
			GPlatesPresentation::VisualLayerType::Type type = locked_curr->get_layer_type();
			if (d_predicate(type))
			{
				QVariant qv;
				qv.setValue(curr);
				addItem(d_visual_layer_registry.get_icon(type), locked_curr->get_name(), qv);
				
				if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_selected = selected.lock())
				{
					if (locked_selected == locked_curr)
					{
						index_to_select = curr_index;
					}
				}

				++curr_index;
			}
		}
	}

	if (index_to_select != -1)
	{
		setCurrentIndex(index_to_select);
	}

	// Reconnect signals and manually emit signal.
	QObject::connect(
			this,
			SIGNAL(currentIndexChanged(int)),
			this,
			SLOT(handle_current_index_changed(int)));
	handle_current_index_changed(currentIndex());
}

