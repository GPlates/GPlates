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

#include <algorithm>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <QPixmap>

#include "VisualLayerRegistry.h"

#include "RasterVisualLayerParams.h"
#include "ReconstructScalarCoverageVisualLayerParams.h"
#include "ReconstructVisualLayerParams.h"
#include "ScalarField3DVisualLayerParams.h"
#include "TopologyGeometryVisualLayerParams.h"
#include "TopologyNetworkVisualLayerParams.h"
#include "VelocityFieldCalculatorVisualLayerParams.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/LayerTaskRegistry.h"
#include "app-logic/LayerTaskType.h"
#include "app-logic/ReconstructGraph.h"

#include "gui/HTMLColourNames.h"

#include "qt-widgets/CoRegistrationOptionsWidget.h"
#include "qt-widgets/RasterLayerOptionsWidget.h"
#include "qt-widgets/ReconstructLayerOptionsWidget.h"
#include "qt-widgets/ReconstructScalarCoverageLayerOptionsWidget.h"
#include "qt-widgets/ReconstructionLayerOptionsWidget.h"
#include "qt-widgets/ScalarField3DLayerOptionsWidget.h"
#include "qt-widgets/TopologyGeometryResolverLayerOptionsWidget.h"
#include "qt-widgets/TopologyNetworkResolverLayerOptionsWidget.h"
#include "qt-widgets/VelocityFieldCalculatorLayerOptionsWidget.h"

#include "utils/ComponentManager.h"


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
			GPlatesQtWidgets::ViewportWindow *viewport_window,
			QWidget *parent)
	{
		return NULL;
	}

	// A function that instantiates the base VisualLayerParams class for use with
	// create_visual_layer_params_function.
	GPlatesPresentation::VisualLayerParams::non_null_ptr_type
	default_visual_layer_params(
			GPlatesAppLogic::LayerTaskParams &layer_task_params)
	{
		return GPlatesPresentation::VisualLayerParams::create(layer_task_params);
	}
}


void
GPlatesPresentation::VisualLayerRegistry::register_visual_layer_type(
		VisualLayerType::Type visual_layer_type_,
		VisualLayerGroup::Type group_,
		const QString &name_,
		const QString &description_,
		const GPlatesGui::Colour &colour_,
		const create_visual_layer_function_type &create_visual_layer_function_,
		const create_options_widget_function_type &create_options_widget_function_,
		const create_visual_layer_params_function_type &create_visual_layer_params_function_,
		bool produces_rendered_geometries_)
{
	static const int ICON_SIZE = 16;

	d_visual_layer_info_map.insert(
			std::make_pair(
				visual_layer_type_,
				VisualLayerInfo(
					group_,
					name_,
					description_,
					colour_,
					QIcon(get_filled_pixmap(ICON_SIZE, ICON_SIZE, colour_)),
					create_visual_layer_function_,
					create_options_widget_function_,
					create_visual_layer_params_function_,
					produces_rendered_geometries_)));

	d_visual_layer_type_order[static_cast<std::size_t>(group_)].push_back(visual_layer_type_);
	invalidate_order_cache();
}


void
GPlatesPresentation::VisualLayerRegistry::unregister_visual_layer_type(
		VisualLayerType::Type visual_layer_type)
{
	VisualLayerGroup::Type group = get_group(visual_layer_type);
	visual_layer_type_seq_type &group_order = d_visual_layer_type_order[static_cast<std::size_t>(group)];
	group_order.erase(std::find(group_order.begin(), group_order.end(), visual_layer_type));
	invalidate_order_cache();

	d_visual_layer_info_map.erase(visual_layer_type);
}


void
GPlatesPresentation::VisualLayerRegistry::invalidate_order_cache()
{
	d_cached_combined_visual_layer_type_order = boost::none;
	d_cached_visual_layer_type_order_map = boost::none;
}


const GPlatesPresentation::VisualLayerRegistry::visual_layer_type_seq_type &
GPlatesPresentation::VisualLayerRegistry::get_visual_layer_types_in_order() const
{
	if (!d_cached_combined_visual_layer_type_order)
	{
		d_cached_combined_visual_layer_type_order = visual_layer_type_seq_type();

		BOOST_FOREACH(const visual_layer_type_seq_type &group_order, d_visual_layer_type_order)
		{
			d_cached_combined_visual_layer_type_order->insert(
					d_cached_combined_visual_layer_type_order->end(),
					group_order.begin(),
					group_order.end());
		}
	}

	return *d_cached_combined_visual_layer_type_order;
}


const GPlatesPresentation::VisualLayerRegistry::visual_layer_type_order_map_type &
GPlatesPresentation::VisualLayerRegistry::get_visual_layer_type_order_map() const
{
	if (!d_cached_visual_layer_type_order_map)
	{
		const GPlatesPresentation::VisualLayerRegistry::visual_layer_type_seq_type &order =
			get_visual_layer_types_in_order();
		d_cached_visual_layer_type_order_map = visual_layer_type_order_map_type();
		
		for (std::size_t i = 0; i != order.size(); ++i)
		{
			d_cached_visual_layer_type_order_map->insert(std::make_pair(order[i], i));
		}
	}

	return *d_cached_visual_layer_type_order_map;
}


