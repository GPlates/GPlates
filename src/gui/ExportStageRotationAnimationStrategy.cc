/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
 *
 * This file is part of GPlates.
 *
 * GPlates is free software; you can redistribute tree1_edges_iter and/or modify tree1_edges_iter under
 * the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 *
 * GPlates is distributed in the hope that tree1_edges_iter will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include <iostream>
#include <QLocale>
#include <QDebug>

#include "ExportStageRotationAnimationStrategy.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/FeatureCollectionFileState.h"
#include "app-logic/Reconstruction.h"
#include "app-logic/ReconstructionTree.h"
#include "app-logic/ReconstructionTreeCreator.h"
#include "app-logic/RotationUtils.h"

#include "global/GPlatesAssert.h"

#include "gui/ExportAnimationContext.h"
#include "gui/AnimationController.h"
#include "gui/CsvExport.h"

#include "maths/MathsUtils.h"

#include "presentation/ViewState.h"


GPlatesGui::ExportStageRotationAnimationStrategy::ExportStageRotationAnimationStrategy(
		GPlatesGui::ExportAnimationContext &export_animation_context,
		const const_configuration_ptr &configuration):
	ExportAnimationStrategy(export_animation_context),
	d_configuration(configuration)
{
	set_template_filename(d_configuration->get_filename_template());
}

bool
GPlatesGui::ExportStageRotationAnimationStrategy::do_export_iteration(
		std::size_t frame_index)
{	
	GPlatesFileIO::ExportTemplateFilenameSequence::const_iterator &filename_it = 
		*d_filename_iterator_opt;

	GPlatesAppLogic::ApplicationState &application_state =
		d_export_animation_context_ptr->view_state().get_application_state();
	const GPlatesAppLogic::Reconstruction &reconstruction = application_state.get_current_reconstruction();

	// Export the default rotation layer.
	//
	// Now that layers enables users to have more than one reconstruction tree we need to
	// distinguish which one the user intends to export.
	//
	// FIXME: For now we just use the default reconstruction tree generated by the default
	// reconstruction tree layer.
	// Later we might want to let the user choose.
	GPlatesAppLogic::ReconstructionTreeCreator tree_creator =
			reconstruction.get_default_reconstruction_layer_output()->get_reconstruction_tree_creator();

	GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type tree1 =
			tree_creator.get_reconstruction_tree(reconstruction.get_reconstruction_time());
	
	const GPlatesAppLogic::ReconstructionTree::edge_map_type &tree1_edges = tree1->get_all_edges();
	GPlatesAppLogic::ReconstructionTree::edge_map_type::const_iterator tree1_edges_iter;
	GPlatesAppLogic::ReconstructionTree::edge_map_type::const_iterator tree1_edges_begin = tree1_edges.begin();
	GPlatesAppLogic::ReconstructionTree::edge_map_type::const_iterator tree1_edges_end = tree1_edges.end();

	GPlatesGui::CsvExport::LineDataType data_line;
	std::vector<GPlatesGui::CsvExport::LineDataType> data;

	// Use the stage time interval requested.
	const double stage_time_interval = d_configuration->stage_rotation_options.time_interval;
	
	GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type tree2 =
			tree_creator.get_reconstruction_tree(
					reconstruction.get_reconstruction_time() + stage_time_interval);

	for (tree1_edges_iter = tree1_edges_begin; tree1_edges_iter != tree1_edges_end ; ++tree1_edges_iter)
	{
		const bool is_relative_rotation = (
				(d_configuration->rotation_type == Configuration::RELATIVE_COMMA) ||
				(d_configuration->rotation_type == Configuration::RELATIVE_SEMICOLON) ||
				(d_configuration->rotation_type == Configuration::RELATIVE_TAB) );

		const GPlatesAppLogic::ReconstructionTree::Edge *tree1_edge = tree1_edges_iter->second;

		const GPlatesModel::integer_plate_id_type fixed_plate_id = tree1_edge->get_fixed_plate();
		const GPlatesModel::integer_plate_id_type moving_plate_id = tree1_edge->get_moving_plate();

		const GPlatesMaths::UnitQuaternion3D q = is_relative_rotation
				? get_relative_stage_rotation(*tree1, *tree2, moving_plate_id, fixed_plate_id)
				: get_equivalent_stage_rotation(*tree1, *tree2, moving_plate_id);

		QString plate_id_string;
		QString axis_x_string, axis_y_string, axis_z_string;
		QString axis_lat_string, axis_lon_string;
		QString angle_string;

		QLocale locale;

		plate_id_string.setNum(moving_plate_id);

		if ( represents_identity_rotation( q ) ) 
		{
			switch (d_configuration->rotation_options.identity_rotation_format)
			{
			case ExportOptionsUtils::ExportRotationOptions::WRITE_IDENTITY_AS_INDETERMINATE:
				axis_x_string = axis_y_string = axis_z_string = QObject::tr("Indeterminate");
				axis_lat_string = axis_lon_string = QObject::tr("Indeterminate");
				angle_string = QObject::tr("Indeterminate");
				break;

			case ExportOptionsUtils::ExportRotationOptions::WRITE_IDENTITY_AS_NORTH_POLE:
				axis_x_string = locale.toString(0.0);
				axis_y_string = locale.toString(0.0);
				axis_z_string = locale.toString(1.0);
				axis_lat_string = locale.toString(90.0);
				axis_lon_string = locale.toString(0.0);
				angle_string = locale.toString(0.0);
				break;

			default:
				// Shouldn't get here.
				GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
				break;
			}
		}
		else
		{
			// Note we're not using an axis hint here.
			//
			// Since stage rotations are 'differences' between two total rotations at nearby times
			// the resulting stage rotation is not likely to be aligned with either total rotation.
			// Hence the axis hint is not useful - also note that the axis hint chooses between
			// two rotations that are equivalent to each other where one is the antipodal axis of the
			// other (and the negative angle of the other) - but the effective rotation is the same.
			const GPlatesMaths::UnitQuaternion3D::RotationParams params =
					q.get_rotation_params(boost::none);

			axis_x_string = locale.toString(params.axis.x().dval());
			axis_y_string = locale.toString(params.axis.y().dval());
			axis_z_string = locale.toString(params.axis.z().dval());

			GPlatesMaths::PointOnSphere euler_pole(params.axis);
			GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(euler_pole);

			axis_lat_string = locale.toString(llp.latitude());
			axis_lon_string = locale.toString(llp.longitude());

			angle_string = locale.toString(
					GPlatesMaths::convert_rad_to_deg(params.angle).dval());
		}
		
		data_line.push_back(plate_id_string);

		// Write out the euler pole depending on the pole format requested.
		switch (d_configuration->rotation_options.euler_pole_format)
		{
		case ExportOptionsUtils::ExportRotationOptions::WRITE_EULER_POLE_AS_LATITUDE_LONGITUDE:
			data_line.push_back(axis_lat_string);
			data_line.push_back(axis_lon_string);
			break;

		case ExportOptionsUtils::ExportRotationOptions::WRITE_EULER_POLE_AS_CARTESIAN:
			data_line.push_back(axis_x_string);
			data_line.push_back(axis_y_string);
			data_line.push_back(axis_z_string);
			break;

		default:
			// Shouldn't get here.
			GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
			break;
		}

		data_line.push_back(angle_string);

		if (is_relative_rotation)
		{
			QString fixed_plate_id_string;
			fixed_plate_id_string.setNum(fixed_plate_id);

			data_line.push_back(fixed_plate_id_string);
		}

		data.push_back(data_line);
		data_line.clear();
	}

	CsvExport::ExportOptions option;
	switch(d_configuration->rotation_type)
	{
	case Configuration::RELATIVE_COMMA:
	case Configuration::EQUIVALENT_COMMA:
		option.delimiter = ',';
		break;

	case Configuration::RELATIVE_SEMICOLON:
	case Configuration::EQUIVALENT_SEMICOLON:
		option.delimiter = ';';
		break;

	case Configuration::RELATIVE_TAB:
	case Configuration::EQUIVALENT_TAB:
	default:
		option.delimiter = '\t';
		break;
	}
		
	CsvExport::export_data(
			QDir(d_export_animation_context_ptr->target_dir()).absoluteFilePath(
					*filename_it),
			option,
			data);

	filename_it++;

	// Normal exit, all good, ask the Context process the next iteration please.
	return true;
}


