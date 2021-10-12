/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_APPLOGICUTILS_H
#define GPLATES_APP_LOGIC_APPLOGICUTILS_H

#include "model/FeatureCollectionHandle.h"


namespace GPlatesModel
{
	class FeatureVisitor;
}

namespace GPlatesAppLogic
{
	namespace AppLogicUtils
	{
		/**
		 * A convenience function for iterating over a the features in a
		 * @a GPlatesModel::FeatureCollectionHandle::weak_ref and visiting
		 * them with a @a GPlatesModel::FeatureVisitor.
		 */
		void
		visit_feature_collection(
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection,
				GPlatesModel::FeatureVisitor &visitor);


		/**
		 * A convenience function for iterating over a the features in a
		 * @a GPlatesModel::FeatureCollectionHandle::const_weak_ref and visiting
		 * them with a @a GPlatesModel::ConstFeatureVisitor.
		 */
		void
		visit_feature_collection(
				const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection,
				GPlatesModel::ConstFeatureVisitor &visitor);


		/**
		 * A convenience function for iterating over a the features in a
		 * @a GPlatesModel::FeatureCollectionHandle::weak_ref and visiting
		 * them with a @a GPlatesModel::ConstFeatureVisitor.
		 */
		void
		visit_feature_collection(
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection,
				GPlatesModel::ConstFeatureVisitor &visitor);


		/**
		 * A convenience function for iterating over a sequence of
		 * @a GPlatesModel::FeatureCollectionHandle::weak_ref objects and visiting
		 * them with a @a GPlatesModel::FeatureVisitor.
		 */
		template< typename FeatureCollectionWeakRefIterator >
		void
		visit_feature_collections(
				FeatureCollectionWeakRefIterator collections_begin, 
				FeatureCollectionWeakRefIterator collections_end,
				GPlatesModel::FeatureVisitor &visitor)
		{
			// We visit each of the features in each of the feature collections in
			// the given range.
			FeatureCollectionWeakRefIterator collections_iter = collections_begin;
			for ( ; collections_iter != collections_end; ++collections_iter)
			{
				visit_feature_collection(*collections_iter, visitor);
			}
		}


		/**
		 * A convenience function for iterating over a sequence of
		 * @a GPlatesModel::FeatureCollectionHandle::weak_ref or
		 * @a GPlatesModel::FeatureCollectionHandle::const_weak_ref objects and
		 * visiting them with a @a GPlatesModel::ConstFeatureVisitor.
		 */
		template< typename FeatureCollectionWeakRefIterator >
		void
		visit_feature_collections(
				FeatureCollectionWeakRefIterator collections_begin, 
				FeatureCollectionWeakRefIterator collections_end,
				GPlatesModel::ConstFeatureVisitor &visitor)
		{
			// We visit each of the features in each of the feature collections in
			// the given range.
			FeatureCollectionWeakRefIterator collections_iter = collections_begin;
			for ( ; collections_iter != collections_end; ++collections_iter)
			{
				// NOTE: 'collections_iter' can reference either a
				// 'GPlatesModel::FeatureCollectionHandle::weak_ref' or a
				// 'GPlatesModel::FeatureCollectionHandle::const_weak_ref' depending on
				// the type of the template parameter 'FeatureCollectionWeakRefIterator'.
				visit_feature_collection(*collections_iter, visitor);
			}
		}
	}
}

#endif // GPLATES_APP_LOGIC_APPLOGICUTILS_H
