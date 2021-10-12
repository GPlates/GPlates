/* $Id$ */

/**
 * \file 
 * Contains the definition of the FeatureStoreRootHandle class.
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

#ifndef GPLATES_MODEL_FEATURESTOREROOTHANDLE_H
#define GPLATES_MODEL_FEATURESTOREROOTHANDLE_H

#include <boost/scoped_ptr.hpp>

#include "BasicHandle.h"
#include "FeatureStoreRootRevision.h"

#include "global/PointerTraits.h"
#include "utils/ReferenceCount.h"

namespace GPlatesModel
{
	/**
	 * A feature store root handle acts as a persistent handle to the revisioned content of a
	 * conceptual feature store root.
	 *
	 * The feature store root is the top layer/component of the three-tiered conceptual
	 * hierarchy of revisioned objects contained in, and managed by, the feature store:  It is
	 * the "root" node of the tree of revisioned objects.  The feature store contains a single
	 * feature store root, which in turn contains all the currently-loaded feature collections
	 * (each of which corresponds to a single data file).  Every currently-loaded feature is
	 * contained within a currently-loaded feature collection.
	 *
	 * The conceptual feature store root is implemented in two pieces: FeatureStoreRootHandle
	 * and FeatureStoreRootRevision.  A FeatureStoreRootHandle instance contains and manages a
	 * FeatureStoreRootRevision instance, which in turn contains the revisioned content of the
	 * conceptual feature store root.  A FeatureStoreRootHandle instance is contained within,
	 * and managed by, a FeatureStore instance.
	 *
	 * A feature store root handle instance is "persistent" in the sense that it will endure,
	 * in the same memory location, for as long as the conceptual feature store root exists
	 * (which will be determined by the lifetime of the feature store).  The revisioned content
	 * of the conceptual feature store root will be contained within a succession of feature
	 * store root revisions (with a new revision created as the result of every modification),
	 * but the handle will endure as a persistent means of accessing the current revision and
	 * the content within it.
	 */
	class FeatureStoreRootHandle :
			public BasicHandle<FeatureStoreRootHandle>,
			public GPlatesUtils::ReferenceCount<FeatureStoreRootHandle>
	{

	public:

		/**
		 * The type of this class.
		 */
		typedef FeatureStoreRootHandle this_type;

		/**
		 * Creates a new FeatureStoreRootHandle instance.
		 */
		static
		const non_null_ptr_type
		create();

	private:

		/**
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		FeatureStoreRootHandle();

		/**
		 * This constructor should not be defined, because we don't want to be able
		 * to copy construct one of these objects.
		 */
		FeatureStoreRootHandle(
				const this_type &other);

		/**
		 * This should not be defined, because we don't want to be able to copy
		 * one of these objects.
		 */
		this_type &
		operator=(
				const this_type &);

		friend class Model;

	};

}

// This include is not necessary for this header to function, but it would be
// convenient if client code could include this header and be able to use
// iterator or const_iterator without having to separately include the
// following header. It isn't placed above with the other includes because of
// cyclic dependencies.
#include "RevisionAwareIterator.h"

#endif  // GPLATES_MODEL_FEATURESTOREROOTHANDLE_H
