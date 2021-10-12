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
 
#ifndef GPLATES_PRESENTATION_VISUALLAYERS_H
#define GPLATES_PRESENTATION_VISUALLAYERS_H

#include <list>
#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <QObject>

#include "VisualLayer.h"

#include "app-logic/FeatureCollectionFileState.h"
#include "app-logic/Layer.h"

#include "view-operations/RenderedGeometryCollection.h"


namespace GPlatesAppLogic
{
	class ApplicationState;
	class ReconstructGraph;
}

namespace GPlatesPresentation
{
	class ViewState;

	class VisualLayers :
			public QObject,
			private boost::noncopyable
	{
		Q_OBJECT

	public:

		/**
		 * Constructor.
		 */
		VisualLayers(
				GPlatesAppLogic::ApplicationState &application_state,
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection);

		/**
		 * Returns the number of visual layers.
		 */
		size_t
		size() const;

		/**
		 * Moves the layer at @a from_index to @a to_index.
		 *
		 * If the layer is moved down in the ordering (i.e. @a from_index is less than
		 * @a to_index), layers between from_index and to_index get shifted upwards.
		 *
		 * If the layer is moved up in the ordering (i.e. @a from_index is greater than
		 * @a to_index), layers between from_index and to_index get shifted downwards.
		 */
		void
		move_layer(
				size_t from_index,
				size_t to_index);

		/**
		 * Returns the visual layer that is at position @a index in the layer ordering.
		 */
		boost::weak_ptr<const VisualLayer>
		visual_layer_at(
				size_t index) const;

		/**
		 * Returns the visual layer that is at position @a index in the layer ordering.
		 */
		boost::weak_ptr<VisualLayer>
		visual_layer_at(
				size_t index);

		/**
		 * Returns the rendered geometry child layer index belonging to the visual
		 * layer at @a index.
		 */
		GPlatesViewOperations::RenderedGeometryCollection::child_layer_index_type
		child_layer_index_at(
				size_t index) const;

		/**
		 * Returns the rendered geometry child layer index belonging to the visual
		 * layer at @a index.
		 */
		GPlatesViewOperations::RenderedGeometryCollection::child_layer_index_type
		child_layer_index_at(
				size_t index);

		/**
		 * Returns the corresponding visual layer for the given @a layer.
		 *
		 * Returns an invalid weak pointer if @a layer has no corresponding visual layer.
		 */
		boost::weak_ptr<const VisualLayer>
		get_visual_layer(
				const GPlatesAppLogic::Layer &layer) const;

		/**
		 * Returns the corresponding visual layer for the given @a layer.
		 *
		 * Returns an invalid weak pointer if @a layer has no corresponding visual layer.
		 */
		boost::weak_ptr<VisualLayer>
		get_visual_layer(
				const GPlatesAppLogic::Layer &layer);

		/**
		 * Typedef for the container that stores the visual layers ordering.
		 */
		typedef std::vector<GPlatesViewOperations::RenderedGeometryCollection::child_layer_index_type>
			rendered_geometry_layer_seq_type;

		/**
		 * Returns the visual layers ordering as a sequence of rendered geometry layers indices.
		 */
		const rendered_geometry_layer_seq_type &
		get_layer_order() const;

		/**
		 * Typedef for const iterator over the ordering of visual layers.
		 *
		 * The order traversed by this iterator is the order in which the visual
		 * layers should be drawn, i.e. from back to front.
		 */
		typedef rendered_geometry_layer_seq_type::const_iterator const_iterator;

		/**
		 * Returns the 'begin' iterator for the visual layers ordering.
		 */
		const_iterator
		order_begin() const;

		/**
		 * Returns the 'end' iterator for the visual layers ordering.
		 */
		const_iterator
		order_end() const;

	public slots:

		// NOTE: all signals/slots should use namespace scope for all arguments
		//       otherwise differences between signals and slots will cause Qt
		//       to not be able to connect them at runtime.

		/**
		 * Creates rendered geometries for each active visual layer.
		 *
		 * Each visual layer has its own rendered geometry layer created inside the
		 * @a RenderedGeometryCollection (contained in the @a ViewState passed into the
		 * constructor). There rendered geometry layers are created inside the
		 * 'RECONSTRUCTION_LAYER' main rendered layer.
		 *
		 * NOTE: This won't perform a new reconstruction, it'll just iterate over the
		 * visual layers and convert any reconstruction geometries (created by the most
		 * recent reconstruction in @a ApplicationState) into rendered geometries
		 * (and remove the old rendered geometries from the individual rendered geometry layers).
		 *
		 * This call will automatically get triggered, however, when the @a ApplicationState
		 * performs a new reconstruction.
		 *
		 * This method can be explicitly called when render settings/styles have changed to
		 * avoid performing a new reconstruction when it's not necessary.
		 */
		void
		create_rendered_geometries();

	signals:

		/**
		 * Indicates that there has been a change in the ordering of layer indices
		 * from @a first_index to @a last_index, inclusive.
		 */
		void
		layer_order_changed(
				size_t first_index,
				size_t last_index);

		/**
		 * This signal is emitted just before a new visual layer is added.
		 *
		 * The @a index provided is the prospective index of the new visual layer in
		 * the ordering of visual layers.
		 */
		void
		layer_about_to_be_added(
				size_t index);

		/**
		 * This signal is emitted just after a new visual layer is added.
		 *
		 * The @a index provided is the index of the new visual layer in
		 * the ordering of visual layers.
		 */
		void
		layer_added(
				size_t index);

		/**
		 * This signal is emitted just after a new visual layer is added.
		 *
		 * Both layer_added variations are emitted, so it should only be necessary to
		 * connect to the signal with the most convenient form.
		 */
		void
		layer_added(
				boost::weak_ptr<GPlatesPresentation::VisualLayer> visual_layer);

		/**
		 * This signal is emitted just before a visual layer is removed.
		 *
		 * The @a index provided is the index of the visual layer that is to be
		 * removed in the ordering of visual layers.
		 */
		void
		layer_about_to_be_removed(
				size_t index);

		/**
		 * This signal is emitted just before a visual layer is removed.
		 *
		 * Both layer_about_to_be_removed variations are emitted, so it should only be
		 * necessary to connect to the signal with the most convenient form.
		 */
		void
		layer_about_to_be_removed(
				boost::weak_ptr<GPlatesPresentation::VisualLayer> visual_layer);

		/**
		 * This signal is emitted just after a visual layer is removed.
		 *
		 * The @a index provided is the former index of the visual layer that was
		 * removed in the ordering of visual layers.
		 */
		void
		layer_removed(
				size_t index);

		/**
		 * This signal is emitted just after a visual layer's underlying reconstruct
		 * graph layer is modified. This signal is also emitted when one of a visual
		 * layer's properties is modified.
		 *
		 * In particular, this signal is emitted after a change in the layer's
		 * activation or after an input connection is added or removed.
		 *
		 * This signal is also emitted when a visual layer is expanded or collapsed,
		 * or its visibility is toggled on or off.
		 *
		 * The @a index provided is the index of the visual layer in the ordering
		 * of visual layers.
		 */
		void
		layer_modified(
				size_t index);

	private slots:

		// NOTE: all signals/slots should use namespace scope for all arguments
		//       otherwise differences between signals and slots will cause Qt
		//       to not be able to connect them at runtime.

		void
		handle_layer_added(
				GPlatesAppLogic::ReconstructGraph &reconstruct_graph,
				GPlatesAppLogic::Layer layer);

		void
		handle_layer_about_to_be_removed(
				GPlatesAppLogic::ReconstructGraph &reconstruct_graph,
				GPlatesAppLogic::Layer layer);

		void
		handle_layer_activation_changed(
				GPlatesAppLogic::ReconstructGraph &reconstruct_graph,
				GPlatesAppLogic::Layer layer,
				bool activation);

		void
		handle_layer_added_input_connection(
				GPlatesAppLogic::ReconstructGraph &reconstruct_graph,
				GPlatesAppLogic::Layer layer,
				GPlatesAppLogic::Layer::InputConnection input_connection);

		void
		handle_layer_removed_input_connection(
				GPlatesAppLogic::ReconstructGraph &reconstruct_graph,
				GPlatesAppLogic::Layer layer);

		void
		handle_file_state_file_info_changed(
				GPlatesAppLogic::FeatureCollectionFileState &file_state,
				GPlatesAppLogic::FeatureCollectionFileState::file_reference file);

	private:

		/**
		 * Typedef for a shared pointer to a @a VisualLayer.
		 */
		typedef boost::shared_ptr<VisualLayer> visual_layer_ptr_type;

		/**
		 * Typedef for mapping a layer to a visual layer.
		 */
		typedef std::map<GPlatesAppLogic::Layer, visual_layer_ptr_type> visual_layer_map_type;

		/**
		 * Typedef for mapping a rendered geometry layer index to a visual layer.
		 */
		typedef std::map<
			GPlatesViewOperations::RenderedGeometryCollection::child_layer_index_type,
			boost::weak_ptr<VisualLayer>
		> index_map_type;

		void
		connect_to_application_state_signals();

		visual_layer_ptr_type
		create_visual_layer(
				const GPlatesAppLogic::Layer &layer);

		void
		add_layer(
				const GPlatesAppLogic::Layer &layer);

		void
		remove_layer(
				const GPlatesAppLogic::Layer &layer);

		void
		handle_layer_modified(
				const GPlatesAppLogic::Layer &layer);

		void
		refresh_all_layers();

		/**
		 * Returns the visual layer that owns the rendered geometry layer with the
		 * given index.
		 *
		 * Returns an invalid weak pointer if the index does not have a corresponding
		 * visual layer.
		 */
		boost::weak_ptr<const VisualLayer>
		get_visual_layer(
				GPlatesViewOperations::RenderedGeometryCollection::child_layer_index_type index) const;

		/**
		 * Returns the visual layer that owns the rendered geometry layer with the
		 * given index.
		 *
		 * Returns an invalid weak pointer if the index does not have a corresponding
		 * visual layer.
		 */
		boost::weak_ptr<VisualLayer>
		get_visual_layer(
				GPlatesViewOperations::RenderedGeometryCollection::child_layer_index_type index);

		/**
		 * Emits the layer_modified signal, if @a index is found in the layer ordering.
		 */
		void
		emit_layer_modified(
				GPlatesViewOperations::RenderedGeometryCollection::child_layer_index_type index);

		// VisualLayer causes VisualLayers to emit layer_modified.
		friend class VisualLayer;

		GPlatesAppLogic::ApplicationState &d_application_state;

		GPlatesViewOperations::RenderedGeometryCollection &d_rendered_geometry_collection;

		/**
		 * Record of all visual layers associated with application state layers.
		 *
		 * Each layer has its own rendered geometry layer so that draw order
		 * of the layers can be controlled.
		 */
		visual_layer_map_type d_visual_layers;

		/**
		 * A custom ordering of child layers in the RECONSTRUCTION_LAYER.
		 *
		 * Layers are stored in increasing z-order, i.e. when drawing these layers,
		 * start from the front and work towards the back.
		 */
		rendered_geometry_layer_seq_type d_layer_order;

		/**
		 * Associates rendered geometry collection layer indices with a visual layer.
		 */
		index_map_type d_index_map;

		/**
		 * The number that will be given to the next visual layer created.
		 */
		int d_next_visual_layer_number;
	};
}

#endif // GPLATES_PRESENTATION_VISUALLAYERS_H
