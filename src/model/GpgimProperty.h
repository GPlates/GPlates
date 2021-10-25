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

#ifndef GPLATES_MODEL_GPGIMPROPERTY_H
#define GPLATES_MODEL_GPGIMPROPERTY_H

#include <bitset>
#include <vector>
#include <boost/optional.hpp>
#include <QString>

#include "GpgimStructuralType.h"
#include "PropertyName.h"

#include "property-values/StructuralType.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "utils/ReferenceCount.h"


namespace GPlatesModel
{
	/**
	 * Defines a property of a feature in the GPlates Geological Information Model (GPGIM).
	 *
	 * The definition includes:
	 *  - the property name,
	 *  - the allowed structural types,
	 *  - whether the property is time-dependent or not (and the allowed time-dependent styles), and
	 *  - the multiplicity of the property.
	 */
	class GpgimProperty :
			public GPlatesUtils::ReferenceCount<GpgimProperty>
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GpgimProperty.
		typedef GPlatesUtils::non_null_intrusive_ptr<GpgimProperty> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GpgimProperty.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpgimProperty> non_null_ptr_to_const_type;

		//! Typedef for a sequence of structural types.
		typedef std::vector<GpgimStructuralType::non_null_ptr_to_const_type> structural_type_seq_type;

		/**
		 * The number of times this property can occur in its parent feature.
		 *
		 * This concept may need to be extended to the 'minOccurs' and 'maxOccurs' of XML schema.
		 * But this seems appropriate GPlates and it tends to discourage arbitrary numbers in the
		 * GPGIM (such as allowing [1-3] of a particular property which might mean that that feature
		 * needs to be re-designed).
		 */
		enum MultiplicityType
		{
			ZERO_OR_ONE,
			ONE,
			ZERO_OR_MORE,
			ONE_OR_MORE
		};

		/**
		 * The ways in which a property can be made time-dependent.
		 */
		enum TimeDependentType
		{
			CONSTANT_VALUE,          // If property value can be wrapped in a 'gpml:ConstantValue'.
			PIECEWISE_AGGREGATION,   // If property value can be wrapped in a 'gpml:PiecewiseAggregation'.
			IRREGULAR_SAMPLING,      // If property value can be wrapped in a 'gpml:IrregularSampling'.

			NUM_TIME_DEPENDENT_TYPES // Must be the last enum.
		};

		//! Typedef for a flag of time-dependent types.
		typedef std::bitset<NUM_TIME_DEPENDENT_TYPES> time_dependent_flags_type;


		/**
		 * Creates a @a GpgimProperty.
		 *
		 * @param property_name the name of this property.
		 * @param multiplicity the number of allowed occurrences of this property in its parent feature.
		 * @param structural_types_begin begin iterator over the allowed structural types for this property.
		 * @param structural_types_end end iterator over the allowed structural types for this property.
		 * @param default_structural_type_index the index for the default/suggested structural type.
		 * @param time_dependent_types the allowed time-dependent types, if any, for this property.
		 *
		 * NOTE: For multiple structural types, @a default_structural_type_index should index the
		 * default/suggested type within the sequence [structural_types_begin, structural_types_end).
		 * For a single structural type, @a default_structural_type_index should be zero.
		 *
		 * NOTE: There must be at least one structural type.
		 */
		template <typename StructuralTypeForwardIter>
		static
		non_null_ptr_type
		create(
				const PropertyName &property_name,
				const QString &user_friendly_name,
				const QString &property_description,
				MultiplicityType multiplicity,
				StructuralTypeForwardIter structural_types_begin,
				StructuralTypeForwardIter structural_types_end,
				unsigned int default_structural_type_index,
				time_dependent_flags_type time_dependent_types)
		{
			return non_null_ptr_type(
					new GpgimProperty(
							property_name,
							user_friendly_name,
							property_description,
							multiplicity,
							structural_types_begin,
							structural_types_end,
							default_structural_type_index,
							time_dependent_types));
		}

