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

#include <algorithm>
#include <boost/bind.hpp>
#include <boost/ref.hpp>

#include "XmlNodeUtils.h"


boost::optional<GPlatesModel::XmlElementNode::non_null_ptr_type>
GPlatesModel::XmlNodeUtils::XmlElementNodeExtractionVisitor::get_xml_element_node(
		const XmlNode::non_null_ptr_type &xml_node)
{
	d_xml_element_node = boost::none;

	xml_node->accept_visitor(*this);

	return d_xml_element_node;
}


void
GPlatesModel::XmlNodeUtils::XmlElementNodeExtractionVisitor::visit_element_node(
		const XmlElementNode::non_null_ptr_type &xml_element_node)
{
	// If a matching element name has been requested...
	if (d_xml_element_name)
	{
		if (d_xml_element_name.get() != xml_element_node->get_name())
		{
			d_xml_element_node = boost::none;
			return;
		}
	}

	d_xml_element_node = xml_element_node;
}


boost::optional<QString>
GPlatesModel::XmlNodeUtils::get_text_without_trimming(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem)
{
	TextExtractionVisitor visitor;

	std::for_each(
			elem->children_begin(), elem->children_end(),
			boost::bind(&GPlatesModel::XmlNode::accept_visitor, _1, boost::ref(visitor)));

	if (visitor.encountered_subelement())
	{
		return boost::none;
	}

	return visitor.get_text();
}


boost::optional<QString>
GPlatesModel::XmlNodeUtils::get_text(
		const GPlatesModel::XmlElementNode::non_null_ptr_type &elem)
{
	boost::optional<QString> text_without_trimming = get_text_without_trimming(elem);
	if (!text_without_trimming)
	{
		return boost::none;
	}

	return text_without_trimming->trimmed();
}
