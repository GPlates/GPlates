/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 The University of Sydney, Australia
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
#include "maths/ProximityHitDetail.h"


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
					GPlatesModel::ReconstructionGeometry::non_null_ptr_type recon_geometry,
					GPlatesMaths::ProximityHitDetail::non_null_ptr_type detail):
				d_recon_geometry(recon_geometry),
				d_detail(detail),
				d_proximity(detail->closeness())
			{  }

			bool
			operator<(
					const ProximityHit &other) const
			{
				return (d_proximity < other.d_proximity);
			}

			GPlatesModel::ReconstructionGeometry::non_null_ptr_type d_recon_geometry;
			GPlatesMaths::ProximityHitDetail::non_null_ptr_type d_detail;
			double d_proximity;
		};


#if 0
		struct ProximityAggregator
		{
			explicit
			ProximityAggregator(
					std::priority_queue<ProximityHit> &sorted_hits):
				d_sorted_hits_ptr(&sorted_hits)
			{  }

			void
			insert_hit(
					GPlatesModel::FeatureHandle &feature,
					const double &proximity)
			{
				d_sorted_hits_ptr->push(ProximityHit(feature.reference(), proximity));
			}

			std::priority_queue<ProximityHit> *d_sorted_hits_ptr;
		};
#endif


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
				const double &proximity_inclusion_threshold);
	}
}

#endif  // GPLATES_GUI_PROXIMITYTESTS_H
