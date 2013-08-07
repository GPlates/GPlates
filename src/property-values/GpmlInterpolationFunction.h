/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_PROPERTYVALUES_GPMLINTERPOLATIONFUNCTION_H
#define GPLATES_PROPERTYVALUES_GPMLINTERPOLATIONFUNCTION_H

#include "StructuralType.h"

#include "model/PropertyValue.h"
#include "utils/UnicodeStringUtils.h"


namespace GPlatesPropertyValues
{

	/**
	 * This is an abstract class, because it derives from class PropertyValue, which contains
	 * the pure virtual member functions @a clone and @a accept_visitor, which this class does
	 * not override with non-pure-virtual definitions.
	 */
	class GpmlInterpolationFunction:
			public GPlatesModel::PropertyValue
	{

	public:

		/**
		 * A convenience typedef for boost::intrusive_ptr<GpmlInterpolationFunction>.
		 */
		typedef boost::intrusive_ptr<GpmlInterpolationFunction> maybe_null_ptr_type;

		/**
		 * A convenience typedef for boost::intrusive_ptr<const GpmlInterpolationFunction>.
		 */
		typedef boost::intrusive_ptr<const GpmlInterpolationFunction> maybe_null_ptr_to_const_type;

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<GpmlInterpolationFunction>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GpmlInterpolationFunction> non_null_ptr_type;

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<const GpmlInterpolationFunction>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpmlInterpolationFunction> non_null_ptr_to_const_type;


		virtual
		~GpmlInterpolationFunction()
		{  }

		const non_null_ptr_type
		clone() const
		{
			return GPlatesUtils::dynamic_pointer_cast<GpmlInterpolationFunction>(clone_impl());
		}

		// Note that no "setter" is provided:  The value type of a
		// GpmlInterpolationFunction instance should never be changed.
		const StructuralType &
		get_value_type() const
		{
			return get_current_revision<Revision>().value_type;
		}

		/**
		 * Returns the structural type associated with this property value class.
		 */
		virtual
		StructuralType
		get_structural_type() const
		{
			static const StructuralType STRUCTURAL_TYPE = StructuralType::create_gpml("InterpolationFunction");
			return STRUCTURAL_TYPE;
		}

		virtual
		std::ostream &
		print_to(
				std::ostream &os) const;

	protected:

		/**
		 * Construct a GpmlInterpolationFunction instance.
		 */
		explicit
		GpmlInterpolationFunction(
				const Revision::non_null_ptr_type &revision):
			PropertyValue(revision)
		{  }

		/**
		 * Property value data that is mutable/revisionable.
		 *
		 * Derived revision classes must implement pure virtual methods from GPlatesModel::PropertyValue::Revision.
		 */
		struct Revision :
				public GPlatesModel::PropertyValue::Revision
		{
			explicit
			Revision(
					const StructuralType &value_type_) :
				value_type(value_type_)
			{  }

			virtual
			GPlatesModel::PropertyValue::Revision::non_null_ptr_type
			clone() const
			{
				return non_null_ptr_type(new Revision(*this));
			}

			virtual
			bool
			equality(
					const GPlatesModel::PropertyValue::Revision &other) const
			{
				const Revision &other_revision = dynamic_cast<const Revision &>(other);

				return value_type == other_revision.value_type &&
					GPlatesModel::PropertyValue::Revision::equality(other);
			}

			StructuralType value_type;
		};

	private:

	};

}

#endif  // GPLATES_PROPERTYVALUES_GPMLINTERPOLATIONFUNCTION_H
