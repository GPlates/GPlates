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

#ifndef GPLATES_SCRIBE_SCRIBEINTERNALUTILS_H
#define GPLATES_SCRIBE_SCRIBEINTERNALUTILS_H

#include <functional>
#include <typeinfo>
#include <utility>
#include <boost/shared_ptr.hpp>
#include <boost/type_traits/is_polymorphic.hpp>

#include "ScribeOptions.h"
#include "Transcribe.h"
#include "TranscriptionScribeContext.h"

#include "utils/ReferenceCount.h"


namespace GPlatesScribe
{
	// Forward declarations.
	class Scribe;


	/**
	 * Miscellaneous utilities that are only useful for the Scribe library implementation itself should go here.
	 */
	namespace InternalUtils
	{
		//! Typedef for an integer identifier for a transcribed object.
		typedef TranscriptionScribeContext::object_id_type object_id_type;


		/**
		 * Used to order std::type_info objects in a std::map.
		 *
		 * Note that std::type_info is not copy-constructable so we use pointers instead of references.
		 */
		struct SortTypeInfoPredicate :
				public std::binary_function<const std::type_info *, const std::type_info *, bool>
		{
			SortTypeInfoPredicate()
			{  }

			bool
			operator()(
					const std::type_info *lhs,
					const std::type_info *rhs) const
			{
				// The '!=' is because MSVC returns 'int' instead of 'bool'.
				return lhs->before(*rhs) != 0;
			}
		};


		/**
		 * An identifier for an object address that uses the address and the object type.
		 *
		 * The reason for including the object type is to distinguish different types of objects
		 * at the same address as in the following example:
		 *
		 *   class A { int x; int y; };
		 *   A a;
		 *   assert(static_cast<const void *>(&a) == static_cast<const void *>(&a.x));
		 *
		 * ...where both object 'a' and its internal sub-object 'a.x' have
		 * the same address but different types.
		 */
		struct ObjectAddress
		{
			//! Default constructor sets the NULL 'void' pointer.
			ObjectAddress() :
				address(NULL),
				type(&typeid(void))
			{ }

			ObjectAddress(
					const std::type_info &type_) :
				address(NULL),
				type(&type_)
			{ }

			ObjectAddress(
					void *address_,
					const std::type_info &type_) :
				address(address_),
				type(&type_)
			{ }

			bool
			operator==(
					const ObjectAddress &other) const
			{
				return address == other.address &&
						*type == *other.type;
			}

			bool
			operator!=(
					const ObjectAddress &other) const
			{
				return !operator==(other);
			}

			void *address;

			// Note that std::type_info is not copy-constructable so we use pointers instead of references...
			const std::type_info *type;
		};

		/**
		 * Used to order @a ObjectAddress keys in a std::map.
		 */
		struct SortObjectAddressPredicate :
				public std::binary_function<ObjectAddress, ObjectAddress, bool>
		{
			SortObjectAddressPredicate()
			{  }

			bool
			operator()(
					const ObjectAddress &lhs,
					const ObjectAddress &rhs) const
			{
				if (lhs.address < rhs.address)
				{
					return true;
				}

				if (lhs.address > rhs.address)
				{
					return false;
				}

				// The '!=' is because MSVC returns 'int' instead of 'bool'.
				return lhs.type->before(*rhs.type) != 0;
			}
		};


		namespace Implementation
		{
			//! Overload for polymorphic types - returns address of the entire (dynamic) object.
			template <typename ObjectType>
			ObjectAddress
			get_object_address(
					ObjectType *object_address,
					boost::mpl::true_/*is_polymorphic*/)
			{
				return ObjectAddress(
						dynamic_cast<void *>(object_address),
						typeid(*object_address));
			}

			//! Overload for non-polymorphic types - just returns the address passed in.
			template <typename ObjectType>
			ObjectAddress
			get_object_address(
					ObjectType *object_address,
					boost::mpl::false_/*is_polymorphic*/)
			{
				return ObjectAddress(
						static_cast<void *>(object_address),
						typeid(ObjectType));
			}
		}


