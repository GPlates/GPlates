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

#ifndef GPLATES_PROPERTY_VALUES_GPMLSCALARFIELD3DFILE_H
#define GPLATES_PROPERTY_VALUES_GPMLSCALARFIELD3DFILE_H

#include <vector>

#include "XsString.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "model/PropertyValue.h"
#include "model/RevisionContext.h"
#include "model/RevisionedReference.h"


// Enable GPlatesFeatureVisitors::get_revisionable() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GpmlScalarField3DFile, visit_gpml_scalar_field_3d_file)

namespace GPlatesPropertyValues
{
	/**
	 * This class implements the PropertyValue referencing a GPlates-specific 3D scalar field file.
	 */
	class GpmlScalarField3DFile :
			public GPlatesModel::PropertyValue,
			public GPlatesModel::RevisionContext
	{
	public:

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<GpmlScalarField3DFile>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GpmlScalarField3DFile> non_null_ptr_type;

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<const GpmlScalarField3DFile>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpmlScalarField3DFile> non_null_ptr_to_const_type;


		virtual
		~GpmlScalarField3DFile()
		{  }

		/**
		 * Create a GpmlScalarField3DFile instance from a filename.
		 */
		static
		const non_null_ptr_type
		create(
				XsString::non_null_ptr_type filename_);

		const non_null_ptr_type
		clone() const
		{
			return GPlatesUtils::dynamic_pointer_cast<GpmlScalarField3DFile>(clone_impl());
		}

		/**
		 * Returns the 'const' file name - the file name shouldn't be modifiable.
		 */
		XsString::non_null_ptr_to_const_type
		get_file_name() const
		{
			return get_current_revision<Revision>().filename.get_revisionable();
		}

		void
		set_file_name(
				XsString::non_null_ptr_type filename_);

		/**
		 * Returns the structural type associated with this property value class.
		 */
		virtual
		StructuralType
		get_structural_type() const
		{
			static const StructuralType STRUCTURAL_TYPE = StructuralType::create_gpml("ScalarField3DFile");
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
			visitor.visit_gpml_scalar_field_3d_file(*this);
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
			visitor.visit_gpml_scalar_field_3d_file(*this);
		}

		virtual
		std::ostream &
		print_to(
				std::ostream &os) const;

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		explicit
		GpmlScalarField3DFile(
				GPlatesModel::ModelTransaction &transaction_,
				XsString::non_null_ptr_type filename_) :
			PropertyValue(Revision::non_null_ptr_type(new Revision(transaction_, *this, filename_)))
		{  }

		//! Constructor used when cloning.
		GpmlScalarField3DFile(
				const GpmlScalarField3DFile &other_,
				boost::optional<RevisionContext &> context_) :
			PropertyValue(
					Revision::non_null_ptr_type(
							// Use deep-clone constructor...
							new Revision(other_.get_current_revision<Revision>(), context_, *this)))
		{  }

		virtual
		const Revisionable::non_null_ptr_type
		clone_impl(
				boost::optional<RevisionContext &> context = boost::none) const
		{
			return non_null_ptr_type(new GpmlScalarField3DFile(*this, context));
		}

	private:

		/**
		 * Used when modifications bubble up to us.
		 *
		 * Inherited from @a RevisionContext.
		 */
		virtual
		GPlatesModel::Revision::non_null_ptr_type
		bubble_up(
				GPlatesModel::ModelTransaction &transaction,
				const Revisionable::non_null_ptr_to_const_type &child_revisionable);

		/**
		 * Inherited from @a RevisionContext.
		 */
		virtual
		boost::optional<GPlatesModel::Model &>
		get_model()
		{
			return PropertyValue::get_model();
		}

		/**
		 * Property value data that is mutable/revisionable.
		 */
		struct Revision :
				public PropertyValue::Revision
		{
			explicit
			Revision(
					GPlatesModel::ModelTransaction &transaction_,
					RevisionContext &child_context_,
					XsString::non_null_ptr_type filename_) :
				filename(
						GPlatesModel::RevisionedReference<XsString>::attach(
								transaction_, child_context_, filename_))
			{  }

			//! Deep-clone constructor.
			Revision(
					const Revision &other_,
					boost::optional<RevisionContext &> context_,
					RevisionContext &child_context_) :
				PropertyValue::Revision(context_),
				filename(other_.filename)
			{
				// Clone data members that were not deep copied.
				filename.clone(child_context_);
			}

			//! Shallow-clone constructor.
			Revision(
					const Revision &other_,
					boost::optional<RevisionContext &> context_) :
				PropertyValue::Revision(context_),
				filename(other_.filename)
			{  }

			virtual
			GPlatesModel::Revision::non_null_ptr_type
			clone_revision(
					boost::optional<RevisionContext &> context) const
			{
				// Use shallow-clone constructor.
				return non_null_ptr_type(new Revision(*this, context));
			}

			virtual
			bool
			equality(
					const GPlatesModel::Revision &other) const
			{
				const Revision &other_revision = dynamic_cast<const Revision &>(other);

				return *filename.get_revisionable() == *other_revision.filename.get_revisionable() &&
						GPlatesModel::Revision::equality(other);
			}

			GPlatesModel::RevisionedReference<XsString> filename;
		};

	};

}

#endif // GPLATES_PROPERTY_VALUES_GPMLSCALARFIELD3DFILE_H
