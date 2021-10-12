/* $Id$ */

/**
 * \file 
 * Interface for visiting elements of a @a RenderedGeometryCollection.
 * Extends interface of @a RenderedGeometryVisitor.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2008, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_VIEWOPERATIONS_RENDEREDGEOMETRYCOLLECTIONVISITOR_H
#define GPLATES_VIEWOPERATIONS_RENDEREDGEOMETRYCOLLECTIONVISITOR_H

#include "RenderedGeometryLayerVisitor.h"
#include "RenderedGeometryCollection.h"


namespace GPlatesViewOperations
{
	/**
	 * Interface for visiting a @a RenderedGeometryCollection object and its
	 * @a RenderedGeometryLayer objects and its @a RenderedGeometry objects in turn.
	 *
	 * Visits const @a RenderedGeometryCollection object and its const
	 * @a RenderedGeometryLayer, and visits its const @a RenderedGeometry objects.
	 *
	 * The template parameter @a ForwardReadableRange is the type of the sequence
	 * of child layer indices used for custom order of visitation.
	 */
	template<class ForwardReadableRange = RenderedGeometryCollection::child_layer_index_seq_type>
	class ConstRenderedGeometryCollectionVisitor :
			public ConstRenderedGeometryLayerVisitor
	{
	public:
		/**
		 * Visit a main rendered layer.
		 *
		 * Return true if would like to visit the main render layer
		 * corresponding to @a main_render_layer_type (this includes any
		 * @a RenderedGeometry objects in the main layer and any child layers).
		 *
		 * Default is to only visit if main layer is active.
		 *
		 * @param rendered_geometry_collection the collection being visited.
		 * @param main_rendered_layer_type the type of main layer being visited.
		 * @return true if wish to visit the main rendered layer.
		 */
		virtual
		bool
		visit_main_rendered_layer(
				const RenderedGeometryCollection &rendered_geometry_collection,
				RenderedGeometryCollection::MainLayerType main_rendered_layer_type)
		{
			// Default is to only visit if layer is active.
			return rendered_geometry_collection.is_main_layer_active(
					main_rendered_layer_type);
		}

		// FIXME: The following virtual function should return a boost::optional of
		// ForwardReadableRange, to avoid having to deal with memory issues.

		/**
		 * Returns a sequence of child layer indices used for custom order of
		 * visitation of child layers for the given main layer.
		 *
		 * Default returns NULL, which indicates that the default order of
		 * visitation (by order of creation) is to be used.
		 */
		virtual
		const ForwardReadableRange *
		get_custom_child_layers_order(
				RenderedGeometryCollection::MainLayerType parent_layer)
		{
			return NULL;
		}
	};

	/**
	 * Interface for visiting a @a RenderedGeometryCollection object and its
	 * @a RenderedGeometryLayer objects and its @a RenderedGeometry objects in turn.
	 *
	 * Visits non-const @a RenderedGeometryCollection object and its non-const
	 * @a RenderedGeometryLayer but visits its const @a RenderedGeometry objects.
	 *
	 * The template parameter @a ForwardReadableRange is the type of the sequence
	 * of child layer indices used for custom order of visitation.
	 */
	template<class ForwardReadableRange = RenderedGeometryCollection::child_layer_index_seq_type>
	class RenderedGeometryCollectionVisitor :
			public RenderedGeometryLayerVisitor
	{
	public:
		/**
		 * Visit a main rendered layer.
		 *
		 * Return true if would like to visit the main render layer
		 * corresponding to @a main_render_layer_type (this includes any
		 * @a RenderedGeometry objects in the main layer and any child layers).
		 *
		 * Default is to only visit if main layer is active.
		 *
		 * @param rendered_geometry_collection the collection being visited.
		 * @param main_rendered_layer_type the type of main layer being visited.
		 * @return true if wish to visit the main rendered layer.
		 */
		virtual
		bool
		visit_main_rendered_layer(
				RenderedGeometryCollection &rendered_geometry_collection,
				RenderedGeometryCollection::MainLayerType main_rendered_layer_type)
		{
			// Default is to only visit if layer is active.
			return rendered_geometry_collection.is_main_layer_active(
					main_rendered_layer_type);
		}

		/**
		 * Returns a sequence of child layer indices used for custom order of
		 * visitation of child layers for the given main layer.
		 *
		 * Default returns NULL, which indicates that the default order of
		 * visitation (by order of creation) is to be used.
		 */
		virtual
		const ForwardReadableRange *
		get_custom_child_layers_order(
				RenderedGeometryCollection::MainLayerType parent_layer)
		{
			return NULL;
		}
	};
}

#endif // GPLATES_VIEWOPERATIONS_RENDEREDGEOMETRYCOLLECTIONVISITOR_H
