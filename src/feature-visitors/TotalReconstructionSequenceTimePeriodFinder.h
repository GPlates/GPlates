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

#ifndef GPLATES_FEATUREVISITORS_TOTALRECONSTRUCTIONSEQUENCETIMEPERIODFINDER_H
#define GPLATES_FEATUREVISITORS_TOTALRECONSTRUCTIONSEQUENCETIMEPERIODFINDER_H

#include <vector>
#include <boost/optional.hpp>

#include "model/FeatureVisitor.h"
#include "model/PropertyName.h"
#include "model/FeatureHandle.h"
#include "property-values/GeoTimeInstant.h"


namespace GPlatesFeatureVisitors
{
	/**
	 * This const feature visitor finds the begin and end times of a
	 * total reconstruction sequence feature.
	 */
	class TotalReconstructionSequenceTimePeriodFinder:
			public GPlatesModel::ConstFeatureVisitor
	{
	public:
		/**
		 * Create a new finder instance.
		 *
		 * In general, you want @a skip_over_disabled_samples to be true, unless you have
		 * a specific reason to retain disabled samples (for example, if you're displaying
		 * the contents of rotation files).
		 */
		explicit
		TotalReconstructionSequenceTimePeriodFinder(
				bool skip_over_disabled_samples = true);

		virtual
		~TotalReconstructionSequenceTimePeriodFinder()
		{  }

		/**
		 * Reset a TotalReconstructionSequenceTimePeriodFinder instance, as if it were
		 * freshly instantiated.
		 *
		 * This operation is cheaper than instantiating a new instance.
		 */
		void
		reset();

		/**
		 * Access the "begin" time of the TRS, if one was found.
		 *
		 * Note that this boost::optional might be boost::none, the sequence didn't contain
		 * any non-disabled time samples.
		 *
		 * The "begin" and "end" values are analogous to the properties of the same name in
		 * the "gml:TimePeriod" structural type.  In GPlates, the "begin" property is
		 * assumed to be earlier than (or simultaneous with) the "end" property; similarly,
		 * this "begin" value, if found, will be earlier (ie, further in the past) than the
		 * "end" value.
		 */
		const boost::optional<GPlatesPropertyValues::GeoTimeInstant> &
		begin_time() const
		{
			return d_begin_time;
		}

		/**
		 * Access the "end" time of the TRS, if one was found.
		 *
		 * Note that this boost::optional might be boost::none, the sequence didn't contain
		 * any non-disabled time samples.
		 *
		 * The "begin" and "end" values are analogous to the properties of the same name in
		 * the "gml:TimePeriod" structural type.  In GPlates, the "end" property is assumed
		 * to be later than (or simultaneous with) the "begin" property; similarly, this
		 * "end" value, if found, will be later (ie, less far in the past) than the "begin"
		 * value.
		 */
		const boost::optional<GPlatesPropertyValues::GeoTimeInstant> &
		end_time() const
		{
			return d_end_time;
		}

	protected:

		virtual
		bool
		initialise_pre_property_values(
				const GPlatesModel::TopLevelPropertyInline &top_level_property_inline);

		virtual
		void
		visit_gpml_irregular_sampling(
				const GPlatesPropertyValues::GpmlIrregularSampling &gpml_irregular_sampling);

	private:
		/**
		 * Whether client code wants us to skip over any disabled time samples when
		 * iterating through the irregular sampling.
		 *
		 * (In general, it @em will want us to skip over any disabled time samples, which
		 * is why this member is initialised to true by default.  However, when displaying
		 * rotation files in the TotalReconstructionSequencesDialog, we want to include
		 * @em all time samples, even disabled ones, and @em all reconstruction sequences,
		 * even those that contain @em only disabled time samples.)
		 */
		bool d_skip_over_disabled_samples;

		std::vector<GPlatesModel::PropertyName> d_property_names_to_allow;
		boost::optional<GPlatesModel::PropertyName> d_most_recent_propname_read;
		boost::optional<GPlatesPropertyValues::GeoTimeInstant> d_begin_time;
		boost::optional<GPlatesPropertyValues::GeoTimeInstant> d_end_time;
	};
}

#endif  // GPLATES_FEATUREVISITORS_TOTALRECONSTRUCTIONSEQUENCETIMEPERIODFINDER_H
