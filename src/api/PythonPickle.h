/**
 * Copyright (C) 2023 The University of Sydney, Australia
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

#ifndef GPLATES_API_PYTHONPICKLE_H
#define GPLATES_API_PYTHONPICKLE_H

#include <string>
#include <QBuffer>
#include <QByteArray>
#include <QDataStream>

#include "global/python.h"

// Try to only include the heavyweight "Scribe.h" in '.cc' files where possible.
// Included here since it's used in template functions (but this header typically only included in '.cc' files anyway).
#include "scribe/Scribe.h"
#include "scribe/ScribeExceptions.h"


namespace GPlatesApi
{
	namespace PythonPickle
	{
		/**
		 * The pickle "section version" written at the front of every pickle payload (as a top-level
		 * transcription object) by the entry-point below, and checked on unpickle.
		 *
		 * This is the client-owned coarse version gate described in the fast-path ("raw lane") design
		 * (see "doc-cpp/design/scribe-system/fast-path-plan.md"). It sits *above* the two lower-level
		 * gates that already protect a pickle:
		 *   - the raw-stream *codec* version (scribe-owned, written at the head of each raw blob), and
		 *   - the binary *archive* format version (bumped to 1 only when a raw stream is present, so an
		 *     older build reading a newer raw pickle fails cleanly rather than mis-parsing).
		 * Bump this only when the *layout of what PythonPickle itself writes* changes in a way not
		 * already covered by those gates. A newer pickle carrying a higher section version is rejected
		 * here with a clean 'UnsupportedVersion' (rather than mis-loaded).
		 *
		 * Note: Pickles written by an older pygplates (before the raw lane existed) do *not* contain
		 *       this object at all - the unpickle path detects its absence and treats it as version 0,
		 *       and the object itself loads via the raw-lane's general-path fallback (the boundary
		 *       object was saved as COMPOSITE, not RAW_STREAM). See 'Impl::unpickle'.
		 */
		const unsigned int CURRENT_PICKLE_SECTION_VERSION = 0;

		/**
		 * The transcription tag under which @a CURRENT_PICKLE_SECTION_VERSION is stored.
		 */
		const char *const PICKLE_SECTION_VERSION_TAG = "pickle_section_version";


		/**
		 * The default method of transcribing (loading/saving) an object to be used when picking/unpickling an object.
		 *
		 * This just delegates transcribing directly to the object itself (via its holder pointer 'ObjectHolderType'),
		 * which means that the object type must support transcribing (see "scribe/Transcribe.h").
		 *
		 * This class can be specialised for a specific 'ObjectHolderType'. This might be desired if you don't want to
		 * implement transcribing directly on a specific object type (such as when it's not clear the best
		 * way to do that and so you'd rather just implement something that only applies to pickling and not
		 * to other use cases that might rely on direct transcribing such as saving/loading project files).
		 *
		 * Note: If using this default implementation then the object type must support transcribing (see "scribe/Transcribe.h").
		 *       This means 'ObjectHolderType' must also support transcribing, but this is supported for commonly used smart pointers
		 *       like 'boost::shared_ptr<ObjectType>' and 'GPlatesUtils::non_null_intrusive_ptr<ObjectType>', so only
		 *       the object type ('ObjectType') needs to implement transcribing for this default implementation to work.
		 */
		template <typename ObjectHolderType>
		class Transcribe
		{
		public:

			static
			void
			pickle(
					GPlatesScribe::Scribe &scribe,
					const ObjectHolderType &object)
			{
				// This saves the object holder pointer which in turns saves the object
				// (because the holder pointer should be an owning pointer).
				//
				// The 'RAW' option streams the entire object subtree into a single raw-stream blob
				// (the "raw lane") instead of the usual one-transcription-object-per-child encoding -
				// this is the fast path for pickling (see "scribe/ScribeOptions.h" and
				// "doc-cpp/design/scribe-system/fast-path-plan.md").
				scribe.save(TRANSCRIBE_SOURCE, object, "object", GPlatesScribe::RAW);
			}

			static
			ObjectHolderType
			unpickle(
					GPlatesScribe::Scribe &scribe)
			{
				// This loads the object holder pointer which in turns loads the object
				// (because the holder pointer should be an owning pointer).
				//
				// The 'RAW' option is the counterpart to the 'RAW' save above. Note that it is safe
				// to pass on load even for pickles that were *not* saved with it (eg, pickles written
				// by an older pygplates without the raw lane): the boundary object dispatches on its
				// transcription kind, so a COMPOSITE boundary transparently falls back to the general
				// path (see "scribe/ScribeOptions.h").
				GPlatesScribe::LoadRef<ObjectHolderType> object =
						scribe.load<ObjectHolderType>(TRANSCRIBE_SOURCE, "object", GPlatesScribe::RAW);
				// If transcribing (loading) the object failed then it is due to backwards/forwards
				// compatibility differences between the object that was pickled into the byte stream and
				// the object we are attempting to unpickle. This shouldn't happen unless the version of
				// pygplates that pickled was different than the unpickling version (this version).
				GPlatesGlobal::Assert<GPlatesScribe::Exceptions::UnsupportedVersion>(
						object.is_valid(),
						GPLATES_ASSERTION_SOURCE);

				return object.get();
			}
		};


		namespace Impl
		{
			/**
			 * Pickled objects are transcribed as a byte stream.
			 *
			 * Pickling involves serialising a C++ object to a transcribed byte stream.
			 * Unpickling involves unserialising a transcribed byte stream back to a C++ object.
			 */
			struct Bytes
			{
				//! When passing from C++ to Python.
				explicit
				Bytes(
						const QByteArray &bytes_) :
					bytes(bytes_)
				{  }

				//! When passing from Python to C++.
				explicit
				Bytes(
						boost::python::object bytes_object);

				QByteArray bytes;
			};


			/**
			 * Convert an object transcription to a byte stream.
			 */
			Bytes
			transcription_to_bytes(
					const GPlatesScribe::Transcription &object_transcription);

			/**
			 * Convert a byte stream to an object transcription.
			 */
			GPlatesScribe::Transcription::non_null_ptr_type
			bytes_to_transcription(
					const Bytes &object_bytes);


			template <typename ObjectHolderType>
			Bytes
			pickle(
					const ObjectHolderType &object)
			{
				// The scribe used to save the object to a transcription.
				GPlatesScribe::Scribe scribe;

				// Write the pickle section version at the front of the pickle payload (as a top-level
				// transcription object) so that a future layout change can be detected on unpickle.
				const unsigned int pickle_section_version = CURRENT_PICKLE_SECTION_VERSION;
				scribe.save(TRANSCRIBE_SOURCE, pickle_section_version, PICKLE_SECTION_VERSION_TAG);

				// Transcribe the object.
				Transcribe<ObjectHolderType>::pickle(scribe, object);

				// Get the transcription.
				GPlatesScribe::Transcription::non_null_ptr_to_const_type object_transcription =
						scribe.get_transcription();

				// Convert transcription to a Bytes object.
				Bytes object_bytes = transcription_to_bytes(*object_transcription);

				return object_bytes;
			}

			template <typename ObjectHolderType>
			ObjectHolderType
			unpickle(
					const Bytes &object_bytes)
			{
				// Convert the byte stream to the object's transcription.
				GPlatesScribe::Transcription::non_null_ptr_type object_transcription =
						bytes_to_transcription(object_bytes);

				// The scribe used to load the object from the transcription.
				GPlatesScribe::Scribe scribe(object_transcription);

				// Check the pickle section version at the front of the pickle payload.
				//
				// Pickles written by an older pygplates (before the raw lane existed) do not contain
				// this object - treat its absence as version 0. Using 'is_in_transcription' (a pure
				// query that does not touch the load state) avoids attempting - and failing - a load
				// of a tag that isn't present.
				unsigned int pickle_section_version = 0;
				if (scribe.is_in_transcription(PICKLE_SECTION_VERSION_TAG))
				{
					GPlatesScribe::LoadRef<unsigned int> loaded_section_version =
							scribe.load<unsigned int>(TRANSCRIBE_SOURCE, PICKLE_SECTION_VERSION_TAG);
					GPlatesGlobal::Assert<GPlatesScribe::Exceptions::UnsupportedVersion>(
							loaded_section_version.is_valid(),
							GPLATES_ASSERTION_SOURCE);
					pickle_section_version = loaded_section_version.get();
				}

				// A pickle written by a *newer* pygplates may use a layout this version does not
				// understand - reject it cleanly rather than mis-loading it.
				GPlatesGlobal::Assert<GPlatesScribe::Exceptions::UnsupportedVersion>(
						pickle_section_version <= CURRENT_PICKLE_SECTION_VERSION,
						GPLATES_ASSERTION_SOURCE);

				// Transcribe the object.
				return Transcribe<ObjectHolderType>::unpickle(scribe);
			}


			/**
			 * Pickle suite for template type 'ObjectHolderType'.
			 *
			 * This is used as:
			 *
			 *     .def_pickle(GPlatesApi::PythonPickle::Impl::PickleSuite<ObjectHolderType>())
			 *
			 * Note: Previously we only implemented 'getinitargs()' - not 'getstate()', 'setstate()' or 'getstate_manages_dict()'.
			 *       This is because 'getinitargs()' (a part of boost::python::pickle_suite) will pickle an object
			 *       (of type 'ObjectHolderType') into a byte stream (a Python 'bytes' object), and then
			 *       @a unpickle will reverse that to convert the byte stream back into an object.
			 *       And so we only needed 'getinitargs()' to achieve this.
			 *       However, if an attribute is added to a pyGPlates object from the Python side then we would
			 *       get the following error when pickling it:
			 *           RuntimeError: Incomplete pickle support (__getstate_manages_dict__ not set)
			 *       So now we also implement 'getstate()' and 'setstate()' (and 'getstate_manages_dict()') such that the
			 *       Python object's __dict__ is also pickled. Note that __dict__ will be empty if no attributes were added.
			 */
			template <typename ObjectHolderType>
			class PickleSuite :
					public boost::python::pickle_suite
			{
			public:

				static
				boost::python::tuple
				getinitargs(
						const ObjectHolderType &object)
				{
					// Pickle object into a Bytes object.
					Bytes object_bytes = pickle(object);

					// Convert Bytes object from C++ to Python and return it in the tuple.
					return boost::python::make_tuple(object_bytes);
				}

				static
				boost::python::tuple
				getstate(
						boost::python::object object)
				{
					// Save the __dict__ of the Python object.
					return boost::python::make_tuple(object.attr("__dict__"));
				}

				static
				void
				setstate(
						boost::python::object object,
						boost::python::tuple state)
				{
					// Restore the __dict__ of the Python object.
					boost::python::extract<boost::python::dict> extract_state_dict(state[0]);
					if (extract_state_dict.check())
					{
						boost::python::dict state_dict = extract_state_dict();
						if (state_dict)  // not empty
						{
							object.attr("__dict__").attr("update")(state_dict);
						}
					}
				}

				static
				bool
				getstate_manages_dict()
				{
					// Signal that we are handling the __dict__ (by copying it).
					return true;
				}
			};

			/**
			 * Initialise an object by unpickling a byte stream back into an object.
			 *
			 * This is used to create a single-argument constructor using:
			 *
			 *     .def("__init__", boost::python::make_constructor(&GPlatesApi::PythonPickle::Impl::init<ObjectHolderType>))
			 */
			template <typename ObjectHolderType>
			ObjectHolderType
			init(
					const Bytes &object_bytes)
			{
				// Unpickle a Bytes object into an object.
				return unpickle<ObjectHolderType>(object_bytes);
			}
		}


		/**
		 * A boost::python::class_ "def" visitor that handles pickling/unpicking for an object.
		 *
		 * The 'ObjectHolderType' is typically a smart pointer (such as boost::shared_ptr<ObjectType>).
		 *
		 * If 'ObjectType' is wrapped with "boost::python::class_<ObjectType, HeldType>" then use 'HeldType' for 'ObjectHolderType':
		 *
		 *     .def(GPlatesApi::PythonPickle::PickleDefVisitor<HeldType>())
		 *
		 * If it's wrapped with just "boost::python::class_<ObjectType>" then use 'boost::shared_ptr<ObjectType>' for 'ObjectHolderType':
		 *
		 *     .def(GPlatesApi::PythonPickle::PickleDefVisitor<boost::shared_ptr<ObjectType>>())
		 *
		 * Note: 'ObjectType' must support transcribing (see "scribe/Transcribe.h").
		 *       This means 'ObjectHolderType' must also support transcribing, but this is supported for commonly used smart pointers
		 *       like 'boost::shared_ptr<ObjectType>' and 'GPlatesUtils::non_null_intrusive_ptr<ObjectType>'.
		 */
		template <typename ObjectHolderType>
		class PickleDefVisitor :
				public boost::python::def_visitor<PickleDefVisitor<ObjectHolderType>>
		{
		public:

			/**
			 * Construct a visitor to handle pickling/unpicking for an object type.
			 *
			 * Optionally document the pickled class as non-instantiable (in docstring of __init__ method generated for pickling).
			 *
			 * This is useful when a class's only constructor (__init__) is for pickling (ie, the class does not define its own __init__ methods).
			 * In this case, if you hadn't provided pickle support for your class then you would just have had bp::no_init which causes boost-python
			 * to automatically generate the __init__ docstring: "Raises an exception\nThis class cannot be instantiated from Python\n".
			 * However, because an __init__ method is generated (for pickling) then boost-python will not do this.
			 * And so the user reading the API documentation could get confused (and think the class *can* be constructed).
			 * In this case you can set @a document_init_as_non_instantiable to 'true' so that a note about this is added to the docstring.
			 */
			PickleDefVisitor(
					bool document_class_as_non_instantiable = false) :
				d_document_class_as_non_instantiable(document_class_as_non_instantiable)
			{  }

			template <class PythonClassType>
			void
			visit(
					PythonClassType &python_class) const
			{
				std::string constructor_docstring;
				if (d_document_class_as_non_instantiable)
				{
					// Document class as non-instantiable.
					constructor_docstring =
							"You cannot directly instantiate this class from Python.\n"
							"\n"
							".. note:: This constructor is only provided for `pickle <https://docs.python.org/3/library/pickle.html>`_ support.\n";
				}
				else
				{
					// Note: Instead of an empty (or no) docstring we use a single space.
					//       This prevents boost-python from duplicating the docstring of the last
					//       __init__ method added to the class.
					constructor_docstring = " ";
				}

				python_class.def(
						"__init__",
						boost::python::make_constructor(&Impl::init<ObjectHolderType>),
						constructor_docstring.c_str());

				python_class.def_pickle(Impl::PickleSuite<ObjectHolderType>());
			}
			
		private:
			bool d_document_class_as_non_instantiable;
		};
	}
}

#endif // GPLATES_API_PYTHONPICKLE_H
