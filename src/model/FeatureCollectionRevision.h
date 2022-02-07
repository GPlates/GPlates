/* $Id$ */

/**
 * \file 
 * Contains the definition of the FeatureCollectionRevision class.
 *
 * Most recent change:
 *   $Date$
 *
 * Copyright (C) 2006, 2007, 2009, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_FEATURECOLLECTIONREVISION_H
#define GPLATES_MODEL_FEATURECOLLECTIONREVISION_H

#include "BasicRevision.h"
#include "FeatureHandle.h"

#include "global/PointerTraits.h"
#include "utils/ReferenceCount.h"

namespace GPlatesModel
{
	class FeatureCollectionHandle;

	/**
	 * A feature collection revision contains the revisioned content of a conceptual feature
	 * collection.
	 *
	 * The feature collection is the middle layer/component of the three-tiered conceptual
	 * hierarchy of revisioned objects contained in, and managed by, the feature store:  The
	 * feature collection aggregates a set of features into a collection which may be loaded,
	 * saved or unloaded in a single operation.  The feature store contains a single feature
	 * store root, which in turn contains all the currently-loaded feature collections.  Every
	 * currently-loaded feature is contained within a currently-loaded feature collection.
	 *
	 * The conceptual feature collection is implemented in two pieces: FeatureCollectionHandle
	 * and FeatureCollectionRevision.  A FeatureCollectionRevision instance contains the
	 * revisioned content of the conceptual feature collection, and is in turn referenced by
	 * either a FeatureCollectionHandle instance or a TransactionItem instance.
	 *
	 * A new instance of FeatureCollectionRevision will be created whenever the conceptual
	 * feature collection is modified by the addition or removal of feature elements -- a new
	 * instance of FeatureCollectionRevision is created, because the existing ("current")
	 * FeatureCollectionRevision instance will not be modified.  The newly-created
	 * FeatureCollectionRevision instance will then be "scheduled" in a TransactionItem.  When
	 * the TransactionItem is "committed", the pointer (in the TransactionItem) to the new
	 * FeatureCollectionRevision instance will be swapped with the pointer (in the
	 * FeatureCollectionHandle instance) to the "current" instance, so that the "new" instance
	 * will now become the "current" instance (referenced by the pointer in the
	 * FeatureCollectionHandle) and the "current" instance will become the "old" instance
	 * (referenced by the pointer in the now-committed TransactionItem).
	 *
	 * Client code should not reference FeatureCollectionRevision instances directly; rather,
	 * it should always access the "current" instance (whichever FeatureCollectionRevision
	 * instance it may be) through the feature collection handle.
	 */
	class FeatureCollectionRevision :
			public BasicRevision<FeatureCollectionHandle>,
			public GPlatesUtils::ReferenceCount<FeatureCollectionRevision>
	{

	public:

		/**
		 * The type of this class.
		 */
		typedef FeatureCollectionRevision this_type;

		/**
		 * Creates a new FeatureCollectionRevision instance.
		 */
		static
		const non_null_ptr_type
		create();

	private:

		/**
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		FeatureCollectionRevision();

		/**
		 * This constructor should not be defined, because we don't want to be able
		 * to copy construct one of these objects.
		 */
		FeatureCollectionRevision(
				const this_type &other);

		/**
		 * This should not be defined, because we don't want to be able to copy
		 * one of these objects.
		 */
		this_type &
		operator=(
				const this_type &);

	};

}


#endif  // GPLATES_MODEL_FEATURECOLLECTIONREVISION_H
