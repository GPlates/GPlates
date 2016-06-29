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

#ifndef GPLATES_SCRIBE_SCRIBE_H
#define GPLATES_SCRIBE_SCRIBE_H

#include <cstddef>
#include <functional>
#include <map>
#include <stack>
#include <typeinfo>
#include <utility>
#include <vector>
#include <boost/cast.hpp>
#include <boost/mpl/equal_to.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/greater.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/not.hpp>
#include <boost/mpl/or.hpp>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/pool/object_pool.hpp>
#include <boost/preprocessor/arithmetic/dec.hpp>
#include <boost/preprocessor/arithmetic/div.hpp>
#include <boost/preprocessor/arithmetic/inc.hpp>
#include <boost/preprocessor/arithmetic/mod.hpp>
#include <boost/preprocessor/arithmetic/mul.hpp>
#include <boost/preprocessor/comparison/greater.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/control/expr_iif.hpp>
#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/control/iif.hpp>
#include <boost/preprocessor/enum_shifted.hpp>
#include <boost/preprocessor/enum_shifted_params.hpp>
#include <boost/preprocessor/facilities/empty.hpp>
#include <boost/preprocessor/facilities/identity.hpp>
#include <boost/preprocessor/repetition/for.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/repetition/repeat_from_to.hpp>
#include <boost/preprocessor/seq/enum.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_array.hpp>
#include <boost/type_traits/is_abstract.hpp>
#include <boost/type_traits/is_const.hpp>
#include <boost/type_traits/is_class.hpp>
#include <boost/type_traits/is_enum.hpp>
#include <boost/type_traits/is_arithmetic.hpp>
#include <boost/type_traits/is_pointer.hpp>
#include <boost/type_traits/is_polymorphic.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/remove_const.hpp>
#include <boost/type_traits/remove_pointer.hpp>
#include <boost/utility/in_place_factory.hpp>

#include "ScribeAccess.h"
#include "ScribeExceptions.h"
#include "ScribeInternalUtils.h"
#include "ScribeLoadRef.h"
#include "ScribeObjectTag.h"
#include "ScribeOptions.h"
#include "ScribeSaveLoadConstructObject.h"
#include "ScribeVoidCastRegistry.h"
#include "Transcribe.h"
#include "TranscribeContext.h"
#include "TranscribeResult.h"
#include "Transcription.h"
#include "TranscriptionScribeContext.h"

#include "global/GPlatesAssert.h"

#include "utils/CallStackTracker.h"
#include "utils/ObjectPool.h"
#include "utils/ReferenceCount.h"
#include "utils/SafeBool.h"
#include "utils/SmartNodeLinkedList.h"


/**
 * The maximum dimension of transcribable multi-level pointers.
 *
 * For example, 'const int *const *' has dimension 2.
 *
 * NOTE: Setting this above 5 slows down compilation noticeably.
 * And at 7, on the MSVC2005 compiler, we get the following error:
 *   "fatal error C1009: compiler limit : macros nested too deeply"
 * Setting this to 4 on Ubuntu 14.04 causes the compiler to use up to 2Gb of memory and
 * bringing it down to 3 takes that down to 1Gb.
 *
 * Each increment of 'GPLATES_SCRIBE_MAX_POINTER_DIMENSION' doubles the number of combinations.
 * So the slowdown/memory-usage is exponential.
 */
#define GPLATES_SCRIBE_MAX_POINTER_DIMENSION 2

/**
 * The maximum dimension of transcribable native arrays.
 *
 * For example, 'const int [3][3]' has dimension 2.
 *
 * Actually 'rank' might be a better term than 'dimension'.
 *
 * This doesn't have as much impact on compilation time as 'GPLATES_SCRIBE_MAX_POINTER_DIMENSION'.
 */
#define GPLATES_SCRIBE_MAX_ARRAY_DIMENSION 3


/**
 * The file and line number of a transcribe call site.
 */
#ifndef TRANSCRIBE_SOURCE
#define TRANSCRIBE_SOURCE \
		GPlatesUtils::CallStack::Trace(__FILE__, __LINE__)
#endif


namespace GPlatesScribe
{
	//
	// Forward declarations
	//
	class ExportClassType;
	class Scribe;
	class ScribeInternalAccess;


