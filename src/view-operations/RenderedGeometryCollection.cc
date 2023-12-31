/* $Id$ */

/**
 * \file 
 * Interface for managing RenderedGeometry objects.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2008, 2009, 2010 The University of Sydney, Australia
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

#include "RenderedGeometryCollection.h"
#include "RenderedGeometryCollectionVisitor.h"
#include "global/GPlatesAssert.h"
#include "global/AssertionFailureException.h"
#include "utils/Singleton.h"

#include <boost/bind/bind.hpp>
#include <boost/noncopyable.hpp>
#include <list>
#include <QDebug>


namespace GPlatesViewOperations
{
	namespace
	{
		/**
		 * Singleton instance to keep track of @a RenderedGeometryCollection objects.
		 */
		class RenderedGeometryCollectionManager :
				public GPlatesUtils::Singleton<RenderedGeometryCollectionManager>
		{

			GPLATES_SINGLETON_CONSTRUCTOR_DEF(RenderedGeometryCollectionManager)

		public:
			/**
			 * Typedef for sequence of @a RenderedGeometryCollection pointers.
			 */
			typedef std::list<RenderedGeometryCollection *> collection_seq_type;

			void
			register_collection(
					RenderedGeometryCollection *rendered_geom_collection)
			{
				d_registered_collections.push_back(rendered_geom_collection);
			}

			void
			unregister_collection(
					RenderedGeometryCollection *rendered_geom_collection)
			{
				d_registered_collections.remove(rendered_geom_collection);
			}

			const collection_seq_type &
			get_registered_collections() const
			{
				return d_registered_collections;
			}

		private:
			collection_seq_type d_registered_collections;
		};
	}
}


const GPlatesViewOperations::RenderedGeometryCollection::main_layers_update_type
	GPlatesViewOperations::RenderedGeometryCollection::ALL_MAIN_LAYERS =
			~GPlatesViewOperations::RenderedGeometryCollection::main_layers_update_type();


GPlatesViewOperations::RenderedGeometryCollection::RenderedGeometryCollection() :
	d_current_viewport_zoom_factor(1.0),
	d_update_collection_depth(0),
	d_update_notify_queued(false)
{
	d_main_layer_seq.reserve(NUM_LAYERS);

	//
	// Connect each main layer's RenderedGeometryLayer signal
	// to our slot so we get notified when it is updated.
	//
	main_layer_seq_type::size_type main_layer_index;
	for (main_layer_index = 0;
		main_layer_index < NUM_LAYERS;
		++main_layer_index)
	{
		// The static_cast is a bit dangerous.
		// Is there a better way to iterate over the enumerations ?
		MainLayerType main_rendered_layer_type =
			static_cast<MainLayerType>(main_layer_index);

		d_main_layer_seq.push_back(
				MainLayer(main_rendered_layer_type, d_current_viewport_zoom_factor));

		// Get this main layer's RenderedGeometryLayer object.
		RenderedGeometryLayer *rendered_geom_layer =
			d_main_layer_seq[main_layer_index].d_rendered_geom_layer.get();

		// We'd like to know when this rendered geometry layer has been updated.
		connect_to_rendered_geometry_layer_signal(rendered_geom_layer);
	}

	// This should happen last in case an exception is thrown.
	RenderedGeometryCollectionManager::instance().register_collection(this);
}


GPlatesViewOperations::RenderedGeometryCollection::~RenderedGeometryCollection()
{
	RenderedGeometryCollectionManager::instance().unregister_collection(this);
}


