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

#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>
#include <list>

#include <QDebug>

#include "RenderedGeometryCollection.h"
#include "RenderedGeometryCollectionVisitor.h"
#include "global/GPlatesAssert.h"
#include "global/AssertionFailureException.h"


namespace GPlatesViewOperations
{
	namespace
	{
		/**
		 * Singleton instance to keep track of @a RenderedGeometryCollection objects.
		 */
		class RenderedGeometryCollectionManager :
			private boost::noncopyable
		{
		public:
			/**
			 * Typedef for sequence of @a RenderedGeometryCollection pointers.
			 */
			typedef std::list<RenderedGeometryCollection *> collection_seq_type;

			/**
			 * Singleton instance.
			 */
			static
			RenderedGeometryCollectionManager &
			instance()
			{
				return s_instance;
			}

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
			RenderedGeometryCollectionManager()
			{  }

			static RenderedGeometryCollectionManager s_instance;

			collection_seq_type d_registered_collections;
		};

		RenderedGeometryCollectionManager RenderedGeometryCollectionManager::s_instance;
	}

}

const GPlatesViewOperations::RenderedGeometryCollection::main_layers_update_type
	GPlatesViewOperations::RenderedGeometryCollection::ALL_MAIN_LAYERS =
			~GPlatesViewOperations::RenderedGeometryCollection::main_layers_update_type();

GPlatesViewOperations::RenderedGeometryCollection::RenderedGeometryCollection() :
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

		d_main_layer_seq.push_back(MainLayer(main_rendered_layer_type));

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
	const RenderedGeometryLayerIndex child_layer_index =
		d_rendered_geometry_layer_manager.create_rendered_geometry_layer(parent_layer);

	RenderedGeometryLayer *rendered_geom_layer = d_rendered_geometry_layer_manager
		.get_rendered_geometry_layer(child_layer_index);

	// We'd like to know when this rendered geometry layer has been updated.
	connect_to_rendered_geometry_layer_signal(rendered_geom_layer);

	// Add to list of children of the parent layer.
	d_main_layer_seq[parent_layer].d_child_layer_index_seq.push_back(child_layer_index);

	// Let observers know that our state has been modified.
	signal_update(parent_layer);

	// Ownership of rendered geometry layer is passed to caller.
	return child_layer_index;
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

GPlatesViewOperations::RenderedGeometryLayer *
GPlatesViewOperations::RenderedGeometryCollection::get_child_rendered_layer(
		child_layer_index_type child_layer_index)
{
	return d_rendered_geometry_layer_manager.get_rendered_geometry_layer(child_layer_index);
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

template <class RenderedGeometryLayerType,
		class RenderedGeometryCollectionVisitorType>
void
GPlatesViewOperations::RenderedGeometryCollection::visit_rendered_geometry_layer(
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

template <class RenderedGeometryLayerType,
		class RenderedGeometryCollectionType,
		class RenderedGeometryCollectionVisitorType>
void
GPlatesViewOperations::RenderedGeometryCollection::visit_main_rendered_layer(
		RenderedGeometryCollectionVisitorType &visitor,
		MainLayerType main_layer_type,
		RenderedGeometryCollectionType &rendered_geom_collection)
{
	// Ask the visitor if it wants to visit this main layer.
	// It can query the active status of this main layer and use that to decide.
	if (visitor.visit_main_rendered_layer(rendered_geom_collection, main_layer_type))
	{
		const MainLayer &main_layer = rendered_geom_collection.d_main_layer_seq[main_layer_type];

		//
		// Visit the main render layer first.
		//

		RenderedGeometryLayerType &main_rendered_geom_layer = *main_layer.d_rendered_geom_layer;

		visit_rendered_geometry_layer(
				visitor,
				main_rendered_geom_layer);

		//
		// Visit the child render layers second.
		//

		MainLayer::child_layer_index_seq_type::const_iterator child_layer_iter;
		for (child_layer_iter = main_layer.d_child_layer_index_seq.begin();
			child_layer_iter != main_layer.d_child_layer_index_seq.end();
			++child_layer_iter)
		{
			const RenderedGeometryLayerIndex child_layer_index = *child_layer_iter;

			RenderedGeometryLayerType *child_rendered_geom_layer =
					rendered_geom_collection.d_rendered_geometry_layer_manager.
							get_rendered_geometry_layer(child_layer_index);

			// Visit child rendered geometry layer.
			visit_rendered_geometry_layer(
					visitor,
					*child_rendered_geom_layer);
		}
	}
}

template <class RenderedGeometryLayerType,
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
		main_layer_index < rendered_geom_collection.d_main_layer_seq.size();
		++main_layer_index)
	{
		// The static_cast is a bit dangerous.
		// Is there a better way to iterate over the enumerations ?
		MainLayerType main_layer_type =
			static_cast<MainLayerType>(main_layer_index);

		visit_main_rendered_layer<RenderedGeometryLayerType>(
				visitor,
				main_layer_type,
				rendered_geom_collection);
	}
}

