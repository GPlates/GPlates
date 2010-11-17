/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2009 Geological Survey of Norway
 * Copyright (C) 2010 The University of Sydney, Australia
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

#ifndef GPLATES_FEATUREVISITORS_POPULATESHAPEFILEATTRIBUTESVISITOR_H
#define GPLATES_FEATUREVISITORS_POPULATESHAPEFILEATTRIBUTESVISITOR_H

#include <vector>
#include <QVariant>
#include <Qt>
#include "model/FeatureVisitor.h"
#include "model/PropertyValue.h"
#include "model/PropertyName.h"

namespace GPlatesPropertyValues
{
	class GpmlKeyValueDictionaryElement;
}

namespace GPlatesDataMining
{
	/**
	 * The ToQvariantConverter feature-visitor is used to locate specific property values
	 * within a Feature and convert them to QVariants, if possible. It is used by the
	 * FeaturePropertyTableModel Qt model.
	 */
	class PopulateShapeFileAttributesVisitor:
			public GPlatesModel::ConstFeatureVisitor
	{
	public:
		
		virtual
		~PopulateShapeFileAttributesVisitor() {  }

		std::vector < QString >&
		get_shape_file_attr_names()
		{
			return d_names;
		}

	protected:

		virtual
		bool
		initialise_pre_property_values(
				const GPlatesModel::TopLevelPropertyInline &top_level_property_inline);

		virtual
		void
		visit_gpml_key_value_dictionary(
				const GPlatesPropertyValues::GpmlKeyValueDictionary &dictionary);

		virtual
		void
		visit_xs_boolean(
				const GPlatesPropertyValues::XsBoolean &xs_boolean);

		virtual
		void
		visit_xs_double(
				const GPlatesPropertyValues::XsDouble &xs_double);

		virtual
		void
		visit_xs_integer(
				const GPlatesPropertyValues::XsInteger& xs_integer);

		virtual
		void
		visit_xs_string(
				const GPlatesPropertyValues::XsString &xs_string);

	private:
		std::map< QString, QVariant > d_attr_map;
		std::vector < QString > d_names;
	};
}

#endif  // GPLATES_FEATUREVISITORS_POPULATESHAPEFILEATTRIBUTESVISITOR_H

