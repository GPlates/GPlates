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


			template <typename ObjectType>
			Bytes
			pickle(
					const ObjectType &object)
			{
				// The scribe used to save the object to a transcription.
				GPlatesScribe::Scribe scribe;

				// Transcribe the object.
				scribe.save(TRANSCRIBE_SOURCE, object, "object");

				// Get the transcription.
				GPlatesScribe::Transcription::non_null_ptr_to_const_type object_transcription =
						scribe.get_transcription();

				// Convert transcription to a Bytes object.
				Bytes object_bytes = transcription_to_bytes(*object_transcription);

				return object_bytes;
			}

			template <typename ObjectType>
			GPlatesScribe::LoadRef<ObjectType>
			unpickle(
					const Bytes &object_bytes)
			{
				// Convert the byte stream to the object's transcription.
				GPlatesScribe::Transcription::non_null_ptr_type object_transcription =
						bytes_to_transcription(object_bytes);

				// The scribe used to load the object from the transcription.
				GPlatesScribe::Scribe scribe(object_transcription);

				// Transcribe the object.
				GPlatesScribe::LoadRef<ObjectType> object = scribe.load<ObjectType>(TRANSCRIBE_SOURCE, "object");
				// If transcribing (loading) the object failed then it is due to backwards/forwards
				// compatibility differences between the object that was pickled into the byte stream and
				// the object we are attempting to unpickle. This shouldn't happen unless the version of
				// pygplates that pickled was different than the unpickling version (this version).
				GPlatesGlobal::Assert<GPlatesScribe::Exceptions::UnsupportedVersion>(
						object.is_valid(),
						GPLATES_ASSERTION_SOURCE);

				return object;
			}
		}


		/**
		 * Pickle suite for template type 'ObjectType'.
		 *
		 * This can be used as:
		 *
		 *     .def_pickle(GPlatesApi::PythonPickle::PickleSuite<ObjectType>())
		 *
		 * Note: We only implement 'getinitargs()'.
		 *       We don't use 'getstate()', 'setstate()' or 'getstate_manages_dict()'.
		 *       This is because 'getinitargs()' (a part of boost::python::pickle_suite) will pickle
		 *       an object (of type 'ObjectType') into a byte stream (a Python 'bytes' object), and
		 *       then @a unpickle will reverse that to convert the byte stream back into an object.
		 *
		 * Note: 'ObjectType' must support transcribing (see "scribe/Transcribe.h").
		 */
		template <typename ObjectType>
		class PickleSuite :
				public boost::python::pickle_suite
		{
		public:

			static
			boost::python::tuple
			getinitargs(
					const ObjectType &object)
			{
				// Pickle object into a Bytes object.
				Impl::Bytes object_bytes = Impl::pickle(object);

				// Convert Bytes object from C++ to Python and return it in the tuple.
				return boost::python::make_tuple(object_bytes);
			}
		};


		/**
		 * Initialise an object by unpickling a byte stream back into an object (of type 'ObjectType').
		 *
		 * This can be used to create a single-argument constructor using:
		 *
		 *     .def("__init__", boost::python::make_constructor(&GPlatesApi::PythonPickle::init<ObjectType>))
		 *
		 * Note: 'ObjectType' must support transcribing (see "scribe/Transcribe.h").
		 */
		template <typename ObjectType>
		ObjectType
		init(
				const Impl::Bytes &object_bytes)
		{
			// Unpickle a Bytes object into an object.
			GPlatesScribe::LoadRef<ObjectType> object = Impl::unpickle<ObjectType>(object_bytes);

			// Return a copy of the loaded object.
			return object.get();
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

			template <class PythonClassType>
			void
			visit(
					PythonClassType &python_class) const
			{
				python_class
						.def("__init__", boost::python::make_constructor(&init<ObjectHolderType>))
						.def_pickle(PickleSuite<ObjectHolderType>())
				;
			}
		};
	}
}

#endif // GPLATES_API_PYTHONPICKLE_H
