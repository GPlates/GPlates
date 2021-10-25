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
#ifndef GENERATE_VELOCITY_DOMAIN_LATLON_DIALOG_H
#define GENERATE_VELOCITY_DOMAIN_LATLON_DIALOG_H

#include <iostream>
#include <QObject>
#include <QDialog>

#include "GenerateVelocityDomainLatLonDialogUi.h"

#include "GPlatesDialog.h"
#include "InformationDialog.h"
#include "OpenDirectoryDialog.h"

#include "maths/MultiPointOnSphere.h"


namespace GPlatesQtWidgets
{
	class ViewportWindow;

	class GenerateVelocityDomainLatLonDialog: 
			public GPlatesDialog,
			protected Ui_GenerateVelocityDomainLatLonDialog
	{
		Q_OBJECT

	public:

		GenerateVelocityDomainLatLonDialog(
				ViewportWindow &main_window_,
				QWidget *parent_ = NULL);
		
	private Q_SLOTS:

		void 
		generate_velocity_domain();

		void
		react_top_extents_spin_box_value_changed(
				double value);
		void
		react_bottom_extents_spin_box_value_changed(
				double value);
		void
		react_left_extents_spin_box_value_changed(
				double value);
		void
		react_right_extents_spin_box_value_changed(
				double value);

		void
		handle_use_global_extents_button_clicked();

		void
		handle_num_latitude_grid_intervals_value_changed(
				int num_latitude_grid_intervals);

		void
		handle_num_longitude_grid_intervals_value_changed(
				int num_longitude_grid_intervals);

		void
		react_cell_centred_check_box_changed();

		void
		set_path();
		
		void
		select_path();
		
		void
		set_file_name_template();
		
	private:

		ViewportWindow &d_main_window;

		unsigned int d_num_latitude_grid_intervals;
		unsigned int d_num_longitude_grid_intervals;
		double d_extents_top;
		double d_extents_bottom;
		double d_extents_left;
		double d_extents_right;
		bool d_cell_centred_nodes;

		QString d_path;
		std::string d_file_name_template;

		InformationDialog *d_help_dialog_configuration;
		InformationDialog *d_help_dialog_output;

		OpenDirectoryDialog d_open_directory_dialog;


		unsigned int
		get_num_latitude_nodes() const;

		unsigned int
		get_num_longitude_nodes() const;

		void
		display_num_nodes();

		GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type
		generate_lat_lon_domain();

		bool
		save_velocity_domain_file(
				const GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type &velocity_sub_domain);

	};
}

#endif


