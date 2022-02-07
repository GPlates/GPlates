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

#ifndef GPLATES_SCRIBE_TRANSCRIBEIMPL_H
#define GPLATES_SCRIBE_TRANSCRIBEIMPL_H

#include <boost/mpl/bool.hpp>
#include <boost/mpl/not.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_enum.hpp>
#include <boost/type_traits/is_pointer.hpp>

#include "Scribe.h"
#include "ScribeAccess.h"
#include "ScribeConstructObject.h"
#include "Transcribe.h"


namespace GPlatesScribe
{
	template <typename ObjectType>
	TranscribeResult
	transcribe(
			Scribe &scribe,
			ObjectType &object,
			bool transcribed_construct_data)
	{
		// Compile-time assertion to ensure pointers do not pass through here.
		//
		// Pointers, like everything else, should be transcribed directly to the 'Scribe' object.
		//
		BOOST_STATIC_ASSERT(boost::mpl::not_< boost::is_pointer<ObjectType> >::value);


		// Compile-time assertion to ensure enumerations do not pass through here.
		//
		// Each enumeration type needs to specialise or overload 'transcribe()'.
		// If this is not done then this compile-time assertion will get triggered.
		//
		//
		// A 'transcribe()' overload for an enumeration can be implemented as:
		//
		//
		//	#include "scribe/TranscribeEnumProtocol.h"
		//
		//	enum Enum
		//	{
		//		ENUM_VALUE_1,
		//		ENUM_VALUE_2,
		//		ENUM_VALUE_3
		//		
		//		// NOTE: Any new values should also be added to @a transcribe.
		//	};
		//  
		//	GPlatesScribe::TranscribeResult
		//	transcribe(
		//			GPlatesScribe::Scribe &scribe,
		//			Enum &e,
		//			bool transcribed_construct_data)
		//	{
		//		// WARNING: Changing the string ids will break backward/forward compatibility.
		//		//          So don't change the string ids even if the enum name changes.
		//		static const GPlatesScribe::EnumValue enum_values[] =
		//		{
		//			GPlatesScribe::EnumValue("ENUM_VALUE_1", ENUM_VALUE_1),
		//			GPlatesScribe::EnumValue("ENUM_VALUE_2", ENUM_VALUE_2),
		//			GPlatesScribe::EnumValue("ENUM_VALUE_3", ENUM_VALUE_3)
		//		};
		//	
		//		return GPlatesScribe::transcribe_enum_protocol(
		//				TRANSCRIBE_SOURCE,
		//				scribe,
		//				e,
		//				enum_values,
		//				enum_values + sizeof(enum_values) / sizeof(enum_values[0]));
		//	}
		//
		//
		// ...and if the enumeration is inside a class and is *private* then you'll need to
		// implement it as a friend function declared and defined in the class body as in...
		//
		//
		//	class MyClass
		//	{
		//	private: // private enum...
		//
		//		enum Enum
		//		{
		//			ENUM_VALUE_1,
		//			ENUM_VALUE_2,
		//			ENUM_VALUE_3
		//		
		//			// NOTE: Any new values should also be added to @a transcribe.
		//		};
		//
		//
		//		//
		//		// Transcribe for sessions/projects.
		//		//
		//		// Use friend function (injection) so can access private enum.
		//		// And implement in class body otherwise some compilers will complain
		//		// that the enum argument is not accessible (since enum is private).
		//		//
		//		friend
		//		GPlatesScribe::TranscribeResult
		//		transcribe(
		//				GPlatesScribe::Scribe &scribe,
		//				Enum &e,
		//				bool transcribed_construct_data)
		//		{
		//			// WARNING: Changing the string ids will break backward/forward compatibility.
		//			//          So don't change the string ids even if the enum name changes.
		//			static const GPlatesScribe::EnumValue enum_values[] =
		//			{
		//				GPlatesScribe::EnumValue("ENUM_VALUE_1", ENUM_VALUE_1),
		//				GPlatesScribe::EnumValue("ENUM_VALUE_2", ENUM_VALUE_2),
		//				GPlatesScribe::EnumValue("ENUM_VALUE_3", ENUM_VALUE_3)
		//			};
		//		
		//			return GPlatesScribe::transcribe_enum_protocol(
		//					TRANSCRIBE_SOURCE,
		//					scribe,
		//					e,
		//					enum_values,
		//					enum_values + sizeof(enum_values) / sizeof(enum_values[0]));
		//		}
		//	
		//	public:
		//		...
		//	};
		//
		//
		BOOST_STATIC_ASSERT(boost::mpl::not_< boost::is_enum<ObjectType> >::value);


		// If you get a compile-time error here then you need to either:
		//   (1) Provide a specialisation, or overload, of this non-member function
		//       to match your 'ObjectType' class, or
		//   (2) Provide a *private* 'transcribe()' method in your 'ObjectType' class and
		//       make this 'transcribe()' function a friend of your 'ObjectType' class using:
		//          friend class GPlatesScribe::Access;
		//
		return Access::transcribe(scribe, object, transcribed_construct_data);
	}