GPlatesMaths::UnitQuaternion3D
GPlatesGui::ExportStageRotationAnimationStrategy::get_relative_stage_rotation(
		const GPlatesAppLogic::ReconstructionTree &tree1,
		const GPlatesAppLogic::ReconstructionTree &tree2,
		GPlatesModel::integer_plate_id_type moving_plate_id,
		GPlatesModel::integer_plate_id_type fixed_plate_id) const
{
	// This rotation represents a rotation from t2 (the older time) to t1 (closer to present day).
	// The rotation is from the fixed plate to the moving plate.
	return GPlatesAppLogic::RotationUtils::get_stage_pole(
			tree2,
			tree1,
			moving_plate_id,
			fixed_plate_id).unit_quat();
}


GPlatesMaths::UnitQuaternion3D
GPlatesGui::ExportStageRotationAnimationStrategy::get_equivalent_stage_rotation(
		const GPlatesAppLogic::ReconstructionTree &tree1,
		const GPlatesAppLogic::ReconstructionTree &tree2,
		GPlatesModel::integer_plate_id_type moving_plate_id) const
{
	GPlatesMaths::FiniteRotation fr_t1 = tree1.get_composed_absolute_rotation(moving_plate_id);
	GPlatesMaths::FiniteRotation fr_t2 = tree2.get_composed_absolute_rotation(moving_plate_id);

	// This quaternion represents a rotation from t2 (the older time) to t1 (closer to present day).
	// The rotation is from the anchor plate to the moving plate.
	//
	// R(t2->t1,A->M)
	//    = R(0->t1,A->M) * R(t2->0,A->M)
	//    = R(0->t1,A->M) * inverse[R(0->t2,A->M)]
	//
	// ...where 'A' is the anchor plate and 'M' is the moving plate.
	//
	return fr_t1.unit_quat() * fr_t2.unit_quat().get_inverse();
}
