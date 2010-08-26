/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#include <map>
#include <QCheckBox>
#include <QLabel>
#include <QFileDialog>
#include <QDir>
#include <QFileInfo>
#include <QColorGroup>
#include "ConfigureExportParametersDialog.h"
#include "ExportAnimationDialog.h"

#include "gui/ExportResolvedTopologyAnimationStrategy.h"
#include "gui/ExportVelocityAnimationStrategy.h"
#include "gui/ExportReconstructedGeometryAnimationStrategy.h"
#include "gui/ExportResolvedTopologyAnimationStrategy.h"
#include "gui/ExportSvgAnimationStrategy.h"
#include "gui/ExportRasterAnimationStrategy.h"

#include "utils/ExportAnimationStrategyFactory.h"

std::map<GPlatesQtWidgets::ConfigureExportParametersDialog::ExportItemName, QString> 
		GPlatesQtWidgets::ConfigureExportParametersDialog::d_name_map;

std::map<GPlatesQtWidgets::ConfigureExportParametersDialog::ExportItemType, QString> 
		GPlatesQtWidgets::ConfigureExportParametersDialog::d_type_map;

std::map<GPlatesQtWidgets::ConfigureExportParametersDialog::ExportItemName, QString> 
		GPlatesQtWidgets::ConfigureExportParametersDialog::d_desc_map;

bool GPlatesQtWidgets::ConfigureExportParametersDialog::dummy = 
		GPlatesQtWidgets::ConfigureExportParametersDialog::initialize_item_name_and_type_map();

GPlatesQtWidgets::ConfigureExportParametersDialog::ConfigureExportParametersDialog(
		GPlatesGui::ExportAnimationContext::non_null_ptr_type export_animation_context_ptr,
		QWidget *parent_):
	QDialog(
			parent_, 
			Qt::CustomizeWindowHint | 
			Qt::WindowTitleHint | 
			Qt::WindowSystemMenuHint),
	d_export_animation_context_ptr(
			export_animation_context_ptr),
	d_is_single_frame(false)
{
	setupUi(this);
	initialize_export_item_map();
	initialize_export_item_list_widget();
	initialize_item_desc_map();

	button_add_item->setEnabled(false);
	
	QObject::connect(button_add_item, SIGNAL(clicked()),
		this, SLOT(react_add_item_clicked()));
	QObject::connect(listWidget_export_items, SIGNAL(itemSelectionChanged()),
		this, SLOT(react_export_items_selection_changed()));
	QObject::connect(listWidget_export_items, SIGNAL(itemClicked(QListWidgetItem *)),
		this, SLOT(react_export_items_selection_changed()));
	QObject::connect(listWidget_format, SIGNAL(itemSelectionChanged()),
		this, SLOT(react_format_selection_changed()));
	QObject::connect(lineEdit_filename, SIGNAL(cursorPositionChanged(int, int)),
		this, SLOT(react_filename_template_changing()));
	QObject::connect(lineEdit_filename, SIGNAL(editingFinished()),
		this, SLOT(react_filename_template_changed()));
}

bool 
GPlatesQtWidgets::ConfigureExportParametersDialog::initialize_item_name_and_type_map()
{
	//TODO: these maps should be integrated into exporter classes.
	d_name_map[RECONSTRUCTED_GEOMETRIES]=QObject::tr("Reconstructed Geometries");
	d_name_map[PROJECTED_GEOMETRIES]    =QObject::tr("Projected Geometries");
	d_name_map[MESH_VELOCITIES]         =QObject::tr("Colat/lon Mesh Velocities");
	d_name_map[RESOLVED_TOPOLOGIES]     =QObject::tr("Resolved Topologies");
	d_name_map[RELATIVE_ROTATION]       =QObject::tr("Relative Total Rotation");
	d_name_map[EQUIVALENT_ROTATION]     =QObject::tr("Equivalent Total Rotation");
	d_name_map[ROTATION_PARAMS]			=QObject::tr("Equivalent Stage Rotation");
	d_name_map[RASTER]				    =QObject::tr("Raster");

	d_type_map[GMT]             =QObject::tr("GMT (*.xy)");
	d_type_map[GPML]			=QObject::tr("GPML (*.gpml)");
	d_type_map[SHAPEFILE]		=QObject::tr("Shapefiles (*.shp)");
	d_type_map[SVG]             =QObject::tr("SVG (*.svg)");
	d_type_map[CSV_COMMA]       =QObject::tr("CSV file (comma delimited) (*.csv)");
	d_type_map[CSV_SEMICOLON]   =QObject::tr("CSV file (semicolon delimited) (*.csv)");
	d_type_map[CSV_TAB]         =QObject::tr("CSV file (tab delimited) (*.csv)");
	d_type_map[BMP]				=QObject::tr("Windows Bitmap (*.bmp)");
	d_type_map[JPG]				=QObject::tr("Joint Photographic Experts Group (*.jpg)");
	d_type_map[JPEG]			=QObject::tr("Joint Photographic Experts Group (*.jpeg)");
	d_type_map[PNG]				=QObject::tr("Portable Network Graphics (*.png)");
	d_type_map[PPM]				=QObject::tr("Portable Pixmap (*.ppm)");
	d_type_map[TIFF]			=QObject::tr("Tagged Image File Format (*.tiff)");
	d_type_map[XBM]				=QObject::tr("X11 Bitmap (*.xbm)");
	d_type_map[XPM]				=QObject::tr("X11 Pixmap (*.xpm)");

	return true;
}

