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
 
#ifndef GPLATES_PRESENTATION_VISUALLAYER_H
#define GPLATES_PRESENTATION_VISUALLAYER_H

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <QString>

#include "VisualLayerType.h"

#include "app-logic/Layer.h"

#include "model/FeatureCollectionHandle.h"

#include "view-operations/RenderedGeometryCollection.h"


namespace GPlatesAppLogic
{
	class ApplicationState;
}

namespace GPlatesPresentation
{
	class VisualLayers;

	/**
	 * Represents a layer that processes inputs (such as a feature collection)
	 * to an output (such as reconstruction geometries) and determines how
	 * to visualise the output.
	 */
	class VisualLayer :
			private boost::noncopyable
	{
	public:

		/**
		 * Constructor wraps a visual layer around @a layer created in @a ReconstructGraph.
		 */
		VisualLayer(
				VisualLayers &visual_layers,
				const GPlatesAppLogic::Layer &layer,
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
				int layer_number);

		/**
		 * Returns the layer used by @a ReconstructGraph.
		 */
		const GPlatesAppLogic::Layer &
		get_reconstruct_graph_layer() const
		{
			return d_layer;
		}

		GPlatesViewOperations::RenderedGeometryCollection::child_layer_index_type
		get_rendered_geometry_layer_index() const
		{
			return d_rendered_geometry_layer_index;
		}

		/**
		 * Returns the type of the visual layer as an enumeration.
		 */
		VisualLayerType::Type
		get_layer_type() const;

		/**
		 * Creates rendered geometries for this visual layer.
		 *
		 * Each visual layer has its own rendered geometry layer created inside the
		 * @a RenderedGeometryCollection passed into the constructor.
		 */
		void
		create_rendered_geometries();

		/**
		 * Returns true if the visual layer is expanded in the user interface.
		 * All of its gory details can then be seen.
		 *
		 * If false, the visual layer is displayed as collapsed in the user interface.
		 * In this case, only basic information about the visual layer is displayed.
		 */
		bool
		is_expanded() const;

		/**
		 * Sets whether the visual layer is expanded in the user interface or not.
		 */
		void
		set_expanded(
				bool expanded = true);

		/**
		 * Toggles whether the visual layer is expanded in the user interface or not.
		 */
		void
		toggle_expanded();

		/**
		 * Returns whether the visual layer is rendered onto the viewport.
		 *
		 * This state does not affect whether the underlying layer is processed at the
		 * app-logic level or not.
		 */
		bool
		is_visible() const;

		/**
		 * Sets whether the visual layer is rendered onto the viewport.
		 */
		void
		set_visible(
				bool visible = true);

		/**
		 * Toggles whether the visual layer is rendered onto the viewport.
		 */
		void
		toggle_visible();

		/**
		 * Returns the automatically generated name for this layer. The automatically
		 * generated name is based on the inputs to this layer.
		 *
		 * Use @a get_custom_name in preference to the automatically generated name if
		 * it is set.
		 */
		QString
		get_generated_name() const;

		/**
		 * Returns the custom name explicitly given to this layer. If no custom name
		 * has been set, returns boost::none.
		 */
		const boost::optional<QString> &
		get_custom_name() const;

		/**
		 * Sets the custom name for this layer. To clear the custom name, provide
		 * boost::none as the @a custom_name.
		 */
		void
		set_custom_name(
				const boost::optional<QString> &custom_name);

		/**
		 * Returns the custom name if it is set, or the automatically generated name
		 * if the custom name is not set.
		 */
		QString
		get_name() const;

	private:

		void
		emit_layer_modified();

		/**
		 * The reconstruct graph layer for which this is the counterpart in the
		 * presentation application tier.
		 */
		GPlatesAppLogic::Layer d_layer;

		/**
		 * The index of the rendered geometry layer to which this visual layer will
		 * place rendered geometries.
		 *
		 * This index is used to represent this visual layer in the ordering of
		 * visual layers for visualisation purposes.
		 */
		const GPlatesViewOperations::RenderedGeometryCollection::child_layer_index_type
			d_rendered_geometry_layer_index;

		/**
		 * A pointer to the rendered geometry layer to which this visual layer will
		 * place rendered geometries.
		 *
		 * We acquire ownership of rendered geometry layer, i.e. when this object is
		 * destroyed, the rendered geometry layer will also be destroyed.
		 */
		const GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type
			d_rendered_geometry_layer;

		VisualLayers &d_visual_layers;

		/**
		 * Whether this visual layer is displayed as expanded in the user interface.
		 */
		bool d_expanded;

		/**
		 * Whether this visual layer is rendered onto the viewport.
		 */
		bool d_visible;

		/**
		 * The name that the user has explicitly given to this layer, if any.
		 */
		boost::optional<QString> d_custom_name;

		/**
		 * Each visual layer has a unique number that is used as a last resort to
		 * generate a name for the visual layer, if the usual methods for generating
		 * a name fail.
		 */
		int d_layer_number;
	};
}

#endif // GPLATES_PRESENTATION_VISUALLAYER_H
