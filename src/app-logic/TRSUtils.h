/* $Id: EditTimeSequenceWidget.cc 8310 2010-05-06 15:02:23Z rwatson $ */

/**
 * \file 
 * $Revision: 8310 $
 * $Date: 2010-05-06 17:02:23 +0200 (to, 06 mai 2010) $ 
 * 
 * Copyright (C) 2011 Geological Survey of Norway
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
#ifndef GPLATESAPPLOGIC_TRSUTILS_H
#define GPLATESAPPLOGIC_TRSUTILS_H

#include <QTreeWidgetItem>

#include "model/FeatureHandle.h"
#include "model/FeatureVisitor.h"
#include "property-values/GpmlIrregularSampling.h"

namespace GPlatesAppLogic
{
	namespace TRSUtils
	{


	/**
	 * Finds irregular-sampling and plate-id properties, and their iterators, from a TRS feature.
	 *
	 */
	class TRSFinder:
		public GPlatesModel::FeatureVisitor
	{

	public:

		TRSFinder();

		virtual
		~TRSFinder()
		{ }

		void
		reset();

		bool
		can_process_trs();

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

		const boost::optional<GPlatesPropertyValues::GpmlIrregularSampling::non_null_ptr_type> &
		irregular_sampling() const
		{
			return d_irregular_sampling;
		}

		boost::optional<GPlatesModel::FeatureHandle::iterator>
		irregular_sampling_property_iterator()
		{
			return d_irregular_sampling_iterator;
		}

		boost::optional<GPlatesModel::FeatureHandle::iterator>
		moving_ref_frame_property_iterator()
		{
			return d_moving_ref_frame_iterator;
		}

		boost::optional<GPlatesModel::FeatureHandle::iterator>
		fixed_ref_frame_property_iterator()
		{
			return d_fixed_ref_frame_iterator;
		}


	private:


		virtual
		bool
                initialise_pre_property_values(
                        top_level_property_inline_type &top_level_property_inline);

		virtual
		void
		visit_gpml_constant_value(
			GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value);

		virtual
		void
		visit_gpml_irregular_sampling(
			GPlatesPropertyValues::GpmlIrregularSampling &gpml_irregular_sampling);

		virtual
		void
		visit_gpml_plate_id(
			GPlatesPropertyValues::GpmlPlateId &gpml_plate_id);

		std::vector<GPlatesModel::PropertyName> d_property_names_to_allow;
		boost::optional<GPlatesModel::FeatureHandle::iterator> d_irregular_sampling_iterator;
		boost::optional<GPlatesPropertyValues::GpmlIrregularSampling::non_null_ptr_type> d_irregular_sampling;
		boost::optional<GPlatesModel::FeatureHandle::iterator> d_moving_ref_frame_iterator;
		boost::optional<GPlatesModel::integer_plate_id_type> d_moving_ref_frame_plate_id;
		boost::optional<GPlatesModel::FeatureHandle::iterator> d_fixed_ref_frame_iterator;
		boost::optional<GPlatesModel::integer_plate_id_type> d_fixed_ref_frame_plate_id;
	};

		QString 
		build_trs_summary_string_from_trs_feature(
			const GPlatesModel::FeatureHandle::weak_ref &trs_feature);

		bool
		one_of_trs_plate_ids_is_999
			(const GPlatesModel::FeatureHandle::weak_ref &trs_feature);
	}
}

#endif // GPLATESAPPLOGIC_TRSUTILS_H
