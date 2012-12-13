/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#include "ExportCitcomsResolvedTopologyOptionsWidget.h"

#include "DatelineWrapOptionsWidget.h"
#include "QtWidgetUtils.h"


GPlatesQtWidgets::ExportCitcomsResolvedTopologyOptionsWidget::ExportCitcomsResolvedTopologyOptionsWidget(
		QWidget *parent_,
		const GPlatesGui::ExportCitcomsResolvedTopologyAnimationStrategy::const_configuration_ptr &
				default_export_configuration,
		bool configure_dateline_wrapping) :
	ExportOptionsWidget(parent_),
	d_export_configuration(*default_export_configuration),
	d_dateline_wrap_options_widget(NULL)
{
	setupUi(this);

	if (configure_dateline_wrapping)
	{
		d_dateline_wrap_options_widget = new DatelineWrapOptionsWidget(
				this,
				d_export_configuration.output_options.wrap_geometries_to_the_dateline);
		QtWidgetUtils::add_widget_to_placeholder(
				d_dateline_wrap_options_widget,
				widget_shapefile_dateline_wrap);
	}

	//
	// Set the state of the export options widget according to the default export configuration
	// passed to us.  
	//

	// polygon 
	checkBox_export_all_plate_polygons_to_single_file->setCheckState(
			d_export_configuration.output_options.export_all_plate_polygons_to_a_single_file
			? Qt::Checked
			: Qt::Unchecked);

	checkBox_export_individual_plate_polygon_files->setCheckState(
			d_export_configuration.output_options.export_individual_plate_polygon_files
			? Qt::Checked
			: Qt::Unchecked);

#if 0
// No longer needed ; redundant export 
	checkBox_export_plate_polygon_subsegments_to_single_file->setCheckState(
			d_export_configuration.output_options.export_plate_polygon_subsegments_to_lines
			? Qt::Checked
			: Qt::Unchecked);
#endif

	checkBox_export_plate_polygon_subsegments_to_type_files->setCheckState(
			(
					d_export_configuration.output_options.export_ridge_transforms &&
					d_export_configuration.output_options.export_subductions &&
					d_export_configuration.output_options.export_left_subductions &&
					d_export_configuration.output_options.export_right_subductions
			)
			? Qt::Checked
			: Qt::Unchecked);



	// network 
	checkBox_export_individual_network_polygon_files->setCheckState(
			d_export_configuration.output_options.export_individual_network_polygon_files
			? Qt::Checked
			: Qt::Unchecked);

	checkBox_export_all_network_polygons_to_single_file->setCheckState(
			d_export_configuration.output_options.export_all_network_polygons_to_a_single_file
			? Qt::Checked
			: Qt::Unchecked);

#if 0
// No longer needed ; redundant export 
	checkBox_export_network_polygon_subsegments_to_single_file->setCheckState(
			d_export_configuration.output_options.export_network_polygon_subsegments_to_lines
			? Qt::Checked
			: Qt::Unchecked);
#endif

	checkBox_export_network_polygon_subsegments_to_type_files->setCheckState(
			(
					d_export_configuration.output_options.export_network_ridge_transforms &&
					d_export_configuration.output_options.export_network_subductions &&
					d_export_configuration.output_options.export_network_left_subductions &&
					d_export_configuration.output_options.export_network_right_subductions
			)
			? Qt::Checked
			: Qt::Unchecked);

	// slab 
	checkBox_export_all_slab_polygons_to_single_file->setCheckState(
			d_export_configuration.output_options.export_all_slab_polygons_to_a_single_file
			? Qt::Checked
			: Qt::Unchecked);

	checkBox_export_individual_slab_polygon_files->setCheckState(
			d_export_configuration.output_options.export_individual_slab_polygon_files
			? Qt::Checked
			: Qt::Unchecked);

#if 0
// No longer needed ; redundant export 
	checkBox_export_slab_polygon_subsegments_to_single_file->setCheckState(
			d_export_configuration.output_options.export_slab_polygon_subsegments_to_lines
			? Qt::Checked
			: Qt::Unchecked);
#endif

	checkBox_export_slab_polygon_subsegments_to_type_files->setCheckState(
			(
					d_export_configuration.output_options.export_slab_edge_leading &&
					d_export_configuration.output_options.export_slab_edge_leading_left &&
					d_export_configuration.output_options.export_slab_edge_leading_right &&
					d_export_configuration.output_options.export_slab_edge_trench &&
					d_export_configuration.output_options.export_slab_edge_side
			)
			? Qt::Checked
			: Qt::Unchecked);

	make_signal_slot_connections();
}


