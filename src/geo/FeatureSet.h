/**
 * @file 
 * $Revision$
 * $Date$
 *
 * Copyright (C) 2003 The GPlates Consortium
 *
 * This file is part of GPlates.
 *
 * GPlates is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * GPlates is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef GPLATES_GEO_FEATURESET_H
#define GPLATES_GEO_FEATURESET_H

#include <list>
#include "Feature.h"

namespace Geo {

	/**
	 * FeatureSet represents a collection of Features.
	 */
	class FeatureSet {

		public:
			
			/**
			 * FileInfo contains information about the file from which
			 * a FeatureSet was derived.
			 */
			class FileInfo {

			};

			// The actual typedefs will probably change.
			typedef std::list< Feature * > collection;
			typedef collection::const_iterator const_iterator;

			/**
			 * Create a FeatureSet with the associated @a file_info 
			 * but without any elements.
			 */
			FeatureSet(
			 FileInfo &file_info)
			 : m_file_info(file_info) {  }

			~FeatureSet() {

				const_iterator 
				 i = m_feature_set.begin(), 
				 end = m_feature_set.end();

				for ( ; i != end; ++i)
					delete *i;
			}

			/**
			 * Add a Feature to the FeatureSet.
			 * 
			 * XXX Presumably this FeatureSet is now responsible for the
			 * deletion of the memory associated with @a feature.  Let's
			 * hope no one inserts an automatic variable!
			 */
			void
			insert(
			 Feature &feature) {

				m_feature_set.push_back(&feature);
			}

			/**
			 * Acquire an iterator to the beginning of the collection of
			 * Features.
			 */
			const_iterator
			begin() const {

				return m_feature_set.begin();
			}

			/**
			 * Acquire an iterator to the end of the collection of
			 * Features.
			 */
			const_iterator
			end() const {

				return m_feature_set.end();
			}

		private:

			FileInfo m_file_info;
			collection m_feature_set;
	};

}

#endif  /* GPLATES_GEO_FEATURESET_H */
