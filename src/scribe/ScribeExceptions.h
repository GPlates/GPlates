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

#ifndef GPLATES_SCRIBE_SCRIBEEXCEPTIONS_H
#define GPLATES_SCRIBE_SCRIBEEXCEPTIONS_H

#include <string>
#include <typeinfo>
#include <boost/optional.hpp>
#include <QString>
#include <QStringList>

#include "global/GPlatesException.h"


namespace GPlatesScribe
{
	namespace Exceptions
	{
		/**
		 * The base exception class for all Scribe exceptions.
		 *
		 * This can be caught if you just want to catch all Scribe exceptions and are not
		 * interested in the specific Scribe error.
		 */
		class BaseException :
				public GPlatesGlobal::Exception
		{
		public:

			~BaseException() throw() { }

		protected:

			explicit
			BaseException(
					const GPlatesUtils::CallStack::Trace &exception_source) :
				GPlatesGlobal::Exception(exception_source)
			{  }

		};


		/**
		 * Exception thrown if the archive stream (being read) was written using a future version
		 * of the scribe library and/or archive.
		 */
		class UnsupportedVersion :
				public BaseException
		{
		public:

			explicit
			UnsupportedVersion(
					const GPlatesUtils::CallStack::Trace &exception_source) :
				BaseException(exception_source)
			{  }

			~UnsupportedVersion() throw() { }

		protected:

			virtual
			const char *
			exception_name() const
			{
				return "UnsupportedVersion";
			}

			virtual
			void
			write_message(
					std::ostream &os) const;

		private:

		};


		/**
		 * Exception thrown if the archive stream has an invalid signature.
		 */
		class InvalidArchiveSignature :
				public BaseException
		{
		public:

			explicit
			InvalidArchiveSignature(
					const GPlatesUtils::CallStack::Trace &exception_source) :
				BaseException(exception_source)
			{  }

			~InvalidArchiveSignature() throw() { }

		protected:

			virtual
			const char *
			exception_name() const
			{
				return "InvalidArchiveSignature";
			}

			virtual
			void
			write_message(
					std::ostream &os) const;

		private:

		};


		/**
		 * Exception thrown due to failure to read or write to the archive stream.
		 *
		 * This is mostly due to the standard stream failbit, badbit or eofbit encountered
		 * when reading/writing.
		 */
		class ArchiveStreamError :
				public BaseException
		{
		public:

			explicit
			ArchiveStreamError(
					const GPlatesUtils::CallStack::Trace &exception_source,
					const std::string &message) :
				BaseException(exception_source),
				d_message(message)
			{  }

			~ArchiveStreamError() throw() { }

		protected:

			virtual
			const char *
			exception_name() const
			{
				return "ArchiveStreamError";
			}

			virtual
			void
			write_message(
					std::ostream &os) const;

		private:

			std::string d_message;
		};


		/**
		 * A non-specific error internal to the Scribe library.
		 *
		 * This indicates either an error in the transcribed stream/archive or an error
		 * in the Scribe library implementation.
		 *
		 * Errors due to incorrect usage of the Scribe library should generate different exceptions.
		 * Although in some cases this exception can get thrown due to either an internal library error
		 * or incorrect usage of the Scribe library.
		 */
		class ScribeLibraryError :
				public BaseException
		{
		public:

			ScribeLibraryError(
					const GPlatesUtils::CallStack::Trace &exception_source,
					const std::string &message) :
				BaseException(exception_source),
				d_message(message)
			{  }

			~ScribeLibraryError() throw() { }

		protected:

			virtual
			const char *
			exception_name() const
			{
				return "ScribeLibraryError";
			}

			virtual
			void
			write_message(
					std::ostream &os) const;

		private:

			std::string d_message;
		};


		/**
		 * A non-specific error in the usage of the Scribe library (not a bug in the library itself).
		 *
		 * This is used for things like calling Scribe library functions when saving/creating
		 * an archive, but that should only be called when loading an archive.
		 * More specific usage errors are listed below.
		 */
		class ScribeUserError :
				public BaseException
		{
		public:

			ScribeUserError(
					const GPlatesUtils::CallStack::Trace &exception_source,
					const std::string &message) :
				BaseException(exception_source),
				d_message(message)
			{  }

			~ScribeUserError() throw() { }

		protected:

			virtual
			const char *
			exception_name() const
			{
				return "ScribeUserError";
			}

			virtual
			void
			write_message(
					std::ostream &os) const;

		private:

			std::string d_message;
		};