void 
GPlatesQtWidgets::ConfigureExportParametersDialog::initialize_item_desc_map()
{
	//TODO: this map should be integrated into exporter classes.
	d_desc_map[RECONSTRUCTED_GEOMETRIES] = 
		GPlatesGui::ExportReconstructedGeometryAnimationStrategy::RECONSTRUCTED_GEOMETRIES_DESC;
	d_desc_map[PROJECTED_GEOMETRIES]     =
		GPlatesGui::ExportSvgAnimationStrategy::PROJECTED_GEOMETRIES_DESC;
	d_desc_map[MESH_VELOCITIES]          =
		GPlatesGui::ExportVelocityAnimationStrategy::MESH_VELOCITIES_DESC;
	d_desc_map[RESOLVED_TOPOLOGIES]      = 
		GPlatesGui::ExportResolvedTopologyAnimationStrategy::RESOLOVED_TOPOLOGIES_DESC;
	d_desc_map[RELATIVE_ROTATION]        = 
		GPlatesGui::ExportRotationAnimationStrategy::RELATIVE_ROTATION_DESC;
	d_desc_map[EQUIVALENT_ROTATION]      = 
		GPlatesGui::ExportRotationAnimationStrategy::EQUIVALENT_ROTATION_DESC;
	d_desc_map[ROTATION_PARAMS]          = 
		GPlatesGui::ExportRotationParamsAnimationStrategy::ROTATION_PARAMS_DESC;
}

void
GPlatesQtWidgets::ConfigureExportParametersDialog::initialize_export_item_list_widget()
{
	listWidget_export_items->clear();
	listWidget_format->clear();
	export_item_map_type::const_iterator it;
	for (it=d_export_item_map.begin(); it != d_export_item_map.end(); it++ )
	{
		if(all_types_has_been_added((*it).second))
		{
			//if all types has been added to export items table,
			//we don't add this item to export_item_list
			continue;
		}
		QListWidgetItem *item = new ExportItem((*it).first);
		listWidget_export_items->addItem(item);
		item->setText(d_name_map[(*it).first]);
	}
}

#define REGISTER_EXPORT_ITEM(ITEM_NAME,ITEM_TYPE) \
	d_export_item_map[ITEM_NAME][ITEM_TYPE].class_id=GPlatesUtils::ITEM_NAME##_##ITEM_TYPE; \
	d_export_item_map[ITEM_NAME][ITEM_TYPE].has_been_added = false;

