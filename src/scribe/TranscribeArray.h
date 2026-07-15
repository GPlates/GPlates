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

#ifndef GPLATES_SCRIBE_TRANSCRIBEARRAY_H
#define GPLATES_SCRIBE_TRANSCRIBEARRAY_H

#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/type_traits/is_array.hpp>
#include <boost/type_traits/is_arithmetic.hpp>
#include <boost/utility/enable_if.hpp>

#include "ScribeConstructObject.h"
#include "ScribeExceptions.h"
#include "Transcribe.h"

#include "global/GPlatesAssert.h"


namespace GPlatesScribe
{
	//
	// Overloads of the transcribe function for static (multi-dimensional) arrays.
	//
	// An example array type for 2D integers is:
	// 
	//   typedef int array_2d_type[2][3];
	// 
	// ...which is the same as...
	// 
	//   typedef int array_1d_type[3];
	//   typedef array_1d_type array_2d_type[2];
	//

	class Scribe;

	/**
	 * Transcribe an array "T [N]" where 'T' could be another (multi-dimensional) array.
	 *
	 * For example:
	 * 
	 *   typedef int array_1d_type[3];
	 *   typedef array_1d_type array_2d_type[2];
	 *   array_2d_type array_2d = { {1,2,3}, {4,5,6} };
	 *   
	 *   scribe.transcribe(TRANSCRIBE_SOURCE, array_2d, "array_2d");
	 * 
	 * ...will invoke:
	 * 
	 *    transcribe(Scribe, T (&)[N], bool)
	 * 
	 * ...where T is 'int [3]' and N is '2'.
	 * This will in turn invoke (twice):
	 * 
	 *    transcribe(Scribe, T (&)[N], bool)
	 * 
	 * ...where T is 'int' and N is '3'.
	 *
	 *
	 * Note: Native arrays use the sequence protocol so they are transcription compatible with
	 * sequence containers such as std::vector.
	 */
	template <typename T, int N>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			T (&array)[N],
			bool transcribed_construct_data);


	//
	// We don't support using Scribe::load() and Scribe::save() (ie, ConstructObject) on
	// *multidimensional* arrays because they do not support (non-default) constructors
	// (can only be initialised explicitly using braces).
	//
	// However arrays containing non-default constructable items are supported (ie, the element type
	// in the multidimensional array can have non-default constructors):
	//
	//    NonDefaultConstructableType array[1][2] = { ... };
	//    ...
	//    scribe.transcribe(TRANSCRIBE_SOURCE, array, "array");
	//

	template <typename T, int N>
	TranscribeResult
	transcribe_construct_data(
			Scribe &scribe,
			ConstructObject<T [N]> &array)
	{
		// Shouldn't construct object - always transcribe existing object.
		GPlatesGlobal::Assert<Exceptions::ConstructNotAllowed>(
				false,
				GPLATES_ASSERTION_SOURCE,
				typeid(T [N]));

		// Shouldn't be able to get here - keep compiler happy.
		return TRANSCRIBE_INCOMPATIBLE;
	}
}


//
// Template implementation
//


// Placing here avoids cyclic header dependency.
#include "Scribe.h"


namespace GPlatesScribe
{
	namespace Implementation
	{
		template <typename T, int N>
		TranscribeResult
		transcribe_array_size(
				Scribe &scribe,
				T (&array)[N])
		{
			int transcribed_array_size;

			if (scribe.is_saving())
			{
				transcribed_array_size = N;
			}

			if (!scribe.transcribe(TRANSCRIBE_SOURCE, transcribed_array_size, ObjectTag().sequence_size()))
			{
				return scribe.get_transcribe_result();
			}

			// Make sure the array size has not changed.
			if (scribe.is_loading())
			{
				// The size of the array differs from its size when it was saved to an archive.
				// This indicates that the array size was changed in source code between then and now.
				// To make things backward compatible you'll need to store the objects in a new container
				// class type and explicitly iterate through the array in the client code and
				// transcribe directly - that's the only way to make adjustments to the number of
				// objects read into the array based on the class version of the new container class.
				if (transcribed_array_size != N)
				{
					return TRANSCRIBE_INCOMPATIBLE;
				}
			}

			return TRANSCRIBE_SUCCESS;
		}


		/**
		 * Implementation path when 'T' is an array.
		 *
		 * This path avoids using ConstructObject since it's not supported for arrays.
		 */
		template <typename T, int N>
		typename boost::enable_if< boost::is_array<T>, TranscribeResult >::type
		transcribe_impl(
				Scribe &scribe,
				T (&array)[N])
		{
			TranscribeResult transcribe_result = transcribe_array_size(scribe, array);
			if (transcribe_result != TRANSCRIBE_SUCCESS)
			{
				return transcribe_result;
			}

			// Transcribe each object in the array which is, in turn, another array.
			for (unsigned int n = 0; n < N; ++n)
			{
				if (!scribe.transcribe(TRANSCRIBE_SOURCE, array[n], ObjectTag()[n], TRACK))
				{
					return scribe.get_transcribe_result();
				}
			}

			return TRANSCRIBE_SUCCESS;
		}