GPlatesPresentation::VisualLayerGroup::Type
GPlatesPresentation::VisualLayerRegistry::get_group(
		VisualLayerType::Type visual_layer_type) const
{
	visual_layer_info_map_type::const_iterator iter = d_visual_layer_info_map.find(visual_layer_type);
	if (iter == d_visual_layer_info_map.end())
	{
		return VisualLayerGroup::NUM_GROUPS;
	}
	else
	{
		return iter->second.group;
	}
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
		GPlatesQtWidgets::ViewportWindow *viewport_window,
		QWidget *parent) const
{
	visual_layer_info_map_type::const_iterator iter = d_visual_layer_info_map.find(visual_layer_type);
	if (iter != d_visual_layer_info_map.end())
	{
		return iter->second.create_options_widget_function(
				application_state,
				view_state,
				viewport_window,
				parent);
	}
	return NULL;
}


GPlatesPresentation::VisualLayerParams::non_null_ptr_type
GPlatesPresentation::VisualLayerRegistry::create_visual_layer_params(
		VisualLayerType::Type visual_layer_type,
		GPlatesAppLogic::LayerTaskParams &layer_task_params) const
{
	visual_layer_info_map_type::const_iterator iter = d_visual_layer_info_map.find(visual_layer_type);
	if (iter != d_visual_layer_info_map.end())
	{
		return iter->second.create_visual_layer_params_function(layer_task_params);
	}
	return VisualLayerParams::create(layer_task_params);
}


bool
GPlatesPresentation::VisualLayerRegistry::produces_rendered_geometries(
		VisualLayerType::Type visual_layer_type) const
{
	visual_layer_info_map_type::const_iterator iter = d_visual_layer_info_map.find(visual_layer_type);
	if (iter != d_visual_layer_info_map.end())
	{
		return iter->second.produces_rendered_geometries;
	}
	return false;
}


