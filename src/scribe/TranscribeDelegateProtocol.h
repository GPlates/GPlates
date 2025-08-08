/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2016 The University of Sydney, Australia
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

#ifndef GPLATES_SCRIBE_TRANSCRIBEDELEGATEPROTOCOL_H
#define GPLATES_SCRIBE_TRANSCRIBEDELEGATEPROTOCOL_H

#include "Scribe.h"
#include "ScribeBool.h"
#include "ScribeInternalAccess.h"
#include "TranscribeResult.h"

#include "utils/CallStackTracker.h"


namespace GPlatesScribe
{
	/**
	 * This is useful for making classes transcription compatible with each other so that they can be
	 * switched without breaking backward/forward compatibility.
	 *
	 * For example, to make a QString wrapper compatible with a QString (so either can be loaded
	 * from a transcription)...
	 *
	 *   struct QStringWrapper
	 *   {
	 *     QString qstring;
	 *   };
	 *
	 *   GPlatesScribe::TranscribeResult
	 *   transcribe(
	 *			GPlatesScribe::Scribe &scribe,
	 *			QStringWrapper &wrapper,
	 *			bool transcribed_construct_data)
	 *   {
	 *		if (!transcribe_delegate_protocol(TRANSCRIBE_SOURCE, scribe, wrapper.qstring))
	 *		{
	 *			return scribe.get_transcribe_result();
	 *		}
	 *
	 *		return GPlatesScribe::TRANSCRIBE_SUCCESS;
	 *   }
	 *	 
	 * Note that there are no options in 'transcribe_delegate_protocol()' and the delegated object
	 * is *not* tracked.
	 *	 
	 *	 
	 * The above means you can save a QString and load it as a QStringWrapper (or vice versa)...
	 *	 
	 *	 QString string;
	 *	 scribe.save(TRANSCRIBE_SOURCE, string, "string");
	 *	 
	 *	 QStringWrapper string_wrapper;
	 *	 if (!scribe.transcribe(TRANSCRIBE_SOURCE, string_wrapper, "string"))
	 *	 {
	 *	   ...
	 *	 }
	 *	 
	 *	 
	 * However, if instead the approach below is used then a QStringWrapper would not be transcription
	 * compatible with a QString because of the extra "qstring" tag used by QStringWrapper...
	 *
	 *   GPlatesScribe::TranscribeResult
	 *   transcribe(
	 *			GPlatesScribe::Scribe &scribe,
	 *			QStringWrapper &wrapper,
	 *			bool transcribed_construct_data)
	 *   {
	 *		if (!scribe.transcribe(TRANSCRIBE_SOURCE, wrapper.qstring, "qstring"))
	 *		{
	 *			return scribe.get_transcribe_result();
	 *		}
	 *
	 *		return GPlatesScribe::TRANSCRIBE_SUCCESS;
	 *   }
	 */
	template <typename ObjectType>
	Bool
	transcribe_delegate_protocol(
			const GPlatesUtils::CallStack::Trace &transcribe_source, // Use 'TRANSCRIBE_SOURCE' here
			Scribe &scribe,
			ObjectType &object)
	{
		// Track the file/line of the call site for exception messages.
		GPlatesUtils::CallStackTracker call_stack_tracker(transcribe_source);

		// Wrap in a Bool object to force caller to check return code.
		return ScribeInternalAccess::create_bool(
				transcribe_source,
				ScribeInternalAccess::transcribe_delegate(scribe, object),
				scribe.is_loading()/*require_check*/);
	}


	/**
	 * Similar to @a transcribe_delegate_protocol but used on the *save* path
	 * (when need to use @a load_delegate_protocol on the *load* path).
	 *
	 *   save_delegate_protocol(TRANSCRIBE_SOURCE, scribe, object);
	 */
	template <typename ObjectType>
	void
	save_delegate_protocol(
			const GPlatesUtils::CallStack::Trace &transcribe_source, // Use 'TRANSCRIBE_SOURCE' here
			Scribe &scribe,
			const ObjectType &object)
	{
		// Track the file/line of the call site for exception messages.
		GPlatesUtils::CallStackTracker call_stack_tracker(transcribe_source);

		ScribeInternalAccess::save_delegate(scribe, object);
	}


	/**
	 * Similar to @a transcribe_delegate_protocol but used on the *load* path when ObjectType
	 * has no default constructor.
	 *
	 * Note: You'll need to specify 'GPlatesScribe::load_delegate_protocol<ObjectType>()' instead of
	 *       'load_delegate_protocol<ObjectType>()'.
	 *
	 *  GPlatesScribe::LoadRef<ObjectType> object =
	 *  		GPlatesScribe::load_delegate_protocol<ObjectType>(TRANSCRIBE_SOURCE, scribe);
	 *  if (!object.is_valid())
	 *  {
	 *  	return scribe.get_transcribe_result();
	 *  }
	 */
	template <typename ObjectType>
	LoadRef<ObjectType>
	load_delegate_protocol(
			const GPlatesUtils::CallStack::Trace &transcribe_source, // Use 'TRANSCRIBE_SOURCE' here
			Scribe &scribe)
	{
		// Track the file/line of the call site for exception messages.
		GPlatesUtils::CallStackTracker call_stack_tracker(transcribe_source);

		return ScribeInternalAccess::load_delegate<ObjectType>(transcribe_source, scribe);
	}
}

#endif // GPLATES_SCRIBE_TRANSCRIBEDELEGATEPROTOCOL_H
