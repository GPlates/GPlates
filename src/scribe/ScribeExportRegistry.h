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

#ifndef GPLATES_SCRIBE_SCRIBEEXPORTREGISTRY_H
#define GPLATES_SCRIBE_SCRIBEEXPORTREGISTRY_H

#include <map>
#include <string>
#include <typeinfo>
#include <boost/mpl/not.hpp>
#include <boost/optional.hpp>
#include <boost/pool/object_pool.hpp>
#include <boost/ref.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_abstract.hpp>

#include "ScribeExceptions.h"
#include "ScribeInternalUtils.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/Singleton.h"


namespace GPlatesScribe
{
	/**
	 * Export registered information for a class type.
	 */
	class ExportClassType
	{
	public:

		ExportClassType(
				const std::string &type_id_name_,
				const std::type_info &type_info_,
				const InternalUtils::TranscribeOwningPointer::non_null_ptr_to_const_type &transcribe_owning_pointer_) :
			type_id_name(type_id_name_),
			type_info(type_info_),
			transcribe_owning_pointer(transcribe_owning_pointer_)
		{  }

		std::string type_id_name;
		boost::reference_wrapper<const std::type_info> type_info;
		InternalUtils::TranscribeOwningPointer::non_null_ptr_to_const_type transcribe_owning_pointer;
	};


	/**
	 * Used to register types to the scribe system so that they can be transcribed through
	 * base class pointers (ie, where the pointer dereference type is not the actual object type)
	 * and transcribed as stored types inside boost::variant.
	 */
	class ExportRegistry :
			public GPlatesUtils::Singleton<ExportRegistry>
	{
		GPLATES_SINGLETON_CONSTRUCTOR_DEF(ExportRegistry)

	public:

		/**
		 * Registers a class type.
		 *
		 * To register a class add an entry (see 'ScribeExportRegistration.h') instead of calling this directly.
		 */
		template <class Type>
		const ExportClassType &
		register_class_type(
				const char *class_id_name);


		/**
		 * Returns the registered class type associated with the specified class id name.
		 *
		 * Returns boost::none if the class type has not been registered.
		 */
		boost::optional<const ExportClassType &>
		get_class_type(
				const std::string &class_id_name) const;


		/**
		 * Returns the registered class type associated with the specified class type info.
		 *
		 * Returns boost::none if the class type has not been registered.
		 */
		boost::optional<const ExportClassType &>
		get_class_type(
				const std::type_info &class_type_info) const;

	public: // NOTE: This is only used for testing purposes...

		/**
		 * Unregisters a class type.
		 *
		 * NOTE: This is only used for testing purposes.
		 */
		template <class Type>
		ExportClassType
		unregister_class_type();

	private:

		//! Typedef for an object pool for @a ExportClassType.
		typedef boost::object_pool<ExportClassType> class_type_pool_type;

		//! Typedef for a mapping from class type info to @a ExportClassType.
		typedef std::map<const std::type_info *, const ExportClassType *, InternalUtils::SortTypeInfoPredicate>
				class_type_info_to_type_map_type;

		//! Typedef for a mapping from class id name to @a ExportClassType.
		typedef std::map<std::string, const ExportClassType *> class_id_name_to_type_map_type;


		//! Pool allocator for @a ExportClassType objects.
		class_type_pool_type d_class_type_pool;

		//! For searching @a ExportClassType by class type info.
		class_type_info_to_type_map_type d_class_type_info_to_type_map;

		//! For searching @a ExportClassType by class id name.
		class_id_name_to_type_map_type d_class_id_name_to_type_map;
	};
}


//
// Implementation
//

// Avoid cyclic header dependency.
#include "ScribeInternalUtilsImpl.h"

