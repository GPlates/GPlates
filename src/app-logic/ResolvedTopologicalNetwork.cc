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
#include "global/IntrusivePointerZeroRefCountException.h"
#include "global/NotYetImplementedException.h"

#include "maths/ProjectionUtils.h"

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


void
GPlatesAppLogic::ResolvedTopologicalNetwork::report_deformation_to_file() const
{
#if 0
	// Get the current reconstruction time 
	const double t = get_reconstruction_time();

	// Compute the centroid of the boundary polygon and get lat lon for projection
	const GPlatesMaths::ProjectionUtils::AzimuthalEqualArea &projection =
			get_delaunay_triangulation_2().get_projection();

	// create and open a file and set up a Q text stream to write 
	QString debug_file_name = "LOG_Network_";
	static const GPlatesModel::PropertyName prop = GPlatesModel::PropertyName::create_gml("name");
	const GPlatesPropertyValues::XsString *name;
	QString feature_name = "unknown_name";
	if ( GPlatesFeatureVisitors::get_property_value( *feature_handle_ptr(), prop, name) ) {
		QString n = GPlatesUtils::make_qstring( name->value() );
		feature_name = n.replace(" ", "-");
	} 
	debug_file_name += feature_name;
	debug_file_name += "_at_";
	debug_file_name += QString::number(t);
	debug_file_name += "Ma";
	debug_file_name += ".xy";
	qDebug() << "debug_file_name = " << debug_file_name;
	QFile debug_qfile(debug_file_name);
	debug_qfile.open(QIODevice::WriteOnly | QIODevice::Text);
	QTextStream debug_qts(&debug_qfile);
	// write the header data 
	debug_qts << "# reconstuction age = " << t << "\n";
	debug_qts << "# proj_center lat = " << projection.get_center_of_projection().latitude() << "\n";
	debug_qts << "# proj_center lon = " << projection.get_center_of_projection().longitude() << "\n";

	// Iterate over the individual faces of the triangulation.
	ResolvedTriangulation::Delaunay_2::Finite_faces_iterator finite_faces_2_iter =
			get_delaunay_triangulation_2().finite_faces_begin();
	ResolvedTriangulation::Delaunay_2::Finite_faces_iterator finite_faces_2_end =
			get_delaunay_triangulation_2().finite_faces_end();
	for ( ; finite_faces_2_iter != finite_faces_2_end; ++finite_faces_2_iter)
	{
		debug_qts << "# face_index = " << finite_faces_2_iter->get_face_index() << "\n";

		debug_qts << "# ux1 = "
			<< finite_faces_2_iter->vertex(0)->calc_velocity_colat_lon().get_vector_longitude().dval()
			<< "(cm/yr)\n";
		debug_qts << "# uy1 = "
			<< finite_faces_2_iter->vertex(0)->calc_velocity_colat_lon().get_vector_colatitude().dval()
			<< "(cm/yr)\n";

		debug_qts << "# ux1 = "
			<< finite_faces_2_iter->vertex(1)->calc_velocity_colat_lon().get_vector_longitude().dval()
			<< "(cm/yr)\n";
		debug_qts << "# uy1 = "
			<< finite_faces_2_iter->vertex(1)->calc_velocity_colat_lon().get_vector_colatitude().dval()
			<< "(cm/yr)\n";

		debug_qts << "# ux1 = "
			<< finite_faces_2_iter->vertex(2)->calc_velocity_colat_lon().get_vector_longitude().dval()
			<< "(cm/yr)\n";
		debug_qts << "# uy1 = "
			<< finite_faces_2_iter->vertex(2)->v().get_vector_colatitude().dval()
			<< "(cm/yr)\n";

		const ResolvedTriangulation::Delaunay_2::Face::DeformationInfo &deformation_info =
				finite_faces_2_iter->get_deformation_info();  // DEBUG_FILE

		debug_qts << "# SR22 = " << deformation_info.SR22 << "(1/s)\n";
		debug_qts << "# SR33 = " << deformation_info.SR33 << "(1/s)\n";
		debug_qts << "# SR23 = " << deformation_info.SR23 << "(1/s)\n";
		debug_qts << "# SR23 = " << deformation_info.SR23 << "(1/s)\n";
		debug_qts << "# dil  = " << deformation_info.dilitation << "(1/s) [[ SR22 + SR33 ]]\n";
		debug_qts << "# 2ndI = " << deformation_info.second_invariant
			<< "(1/s) [[ std::sqrt(SR22 * SR22 + SR33 * SR33 + 2.0 * SR23 * SR23) ]]\n";
		// write the actual data line 
		debug_qts << " " << deformation_info.dilitation; // NOTE: no eol character
		debug_qts << "\n";
	}
#endif
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

