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

#include <boost/optional.hpp>

#include "global/PreconditionViolationError.h"

#include "app-logic/TopologyInternalUtils.h"

#include "model/FeatureType.h"
#include "model/PropertyName.h"


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
}

#endif // GPLATES_API_PYFEATURE_H
