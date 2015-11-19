/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2014 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_GPGIMTEMPLATESTRUCTURALTYPE_H
#define GPLATES_MODEL_GPGIMTEMPLATESTRUCTURALTYPE_H

#include "GpgimStructuralType.h"


namespace GPlatesModel
{
	/**
	 * Information about a property *template* structural type in the GPlates Geological Information Model (GPGIM).
	 *
	 * This is essentially a template instantiation which is a structural type *and* a contained
	 * value type such as 'gpml:Array' and 'gml:TimePeriod'. This is in contrast to an uninstantiated
	 * template type which is just the structural type (eg, 'gpml:Array') and is instead represented
	 * by class @a GpgimStructuralType (as are non-template types like 'gml:TimePeriod').
	 */
	class GpgimTemplateStructuralType :
			public GpgimStructuralType
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GpgimTemplateStructuralType.
		typedef GPlatesUtils::non_null_intrusive_ptr<GpgimTemplateStructuralType> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GpgimTemplateStructuralType.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpgimTemplateStructuralType> non_null_ptr_to_const_type;


		/**
		 * Creates a @a GpgimTemplateStructuralType.
		 */
		static
		non_null_ptr_type
		create(
				const GPlatesPropertyValues::StructuralType &structural_type,
				const GPlatesPropertyValues::StructuralType &value_type,
				const QString &description)
		{
			return non_null_ptr_type(new GpgimTemplateStructuralType(structural_type, value_type, description));
		}


		/**
		 * Creates a @a GpgimTemplateStructuralType from a @a GpgimStructuralType and a value type.
		 */
		static
		non_null_ptr_type
		create(
				const GpgimStructuralType &gpgim_structural_type,
				const GPlatesPropertyValues::StructuralType &value_type)
		{
			return non_null_ptr_type(new GpgimTemplateStructuralType(gpgim_structural_type, value_type));
		}


		/**
		 * Returns the value type.
		 *
		 * This is the type that instantiates the template.
		 */
		const GPlatesPropertyValues::StructuralType &
		get_value_type() const
		{
			return d_value_type;
		}


		/**
		 * Returns the template's instantiation type (structural type plus value type).
		 *
		 * Template structural types (such as 'gpml:Array') need a value type to be specified
		 * in order to complete the type or instantiate the type (eg, 'gpml:Array<gml:TimePeriod>').
		 */
		virtual
		instantiation_type
		get_instantiation_type() const
		{
			return instantiation_type(get_structural_type(), get_value_type());
		}

	protected:

		GpgimTemplateStructuralType(
				const GPlatesPropertyValues::StructuralType &structural_type,
				const GPlatesPropertyValues::StructuralType &value_type,
				const QString &description) :
			GpgimStructuralType(structural_type, description),
			d_value_type(value_type)
		{  }

		GpgimTemplateStructuralType(
				const GpgimStructuralType &gpgim_structural_type,
				const GPlatesPropertyValues::StructuralType &value_type) :
			GpgimStructuralType(
						gpgim_structural_type.get_structural_type(),
						gpgim_structural_type.get_description()),
			d_value_type(value_type)
		{  }

	private:

		//! The value structural type.
		GPlatesPropertyValues::StructuralType d_value_type;

	};
}

#endif // GPLATES_MODEL_GPGIMTEMPLATESTRUCTURALTYPE_H
