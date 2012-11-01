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

#ifndef GPLATES_MODEL_GPGIMENUMERATIONTYPE_H
#define GPLATES_MODEL_GPGIMENUMERATIONTYPE_H

#include <vector>
#include <QString>

#include "GpgimStructuralType.h"

#include "utils/ReferenceCount.h"


namespace GPlatesModel
{
	/**
	 * Information about a property enumeration (structural) type in the GPlates Geological Information Model (GPGIM).
	 *
	 * Since an enumeration is a property structural type, this is a sub-class of @a GpgimStructuralType
	 * containing extra enumeration-specific data - which is the allowed enumeration values for an
	 * enumeration (structural) type.
	 */
	class GpgimEnumerationType :
			public GpgimStructuralType
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a GpgimEnumerationType.
		typedef GPlatesUtils::non_null_intrusive_ptr<GpgimEnumerationType> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GpgimEnumerationType.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpgimEnumerationType> non_null_ptr_to_const_type;


		/**
		 * A content of this enumeration containing allowed enumeration value and a description of that value.
		 */
		struct Content
		{
			Content(
					const QString &value_,
					const QString &description_) :
				value(value_),
				description(description_)
			{  }

			QString value;
			QString description;
		};

		//! Typdef for a sequence of @a Content objects.
		typedef std::vector<Content> content_seq_type;


		/**
		 * Creates a @a GpgimEnumerationType.
		 *
		 * @param structural_type is the enumeration structural type.
		 * @param description a description of this enumeration type.
		 * @param contents_begin begin iterator over the enumeration content for this enumeration type.
		 * @param contents_end end iterator over the enumeration content for this enumeration type.
		 */
		template <typename ContentForwardIter>
		static
		non_null_ptr_type
		create(
				const GPlatesPropertyValues::StructuralType &structural_type,
				const QString &description,
				ContentForwardIter contents_begin,
				ContentForwardIter contents_end)
		{
			return non_null_ptr_type(
					new GpgimEnumerationType(
							structural_type, description, contents_begin, contents_end));
		}


		/**
		 * Returns the allowed content of this enumeration type.
		 */
		const content_seq_type &
		get_contents() const
		{
			return d_contents;
		}

	private:

		//! The allowed content of this enumeration type.
		content_seq_type d_contents;


		template <typename ContentForwardIter>
		GpgimEnumerationType(
				const GPlatesPropertyValues::StructuralType &structural_type,
				const QString &description,
				ContentForwardIter contents_begin,
				ContentForwardIter contents_end) :
			GpgimStructuralType(structural_type, description),
			d_contents(contents_begin, contents_end)
		{  }

	};
}

#endif // GPLATES_MODEL_GPGIMENUMERATIONTYPE_H
