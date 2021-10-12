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

#include <QFileDialog>
#include <QDebug>
#include <QMessageBox>
#include <sstream>
#include <string>

#include "MeshDialogUi.h"
#include "MeshDialog.h"
#include "ProgressDialog.h"

#include "app-logic/FeatureCollectionFileIO.h"
#include "app-logic/MeshGenerator.h"
#include "app-logic/ApplicationState.h"

#include "feature-visitors/GeometryFinder.h"

#include "gui/FeatureFocus.h"

#include "maths/MultiPointOnSphere.h"

#include "model/FeatureCollectionHandle.h"
#include "model/FeatureHandle.h"
#include "model/FeatureType.h"
#include "model/ModelInterface.h"
#include "model/Model.h"
#include "model/ModelUtils.h"

#include "property-values/GmlMultiPoint.h"
#include "property-values/GpmlPlateId.h"

namespace
{
	const std::string CAP_NUM_PLACE_HOLDER = "%c";
	const std::string DENSITY_PLACE_HOLDER = "%d";
}
const QString GPlatesQtWidgets::MeshDialog::s_help_dialog_title_resolution = QObject::tr(
	"Setting the mesh resolution");

const QString GPlatesQtWidgets::MeshDialog::s_help_dialog_text_resolution = QObject::tr(
	"<html><body>"
	"<p/>"
	"<p>The nodex and nodey parameters specify the number of nodes in each edge of cap diamonds.</p>"
	"<p>These	nodes are used to divide the diamonds into smaller ones evenly.</p>"
	"<p>For the global mesh, the nodex always equals nodey.</p>" 
	"<p>In current release, we only support global mesh. The regional mesh might come in the future.</p>"
	"</body></html>");

const QString GPlatesQtWidgets::MeshDialog::s_help_dialog_title_output = QObject::tr(
	"Setting output directory and file name template");

const QString GPlatesQtWidgets::MeshDialog::s_help_dialog_text_output = QObject::tr(
	"<html><body>"
	"<p/>"
	"<p>12 files will be generated in the specifed output directory.</p>"
	"<p>The file name template can be specified as something like %d.mesh.%c "
	"where the '%d' represents the mesh point density and '%c' represents the cap number.</p>"
	"<p>%d and %c must appear in the template once and only once.</p>"
	"</body></html>\n");

GPlatesQtWidgets::MeshDialog::MeshDialog(
		GPlatesPresentation::ViewState & view_state,
		GPlatesQtWidgets::ManageFeatureCollectionsDialog& manage_feature_collections_dialog,
		QWidget *parent_ )
	:QDialog(
			parent_, 
			Qt::CustomizeWindowHint | 
			Qt::WindowTitleHint | 
			Qt::WindowSystemMenuHint | 
			Qt::MSWindowsFixedSizeDialogHint),
	d_node_x(1),
	d_view_state(
			view_state),
	d_help_dialog_resolution(
			new InformationDialog(
					s_help_dialog_text_resolution, 
					s_help_dialog_title_resolution, 
					this)),
	d_help_dialog_output(
			new InformationDialog(
					s_help_dialog_text_output, 
					s_help_dialog_title_output, 
					this)),
	d_file_name_template(
			DENSITY_PLACE_HOLDER+".mesh."+CAP_NUM_PLACE_HOLDER),
	d_manage_feature_collections_dialog(
			manage_feature_collections_dialog)
{
	setupUi(this);
	
	node_Y->setDisabled(true);
	lineEdit_path->setText(QDir::currentPath());

	lineEdit_file_template->setText(d_file_name_template.c_str());
	
	QObject::connect(button_gen, SIGNAL(clicked()),
		this, SLOT(gen_mesh()));
	QObject::connect(button_path, SIGNAL(clicked()),
		this, SLOT(select_path()));
	QObject::connect(lineEdit_path, SIGNAL(editingFinished()),
		this, SLOT(set_path()));
	QObject::connect(lineEdit_file_template, SIGNAL(editingFinished()),
		this, SLOT(set_file_name_template()));
	QObject::connect(node_X, SIGNAL(valueChanged(int)), 
	    this, SLOT(set_node_x(int)));
	QObject::connect(pushButton_info_output, SIGNAL(clicked()),
		d_help_dialog_output, SLOT(show()));
	QObject::connect(pushButton_info_resolution, SIGNAL(clicked()),
		d_help_dialog_resolution, SLOT(show()));
}

void
GPlatesQtWidgets::MeshDialog::set_path()
{
	QString new_path=lineEdit_path->text();
	
	QFileInfo new_path_info(new_path);

	if (new_path_info.exists() && 
		new_path_info.isDir() && 
		new_path_info.isWritable()) 
	{
		d_path = new_path;
		
		//make sure the path ends with a directory separator
		if(!d_path.endsWith(QDir::separator()))
		{
			d_path = d_path+QDir::separator();
		}
	}
	else
	{
		//if the new path is invalid, we don't allow the path change.
		lineEdit_path->setText(d_path);
	}
	return;
}

void
GPlatesQtWidgets::MeshDialog::select_path()
{
	QString pathname = 
		QFileDialog::getExistingDirectory(
				parentWidget(), 
				tr("Select Path"), 
				lineEdit_path->text());
	
	if(!pathname.isEmpty())
	{
		lineEdit_path->setText(pathname);
		set_path();
	}
	return;
}