		/**
		 * This exception is thrown when a transcribe result from class Scribe (eg, Scribe::transcribe())
		 * has not been checked.
		 */
		class ScribeTranscribeResultNotChecked :
				public BaseException
		{
		public:

			explicit
			ScribeTranscribeResultNotChecked(
					const GPlatesUtils::CallStack::Trace &exception_source) :
				BaseException(exception_source)
			{  }

			~ScribeTranscribeResultNotChecked() throw() { }

		protected:

			virtual
			const char *
			exception_name() const
			{
				return "ScribeTranscribeResultNotChecked";
			}

			virtual
			void
			write_message(
					std::ostream &os) const;

		};


		/**
		 * Use this exception when you don't want a class type to be save/load constructed (only transcribed).
		 */
		class ConstructNotAllowed :
				public BaseException
		{
		public:

			explicit
			ConstructNotAllowed(
					const GPlatesUtils::CallStack::Trace &exception_source,
					const std::type_info &object_type_info) :
				BaseException(exception_source),
				d_object_type_name(object_type_info.name())
			{  }

			~ConstructNotAllowed() throw() { }

		protected:

			virtual
			const char *
			exception_name() const
			{
				return "ConstructNotAllowed";
			}

			virtual
			void
			write_message(
					std::ostream &os) const;

		private:

			std::string d_object_type_name;
		};


		/**
		 * When invalid options are passed to Scribe::transcribe().
		 */
		class InvalidTranscribeOptions :
				public BaseException
		{
		public:

			explicit
			InvalidTranscribeOptions(
					const GPlatesUtils::CallStack::Trace &exception_source,
					const std::string &message) :
				BaseException(exception_source),
				d_message(message)
			{  }

			~InvalidTranscribeOptions() throw() { }

		protected:

			virtual
			const char *
			exception_name() const
			{
				return "InvalidTranscribeOptions";
			}

			virtual
			void
			write_message(
					std::ostream &os) const;

		private:

			std::string d_message;
		};


		/**
		 * When the start or end of an XML element with a specific element name is not encountered.
		 */
		class UnexpectedXmlElementName :
				public BaseException
		{
		public:

			explicit
			UnexpectedXmlElementName(
					const GPlatesUtils::CallStack::Trace &exception_source,
					const QString &element_name,
					bool is_start_element) :
				BaseException(exception_source),
				d_element_names(element_name),
				d_is_start_element(is_start_element)
			{  }

			explicit
			UnexpectedXmlElementName(
					const GPlatesUtils::CallStack::Trace &exception_source,
					const QStringList &element_names,
					bool is_start_element) :
				BaseException(exception_source),
				d_element_names(element_names),
				d_is_start_element(is_start_element)
			{  }

			~UnexpectedXmlElementName() throw() { }

		protected:

			virtual
			const char *
			exception_name() const
			{
				return "UnexpectedXmlElementName";
			}

			virtual
			void
			write_message(
					std::ostream &os) const;

		private:

			QStringList d_element_names;
			bool d_is_start_element;
		};


		/**
		 * An invalid XML element name (obtained via an object tag).
		 */
		class InvalidXmlElementName :
				public BaseException
		{
		public:

			explicit
			InvalidXmlElementName(
					const GPlatesUtils::CallStack::Trace &exception_source,
					boost::optional<std::string> xml_element_name = boost::none) :
				BaseException(exception_source),
				d_xml_element_name(xml_element_name)
			{  }

			~InvalidXmlElementName() throw() { }

		protected:

			virtual
			const char *
			exception_name() const
			{
				return "InvalidXmlElementName";
			}

			virtual
			void
			write_message(
					std::ostream &os) const;

		private:

			boost::optional<std::string> d_xml_element_name;
		};


