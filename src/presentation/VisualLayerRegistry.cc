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
#include <QPixmap>

#include "VisualLayerRegistry.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/LayerTaskRegistry.h"
#include "app-logic/LayerTaskType.h"
#include "app-logic/ReconstructGraph.h"

#include "gui/HTMLColourNames.h"

#include "qt-widgets/RasterLayerOptionsWidget.h"
#include "qt-widgets/ReconstructionLayerOptionsWidget.h"


namespace
{
	QPixmap
	get_filled_pixmap(
			int width,
			int height,
			const GPlatesGui::Colour &colour)
	{
		QPixmap result(width, height);
		result.fill(colour);
		return result;
	}

	// A do-nothing function for use with create_visual_layer_function.
	void
	do_nothing()
	{  }

	/**
	 * A helper functor for use with create_visual_layer_function.
	 */
	class CreateAppLogicLayer
	{
	public:

		CreateAppLogicLayer(
				GPlatesAppLogic::ReconstructGraph &reconstruct_graph,
				GPlatesAppLogic::LayerTaskRegistry &layer_task_registry,
				GPlatesAppLogic::LayerTaskType::Type layer_type) :
			d_reconstruct_graph(reconstruct_graph)
		{
			std::vector<GPlatesAppLogic::LayerTaskRegistry::LayerTaskType> layer_task_types =
				layer_task_registry.get_all_layer_task_types();

			BOOST_FOREACH(const GPlatesAppLogic::LayerTaskRegistry::LayerTaskType &layer_task_type, layer_task_types)
			{
				if (layer_task_type.get_layer_type() == layer_type)
				{
					d_layer_task_type = layer_task_type;
					break;
				}
			}
		}

		void
		operator()()
		{
			boost::shared_ptr<GPlatesAppLogic::LayerTask> layer_task = d_layer_task_type.create_layer_task();
			d_reconstruct_graph.add_layer(layer_task);
		}

	private:

		GPlatesAppLogic::ReconstructGraph &d_reconstruct_graph;
		GPlatesAppLogic::LayerTaskRegistry::LayerTaskType d_layer_task_type;
	};

	// A function that always returns NULL for use with create_options_widget_function.
	GPlatesQtWidgets::LayerOptionsWidget *
	no_widget(
			GPlatesAppLogic::ApplicationState &application_state,
			GPlatesPresentation::ViewState &view_state,
			GPlatesQtWidgets::ReadErrorAccumulationDialog *read_errors_dialog,
			QWidget *parent)
	{
		return NULL;
	}
}


void
GPlatesPresentation::VisualLayerRegistry::register_visual_layer_type(
		VisualLayerType::Type visual_layer_type,
		const QString &name,
		const QString &description,
		const GPlatesGui::Colour &colour,
		const create_visual_layer_function_type &create_visual_layer_function,
		const create_options_widget_function_type &create_options_widget_function)
{
	static const int ICON_SIZE = 16;

	d_visual_layer_info_map.insert(
			std::make_pair(
				visual_layer_type,
				VisualLayerInfo(
					name,
					description,
					colour,
					QIcon(get_filled_pixmap(ICON_SIZE, ICON_SIZE, colour)),
					create_visual_layer_function,
					create_options_widget_function)));
}


void
GPlatesPresentation::VisualLayerRegistry::unregister_visual_layer_type(
		VisualLayerType::Type visual_layer_type)
{
	d_visual_layer_info_map.erase(visual_layer_type);
}


GPlatesPresentation::VisualLayerRegistry::visual_layer_type_seq_type
GPlatesPresentation::VisualLayerRegistry::get_registered_visual_layer_types() const
{
	visual_layer_type_seq_type result;
	BOOST_FOREACH(const visual_layer_info_map_type::value_type &elem, d_visual_layer_info_map)
	{
		result.push_back(elem.first);
	}
	return result;
}


