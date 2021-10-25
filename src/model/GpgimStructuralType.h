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

#ifndef GPLATES_MODEL_GPGIMSTRUCTURALTYPE_H
#define GPLATES_MODEL_GPGIMSTRUCTURALTYPE_H

#include <boost/optional.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/tuple/tuple_comparison.hpp>
#include <QString>

#include "property-values/StructuralType.h"

#include "utils/ReferenceCount.h"


namespace GPlatesModel
{
	/**
	 * Information about a property structural type in the GPlates Geological Information Model (GPGIM).
	 */
	class GpgimStructuralType :
			public GPlatesUtils::ReferenceCount<GpgimStructuralType>
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GpgimStructuralType.
		typedef GPlatesUtils::non_null_intrusive_ptr<GpgimStructuralType> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GpgimStructuralType.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpgimStructuralType> non_null_ptr_to_const_type;

		/**
		 * The instantiation type is the structural type of the property and an optional
		 * value type (only used if structural type is a template such as 'gpml:Array').
		 *
		 * Template structural types (such as 'gpml:Array') need a value type to be specified
		 * in order to complete the type or instantiate the type (eg, 'gpml:Array<gml:TimePeriod>').
		 * Non-template structural types do not need a value type.
		 *
		 * TODO: If template types become more complex and have more than one template parameter
		 * then may have to change this to something more suitable.
		 */
		class InstantiationType :
				public boost::less_than_comparable<InstantiationType,
						boost::equality_comparable<InstantiationType> >
		{
		public:

			/**
			 * Note: Allow implicit conversion from 'GPlatesPropertyValues::StructuralType'
			 * (ie, no 'explicit') since it makes constructing non-template types easier.
			 */
			InstantiationType(
					const GPlatesPropertyValues::StructuralType &structural_type,
					boost::optional<GPlatesPropertyValues::StructuralType> value_type = boost::none) :
				d_structural_type(structural_type),
				d_value_type(value_type)
			{  }

			//! Returns true if the structural type is a template (ie, has a non-none value type).
			bool
			is_template() const
			{
				return static_cast<bool>(d_value_type);
			}

			const GPlatesPropertyValues::StructuralType &
			get_structural_type() const
			{
				return d_structural_type;
			}

			const boost::optional<GPlatesPropertyValues::StructuralType> &
			get_value_type() const
			{
				return d_value_type;
			}

			//! Equality comparison operator.
			bool
			operator==(
					const InstantiationType &rhs) const
			{
				return d_structural_type == rhs.d_structural_type &&
						d_value_type == rhs.d_value_type;
			}

			//! Less than comparison operator.
			bool
			operator<(
					const InstantiationType &rhs) const
			{
				return d_structural_type < rhs.d_structural_type ||
					(d_structural_type == rhs.d_structural_type && d_value_type < rhs.d_value_type);
			}

		private:

			GPlatesPropertyValues::StructuralType d_structural_type;
			boost::optional<GPlatesPropertyValues::StructuralType> d_value_type;

		};

		typedef InstantiationType instantiation_type;


		/**
		 * Creates a @a GpgimStructuralType.
		 */
		static
		non_null_ptr_type
		create(
				const GPlatesPropertyValues::StructuralType &structural_type,
				const QString &description,
				bool is_geometry_structural_type_ = false)
		{
			return non_null_ptr_type(new GpgimStructuralType(structural_type, description, is_geometry_structural_type_));
		}


		/**
		 * Virtual destructor since @a GpgimEnumerationType is sub-class and GPlatesUtils::dynamic_pointer_cast is used.
		 *
		 * Also we now have virtual methods.
		 */
		virtual
		~GpgimStructuralType()
		{  }


		/**
		 * Returns the structural type.
		 */
		const GPlatesPropertyValues::StructuralType &
		get_structural_type() const
		{
			return d_structural_type;
		}


		/**
		 * Returns true if the structural type represents a geometry.
		 *
		 * Geometry structural types include:
		 *   - gml:Point
		 *   - gml:MultiPoint
		 *   - gml:Polygon
		 *   - gml:LineString
		 *   - gml:OrientableCurve
		 *   - gpml:TopologicalLine
		 *   - gpml:TopologicalNetwork
		 *   - gpml:TopologicalPolygon
		 */
		bool
		is_geometry_structural_type() const
		{
			return d_is_geometry_structural_type;
		}


		/**
		 * Returns the description of the structural type.
		 */
		const QString &
		get_description() const
		{
			return d_description;
		}


		/**
		 * Returns the instantiation type.
		 *
		 * Non-template structural types do not need a value type (implementation below).
		 *
		 * Template structural types (such as 'gpml:Array') need a value type to be specified
		 * in order to complete the type or instantiate the type (eg, 'gpml:Array<gml:TimePeriod>').
		 * Hence template structural types use the derived class @a GpgimTemplateStructuralType
		 * which overrides this method.
		 */
		virtual
		instantiation_type
		get_instantiation_type() const
		{
			// Base class has no value type.
			// The structural type is what gets instantiated.
			return instantiation_type(get_structural_type());
		}

	protected:

		GpgimStructuralType(
				const GPlatesPropertyValues::StructuralType &structural_type,
				const QString &description,
				bool is_geometry_structural_type_ = false) :
			d_structural_type(structural_type),
			d_description(description),
			d_is_geometry_structural_type(is_geometry_structural_type_)
		{  }

	private:

		//! The structural type.
		GPlatesPropertyValues::StructuralType d_structural_type;

		//! The description of the structural type.
		QString d_description;

		//! Is a geometry?
		bool d_is_geometry_structural_type;

	};
}

#endif // GPLATES_MODEL_GPGIMSTRUCTURALTYPE_H
