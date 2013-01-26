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
#ifndef GENERATE_VELOCITY_DOMAIN_TERRA_DIALOG_H
#define GENERATE_VELOCITY_DOMAIN_TERRA_DIALOG_H

#include <QObject>
#include <QSpinBox>

#include "GenerateVelocityDomainTerraDialogUi.h"

#include "GPlatesDialog.h"
#include "InformationDialog.h"
#include "OpenDirectoryDialog.h"

#include "maths/MultiPointOnSphere.h"


namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{
	class GenerateVelocityDomainTerraDialog: 
			public GPlatesDialog,
			protected Ui_GenerateVelocityDomainTerraDialog 
	{
		Q_OBJECT

	public:

		GenerateVelocityDomainTerraDialog(
				GPlatesPresentation::ViewState &view_state_,
				QWidget *parent_ = NULL);
		
	private Q_SLOTS:

		void 
		generate_velocity_domain();

		void
		handle_mt_value_changed(
				int mt);

		void
		handle_nt_value_changed(
				int nt);

		void
		handle_nd_value_changed(
				int nd);
		
		void
		set_path();
		
		void
		select_path();
		
		void
		set_file_name_template();
		
	private:

		GPlatesPresentation::ViewState &d_view_state;

		int d_mt; //!< Terra 'mt' parameter.
		int d_nt; //!< Terra 'nt' parameter.
		int d_nd; //!< Terra 'nd' parameter.
		int d_num_processors;

		QString d_path;
		std::string d_file_name_template;

		QSpinBox *d_mt_spinbox; //!< Spinbox for Terra 'mt' parameter.
		QSpinBox *d_nt_spinbox; //!< Spinbox for Terra 'nt' parameter.

		InformationDialog *d_help_dialog_configuration;
		InformationDialog *d_help_dialog_output;

		OpenDirectoryDialog d_open_directory_dialog;


		void
		set_num_processors();

		void
		save_velocity_domain_file(
				const GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type &velocity_sub_domain,
				int processor_number);

	};
}

#endif


