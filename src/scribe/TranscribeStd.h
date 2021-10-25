/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#ifndef GPLATES_SCRIBE_TRANSCRIBESTD_H
#define GPLATES_SCRIBE_TRANSCRIBESTD_H

#include <deque>
#include <list>
#include <map>
#include <memory>
#include <queue>
#include <set>
#include <stack>
#include <utility>
#include <vector>
#include <boost/type_traits/remove_const.hpp>

#include "ScribeConstructObject.h"
#include "ScribeExceptions.h"
#include "Transcribe.h"
#include "TranscribeMappingProtocol.h"
#include "TranscribeSequenceProtocol.h"
#include "TranscribeSmartPointerProtocol.h"

#include "global/GPlatesAssert.h"


namespace GPlatesScribe
{
	//
	// Standard library (std) specialisations and overloads of the transcribe function.
	//

	class Scribe;

	/**
	 * Transcribe std::pair.
	 *
	 * We also need save/load data construction in case pair is not default-constructable.
	 * A std::pair instantiation is only default constructable if both its types are default constructable.
	 *
	 * If a std::pair instantiation is default-constructable then it can be transcribed with or
	 * without save/load construction. An example without save/load construct is:
	 * 
	 *     std::pair<...> x;
	 *     scribe.transcribe(TRANSCRIBE_SOURCE, x, "x", GPlatesScribe::TRACK);
	 * 
	 * ...but if it is not default constructable then it must be transcribed using save/load construction
	 * or initialised with a dummy value (and then transcribed).
	 * For example:
	 * 
	 *     GPlatesScribe::LoadRef< std::pair<...> > x =
	 *         scribe.load< std::pair<...> >(TRANSCRIBE_SOURCE, "x", GPlatesScribe::TRACK);
	 * 
	 * ...or...
	 * 
	 *     std::pair<...> x(dummy_first, dummy_second);
	 *     scribe.transcribe(TRANSCRIBE_SOURCE, x, "x", GPlatesScribe::TRACK);
	 */
	template <typename T1, typename T2>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			std::pair<T1, T2> &pair_object,
			bool transcribed_construct_data);

	template <typename T1, typename T2>
	TranscribeResult
	transcribe_construct_data(
			Scribe &scribe,
			ConstructObject< std::pair<T1, T2> > &pair_object);


	//! Transcribe std::unique_ptr.
	template <typename T>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			std::unique_ptr<T> &unique_ptr_object,
			bool transcribed_construct_data);


	//
	// Std containers.
	//


	//! Transcribe std::deque.
	template <typename T, class Allocator>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			std::deque<T, Allocator> &deque_object,
			bool transcribed_construct_data);

	//! Relocated std::deque.
	template <typename T, class Allocator>
	void
	relocated(
			Scribe &scribe,
			const std::deque<T, Allocator> &relocated_deque_object,
			const std::deque<T, Allocator> &transcribed_deque_object);

	//! Transcribe std::list.
	template <typename T, class Allocator>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			std::list<T, Allocator> &list_object,
			bool transcribed_construct_data);

	//! Relocated std::list.
	template <typename T, class Allocator>
	void
	relocated(
			Scribe &scribe,
			const std::list<T, Allocator> &relocated_list_object,
			const std::list<T, Allocator> &transcribed_list_object);

	//! Transcribe std::map.
	template <typename Key, typename T, class Compare, class Allocator>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			std::map<Key, T, Compare, Allocator> &map_object,
			bool transcribed_construct_data);

	//! Relocated std::map.
	template <typename Key, typename T, class Compare, class Allocator>
	void
	relocated(
			Scribe &scribe,
			const std::map<Key, T, Compare, Allocator> &relocated_map_object,
			const std::map<Key, T, Compare, Allocator> &transcribed_map_object);

	//! Transcribe std::multimap.
	template <typename Key, typename T, class Compare, class Allocator>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			std::multimap<Key, T, Compare, Allocator> &multimap_object,
			bool transcribed_construct_data);

	//! Relocated std::map.
	template <typename Key, typename T, class Compare, class Allocator>
	void
	relocated(
			Scribe &scribe,
			const std::multimap<Key, T, Compare, Allocator> &relocated_multimap_object,
			const std::multimap<Key, T, Compare, Allocator> &transcribed_multimap_object);

	//! Transcribe std::set.
	template <typename T, class Compare, class Allocator>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			std::set<T, Compare, Allocator> &set_object,
			bool transcribed_construct_data);

	//! Relocated std::set.
	template <typename T, class Compare, class Allocator>
	void
	relocated(
			Scribe &scribe,
			const std::set<T, Compare, Allocator> &relocated_set_object,
			const std::set<T, Compare, Allocator> &transcribed_set_object);

	//! Transcribe std::multiset.
	template <typename T, class Compare, class Allocator>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			std::multiset<T, Compare, Allocator> &multiset_object,
			bool transcribed_construct_data);

	//! Relocated std::multiset.
	template <typename T, class Compare, class Allocator>
	void
	relocated(
			Scribe &scribe,
			const std::multiset<T, Compare, Allocator> &relocated_multiset_object,
			const std::multiset<T, Compare, Allocator> &transcribed_multiset_object);

	//! Transcribe std::priority_queue.
	template <typename T, class Container, class Compare>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			std::priority_queue<T, Container, Compare> &priority_queue_object,
			bool transcribed_construct_data);

	//! Relocated std::priority_queue.
	template <typename T, class Container, class Compare>
	void
	relocated(
			Scribe &scribe,
			const std::priority_queue<T, Container, Compare> &relocated_priority_queue_object,
			const std::priority_queue<T, Container, Compare> &transcribed_priority_queue_object);

	//! Transcribe std::queue.
	template <typename T, class Container>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			std::queue<T, Container> &queue_object,
			bool transcribed_construct_data);

	//! Relocated std::queue.
	template <typename T, class Container>
	void
	relocated(
			Scribe &scribe,
			const std::queue<T, Container> &relocated_queue_object,
			const std::queue<T, Container> &transcribed_queue_object);

	//! Transcribe std::stack.
	template <typename T, class Container>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			std::stack<T, Container> &stack_object,
			bool transcribed_construct_data);

	//! Relocated std::stack.
	template <typename T, class Container>
	void
	relocated(
			Scribe &scribe,
			const std::stack<T, Container> &relocated_stack_object,
			const std::stack<T, Container> &transcribed_stack_object);

	//! Transcribe std::vector.
	template <typename T, class Allocator>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			std::vector<T, Allocator> &vector_object,
			bool transcribed_construct_data);

	//! Relocated std::vector.
	template <typename T, class Allocator>
	void
	relocated(
			Scribe &scribe,
			const std::vector<T, Allocator> &relocated_vector_object,
			const std::vector<T, Allocator> &transcribed_vector_object);
}


