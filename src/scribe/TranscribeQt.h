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

#ifndef GPLATES_SCRIBE_TRANSCRIBEQT_H
#define GPLATES_SCRIBE_TRANSCRIBEQT_H

#include <vector>
#include <QByteArray>
#include <QDateTime>
#include <QLinkedList>
#include <QList>
#include <QMap>
#include <QMultiMap>
#include <QQueue>
#include <QSet>
#include <QStack>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVector>

#include "ScribeExceptions.h"
#include "Transcribe.h"
#include "TranscribeMappingProtocol.h"
#include "TranscribeSequenceProtocol.h"

#include "global/GPlatesAssert.h"


namespace GPlatesScribe
{
	//
	// Qt library specialisations and overloads of the transcribe function.
	//

	class Scribe;

	/**
	 * Transcribe QByteArray by converting it to base 64 encoding.
	 *
	 * Unlike the overload for QString, the QByteArray can contain arbitrary data including embedded zeros.
	 * This method converts to base 64 which amplifies the data size.
	 */
	TranscribeResult
	transcribe(
			Scribe &scribe,
			QByteArray &qbytearray_object,
			bool transcribed_construct_data);

	//! Transcribe QDateTime.
	TranscribeResult
	transcribe(
			Scribe &scribe,
			QDateTime &qdatetime_object,
			bool transcribed_construct_data);

	/**
	 * Transcribe QString by converting it to UTF8 format.
	 *
	 * The string should not contain embedded zeros.
	 * If it does then use the QByteArray overload instead.
	 */
	TranscribeResult
	transcribe(
			Scribe &scribe,
			QString &qstring_object,
			bool transcribed_construct_data);

	/**
	 * Transcribe QVariant.
	 *
	 * NOTE: If the type stored in the QVariant is a user type (ie, not a builtin type - see 'QVariant::Type')
	 * then it must be registered with 'qRegisterMetaType()' and 'qRegisterMetaTypeStreamOperators()'.
	 * And it must also supply QDataStream '<<' and '>>' operators for the user type.
	 * This also applies to any user types that the stored type depends on (if it is a template type).
	 *
	 * For example a QVariant containing a QList<MyClassType> must register 'MyClassType', but it
	 * doesn't need to register QList since that's a QVariant builtin type.
	 * Note that this differs from a QList<MyClassType> that is *not* wrapped in a QVariant - this is
	 * transcribed using a Scribe both on QList (already provided below) and on MyClassType.
	 *
	 * Essentially transcribing a QVariant means bypassing the Scribe system for anything wrapped inside
	 * the QVariant, and instead relying on QDataStream '<<' and '>>' operators which are not as flexible
	 * (eg, don't directly support backwards/forwards compatibility when MyClassType is changed).
	 *
	 * Note that using boost::variant (instead of QVariant) avoids having to provide QDataStream
	 * '<<' and '>>' operators for user-defined types. Instead it just requires export registration
	 * (see 'ScribeExportRegistration.h').
	 */
	TranscribeResult
	transcribe(
			Scribe &scribe,
			QVariant &qvariant_object,
			bool transcribed_construct_data);


	//
	// Qt containers.
	//
	// NOTE: For Qt containers we do *not* need to overload the 'relocated()' function.
	// This is because all Qt containers use "implicit sharing" where the container data is shared
	// when the container is copied (shallow copy) - a deep copy is only made if the copy is
	// subsequently modified. So, for example, if a QVector< QMap<...> > is loaded by the Scribe
	// then each QMap in the vector is individually loaded/created before being copied into the
	// vector - and that QMap copy does not need a 'relocated()' implementation.
	//
	// See http://doc.qt.digia.com/4.4/shared.html#implicitly-shared
	//


