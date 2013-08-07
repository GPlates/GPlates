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
			return GPlatesUtils::dynamic_pointer_cast<GpmlFeatureSnapshotReference>(clone_impl());
		}

		const GPlatesModel::FeatureId &
		get_feature_id() const
		{
			return get_current_revision<Revision>().feature;
		}

		void
		set_feature_id(
				const GPlatesModel::FeatureId &feature);

		const GPlatesModel::RevisionId &
		get_revision_id() const
		{
			return get_current_revision<Revision>().revision;
		}

		void
		set_revision_id(
				const GPlatesModel::RevisionId &revision);

		// Note that no "setter" is provided:  The value type of a GpmlFeatureSnapshotReference
		// instance should never be changed.
		const GPlatesModel::FeatureType &
		get_value_type() const
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
			PropertyValue(Revision::non_null_ptr_type(new Revision(feature_, revision_))),
			d_value_type(value_type_)
		{  }

		virtual
		const GPlatesModel::PropertyValue::non_null_ptr_type
		clone_impl() const
		{
			return non_null_ptr_type(new GpmlFeatureSnapshotReference(*this));
		}

		virtual
		bool
		equality(
				const PropertyValue &other) const
		{
			const GpmlFeatureSnapshotReference &other_pv =
					dynamic_cast<const GpmlFeatureSnapshotReference &>(other);

			return d_value_type == other_pv.d_value_type &&
					// The revisioned data comparisons are handled here...
					GPlatesModel::PropertyValue::equality(other);
		}

	private:

		/**
		 * Property value data that is mutable/revisionable.
		 */
		struct Revision :
				public GPlatesModel::PropertyValue::Revision
		{
			Revision(
					const GPlatesModel::FeatureId &feature_,
					const GPlatesModel::RevisionId &revision_) :
				feature(feature_),
				revision(revision_)
			{  }

			virtual
			GPlatesModel::PropertyValue::Revision::non_null_ptr_type
			clone() const
			{
				return non_null_ptr_type(new Revision(*this));
			}

			virtual
			bool
			equality(
					const GPlatesModel::PropertyValue::Revision &other) const
			{
				const Revision &other_revision = dynamic_cast<const Revision &>(other);

				return feature == other_revision.feature &&
						revision == other_revision.revision &&
						GPlatesModel::PropertyValue::Revision::equality(other);
			}

			GPlatesModel::FeatureId feature;
			GPlatesModel::RevisionId revision;
		};

		// Immutable, so doesn't need revisioning.
		GPlatesModel::FeatureType d_value_type;

	};

}

#endif  // GPLATES_PROPERTYVALUES_GPMLFEATURESNAPSHOTREFERENCE_H
