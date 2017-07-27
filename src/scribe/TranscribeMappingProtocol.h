/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2015 The University of Sydney, Australia
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

#ifndef GPLATES_SCRIBE_TRANSCRIBEMAPPINGPROTOCOL_H
#define GPLATES_SCRIBE_TRANSCRIBEMAPPINGPROTOCOL_H

#include <utility>
#include <boost/optional.hpp>

#include "ScribeExceptions.h"
#include "TranscribeResult.h"

#include "global/GPlatesAssert.h"

#include "utils/CallStackTracker.h"


namespace GPlatesScribe
{
	// Forward declarations.
	class Scribe;


	/**
	 * Used to ensure different mapping types are transcribed such that they can be switched
	 * without breaking backward/forward compatibility.
	 *
	 * Mapping types include std::map, std::multimap, QMap and QMultiMap.
	 *
	 * These mapping types first need to specialise the template class @a TranscribeMap.
	 *
	 * Note that 'TRANSCRIBE_UNKNOWN_TYPE' is returned if any mapped elements (key/value)
	 * are encountered (when loading the map from an archive) that have a transcribe result of
	 * 'TRANSCRIBE_UNKNOWN_TYPE' (eg, polymorphic pointers to unknown derived classes).
	 *
	 * If you want to test elements for 'TRANSCRIBE_UNKNOWN_TYPE', to skip them for example,
	 * then you can explicitly use the mapping protocol which is:
	 *  (1) Load an integer with object tag specifying ObjectTag::map_size(), and
	 *  (2) Load up to ObjectTag::map_size() number of elements.
	 *      Each element has:
	 *      (a) A key with with object tag ObjectTag::map_item_key(index), and
	 *      (b) A value with with object tag ObjectTag::map_item_value(index).
	 */
	template <typename MapType>
	TranscribeResult
	transcribe_mapping_protocol(
			const GPlatesUtils::CallStack::Trace &transcribe_source, // Use 'TRANSCRIBE_SOURCE' here
			Scribe &scribe,
			MapType &map);


	/**
	 * Used when relocating a map transcribed with @a transcribe_mapping_protocol.
	 */
	template <typename MapType>
	void
	relocated_mapping_protocol(
			Scribe &scribe,
			const MapType &relocated_map,
			const MapType &transcribed_map);


	/**
	 * Specialisations of this class implement mapping functions used in @a transcribe_mapping_protocol.
	 *
	 * This enables @a transcribe_mapping_protocol to transcribe different mapping types such as
	 * std::map, std::multimap, QMap and QMultiMap such that they can be switched without
	 * breaking backward/forward compatibility.
	 *
	 * Note: This primary class template declares functions (to document them) but does not define them.
	 * You'll need to specialise this class for your mapping type.
	 */
	template <class MapType>
	struct TranscribeMap
	{
		//
		// The following typedefs and functions should be implemented in specialisation.
		//

		typedef MapType map_type;
		typedef typename map_type::key_type key_type;
		typedef typename map_type::mapped_type mapped_type;
		typedef typename map_type::iterator map_iterator;
		typedef typename map_type::const_iterator map_const_iterator;


		/**
		 * Get length of existing map (for saving).
		 */
		static
		unsigned int
		get_length(
				const map_type &map);


		/**
		 * Get (begin, end) range of const-iterators over existing map (for loading).
		 */
		static
		std::pair<map_const_iterator, map_const_iterator>
		get_items(
				const map_type &map);

		/**
		 * Get (begin, end) range of iterators over existing map (for saving).
		 */
		static
		std::pair<map_iterator, map_iterator>
		get_items(
				map_type &map);


		/**
		 * Get the key associated with the specified iterator (for saving).
		 */
		static
		const key_type &
		get_key(
				map_const_iterator iterator);


		/**
		 * Get the value associated with the specified iterator (for saving and loading).
		 */
		static
		const mapped_type &
		get_value(
				map_const_iterator iterator);


		/**
		 * Make sure map is empty (for loading).
		 */
		static
		void
		clear(
				map_type &map);


		/**
		 * Add a loaded item to a map (for loading).
		 *
		 * Returns whether the item was added or not.
		 * For example, some map-like types don't support duplicate keys and will return
		 * none if the same key has already been added.
		 */
		static
		boost::optional<map_iterator>
		add_item(
				map_type &map,
				const key_type &key,
				const mapped_type &value);
	};
}


//
// Implementation
//


// Placing here avoids cyclic header dependency.
#include "Scribe.h"


