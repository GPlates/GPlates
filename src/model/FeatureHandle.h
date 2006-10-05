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
#include "base/FeatureId.h"
#include "base/FeatureType.h"

namespace GPlatesModel {

	// Forward declaration for intrusive-pointer.
	class FeatureRevision;

	class FeatureHandle {

	public:

	private:

		boost::intrusive_ptr< FeatureRevision > d_curr_rev;
		GPlatesBase::FeatureId d_feature_id;
		GPlatesBase::FeatureType d_feature_type;

	};

}

#endif  // GPLATES_MODEL_FEATUREHANDLE_H
