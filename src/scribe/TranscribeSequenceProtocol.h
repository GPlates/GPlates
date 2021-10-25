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

#ifndef GPLATES_SCRIBE_TRANSCRIBESEQUENCEPROTOCOL_H
#define GPLATES_SCRIBE_TRANSCRIBESEQUENCEPROTOCOL_H

#include <string>
#include <utility>
#include <vector>

#include "ScribeExceptions.h"
#include "TranscribeResult.h"

#include "global/GPlatesAssert.h"

#include "utils/CallStackTracker.h"


namespace GPlatesScribe
{
	// Forward declarations.
	class Scribe;


	/**
	 * Used to ensure different sequence types are transcribed such that they can be switched
	 * without breaking backward/forward compatibility.
	 *
	 * Sequence types include std::vector, std::list, std::set, QList, QSet, etc.
	 *
	 * These sequence types first need to specialise the template class @a TranscribeSequence.
	 *
	 * Note that 'TRANSCRIBE_UNKNOWN_TYPE' is returned if any sequence elements are
	 * encountered (when loading the sequence from an archive) that have a transcribe result of
	 * 'TRANSCRIBE_UNKNOWN_TYPE' (eg, polymorphic pointers to unknown derived classes).
	 *
	 * If you want to test elements for 'TRANSCRIBE_UNKNOWN_TYPE', to skip them for example,
	 * then you can explicitly use the sequence protocol which is:
	 *  (1) Load an integer with an object tag specifying ObjectTag::sequence_size(), and
	 *  (2) Load up to ObjectTag::sequence_size() number of elements using ObjectTag::sequence_item(index).
	 */
	template <typename SequenceType>
	TranscribeResult
	transcribe_sequence_protocol(
			const GPlatesUtils::CallStack::Trace &transcribe_source, // Use 'TRANSCRIBE_SOURCE' here
			Scribe &scribe,
			SequenceType &sequence);


	/**
	 * Used when relocating a sequence transcribed with @a transcribe_sequence_protocol.
	 */
	template <typename SequenceType>
	void
	relocated_sequence_protocol(
			Scribe &scribe,
			const SequenceType &relocated_sequence,
			const SequenceType &transcribed_sequence);


	/**
	 * Specialisations of this class implement sequence functions used in @a transcribe_sequence_protocol.
	 *
	 * This enables @a transcribe_sequence_protocol to transcribe different sequence types such as
	 * std::vector, std::list, std::set, QList, QSet, etc, such that they can be switched without
	 * breaking backward/forward compatibility.
	 *
	 * The default implementation (in primary template class) works for std::vector-compatible sequences.
	 */
	template <class SequenceType>
	struct TranscribeSequence
	{
		typedef SequenceType sequence_type;
		typedef typename sequence_type::value_type item_type;
		typedef typename sequence_type::iterator sequence_iterator;
		typedef typename sequence_type::const_iterator sequence_const_iterator;


		/**
		 * Get length of existing sequence (for saving).
		 */
		static
		unsigned int
		get_length(
				const sequence_type &sequence)
		{
			return sequence.size();
		}


		/**
		 * Get (begin, end) range of const-iterators over existing sequence (for saving and loading).
		 */
		static
		std::pair<sequence_const_iterator, sequence_const_iterator>
		get_items(
				const sequence_type &sequence)
		{
			return std::make_pair(sequence.begin(), sequence.end());
		}

		/**
		 * Get (begin, end) range of iterators over existing sequence (for saving and loading).
		 */
		static
		std::pair<sequence_iterator, sequence_iterator>
		get_items(
				sequence_type &sequence)
		{
			return std::make_pair(sequence.begin(), sequence.end());
		}


		/**
		 * Make sure sequence is empty (for loading).
		 */
		static
		void
		clear(
				sequence_type &sequence)
		{
			sequence.clear();
		}


		/**
		 * Add a loaded item to a sequence (for loading).
		 *
		 * Returns whether the item was added or not.
		 * For example, set-like sequences that don't support duplicate items will return
		 * false if the same item has already been added.
		 */
		static
		bool
		add_item(
				sequence_type &sequence,
				const item_type &item)
		{
			sequence.push_back(item);

			// Sequence supports duplicate items.
			return true;
		}
	};
}


//
// Implementation
//


// Placing here avoids cyclic header dependency.
#include "Scribe.h"