void
GPlatesQtWidgets::ConfigureExportParametersDialog::initialize_export_item_map()
{
	d_export_item_map.clear();

	//RECONSTRUCTED_GEOMETRY_GMT
	REGISTER_EXPORT_ITEM(RECONSTRUCTED_GEOMETRIES,GMT);
	//RECONSTRUCTED_GEOMETRY_SHAPEFILE
	REGISTER_EXPORT_ITEM(RECONSTRUCTED_GEOMETRIES,SHAPEFILE);
	//PROJECTED_GEOMETRIES_SVG
	REGISTER_EXPORT_ITEM(PROJECTED_GEOMETRIES,SVG);
	//MESH_VELOCITIES_GPML
	REGISTER_EXPORT_ITEM(MESH_VELOCITIES,GPML);
	//RESOLVED_TOPOLOGIES_GMT
	REGISTER_EXPORT_ITEM(RESOLVED_TOPOLOGIES,GMT);
	//RELATIVE_ROTATION_CSV_COMMA
	REGISTER_EXPORT_ITEM(RELATIVE_ROTATION,CSV_COMMA);
	//RELATIVE_ROTATION_CSV_SEMICOLON
	REGISTER_EXPORT_ITEM(RELATIVE_ROTATION,CSV_SEMICOLON);
	//RELATIVE_ROTATION_CSV_TAB
	REGISTER_EXPORT_ITEM(RELATIVE_ROTATION,CSV_TAB);
	//EQUIVALENT_ROTATION_CSV_COMMA
	REGISTER_EXPORT_ITEM(EQUIVALENT_ROTATION,CSV_COMMA);
	//EQUIVALENT_ROTATION_CSV_SEMICOLON
	REGISTER_EXPORT_ITEM(EQUIVALENT_ROTATION,CSV_SEMICOLON);
	//EQUIVALENT_ROTATION_CSV_TAB
	REGISTER_EXPORT_ITEM(EQUIVALENT_ROTATION,CSV_TAB);
	//ROTATION_PARAMS_CSV_COMMA
	REGISTER_EXPORT_ITEM(ROTATION_PARAMS,CSV_COMMA);
	//ROTATION_PARAMS_CSV_SEMICOLON
	REGISTER_EXPORT_ITEM(ROTATION_PARAMS,CSV_SEMICOLON);
	//ROTATION_PARAMS_CSV_TAB
	REGISTER_EXPORT_ITEM(ROTATION_PARAMS,CSV_TAB);
	//RASTER_BMP
	REGISTER_EXPORT_ITEM(RASTER,BMP);
	//RASTER_JPG
	REGISTER_EXPORT_ITEM(RASTER,JPG);
	//RASTER_JPEG
	REGISTER_EXPORT_ITEM(RASTER,JPEG);
	//RASTER_PNG
	REGISTER_EXPORT_ITEM(RASTER,PNG);
	//RASTER_PPM
	REGISTER_EXPORT_ITEM(RASTER,PPM);
	//RASTER_TIFF
	REGISTER_EXPORT_ITEM(RASTER,TIFF);
	//RASTER_XBM
	REGISTER_EXPORT_ITEM(RASTER,XBM);
	//RASTER_XPM
	REGISTER_EXPORT_ITEM(RASTER,XPM);
}

void
GPlatesQtWidgets::ConfigureExportParametersDialog::react_format_selection_changed()
{

	if(!listWidget_export_items->currentItem() || !listWidget_format->currentItem())
	{
		return;
	}
	ExportItemName selected_item = get_export_item_name(listWidget_export_items->currentItem());
	ExportItemType selected_type = get_export_item_type(listWidget_format->currentItem());
	
	if(selected_type == INVALID_TYPE||selected_item == INVALID_NAME)
	{	
		qWarning()<<"invalid export type or item!";
		return;
	}
	//initialize_export_item_map();
	if((d_export_item_map[selected_item]).find(selected_type)==
		(d_export_item_map[selected_item]).end())
	{
		qWarning()<<"format widget already invalid!";
		listWidget_format->clear();
		return;		
	}

	QString filename_template = 
		GPlatesUtils::ExportAnimationStrategyFactory::create_exporter(
				d_export_item_map[selected_item][selected_type].class_id,
				*d_export_animation_context_ptr)->get_default_filename_template();
	
	button_add_item->setEnabled(true);

	lineEdit_filename->setText(
			filename_template.toStdString().substr(
					0, filename_template.toStdString().find_last_of(".")).c_str());
	
	label_file_extension->setText(
			filename_template.toStdString().substr(
					filename_template.toStdString().find_last_of(".")).c_str());

	label_filename_desc->setText(
		GPlatesUtils::ExportAnimationStrategyFactory::create_exporter(
		d_export_item_map[selected_item][selected_type].class_id,
		*d_export_animation_context_ptr)->get_filename_template_desc());
	QPalette pal=label_filename_desc->palette();
	pal.setColor(QPalette::WindowText, QColor("black")); 
	label_filename_desc->setPalette(pal);
	
}

void
GPlatesQtWidgets::ConfigureExportParametersDialog::react_export_items_selection_changed()
{
	if(!listWidget_export_items->currentItem())
		return;
	button_add_item->setEnabled(false);
	lineEdit_filename->clear();
	label_file_extension->clear();
	listWidget_format->clear();
	
	ExportItemName selected_item = get_export_item_name(
			listWidget_export_items->currentItem());
		
	export_type_map_type::const_iterator type_it;	
	export_item_map_type::const_iterator item_it;


	//iterate through the map to add available export items.
	if((item_it=d_export_item_map.find(selected_item)) == d_export_item_map.end())
	{
		return;
	}
	else
	{
		type_it=(*item_it).second.begin();
		for (; type_it != (*item_it).second.end(); type_it++ )
		{
			if((*type_it).second.has_been_added)
				continue;
			else
			{
				QListWidgetItem *item = new ExportTypeItem((*type_it).first);
				listWidget_format->addItem(item);
				item->setText(d_type_map[(*type_it).first]);
			}
		}
		label_export_description->setText(
				d_desc_map[selected_item]);
	}
}

