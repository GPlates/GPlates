/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

#include <iostream>
#include <QDebug>

#include "GpmlTotalReconstructionPole.h"


std::ostream &
GPlatesPropertyValues::GpmlTotalReconstructionPole::print_to(
		std::ostream &os) const
{
	qWarning() << "TODO: implement this function.";
	os << "TODO: implement this function.";
	return  os;
}


GPlatesPropertyValues::GpmlTotalReconstructionPole::GpmlTotalReconstructionPole(
		const GPlatesMaths::FiniteRotation &finite_rotation_, 
		GPlatesModel::XmlElementNode::non_null_ptr_type xml_element) :
	GpmlFiniteRotation(finite_rotation_)
{
	static const GPlatesModel::XmlElementName META = 
			GPlatesModel::XmlElementName::create_gpml("meta");

	std::pair<
			GPlatesModel::XmlElementNode::child_const_iterator, 
			boost::optional<GPlatesModel::XmlElementNode::non_null_ptr_type> >
					child = 
							xml_element->get_next_child_by_name(
									META,
									xml_element->children_begin());

	while (child.second)
	{
		QString buf;
		QXmlStreamWriter writer(&buf);
		(*child.second)->write_to(writer);
		QXmlStreamReader reader(buf);
		GPlatesUtils::XQuery::next_start_element(reader);
		QXmlStreamAttributes attr =	reader.attributes(); 
		QStringRef name =attr.value("name");
		QString value = reader.readElementText();
		d_meta.push_back(
				boost::shared_ptr<GPlatesModel::Metadata>(
						new GPlatesModel::Metadata(name.toString(),value)));
		++child.first;
		child = xml_element->get_next_child_by_name(META, child.first);
	}
}
