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

#ifndef GPLATES_SCRIBE_SCRIBEVOIDCASTREGISTRY_H
#define GPLATES_SCRIBE_SCRIBEVOIDCASTREGISTRY_H

#include <map>
#include <typeinfo>
#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/pool/object_pool.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/type_traits/is_polymorphic.hpp>

#include "ScribeExceptions.h"
#include "ScribeInternalUtils.h"

#include "utils/IntrusiveSinglyLinkedList.h"
#include "utils/ReferenceCount.h"


namespace GPlatesScribe
{
	/**
	 * Handles casting 'void *' pointers from base to derived classes and vice versa.
	 *
	 * The inheritance links between base and derived classes must be registered for this to work.
	 */
	class VoidCastRegistry :
			private boost::noncopyable
	{
	public:

		/**
		 * Registers an inheritance link between the specified base and derived class types.
		 *
		 * If the link has been previously registered then nothing is done.
		 */
		template <class DerivedType, class BaseType>
		void
		register_derived_base_class_inheritance();


		/**
		 * Casts a 'void' pointer from a derived class to a base class.
		 *
		 * @a derived_object_address is expected to be a pointer to type @a derived_type.
		 *
		 * Throws exception Exceptions::AmbiguousCast if there is more than one inheritance path between
		 * the specified base and derived types. For example:
		 *
		 *  A   A
		 *  |   |
		 *  B   C
		 *   \ /
		 *    D
		 *
		 * ...will generate the exception between class D and class A.
		 * NOTE: A virtual inheritance diamond (only one 'A' sub-object) will also throw
		 * Exceptions::AmbiguousCast since we currently don't support diamonds.
		 *
		 * Return none if an inheritance path between the specified base and derived types cannot be found.
		 */
		boost::optional<void *>
		up_cast(
				const std::type_info &derived_type,
				const std::type_info &base_type,
				void *derived_object_address) const;


		/**
		 * Helper function for up-casting a boost::shared_ptr.
		 *
		 * This is necessary because Scribe treats boost::shared_ptr as a special type.
		 *
		 * Throws exception Exceptions::AmbiguousCast if there is more than one inheritance path between
		 * the specified base and derived types. For example:
		 *
		 *  A   A
		 *  |   |
		 *  B   C
		 *   \ /
		 *    D
		 *
		 * ...will generate the exception between class D and class A.
		 * NOTE: A virtual inheritance diamond (only one 'A' sub-object) will also throw
		 * Exceptions::AmbiguousCast since we currently don't support diamonds.
		 *
		 * Return none if an inheritance path between the specified base and derived types cannot be found.
		 */
		boost::optional< boost::shared_ptr<void> >
		up_cast(
				const std::type_info &derived_type,
				const std::type_info &base_type,
				const boost::shared_ptr<void> &derived_object_address) const;


		/**
		 * Casts a 'void' pointer from a base class to a derived class.
		 *
		 * @a base_object_address is expected to be a pointer to type @a base_type.
		 *
		 * Throws exception Exceptions::AmbiguousCast if there is more than one inheritance path between
		 * the specified base and derived types. For example:
		 *
		 *  A   A
		 *  |   |
		 *  B   C
		 *   \ /
		 *    D
		 *
		 * ...will generate the exception between class D and class A.
		 * NOTE: A virtual inheritance diamond (only one 'A' sub-object) will also throw
		 * Exceptions::AmbiguousCast since we currently don't support diamonds.
		 *
		 * Return none if an inheritance path between the specified base and derived types cannot be found.
		 */
		boost::optional<void *>
		down_cast(
				const std::type_info &derived_type,
				const std::type_info &base_type,
				void *base_object_address) const;


		/**
		 * Helper function for down-casting a boost::shared_ptr.
		 *
		 * This is necessary because Scribe treats boost::shared_ptr as a special type.
		 *
		 * Throws exception Exceptions::AmbiguousCast if there is more than one inheritance path between
		 * the specified base and derived types. For example:
		 *
		 *  A   A
		 *  |   |
		 *  B   C
		 *   \ /
		 *    D
		 *
		 * ...will generate the exception between class D and class A.
		 * NOTE: A virtual inheritance diamond (only one 'A' sub-object) will also throw
		 * Exceptions::AmbiguousCast since we currently don't support diamonds.
		 *
		 * Return none if an inheritance path between the specified base and derived types cannot be found.
		 */
		boost::optional< boost::shared_ptr<void> >
		down_cast(
				const std::type_info &derived_type,
				const std::type_info &base_type,
				const boost::shared_ptr<void> &base_object_address) const;

	private:

		struct ClassNode;
		struct ClassLink;

		//! Typedef for a mapping from class type info to @a ClassNode.
		typedef std::map<const std::type_info *, ClassNode *, InternalUtils::SortTypeInfoPredicate>
				class_type_info_to_node_map_type;

		//! Typedef for a mapping from class type info to @a ClassNode.
		typedef std::map<const std::type_info *, ClassLink *, InternalUtils::SortTypeInfoPredicate>
				class_type_info_to_link_map_type;

		/**
		 * Represents a class in the inheritance graph.
		 */
		struct ClassNode
		{
			explicit
			ClassNode(
					const std::type_info &class_type_info_) :
				class_type_info(&class_type_info_)
			{  }


			//! The class type info associated with this class node.
			const std::type_info *class_type_info;

			//! References to base class nodes (if any) accessed by class type info.
			class_type_info_to_link_map_type base_class_links;
		};

		/**
		 * Represents an inheritance link between two classes in the inheritance graph.
		 */
		struct ClassLink :
			public GPlatesUtils::ReferenceCount<ClassLink>,
			public GPlatesUtils::IntrusiveSinglyLinkedList<const ClassLink>::Node
		{

			// Convenience typedefs for a shared pointer to a @a ClassLink.
			typedef GPlatesUtils::non_null_intrusive_ptr<ClassLink> non_null_ptr_type;
			typedef GPlatesUtils::non_null_intrusive_ptr<const ClassLink> non_null_ptr_to_const_type;


			virtual
			~ClassLink()
			{  }

			virtual
			void *
			upcast(
					void *derived) const = 0;

			virtual
			boost::shared_ptr<void>
			upcast(
					const boost::shared_ptr<void> &derived) const = 0;

			virtual
			void *
			downcast(
					void *base) const = 0;

			virtual
			boost::shared_ptr<void>
			downcast(
					const boost::shared_ptr<void> &base) const = 0;

			ClassNode *derived_class_node;
			ClassNode *base_class_node;

		protected:

			ClassLink(
					ClassNode *derived_class_node_,
					ClassNode *base_class_node_) :
				derived_class_node(derived_class_node_),
				base_class_node(base_class_node_)
			{  }

		};

		template <class DerivedType, class BaseType>
		struct DerivedBaseClassLink :
				public ClassLink
		{

			// Convenience typedefs for a shared pointer to a @a DerivedBaseClassLink.
			typedef GPlatesUtils::non_null_intrusive_ptr<DerivedBaseClassLink> non_null_ptr_type;
			typedef GPlatesUtils::non_null_intrusive_ptr<const DerivedBaseClassLink> non_null_ptr_to_const_type;

			static
			non_null_ptr_type
			create(
					ClassNode *derived_class_node_,
					ClassNode *base_class_node_)
			{
				return non_null_ptr_type(new DerivedBaseClassLink(derived_class_node_, base_class_node_));
			}

			virtual
			void *
			upcast(
					void *derived) const
			{
				return upcast(derived, typename boost::is_polymorphic<DerivedType>::type());
			}

			virtual
			boost::shared_ptr<void>
			upcast(
					const boost::shared_ptr<void> &derived) const
			{
				return upcast(derived, typename boost::is_polymorphic<DerivedType>::type());
			}

			virtual
			void *
			downcast(
					void *base) const
			{
				return downcast(base, typename boost::is_polymorphic<BaseType>::type());
			}

			virtual
			boost::shared_ptr<void>
			downcast(
					const boost::shared_ptr<void> &base) const
			{
				return downcast(base, typename boost::is_polymorphic<BaseType>::type());
			}

		private:

			DerivedBaseClassLink(
					ClassNode *derived_class_node_,
					ClassNode *base_class_node_) :
				ClassLink(derived_class_node_, base_class_node_)
			{  }

			void *
			upcast(
					void *derived,
					boost::mpl::true_) const
			{
				BaseType *base = dynamic_cast<BaseType *>(static_cast<DerivedType *>(derived));
				if (base == NULL)
				{
					throw std::bad_cast();
				}
				return base;
			}

			void *
			upcast(
					void *derived,
					boost::mpl::false_) const
			{
				return static_cast<BaseType *>(static_cast<DerivedType *>(derived));
			}

			boost::shared_ptr<void>
			upcast(
					const boost::shared_ptr<void> &derived,
					boost::mpl::true_) const
			{
				boost::shared_ptr<BaseType> base = boost::dynamic_pointer_cast<BaseType>(
						boost::static_pointer_cast<DerivedType>(derived));
				if (!base)
				{
					throw std::bad_cast();
				}
				return base;
			}

			boost::shared_ptr<void>
			upcast(
					const boost::shared_ptr<void> &derived,
					boost::mpl::false_) const
			{
				return boost::static_pointer_cast<BaseType>(boost::static_pointer_cast<DerivedType>(derived));
			}

			void *
			downcast(
					void *base,
					boost::mpl::true_) const
			{
				DerivedType *derived = dynamic_cast<DerivedType *>(static_cast<BaseType *>(base));
				if (derived == NULL)
				{
					throw std::bad_cast();
				}
				return derived;
			}

			void *
			downcast(
					void *base,
					boost::mpl::false_) const
			{
				return static_cast<DerivedType *>(static_cast<BaseType *>(base));
			}

			boost::shared_ptr<void>
			downcast(
					const boost::shared_ptr<void> &base,
					boost::mpl::true_) const
			{
				boost::shared_ptr<DerivedType> derived = boost::dynamic_pointer_cast<DerivedType>(
						boost::static_pointer_cast<BaseType>(base));
				if (!derived)
				{
					throw std::bad_cast();
				}
				return derived;
			}

			boost::shared_ptr<void>
			downcast(
					const boost::shared_ptr<void> &base,
					boost::mpl::false_) const
			{
				return boost::static_pointer_cast<DerivedType>(boost::static_pointer_cast<BaseType>(base));
			}
		};

		//! Typedef for a linked list of class links.
		typedef GPlatesUtils::IntrusiveSinglyLinkedList<const ClassLink> class_links_list_type;

		//! Typedef for a pool of @a ClassNode objects.
		typedef boost::object_pool<ClassNode> class_node_pool_type;

		//! Typedef for a sequence of class links.
		typedef std::vector<ClassLink::non_null_ptr_type> link_seq_type;


		class_node_pool_type d_class_node_storage;
		link_seq_type d_class_link_storage;
		class_type_info_to_node_map_type d_class_type_info_to_node_map;


		/**
		 * Gets, or creates if doesn't exist, the class node for the specified class type info.
		 */
		ClassNode *
		get_or_create_class_node(
				const std::type_info &class_type_info);

		/**
		 * Creates, if necessary, a class link between the specified derived and base class nodes.
		 */
		template <class DerivedType, class BaseType>
		void
		create_class_link_if_necessary(
				ClassNode *derived_class_node,
				ClassNode *base_class_node);

		/**
		 * Used when recursively searching for a path from derived type to a base type.
		 */
		bool
		find_derived_to_base_path(
				class_links_list_type &derived_to_base_path,
				const std::type_info &derived_type,
				const std::type_info &base_type) const;

		/**
		 * Used when recursively searching for a path from derived type to a base type.
		 */
		bool
		find_derived_to_base_path(
				class_links_list_type &derived_to_base_path,
				const ClassNode &current_class_node,
				const std::type_info &derived_type,
				const std::type_info &base_type) const;

		/**
		 * Reverses the derived-to-base path to get the base-to-derived path.
		 */
		void
		get_base_to_derived_path(
				class_links_list_type &base_to_derived_path,
				class_links_list_type::const_iterator derived_to_base_path_iter,
				class_links_list_type::const_iterator derived_to_base_path_end) const;
	};
}