bool
GPlatesQtWidgets::ConfigureExportParametersDialog::all_types_has_been_added(
		export_type_map_type type_map)
{
	export_type_map_type::iterator type_it;	
	bool flag=false;

	for (type_it=type_map.begin(); type_it != type_map.end(); type_it++ )
	{
		if(!(*type_it).second.has_been_added)
		{
			flag=false;
			break;
		}
		else
		{
			flag=true;
			continue;
		}
	}

	return flag;
}

void
GPlatesQtWidgets::ConfigureExportParametersDialog::react_add_item_clicked()
{
	if(!listWidget_export_items->currentItem() || !listWidget_format->currentItem())
		return;

	ExportItemName selected_item = get_export_item_name(
			listWidget_export_items->currentItem());
	ExportItemType selected_type = get_export_item_type(
			listWidget_format->currentItem());
	
	QString filename_template = lineEdit_filename->text()+label_file_extension->text();		
	
	boost::shared_ptr<GPlatesUtils::ExportFileNameTemplateValidator> validator=
		GPlatesUtils::ExportFileNameTemplateValidatorFactory::create_validator(
				d_export_item_map[selected_item][selected_type].class_id);
	
	if(!validator->is_valid(filename_template))	
	{
		label_filename_desc->setText(
				validator->get_result_report().message());
		QPalette pal=label_filename_desc->palette();
		pal.setColor(QPalette::WindowText, QColor("red")); 
		label_filename_desc->setPalette(pal);
		button_add_item->setEnabled(false);
		return;
	}
	
	d_export_item_map[selected_item][selected_type].has_been_added=true;

	delete listWidget_format->takeItem(listWidget_format->currentRow());
	
	if(listWidget_format->count()==0)
	{
		delete listWidget_export_items->takeItem(listWidget_export_items->currentRow());
	}

	d_export_animation_context_ptr->get_export_dialog()->insert_item(
			selected_item,
			selected_type,
			filename_template);
}

void
GPlatesQtWidgets::ConfigureExportParametersDialog::init(
		ExportAnimationDialog* dialog, 
		QTableWidget* table)
{
	d_export_item_map.clear();
	
	initialize_export_item_map();
	for(int i=0; i<table->rowCount();i++)
	{
		d_export_item_map
			[dialog->get_export_item_name(table->item(i,0))]
			[dialog->get_export_item_type(table->item(i,1))]
			.has_been_added=true;
	}
	initialize_export_item_list_widget();
	lineEdit_filename->clear();
	label_file_extension->clear();
	label_export_description->clear();
}

void
GPlatesQtWidgets::ConfigureExportParametersDialog::react_filename_template_changed()
{
#if 0
	//comments out these code
	//the validation of file name template will be done when "add" button is clicked.
	ExportItemName selected_item = get_export_item_name(
			listWidget_export_items->currentItem());
	ExportItemType selected_type = get_export_item_type(
			listWidget_format->currentItem());

	if(selected_type==INVALID_TYPE || selected_item==INVALID_NAME)
	{
		return;
	}

	QString filename_template = lineEdit_filename->text()+label_file_extension->text();
	
	boost::shared_ptr<GPlatesUtils::ExportFileNameTemplateValidator> validator=
		GPlatesUtils::ExportFileNameTemplateValidatorFactory::create_validator(
				d_export_item_map[selected_item][selected_type].class_id);
	if(validator->is_valid(filename_template))	
	{
		//d_filename_template=filename_template;
		button_add_item->setEnabled(true);
	}
	else
	{
		label_filename_desc->setText(
				validator->get_result_report().message());
		QPalette pal=label_filename_desc->palette();
		pal.setColor(QPalette::WindowText, QColor("red")); 
		label_filename_desc->setPalette(pal);
		button_add_item->setEnabled(false);
	}
#endif	
}

void
GPlatesQtWidgets::ConfigureExportParametersDialog::react_filename_template_changing()
{
	if(!listWidget_export_items->currentItem() || !listWidget_format->currentItem())
		return;
	ExportItemName selected_item = get_export_item_name(
			listWidget_export_items->currentItem());
	ExportItemType selected_type = get_export_item_type(
			listWidget_format->currentItem());

	if(selected_type==INVALID_TYPE || selected_item==INVALID_NAME)
		return;
	
	if((d_export_item_map[selected_item]).find(selected_type)==
		(d_export_item_map[selected_item]).end())
	{
		qWarning()<<"invalid selected items!";
		return;		
	}

	label_filename_desc->setText(
			GPlatesUtils::ExportAnimationStrategyFactory::create_exporter(
					d_export_item_map[selected_item][selected_type].class_id,
					*d_export_animation_context_ptr)->get_filename_template_desc());
	QPalette pal=label_filename_desc->palette();
	pal.setColor(QPalette::WindowText, QColor("black")); 
	label_filename_desc->setPalette(pal);
	button_add_item->setEnabled(true);
}