//
// Template implementation
//


// Placing here avoids cyclic header dependency.
#include "Scribe.h"


namespace GPlatesScribe
{
	template <typename T1, typename T2>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			std::pair<T1, T2> &pair_object,
			bool transcribed_construct_data)
	{
		// If already transcribed using (non-default) constructor then nothing left to do.
		if (!transcribed_construct_data)
		{
			if (!scribe.transcribe(TRANSCRIBE_SOURCE, pair_object.first, "first", TRACK))
			{
				return scribe.get_transcribe_result();
			}

			if (!scribe.transcribe(TRANSCRIBE_SOURCE, pair_object.second, "second", TRACK))
			{
				return scribe.get_transcribe_result();
			}
		}

		return TRANSCRIBE_SUCCESS;
	}

	template <typename T1, typename T2>
	TranscribeResult
	transcribe_construct_data(
			Scribe &scribe,
			ConstructObject< std::pair<T1, T2> > &pair_object)
	{
		if (scribe.is_saving())
		{
			scribe.save(TRANSCRIBE_SOURCE, pair_object->first, "first", TRACK);
			scribe.save(TRANSCRIBE_SOURCE, pair_object->second, "second", TRACK);
		}
		else // loading...
		{
			LoadRef<T1> first = scribe.load<T1>(TRANSCRIBE_SOURCE, "first", TRACK);
			if (!first.is_valid())
			{
				return scribe.get_transcribe_result();
			}

			LoadRef<T2> second = scribe.load<T2>(TRANSCRIBE_SOURCE, "second", TRACK);
			if (!second.is_valid())
			{
				return scribe.get_transcribe_result();
			}

			// Construct the object from the constructor data.
			pair_object.construct_object(first, second);

			scribe.relocated(TRANSCRIBE_SOURCE, pair_object->first, first);
			scribe.relocated(TRANSCRIBE_SOURCE, pair_object->second, second);
		}

		return TRANSCRIBE_SUCCESS;
	}


	template <typename T>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			std::unique_ptr<T> &unique_ptr_object,
			bool transcribed_construct_data)
	{
		T *raw_ptr = NULL;

		if (scribe.is_saving())
		{
			raw_ptr = unique_ptr_object.get();
		}

		TranscribeResult transcribe_result =
				transcribe_smart_pointer_protocol(TRANSCRIBE_SOURCE, scribe, raw_ptr, false/*shared_owner*/);
		if (transcribe_result != TRANSCRIBE_SUCCESS)
		{
			return transcribe_result;
		}

		if (scribe.is_loading())
		{
			unique_ptr_object.reset(raw_ptr);
		}

		return TRANSCRIBE_SUCCESS;
	}


	/**
	 * std::set transcribe sequence protocol implementation.
	 */
	template <typename T, class Compare, class Allocator>
	struct TranscribeSequence< std::set<T, Compare, Allocator> >
	{
		typedef std::set<T, Compare, Allocator> sequence_type;
		typedef typename sequence_type::value_type item_type;
		typedef typename sequence_type::iterator sequence_iterator;
		typedef typename sequence_type::const_iterator sequence_const_iterator;

		static
		unsigned int
		get_length(
				const sequence_type &sequence)
		{
			return sequence.size();
		}

		static
		std::pair<sequence_const_iterator, sequence_const_iterator>
		get_items(
				const sequence_type &sequence)
		{
			return std::make_pair(sequence.begin(), sequence.end());
		}

		static
		std::pair<sequence_iterator, sequence_iterator>
		get_items(
				sequence_type &sequence)
		{
			return std::make_pair(sequence.begin(), sequence.end());
		}

		static
		void
		clear(
				sequence_type &sequence)
		{
			sequence.clear();
		}

		static
		bool
		add_item(
				sequence_type &sequence,
				const item_type &item)
		{
			// Attempt to insert item.
			const std::pair<typename sequence_type::const_iterator, bool> result = sequence.insert(item);
			return result.second;
		}
	};


	/**
	 * std::multiset transcribe sequence protocol implementation.
	 */
	template <typename T, class Compare, class Allocator>
	struct TranscribeSequence< std::multiset<T, Compare, Allocator> >
	{
		typedef std::multiset<T, Compare, Allocator> sequence_type;
		typedef typename sequence_type::value_type item_type;
		typedef typename sequence_type::iterator sequence_iterator;
		typedef typename sequence_type::const_iterator sequence_const_iterator;

		static
		unsigned int
		get_length(
				const sequence_type &sequence)
		{
			return sequence.size();
		}

		static
		std::pair<sequence_const_iterator, sequence_const_iterator>
		get_items(
				const sequence_type &sequence)
		{
			return std::make_pair(sequence.begin(), sequence.end());
		}

		static
		std::pair<sequence_iterator, sequence_iterator>
		get_items(
				sequence_type &sequence)
		{
			return std::make_pair(sequence.begin(), sequence.end());
		}

		static
		void
		clear(
				sequence_type &sequence)
		{
			sequence.clear();
		}

		static
		bool
		add_item(
				sequence_type &sequence,
				const item_type &item)
		{
			// Duplicate items allowed so insert always succeeds.
			sequence.insert(item);
			return true;
		}
	};


	/**
	 * std::map transcribe mapping protocol implementation.
	 */
	template <typename Key, typename T, class Compare, class Allocator>
	struct TranscribeMap< std::map<Key, T, Compare, Allocator> >
	{
		typedef std::map<Key, T, Compare, Allocator> map_type;
		typedef typename map_type::key_type key_type;
		typedef typename map_type::mapped_type mapped_type;
		typedef typename map_type::iterator map_iterator;
		typedef typename map_type::const_iterator map_const_iterator;

		static
		unsigned int
		get_length(
				const map_type &map)
		{
			return map.size();
		}

		static
		std::pair<map_const_iterator, map_const_iterator>
		get_items(
				const map_type &map)
		{
			return std::make_pair(map.begin(), map.end());
		}

		static
		std::pair<map_iterator, map_iterator>
		get_items(
				map_type &map)
		{
			return std::make_pair(map.begin(), map.end());
		}

		static
		const key_type &
		get_key(
				map_const_iterator iterator)
		{
			return iterator->first;
		}

		static
		const mapped_type &
		get_value(
				map_const_iterator iterator)
		{
			return iterator->second;
		}

		static
		void
		clear(
				map_type &map)
		{
			map.clear();
		}

		static
		boost::optional<map_iterator>
		add_item(
				map_type &map,
				const key_type &key,
				const mapped_type &value)
		{
			const std::pair<map_iterator, bool> result = map.insert(typename map_type::value_type(key, value));
			if (!result.second)
			{
				return boost::none;
			}

			return result.first;
		}
	};


	/**
	 * std::multimap transcribe mapping protocol implementation.
	 */
	template <typename Key, typename T, class Compare, class Allocator>
	struct TranscribeMap< std::multimap<Key, T, Compare, Allocator> >
	{
		typedef std::multimap<Key, T, Compare, Allocator> map_type;
		typedef typename map_type::key_type key_type;
		typedef typename map_type::mapped_type mapped_type;
		typedef typename map_type::iterator map_iterator;
		typedef typename map_type::const_iterator map_const_iterator;

		static
		unsigned int
		get_length(
				const map_type &map)
		{
			return map.size();
		}

		static
		std::pair<map_const_iterator, map_const_iterator>
		get_items(
				const map_type &map)
		{
			return std::make_pair(map.begin(), map.end());
		}

		static
		std::pair<map_iterator, map_iterator>
		get_items(
				map_type &map)
		{
			return std::make_pair(map.begin(), map.end());
		}

		static
		const key_type &
		get_key(
				map_const_iterator iterator)
		{
			return iterator->first;
		}

		static
		const mapped_type &
		get_value(
				map_const_iterator iterator)
		{
			return iterator->second;
		}

		static
		void
		clear(
				map_type &map)
		{
			map.clear();
		}

		static
		boost::optional<map_iterator>
		add_item(
				map_type &map,
				const key_type &key,
				const mapped_type &value)
		{
			return map.insert(typename map_type::value_type(key, value));
		}
	};


	template <typename T, class Allocator>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			std::deque<T, Allocator> &deque_object,
			bool transcribed_construct_data)
	{
		return transcribe_sequence_protocol(TRANSCRIBE_SOURCE, scribe, deque_object);
	}


	template <typename T, class Allocator>
	void
	relocated(
			Scribe &scribe,
			const std::deque<T, Allocator> &relocated_deque_object,
			const std::deque<T, Allocator> &transcribed_deque_object)
	{
		relocated_sequence_protocol(scribe, relocated_deque_object, transcribed_deque_object);
	}


	template <typename T, class Allocator>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			std::list<T, Allocator> &list_object,
			bool transcribed_construct_data)
	{
		return transcribe_sequence_protocol(TRANSCRIBE_SOURCE, scribe, list_object);
	}


	template <typename T, class Allocator>
	void
	relocated(
			Scribe &scribe,
			const std::list<T, Allocator> &relocated_list_object,
			const std::list<T, Allocator> &transcribed_list_object)
	{
		relocated_sequence_protocol(scribe, relocated_list_object, transcribed_list_object);
	}


	template <typename Key, typename T, class Compare, class Allocator>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			std::map<Key, T, Compare, Allocator> &map_object,
			bool transcribed_construct_data)
	{
		return transcribe_mapping_protocol(TRANSCRIBE_SOURCE, scribe, map_object);
	}


	template <typename Key, typename T, class Compare, class Allocator>
	void
	relocated(
			Scribe &scribe,
			const std::map<Key, T, Compare, Allocator> &relocated_map_object,
			const std::map<Key, T, Compare, Allocator> &transcribed_map_object)
	{
		relocated_mapping_protocol(scribe, relocated_map_object, transcribed_map_object);
	}


	template <typename Key, typename T, class Compare, class Allocator>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			std::multimap<Key, T, Compare, Allocator> &multimap_object,
			bool transcribed_construct_data)
	{
		return transcribe_mapping_protocol(TRANSCRIBE_SOURCE, scribe, multimap_object);
	}


	template <typename Key, typename T, class Compare, class Allocator>
	void
	relocated(
			Scribe &scribe,
			const std::multimap<Key, T, Compare, Allocator> &relocated_multimap_object,
			const std::multimap<Key, T, Compare, Allocator> &transcribed_multimap_object)
	{
		relocated_mapping_protocol(scribe, relocated_multimap_object, transcribed_multimap_object);
	}


	template <typename T, class Compare, class Allocator>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			std::set<T, Compare, Allocator> &set_object,
			bool transcribed_construct_data)
	{
		return transcribe_sequence_protocol(TRANSCRIBE_SOURCE, scribe, set_object);
	}


	template <typename T, class Compare, class Allocator>
	void
	relocated(
			Scribe &scribe,
			const std::set<T, Compare, Allocator> &relocated_set_object,
			const std::set<T, Compare, Allocator> &transcribed_set_object)
	{
		relocated_sequence_protocol(scribe, relocated_set_object, transcribed_set_object);
	}


	template <typename T, class Compare, class Allocator>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			std::multiset<T, Compare, Allocator> &multiset_object,
			bool transcribed_construct_data)
	{
		return transcribe_sequence_protocol(TRANSCRIBE_SOURCE, scribe, multiset_object);
	}


	template <typename T, class Compare, class Allocator>
	void
	relocated(
			Scribe &scribe,
			const std::multiset<T, Compare, Allocator> &relocated_multiset_object,
			const std::multiset<T, Compare, Allocator> &transcribed_multiset_object)
	{
		relocated_sequence_protocol(scribe, relocated_multiset_object, transcribed_multiset_object);
	}


	namespace Implementation
	{
		// A trick for extracting the internal container in a 'std::stack, 'std::queue' and 'std::priority_queue':
		//   http://stackoverflow.com/questions/4523178/how-to-print-out-all-elements-in-a-stdstack-or-stdqueue-conveniently
		//
		template <class QualifiedContainer, class QualifiedAdapter>
		QualifiedContainer &
		get_adapter_container(
				QualifiedAdapter &adapter)
		{
			struct ExtractContainerFromAdapter :
					private boost::remove_const<QualifiedAdapter>::type
			{
				static
				QualifiedContainer &
				get_adapter_container(
						QualifiedAdapter &adapter_2)
				{
					return adapter_2.*&ExtractContainerFromAdapter::c;
				}
			};

			return ExtractContainerFromAdapter::get_adapter_container(adapter);
		}
	}


	template <typename T, class Container, class Compare>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			std::priority_queue<T, Container, Compare> &priority_queue_object,
			bool transcribed_construct_data)
	{
		return transcribe_sequence_protocol(
				TRANSCRIBE_SOURCE,
				scribe,
				Implementation::get_adapter_container<Container>(priority_queue_object));
	}


	template <typename T, class Container, class Compare>
	void
	relocated(
			Scribe &scribe,
			const std::priority_queue<T, Container, Compare> &relocated_priority_queue_object,
			const std::priority_queue<T, Container, Compare> &transcribed_priority_queue_object)
	{
		relocated_sequence_protocol(
				scribe,
				Implementation::get_adapter_container<const Container>(relocated_priority_queue_object),
				Implementation::get_adapter_container<const Container>(transcribed_priority_queue_object));
	}


	template <typename T, class Container>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			std::queue<T, Container> &queue_object,
			bool transcribed_construct_data)
	{
		return transcribe_sequence_protocol(
				TRANSCRIBE_SOURCE,
				scribe,
				Implementation::get_adapter_container<Container>(queue_object));
	}


	template <typename T, class Container>
	void
	relocated(
			Scribe &scribe,
			const std::queue<T, Container> &relocated_queue_object,
			const std::queue<T, Container> &transcribed_queue_object)
	{
		relocated_sequence_protocol(
				scribe,
				Implementation::get_adapter_container<const Container>(relocated_queue_object),
				Implementation::get_adapter_container<const Container>(transcribed_queue_object));
	}


	template <typename T, class Container>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			std::stack<T, Container> &stack_object,
			bool transcribed_construct_data)
	{
		return transcribe_sequence_protocol(
				TRANSCRIBE_SOURCE,
				scribe,
				Implementation::get_adapter_container<Container>(stack_object));
	}


	template <typename T, class Container>
	void
	relocated(
			Scribe &scribe,
			const std::stack<T, Container> &relocated_stack_object,
			const std::stack<T, Container> &transcribed_stack_object)
	{
		relocated_sequence_protocol(
				scribe,
				Implementation::get_adapter_container<const Container>(relocated_stack_object),
				Implementation::get_adapter_container<const Container>(transcribed_stack_object));
	}


	template <typename T, class Allocator>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			std::vector<T, Allocator> &vector_object,
			bool transcribed_construct_data)
	{
		return transcribe_sequence_protocol(TRANSCRIBE_SOURCE, scribe, vector_object);
	}


	template <typename T, class Allocator>
	void
	relocated(
			Scribe &scribe,
			const std::vector<T, Allocator> &relocated_vector_object,
			const std::vector<T, Allocator> &transcribed_vector_object)
	{
		relocated_sequence_protocol(scribe, relocated_vector_object, transcribed_vector_object);
	}
}

#endif // GPLATES_SCRIBE_TRANSCRIBESTD_H