	/**
	 * Delegate to the static class method 'transcribe_construct_data()' declared in class 'ObjectType'.
	 */
	template <typename ObjectType>
	TranscribeResult
	transcribe_construct_data_impl(
			Scribe &scribe,
			ConstructObject<ObjectType> &object,
			boost::mpl::true_)
	{
		return Access::transcribe_construct_data(scribe, object);
	}

	/**
	 * The default implementation when 'ObjectType' does *not* have a
	 * static class method 'transcribe_construct_data()'.
	 */
	template <typename ObjectType>
	TranscribeResult
	transcribe_construct_data_impl(
			Scribe &scribe,
			ConstructObject<ObjectType> &object,
			boost::mpl::false_)
	{
		if (scribe.is_saving())
		{
			// Saves no data because default constructor (in load path) has no constructor arguments.
		}
		else // loading...
		{
			// Construct object using default constructor.
			object.construct_object();
		}

		return TRANSCRIBE_SUCCESS;
	}


	template <typename ObjectType>
	TranscribeResult
	transcribe_construct_data(
			Scribe &scribe,
			ConstructObject<ObjectType> &object)
	{
		//
		// If class 'ObjectType' has the following static class method:
		// 
		//		static
		//		void
		//		ObjectType::transcribe_construct_data(
		//				Scribe &,
		//				ConstructObject<ObjectType> &);
		//
		// ...then delegate to it.
		//
		// Otherwise use a default implementation that simply calls default constructor in load path
		// (and does nothing in save path).
		//
		// The default implementation is useful when 'ObjectType' has a *default* constructor.
		// Otherwise 'transcribe_construct_data()' needs to be specialised or overloaded, or
		// implemented as the above-mentioned static class method in class 'ObjectType'.
		//
		return transcribe_construct_data_impl(
				scribe,
				object,
				// Only class Access can form the expression '&ObjectType::transcribe_construct_data'
				// because that static method is private to ObjectType and only class Access can
				// privately access class ObjectType.
				typename Access::HasStaticMemberTranscribeConstructData<ObjectType>::type());
	}


	/**
	 * Delegate to the static class method 'relocated()' declared in class 'ObjectType'.
	 */
	template <typename ObjectType>
	void
	relocated_impl(
			Scribe &scribe,
			const ObjectType &relocated_object,
			const ObjectType &transcribed_object,
			boost::mpl::true_)
	{
		Access::relocated(scribe, relocated_object, transcribed_object);
	}

	/**
	 * The default implementation when 'ObjectType' does *not* have a
	 * static class method 'relocated()'.
	 */
	template <typename ObjectType>
	void
	relocated_impl(
			Scribe &scribe,
			const ObjectType &relocated_object,
			const ObjectType &transcribed_object,
			boost::mpl::false_)
	{
		// Default does nothing.
	}


	template <typename ObjectType>
	void
	relocated(
			Scribe &scribe,
			const ObjectType &relocated_object,
			const ObjectType &transcribed_object)
	{
		//
		// If class 'ObjectType' has the following static class method:
		// 
		//		static
		//		void
		//		ObjectType::relocated(
		//				Scribe &,
		//				const ObjectType &,
		//				const ObjectType &);
		//
		// ...then delegate to it.
		//
		// Otherwise use a default implementation that does nothing.
		//
		// The default implementation is sufficient for most object types since the scribe system
		// automatically relocates its sub-objects (data members and base classes) for it.
		// Otherwise 'relocated()' needs to be specialised or overloaded, or
		// implemented as the above-mentioned static class method in class 'ObjectType'.
		//
		relocated_impl(
				scribe,
				relocated_object,
				transcribed_object,
				// Only class Access can form the expression '&ObjectType::relocated'
				// because that static method is private to ObjectType and only class Access can
				// privately access class ObjectType.
				typename Access::HasStaticMemberRelocated<ObjectType>::type());
	}
}

#endif // GPLATES_SCRIBE_TRANSCRIBEIMPL_H