		/**
		 * Tag types used to select the leaf transcription path for the terminal (non-array)
		 * element type of a (possibly multidimensional) array - mirrors the
		 * 'StreamPrimitiveTag'/'StreamTranscribeTag' dispatch used by 'Scribe::stream'.
		 */
		class LeafArithmeticTag { };
		class LeafObjectTag { };


		/**
		 * General (non-raw-lane) per-element leaf transcription.
		 *
		 * This path uses ConstructObject since it is supported for non-array 'T' objects and we
		 * don't know if 'T' is default-constructable or not. Shared by both the arithmetic leaf
		 * path (outside the raw lane) and the non-arithmetic leaf path (always).
		 */
		template <typename T, int N>
		TranscribeResult
		transcribe_impl_leaf_general(
				Scribe &scribe,
				T (&array)[N])
		{
			if (scribe.is_saving())
			{
				for (unsigned int n = 0; n < N; ++n)
				{
					// Mirror the load path.
					scribe.save(TRANSCRIBE_SOURCE, array[n], ObjectTag()[n], TRACK);
				}
			}
			else // loading...
			{
				for (unsigned int n = 0; n < N; ++n)
				{
					LoadRef<T> array_item = scribe.load<T>(TRANSCRIBE_SOURCE, ObjectTag()[n], TRACK);
					if (!array_item.is_valid())
					{
						return scribe.get_transcribe_result();
					}

					// Copy array item into array.
					array[n] = array_item;

					// The transcribed item now has a new address.
					scribe.relocated(TRANSCRIBE_SOURCE, array[n], array_item);
				}
			}

			return TRANSCRIBE_SUCCESS;
		}


		/**
		 * Leaf transcription when the element type 'T' is arithmetic.
		 *
		 * In the raw lane, streams the whole array as one bulk block (see
		 * 'Scribe::transcribe_raw_array') instead of N per-element scalar raw calls - this is the
		 * dominant per-element cost for large fixed-size arithmetic arrays. Outside the raw lane,
		 * falls back to the general per-element path (encoding unchanged).
		 */
		template <typename T, int N>
		TranscribeResult
		transcribe_impl_leaf(
				Scribe &scribe,
				T (&array)[N],
				LeafArithmeticTag)
		{
			if (scribe.is_transcribing_raw())
			{
				if (!scribe.transcribe_raw_array(TRANSCRIBE_SOURCE, array, N))
				{
					return scribe.get_transcribe_result();
				}

				return TRANSCRIBE_SUCCESS;
			}

			return transcribe_impl_leaf_general(scribe, array);
		}


		/**
		 * Leaf transcription when the element type 'T' is *not* arithmetic.
		 *
		 * 'Scribe::transcribe_raw_array' only supports arithmetic element types, so there is no
		 * raw-lane fast path here; if 'T' itself has raw-lane behaviour, the recursive
		 * 'scribe.save()'/'scribe.load()' calls in the general path pick that up.
		 */
		template <typename T, int N>
		TranscribeResult
		transcribe_impl_leaf(
				Scribe &scribe,
				T (&array)[N],
				LeafObjectTag)
		{
			return transcribe_impl_leaf_general(scribe, array);
		}


		/**
		 * Implementation path when 'T' is *not* an array.
		 *
		 * This path is the final (recursion) terminating path for multidimensional arrays.
		 */
		template <typename T, int N>
		typename boost::disable_if< boost::is_array<T>, TranscribeResult >::type
		transcribe_impl(
				Scribe &scribe,
				T (&array)[N])
		{
			TranscribeResult transcribe_result = transcribe_array_size(scribe, array);
			if (transcribe_result != TRANSCRIBE_SUCCESS)
			{
				return transcribe_result;
			}

			return transcribe_impl_leaf(
					scribe,
					array,
					typename boost::mpl::eval_if<
							boost::is_arithmetic<T>,
							boost::mpl::identity<LeafArithmeticTag>,
							boost::mpl::identity<LeafObjectTag> >::type());
		}
	}


	template <typename T, int N>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			T (&array)[N],
			bool transcribed_construct_data)
	{
		// Select one of two implementation paths.
		// One path is when 'T' is an array and the other when 'T' is not.
		return Implementation::transcribe_impl(scribe, array);
	}
}

#endif // GPLATES_SCRIBE_TRANSCRIBEARRAY_H