		/**
		 * Exception thrown when a parse error reading XML stream is encountered.
		 */
		class XmlStreamParseError :
				public BaseException
		{
		public:

			explicit
			XmlStreamParseError(
					const GPlatesUtils::CallStack::Trace &exception_source,
					const QString &xml_error_message) :
				BaseException(exception_source),
				d_xml_error_message(xml_error_message)
			{  }

			~XmlStreamParseError() throw() { }

		protected:

			virtual
			const char *
			exception_name() const
			{
				return "XmlStreamParseError";
			}

			virtual
			void
			write_message(
					std::ostream &os) const;

		private:

			QString d_xml_error_message;
		};


		/**
		 * Exception thrown when a transcription is incomplete (eg, there are uninitialised
		 * transcribed objects after an archive has been saved or loaded).
		 */
		class TranscriptionIncomplete :
				public BaseException
		{
		public:

			TranscriptionIncomplete(
					const GPlatesUtils::CallStack::Trace &exception_source) :
				BaseException(exception_source)
			{  }

			~TranscriptionIncomplete() throw() { }

		protected:

			virtual
			const char *
			exception_name() const
			{
				return "TranscriptionIncomplete";
			}

			virtual
			void
			write_message(
					std::ostream &os) const;

		};


		/**
		 * Exception thrown when a transcription was not able to be transcribed because it was
		 * incompatible (this can happen due to breaking of backward/forward compatibility).
		 */
		class TranscriptionIncompatible :
				public BaseException
		{
		public:

			TranscriptionIncompatible(
					const GPlatesUtils::CallStack::Trace &exception_source) :
				BaseException(exception_source)
			{  }

			~TranscriptionIncompatible() throw() { }

		protected:

			virtual
			const char *
			exception_name() const
			{
				return "TranscriptionIncompatible";
			}

			virtual
			void
			write_message(
					std::ostream &os) const;

		};


		/**
		 * Exception thrown when transcribing a reference-to-an-object instead of the object directly
		 * and the object's actual (RTTI) type is different than the reference type.
		 */
		class TranscribedReferenceInsteadOfObject :
				public BaseException
		{
		public:

			template <typename ObjectType>
			TranscribedReferenceInsteadOfObject(
					const GPlatesUtils::CallStack::Trace &exception_source,
					const ObjectType &referenced_object) :
				BaseException(exception_source),
				d_reference_type_name(typeid(ObjectType).name()),
				d_object_type_name(typeid(referenced_object).name())
			{  }

			~TranscribedReferenceInsteadOfObject() throw() { }

		protected:

			virtual
			const char *
			exception_name() const
			{
				return "TranscribedReferenceInsteadOfObject";
			}

			virtual
			void
			write_message(
					std::ostream &os) const;

		private:

			std::string d_reference_type_name;
			std::string d_object_type_name;
		};


		/**
		 * Exception thrown if a tracked object has already been saved at a particular memory address,
		 * or already been loaded (at same object tag location in transcription).
		 */
		class AlreadyTranscribedObject :
				public BaseException
		{
		public:

			AlreadyTranscribedObject(
					const GPlatesUtils::CallStack::Trace &exception_source,
					const std::type_info &object_type_info,
					bool scribe_is_saving) :
				BaseException(exception_source),
				d_object_type_name(object_type_info.name()),
				d_scribe_is_saving(scribe_is_saving)
			{  }

			~AlreadyTranscribedObject() throw() { }

		protected:

			virtual
			const char *
			exception_name() const
			{
				return "AlreadyTranscribedObject";
			}

			virtual
			void
			write_message(
					std::ostream &os) const;

		private:

			std::string d_object_type_name;
			bool d_scribe_is_saving;
		};


		/**
		 * Exception thrown if an attempted to transcribe an object via an owning pointer but
		 * the object has already been transcribed without one.
		 */
		class AlreadyTranscribedObjectWithoutOwningPointer :
				public BaseException
		{
		public:

			AlreadyTranscribedObjectWithoutOwningPointer(
					const GPlatesUtils::CallStack::Trace &exception_source,
					const std::type_info &object_type_info) :
				BaseException(exception_source),
				d_object_type_name(object_type_info.name())
			{  }

			~AlreadyTranscribedObjectWithoutOwningPointer() throw() { }

		protected:

			virtual
			const char *
			exception_name() const
			{
				return "AlreadyTranscribedObjectWithoutOwningPointer";
			}

			virtual
			void
			write_message(
					std::ostream &os) const;

		private:

			std::string d_object_type_name;
		};


