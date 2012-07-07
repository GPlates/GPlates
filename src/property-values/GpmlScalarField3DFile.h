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


// Enable GPlatesFeatureVisitors::getPropertyValue() to work with this property value.
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
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<const GpmlScalarField3DFile>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpmlScalarField3DFile> non_null_ptr_to_const_type;

		virtual
		~GpmlScalarField3DFile()
		{  }

		typedef XsString::non_null_ptr_to_const_type file_name_type;

		/**
		 * Create a GpmlScalarField3DFile instance from a filename.
		 */
		static
		const non_null_ptr_type
		create(
				const file_name_type &filename_);

		const non_null_ptr_type
		clone() const
		{
			non_null_ptr_type dup(new GpmlScalarField3DFile(*this));
			return dup;
		}

		const non_null_ptr_type
		deep_clone() const
		{
			// This class doesn't reference any mutable objects by pointer, so there's
			// no need for any recursive cloning.  Hence, regular clone will suffice.
			return clone();
		}

		DEFINE_FUNCTION_DEEP_CLONE_AS_PROP_VAL()

		const file_name_type &
		file_name() const
		{
			return d_filename;
		}

		void
		set_file_name(
				const file_name_type &filename_)
		{
			d_filename = filename_;
			update_instance_id();
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
			PropertyValue(),
			d_filename(filename_)
		{  }


		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		GpmlScalarField3DFile(
				const GpmlScalarField3DFile &other) :
			PropertyValue(other), /* share instance id */
			d_filename(other.d_filename)
		{  }

	private:

		file_name_type d_filename;

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		GpmlScalarField3DFile &
		operator=(
				const GpmlRasterBandNames &);

	};

}

#endif // GPLATES_PROPERTY_VALUES_GPMLSCALARFIELD3DFILE_H