const QString &
GPlatesPresentation::VisualLayerRegistry::get_name(
		VisualLayerType::Type visual_layer_type) const
{
	visual_layer_info_map_type::const_iterator iter = d_visual_layer_info_map.find(visual_layer_type);
	if (iter == d_visual_layer_info_map.end())
	{
		static const QString DEFAULT_NAME;
		return DEFAULT_NAME;
	}
	else
	{
		return iter->second.name;
	}
}


const QString &
GPlatesPresentation::VisualLayerRegistry::get_description(
		VisualLayerType::Type visual_layer_type) const
{
	visual_layer_info_map_type::const_iterator iter = d_visual_layer_info_map.find(visual_layer_type);
	if (iter == d_visual_layer_info_map.end())
	{
		static const QString DEFAULT_DESCRIPTION;
		return DEFAULT_DESCRIPTION;
	}
	else
	{
		return iter->second.description;
	}
}


const GPlatesGui::Colour &
GPlatesPresentation::VisualLayerRegistry::get_colour(
		VisualLayerType::Type visual_layer_type) const
{
	visual_layer_info_map_type::const_iterator iter = d_visual_layer_info_map.find(visual_layer_type);
	if (iter == d_visual_layer_info_map.end())
	{
		static const GPlatesGui::Colour DEFAULT_COLOUR;
		return DEFAULT_COLOUR;
	}
	else
	{
		return iter->second.colour;
	}
}


const QIcon &
GPlatesPresentation::VisualLayerRegistry::get_icon(
		VisualLayerType::Type visual_layer_type) const
{
	visual_layer_info_map_type::const_iterator iter = d_visual_layer_info_map.find(visual_layer_type);
	if (iter == d_visual_layer_info_map.end())
	{
		static const QIcon DEFAULT_ICON;
		return DEFAULT_ICON;
	}
	else
	{
		return iter->second.icon;
	}
}


void
GPlatesPresentation::VisualLayerRegistry::create_visual_layer(
		VisualLayerType::Type visual_layer_type) const
{
	visual_layer_info_map_type::const_iterator iter = d_visual_layer_info_map.find(visual_layer_type);
	if (iter != d_visual_layer_info_map.end())
	{
		iter->second.create_visual_layer_function();
	}
}


GPlatesQtWidgets::LayerOptionsWidget *
GPlatesPresentation::VisualLayerRegistry::create_options_widget(
		VisualLayerType::Type visual_layer_type,
		GPlatesAppLogic::ApplicationState &application_state,
		GPlatesPresentation::ViewState &view_state,
		GPlatesQtWidgets::ReadErrorAccumulationDialog *read_errors_dialog,
		QWidget *parent) const
{
	visual_layer_info_map_type::const_iterator iter = d_visual_layer_info_map.find(visual_layer_type);
	if (iter != d_visual_layer_info_map.end())
	{
		return iter->second.create_options_widget_function(
				application_state,
				view_state,
				read_errors_dialog,
				parent);
	}
	return NULL;
}


GPlatesPresentation::VisualLayerRegistry::VisualLayerInfo::VisualLayerInfo(
		const QString &name_,
		const QString &description_,
		const GPlatesGui::Colour &colour_,
		const QIcon &icon_,
		const create_visual_layer_function_type &create_visual_layer_function_,
		const create_options_widget_function_type &create_options_widget_function_) :
	name(name_),
	description(description_),
	colour(colour_),
	icon(icon_),
	create_visual_layer_function(create_visual_layer_function_),
	create_options_widget_function(create_options_widget_function_)
{  }


