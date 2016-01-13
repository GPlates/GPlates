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

#ifndef GPLATES_API_PYPLATEPARTITIONER_H
#define GPLATES_API_PYPLATEPARTITIONER_H

#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>

#include "PyFeatureCollection.h"
#include "PyRotationModel.h"

#include "app-logic/GeometryCookieCutter.h"

#include "global/PreconditionViolationError.h"


#if !defined(GPLATES_NO_PYTHON)

namespace GPlatesApi
{
	/**
	 * The reconstruction times of a group of partitioning plates are not all the same.
	 */
	class DifferentTimesInPartitioningPlatesException :
			public GPlatesGlobal::PreconditionViolationError
	{
	public:
		explicit
		DifferentTimesInPartitioningPlatesException(
				const GPlatesUtils::CallStack::Trace &exception_source) :
			GPlatesGlobal::PreconditionViolationError(exception_source)
		{  }

		~DifferentTimesInPartitioningPlatesException() throw()
		{  }

	protected:

		virtual
		const char *
		exception_name() const
		{
			return "DifferentTimesInPartitioningPlatesException";
		}

	};


	/**
	 * Enumerated properties to copy.
	 *
	 * This includes a very small number of common properties.
	 * Other properties should be specified using property names.
	 *
	 * This also handles those cases where *part* of a property needs to be copied
	 * (such as only the begin time of a 'gpml:validTime' property).
	 */
	namespace PartitionProperty
	{
		enum Value
		{
			//! Property name 'gpml:reconstructionPlateId'.
			RECONSTRUCTION_PLATE_ID,

			//! Property name 'gpml:validTime'.
			VALID_TIME_PERIOD,

			//! Begin time of property name 'gpml:validTime'.
			VALID_TIME_BEGIN,

			//! End time of property name 'gpml:validTime'.
			VALID_TIME_END
		};
	};


	/**
	 * Enumerated properties to copy.
	 *
	 * This includes a very small number of common properties.
	 * Other properties should be specified using property names.
	 *
	 * This also handles those cases where *part* of a property needs to be copied
	 * (such as only the begin time of a 'gpml:validTime' property).
	 */
	namespace PartitionReturn
	{
		enum Value
		{
			//! Return a combined list of partitioned and unpartitioned features.
			COMBINED_PARTITIONED_AND_UNPARTITIONED,

			//! Return separate lists for partitioned and unpartitioned features.
			SEPARATE_PARTITIONED_AND_UNPARTITIONED,

			/**
			 * Return a list of partitioned groups and a list of unpartitioned features.
			 *
			 * Each partition group is a 2-tuple (partitioning plate, features inside partition).
			 */
			PARTITIONED_GROUPS_AND_UNPARTITIONED
		};
	};


	/**
	 * Enumerated ways to sort partitioning plates.
	 */
	namespace SortPartitioningPlates
	{
		enum Value
		{
			// Group in order of resolved topological networks then resolved topological boundaries
			// then reconstructed static polygons, but with no sorting within each group.
			BY_PARTITION_TYPE,

			// Same as @a BY_PARTITION_TYPE but also sort by plate ID (from highest to lowest)
			// within each partition type group.
			BY_PARTITION_TYPE_THEN_PLATE_ID,

			// Same as @a BY_PARTITION_TYPE but also sort by plate area (from highest to lowest)
			// within each partition type group.
			BY_PARTITION_TYPE_THEN_PLATE_AREA,

			// Sort by plate ID (from highest to lowest), but no grouping by partition type.
			BY_PLATE_ID,

			// Sort by plate area (from highest to lowest), but no grouping by partition type.
			BY_PLATE_AREA
		};
	};


	/**
	 * A Python wrapper around a @a GeometryCookieCutter that keeps any referenced features alive
	 * (if plates reconstructed/resolved from features).
	 *
	 * Keeping the referenced features alive is important because partitioning results can return
	 * @a ReconstructionGeometry objects which, in turn, reference features (but only weak references).
	 *
	 * This is the wrapper type that gets stored in the python object.
	 */
	class PlatePartitionerWrapper
	{
	public:

		/**
		 * The default boost-python 'pointee<HeldType>::type' is defined as 'HeldType::element_type'.
		 *
		 * This is needed for wrapped types ('HeldType') that are not already smart pointers.
		 */
	    typedef GPlatesAppLogic::GeometryCookieCutter element_type;

		explicit
		PlatePartitionerWrapper(
				const boost::shared_ptr<GPlatesAppLogic::GeometryCookieCutter> &geometry_cookie_cutter,
				boost::optional<RotationModel::non_null_ptr_type> rotation_model = boost::none,
				boost::optional<const FeatureCollectionSequenceFunctionArgument &> partitioning_features = boost::none,
				boost::optional<const std::vector<boost::python::object> &> partitioning_plates = boost::none) :
			d_geometry_cookie_cutter(geometry_cookie_cutter),
			d_rotation_model(rotation_model)
		{
			if (partitioning_features)
			{
				d_partitioning_features = partitioning_features.get();
			}
			if (partitioning_plates)
			{
				d_partitioning_plates = partitioning_plates.get();
			}
		}

		/**
		 * Get the wrapped geometry cookie cutter.
		 */
		boost::shared_ptr<GPlatesAppLogic::GeometryCookieCutter>
		get_geometry_cookie_cutter() const
		{
			return d_geometry_cookie_cutter;
		}

		/**
		 * Get the rotation model (if any).
		 */
		boost::optional<RotationModel::non_null_ptr_type>
		get_rotation_model() const
		{
			return d_rotation_model;
		}

	private:

		boost::shared_ptr<GPlatesAppLogic::GeometryCookieCutter> d_geometry_cookie_cutter;

		//! A rotation model if needed to reverse reconstruct feature geometries after cookie-cutting.
		boost::optional<RotationModel::non_null_ptr_type> d_rotation_model;

		//! Keep any partitioning features alive since returned partitioning reconstruction geometries reference them.
		boost::optional<FeatureCollectionSequenceFunctionArgument> d_partitioning_features;

		/**
		 * The Python reconstruction geometries.
		 *
		 * These objects keep their referenced partitioning features alive.
		 * This keeps the features alive until the partitioning reconstruction geometries are returned
		 * back to Python (from C++) at which point the returned reconstruction geometries will again
		 * keep their referenced features alive.
		 *
		 * This is useful when the Python user reconstructs/resolves some reconstruction geometries,
		 * then uses them to create a plate partitioner and then discards them (and their referenced
		 * features). If we (the Python-wrapped plate partitioner) didn't keep the features alive then
		 * the returned reconstruction geometries (in partitioning results) would have null references
		 * to their features.
		 */
		std::vector<boost::python::object> d_partitioning_plates;
	};

	/**
	 * Boost-python requires 'get_pointer(HeldType)' for wrapped types ('HeldType') that
	 * are not already smart pointers.
	 */
	inline
	GPlatesAppLogic::GeometryCookieCutter *
	get_pointer(
			const PlatePartitionerWrapper &wrapper)
	{
		return wrapper.get_geometry_cookie_cutter().get();
	}
}

#endif   // GPLATES_NO_PYTHON

#endif // GPLATES_API_PYPLATEPARTITIONER_H
