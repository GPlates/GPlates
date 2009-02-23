/* $Id$ */

/**
 * \file 
 * Interface for managing RenderedGeometry objects.
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

#ifndef GPLATES_VIEWOPERATIONS_RENDEREDGEOMETRYCOLLECTION_H
#define GPLATES_VIEWOPERATIONS_RENDEREDGEOMETRYCOLLECTION_H

#include <vector>
#include <list>
#include <stack>
#include <bitset>
#include <boost/any.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <QObject>

#include "RenderedGeometryLayer.h"


namespace GPlatesViewOperations
{
	class ConstRenderedGeometryCollectionVisitor;
	class RenderedGeometryCollectionVisitor;
	class RenderedGeometryLayer;

	class RenderedGeometryCollection :
		public QObject,
		private boost::noncopyable
	{
		Q_OBJECT

	public:
		/**
		 * The main render layers.
		 *
		 * The order of enumerations is the order in which a
		 * @a ConstRenderedGeometryCollectionVisitor object will visit the
		 * main render layers.
		 */
		enum MainLayerType
		{
			RECONSTRUCTION_LAYER,
			COMPUTATIONAL_MESH_LAYER,
			DIGITISATION_LAYER,
			GEOMETRY_FOCUS_HIGHLIGHT_LAYER,
			GEOMETRY_FOCUS_MANIPULATION_LAYER,
			POLE_MANIPULATION_LAYER,
			CREATE_TOPOLOGY_LAYER,
			TOPOLOGY_TOOL_LAYER,
			MOUSE_MOVEMENT_LAYER,

			NUM_LAYERS // Must be the last enum.
		};

		/**
		 * A std::bitset for setting which main layers are orthogonal.
		 */
		typedef std::bitset<NUM_LAYERS> orthogonal_main_layers_type;

		/**
		 * A std::bitset for querying which main layers were updated.
		 * Used by the @a collection_was_updated signal.
		 */
		typedef std::bitset<NUM_LAYERS> main_layers_update_type;

		/**
		 * Typedef for an index to a child rendered layer.
		 */
		typedef unsigned int child_layer_index_type;

		/**
		 * Typedef for a handle that owns a child layer.
		 */
		typedef boost::shared_ptr<RenderedGeometryLayer> child_layer_owner_ptr_type;

		//! Specifies all main layers.
		static const main_layers_update_type ALL_MAIN_LAYERS;

		RenderedGeometryCollection();

		~RenderedGeometryCollection();

		/**
		 * Get the @a RenderedGeometryLayer corresponding to specified main layer.
		 *
		 * This is a convenient place to add @a RenderedGeometry objects when you don't
		 * need to use child layers.
		 */
		RenderedGeometryLayer *
		get_main_rendered_layer(
				MainLayerType);

		/**
		 * Create a rendered layer that is a child of the specified main rendered layer.
		 *
		 * Child layers are used when you need to order groups of @a RenderedGeometry
		 * objects within a main layer.
		 *
		 * The order in which child layers are created within a parent main layer is
		 * the order in which the child layers will be visited when @a accept_visitor()
		 * is called.
		 */
		child_layer_index_type
		create_child_rendered_layer(
				MainLayerType parent_layer);

		/**
		 * Destroy a rendered layer created with @a create_child_rendered_layer().
		 */
		void
		destroy_child_rendered_layer(
				child_layer_index_type,
				MainLayerType parent_layer);

		/**
		 * Transfers ownership of a child rendered layer to the object returned.
		 *
		 * Object returned is a pointer that dereferences a @a RenderedGeometryLayer.
		 *
		 * This removes the need to explicitly call @a destroy_child_rendered_layer().
		 * When the last of the returned object and any of its copies is destroyed
		 * then @a destroy_child_rendered_layer() will be called automatically.
		 *
		 * @param  child_layer_index references child layer we are transferring ownership from.
		 * @return object that owns the child layer referenced by @a child_layer_index.
		 */
		child_layer_owner_ptr_type
		transfer_ownership_of_child_rendered_layer(
				child_layer_index_type child_layer_index,
				MainLayerType parent_layer);

		/**
		 * Yet another convenient method - creates child rendered layer
		 * and transfers ownership to returned pointer type.
		 *
		 * Basically calls @a create_child_rendered_layer and then
		 * @a transfer_ownership_of_child_rendered_layer.
		 */
		child_layer_owner_ptr_type
		create_child_rendered_layer_and_transfer_ownership(
				MainLayerType parent_layer);

		/**
		 * Get the @a RenderedGeometryLayer corresponding to specified child layer.
		 */
		RenderedGeometryLayer *
		get_child_rendered_layer(
				child_layer_index_type);

		/**
		 * Set a specific main layer as active/inactive.
		 *
		 * If @a active is true and @a main_layer_type is in the orthogonal
		 * main layers group (set by @a set_orthogonal_main_layers) then any
		 * other layers in that group are deactivated.
		 * If @a active is false then this does not apply.
		 * This is the only method affected by @a set_orthogonal_main_layers.
		 */
		void
		set_main_layer_active(
				MainLayerType main_layer_type,
				bool active = true);

		/**
		 * See if a specific main layer is currently active.
		 */
		bool
		is_main_layer_active(
				MainLayerType main_layer_type) const;

		/**
		 * Specify which main layers are orthogonal to each other in terms of activation.
		 * Use @a std::bitset operations to specify the layer flags.
		 *
		 * Only affects @a set_main_layer_active method in the following way - if any
		 * main layer is activated that is in this group then the others in the group
		 * are automatically deactivated - nothing happens if a main layer is deactivated though.
		 */
		void
		set_orthogonal_main_layers(
				orthogonal_main_layers_type orthogonal_main_layers);

		/**
		 * Returns group of main layers that are orthogonal to each other in activation terms.
		 */
		orthogonal_main_layers_type
		get_orthogonal_main_layers() const;

		/**
		 * Opaque type contains main layer active state for all main layers.
		 * Can test for active status of any main layer but cannot set active status.
		 */
		class MainLayerActiveState
		{
		public:
			typedef boost::any impl_type;

			MainLayerActiveState(
					impl_type);

			//! Is specified main layer active.
			bool
			is_active(
					MainLayerType) const;

			impl_type
			get_impl() const;

		private:
			impl_type d_impl;
		};

		/**
		 * Capture active status of all main layers for later restore.
		 * Does not capture the active status of main or child
		 * @a RenderedGeometryLayer objects in any main layers.
		 */
		MainLayerActiveState
		capture_main_layer_active_state() const;

		/**
		 * Restores active status of all main layers.
		 * Does not affect the active status of main or child
		 * @a RenderedGeometryLayer objects in any main layers.
		 */
		void
		restore_main_layer_active_state(
				MainLayerActiveState main_layer_active_state);


		/**
		 * Recursively visit the main rendered layers, their child rendered
		 * layers and the @a RenderedGeometry objects in them.
		 *
		 * The main layers are visited in the order of their @a MainLayerType
		 * enumerations.
		 * Within a main layer its @a RenderedGeometry objects are visited first and
		 * then any child layers are visited next (in the order they were created).
		 * Within a child layer its @a RenderedGeometry objects are visited in the order
		 * in which they were added.
		 *
		 * If a main layer is not active then the @a ConstRenderedGeometryCollectionVisitor
		 * can test this in its @a visit_main_rendered_layer() method and return false
		 * to indicate it doesn't want the main layer to be traversed and visited.
		 *
		 * NOTE: It is up to the visitor to determine which main layers will get visited.
		 *       This is useful if some visitors don't care about active status - they
		 *       don't have to change the active status and restore it. The active status
		 *       is not really a property of the rendered geometry collection - it is
		 *       really extrinsic state of the visitor - but it is useful to store it
		 *       on the rendered geometry collection.
		 */
		void
		accept_visitor(
				ConstRenderedGeometryCollectionVisitor &) const;

		/**
		 * Recursively visit the main rendered layers, their child rendered
		 * layers and the @a RenderedGeometry objects in them.
		 *
		 * Same as @a accept_visitor for const visitor except visits a
		 * non-const visitor.
		 *
		 */
		void
		accept_visitor(
				RenderedGeometryCollectionVisitor &);

		//@{
		/**
		 * Delays signaling updates to observers.
		 *
		 * Observers of our update signal(s) are not notified
		 * between @a begin_update_collection() and @a end_update_collection().
		 * Notification is delayed until @a end_update_collection().
		 * This allows us to make multiple modifications to the collection followed
		 * by a single update notification to any observers.
		 *
		 * These scope blocks can be nested.
		 *
		 * NOTE: it is not necessary to call these methods - they are just an
		 * optimisation to reduce update notifications.
		 */
		void
		begin_update_collection();

		void
		end_update_collection();
		//@}

		/**
		 * A convenience structure for automating calls to
		 * @a begin_update_collection() and @a end_update_collection()
		 * in a scope block.
		 * If there happen to be multiple @a RenderedGeometryCollection objects
		 * in existence then calls will be made on them all (ie, updates will be blocked
		 * on all existing @a RenderedGeometryCollection objects while the @a UpdateGuard
		 * is in scope).
		 */
		struct UpdateGuard :
			public boost::noncopyable
		{
			UpdateGuard();

			~UpdateGuard();
		};

	public slots:
		// NOTE: all signals/slots should use namespace scope for all arguments
		//       otherwise differences between signals and slots will cause Qt
		//       to not be able to connect them at runtime.

		/**
		 * Called when a @a RenderedGeometryLayer is modified.
		 */
		void
		rendered_geometry_layer_was_updated(
				GPlatesViewOperations::RenderedGeometryLayer &,
				GPlatesViewOperations::RenderedGeometryLayer::user_data_type);

	signals:
		// NOTE: all signals/slots should use namespace scope for all arguments
		//       otherwise differences between signals and slots will cause Qt
		//       to not be able to connect them at runtime.

		/**
		 * Signal is emitted whenever this rendered geometry collection has been
		 * updated.
		 *
		 * This includes:
		 * - any @a RenderedGeometry objects added to a @a RenderedGeometryLayer object.
		 * - clearing of @a RenderedGeometry objects from a @a RenderedGeometryLayer object.
		 * - changing a main layer active status.
		 * - changing a @a RenderedGeometryObject active status.
		 * - creating or destroying a child layer.
		 *
		 * @param rendered_geom_collection reference to this @a RenderedGeometryCollection.
		 * @param main_layers_updated a std::bitset indicating which main layers were updated.
		 */
		void
		collection_was_updated(
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
				GPlatesViewOperations::RenderedGeometryCollection::main_layers_update_type main_layers_updated);

	private:
		/**
		 * Type contains main layer active state.
		 */
		typedef std::bitset<NUM_LAYERS> main_layer_active_state_internal_type;

		typedef unsigned int RenderedGeometryLayerIndex;

		class RenderedGeometryLayerManager
		{
		public:
			RenderedGeometryLayer *
			get_rendered_geometry_layer(
					RenderedGeometryLayerIndex);

			const RenderedGeometryLayer *
			get_rendered_geometry_layer(
					RenderedGeometryLayerIndex) const;

			/**
			 * Creates a new @a RenderedGeometrylayer and returns an index
			 * that can be used to @a get_rendered_geometry_layer().
			 */
			RenderedGeometryLayerIndex
			create_rendered_geometry_layer(
					MainLayerType main_layer);

			/**
			 * Destroys the rendered geometry layered specified.
			 */
			void
			destroy_rendered_geometry_layer(
					RenderedGeometryLayerIndex);

		private:
			typedef RenderedGeometryLayer *rendered_geometry_layer_ptr_type;
			typedef std::vector<rendered_geometry_layer_ptr_type> rendered_geometry_layer_seq_type;
			typedef std::list<child_layer_index_type> child_layer_index_seq_type;
			typedef std::stack<child_layer_index_type> available_child_layer_indices_type;

			rendered_geometry_layer_seq_type d_layer_storage;
			child_layer_index_seq_type d_layers;
			available_child_layer_indices_type d_layers_available_for_reuse;
		};

		struct MainLayer
		{
			MainLayer(
					MainLayerType main_layer_type);

			typedef std::list<RenderedGeometryLayerIndex> child_layer_index_seq_type;
			typedef boost::shared_ptr<RenderedGeometryLayer> rendered_geom_layer_ptr_type;

			rendered_geom_layer_ptr_type d_rendered_geom_layer;
			child_layer_index_seq_type d_child_layer_index_seq;
		};

		/**
		 * Typedef for sequence of main rendered geometry layers.
		 */
		typedef std::vector<MainLayer> main_layer_seq_type;

		/**
		 * Manages creation and destruction of @a RenderedGeometryLayer objects.
		 */
		RenderedGeometryLayerManager d_rendered_geometry_layer_manager;

		/**
		 * Sequence of main rendered layers.
		 */
		main_layer_seq_type d_main_layer_seq;

		/**
		 * Bitwise 'or' of main layer flags showing which are active.
		 */
		main_layer_active_state_internal_type d_main_layer_active_state;

		/**
		 * If any main layer is activated that is in this group then the others
		 * in the group are automatically deactivated.
		 */
		orthogonal_main_layers_type d_main_layers_orthogonal;

		/**
		 * Used by @a begin_update_collection and @a end_update_collection to
		 * keep track of the nested call depth.
		 */
		int d_update_collection_depth;

		/**
		 * Is true if an update to rendered geometry collection occurred
		 * inside a @a begin_update_collection / @a end_update_collection block.
		 */
		bool d_update_notify_queued;

		/**
		 * Keeps track of which main layers have been updated since the last
		 * @a collection_was_updated signal was emitted.
		 */
		main_layers_update_type d_main_layers_updated;

		/**
		 * Does the actual 'emit' of the @a collection_was_updated signal.
		 */
		void
		send_update_signal();

		/**
		 * Should we delay signaling our observers that we've been updated?
		 */
		bool
		delay_update_notification() const
		{
			return d_update_collection_depth > 0;
		}

		/**
		 * Signal to our observers that we've been updated.
		 *
		 * @param main_layer_type the main layer that was updated.
		 */
		void
		signal_update(
				MainLayerType main_layer_type);

		/**
		 * Signal to our observers that we've been updated.
		 *
		 * @param main_layers_updated specifies which main layers were updated.
		 */
		void
		signal_update(
				main_layers_update_type main_layers_updated);

		template <class RenderedGeometryCollectionVisitorType>
		void
		accept_visitor_internal(
				RenderedGeometryCollectionVisitorType &visitor);

		template <class RenderedGeometryCollectionVisitorType>
		void
		visit_main_rendered_layer(
				RenderedGeometryCollectionVisitorType &visitor,
				MainLayerType main_layer_type);

		template <class RenderedGeometryCollectionVisitorType>
		void
		visit_rendered_geometry_layer(
				RenderedGeometryCollectionVisitorType &visitor,
				RenderedGeometryLayer &rendered_geom_layer);

		/**
		 * Observe specified @a RenderedGeometryLayer for updates.
		 */
		void
		connect_to_rendered_geometry_layer_signal(
				RenderedGeometryLayer *rendered_geom_layer);
	};
}

#endif // GPLATES_VIEWOPERATIONS_RENDEREDGEOMETRYCOLLECTION_H