	//! Transcribe QLinkedList.
	template <typename T>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			QLinkedList<T> &linked_list_object,
			bool transcribed_construct_data);

	//! Transcribe QList.
	template <typename T>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			QList<T> &list_object,
			bool transcribed_construct_data);

	//! Transcribe QMap.
	template <typename Key, typename T>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			QMap<Key, T> &map_object,
			bool transcribed_construct_data);

	//! Transcribe QMultiMap.
	template <typename Key, typename T>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			QMultiMap<Key, T> &multimap_object,
			bool transcribed_construct_data);

	//! Transcribe QQueue.
	template <typename T>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			QQueue<T> &queue_object,
			bool transcribed_construct_data);

	//! Transcribe QSet.
	template <typename T>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			QSet<T> &set_object,
			bool transcribed_construct_data);

	//! Transcribe QStack.
	template <typename T>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			QStack<T> &stack_object,
			bool transcribed_construct_data);

	//! Transcribe QStringList.
	TranscribeResult
	transcribe(
			Scribe &scribe,
			QStringList &string_list_object,
			bool transcribed_construct_data);

	//! Transcribe QVector.
	template <typename T>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			QVector<T> &vector_object,
			bool transcribed_construct_data);
}


//
// Template implementation
//


// Placing here avoids cyclic header dependency.
#include "Scribe.h"


namespace GPlatesScribe
{
	/**
	 * QSet transcribe sequence protocol implementation.
	 */
	template <typename T>
	struct TranscribeSequence< QSet<T> >
	{
		typedef QSet<T> sequence_type;
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
			if (sequence.contains(item))
			{
				return false;
			}

			sequence.insert(item);
			return true;
		}
	};


	/**
	 * QMap transcribe mapping protocol implementation.
	 */
	template <class Key, class T>
	struct TranscribeMap< QMap<Key, T> >
	{
		typedef QMap<Key, T> map_type;
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
			return iterator.key();
		}

		static
		const mapped_type &
		get_value(
				map_const_iterator iterator)
		{
			return iterator.value();
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
			// Insert into the map.
			// Note that we use QMap::insertMulti() instead of QMap::insert().
			// This is because it's possible the client has used QMap like a QMultiMap by
			// storing multiple elements with the same key.
			return map.insertMulti(key, value);
		}
	};


	/**
	 * QMultiMap transcribe mapping protocol implementation.
	 */
	template <class Key, class T>
	struct TranscribeMap< QMultiMap<Key, T> > :
			// Delegate to QMap (since QMultiMap inherits from QMap)...
			public TranscribeMap< QMap<Key, T> > // ...and uses QMap::insertMulti().
	{
	};


	template <typename T>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			QLinkedList<T> &linked_list_object,
			bool transcribed_construct_data)
	{
		return transcribe_sequence_protocol(TRANSCRIBE_SOURCE, scribe, linked_list_object);
	}


	template <typename T>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			QList<T> &list_object,
			bool transcribed_construct_data)
	{
		return transcribe_sequence_protocol(TRANSCRIBE_SOURCE, scribe, list_object);
	}


	template <typename Key, typename T>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			QMap<Key, T> &map_object,
			bool transcribed_construct_data)
	{
		return transcribe_mapping_protocol(TRANSCRIBE_SOURCE, scribe, map_object);
	}


	template <typename Key, typename T>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			QMultiMap<Key, T> &multimap_object,
			bool transcribed_construct_data)
	{
		return transcribe_mapping_protocol(TRANSCRIBE_SOURCE, scribe, multimap_object);
	}


	template <typename T>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			QQueue<T> &queue_object,
			bool transcribed_construct_data)
	{
		return transcribe_sequence_protocol(TRANSCRIBE_SOURCE, scribe, queue_object);
	}


	template <typename T>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			QSet<T> &set_object,
			bool transcribed_construct_data)
	{
		return transcribe_sequence_protocol(TRANSCRIBE_SOURCE, scribe, set_object);
	}


	template <typename T>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			QStack<T> &stack_object,
			bool transcribed_construct_data)
	{
		return transcribe_sequence_protocol(TRANSCRIBE_SOURCE, scribe, stack_object);
	}


	template <typename T>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			QVector<T> &vector_object,
			bool transcribed_construct_data)
	{
		return transcribe_sequence_protocol(TRANSCRIBE_SOURCE, scribe, vector_object);
	}
}

#endif // GPLATES_SCRIBE_TRANSCRIBEQT_H