void
GPlatesViewOperations::RenderedGeometryCollection::accept_visitor(
		ConstRenderedGeometryCollectionVisitor &visitor) const
{
	accept_visitor_internal<const RenderedGeometryLayer>(visitor, *this);
}

void
GPlatesViewOperations::RenderedGeometryCollection::accept_visitor(
		RenderedGeometryCollectionVisitor &visitor)
{
	accept_visitor_internal<RenderedGeometryLayer>(visitor, *this);
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
	++d_update_collection_depth;
}

void
GPlatesViewOperations::RenderedGeometryCollection::end_update_collection()
{
	GPlatesGlobal::Assert(d_update_collection_depth > 0,
		GPlatesGlobal::AssertionFailureException(GPLATES_EXCEPTION_SOURCE));

	--d_update_collection_depth;

	// If an update signal was delayed try signaling it now.
	if (d_update_collection_depth == 0 &&
		d_update_notify_queued)
	{
		send_update_signal();
	}
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
	emit collection_was_updated(*this, d_main_layers_updated);
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
	// Get list of RenderedGeometryCollection objects.
	const RenderedGeometryCollectionManager::collection_seq_type &collections =
		RenderedGeometryCollectionManager::instance().get_registered_collections();

	// Call begin_update_collection() on each collection.
	std::for_each(
			collections.begin(),
			collections.end(),
			boost::bind(&RenderedGeometryCollection::begin_update_collection, _1));
}

GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard::~UpdateGuard()
{
	// Since this is a destructor we cannot let any exceptions escape.
	// If one is thrown we just have to lump it and continue on.
	try
	{
		// Get list of RenderedGeometryCollection objects.
		const RenderedGeometryCollectionManager::collection_seq_type &collections =
			RenderedGeometryCollectionManager::instance().get_registered_collections();

		// Call end_update_collection() on each collection.
		std::for_each(
				collections.begin(),
				collections.end(),
				boost::bind(&RenderedGeometryCollection::end_update_collection, _1));
	}
	catch (...)
	{
	}
}

const GPlatesViewOperations::RenderedGeometryLayer *
GPlatesViewOperations::RenderedGeometryCollection::RenderedGeometryLayerManager::get_rendered_geometry_layer(
		RenderedGeometryLayerIndex layer_index) const
{
	GPlatesGlobal::Assert(
			layer_index < d_layer_storage.size(),
			GPlatesGlobal::AssertionFailureException(GPLATES_EXCEPTION_SOURCE));

	return d_layer_storage[layer_index];
}

GPlatesViewOperations::RenderedGeometryLayer *
GPlatesViewOperations::RenderedGeometryCollection::RenderedGeometryLayerManager::get_rendered_geometry_layer(
		RenderedGeometryLayerIndex layer_index)
{
	return const_cast<RenderedGeometryLayer *>(
			static_cast<const RenderedGeometryLayerManager *>(this)->
					get_rendered_geometry_layer(layer_index));
}

GPlatesViewOperations::RenderedGeometryCollection::RenderedGeometryLayerIndex
GPlatesViewOperations::RenderedGeometryCollection::RenderedGeometryLayerManager::create_rendered_geometry_layer(
		MainLayerType main_layer)
{
	// TODO: make this method exception-safe

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

	// Make sure slot isn't already being used.
	GPlatesGlobal::Assert(
			d_layer_storage[layer_index] == NULL,
			GPlatesGlobal::AssertionFailureException(GPLATES_EXCEPTION_SOURCE));

	// Pass main layer to RenderedGeometryLayer so it can return it to us
	// when it emits its layer_was_updated signal.
	// This way we can know which main layer was updated.
	RenderedGeometryLayer::user_data_type user_data(main_layer);

	// Create a new rendered geometry layer.
	d_layer_storage[layer_index] = new RenderedGeometryLayer(user_data);

	return layer_index;
}

void
GPlatesViewOperations::RenderedGeometryCollection::RenderedGeometryLayerManager::destroy_rendered_geometry_layer(
		RenderedGeometryLayerIndex layer_index)
{
	// TODO: make this method exception-safe

	GPlatesGlobal::Assert(
			layer_index < d_layer_storage.size(),
			GPlatesGlobal::AssertionFailureException(GPLATES_EXCEPTION_SOURCE));

	// Make sure layer index is actually being used.
	GPlatesGlobal::Assert(
			std::find(d_layers.begin(), d_layers.end(), layer_index) !=
					d_layers.end(),
			GPlatesGlobal::AssertionFailureException(GPLATES_EXCEPTION_SOURCE));

	// Remove from list of layers in use.
	d_layers.remove(layer_index);

	// Make layer index available for re-use.
	d_layers_available_for_reuse.push(layer_index);

	// Reset the layer rendered geometry layer for re-use.
	delete d_layer_storage[layer_index];
	d_layer_storage[layer_index] = NULL;
}

GPlatesViewOperations::RenderedGeometryCollection::MainLayer::MainLayer(
		MainLayerType main_layer_type)
{
	RenderedGeometryLayer::user_data_type user_data(main_layer_type);

	d_rendered_geom_layer.reset(new RenderedGeometryLayer(user_data));
}
