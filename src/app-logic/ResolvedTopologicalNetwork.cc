/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
 * Copyright (C) 2012, 2013 California Institute of Technology
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


// FIXME: does not work ; need to fix references to triangulation
// NOTE: use with caution: this can cause the Log window to lag during resize events.
// #define DEBUG_FILE


#include <QDebug>
#include <QFile>
#include <QString>
#include <QTextStream>

#include "ResolvedTopologicalNetwork.h"

#include "ApplicationState.h"
#include "ReconstructionGeometryVisitor.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/NotYetImplementedException.h"

#include "maths/AzimuthalEqualAreaProjection.h"

#include "model/PropertyName.h"
#include "model/WeakObserverVisitor.h"

#include "property-values/XsString.h"

#include "utils/GeometryCreationUtils.h"
#include "utils/UnicodeStringUtils.h"


const GPlatesModel::FeatureHandle::weak_ref
GPlatesAppLogic::ResolvedTopologicalNetwork::get_feature_ref() const
{
	if (is_valid()) {
		return feature_handle_ptr()->reference();
	} else {
		return GPlatesModel::FeatureHandle::weak_ref();
	}
}


const GPlatesAppLogic::resolved_vertex_source_info_seq_type &
GPlatesAppLogic::ResolvedTopologicalNetwork::get_vertex_source_infos() const
{
	// Cache all vertex source infos on first call.
	if (!d_vertex_source_infos)
	{
		calc_vertex_source_infos();
	}

	return d_vertex_source_infos.get();
}


void
GPlatesAppLogic::ResolvedTopologicalNetwork::calc_vertex_source_infos() const
{
	d_vertex_source_infos = resolved_vertex_source_info_seq_type();
	resolved_vertex_source_info_seq_type &vertex_source_infos = d_vertex_source_infos.get();

	// Copy source infos from points in each boundary subsegment.
	sub_segment_seq_type::const_iterator boundary_sub_segments_iter = d_boundary_sub_segment_seq.begin();
	sub_segment_seq_type::const_iterator boundary_sub_segments_end = d_boundary_sub_segment_seq.end();
	for ( ; boundary_sub_segments_iter != boundary_sub_segments_end; ++boundary_sub_segments_iter)
	{
		const ResolvedTopologicalGeometrySubSegment::non_null_ptr_type &boundary_sub_segment = *boundary_sub_segments_iter;
		// Subsegment should be reversed if that's how it contributed to the resolved topological network...
		boundary_sub_segment->get_reversed_sub_segment_point_source_infos(vertex_source_infos);
	}
}


void
GPlatesAppLogic::ResolvedTopologicalNetwork::accept_visitor(
		ConstReconstructionGeometryVisitor &visitor) const
{
	visitor.visit(this->get_non_null_pointer_to_const());
}


void
GPlatesAppLogic::ResolvedTopologicalNetwork::accept_visitor(
		ReconstructionGeometryVisitor &visitor)
{
	visitor.visit(this->get_non_null_pointer());
}

void
GPlatesAppLogic::ResolvedTopologicalNetwork::accept_weak_observer_visitor(
		GPlatesModel::WeakObserverVisitor<GPlatesModel::FeatureHandle> &visitor)
{
	visitor.visit_resolved_topological_network(*this);
}
