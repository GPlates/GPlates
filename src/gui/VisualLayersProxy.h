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

#ifndef GPLATES_GUI_VISUALLAYERSPROXY_H
#define GPLATES_GUI_VISUALLAYERSPROXY_H

#include <cstring> // for size_t
#include <boost/weak_ptr.hpp>
#include <QObject>

#include "view-operations/RenderedGeometryCollection.h"


namespace GPlatesAppLogic
{
	class Layer;
}

namespace GPlatesPresentation
{
	class VisualLayer;
	class VisualLayers;
}

namespace GPlatesGui
{
	/**
	 * VisualLayersProxy is a simple wrapper around VisualLayers that reverses the
	 * order of visual layers.
	 *
	 * The ordering stored by VisualLayers is the order in which the visual layers
	 * should be drawn (i.e. back to front). However, the user interface presents
	 * the top layer first (i.e. front to back), and that is the conversion
	 * performed by this wrapper class.
	 */
	class VisualLayersProxy :
			public QObject
	{
		Q_OBJECT

	public:

		explicit
		VisualLayersProxy(
				GPlatesPresentation::VisualLayers &visual_layers);

		size_t
		size() const;

		void
		move_layer(
				size_t from_index,
				size_t to_index);

		boost::weak_ptr<GPlatesPresentation::VisualLayer>
		visual_layer_at(
				size_t index) const;

		boost::weak_ptr<GPlatesPresentation::VisualLayer>
		visual_layer_at(
				size_t index);

		GPlatesViewOperations::RenderedGeometryCollection::child_layer_index_type
		child_layer_index_at(
				size_t index) const;

		GPlatesViewOperations::RenderedGeometryCollection::child_layer_index_type
		child_layer_index_at(
				size_t index);

		boost::weak_ptr<const GPlatesPresentation::VisualLayer>
		get_visual_layer(
				const GPlatesAppLogic::Layer &layer) const;

		boost::weak_ptr<GPlatesPresentation::VisualLayer>
		get_visual_layer(
				const GPlatesAppLogic::Layer &layer);

	private slots:

		void
		handle_layer_order_changed(
				size_t first_index,
				size_t last_index);

		void
		handle_layer_about_to_be_added(
				size_t index);

		void
		handle_layer_added(
				size_t index);

		void
		handle_layer_added(
				boost::weak_ptr<GPlatesPresentation::VisualLayer> visual_layer);

		void
		handle_layer_about_to_be_removed(
				size_t index);

		void
		handle_layer_about_to_be_removed(
				boost::weak_ptr<GPlatesPresentation::VisualLayer> visual_layer);

		void
		handle_layer_removed(
				size_t index);

		void
		handle_layer_modified(
				size_t index);

		void
		handle_layer_modified(
				boost::weak_ptr<GPlatesPresentation::VisualLayer> visual_layer);

	signals:

		void
		layer_order_changed(
				size_t first_index,
				size_t last_index);

		void
		layer_about_to_be_added(
				size_t index);

		void
		layer_added(
				size_t index);

		void
		layer_added(
				boost::weak_ptr<GPlatesPresentation::VisualLayer> visual_layer);

		void
		layer_about_to_be_removed(
				size_t index);

		void
		layer_about_to_be_removed(
				boost::weak_ptr<GPlatesPresentation::VisualLayer> visual_layer);

		void
		layer_removed(
				size_t index);

		void
		layer_modified(
				size_t index);

		void
		layer_modified(
				boost::weak_ptr<GPlatesPresentation::VisualLayer> visual_layer);

	private:

		inline
		size_t
		fix_index(
				size_t index) const
		{
			return size() - 1 - index;
		}

		inline
		size_t
		fix_index(
				size_t index,
				size_t custom_visual_layers_size) const
		{
			return custom_visual_layers_size - 1 - index;
		}

		void
		make_signal_slot_connections();

		GPlatesPresentation::VisualLayers &d_visual_layers;
	};
}

#endif // GPLATES_GUI_VISUALLAYERSPROXY_H
