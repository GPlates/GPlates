/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date $
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

#ifndef GPLATES_PROPERTYVALUES_GPMLPOLARITYCHRONID_H
#define GPLATES_PROPERTYVALUES_GPMLPOLARITYCHRONID_H

#include <boost/optional.hpp>
#include <QString>

#include "feature-visitors/PropertyValueFinder.h"

#include "model/PropertyValue.h"


// Enable GPlatesFeatureVisitors::get_property_value() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GpmlPolarityChronId, visit_gpml_polarity_chron_id)

namespace GPlatesPropertyValues
{
	/**
	 * This class implements the PropertyValue which corresponds to "gpml:PolarityChronId".
	 */
	class GpmlPolarityChronId:
			public GPlatesModel::PropertyValue
	{

	public:

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<GpmlPolarityChronId>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GpmlPolarityChronId> non_null_ptr_type;

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<const GpmlPolarityChronId>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpmlPolarityChronId> non_null_ptr_to_const_type;


		virtual
		~GpmlPolarityChronId()
		{  }

		/**
		 * Create a GpmlPolarityChronId instance.  Note that all of the parameters 
		 * are boost::optional.
		 */
		static
		const non_null_ptr_type
		create(
				boost::optional<QString> era,
				boost::optional<unsigned int> major_region,
				boost::optional<QString> minor_region)
		{
			return non_null_ptr_type(new GpmlPolarityChronId(era, major_region, minor_region));
		}

		const non_null_ptr_type
		clone() const
		{
			return non_null_ptr_type(new GpmlPolarityChronId(*this));
		}

		const non_null_ptr_type
		deep_clone() const
		{
			// This class doesn't reference any mutable objects by pointer, so there's
			// no need for any recursive cloning.  Hence, regular clone will suffice.
			return clone();
		}

		DEFINE_FUNCTION_DEEP_CLONE_AS_PROP_VAL()

		/**
		 * Return the "era" attribute of this GpmlPolarityChronId instance.
		 */
		const boost::optional<QString> &
		get_era() const
		{
			return d_era;
		}

		/**
		 * Set the "era" attribute of this GpmlPolarityChronId instance.
		 */
		void
		set_era(
				const QString &era)
		{
			d_era = era;
			update_instance_id();
		}

		/**
		 * Return the "major region" attribute of this GpmlPolarityChronId instance.
		 */
		const boost::optional<unsigned int> &
		get_major_region() const
		{
			return d_major_region;
		}

		/**
		 * Set the "major region" attribute of this GpmlPolarityChronId instance.
		 */
		void
		set_major_region(
				unsigned int major_region)
		{
			d_major_region = major_region;
			update_instance_id();
		}

		/**
		 * Return the "minor region" attribute of this GpmlPolarityChronId instance.
		 */
		const boost::optional<QString> &
		get_minor_region() const
		{
			return d_minor_region;
		}

		/**
		 * Set the "minor region" attribute of this GpmlPolarityChronId instance.
		 */
		void
		set_minor_region(
				const QString &minor_region)
		{
			d_minor_region = minor_region;
			update_instance_id();
		}

		/**
		 * Returns the structural type associated with this property value class.
		 */
		virtual
		StructuralType
		get_structural_type() const
		{
			static const StructuralType STRUCTURAL_TYPE = StructuralType::create_gpml("PolarityChronId");
			return STRUCTURAL_TYPE;
		}

		/**
		 * Accept a ConstFeatureVisitor instance.
		 *
		 * See the Visitor pattern (p.331) in Gamma95 for information on the purpose of
		 * this function.
		 */
		virtual
		void
		accept_visitor(
				GPlatesModel::ConstFeatureVisitor &visitor) const
		{
			visitor.visit_gpml_polarity_chron_id(*this);
		}

		/**
		 * Accept a FeatureVisitor instance.
		 *
		 * See the Visitor pattern (p.331) in Gamma95 for information on the purpose of
		 * this function.
		 */
		virtual
		void
		accept_visitor(
				GPlatesModel::FeatureVisitor &visitor)
		{
			visitor.visit_gpml_polarity_chron_id(*this);
		}

		virtual
		std::ostream &
		print_to(
				std::ostream &os) const;

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		GpmlPolarityChronId(
				boost::optional<QString> era,
				boost::optional<unsigned int> major_region,
				boost::optional<QString> minor_region):
			PropertyValue(),
			d_era(era),
			d_major_region(major_region),
			d_minor_region(minor_region)
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		GpmlPolarityChronId(
				const GpmlPolarityChronId &other) :
			PropertyValue(other), /* share instance id */
			d_era(other.d_era),
			d_major_region(other.d_major_region),
			d_minor_region(other.d_minor_region)
		{  }

	private:

		boost::optional<QString> d_era;
		boost::optional<unsigned int> d_major_region;
		boost::optional<QString> d_minor_region;

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		GpmlPolarityChronId &
		operator=(const GpmlPolarityChronId &);

	};

}

#endif  // GPLATES_PROPERTYVALUES_GPMLPOLARITYCHRONID_H
