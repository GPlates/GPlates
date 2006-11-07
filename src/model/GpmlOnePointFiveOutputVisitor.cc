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

#include "GpmlOnePointFiveOutputVisitor.h"
#include "XmlOutputInterface.h"
#include "model/FeatureHandle.h"


void
GPlatesFileIO::GpmlOnePointFiveOutputVisitor::visit_feature_handle(
		const GPlatesModel::FeatureHandle &feature_handle) {
	if (feature_handle.current_revision() == NULL) {
		// If a feature handle doesn't contain a revision, we pretend the handle doesn't
		// exist.
		return;
	}

	XmlOutputInterface xoi = XmlOutputInterface::create_for_stdout();

	XmlOutputInterface::ElementPairStackFrame f1(xoi, feature_handle.feature_type().get());
	{
		XmlOutputInterface::ElementPairStackFrame f2(xoi, "gpml:identity");
		xoi.write_string_content_line(feature_handle.feature_id().get());
	}
}