	/**
	 * Class scribe is the main access point for transcribing object graphs (networks of interconnected objects).
	 *
	 * Transcribing essentially means serializing/deserializing.
	 *
	 * Please refer to "Transcribe.h" for more details on how to transcribe arbitrary classes or types.
	 *
	 * This serialization system is very similar to boost:serialization which we were very close
	 * to using, but unfortunately there were just enough issues to tip the balance in favour
	 * of using another serialization library or implementing our own (we chose to implement our own).
	 *
	 * See "DesignRationale.txt" for more details.
	 */
	class Scribe :
			private boost::noncopyable
	{
	public:

		/**
		 * Creates a @a Scribe to *save* to a transcription.
		 *
		 * The final transcription can be obtained using @a get_transcription.
		 */
		Scribe();

		/**
		 * Creates a @a Scribe to *load* from the specified transcription.
		 *
		 * @throws Exceptions::TranscriptionIncomplete if @a transcription is not complete.
		 */
		explicit
		Scribe(
				const Transcription::non_null_ptr_type &transcription);


		/**
		 * Is the scribe saving objects to an archive.
		 */
		bool
		is_saving() const
		{
			return d_is_saving;
		}


		/**
		 * Is the scribe loading objects from an archive.
		 */
		bool
		is_loading() const
		{
			return !is_saving();
		}


		/**
		 * Boolean result for transcribe methods.
		 *
		 * This class is used instead of a 'bool' to ensure the caller checks transcribe results.
		 * If a return result is not checked then Exceptions::ScribeTranscribeResultNotChecked is
		 * thrown to notify the programmer to insert the check.
		 *
		 * For example, to check the return result of Scribe::transcribe():
		 *
		 *	if (!scribe.transcribe(...))
		 *	{
		 *		return scribe.get_transcribe_result();
		 *	}
		 *
		 * NOTE: Only the *load* path needs to be checked.
		 * @a transcribe handles both the load and save paths but if you split it into separate
		 * save and load paths then only the load path needs to be checked.
		 * For example:
		 *
		 *	if (scribe.is_saving())
		 *	{
		 *		scribe.transcribe(...);
		 *	}
		 *	else // loading...
		 *	{
		 *		if (!scribe.transcribe(...))
		 *		{
		 *			return scribe.get_transcribe_result();
		 *		}
		 *	}
		 */
		class Bool :
				public GPlatesUtils::SafeBool<Bool>
		{
		public:

			/**
			 * Boolean test - don't use directly.
			 *
			 * Instead use (for example):
			 *
			 *		if (!scribe.transcribe(...))
			 *		{
			 *			return scribe.get_transcribe_result();
			 *		}
			 *
			 * ...where Scribe::transcribe() returns a 'Bool'.
			 */
			bool
			boolean_test() const;

		private:
			struct CheckDeleter; //!< Custom boost::shared_ptr deleter when result checking required.

			Bool(
					// The location of the caller site that should be checking this returned 'Bool'...
					const GPlatesUtils::CallStack::Trace &transcribe_source,
					// Actual boolean result...
					bool result,
					// Whether to throw exception if boolean result is not checked...
					bool require_check);

			boost::shared_ptr<bool> d_bool;

			friend class Scribe;
			friend class ScribeInternalAccess;
		};


		//
		// Transcribe methods.
		//


		/**
		 * Transcribe an object.
		 *
		 * This is typically used for regular object and primitive types.
		 *
		 * Returns false if transcribe fails while *loading* (from an archive).
		 * The reason for the failure can then be obtained from @a get_transcribe_result.
		 * NOTE: On *saving* (to an archive) true is always returned.
		 * Note that if this method returns false the caller can decide if it wants (or not) to provide
		 * a default value in place of the (failed) loaded object (and hence recover from the error).
		 *
		 * Primitive types include enumerations which are transcribed as integers by default
		 * (unless explicitly specialised or overloaded for a specific enumeration type  - see
		 * the 'transcribe' function in "Transcribe.h") and hence, like other primitive types,
		 * require no special handling on the part of clients.
		 *
		 * A pointer is itself an object - because other objects can reference or point to a pointer.
		 * So pointers also pass through this method and can be tracked like regular (non-pointer) objects.
		 * In other words the *pointer* itself is tracked (we're not referring to the object it points to).
		 * This enables subsequent pointer-to-pointer's to link up to pointers and so on.
		 *
		 * If @a object is a pointer then EXCLUSIVE_OWNER or SHARED_OWNER can optionally be specified
		 * in @a options to indicate whether the pointer *owns* the pointed-to object.
		 * In this case both the *pointer* object (@a object) and the pointed-to object are transcribed.
		 * Specifying SHARED_OWNER indicates multiple pointers share ownership of the pointed-to object.
		 * Note that the *pointed-to* object is always tracked.
		 *
		 * However a reference is not an object (according to the C++ standard) so you should
		 * use @a save_reference and @a load_reference instead when dealing with references.
		 * For example,
		 *   A &a = ...;
		 *   // This will compile, but is logically incorrect...
		 *   // scribe.transcribe(TRANSCRIBE_SOURCE, a, "a");
		 *   // This is correct...
		 *   scribe.save_reference(TRANSCRIBE_SOURCE, a, "a");
		 *
		 * Also native C++ arrays are supported (by "TranscribeArray.h").
		 * And the objects in the array can be regular objects (including pointers) or arrays (hence
		 * enabling arrays of any dimension).
		 *
		 * @a object_tag is an arbitrary name for the object that is used, along with an optional
		 * version (defaults to zero), to identify the transcribed object in the archive - it only needs
		 * to be unique in the scope of the parent transcribed object (ie, what is calling this transcribe function)
		 * because it's only used to search in that scope.
		 * The optional version (in @a object_tag) can be incremented when the transcribed object associated
		 * with @a object_tag has its type changed such that it is no longer forward compatible (such as
		 * changing an 'int' to a 'std::vector<int>'). Old applications will no longer be able
		 * to load the object and 'transcribe()' will return false - note that changing the
		 * @a object_tag string has the same effect because both the object tag and version are used
		 * to locate the object to load in the archive (within the scope of the parent object).
		 *
		 * @a options can contain zero, one or more options combined using the OR ('|') operator as in:
		 *
		 *     GPlatesScribe::TRACK | GPlatesScribe::SHARED_OWNER
		 *
		 * ...for a pointer with shared ownership and tracking (see "ScribeOptions.h").
		 *
		 * Tracking is disabled by default. Tracking means the address of the specified object will be
		 * recorded so that the scribe can detect subsequent references/pointers to the object.
		 * It can be enabled by specifying GPlatesScribe::TRACK in @a options. Leaving tracking disabled
		 * is useful when you want to transcribe a temporary object (such as an integer loop counter)
		 * and you know that no other transcribed object/pointer will reference or point to it.
		 * Enabling tracking has the following benefits:
		 *  (1) During saving: subsequently saved references to the object (if any) can find it, and
		 *  (2) During loading: references to the object can be re-built, and
		 *  (3) During loading: relocated objects can be tracked (in case they are referenced elsewhere).
		 *
		 * However if the object loaded from the archive is only a temporary copy then object tracking should either:
		 *  (1) be left off to prevent errors caused by transcribing the same object address more than once, or
		 *  (2) be turned on, but then use @a relocated to notify the final object destination.
		 *
		 * For example, the following erroneous situation...
		 *
		 *   if (scribe.is_saving())
		 *   {
		 *		scribe.transcribe(TRANSCRIBE_SOURCE, container.at(0), "item", GPlatesScribe::TRACK);
		 *		scribe.transcribe(TRANSCRIBE_SOURCE, container.at(1), "item", GPlatesScribe::TRACK);
		 *   }
		 *   else // scribe.is_loading()
		 *   {
		 *		A a; // Need to construct on C runtime stack if cannot transcribe directly into container.
		 *		
		 *		if (!scribe.transcribe(TRANSCRIBE_SOURCE, a, "item", GPlatesScribe::TRACK))
		 *		{
		 *			return scribe.get_transcribe_result();
		 *		}
		 *		container.append(a);
		 *		
		 *		// Error: have already transcribed to object address "&a"...
		 *		if (!scribe.transcribe(TRANSCRIBE_SOURCE, a, "item", GPlatesScribe::TRACK))
		 *		{
		 *			return scribe.get_transcribe_result();
		 *		}
		 *		container.append(a);
		 *   }
		 *
		 * ...can be corrected by either...
		 *
		 *   if (scribe.is_saving())
		 *   {
		 *		scribe.transcribe(TRANSCRIBE_SOURCE, container.at(0), "item"); // Disable tracking
		 *		scribe.transcribe(TRANSCRIBE_SOURCE, container.at(1), "item"); // Disable tracking
		 *   }
		 *   else // scribe.is_loading()
		 *   {
		 *		A a; // Need to construct on C runtime stack if cannot transcribe directly into container.
		 *		
		 *		if (!scribe.transcribe(TRANSCRIBE_SOURCE, a, "item")) // Disable tracking
		 *		{
		 *			return scribe.get_transcribe_result();
		 *		}
		 *		container.append(a);
		 *		
		 *		if (!scribe.transcribe(TRANSCRIBE_SOURCE, a, "item")) // Disable tracking
		 *		{
		 *			return scribe.get_transcribe_result();
		 *		}
		 *		container.append(a);
		 *   }
		 *
		 * ...which will work fine if no other objects/pointers reference 'container's items
		 * (if they do then they won't be able to link up with it since cannot find untracked items).
		 * Otherwise the following (tracking based) solution is needed...
		 *
		 *   if (scribe.is_saving())
		 *   {
		 *		scribe.transcribe(TRANSCRIBE_SOURCE, container.at(0), "item", GPlatesScribe::TRACK);
		 *		scribe.transcribe(TRANSCRIBE_SOURCE, container.at(1), "item", GPlatesScribe::TRACK);
		 *   }
		 *   else // scribe.is_loading()
		 *   {
		 *		A a; // Need to construct on C runtime stack if cannot transcribe directly into container.
		 *		container.reserve(2); // Avoid re-allocations during 'append'.
		 *		
		 *		if (!scribe.transcribe(TRANSCRIBE_SOURCE, a, "item", GPlatesScribe::TRACK))
		 *		{
		 *			return scribe.get_transcribe_result();
		 *		}
		 *		container.append(a);
		 *		scribe.relocatedTRANSCRIBE_SOURCE, (container[0], a); // Now registers &container[0] as the object address.
		 *		
		 *		// Is fine since &a != &container[0] ...
		 *		if (!scribe.transcribe(TRANSCRIBE_SOURCE, a, "item", GPlatesScribe::TRACK))
		 *		{
		 *			return scribe.get_transcribe_result();
		 *		}
		 *		container.append(a);
		 *		scribe.relocated(TRANSCRIBE_SOURCE, container[1], a);
		 *   }
		 *
		 * Note: Internally all 'const's in 'ObjectType' are cast away (eg, 'const int **const*' -> 'int ***').
		 * Casting away 'const' is not generally a good idea but we either need to read from
		 * the archive *into* the object (which modifies object) or vice versa (which does not).
		 * Combining the two means that clients only need to write one code path that both reads
		 * and writes archives - reducing the likelihood of changes to the write path getting
		 * out-of-sync with the read path, and vice versa.
		 */
		template <typename ObjectType>
		Bool
		transcribe(
				const GPlatesUtils::CallStack::Trace &transcribe_source, // Use 'TRANSCRIBE_SOURCE' here
				ObjectType &object,
				const ObjectTag &object_tag,
				unsigned int options = 0);

		/**
		 * Transcribe the base object sub-part (with type 'BaseType') of the specified derived object
		 * (with type 'DerivedType').
		 *
		 * When a derived class object is transcribed it should, in turn, use this method to
		 * transcribe its base class object(s). You should *not* directly transcribe the base class
		 * by directly calling its 'transcribe()' method or function.
		 *
		 * Returns false if transcribe fails while *loading* (from an archive).
		 * The reason for the failure can then be obtained from @a get_transcribe_result.
		 * NOTE: On *saving* (to an archive) true is always returned.
		 * Note that if this method returns false the caller can decide if it wants (or not) to provide
		 * a default value in place of the (failed) loaded base object (and hence recover from the error).
		 *
		 * @a base_object_tag is an arbitrary name for the base class sub-object that is used,
		 * along with @a version, to identify the transcribed sub-object in the archive - it only needs
		 * to be unique in the scope of the transcribed derived object (ie, what is calling this
		 * @a transcribe_base method) because it's only used to search in that scope.
		 *
		 * NOTE: Calling this method also registers that class 'DerivedType' derives from class 'BaseType'.
		 * Doing this enables base class pointers and references to be correctly upcast from the derived
		 * class objects they are pointing to (in the presence of multiple inheritance).
		 * Failure to do this can result in the Exceptions::UnregisteredCast exception being thrown
		 * when attempting to transcribe via a base class pointer.
		 *
		 * An example,
		 *
		 *   class A { public: virtual ~A(); int a; virtual void do() = 0; }; // Abstract base class.
		 *   class B : public A { public: int b; virtual void do(); };
		 *
		 *   GPlatesScribe::TranscribeResult
		 *   transcribe(
		 *			GPlatesScribe::Scribe &scribe,
		 *			A &a,
		 *			bool transcribed_construct_data)
		 *   {
		 *		if (!scribe.transcribe(TRANSCRIBE_SOURCE, a.a, "a", GPlatesScribe::TRACK))
		 *		{
		 *			return scribe.get_transcribe_result();
		 *		}
		 *		
		 *		return GPlatesScribe::TRANSCRIBE_SUCCESS;
		 *   }
		 *
		 *   GPlatesScribe::TranscribeResult
		 *   transcribe(
		 *			GPlatesScribe::Scribe &scribe,
		 *			B &b,
		 *			bool transcribed_construct_data)
		 *   {
		 *		// Transcribe base class A.
		 *		if (!scribe.transcribe_base<A>(TRANSCRIBE_SOURCE, b, "A") ||
		 *			!scribe.transcribe(TRANSCRIBE_SOURCE, b.b, "b", GPlatesScribe::TRACK))
		 *		{
		 *			return scribe.get_transcribe_result();
		 *		}
		 *		
		 *		return GPlatesScribe::TRANSCRIBE_SUCCESS;
		 *   }
		 *
		 * ...where 'transcribe()' uses the non-intrusive approach.
		 * The alternative intrusive approach is demonstrated...
		 *
		 *   class A
		 *   {
		 *   public:
		 *		virtual ~A() { }
		 *		
		 *		int a;
		 *
		 *		virtual void do() = 0;
		 *		
		 *   protected:
		 *		//! Only the scribe system should call @a transcribe method.
		 *		friend class GPlatesScribe::Access;
		 *
		 *		//! Transcribe to/from serialization archives.
		 *		GPlatesScribe::TranscribeResult
		 *		transcribe(
		 *				GPlatesScribe::Scribe &scribe,
		 *				bool transcribed_construct_data)
		 *		{
		 *			if (!scribe.transcribe(TRANSCRIBE_SOURCE, a, "a", GPlatesScribe::TRACK))
		 *			{
		 *				return scribe.get_transcribe_result();
		 *			}
		 *		
		 *			return GPlatesScribe::TRANSCRIBE_SUCCESS;
		 *		}
		 *   };
		 *
		 *   class B :
		 *		public A
		 *   {
		 *   public:
		 *		int b;
		 *
		 *		virtual void do();
		 *		
		 *   private:
		 *		//! Only the scribe system should call @a transcribe method.
		 *		friend class GPlatesScribe::Access;
		 *
		 *		//! Transcribe to/from serialization archives.
		 *		GPlatesScribe::TranscribeResult
		 *		transcribe(
		 *				GPlatesScribe::Scribe &scribe,
		 *				bool transcribed_construct_data)
		 *		{
		 *			// Transcribe base class A.
		 *			if (!scribe.transcribe_base<A>(TRANSCRIBE_SOURCE, *this, "A") ||
		 *				!scribe.transcribe(TRANSCRIBE_SOURCE, b, "b", GPlatesScribe::TRACK))
		 *			{
		 *				return scribe.get_transcribe_result();
		 *			}
		 *
		 *			return GPlatesScribe::TRANSCRIBE_SUCCESS;
		 *		}
		 *   };
		 *
		 *
		 * NOTE: If 'BaseType' requires no transcribing (eg, because it has no data members) then to
		 * ensure the derived-to-base inheritance link is still registered you should call the other
		 * overload of @a transcribe_base that accepts no arguments.
		 */
		template <class BaseType, class DerivedType>
		Bool
		transcribe_base(
				const GPlatesUtils::CallStack::Trace &transcribe_source, // Use 'TRANSCRIBE_SOURCE' here
				DerivedType &derived_object,
				const ObjectTag &base_object_tag);


		/**
		 * This overload of @a transcribe_base only records the BaseType/DerivedType inheritance
		 * relationship - it does not transcribe the BaseType sub-object of a DerivedType object.
		 *
		 * This is useful when there is nothing in the base class to transcribe but when there are
		 * pointers (or references) to *polymorphic* base class that are transcribed.
		 * If @a transcribe_base were not called, from within the derived class 'transcribe()'
		 * method/function, then when a pointer to the *polymorphic* base class is loaded it would
		 * fail (since it would be unable to cast from derived to base).
		 * Failure to call this method can result in the Exceptions::UnregisteredCast
		 * exception being thrown when attempting to transcribe a base class pointer.
		 *
		 * NOTE: Calling this method registers that class 'DerivedType' derives from class 'BaseType'.
		 * Doing this enables base class pointers and references to be correctly upcast from the
		 * derived class objects they are pointing to (in the presence of multiple inheritance).
		 *
		 * An example,
		 *
		 *   class A { public: virtual ~A(); virtual void do() = 0; }; // Abstract base class.
		 *   class B : public A { public: int b; virtual void do(); };
		 *
		 *   GPlatesScribe::TranscribeResult
		 *   transcribe(
		 *			GPlatesScribe::Scribe &scribe,
		 *			B &b,
		 *			bool transcribed_construct_data)
		 *   {
		 *		// Transcribe base class A - it is not transcribable (has not transcribe method
		 *		// because it has no data members).
		 *		// So we transcribe it without passing the base class sub-object.
		 *		// Really just registers conversion casts between B <--> A.
		 *		if (!scribe.transcribe_base<A, B>(TRANSCRIBE_SOURCE) ||
		 *			!scribe.transcribe(TRANSCRIBE_SOURCE, b.b, "b", GPlatesScribe::TRACK))
		 *		{
		 *			return scribe.get_transcribe_result();
		 *		}
		 *		
		 *		return GPlatesScribe::TRANSCRIBE_SUCCESS;
		 *   }
		 *
		 * ...where 'transcribe()' uses the non-intrusive approach.
		 * The alternative intrusive approach is demonstrated...
		 *
		 *   class B :
		 *		public A
		 *   {
		 *   public:
		 *		int b;
		 *
		 *		virtual void do();
		 *		
		 *   private:
		 *		//! Only the scribe system should call @a transcribe method.
		 *		friend class GPlatesScribe::Access;
		 *
		 *		//! Transcribe to/from serialization archives.
		 *		GPlatesScribe::TranscribeResult
		 *		transcribe(
		 *				GPlatesScribe::Scribe &scribe,
		 *				bool transcribed_construct_data)
		 *		{
		 *			// Transcribe base class A - it is not transcribable (has not transcribe method
		 *			// because it has no data members).
		 *			// So we transcribe it without passing the base class sub-object.
		 *			// Really just registers conversion casts between B <--> A.
		 *			if (!scribe.transcribe_base<A, B>(TRANSCRIBE_SOURCE) ||
		 *				!scribe.transcribe(TRANSCRIBE_SOURCE, b, "b", GPlatesScribe::TRACK))
		 *			{
		 *				return scribe.get_transcribe_result();
		 *			}
		 *
		 *			return GPlatesScribe::TRANSCRIBE_SUCCESS;
		 *		}
		 *   };
		 *
		 *
		 * And this enables the loading of 'a' in the following to succeed:
		 *   
		 *   B b;
		 *   A *a = &b;
		 *   scribe.transcribe(TRANSCRIBE_SOURCE, b, "b", GPlatesScribe::TRACK);
		 *   scribe.transcribe(TRANSCRIBE_SOURCE, a, "a", GPlatesScribe::TRACK);
		 *
		 *
		 * NOTE: Currently this method always returns true (but we still return a result to keep
		 * compatible with other @a transcribe_base overload).
		 */
		template <class BaseType, class DerivedType>
		Bool
		transcribe_base(
				const GPlatesUtils::CallStack::Trace &transcribe_source/* Use 'TRANSCRIBE_SOURCE' here */);

		/**
		 * Saves the specified object to the archive.
		 *
		 * Although the one @a transcribe function handles both saving and loading of *objects*,
		 * when transcribing *constructor* data (for a parent object), the save and load paths need
		 * to be separate as shown in the following example...
		 *
		 *   struct A
		 *   {
		 *		A(const X &x) : d_x(x) {  }
		 *		X d_x;
		 *   };
		 *
		 *   GPlatesScribe::TranscribeResult
		 *   transcribe_construct_data(
		 *			GPlatesScribe::Scribe &scribe,
		 *			GPlatesScribe::ConstructObject<A> &a)
		 *   {
		 *		if (scribe.is_saving())
		 *		{
		 *			scribe.save(TRANSCRIBE_SOURCE, a->d_x, "x", GPlatesScribe::TRACK);
		 *		}
		 *		else // loading
		 *		{
		 *			GPlatesScribe::LoadRef<X> x = scribe.load<X>(TRANSCRIBE_SOURCE, "x", GPlatesScribe::TRACK);
		 *			if (!x.is_valid())
		 *			{
		 *				return scribe.get_transcribe_result();
		 *			}
		 *			
		 *			a.construct_object(x);
		 *			scribe.relocated(TRANSCRIBE_SOURCE, a->d_x, x);
		 *		}
		 *		
		 *		return GPlatesScribe::TRANSCRIBE_SUCCESS;
		 *   }
		 */
		template <typename ObjectType>
		void
		save(
				const GPlatesUtils::CallStack::Trace &transcribe_source, // Use 'TRANSCRIBE_SOURCE' here
				const ObjectType &object,
				const ObjectTag &object_tag,
				unsigned int options = 0);


		/**
		 * Loads an object from the archive.
		 *
		 * If loading fails then the returned 'LoadRef' will test false.
		 * The reason for the failure can then be obtained from @a get_transcribe_result.
		 *
		 * Although the one @a transcribe function handles both saving and loading of *objects*,
		 * when transcribing *constructor* data (for a parent object), the save and load paths need
		 * to be separate as shown in the following example...
		 *
		 *   struct A
		 *   {
		 *		A(const X &x) : d_x(x) {  }
		 *		X d_x;
		 *   };
		 *
		 *   GPlatesScribe::TranscribeResult
		 *   transcribe_construct_data(
		 *			GPlatesScribe::Scribe &scribe,
		 *			GPlatesScribe::ConstructObject<A> &a)
		 *   {
		 *		if (scribe.is_saving())
		 *		{
		 *			scribe.save(TRANSCRIBE_SOURCE, a->d_x, "x", GPlatesScribe::TRACK);
		 *		}
		 *		else // loading
		 *		{
		 *			GPlatesScribe::LoadRef<X> x = scribe.load<X>(TRANSCRIBE_SOURCE, "x", GPlatesScribe::TRACK);
		 *			if (!x.is_valid())
		 *			{
		 *				return scribe.get_transcribe_result();
		 *			}
		 *			
		 *			a.construct_object(x); // <----- can use 'x' instead of 'boost::ref(x.get())'
		 *			scribe.relocated(TRANSCRIBE_SOURCE, a->d_x, x);
		 *		}
		 *		
		 *		return GPlatesScribe::TRANSCRIBE_SUCCESS;
		 *   }
		 * 
		 * Note that the loaded object 'x' has been relocated from outside 'A' to inside 'A'.
		 *
		 * If you don't relocate (a tracked object) then, when all LoadRef's to the tracked object
		 * go out of scope, the object is automatically untracked/discarded. This assumes that you
		 * decided not to use the loaded object for some reason. If you meant to relocate but forgot
		 * to then it should still be OK unless a transcribed pointer references the untracked object
		 * in which case loading will fail.
		 *
		 *
		 * Alternatively, if the loaded object is not being tracked then it does not need to be
		 * relocated as the following example shows...
		 *
		 *	 GPlatesScribe::LoadRef<int> num_elements_ref =
		 *	     scribe.load<int>(TRANSCRIBE_SOURCE, "num_elements", GPlatesScribe::TRACK);
		 *	 if (!num_elements_ref.is_valid())
		 *	 {
		 *		return scribe.get_transcribe_result();
		 *	 }
		 *   int num_elements = num_elements_ref;
		 *
		 * ...where the 'LoadRef<int>' returned by Scribe::load() is automatically converted to an 'int'.
		 */
		template <typename ObjectType>
		LoadRef<ObjectType>
		load(
				const GPlatesUtils::CallStack::Trace &transcribe_source, // Use 'TRANSCRIBE_SOURCE' here
				const ObjectTag &object_tag,
				unsigned int options = 0);


		/**
		 * Saves the specified reference to the archive.
		 *
		 * Although the one @a transcribe function handles both saving and loading of *objects*,
		 * with object *references* this is separated into @a save_reference and @a load_reference.
		 * This is because their function signatures are different.
		 *
		 * If the reference is a data member of a class then you'll likely need to specialise or
		 * overload 'transcribe_construct_data()' or implement a static class method
		 * 'ObjectType::transcribe_construct_data()' - see "Transcribe.h".
		 *
		 * For example...
		 *
		 *   struct A
		 *   {
		 *		A(X &x) : d_x(x) {  }
		 *		X &d_x;
		 *   };
		 *
		 *   GPlatesScribe::TranscribeResult
		 *   transcribe_construct_data(
		 *			GPlatesScribe::Scribe &scribe,
		 *			GPlatesScribe::ConstructObject<A> &a)
		 *   {
		 *		if (scribe.is_saving())
		 *		{
		 *			scribe.save_reference(TRANSCRIBE_SOURCE, a->d_x, "x");
		 *		}
		 *		else // loading
		 *		{
		 *			GPlatesScribe::LoadRef<X> x = scribe.load_reference<X>(TRANSCRIBE_SOURCE, "x");
		 *			if (!x.is_valid())
		 *			{
		 *				return scribe.get_transcribe_result();
		 *			}
		 *			
		 *			a.construct_object(x); // <----- can use 'x' instead of 'boost::ref(x.get())'
		 *			
		 *			// NOTE: No relocation needed for object *reference*.
		 *		}
		 *		
		 *		return GPlatesScribe::TRANSCRIBE_SUCCESS;
		 *   }
		 *
		 * Note that a pointer is also an object, so a reference to a pointer is allowed
		 * (as is a reference to a pointer-to-pointer, etc).
		 */
		template <typename ObjectType>
		void
		save_reference(
				const GPlatesUtils::CallStack::Trace &transcribe_source, // Use 'TRANSCRIBE_SOURCE' here
				ObjectType &object_reference,
				const ObjectTag &object_tag);


		/**
		 * Loads a reference to a previously transcribed object.
		 *
		 * If loading fails then the returned 'LoadRef' will test false.
		 * The reason for the failure can then be obtained from @a get_transcribe_result.
		 *
		 * Although the one @a transcribe function handles both saving and loading of *objects*,
		 * with object *references* this is separated into @a save_reference and @a load_reference.
		 * This is because their function signatures are different.
		 *
		 * If the reference is a data member of a class then you'll likely need to specialise or
		 * overload 'transcribe_construct_data()' or implement a static class method
		 * 'ObjectType::transcribe_construct_data()' - see "Transcribe.h".
		 *
		 * For example...
		 *
		 *   struct A
		 *   {
		 *		A(X &x) : d_x(x) {  }
		 *		X &d_x;
		 *   };
		 *
		 *   GPlatesScribe::TranscribeResult
		 *   transcribe_construct_data(
		 *			GPlatesScribe::Scribe &scribe,
		 *			GPlatesScribe::ConstructObject<A> &a)
		 *   {
		 *		if (scribe.is_saving())
		 *		{
		 *			scribe.save_reference(TRANSCRIBE_SOURCE, a->d_x, "x");
		 *		}
		 *		else // loading
		 *		{
		 *			GPlatesScribe::LoadRef<X> x = scribe.load_reference<X>(TRANSCRIBE_SOURCE, "x");
		 *			if (!x.is_valid())
		 *			{
		 *				return scribe.get_transcribe_result();
		 *			}
		 *			
		 *			a.construct_object(x); // <----- can use 'x' instead of 'boost::ref(x.get())'
		 *			
		 *			// NOTE: No relocation needed for object *reference*.
		 *		}
		 *		
		 *		return GPlatesScribe::TRANSCRIBE_SUCCESS;
		 *   }
		 *
		 * NOTE: Since you are loading a *reference* (to an object), instead of an *object*,
		 * you should *not* relocate the (referenced) object.
		 *
		 * Note that a pointer is also an object, so a reference to a pointer is allowed
		 * (as is a reference to a pointer-to-pointer, etc).
		 */
		template <typename ObjectType>
		LoadRef<ObjectType>
		load_reference(
				const GPlatesUtils::CallStack::Trace &transcribe_source, // Use 'TRANSCRIBE_SOURCE' here
				const ObjectTag &object_tag);


		/**
		 * Returns the result of transcribing the most recently transcribed object in the *load* path.
		 *
		 * This is the result of transcribing using:
		 *   1) @a transcribe (in the *load* path), or
		 *   2) @a transcribe_base (in the *load* path), or
		 *   3) @a load, or
		 *   4) @a load_reference.
		 *
		 * For example:
		 *
		 *		GPlatesScribe::TranscribeResult
		 *		transcribe(
		 *				GPlatesScribe::Scribe &scribe,
		 *				A &a,
		 *				bool transcribed_construct_data)
		 *		{
		 *			if (!scribe.transcribe(TRANSCRIBE_SOURCE, a.x, "x", GPlatesScribe::TRACK) ||
		 *				!scribe.transcribe(TRANSCRIBE_SOURCE, a.y, "y", GPlatesScribe::TRACK))
		 *			{
		 *				return scribe.get_transcribe_result();
		 *			}
		 *			
		 *			return GPlatesScribe::TRANSCRIBE_SUCCESS;
		 *		}
		 */
		TranscribeResult
		get_transcribe_result() const
		{
			return d_transcribe_result;
		}


		/**
		 * Notifies the Scribe that a previously transcribed (loaded) object has been moved to a
		 * new memory location.
		 *
		 * This only applies to *tracked* objects (ie, objects transcribed with the TRACK option).
		 *
		 * This enables tracked objects to continue to be tracked which is essential for
		 * resolving multiple pointers or references to the same object.
		 *
		 * Moving transcribed objects is sometimes necessary when loading an object.
		 * For example:
		 *
		 *   if (scribe.is_saving())
		 *   {
		 *		scribe.transcribe(TRANSCRIBE_SOURCE, container.at(0), "item", GPlatesScribe::TRACK);
		 *		scribe.transcribe(TRANSCRIBE_SOURCE, container.at(1), "item", GPlatesScribe::TRACK);
		 *   }
		 *   else // scribe.is_loading()
		 *   {
		 *		A a; // Need to construct on C runtime stack if cannot transcribe directly into container.
		 *		container.reserve(2); // Avoid re-allocations during 'append'.
		 *		
		 *		if (!scribe.transcribe(TRANSCRIBE_SOURCE, a, "item", GPlatesScribe::TRACK))
		 *		{
		 *			return scribe.get_transcribe_result();
		 *		}
		 *		container.append(a);
		 *		scribe.relocated(TRANSCRIBE_SOURCE, container[0], a); // Now registers &container[0] as the object address.
		 *		
		 *		if (!scribe.transcribe(TRANSCRIBE_SOURCE, a, "item", GPlatesScribe::TRACK)) // Is fine since &a != &container[0]
		 *		{
		 *			return scribe.get_transcribe_result();
		 *		}
		 *		container.append(a);
		 *		scribe.relocated(TRANSCRIBE_SOURCE, container[1], a);
		 *   }
		 *
		 * NOTE: Both @a relocated_object and @a transcribed_object must be currently existing objects.
		 *
		 * Note: The template parameters 'ObjectFirstQualifiedType' and 'ObjectSecondQualifiedType'
		 * must be the same type once top-level const/volatile qualifiers are removed. In other words,
		 * a mixture of const and non-const versions of the same type are allowed.
		 * For multi-level pointers only the top-level const can be different. For example:
		 *   const int **const*
		 *   const int **const*const
		 * ...are fine.
		 *
		 * Note: Internally all 'const's in 'ObjectFirstQualifiedType' and 'ObjectSecondQualifiedType'
		 * are cast away (eg, 'const int **const*' -> 'int ***').
		 * Casting away 'const' is not generally a good idea but we need to be able to modify/fix-up
		 * pointers to objects when those objects are moved in memory. In any case, we already
		 * keep 'non-const' pointers to possibly const objects due to @a transcribe.
		 */
		template <typename ObjectFirstQualifiedType, typename ObjectSecondQualifiedType>
		void
		relocated(
				const GPlatesUtils::CallStack::Trace &transcribe_source, // Use 'TRANSCRIBE_SOURCE' here
				ObjectFirstQualifiedType &relocated_object,
				ObjectSecondQualifiedType &transcribed_object);

		/**
		 * A convenient overload of @a relocated when the transcribed object is a @a LoadRef.
		 */
		template <typename ObjectFirstQualifiedType, typename ObjectSecondQualifiedType>
		void
		relocated(
				const GPlatesUtils::CallStack::Trace &transcribe_source, // Use 'TRANSCRIBE_SOURCE' here
				ObjectFirstQualifiedType &relocated_object,
				LoadRef<ObjectSecondQualifiedType> transcribed_object)
		{
			relocated(transcribe_source, relocated_object, transcribed_object.get());
		}


		/**
		 * Used to determine if the specified (tracked) object has been transcribed.
		 *
		 * NOTE: The object must be a tracked object. If it was not transcribed as a tracked object then
		 * this method will return false regardless of whether the object was transcribed or not.
		 *
		 * Transcribing an object includes calling @a transcribe, @a transcribe_base, @a load or @a save.
		 *
		 * This is useful when an object's class has a non-default constructor.
		 * In this case it's possible that some of the object's data members were transcribed
		 * as constructor parameters and then relocated into the object's data member.
		 * This method can be used to detect this situation to avoid transcribing the data members twice.
		 * Although this can also now be done using the 'transcribed_construct_data' parameter of
		 * the object's 'transcribe()' function (see "Transcribe.h").
		 *
		 * Note that even though @a transcribe may have previously been called on @a object, the object
		 * won't necessarily be completely initialised (when loading from archive) - this is the case
		 * with objects that are pointers when the pointed-to object has not yet been loaded -
		 * but once it's loaded the pointer, @a object, will get initialised to point to it.
		 * Regardless of when the object is actually initialised, this method will always return
		 * true if @a transcribe has been called on the object.
		 *
		 * Note that the actual dynamic object is checked (if 'ObjectType' is polymorphic).
		 */
		template <typename ObjectType>
		bool
		has_been_transcribed(
				ObjectType &object);


		/**
		 * Determines whether the specified object tag exists in the transcription
		 * (transcription is either being written to, on save path, or read from, on load path).
		 *
		 * As with @a transcribe, @a save, @a load, @a save_reference and @a load_reference, the
		 * object tag is relative to the scope of the parent transcribed object (if any, ie, what
		 * is calling this transcribe function) because it's only used to search in that scope.
		 */
		bool
		is_in_transcription(
				const ObjectTag &object_tag) const;


		//
		// Transcribe context methods.
		//


		/**
		 * Pushes a reference to a transcribe context (for the object type 'ObjectType').
		 *
		 * Each 'ObjectType' has its own stack for transcribe contexts.
		 * Pushing and popping transcribe contexts provides an easy way to set a new context (push)
		 * and then restore the previous context (pop).
		 *
		 * Transcribe context is a specialisation of class TranscribeContext for the object type 'ObjectType'
		 * and TranscribeContext should be specialised for 'ObjectType' before using this method.
		 * Note that @a transcribe_context is not copied internally - in other words @a get_transcribe_context
		 * will return a reference to @a transcribe_context.
		 *
		 * NOTE: 'ObjectType' can be any type - it doesn't have to be a type that is transcribed or
		 * export registered, etc - it just needs a TranscribeContext class specialisation as a way
		 * to transport context information.
		 */
		template <typename ObjectType>
		void
		push_transcribe_context(
				TranscribeContext<ObjectType> &transcribe_context);


		/**
		 * Accesses the most recently pushed transcribe context for the specified 'ObjectType', or
		 * return boost::none if none have been pushed (for 'ObjectType').
		 *
		 * This is typically called from within a 'transcribe_construct_data()' specialisation or overload
		 * (see "Transcribe.h") in order to access information required to construct an object that is
		 * being loaded from an archive. Normally all the constructor data is transcribed to the
		 * archive and hence a transcribe context is not necessary. However sometimes a transcribed
		 * object requires a reference to another object that is *not* transcribed and this is where
		 * the transcribe context becomes useful.
		 *
		 * If the transcribe context stack is currently empty for 'ObjectType' then an exception is thrown.
		 *
		 * An example scenario...
		 *
		 *   struct A
		 *   {
		 *		A(X &x, Y& y) : d_x(x), d_y(y) {  }
		 *		X &d_x;
		 *		Y &d_y;
		 *   };
		 *
		 *   template <>
		 *   class TranscribeContext<A>
		 *   {
		 *   public:
		 *		TranscribeContext(Y &y_) : y(y_) {  }
		 *		Y &y;
		 *   };
		 *
		 *   GPlatesScribe::TranscribeResult
		 *   transcribe_construct_data(
		 *			GPlatesScribe::Scribe &scribe,
		 *			GPlatesScribe::ConstructObject<A> &a)
		 *   {
		 *		if (scribe.is_saving())
		 *		{
		 *			scribe.save_reference(TRANSCRIBE_SOURCE, a->d_x, "x");
		 *		}
		 *		else // loading
		 *		{
		 *			// Get information that *is* transcribed into the archive.
		 *			GPlatesScribe::LoadRef<X> x = scribe.load_reference<X>(TRANSCRIBE_SOURCE, "x");
		 *			if (!x.is_valid())
		 *			{
		 *				return scribe.get_transcribe_result();
		 *			}
		 *
		 *			// Get information that is *not* transcribed into the archive.
		 *			boost::optional<GPlatesScribe::TranscribeContext<A> &> a_context = scribe.get_transcribe_context<A>();
		 *			if (!a_context)
		 *			{
		 *				return GPlatesScribe::TRANSCRIBE_INCOMPATIBLE;
		 *			}
		 *			
		 *			a.construct_object(x, a_context->y);
		 *		}
		 *		
		 *		return GPlatesScribe::TRANSCRIBE_SUCCESS;
		 *   }
		 *
		 *   Y untranscribed_y;
		 *   TranscribeContext<A> a_context(untranscribed_y);
		 *   GPlatesScribe::ScopedTranscribeContextGuard<A> a_context_guard(scribe, a_context);
		 *   ... // Start transcribing.
		 */
		template <typename ObjectType>
		boost::optional<TranscribeContext<ObjectType> &>
		get_transcribe_context();


		/**
		 * Pops the most recently pushed transcribe context for the object type 'ObjectType'.
		 */
		template <typename ObjectType>
		void
		pop_transcribe_context();


		/**
		 * A convenience RAII class that ensures a pushed transcribe context is popped on scope exit.
		 */
		template <typename ObjectType>
		class ScopedTranscribeContextGuard :
				public boost::noncopyable
		{
		public:

			ScopedTranscribeContextGuard(
					Scribe &scribe,
					TranscribeContext<ObjectType> &transcribe_context) :
				d_scribe(scribe)
			{
				d_scribe.push_transcribe_context(transcribe_context);
			}

			~ScopedTranscribeContextGuard()
			{
				d_scribe.pop_transcribe_context<ObjectType>();
			}

		private:

			Scribe &d_scribe;
		};


		//
		// General query methods.
		//


		/**
		 * Returns the call stack trace at the last point the transcribe was incompatible.
		 *
		 * This is useful when a TranscribeResult code other than TRANSCRIBE_SUCCESS
		 * is propagated back to a root transcribe call (which usually means the session/project
		 * restore failed to load due to an incompatible archive - ie, it was too old or too new).
		 *
		 * The returned trace is empty if the last transcribe call succeeded.
		 */
		std::vector<GPlatesUtils::CallStack::Trace>
		get_transcribe_incompatible_call_stack() const
		{
			return d_transcribe_incompatible_call_stack;
		}


		/**
		 * Returns the current version of the Scribe library/system.
		 *
		 * Note that this is just for modifications to the Scribe library itself.
		 * Modifications to the transcribing of client objects using this Scribe library are
		 * handled naturally by this Scribe library - whether the client changes break
		 * backward/forward compatibility is dependent on how the client handles changes to how
		 * it transcribes. In other words, those client changes do not affect this version number.
		 *
		 * This version gets incremented when modifications are made to the scribe library/system
		 * that break forward compatibility (when newly created archives cannot be read by older
		 * Scribe versions built into older versions of GPlates).
		 */
		static
		unsigned int
		get_current_scribe_version()
		{
			return CURRENT_SCRIBE_VERSION;
		}


		/**
		 * Returns true if transcription is complete.
		 *
		 * This should typically be called after having transcribed all objects to/from the archive.
		 *
		 * Note that this can be called both when saving and loading an archive.
		 * On saving an archive this ensures (among other things) that all pointers will be resolved
		 * when the same archive is loaded and so it is a good idea to only keep the saved archive
		 * if this method returns true (otherwise the object state, when *loading* from the archive,
		 * will be incomplete).
		 *
		 * The main reason for having this method is tracked pointers do not need to be initialised at
		 * the time they are transcribed - they can get initialised afterwards when the pointed-to
		 * object is transcribed (if it's transcribed after rather than before).
		 * But that pointed-to object may never get transcribed, leaving the pointer un-initialised.
		 * So this method checks if that happened.
		 *
		 * If @a emit_warnings is true then a warning is emitted for each uninitialised object.
		 */
		bool
		is_transcription_complete(
				bool emit_warnings = true) const;


		/**
		 * Returns the transcription.
		 *
		 * If saving (default constructor) then the transcription contains results of transcribing.
		 * If loading then just returns the transcription passed into the constructor.
		 */
		Transcription::non_null_ptr_to_const_type
		get_transcription() const
		{
			return d_transcription;
		}

	private:

		//! Typedef for a stack of 'void' references of transcribe contexts.
		typedef std::stack<void *> transcribe_context_stack_type;


		//! Typedef for an integer identifier for a class (or type).
		typedef unsigned int class_id_type;

		/**
		 * Information associated with each registered class (or type).
		 */
		struct ClassInfo
		{
			explicit
			ClassInfo(
					class_id_type class_id_) :
				class_id(class_id_),
				initialised(false)
			{  }

			class_id_type class_id;

			/**
			 * A stack of transcribe contexts pushed by the client.
			 *
			 * These are used to provide context when transcribing objects of this class type.
			 */
			transcribe_context_stack_type transcribe_context_stack;

			/**
			 * Is true if all boost::optional data members below have been initialised.
			 *
			 * They are all initialised in one function call @a initialise_class_info.
			 *
			 * However we keep using boost::optional for the data members to trap us if we
			 * try to access them without first testing this boolean.
			 */
			bool initialised;

			/**
			 * The size of an object associated with this class.
			 */
			boost::optional<std::size_t> object_size;

			/**
			 * The class type info (if available yet).
			 */
			boost::optional<const std::type_info *> object_type_info;

			/**
			 * The class type info for the *dereference* (if a pointer type).
			 *
			 * NOTE: This is only used for pointers (ie, if 'this' class id refers to a pointer type).
			 * In which case @a object_type_info is the pointer type and @a dereference_type_info
			 * is the compile-time type that the pointer dereferences to.
			 * For example,
			 *
			 *   class A { virtual ~A() { } };
			 *   class B : public A { };
			 *   A *a = new B();
			 *
			 * ...where, for pointer type 'A *', 'object_type_info' is typeid(A *) and
			 * 'dereference_type_info' is typeid(A) (not typeid(B)).
			 */
			boost::optional<const std::type_info *> dereference_type_info;

			/**
			 * Used when an object is relocated during archive loading.
			 *
			 * NOTE: This is only used when loading from an archive (not saving to an archive).
			 */
			boost::optional<InternalUtils::Relocated::non_null_ptr_to_const_type> relocated_handler;

			/**
			 * Used to transcribe an object that is referenced by a pointer that *owns* the object.
			 *
			 * It knows how to transcribe a derived class object through a base class pointer.
			 *
			 * This is only used if an object (of 'this' class type) is owned by a pointer.
			 *
			 * NOTE: This is always none for abstract classes.
			 */
			boost::optional<InternalUtils::TranscribeOwningPointer::non_null_ptr_to_const_type> transcribe_owning_pointer;
		};

		/**
		 * Typedef for a pool allocator of class infos.
		 *
		 * Using a boost::object_pool instead of a GPlatesUtils::ObjectPool since we never
		 * individually release the objects back to the pool - see GPlatesUtils::ObjectPool comments.
		 */
		typedef boost::object_pool<ClassInfo> class_info_pool_type;

		//! Typedef for a mapping from registered type to integer class identifier.
		typedef std::map<const std::type_info *, class_id_type, InternalUtils::SortTypeInfoPredicate>
				class_type_to_id_map_type;

		//! Typedef for a sequence of class info structure pointers.
		typedef std::vector<ClassInfo *> class_info_seq_type;


		//! Typedef for an integer identifier for a transcribed object.
		typedef TranscriptionScribeContext::object_id_type object_id_type;

		/**
		 * Typedef for a linked list of object ids.
		 *
		 * We need a doubly-linked list so we can remove elements.
		 */
		typedef GPlatesUtils::SmartNodeLinkedList<object_id_type> object_ids_list_type;

		/**
		 * Typedef for a pool of object id linked list nodes.
		 *
		 * Using a boost::object_pool instead of a GPlatesUtils::ObjectPool since we're not
		 * worried about re-using nodes that are no longer in use and hence we don't individually
		 * release the nodes back to the pool - see GPlatesUtils::ObjectPool comments.
		 */
		typedef boost::object_pool<object_ids_list_type::Node> object_ids_list_node_pool_type;

		/**
		 * Typedef for a call stack (sequence of traces).
		 */
		typedef std::vector<GPlatesUtils::CallStack::Trace> transcribe_call_stack_type;

		/**
		 * Information associated with each transcribed object.
		 *
		 * This includes *pointers* to transcribed objects which are themselves objects because
		 * a client might reference (or have a pointer to) a pointer and so on (eg, a pointer
		 * to a pointer to a pointer).
		 */
		struct ObjectInfo
		{
			explicit
			ObjectInfo(
					object_id_type object_id_) :
				object_id(object_id_),
				is_object_pre_initialised(false),
				is_object_post_initialised(false),
				is_load_object_bound_to_a_reference_or_untracked_pointer(false)
			{  }

			/**
			 * Resets the state when an object is untracked.
			 *
			 * All state is reset except information regarding pointers and references since
			 * that cannot be regained once it's reset.
			 */
			void
			untrack()
			{
				object_address = boost::none;
				is_object_pre_initialised = false;
				is_object_post_initialised = false;
				uninitialised_transcribe_call_stack = boost::none;
				parent_object = boost::none;
				child_objects.clear();
				sub_objects.clear();
				base_class_sub_objects.clear();

				// NOTE: We don't reset 'is_load_object_bound_to_a_reference_or_untracked_pointer',
				// 'pointers_referencing_object' or 'object_referenced_by_pointer' because they
				// cannot be regained once reset.
			}


			object_id_type object_id;

			/**
			 * The class type of this object (if available yet).
			 */
			boost::optional<class_id_type> class_id;

			/**
			 * The tracked object address (if available yet).
			 *
			 * The address gets initialised when transcribing has started on the object.
			 * For a tracked object this address will remain set (and possibly relocated).
			 * For an untracked object this address will get reset to none once transcribing
			 * has ended on the object - untracked objects include objects specified by client
			 * *without* 'TRACK' as well as tracked objects that fail to be successfully transcribed.
			 */
			boost::optional<void *> object_address;

			/**
			 * Has the object been submitted for transcribing.
			 *
			 * This means the object has been, or is currently being, transcribed.
			 *
			 * This does not necessarily mean that the object has been initialised or streamed yet.
			 * @a is_object_post_initialised signifies when that has happened.
			 *
			 * If a pointer, that does not own its pointed-to object, is transcribed before its
			 * pointed-to object is transcribed then, on loading, even though the scribe will know
			 * the object info/id of the pointed-to object it won't have been pre-initialised yet.
			 * That will only happen once the client explicitly transcribes the pointed-to object.
			 */
			bool is_object_pre_initialised;

			/**
			 * Has the object been fully initialised/transcribed/streamed yet.
			 *
			 * On saving an archive this means that this object, if referenced by a pointer, has
			 * actually been transcribed and therefore will be available to resolve pointer
			 * references when the same archive is loaded.
			 *
			 * On loading an archive this tests whether the object is valid (has a valid value).
			 * This also applies to pointer objects (whether they actually point to something yet).
			 */
			bool is_object_post_initialised;

			/**
			 * The call stack at the time an uninitialised object is transcribed.
			 *
			 * This is for those (non-owning) transcribed pointer objects whose initialisation
			 * is delayed (until the pointed-to object is transcribed). If they never get initialised
			 * then the 'Scribe::is_transcription_complete()' check prints an error message for each
			 * one and prints out the transcribe call site so that the scribe client knows where to look.
			 */
			boost::optional<transcribe_call_stack_type> uninitialised_transcribe_call_stack;

			/**
			 * Is the load object referenced by a 'reference' (cannot be re-bound if load object relocated)
			 * or an 'untracked pointer' (cannot be updated if load object relocated) ? 
			 *
			 * NOTE: This is only used when loading from an archive (not saving to an archive).
			 */
			bool is_load_object_bound_to_a_reference_or_untracked_pointer;

			/**
			 * A list of *pointers* (not references) that reference this object.
			 *
			 * In the load path these pointers can either be resolved (if they already point to this
			 * object) or unresolved (if they are waiting for this object to load before pointing to it).
			 */
			object_ids_list_type pointers_referencing_object;

			/**
			 * If this object is a pointer then this is the pointed-to object id.
			 */
			boost::optional<object_id_type> object_referenced_by_pointer;

			/**
			 * The (optional) parent object of this object.
			 *
			 * This object is a child-object of the parent which means this object is being
			 * transcribed whilst a parent object is being transcribed.
			 *
			 * If there is no parent then this object is a top-level transcribed object.
			 * In other words this object is not being transcribed whilst a parent object is
			 * being transcribed.
			 */
			boost::optional<object_id_type> parent_object;

			/**
			 * A list of all objects that were transcribed while this object was being transcribed.
			 *
			 * This includes 'sub_objects' (those child objects that lie inside this object's
			 * memory region) as well as objects outside this object's memory region (such as
			 * elements in a std::vector - the std::vector object itself has a pointer to
			 * heap-allocated memory where the elements reside but this is outside the std::vector object).
			 *
			 * Each child-object can in turn contain its own child/sub/base objects, etc.
			 */
			object_ids_list_type child_objects;

			/**
			 * A list of all objects that were transcribed while this object was being transcribed
			 * *and* where these sub-objects lie inside this object's memory region.
			 *
			 * Sub-objects include data members and direct base classes.
			 *
			 * Each sub-object can in turn contain its own child/sub/base objects, etc.
			 */
			object_ids_list_type sub_objects;

			/**
			 * A list of base class sub-objects of this object.
			 *
			 * This is a list of base class sub-objects that were transcribed while this object
			 * was in the process of being transcribed.
			 *
			 * Each base class sub-object can in turn contain its own child/sub/base objects, etc.
			 */
			object_ids_list_type base_class_sub_objects;
		};

		/**
		 * Typedef for a pool allocator of object infos.
		 *
		 * Using a boost::object_pool instead of a GPlatesUtils::ObjectPool since we're not
		 * worried about re-using objects that are no longer in use and hence we don't individually
		 * release the objects back to the pool - see GPlatesUtils::ObjectPool comments.
		 */
		typedef boost::object_pool<ObjectInfo> object_info_pool_type;

		//! Typedef for an identifier for an object address that uses the address and the object type.
		typedef InternalUtils::ObjectAddress object_address_type;

		/**
		 * Typedef for a mapping from tracked object address to integer object identifier.
		 */
		typedef std::map<object_address_type, object_id_type, InternalUtils::SortObjectAddressPredicate>
				tracked_object_address_to_id_map_type;

		//! Typedef for a sequence of object info structure pointers.
		typedef std::vector<ObjectInfo *> object_info_seq_type;

		//! Typedef for a stack of object ids to track parent-to-sub-object transcribe relationships.
		typedef std::stack<object_id_type> transcribed_object_stack_type;


		/**
		 * Increment this version number when modifications are made to the scribe library/system
		 * break forward compatibility (when newly created archives cannot be read by older Scribe
		 * versions built into older versions of GPlates).
		 */
		static const unsigned int CURRENT_SCRIBE_VERSION = 0;

		/**
		 * The object ID used to identify NULL pointers.
		 */
		static const object_id_type NULL_POINTER_OBJECT_ID = TranscriptionScribeContext::NULL_POINTER_OBJECT_ID;

		/**
		 * An object tag used to transcribe the id of an object pointed-to by a pointer.
		 */
		static const ObjectTag POINTS_TO_OBJECT_TAG;

		/**
		 * An object tag used to transcribe the class name of an object pointed-to by a pointer.
		 */
		static const ObjectTag POINTS_TO_CLASS_TAG;


		/**
		 * Whether the transcription was read from an archive or will be written to one.
		 */
		bool d_is_saving;

		/**
		 * The transcription contains the transcribed state.
		 */
		Transcription::non_null_ptr_type d_transcription;

		/**
		 * Used to save/load to/from the transcription.
		 */
		TranscriptionScribeContext d_transcription_context;

		/**
		 * Used to cast a derived class 'void *' to a base class 'void *' or vice versa.
		 *
		 * This takes care of multiple inheritance pointer fix ups when we don't have the
		 * class types to help us (ie, just have void pointers).
		 */
		VoidCastRegistry d_void_cast_registry;

		//! Keeps track of parent-to-sub-object relationships as objects are transcribed.
		transcribed_object_stack_type d_transcribed_object_stack;

		//! Pool allocator for object id linked list nodes.
		object_ids_list_node_pool_type d_object_ids_list_node_pool;

		//! Pool allocator for object info structures.
		object_info_pool_type d_object_info_pool;

		//! Information about each object, indexed by object id.
		object_info_seq_type d_object_infos;

		//! Maps addresses of tracked objects to their integer object ids.
		tracked_object_address_to_id_map_type d_tracked_object_address_to_id_map;

		//! Maps addresses of registered classes (or types) to their integer class ids.
		class_type_to_id_map_type d_class_type_to_id_map;

		//! Pool allocator for class info structures.
		class_info_pool_type d_class_info_pool;

		//! Information about each class, indexed by class id.
		class_info_seq_type d_class_infos;

		//! The result of transcribing the last transcribed object.
		TranscribeResult d_transcribe_result;

		//! The call stack trace when an incompatible transcribe is first detected.
		transcribe_call_stack_type d_transcribe_incompatible_call_stack;


		//! This is only here to force 'ScribeAccess.o' object file to get referenced and included by linker.
		const Access::export_registered_classes_type &d_exported_registered_classes;


		////////////////////////////////
		// Const conversion delegates //
		////////////////////////////////

		//
		// The methods cast away 'const'ness in the objects passed in via this class's public interface.
		//

		//
		// Why is const conversion needed ?
		//
		// Const conversion is necessary because objects are tracked based on both their address and
		// their *type*. The *type* tracking assumes all 'const's have been removed. This is because
		// we need to be able to link pointers to the objects that they point to as the following
		// example demonstrates...
		//
		//     int var = 1;
		// 	   int *var_ptr = &var;
		//     const int *const *const var_ptr_ptr1 = &var_ptr; // Needs const conversion to link up.
		//     int * *const var_ptr_ptr2 = &var_ptr;            // Happens to be fine without const conversion.
		//
		// ...where the type of the object that both 'var_ptr_ptr1' and 'var_ptr_ptr2' points to
		// (ie, 'var_ptr') is 'int *'. However when 'var_ptr_ptr1' is transcribed, the scribe system
		// sees that it points to an object of type 'const int *const' (the type of '*var_ptr_ptr1')
		// and records this (as well as the address of 'var_ptr' which it gets from the value of 'var_ptr_ptr1').
		// If 'var_ptr' is subsequently transcribed (after 'var_ptr_ptr1') then the scribe system will
		// register its address and the type 'int *'. Even though the addresses are the same,
		// the types are different and so the scribe system will not link 'var_ptr_ptr1' to 'var_ptr'
		// and will complain that the object pointed to by 'var_ptr_ptr1' was never transcribed
		// (with tracking enabled). The other pointer-to-pointer, 'var_ptr_ptr2', just happens to
		// get lucky because the type of '*var_ptr_ptr2' is the same as the type of 'var_ptr'.
		//
		// Removing all 'const's from 'const int *const' to get 'int *' solves the problem.
		//
		// And the reason *type*s are used (as well as addresses) to link pointers to their pointed-to
		// objects is there can be multiple objects at the same address. For example the first
		// data member of a class object has the same address as the class object itself.
		// Another example is the first inherited base class object and the derived class object.
		// But the types at the same address are always guaranteed to be different so we can use
		// the address *and* the type to distinguish between different objects.
		// This is why the Empty Base Optimisation cannot always optimise away empty base classes
		// (see http://en.cppreference.com/w/cpp/language/ebo).
		//

		//
		// A typical delegate const-cast looks like (for a pointer-to-pointer-to-2D-array)...
		// 
		//    template <typename ObjectType, int N1, int N2>
		//    void transcribe_const_cast(
		//            const ObjectType (*const *const &object_array)[N1][N2],
		//            const ObjectTag &object_tag,
		//            unsigned int options)
		//    {
		//        // Delegate to non-const version.
		//        transcribe_object(
		//                const_cast<ObjectType (**&)[N1][N2]>(object_array),
		//                object_tag,
		//                options);
		//    }
		//

		//
		// Due to the existence of (multi-level) pointers and multi-dimensional native arrays
		// we end up with quite a large number of functions to cover all the 'const' combinations.
		//
		// So we resort to using the power of the boost preprocessor library to do all the heavy
		// lifting for us. The parameters 'GPLATES_SCRIBE_MAX_POINTER_DIMENSION' and
		// 'GPLATES_SCRIBE_MAX_ARRAY_DIMENSION' determine the number of combinations.
		//

		//
		// Preprocessor technical detail:
		//
		// It appears *empty* macro arguments are not guaranteed to be supported in C++98 according to...
		//
		//   http://boost.2283326.n4.nabble.com/preprocessor-missing-IS-EMPTY-documentation-tp2663247p2663248.html
		//
		// ...specifically the part (in the above link) that mentions...
		// 
		//    #define BLANK
		//
		//    #define A(x) x
		//
		//    A(BLANK) // valid, even in C90 and C++98
		//    A()      // invalid in C90/C++98
		//             //   but valid in C99/C++0x
		//
		//    #define B(x) A(x)
		//
		//    B(BLANK) // invalid in C90/C++98
		//             //   but valid in C99/C++0x
		//
		// ...so while we could use 'BOOST_PP_EMPTY()' in place of 'BLANK', it would fail (for C++98)
		// if we, in turn, passed that argument onto another macro call as seen with 'BLANK' in the
		// 'B()' macro in the above example.
		//
		// So our solution is to use 'BOOST_PP_EMPTY' *without* the parentheses, pass that through
		// multiple macro calls and only expand (using parentheses) it in the final macro that
		// uses the parameter. So the above example would then be...
		//
		//    #define A(x) x
		//
		//    A(BOOST_PP_EMPTY)   // valid, even in C90 and C++98
		//
		//    #define B(x) A(x)() // note the extra parentheses
		//
		//    B(BOOST_PP_EMPTY)   // now valid, even in C90 and C++98
		//
		// ...and to pass non-empty parameters we use BOOST_PP_IDENTITY which is defined as...
		//
		//    #define BOOST_PP_IDENTITY(item) item BOOST_PP_EMPTY
		//
		// ...which eventually gets expanded using 'BOOST_PP_IDENTITY(item)()' which is the same as
		// 'item BOOST_PP_EMPTY()' which is just 'item'.
		//


		/////////////////////////////////////////////
		// Begin boost preprocessor library macros //
		//                                         //

		// Predicate for GPLATES_SCRIBE_POW2.
		#define GPLATES_SCRIBE_POW2_PRED(d, state) \
				BOOST_PP_TUPLE_ELEM(2, 1, state) \
				/**/

		// Operation for GPLATES_SCRIBE_POW2.
		#define GPLATES_SCRIBE_POW2_MUL_BY_2(d, state) \
				( \
						BOOST_PP_MUL_D(d, BOOST_PP_TUPLE_ELEM(2, 0, state), 2), \
						BOOST_PP_DEC(BOOST_PP_TUPLE_ELEM(2, 1, state)) \
				) \
				/**/

		// This is just pow(2,n) implemented as 1*2*2*2*, ie, repeated 'n' times...
		#define GPLATES_SCRIBE_POW2(n) \
				BOOST_PP_TUPLE_ELEM( \
						2, \
						0, \
						BOOST_PP_WHILE( \
								GPLATES_SCRIBE_POW2_PRED, \
								GPLATES_SCRIBE_POW2_MUL_BY_2, \
								(1, n)) \
						) \
				/**/

		// Predicate tests if array dimension decremented to zero.
		#define GPLATES_SCRIBE_ARRAY_INDICES_PRED(r, state) \
				BOOST_PP_TUPLE_ELEM(2, 1, state) \
				/**/

		// Increment array index and decrements predicate counter.
		#define GPLATES_SCRIBE_ARRAY_INDICES_OP(r, state) \
				( \
						BOOST_PP_INC(BOOST_PP_TUPLE_ELEM(2, 0, state)), \
						BOOST_PP_DEC(BOOST_PP_TUPLE_ELEM(2, 1, state)) \
				) \
				/**/

		// Returns array template index as, eg, '[N3]'.
		#define GPLATES_SCRIBE_ARRAY_TEMPLATE_INDICES_MACRO(r, state) \
				[BOOST_PP_CAT(N, BOOST_PP_TUPLE_ELEM(2, 0, state))] \
				/**/

		// Array template template indices (eg, '[N1] [N2] [N3]').
		//
		// NOTE: We can't use BOOST_PP_REPEAT because we've exceeded its maximum nested depth of 3.
		// So we use BOOST_PP_FOR instead.
		#define GPLATES_SCRIBE_ARRAY_TEMPLATE_INDICES(array_dim) \
				BOOST_PP_FOR( \
						(1, array_dim), \
						GPLATES_SCRIBE_ARRAY_INDICES_PRED, \
						GPLATES_SCRIBE_ARRAY_INDICES_OP, \
						GPLATES_SCRIBE_ARRAY_TEMPLATE_INDICES_MACRO) \
				/**/

		// Returns array template parameter index as, eg, '(int N3)'.
		// The parenthesis are because we're building up a boost preprocessor 'sequence'.
		#define GPLATES_SCRIBE_ARRAY_TEMPLATE_PARAMETER_INDICES_MACRO(r, state) \
				(BOOST_PP_CAT(int N, BOOST_PP_TUPLE_ELEM(2, 0, state))) \
				/**/

		// Array template parameter indices returned as a sequence (eg, '(int N1) (int N2) (int N3)').
		// Later the sequence will get converted to, eg, 'int N1, int N2, int N3'.
		// Using a sequence avoids problem of commas in a macro argument.
		//
		// NOTE: We can't use BOOST_PP_REPEAT because we've exceeded its maximum nested depth of 3.
		// So we use BOOST_PP_FOR instead.
		#define GPLATES_SCRIBE_ARRAY_TEMPLATE_PARAMETER_INDICES(array_dim) \
				BOOST_PP_FOR( \
						(1, array_dim), \
						GPLATES_SCRIBE_ARRAY_INDICES_PRED, \
						GPLATES_SCRIBE_ARRAY_INDICES_OP, \
						GPLATES_SCRIBE_ARRAY_TEMPLATE_PARAMETER_INDICES_MACRO) \
				/**/

		// Returns 'const' if least-significant bit of 'index' is set, otherwise nothing.
		#define GPLATES_SCRIBE_QUALIFIED_OBJECT(index) \
				BOOST_PP_EXPR_IIF( \
						BOOST_PP_MOD(index, 2), \
						const) \
				/**/

		#define GPLATES_SCRIBE_PRINT(z, n, text) text

		// Repeat '*' character 'pointer_level' times.
		//
		// NOTE: Returns nothing when 'pointer_level' is 0.
		#define GPLATES_SCRIBE_UNQUALIFIED_POINTER(z, pointer_level) \
				BOOST_PP_CAT(BOOST_PP_REPEAT_,z)(pointer_level, GPLATES_SCRIBE_PRINT, *) \
				/**/

		// Predicate tests if pointer-level counter is zero.
		#define GPLATES_SCRIBE_QUALIFIED_POINTER_PRED(r, state) \
				BOOST_PP_TUPLE_ELEM(3, 2, state) \
				/**/

		// Right shifts by one bit and tests the least-significant bit (that was shifted out).
		// Also decrements pointer-level counter for predicate.
		#define GPLATES_SCRIBE_QUALIFIED_POINTER_OP(r, state) \
				( \
						BOOST_PP_DIV(BOOST_PP_TUPLE_ELEM(3, 0, state), 2), \
						BOOST_PP_MOD(BOOST_PP_TUPLE_ELEM(3, 0, state), 2), \
						BOOST_PP_DEC(BOOST_PP_TUPLE_ELEM(3, 2, state)) \
				) \
				/**/

		// Return '*const' or '*' depending on the state.
		#define GPLATES_SCRIBE_QUALIFIED_POINTER_MACRO(r, state) \
				BOOST_PP_IIF( \
						BOOST_PP_TUPLE_ELEM(3, 1, state), \
						*const, \
						*) \
				/**/

		// Repeat '*const' or '*' character 'pointer_level' times depending on
		// 'pointer_level' number of bit flags in 'index'.
		//
		// NOTE: Returns nothing when 'pointer_level' is 0.
		#define GPLATES_SCRIBE_QUALIFIED_POINTER(index, pointer_level) \
				BOOST_PP_FOR( \
						( \
								BOOST_PP_DIV(index, 2), \
								BOOST_PP_MOD(index, 2), \
								pointer_level \
						), \
						GPLATES_SCRIBE_QUALIFIED_POINTER_PRED, \
						GPLATES_SCRIBE_QUALIFIED_POINTER_OP, \
						GPLATES_SCRIBE_QUALIFIED_POINTER_MACRO) \
				/**/

		// Generates single argument function delegate overloads for *non-arrays* for a specific multi-level pointer level.
		#define GPLATES_SCRIBE_DELEGATE_SINGLE_ARG_FUNCTIONS_NON_ARRAY_CALL( \
						/* The following end with BOOST_PP_EMPTY... */ \
						qualified_object, \
						qualified_pointer, \
						unqualified_pointer) \
				\
				template <typename ObjectType> \
				bool \
				transcribe_const_cast( \
						qualified_object() ObjectType qualified_pointer() &object, \
						const ObjectTag &object_tag, \
						unsigned int options) \
				{ \
					return transcribe_object( \
							const_cast<ObjectType unqualified_pointer() &>(object), \
							object_tag, \
							options); \
				} \
				\
				template <typename ObjectType> \
				bool \
				transcribe_construct_const_cast( \
						ConstructObject<qualified_object() ObjectType qualified_pointer()> &construct_object, \
						const ObjectTag &object_tag, \
						unsigned int options) \
				{ \
					return transcribe_construct_object( \
							reinterpret_cast<ConstructObject<ObjectType unqualified_pointer()> &>(construct_object), \
							object_tag, \
							options); \
				} \
				\
				template <typename ObjectType> \
				bool \
				transcribe_construct_const_cast( \
						ConstructObject<qualified_object() ObjectType qualified_pointer()> &construct_object, \
						object_id_type object_id, \
						unsigned int options) \
				{ \
					return transcribe_construct_object( \
							reinterpret_cast<ConstructObject<ObjectType unqualified_pointer()> &>(construct_object), \
							object_id, \
							options); \
				} \
				\
				/* Note that we get a non-pointer overload in this set but it never gets used \
				because 'transcribe_smart_pointer_object()' expects a pointer. */ \
				template <typename ObjectType> \
				bool \
				transcribe_smart_pointer_const_cast( \
						qualified_object() ObjectType qualified_pointer() &object, \
						bool shared_owner) \
				{ \
					return transcribe_smart_pointer_object( \
							const_cast<ObjectType unqualified_pointer() &>(object), \
							shared_owner); \
				} \
				\
				template <typename ObjectType> \
				bool \
				has_been_transcribed_const_cast( \
						qualified_object() ObjectType qualified_pointer() &object) \
				{ \
					return has_object_been_transcribed( \
							const_cast<ObjectType unqualified_pointer() &>(object)); \
				} \
				\
				template <typename ObjectType> \
				void \
				untrack_const_cast( \
						qualified_object() ObjectType qualified_pointer() &object, \
						bool discard) \
				{ \
					untrack_object( \
							const_cast<ObjectType unqualified_pointer() &>(object), \
							discard); \
				} \
				\
				template <typename ObjectType> \
				void \
				save_reference_const_cast( \
						qualified_object() ObjectType qualified_pointer() &object_reference, \
						const ObjectTag &object_tag) \
				{ \
					save_object_reference( \
							const_cast<ObjectType unqualified_pointer() &>(object_reference), \
							object_tag); \
				} \
				/**/

