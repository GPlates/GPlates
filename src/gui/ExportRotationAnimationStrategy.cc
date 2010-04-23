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
#include <QLocale> 

#include "ExportRotationAnimationStrategy.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/FeatureCollectionFileState.h"
#include "app-logic/Reconstruct.h"

#include "gui/ExportAnimationContext.h"
#include "gui/AnimationController.h"
#include "gui/CsvExport.h"

#include "maths/MathsUtils.h"

#include "model/ReconstructionTreeEdge.h"

#include "utils/FloatingPointComparisons.h"

#include "presentation/ViewState.h"

 const QString GPlatesGui::ExportRotationAnimationStrategy::DEFAULT_RELATIVE_COMMA_FILENAME_TEMPLATE
		="relative_rotation_comma_%0.2f.csv";

 const QString GPlatesGui::ExportRotationAnimationStrategy::DEFAULT_RELATIVE_SEMI_FILENAME_TEMPLATE
		="relative_rotation_semicomma_%0.2f.csv";
 
 const QString GPlatesGui::ExportRotationAnimationStrategy::DEFAULT_RELATIVE_TAB_FILENAME_TEMPLATE
		="relative_rotation_tab_%0.2f.csv";
 
 const QString GPlatesGui::ExportRotationAnimationStrategy::DEFAULT_EQUIVALENT_COMMA_FILENAME_TEMPLATE
		="equivalent_rotation_comma_%0.2f.csv";
 
 const QString GPlatesGui::ExportRotationAnimationStrategy::DEFAULT_EQUIVALENT_SEMI_FILENAME_TEMPLATE
		="equivalent_rotation_semicomma_%0.2f.csv";
 
 const QString GPlatesGui::ExportRotationAnimationStrategy::DEFAULT_EQUIVALENT_TAB_FILENAME_TEMPLATE
		="equivalent_rotation_tab_%0.2f.csv";
 
 const QString GPlatesGui::ExportRotationAnimationStrategy::ROTATION_FILENAME_TEMPLATE_DESC
		=FORMAT_CODE_DESC;

 const QString GPlatesGui::ExportRotationAnimationStrategy::RELATIVE_ROTATION_DESC
		="Export relative rotation data.";

 const QString GPlatesGui::ExportRotationAnimationStrategy::EQUIVALENT_ROTATION_DESC
		="Export equivalent rotation data.";

const GPlatesGui::ExportRotationAnimationStrategy::non_null_ptr_type
GPlatesGui::ExportRotationAnimationStrategy::create(
		GPlatesGui::ExportAnimationContext &export_animation_context,
		ExportRotationAnimationStrategy::RotationType type,
		const ExportAnimationStrategy::Configuration& cfg)
{
	ExportRotationAnimationStrategy* ptr=
		new ExportRotationAnimationStrategy(
				export_animation_context,
				cfg.filename_template());
	
	ptr->d_type=type;

	switch(type)
	{
	case RELATIVE_COMMA:
	case EQUIVALENT_COMMA:
		ptr->d_delimiter=',';
		break;
	case RELATIVE_SEMI:
	case EQUIVALENT_SEMI:
		ptr->d_delimiter=';';
		break;
	case RELATIVE_TAB:
	case EQUIVALENT_TAB:
	default:
		ptr->d_delimiter='\t';
		break;
	}
	return non_null_ptr_type(
			ptr,
			GPlatesUtils::NullIntrusivePointerHandler());
}

GPlatesGui::ExportRotationAnimationStrategy::ExportRotationAnimationStrategy(
		GPlatesGui::ExportAnimationContext &export_animation_context,
		const QString &filename_template):
	ExportAnimationStrategy(export_animation_context)
{
	set_template_filename(filename_template);
}

