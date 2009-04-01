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

#ifndef GPLATES_VIEWOPERATIONS_RENDEREDGEOMETRYCOLLECTIONUTILS_H
#define GPLATES_VIEWOPERATIONS_RENDEREDGEOMETRYCOLLECTIONUTILS_H

#include <boost/function.hpp>

#include "RenderedGeometryCollection.h"
#include "RenderedGeometryCollectionVisitor.h"

namespace GPlatesViewOperations
{
	/**
	 * Returns number of @a RenderedGeometryLayer objects that are not empty.
	 */
	unsigned int
	get_num_active_non_empty_layers(
			const RenderedGeometryCollection &rendered_geom_collection,
			RenderedGeometryCollection::MainLayerType main_layer_type,
			bool only_if_main_layer_active = true);

	/**
	 * Returns number of @a RenderedGeometryLayer objects that are not empty.
	 */
	unsigned int
	get_num_active_non_empty_layers(
			const RenderedGeometryCollection &rendered_geom_collection,
			RenderedGeometryCollection::main_layers_update_type main_layers =
					RenderedGeometryCollection::ALL_MAIN_LAYERS,
			bool only_if_main_layer_active = true);

	/**
	 * Activate all @a RenderedGeometryLayer objects in the specified main layer.
	 * If @a only_if_main_layer_active is true then only activates if main
	 * layer is active.
	 */
	void
	activate_rendered_geometry_layers(
			RenderedGeometryCollection &rendered_geom_collection,
			RenderedGeometryCollection::MainLayerType main_layer_type,
			bool only_if_main_layer_active = true);

	/**
	 * Activate all @a RenderedGeometryLayer objects in the specified main layers.
	 * If @a only_if_main_layer_active is true then only activates for those main
	 * layers that are active.
	 */
	void
	activate_rendered_geometry_layers(
			RenderedGeometryCollection &rendered_geom_collection,
			RenderedGeometryCollection::main_layers_update_type main_layers =
					RenderedGeometryCollection::ALL_MAIN_LAYERS,
			bool only_if_main_layer_active = true);

	/**
	 * Deactivate all @a RenderedGeometryLayer objects in the specified main layer.
	 * If @a only_if_main_layer_active is true then only deactivates if main
	 * layer is active.
	 */
	void
	deactivate_rendered_geometry_layers(
			RenderedGeometryCollection &rendered_geom_collection,
			RenderedGeometryCollection::MainLayerType main_layer_type,
			bool only_if_main_layer_active = true);

	/**
	 * Deactivate all @a RenderedGeometryLayer objects in the specified main layers.
	 * If @a only_if_main_layer_active is true then only deactivates for those main
	 * layers that are active.
	 */
	void
	deactivate_rendered_geometry_layers(
			RenderedGeometryCollection &rendered_geom_collection,
			RenderedGeometryCollection::main_layers_update_type main_layers =
					RenderedGeometryCollection::ALL_MAIN_LAYERS,
			bool only_if_main_layer_active = true);


	/**
	 * Visits a @a RenderedGeometryCollection and calls a user-specified function,
	 * class method or functor on each @a RenderedGeometryLayer object contained within.
	 *
	 * The above functions use this class.
	 */
	class VisitFunctionOnRenderedGeometryLayers :
		private RenderedGeometryCollectionVisitor
	{
	public:
		/**
		 * A function to call when visiting each @a RenderedGeometryLayer.
		 */
		typedef boost::function<void (RenderedGeometryLayer &)> rendered_geometry_layer_function_type;

		/**
		 * Specify the main layers in which @a rendered_geom_layer_function will be
		 * called on the @a RenderedGeometryLayer objects in the collection.
		 *
		 * @param rendered_geom_layer_function the function that will be called on
		 *        each @a RenderedGeometryLayer.
		 * @param main_layers the list of main layers to visit.
		 * @param only_if_main_layer_active only calls function on
		 *        @a RenderedGeometryLayer objects that belong to active main layers.
		 */
		VisitFunctionOnRenderedGeometryLayers(
				rendered_geometry_layer_function_type rendered_geom_layer_function,
				RenderedGeometryCollection::main_layers_update_type main_layers =
						RenderedGeometryCollection::ALL_MAIN_LAYERS,
				bool only_if_main_layer_active = true);

		void
		call_function(
				RenderedGeometryCollection &rendered_geom_collection);

	private:
		virtual
		bool
		visit_main_rendered_layer(
				RenderedGeometryCollection &rendered_geometry_collection,
				RenderedGeometryCollection::MainLayerType main_layer_type);

		virtual
		bool
		visit_rendered_geometry_layer(
				RenderedGeometryLayer &rendered_geometry_layer);

	private:
		rendered_geometry_layer_function_type d_rendered_geom_layer_function;
		RenderedGeometryCollection::main_layers_update_type d_main_layers;
		bool d_only_if_main_layer_active;
	};


	/**
	 * Visits a @a RenderedGeometryCollection and calls a user-specified function,
	 * class method or functor on each @a RenderedGeometryLayer object contained within.
	 *
	 * The above functions use this class.
	 */
	class ConstVisitFunctionOnRenderedGeometryLayers :
		private ConstRenderedGeometryCollectionVisitor
	{
	public:
		/**
		 * A function to call when visiting each @a RenderedGeometryLayer.
		 */
		typedef boost::function<void (const RenderedGeometryLayer &)> rendered_geometry_layer_function_type;

		/**
		 * Specify the main layers in which @a rendered_geom_layer_function will be
		 * called on the @a RenderedGeometryLayer objects in the collection.
		 *
		 * @param rendered_geom_layer_function the function that will be called on
		 *        each @a RenderedGeometryLayer.
		 * @param main_layers the list of main layers to visit.
		 * @param only_if_main_layer_active only calls function on
		 *        @a RenderedGeometryLayer objects that belong to active main layers.
		 */
		ConstVisitFunctionOnRenderedGeometryLayers(
				rendered_geometry_layer_function_type rendered_geom_layer_function,
				RenderedGeometryCollection::main_layers_update_type main_layers =
						RenderedGeometryCollection::ALL_MAIN_LAYERS,
				bool only_if_main_layer_active = true);

		void
		call_function(
				const RenderedGeometryCollection &rendered_geom_collection);

	private:
		virtual
		bool
		visit_main_rendered_layer(
				const RenderedGeometryCollection &rendered_geometry_collection,
				RenderedGeometryCollection::MainLayerType main_layer_type);

		virtual
		bool
		visit_rendered_geometry_layer(
				const RenderedGeometryLayer &rendered_geometry_layer);

	private:
		rendered_geometry_layer_function_type d_rendered_geom_layer_function;
		RenderedGeometryCollection::main_layers_update_type d_main_layers;
		bool d_only_if_main_layer_active;
	};
}

#endif // GPLATES_VIEWOPERATIONS_RENDEREDGEOMETRYCOLLECTIONUTILS_H
