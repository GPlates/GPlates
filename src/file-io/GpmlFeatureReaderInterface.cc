/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#include "GpmlFeatureReaderInterface.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"


GPlatesFileIO::GpmlFeatureReaderInterface::GpmlFeatureReaderInterface(
		const GpmlFeatureReaderImpl::non_null_ptr_type &impl) :
	d_impl(impl)
{
}


GPlatesModel::FeatureHandle::non_null_ptr_type
GPlatesFileIO::GpmlFeatureReaderInterface::read_feature(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &feature_xml_element,
		GpmlReaderUtils::ReaderParams &reader_params) const
{
	// Get a list of all the feature property nodes to be processed.
	GpmlFeatureReaderImpl::xml_node_seq_type unprocessed_feature_property_xml_nodes(
			feature_xml_element->children_begin(),
			feature_xml_element->children_end());

	// Read the feature properties and create a new feature.
	const GPlatesModel::FeatureHandle::non_null_ptr_type feature =
			d_impl->read_feature(
					feature_xml_element,
					unprocessed_feature_property_xml_nodes,
					reader_params);

	// All the feature property nodes must have been processed.
	// Usually the last feature reader impl in the chain will read all unprocessed property nodes
	// as 'UninterpretedPropertyValue' objects.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			unprocessed_feature_property_xml_nodes.empty(),
			GPLATES_ASSERTION_SOURCE);

	return feature;
}
