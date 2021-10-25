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

#include "VisualLayersProxy.h"

#include "presentation/VisualLayers.h"


GPlatesGui::VisualLayersProxy::VisualLayersProxy(
		GPlatesPresentation::VisualLayers &visual_layers) :
	d_visual_layers(visual_layers)
{
	make_signal_slot_connections();
}


size_t
GPlatesGui::VisualLayersProxy::size() const
{
	return d_visual_layers.size();
}


void
GPlatesGui::VisualLayersProxy::move_layer(
		size_t from_index,
		size_t to_index)
{
	d_visual_layers.move_layer(fix_index(from_index), fix_index(to_index));
}


boost::weak_ptr<GPlatesPresentation::VisualLayer>
GPlatesGui::VisualLayersProxy::visual_layer_at(
		size_t index) const
{
	return d_visual_layers.visual_layer_at(fix_index(index));
}


boost::weak_ptr<GPlatesPresentation::VisualLayer>
GPlatesGui::VisualLayersProxy::visual_layer_at(
		size_t index)
{
	return d_visual_layers.visual_layer_at(fix_index(index));
}


GPlatesViewOperations::RenderedGeometryCollection::child_layer_index_type
GPlatesGui::VisualLayersProxy::child_layer_index_at(
		size_t index) const
{
	return d_visual_layers.child_layer_index_at(fix_index(index));
}


GPlatesViewOperations::RenderedGeometryCollection::child_layer_index_type
GPlatesGui::VisualLayersProxy::child_layer_index_at(
		size_t index)
{
	return d_visual_layers.child_layer_index_at(fix_index(index));
}


boost::weak_ptr<const GPlatesPresentation::VisualLayer>
GPlatesGui::VisualLayersProxy::get_visual_layer(
		const GPlatesAppLogic::Layer &layer) const
{
	return d_visual_layers.get_visual_layer(layer);
}


boost::weak_ptr<GPlatesPresentation::VisualLayer>
GPlatesGui::VisualLayersProxy::get_visual_layer(
		const GPlatesAppLogic::Layer &layer)
{
	return d_visual_layers.get_visual_layer(layer);
}

void
GPlatesGui::VisualLayersProxy::show_all()
{
	d_visual_layers.show_all();
}

void
GPlatesGui::VisualLayersProxy::hide_all()
{
	d_visual_layers.hide_all();
}


void
GPlatesGui::VisualLayersProxy::handle_layer_order_changed(
		size_t first_index,
		size_t last_index)
{
	// Note that we need to flip the order of the indices passed on.
	Q_EMIT layer_order_changed(fix_index(last_index), fix_index(first_index));
}


void
GPlatesGui::VisualLayersProxy::handle_begin_add_or_remove_layers()
{
	Q_EMIT begin_add_or_remove_layers();
}


void
GPlatesGui::VisualLayersProxy::handle_end_add_or_remove_layers()
{
	Q_EMIT end_add_or_remove_layers();
}


void
GPlatesGui::VisualLayersProxy::handle_layer_about_to_be_added(
		size_t index)
{
	// Note that here, the index is an index into the container of visual layers
	// after it has been resized.
	Q_EMIT layer_about_to_be_added(fix_index(index, size() + 1));
}


void
GPlatesGui::VisualLayersProxy::handle_layer_added(
		size_t index)
{
	Q_EMIT layer_added(fix_index(index));
}


void
GPlatesGui::VisualLayersProxy::handle_layer_added(
		boost::weak_ptr<GPlatesPresentation::VisualLayer> visual_layer)
{
	Q_EMIT layer_added(visual_layer);
}


void
GPlatesGui::VisualLayersProxy::handle_layer_about_to_be_removed(
		size_t index)
{
	Q_EMIT layer_about_to_be_removed(fix_index(index));
}


void
GPlatesGui::VisualLayersProxy::handle_layer_about_to_be_removed(
		boost::weak_ptr<GPlatesPresentation::VisualLayer> visual_layer)
{
	Q_EMIT layer_about_to_be_removed(visual_layer);
}


void
GPlatesGui::VisualLayersProxy::handle_layer_removed(
		size_t index)
{
	Q_EMIT layer_removed(fix_index(index));
}


void
GPlatesGui::VisualLayersProxy::handle_layer_modified(
		size_t index)
{
	Q_EMIT layer_modified(fix_index(index));
}


void
GPlatesGui::VisualLayersProxy::handle_layer_modified(
		boost::weak_ptr<GPlatesPresentation::VisualLayer> visual_layer)
{
	Q_EMIT layer_modified(visual_layer);
}


void
GPlatesGui::VisualLayersProxy::make_signal_slot_connections()
{
	// Connect to VisualLayers signals so we can pass them on.
	QObject::connect(
			&d_visual_layers,
			SIGNAL(layer_order_changed(size_t, size_t)),
			this,
			SLOT(handle_layer_order_changed(size_t, size_t)));
	QObject::connect(
			&d_visual_layers,
			SIGNAL(begin_add_or_remove_layers()),
			this,
			SLOT(handle_begin_add_or_remove_layers()));
	QObject::connect(
			&d_visual_layers,
			SIGNAL(end_add_or_remove_layers()),
			this,
			SLOT(handle_end_add_or_remove_layers()));
	QObject::connect(
			&d_visual_layers,
			SIGNAL(layer_about_to_be_added(size_t)),
			this,
			SLOT(handle_layer_about_to_be_added(size_t)));
	QObject::connect(
			&d_visual_layers,
			SIGNAL(layer_added(size_t)),
			this,
			SLOT(handle_layer_added(size_t)));
	QObject::connect(
			&d_visual_layers,
			SIGNAL(layer_added(boost::weak_ptr<GPlatesPresentation::VisualLayer>)),
			this,
			SLOT(handle_layer_added(boost::weak_ptr<GPlatesPresentation::VisualLayer>)));
	QObject::connect(
			&d_visual_layers,
			SIGNAL(layer_about_to_be_removed(size_t)),
			this,
			SLOT(handle_layer_about_to_be_removed(size_t)));
	QObject::connect(
			&d_visual_layers,
			SIGNAL(layer_about_to_be_removed(boost::weak_ptr<GPlatesPresentation::VisualLayer>)),
			this,
			SLOT(handle_layer_about_to_be_removed(boost::weak_ptr<GPlatesPresentation::VisualLayer>)));
	QObject::connect(
			&d_visual_layers,
			SIGNAL(layer_removed(size_t)),
			this,
			SLOT(handle_layer_removed(size_t)));
	QObject::connect(
			&d_visual_layers,
			SIGNAL(layer_modified(size_t)),
			this,
			SLOT(handle_layer_modified(size_t)));
	QObject::connect(
			&d_visual_layers,
			SIGNAL(layer_modified(boost::weak_ptr<GPlatesPresentation::VisualLayer>)),
			this,
			SLOT(handle_layer_modified(boost::weak_ptr<GPlatesPresentation::VisualLayer>)));
}

