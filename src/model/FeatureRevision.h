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

#ifndef GPLATES_MODEL_FEATUREREVISION_H
#define GPLATES_MODEL_FEATUREREVISION_H

#include <vector>
#include <boost/intrusive_ptr.hpp>
#include "base/RevisionId.h"


namespace GPlatesModel {

	// Forward declaration for intrusive-pointer.
	class AbstractPropertyContainer;

	class FeatureRevision {

	public:

		typedef long ref_count_type;

		ref_count_type
		ref_count() const {
			return d_ref_count;
		}

		void
		increment_ref_count() {
			++d_ref_count;
		}

		ref_count_type
		decrement_ref_count() {
			return --d_ref_count;
		}

	private:

		ref_count_type d_ref_count;
		GPlatesBase::RevisionId d_revision_id;
		std::vector<boost::intrusive_ptr<AbstractPropertyContainer> > d_props;

	};


	void
	intrusive_ptr_add_ref(FeatureRevision *p) {
		p->increment_ref_count();
	}


	void
	intrusive_ptr_release(FeatureRevision *p) {
		if (p->decrement_ref_count() == 0) {
			delete p;
		}
	}

}

#endif  // GPLATES_MODEL_FEATUREREVISION_H