void
GPlatesViewOperations::RenderedGeometryCollection::set_viewport_zoom_factor(
		const double &viewport_zoom_factor)
{
	d_current_viewport_zoom_factor = viewport_zoom_factor;

	//
	// Notify all zoom-dependent rendered geometry layers of the new zoom factor.
	//
	main_layer_seq_type::size_type main_layer_index;
	for (main_layer_index = 0;
		main_layer_index < NUM_LAYERS;
		++main_layer_index)
	{
		// Notify main layer - currently isn't necessary since only child layers can
		// be zoom-dependent (but this might change).
		RenderedGeometryLayer *main_layer =
				d_main_layer_seq[main_layer_index].d_rendered_geom_layer.get();
		main_layer->set_viewport_zoom_factor(d_current_viewport_zoom_factor);

		// Notify the child layers.
		child_layer_index_seq_type::iterator child_layer_iter;
		for (child_layer_iter = d_main_layer_seq[main_layer_index].d_child_layer_index_seq.begin();
			child_layer_iter != d_main_layer_seq[main_layer_index].d_child_layer_index_seq.end();
			++child_layer_iter)
		{
			const child_layer_index_type child_layer_index = *child_layer_iter;

			RenderedGeometryLayer *child_layer =
					d_rendered_geometry_layer_manager.get_rendered_geometry_layer(
							child_layer_index);

			child_layer->set_viewport_zoom_factor(d_current_viewport_zoom_factor);
		}
	}
}


GPlatesViewOperations::RenderedGeometryLayer *
GPlatesViewOperations::RenderedGeometryCollection::get_main_rendered_layer(
		MainLayerType main_rendered_layer_type)
{
	return d_main_layer_seq[main_rendered_layer_type].d_rendered_geom_layer.get();
}


GPlatesViewOperations::RenderedGeometryCollection::child_layer_index_type
GPlatesViewOperations::RenderedGeometryCollection::create_child_rendered_layer(
		MainLayerType parent_layer)
{
	// Create rendered geometry layer.
	const child_layer_index_type child_layer_index =
			d_rendered_geometry_layer_manager.create_rendered_geometry_layer(
					parent_layer,
					d_current_viewport_zoom_factor);

	connect_child_rendered_layer_to_parent(child_layer_index, parent_layer);

	// Ownership of rendered geometry layer is passed to caller.
	return child_layer_index;
}


GPlatesViewOperations::RenderedGeometryCollection::child_layer_index_type
GPlatesViewOperations::RenderedGeometryCollection::create_child_rendered_layer(
		MainLayerType parent_layer,
		float ratio_zoom_dependent_bin_dimension_to_globe_radius)
{
	// Create rendered geometry layer.
	const child_layer_index_type child_layer_index =
			d_rendered_geometry_layer_manager.create_rendered_geometry_layer(
					parent_layer,
					ratio_zoom_dependent_bin_dimension_to_globe_radius,
					d_current_viewport_zoom_factor);

	connect_child_rendered_layer_to_parent(child_layer_index, parent_layer);

	// Ownership of rendered geometry layer is passed to caller.
	return child_layer_index;
}


void
GPlatesViewOperations::RenderedGeometryCollection::connect_child_rendered_layer_to_parent(
		const child_layer_index_type child_layer_index,
		MainLayerType parent_layer)
{
	RenderedGeometryLayer *rendered_geom_layer = d_rendered_geometry_layer_manager
		.get_rendered_geometry_layer(child_layer_index);

	// We'd like to know when this rendered geometry layer has been updated.
	connect_to_rendered_geometry_layer_signal(rendered_geom_layer);

	// Add to list of children of the parent layer.
	d_main_layer_seq[parent_layer].d_child_layer_index_seq.push_back(child_layer_index);

	// Let observers know that our state has been modified.
	signal_update(parent_layer);
}


void
GPlatesViewOperations::RenderedGeometryCollection::destroy_child_rendered_layer(
		child_layer_index_type child_layer_index,
		MainLayerType parent_layer)
{
	// Destroy rendered geometry layer.
	d_rendered_geometry_layer_manager.destroy_rendered_geometry_layer(child_layer_index);

	// Remove from list of children of the parent layer.
	d_main_layer_seq[parent_layer].d_child_layer_index_seq.remove(child_layer_index);

	// Let observers know that our state has been modified.
	signal_update(parent_layer);
}


GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type
GPlatesViewOperations::RenderedGeometryCollection::transfer_ownership_of_child_rendered_layer(
		child_layer_index_type child_layer_index,
		MainLayerType parent_layer)
{
	RenderedGeometryLayer *rendered_geom_layer_ptr = get_child_rendered_layer(child_layer_index);

	// Using boost::shared_ptr to handle reference counting so owner can be copied and
	// using boost::bind to make the destroy call when reference count goes to zero.
	// Note that the 'rendered_geom_layer_ptr' can be used for dereferencing but
	// does not get used in the shared_ptr destroy call.
	return boost::shared_ptr<RenderedGeometryLayer>(
			rendered_geom_layer_ptr,
			boost::bind(
				&RenderedGeometryCollection::destroy_child_rendered_layer,
				this,
				child_layer_index,
				parent_layer));
}


GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type
GPlatesViewOperations::RenderedGeometryCollection::create_child_rendered_layer_and_transfer_ownership(
		MainLayerType parent_layer)
{
	// Create a child rendered layer of the main layer.
	const RenderedGeometryCollection::child_layer_index_type child_rendered_geom_layer_index =
			create_child_rendered_layer(parent_layer);

	// Make it so we don't have to destroy the child layer explicitly.
	return transfer_ownership_of_child_rendered_layer(
				child_rendered_geom_layer_index,
				parent_layer);
}


GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type
GPlatesViewOperations::RenderedGeometryCollection::create_child_rendered_layer_and_transfer_ownership(
		MainLayerType parent_layer,
		float ratio_zoom_dependent_bin_dimension_to_globe_radius)
{
	// Create a child rendered layer of the main layer.
	const RenderedGeometryCollection::child_layer_index_type child_rendered_geom_layer_index =
			create_child_rendered_layer(
					parent_layer,
					ratio_zoom_dependent_bin_dimension_to_globe_radius);

	// Make it so we don't have to destroy the child layer explicitly.
	return transfer_ownership_of_child_rendered_layer(
				child_rendered_geom_layer_index,
				parent_layer);
}


GPlatesViewOperations::RenderedGeometryLayer *
GPlatesViewOperations::RenderedGeometryCollection::get_child_rendered_layer(
		child_layer_index_type child_layer_index)
{
	return d_rendered_geometry_layer_manager.get_rendered_geometry_layer(child_layer_index);
}


const GPlatesViewOperations::RenderedGeometryCollection::child_layer_index_seq_type &
GPlatesViewOperations::RenderedGeometryCollection::get_child_rendered_layer_indices(
		MainLayerType parent_layer) const
{
	return d_main_layer_seq[parent_layer].d_child_layer_index_seq;
}


bool
GPlatesViewOperations::RenderedGeometryCollection::is_main_layer_active(
		MainLayerType main_layer_type) const
{
	return d_main_layer_active_state.test(main_layer_type);
}


void
GPlatesViewOperations::RenderedGeometryCollection::set_main_layer_active(
		MainLayerType main_layer_type,
		bool active)
{
	if (d_main_layer_active_state.test(main_layer_type) != active)
	{
		// Guard against multiple update signals.
		// We might signal as well as a RenderedGeometryLayer.
		UpdateGuard update_guard;

		// Keep track of previous state so we know what's changed.
		const main_layer_active_state_internal_type prev_main_layer_active_state =
			d_main_layer_active_state;

		// Change main layer active flag.
		d_main_layer_active_state.set(main_layer_type, active);

		// If activating a main orthogonal layer then deactivate any other orthogonal layers.
		if (active && d_main_layers_orthogonal.test(main_layer_type))
		{
			main_layer_seq_type::size_type orthogonal_layer_index;
			for (orthogonal_layer_index = 0;
				orthogonal_layer_index < NUM_LAYERS;
				++orthogonal_layer_index)
			{
				// The static_cast is a bit dangerous.
				// Is there a better way to iterate over the enumerations ?
				MainLayerType orthogonal_layer_type =
					static_cast<MainLayerType>(orthogonal_layer_index);

				// If layer is orthogonal then deactivate it.
				if (orthogonal_layer_type != main_layer_type &&
					d_main_layers_orthogonal.test(orthogonal_layer_type))
				{
					d_main_layer_active_state.reset(orthogonal_layer_type);
				}
			}
		}

		// Let observers know that our state has been modified.
		signal_update(prev_main_layer_active_state ^ d_main_layer_active_state);
	}
}


void
GPlatesViewOperations::RenderedGeometryCollection::set_orthogonal_main_layers(
		orthogonal_main_layers_type orthogonal_main_layers)
{
	d_main_layers_orthogonal = orthogonal_main_layers;
}