		/**
		 * Creates a @a GpgimProperty.
		 *
		 * Same as other overload of @a create but with a single structural type (instead of multiple).
		 */
		static
		non_null_ptr_type
		create(
				const PropertyName &property_name,
				const QString &user_friendly_name,
				const QString &property_description,
				MultiplicityType multiplicity,
				const GpgimStructuralType::non_null_ptr_to_const_type &structural_type,
				time_dependent_flags_type time_dependent_types)
		{
			GpgimStructuralType::non_null_ptr_to_const_type structural_types[1] = { structural_type };

			return non_null_ptr_type(
					new GpgimProperty(
							property_name,
							user_friendly_name,
							property_description,
							multiplicity,
							structural_types,
							structural_types + 1,
							0/*default_structural_type_index*/,
							time_dependent_types));
		}


		/**
		 * Clones 'this' object.
		 */
		non_null_ptr_type
		clone() const
		{
			return non_null_ptr_type(
					new GpgimProperty(
							d_property_name,
							d_user_friendly_name,
							d_property_description,
							d_multiplicity,
							d_structural_types.begin(),
							d_structural_types.end(),
							// Default structural type is always at index 0...
							0,
							d_time_dependent_types));
		}


		/**
		 * Returns the property name.
		 */
		const PropertyName &
		get_property_name() const
		{
			return d_property_name;
		}

		/**
		 * Sets the property name.
		 */
		void
		set_property_name(
				const PropertyName &property_name)
		{
			d_property_name = property_name;
		}


		/**
		 * Returns the user friendly name of this property.
		 *
		 * This is what is displayed in the GUI to the user.
		 * It is a more human-readable version of the property name.
		 */
		const QString &
		get_user_friendly_name() const
		{
			return d_user_friendly_name;
		}

		/**
		 * Sets the user friendly name.
		 */
		void
		set_user_friendly_name(
				const QString &user_friendly_name)
		{
			d_user_friendly_name = user_friendly_name;
		}


		/**
		 * Returns the property description.
		 */
		const QString &
		get_property_description() const
		{
			return d_property_description;
		}

		/**
		 * Sets the property description.
		 */
		void
		set_property_description(
				const QString &property_description)
		{
			d_property_description = property_description;
		}


		/**
		 * Returns the allowed structural types for this property.
		 *
		 * There can be more than one allowed structural type in some cases such as geometry properties.
		 * For example, some possible options for a geometry property are:
		 *  (1) 'gml:Point', or
		 *  (2) 'gml:Point', 'gml:MultiPoint', 'gml:OrientableCurve', 'gml:Polygon', or
		 *  (3) 'gpml:TopologicalPolygon', 'gpml:TopologicalLine', or
		 *  (4) 'gml:Point', 'gml:MultiPoint', 'gml:OrientableCurve', 'gml:Polygon',
		 *      'gpml:TopologicalPolygon', 'gpml:TopologicalLine', or
		 *   ...etc.
		 */
		const structural_type_seq_type &
		get_structural_types() const
		{
			return d_structural_types;
		}


		/**
		 * Returns the default structural type for this property.
		 *
		 * For a property with only a single structural type this method returns that type.
		 * For a property with multiple structural types the GPGIM lists one type as the default/suggested type.
		 */
		GpgimStructuralType::non_null_ptr_to_const_type
		get_default_structural_type() const
		{
			// The default is always placed at the front of the sequence.
			return d_structural_types.front();
		}


		/**
		 * Convenience method returns the structural type of this property matching the specified type.
		 *
		 * Returns boost::none if the specified structural type is not found.
		 */
		boost::optional<GpgimStructuralType::non_null_ptr_to_const_type>
		get_structural_type(
				const GPlatesPropertyValues::StructuralType &structural_type) const;


