/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006 The University of Sydney, Australia
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

		void
		set_current_revision(
				boost::intrusive_ptr<FeatureRevision> rev) {
			d_current_revision = rev;
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