		// Generates single argument function delegate overloads for native *arrays* for a specific multi-level pointer level.
		#define GPLATES_SCRIBE_DELEGATE_SINGLE_ARG_FUNCTIONS_ARRAY_CALL( \
						array_template_parameter_indices, \
						array_template_indices, \
						/* The following end with BOOST_PP_EMPTY... */ \
						qualified_object, \
						qualified_pointer, \
						unqualified_pointer) \
				\
				template <typename ObjectType, BOOST_PP_SEQ_ENUM(array_template_parameter_indices)> \
				bool \
				transcribe_const_cast( \
						qualified_object() ObjectType (qualified_pointer() &array) array_template_indices, \
						const ObjectTag &object_tag, \
						unsigned int options) \
				{ \
					/* Check array dimension (rank) does not exceed GPLATES_SCRIBE_MAX_ARRAY_DIMENSION... */ \
					BOOST_STATIC_ASSERT(boost::mpl::not_< boost::is_array<ObjectType> >::value); \
					\
					return transcribe_object( \
							const_cast<ObjectType (unqualified_pointer() &) array_template_indices>(array), \
							object_tag, \
							options); \
				} \
				\
				/* Exclude 'transcribe_construct_const_cast()' since not allowing it for native arrays. \
				If it does get used by client then it'll either get trapped in a compile-time 'const'
				assertion check in 'transcribe_construct_object()' or a run-time assertion in 'transcribe_construct_data()'
				in 'TranscribeArray.h'. */ \
				\
				/* We also exclude 'transcribe_smart_pointer_const_cast()' since we're not expecting \
				arrays to be heap-allocated (perhaps support in future though). */ \
				\
				template <typename ObjectType, BOOST_PP_SEQ_ENUM(array_template_parameter_indices)> \
				bool \
				has_been_transcribed_const_cast( \
						qualified_object() ObjectType (qualified_pointer() &array) array_template_indices) \
				{ \
					/* Check array dimension (rank) does not exceed GPLATES_SCRIBE_MAX_ARRAY_DIMENSION... */ \
					BOOST_STATIC_ASSERT(boost::mpl::not_< boost::is_array<ObjectType> >::value); \
					\
					return has_object_been_transcribed( \
							const_cast<ObjectType (unqualified_pointer() &) array_template_indices>(array)); \
				}  \
				\
				template <typename ObjectType, BOOST_PP_SEQ_ENUM(array_template_parameter_indices)> \
				void \
				untrack_const_cast( \
						qualified_object() ObjectType (qualified_pointer() &array) array_template_indices, \
						bool discard) \
				{ \
					/* Check array dimension (rank) does not exceed GPLATES_SCRIBE_MAX_ARRAY_DIMENSION... */ \
					BOOST_STATIC_ASSERT(boost::mpl::not_< boost::is_array<ObjectType> >::value); \
					\
					untrack_object( \
							const_cast<ObjectType (unqualified_pointer() &) array_template_indices>(array), \
							discard); \
				} \
				\
				template <typename ObjectType, BOOST_PP_SEQ_ENUM(array_template_parameter_indices)> \
				void \
				save_reference_const_cast( \
						qualified_object() ObjectType (qualified_pointer() &array_reference) array_template_indices, \
						const ObjectTag &object_tag) \
				{ \
					/* Check array dimension (rank) does not exceed GPLATES_SCRIBE_MAX_ARRAY_DIMENSION... */ \
					BOOST_STATIC_ASSERT(boost::mpl::not_< boost::is_array<ObjectType> >::value); \
					\
					save_object_reference( \
							const_cast<ObjectType (unqualified_pointer() &) array_template_indices>(array_reference), \
							object_tag); \
				} \
				/**/

		// Generates single argument function delegate overloads for native *arrays* for a specific multi-level pointer level.
		#define GPLATES_SCRIBE_DELEGATE_SINGLE_ARG_FUNCTIONS_ARRAY(z, array_dim, data) \
				GPLATES_SCRIBE_DELEGATE_SINGLE_ARG_FUNCTIONS_ARRAY_CALL( \
						/* This is actually a sequence, eg, '(int N1) (int N2) (int N3)' \
						so that we can pass to macro without worrying about commas... */ \
						GPLATES_SCRIBE_ARRAY_TEMPLATE_PARAMETER_INDICES(array_dim), \
						GPLATES_SCRIBE_ARRAY_TEMPLATE_INDICES(array_dim), \
						/* The following end with BOOST_PP_EMPTY... */ \
						BOOST_PP_TUPLE_ELEM(3, 0, data), /*qualified_object*/ \
						BOOST_PP_TUPLE_ELEM(3, 1, data), /*qualified_pointer*/ \
						BOOST_PP_TUPLE_ELEM(3, 2, data) /*unqualified_pointer*/) \
				/**/

		// Generates single argument function delegate overloads for a specific multi-level pointer level.
		#define GPLATES_SCRIBE_DELEGATE_SINGLE_ARG_FUNCTIONS_CALL( \
						z, \
						/* The following end with BOOST_PP_EMPTY... */ \
						qualified_object, \
						qualified_pointer, \
						unqualified_pointer) \
				\
				/* Delegate non-arrays... */ \
				GPLATES_SCRIBE_DELEGATE_SINGLE_ARG_FUNCTIONS_NON_ARRAY_CALL( \
						/* The following end with BOOST_PP_EMPTY... */ \
						qualified_object, \
						qualified_pointer, \
						unqualified_pointer) \
				/* Delegate arrays by looping over array dimensions [1, GPLATES_SCRIBE_MAX_ARRAY_DIMENSION] ... */ \
				BOOST_PP_CAT(BOOST_PP_REPEAT_FROM_TO_,z)( \
						1, \
						BOOST_PP_INC(GPLATES_SCRIBE_MAX_ARRAY_DIMENSION), \
						GPLATES_SCRIBE_DELEGATE_SINGLE_ARG_FUNCTIONS_ARRAY, \
						/* The following end with BOOST_PP_EMPTY... */ \
						(qualified_object, qualified_pointer, unqualified_pointer)) \
				/**/

		// Generates single argument function delegate overloads for
		// a multi-level pointer of dimension 'pointer_level'.
		//
		// Note: The 'BOOST_PP_DIV(index, 2)' shifts out the least-significant bit of 'index' used by
		// 'GPLATES_SCRIBE_QUALIFIED_OBJECT()' - the remaining bits are used for the multi-level
		// pointer levels in 'GPLATES_SCRIBE_QUALIFIED_POINTER()'.
		//
		// Note: Calculating 'GPLATES_SCRIBE_QUALIFIED_OBJECT',
		// 'GPLATES_SCRIBE_UNQUALIFIED_POINTER()' and
		// 'GPLATES_SCRIBE_QUALIFIED_POINTER' only once here (instead of once per
		// delegated function) reduces time spent in the preprocessor noticeably.
		#define GPLATES_SCRIBE_DELEGATE_SINGLE_ARG_FUNCTIONS_INDEX(z, index, pointer_level) \
				GPLATES_SCRIBE_DELEGATE_SINGLE_ARG_FUNCTIONS_CALL( \
						z, \
						/* Using BOOST_PP_IDENTITY since the following macros may return nothing. */ \
						/* This causes the following to end with BOOST_PP_EMPTY... */ \
						BOOST_PP_IDENTITY(GPLATES_SCRIBE_QUALIFIED_OBJECT(index)), \
						BOOST_PP_IDENTITY(GPLATES_SCRIBE_QUALIFIED_POINTER(BOOST_PP_DIV(index, 2), pointer_level)), \
						BOOST_PP_IDENTITY(GPLATES_SCRIBE_UNQUALIFIED_POINTER(z, pointer_level))) \
				/**/

		// Iterate over half-open range [ 0, 2*pow(2,pointer_level) ) and generate all const/non-const
		// multi-level pointer combinations for a particular pointer-level.
		// Pass 'pointer_level' as auxiliary data.
		//
		// Note: The 'BOOST_PP_INC(pointer_level)' is because we need two times as many combinations
		// of 'const' due to the const/non-const of the final pointed-to object type.
		// 'GPLATES_SCRIBE_QUALIFIED(index)'.
		#define GPLATES_SCRIBE_DELEGATE_SINGLE_ARG_FUNCTIONS(z, pointer_level, _) \
				BOOST_PP_CAT(BOOST_PP_REPEAT_,z)( \
						GPLATES_SCRIBE_POW2(BOOST_PP_INC(pointer_level)), \
						GPLATES_SCRIBE_DELEGATE_SINGLE_ARG_FUNCTIONS_INDEX, \
						pointer_level) \
				/**/


		// Generate single argument function delegate overloads for
		// all multi-level pointer levels up to maximum pointer dimension.
		//
		// NOTE: If the following compile-time assertion is triggered here...
		//
		//    "use of undefined type 'boost::STATIC_ASSERTION_FAILURE<x>'"
		//
		// ...then a native array with dimension (actually rank) greater than
		// 'GPLATES_SCRIBE_MAX_ARRAY_DIMENSION' was transcribed, etc.
		//
		BOOST_PP_REPEAT( \
				BOOST_PP_INC(GPLATES_SCRIBE_MAX_POINTER_DIMENSION), \
				GPLATES_SCRIBE_DELEGATE_SINGLE_ARG_FUNCTIONS, \
				_)


		// Generates double argument function delegate overloads for *non-arrays*
		// for a specific multi-level pointer level.
		#define GPLATES_SCRIBE_DELEGATE_DOUBLE_ARG_FUNCTIONS_NON_ARRAY_CALL( \
						/* The following end with BOOST_PP_EMPTY... */ \
						qualified_object, \
						qualified_pointer, \
						unqualified_pointer) \
				\
				template <typename ObjectType> \
				void \
				relocated_const_cast( \
						qualified_object() ObjectType qualified_pointer() &relocated_object, \
						qualified_object() ObjectType qualified_pointer() &transcribed_object) \
				{ \
					relocated_transcribed_object( \
							const_cast<ObjectType unqualified_pointer() &>(relocated_object), \
							const_cast<ObjectType unqualified_pointer() &>(transcribed_object)); \
				} \
				\
				template <typename ObjectType> \
				void \
				relocated_const_cast( \
						qualified_object() ObjectType qualified_pointer() const &relocated_object, \
						qualified_object() ObjectType qualified_pointer() &transcribed_object) \
				{ \
					relocated_transcribed_object( \
							const_cast<ObjectType unqualified_pointer() &>(relocated_object), \
							const_cast<ObjectType unqualified_pointer() &>(transcribed_object)); \
				} \
				\
				template <typename ObjectType> \
				void \
				relocated_const_cast( \
						qualified_object() ObjectType qualified_pointer() &relocated_object, \
						qualified_object() ObjectType qualified_pointer() const &transcribed_object) \
				{ \
					relocated_transcribed_object( \
							const_cast<ObjectType unqualified_pointer() &>(relocated_object), \
							const_cast<ObjectType unqualified_pointer() &>(transcribed_object)); \
				} \
				\
				template <typename ObjectType> \
				void \
				relocated_const_cast( \
						qualified_object() ObjectType qualified_pointer() const &relocated_object, \
						qualified_object() ObjectType qualified_pointer() const &transcribed_object) \
				{ \
					relocated_transcribed_object( \
							const_cast<ObjectType unqualified_pointer() &>(relocated_object), \
							const_cast<ObjectType unqualified_pointer() &>(transcribed_object)); \
				} \
				/**/

		// Generates double argument function delegate overloads for native *arrays*
		// for a specific multi-level pointer level.
		#define GPLATES_SCRIBE_DELEGATE_DOUBLE_ARG_FUNCTIONS_ARRAY_CALL( \
						array_template_parameter_indices, \
						array_template_indices, \
						/* The following end with BOOST_PP_EMPTY... */ \
						qualified_object, \
						qualified_object_const, \
						qualified_pointer, \
						qualified_pointer_const, \
						unqualified_pointer) \
				\
				template <typename ObjectType, BOOST_PP_SEQ_ENUM(array_template_parameter_indices)> \
				void \
				relocated_const_cast( \
						qualified_object() ObjectType (qualified_pointer() &relocated_array) array_template_indices, \
						qualified_object() ObjectType (qualified_pointer() &transcribed_array) array_template_indices) \
				{ \
					/* Check array dimension (rank) does not exceed GPLATES_SCRIBE_MAX_ARRAY_DIMENSION... */ \
					BOOST_STATIC_ASSERT(boost::mpl::not_< boost::is_array<ObjectType> >::value); \
					\
					relocated_transcribed_object( \
							const_cast<ObjectType (unqualified_pointer() &) array_template_indices>(relocated_array), \
							const_cast<ObjectType (unqualified_pointer() &) array_template_indices>(transcribed_array)); \
				} \
				\
				template <typename ObjectType, BOOST_PP_SEQ_ENUM(array_template_parameter_indices)> \
				void \
				relocated_const_cast( \
						qualified_object_const() ObjectType (qualified_pointer_const() &relocated_array) array_template_indices, \
						qualified_object() ObjectType (qualified_pointer() &transcribed_array) array_template_indices) \
				{ \
					/* Check array dimension (rank) does not exceed GPLATES_SCRIBE_MAX_ARRAY_DIMENSION... */ \
					BOOST_STATIC_ASSERT(boost::mpl::not_< boost::is_array<ObjectType> >::value); \
					\
					relocated_transcribed_object( \
							const_cast<ObjectType (unqualified_pointer() &) array_template_indices>(relocated_array), \
							const_cast<ObjectType (unqualified_pointer() &) array_template_indices>(transcribed_array)); \
				} \
				\
				template <typename ObjectType, BOOST_PP_SEQ_ENUM(array_template_parameter_indices)> \
				void \
				relocated_const_cast( \
						qualified_object() ObjectType (qualified_pointer() &relocated_array) array_template_indices, \
						qualified_object_const() ObjectType (qualified_pointer_const() &transcribed_array) array_template_indices) \
				{ \
					/* Check array dimension (rank) does not exceed GPLATES_SCRIBE_MAX_ARRAY_DIMENSION... */ \
					BOOST_STATIC_ASSERT(boost::mpl::not_< boost::is_array<ObjectType> >::value); \
					\
					relocated_transcribed_object( \
							const_cast<ObjectType (unqualified_pointer() &) array_template_indices>(relocated_array), \
							const_cast<ObjectType (unqualified_pointer() &) array_template_indices>(transcribed_array)); \
				} \
				\
				template <typename ObjectType, BOOST_PP_SEQ_ENUM(array_template_parameter_indices)> \
				void \
				relocated_const_cast( \
						qualified_object_const() ObjectType (qualified_pointer_const() &relocated_array) array_template_indices, \
						qualified_object_const() ObjectType (qualified_pointer_const() &transcribed_array) array_template_indices) \
				{ \
					/* Check array dimension (rank) does not exceed GPLATES_SCRIBE_MAX_ARRAY_DIMENSION... */ \
					BOOST_STATIC_ASSERT(boost::mpl::not_< boost::is_array<ObjectType> >::value); \
					\
					relocated_transcribed_object( \
							const_cast<ObjectType (unqualified_pointer() &) array_template_indices>(relocated_array), \
							const_cast<ObjectType (unqualified_pointer() &) array_template_indices>(transcribed_array)); \
				} \
				/**/

		// Generates double *non-pointer* argument function delegate overloads for native *arrays*.
		#define GPLATES_SCRIBE_DELEGATE_DOUBLE_ARG_NON_POINTER_FUNCTIONS_ARRAY(z, array_dim, _) \
				GPLATES_SCRIBE_DELEGATE_DOUBLE_ARG_FUNCTIONS_ARRAY_CALL( \
						/* This is actually a sequence, eg, '(int N1) (int N2) (int N3)' \
						so that we can pass to macro without worrying about commas... */ \
						GPLATES_SCRIBE_ARRAY_TEMPLATE_PARAMETER_INDICES(array_dim), \
						GPLATES_SCRIBE_ARRAY_TEMPLATE_INDICES(array_dim), \
						/* The following end with BOOST_PP_EMPTY... */ \
						BOOST_PP_EMPTY,           /*qualified_object*/ \
						/* For arrays the const goes on the the final pointed-to object. */ \
						/* However two of the four functions generated will never get used because \
						only the top-level const can differ (due to compile-time assertion in \
						public 'relocated()' method) and with arrays this means the actual array \
						objects will be the same type (same const-ness). \
						But we'll keep them anyway since it's easier to code. */ \
						BOOST_PP_IDENTITY(const), /*qualified_object_const*/ \
						BOOST_PP_EMPTY,           /*qualified_pointer*/ \
						BOOST_PP_EMPTY,           /*qualified_pointer_const*/ \
						BOOST_PP_EMPTY)           /*unqualified_pointer*/ \
				/**/

		// Double *non-pointer* argument function delegates.
		#define GPLATES_SCRIBE_DELEGATE_DOUBLE_ARG_NON_POINTER_FUNCTIONS() \
				/* Delegate non-arrays... */ \
				GPLATES_SCRIBE_DELEGATE_DOUBLE_ARG_FUNCTIONS_NON_ARRAY_CALL( \
						/* The following end with BOOST_PP_EMPTY... */ \
						BOOST_PP_EMPTY, /*qualified_object*/ \
						BOOST_PP_EMPTY, /*qualified_pointer*/ \
						BOOST_PP_EMPTY) /*unqualified_pointer*/ \
				/* Delegate arrays by looping over array dimensions [1, GPLATES_SCRIBE_MAX_ARRAY_DIMENSION] ... */ \
				BOOST_PP_REPEAT_FROM_TO( \
						1, \
						BOOST_PP_INC(GPLATES_SCRIBE_MAX_ARRAY_DIMENSION), \
						GPLATES_SCRIBE_DELEGATE_DOUBLE_ARG_NON_POINTER_FUNCTIONS_ARRAY, \
						_) \
				/**/

		// Generates double pointer argument function delegate overloads for native *arrays*
		// for a specific multi-level pointer level.
		#define GPLATES_SCRIBE_DELEGATE_DOUBLE_ARG_POINTER_FUNCTIONS_ARRAY(z, array_dim, data) \
				GPLATES_SCRIBE_DELEGATE_DOUBLE_ARG_FUNCTIONS_ARRAY_CALL( \
						/* This is actually a sequence, eg, '(int N1) (int N2) (int N3)' \
						so that we can pass to macro without worrying about commas... */ \
						GPLATES_SCRIBE_ARRAY_TEMPLATE_PARAMETER_INDICES(array_dim), \
						GPLATES_SCRIBE_ARRAY_TEMPLATE_INDICES(array_dim), \
						/* The following end with BOOST_PP_EMPTY... */ \
						BOOST_PP_TUPLE_ELEM(3, 0, data),       /*qualified_object*/ \
						/* For pointers-to-arrays the const goes on the pointer not the final pointed-to object... */ \
						BOOST_PP_TUPLE_ELEM(3, 0, data),       /*qualified_object_const*/ \
						BOOST_PP_TUPLE_ELEM(3, 1, data),       /*qualified_pointer*/ \
						/* For pointers-to-arrays the const goes on the pointer not the final pointed-to object... */ \
						BOOST_PP_IDENTITY(BOOST_PP_TUPLE_ELEM(3, 1, data)() const), /*qualified_pointer_const*/ \
						BOOST_PP_TUPLE_ELEM(3, 2, data))       /*unqualified_pointer*/ \
				/**/

		// Generates double pointer argument function delegate overloads for a specific multi-level pointer level.
		#define GPLATES_SCRIBE_DELEGATE_DOUBLE_ARG_POINTER_FUNCTIONS_CALL( \
						z, \
						/* The following end with BOOST_PP_EMPTY... */ \
						qualified_object, \
						qualified_pointer, \
						unqualified_pointer) \
				\
				/* Delegate non-arrays... */ \
				GPLATES_SCRIBE_DELEGATE_DOUBLE_ARG_FUNCTIONS_NON_ARRAY_CALL( \
						/* The following end with BOOST_PP_EMPTY... */ \
						qualified_object, \
						qualified_pointer, \
						unqualified_pointer) \
				/* Delegate arrays by looping over array dimensions [1, GPLATES_SCRIBE_MAX_ARRAY_DIMENSION] ... */ \
				BOOST_PP_CAT(BOOST_PP_REPEAT_FROM_TO_,z)( \
						1, \
						BOOST_PP_INC(GPLATES_SCRIBE_MAX_ARRAY_DIMENSION), \
						GPLATES_SCRIBE_DELEGATE_DOUBLE_ARG_POINTER_FUNCTIONS_ARRAY, \
						/* The following end with BOOST_PP_EMPTY... */ \
						(qualified_object, qualified_pointer, unqualified_pointer)) \
				/**/

		// Generates double argument pointer function delegate overloads for
		// a multi-level pointer of dimension 'pointer_level'.
		//
		// Note: The 'BOOST_PP_DIV(index, 2)' shifts out the least-significant bit of 'index' used by
		// 'GPLATES_SCRIBE_QUALIFIED_OBJECT()' - the remaining bits are used for the multi-level
		// pointer levels in 'GPLATES_SCRIBE_QUALIFIED_POINTER()'.
		//
		// Note: The 'BOOST_PP_DEC(pointer_level)' is there because the last pointer '*'
		// (which is explicitly concatenated below) is always non-const - its const-ness is
		// manually enumerated in 'GPLATES_SCRIBE_DELEGATE_DOUBLE_ARG_FUNCTIONS_CALL()'.
		//
		// Note: Calculating 'GPLATES_SCRIBE_QUALIFIED_OBJECT',
		// 'GPLATES_SCRIBE_UNQUALIFIED_POINTER()' and
		// 'GPLATES_SCRIBE_QUALIFIED_POINTER' only once here (instead of once per
		// delegated function) reduces time spent in the preprocessor noticeably.
		#define GPLATES_SCRIBE_DELEGATE_DOUBLE_ARG_POINTER_FUNCTIONS_INDEX(z, index, pointer_level) \
				GPLATES_SCRIBE_DELEGATE_DOUBLE_ARG_POINTER_FUNCTIONS_CALL( \
						z, \
						/* Using BOOST_PP_IDENTITY since the following macros may return nothing. */ \
						/* This causes the following to end with BOOST_PP_EMPTY... */ \
						BOOST_PP_IDENTITY(GPLATES_SCRIBE_QUALIFIED_OBJECT(index)), \
						BOOST_PP_IDENTITY( \
								GPLATES_SCRIBE_QUALIFIED_POINTER( \
										BOOST_PP_DIV(index, 2), \
										BOOST_PP_DEC(pointer_level)) \
								*), \
						BOOST_PP_IDENTITY(GPLATES_SCRIBE_UNQUALIFIED_POINTER(z, pointer_level))) \
				/**/

		// Double *pointer* argument function delegates.
		// Iterate over half-open range [ 0, pow(2,pointer_level) ) and generate all const/non-const
		// multi-level pointer combinations for a particular pointer-level.
		// Pass 'pointer_level' as auxiliary data.
		#define GPLATES_SCRIBE_DELEGATE_DOUBLE_ARG_POINTER_FUNCTIONS(z, pointer_level, _) \
				BOOST_PP_CAT(BOOST_PP_REPEAT_,z)( \
						GPLATES_SCRIBE_POW2(pointer_level), \
						GPLATES_SCRIBE_DELEGATE_DOUBLE_ARG_POINTER_FUNCTIONS_INDEX, \
						pointer_level) \
				/**/


		// Generate double argument function delegate overloads for
		// all multi-level pointer levels up to maximum pointer dimension.
		//
		// NOTE: If the following compile-time assertion is triggered here...
		//
		//    "use of undefined type 'boost::STATIC_ASSERTION_FAILURE<x>'"
		//
		// ...then a native array with dimension (actually rank) greater than
		// 'GPLATES_SCRIBE_MAX_ARRAY_DIMENSION' was transcribed, etc.
		//
		GPLATES_SCRIBE_DELEGATE_DOUBLE_ARG_NON_POINTER_FUNCTIONS() /* Handle *non-pointer* case. */
		BOOST_PP_REPEAT_FROM_TO( \
				1, \
				BOOST_PP_INC(GPLATES_SCRIBE_MAX_POINTER_DIMENSION), \
				GPLATES_SCRIBE_DELEGATE_DOUBLE_ARG_POINTER_FUNCTIONS, \
				_) /* Handle *pointer* cases. */


		#undef GPLATES_SCRIBE_POW2_PRED
		#undef GPLATES_SCRIBE_POW2_MUL_BY_2
		#undef GPLATES_SCRIBE_POW2
		#undef GPLATES_SCRIBE_ARRAY_INDICES_PRED
		#undef GPLATES_SCRIBE_ARRAY_INDICES_OP
		#undef GPLATES_SCRIBE_ARRAY_TEMPLATE_INDICES_MACRO
		#undef GPLATES_SCRIBE_ARRAY_TEMPLATE_INDICES
		#undef GPLATES_SCRIBE_ARRAY_TEMPLATE_PARAMETER_INDICES_MACRO
		#undef GPLATES_SCRIBE_ARRAY_TEMPLATE_PARAMETER_INDICES
		#undef GPLATES_SCRIBE_QUALIFIED_OBJECT
		#undef GPLATES_SCRIBE_PRINT
		#undef GPLATES_SCRIBE_UNQUALIFIED_POINTER
		#undef GPLATES_SCRIBE_QUALIFIED_POINTER_PRED
		#undef GPLATES_SCRIBE_QUALIFIED_POINTER_OP
		#undef GPLATES_SCRIBE_QUALIFIED_POINTER_MACRO
		#undef GPLATES_SCRIBE_QUALIFIED_POINTER
		#undef GPLATES_SCRIBE_DELEGATE_SINGLE_ARG_FUNCTIONS_NON_ARRAY_CALL
		#undef GPLATES_SCRIBE_DELEGATE_SINGLE_ARG_FUNCTIONS_ARRAY_CALL
		#undef GPLATES_SCRIBE_DELEGATE_SINGLE_ARG_FUNCTIONS_ARRAY
		#undef GPLATES_SCRIBE_DELEGATE_SINGLE_ARG_FUNCTIONS_CALL
		#undef GPLATES_SCRIBE_DELEGATE_SINGLE_ARG_FUNCTIONS_INDEX
		#undef GPLATES_SCRIBE_DELEGATE_SINGLE_ARG_FUNCTIONS
		#undef GPLATES_SCRIBE_DELEGATE_DOUBLE_ARG_FUNCTIONS_NON_ARRAY_CALL
		#undef GPLATES_SCRIBE_DELEGATE_DOUBLE_ARG_FUNCTIONS_ARRAY_CALL
		#undef GPLATES_SCRIBE_DELEGATE_DOUBLE_ARG_NON_POINTER_FUNCTIONS_ARRAY
		#undef GPLATES_SCRIBE_DELEGATE_DOUBLE_ARG_NON_POINTER_FUNCTIONS
		#undef GPLATES_SCRIBE_DELEGATE_DOUBLE_ARG_POINTER_FUNCTIONS_ARRAY
		#undef GPLATES_SCRIBE_DELEGATE_DOUBLE_ARG_POINTER_FUNCTIONS_CALL
		#undef GPLATES_SCRIBE_DELEGATE_DOUBLE_ARG_POINTER_FUNCTIONS_INDEX
		#undef GPLATES_SCRIBE_DELEGATE_DOUBLE_ARG_POINTER_FUNCTIONS

		//                                       //
		// End boost preprocessor library macros //
		///////////////////////////////////////////


		/**
		 * A metafunction used to catch (at compile-time) any transcribed objects that are
		 * pointers with a dimension greater than 'GPLATES_SCRIBE_MAX_POINTER_DIMENSION'.
		 *
		 * These high dimension multi-level pointer objects cannot be properly const-cast and
		 * hence are not supported for transcribing.
		 *
		 * Can be used like:
		 *
		 *    BOOST_STATIC_ASSERT(boost::mpl::not_< UnsupportedPointerType<ObjectType> >::value);
		 *
		 */
		template <typename ObjectType, int Dim=0> // Primary template.
		struct UnsupportedPointerType :
				public boost::mpl::eval_if<
						boost::is_pointer<ObjectType>,
						UnsupportedPointerType<typename boost::remove_pointer<ObjectType>::type, Dim+1>,
						boost::mpl::false_>
		{  };
		template <typename ObjectType> // Partial specialisation to terminate recursion.
		struct UnsupportedPointerType<ObjectType, GPLATES_SCRIBE_MAX_POINTER_DIMENSION+1> :
				public boost::mpl::true_
		{  };

		// 		template <typename ObjectType, int Dim=0>
		// 		struct UnsupportedPointerType :
		// 				public boost::mpl::eval_if<
		// 						boost::is_pointer<ObjectType>,
		// 						boost::mpl::eval_if<
		// 								boost::mpl::greater<
		// 										boost::mpl::int_<Dim+1>,
		// 										boost::mpl::int_<GPLATES_SCRIBE_MAX_POINTER_DIMENSION> >,
		// 								boost::mpl::true_,
		// 								UnsupportedPointerType<typename boost::remove_pointer<ObjectType>::type, Dim+1> >,
		// 						boost::mpl::false_>
		// 		{  };


		/**
		 * Untrack a tracked object.
		 *
		 * If @a discard is true then the transcribed object is not going to be used and hence
		 * all objects transcribed while it was being transcribed will also be untracked.
		 * If @a discard is false then the object is simply being untracked but will still get used.
		 *
		 * Note that the actual dynamic object is untracked (if 'ObjectType' is polymorphic).
		 */
		template <typename ObjectType>
		void
		untrack(
				ObjectType &object,
				bool discard);


		/**
		 * A version of @a transcribe that accepts a un-initialised object that needs to be constructed.
		 *
		 * The un-initialised object is wrapped in a 'ConstructObject'. This tells the Scribe that
		 * first the object needs to be constructed using a possibly non-default constructor, and
		 * then transcribed as in the case of the other @a transcribe overload.
		 */
		template <typename ObjectType>
		bool
		transcribe_construct(
				ConstructObject<ObjectType> &object,
				const ObjectTag &object_tag,
				unsigned int options)
		{
			return transcribe_construct_const_cast(object, object_tag, options);
		}


		/**
		 * An overload of @a transcribe_construct that accepts an object id instead of object tag name/version.
		 *
		 * This is used when the object id has already been transcribed.
		 */
		template <typename ObjectType>
		bool
		transcribe_construct(
				ConstructObject<ObjectType> &object,
				object_id_type object_id,
				unsigned int options)
		{
			return transcribe_construct_const_cast(object, object_id, options);
		}


		/**
		 * Transcribe a pointer-owned object according to the smart pointer protocol.
		 *
		 * This method enables the smart pointer protocol whereby smart pointer classes are interchangeable
		 * with each other (and raw pointers) without breaking backward/forward compatibility.
		 *
		 * If @a shared_owner is true then ownership is shared amongst one or more pointers,
		 * otherwise ownership is exclusive to a single pointer.
		 */
		template <typename ObjectType>
		bool
		transcribe_smart_pointer(
				ObjectType *&object_ptr,
				bool shared_owner)
		{
			return transcribe_smart_pointer_const_cast(object_ptr, shared_owner);
		}


		/**
		 * A transcribed object type has delegated transcribing to another object type.
		 *
		 * This method enables object types to be interchangeable since they are transcription
		 * compatible with each other without breaking backward/forward compatibility.
		 */
		template <typename ObjectType>
		bool
		transcribe_delegate(
				ObjectType &object);

		/**
		 * A transcribed object type has delegated transcribing to another object type.
		 *
		 * This method enables object types to be interchangeable since they are transcription
		 * compatible with each other without breaking backward/forward compatibility.
		 */
		template <typename ObjectType>
		void
		save_delegate(
				const ObjectType &object);

		/**
		 * A transcribed object type has delegated transcribing to another object type.
		 *
		 * This method enables object types to be interchangeable since they are transcription
		 * compatible with each other without breaking backward/forward compatibility.
		 */
		template <typename ObjectType>
		LoadRef<ObjectType>
		load_delegate(
				const GPlatesUtils::CallStack::Trace &transcribe_source);


		/**
		 * Delegates to @a transcribe_delegate_object.
		 *
		 * We don't have to worry about const-casting pointers and native arrays.
		 */
		template <typename ObjectType>
		bool
		transcribe_delegate_const_cast(
				ObjectType &object)
		{
			return transcribe_delegate_object(object);
		}

		/**
		 * Delegates to @a transcribe_delegate_object.
		 *
		 * We don't have to worry about const-casting pointers and native arrays.
		 */
		template <typename ObjectType>
		bool
		transcribe_delegate_const_cast(
				const ObjectType &object)
		{
			return transcribe_delegate_object(const_cast<ObjectType &>(object));
		}


		/**
		 * A transcribed object type has delegated transcribing to another object type.
		 */
		template <typename ObjectType>
		bool
		transcribe_delegate_object(
				ObjectType &object);


		/**
		 * Delegates to @a transcribe_delegate_construct_object.
		 *
		 * We don't have to worry about const-casting pointers and native arrays.
		 */
		template <typename ObjectType>
		bool
		transcribe_delegate_construct_const_cast(
				ConstructObject<ObjectType> &construct_object)
		{
			return transcribe_delegate_construct_object(construct_object);
		}

		/**
		 * Delegates to @a transcribe_delegate_construct_object.
		 *
		 * We don't have to worry about const-casting pointers and native arrays.
		 */
		template <typename ObjectType>
		bool
		transcribe_delegate_construct_const_cast(
				ConstructObject<const ObjectType> &construct_object)
		{
			return transcribe_delegate_construct_object(
					reinterpret_cast<ConstructObject<ObjectType> &>(construct_object));
		}


		/**
		 * A transcribed object type has delegated transcribing to another object type.
		 */
		template <typename ObjectType>
		bool
		transcribe_delegate_construct_object(
				ConstructObject<ObjectType> &construct_object);


		/**
		 * Delegates to @a transcribe_base_object.
		 *
		 * We don't have to worry about const-casting pointers, etc, because BaseType and DerivedType
		 * are always classes (ie, not pointers).
		 */
		template <class BaseType, class DerivedType>
		bool
		transcribe_base_const_cast(
				DerivedType &derived_object,
				const ObjectTag &base_object_tag)
		{
			return transcribe_base_object<
					// Remove 'const' from 'BaseType' (if needed)...
					typename boost::remove_const<BaseType>::type>(
							derived_object,
							base_object_tag);
		}

		/**
		 * Delegates to @a transcribe_base_object.
		 *
		 * We don't have to worry about const-casting pointers, etc, because BaseType and DerivedType
		 * are always classes (ie, not pointers).
		 */
		template <class BaseType, class DerivedType>
		bool
		transcribe_base_const_cast(
				const DerivedType &derived_object,
				const ObjectTag &base_object_tag)
		{
			return transcribe_base_object<
					// Remove 'const' from 'BaseType' (if needed)...
					typename boost::remove_const<BaseType>::type>(
							const_cast<DerivedType &>(derived_object),
							base_object_tag);
		}


		/**
		 * Delegates to @a transcribe_base_object.
		 *
		 * This overload just registers the base-derived inheritance.
		 * It doesn't also transcribe base sub-object.
		 *
		 * We don't have to worry about const-casting pointers, etc, because BaseType and DerivedType
		 * are always classes (ie, not pointers).
		 */
		template <class BaseType, class DerivedType>
		bool
		transcribe_base_const_cast()
		{
			return transcribe_base_object<
					// Remove 'const' from 'BaseType' and 'DerivedType' (if needed)...
					typename boost::remove_const<BaseType>::type,
					typename boost::remove_const<DerivedType>::type>();
		}


		/**
		 * Transcribe a *non-pointer* object.
		 */
		template <typename ObjectType>
		bool
		transcribe_object(
				ObjectType &object,
				const ObjectTag &object_tag,
				unsigned int options);

		/**
		 * Transcribe a *non-pointer* object.
		 *
		 * An overload of @a transcribe_object used when object id has already been transcribed.
		 */
		template <typename ObjectType>
		bool
		transcribe_object(
				ObjectType &object,
				object_id_type object_id,
				unsigned int options);

		/**
		 * Transcribe a *non-pointer* ConstructObject object wrapper.
		 */
		template <typename ObjectType>
		bool
		transcribe_construct_object(
				ConstructObject<ObjectType> &construct_object,
				const ObjectTag &object_tag,
				unsigned int options);

		/**
		 * Transcribe a *non-pointer* ConstructObject object wrapper.
		 *
		 * An overload of @a transcribe_construct_object used when object id has already been transcribed.
		 */
		template <typename ObjectType>
		bool
		transcribe_construct_object(
				ConstructObject<ObjectType> &construct_object,
				object_id_type object_id,
				unsigned int options);

		/**
		 * Transcribe a *pointer* object and possibly transcribe the pointed-to object depending
		 * on ownership options.
		 */
		template <typename ObjectType>
		bool
		transcribe_object(
				ObjectType *&object_ptr,
				const ObjectTag &object_tag,
				unsigned int options);

		/**
		 * Transcribe a *pointer* object and possibly transcribe the pointed-to object depending
		 * on ownership options.
		 *
		 * An overload of @a transcribe_object used when object id has already been transcribed.
		 */
		template <typename ObjectType>
		bool
		transcribe_object(
				ObjectType *&object_ptr,
				const object_id_type object_id,
				unsigned int options);

		/**
		 * Transcribe a *pointer* object, in ConstructObject wrapper, and possibly transcribe the
		 * pointed-to object depending on ownership options.
		 */
		template <typename ObjectType>
		bool
		transcribe_construct_object(
				ConstructObject<ObjectType *> &construct_object_ptr,
				const ObjectTag &object_tag,
				unsigned int options);

		/**
		 * Transcribe a *pointer* object, in ConstructObject wrapper, and possibly transcribe the
		 * pointed-to object depending on ownership options.
		 *
		 * An overload of @a transcribe_construct_object used when object id has already been transcribed.
		 */
		template <typename ObjectType>
		bool
		transcribe_construct_object(
				ConstructObject<ObjectType *> &construct_object_ptr,
				object_id_type object_id,
				unsigned int options);


		/**
		 * Transcribe a pointer-owned object according to the smart pointer protocol.
		 */
		template <typename ObjectType>
		bool
		transcribe_smart_pointer_object(
				ObjectType *&object_ptr,
				bool shared_ownership);


		/**
		 * Transcribe the object owned by the pointer.
		 */
		template <typename ObjectType>
		bool
		transcribe_pointer_owned_object(
				ObjectType *&object_ptr,
				bool shared_ownership,
				boost::optional<object_id_type &> return_object_id = boost::none);


		/**
		 * Setup an object prior to streaming/initialisation.
		 *
		 * The main purpose of this function is to avoid instantiating duplicate code for
		 * ObjectType and ConstructObject<ObjectType> for every ObjectType type.
		 *
		 * Note: @a object_address must be valid in both the *save* and *load* paths.
		 */
		void
		pre_transcribe(
				object_id_type object_id,
				class_id_type class_id,
				const object_address_type &object_address);

		/**
		 * Finish up after an object was streamed/initialised.
		 *
		 * The main purpose of this function is to avoid instantiating duplicate code for
		 * ObjectType and ConstructObject<ObjectType> for every ObjectType type.
		 */
		void
		post_transcribe(
				object_id_type object_id,
				unsigned int options,
				bool discard,
				bool is_object_initialised = true);


		/**
		 * Transcribe the base object part of the specified derived object.
		 *
		 * Also transcribes the BaseType/DerivedType inheritance relationship.
		 */
		template <class BaseType, class DerivedType>
		bool
		transcribe_base_object(
				DerivedType &derived_object,
				const ObjectTag &base_object_tag);

		/**
		 * Transcribe the BaseType/DerivedType inheritance relationship only.
		 */
		template <class BaseType, class DerivedType>
		bool
		transcribe_base_object();

		/**
		 * Save a *reference* to an object.
		 */
		template <typename ObjectType>
		void
		save_object_reference(
				ObjectType &object_reference,
				const ObjectTag &object_tag);

		/**
		 * Load a *reference* to an object.
		 */
		template <typename ObjectType>
		LoadRef<ObjectType>
		load_object_reference(
				const GPlatesUtils::CallStack::Trace &transcribe_source,
				const ObjectTag &object_tag);

		/**
		 * A previously transcribed (loaded) object has been moved to a new memory location.
		 */
		template <typename ObjectType>
		void
		relocated_transcribed_object(
				ObjectType &relocated_object,
				ObjectType &transcribed_object);

		/**
		 * The non-template implementation of @a relocated_object.
		 */
		void
		relocated_address(
				object_id_type transcribed_object_id,
				const object_address_type &transcribed_object_address,
				const object_address_type &relocated_object_address,
				std::size_t relocation_pointer_offset,
				bool is_relocation_pointer_offset_positive);

		/**
		 * Determines if the specified object has been transcribed (client has called transcribe() on it).
		 */
		template <typename ObjectType>
		bool
		has_object_been_transcribed(
				ObjectType &object);

		/**
		 * Untrack a tracked object.
		 */
		template <typename ObjectType>
		void
		untrack_object(
				ObjectType &object,
				bool discard);

		/**
		 * Obtain and transcribe the object id for the specified object address.
		 *
		 * Note: On the *save* path @a save_object_address is always non-null except when
		 * transcribing a null pointer.
		 * On the *load* path @a save_object_address is always ignored.
		 *
		 * Returns false if could not find 'object_tag' (in the load path) within
		 * parent object scope in the archive.
		 *
		 * The save path never returns false.
		 */
		bool
		transcribe_object_id(
				const object_address_type &save_object_address,
				const ObjectTag &object_tag,
				boost::optional<object_id_type &> return_object_id = boost::none);

		/**
		 * Obtain and transcribe the class name for the specified class type info.
		 *
		 * Set @a save_class_type_info to NULL for the load path.
		 *
		 * The load path returns false for the following cases:
		 *   1) TRANSCRIBE_UNKNOWN_TYPE: The actual pointed-to type is unknown (does not match
		 *      anything we've export registered) which means either:
		 *      * the archive was created by a future GPlates with a class name we don't know about, or
		 *      * the archive was created by an old GPlates with a class name we have since removed.
		 *   2) TRANSCRIBE_INCOMPATIBLE: If the transcription did not record the derived object type.
		 *      This happens if 'ObjectType' was a non-polymorphic concrete class in the save path,
		 *      hence we don't know the type of actual object.
		 *
		 * The save path never returns false.
		 */
		bool
		transcribe_class_name(
				const std::type_info *save_class_type_info,
				boost::optional<
						boost::optional<const ExportClassType &> &> return_export_class_type = boost::none);

		/**
		 * Obtain and transcribe the class name for the object pointed to by @a object_ptr if
		 * 'ObjectType' is polymorphic.
		 *
		 * If @a owns is specified then also returns a @a TranscribeOwningPointer that can be used
		 * to transcribe the pointed-to object.
		 *
		 * Note that @a object_ptr is ignored for the load path.
		 *
		 * The load path returns false if 'ObjectType' is polymorphic *and*:
		 *   1) TRANSCRIBE_UNKNOWN_TYPE: The actual pointed-to type is unknown (does not match
		 *      anything we've export registered) which means either:
		 *      * the archive was created by a future GPlates with a class name we don't know about, or
		 *      * the archive was created by an old GPlates with a class name we have since removed.
		 *      This is regardless of whether @a owns was specified or not.
		 *   2) TRANSCRIBE_INCOMPATIBLE: If @a owns was specified and transcription did not record
		 *      the derived object type and 'ObjectType' has not been export registered
		 *      (eg, is an abstract class, or has no 'transcribe()' specialisation/overload because
		 *      it's an empty base class). This happens if 'ObjectType' was a non-polymorphic
		 *      concrete class in the save path, hence we don't know the type of actual object,
		 *      and we can't use 'ObjectType'.
		 *
		 * The save path never returns false.
		 */
		template <typename ObjectType>
		bool
		transcribe_pointed_to_class_name_if_polymorphic(
				ObjectType *object_ptr,
				boost::optional<
					boost::optional<InternalUtils::TranscribeOwningPointer::non_null_ptr_to_const_type> &>
							owns = boost::none);

		template <typename ObjectType>
		bool
		transcribe_pointed_to_class_name_if_polymorphic(
				ObjectType *object_ptr,
				boost::optional<
					boost::optional<InternalUtils::TranscribeOwningPointer::non_null_ptr_to_const_type> &> owns,
				boost::mpl::true_/*'ObjectType' is polymorphic*/);

		template <typename ObjectType>
		bool
		transcribe_pointed_to_class_name_if_polymorphic(
				ObjectType *object_ptr,
				boost::optional<
					boost::optional<InternalUtils::TranscribeOwningPointer::non_null_ptr_to_const_type> &> owns,
				boost::mpl::false_/*'ObjectType' is *not* polymorphic*/);

		/**
		 * Set the current transcribe result.
		 *
		 * Also keeps track of the current stack trace if the result is not TRANSCRIBE_SUCCESS.
		 */
		void
		set_transcribe_result(
				const GPlatesUtils::CallStack::Trace &transcribe_source, // Use 'TRANSCRIBE_SOURCE' here
				TranscribeResult transcribe_result);

		/**
		 * Registers the object type 'ObjectType' by creating a new class id for it (if necessary)
		 * and initialising the associated class info.
		 */
		template <typename ObjectType>
		class_id_type
		register_object_type();
	
		//! Initialise ClassInfo data members only if 'ObjectType' is instantiable.
		template <typename ObjectType>
		void
		register_instantiable_class_info(
				ClassInfo &class_info,
				boost::mpl::true_/*'ObjectType' is instantiable*/);

		//! Initialise ClassInfo data members only if 'ObjectType' is *not* instantiable.
		template <typename ObjectType>
		void
		register_instantiable_class_info(
				ClassInfo &class_info,
				boost::mpl::false_/*'ObjectType' is not instantiable*/);

		/**
		 * Gets the object id associated with the specified tracked object address.
		 *
		 * If the tracked object address is not found then a new object id/info is created and
		 * associated with the specified tracked object address (address is mapped to id).
		 *
		 * Note: This should only be used on the *save* path.
		 */
		object_id_type
		get_or_create_save_object_id_and_map_tracked_object_address(
				const object_address_type &object_address);

		/**
		 * If the object id is not found then a new object info is created and associated with it.
		 *
		 * Note: This should only be used on the *load* path.
		 */
		void
		get_or_create_load_object_info(
				object_id_type object_id);

		/**
		 * Map the tracked load object address with the specified object id.
		 *
		 * Note: This should only be used on the *load* path.
		 */
		void
		map_tracked_load_object_address_to_object_id(
				const object_address_type &object_address,
				object_id_type object_id);

		/**
		 * Unmap tracked object address associated with the specified object id and unmap all
		 * child-object addresses recursively.
		 */
		void
		unmap_tracked_object_address_to_object_id(
				object_id_type object_id,
				bool discard);

		/**
		 * Returns the ObjectInfo associated with the specified object id.
		 */
		ObjectInfo &
		get_object_info(
				object_id_type object_id);

		/**
		 * Returns the object address of the specified object.
		 *
		 * The object is expected to have its address and its object type initialised.
		 */
		object_address_type
		get_object_address(
				object_id_type object_id);

		/**
		 * Returns the object address of the specified object (if any).
		 */
		boost::optional<object_address_type>
		find_object_address(
				object_id_type object_id);

		/**
		 * Returns the object id of the object at the specified object address.
		 *
		 * The object is expected to have its address initialised.
		 */
		object_id_type
		get_object_id(
				const object_address_type &object_address);

		/**
		 * Returns the object id of the object at the specified object address (if any).
		 */
		boost::optional<object_id_type>
		find_object_id(
				const object_address_type &object_address);

		/**
		 * Starting transcribing a new object.
		 */
		void
		push_transcribed_object(
				object_id_type transcribed_object_id);

		/**
		 * Finished transcribing the current object.
		 */
		void
		pop_transcribed_object(
				object_id_type transcribed_object_id);

		/**
		 * Returns the object currently being transcribed (or boost::none if none).
		 *
		 * This is controlled via @a push_transcribed_object and @a pop_transcribed_object.
		 */
		boost::optional<ObjectInfo &>
		get_current_transcribed_object();

		/**
		 * Returns true if the address of the specified child object is contained inline within
		 * its parent object (specified as @a parent_object_id).
		 */
		bool
		is_child_object_inside_parent_object_memory(
				object_id_type child_object_id,
				object_id_type parent_object_id);

		/**
		 * Adds the specified child object as a sub-object of its parent if it lies *inside*
		 * the memory area of its parent.
		 */
		void
		add_child_as_sub_object_if_inside_parent(
				object_id_type child_object_id);

		/**
		 * Removes the specified child object as a sub-object of its parent if it lies *outside*
		 * the memory area of its parent.
		 */
		void
		remove_child_as_sub_object_if_outside_parent(
				object_id_type child_object_id);

		/**
		 * Adds, or removes, the specified relocated object as a sub-object of its parent if it lies
		 * inside, or outside, the memory area of its parent (if it's not already the case).
		 */
		void
		add_or_remove_relocated_child_as_sub_object_if_inside_or_outside_parent(
				object_id_type relocated_object_id);

		/**
		 * Removes the specified parent object from its child objects.
		 */
		void
		remove_parent_object_from_children(
				object_id_type parent_object_id);

		/**
		 * Adds the specified child object to its parent.
		 */
		void
		add_child_object_to_parent(
				object_id_type child_object_id);

		/**
		 * Removes the specified child object from its parent's child/sub/base lists (if has a parent).
		 */
		void
		remove_child_object_from_parent(
				object_id_type child_object_id);

		/**
		 * Add a pointer to the list of pointers that reference a pointed-to object.
		 */
		void
		add_pointer_referencing_object(
				object_id_type object_id,
				object_id_type pointer_object_id);

		/**
		 * Remove a pointer from the list of pointers that reference a pointed-to object.
		 */
		void
		remove_pointer_referencing_object(
				object_id_type pointer_object_id);


		/**
		 * Sets all pointers (referencing the specified object) to point to the object's address.
		 *
		 * Each pointer is either:
		 *  (1) Unresolved: does not yet point to an object and will get initialised here, or
		 *  (2) Resolved: already points to an object and will point to a new object address here.
		 * Case (2) happens when a transcribed object is relocated (all pointers to it must adjust).
		 *
		 * Note that the referenced object itself could be a pointer.
		 *
		 * NOTE: When *saving* to an archive this just records the pointers as initialised.
		 */
		void
		resolve_pointers_referencing_object(
				object_id_type object_id);

		/**
		 * Set the pointer to point to the object (in the load path).
		 *
		 * Does any pointer fix ups in the presence of multiple inheritance.
		 * It's possible that the pointer  (identified by @a pointer_object_id) refers to a
		 * base class of a multiply-inherited derived class object (identified by @a object_id)
		 * and there can be pointer offsets.
		 *
		 * NOTE: When *saving* to an archive this just records the pointer as initialised.
		 */
		void
		resolve_pointer_reference_to_object(
				object_id_type object_id,
				object_id_type pointer_object_id);

		/**
		 * Sets all pointers (referencing the specified object) to NULL.
		 *
		 * NOTE: This only happens on the *load* path since pointed-to objects are only discarded
		 * on the *load* path (due to transcription incompatibility).
		 */
		void
		unresolve_pointers_referencing_object(
				object_id_type object_id);

		/**
		 * Sets pointer (referencing the specified object) to NULL.
		 */
		void
		unresolve_pointer_reference_to_object(
				object_id_type pointer_object_id);

		/**
		 * Set the pointer to point to the object (in the load path).
		 *
		 * Does any pointer fix ups in the presence of multiple inheritance.
		 * It's possible that the pointer refers to a base class of a multiply-inherited
		 * derived class object (identified by @a object_id with address @a object_address)
		 * and there can be pointer offsets.
		 *
		 * Return false if a pointer fix-up failed because the actual referenced object type does
		 * not inherit directly or indirectly from 'ObjectType' and so we can't legally reference it.
		 * This can happen when the actual object is created dynamically (via a base class pointer)
		 * and when it was saved on another system.
		 *
		 * @a object_address should not be NULL, but @a object_ptr can be un-initialised (on calling).
		 *
		 * NOTE: This should only be used on the *load* path.
		 */
		template <typename ObjectType>
		bool
		set_pointer_to_object(
				object_id_type object_id,
				void *object_address,
				ObjectType *&object_ptr);


		/**
		 * Gets, or creates, the class id associated with the specified class type.
		 *
		 * If the class type is not found then a new class id/info is created and
		 * associated with the specified class type.
		 */
		class_id_type
		get_or_create_class_id(
				const std::type_info &class_type);

		/**
		 * Creates a new ClassInfo using the next available class id and returns that id.
		 */
		class_id_type
		create_new_class_info();


		/**
		 * Returns the ClassInfo associated with the specified class id.
		 */
		ClassInfo &
		get_class_info(
				class_id_type class_id);

		/**
		 * Returns the ClassInfo associated with the specified *object* id.
		 */
		ClassInfo &
		get_class_info_from_object(
				object_id_type object_id);

		/**
		 * Returns the transcribe context stack associated with the specified class type, or
		 * boost::none if a ClassInfo has not already been created for the specified class type
		 * (eg, by object type registration or by pushing a transcribe context).
		 */
		boost::optional<transcribe_context_stack_type &>
		get_transcribe_context_stack(
				const std::type_info &class_type_info);

		/**
		 * Save/load construct and transcribe an object.
		 */
		template <typename ObjectType>
		bool
		stream_construct_object(
				ConstructObject<ObjectType> &construct_object);

		/**
		 * Transcribe an object (with no save/load construction).
		 */
		template <typename ObjectType>
		bool
		stream_object(
				ObjectType &object);

		struct StreamPrimitiveTag { };  // Tag to stream primitives.
		struct StreamTranscribeTag { }; // Catch-all tag to stream general objects using 'transcribe()'.

		template <typename ObjectType>
		bool
		stream(
				ObjectType &object,
				bool transcribed_construct_data);

		//! Stream primitives directly to archive.
		template <typename ObjectType>
		bool
		stream(
				ObjectType &object,
				bool transcribed_construct_data,
				StreamPrimitiveTag);

		//! Catch-all stream object using 'transcribe()' specialisation or overload.
		template <typename ObjectType>
		bool
		stream(
				ObjectType &object,
				bool transcribed_construct_data,
				StreamTranscribeTag);


		//! Helper function for transcribing boost::shared_ptr.
		template <typename T>
		void
		reset(
				boost::shared_ptr<T> &shared_ptr_object,
				T *raw_ptr)
		{
			reset_impl(shared_ptr_object, raw_ptr);
		}

		//! Const overload for helper function for transcribing boost::shared_ptr.
		template <typename T>
		void
		reset(
				boost::shared_ptr<const T> &shared_ptr_object,
				const T *raw_ptr)
		{
			// The raw pointer needs to be 'non-const' as do all object addresses.
			reset_impl(shared_ptr_object, const_cast<T *>(raw_ptr));
		}

		//! Helper function for transcribing boost::shared_ptr.
		template <typename T, typename NonConstT>
		void
		reset_impl(
				boost::shared_ptr<T> &shared_ptr_object,
				NonConstT *raw_ptr);


		//! Typedef for a map of shared pointers searched by the pointed-to object address.
		typedef std::map<
				object_address_type,
				boost::shared_ptr<void>,
				InternalUtils::SortObjectAddressPredicate>
						shared_ptr_map_type;

		shared_ptr_map_type d_shared_ptr_map;


		// Give friend access class ScribeInternalAccess in order to limit the access to our internals.
		// Other classes will be friend of it and use its limited access to us.
		friend class ScribeInternalAccess;
	};
}