		/**
		 * Exception thrown when an untracked pointer is transcribed before the pointed-to object -
		 * because its untracked it won't get initialised properly later when the pointed-to object
		 * is transcribed.
		 */
		class TranscribedUntrackedPointerBeforeReferencedObject :
				public BaseException
		{
		public:

			TranscribedUntrackedPointerBeforeReferencedObject(
					const GPlatesUtils::CallStack::Trace &exception_source,
					const std::type_info &object_type_info) :
				BaseException(exception_source),
				d_object_type_name(object_type_info.name())
			{  }

			~TranscribedUntrackedPointerBeforeReferencedObject() throw() { }

		protected:

			virtual
			const char *
			exception_name() const
			{
				return "TranscribedUntrackedPointerBeforeReferencedObject";
			}

			virtual
			void
			write_message(
					std::ostream &os) const;

		private:

			std::string d_object_type_name;
		};


		/**
		 * Exception thrown when an object is untracked (or discarded) and it has transcribed pointers
		 * or references referencing it.
		 */
		class UntrackingObjectWithReferences :
				public BaseException
		{
		public:

			UntrackingObjectWithReferences(
					const GPlatesUtils::CallStack::Trace &exception_source,
					const std::type_info &object_type_info) :
				BaseException(exception_source),
				d_object_type_name(object_type_info.name())
			{  }

			~UntrackingObjectWithReferences() throw() { }

		protected:

			virtual
			const char *
			exception_name() const
			{
				return "UntrackingObjectWithReferences";
			}

			virtual
			void
			write_message(
					std::ostream &os) const;

		private:

			std::string d_object_type_name;
		};


		/**
		 * Exception thrown when a reference-to-an-object cannot find the referenced object at
		 * the time when the reference is transcribed.
		 */
		class TranscribedReferenceBeforeReferencedObject :
				public BaseException
		{
		public:

			TranscribedReferenceBeforeReferencedObject(
					const GPlatesUtils::CallStack::Trace &exception_source,
					const std::type_info &object_type_info) :
				BaseException(exception_source),
				d_object_type_name(object_type_info.name())
			{  }

			~TranscribedReferenceBeforeReferencedObject() throw() { }

		protected:

			virtual
			const char *
			exception_name() const
			{
				return "TranscribedReferenceBeforeReferencedObject";
			}

			virtual
			void
			write_message(
					std::ostream &os) const;

		private:

			std::string d_object_type_name;
		};


		/**
		 * Exception thrown when relocating a reference-to-an-object instead of the object directly
		 * and the object's actual (RTTI) type is different than the reference type.
		 */
		class RelocatedReferenceInsteadOfObject :
				public BaseException
		{
		public:

			template <typename ObjectType>
			RelocatedReferenceInsteadOfObject(
					const GPlatesUtils::CallStack::Trace &exception_source,
					const ObjectType &referenced_object) :
				BaseException(exception_source),
				d_reference_type_name(typeid(ObjectType).name()),
				d_object_type_name(typeid(referenced_object).name())
			{  }

			~RelocatedReferenceInsteadOfObject() throw() { }

		protected:

			virtual
			const char *
			exception_name() const
			{
				return "RelocatedReferenceInsteadOfObject";
			}

			virtual
			void
			write_message(
					std::ostream &os) const;

		private:

			std::string d_reference_type_name;
			std::string d_object_type_name;
		};


		/**
		 * Exception thrown when a reference-to-an-object cannot find the referenced object at
		 * the time when the reference is transcribed.
		 */
		class RelocatedUntrackedObject :
				public BaseException
		{
		public:

			explicit
			RelocatedUntrackedObject(
					const GPlatesUtils::CallStack::Trace &exception_source) :
				BaseException(exception_source)
			{  }

			~RelocatedUntrackedObject() throw() { }

		protected:

			virtual
			const char *
			exception_name() const
			{
				return "RelocatedUntrackedObject";
			}

			virtual
			void
			write_message(
					std::ostream &os) const;

		private:

		};


