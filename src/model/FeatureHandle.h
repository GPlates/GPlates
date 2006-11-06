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
#include "FeatureRevision.h"
#include "FeatureId.h"
#include "FeatureType.h"
#include "ConstFeatureVisitor.h"

namespace GPlatesModel {

	class FeatureHandle {

	public:
		FeatureHandle(
				const FeatureId &feature_id_,
				const FeatureType &feature_type_) :
			d_current_revision(NULL),
			d_feature_id(feature_id_),
			d_feature_type(feature_type_) {  }

		const FeatureId &
		feature_id() const {
			return d_feature_id;
		}

		// No non-const 'feature_id':  The feature-ID should never be changed.

		const FeatureType &
		feature_type() const {
			return d_feature_type;
		}

		// No non-const 'feature_type':  The feature-type should never be changed.

		boost::intrusive_ptr<const FeatureRevision>
		current_revision() const {
			return d_current_revision;
		}

		boost::intrusive_ptr<FeatureRevision>
		current_revision() {
			return d_current_revision;
		}

		// Yes, I'm pretty sure that the parameter should be pass-by-reference rather than
		// pass-by-value.
		void
		swap_revision(
				boost::intrusive_ptr<FeatureRevision> &rev) {
			d_current_revision.swap(rev);
		}

		void
		accept_visitor(
				ConstFeatureVisitor &visitor) const {
			visitor.visit_feature_handle(*this);
		}

	private:

		boost::intrusive_ptr<FeatureRevision> d_current_revision;
		FeatureId d_feature_id;
		FeatureType d_feature_type;

	};

}

#endif  // GPLATES_MODEL_FEATUREHANDLE_H