//
// Implementation
//


// Placing headers here avoids cyclic header dependency...

#include "ScribeExportRegistry.h"
#include "ScribeInternalUtilsImpl.h"
#include "ScribeLoadRefImpl.h"

// Include the default generic 'transcribe()' implementation.
#include "TranscribeImpl.h"

// Include 'transcribe() specialisation/overloads for handling native C++ arrays.
#include "TranscribeArray.h"

// Include 'transcribe()' specialisations and overloads for external libraries.
// This has to be included explicitly because external library header files do not contain these
// overloads (because we can't, of course, edit those files).
// This is not necessary for GPlates classes (ie, non-external source files) because header files
// for those classes contain the necessary specialisations/overloads/methods for 'transcribe()'.
#include "TranscribeExternal.h"


namespace GPlatesScribe
{
	template <typename ObjectType>
	Scribe::Bool
	Scribe::transcribe(
			const GPlatesUtils::CallStack::Trace &transcribe_source,
			ObjectType &object,
			const ObjectTag &object_tag,
			unsigned int options)
	{
		// Track the file/line of the call site for exception messages.
		GPlatesUtils::CallStackTracker call_stack_tracker(transcribe_source);

		// Throw exception if the object is not the type we expect it to be (it should be type 'ObjectType').
		//
		// If this assertion is triggered then it means:
		//   * A Scribe client has called 'transcribe' on an object *reference* instead of an object.
		//
		// The following example can trigger this assertion:
		//
		//   class A { public: virtual ~A() { } };
		//   class B : public A { };
		//   B b;
		//   A &a = b;
		//   if (!scribe.transcribe(TRANSCRIBE_SOURCE, a, "a", GPlatesScribe::TRACK)) // Error: typeid(A) != typeid(a)
		//   {
		//		return scribe.get_transcribe_result();
		//   }
		//
		// ...which can be solved by...
		//
		//   B b;
		//   if (!scribe.transcribe(TRANSCRIBE_SOURCE, b, "b", GPlatesScribe::TRACK)) // This is fine since typeid(B) == typeid(b)
		//   {
		//		return scribe.get_transcribe_result();
		//   }
		//   A &a = b;
		//
		// Note that this detection only works for polymorphic types.
		// For example, if class A had no virtual methods then this assertion would not trigger and
		// we would transcribe a sliced object (transcribe only the A part of B).
		//
		// Non-polymorphic types include pointers but these are fine since the pointer itself
		// and the pointer type (ObjectType) will have the same typeid.
		//
		GPlatesGlobal::Assert<Exceptions::TranscribedReferenceInsteadOfObject>(
				typeid(ObjectType) == typeid(object),
				GPLATES_ASSERTION_SOURCE,
				object);

		// Wrap in a Bool object to force caller to check return code.
		return Bool(
				transcribe_source,
				transcribe_const_cast(object, object_tag, options),
				is_loading()/*require_check*/);
	}


