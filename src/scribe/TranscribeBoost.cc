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

#include "TranscribeBoost.h"


GPlatesScribe::TranscribeResult
GPlatesScribe::transcribe(
		Scribe &scribe,
		boost::any &any_object,
		bool transcribed_construct_data)
{
	bool is_empty;

	if (scribe.is_saving())
	{
		is_empty = any_object.empty();
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, is_empty, "empty"))
	{
		return scribe.get_transcribe_result();
	}

	if (is_empty)
	{
		if (scribe.is_loading())
		{
			any_object = boost::any();
		}
	}
	else // not empty...
	{
		if (scribe.is_saving())
		{
			// Find the export registered class type for the stored object.
			boost::optional<const ExportClassType &> export_class_type =
					ExportRegistry::instance().get_class_type(any_object.type());

			// Throw exception if the stored object's type has not been export registered.
			//
			// If this assertion is triggered then it means:
			//   * The stored object's type was not export registered (see 'ScribeExportRegistration.h').
			//
			GPlatesGlobal::Assert<Exceptions::UnregisteredClassType>(
					export_class_type,
					GPLATES_ASSERTION_SOURCE,
					any_object.type());

			// Transcribe the stored type name.
			const std::string &stored_type_name = export_class_type->type_id_name;
			scribe.transcribe(TRANSCRIBE_SOURCE, stored_type_name, "stored_type");

			// Mirror the load path.
			export_class_type->transcribe_any_object->save_object(scribe, any_object);
		}
		else // loading...
		{
			// Transcribe the stored type name.
			std::string stored_type_name;
			if (!scribe.transcribe(TRANSCRIBE_SOURCE, stored_type_name, "stored_type"))
			{
				return scribe.get_transcribe_result();
			}

			// Find the export registered class type associated with the stored object.
			boost::optional<const ExportClassType &> export_class_type =
					ExportRegistry::instance().get_class_type(stored_type_name);

			// If the store type name has not been export registered then it means either:
			//   * the archive was created by a future GPlates with a stored type we don't know about, or
			//   * the archive was created by an old GPlates with a stored type we have since removed.
			//
			if (!export_class_type)
			{
				return TRANSCRIBE_UNKNOWN_TYPE;
			}

			// Load the boost::any object (now that we know its stored type).
			if (!export_class_type->transcribe_any_object->load_object(scribe, any_object))
			{
				return scribe.get_transcribe_result();
			}
		}
	}

	return TRANSCRIBE_SUCCESS;
}
