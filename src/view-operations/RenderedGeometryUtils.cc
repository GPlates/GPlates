/* $Id$ */

/**
 * \file 
 * Various helper functions, classes that use @a RenderedGeometryCollection.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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

#include <boost/bind.hpp>

#include "RenderedGeometryUtils.h"


namespace GPlatesViewOperations
{
	namespace
	{
		/**
		 * Increments count for every non-empty @a RenderedGeometryLayer.
		 */
		struct CountNonEmptyRenderedGeometries
		{
			CountNonEmptyRenderedGeometries() :
			d_count(0)
			{  }

			void
			operator()(
					const RenderedGeometryLayer &rendered_geom_layer)
			{
				if (rendered_geom_layer.is_active() && !rendered_geom_layer.is_empty())
				{
					++d_count;
				}
			}

			unsigned d_count;
		};
	}
}

unsigned int
GPlatesViewOperations::get_num_active_non_empty_layers(
		const RenderedGeometryCollection &rendered_geom_collection,
		RenderedGeometryCollection::MainLayerType main_layer_type,
		bool only_if_main_layer_active)
{
	RenderedGeometryCollection::main_layers_update_type main_layers;
	main_layers.set(main_layer_type);

	return get_num_active_non_empty_layers(
			rendered_geom_collection, main_layers, only_if_main_layer_active);
}

unsigned int
GPlatesViewOperations::get_num_active_non_empty_layers(
		const RenderedGeometryCollection &rendered_geom_collection,
		RenderedGeometryCollection::main_layers_update_type main_layers,
		bool only_if_main_layer_active)
{
	CountNonEmptyRenderedGeometries count;

	ConstVisitFunctionOnRenderedGeometryLayers get_count(
			boost::ref(count),
			main_layers,
			only_if_main_layer_active);

	get_count.call_function(rendered_geom_collection);

	return count.d_count;
}

void
GPlatesViewOperations::activate_rendered_geometry_layers(
		RenderedGeometryCollection &rendered_geom_collection,
		RenderedGeometryCollection::MainLayerType main_layer_type,
		bool only_if_main_layer_active)
{
	RenderedGeometryCollection::main_layers_update_type main_layers;
	main_layers.set(main_layer_type);

	activate_rendered_geometry_layers(
			rendered_geom_collection, main_layers, only_if_main_layer_active);
}

void
GPlatesViewOperations::activate_rendered_geometry_layers(
		RenderedGeometryCollection &rendered_geom_collection,
		RenderedGeometryCollection::main_layers_update_type main_layers,
		bool only_if_main_layer_active)
{
	VisitFunctionOnRenderedGeometryLayers activate(
			boost::bind(&RenderedGeometryLayer::set_active, _1, true),
			main_layers,
			only_if_main_layer_active);

	activate.call_function(rendered_geom_collection);
}

void
GPlatesViewOperations::deactivate_rendered_geometry_layers(
		RenderedGeometryCollection &rendered_geom_collection,
		RenderedGeometryCollection::MainLayerType main_layer_type,
		bool only_if_main_layer_active)
{
	RenderedGeometryCollection::main_layers_update_type main_layers;
	main_layers.set(main_layer_type);

	deactivate_rendered_geometry_layers(
			rendered_geom_collection, main_layers, only_if_main_layer_active);
}

void
GPlatesViewOperations::deactivate_rendered_geometry_layers(
		RenderedGeometryCollection &rendered_geom_collection,
		RenderedGeometryCollection::main_layers_update_type main_layers,
		bool only_if_main_layer_active)
{
	VisitFunctionOnRenderedGeometryLayers deactivate(
			boost::bind(&RenderedGeometryLayer::set_active, _1, false),
			main_layers,
			only_if_main_layer_active);

	deactivate.call_function(rendered_geom_collection);
}


GPlatesViewOperations::VisitFunctionOnRenderedGeometryLayers::VisitFunctionOnRenderedGeometryLayers(
		rendered_geometry_layer_function_type rendered_geom_layer_function,
		RenderedGeometryCollection::main_layers_update_type main_layers,
		bool only_if_main_layer_active) :
d_rendered_geom_layer_function(rendered_geom_layer_function),
d_main_layers(main_layers),
d_only_if_main_layer_active(only_if_main_layer_active)
{
}

void
GPlatesViewOperations::VisitFunctionOnRenderedGeometryLayers::call_function(
		RenderedGeometryCollection &rendered_geom_collection)
{
	rendered_geom_collection.accept_visitor(*this);
}

bool
GPlatesViewOperations::VisitFunctionOnRenderedGeometryLayers::visit_main_rendered_layer(
		RenderedGeometryCollection &rendered_geometry_collection,
		RenderedGeometryCollection::MainLayerType main_layer_type)
{
	if (d_only_if_main_layer_active &&
		!rendered_geometry_collection.is_main_layer_active(main_layer_type))
	{
		return false;
	}

	// Only visit if current main layer is one of the layers we're interested in.
	return d_main_layers.test(main_layer_type);
}

bool
GPlatesViewOperations::VisitFunctionOnRenderedGeometryLayers::visit_rendered_geometry_layer(
		RenderedGeometryLayer &rendered_geometry_layer)
{
	// If we get here then we've been approved for calling user-specified
	// function on this rendered geometry layer.
	d_rendered_geom_layer_function(rendered_geometry_layer);

	// Not interesting in visiting RenderedGeometry objects.
	return false;
}


GPlatesViewOperations::ConstVisitFunctionOnRenderedGeometryLayers::ConstVisitFunctionOnRenderedGeometryLayers(
		rendered_geometry_layer_function_type rendered_geom_layer_function,
		RenderedGeometryCollection::main_layers_update_type main_layers,
		bool only_if_main_layer_active) :
d_rendered_geom_layer_function(rendered_geom_layer_function),
d_main_layers(main_layers),
d_only_if_main_layer_active(only_if_main_layer_active)
{
}

void
GPlatesViewOperations::ConstVisitFunctionOnRenderedGeometryLayers::call_function(
		const RenderedGeometryCollection &rendered_geom_collection)
{
	rendered_geom_collection.accept_visitor(*this);
}

bool
GPlatesViewOperations::ConstVisitFunctionOnRenderedGeometryLayers::visit_main_rendered_layer(
		const RenderedGeometryCollection &rendered_geometry_collection,
		RenderedGeometryCollection::MainLayerType main_layer_type)
{
	if (d_only_if_main_layer_active &&
		!rendered_geometry_collection.is_main_layer_active(main_layer_type))
	{
		return false;
	}

	// Only visit if current main layer is one of the layers we're interested in.
	return d_main_layers.test(main_layer_type);
}

bool
GPlatesViewOperations::ConstVisitFunctionOnRenderedGeometryLayers::visit_rendered_geometry_layer(
		const RenderedGeometryLayer &rendered_geometry_layer)
{
	// If we get here then we've been approved for calling user-specified
	// function on this rendered geometry layer.
	d_rendered_geom_layer_function(rendered_geometry_layer);

	// Not interesting in visiting RenderedGeometry objects.
	return false;
}
