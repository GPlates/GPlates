/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2015 The University of Sydney, Australia
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

#include "WeakReference.h"

#include "FeatureCollectionHandle.h"

#include "global/PointerTraits.h"

#include "scribe/Scribe.h"

#include "utils/non_null_intrusive_ptr.h"


namespace GPlatesModel
{
	template <typename H>
	GPlatesScribe::TranscribeResult
	transcribe_weak_ref_impl(
			GPlatesScribe::Scribe &scribe,
			WeakReference<H> &weak_ref)
	{
		typedef typename GPlatesGlobal::PointerTraits<H>::non_null_ptr_type
				feature_collection_handle_non_null_ptr_type;

		bool is_valid;
		
		if (scribe.is_saving())
		{
			is_valid = weak_ref.is_valid();
		}

		if (!scribe.transcribe(TRANSCRIBE_SOURCE, is_valid, "is_valid", GPlatesScribe::DONT_TRACK))
		{
			return scribe.get_transcribe_result();
		}

		if (is_valid)
		{
			if (scribe.is_saving())
			{
				// Create another non-null pointer referencing the existing feature collection handle.
				feature_collection_handle_non_null_ptr_type
						feature_collection_handle_non_null_ptr(weak_ref.handle_ptr());

				// Mirror the load path...
				scribe.save(
						TRANSCRIBE_SOURCE,
						feature_collection_handle_non_null_ptr,
						"non_null_ptr",
						GPlatesScribe::DONT_TRACK);
			}
			else // loading...
			{
				// NOTE: When the feature collection handle is first created, possibly during this
				// transcribe call if it hasn't already been created, it will add itself to the model.
				GPlatesScribe::LoadRef<feature_collection_handle_non_null_ptr_type>
						feature_collection_handle_non_null_ptr =
								scribe.load<feature_collection_handle_non_null_ptr_type>(
										TRANSCRIBE_SOURCE, "non_null_ptr", GPlatesScribe::DONT_TRACK);
				if (!feature_collection_handle_non_null_ptr.is_valid())
				{
					return scribe.get_transcribe_result();
				}

				H &feature_collection_handle = *feature_collection_handle_non_null_ptr.get();

				// The weak reference is not enough to keep the feature collection handle alive,
				// but when it was transcribed it was added to the model which keeps it alive.
				weak_ref = WeakReference<H>(feature_collection_handle);
			}
		}
		else
		{
			if (scribe.is_loading())
			{
				weak_ref = WeakReference<H>();
			}
		}

		// Transcribe the callback (stored inside the weak reference).
		typename WeakReferenceCallback<H>::maybe_null_ptr_type callback;
		if (scribe.is_saving())
		{
			callback = weak_ref.callback();
		}

		if (!scribe.transcribe(TRANSCRIBE_SOURCE, callback, "callback", GPlatesScribe::DONT_TRACK))
		{
			return scribe.get_transcribe_result();
		}

		if (scribe.is_loading())
		{
			weak_ref.attach_callback(callback);
		}

		return GPlatesScribe::TRANSCRIBE_SUCCESS;
	}
}


GPlatesScribe::TranscribeResult
GPlatesModel::transcribe(
		GPlatesScribe::Scribe &scribe,
		WeakReference<FeatureCollectionHandle> &weak_ref,
		bool transcribed_construct_data)
{
	return transcribe_weak_ref_impl(scribe, weak_ref);
}


GPlatesScribe::TranscribeResult
GPlatesModel::transcribe(
		GPlatesScribe::Scribe &scribe,
		WeakReference<const FeatureCollectionHandle> &weak_ref,
		bool transcribed_construct_data)
{
	return transcribe_weak_ref_impl(scribe, weak_ref);
}