GPlatesViewOperations::RenderedGeometryCollection::orthogonal_main_layers_type
GPlatesViewOperations::RenderedGeometryCollection::get_orthogonal_main_layers() const
{
	return d_main_layers_orthogonal;
}


GPlatesViewOperations::RenderedGeometryCollection::MainLayerActiveState
GPlatesViewOperations::RenderedGeometryCollection::capture_main_layer_active_state() const
{
	// We use the opaque type boost::any to hide the active state from the caller to
	// prevent them from modifying it. If they can modify it then they might set the
	// active state in a way that conflicts with the orthogonal layers.
	return MainLayerActiveState(boost::any(d_main_layer_active_state));
}


void
GPlatesViewOperations::RenderedGeometryCollection::restore_main_layer_active_state(
		MainLayerActiveState main_layer_active_state_opaque)
{
	// We use the opaque type boost::any to hide the active state from the caller to
	// prevent them from modifying it. If they can modify it then they might set the
	// active state in a way that conflicts with the orthogonal layers.
	const main_layer_active_state_internal_type main_layer_active_state =
			boost::any_cast<main_layer_active_state_internal_type>(
					main_layer_active_state_opaque.get_impl());

	if (main_layer_active_state != d_main_layer_active_state)
	{
		// Guard against multiple update signals.
		// We might signal as well as a RenderedGeometryLayer.
		UpdateGuard update_guard;

		d_main_layer_active_state = main_layer_active_state;

		// Let observers know that our state has been modified.
		// Signal an update specifying the flags that changed (using exclusive 'or').
		signal_update(main_layer_active_state ^ d_main_layer_active_state);
	}
}


void
GPlatesViewOperations::RenderedGeometryCollection::connect_to_rendered_geometry_layer_signal(
		RenderedGeometryLayer *rendered_geom_layer)
{
	// Listen for updates other that changes in RenderedGeometryLayer active status.
	QObject::connect(
		rendered_geom_layer,
		SIGNAL(layer_was_updated(
				GPlatesViewOperations::RenderedGeometryLayer &,
				GPlatesViewOperations::RenderedGeometryLayer::user_data_type)),
		this,
		SLOT(rendered_geometry_layer_was_updated(
				GPlatesViewOperations::RenderedGeometryLayer &,
				GPlatesViewOperations::RenderedGeometryLayer::user_data_type)));
}


void
GPlatesViewOperations::RenderedGeometryCollection::begin_update_collection()
{
	boost::mutex::scoped_lock lock(d_update_collection_depth_mutex);
	++d_update_collection_depth;
}


void
GPlatesViewOperations::RenderedGeometryCollection::end_update_collection()
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_update_collection_depth > 0,
			GPLATES_ASSERTION_SOURCE);

	{
		boost::mutex::scoped_lock lock(d_update_collection_depth_mutex);
		--d_update_collection_depth;
	}

	// If an update signal was delayed try signaling it now.
	if (d_update_collection_depth == 0 &&
		d_update_notify_queued)
	{
		send_update_signal();
	}
}


void
GPlatesViewOperations::RenderedGeometryCollection::begin_update_all_registered_collections()
{
	// Get list of RenderedGeometryCollection objects.
	const RenderedGeometryCollectionManager::collection_seq_type &collections =
		RenderedGeometryCollectionManager::instance().get_registered_collections();

	// Call begin_update_collection() on each collection.
	std::for_each(
			collections.begin(),
			collections.end(),
			boost::bind(&RenderedGeometryCollection::begin_update_collection, boost::placeholders::_1));
}


void
GPlatesViewOperations::RenderedGeometryCollection::end_update_all_registered_collections()
{
	// Get list of RenderedGeometryCollection objects.
	const RenderedGeometryCollectionManager::collection_seq_type &collections =
		RenderedGeometryCollectionManager::instance().get_registered_collections();

	// Call end_update_collection() on each collection.
	std::for_each(
			collections.begin(),
			collections.end(),
			boost::bind(&RenderedGeometryCollection::end_update_collection, boost::placeholders::_1));
}


