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
 
#ifndef GPLATES_PRESENTATION_VISUALLAYERREGISTRY_H
#define GPLATES_PRESENTATION_VISUALLAYERREGISTRY_H

#include <map>
#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/function.hpp>
#include <QString>
#include <QIcon>

#include "VisualLayerType.h"

#include "gui/Colour.h"

// Forward declaration.
class QWidget;

namespace GPlatesAppLogic
{
	class ApplicationState;
}

namespace GPlatesQtWidgets
{
	class LayerOptionsWidget;
	class ViewportWindow;
}

namespace GPlatesPresentation
{
	class ViewState;

	/**
	 * Stores user interface-related information about visual layers.
	 */
	class VisualLayerRegistry :
			public boost::noncopyable
	{
	public:

		/**
		 * Convenience typedef for a function that causes a visual layer to be
		 * added to the VisualLayers.
		 *
		 * The function takes no arguments. Note that it returns void; for
		 * visual layers that correspond to app-logic layers, this function
		 * should cause the corresponding app-logic layer to be inserted into
		 * the reconstruct graph, which will then cause a corresponding visual
		 * layer to be automatically created.
		 */
		typedef boost::function< void () > create_visual_layer_function_type;

		/**
		 * Convenience typedef for a function that creates a widget for editing
		 * the visual layer's options.
		 *
		 * The function takes as arguments the ApplicationState, the ViewState,
		 * a pointer to the ViewportWindow, and a pointer to the
		 * parent QWidget, and returns a pointer to a LayerOptionsWidget subtype.
		 * Returns NULL if there is no widget to edit the visual layer's options.
		 */
		typedef boost::function<
			GPlatesQtWidgets::LayerOptionsWidget *(
				GPlatesAppLogic::ApplicationState &,
				GPlatesPresentation::ViewState &,
				GPlatesQtWidgets::ViewportWindow *,
				QWidget *)
		> create_options_widget_function_type;

		/**
		 * Stores information about the given @a visual_layer_type.
		 *
		 * @a produces_rendered_geometries_ should be set to false only if this
		 * particular type of visual layer, almost paradoxically, will never
		 * produce rendered geometries (i.e. it is never visible).
		 */
		void
		register_visual_layer_type(
				VisualLayerType::Type visual_layer_type_,
				const QString &name_,
				const QString &description_,
				const GPlatesGui::Colour &colour_,
				const create_visual_layer_function_type &create_visual_layer_function_,
				const create_options_widget_function_type &create_options_widget_function_,
				bool produces_rendered_geometries_);

		void
		unregister_visual_layer_type(
				VisualLayerType::Type visual_layer_type);

		typedef std::vector<VisualLayerType::Type> visual_layer_type_seq_type;

		visual_layer_type_seq_type
		get_registered_visual_layer_types() const;

		/**
		 * Returns a human-readable name for the given visual layer type,
		 * or the empty string if the given type has not been registered.
		 */
		const QString &
		get_name(
				VisualLayerType::Type visual_layer_type) const;

		/**
		 * Returns a human-readable description for the given visual layer type,
		 * or the empty string if the given type has not been registered.
		 */
		const QString &
		get_description(
				VisualLayerType::Type visual_layer_type) const;

		/**
		 * Returns the colour associated with the given visual layer type,
		 * or black if the given type has not been registered.
		 */
		const GPlatesGui::Colour &
		get_colour(
				VisualLayerType::Type visual_layer_type) const;

		/**
		 * Returns an icon associated with the given visual layer type,
		 * or an uninitialised icon if the given type has not been registered.
		 */
		const QIcon &
		get_icon(
				VisualLayerType::Type visual_layer_type) const;

		/**
		 * Causes a new visual layer of the given type to be created;
		 * the visual layer type must have been already registered.
		 */
		void
		create_visual_layer(
				VisualLayerType::Type visual_layer_type) const;

		/**
		 * Returns a widget for editing the given visual layer type's options.
		 * Returns NULL if there is no widget for this visual layer type,
		 * or if the given type has not been registered.
		 */
		GPlatesQtWidgets::LayerOptionsWidget *
		create_options_widget(
				VisualLayerType::Type visual_layer_type,
				GPlatesAppLogic::ApplicationState &application_state,
				GPlatesPresentation::ViewState &view_state,
				GPlatesQtWidgets::ViewportWindow *viewport_window,
				QWidget *parent) const;

		/**
		 * Returns whether the given @a visual_layer_type ever produces rendered
		 * geometries. If it is false, this layer has no output that can be
		 * rendered on the globe or map.
		 */
		bool
		produces_rendered_geometries(
				VisualLayerType::Type visual_layer_type) const;

	private:

		struct VisualLayerInfo
		{
			VisualLayerInfo(
					const QString &name_,
					const QString &description_,
					const GPlatesGui::Colour &colour_,
					const QIcon &icon_,
					const create_visual_layer_function_type &create_visual_layer_function_,
					const create_options_widget_function_type &create_options_widget_function_,
					bool produces_rendered_geometries_);

			QString name;
			QString description;
			GPlatesGui::Colour colour;
			QIcon icon;
			create_visual_layer_function_type create_visual_layer_function;
			create_options_widget_function_type create_options_widget_function;
			bool produces_rendered_geometries;
		};

		typedef std::map<VisualLayerType::Type, VisualLayerInfo> visual_layer_info_map_type;

		visual_layer_info_map_type d_visual_layer_info_map;
	};

	/**
	 * Registers information about the default, built-in visual layers with the
	 * given @a registry.
	 */
	void
	register_default_visual_layers(
			VisualLayerRegistry &registry,
			GPlatesAppLogic::ApplicationState &application_state,
			GPlatesPresentation::ViewState &view_state);
}

#endif // GPLATES_PRESENTATION_VISUALLAYERREGISTRY_H
