/* $Id$ */

/**
 * \file 
 * Contains the definition of the FeatureCollectionHandle class.
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

#ifndef GPLATES_MODEL_FEATURECOLLECTIONHANDLE_H
#define GPLATES_MODEL_FEATURECOLLECTIONHANDLE_H

#include <string>
#include <map>
#include <boost/any.hpp>
#include <boost/optional.hpp>
#include <boost/scoped_ptr.hpp>

#include "BasicHandle.h"
#include "FeatureCollectionRevision.h"
#include "ModelInterface.h"

#include "global/PointerTraits.h"

#include "scribe/Transcribe.h"
#include "scribe/TranscribeContext.h"

#include "utils/ReferenceCount.h"


namespace GPlatesModel
{
	/**
	 * A feature collection handle acts as a persistent handle to the revisioned content of a
	 * conceptual feature collection.
	 *
	 * The feature collection is the middle layer/component of the three-tiered conceptual
	 * hierarchy of revisioned objects contained in, and managed by, the feature store:  The
	 * feature collection aggregates a set of features into a collection which may be loaded,
	 * saved or unloaded in a single operation.  The feature store contains a single feature
	 * store root, which in turn contains all the currently-loaded feature collections.  Every
	 * currently-loaded feature is contained within a currently-loaded feature collection.
	 *
	 * The conceptual feature collection is implemented in two pieces: FeatureCollectionHandle
	 * and FeatureCollectionRevision.  A FeatureCollectionHandle instance contains and manages
	 * a FeatureCollectionRevision instance, which in turn contains the revisioned content of
	 * the conceptual feature collection.  A FeatureCollectionHandle instance is contained
	 * within, and managed by, a FeatureStoreRootRevision instance.
	 *
	 * A feature collection handle instance is "persistent" in the sense that it will endure,
	 * in the same memory location, for as long as the conceptual feature collection exists
	 * (which will be determined by the user's choice of when to "flush" deleted features and
	 * unloaded feature collections, after the feature collection has been unloaded).  The
	 * revisioned content of the conceptual feature collection will be contained within a
	 * succession of feature collection revisions (with a new revision created as the result of
	 * every modification), but the handle will endure as a persistent means of accessing the
	 * current revision and the content within it.
	 *
	 * The name "feature collection" derives from the GML term for a collection of GML features
	 * -- one GML feature collection corresponds roughly to one data file, although it may be
	 * the transient result of a database query, for instance, rather than necessarily a file
	 * saved on disk.
	 */
	class FeatureCollectionHandle :
			public BasicHandle<FeatureCollectionHandle>,
			public GPlatesUtils::ReferenceCount<FeatureCollectionHandle>
	{

	public:

		/**
		 * The type of this class.
		 */
		typedef FeatureCollectionHandle this_type;

		/**
		 * The type of the collection of metadata.
		 */
		typedef std::map<std::string, boost::any> tags_type;

		/**
		 * Creates a new FeatureCollectionHandle instance.
		 *
		 * This new FeatureCollectionHandle instance is not in the model. It is the
		 * responsibility of the caller to add it into a FeatureStoreRootHandle if
		 * that is desired.
		 */
		static
		const non_null_ptr_type
		create();

		/**
		 * Creates a new FeatureCollectionHandle instance.
		 *
		 * This new FeatureCollectionHandle instance is added to
		 * @a feature_store_root and a weak-ref is to the new instance is returned.
		 */
		static
		const weak_ref
		create(
				const WeakReference<FeatureStoreRootHandle> &feature_store_root);

		/**
		 * Returns the collection of miscellaneous metadata associated with this
		 * feature collection.
		 */
		tags_type &
		tags();

		/**
		 * Returns the collectino of miscellaneous metadata associated with this
		 * feature collection.
		 */
		const tags_type &
		tags() const;

	private:

		/**
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		FeatureCollectionHandle();

		/**
		 * This constructor should not be defined, because we don't want to be able
		 * to copy construct one of these objects.
		 */
		FeatureCollectionHandle(
				const this_type &other);
		
		/**
		 * This should not be defined, because we don't want to be able to copy
		 * one of these objects.
		 */
		this_type &
		operator=(
				const this_type &);

		/**
		 * A miscellaneous collection of metadata associated with this feature collection.
		 * It may be worthwhile promoting a tag to an instance variable in this class
		 * if most feature collection handles have such a tag.
		 */
		tags_type d_tags;

	private: // Transcribing...

		GPlatesScribe::TranscribeResult
		transcribe(
				GPlatesScribe::Scribe &scribe,
				bool transcribed_construct_data)
		{
			// Do nothing.
			//
			// NOTE: We don't actually transcribe the feature collection (and its contents).
			// We only transcribe to track the address and hence make it easier to link
			// feature collections to various transcribed objects that reference them.
			// The feature collection still needs to be explicitly loaded from a file though.
			return GPlatesScribe::TRANSCRIBE_SUCCESS;
		}

		// Only the scribe system should be able to transcribe.
		friend class GPlatesScribe::Access;

	};


	//! Overload to save/load construct data for GPlatesModel::FeatureCollectionHandle.
	GPlatesScribe::TranscribeResult
	transcribe_construct_data(
			GPlatesScribe::Scribe &scribe,
			GPlatesScribe::ConstructObject<GPlatesModel::FeatureCollectionHandle> &construct_feature_collection_handle);

}

// This include is not necessary for this header to function, but it would be
// convenient if client code could include this header and be able to use
// iterator or const_iterator without having to separately include the
// following header. It isn't placed above with the other includes because of
// cyclic dependencies.
#include "RevisionAwareIterator.h"


namespace GPlatesScribe
{
	template <>
	class TranscribeContext<GPlatesModel::FeatureCollectionHandle>
	{
	public:
		explicit
		TranscribeContext(
				const GPlatesModel::ModelInterface &model_interface_) :
			model_interface(model_interface_)
		{  }

		GPlatesModel::ModelInterface model_interface;
	};
}

#endif  // GPLATES_MODEL_FEATURECOLLECTIONHANDLE_H
