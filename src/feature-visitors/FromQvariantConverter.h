/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2009, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_FEATUREVISITORS_FROMQVARIANTCONVERTER_H
#define GPLATES_FEATUREVISITORS_FROMQVARIANTCONVERTER_H

#include <QVariant>
#include <boost/optional.hpp>
#include "model/FeatureVisitor.h"
#include "model/PropertyValue.h"
#include "model/PropertyName.h"


namespace GPlatesFeatureVisitors
{
	/**
	 * The FromQvariantConverter feature-visitor is used to create a property value
	 * from a QVariant, if possible. It is used by the FeaturePropertyTableModel Qt model.
	 *
	 * To use, pass it the QVariant that you wish to convert, and then get the existing
	 * PropertyValue to accept_visitor(this). FromQvariantConverter will perform the
	 * neccessary conversion and provide the new PropertyValue via get_property_value().
	 *
	 * If it visits an TopLevelPropertyInline with multiple PropertyValues, it will only
	 * consider the first PropertyValue.
	 *
	 * As the conversion may not be possible, get_property_value returns a boost::optional
	 * of PropertyValue::non_null_ptr_type.
	 */
	class FromQvariantConverter:
			public GPlatesModel::ConstFeatureVisitor
	{
	public:

		explicit
		FromQvariantConverter(
				const QVariant &new_property_value):
			d_qvariant(new_property_value)
		{  }


		virtual
		~FromQvariantConverter() {  }


		/**
		 * Returns the PropertyValue that has been created from the given QVariant.
		 * This is wrapped in a boost::optional - you must test it before dereferencing it!
		 */
		boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type>
		get_property_value()
		{
			return d_property_value;
		}

	protected:

		virtual
		void
		visit_enumeration(
				const GPlatesPropertyValues::Enumeration &enumeration);


		void
		visit_gml_time_instant(
				const GPlatesPropertyValues::GmlTimeInstant &gml_time_instant);


		virtual
		void
		visit_gpml_constant_value(
				const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value);


		virtual
		void
		visit_gpml_plate_id(
				const GPlatesPropertyValues::GpmlPlateId &gpml_plate_id);


		virtual
		void
		visit_gpml_old_plates_header(
				const GPlatesPropertyValues::GpmlOldPlatesHeader &gpml_old_plates_header);

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
	
		void
		set_return_value(
				GPlatesModel::PropertyValue::non_null_ptr_type new_value);
		
		/**
		 * The newly created PropertyValue.
		 * Since we might not be able to construct one, it is enclosed in a boost::optional.
		 * Yes, I know - an optional non_null_ptr_type. Crazy.
		 */
		boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type> d_property_value;
		
		/**
		 * The QVariant that we must convert into a PropertyValue.
		 */
		const QVariant &d_qvariant;
	};

}

#endif  // GPLATES_FEATUREVISITORS_FROMQVARIANTCONVERTER_H