void
GPlatesQtWidgets::MeshDialog::set_node_x(
		int val)
{
	if(val<=0)
	{
		d_node_x = 1;
	}
	else
	{
		d_node_x = val-1;
		node_Y->setValue(val);
	}
}

void
GPlatesQtWidgets::MeshDialog::set_file_name_template()
{
	const QString text = lineEdit_file_template->text();
	std::size_t c_pos = text.toStdString().find(CAP_NUM_PLACE_HOLDER);
	std::size_t d_pos = text.toStdString().find(DENSITY_PLACE_HOLDER);
	
		if(text.isEmpty() ||
			c_pos == std::string::npos ||
			d_pos == std::string::npos ||
			text.toStdString().find(CAP_NUM_PLACE_HOLDER, c_pos+1) != std::string::npos  ||
			text.toStdString().find(DENSITY_PLACE_HOLDER, d_pos+1) != std::string::npos )
		{
			QMessageBox::warning(
					this, 
					tr("Invalid template"), 
					tr("The file name template is not valid. "),
					QMessageBox::Ok, QMessageBox::Ok);
			lineEdit_file_template->setText(d_file_name_template.c_str());
			return;
		}
		d_file_name_template = 	text.toStdString();
}

void
GPlatesQtWidgets::MeshDialog::gen_mesh()
{
	button_gen->setDisabled(true);
	button_cancel->setDisabled(true);

	std::vector<GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type> geometries;
	ProgressDialog *progress_dlg = new ProgressDialog(this);
	progress_dlg->setRange(0,12);
	progress_dlg->setValue(0);
	progress_dlg->show();
	
	for(int i=0; i<12; i++)//for global mesh, there are 12 diamonds. Hard coded here.
	{
		std::stringstream stream;
		stream<<tr("generating diamond # ").toStdString()<<i<<" ...";
		progress_dlg->update_progress(i, stream.str().c_str());
		
		geometries.push_back(
				GPlatesAppLogic::MeshGenerator::generate_mesh_geometry(d_node_x, i));
		
		if(progress_dlg->canceled())
		{
			progress_dlg->close();
			button_gen->setDisabled(false);
			button_cancel->setDisabled(false);
			this->close();
			return;
		}
	}
	progress_dlg->disable_cancel_button(true);
	progress_dlg->update_progress(12, tr("Saving feature files..."));

	static const GPlatesModel::FeatureType mesh_node_feature_type = 
		GPlatesModel::FeatureType::create_gpml("MeshNode");

	std::stringstream resolution;
	resolution<<d_node_x+1;
	std::string res_str = resolution.str();

	for(int i=geometries.size()-1; i>=0; i--)
	{
		GPlatesModel::ModelInterface model =
				d_view_state.get_application_state().get_model_interface();

		// Create a feature collection that is not added to the model.
		const GPlatesModel::FeatureCollectionHandle::non_null_ptr_type feature_collection = 
				GPlatesModel::FeatureCollectionHandle::create();

		// Get a weak reference so we can add features to the feature collection.
		const GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection_ref =
				feature_collection->reference();

		GPlatesModel::FeatureHandle::weak_ref feature = GPlatesModel::FeatureHandle::create(
				feature_collection_ref,
				mesh_node_feature_type);

		//create the geometry property and append to feature
		feature->add(
				GPlatesModel::TopLevelPropertyInline::create(
					GPlatesModel::PropertyName::create_gpml("meshPoints"),
					GPlatesPropertyValues::GmlMultiPoint::create(geometries[i])));


		//add plateid and validtime to mesh points feature
		//these two properties are needed to show mesh points on globe.
		GPlatesPropertyValues::GpmlPlateId::non_null_ptr_type gpml_plate_id = 
			GPlatesPropertyValues::GpmlPlateId::create(0);
		feature->add(
				GPlatesModel::TopLevelPropertyInline::create(
					GPlatesModel::PropertyName::create_gpml("reconstructionPlateId"),
					GPlatesModel::ModelUtils::create_gpml_constant_value(
						gpml_plate_id,
						GPlatesPropertyValues::TemplateTypeParameterType::create_gpml("plateId"))));


		GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type gml_valid_time = 
			GPlatesModel::ModelUtils::create_gml_time_period(
			GPlatesPropertyValues::GeoTimeInstant::create_distant_past(),
			GPlatesPropertyValues::GeoTimeInstant::create_distant_future());
		feature->add(
				GPlatesModel::TopLevelPropertyInline::create(
					GPlatesModel::PropertyName::create_gml("validTime"),
					gml_valid_time));

		// Make a new FileInfo object to tell save_file() what the new name should be.
		// This also copies any other info stored in the FileInfo.
		std::stringstream cap_number_stream;
		cap_number_stream<<i;
		std::string cap_num = cap_number_stream.str();
		std::string file_name = d_file_name_template;	
		
		file_name.append(".gpml");
		file_name.replace(file_name.find(DENSITY_PLACE_HOLDER), 2, res_str);
		file_name.replace(file_name.find(CAP_NUM_PLACE_HOLDER), 2, cap_num);
		file_name=d_path.toStdString()+file_name;

		GPlatesFileIO::FileInfo new_fileinfo(file_name.c_str());

		// Save the feature collection to a file that is registered with
		// FeatureCollectionFileState (maintains list of all loaded files).
		d_view_state.get_application_state().get_feature_collection_file_io().create_file(
				new_fileinfo, feature_collection);

	}

	button_gen->setDisabled(false);
	button_cancel->setDisabled(false);
	progress_dlg->reject();
	reject();
}