	template <class BaseType, class DerivedType>
	Scribe::Bool
	Scribe::transcribe_base(
			const GPlatesUtils::CallStack::Trace &transcribe_source,
			DerivedType &derived_object,
			const ObjectTag &base_object_tag)
	{
		// Track the file/line of the call site for exception messages.
		GPlatesUtils::CallStackTracker call_stack_tracker(transcribe_source);

		// Wrap in a Bool object to force caller to check return code.
		return Bool(
				transcribe_source,
				transcribe_base_const_cast<BaseType>(derived_object, base_object_tag),
				is_loading()/*require_check*/);
	}


	template <class BaseType, class DerivedType>
	Scribe::Bool
	Scribe::transcribe_base(
			const GPlatesUtils::CallStack::Trace &transcribe_source)
	{
		// Wrap in a Bool object to force caller to check return code.
		return Bool(
				transcribe_source,
				transcribe_base_const_cast<BaseType, DerivedType>(),
				is_loading()/*require_check*/);
	}


	template <typename ObjectType>
	void
	Scribe::save(
			const GPlatesUtils::CallStack::Trace &transcribe_source,
			const ObjectType &object,
			const ObjectTag &object_tag,
			unsigned int options)
	{
		// Compile-time assertion to ensure no native arrays use 'save()' (must use 'transcribe()' instead).
		//
		// If this assertion is triggered then it means:
		//   * 'save()' has been called for a native array (instead of 'transcribe()').
		//
		// See "TranscribeArray.h" for more details.
		//
		BOOST_STATIC_ASSERT(boost::mpl::not_< boost::is_array<ObjectType> >::value);

		// Track the file/line of the call site for exception messages.
		GPlatesUtils::CallStackTracker call_stack_tracker(transcribe_source);

		// Throw exception if the object is not the type we expect it to be (it should be type 'ObjectType').
		//
		// If this assertion is triggered then it means:
		//   * A Scribe client has called 'transcribe' on an object *reference* instead of an object.
		//
		// The following example can trigger this assertion:
		//
		//   class A { public: virtual ~A() { } };
		//   class B : public A { };
		//   B b;
		//   A &a = b;
		//   scribe.transcribe(TRANSCRIBE_SOURCE, a, "a", GPlatesScribe::TRACK); // Error: typeid(A) != typeid(a)
		//
		// ...which can be solved by...
		//
		//   B b;
		//   scribe.transcribe(TRANSCRIBE_SOURCE, b, "b", GPlatesScribe::TRACK); // This is fine since typeid(B) == typeid(b)
		//   A &a = b;
		//
		// Note that this detection only works for polymorphic types.
		// For example, if class A had no virtual methods then this assertion would not trigger and
		// we would transcribe a sliced object (transcribe only the A part of B).
		//
		// Non-polymorphic types include pointers but these are fine since the pointer itself
		// and the pointer type (ObjectType) will have the same typeid.
		//
		GPlatesGlobal::Assert<Exceptions::TranscribedReferenceInsteadOfObject>(
				typeid(ObjectType) == typeid(object),
				GPLATES_ASSERTION_SOURCE,
				object);

		// Mirror the load path.
		SaveConstructObject<ObjectType> save_construct_object(object);
		// We're on the *save* path so no need to check return value.
		transcribe_construct(save_construct_object, object_tag, options);
	}


