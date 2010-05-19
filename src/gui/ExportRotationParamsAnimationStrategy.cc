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
#include <iostream>
#include <sstream>
#include <QLocale> 
#include <QDebug>

#include "ExportRotationParamsAnimationStrategy.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/FeatureCollectionFileState.h"
#include "app-logic/ReconstructUtils.h"

#include "gui/ExportAnimationContext.h"
#include "gui/AnimationController.h"
#include "gui/CsvExport.h"

#include "maths/MathsUtils.h"

#include "model/ReconstructionTreeEdge.h"

#include "utils/FloatingPointComparisons.h"

#include "presentation/ViewState.h"

const QString GPlatesGui::ExportRotationParamsAnimationStrategy::DEFAULT_ROTATION_PARAMS_COMMA_FILENAME_TEMPLATE
		="equivalent_stage_rotation_comma_%0.2f.csv";
const QString GPlatesGui::ExportRotationParamsAnimationStrategy::DEFAULT_ROTATION_PARAMS_SEMI_FILENAME_TEMPLATE
		="equivalent_stage_rotation_semi_%0.2f.csv";

const QString GPlatesGui::ExportRotationParamsAnimationStrategy::DEFAULT_ROTATION_PARAMS_TAB_FILENAME_TEMPLATE
		="equivalent_stage_rotation_tab_%0.2f.csv";

const QString GPlatesGui::ExportRotationParamsAnimationStrategy::ROTATION_PARAMS_FILENAME_TEMPLATE_DESC
		=FORMAT_CODE_DESC;

const QString GPlatesGui::ExportRotationParamsAnimationStrategy::ROTATION_PARAMS_DESC =
		"Export equivalent stage(1My) rotation data:\n"
		"- 'equivalent' is from an exported plate id to the anchor plate,\n"
		"- 'stage' is from 't+1' Ma to 't' Ma where 't' is the export reconstruction time.\n"
		"Each line in exported file(s) will contain the following entries...\n"
		" 'plate_id' 'stage_pole_x' 'stage_pole_y' 'stage_pole_z' 'stage_pole_1My_angle'\n";

 

const GPlatesGui::ExportRotationParamsAnimationStrategy::non_null_ptr_type
GPlatesGui::ExportRotationParamsAnimationStrategy::create(
		GPlatesGui::ExportAnimationContext &export_animation_context,
		File_Format format,
		const ExportAnimationStrategy::Configuration& cfg)
{
	ExportRotationParamsAnimationStrategy* ptr=
		new ExportRotationParamsAnimationStrategy(
				export_animation_context,
				cfg.filename_template());
	ptr->d_format = format;
	
	switch(format)
	{
	case COMMA:
		ptr->d_delimiter=',';
		break;
	case SEMICOLON:
		ptr->d_delimiter=';';
		break;
	case TAB:
	default:
		ptr->d_delimiter='\t';
		break;
	}
		
	return non_null_ptr_type(
			ptr,
			GPlatesUtils::NullIntrusivePointerHandler());
}

GPlatesGui::ExportRotationParamsAnimationStrategy::ExportRotationParamsAnimationStrategy(
		GPlatesGui::ExportAnimationContext &export_animation_context,
		const QString &filename_template):
	ExportAnimationStrategy(export_animation_context)
{
	set_template_filename(filename_template);
}