		/**
		 * Returns the actual address associated with the specified object's address.
		 *
		 * For non-polymorphic types this just returns the address passed in.
		 *
		 * For polymorphic types this returns the address of the *entire* object using 'dynamic_cast'.
		 *
		 * Note that 'dynamic_cast' can differ from 'static_cast' when there's multiple inheritance.
		 * For example:
		 *
		 *   class A { virtual ~A() { } };
		 *   class B { virtual ~B() { } };
		 *   class D : public A, public B { };
		 *   D d;
		 *   B &b = d;
		 *   assert(static_cast<const void *>(&b) != dynamic_cast<const void *>(&b));
		 *
		 * ...because A and B are treated like sub-objects inside D, and B is the *second* sub-object
		 * and so has a different address than A (and hence D) and only 'dynamic_cast' knows this.
		 *
		 * In the following situation class B is no longer polymorphic:
		 *
		 *   class A { virtual ~A() { } };
		 *   class B { }; // NOTE: Class B is *not* polymorphic.
		 *   class D : public A, public B { };
		 *   D d;
		 *   B &b = d;
		 *   assert(static_cast<void *>(&b) != static_cast<void *>(&d));
		 *
		 * ...here we only have a reference to B we can't know whether it's actually pointing
		 * to a D object or not, and we can't use 'dynamic_cast' on it because it's not polymorphic.
		 *
		 *
		 * This function can also be used on the addresses of pointers and addresses
		 * of pointer-to-pointer's, etc.
		 * Note that pointer-to-pointer's (eg, "double **") can still be converted to addresses
		 * that do not specify the type that the pointer is pointing to (ie, converted to "void *")
		 * - this is just stating that even pointer-to-pointer's have a location in memory
		 * - the type being pointed to (eg, "double *") is replaced by "void".
		 */
		template <typename ObjectType>
		ObjectAddress
		get_dynamic_object_address(
				ObjectType *object_address)
		{
			if (object_address == NULL)
			{
				return ObjectAddress(typeid(ObjectType));
			}

			return Implementation::get_object_address(
					object_address,
					typename boost::is_polymorphic<ObjectType>::type());
		}


		/**
		 * Returns the static address - static cast to 'void *'.
		 */
		template <typename ObjectType>
		ObjectAddress
		get_static_object_address(
				ObjectType *object_address)
		{
			if (object_address == NULL)
			{
				return ObjectAddress(typeid(ObjectType));
			}

			return Implementation::get_object_address(
					object_address,
					boost::mpl::false_());
		}


		namespace Implementation
		{
			template <typename T, typename U>
			boost::shared_ptr<T>
			shared_ptr_cast(
					const boost::shared_ptr<U> &ptr,
					boost::mpl::true_)
			{
				return boost::dynamic_pointer_cast<T>(ptr);
			}

			template <typename T, typename U>
			boost::shared_ptr<T>
			shared_ptr_cast(
					const boost::shared_ptr<U> &ptr,
					boost::mpl::false_)
			{
				return boost::static_pointer_cast<T>(ptr);
			}
		}

		/**
		 * Cast a boost::shared_ptr using dynamic_cast for polymorphic types, otherwise static_cast.
		 */
		template <typename T, typename U>
		boost::shared_ptr<T>
		shared_ptr_cast(
				const boost::shared_ptr<U> &ptr)
		{
			return Implementation::shared_ptr_cast<T>(ptr, typename boost::is_polymorphic<U>::type());
		}


		/**
		 * Interface for loading/saving an object, allocated on the heap, via its pointer.
		 *
		 * This interface does not know the type of the object being loaded/saved.
		 */
		class TranscribeOwningPointer :
				public GPlatesUtils::ReferenceCount<TranscribeOwningPointer>
		{
		public:

			// Convenience typedefs for a shared pointer to a @a TranscribeOwningPointer.
			typedef GPlatesUtils::non_null_intrusive_ptr<TranscribeOwningPointer> non_null_ptr_type;
			typedef GPlatesUtils::non_null_intrusive_ptr<const TranscribeOwningPointer> non_null_ptr_to_const_type;

			virtual
			~TranscribeOwningPointer()
			{  }


			/**
			 * Saves the specified object (on the heap) to the archive.
			 *
			 * The object is expected to be the same type used by the derived @a TranscribeOwningPointerTemplate class.
			 *
			 * The object is associated with @a object_id.
			 */
			virtual
			void
			save_object(
					Scribe &scribe,
					void *object_ptr,
					object_id_type object_id,
					unsigned int options) const = 0;


			/**
			 * Creates a new object on the heap and loads it from the archive using @a object_id.
			 *
			 * The loaded object is the same type used by the derived @a TranscribeOwningPointerTemplate class.
			 *
			 * Ownership of the object naturally becomes the pointer referencing @a object_id
			 * (which is then responsible for calling 'delete').
			 *
			 * Returns true if transcribe was successful.
			 */
			virtual
			bool
			load_object(
					Scribe &scribe,
					object_id_type object_id,
					unsigned int options) const = 0;

		};


