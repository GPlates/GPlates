/* $Id$ */

/**
 * @file 
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

#ifndef GPLATES_FILEIO_GPMLPROPERTYSTRUCTURALTYPEREADER_H
#define GPLATES_FILEIO_GPMLPROPERTYSTRUCTURALTYPEREADER_H

#include <map>
#include <boost/function.hpp>

#include <global/PointerTraits.h>

#include "model/PropertyValue.h"
#include "model/XmlNode.h"

#include "property-values/StructuralType.h"

#include "utils/ReferenceCount.h"


namespace GPlatesModel
{
	class GpgimVersion;
}

namespace GPlatesFileIO
{
	struct ReadErrorAccumulation;

	/**
	 * This class encapsulates mappings from (fully qualified) structural type names (for feature
	 * properties only) to creation functions that read them from a GPML file (XML element nodes).
	 *
	 *   structural type name  ----->  creation_function
	 *
	 * NOTE: Only structural types that can be feature properties are available in this class.
	 * For example, 'gpml:TopologicalSection' cannot be a feature property and hence is not present,
	 * whereas 'gpml:TopologicalPolygon' can be a feature property and hence is present.
	 */
	class GpmlPropertyStructuralTypeReader :
			public GPlatesUtils::ReferenceCount<GpmlPropertyStructuralTypeReader>
	{
	public:

		//! A convenience typedef for a shared pointer to a non-const @a GpmlPropertyStructuralTypeReader.
		typedef GPlatesUtils::non_null_intrusive_ptr<GpmlPropertyStructuralTypeReader> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GpmlPropertyStructuralTypeReader.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpmlPropertyStructuralTypeReader> non_null_ptr_to_const_type;


		/**
		 * Typedef for a function that reads a structural type (returned as a @a PropertyValue)
		 * from an XML element node.
		 */
		typedef boost::function<
				GPlatesModel::PropertyValue::non_null_ptr_type (
						const GPlatesModel::XmlElementNode::non_null_ptr_type &,
						const GPlatesModel::GpgimVersion &/*gpml_file_version*/,
						ReadErrorAccumulation &)>
								structural_type_reader_function_type;


		/**
		 * Creates a @a GpmlPropertyStructuralTypeReader object containing all structural types specified
		 * in the GPGIM (including the time-dependent wrapper structural types such as 'gpml:ConstantValue').
		 */
		static
		non_null_ptr_type
		create();


		/**
		 * Creates a @a GpmlPropertyStructuralTypeReader object with *no* structural types defined.
		 */
		static
		non_null_ptr_type
		create_empty()
		{
			return non_null_ptr_type(new GpmlPropertyStructuralTypeReader());
		}


		~GpmlPropertyStructuralTypeReader();


		/**
		 * Returns the structural type reader function associated with the specified
		 * (fully qualified) structural type.
		 *
		 * Returns boost::none if the specified structural type is not recognised.
		 */
		boost::optional<structural_type_reader_function_type>
		get_structural_type_reader_function(
				const GPlatesPropertyValues::StructuralType &structural_type) const;


		/**
		 * Adds the time-dependent wrapper structural types.
		 *
		 * Currently this includes:
		 *    'gpml:ConstantValue',
		 *    'gpml:IrregularSample', and
		 *    'gpml:PiecewiseAggregation'.
		 *
		 * Note: This is only really necessary if 'this' was created with @a create_empty.
		 */
		void
		add_time_dependent_wrapper_structural_types();


		/**
		 * Adds all native (non-enumeration) property structural types defined in the GPGIM.
		 *
		 * Note that native and enumeration structural types together form the entire set of
		 * non-time-dependent property structural types.
		 *
		 * Note: This is only really necessary if 'this' was created with @a create_empty.
		 */
		void
		add_native_structural_types();


		/**
		 * Adds all enumeration types defined in the GPGIM.
		 *
		 * Note that native and enumeration structural types together form the entire set of
		 * non-time-dependent property structural types.
		 *
		 * Note: This is only really necessary if 'this' was created with @a create_empty.
		 */
		void
		add_enumeration_structural_types();


		/**
		 * Adds an arbitrary structural type with its associated reader function.
		 *
		 * This is useful when reading old version GPML files containing deprecated native properties.
		 * In this case a reader function can be registered for the deprecated structural type so that
		 * it can be read. Subsequent processing can then upgrade it to a current version native property.
		 *
		 * Note: This is only really necessary if 'this' was created with @a create_empty.
		 */
		void
		add_structural_type(
				const GPlatesPropertyValues::StructuralType &structural_type,
				const structural_type_reader_function_type &reader_function);

	private:

		//! Typedef for a map of structural type to structural reader function.
		typedef std::map<
				GPlatesPropertyValues::StructuralType, 
				structural_type_reader_function_type> 
						structural_type_reader_map_type;


		structural_type_reader_map_type d_structural_type_reader_map;


		explicit
		GpmlPropertyStructuralTypeReader();


		void
		add_all_structural_types();

	};
}

#endif // GPLATES_FILEIO_GPMLPROPERTYSTRUCTURALTYPEREADER_H
