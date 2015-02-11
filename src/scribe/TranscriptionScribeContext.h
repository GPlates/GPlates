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

#include <map>
#include <stack>
#include <string>
#include <utility>
#include <vector>
#include <boost/optional.hpp>

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
		 * Typedef for an integer object tag version.
		 */
		typedef Transcription::object_tag_version_type object_tag_version_type;


		/**
		 * Transcribe using the specified transcription.
		 *
		 * Saving or loading should be set (via @a is_saving_) depending on whether the
		 * transcription is empty (saving) or whether the transcription has been
		 * loaded from an archive (loading).
		 *
		 * @a root_object_id is the object id of the root object used to store root-level transcribe
		 * calls. It's not a real object - it's just we need to reserve an id to emulate a root object.
		 */
		explicit
		TranscriptionScribeContext(
				const Transcription::non_null_ptr_type &transcription,
				object_id_type root_object_id,
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


		void
		push_transcribed_object(
				object_id_type object_id);

		void
		pop_transcribed_object();


		bool
		transcribe_object_id(
				object_id_type &object_id,
				const char *object_tag,
				object_tag_version_type object_tag_version);


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


			/**
			 * Load the next child object id associated with the child object key.
			 *
			 * Does not apply if this is a primitive (non-composite) object.
			 */
			void
			save_child_object_id(
					object_id_type child_object_id,
					const Transcription::object_key_type &object_key,
					Transcription::CompositeObject &composite_object)
			{
				composite_object.add_child(object_key, child_object_id);
			}

			/**
			 * Load the next child object id associated with the child object key.
			 *
			 * Does not apply if this is a primitive (non-composite) object.
			 */
			bool
			load_child_object_id(
					object_id_type &child_object_id,
					const Transcription::object_key_type &object_key,
					const Transcription::CompositeObject &composite_object);

		private:

			//! Typedef for a range of child object id indices associated with a specific object key.
			typedef std::pair<unsigned int/*index*/, unsigned int/*num_with_key*/> range_type;

			//! Typedef for a map of object keys to child object id indices.
			typedef std::map<Transcription::object_key_type, range_type> child_object_id_map_type;

			//! Used to keep track of which child objects have already been loaded.
			child_object_id_map_type d_child_object_id_map;
		};

		typedef std::stack<TranscribedObject> transcribed_object_stack_type;


		//! Whether transcription was read from an archive or will be written to one.
		bool d_is_saving;

		Transcription::non_null_ptr_type d_transcription;

		transcribed_object_stack_type d_transcribed_object_stack;
	};
}

#endif // GPLATES_SCRIBE_TRANSCRIPTIONSCRIBECONTEXT_H
