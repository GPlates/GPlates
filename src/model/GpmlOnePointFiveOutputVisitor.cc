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
#include "model/FeatureHandle.h"
#include "model/FeatureRevision.h"
#include "model/GpmlConstantValue.h"
#include "model/GpmlPlateId.h"
#include "model/SingleValuedPropertyContainer.h"


void
GPlatesFileIO::GpmlOnePointFiveOutputVisitor::visit_feature_handle(
		const GPlatesModel::FeatureHandle &feature_handle) {

	// If a feature handle doesn't contain a revision, we pretend the handle doesn't exist.
	if (feature_handle.current_revision() == NULL) {
		return;
	}

	XmlOutputInterface::ElementPairStackFrame f1(d_output, feature_handle.feature_type().get());
	{
		XmlOutputInterface::ElementPairStackFrame f2(d_output, "gpml:identity");
		d_output.write_string_content_line(feature_handle.feature_id().get());
	}
	feature_handle.current_revision()->accept_visitor(*this);
}


void
GPlatesFileIO::GpmlOnePointFiveOutputVisitor::visit_feature_revision(
		const GPlatesModel::FeatureRevision &feature_revision) {

	{
		XmlOutputInterface::ElementPairStackFrame f1(d_output, "gpml:revision");
		d_output.write_string_content_line(feature_revision.revision_id().get());
	}

	// Now visit each of the properties in turn.
	std::vector<boost::intrusive_ptr<GPlatesModel::PropertyContainer> >::const_iterator iter =
			feature_revision.properties().begin();
	std::vector<boost::intrusive_ptr<GPlatesModel::PropertyContainer> >::const_iterator end =
			feature_revision.properties().end();
	for ( ; iter != end; ++iter) {
		// Elements of this properties vector can be NULL pointers.  (See the comment in
		// "model/FeatureRevision.h" for more details.)
		if (*iter != NULL) {
			(*iter)->accept_visitor(*this);
		}
	}
}


void
GPlatesFileIO::GpmlOnePointFiveOutputVisitor::visit_gpml_constant_value(
		const GPlatesModel::GpmlConstantValue &gpml_constant_value) {
}


void
GPlatesFileIO::GpmlOnePointFiveOutputVisitor::visit_gpml_plate_id(
		const GPlatesModel::GpmlPlateId &gpml_plate_id) {
}


void
GPlatesFileIO::GpmlOnePointFiveOutputVisitor::visit_single_valued_property_container(
		const GPlatesModel::SingleValuedPropertyContainer &single_valued_property_container) {
	XmlOutputInterface::ElementPairStackFrame f1(d_output, single_valued_property_container.property_name().get(),
			single_valued_property_container.xml_attributes().begin(),
			single_valued_property_container.xml_attributes().end());
}