GPlatesGui::ExportAnimationStrategy::const_configuration_base_ptr
GPlatesQtWidgets::ExportCitcomsResolvedTopologyOptionsWidget::create_export_animation_strategy_configuration(
		const QString &filename_template)
{
	// Get the dateline wrapping options if they've been configured to allow the user to edit them.
	if (d_dateline_wrap_options_widget)
	{
		d_export_configuration.output_options.wrap_geometries_to_the_dateline =
				d_dateline_wrap_options_widget->get_wrap_to_dateline();
	}

	d_export_configuration.set_filename_template(filename_template);

	return GPlatesGui::ExportCitcomsResolvedTopologyAnimationStrategy::const_configuration_ptr(
			new GPlatesGui::ExportCitcomsResolvedTopologyAnimationStrategy::Configuration(
					d_export_configuration));
}


void
GPlatesQtWidgets::ExportCitcomsResolvedTopologyOptionsWidget::make_signal_slot_connections()
{
	QObject::connect(
			checkBox_export_all_plate_polygons_to_single_file,
			SIGNAL(stateChanged(int)),
			this,
			SLOT(react_check_box_state_changed(int)));
	QObject::connect(
			checkBox_export_all_slab_polygons_to_single_file,
			SIGNAL(stateChanged(int)),
			this,
			SLOT(react_check_box_state_changed(int)));
	QObject::connect(
			checkBox_export_all_network_polygons_to_single_file,
			SIGNAL(stateChanged(int)),
			this,
			SLOT(react_check_box_state_changed(int)));

	QObject::connect(
			checkBox_export_individual_plate_polygon_files,
			SIGNAL(stateChanged(int)),
			this,
			SLOT(react_check_box_state_changed(int)));
	QObject::connect(
			checkBox_export_individual_slab_polygon_files,
			SIGNAL(stateChanged(int)),
			this,
			SLOT(react_check_box_state_changed(int)));
	QObject::connect(
			checkBox_export_individual_network_polygon_files,
			SIGNAL(stateChanged(int)),
			this,
			SLOT(react_check_box_state_changed(int)));

#if 0
// No longer needed ; redundant export 
	QObject::connect(
			checkBox_export_plate_polygon_subsegments_to_single_file,
			SIGNAL(stateChanged(int)),
			this,
			SLOT(react_check_box_state_changed(int)));

	QObject::connect(
			checkBox_export_slab_polygon_subsegments_to_single_file,
			SIGNAL(stateChanged(int)),
			this,
			SLOT(react_check_box_state_changed(int)));

	QObject::connect(
			checkBox_export_network_polygon_subsegments_to_single_file,
			SIGNAL(stateChanged(int)),
			this,
			SLOT(react_check_box_state_changed(int)));
#endif

	QObject::connect(
			checkBox_export_plate_polygon_subsegments_to_type_files,
			SIGNAL(stateChanged(int)),
			this,
			SLOT(react_check_box_state_changed(int)));
	QObject::connect(
			checkBox_export_slab_polygon_subsegments_to_type_files,
			SIGNAL(stateChanged(int)),
			this,
			SLOT(react_check_box_state_changed(int)));
	QObject::connect(
			checkBox_export_network_polygon_subsegments_to_type_files,
			SIGNAL(stateChanged(int)),
			this,
			SLOT(react_check_box_state_changed(int)));
}