		/**
		 * Exception thrown when an attempt is made to relocate a transcribed object that already
		 * has a reference bound to it (the reference cannot be re-bound to the relocated object) or
		 * an untracked pointer (cannot be updated to point to relocated object).
		 */
		class RelocatedObjectBoundToAReferenceOrUntrackedPointer :
				public BaseException
		{
		public:

			explicit
			RelocatedObjectBoundToAReferenceOrUntrackedPointer(
					const GPlatesUtils::CallStack::Trace &exception_source) :
				BaseException(exception_source)
			{  }

			~RelocatedObjectBoundToAReferenceOrUntrackedPointer() throw() { }

		protected:

			virtual
			const char *
			exception_name() const
			{
				return "RelocatedObjectBoundToAReferenceOrUntrackedPointer";
			}

			virtual
			void
			write_message(
					std::ostream &os) const;

		private:

		};


		/**
		 * Exception thrown when a tracked object is loaded (in Scribe::load()) but was not relocated.
		 */
		class LoadedObjectTrackedButNotRelocated :
				public BaseException
		{
		public:

			explicit
			LoadedObjectTrackedButNotRelocated(
					const GPlatesUtils::CallStack::Trace &exception_source) :
				BaseException(exception_source)
			{  }

			~LoadedObjectTrackedButNotRelocated() throw() { }

		protected:

			virtual
			const char *
			exception_name() const
			{
				return "LoadedObjectTrackedButNotRelocated";
			}

			virtual
			void
			write_message(
					std::ostream &os) const;

		private:

		};


		/**
		 * Exception thrown when unable to void cast between a derived and base class.
		 */
		class UnregisteredCast :
				public BaseException
		{
		public:

			UnregisteredCast(
					const GPlatesUtils::CallStack::Trace &exception_source,
					const std::type_info &derived_class_type,
					const std::type_info &base_class_type) :
				BaseException(exception_source),
				d_derived_class_name(derived_class_type.name()),
				d_base_class_name(base_class_type.name())
			{  }

			~UnregisteredCast() throw() { }

		protected:

			virtual
			const char *
			exception_name() const
			{
				return "UnregisteredCast";
			}

			virtual
			void
			write_message(
					std::ostream &os) const;

		private:

			std::string d_derived_class_name;
			std::string d_base_class_name;
		};


		/**
		 * Exception thrown when there is more than one path between between a derived and a base class.
		 *
		 * For example:
		 *
		 *  A   A
		 *  |   |
		 *  B   C
		 *   \ /
		 *    D
		 *
		 * ...will generate the exception between class D and class A.
		 */
		class AmbiguousCast :
				public BaseException
		{
		public:

			AmbiguousCast(
					const GPlatesUtils::CallStack::Trace &exception_source,
					const std::type_info &derived_class_type,
					const std::type_info &base_class_type) :
				BaseException(exception_source),
				d_derived_class_name(derived_class_type.name()),
				d_base_class_name(base_class_type.name())
			{  }

			~AmbiguousCast() throw() { }

		protected:

			virtual
			const char *
			exception_name() const
			{
				return "AmbiguousCast";
			}

			virtual
			void
			write_message(
					std::ostream &os) const;

		private:

			std::string d_derived_class_name;
			std::string d_base_class_name;
		};


		/**
		 * Exception thrown when attempting to transcribe an enumeration value that is not registered.
		 */
		class UnregisteredEnumValue :
				public BaseException
		{
		public:

			UnregisteredEnumValue(
					const GPlatesUtils::CallStack::Trace &exception_source,
					const std::type_info &enum_type,
					int enum_value) :
				BaseException(exception_source),
				d_enum_type(enum_type.name()),
				d_enum_value(enum_value)
			{  }

			~UnregisteredEnumValue() throw() { }

		protected:

			virtual
			const char *
			exception_name() const
			{
				return "UnregisteredEnumValue";
			}

			virtual
			void
			write_message(
					std::ostream &os) const;

		private:

			std::string d_enum_type;
			int d_enum_value;
		};


