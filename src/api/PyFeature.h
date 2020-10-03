/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2015 The University of Sydney, Australia
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

#ifndef GPLATES_API_PYFEATURE_H
#define GPLATES_API_PYFEATURE_H

#include <tuple>
#include <vector>
#include <boost/optional.hpp>

#include "app-logic/TopologyInternalUtils.h"

#include "global/PreconditionViolationError.h"
#include "global/python.h"

#include "maths/GeometryOnSphere.h"

#include "model/FeatureType.h"
#include "model/PropertyName.h"

#include "property-values/GmlDataBlock.h"


namespace GPlatesApi
{
	/**
	 * This exception can be thrown when there is more than one matching coverage with the same
	 * number of points (or same number of scalars).
	 *
	 * This means it's ambiguous which coverage range belongs to which coverage domain since they
	 * use the same domain/range property name.
	 */
	class AmbiguousGeometryCoverageException :
			public GPlatesGlobal::PreconditionViolationError
	{
	public:

		explicit
		AmbiguousGeometryCoverageException(
				const GPlatesUtils::CallStack::Trace &exception_source,
				const GPlatesModel::PropertyName &domain_property_name) :
			GPlatesGlobal::PreconditionViolationError(exception_source),
			d_domain_property_name(domain_property_name)
		{  }

		~AmbiguousGeometryCoverageException() throw()
		{  }

	protected:

		virtual
		const char *
		exception_name() const
		{
			return "AmbiguousGeometryCoverageException";
		}

		virtual
		void
		write_message(
				std::ostream &os) const;

	private:

		GPlatesModel::PropertyName d_domain_property_name;
	};


	/**
	 * Wrapping an enumeration instead of boolean since 'CoverageReturn.geometry_only' documents
	 * Python code better than a boolean.
	 */
	namespace CoverageReturn
	{
		enum Value
		{
			GEOMETRY_ONLY,       // Return only geometry.
			GEOMETRY_AND_SCALARS // Return geometry and scalars which is the coverage domain and range.
		};
	};


	/**
	 * Topological geometry property value types (topological line, polygon and network).
	 */
	typedef GPlatesAppLogic::TopologyInternalUtils::topological_geometry_property_value_type
			topological_geometry_property_value_type;


	/**
	 * Returns the default geometry property name associated with the specified feature type.
	 */
	boost::optional<GPlatesModel::PropertyName>
	get_default_geometry_property_name(
			const GPlatesModel::FeatureType &feature_type);


	/**
	 * Extract one geometry or coverage (geometry + scalars).
	 *
	 * 'geometry_or_coverage_object' can be:
	 *   1) a GeometryOnSphere, or
	 *   2) a coverage.
	 *
	 * ...where a 'coverage' is a (geometry-domain, geometry-range) sequence (eg, 2-tuple)
	 * and 'geometry-domain' is GeometryOnSphere and 'geometry-range' is a 'dict', or a sequence,
	 * of (scalar type, sequence of scalar values) 2-tuples.
	 */
	std::tuple<
			GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type,
			boost::optional<GPlatesPropertyValues::GmlDataBlock::non_null_ptr_type>>
	extract_geometry_or_coverage(
			boost::python::object geometry_or_coverage_object);


	/**
	 * Extract zero, one or more geometries or coverages (geometry + scalars).
	 *
	 * 'geometries_or_coverages_object' can be:
	 *   1) a GeometryOnSphere, or
	 *   2) a sequence of GeometryOnSphere's, or
	 *   3) a coverage, or
	 *   4) a sequence of coverages.
	 *
	 * ...where a 'coverage' is a (geometry-domain, geometry-range) sequence (eg, 2-tuple)
	 * and 'geometry-domain' is GeometryOnSphere and 'geometry-range' is a 'dict', or a sequence,
	 * of (scalar type, sequence of scalar values) 2-tuples.
	 */
	std::tuple<
			std::vector<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>,
			boost::optional<std::vector<GPlatesPropertyValues::GmlDataBlock::non_null_ptr_type>>>
	extract_geometries_or_coverages(
			boost::python::object geometries_or_coverages_object);
}

#endif // GPLATES_API_PYFEATURE_H