	template <typename ObjectType>
	LoadRef<ObjectType>
	Scribe::load(
			const GPlatesUtils::CallStack::Trace &transcribe_source,
			const ObjectTag &object_tag,
			unsigned int options)
	{
		// Compile-time assertion to ensure no native arrays use 'load()' (must use 'transcribe()' instead).
		//
		// If this assertion is triggered then it means:
		//   * 'load()' has been called for a native array (instead of 'transcribe()').
		//
		// See "TranscribeArray.h" for more details.
		//
		BOOST_STATIC_ASSERT(boost::mpl::not_< boost::is_array<ObjectType> >::value);

		// Track the file/line of the call site for exception messages.
		GPlatesUtils::CallStackTracker call_stack_tracker(transcribe_source);

		LoadConstructObjectOnHeap<ObjectType> load_construct_object;
		if (!transcribe_construct(load_construct_object, object_tag, options))
		{
			// Heap-allocated object destructed/deallocated by construct object 'load_construct_object' on returning...
			return LoadRef<ObjectType>();
		}

		// Object now owned by LoadRef (released from construct object)...
		return LoadRef<ObjectType>(
				transcribe_source,
				*this,
				load_construct_object.release(),
				// Transferring ownership...
				true/*release*/);
	}


	template <typename ObjectType>
	void
	Scribe::save_reference(
			const GPlatesUtils::CallStack::Trace &transcribe_source,
			ObjectType &object_reference,
			const ObjectTag &object_tag)
	{
		// Track the file/line of the call site for exception messages.
		GPlatesUtils::CallStackTracker call_stack_tracker(transcribe_source);

		save_reference_const_cast(object_reference, object_tag);
	}


	template <typename ObjectType>
	LoadRef<ObjectType>
	Scribe::load_reference(
			const GPlatesUtils::CallStack::Trace &transcribe_source,
			const ObjectTag &object_tag)
	{
		// Track the file/line of the call site for exception messages.
		GPlatesUtils::CallStackTracker call_stack_tracker(transcribe_source);

		return load_object_reference<ObjectType>(transcribe_source, object_tag);
	}


	template <typename ObjectFirstQualifiedType, typename ObjectSecondQualifiedType>
	void
	Scribe::relocated(
			const GPlatesUtils::CallStack::Trace &transcribe_source,
			ObjectFirstQualifiedType &relocated_object,
			ObjectSecondQualifiedType &transcribed_object)
	{
		// Track the file/line of the call site for exception messages.
		GPlatesUtils::CallStackTracker call_stack_tracker(transcribe_source);

		// Compile-time assertion to ensure the two non-const-qualified object types are the same.
		//
		// NOTE: This only removes the *top-level* const qualifier - the rest must match.
		// So for multi-level pointers only the top-level const can be different. For example:
		//
		//   const int **const*
		//   const int **const*const
		//
		// ...are fine, but...
		// 
		//   const int *const**
		//   const int **const*
		//
		// ...will trigger the following compile-time assertion.
		BOOST_STATIC_ASSERT((boost::is_same<
				typename boost::remove_const<ObjectFirstQualifiedType>::type,
				typename boost::remove_const<ObjectSecondQualifiedType>::type>::value));

		relocated_const_cast(relocated_object, transcribed_object);
	}


	template <typename ObjectType>
	bool
	Scribe::has_been_transcribed(
			ObjectType &object)
	{
		return has_been_transcribed_const_cast(object);
	}


	template <typename ObjectType>
	bool
	Scribe::has_object_been_transcribed(
			ObjectType &object)
	{
		// Compile-time assertion to ensure that 'ObjectType' is not const.
		BOOST_STATIC_ASSERT(boost::mpl::not_< boost::is_const<ObjectType> >::value);

		// If:
		//   1) the object address is associated with an object id, and
		//   2) the object has been pre-initialised
		// ...then the client has explicitly called transcribe(), or its equivalents, on the object
		// with tracking enabled.

		// We need the *dynamic* object address since we want the full dynamic object instead
		// of a (potential) base class sub-object (referenced by 'ObjectType' reference).
		const object_address_type object_address =
				InternalUtils::get_dynamic_object_address(&object);

		boost::optional<object_id_type> object_id  = find_object_id(object_address);
		if (!object_id)
		{
			return false;
		}

		return get_object_info(object_id.get()).is_object_pre_initialised;
	}


	template <typename ObjectType>
	void
	Scribe::push_transcribe_context(
			TranscribeContext<ObjectType> &transcribe_context)
	{
		// Create a new class id if the 'ObjectType' has not been seen before.
		// It's likely we are pushing a transcribe context for an object type that has not yet
		// been transcribed (and hence not yet registered), or it could be any object type that
		// will never get transcribed.
		//
		// Note that we don't register the object type (with 'register_object_type<>()')
		// because we only need a place to store the transcribe context (stack in ClassInfo)
		// and registering the object type might generate a compile-time error if 'ObjectType'
		// does not have a 'transcribe()' specialisation/overload (eg, because it's an empty
		// base class with nothing to transcribe).
		// And we want this to work with non-transcribed, non-registered object types.
		const class_id_type class_id =
				get_or_create_class_id(
						// Remove 'const' from the object type (if needed)...
						typeid(typename boost::remove_const<ObjectType>::type));

		// Get the class info.
		ClassInfo &class_info = get_class_info(class_id);

		// Add the transcribe context to the class type's stack.
		// We don't use 'dynamic_cast<void *>' because we always cast between 'void' and
		// 'TranscribeContext<ObjectType>' and hence it is not necessary - also we don't know
		// if TranscribeContext has been specialised (for 'ObjectType') as a polymorphic class or not.
		class_info.transcribe_context_stack.push(static_cast<void *>(&transcribe_context));
	}


	template <typename ObjectType>
	boost::optional<TranscribeContext<ObjectType> &>
	Scribe::get_transcribe_context()
	{
		// Remove 'const' from the object type (if needed).
		typedef typename boost::remove_const<ObjectType>::type non_const_object_type;

		const std::type_info &class_type_info = typeid(non_const_object_type);

		// Get the transcribe context stack unless class type has not been registered or
		// its associated transcribe context hasn't never pushed yet.
		boost::optional<transcribe_context_stack_type &> transcribe_context_stack =
				get_transcribe_context_stack(class_type_info);
		if (!transcribe_context_stack ||
			transcribe_context_stack->empty())
		{
			return boost::none;
		}

		// We don't use 'dynamic_cast<void *>' because we always cast between 'void' and
		// 'TranscribeContext<ObjectType>' and hence it is not necessary.
		TranscribeContext<ObjectType> *transcribe_context =
				static_cast<TranscribeContext<ObjectType> *>(
						transcribe_context_stack->top());

		return *transcribe_context;
	}


	template <typename ObjectType>
	void
	Scribe::pop_transcribe_context()
	{
		// Remove 'const' from the object type (if needed).
		typedef typename boost::remove_const<ObjectType>::type non_const_object_type;

		const std::type_info &class_type_info = typeid(non_const_object_type);

		// Get the transcribe context stack. This should not fail unless the
		// transcribe context (associated with class type) has never been pushed.
		boost::optional<transcribe_context_stack_type &> transcribe_context_stack =
				get_transcribe_context_stack(class_type_info);

		GPlatesGlobal::Assert<Exceptions::ScribeUserError>(
				transcribe_context_stack &&
					!transcribe_context_stack->empty(),
				GPLATES_ASSERTION_SOURCE,
				std::string("No transcribe context available for the object type '") +
						class_type_info.name() +
						"'.");

		// Pop the transcribe context off the stack.
		transcribe_context_stack->pop();
	}


	template <typename ObjectType>
	bool
	Scribe::transcribe_delegate(
			ObjectType &object)
	{
		// Compile-time assertion to ensure no pointers or native arrays are transcribed.
		//
		// If this assertion is triggered then it means:
		//   * 'transcribe_delegate_protocol()' has been called on a pointer or a native array.
		//
		BOOST_STATIC_ASSERT(boost::mpl::not_< boost::is_pointer<ObjectType> >::value);
		BOOST_STATIC_ASSERT(boost::mpl::not_< boost::is_array<ObjectType> >::value);

		// Throw exception if the object is not the type we expect it to be (it should be type 'ObjectType').
		//
		// If this assertion is triggered then it means:
		//   * A Scribe client has called 'transcribe_delegate_protocol' on an object *reference* instead of an object.
		//
		// Note that this detection only works for polymorphic types.
		//
		GPlatesGlobal::Assert<Exceptions::TranscribedReferenceInsteadOfObject>(
				typeid(ObjectType) == typeid(object),
				GPLATES_ASSERTION_SOURCE,
				object);

		return transcribe_delegate_const_cast(object);
	}


	template <typename ObjectType>
	void
	Scribe::save_delegate(
			const ObjectType &object)
	{
		// Compile-time assertion to ensure no pointers or native arrays are transcribed.
		//
		// If this assertion is triggered then it means:
		//   * 'save_delegate_protocol()' has been called for a pointer or a native array.
		//
		BOOST_STATIC_ASSERT(boost::mpl::not_< boost::is_pointer<ObjectType> >::value);
		BOOST_STATIC_ASSERT(boost::mpl::not_< boost::is_array<ObjectType> >::value);

		// Throw exception if the object is not the type we expect it to be (it should be type 'ObjectType').
		//
		// If this assertion is triggered then it means:
		//   * A Scribe client has called 'save_delegate_protocol' on an object *reference* instead of an object.
		//
		// Note that this detection only works for polymorphic types.
		//
		GPlatesGlobal::Assert<Exceptions::TranscribedReferenceInsteadOfObject>(
				typeid(ObjectType) == typeid(object),
				GPLATES_ASSERTION_SOURCE,
				object);

		// Mirror the load path.
		SaveConstructObject<ObjectType> save_construct_object(object);
		// We're on the *save* path so no need to check return value.
		transcribe_delegate_construct_const_cast(save_construct_object);
	}


	template <typename ObjectType>
	LoadRef<ObjectType>
	Scribe::load_delegate(
			const GPlatesUtils::CallStack::Trace &transcribe_source)
	{
		// Compile-time assertion to ensure no pointers or native arrays are transcribed.
		//
		// If this assertion is triggered then it means:
		//   * 'load_delegate_protocol()' has been called for a pointer or a native array.
		//
		BOOST_STATIC_ASSERT(boost::mpl::not_< boost::is_pointer<ObjectType> >::value);
		BOOST_STATIC_ASSERT(boost::mpl::not_< boost::is_array<ObjectType> >::value);

		LoadConstructObjectOnHeap<ObjectType> load_construct_object;
		if (!transcribe_delegate_construct_const_cast(load_construct_object))
		{
			// Heap-allocated object destructed/deallocated by construct object 'load_construct_object' on returning...
			return LoadRef<ObjectType>();
		}

		// Object now owned by LoadRef (released from construct object)...
		return LoadRef<ObjectType>(
				transcribe_source,
				*this,
				load_construct_object.release(),
				// Transferring ownership...
				true/*release*/);
	}


	template <typename ObjectType>
	bool
	Scribe::transcribe_delegate_object(
			ObjectType &object)
	{
		// This streams directly to ObjectType to transcribe the object.
		return stream_object(object);
	}


	template <typename ObjectType>
	bool
	Scribe::transcribe_delegate_construct_object(
			ConstructObject<ObjectType> &construct_object)
	{
		// This streams a ConstructObject<ObjectType> to both save/load construct the object and to transcribe it.
		return stream_construct_object(construct_object);
	}


	template <typename ObjectType>
	bool
	Scribe::transcribe_object(
			ObjectType &object,
			const ObjectTag &object_tag,
			unsigned int options)
	{
		//
		// Transcribe the object id.
		//

		// Using 'static' address since we know actual type of object is 'ObjectType'.
		const object_address_type object_address = InternalUtils::get_static_object_address(&object);

		object_id_type object_id;
		if (!transcribe_object_id(object_address, object_tag, object_id))
		{
			return false;
		}

		//
		// Transcribe the object.
		//

		return transcribe_object(object, object_id, options);
	}


	template <typename ObjectType>
	bool
	Scribe::transcribe_object(
			ObjectType &object,
			object_id_type object_id,
			unsigned int options)
	{
		// Compile-time assertion to ensure that 'ObjectType' is not const.
		BOOST_STATIC_ASSERT(boost::mpl::not_< boost::is_const<ObjectType> >::value);

		// Object is not a pointer so it shouldn't have any pointer ownership options.
		GPlatesGlobal::Assert<Exceptions::InvalidTranscribeOptions>(
				!(options & (EXCLUSIVE_OWNER | SHARED_OWNER)),
				GPLATES_ASSERTION_SOURCE,
				"Pointer ownership options were specified for a non-pointer object.");

		//
		// Register the object's class type
		//

		// We don't transcribe the object type - if 'ObjectType' differs in the save and load paths
		// then loading will only succeed if they are transcription-compatible.
		const class_id_type class_id = register_object_type<ObjectType>();

		//
		// Perform operations *before* streaming object.
		//

		// Using 'static' address since we know actual type of object is 'ObjectType'.
		const object_address_type object_address = InternalUtils::get_static_object_address(&object);

		pre_transcribe(object_id, class_id, object_address);

		//
		// Transcribe object.
		//

		// This streams directly to ObjectType to transcribe the object.
		const bool streamed = stream_object(object);

		//
		// Perform operations *after* streaming object.
		//

		post_transcribe(object_id, options, !streamed/*discard*/);

		return streamed;
	}


	template <typename ObjectType>
	bool
	Scribe::transcribe_construct_object(
			ConstructObject<ObjectType> &construct_object,
			const ObjectTag &object_tag,
			unsigned int options)
	{
		//
		// Transcribe the object id.
		//

		// Using 'static' address since we know actual type of object is 'ObjectType'.
		const object_address_type object_address =
				InternalUtils::get_static_object_address(
						// In load path the object is not constructed yet so we get the
						// (initialised or uninitialised) object's address...
						construct_object.get_object_address());

		object_id_type object_id;
		if (!transcribe_object_id(object_address, object_tag, object_id))
		{
			return false;
		}

		//
		// Transcribe the object.
		//

		return transcribe_construct_object(construct_object, object_id, options);
	}


	template <typename ObjectType>
	bool
	Scribe::transcribe_construct_object(
			ConstructObject<ObjectType> &construct_object,
			object_id_type object_id,
			unsigned int options)
	{
		// Compile-time assertion to ensure that 'ObjectType' is not const.
		BOOST_STATIC_ASSERT(boost::mpl::not_< boost::is_const<ObjectType> >::value);

		// Object is not a pointer so it shouldn't have any pointer ownership options.
		GPlatesGlobal::Assert<Exceptions::InvalidTranscribeOptions>(
				!(options & (EXCLUSIVE_OWNER | SHARED_OWNER)),
				GPLATES_ASSERTION_SOURCE,
				"Pointer ownership options were specified for a non-pointer object.");

		//
		// Register the object's class type
		//

		// We don't transcribe the object type - if 'ObjectType' differs in the save and load paths
		// then loading will only succeed if they are transcription-compatible.
		const class_id_type class_id = register_object_type<ObjectType>();

		//
		// Perform operations *before* streaming object.
		//

		// Using 'static' address since we know actual type of object is 'ObjectType'.
		const object_address_type object_address =
				InternalUtils::get_static_object_address(
						// In load path the object is not constructed yet so we get the
						// (initialised or uninitialised) object's address...
						construct_object.get_object_address());

		pre_transcribe(object_id, class_id, object_address);

		//
		// Transcribe object.
		//

		// This streams a ConstructObject<ObjectType> to both save/load construct the object and to transcribe it.
		const bool streamed = stream_construct_object(construct_object);

		//
		// Perform operations *after* streaming object.
		//

		post_transcribe(object_id, options, !streamed/*discard*/);

		return streamed;
	}


	template <typename ObjectType>
	bool
	Scribe::transcribe_object(
			ObjectType *&object_ptr,
			const ObjectTag &object_tag,
			unsigned int options)
	{
		// If loading then set the pointer to NULL in case it doesn't get initialised later.
		// This can happen when the pointer does not own the pointed-to object and the pointed-to
		// object has not yet been transcribed. So in the meantime we set it to NULL in case the
		// client tries to use it.
		//
		// Also it might actually be a NULL pointer (ie, save path transcribed a NULL pointer).
		if (is_loading())
		{
			object_ptr = NULL;
		}

		//
		// Transcribe the pointer object id
		//

		// Using 'static' address since we know actual type of object is 'ObjectType *'.
		const object_address_type pointer_object_address = InternalUtils::get_static_object_address(&object_ptr);

		object_id_type pointer_object_id;
		if (!transcribe_object_id(pointer_object_address, object_tag, pointer_object_id))
		{
			return false;
		}

		//
		// Transcribe the pointer itself (including pointed-to object if it owns it).
		//

		return transcribe_object(object_ptr, pointer_object_id, options);
	}


