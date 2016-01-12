/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007, 2008, 2009, 2010 The University of Sydney, Australia
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
 
#ifndef GPLATES_PROPERTYVALUES_GPMLOLDPLATESHEADER_H
#define GPLATES_PROPERTYVALUES_GPMLOLDPLATESHEADER_H

#include <cstdlib>
#include <ctime>
#include <string>
#include <sstream>

#include "TextContent.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "global/unicode.h"

#include "maths/MathsUtils.h"

#include "model/PropertyValue.h"
#include "model/types.h"

#include "utils/UnicodeStringUtils.h"


// Enable GPlatesFeatureVisitors::get_property_value() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GpmlOldPlatesHeader, visit_gpml_old_plates_header)

namespace GPlatesPropertyValues 
{

	class GpmlOldPlatesHeader:
			public GPlatesModel::PropertyValue
	{

	public:

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<GpmlOldPlatesHeader>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GpmlOldPlatesHeader> non_null_ptr_type;

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<const GpmlOldPlatesHeader>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpmlOldPlatesHeader> non_null_ptr_to_const_type;


		virtual
		~GpmlOldPlatesHeader() 
		{  }
		
		static
		const non_null_ptr_type
		create(		
				unsigned int region_number,
				unsigned int reference_number,
				unsigned int string_number,
				const GPlatesUtils::UnicodeString &geographic_description,
				GPlatesModel::integer_plate_id_type plate_id_number,
				const double &age_of_appearance,
				const double &age_of_disappearance,
				const GPlatesUtils::UnicodeString &data_type_code,
				unsigned int data_type_code_number,
				const GPlatesUtils::UnicodeString &data_type_code_number_additional,
				GPlatesModel::integer_plate_id_type conjugate_plate_id_number,
				unsigned int colour_code,
				unsigned int number_of_points)
		{
			return non_null_ptr_type(
					new GpmlOldPlatesHeader(
							region_number, reference_number, 
							string_number, geographic_description, plate_id_number, 
							age_of_appearance, age_of_disappearance, data_type_code,
							data_type_code_number, data_type_code_number_additional,
							conjugate_plate_id_number, colour_code, number_of_points));
		}

		const non_null_ptr_type
		clone() const 
		{
			return GPlatesUtils::dynamic_pointer_cast<GpmlOldPlatesHeader>(clone_impl());
		}

		unsigned int
		get_region_number() const 
		{
			return get_current_revision<Revision>().region_number;
		}
		
		/**
		 * Set the region number to @a i.
		 */
		void
		set_region_number(
				const unsigned int &i);


		unsigned int
		get_reference_number() const 
		{
			return get_current_revision<Revision>().reference_number;
		}

		/**
		 * Set the reference number to @a i.
		 */
		void
		set_reference_number(
				const unsigned int &i);

		
		unsigned int
		get_string_number() const 
		{
			return get_current_revision<Revision>().string_number;
		}
		
		/**
		 * Set the string number to @a i.
		 */
		void
		set_string_number(
				const unsigned int &i);

		
		const GPlatesUtils::UnicodeString &
		get_geographic_description() const 
		{
			return get_current_revision<Revision>().geographic_description.get();
		}

		/**
		 * Set the geographic description to @a us.
		 */
		void
		set_geographic_description(
				const GPlatesUtils::UnicodeString &us);

		
		GPlatesModel::integer_plate_id_type
		get_plate_id_number() const 
		{
			return get_current_revision<Revision>().plate_id_number;
		}

		/**
		 * Set the plate id number to @a i.
		 */
		void
		set_plate_id_number(
				const GPlatesModel::integer_plate_id_type &i);

		
		const double &
		get_age_of_appearance() const 
		{
			return get_current_revision<Revision>().age_of_appearance;
		}

		/**
		 * Set the age of appearance to @a d.
		 */
		void
		set_age_of_appearance(
				const double &d);

		
		const double &
		get_age_of_disappearance() const 
		{
			return get_current_revision<Revision>().age_of_disappearance;
		}

		/**
		 * Set the age of disappearance to @a d.
		 */
		void
		set_age_of_disappearance(
				const double &d);

		
		const GPlatesUtils::UnicodeString & 
		get_data_type_code() const 
		{
			return get_current_revision<Revision>().data_type_code.get();
		}

		/**
		 * Set the data type code to @a us.
		 */
		void
		set_data_type_code(
				const GPlatesUtils::UnicodeString &us);

		
		unsigned int
		get_data_type_code_number() const 
		{
			return get_current_revision<Revision>().data_type_code_number;
		}

		/**
		 * Set the data type code number to @a i.
		 */
		void
		set_data_type_code_number(
				const unsigned int &i);

		
		const GPlatesUtils::UnicodeString & 
		get_data_type_code_number_additional() const 
		{
			return get_current_revision<Revision>().data_type_code_number_additional.get();
		}

		/**
		 * Set the data type code number (additional) string to @a us.
		 */
		void
		set_data_type_code_number_additional(
				const GPlatesUtils::UnicodeString &us);

		
		GPlatesModel::integer_plate_id_type
		get_conjugate_plate_id_number() const 
		{
			return get_current_revision<Revision>().conjugate_plate_id_number;
		}

		/**
		 * Set the conjugate plate id number to @a i.
		 */
		void
		set_conjugate_plate_id_number(
				const GPlatesModel::integer_plate_id_type &i);
		
		
		unsigned int
		get_colour_code() const 
		{
			return get_current_revision<Revision>().colour_code;
		}
		
		/**
		 * Set the colour code to @a i.
		 */
		void
		set_colour_code(
				const unsigned int &i);

			
		unsigned int
		get_number_of_points() const 
		{
			return get_current_revision<Revision>().number_of_points;
		}

		/**
		 * Set the number of points to @a i.
		 */
		void
		set_number_of_points(
				const unsigned int &i);


		/**
		 * Returns the structural type associated with this property value class.
		 */
		virtual
		StructuralType
		get_structural_type() const
		{
			return STRUCTURAL_TYPE;
		}

		/**
		 * Static access to the structural type as GpmlOldPlatesHeader::STRUCTURAL_TYPE.
		 */
		static const StructuralType STRUCTURAL_TYPE;


		/**
		 * Accept a ConstFeatureVisitor instance.
		 *
		 * See the Visitor pattern (p.331) in Gamma95 for information on the purpose of
		 * this function.
		 */
		virtual
		void
		accept_visitor(
				GPlatesModel::ConstFeatureVisitor &visitor) const
		{
			visitor.visit_gpml_old_plates_header(*this);
		}

		/**
		 * Accept a FeatureVisitor instance.
		 *
		 * See the Visitor pattern (p.331) in Gamma95 for information on the purpose of
		 * this function.
		 */
		virtual
		void
		accept_visitor(
				GPlatesModel::FeatureVisitor &visitor)
		{
			visitor.visit_gpml_old_plates_header(*this);
		}

		/**
		* Return the old plates header formatted to match GPlates 0.8 style feature Id's
		* for example: 
		# "gplates_00_00_0000_Front_Polygon_Rotates_About_X_BOUNDARY_LINE_101_ 999.0_-999.0_RI_0000_000_"
		*/
		// FIXME:  Make it return 'const UnicodeString' rather than 'std::string'.
		std::string
		old_feature_id() const;

		virtual
		std::ostream &
		print_to(
				std::ostream &os) const;

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		GpmlOldPlatesHeader(
				unsigned int region_number_,
				unsigned int reference_number_,
				unsigned int string_number_,
				const GPlatesUtils::UnicodeString &geographic_description_,
				GPlatesModel::integer_plate_id_type plate_id_number_,
				const double &age_of_appearance_,
				const double &age_of_disappearance_,
				const GPlatesUtils::UnicodeString &data_type_code_,
				unsigned int data_type_code_number_,
				const GPlatesUtils::UnicodeString &data_type_code_number_additional_,
				GPlatesModel::integer_plate_id_type conjugate_plate_id_number_,
				unsigned int colour_code_,
				unsigned int number_of_points_) :
			PropertyValue(
					Revision::non_null_ptr_type(
							new Revision(
									region_number_, reference_number_, 
									string_number_, geographic_description_, plate_id_number_, 
									age_of_appearance_, age_of_disappearance_, data_type_code_,
									data_type_code_number_, data_type_code_number_additional_,
									conjugate_plate_id_number_, colour_code_, number_of_points_)))
		{ }

		//! Constructor used when cloning.
		GpmlOldPlatesHeader(
				const GpmlOldPlatesHeader &other_,
				boost::optional<GPlatesModel::RevisionContext &> context_) :
			PropertyValue(
					Revision::non_null_ptr_type(
							new Revision(other_.get_current_revision<Revision>(), context_)))
		{  }

		virtual
		const Revisionable::non_null_ptr_type
		clone_impl(
				boost::optional<GPlatesModel::RevisionContext &> context = boost::none) const
		{
			return non_null_ptr_type(new GpmlOldPlatesHeader(*this, context));
		}

	private:

		/**
		 * Property value data that is mutable/revisionable.
		 */
		struct Revision :
				public PropertyValue::Revision
		{
			Revision(
					unsigned int region_number_,
					unsigned int reference_number_,
					unsigned int string_number_,
					const GPlatesUtils::UnicodeString &geographic_description_,
					GPlatesModel::integer_plate_id_type plate_id_number_,
					const double &age_of_appearance_,
					const double &age_of_disappearance_,
					const GPlatesUtils::UnicodeString &data_type_code_,
					unsigned int data_type_code_number_,
					const GPlatesUtils::UnicodeString &data_type_code_number_additional_,
					GPlatesModel::integer_plate_id_type conjugate_plate_id_number_,
					unsigned int colour_code_,
					unsigned int number_of_points_) :
				region_number(region_number_),
				reference_number(reference_number_),
				string_number(string_number_),
				geographic_description(geographic_description_),
				plate_id_number(plate_id_number_),
				age_of_appearance(age_of_appearance_),
				age_of_disappearance(age_of_disappearance_),
				data_type_code(data_type_code_),
				data_type_code_number(data_type_code_number_),
				data_type_code_number_additional(data_type_code_number_additional_),
				conjugate_plate_id_number(conjugate_plate_id_number_),
				colour_code(colour_code_),
				number_of_points(number_of_points_)
			{  }

			//! Clone constructor.
			Revision(
					const Revision &other_,
					boost::optional<GPlatesModel::RevisionContext &> context_) :
				PropertyValue::Revision(context_),
				region_number(other_.region_number),
				reference_number(other_.reference_number),
				string_number(other_.string_number),
				geographic_description(other_.geographic_description),
				plate_id_number(other_.plate_id_number),
				age_of_appearance(other_.age_of_appearance),
				age_of_disappearance(other_.age_of_disappearance),
				data_type_code(other_.data_type_code),
				data_type_code_number(other_.data_type_code_number),
				data_type_code_number_additional(other_.data_type_code_number_additional),
				conjugate_plate_id_number(other_.conjugate_plate_id_number),
				colour_code(other_.colour_code),
				number_of_points(other_.number_of_points)
			{  }

			virtual
			GPlatesModel::Revision::non_null_ptr_type
			clone_revision(
					boost::optional<GPlatesModel::RevisionContext &> context) const
			{
				return non_null_ptr_type(new Revision(*this, context));
			}

			virtual
			bool
			equality(
					const GPlatesModel::Revision &other) const
			{
				const Revision &other_revision = dynamic_cast<const Revision &>(other);

				return region_number == other_revision.region_number &&
						reference_number == other_revision.reference_number &&
						string_number == other_revision.string_number &&
						geographic_description == other_revision.geographic_description &&
						plate_id_number == other_revision.plate_id_number &&
						GPlatesMaths::are_almost_exactly_equal(age_of_appearance, other_revision.age_of_appearance) &&
						GPlatesMaths::are_almost_exactly_equal(age_of_disappearance, other_revision.age_of_disappearance) &&
						data_type_code == other_revision.data_type_code &&
						data_type_code_number == other_revision.data_type_code_number &&
						data_type_code_number_additional == other_revision.data_type_code_number_additional &&
						conjugate_plate_id_number == other_revision.conjugate_plate_id_number &&
						colour_code == other_revision.colour_code &&
						number_of_points == other_revision.number_of_points &&
						PropertyValue::Revision::equality(other);
			}

			unsigned int region_number;
			unsigned int reference_number;
			unsigned int string_number;
			TextContent geographic_description;
			GPlatesModel::integer_plate_id_type plate_id_number;
			double age_of_appearance;
			double age_of_disappearance;
			TextContent data_type_code;
			unsigned int data_type_code_number;
			TextContent data_type_code_number_additional;
			GPlatesModel::integer_plate_id_type conjugate_plate_id_number;
			unsigned int colour_code;
			unsigned int number_of_points;
		};
		
	};
}

#endif // GPLATES_PROPERTYVALUES_GPMLOLDPLATESHEADER_H
