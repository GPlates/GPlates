/* $Id$ */

/**
 * \file 
 * Contains the definition of the class HandleTraits.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_HANDLETRAITS_H
#define GPLATES_MODEL_HANDLETRAITS_H

#include <boost/intrusive_ptr.hpp>

#include "global/PointerTraits.h"

namespace GPlatesModel
{
	class FeatureHandle;
	class FeatureRevision;
	class FeatureCollectionHandle;
	class FeatureCollectionRevision;
	class FeatureStoreRootHandle;
	class FeatureStoreRootRevision;
	class Model;
	template<class H> class RevisionAwareIterator;
	class TopLevelProperty;
	class TopLevelPropertyRef;
	template<class H> class WeakReference;

	namespace HandleTraitsInternals
	{
		/**
		 * Contains typedefs common to all HandleTraits.
		 */
		template<class HandleType>
		struct HandleTraitsBase
		{
			/**
			 * Typedef for GPlatesGlobal::PointerTraits<HandleType>::non_null_ptr_type.
			 */
			typedef typename GPlatesGlobal::PointerTraits<HandleType>::non_null_ptr_type non_null_ptr_type;

			/**
			 * Typedef for GPlatesGlobal::PointerTraits<const HandleType>::non_null_ptr_type.
			 */
			typedef typename GPlatesGlobal::PointerTraits<const HandleType>::non_null_ptr_type non_null_ptr_to_const_type;

			/**
			 * Typedef for WeakReference<HandleType>.
			 */
			typedef WeakReference<HandleType> weak_ref;

			/**
			 * Typedef for WeakReference<const HandleType>.
			 */
			typedef WeakReference<const HandleType> const_weak_ref;

			/**
			 * Typedef for RevisionAwareIterator<HandleType>.
			 */
			typedef RevisionAwareIterator<HandleType> iterator;

			/**
			 * Typedef for RevisionAwareIterator<const HandleType>.
			 */
			typedef RevisionAwareIterator<const HandleType> const_iterator;
		};


		/**
		 * A policy class that indicates that a handle type stores an unsaved
		 * changes flag.
		 */
		class WithUnsavedChangesFlag
		{
		public:

			bool
			contains_unsaved_changes() const
			{
				return d_contains_unsaved_changes;
			}

			void
			clear_unsaved_changes()
			{
				d_contains_unsaved_changes = false;
			}

		protected:

			WithUnsavedChangesFlag() :
				d_contains_unsaved_changes(false)
			{  }

			~WithUnsavedChangesFlag()
			{  }

			void
			set_unsaved_changes()
			{
				d_contains_unsaved_changes = true;
			}
			
		private:

			bool d_contains_unsaved_changes;
		};


		/**
		 * A policy class that indicates that a handle type does not store an
		 * unsaved changes flag.
		 */
		class WithoutUnsavedChangesFlag
		{
		protected:

			WithoutUnsavedChangesFlag()
			{  }

			~WithoutUnsavedChangesFlag()
			{  }

			void
			set_unsaved_changes()
			{
				// Do nothing.
			}
		};
	}

	/**
	 * HandleTraits is a traits class to provide type information about
	 * FeatureHandle, FeatureCollectionHandle and FeatureStoreRootHandle.
	 *
	 * It is useful in situations where you want to find out type information
	 * about one of the Handle classes without including the header file for that Handle.
	 */
	template<class HandleType>
	struct HandleTraits
	{
	};

	/**
	 * Specialisation of HandleTraits for FeatureHandle.
	 */
	template<>
	struct HandleTraits<FeatureHandle> :
			public HandleTraitsInternals::HandleTraitsBase<FeatureHandle>
	{
		/**
		 * Typedef for FeatureRevision, the corresponding revision type to FeatureHandle.
		 */
		typedef FeatureRevision revision_type;

		/**
		 * Typedef for FeatureCollectionHandle, the type one level above FeatureHandle
		 * in the tree of nodes.
		 */
		typedef FeatureCollectionHandle parent_type;

		/**
		 * Typedef for TopLevelProperty, the type one level below FeatureHandle
		 * in the tree of nodes.
		 */
		typedef TopLevelProperty child_type;

		/**
		 * Typedef for TopLevelPropertyRef, the type returned on dereference of the
		 * FeatureHandle non-const iterator.
		 */
		typedef TopLevelPropertyRef iterator_value_type;

		/**
		 * Typedef for PointerTraits<const TopLevelProperty>::non_null_ptr_type, the type
		 * returned on dereference of the FeatureHandle const iterator.
		 */
		typedef GPlatesGlobal::PointerTraits<const TopLevelProperty>::non_null_ptr_type const_iterator_value_type;

		/**
		 * FeatureHandles don't have an unsaved changes flag.
		 */
		typedef HandleTraitsInternals::WithoutUnsavedChangesFlag unsaved_changes_flag_policy;
	};

	template<>
	struct HandleTraits<const FeatureHandle> :
			public HandleTraits<FeatureHandle>
	{
	};

	/**
	 * Specialisation of HandleTraits for FeatureCollectionHandle.
	 */
	template<>
	struct HandleTraits<FeatureCollectionHandle> :
			public HandleTraitsInternals::HandleTraitsBase<FeatureCollectionHandle>
	{
		/**
		 * Typedef for FeatureCollectionRevision, the corresponding revision type to FeatureCollectionHandle.
		 */
		typedef FeatureCollectionRevision revision_type;

		/**
		 * Typedef for FeatureStoreRootHandle, the type one level above FeatureCollectionHandle
		 * in the tree of nodes.
		 */
		typedef FeatureStoreRootHandle parent_type;

		/**
		 * Typedef for FeatureHandle, the type one level below FeatureCollectionHandle
		 * in the tree of nodes.
		 */
		typedef FeatureHandle child_type;

		/**
		 * Typedef for PointerTraits<FeatureHandle>::non_null_ptr_type, the type
		 * returned on dereference of the FeatureCollectionHandle non-const iterator.
		 */
		typedef GPlatesGlobal::PointerTraits<FeatureHandle>::non_null_ptr_type iterator_value_type;

		/**
		 * Typedef for PointerTraits<const FeatureHandle>::non_null_ptr_type, the type
		 * returned on dereference of the FeatureCollectionHandle const iterator.
		 */
		typedef GPlatesGlobal::PointerTraits<const FeatureHandle>::non_null_ptr_type const_iterator_value_type;

		/**
		 * FeatureCollectionHandles have an unsaved changes flag.
		 */
		typedef HandleTraitsInternals::WithUnsavedChangesFlag unsaved_changes_flag_policy;
	};

	template<>
	struct HandleTraits<const FeatureCollectionHandle> :
			public HandleTraits<FeatureCollectionHandle>
	{
	};

	/**
	 * Specialisation of HandleTraits for FeatureStoreRootHandle.
	 */
	template<>
	struct HandleTraits<FeatureStoreRootHandle> :
			public HandleTraitsInternals::HandleTraitsBase<FeatureStoreRootHandle>
	{
		/**
		 * Typedef for FeatureStoreRootRevision, the corresponding revision type to FeatureStoreRootHandle.
		 */
		typedef FeatureStoreRootRevision revision_type;

		/**
		 * Typedef for Model, the type one level above FeatureStoreRootHandle
		 * in the tree of nodes.
		 */
		typedef Model parent_type;

		/**
		 * Typedef for FeatureCollectionHandle, the type one level below FeatureStoreRootHandle
		 * in the tree of nodes.
		 */
		typedef FeatureCollectionHandle child_type;

		/**
		 * Typedef for PointerTraits<FeatureCollectionHandle>::non_null_ptr_type, the type
		 * returned on dereference of the FeatureStoreRootHandle non-const iterator.
		 */
		typedef GPlatesGlobal::PointerTraits<FeatureCollectionHandle>::non_null_ptr_type iterator_value_type;

		/**
		 * Typedef for PointerTraits<const FeatureCollectionHandle>::non_null_ptr_type, the type
		 * returned on dereference of the FeatureStoreRoothandle const iterator.
		 */
		typedef GPlatesGlobal::PointerTraits<const FeatureCollectionHandle>::non_null_ptr_type const_iterator_value_type;
		
		/**
		 * FeatureStoreRootHandles don't have an unsaved changes flag.
		 */
		typedef HandleTraitsInternals::WithoutUnsavedChangesFlag unsaved_changes_flag_policy;
	};

	template<>
	struct HandleTraits<const FeatureStoreRootHandle> :
			public HandleTraits<FeatureStoreRootHandle>
	{
	};

}

#endif  // GPLATES_MODEL_HANDLETRAITS_H