		/**
		 * Load/save an object, allocated on the heap, via its pointer.
		 */
		template <typename ObjectType>
		class TranscribeOwningPointerTemplate :
				public TranscribeOwningPointer
		{
		public:

			typedef TranscribeOwningPointerTemplate<ObjectType> this_type;

			// Convenience typedefs for a shared pointer to a 'TranscribeOwningPointerTemplate<ObjectType>'.
			typedef GPlatesUtils::non_null_intrusive_ptr<this_type> non_null_ptr_type;
			typedef GPlatesUtils::non_null_intrusive_ptr<const this_type> non_null_ptr_to_const_type;


			/**
			 * Creates an instance of 'TranscribeOwningPointerTemplate<ObjectType>'.
			 */
			static
			non_null_ptr_type
			create()
			{
				return non_null_ptr_type(new TranscribeOwningPointerTemplate());
			}


			/**
			 * Saves the specified object of type 'ObjectType' (on the heap) to the archive.
			 *
			 * Implementation is in "ScribeInternalUtilsImpl.h" since it includes heavyweight "Scribe.h".
			 */
			virtual
			void
			save_object(
					Scribe &scribe,
					void *object_ptr,
					object_id_type object_id,
					unsigned int options) const;


			/**
			 * Creates a new object on the heap and loads it from the archive using @a object_id.
			 *
			 * Implementation is in "ScribeInternalUtilsImpl.h" since it includes heavyweight "Scribe.h".
			 */
			virtual
			bool
			load_object(
					Scribe &scribe,
					object_id_type object_id,
					unsigned int options) const;

		private:

			TranscribeOwningPointerTemplate()
			{  }
		};


		/**
		 * Interface for responding to a relocation of a loaded object (to keep object tracking intact).
		 *
		 * This interface does not know the type of the object that was relocated.
		 */
		class Relocated :
				public GPlatesUtils::ReferenceCount<Relocated>
		{
		public:

			// Convenience typedefs for a shared pointer to a @a Relocated.
			typedef GPlatesUtils::non_null_intrusive_ptr<Relocated> non_null_ptr_type;
			typedef GPlatesUtils::non_null_intrusive_ptr<const Relocated> non_null_ptr_to_const_type;

			virtual
			~Relocated()
			{  }

			/**
			 * Notification from the Scribe that a previously transcribed (loaded) object has been
			 * moved to a new memory location.
			 */
			virtual
			void
			relocated(
					Scribe &scribe,
					const void *relocated_object,
					const void *transcribed_object) const = 0;

		};


		namespace Implementation
		{
			//
			// In order to get Argument Dependent Lookup (ADL) for the non-member 'relocated()' function,
			// based on the namespace in which 'ObjectType' is declared, we use a non-member
			// helper function to avoid the clash with same-named member function
			// 'RelocatedTemplate<ObjectType>::relocated()'.
			//
			template <typename ObjectType>
			void
			relocated_ADL(
					Scribe &scribe,
					const ObjectType &relocated_object,
					const ObjectType &transcribed_object)
			{
				// Note: We don't qualify the 'relocated()' call with GPlatesScribe. This allows clients
				// to implement 'relocated()' in a different namespace - the function is then found by
				// Argument Dependent Lookup (ADL) based on the namespace in which 'ObjectType' is declared.
				return relocated(scribe, relocated_object, transcribed_object);
			}
		}


		/**
		 * Delegates response (to a relocation of a loaded object) to the appropriate specialisation
		 * or overload (for 'ObjectType') of the non-member function 'relocated()'.
		 */
		template <typename ObjectType>
		class RelocatedTemplate :
				public Relocated
		{
		public:

			typedef RelocatedTemplate<ObjectType> this_type;

			// Convenience typedefs for a shared pointer to a 'RelocatedTemplate<ObjectType>'.
			typedef GPlatesUtils::non_null_intrusive_ptr<this_type> non_null_ptr_type;
			typedef GPlatesUtils::non_null_intrusive_ptr<const this_type> non_null_ptr_to_const_type;

			/**
			 * Creates an instance of 'RelocatedTemplate<ObjectType>'.
			 */
			static
			non_null_ptr_type
			create()
			{
				return non_null_ptr_type(new RelocatedTemplate());
			}

			/**
			 * Notification from the Scribe that a previously transcribed (loaded) object has been
			 * moved to a new memory location.
			 */
			virtual
			void
			relocated(
					Scribe &scribe,
					const void *relocated_object,
					const void *transcribed_object) const
			{
				// Call the *non-member* function 'relocated()' via Argument Dependent Lookup (ADL).
				Implementation::relocated_ADL(
						scribe,
						*static_cast<const ObjectType *>(relocated_object),
						*static_cast<const ObjectType *>(transcribed_object));
			}

		private:

			RelocatedTemplate()
			{  }
		};
	}
}

#endif // GPLATES_SCRIBE_SCRIBEINTERNALUTILS_H
