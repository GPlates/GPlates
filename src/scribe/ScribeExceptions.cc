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

#include <ostream>

#include "ScribeExceptions.h"

#include "Scribe.h"


void
GPlatesScribe::Exceptions::UnsupportedVersion::write_message(
		std::ostream &os) const
{
	os << "Scribe archive stream was written using an unsupported future version "
		"of the scribe library and/or archive.";
}


void
GPlatesScribe::Exceptions::InvalidArchiveSignature::write_message(
		std::ostream &os) const
{
	os << "Scribe archive stream has an invalid signature.";
}


void
GPlatesScribe::Exceptions::ArchiveStreamError::write_message(
		std::ostream &os) const
{
	os << "Error transcribing archive stream: " << d_message;
}


void
GPlatesScribe::Exceptions::ScribeLibraryError::write_message(
		std::ostream &os) const
{
	os << "Internal error in Scribe library: " << d_message;
}


void
GPlatesScribe::Exceptions::ScribeUserError::write_message(
		std::ostream &os) const
{
	os << "Incorrect Scribe usage: " << d_message;
}


void
GPlatesScribe::Exceptions::ScribeTranscribeResultNotChecked::write_message(
		std::ostream &os) const
{
	os << "Incorrect Scribe usage: the return result of a transcribe call (to class Scribe) was not checked.";
}


void
GPlatesScribe::Exceptions::ConstructNotAllowed::write_message(
		std::ostream &os) const
{
	os << "Incorrect Scribe usage: object type '" << d_object_type_name
		<< "' should not be save/load constructed.";
}


void
GPlatesScribe::Exceptions::InvalidTranscribeOptions::write_message(
		std::ostream &os) const
{
	os << "Incorrect Scribe usage: invalid transcribe options: " << d_message;
}


void
GPlatesScribe::Exceptions::UnexpectedXmlElementName::write_message(
		std::ostream &os) const
{
	os << "Incorrect Scribe usage: expected "
		<< (d_is_start_element ? "start" : "end") << " XML element named";

	for (int n = 0; n < d_element_names.size(); ++n)
	{
		os << " '" << d_element_names[n].toStdString() << "'";
	}
}


void
GPlatesScribe::Exceptions::InvalidXmlElementName::write_message(
		std::ostream &os) const
{
	os << "Incorrect Scribe usage: ";

	if (d_xml_element_name)
	{
		os << "invalid XML element name '" << d_xml_element_name.get() << "'.";
	}
	else
	{
		os << "invalid XML element name.";
	}
}


void
GPlatesScribe::Exceptions::XmlStreamParseError::write_message(
		std::ostream &os) const
{
	os << "Error parsing XML stream: " << d_xml_error_message.toStdString();
}


void
GPlatesScribe::Exceptions::TranscriptionIncomplete::write_message(
		std::ostream &os) const
{
	os << "Transcription is incomplete - there are transcribed objects that could not be found, "
		"or transcribed pointers to untranscribed objects.";
}


void
GPlatesScribe::Exceptions::TranscriptionIncompatible::write_message(
		std::ostream &os) const
{
	os << "Transcription is incompatible - most likely due to breaking of "
		"backward/forward compatibility.";
}


void
GPlatesScribe::Exceptions::TranscribedReferenceInsteadOfObject::write_message(
		std::ostream &os) const
{
	os << "Incorrect Scribe usage: "
		<< "Attempted to transcribe an object as type '" << d_reference_type_name
		<< "' but its actual type is '" << d_object_type_name << "'.";
}


void
GPlatesScribe::Exceptions::AlreadyTranscribedObject::write_message(
		std::ostream &os) const
{
	os << "Incorrect Scribe usage: "
		<< "Attempted to transcribe an object of type '" << d_object_type_name
		<< "' that has already been transcribed at the same memory address.";
}


void
GPlatesScribe::Exceptions::AlreadyTranscribedObjectWithoutOwningPointer::write_message(
		std::ostream &os) const
{
	os << "Incorrect Scribe usage: "
		<< "Attempted to transcribe an object of type '" << d_object_type_name
		<< "' via an owning pointer, but it has already been transcribed without one.";
}