bool
GPlatesGui::ExportRotationParamsAnimationStrategy::do_export_iteration(
		std::size_t frame_index)
{	
	if(!check_filename_sequence())
	{
		return false;
	}
	
	GPlatesUtils::ExportTemplateFilenameSequence::const_iterator &filename_it = 
		*d_filename_iterator_opt;

	GPlatesAppLogic::ApplicationState &application_state =
		d_export_animation_context_ptr->view_state().get_application_state();

	GPlatesModel::ReconstructionTree& tree1 = 
		application_state.get_current_reconstruction().reconstruction_tree();

	
	std::multimap<GPlatesModel::integer_plate_id_type,
			GPlatesModel::ReconstructionTreeEdge::non_null_ptr_type>::const_iterator it;
	std::multimap<GPlatesModel::integer_plate_id_type,
			GPlatesModel::ReconstructionTreeEdge::non_null_ptr_type>::const_iterator it_begin = 
			tree1.edge_map_begin();
	std::multimap<GPlatesModel::integer_plate_id_type,
			GPlatesModel::ReconstructionTreeEdge::non_null_ptr_type>::const_iterator it_end = 
			tree1.edge_map_end();

	GPlatesGui::CsvExport::LineDataType data_line;
	std::vector<GPlatesGui::CsvExport::LineDataType> data;
	
	GPlatesModel::ReconstructionTree::non_null_ptr_type tree2=
		GPlatesAppLogic::ReconstructUtils::create_reconstruction_tree(
				tree1.get_reconstruction_time()+1,
				tree1.get_anchor_plate_id(),
				tree1.get_reconstruction_features());

	for(it = it_begin; it != it_end ; ++it)
	{
		QString plate_id_string;
		QString axis_x_string, axis_y_string, axis_z_string;
		QString angle_string;

		GPlatesMaths::FiniteRotation fr_t2 = 
			tree2->get_composed_absolute_rotation(it->first).first;
		GPlatesMaths::FiniteRotation fr_t1 = 
			tree1.get_composed_absolute_rotation(it->first).first;

		// This quaternion represents a rotation between t1 and t2
		GPlatesMaths::UnitQuaternion3D q = fr_t2.unit_quat().get_inverse() * fr_t1.unit_quat();

		if ( represents_identity_rotation( q ) ) 
		{
			axis_x_string = axis_y_string = axis_z_string = angle_string = QObject::tr("Indeterminate");
		}
		else
		{
			const GPlatesMaths::UnitQuaternion3D::RotationParams params = 
				q.get_rotation_params(
						fr_t1.axis_hint());

			std::ostringstream ostr;
			ostr<<params.angle;
			angle_string=ostr.str().c_str();
			ostr.seekp(0);ostr.str("");
			
			ostr<<params.axis.x();
			axis_x_string=ostr.str().c_str();
			ostr.seekp(0);ostr.str("");
			
			ostr<<params.axis.y();
			axis_y_string=ostr.str().c_str();
			ostr.seekp(0);ostr.str("");
			
			ostr<<params.axis.z();
			axis_z_string=ostr.str().c_str();
			ostr.seekp(0);ostr.str("");
		}	
		
		plate_id_string.setNum(it->first);
		
		data_line.push_back(plate_id_string);
		data_line.push_back(axis_x_string);
		data_line.push_back(axis_y_string);
		data_line.push_back(axis_z_string);
		data_line.push_back(angle_string);
		
		data.push_back(data_line);
		data_line.clear();
	}
	
	CsvExport::ExportOptions option;
	option.delimiter=d_delimiter;
		
	CsvExport::export_data(
			QDir(
					d_export_animation_context_ptr->target_dir()).absoluteFilePath(
							*filename_it),
			option,
			data);

	filename_it++;

	// Normal exit, all good, ask the Context process the next iteration please.
	return true;
}

const QString&
GPlatesGui::ExportRotationParamsAnimationStrategy::get_default_filename_template()
{
	switch(d_format)
	{
	case COMMA:
		return DEFAULT_ROTATION_PARAMS_COMMA_FILENAME_TEMPLATE;
		break;
	case SEMICOLON:
		return DEFAULT_ROTATION_PARAMS_SEMI_FILENAME_TEMPLATE;
		break;
	case TAB:
	default:
		return DEFAULT_ROTATION_PARAMS_TAB_FILENAME_TEMPLATE;
		break;
	}
}

const QString&
GPlatesGui::ExportRotationParamsAnimationStrategy::get_filename_template_desc()
{
	return ROTATION_PARAMS_FILENAME_TEMPLATE_DESC;
}


