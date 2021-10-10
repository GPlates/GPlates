/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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

#ifndef GPLATES_FEATUREVISITORS_PROPERTYVALUESETTER_H
#define GPLATES_FEATUREVISITORS_PROPERTYVALUESETTER_H

#include <QVariant>
#include <boost/optional.hpp>
#include "model/ConstFeatureVisitor.h"
#include "model/FeatureHandle.h"
#include "model/PropertyValue.h"
#include "model/PropertyName.h"

// FIXME: Don't forget about XML attributes!!!

namespace GPlatesFeatureVisitors
{
	/**
	 * The PropertyValueSetter feature-visitor is used to visit a FeatureHandle, locate
	 * an existing InlinePropertyContainer (via a given properties_iterator), and assign
	 * a new PropertyValue as the first value corresponding to that property.
	 *
	 * In the future, it should be expanded to deal with properties containing multiple values.
	 */
	class PropertyValueSetter:
			public GPlatesModel::FeatureVisitor
	{
	public:

		explicit
		PropertyValueSetter(
				GPlatesModel::FeatureHandle::properties_iterator target_property_iter,
				GPlatesModel::PropertyValue::non_null_ptr_type new_property_value):
			d_target_property_iter(target_property_iter),
			d_new_property_value(new_property_value)
		{  }

		virtual
		~PropertyValueSetter() {  }
		

		virtual
		void
		visit_feature_handle(
				GPlatesModel::FeatureHandle &feature_handle);

		virtual
		void
		visit_inline_property_container(
				GPlatesModel::InlinePropertyContainer &inline_property_container);

		boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type>
		old_property_value()
		{
			return d_old_property_value;
		}
		

	private:
	
		GPlatesModel::FeatureHandle::properties_iterator d_target_property_iter;
		GPlatesModel::PropertyValue::non_null_ptr_type d_new_property_value;
		
		boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type> d_old_property_value;
	};

}

#endif  // GPLATES_FEATUREVISITORS_PROPERTYVALUESETTER_H

