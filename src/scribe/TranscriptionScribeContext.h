/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2014 The University of Sydney, Australia
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

#ifndef GPLATES_SCRIBE_TRANSCRIPTIONSCRIBECONTEXT_H
#define GPLATES_SCRIBE_TRANSCRIPTIONSCRIBECONTEXT_H

#include <stack>
#include <string>
#include <vector>
#include <boost/optional.hpp>

#include "ScribeObjectTag.h"
#include "Transcription.h"


namespace GPlatesScribe
{
	/**
	 * A TranscriptionScribeContext is used by class Scribe to transcribe the object network to/from a Transcription.
	 *
	 * Archive readers and writers access Transcription directly but class Scribe, which
	 * performs transcribing, uses TranscriptionScribeContext to indirectly read/write Transcription.
	 */
	class TranscriptionScribeContext
	{
	public:

		/**
		 * Typedef for an integer identifier for a transcribed object.
		 */
		typedef Transcription::object_id_type object_id_type;


		/**
		 * A value of 0 is used to identify NULL pointers.
		 *
		 * Ie, the pointed-to object has an object id of 0 (not the pointer 'object' itself).
		 */
		static const object_id_type NULL_POINTER_OBJECT_ID = 0;

		/**
		 * The object id of the root object used to store root-level transcribe calls.
		 *
		 * It's not a real object - it's just we need to reserve an ID to emulate a root object.
		 */
		static const object_id_type ROOT_OBJECT_ID = 1;


		/**
		 * Transcribe using the specified transcription.
		 *
		 * Saving or loading should be set (via @a is_saving_) depending on whether the
		 * transcription is empty (saving) or whether the transcription has been
		 * loaded from an archive (loading).
		 */
		explicit
		TranscriptionScribeContext(
				const Transcription::non_null_ptr_type &transcription,
				bool is_saving_);


		/**
		 * Is saving state (that can be written to an archive).
		 */
		bool
		is_saving() const
		{
			return d_is_saving;
		}


		/**
		 * Is loading state (that was read from an archive).
		 */
		bool
		is_loading() const
		{
			return !is_saving();
		}


		/**
		 * Allocate the next available object id.
		 *
		 * This is only needed for the 'save' path in class Scribe.
		 */
		object_id_type
		allocate_save_object_id();


		/**
		 * Determines whether the specified object tag exists in the transcription
		 * (transcription is either being written to, on save path, or read from, on load path).
		 *
		 * As with @a transcribe_object_id the object tag is relative to the scope of the parent
		 * transcribed object (if any) - see @a push_transcribed_object.
		 */
		boost::optional<object_id_type>
		is_in_transcription(
				const ObjectTag &object_tag) const;


		/**
		 * Transcribe the (child) object ID associated with the object tag that is relative to the
		 * currently pushed transcribed (parent) object (see @a push_transcribed_object).
		 *
		 * The first call will be relative to the root object.
		 */
		bool
		transcribe_object_id(
				object_id_type &object_id,
				const ObjectTag &object_tag);


		/**
		 * All subsequent @a transcribe and @a transcribe_object_id calls will now be
		 * relative to the specified object (@a object_id).
		 *
		 * If @a transcribe is subsequently called then @a object_id will be a primitive object.
		 *
		 * If @a transcribe_object_id is subsequently called then @a object_id will be a composite object
		 * (with the @a transcribe_object_id call and associated call to @a push_transcribed_object
		 * referring to one of its child objects which, in turn, could be a composite or primitive object).
		 */
		void
		push_transcribed_object(
				object_id_type object_id);

		void
		pop_transcribed_object();


		/**
		 * Transcribe a std::string primitive.
		 *
		 * This is handled as a special-case primitive (in addition to the integral and
		 * floating-point types) in order to generate more compact archives than would
		 * be possible with an array of 'char'.
		 */
		bool
		transcribe(
				std::string &object);


		//
		// Transcribe integral and floating-point primitives.
		//
		// NOTE: Anything that satisfies the "boost::is_arithmetic" type traits should be handled here.
		// This is because Scribe delegates these types directly to this interface
		// bypassing the general non-member 'GPlatesScribe::transcribe()' mechanism.
		//

		bool
		transcribe(
				bool &object);

		// Apparently 'char', 'signed char' and 'unsigned char' are three distinct types (unlike integer types).
		bool
		transcribe(
				char &object);

		bool
		transcribe(
				signed char &object);

		bool
		transcribe(
				unsigned char &object);

		bool
		transcribe(
				short &object);

		bool
		transcribe(
				unsigned short &object);

		bool
		transcribe(
				int &object);

		bool
		transcribe(
				unsigned int &object);

		bool
		transcribe(
				long &object);

		bool
		transcribe(
				unsigned long &object);

		bool
		transcribe(
				float &object);

		bool
		transcribe(
				double &object);

		bool
		transcribe(
				long double &object);

	private:

		/**
		 * Used to keep track of the object currently being transcribed.
		 */
		struct TranscribedObject
		{
			enum ObjectCategory
			{
				COMPOSITE,
				PRIMITIVE
			};


			explicit
			TranscribedObject(
					object_id_type object_id_) :
				object_id(object_id_)
			{  }


			//! Whether this transcribed object is primitive or composite.
			boost::optional<ObjectCategory> object_category;

			//! Object id of this transcribed object.
			object_id_type object_id;
		};

		typedef std::stack<TranscribedObject> transcribed_object_stack_type;


		//! Whether transcription was read from an archive or will be written to one.
		bool d_is_saving;

		//! The next available object id for the *save* path.
		unsigned int d_next_save_object_id;

		Transcription::non_null_ptr_type d_transcription;

		transcribed_object_stack_type d_transcribed_object_stack;


		void
		save_tag_section(
				const std::string &tag_name,
				unsigned int tag_version,
				Transcription::CompositeObject *&section_composite_object,
				boost::optional<object_id_type &> object_id);
		bool
		load_tag_section(
				const std::string &tag_name,
				unsigned int tag_version,
				Transcription::CompositeObject *&section_composite_object,
				boost::optional<object_id_type &> object_id) const;


		void
		save_array_index_section(
				const std::string &array_item_tag_name,
				unsigned int array_item_tag_version,
				unsigned int array_index,
				Transcription::CompositeObject *&section_composite_object,
				boost::optional<object_id_type &> object_id);

		bool
		load_array_index_section(
				const std::string &array_item_tag_name,
				unsigned int array_item_tag_version,
				unsigned int array_index,
				Transcription::CompositeObject *&section_composite_object,
				boost::optional<object_id_type &> object_id) const;


		void
		save_array_size_section(
				const std::string &array_size_tag_name,
				unsigned int array_size_tag_version,
				Transcription::CompositeObject *&section_composite_object,
				boost::optional<object_id_type &> object_id);

		bool
		load_array_size_section(
				const std::string &array_size_tag_name,
				unsigned int array_size_tag_version,
				Transcription::CompositeObject *&section_composite_object,
				boost::optional<object_id_type &> object_id) const;
	};
}

#endif // GPLATES_SCRIBE_TRANSCRIPTIONSCRIBECONTEXT_H