	template <typename ObjectType>
	bool
	Scribe::transcribe_object(
			ObjectType *&object_ptr,
			object_id_type pointer_object_id,
			unsigned int options)
	{
		// Ensure maximum supported pointer dimension has not been exceeded.
		//
		// If this assertion is triggered then a multi-level pointer with dimension greater than
		// 'GPLATES_SCRIBE_MAX_POINTER_DIMENSION' is being transcribed.
		//
		BOOST_STATIC_ASSERT(boost::mpl::not_< UnsupportedPointerType<ObjectType*> >::value);

		// Compile-time assertion to ensure that 'ObjectType' is not const.
		BOOST_STATIC_ASSERT(boost::mpl::not_< boost::is_const<ObjectType> >::value);

		// Should not have both pointer ownership options specified together.
		GPlatesGlobal::Assert<Exceptions::InvalidTranscribeOptions>(
				!((options & EXCLUSIVE_OWNER) && (options & SHARED_OWNER)),
				GPLATES_ASSERTION_SOURCE,
				"Pointer ownership cannot be both shared and exclusive at the same time.");

		// If loading then set the pointer to NULL in case it doesn't get initialised later.
		// This can happen when the pointer does not own the pointed-to object and the pointed-to
		// object has not yet been transcribed. So in the meantime we set it to NULL in case the
		// client tries to use it.
		//
		// Also it might actually be a NULL pointer (ie, save path transcribed a NULL pointer).
		if (is_loading())
		{
			object_ptr = NULL;
		}

		//
		// Register the pointer's class type (not the pointed-to object's class type).
		//

		// We don't transcribe the pointer type - if 'ObjectType *' differs in the save and load paths
		// then loading will only succeed if both 'ObjectType's are transcription-compatible.
		// This includes cases where 'ObjectType' is polymorphic and the dynamic loaded object type
		// inherits from 'ObjectType' (in the load path).
		//
		// Note that the registered type is 'ObjectType *' (not 'ObjectType').
		const class_id_type pointer_class_id = register_object_type<ObjectType *>();

		//
		// A pointer object is like a regular non-pointer object so
		// perform the same pre-transcribe operations.
		//
		// The main difference compared to a non-pointer object is a pointer is not streamed,
		// instead it records the object id of the pointed-to object.
		//

		// Using 'static' address since we know actual type of object is 'ObjectType *'.
		const object_address_type pointer_object_address = InternalUtils::get_static_object_address(&object_ptr);

		pre_transcribe(pointer_object_id, pointer_class_id, pointer_object_address);

		// If we can find the pointed-to object then the pointer will be considered to be successfully streamed.
		bool pointer_streamed = false;

		// The pointer will get initialised (point to a pointed-to object) unless the pointed-to
		// object has not been transcribed (and pointer doesn't own pointed-to object).
		// A NULL pointer is considered initialised.
		bool pointer_is_initialised = false;

		//
		// Possibly transcribe the pointed-to object.
		//

		// If the pointer owns the pointed-to object then we need to transcribe the pointed-to object.
		if (options & (EXCLUSIVE_OWNER | SHARED_OWNER))
		{
			object_id_type object_id;
			if (transcribe_pointer_owned_object(
					// The pointer will be NULL in the load path before calling function...
					object_ptr,
					options & SHARED_OWNER/*shared_ownership*/,
					object_id))
			{
				pointer_streamed = true;

				// Exclude NULL pointers since there's no pointed-to object to point to...
				if (object_id == NULL_POINTER_OBJECT_ID)
				{
					// A pointer transcribed as NULL is considered to be initialised.
					pointer_is_initialised = true;
				}
				else // pointer is not NULL...
				{
					// The above 'transcribe_pointer_owned_object()' call initialised 'object_ptr' to
					// the transcribed pointed-to object.
					pointer_is_initialised = true;

					if (options & TRACK)
					{
						// Add our pointer to the list of pointers that reference the pointed-to object.
						// This is useful if the pointed-to object is later relocated - in which case the
						// pointer will get re-initialised to point to the object's relocated location.
						add_pointer_referencing_object(object_id, pointer_object_id);
					}
					else // The pointer is *not* being tracked...
					{
						// Note: We're not tracking the pointer but we also don't mark the pointed-to object as
						// referenced by an untracked pointer (as is the case with a non-owning pointer) because
						// relocating the pointed-to object means a new owning pointer is being created with a
						// new pointed-to object. In this case we don't want the original owning pointer to point
						// to the new pointed-to object (it should still point to the original pointed-to object).
						// For example when an exclusive owning pointer is copied and relocated:
						//
						//   int *x;
						//   scribe.transcribe(TRANSCRIBE_SOURCE, x, "x", GPlatesScribe::EXCLUSIVE_OWNER | GPlatesScribe::TRACK);
						//   int *y = new int(*x);  // Copy 'y' from 'x' keeping same integer value.
						//   scribe.relocated(TRANSCRIBE_SOURCE, y, x);
						//   scribe.relocated(TRANSCRIBE_SOURCE, *y, *x);
						//   delete x;
						//
						// ...where the last line 'scribe.relocated(TRANSCRIBE_SOURCE, *y, *x)' is only possible
						// if we don't mark the pointed-to object '*x' as referenced by an untracked pointer.
						//
						// And so we don't want to prevent relocation of the pointed-to object in this case.
						//
						// Essentially we can view the owning pointer and its pointed-to object as a single unit.
						// If one is relocated then so is the other. If the owning pointer is a shared owner
						// (versus exclusive) then typically the pointed-to object never needs to be relocated
						// (because when a shared pointer is copied/moved it still points to the same pointed-to
						// object address) and so this problem doesn't arise.
						//
						//
						// UPDATE: The above no longer applies because now an untracked owning pointer also
						// results in an untracked pointed-to object.
						//
					}
				}
			}
		}
		else // pointed-to object is not *owned* by the pointer...
		{
			// We don't transcribe the pointed-to object here because our pointer does not own the object.

			//
			// Link to the pointed-to object.
			//

			// We need the *dynamic* object address since we want the full dynamic object
			// instead of a (potential) base class sub-object (referenced by pointer).
			const object_address_type object_address = InternalUtils::get_dynamic_object_address(object_ptr);

			// Transcribe the pointed-to object's id.
			// Note: 'object_address' will be NULL in the load path...
			object_id_type object_id;
			if (transcribe_object_id(object_address, POINTS_TO_OBJECT_TAG, object_id))
			{
				// Exclude NULL pointers since there's no pointed-to object to point to...
				if (object_id == NULL_POINTER_OBJECT_ID)
				{
					pointer_streamed = true;
					// A pointer transcribed as NULL is considered to be initialised.
					pointer_is_initialised = true;
				}
				// Else make sure the pointed-to object type matches something we've export registered,
				// if 'ObjectType' is polymorphic. Even though we're not going to transcribe the
				// pointed-to object now we want to fail to load/stream the pointer to improve our
				// chances of backward/forward compatibility in the following cases:
				//   * the archive was created by a future GPlates with a class name we don't know about, or
				//   * the archive was created by an old GPlates with a class name we have since removed.
				else if (transcribe_pointed_to_class_name_if_polymorphic(object_ptr))
				{
					// Get the pointed-to object info.
					ObjectInfo &object_info = get_object_info(object_id);

					if (object_info.object_address)
					{
						if (is_loading())
						{
							// Initialise object pointer.
							//
							// We need to do any pointer fix ups in the presence of multiple inheritance.
							// It's possible that the pointer refers to a base class of a multiply-inherited
							// derived class object and there can be pointer offsets.
							// So we need to use the void cast registry to apply any necessary pointer offsets.
							//
							// Note that the up-cast path should be available because the pointed-to object has
							// already been transcribed (which records base<->derived relationships).
							if (set_pointer_to_object(
									// Object id of the actual (dynamic) pointed-to object...
									object_id,
									// Address of the actual (dynamic) pointed-to object...
									object_info.object_address.get(),
									object_ptr))
							{
								pointer_streamed = true;
								pointer_is_initialised = true;
							}
							// ...else failed - actual pointed-to object type does not inherit from 'ObjectType'.
						}
						else // saving...
						{
							pointer_streamed = true;
							pointer_is_initialised = true;
						}
					}
					else // pointed-to object not yet transcribed...
					{
						pointer_streamed = true;

						// We have to wait until the pointed-to object is transcribed before we
						// can initialise our pointer to point to it (if pointer is tracked).
						// This can happen when either:
						//  (1) 'transcribe' is called on another pointer (with ownership flags) that
						//      points to the same object as us, or
						//  (2) when 'transcribe' is called on the pointed-to object itself.
						// Note that this also works for pointer-to-pointer's, etc (in which case the
						// pointed-to object is itself a pointer).
					}

					if (pointer_streamed)
					{
						if (options & TRACK)
						{
							// Add our pointer to the list of pointers that reference the pointed-to object.
							//
							// In the load path, if the pointed-to object address was not available, then
							// we are now delaying initialisation of the pointer until the pointed-to object
							// is loaded.
							//
							// Even if the pointer was initialised above, this is still useful for when/if
							// the pointed-to object is subsequently relocated (*after* the pointer is
							// initialised to point to it) - in which case the pointer will get
							// re-initialised to point to the object's relocated location.
							add_pointer_referencing_object(object_id, pointer_object_id);
						}
						else // The pointer is *not* being tracked...
						{
							// The pointer is *not* being tracked so we avoid using 'add_pointer_referencing_object()'
							// (and 'resolve_pointer_reference_to_object()') since they record our pointer's address
							// which is problematic later on if the pointed-to object is relocated in which case
							// the pointer will get initialised or updated but, even though we have the address of
							// the pointer, we cannot assume the pointer will remain at that address once we return
							// from this transcribe call - by turning off tracking the client is telling us this.

							// Mark the pointed-to object as referenced by an untracked pointer so we can raise
							// an error if an attempt is later made to relocate the pointed-to object.
							//
							// Strictly speaking this isn't needed if 'pointer_is_initialised' is false
							// because this untracked pointer will fail to load at the end of this method anyway
							// (since untracked pointers cannot be delay-initialised).
							// So we'll avoid permanently marking the pointed-to object as unrelocatable
							// to improve the scribe client's chances of recovering from an error.
							// Update: Actually it doesn't matter because failing to initialise an
							// untracked pointer is an unrecoverable error (exception).
							if (pointer_is_initialised)
							{
								object_info.is_load_object_bound_to_a_reference_or_untracked_pointer = true;
							}
						}
					}
				}
			}
		}

		//
		// A pointer object is like a regular non-pointer object so
		// perform the same post-transcribe operations.
		//

		post_transcribe(pointer_object_id, options, !pointer_streamed/*discard*/, pointer_is_initialised);

		return pointer_streamed;
	}


	template <typename ObjectType>
	bool
	Scribe::transcribe_construct_object(
			ConstructObject<ObjectType *> &construct_object_ptr,
			const ObjectTag &object_tag,
			unsigned int options)
	{
		// Pointers don't have non-default constructors like regular objects and don't need to
		// be constructed like regular objects (because can just assign a pointer value to them).
		// So we can just initialise them with NULL and then get a reference to the pointer.

		// Only the load path requires initialisation.
		// For the save path we don't want to overwrite the pointer.
		if (is_loading())
		{
			construct_object_ptr.construct_object(reinterpret_cast<ObjectType *>(0));
		}

		return transcribe_object(
				construct_object_ptr.get_object(),
				object_tag,
				options);
	}


	template <typename ObjectType>
	bool
	Scribe::transcribe_construct_object(
			ConstructObject<ObjectType *> &construct_object_ptr,
			object_id_type object_id,
			unsigned int options)
	{
		// Pointers don't have non-default constructors like regular objects and don't need to
		// be constructed like regular objects (because can just assign a pointer value to them).
		// So we can just initialise them with NULL and then get a reference to the pointer.

		// Only the load path requires initialisation.
		// For the save path we don't want to overwrite the pointer.
		if (is_loading())
		{
			construct_object_ptr.construct_object(reinterpret_cast<ObjectType *>(0));
		}

		return transcribe_object(
				construct_object_ptr.get_object(),
				object_id,
				options);
	}


	template <typename ObjectType>
	bool
	Scribe::transcribe_pointer_owned_object(
			ObjectType *&object_ptr,
			bool shared_ownership,
			boost::optional<object_id_type &> return_object_id)
	{
		// The pointer owns the pointed-to object...

		// If loading then set the pointer to NULL in the meantime until it gets initialised.
		//
		// Also it might actually be a NULL pointer (ie, if save path transcribed a NULL pointer).
		if (is_loading())
		{
			object_ptr = NULL;
		}

		//
		// The pointed-to object.
		//

		// We need the *dynamic* object address since we want the full dynamic object
		// instead of a (potential) base class sub-object (referenced by pointer).
		const object_address_type object_address = InternalUtils::get_dynamic_object_address(object_ptr);

		// Transcribe the pointed-to object's id.
		// Note: 'object_address' will be NULL in the load path...
		object_id_type object_id;
		if (!transcribe_object_id(object_address, POINTS_TO_OBJECT_TAG, object_id))
		{
			return false;
		}

		// Exclude NULL pointers since there's no pointed-to object to transcribe or link to...
		if (object_id == NULL_POINTER_OBJECT_ID)
		{
			// Return object id to caller if requested.
			if (return_object_id)
			{
				return_object_id.get() = object_id;
			}

			// Nothing left to do - pointer has already been set to NULL on load path.
			return true;
		}

		// Find out how to transcribe the actual pointed-to object.
		boost::optional<InternalUtils::TranscribeOwningPointer::non_null_ptr_to_const_type> transcribe_owning_pointer;
		if (!transcribe_pointed_to_class_name_if_polymorphic(object_ptr, transcribe_owning_pointer))
		{
			// The pointed-to object type does not match anything we've export registered - which means either:
			//   * the archive was created by a future GPlates with a class name we don't know about, or
			//   * the archive was created by an old GPlates with a class name we have since removed.
			return false;
		}

		// Get the pointed-to object info.
		ObjectInfo &object_info = get_object_info(object_id);

		// Transcribe the pointed-to object unless it already has been (by a *shared* ownership pointer).
		if (object_info.is_object_pre_initialised)
		{
			// Throw exception if the object does not have shared ownership.
			//
			// If this assertion is triggered then it means:
			//   * Scribe client has transcribed an object more than once via a non-sharing owning pointer, or
			//   * Scribe client has transcribed an object via a non-sharing owning pointer and that object has
			//     already been transcribed but not through an (owning) pointer, or
			//   * Scribe client has created an island of objects that cyclically own each other (memory leak)
			//     via non-sharing owning pointers.
			//
			GPlatesGlobal::Assert<Exceptions::AlreadyTranscribedObject>(
					shared_ownership,
					GPLATES_ASSERTION_SOURCE,
					// Note that this is the pointed-to object itself and not the pointer...
					typeid(ObjectType),
					is_saving());

			// A pre-initialised object should have an address.
			GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
					object_info.object_address,
					GPLATES_ASSERTION_SOURCE,
					"Expected pre-initialised object to have an address.");

			if (is_loading())
			{
				// Initialise object pointer.
				//
				// We need to do any pointer fix ups in the presence of multiple inheritance.
				// It's possible that the pointer refers to a base class of a multiply-inherited
				// derived class object and there can be pointer offsets.
				// So we need to use the void cast registry to apply any necessary pointer offsets.
				//
				// Note that the up-cast path should be available because the pointed-to object has
				// already been transcribed (which records base<->derived relationships).
				if (!set_pointer_to_object(
						// Object id of the actual (dynamic) pointed-to object...
						object_id,
						// Address of the actual (dynamic) pointed-to object...
						object_info.object_address.get(),
						object_ptr))
				{
					// Failed - actual pointed-to object type does not inherit from 'ObjectType'.
					return false;
				}
			}

			// Return object id to caller if requested.
			if (return_object_id)
			{
				return_object_id.get() = object_id;
			}

			return true;
		}

		// Transcribe the pointed-to object.
		if (is_saving())
		{
			// Save the tracked object to the archive.
			//
			// NOTE: We don't need to set the object's address here because 'save_object' will transcribe
			// the object which will in turn record its address into 'object_info.object_address'.
			// In fact if we try to set it here *before* calling 'save_object()' then when the object
			// is transcribed it'll throw an exception thinking, because the object's address
			// has already been set, that an attempt was made to transcribe the same object twice.
			transcribe_owning_pointer.get()->save_object(
					*this,
					// Note: We need the *dynamic* object address (instead of the potentially
					// base class pointer that can differ under multiple inheritance) since
					// that's where the full dynamic object will get created...
					object_address.address,
					object_id,
					TRACK);
		}
		else // loading...
		{
			// Load the tracked object from the archive.
			//
			// NOTE: We don't need to set the object's address here because 'load_object' will transcribe
			// the object which will in turn record its address into 'object_info.object_address'.
			if (!transcribe_owning_pointer.get()->load_object(
					*this,
					object_id,
					TRACK))
			{
				return false;
			}
		}

