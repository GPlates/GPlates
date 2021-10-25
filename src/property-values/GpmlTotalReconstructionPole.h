/* $Id:  $ */
/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date: 2010-04-12 18:48:29 +1000 (Mon, 12 Apr 2010) $
 * 
 * Copyright (C) 2006, 2007, 2009, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_PROPERTYVALUES_GPMLTRS_H
#define GPLATES_PROPERTYVALUES_GPMLTRS_H
#include "feature-visitors/PropertyValueFinder.h"
#include "maths/FiniteRotation.h"
#include "model/Metadata.h"
#include "model/PropertyValue.h"
#include "property-values/GpmlFiniteRotation.h"


// Enable GPlatesFeatureVisitors::getPropertyValue() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GpmlTotalReconstructionPole, visit_gpml_total_reconstruction_pole)

namespace GPlatesPropertyValues 
{
	class GpmlTotalReconstructionPole:
		public GpmlFiniteRotation
	{
	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<GpmlTotalReconstructionPole> non_null_ptr_type;

		explicit
		GpmlTotalReconstructionPole(
				const GPlatesMaths::FiniteRotation &fr):
			GpmlFiniteRotation(*GpmlFiniteRotation::create(fr))
		{ }

		
		GpmlTotalReconstructionPole(
				const GPlatesMaths::FiniteRotation &fr, 
				GPlatesModel::XmlElementNode::non_null_ptr_type xml_element):
			GpmlFiniteRotation(*GpmlFiniteRotation::create(fr))
		{
			static const GPlatesModel::XmlElementName META = 
				GPlatesModel::XmlElementName::create_gpml("meta");
			std::pair<
				GPlatesModel::XmlElementNode::child_const_iterator, 
				boost::optional<GPlatesModel::XmlElementNode::non_null_ptr_type> > child = 
				xml_element->get_next_child_by_name(META, xml_element->children_begin());
			while(child.second)
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
			
		const GpmlTotalReconstructionPole::non_null_ptr_type
		deep_clone() const
		{
			GpmlTotalReconstructionPole* p = new GpmlTotalReconstructionPole(finite_rotation());
			p->metadata() = d_meta;
			return non_null_ptr_type(p);
		}

		DEFINE_FUNCTION_DEEP_CLONE_AS_PROP_VAL()

		void
		accept_visitor(
				GPlatesModel::FeatureVisitor &visitor) 
		{
			visitor.visit_gpml_total_reconstruction_pole(*this);
		}

		std::ostream &
		print_to(
				std::ostream &os) const
		{
			qWarning() << "TODO: implement this function.";
			os << "TODO: implement this function.";
			return  os;
		}

		void
		accept_visitor(
				GPlatesModel::ConstFeatureVisitor &visitor) const
		{
			visitor.visit_gpml_total_reconstruction_pole(*this);
		}

		
		std::vector<boost::shared_ptr<GPlatesModel::Metadata> >&
		metadata()
		{
			return d_meta;
		}


		const std::vector<boost::shared_ptr<GPlatesModel::Metadata> >&
		metadata() const 
		{
			return d_meta;
		}

		GPlatesPropertyValues::StructuralType
		get_structural_type() const 
		{
			static const StructuralType STRUCTURAL_TYPE = StructuralType::create_gml("TotalReconstructionPole");
			return STRUCTURAL_TYPE;
		}

	protected:
		std::vector<boost::shared_ptr< GPlatesModel::Metadata> > d_meta;
	};

}

#endif  // GPLATES_PROPERTYVALUES_GPMLTRS_H