void
GPlatesViewOperations::RenderedGeometryCollection::signal_update(
		MainLayerType main_layer_type)
{
	main_layers_update_type main_layers_updated;

	main_layers_updated.set(main_layer_type);

	signal_update(main_layers_updated);
}


void
GPlatesViewOperations::RenderedGeometryCollection::signal_update(
		main_layers_update_type main_layers_updated)
{
	// Update which main layers have been updated since signal last emitted.
	d_main_layers_updated |= main_layers_updated;

	if (delay_update_notification())
	{
		d_update_notify_queued = true;
	}
	else
	{
		send_update_signal();
	}
}


void
GPlatesViewOperations::RenderedGeometryCollection::send_update_signal()
{
	Q_EMIT collection_was_updated(*this, d_main_layers_updated);
	d_main_layers_updated.reset();

	d_update_notify_queued = false;
}


void
GPlatesViewOperations::RenderedGeometryCollection::rendered_geometry_layer_was_updated(
		GPlatesViewOperations::RenderedGeometryLayer &/*rendered_geom_layer*/,
		GPlatesViewOperations::RenderedGeometryLayer::user_data_type user_data)
{
	// One of our rendered geometry layers has notified us that it was modified.
	// We, in turn, will notify our observers that we've effectively been modified too.

	// Get the user data we passed to the constructor of the RenderedGeometryLayer.
	// This contains the main layer type that the layer belongs to.
	MainLayerType main_layer_type = boost::any_cast<MainLayerType>(user_data);

	signal_update(main_layer_type);
}



GPlatesViewOperations::RenderedGeometryCollection::MainLayerActiveState::MainLayerActiveState(
		impl_type impl) :
	d_impl(impl)
{
}


GPlatesViewOperations::RenderedGeometryCollection::MainLayerActiveState::impl_type
GPlatesViewOperations::RenderedGeometryCollection::MainLayerActiveState::get_impl() const
{
	return d_impl;
}


bool
GPlatesViewOperations::RenderedGeometryCollection::MainLayerActiveState::is_active(
		MainLayerType main_layer_type) const
{
	// We use the opaque type boost::any to hide the active state from the caller to
	// prevent them from modifying it. If they can modify it then they might set the
	// active state in a way that conflicts with the orthogonal layers.
	const main_layer_active_state_internal_type main_layer_active_state =
			boost::any_cast<main_layer_active_state_internal_type>(d_impl);

	return main_layer_active_state.test(main_layer_type);
}


GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard::UpdateGuard()
{
	begin_update_all_registered_collections();
}


GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard::~UpdateGuard()
{
	// Since this is a destructor we cannot let any exceptions escape.
	// If one is thrown we just have to lump it and continue on.
	try
	{
		end_update_all_registered_collections();
	}
	catch (...)
	{
	}
}


GPlatesViewOperations::RenderedGeometryCollection::RenderedGeometryLayerManager::~RenderedGeometryLayerManager()
{
	// Delete any layers that have not been released yet.
	rendered_geometry_layer_seq_type::iterator iter;
	for (iter = d_layer_storage.begin();
		iter != d_layer_storage.end();
		++iter)
	{
		// 'delete' tests for NULL for us.
		delete *iter;
	}
}


const GPlatesViewOperations::RenderedGeometryLayer *
GPlatesViewOperations::RenderedGeometryCollection::RenderedGeometryLayerManager::get_rendered_geometry_layer(
		child_layer_index_type layer_index) const
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			layer_index < d_layer_storage.size(),
			GPLATES_ASSERTION_SOURCE);

	return d_layer_storage[layer_index];
}


GPlatesViewOperations::RenderedGeometryLayer *
GPlatesViewOperations::RenderedGeometryCollection::RenderedGeometryLayerManager::get_rendered_geometry_layer(
		child_layer_index_type layer_index)
{
	return const_cast<RenderedGeometryLayer *>(
			static_cast<const RenderedGeometryLayerManager *>(this)->
					get_rendered_geometry_layer(layer_index));
}


