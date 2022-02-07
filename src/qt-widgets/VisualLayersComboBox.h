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
 
#ifndef GPLATES_QTWIDGETS_VISUALLAYERSCOMBOBOX_H
#define GPLATES_QTWIDGETS_VISUALLAYERSCOMBOBOX_H

#include <boost/optional.hpp>
#include <boost/function.hpp>
#include <boost/weak_ptr.hpp>
#include <QComboBox>
#include <QObject>

#include "presentation/VisualLayer.h"
#include "presentation/VisualLayerType.h"


namespace GPlatesPresentation
{
	class VisualLayerRegistry;
	class VisualLayers;
}

namespace GPlatesQtWidgets
{
	/**
	 * VisualLayersComboBox allows the user to select a visual layer.
	 */
	class VisualLayersComboBox :
			public QComboBox
	{
		Q_OBJECT

	public:

		typedef boost::function< bool ( GPlatesPresentation::VisualLayerType::Type ) > predicate_type;

		/**
		 * Constructs a VisualLayersComboBox that shows visual layers that meet a the
		 * given @a predicate based on the type of the visual layer.
		 */
		VisualLayersComboBox(
				GPlatesPresentation::VisualLayers &visual_layers,
				GPlatesPresentation::VisualLayerRegistry &visual_layer_registry,
				const predicate_type &predicate,
				QWidget *parent_ = NULL);

		virtual
		~VisualLayersComboBox(){ }

		boost::weak_ptr<GPlatesPresentation::VisualLayer>
		get_selected_visual_layer() const;

		virtual
		void
		set_selected_visual_layer(
				boost::weak_ptr<GPlatesPresentation::VisualLayer> visual_layer);

	Q_SIGNALS:

		void
		selected_visual_layer_changed(
				boost::weak_ptr<GPlatesPresentation::VisualLayer> visual_layer);

	protected Q_SLOTS:

		/**
		 * Called when anything in the visual layers state is changed.
		 */
		void
		handle_visual_layers_changed();

		void
		handle_current_index_changed(
				int index);

	protected:

		void
		make_signal_slot_connections(
				GPlatesPresentation::VisualLayers *visual_layers);

		virtual
		void
		populate();

		GPlatesPresentation::VisualLayers &d_visual_layers;
		GPlatesPresentation::VisualLayerRegistry &d_visual_layer_registry;
		predicate_type d_predicate;
	};
}


#endif	// GPLATES_QTWIDGETS_VISUALLAYERSCOMBOBOX_H