void
GPlatesPresentation::register_default_visual_layers(
		VisualLayerRegistry &registry,
		GPlatesAppLogic::ApplicationState &application_state,
		GPlatesPresentation::ViewState &view_state)
{
	using namespace GPlatesAppLogic::LayerTaskType;

	GPlatesGui::HTMLColourNames &html_colours = GPlatesGui::HTMLColourNames::instance();
	GPlatesAppLogic::ReconstructGraph &reconstruct_graph = application_state.get_reconstruct_graph();
	GPlatesAppLogic::LayerTaskRegistry &layer_task_registry = application_state.get_layer_task_registry();

	//
	// The following visual layer types are those that have corresponding app-logic layers.
	//

	registry.register_visual_layer_type(
			VisualLayerType::Type(RECONSTRUCTION),
			"Reconstruction Tree",
			"A plate-reconstruction hierarchy of total reconstruction poles "
			"that can be used to reconstruct geometries in other layers",
			*html_colours.get_colour("gold"),
			CreateAppLogicLayer(
				reconstruct_graph,
				layer_task_registry,
				RECONSTRUCTION),
			&GPlatesQtWidgets::ReconstructionLayerOptionsWidget::create);

	registry.register_visual_layer_type(
			VisualLayerType::Type(RECONSTRUCT),
			"Reconstructed Geometries",
			"Geometries in this layer will be reconstructed to the current reconstruction "
			"time when this layer is connected to a reconstruction tree layer",
			*html_colours.get_colour("yellowgreen"),
			CreateAppLogicLayer(
				reconstruct_graph,
				layer_task_registry,
				RECONSTRUCT),
			&no_widget);

	registry.register_visual_layer_type(
			VisualLayerType::Type(RASTER),
			"Reconstructed Raster",
			"A raster in this layer will be reconstructed when "
			"this layer is connected to a static plate polygon feature collection and "
			"to a reconstruction tree layer",
			*html_colours.get_colour("tomato"),
			&do_nothing,
			&GPlatesQtWidgets::RasterLayerOptionsWidget::create);

	registry.register_visual_layer_type(
			VisualLayerType::Type(AGE_GRID),
			"Age Grid",
			"An age grid can be attached to a reconstructed raster layer to provide "
			"smoother raster reconstructions",
			*html_colours.get_colour("darkturquoise"),
			CreateAppLogicLayer(
				reconstruct_graph,
				layer_task_registry,
				AGE_GRID),
			&no_widget);

	registry.register_visual_layer_type(
			VisualLayerType::Type(TOPOLOGY_BOUNDARY_RESOLVER),
			"Resolved Topological Closed Plate Boundaries",
			"Plate boundaries will be generated dynamically by referencing topological section "
			"features, that have been reconstructed to a geological time, and joining them to "
			"form a closed polygon boundary",
			*html_colours.get_colour("plum"),
			CreateAppLogicLayer(
				reconstruct_graph,
				layer_task_registry,
				TOPOLOGY_BOUNDARY_RESOLVER),
			&no_widget);

	registry.register_visual_layer_type(
			VisualLayerType::Type(TOPOLOGY_NETWORK_RESOLVER),
			"Resolved Topological Networks",
			"Deforming regions will be simulated dynamically by referencing topological section "
			"features, that have been reconstructed to a geological time, and triangulating "
			"the convex hull region defined by these reconstructed sections while excluding "
			"any micro-block sections from the triangulation",
			*html_colours.get_colour("darkkhaki"),
			CreateAppLogicLayer(
				reconstruct_graph,
				layer_task_registry,
				TOPOLOGY_NETWORK_RESOLVER),
			&no_widget);

	registry.register_visual_layer_type(
			VisualLayerType::Type(VELOCITY_FIELD_CALCULATOR),
			"Calculated Velocity Fields",
			"Lithosphere-motion velocity vectors will be calculated dynamically at mesh points "
			"that lie within resolved topological boundaries or topological networks",
			*html_colours.get_colour("aquamarine"),
			CreateAppLogicLayer(
				reconstruct_graph,
				layer_task_registry,
				VELOCITY_FIELD_CALCULATOR),
			&no_widget);

	//
	// The following visual layer types do not have corresponding app-logic layers
	// (none implemented as yet).
	//
}