		// Object should have been initialised.
		GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
				object_info.object_address &&
					object_info.is_object_post_initialised,
				GPLATES_ASSERTION_SOURCE,
				"Expected pointer-owned object to be initialised.");

		if (is_loading())
		{
			// Initialise object pointer.
			//
			// We need to do any pointer fix ups in the presence of multiple inheritance.
			// It's possible that the pointer refers to a base class of a multiply-inherited
			// derived class object and there can be pointer offsets.
			// So we need to use the void cast registry to apply any necessary pointer offsets.
			//
			// Note that the up-cast path should be available because the pointed-to object has
			// already been transcribed (which records base<->derived relationships).
			if (!set_pointer_to_object(
					// Object id of the actual (dynamic) pointed-to object...
					object_id,
					// Address of the actual (dynamic) pointed-to object...
					object_info.object_address.get(),
					object_ptr))
			{
				// Failed - actual pointed-to object type does not inherit from 'ObjectType'.
				return false;
			}
		}

		// Return object id to caller if requested.
		if (return_object_id)
		{
			return_object_id.get() = object_id;
		}

		return true;
	}


	template <typename ObjectType>
	bool
	Scribe::transcribe_smart_pointer_object(
			ObjectType *&object_ptr,
			bool shared_ownership)
	{
		// Note: We don't mark the pointed-to object as referenced by an untracked pointer because
		// relocating the pointed-to object means a new smart pointer is being created with a new
		// pointed-to object. In this case we don't want the original smart pointer to point to
		// the new pointed-to object (it should still point to the original pointed-to object).
		// For example when a boost::scoped_ptr<> is copied and relocated:
		//
		//   boost::scoped_ptr<int> x;
		//   scribe.transcribe(TRANSCRIBE_SOURCE, x, "x", GPlatesScribe::TRACK);
		//   boost::scoped_ptr<int> y(new int(*x));  // Copy 'y' from 'x' keeping same integer value.
		//   scribe.relocated(TRANSCRIBE_SOURCE, y, x);
		//   scribe.relocated(TRANSCRIBE_SOURCE, *y, *x);
		//
		// ...where the last line 'scribe.relocated(TRANSCRIBE_SOURCE, *y, *x)' is only possible if
		// we don't mark the pointed-to object '*x' as referenced by an untracked pointer.
		//
		// And so we don't want to prevent relocation of the pointed-to object in this case.
		//
		// Essentially we can view the smart pointer and its pointed-to object as a single unit.
		// If one is relocated then so is the other. If the smart pointer is a shared owner
		// (versus exclusive) then typically the pointed-to object never needs to be relocated
		// (because when a shared pointer is copied/moved it still points to the same pointed-to
		// object address) and so this problem doesn't arise.
		return transcribe_pointer_owned_object(
				// The pointer will be NULL in the load path before calling function...
				object_ptr,
				shared_ownership);
	}


	template <class BaseType, class DerivedType>
	bool
	Scribe::transcribe_base_object(
			DerivedType &derived_object,
			const ObjectTag &base_object_tag)
	{
		// Registers that class 'DerivedType' derives from class 'BaseType'.
		// Doing this enables base class pointers and references to be correctly upcast from
		// derived class objects to (in the presence of multiple inheritance).
		if (!transcribe_base_object<BaseType, DerivedType>())
		{
			return false;
		}

		// Get the derived object info.
		//
		// Note: We assume the currently transcribed object is the derived object because we don't
		// yet have the object address of the derived object recorded yet (it's still being transcribed).
		// and so we cannot find its object id.
		boost::optional<ObjectInfo &> derived_object_info = get_current_transcribed_object();
		GPlatesGlobal::Assert<Exceptions::ScribeUserError>(
				derived_object_info,
				GPLATES_ASSERTION_SOURCE,
				"Attempted to transcribe base class object outside of transcribing derived class object.");

		// Note that we don't use 'ConstructObject' because the 'BaseType' object has already been
		// constructed (in the derived object's constructor). And in any case 'BaseType' may be an
		// abstract base class (which is not constructable).
		//
		// Note: We don't call 'transcribe()' directly because it would require the object's
		// actual type to be 'BaseType' (but it's really 'DerivedType' or some derivation of that).
		// And we don't call 'transcribe_object()' directly because that bypasses the const conversions
		// (although in our case we use non-const classes so it wouldn't actually matter).
		if (!transcribe_const_cast(
				static_cast<BaseType &>(derived_object),
				base_object_tag,
				// The tracking of base class object should always be enabled even if the client requested
				// tracking be disabled for the derived class object.
				// When the derived class object has finished transcribing it will disable tracking of all
				// its child-objects (includes base classes and data members)...
				TRACK))
		{
			return false;
		}

		//
		// Record the base class sub-object in the derived class.
		//

		// Using 'static' address since we know actual type of base class sub-object is 'BaseType'.
		const object_id_type base_object_id = get_object_id(
				InternalUtils::get_static_object_address(
						&static_cast<BaseType &>(derived_object)));

		// Allocate a list node from the node pool allocator.
		// Place the base class sub-object id in the node.
		object_ids_list_type::Node *base_class_sub_object_list_node =
				d_object_ids_list_node_pool.construct(
						object_ids_list_type::Node(base_object_id));

		// Add our pointer to the list.
		derived_object_info->base_class_sub_objects.append(*base_class_sub_object_list_node);

		return true;
	}


	template <class BaseType, class DerivedType>
	bool
	Scribe::transcribe_base_object()
	{
		// Compile-time assertion to ensure that 'BaseType' and 'DerivedType' are classes.
		// Only classes can inherit from each other.
		BOOST_STATIC_ASSERT(boost::is_class<BaseType>::value);
		BOOST_STATIC_ASSERT(boost::is_class<DerivedType>::value);

		// Compile-time assertion to ensure that 'BaseType' is not const.
		BOOST_STATIC_ASSERT(boost::mpl::not_< boost::is_const<BaseType> >::value);

		// Compile-time assertion to ensure that 'DerivedType' is not const.
		BOOST_STATIC_ASSERT(boost::mpl::not_< boost::is_const<DerivedType> >::value);

		// Registers that class 'DerivedType' derives from class 'BaseType'.
		// Doing this enables base class pointers and references to be correctly upcast from
		// derived class objects to (in the presence of multiple inheritance).
		d_void_cast_registry.register_derived_base_class_inheritance<DerivedType, BaseType>();

		// Currently this method always returns true.
		return true;
	}


	template <typename ObjectType>
	void
	Scribe::save_object_reference(
			ObjectType &object_reference,
			const ObjectTag &object_tag)
	{
		// Compile-time assertion to ensure that 'ObjectType' is not const.
		BOOST_STATIC_ASSERT(boost::mpl::not_< boost::is_const<ObjectType> >::value);

		GPlatesGlobal::Assert<Exceptions::ScribeUserError>(
				is_saving(),
				GPLATES_ASSERTION_SOURCE,
				"Attempted to save an object reference when loading an archive.");

		// We need the *dynamic* object address since we want the full dynamic object
		// instead of a (potential) base class sub-object (referenced by reference).
		const object_address_type object_address =
				InternalUtils::get_dynamic_object_address(&object_reference);

		// See if the referenced object has been visited yet.
		const boost::optional<object_id_type> object_id = find_object_id(object_address);

		// Throw exception if the referenced object has not yet been visited.
		//
		// If this assertion is triggered then it means:
		//   * The reference was transcribed before the referenced object, or
		//   * Object tracking was turned off when the referenced object itself was saved (ie, we can't find it), or
		//   * The referenced object was never transcribed by the scribe client.
		//
		GPlatesGlobal::Assert<Exceptions::TranscribedReferenceBeforeReferencedObject>(
				object_id,
				GPLATES_ASSERTION_SOURCE,
				typeid(ObjectType));

		// Get the referenced object info.
		ObjectInfo &object_info = get_object_info(object_id.get());

		// Throw exception if the referenced object has not yet been saved.
		//
		// If this assertion is triggered then it means:
		//   * The reference was transcribed before the referenced object was saved.
		//
		GPlatesGlobal::Assert<Exceptions::TranscribedReferenceBeforeReferencedObject>(
				// When loading the archive (that we're saving) the referenced object must have a
				// valid address to bind the reference to, because references cannot be re-bound and
				// so binding cannot be delayed like it can for pointers.
				// Note that this doesn't mean the loaded object itself will be initialised,
				// for example if the object is actually a pointer then we can still bind a reference
				// to it (reference-to-pointer) before the pointer actually points to anything.
				object_info.object_address,
				GPLATES_ASSERTION_SOURCE,
				typeid(ObjectType));

		// Transcribe the referenced object id.
		// We're on the *save* path so no need to check return value.
		transcribe_object_id(object_address, object_tag);
	}


	template <typename ObjectType>
	LoadRef<ObjectType>
	Scribe::load_object_reference(
			const GPlatesUtils::CallStack::Trace &transcribe_source,
			const ObjectTag &object_tag)
	{
		GPlatesGlobal::Assert<Exceptions::ScribeUserError>(
				is_loading(),
				GPLATES_ASSERTION_SOURCE,
				"Attempted to load an object reference when saving an archive.");

		// Transcribe the referenced object id
		//
		// Note: The object address is NULL in this load path (we don't know it until we get the object id)...
		object_id_type object_id;
		if (!transcribe_object_id(object_address_type(), object_tag, object_id))
		{
			// Return NULL reference.
			return LoadRef<ObjectType>();
		}

		// Get the referenced object info.
		ObjectInfo &object_info = get_object_info(object_id);

		// Check if the referenced object has been transcribed yet.
		//
		// Usually this gets detected on archive creation (an exception during saving) so it
		// shouldn't normally trigger an error here.
		//
		// However it's possible that (due to backward/forward compatibility) the transcribed object
		// was not loaded (eg, object is only known by the future GPlates that created the archive).
		// In this case we flag the (referenced) object as unknown so that caller can potentially
		// ignore these future unknown objects and continue transcribing (rather than failing) thus
		// improving forward compatibility.
		//
		if (!object_info.object_address)
		{
			// Record the reason for transcribe failure.
			set_transcribe_result(TRANSCRIBE_SOURCE, TRANSCRIBE_UNKNOWN_TYPE);

			// Return NULL reference.
			return LoadRef<ObjectType>();
		}

		// We need to do any pointer fix ups in the presence of multiple inheritance.
		// It's possible that the reference refers to a base class of a multiply-inherited
		// derived class object and there can be pointer offsets.
		// So we need to use the void cast registry to apply any necessary pointer offsets.
		//
		// Note that the up-cast path should be available because the pointed-to object has
		// already been transcribed (which records base<->derived relationships).
		ObjectType *referenced_object_ptr;
		if (!set_pointer_to_object(
				// Object id of the actual (dynamic) pointed-to object...
				object_id,
				// Address of the actual (dynamic) pointed-to object...
				object_info.object_address.get(),
				referenced_object_ptr))
		{
			// Return NULL reference.
			return LoadRef<ObjectType>();
		}

		// Mark the referenced object as referenced so we can raise an error if an attempt
		// is later made to relocate the referenced object.
		//
		// Note that this means if client tries to relocate returned LoadRef then
		// 'RelocatedObjectBoundToAReferenceOrUntrackedPointer' will get thrown.
		object_info.is_load_object_bound_to_a_reference_or_untracked_pointer = true;

		// Return reference to the object.
		return LoadRef<ObjectType>(
				transcribe_source,
				*this,
				static_cast<ObjectType *>(referenced_object_ptr),
				// Referencing an existing object (not transferring ownership)...
				false/*release*/);
	}


	template <typename ObjectType>
	void
	Scribe::relocated_transcribed_object(
			ObjectType &relocated_object,
			ObjectType &transcribed_object)
	{
		// Compile-time assertion to ensure that 'ObjectType' is not const.
		BOOST_STATIC_ASSERT(boost::mpl::not_< boost::is_const<ObjectType> >::value);

		// Throw exception if the object is not the type we expect it to be (it should be type 'ObjectType').
		//
		// If this assertion is triggered then it means:
		//   * A Scribe client has called 'relocated' on an object *reference* instead of an object
		//     and the object type is different than the reference type.
		//
		GPlatesGlobal::Assert<Exceptions::RelocatedReferenceInsteadOfObject>(
				typeid(ObjectType) == typeid(transcribed_object) &&
					typeid(ObjectType) == typeid(relocated_object),
				GPLATES_ASSERTION_SOURCE,
				transcribed_object);

		// Associate type information with the object addresses.
		object_address_type relocated_object_address =
				InternalUtils::get_static_object_address(&relocated_object);
		object_address_type transcribed_object_address =
				InternalUtils::get_static_object_address(&transcribed_object);

		// Calculate the pointer offset from the transcribed object address to the relocated object address.
		//
		// Note that we don't use "std::ptrdiff_t" (which is signed) to store the pointer offset
		// because it's possible (although highly unlikely) that it could overflow the signed range.
		std::size_t relocation_pointer_offset;
		bool is_relocation_pointer_offset_positive;
		if (relocated_object_address.address > transcribed_object_address.address)
		{
			relocation_pointer_offset =
					reinterpret_cast<std::size_t>(relocated_object_address.address) -
						reinterpret_cast<std::size_t>(transcribed_object_address.address);
			is_relocation_pointer_offset_positive = true;
		}
		else
		{
			relocation_pointer_offset =
					reinterpret_cast<std::size_t>(transcribed_object_address.address) -
						reinterpret_cast<std::size_t>(relocated_object_address.address);
			is_relocation_pointer_offset_positive = false;
		}

		// The transcribed object id.
		const boost::optional<object_id_type> transcribed_object_id =
				find_object_id(transcribed_object_address);

		// Throw exception if the transcribed object cannot be found.
		//
		// If this assertion is triggered then it means:
		//   * Object tracking was turned off (ie, we can't find the object being relocated), or
		//   * The specified object was never transcribed by the scribe client.
		//
		GPlatesGlobal::Assert<Exceptions::RelocatedUntrackedObject>(
				transcribed_object_id,
				GPLATES_ASSERTION_SOURCE);

		// Cast to 'void' pointers and delegate to the non-template method.
		relocated_address(
				transcribed_object_id.get(),
				transcribed_object_address,
				relocated_object_address,
				relocation_pointer_offset,
				is_relocation_pointer_offset_positive);

		// If the relocated object address is inside, or outside, the memory area of its parent then
		// add, or remove, the object as a sub-object of its parent (if it's not already the case).
		// 
		// Adding a sub-object can happen when objects with no default constructor are transcribed -
		// they need to implement 'transcribe_construct_data()' (as well as 'transcribe()') in order
		// to transcribe their constructor parameters - these constructor parameters are initially
		// outside the object's memory area but subsequently get relocated inside the object when the
		// object is transcribed - when this happens the relocated constructor parameters need to be
		// registered as sub-objects so that when/if the object itself is later relocated then those
		// sub-objects (that were constructor parameters) will also get properly relocated.
		// Here we are essentially testing whether the object currently being relocated is one of
		// those constructor parameters and if so then adding it as a sub-object (if isn't already).
		//
		// Removing a sub-object can happen when only part of a transcribed parent object is used.
		// A sub-object is relocated out of the parent object and the rest of the parent is not used.
		// This is unlikely though since the parent must be tracked for relocations to work and
		// leaving an unused but tracked (parent) object lying around can be problematic.
		add_or_remove_relocated_child_as_sub_object_if_inside_or_outside_parent(transcribed_object_id.get());
	}


	template <typename ObjectType>
	void
	Scribe::untrack(
			ObjectType &object,
			bool discard)
	{
		untrack_const_cast(object, discard);
	}


	template <typename ObjectType>
	void
	Scribe::untrack_object(
			ObjectType &object,
			bool discard)
	{
		// Compile-time assertion to ensure that 'ObjectType' is not const.
		BOOST_STATIC_ASSERT(boost::mpl::not_< boost::is_const<ObjectType> >::value);

		// We need the *dynamic* object address since we want the full dynamic object instead
		// of a (potential) base class sub-object (referenced by 'ObjectType' reference).
		const object_address_type object_address =
				InternalUtils::get_dynamic_object_address(&object);

		// An object is currently tracked if its object address is associated with an object id.
		boost::optional<object_id_type> object_id  = find_object_id(object_address);
		if (!object_id)
		{
			// Object is untracked (or couldn't be found at address).
			return;
		}

		unmap_tracked_object_address_to_object_id(object_id.get(), discard);
	}


	template <typename ObjectType>
	bool
	Scribe::transcribe_pointed_to_class_name_if_polymorphic(
			ObjectType *object_ptr,
			boost::optional<
				boost::optional<InternalUtils::TranscribeOwningPointer::non_null_ptr_to_const_type> &> owns)
	{
		// Transcribe differently depending of whether 'ObjectType' is polymorphic and hence
		// the actual (polymorphic) pointed-to type could differ from 'ObjectType'
		// (eg, 'ObjectType' could be a base class and the actual object type could be a derived class).
		return transcribe_pointed_to_class_name_if_polymorphic(
				object_ptr,
				owns,
				// We only want to instantiate polymorphic code for polymorphic 'ObjectType' and
				// non-polymorphic code for non-polymorphic 'ObjectType'.
				// Specifically we don't want to instantiate 'register_object_type<ObjectType>()' for
				// polymorphic 'ObjectType' in case there is no 'transcribe()' overload/specialisation
				// for it (which would result in a compile-time error)...
				typename boost::is_polymorphic<ObjectType>::type());
	}


	template <typename ObjectType>
	bool
	Scribe::transcribe_pointed_to_class_name_if_polymorphic(
			ObjectType *object_ptr,
			boost::optional<
				boost::optional<InternalUtils::TranscribeOwningPointer::non_null_ptr_to_const_type> &> owns,
			boost::mpl::true_/*'ObjectType' is polymorphic*/)
	{
		if (is_saving())
		{
			GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
					object_ptr,
					GPLATES_ASSERTION_SOURCE,
					"Expecting non-null pointer in save path.");

			// The actual (polymorphic) type of the pointed-to object could differ from 'ObjectType'
			// so we transcribe the class name.
			//
			// We expect the actual type to have been export registered (see 'ScribeExportRegistration.h').
			// So we need to search the export registered classes and output a class name.
			const std::type_info &save_object_type_info = typeid(*object_ptr);

			// Transcribe the class name associated with the actual type of the pointed-to object.
			boost::optional<const ExportClassType &> export_class_type;
			transcribe_class_name(&save_object_type_info, export_class_type);

			// Should never be none in save path (if 'transcribe_class_name' detects an error it will throw).
			GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
					export_class_type,
					GPLATES_ASSERTION_SOURCE,
					"Transcribing class name failure should have previously thrown an exception.");

			// Return the ability to transcribe pointed-to object if pointer owns it.
			if (owns)
			{
				owns.get() = export_class_type->transcribe_owning_pointer;
			}
		}

		if (is_loading())
		{
			// Attempt to load the class name associated with the actual type of the pointed-to object.
			//
			// This is the actual type that was export registered (see 'ScribeExportRegistration.h').
			boost::optional<const ExportClassType &> export_class_type;
			if (transcribe_class_name(NULL, export_class_type))
			{
				// The save path determined that 'ObjectType' was polymorphic and hence the type
				// of the pointed-to object could differ from 'ObjectType'.

				// Return the ability to transcribe pointed-to object if pointer owns it.
				if (owns)
				{
					owns.get() = export_class_type->transcribe_owning_pointer;
				}
			}
			else
			{
				if (get_transcribe_result() == TRANSCRIBE_UNKNOWN_TYPE)
				{
					// The class name was successfully loaded, but does not match anything we've
					// export registered - which means either:
					//   * the archive was created by a future GPlates with a class name we don't know about, or
					//   * the archive was created by an old GPlates with a class name we have since removed.
					//
					// Note that we fail even for *non-owning* pointers as a way to improve chances
					// of forward compatibility by failing the pointer load immediately so the scribe
					// client can decide whether to ignore/discard the pointer without aborting loading.
					return false;
				}
				else // TRANSCRIBE_INCOMPATIBLE...
				{
					// Override the error code because we were only checking for the existence of
					// a class name - it's not an error (yet) as far as the caller is concerned.
					set_transcribe_result(TRANSCRIBE_SOURCE, TRANSCRIBE_SUCCESS);

					// We couldn't find "class name" info ('object_tag'), in the load path,
					// within parent object scope.
					//
					// This means the save path did not transcribe the class name because it
					// did not encounter a polymorphic 'ObjectType'.
					//
					// So the end result is, for owning pointers, the transcribed object must be
					// transcription-compatible with 'ObjectType' otherwise the load will fail.
					// In other words they may be different types due to backward/forward compatible
					// changes but the load could still succeed.

					// Return the ability to transcribe pointed-to object if pointer owns it.
					if (owns)
					{
						// We don't transcribe the object type - if 'ObjectType' differs in the save and
						// load paths then loading will only succeed if they are transcription-compatible.
						//
						// Find the export registered class type for the pointed-to object.
						//
						// Note we don't call 'register_object_type<ObjectType>()' because 'ObjectType'
						// may be an abstract class or there may not be a 'transcribe()'
						// specialisation/overload for it (eg, an empty base class).
						export_class_type = ExportRegistry::instance().get_class_type(typeid(ObjectType));
						if (!export_class_type)
						{
							// 'ObjectType' has not been export registered.
							// It should be exported registered though - ideally all polymorphic
							// types should be.
							set_transcribe_result(TRANSCRIBE_SOURCE, TRANSCRIBE_INCOMPATIBLE);

							return false;
						}

						owns.get() = export_class_type->transcribe_owning_pointer;
					}
				}
			}
		}

		return true;
	}


	template <typename ObjectType>
	bool
	Scribe::transcribe_pointed_to_class_name_if_polymorphic(
			ObjectType *object_ptr,
			boost::optional<
				boost::optional<InternalUtils::TranscribeOwningPointer::non_null_ptr_to_const_type> &> owns,
			boost::mpl::false_/*'ObjectType' is *not* polymorphic*/)
	{
		if (is_saving())
		{
			// Return the ability to transcribe pointed-to object if pointer owns it.
			if (owns)
			{
				// The actual type of the pointed-to object is 'ObjectType'.
				//
				// We don't transcribe the object type - if 'ObjectType' differs in the save and load paths
				// then loading will only succeed if they are transcription-compatible.
				//
				// Note: If the actual object type is not 'ObjectType' then it'll get sliced when
				// transcribed - however there's no way to detect slicing (transcribing a derived
				// class object through a *non-polymorphic* base class pointer but only transcribing
				// the base class sub-object).
				const class_id_type class_id = register_object_type<ObjectType>();

				owns.get() = get_class_info(class_id).transcribe_owning_pointer;

				// We know that 'ObjectType' cannot be an abstract class because if it was abstract
				// then it would have run-time type information (RTTI) since it would have (pure)
				// virtual methods. Hence it would be polymorphic and we wouldn't be able to get here.
				// So since 'ObjectType' is not abstract then 'register_object_type<>()' would
				// have created a valid TranscribeOwningPointer for it.
				GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
						owns.get(),
						GPLATES_ASSERTION_SOURCE,
						"Expecting non-abstract, non-array pointed-to object in save path.");
			}
		}

		if (is_loading())
		{
			// 'ObjectType' is *non-polymorphic* so an *owning* pointer will only be able
			// to 'delete' an instance of 'ObjectType'. For this reason we completely ignore
			// any transcribed class name - we have to create an instance of 'ObjectType' regardless.
			// And *non-owning* pointers don't need to create an object instance (because don't own)
			// so they are fine (it's possible that they point to the 'ObjectType' data member or
			// sub-object of another object type - that's OK).
			//
			// So the end result is, for owning pointers, the transcribed object must be
			// transcription-compatible with 'ObjectType' otherwise the load will fail.
			// In other words they may be different types due to backward/forward compatible changes
			// but the load could still succeed.

			// Return the ability to transcribe pointed-to object if pointer owns it.
			if (owns)
			{
				// We don't transcribe the object type - if 'ObjectType' differs in the save and
				// load paths then loading will only succeed if they are transcription-compatible.
				const class_id_type class_id = register_object_type<ObjectType>();

				owns.get() = get_class_info(class_id).transcribe_owning_pointer;

				// We know that 'ObjectType' cannot be an abstract class because if it was abstract
				// then it would have run-time type information (RTTI) since it would have (pure)
				// virtual methods. Hence it would be polymorphic and we wouldn't be able to get here.
				// So since 'ObjectType' is not abstract then 'register_object_type<>()' would
				// have created a valid TranscribeOwningPointer for it.
				GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
						owns.get(),
						GPLATES_ASSERTION_SOURCE,
						"Expecting non-abstract, non-array pointed-to object in load path.");
			}
		}

		return true;
	}


	template <typename ObjectType>
	Scribe::class_id_type
	Scribe::register_object_type()
	{
		// Create a new class id if the 'ObjectType' has not been seen before.
		const class_id_type class_id = get_or_create_class_id(typeid(ObjectType));

		// Get the class info.
		ClassInfo &class_info = get_class_info(class_id);

		// Return early if the class info has already been initialised.
		if (!class_info.initialised)
		{
			// Determine the class size.
			class_info.object_size = sizeof(ObjectType);

			// Record the class type info.
			class_info.object_type_info = &typeid(ObjectType);

			// Record the class dereference type info.
			//
			// If the object type is a pointer then select the dereference type,
			// otherwise just use the object type.
			class_info.dereference_type_info = boost::is_pointer<ObjectType>::value
					? &typeid(typename boost::remove_pointer<ObjectType>::type)
					: &typeid(ObjectType);

			// Create the relocated handler for the object class type.
			class_info.relocated_handler = InternalUtils::RelocatedTemplate<ObjectType>::create();

			// Initialise class info that only applies to classes that can be instantiated which
			// excludes abstract classes.
			//
			// We also exclude native array objects since we don't allocate them on the heap
			// (in 'TranscribeOwningPointerTemplate').
			//
			// NOTE: Boost recommends we implement alternative to boost::is_abstract if BOOST_NO_IS_ABSTRACT
			// is defined - however we use compilers that support this (ie, MSVC8, gcc 4 and above).
#ifdef BOOST_NO_IS_ABSTRACT
			// Evaluates to false - but is dependent on template parameter - compiler workaround...
			BOOST_STATIC_ASSERT(sizeof(ObjectType) == 0);
#endif
			register_instantiable_class_info<ObjectType>(
					class_info,
					typename boost::mpl::not_<
							boost::mpl::or_<
									boost::is_array<ObjectType>,
									boost::is_abstract<ObjectType> > >::type());

			// Mark the class info as initialised so we don't repeat the above initialisation.
			class_info.initialised = true;
		}

		return class_id;
	}


	template <typename ObjectType>
	void
	Scribe::register_instantiable_class_info(
			ClassInfo &class_info,
			boost::mpl::true_/*'ObjectType' is instantiable*/)
	{
		// Create the transcribe-owning-pointer.
		//
		// This enables us to transcribe an object of type 'ObjectType' if we later encounter
		// one via an (owning) pointer.
		class_info.transcribe_owning_pointer =
				InternalUtils::TranscribeOwningPointerTemplate<ObjectType>::create();
	}


	template <typename ObjectType>
	void
	Scribe::register_instantiable_class_info(
			ClassInfo &class_info,
			boost::mpl::false_/*'ObjectType' is not instantiable*/)
	{
		// Do nothing - we're initialising non-array non-abstract class info.
	}


	template <typename ObjectType>
	bool
	Scribe::set_pointer_to_object(
			object_id_type object_id,
			void *object_address,
			ObjectType *&object_ptr)
	{
		GPlatesGlobal::Assert<Exceptions::ScribeUserError>(
				is_loading(),
				GPLATES_ASSERTION_SOURCE,
				"Attempted to set a pointer to an object when saving an archive.");

		// Shouldn't be able to get here without a valid object type info.
		GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
				object_address,
				GPLATES_ASSERTION_SOURCE,
				"Expected a non-null pointed-to object address.");

		ClassInfo &class_info = get_class_info_from_object(object_id);

		// Shouldn't be able to get here without a valid object type info.
		GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
				class_info.object_type_info,
				GPLATES_ASSERTION_SOURCE,
				"Pointer is referencing object before its object type info is available.");

		// We need to do any pointer fix ups in the presence of multiple inheritance.
		// It's possible that the pointer refers to a base class of a multiply-inherited
		// derived class object and there can be pointer offsets.
		// So we need to use the void cast registry to apply any necessary pointer offsets.
		//
		// Note that the up-cast path should be available because the pointed-to object has
		// already been transcribed (which records base<->derived relationships).
		boost::optional<void *> referenced_object_address =
				d_void_cast_registry.up_cast(
						// Actual type of the pointed-to object...
						*class_info.object_type_info.get(),
						// Our reference points to this type...
						typeid(ObjectType),
						// Address of the actual pointed-to object...
						object_address);
		if (!referenced_object_address)
		{
			// Record the reason for transcribe failure.
			//
			// The up-cast failed because the actual referenced object type does not inherit
			// directly or indirectly from 'ObjectType' and so we can't legally reference it.
			// This can happen when the actual object is created dynamically (via a base class
			// pointer) and when it was saved on another system.
			// For example:
			//
			//		template <typename T>
			//		class A
			//		{
			//		public:
			//			virtual ~A() { }
			//		};
			//
			//		template <typename T>
			//		class B : public A<T>
			//		{  };
			//
			// ...where saving on machine X with 'std::size_t' typedef'ed to 'unsigned int'...
			//
			//		boost::shared_ptr< A<std::size_t> > b(new B<std::size_t>());
			//		scribe.transcribe(TRANSCRIBE_SOURCE, b, "b", GPlatesScribe::TRACK);
			//		A<std::size_t> *a = b;
			//		scribe.transcribe(TRANSCRIBE_SOURCE, a, "a", GPlatesScribe::TRACK);
			//
			// ...where loading on machine Y with 'std::size_t' typedef'ed to 'unsigned long'...
			//
			//		boost::shared_ptr< A<std::size_t> > b;
			//		if (!scribe.transcribe(TRANSCRIBE_SOURCE, b, "b", GPlatesScribe::TRACK)) { ... }  // Actually this would fail first
			//		A<std::size_t> *a;
			//		if (!scribe.transcribe(TRANSCRIBE_SOURCE, a, "a", GPlatesScribe::TRACK)) { ... }
			//
			// ...where 'b' is saved on machine X as 'B<unsigned int>' and 'b' is also loaded on
			// machine Y as 'B<unsigned int>' (since that's the class name stored in the transcription).
			// But on machine Y, 'A<unsigned long> *' cannot reference 'B<unsigned int>' because
			// 'B<unsigned int>' does not inherit from 'A<unsigned long>'.
			//
			set_transcribe_result(TRANSCRIBE_SOURCE, TRANSCRIBE_INCOMPATIBLE);

			return false;
		}

		// Set the pointer.
		object_ptr = static_cast<ObjectType *>(referenced_object_address.get());

		return true;
	}


	template <typename ObjectType>
	bool
	Scribe::stream_construct_object(
			ConstructObject<ObjectType> &construct_object)
	{
		// Transcribe object constructor data.
		// On load path also constructs object - but not needed on save path since object already exists.
		//
		// Note: We don't qualify the 'transcribe_construct_data()' call with GPlatesScribe. This allows clients
		// to implement 'transcribe_construct_data()' in a different namespace - the function is then found by
		// Argument Dependent Lookup (ADL) based on the namespace in which 'ObjectType' is declared.
		set_transcribe_result(
				TRANSCRIBE_SOURCE,
				transcribe_construct_data(*this, construct_object));
		if (get_transcribe_result() != TRANSCRIBE_SUCCESS)
		{
			return false;
		}

		// Transcribe object.
		return stream(construct_object.get_object(), true/*transcribed_construct_data*/);
	}


	template <typename ObjectType>
	bool
	Scribe::stream_object(
			ObjectType &object)
	{
		return stream(object, false/*transcribed_construct_data*/);
	}


	template <typename ObjectType>
	bool
	Scribe::stream(
			ObjectType &object,
			bool transcribed_construct_data)
	{
		// Compile-time assertion to ensure pointers are transcribed directly using class 'Scribe'.
		BOOST_STATIC_ASSERT(boost::mpl::not_< boost::is_pointer<ObjectType> >::value);

		// Direct the transcribe to the transcription context or the general
		// 'GPlatesScribe::transcribe()' mechanism depending on the object type.
		return stream(
				object,
				transcribed_construct_data,
				typename boost::mpl::eval_if<
						boost::mpl::or_<
								boost::is_arithmetic<ObjectType>,
								boost::is_same<std::string, ObjectType> >,
						// Transcribe primitives...
						boost::mpl::identity<StreamPrimitiveTag>,
						// Transcribe objects...
						boost::mpl::identity<StreamTranscribeTag> >::type());
	}


	template <typename ObjectType>
	bool
	Scribe::stream(
			ObjectType &object,
			bool transcribed_construct_data,
			StreamPrimitiveTag)
	{
		// Re-direct types handled specifically by the transcription context directly to it.
		// Instead of the general non-member 'GPlatesScribe::transcribe()' mechanism.
		if (!d_transcription_context.transcribe(object))
		{
			set_transcribe_result(TRANSCRIBE_SOURCE, TRANSCRIBE_INCOMPATIBLE);
			return false;
		}

		set_transcribe_result(TRANSCRIBE_SOURCE, TRANSCRIBE_SUCCESS);
		return true;
	}


	namespace Implementation
	{
		//
		// In order to get Argument Dependent Lookup (ADL) for the non-member 'transcribe()' function,
		// based on the namespace in which 'ObjectType' is declared, we need to use a non-member
		// helper function to avoid the clash with same-named member function 'Scribe::transcribe()'.
		//
		// We cannot seem to use 'using GPlatesScribe::transcribe' as an alternative because the
		// differing return types (bool vs TranscribeResult) cause lookup problems.
		//
		template <typename ObjectType>
		TranscribeResult
		transcribe_ADL(
				Scribe &scribe,
				ObjectType &object,
				bool transcribed_construct_data)
		{
			// Call the non-member function 'GPlatesScribe::transcribe()' whose default implementation,
			// along with any specialisations and overloads, determine how to transcribe the specified object.
			//
			// Note: We don't qualify the 'transcribe()' call with GPlatesScribe. This allows clients
			// to implement 'transcribe()' in a different namespace - the function is then found by
			// Argument Dependent Lookup (ADL) based on the namespace in which 'ObjectType' is declared.
			return transcribe(scribe, object, transcribed_construct_data);
		}
	}

	template <typename ObjectType>
	bool
	Scribe::stream(
			ObjectType &object,
			bool transcribed_construct_data,
			StreamTranscribeTag)
	{
		// Call the *non-member* function 'transcribe()' via Argument Dependent Lookup (ADL).
		set_transcribe_result(
				TRANSCRIBE_SOURCE,
				Implementation::transcribe_ADL(*this, object, transcribed_construct_data));

		return get_transcribe_result() == TRANSCRIBE_SUCCESS;
	}


	template <typename T, typename NonConstT>
	void
	Scribe::reset_impl(
			boost::shared_ptr<T> &shared_ptr_object,
			NonConstT *raw_ptr)
	{
		GPlatesGlobal::Assert<Exceptions::ScribeUserError>(
				is_loading(),
				GPLATES_ASSERTION_SOURCE,
				"Attempted to load a boost::shared_ptr when saving an archive.");

		// If the raw pointer is NULL then it means the shared_ptr is empty.
		if (raw_ptr == NULL)
		{
			shared_ptr_object.reset();
			return;
		}

		// Attempt to insert the pointed-to object address into our map of shared_ptr<void>.
		std::pair<shared_ptr_map_type::iterator, bool> shared_ptr_inserted =
				d_shared_ptr_map.insert(
						shared_ptr_map_type::value_type(
								InternalUtils::get_dynamic_object_address(raw_ptr),
								boost::shared_ptr<void>()/*dummy*/));

		// If first time seen the pointed-to object (ie, successfully inserted into map)...
		if (shared_ptr_inserted.second)
		{
			// We don't need any multiple-inheritance pointer fix-ups here because that has already
			// been done when the raw pointer was transcribed.
			shared_ptr_object.reset(raw_ptr);

			// We need to store shared pointer as 'non-const' in case other shared pointers
			// (referencing same pointed-to object) have 'non-const' pointers.
			boost::shared_ptr<NonConstT> non_const_shared_ptr =
					boost::const_pointer_cast<NonConstT>(shared_ptr_object);

			// Due to the possible presence of multiple inheritance we need to store the
			// actual (dynamic) object address using dynamic cast if possible.
			shared_ptr_inserted.first->second =
					InternalUtils::shared_ptr_cast<void>(non_const_shared_ptr);

			return;
		}

		// We need to do any pointer fix ups in the presence of multiple inheritance.
		// It's possible that the pointer refers to a base class of a multiply-inherited
		// derived class object and there can be pointer offsets.
		// So we need to use the void cast registry to apply any necessary pointer offsets.

		// The pointer to the actual (dynamic) object.
		boost::shared_ptr<void> void_shared_ptr = shared_ptr_inserted.first->second;

		// Do any pointer casting/fix-ups.
		boost::optional< boost::shared_ptr<void> > referenced_shared_ptr =
				d_void_cast_registry.up_cast(
						// The actual type of the pointed-to object...
						typeid(*raw_ptr),
						// Our shared pointer points to this type...
						typeid(T),
						// This address is the address of the pointed-to object...
						void_shared_ptr);

		// Throw UnregisteredCast exception if unable to find path between the dynamic pointed-to
		// object type and the static pointer dereference type.
		GPlatesGlobal::Assert<Exceptions::UnregisteredCast>(
				referenced_shared_ptr,
				GPLATES_ASSERTION_SOURCE,
				typeid(*raw_ptr),
				typeid(T));

		// Now we can use static cast to get our shared_ptr<T>.
		shared_ptr_object = boost::static_pointer_cast<T>(referenced_shared_ptr.get());
	}
}

#endif // GPLATES_SCRIBE_SCRIBE_H