bool
GPlatesGui::ExportRotationAnimationStrategy::do_export_iteration(
		std::size_t frame_index)
{	
	if(!check_filename_sequence())
	{
		return false;
	}
	GPlatesUtils::ExportTemplateFilenameSequence::const_iterator &filename_it = 
		*d_filename_iterator_opt;

	GPlatesAppLogic::Reconstruct &reconstruct =
		d_export_animation_context_ptr->view_state().get_reconstruct();

	std::multimap<GPlatesModel::integer_plate_id_type,
			GPlatesModel::ReconstructionTreeEdge::non_null_ptr_type>::const_iterator it;
	std::multimap<GPlatesModel::integer_plate_id_type,
			GPlatesModel::ReconstructionTreeEdge::non_null_ptr_type>::const_iterator it_begin = 
			reconstruct.get_current_reconstruction().reconstruction_tree().edge_map_begin();
	std::multimap<GPlatesModel::integer_plate_id_type,
			GPlatesModel::ReconstructionTreeEdge::non_null_ptr_type>::const_iterator it_end = 
			reconstruct.get_current_reconstruction().reconstruction_tree().edge_map_end();

	GPlatesGui::CsvExport::LineDataType data_line;
	std::vector<GPlatesGui::CsvExport::LineDataType> data;

	for(it = it_begin; it != it_end ; ++it)
	{
		// insert the plate id into the first column of the table
		QString plate_id_string;
		QString euler_pole_lat_string;
		QString euler_pole_lon_string;
		QString angle_string;
		QString fixed_plate_id_string;

		plate_id_string.setNum(it->first);
				
		GPlatesMaths::FiniteRotation fr = it->second->composed_absolute_rotation();
		const GPlatesMaths::UnitQuaternion3D &uq = fr.unit_quat();
		
		if (GPlatesMaths::represents_identity_rotation(uq)) 
		{
			euler_pole_lat_string = euler_pole_lon_string = QObject::tr("Indeterminate");
			angle_string.setNum(0.0);
		} 
		else 
		{
			using namespace GPlatesMaths;
			UnitQuaternion3D::RotationParams params = uq.get_rotation_params(fr.axis_hint());

			PointOnSphere euler_pole(params.axis);
			LatLonPoint llp = make_lat_lon_point(euler_pole);

			QLocale locale_;
			euler_pole_lat_string = locale_.toString(llp.latitude());
			euler_pole_lon_string = locale_.toString(llp.longitude());

			angle_string = locale_.toString(
					GPlatesMaths::convert_rad_to_deg(params.angle).dval());
			
		}
		GPlatesModel::integer_plate_id_type fixed_id = it->second->fixed_plate();
		fixed_plate_id_string.setNum(fixed_id);

		data_line.push_back(plate_id_string);
		data_line.push_back(euler_pole_lat_string);
		data_line.push_back(euler_pole_lon_string);
		data_line.push_back(angle_string);
		
		if( (d_type == RELATIVE_COMMA)||
			(d_type == RELATIVE_SEMI) ||
			(d_type == RELATIVE_TAB) )
		{
			data_line.push_back(fixed_plate_id_string);
		}
		
		data.push_back(data_line);
		data_line.clear();
	}
	
	CsvExport::ExportOptions option;
	option.delimiter=d_delimiter;
		
	CsvExport::export_data(
			QDir(d_export_animation_context_ptr->target_dir()).absoluteFilePath(
					*filename_it),
			option,
			data);
	filename_it++;

	// Normal exit, all good, ask the Context process the next iteration please.
	return true;
}

const QString&
GPlatesGui::ExportRotationAnimationStrategy::get_default_filename_template()
{
	switch(d_type)
	{
	case RELATIVE_COMMA:
		return DEFAULT_RELATIVE_COMMA_FILENAME_TEMPLATE;
		break;
	case RELATIVE_SEMI:
		return DEFAULT_RELATIVE_SEMI_FILENAME_TEMPLATE;
		break;
	case RELATIVE_TAB:
		return DEFAULT_RELATIVE_TAB_FILENAME_TEMPLATE;
		break;
	case EQUIVALENT_COMMA:
		return DEFAULT_EQUIVALENT_COMMA_FILENAME_TEMPLATE;
		break;
	case EQUIVALENT_SEMI:
		return DEFAULT_EQUIVALENT_SEMI_FILENAME_TEMPLATE;
		break;
	case EQUIVALENT_TAB:
		return DEFAULT_EQUIVALENT_TAB_FILENAME_TEMPLATE;
		break;
	default:
		return DEFAULT_RELATIVE_COMMA_FILENAME_TEMPLATE;
		break;
	}
}

const QString&
GPlatesGui::ExportRotationAnimationStrategy::get_filename_template_desc()
{
	return ROTATION_FILENAME_TEMPLATE_DESC;
}