void
GPlatesScribe::Exceptions::TranscribedUntrackedPointerBeforeReferencedObject::write_message(
		std::ostream &os) const
{
	os << "Incorrect Scribe usage: "
		<< "Attempted to transcribe an *untracked* pointer before the pointed-to object of type '"
		<< d_object_type_name
		<< "' - either track the pointer or transcribe the pointed-to object first.";
}


void
GPlatesScribe::Exceptions::UntrackingObjectWithReferences::write_message(
		std::ostream &os) const
{
	os << "Incorrect Scribe usage: "
		<< "An *untracked* object of type '" << d_object_type_name
		<< "' has transcribed pointers or references referencing it - try either tracking the object "
		"or avoid transcribing pointers/references to it.";
}


void
GPlatesScribe::Exceptions::TranscribedReferenceBeforeReferencedObject::write_message(
		std::ostream &os) const
{
	os << "Incorrect Scribe usage: "
		<< "Attempted to transcribe a reference to an object, of type '" << d_object_type_name
		<< "', before the object itself has been transcribed or cannot find transcribed object "
		"(because it was untracked).";
}


void
GPlatesScribe::Exceptions::RelocatedReferenceInsteadOfObject::write_message(
		std::ostream &os) const
{
	os << "Incorrect Scribe usage: "
		<< "Attempted to relocate an object as type '" << d_reference_type_name
		<< "' but its actual type is '" << d_object_type_name << "'.";
}


void
GPlatesScribe::Exceptions::RelocatedUntrackedObject::write_message(
		std::ostream &os) const
{
	os << "Incorrect Scribe usage: "
		<< "Attempted to relocate an untracked object.";
}


void
GPlatesScribe::Exceptions::RelocatedObjectBoundToAReferenceOrUntrackedPointer::write_message(
		std::ostream &os) const
{
	os << "Incorrect Scribe usage: "
		<< "Attempted to relocate a transcribed object that already has a reference bound to it "
		<< "(cannot be re-bound to the relocated object) or an untracked pointer bound to it "
		<< "(cannot be updated to point to relocated object).";
}


void
GPlatesScribe::Exceptions::LoadedObjectTrackedButNotRelocated::write_message(
		std::ostream &os) const
{
	os << "Incorrect Scribe usage: "
		<< "A tracked object was loaded but was not relocated.";
}


void
GPlatesScribe::Exceptions::UnregisteredCast::write_message(
		std::ostream &os) const
{
	os << "Incorrect Scribe usage: "
		<< "Unable to cast between class types '" << d_derived_class_name << "' and '"
		<< d_base_class_name << "' due to missing derived/base transcribe registration, "
		"or attempt to cast between unrelated types.";
}


void
GPlatesScribe::Exceptions::AmbiguousCast::write_message(
		std::ostream &os) const
{
	os << "Incorrect Scribe usage: "
		<< "Ambiguous cast between class types '" << d_derived_class_name << "' and '"
		<< d_base_class_name << "' due to more than one path from derived class to base class.";
}


void
GPlatesScribe::Exceptions::UnregisteredEnumValue::write_message(
		std::ostream &os) const
{
	os << "Incorrect Scribe usage: "
		<< "Attempted to transcribe an enumeration value '" << d_enum_value
		<< "' of enumeration type '" << d_enum_type
		<< "' that was not explicitly registered.";
}


void
GPlatesScribe::Exceptions::UnregisteredClassType::write_message(
		std::ostream &os) const
{
	os << "Incorrect Scribe usage: "
		<< "Attempted to transcribe an object whose class or type '" << d_class_name
		<< "' was not export registered.";
}


void
GPlatesScribe::Exceptions::ExportRegisteredMultipleClassTypesWithSameClassName::write_message(
		std::ostream &os) const
{
	os << "Incorrect Scribe usage: "
		<< "Attempted to export register, for transcribing, two class types '" << d_class_type1
		<< "' and '" << d_class_type2 << "' using the same class name '" << d_class_name << "'.";
}


void
GPlatesScribe::Exceptions::ExportRegisteredMultipleClassNamesWithSameClassType::write_message(
		std::ostream &os) const
{
	os << "Incorrect Scribe usage: "
		<< "Attempted to export register, for transcribing, two class names '" << d_class_name1
		<< "' and '" << d_class_name2 << "' using the same class type '" << d_class_type << "'.";
}
