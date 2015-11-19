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

#include "GpmlFiniteRotation.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "maths/FiniteRotation.h"

#include "model/Metadata.h"
#include "model/PropertyValue.h"


// Enable GPlatesFeatureVisitors::getPropertyValue() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GpmlTotalReconstructionPole, visit_gpml_total_reconstruction_pole)

namespace GPlatesPropertyValues 
{
	class GpmlTotalReconstructionPole :
			public GpmlFiniteRotation
	{
	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<GpmlTotalReconstructionPole> non_null_ptr_type;


		static
		const non_null_ptr_type
		create(
				const GPlatesMaths::FiniteRotation &finite_rotation_)
		{
			return non_null_ptr_type(new GpmlTotalReconstructionPole(finite_rotation_));
		}

		static
		const non_null_ptr_type
		create(
				const GPlatesMaths::FiniteRotation &finite_rotation_,
				GPlatesModel::XmlElementNode::non_null_ptr_type xml_element)
		{
			return non_null_ptr_type(new GpmlTotalReconstructionPole(finite_rotation_, xml_element));
		}

			
		const GpmlTotalReconstructionPole::non_null_ptr_type
		deep_clone() const
		{
			non_null_ptr_type p(new GpmlTotalReconstructionPole(finite_rotation()));
			p->metadata() = d_meta;
			return p;
		}

		DEFINE_FUNCTION_DEEP_CLONE_AS_PROP_VAL()

		
		/**
		 * FIXME: Re-implement MetadataContainer because it's currently possible to modify the
		 * metadata in a 'const' MetadataContainer and this by-passes revisioning.
		 */
		const GPlatesModel::MetadataContainer &
		metadata() const
		{
			return d_meta;
		}

		GPlatesModel::MetadataContainer &
		metadata()
		{
			update_instance_id();
			return d_meta;
		}



		GPlatesPropertyValues::StructuralType
		get_structural_type() const 
		{
			static const StructuralType STRUCTURAL_TYPE = StructuralType::create_gpml("TotalReconstructionPole");
			return STRUCTURAL_TYPE;
		}


		virtual
		void
		accept_visitor(
				GPlatesModel::FeatureVisitor &visitor) 
		{
			visitor.visit_gpml_total_reconstruction_pole(*this);
		}

		virtual
		void
		accept_visitor(
				GPlatesModel::ConstFeatureVisitor &visitor) const
		{
			visitor.visit_gpml_total_reconstruction_pole(*this);
		}

		virtual
		std::ostream &
		print_to(
				std::ostream &os) const;

	protected:

		explicit
		GpmlTotalReconstructionPole(
				const GPlatesMaths::FiniteRotation &finite_rotation_) :
			GpmlFiniteRotation(finite_rotation_)
		{ }

		GpmlTotalReconstructionPole(
				const GPlatesMaths::FiniteRotation &fr, 
				GPlatesModel::XmlElementNode::non_null_ptr_type xml_element);

		GPlatesModel::MetadataContainer d_meta;
	};
}

#endif  // GPLATES_PROPERTYVALUES_GPMLTRS_H