namespace GPlatesScribe
{
	template <typename SequenceType>
	TranscribeResult
	transcribe_sequence_protocol(
			const GPlatesUtils::CallStack::Trace &transcribe_source,
			Scribe &scribe,
			SequenceType &sequence)
	{
		typedef typename TranscribeSequence<SequenceType>::item_type item_type;
		typedef typename TranscribeSequence<SequenceType>::sequence_iterator sequence_iterator;

		// Track the file/line of the call site for exception messages.
		GPlatesUtils::CallStackTracker call_stack_tracker(transcribe_source);

		if (scribe.is_saving())
		{
			// 'sequence_size' won't be referenced by other objects.
			const unsigned int sequence_size = TranscribeSequence<SequenceType>::get_length(sequence);
			scribe.save(TRANSCRIBE_SOURCE, sequence_size, ObjectTag().sequence_size());

			const std::pair<sequence_iterator, sequence_iterator> items =
					TranscribeSequence<SequenceType>::get_items(sequence);

			unsigned int n = 0;
			sequence_iterator items_iter = items.first;
			for ( ; items_iter != items.second; ++items_iter, ++n)
			{
				const item_type &item = *items_iter;

				scribe.save(TRANSCRIBE_SOURCE, item, ObjectTag()[n], TRACK);
			}

			GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
					n == sequence_size,
					GPLATES_ASSERTION_SOURCE,
					"Length of sequence does not match number of items saved.");
		}
		else // loading...
		{
			// Make sure sequence starts out empty.
			TranscribeSequence<SequenceType>::clear(sequence);

			// 'sequence_size' won't be referenced by other objects.
			LoadRef<unsigned int> sequence_size =
					scribe.load<unsigned int>(TRANSCRIBE_SOURCE, ObjectTag().sequence_size());
			if (!sequence_size.is_valid())
			{
				return scribe.get_transcribe_result();
			}

			// Since sequences like std::vector can re-allocate memory during append we need to
			// relocate our transcribed tracked items to their final memory locations *after*
			// all the items have been added to the sequence.
			std::vector< LoadRef<item_type> > item_relocate_array;
			item_relocate_array.reserve(sequence_size.get());

			// Transcribe all items into the sequence.
			unsigned int n;
			for (n = 0; n < sequence_size.get(); ++n)
			{
				LoadRef<item_type> item = scribe.load<item_type>(TRANSCRIBE_SOURCE, ObjectTag()[n], TRACK);
				if (!item.is_valid())
				{
					// Clear the sequence in case caller tries to use it - which they shouldn't
					// because transcribe failed.
					TranscribeSequence<SequenceType>::clear(sequence);

					return scribe.get_transcribe_result();
				}

				// Add the item to the sequence.
				if (TranscribeSequence<SequenceType>::add_item(sequence, item.get()))
				{
					// Item was successfully added (eg, is not a duplicate item in a set).
					// Keep the item reference for later relocation.
					item_relocate_array.push_back(item);
				}
			}

			GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
					item_relocate_array.size() == TranscribeSequence<SequenceType>::get_length(sequence),
					GPLATES_ASSERTION_SOURCE,
					"Length of sequence does not match number of items loaded.");

			// Get the loaded items.
			const std::pair<sequence_iterator, sequence_iterator> items =
					TranscribeSequence<SequenceType>::get_items(sequence);

			// Now that all items have been added to the sequence (and any potential internal sequence
			// re-allocations are done) we can relocate items from the item references to the sequence.
			sequence_iterator items_iter = items.first;
			for (n = 0 ; items_iter != items.second; ++items_iter, ++n)
			{
				// Using 'const' here since some sequences, like QSet, return const values
				// when dereferencing a non-const iterator.
				const item_type &item = *items_iter;

				scribe.relocated(TRANSCRIBE_SOURCE, item, item_relocate_array[n]);
			}
		}

		return TRANSCRIBE_SUCCESS;
	}


	template <typename SequenceType>
	void
	relocated_sequence_protocol(
			Scribe &scribe,
			const SequenceType &relocated_sequence,
			const SequenceType &transcribed_sequence)
	{
		typedef typename TranscribeSequence<SequenceType>::item_type item_type;
		typedef typename TranscribeSequence<SequenceType>::sequence_const_iterator sequence_const_iterator;

		const unsigned int transcribed_sequence_length =
				TranscribeSequence<SequenceType>::get_length(transcribed_sequence);

		// Both sequences should be the same size.
		GPlatesGlobal::Assert<Exceptions::ScribeUserError>(
				TranscribeSequence<SequenceType>::get_length(relocated_sequence) ==
					transcribed_sequence_length,
				GPLATES_ASSERTION_SOURCE,
				"Relocated sequence differs in size to transcribed sequence.");

		// Get the relocated sequence items.
		const std::pair<sequence_const_iterator, sequence_const_iterator> relocated_items =
				TranscribeSequence<SequenceType>::get_items(relocated_sequence);
		// Get the transcribed sequence items.
		const std::pair<sequence_const_iterator, sequence_const_iterator> transcribed_items =
				TranscribeSequence<SequenceType>::get_items(transcribed_sequence);

		sequence_const_iterator relocated_items_iter = relocated_items.first;
		sequence_const_iterator transcribed_items_iter = transcribed_items.first;
		for (unsigned int n = 0;
			n < transcribed_sequence_length;
			++n, ++relocated_items_iter, ++transcribed_items_iter)
		{
			const item_type &relocated_item = *relocated_items_iter;
			const item_type &transcribed_item = *transcribed_items_iter;

			scribe.relocated(TRANSCRIBE_SOURCE, relocated_item, transcribed_item);
		}
	}
}

#endif // GPLATES_SCRIBE_TRANSCRIBESEQUENCEPROTOCOL_H