//
// Implementation
//


namespace GPlatesScribe
{
	template <class DerivedType, class BaseType>
	void
	VoidCastRegistry::register_derived_base_class_inheritance()
	{
		const std::type_info &derived_class_type_info = typeid(DerivedType);
		const std::type_info &base_class_type_info = typeid(BaseType);

		ClassNode *derived_class_node = get_or_create_class_node(derived_class_type_info);
		ClassNode *base_class_node = get_or_create_class_node(base_class_type_info);

		create_class_link_if_necessary<DerivedType, BaseType>(derived_class_node, base_class_node);
	}


	template <class DerivedType, class BaseType>
	void
	VoidCastRegistry::create_class_link_if_necessary(
			ClassNode *derived_class_node,
			ClassNode *base_class_node)
	{
		// Attempt to insert the base class link into the derived class node.
		std::pair<class_type_info_to_link_map_type::iterator, bool> base_class_link_inserted =
				derived_class_node->base_class_links.insert(
						class_type_info_to_link_map_type::value_type(
								base_class_node->class_type_info,
								static_cast<ClassLink *>(NULL)/*dummy*/));

		// If base class link was inserted (ie, didn't previously exist in the map)...
		if (base_class_link_inserted.second)
		{
			// Create a ClassLink between the derived and base class nodes.
			ClassLink::non_null_ptr_type derived_to_base_class_link =
					DerivedBaseClassLink<DerivedType, BaseType>::create(
							derived_class_node,
							base_class_node);
			d_class_link_storage.push_back(derived_to_base_class_link);

			// Point the map entry to the new class link.
			base_class_link_inserted.first->second = derived_to_base_class_link.get();
		}
	}
}

#endif // GPLATES_SCRIBE_SCRIBEVOIDCASTREGISTRY_H
