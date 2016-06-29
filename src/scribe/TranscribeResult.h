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

#ifndef GPLATES_SCRIBE_TRANSCRIBERESULT_H
#define GPLATES_SCRIBE_TRANSCRIBERESULT_H


namespace GPlatesScribe
{
	/**
	 * The result of transcribing an object.
	 */
	enum TranscribeResult
	{
		// The type of the transcribed object was compatible (transcription-protocol-wise) with
		// the transcription (loaded from archive) and hence was successfully transcribed.
		TRANSCRIBE_SUCCESS,


		// The object was not transcribed because it was incompatible with the loaded transcription.
		// This can happen when:
		//   1) The tag name/version of the transcribed object (or one of its nested objects, etc) was
		//      not found in the transcription, or
		//   2) A (potentially nested) transcribed primitive (integer/float/string) was the wrong
		//      type. For example, if transcription had a string and but we attempted to transcribe
		//      an integer or a non-primitive object. Or if transcription had a non-primitive object
		//      but we attempted to transcribe a primitive.
		//
		// This usually happens when there are backwards/forwards compatibility differences between
		// the transcription (loaded from archive) and the version of GPlates reading the transcription.
		// Note that this situation can be recovered from by providing a default value for the object,
		// otherwise the error should be propagated up to the caller.
		//
		// Note: In most cases (see @a TRANSCRIBE_UNKNOWN_TYPE for exceptions) the types of non-primitive
		// objects are not recorded in the transcription and so detection of incompatible types is based
		// purely on whether tags of the object's transcribed data members match the transcription.
		TRANSCRIBE_INCOMPATIBLE,


		// The object was not transcribed because an unknown type was encountered.
		// This can happen when:
		//   1) The transcription (loaded from archive) contains a (raw or smart) pointer to an
		//      unknown polymorphic type (one that is not registered with this version of GPlates), or
		//   2) An enumeration value is encountered that is unknown (not registered for the associated
		//      enumeration type by this version of GPlates), or
		//   3) A variant (eg, boost::variant) is encountered that contains an object whose type is
		//      unknown (not registered with this version of GPlates).
		//
		// However note that unknown types are only detected via polymorphic pointers, enums and
		// variants. So it's still possible that a future version of GPlates introduces a new derived
		// type, but if we never transcribe it via a pointer (or variant) then an attempt to
		// transcribe it (directly as an object) will result in a @a TRANSCRIBE_INCOMPATIBLE return code
		// (instead of @a TRANSCRIBE_UNKNOWN_TYPE).
		//
		// This error type is differentiated from @a TRANSCRIBE_INCOMPATIBLE in order to better support
		// forward compatibility. So if a future version of GPlates adds a new derived class then
		// we can ignore the new type by looking for @a TRANSCRIBE_UNKNOWN_TYPE.
		// For example, a transcribed sequence of smart pointers (via polymorphic base class),
		// such as 'std::vector< boost::shared_ptr<Base> >', can ignore elements containing the
		// new derived type and keep elements containing known types (rather than failing altogether).
		TRANSCRIBE_UNKNOWN_TYPE
	};
}

#endif // GPLATES_SCRIBE_TRANSCRIBERESULT_H