GPlatesViewOperations::RenderedGeometryCollection::child_layer_index_type
GPlatesViewOperations::RenderedGeometryCollection::RenderedGeometryLayerManager::create_rendered_geometry_layer(
		MainLayerType main_layer,
		const double &current_viewport_zoom_factor)
{
	// TODO: make this method exception-safe

	const child_layer_index_type layer_index = allocate_layer_index();

	// Pass main layer to RenderedGeometryLayer so it can return it to us
	// when it emits its layer_was_updated signal.
	// This way we can know which main layer was updated.
	RenderedGeometryLayer::user_data_type user_data(main_layer);

	// Make sure slot isn't already being used.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_layer_storage[layer_index] == NULL,
			GPLATES_ASSERTION_SOURCE);

	// Create a new rendered geometry layer.
	d_layer_storage[layer_index] = new RenderedGeometryLayer(current_viewport_zoom_factor, user_data);

	return layer_index;
}


GPlatesViewOperations::RenderedGeometryCollection::child_layer_index_type
GPlatesViewOperations::RenderedGeometryCollection::RenderedGeometryLayerManager::create_rendered_geometry_layer(
		MainLayerType main_layer,
		float ratio_zoom_dependent_bin_dimension_to_globe_radius,
		const double &current_viewport_zoom_factor)
{
	// TODO: make this method exception-safe

	const child_layer_index_type layer_index = allocate_layer_index();

	// Pass main layer to RenderedGeometryLayer so it can return it to us
	// when it emits its layer_was_updated signal.
	// This way we can know which main layer was updated.
	RenderedGeometryLayer::user_data_type user_data(main_layer);

	// Make sure slot isn't already being used.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_layer_storage[layer_index] == NULL,
			GPLATES_ASSERTION_SOURCE);

	// Create a new rendered geometry layer.
	d_layer_storage[layer_index] = new RenderedGeometryLayer(
			ratio_zoom_dependent_bin_dimension_to_globe_radius,
			current_viewport_zoom_factor,
			user_data);

	return layer_index;
}


void
GPlatesViewOperations::RenderedGeometryCollection::RenderedGeometryLayerManager::destroy_rendered_geometry_layer(
		child_layer_index_type layer_index)
{
	// TODO: make this method exception-safe

	deallocate_layer_index(layer_index);

	// Reset the layer rendered geometry layer for re-use.
	delete d_layer_storage[layer_index];
	d_layer_storage[layer_index] = NULL;
}


GPlatesViewOperations::RenderedGeometryCollection::child_layer_index_type
GPlatesViewOperations::RenderedGeometryCollection::RenderedGeometryLayerManager::allocate_layer_index()
{
	child_layer_index_type layer_index;

	if (!d_layers_available_for_reuse.empty())
	{
		//
		// We re-use an index from a destroyed layer.
		//

		// Pop layer index off the stack.
		layer_index = d_layers_available_for_reuse.top();

		d_layers_available_for_reuse.pop();
	}
	else
	{
		//
		// No available slots so create a new one.
		//

		layer_index = d_layer_storage.size();

		// Increase storage size - we'll assign to this below.
		d_layer_storage.push_back(NULL);
	}

	// Add to list of layers in use.
	d_layers.push_back(layer_index);

	return layer_index;
}


void
GPlatesViewOperations::RenderedGeometryCollection::RenderedGeometryLayerManager::deallocate_layer_index(
		child_layer_index_type layer_index)
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			layer_index < d_layer_storage.size(),
			GPLATES_ASSERTION_SOURCE);

	// Make sure layer index is actually being used.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			std::find(d_layers.begin(), d_layers.end(), layer_index) !=
					d_layers.end(),
			GPLATES_ASSERTION_SOURCE);

	// Remove from list of layers in use.
	d_layers.remove(layer_index);

	// Make layer index available for re-use.
	d_layers_available_for_reuse.push(layer_index);
}


GPlatesViewOperations::RenderedGeometryCollection::MainLayer::MainLayer(
		MainLayerType main_layer_type,
		const double &viewport_zoom_factor)
{
	RenderedGeometryLayer::user_data_type user_data(main_layer_type);

	d_rendered_geom_layer.reset(new RenderedGeometryLayer(viewport_zoom_factor, user_data));
}