		/**
		 * Sets the structural types.
		 *
		 * @param structural_types_begin begin iterator over the allowed structural types for this property.
		 * @param structural_types_end end iterator over the allowed structural types for this property.
		 * @param default_structural_type_index the index for the default/suggested structural type.
		 *
		 * NOTE: For multiple structural types, @a default_structural_type_index should index the
		 * default/suggested type within the sequence [structural_types_begin, structural_types_end).
		 * For a single structural type, @a default_structural_type_index should be zero.
		 *
		 * NOTE: There must be at least one structural type.
		 */
		template <typename StructuralTypeForwardIter>
		void
		set_structural_types(
				StructuralTypeForwardIter structural_types_begin,
				StructuralTypeForwardIter structural_types_end,
				unsigned int default_structural_type_index)
		{
			d_structural_types.clear();
			d_structural_types.insert(d_structural_types.end(), structural_types_begin, structural_types_end);
			set_default_structural_type(default_structural_type_index);
		}


		/**
		 * Returns the number of allowed occurrences of this property in its parent feature.
		 */
		MultiplicityType
		get_multiplicity() const
		{
			return d_multiplicity;
		}

		/**
		 * Sets the property multiplicity.
		 */
		void
		set_multiplicity(
				MultiplicityType multiplicity)
		{
			d_multiplicity = multiplicity;
		}


		/**
		 * Returns true if this property is time-dependent.
		 */
		bool
		is_time_dependent() const
		{
			return d_time_dependent_types.any();
		}


		/**
		 * Returns the allowed time-dependent types, if any, for this property.
		 *
		 * If none of the returned flags are set then the property value should not be wrapped
		 * in a time-dependent wrapper (ie, it is not a property that is associated with time).
		 */
		const time_dependent_flags_type &
		get_time_dependent_types() const
		{
			return d_time_dependent_types;
		}

		/**
		 * Sets the allowed time-dependent types.
		 */
		void
		set_time_dependent_types(
				const time_dependent_flags_type &time_dependent_types)
		{
			d_time_dependent_types = time_dependent_types;
		}

	private:

		//! The name of this property.
		PropertyName d_property_name;

		//! The user-friendly name of this property.
		QString d_user_friendly_name;

		//! The description of this property.
		QString d_property_description;

		//! The number of allowed occurrences of this property in its parent feature.
		MultiplicityType d_multiplicity;

		//! The allowed structural types for this property.
		structural_type_seq_type d_structural_types;

		//! The allowed time-dependent types, if any, for this property.
		time_dependent_flags_type d_time_dependent_types;


		void
		set_default_structural_type(
				unsigned int default_structural_type_index)
		{
			// Should have at least one structural type.
			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					!d_structural_types.empty() &&
						default_structural_type_index < d_structural_types.size(),
					GPLATES_ASSERTION_SOURCE);

			// Move the default structural type to the beginning of the structural types sequence.
			if (default_structural_type_index != 0)
			{
				const GpgimStructuralType::non_null_ptr_to_const_type default_structural_type =
						d_structural_types[default_structural_type_index];
				d_structural_types.erase(d_structural_types.begin() + default_structural_type_index);
				d_structural_types.insert(d_structural_types.begin(), default_structural_type);
			}
		}

		//! Constructor.
		template <typename StructuralTypeForwardIter>
		GpgimProperty(
				const PropertyName &property_name,
				const QString &user_friendly_name,
				const QString &property_description,
				MultiplicityType multiplicity,
				StructuralTypeForwardIter structural_types_begin,
				StructuralTypeForwardIter structural_types_end,
				unsigned int default_structural_type_index,
				time_dependent_flags_type time_dependent_types) :
			d_property_name(property_name),
			d_user_friendly_name(user_friendly_name),
			d_property_description(property_description),
			d_multiplicity(multiplicity),
			d_structural_types(structural_types_begin, structural_types_end),
			d_time_dependent_types(time_dependent_types)
		{
			set_default_structural_type(default_structural_type_index);
		}

	};
}

#endif // GPLATES_MODEL_GPGIMPROPERTY_H