namespace GPlatesScribe
{
	template <typename MapType>
	TranscribeResult
	transcribe_mapping_protocol(
			const GPlatesUtils::CallStack::Trace &transcribe_source,
			Scribe &scribe,
			MapType &map)
	{
		typedef typename TranscribeMap<MapType>::key_type key_type;
		typedef typename TranscribeMap<MapType>::mapped_type mapped_type;
		typedef typename TranscribeMap<MapType>::map_iterator map_iterator;

		// Track the file/line of the call site for exception messages.
		GPlatesUtils::CallStackTracker call_stack_tracker(transcribe_source);

		if (scribe.is_saving())
		{
			const unsigned int map_size = TranscribeMap<MapType>::get_length(map);
			scribe.save(TRANSCRIBE_SOURCE, map_size, ObjectTag().map_size());

			const std::pair<map_iterator, map_iterator> items = TranscribeMap<MapType>::get_items(map);

			unsigned int n = 0;
			map_iterator items_iter = items.first;
			for ( ; items_iter != items.second; ++items_iter, ++n)
			{
				const key_type &key = TranscribeMap<MapType>::get_key(items_iter);
				const mapped_type &value = TranscribeMap<MapType>::get_value(items_iter);

				scribe.save(TRANSCRIBE_SOURCE, key, ObjectTag().map_item_key(n));
				scribe.save(TRANSCRIBE_SOURCE, value, ObjectTag().map_item_value(n), TRACK);
			}

			GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
					n == map_size,
					GPLATES_ASSERTION_SOURCE,
					"Length of map does not match number of items saved.");
		}
		else // loading...
		{
			// Make sure map starts out empty.
			TranscribeMap<MapType>::clear(map);

			LoadRef<unsigned int> map_size = scribe.load<unsigned int>(TRANSCRIBE_SOURCE, ObjectTag().map_size());
			if (!map_size.is_valid())
			{
				return scribe.get_transcribe_result();
			}

			// Mapping types like std::map don't can re-allocate memory during insertion
			// (ie, don't invalidate existing iterators) like sequences (eg, std::vector).
			// So we don't need to wait until *after* all the items have been added to the map to
			// relocate our transcribed tracked items to their final memory locations.
			// Instead we can relocate after each item is added.

			// Transcribe all items into the map.
			unsigned int n;
			for (n = 0; n < map_size.get(); ++n)
			{
				LoadRef<key_type> item_key = scribe.load<key_type>(TRANSCRIBE_SOURCE, ObjectTag().map_item_key(n));
				if (!item_key.is_valid())
				{
					// Clear the map in case caller tries to use it - which they shouldn't
					// because transcribe failed.
					TranscribeMap<MapType>::clear(map);

					return scribe.get_transcribe_result();
				}

				LoadRef<mapped_type> item_value = scribe.load<mapped_type>(TRANSCRIBE_SOURCE, ObjectTag().map_item_value(n), TRACK);
				if (!item_value.is_valid())
				{
					// Clear the map in case caller tries to use it - which they shouldn't
					// because transcribe failed.
					TranscribeMap<MapType>::clear(map);

					return scribe.get_transcribe_result();
				}

				// Add the item to the map.
				boost::optional<map_iterator> item_iter =
						TranscribeMap<MapType>::add_item(map, item_key.get(), item_value.get());
				if (item_iter)
				{
					// Only need to relocate the value since the key is untracked.
					scribe.relocated(
							TRANSCRIBE_SOURCE,
							TranscribeMap<MapType>::get_value(item_iter.get()),
							item_value);
				}
			}
		}

		return TRANSCRIBE_SUCCESS;
	}


	template <typename MapType>
	void
	relocated_mapping_protocol(
			Scribe &scribe,
			const MapType &relocated_map,
			const MapType &transcribed_map)
	{
		typedef typename TranscribeMap<MapType>::mapped_type mapped_type;
		typedef typename TranscribeMap<MapType>::map_const_iterator map_const_iterator;

		const unsigned int transcribed_map_length =
				TranscribeMap<MapType>::get_length(transcribed_map);

		// Both maps should be the same size.
		GPlatesGlobal::Assert<Exceptions::ScribeUserError>(
				TranscribeMap<MapType>::get_length(relocated_map) == transcribed_map_length,
				GPLATES_ASSERTION_SOURCE,
				"Relocated map differs in size to transcribed map.");

		// Get the relocated map items.
		const std::pair<map_const_iterator, map_const_iterator> relocated_items =
				TranscribeMap<MapType>::get_items(relocated_map);
		// Get the transcribed map items.
		const std::pair<map_const_iterator, map_const_iterator> transcribed_items =
				TranscribeMap<MapType>::get_items(transcribed_map);

		map_const_iterator relocated_items_iter = relocated_items.first;
		map_const_iterator transcribed_items_iter = transcribed_items.first;
		for (unsigned int n = 0;
			n < transcribed_map_length;
			++n, ++relocated_items_iter, ++transcribed_items_iter)
		{
			// Only relocate the value - the key was not tracked.
			const mapped_type &relocated_value = TranscribeMap<MapType>::get_value(relocated_items_iter);
			const mapped_type &transcribed_value = TranscribeMap<MapType>::get_value(transcribed_items_iter);

			scribe.relocated(TRANSCRIBE_SOURCE, relocated_value, transcribed_value);
		}
	}
}

#endif // GPLATES_SCRIBE_TRANSCRIBEMAPPINGPROTOCOL_H