GPlatesPresentation::VisualLayerRegistry::VisualLayerInfo::VisualLayerInfo(
		VisualLayerGroup::Type group_,
		const QString &name_,
		const QString &description_,
		const GPlatesGui::Colour &colour_,
		const QIcon &icon_,
		const create_visual_layer_function_type &create_visual_layer_function_,
		const create_options_widget_function_type &create_options_widget_function_,
		const create_visual_layer_params_function_type &create_visual_layer_params_function_,
		bool produces_rendered_geometries_) :
	group(group_),
	name(name_),
	description(description_),
	colour(colour_),
	icon(icon_),
	create_visual_layer_function(create_visual_layer_function_),
	create_options_widget_function(create_options_widget_function_),
	create_visual_layer_params_function(create_visual_layer_params_function_),
	produces_rendered_geometries(produces_rendered_geometries_)
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
	// Note that, for each group, the visual layer types are registered in the
	// order used internally, i.e. opposite to how they are displayed on screen.
	//

	// BASIC_DATA group.
	registry.register_visual_layer_type(
			VisualLayerType::Type(RECONSTRUCT),
			VisualLayerGroup::BASIC_DATA,
			"Reconstructed Geometries",
			"Geometries in this layer will be reconstructed to the current reconstruction "
			"time when this layer is connected to a reconstruction tree layer.",
			*html_colours.get_colour("yellowgreen"),
			CreateAppLogicLayer(
				reconstruct_graph,
				layer_task_registry,
				RECONSTRUCT),
			&GPlatesQtWidgets::ReconstructLayerOptionsWidget::create,
			&ReconstructVisualLayerParams::create,
			true);

	// Need to put reconstructed scalar coverages in same group (BASIC_DATA) as
	// reconstructed feature geometries because the scalar coverages are coloured
	// per-point and this needs to be displayed on top of the feature geometries
	// which have a constant colour across the entire geometry.
	registry.register_visual_layer_type(
			VisualLayerType::Type(RECONSTRUCT_SCALAR_COVERAGE),
			VisualLayerGroup::BASIC_DATA,
			"Reconstructed Scalar Coverages",
			"Geometries containing a scalar value at each point.",
			*html_colours.get_colour("lightslategray"),
			CreateAppLogicLayer(
				reconstruct_graph,
				layer_task_registry,
				RECONSTRUCT_SCALAR_COVERAGE),
			&GPlatesQtWidgets::ReconstructScalarCoverageLayerOptionsWidget::create,
			boost::bind(
					&ReconstructScalarCoverageVisualLayerParams::create,
					_1),
			true);

	registry.register_visual_layer_type(
			VisualLayerType::Type(RECONSTRUCTION),
			VisualLayerGroup::BASIC_DATA,
			"Reconstruction Tree",
			"A plate-reconstruction hierarchy of total reconstruction poles "
			"that can be used to reconstruct geometries in other layers.",
			*html_colours.get_colour("gold"),
			CreateAppLogicLayer(
				reconstruct_graph,
				layer_task_registry,
				RECONSTRUCTION),
			&GPlatesQtWidgets::ReconstructionLayerOptionsWidget::create,
			&default_visual_layer_params,
			false);

	registry.register_visual_layer_type(
			VisualLayerType::Type(RASTER),
			VisualLayerGroup::RASTERS,
			"Reconstructed Raster",
			"A raster in this layer will be reconstructed when "
			"this layer is connected to a static plate polygon feature collection and "
			"to a reconstruction tree layer.",
			*html_colours.get_colour("tomato"),
			CreateAppLogicLayer(
				reconstruct_graph,
				layer_task_registry,
				RASTER),
			&GPlatesQtWidgets::RasterLayerOptionsWidget::create,
			&RasterVisualLayerParams::create,
			true);

	registry.register_visual_layer_type(
			VisualLayerType::Type(SCALAR_FIELD_3D),
			VisualLayerGroup::SCALAR_FIELDS,
			"3D Scalar Field",
			"A sub-surface scalar field visualised using volume rendering.",
			*html_colours.get_colour("teal"),
			CreateAppLogicLayer(
				reconstruct_graph,
				layer_task_registry,
				SCALAR_FIELD_3D),
			&GPlatesQtWidgets::ScalarField3DLayerOptionsWidget::create,
			boost::bind(
					&ScalarField3DVisualLayerParams::create,
					// NOTE: We pass in ViewState and not the GlobeAndMapWidget, obtained from
					// ViewportWindow, because ViewportWindow is not yet available (a reference to
					// it not yet been initialised inside ViewState) so accessing it would crash...
					_1, boost::ref(view_state)),
			true);

	// DERIVED_DATA group.
	registry.register_visual_layer_type(
			VisualLayerType::Type(TOPOLOGY_GEOMETRY_RESOLVER),
			VisualLayerGroup::DERIVED_DATA,
			"Resolved Topological Geometries",
			"Topological plate boundaries and lines will be generated dynamically by referencing "
			"topological section features, that have been reconstructed to a geological time, and "
			"joining them to form a closed polygon boundary or a polyline.",
			*html_colours.get_colour("plum"),
			CreateAppLogicLayer(
				reconstruct_graph,
				layer_task_registry,
				TOPOLOGY_GEOMETRY_RESOLVER),
			&GPlatesQtWidgets::TopologyGeometryResolverLayerOptionsWidget::create,
			&TopologyGeometryVisualLayerParams::create,
			true);

	registry.register_visual_layer_type(
			VisualLayerType::Type(TOPOLOGY_NETWORK_RESOLVER),
			VisualLayerGroup::DERIVED_DATA,
			"Resolved Topological Networks",
			"Deforming regions will be simulated dynamically by referencing topological section "
			"features, that have been reconstructed to a geological time, and triangulating "
			"the convex hull region defined by these reconstructed sections while excluding "
			"any micro-block sections from the triangulation.",
			*html_colours.get_colour("darkkhaki"),
			CreateAppLogicLayer(
				reconstruct_graph,
				layer_task_registry,
				TOPOLOGY_NETWORK_RESOLVER),
			&GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::create,
			&TopologyNetworkVisualLayerParams::create,
			true);

	registry.register_visual_layer_type(
			VisualLayerType::Type(VELOCITY_FIELD_CALCULATOR),
			VisualLayerGroup::DERIVED_DATA,
			"Calculated Velocity Fields",
			"Lithosphere-motion velocity vectors will be calculated dynamically at mesh points "
			"that lie within resolved topological boundaries or topological networks.",
			*html_colours.get_colour("aquamarine"),
			CreateAppLogicLayer(
				reconstruct_graph,
				layer_task_registry,
				VELOCITY_FIELD_CALCULATOR),
			&GPlatesQtWidgets::VelocityFieldCalculatorLayerOptionsWidget::create,
			boost::bind(
					&VelocityFieldCalculatorVisualLayerParams::create,
					_1, boost::cref(view_state.get_rendered_geometry_parameters())),
			true);

	using namespace  GPlatesUtils;
	if (ComponentManager::instance().is_enabled(ComponentManager::Component::data_mining())) 
	{
		registry.register_visual_layer_type(
				VisualLayerType::Type(CO_REGISTRATION),
				VisualLayerGroup::DERIVED_DATA,
				"Co-registration",
				"Co-registration layer for data mining.",
				*html_colours.get_colour("sandybrown"),
				CreateAppLogicLayer(
					reconstruct_graph,
					layer_task_registry,
					CO_REGISTRATION),
				&GPlatesQtWidgets::CoRegistrationOptionsWidget::create,
				&default_visual_layer_params,
				true);
	}

	//
	// The following visual layer types do not have corresponding app-logic layers
	// (none implemented as yet).
	//
}

