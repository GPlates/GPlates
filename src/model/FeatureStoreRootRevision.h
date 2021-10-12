/* $Id$ */

/**
 * \file 
 * Contains the definition of the FeatureStoreRootRevision class.
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

#ifndef GPLATES_MODEL_FEATURESTOREROOTREVISION_H
#define GPLATES_MODEL_FEATURESTOREROOTREVISION_H

#include "BasicRevision.h"
#include "FeatureCollectionHandle.h"

#include "global/PointerTraits.h"
#include "utils/ReferenceCount.h"

namespace GPlatesModel
{
	class FeatureStoreRootHandle;

	/**
	 * A feature store root revision contains the revisioned content of a conceptual feature
	 * store root.
	 *
	 * The feature store root is the top layer/component of the three-tiered conceptual
	 * hierarchy of revisioned objects contained in, and managed by, the feature store:  It is
	 * the "root" node of the tree of revisioned objects.  The feature store contains a single
	 * feature store root, which in turn contains all the currently-loaded feature collections
	 * (each of which corresponds to a single data file).  Every currently-loaded feature is
	 * contained within a currently-loaded feature collection.
	 *
	 * The conceptual feature store root is implemented in two pieces: FeatureStoreRootHandle
	 * and FeatureStoreRootRevision.  A FeatureStoreRootRevision instance contains the
	 * revisioned content of the conceptual feature store root, and is in turn referenced by
	 * either a FeatureStoreRootHandle instance or a TransactionItem instance.
	 *
	 * A new instance of FeatureStoreRootRevision will be created whenever the conceptual
	 * feature store root is modified by the addition or removal of feature-collection elements
	 * -- a new instance of FeatureStoreRootRevision is created, because the existing
	 * ("current") FeatureStoreRootRevision instance will not be modified.  The newly-created
	 * FeatureStoreRootRevision instance will then be "scheduled" in a TransactionItem.  When
	 * the TransactionItem is "committed", the pointer (in the TransactionItem) to the new
	 * FeatureStoreRootRevision instance will be swapped with the pointer (in the
	 * FeatureStoreRootHandle instance) to the "current" instance, so that the "new" instance
	 * will now become the "current" instance (referenced by the pointer in the
	 * FeatureStoreRootHandle) and the "current" instance will become the "old" instance
	 * (referenced by the pointer in the now-committed TransactionItem).
	 *
	 * Client code should not reference FeatureStoreRootRevision instances directly; rather, it
	 * should always access the "current" instance (whichever FeatureStoreRootRevision instance
	 * it may be) through the feature store root handle.
	 */
	class FeatureStoreRootRevision :
			public BasicRevision<FeatureStoreRootHandle>,
			public GPlatesUtils::ReferenceCount<FeatureStoreRootRevision>
	{

	public:

		/**
		 * The type of this class.
		 */
		typedef FeatureStoreRootRevision this_type;

		/**
		 * Creates a new FeatureStoreRootRevision instance.
		 */
		static
		const non_null_ptr_type
		create();

	private:

		/**
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		FeatureStoreRootRevision();

		/**
		 * This constructor should not be defined, because we don't want to be able
		 * to copy construct one of these objects.
		 */
		FeatureStoreRootRevision(
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

#endif  // GPLATES_MODEL_FEATURESTOREROOTREVISION_H