namespace GPlatesScribe
{
	template <typename Type>
	const ExportClassType &
	ExportRegistry::register_class_type(
			const char *class_id_name)
	{
		// Compile-time assertion to ensure not registering abstract types (they can't be instantiated).
		//
		// NOTE: Boost recommends we implement alternative to boost::is_abstract if BOOST_NO_IS_ABSTRACT
		// is defined - however we use compilers that support this (ie, MSVC8, gcc 4 and above).
#ifdef BOOST_NO_IS_ABSTRACT
		// Evaluates to false - but is dependent on template parameter - compiler workaround...
		BOOST_STATIC_ASSERT(sizeof(Type) == 0);
#endif
		BOOST_STATIC_ASSERT(boost::mpl::not_< boost::is_abstract<Type> >::value);


		// Attempt to insert the class type into a map using the class id name as a key.
		std::pair<class_id_name_to_type_map_type::iterator, bool> class_id_name_inserted =
				d_class_id_name_to_type_map.insert(
						class_id_name_to_type_map_type::value_type(
								class_id_name,
								static_cast<const ExportClassType *>(NULL)/*dummy*/));

		// If the class id name has already been inserted then this is OK since it probably
		// means the client put two identical entries (see 'ScribeExportRegistration.h').
		// However, throw an exception if the previously registered class id name has a different class type.
		// This can occur if two different classes have been given the same class id name identifier string.
		if (!class_id_name_inserted.second)
		{
			const ExportClassType &registered_class_type = *class_id_name_inserted.first->second;
			GPlatesGlobal::Assert<Exceptions::ExportRegisteredMultipleClassTypesWithSameClassName>(
					registered_class_type.type_info.get() == typeid(Type),
					GPLATES_ASSERTION_SOURCE,
					class_id_name,
					typeid(Type),
					registered_class_type.type_info);

			return registered_class_type;
		}

		// Add the class type to the object pool.
		// Seems boost::object_pool<>::construct() is limited to 3 constructor arguments in its
		// default compiled mode - so, since we have 4 arguments, we use copy constructor instead.
		const ExportClassType *registered_class_type = d_class_type_pool.construct(
				ExportClassType(
						class_id_name,
						boost::cref(typeid(Type))/*type_info*/,
						InternalUtils::TranscribeOwningPointerTemplate<Type>::create()));

		// Store the registered class type in the class id name map.
		class_id_name_inserted.first->second = registered_class_type;

		// Insert the class type into a map using the class type info as a key.
		std::pair<class_type_info_to_type_map_type::iterator, bool> class_type_info_inserted =
				d_class_type_info_to_type_map.insert(
						class_type_info_to_type_map_type::value_type(
								registered_class_type->type_info.get_pointer(),
								registered_class_type));

		// Throw an exception if the 'std::type_info' for the specified class type has already been registered.
		if (!class_type_info_inserted.second)
		{
			// Revert the class registration before throwing exception.
			d_class_id_name_to_type_map.erase(class_id_name_inserted.first);

			GPlatesGlobal::Assert<Exceptions::ExportRegisteredMultipleClassNamesWithSameClassType>(
					false,
					GPLATES_ASSERTION_SOURCE,
					typeid(Type),
					class_id_name,
					registered_class_type->type_id_name);
		}

		return *registered_class_type;
	}


	template <class Type>
	ExportClassType
	ExportRegistry::unregister_class_type()
	{
		const std::type_info &type_info = typeid(Type);

		class_type_info_to_type_map_type::iterator class_type_info_iter =
				d_class_type_info_to_type_map.find(&type_info);

		GPlatesGlobal::Assert<Exceptions::UnregisteredClassType>(
				class_type_info_iter != d_class_type_info_to_type_map.end(),
				GPLATES_ASSERTION_SOURCE,
				type_info);

		const ExportClassType export_class_type = *class_type_info_iter->second;

		class_id_name_to_type_map_type::iterator class_id_name_iter =
				d_class_id_name_to_type_map.find(export_class_type.type_id_name);

		// If found class type info then should also be able to find class id name.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				class_id_name_iter != d_class_id_name_to_type_map.end(),
				GPLATES_ASSERTION_SOURCE);

		// Unregister.
		d_class_type_info_to_type_map.erase(class_type_info_iter);
		d_class_id_name_to_type_map.erase(class_id_name_iter);

		return export_class_type;
	}
}

#endif // GPLATES_SCRIBE_SCRIBEEXPORTREGISTRY_H
