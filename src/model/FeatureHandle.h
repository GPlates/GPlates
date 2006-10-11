/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006 The GPlates Consortium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef GPLATES_MODEL_FEATUREHANDLE_H
#define GPLATES_MODEL_FEATUREHANDLE_H

#include <boost/intrusive_ptr.hpp>
#include "FeatureId.h"
#include "FeatureType.h"

namespace GPlatesModel {

	// Forward declaration for intrusive-pointer.
	class FeatureRevision;

	class FeatureHandle {

	public:
		explicit
		FeatureHandle(
				const FeatureType &feature_type_) :
			d_curr_rev(NULL),
			d_feature_type(feature_type_) {  }

		const FeatureId &
		feature_id() const {
			return d_feature_id;
		}

		// The feature-id should not be changed after the parsing of the XML has completed.
		FeatureId &
		feature_id() {
			return d_feature_id;
		}

		const FeatureType &
		feature_type() const {
			return d_feature_type;
		}

		// No non-const 'feature_type':  The feature-type should never be changed.

	private:

		boost::intrusive_ptr<FeatureRevision> d_curr_rev;
		FeatureId d_feature_id;
		FeatureType d_feature_type;

	};

}

#endif  // GPLATES_MODEL_FEATUREHANDLE_H
