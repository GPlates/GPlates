/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2010 Geological Survey of Norway.
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

#ifndef GPLATES_PROPERTYVALUES_GPMLKEYVALUEDICTIONARYELEMENT_H
#define GPLATES_PROPERTYVALUES_GPMLKEYVALUEDICTIONARYELEMENT_H

#include <iosfwd>

#include "property-values/TemplateTypeParameterType.h"
#include "property-values/XsString.h"

namespace GPlatesPropertyValues
{

	class GpmlKeyValueDictionaryElement
	{

	public:

		GpmlKeyValueDictionaryElement(
			GPlatesPropertyValues::XsString::non_null_ptr_type key_,
			GPlatesModel::PropertyValue::non_null_ptr_type value_,
			const TemplateTypeParameterType &value_type_):
				d_key(key_),
				d_value(value_),
				d_value_type(value_type_)
		{  }

		virtual
		~GpmlKeyValueDictionaryElement()
		{  }

		const GpmlKeyValueDictionaryElement
		deep_clone() const;

		const GPlatesPropertyValues::XsString::non_null_ptr_to_const_type
		key() const
		{
				return d_key;
		}

		const GPlatesPropertyValues::XsString::non_null_ptr_type
		key()
		{
				return d_key;
		}		

		const GPlatesModel::PropertyValue::non_null_ptr_to_const_type
		value() const
		{
				return d_value;
		}

		const GPlatesModel::PropertyValue::non_null_ptr_type
		value()
		{
				return d_value;
		}

		const TemplateTypeParameterType &
		value_type() const
		{
			return d_value_type;		
		}

		bool
		operator==(
				const GpmlKeyValueDictionaryElement &other) const;

	private:

		XsString::non_null_ptr_type d_key;
		GPlatesModel::PropertyValue::non_null_ptr_type d_value;
		TemplateTypeParameterType d_value_type;

	};


	std::ostream &
	operator<<(
			std::ostream &os,
			const GpmlKeyValueDictionaryElement &element);

}

#endif  // GPLATES_PROPERTYVALUES_GPMLKEYVALUEDICTIONARYELEMENT_H
