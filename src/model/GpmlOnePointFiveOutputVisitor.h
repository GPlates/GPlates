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

#ifndef GPLATES_FILEIO_GPMLONEPOINTFIVEOUTPUTVISITOR_H
#define GPLATES_FILEIO_GPMLONEPOINTFIVEOUTPUTVISITOR_H

#include "XmlOutputInterface.h"
#include "model/ConstFeatureVisitor.h"


namespace GPlatesFileIO {

	class GpmlOnePointFiveOutputVisitor: public GPlatesModel::ConstFeatureVisitor {

	public:

		explicit
		GpmlOnePointFiveOutputVisitor(
				const XmlOutputInterface &xoi):
			d_output(xoi) {  }

		virtual
		~GpmlOnePointFiveOutputVisitor() {  }

		virtual
		void
		visit_feature_handle(
				const GPlatesModel::FeatureHandle &feature_handle);

		virtual
		void
		visit_feature_revision(
				const GPlatesModel::FeatureRevision &feature_revision);

	private:

		XmlOutputInterface d_output;

	};

}

#endif  // GPLATES_FILEIO_GPMLONEPOINTFIVEOUTPUTVISITOR_H
