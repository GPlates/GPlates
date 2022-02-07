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

#ifndef GPLATES_FEATUREVISITORS_TOTALRECONSTRUCTIONSEQUENCEPLATEIDFINDER_H
#define GPLATES_FEATUREVISITORS_TOTALRECONSTRUCTIONSEQUENCEPLATEIDFINDER_H

#include <vector>
#include <boost/optional.hpp>

#include "model/FeatureVisitor.h"
#include "model/PropertyName.h"
#include "model/types.h"
#include "model/FeatureHandle.h"


namespace GPlatesFeatureVisitors
{
	/**
	 * This const feature visitor finds the fixed and moving reference frame plate IDs within a
	 * total reconstruction sequence feature.
	 */
	class TotalReconstructionSequencePlateIdFinder:
			public GPlatesModel::ConstFeatureVisitor
	{
	public:
		// FIXME:  We should also pass the current reconstruction time, so we can correctly
		// handle time-dependent property values.
		TotalReconstructionSequencePlateIdFinder();

		virtual
		~TotalReconstructionSequencePlateIdFinder()
		{  }

		/**
		 * Reset a TotalReconstructionSequencePlateIdFinder instance, as if it were freshly
		 * instantiated.
		 *
		 * This operation is cheaper than instantiating a new instance.
		 */
		void
		reset();

		const boost::optional<GPlatesModel::integer_plate_id_type> &
		fixed_ref_frame_plate_id() const
		{
			return d_fixed_ref_frame_plate_id;
		}

		const boost::optional<GPlatesModel::integer_plate_id_type> &
		moving_ref_frame_plate_id() const
		{
			return d_moving_ref_frame_plate_id;
		}

	protected:

		virtual
		bool
		initialise_pre_property_values(
				const GPlatesModel::TopLevelPropertyInline &top_level_property_inline);

		virtual
		void
		visit_gpml_constant_value(
				const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value);

		virtual
		void
		visit_gpml_plate_id(
				const GPlatesPropertyValues::GpmlPlateId &gpml_plate_id);

	private:
		std::vector<GPlatesModel::PropertyName> d_property_names_to_allow;
		boost::optional<GPlatesModel::integer_plate_id_type> d_fixed_ref_frame_plate_id;
		boost::optional<GPlatesModel::integer_plate_id_type> d_moving_ref_frame_plate_id;
	};
}

#endif  // GPLATES_FEATUREVISITORS_TOTALRECONSTRUCTIONSEQUENCEPLATEIDFINDER_H
