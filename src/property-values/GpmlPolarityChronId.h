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
			return GPlatesUtils::dynamic_pointer_cast<GpmlPolarityChronId>(clone_impl());
		}

		/**
		 * Return the "era" attribute of this GpmlPolarityChronId instance.
		 */
		const boost::optional<QString> &
		get_era() const
		{
			return get_current_revision<Revision>().era;
		}

		/**
		 * Set the "era" attribute of this GpmlPolarityChronId instance.
		 */
		void
		set_era(
				const QString &era);

		/**
		 * Return the "major region" attribute of this GpmlPolarityChronId instance.
		 */
		const boost::optional<unsigned int> &
		get_major_region() const
		{
			return get_current_revision<Revision>().major_region;
		}

		/**
		 * Set the "major region" attribute of this GpmlPolarityChronId instance.
		 */
		void
		set_major_region(
				unsigned int major_region);

		/**
		 * Return the "minor region" attribute of this GpmlPolarityChronId instance.
		 */
		const boost::optional<QString> &
		get_minor_region() const
		{
			return get_current_revision<Revision>().minor_region;
		}

		/**
		 * Set the "minor region" attribute of this GpmlPolarityChronId instance.
		 */
		void
		set_minor_region(
				const QString &minor_region);

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
			PropertyValue(Revision::non_null_ptr_type(new Revision(era, major_region, minor_region)))
		{  }

		//! Constructor used when cloning.
		GpmlPolarityChronId(
				const GpmlPolarityChronId &other_,
				boost::optional<GPlatesModel::RevisionContext &> context_) :
			PropertyValue(
					Revision::non_null_ptr_type(
							new Revision(other_.get_current_revision<Revision>(), context_)))
		{  }

		virtual
		const Revisionable::non_null_ptr_type
		clone_impl(
				boost::optional<GPlatesModel::RevisionContext &> context = boost::none) const
		{
			return non_null_ptr_type(new GpmlPolarityChronId(*this, context));
		}

	private:

		/**
		 * Property value data that is mutable/revisionable.
		 */
		struct Revision :
				public PropertyValue::Revision
		{
			Revision(
					boost::optional<QString> era_,
					boost::optional<unsigned int> major_region_,
					boost::optional<QString> minor_region_) :
				era(era_),
				major_region(major_region_),
				minor_region(minor_region_)
			{  }

			//! Clone constructor.
			Revision(
					const Revision &other_,
					boost::optional<GPlatesModel::RevisionContext &> context_) :
				PropertyValue::Revision(context_),
				era(other_.era),
				major_region(other_.major_region),
				minor_region(other_.minor_region)
			{  }

			virtual
			GPlatesModel::Revision::non_null_ptr_type
			clone_revision(
					boost::optional<GPlatesModel::RevisionContext &> context) const
			{
				return non_null_ptr_type(new Revision(*this, context));
			}

			virtual
			bool
			equality(
					const GPlatesModel::Revision &other) const
			{
				const Revision &other_revision = dynamic_cast<const Revision &>(other);

				return era == other_revision.era &&
						major_region == other_revision.major_region &&
						minor_region == other_revision.minor_region &&
						PropertyValue::Revision::equality(other);
			}

			boost::optional<QString> era;
			boost::optional<unsigned int> major_region;
			boost::optional<QString> minor_region;
		};

	};

}

#endif  // GPLATES_PROPERTYVALUES_GPMLPOLARITYCHRONID_H
