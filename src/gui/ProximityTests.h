/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006, 2007 The University of Sydney, Australia
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
 
#ifndef GPLATES_GUI_PROXIMITYTESTS_H
#define GPLATES_GUI_PROXIMITYTESTS_H

#include <queue>  // std::priority_queue

#include "model/Reconstruction.h"
#include "model/FeatureHandle.h"


namespace GPlatesGui
{
	namespace ProximityTests
	{
		/**
		 * This class represents a "hit" according to proximity on the globe to a
		 * reconstructed feature geometry (RFG) in a reconstruction.
		 *
		 * An instance of this class contains a weak-ref to the RFG's feature, as well as
		 * the calculated proximity of the RFG.
		 *
		 * This class is intended to be used as the value_type argument to an STL
		 * priority-queue.
		 */
		struct ProximityHit
		{
			ProximityHit(
					const GPlatesModel::FeatureHandle::weak_ref &feature,
					const double &proximity):
				d_feature(feature),
				d_proximity(proximity)
			{  }

			bool
			operator<(
					const ProximityHit &other) const
			{
				return (d_proximity < other.d_proximity);
			}

			GPlatesModel::FeatureHandle::weak_ref d_feature;
			double d_proximity;
		};


		/**
		 * Populate the supplied priority-queue @a sorted_hits with ProximityHit instances
		 * which reference features whose reconstructed feature geometry (RFG) in @a recon
		 * is "close" to @a test_point.
		 *
		 * How "close" to @a test_point an RFG must be, to be considered a "hit", is
		 * determined by the value of @a proximity_inclusion_threshold.  The value of this
		 * parameter should be close to, but strictly less-than, 1.0.  The closer the value
		 * of @a proximity_inclusion_threshold to 1.0, the closer an RFG must be to @a
		 * test_point to be considered a "hit".  A useful value might be around 0.9997 or
		 * 0.9998.
		 */
		void
		find_close_rfgs(
				std::priority_queue<ProximityHit> &sorted_hits,
				GPlatesModel::Reconstruction &recon,
				const GPlatesMaths::PointOnSphere &test_point,
				const GPlatesMaths::real_t &proximity_inclusion_threshold);
	}
}

#endif  // GPLATES_GUI_PROXIMITYTESTS_H