void
GPlatesQtWidgets::ExportCitcomsResolvedTopologyOptionsWidget::react_check_box_state_changed(
		int state)
{
	// all plate polygons to a single file 
	d_export_configuration.output_options.export_all_plate_polygons_to_a_single_file =
			checkBox_export_all_plate_polygons_to_single_file->isChecked();
	// all slab polygons to a single file 
	d_export_configuration.output_options.export_all_slab_polygons_to_a_single_file =
			checkBox_export_all_slab_polygons_to_single_file->isChecked();
	// all network polygons to a single file 
	d_export_configuration.output_options.export_all_network_polygons_to_a_single_file =
			checkBox_export_all_network_polygons_to_single_file->isChecked();

    
	// individual plate polygon files
	d_export_configuration.output_options.export_individual_plate_polygon_files =
			checkBox_export_individual_plate_polygon_files->isChecked();
	// individual slab polygon files
	d_export_configuration.output_options.export_individual_slab_polygon_files =
			checkBox_export_individual_slab_polygon_files->isChecked();
	// individual network polygon files
	d_export_configuration.output_options.export_individual_network_polygon_files =
			checkBox_export_individual_network_polygon_files->isChecked();

#if 0
// No longer needed ; redundant export 
	// all sub segments , from various topology types, to a finle file 
	d_export_configuration.output_options.export_plate_polygon_subsegments_to_lines =
			checkBox_export_plate_polygon_subsegments_to_single_file->isChecked();
	d_export_configuration.output_options.export_slab_polygon_subsegments_to_lines =
			checkBox_export_slab_polygon_subsegments_to_single_file->isChecked();
	d_export_configuration.output_options.export_network_polygon_subsegments_to_lines =
			checkBox_export_network_polygon_subsegments_to_single_file->isChecked();
#endif

	// plate polygon sub segments
	d_export_configuration.output_options.export_ridge_transforms =
			checkBox_export_plate_polygon_subsegments_to_type_files->isChecked();
	d_export_configuration.output_options.export_subductions =
			checkBox_export_plate_polygon_subsegments_to_type_files->isChecked();
	d_export_configuration.output_options.export_left_subductions =
			checkBox_export_plate_polygon_subsegments_to_type_files->isChecked();
	d_export_configuration.output_options.export_right_subductions =
			checkBox_export_plate_polygon_subsegments_to_type_files->isChecked();

	// slab polygon sub segments 
	d_export_configuration.output_options.export_slab_edge_leading =
			checkBox_export_slab_polygon_subsegments_to_type_files->isChecked();
	d_export_configuration.output_options.export_slab_edge_leading_left =
			checkBox_export_slab_polygon_subsegments_to_type_files->isChecked();
	d_export_configuration.output_options.export_slab_edge_leading_right =
			checkBox_export_slab_polygon_subsegments_to_type_files->isChecked();
	d_export_configuration.output_options.export_slab_edge_trench =
			checkBox_export_slab_polygon_subsegments_to_type_files->isChecked();
	d_export_configuration.output_options.export_slab_edge_side =
			checkBox_export_slab_polygon_subsegments_to_type_files->isChecked();

	// network polygon sub segments 
	d_export_configuration.output_options.export_network_ridge_transforms =
			checkBox_export_network_polygon_subsegments_to_type_files->isChecked();
	d_export_configuration.output_options.export_network_subductions =
			checkBox_export_network_polygon_subsegments_to_type_files->isChecked();
	d_export_configuration.output_options.export_network_left_subductions =
			checkBox_export_network_polygon_subsegments_to_type_files->isChecked();
	d_export_configuration.output_options.export_network_right_subductions =
			checkBox_export_network_polygon_subsegments_to_type_files->isChecked();

}
