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

#ifndef GPLATES_PROPERTYVALUES_GPMLFEATURESNAPSHOTREFERENCE_H
#define GPLATES_PROPERTYVALUES_GPMLFEATURESNAPSHOTREFERENCE_H

#include "StructuralType.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "model/FeatureId.h"
#include "model/FeatureType.h"
#include "model/PropertyValue.h"
#include "model/RevisionId.h"

#include "utils/UnicodeStringUtils.h"


// Enable GPlatesFeatureVisitors::get_property_value() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GpmlFeatureSnapshotReference, visit_gpml_feature_snapshot_reference)

namespace GPlatesPropertyValues
{

	class GpmlFeatureSnapshotReference:
			public GPlatesModel::PropertyValue
	{

	public:

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<GpmlFeatureSnapshotReference>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GpmlFeatureSnapshotReference> non_null_ptr_type;

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<const GpmlFeatureSnapshotReference>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpmlFeatureSnapshotReference> non_null_ptr_to_const_type;


		virtual
		~GpmlFeatureSnapshotReference()
		{  }

		static
		const non_null_ptr_type
		create(
				const GPlatesModel::FeatureId &feature_,
				const GPlatesModel::RevisionId &revision_,
				const GPlatesModel::FeatureType &value_type_)
		{
			return non_null_ptr_type(new GpmlFeatureSnapshotReference(feature_, revision_, value_type_));
		}

		const non_null_ptr_type
		clone() const
		{
			return non_null_ptr_type(
					new GpmlFeatureSnapshotReference(*this));
		}

		const non_null_ptr_type
		deep_clone() const
		{
			// This class doesn't reference any mutable objects by pointer, so there's
			// no need for any recursive cloning.  Hence, regular clone will suffice.
			return clone();
		}

		DEFINE_FUNCTION_DEEP_CLONE_AS_PROP_VAL()

		const GPlatesModel::FeatureId
		feature_id() const
		{
			return d_feature;
		}

		const GPlatesModel::RevisionId
		revision_id() const
		{
			return d_revision;
		}

		// Note that no "setter" is provided:  The value type of a GpmlFeatureSnapshotReference
		// instance should never be changed.
		const GPlatesModel::FeatureType &
		value_type() const
		{
			return d_value_type;
		}

		/**
		 * Returns the structural type associated with this property value class.
		 */
		virtual
		StructuralType
		get_structural_type() const
		{
			static const StructuralType STRUCTURAL_TYPE = StructuralType::create_gpml("FeatureSnapshotReference");
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
			visitor.visit_gpml_feature_snapshot_reference(*this);
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
			visitor.visit_gpml_feature_snapshot_reference(*this);
		}

		virtual
		std::ostream &
		print_to(
				std::ostream &os) const;

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		GpmlFeatureSnapshotReference(
				const GPlatesModel::FeatureId &feature_,
				const GPlatesModel::RevisionId &revision_,
				const GPlatesModel::FeatureType &value_type_):
			PropertyValue(),
			d_feature(feature_),
			d_revision(revision_),
			d_value_type(value_type_)
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		GpmlFeatureSnapshotReference(
				const GpmlFeatureSnapshotReference &other) :
			PropertyValue(other), /* share instance id */
			d_feature(other.d_feature),
			d_revision(other.d_revision),
			d_value_type(other.d_value_type)
		{  }

	private:

		GPlatesModel::FeatureId d_feature;
		GPlatesModel::RevisionId d_revision;
		GPlatesModel::FeatureType d_value_type;

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		GpmlFeatureSnapshotReference &
		operator=(const GpmlFeatureSnapshotReference &);

	};

}

#endif  // GPLATES_PROPERTYVALUES_GPMLFEATURESNAPSHOTREFERENCE_H
