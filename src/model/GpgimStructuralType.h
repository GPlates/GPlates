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
		 * Creates a @a GpgimStructuralType.
		 */
		static
		non_null_ptr_type
		create(
				const GPlatesPropertyValues::StructuralType &structural_type,
				const QString &description)
		{
			return non_null_ptr_type(new GpgimStructuralType(structural_type, description));
		}


		//! Virtual destructor since @a GpgimEnumerationType is sub-class and GPlatesUtils::dynamic_pointer_cast is used.
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
		 * Returns the description of the structural type.
		 */
		const QString &
		get_description() const
		{
			return d_description;
		}

	protected:

		GpgimStructuralType(
				const GPlatesPropertyValues::StructuralType &structural_type,
				const QString &description) :
			d_structural_type(structural_type),
			d_description(description)
		{  }

	private:

		//! The structural type.
		GPlatesPropertyValues::StructuralType d_structural_type;

		//! The description of the structural type.
		QString d_description;

	};
}

#endif // GPLATES_MODEL_GPGIMSTRUCTURALTYPE_H