		/**
		 * Exception thrown when the class type is not explicitly registered or export registered.
		 */
		class UnregisteredClassType :
				public BaseException
		{
		public:

			UnregisteredClassType(
					const GPlatesUtils::CallStack::Trace &exception_source,
					const std::type_info &class_type) :
				BaseException(exception_source),
				d_class_name(class_type.name())
			{  }

			UnregisteredClassType(
					const GPlatesUtils::CallStack::Trace &exception_source,
					const std::string &class_name) :
				BaseException(exception_source),
				d_class_name(class_name)
			{  }

			~UnregisteredClassType() throw() { }

		protected:

			virtual
			const char *
			exception_name() const
			{
				return "UnregisteredClassType";
			}

			virtual
			void
			write_message(
					std::ostream &os) const;

		private:

			std::string d_class_name;
		};


		/**
		 * Exception thrown when the same class name is used to export register different class types.
		 */
		class ExportRegisteredMultipleClassTypesWithSameClassName :
				public BaseException
		{
		public:

			ExportRegisteredMultipleClassTypesWithSameClassName(
					const GPlatesUtils::CallStack::Trace &exception_source,
					const std::string &class_name,
					const std::type_info &class_type1,
					const std::type_info &class_type2) :
				BaseException(exception_source),
				d_class_name(class_name),
				d_class_type1(class_type1.name()),
				d_class_type2(class_type2.name())
			{  }

			~ExportRegisteredMultipleClassTypesWithSameClassName() throw() { }

		protected:

			virtual
			const char *
			exception_name() const
			{
				return "ExportRegisteredMultipleClassTypesWithSameClassName";
			}

			virtual
			void
			write_message(
					std::ostream &os) const;

		private:

			std::string d_class_name;
			std::string d_class_type1;
			std::string d_class_type2;
		};


		/**
		 * Exception thrown when multiple class names are used to export register the same class type.
		 */
		class ExportRegisteredMultipleClassNamesWithSameClassType :
				public BaseException
		{
		public:

			ExportRegisteredMultipleClassNamesWithSameClassType(
					const GPlatesUtils::CallStack::Trace &exception_source,
					const std::type_info &class_type,
					const std::string &class_name1,
					const std::string &class_name2) :
				BaseException(exception_source),
				d_class_type(class_type.name()),
				d_class_name1(class_name1),
				d_class_name2(class_name2)
			{  }

			~ExportRegisteredMultipleClassNamesWithSameClassType() throw() { }

		protected:

			virtual
			const char *
			exception_name() const
			{
				return "ExportRegisteredMultipleClassNamesWithSameClassType";
			}

			virtual
			void
			write_message(
					std::ostream &os) const;

		private:

			std::string d_class_type;
			std::string d_class_name1;
			std::string d_class_name2;
		};


		/**
		 * Exception thrown when the type stored in a transcribed QVariant is not registered with Qt
		 * using 'qRegisterMetaType()' and 'qRegisterMetaTypeStreamOperators()'.
		 *
		 * Registration is required for any types that are used in transcribed QVariant objects,
		 * except for Qt builtin types (see 'QMetaType::Type'). This enables them to be
		 * serialised/deserialised using QDataStream.
		 */
		class UnregisteredQVariantMetaType :
				public BaseException
		{
		public:

			UnregisteredQVariantMetaType(
					const GPlatesUtils::CallStack::Trace &exception_source,
					const QVariant &qvariant_object) :
				BaseException(exception_source),
				d_type_name(qvariant_object.typeName())
			{  }

			~UnregisteredQVariantMetaType() throw() { }

		protected:

			virtual
			const char *
			exception_name() const
			{
				return "UnregisteredQVariantMetaType";
			}

			virtual
			void
			write_message(
					std::ostream &os) const;

		private:

			std::string d_type_name;
		};
	}
}

#endif // GPLATES_SCRIBE_SCRIBEEXCEPTIONS_H
