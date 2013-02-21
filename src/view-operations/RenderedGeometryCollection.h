/* $Id$ */

/**
 * \file 
 * Interface for managing RenderedGeometry objects.
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

#ifndef GPLATES_VIEWOPERATIONS_RENDEREDGEOMETRYCOLLECTION_H
#define GPLATES_VIEWOPERATIONS_RENDEREDGEOMETRYCOLLECTION_H

#include <vector>
#include <list>
#include <stack>
#include <bitset>
#include <boost/any.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/foreach.hpp>
#include <QObject>

#include "RenderedGeometryLayer.h"

namespace GPlatesViewOperations
{
	template<class T> class ConstRenderedGeometryCollectionVisitor;
	template<class T> class RenderedGeometryCollectionVisitor;
	class RenderedGeometryLayer;

	/**
	 * Manages the rendered geometry system for a particular view.
	 *
     * * All rendering goes through RenderedGeometryCollection. It has main layers
	 *   (eg, digitisation, reconstruction – same as before) and can have child layers if needed.
	 *   This will be used by the move vertex digitise tool to draw the underlying geometry in
	 *   one child layer and the vertices to be grabbed/moved in another layer on top.
	 *   The layers should guarantee the draw order (eg, no z-buffer-fighting artifacts).
     * * RenderedGeometryCollection has a visitor than can visit the main layers, their child
	 *   layers and the RenderedGeometry objects in those layers.
     * * RenderedGeometryCollection notifies any observers whenever any changes have been made
	 *   to it or its layers (that way we don’t have to make manual calls to update the canvas
	 *   whenever we think it needs updating).
     * * RenderedGeometryCollection::UpdateGuard is just an optimisation that blocks this update
	 *   signal until the end of the current scope (that way can make multiple changes to
	 *   RenderedGeometryCollection and have only one update signal and hence only one redraw
	 *   of canvas). Forgetting to put the guard in doesn’t cause program error. It just means
	 *   the canvas is redrawn more times than needed. The update guards can be nested so putting
	 *   too many in is not a problem. For nested guards, only when the highest-level guard exits
	 *   its scope does the update signal get emitted.
     * * Currently GlobeCanvas listens to signals from RenderedGeomeryCollection and redraws when
	 *   a change is made.
     * * Globe in GlobeCanvas draws the RenderedGeometryCollection by visiting it with a single visitor.
     * * The map canvas or view could listen to RenderedGeometryCollection and clear all the
	 *   objects added to QGraphicsScene and re-add them by traversing the collection with a visitor.
     * * RenderedGeometryFactory is used to create RenderedGeometry objects which are added to
	 *   RenderedGeometryCollection.
     * * RenderedGeometryFactory contains global functions for creating different types of
	 *   RenderedGeometry objects.
     * * RenderedGeometry is a pimpl pattern (it’s basically just a boost shared_ptr so it can
	 *   be copied around and it hides the implementation type).
     * * Only way to get RenderedGeometry implementation type is to visit it.
     * * RenderedPointOnSphere is an implementation type of RenderedGeometry that is used to
	 *   draw a point. The factory creation function takes a point size hint and a colour.
	 *   There are also implementations for multipoint, polyline and polygon.
     * * RenderedReconstructionGeometry is an implementation type of RenderedGeometry that is
	 *   used to wrap a reconstruction geometry. This is useful for performing intersection tests
	 *   only on those geometries that are rendered.
     * * A RenderedGeometryLayer is just a simple container of RenderedGeometry objects.
     * * Each RenderedGeometryLayer gets drawn to a separate OpenGL depth layer to ensure that
	 *   RenderedGeometry objects from one container don't overlap or interleave visually with
	 *   RenderedGeometry objects from another container.
     * * Another concept of layer at a higher scope is a main layer like RECONSTRUCTION_LAYER,
	 *   DIGITISATION_CANVAS_TOOL_WORKFLOW_LAYER, etc. These are enumerations in the RenderedGeometryCollection.
     * * Within each main layer there's a single default main RenderedGeometryLayer object that
	 *   doesn't need to be created (it's embedded inside the RenderedGeometryCollection).
     * * Also optionally there can be one or more child RenderedGeometryLayer objects
	 *   (these need to be explicitly created though).
     * * Most uses will just need one RenderedGeometryLayer container per main layer.
	 *   For example, RECONSTRUCTION_LAYER has only one container so it uses the single
	 *   default main RenderedGeometryLayer belonging to RECONSTRUCTION_LAYER and draws
	 *   all its rendered geometries into that.
     * * Some uses require multiple containers per layer. For example, the move vertex tool
	 *   has one container for the base geometry and another container for the yellow highlights
	 *   drawn on top of base geometry - in which case it needs to explicitly create the
	 *   child containers. It doesn't actually use the single default main RenderedGeometryLayer
	 *   belonging to DIGITISATION_CANVAS_TOOL_WORKFLOW_LAYER - it could but it just creates child layers of
	 *   DIGITISATION_CANVAS_TOOL_WORKFLOW_LAYER instead. And the reason for multiple containers here is so the
	 *   yellow highlights in one container get drawn on top of the base geometry
	 *   in the other container.
     * * Regarding activation you can activate an entire layer (such as RECONSTRUCTION_LAYER)
	 *   and you can also activate each RenderedGeometryLayer container.
     * * If you activate an individual container it still won't get rendered unless you also
	 *   activate the entire layer that it belongs to. So in the move vertex tool
	 *   example above, we can activate the child RenderedGeometryLayer objects but if
	 *   the DIGITISATION_CANVAS_TOOL_WORKFLOW_LAYER is not active then none of the child RenderedGeometryLayer
	 *   objects will get rendered.
     * * The main reason for having activation/deactivation of an entire layer (such as
	 *   DIGITISATION_CANVAS_TOOL_WORKFLOW_LAYER) is to avoid having to deactivate each RenderedGeometryLayer container
	 *   within it when switching to a different canvas tool.
     * * AddPointDigitiseOperation is used to add a point when digitising and is used by both
	 *   globe and map tools. There are other similar operations for
	 *   deleting, moving, inserting vertices.
     * * Proximity queries can be performed on RenderedGeometry objects.
	*/
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
			// All reconstructed geometries, etc get rendered into this layer...
			RECONSTRUCTION_LAYER,

			// Canvas tool workflow layers...
			FEATURE_INSPECTION_CANVAS_TOOL_WORKFLOW_LAYER,
			DIGITISATION_CANVAS_TOOL_WORKFLOW_LAYER,
			TOPOLOGY_CANVAS_TOOL_WORKFLOW_LAYER,
			POLE_MANIPULATION_CANVAS_TOOL_WORKFLOW_LAYER,
			SMALL_CIRCLE_CANVAS_TOOL_WORKFLOW_LAYER,

			// Hellinger tool layer. This is not (yet) a canvas tool, but probably
			// will be in the future. At which point we can rename it to fit the pattern
			// above.
			HELLINGER_TOOL_LAYER,

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
		 * Typedef for a sequence of child layer indices.
		 */
		typedef std::list<child_layer_index_type> child_layer_index_seq_type;

		/**
		 * Typedef for a handle that owns a child layer.
		 */
		typedef boost::shared_ptr<RenderedGeometryLayer> child_layer_owner_ptr_type;

		//! Specifies all main layers.
		static const main_layers_update_type ALL_MAIN_LAYERS;

		RenderedGeometryCollection();

		~RenderedGeometryCollection();

		/**
		 * Sets the viewport zoom factor.
		 * It is assumed that a value of 1.0 means the globe almost exactly fills
		 * one of the viewport dimensions and values greater than 1.0 zoom in on the globe
		 * (this is how @a ViewportZoom behaves).
		 */
		void
		set_viewport_zoom_factor(
				const double &viewport_zoom_factor);

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
		 * Same as the other overload of @a create_child_rendered_layer except
		 * the density of some types of rendered geometries is uniformly spaced.
		 *
		 * The types of rendered geometries that are affected currently includes
		 * anything that has a point representation such as a rendered point on sphere
		 * and rendered direction arrow (starting point). Other rendered geometry types
		 * are treated the same as if we'd created a normal rendered geometry layer.
		 *
		 * The uniform spacing is such that only one geometry of potentially
		 * multiple geometries in a sample bin is rendered (this being the closest
		 * to the sample bin centre). The sample bins are all designed to cover the
		 * globe and have roughly equal area. The dimension of each sample bin extends
		 * roughly @a ratio_zoom_dependent_bin_dimension_to_globe_radius times the
		 * globe's radius when the globe is fully visible in the viewport window.
		 * As the zoom changes the apparent projected size of the sample bins onto
		 * the viewport remains constant.
		 *
		 * If using any of this type of child rendered layer then you must call
		 * @a set_viewport_zoom_factor whenever the zoom changes.
		 */
		child_layer_index_type
		create_child_rendered_layer(
				MainLayerType parent_layer,
				float ratio_zoom_dependent_bin_dimension_to_globe_radius);

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
		 * Yet another convenience method - creates child rendered layer
		 * and transfers ownership to returned pointer type.
		 *
		 * Basically calls @a create_child_rendered_layer and then
		 * @a transfer_ownership_of_child_rendered_layer.
		 */
		child_layer_owner_ptr_type
		create_child_rendered_layer_and_transfer_ownership(
				MainLayerType parent_layer);

		/**
		 * Same as the other overload of @a create_child_rendered_layer_and_transfer_ownership
		 * except the density of some types of rendered geometries is uniformly spaced.
		 *
		 * See @a create_child_rendered_layer for more details.
		 */
		child_layer_owner_ptr_type
		create_child_rendered_layer_and_transfer_ownership(
				MainLayerType parent_layer,
				float ratio_zoom_dependent_bin_dimension_to_globe_radius);

		/**
		 * Get the @a RenderedGeometryLayer corresponding to specified child layer.
		 */
		RenderedGeometryLayer *
		get_child_rendered_layer(
				child_layer_index_type);

		/**
		 * Get the sequence of child layer indices for the given parent layer.
		 * This sequence is the default order in which child layers are visited.
		 */
		const child_layer_index_seq_type &
		get_child_rendered_layer_indices(
				MainLayerType parent_layer) const;

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
		template<class ForwardReadableRange>
		void
		accept_visitor(
				ConstRenderedGeometryCollectionVisitor<ForwardReadableRange> &) const;

		/**
		 * Recursively visit the main rendered layers, their child rendered
		 * layers and the @a RenderedGeometry objects in them.
		 *
		 * Same as @a accept_visitor for const visitor except visits a
		 * non-const visitor.
		 */
		template<class ForwardReadableRange>
		void
		accept_visitor(
				RenderedGeometryCollectionVisitor<ForwardReadableRange> &);

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


		//@{
		/**
		 * Delays signaling updates to observers for all registered collections.
		 *
		 * This essentially calls @a begin_update_collection for each @a RenderedGeometryCollection
		 * that currently exists (each collection internally registers itself with a manager).
		 */
		static
		void
		begin_update_all_registered_collections();

		static
		void
		end_update_all_registered_collections();
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

	public Q_SLOTS:
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

	Q_SIGNALS:
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

		class RenderedGeometryLayerManager
		{
		public:
			~RenderedGeometryLayerManager();

			RenderedGeometryLayer *
			get_rendered_geometry_layer(
					child_layer_index_type);

			const RenderedGeometryLayer *
			get_rendered_geometry_layer(
					child_layer_index_type) const;

			/**
			 * Creates a new @a RenderedGeometryLayer and returns an index
			 * that can be used to @a get_rendered_geometry_layer().
			 */
			child_layer_index_type
			create_rendered_geometry_layer(
					MainLayerType main_layer,
					const double &current_viewport_zoom_factor);

			/**
			 * Same as other overloaded @a create_rendered_geometry_layer except
			 * creates a zoom-dependent rendered geometry layer instead of a normal one.
			 */
			child_layer_index_type
			create_rendered_geometry_layer(
					MainLayerType main_layer,
					float ratio_zoom_dependent_bin_dimension_to_globe_radius,
					const double &current_viewport_zoom_factor);

			/**
			 * Destroys the rendered geometry layer specified.
			 */
			void
			destroy_rendered_geometry_layer(
					child_layer_index_type);

		private:
			typedef RenderedGeometryLayer *rendered_geometry_layer_ptr_type;
			typedef std::vector<rendered_geometry_layer_ptr_type> rendered_geometry_layer_seq_type;
			typedef std::stack<child_layer_index_type> available_child_layer_indices_type;

			rendered_geometry_layer_seq_type d_layer_storage;
			child_layer_index_seq_type d_layers;
			available_child_layer_indices_type d_layers_available_for_reuse;

			//! Allocate a handle for a rendered geometry layer about to be created.
			child_layer_index_type
			allocate_layer_index();

			//! Deallocate a handle used by a rendered geometry layer that was just destroyed.
			void
			deallocate_layer_index(
					child_layer_index_type);
		};

		struct MainLayer
		{
			MainLayer(
					MainLayerType main_layer_type,
					const double &viewport_zoom_factor);

			typedef boost::shared_ptr<RenderedGeometryLayer> rendered_geom_layer_ptr_type;

			rendered_geom_layer_ptr_type d_rendered_geom_layer;
			child_layer_index_seq_type d_child_layer_index_seq;
		};

		/**
		 * Typedef for sequence of main rendered geometry layers.
		 */
		typedef std::vector<MainLayer> main_layer_seq_type;

		/**
		 * Current viewport zoom factor.
		 */
		double d_current_viewport_zoom_factor;

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
		boost::mutex d_update_collection_depth_mutex;

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

		template<
			class RenderedGeometryLayerType,
			class ChildLayerIndexRangeType,
			class RenderedGeometryCollectionType,
			class RenderedGeometryCollectionVisitorType>
		static
		void
		accept_visitor_internal(
				RenderedGeometryCollectionVisitorType &visitor,
				RenderedGeometryCollectionType &rendered_geom_collection);

		template<
			class RenderedGeometryLayerType,
			class ChildLayerIndexRangeType,
			class RenderedGeometryCollectionType,
			class RenderedGeometryCollectionVisitorType>
		static
		void
		visit_main_rendered_layer(
				RenderedGeometryCollectionVisitorType &visitor,
				MainLayerType main_layer_type,
				RenderedGeometryCollectionType &rendered_geom_collection);

		template<
			class RenderedGeometryLayerType,
			class ChildLayerIndexRangeType,
			class RenderedGeometryCollectionType,
			class RenderedGeometryCollectionVisitorType>
		static
		void
		visit_main_rendered_layer(
				RenderedGeometryCollectionVisitorType &visitor,
				RenderedGeometryCollectionType &rendered_geom_collection,
				const ChildLayerIndexRangeType &children_range);

		template<
			class RenderedGeometryLayerType,
			class RenderedGeometryCollectionVisitorType>
		static
		void
		visit_rendered_geometry_layer(
				RenderedGeometryCollectionVisitorType &visitor,
				RenderedGeometryLayerType &rendered_geom_layer);

		/**
		 * Does everything required to connect a newly created child rendered layer
		 * to its parent layer.
		 */
		void
		connect_child_rendered_layer_to_parent(
				const child_layer_index_type child_layer_index,
				MainLayerType parent_layer);

		/**
		 * Observe specified @a RenderedGeometryLayer for updates.
		 */
		void
		connect_to_rendered_geometry_layer_signal(
				RenderedGeometryLayer *rendered_geom_layer);
	};
	
	
	template<
		class RenderedGeometryLayerType,
		class RenderedGeometryCollectionVisitorType>
	void
	RenderedGeometryCollection::visit_rendered_geometry_layer(
			RenderedGeometryCollectionVisitorType &visitor,
			RenderedGeometryLayerType &rendered_geom_layer)
	{
		// Ask the visitor if it wants to visit this RenderedGeometryLayer.
		// It can query the active status of this RenderedGeometryLayer to decide.
		if (visitor.visit_rendered_geometry_layer(rendered_geom_layer))
		{
			rendered_geom_layer.accept_visitor(visitor);
		}
	}


	template<
		class RenderedGeometryLayerType,
		class ChildLayerIndexRangeType,
		class RenderedGeometryCollectionType,
		class RenderedGeometryCollectionVisitorType>
	void
	RenderedGeometryCollection::visit_main_rendered_layer(
			RenderedGeometryCollectionVisitorType &visitor,
			RenderedGeometryCollectionType &rendered_geom_collection,
			const ChildLayerIndexRangeType &children_range)
	{
		BOOST_FOREACH(child_layer_index_type child_layer_index, children_range)
		{
			RenderedGeometryLayerType *child_rendered_geom_layer =
				rendered_geom_collection.d_rendered_geometry_layer_manager.get_rendered_geometry_layer(
						child_layer_index);

			// Visit child rendered geometry layer.
			visit_rendered_geometry_layer(
					visitor,
					*child_rendered_geom_layer);
		}
	}


	template<
		class RenderedGeometryLayerType,
		class ChildLayerIndexRangeType,
		class RenderedGeometryCollectionType,
		class RenderedGeometryCollectionVisitorType>
	void
	RenderedGeometryCollection::visit_main_rendered_layer(
			RenderedGeometryCollectionVisitorType &visitor,
			MainLayerType main_layer_type,
			RenderedGeometryCollectionType &rendered_geom_collection)
	{
		// Ask the visitor if it wants to visit this main layer.
		// It can query the active status of this main layer and use that to decide.
		if (visitor.visit_main_rendered_layer(rendered_geom_collection, main_layer_type))
		{
			const MainLayer &main_layer = rendered_geom_collection.d_main_layer_seq[main_layer_type];

			// Visit the main render layer first.
			RenderedGeometryLayerType &main_rendered_geom_layer = *main_layer.d_rendered_geom_layer;

			visit_rendered_geometry_layer(
					visitor,
					main_rendered_geom_layer);

			// Visit the child render layers second.

			// First, determine if the visitor would like to use a custom order of visitation.
			boost::optional<ChildLayerIndexRangeType> custom_child_layers_order =
				visitor.get_custom_child_layers_order(main_layer_type);
			if (custom_child_layers_order)
			{
				visit_main_rendered_layer<RenderedGeometryLayerType>(
						visitor,
						rendered_geom_collection,
						*custom_child_layers_order);
			}
			else
			{
				// If no custom order of visitation, use order of creation.
				visit_main_rendered_layer<RenderedGeometryLayerType>(
						visitor,
						rendered_geom_collection,
						main_layer.d_child_layer_index_seq);
			}
		}
	}


	template<
		class RenderedGeometryLayerType,
		class ChildLayerIndexRangeType,
		class RenderedGeometryCollectionType,
		class RenderedGeometryCollectionVisitorType>
	void
	GPlatesViewOperations::RenderedGeometryCollection::accept_visitor_internal(
			RenderedGeometryCollectionVisitorType &visitor,
			RenderedGeometryCollectionType &rendered_geom_collection)
	{
		// Visit each main rendered layer.
		main_layer_seq_type::size_type main_layer_index;
		for (
			main_layer_index = 0;
			main_layer_index != rendered_geom_collection.d_main_layer_seq.size();
			++main_layer_index)
		{
			// The static_cast is a bit dangerous.
			// Is there a better way to iterate over the enumerations ?
			MainLayerType main_layer_type =
				static_cast<MainLayerType>(main_layer_index);

			visit_main_rendered_layer<RenderedGeometryLayerType, ChildLayerIndexRangeType>(
					visitor,
					main_layer_type,
					rendered_geom_collection);
		}
	}


	template<class ForwardReadableRange>
	void
	RenderedGeometryCollection::accept_visitor(
			ConstRenderedGeometryCollectionVisitor<ForwardReadableRange> &visitor) const
	{
		accept_visitor_internal<const RenderedGeometryLayer, ForwardReadableRange>(visitor, *this);
	}


	template<class ForwardReadableRange>
	void
	RenderedGeometryCollection::accept_visitor(
			RenderedGeometryCollectionVisitor<ForwardReadableRange> &visitor)
	{
		accept_visitor_internal<RenderedGeometryLayer, ForwardReadableRange>(visitor, *this);
	}
}

#endif // GPLATES_VIEWOPERATIONS_RENDEREDGEOMETRYCOLLECTION_H
