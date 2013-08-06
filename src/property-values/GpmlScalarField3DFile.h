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

#include "utils/CopyOnWrite.h"


// Enable GPlatesFeatureVisitors::get_property_value() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GpmlScalarField3DFile, visit_gpml_scalar_field_3d_file)

namespace GPlatesPropertyValues
{
	/**
	 * This class implements the PropertyValue referencing a GPlates-specific 3D scalar field file.
	 */
	class GpmlScalarField3DFile :
			public GPlatesModel::PropertyValue
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


		//! Typedef for the scalar field filename.
		typedef XsString::non_null_ptr_to_const_type file_name_type;


		virtual
		~GpmlScalarField3DFile()
		{  }

		/**
		 * Create a GpmlScalarField3DFile instance from a filename.
		 */
		static
		const non_null_ptr_type
		create(
				const file_name_type &filename_)
		{
			return new GpmlScalarField3DFile(filename_);
		}

		const non_null_ptr_type
		clone() const
		{
			return GPlatesUtils::dynamic_pointer_cast<GpmlScalarField3DFile>(clone_impl());
		}

		/**
		 * Returns the 'const' file name - which is 'const' so that it cannot be
		 * modified and bypass the revisioning system.
		 */
		file_name_type
		get_file_name() const
		{
			return get_current_revision<Revision>().filename.get();
		}

		void
		set_file_name(
				const file_name_type &filename_);

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
				const file_name_type &filename_) :
			PropertyValue(Revision::non_null_ptr_type(new Revision(filename_)))
		{  }


		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		GpmlScalarField3DFile(
				const GpmlScalarField3DFile &other) :
			PropertyValue(other)
		{  }

		virtual
		const GPlatesModel::PropertyValue::non_null_ptr_type
		clone_impl() const
		{
			return non_null_ptr_type(new GpmlScalarField3DFile(*this));
		}

	private:

		/**
		 * Property value data that is mutable/revisionable.
		 */
		struct Revision :
				public GPlatesModel::PropertyValue::Revision
		{
			explicit
			Revision(
					const file_name_type &filename_) :
				filename(filename_)
			{  }

			virtual
			GPlatesModel::PropertyValue::Revision::non_null_ptr_type
			clone() const
			{
				return non_null_ptr_type(new Revision(*this));
			}

			// Don't need 'clone_for_bubble_up_modification()' since we're using CopyOnWrite.

			virtual
			bool
			equality(
					const GPlatesModel::PropertyValue::Revision &other) const
			{
				const Revision &other_revision = dynamic_cast<const Revision &>(other);

				return *filename.get_const() == *other_revision.filename.get_const() &&
					GPlatesModel::PropertyValue::Revision::equality(other);
			}

			GPlatesUtils::CopyOnWrite<file_name_type> filename;
		};

	};

}

#endif // GPLATES_PROPERTY_VALUES_GPMLSCALARFIELD3DFILE_H
