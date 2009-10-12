/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008 Geological Survey of Norway
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
#include <QHeaderView>
#include <QTableWidget>
#include <QMap>
#include <QMessageBox>
#include <QDebug>

#include "TotalReconstructionPolesDialog.h"

#include "app-logic/Reconstruct.h"
#include "gui/CsvExport.h"
#include "model/ReconstructionTreeEdge.h"
#include "presentation/ViewState.h"

#define NUM_ELEMS(a) (sizeof(a) / sizeof((a)[0]))


namespace ColumnNames
{
	/**
	 * These should match the columns set up in the designer.
	 */
	enum ColumnName
	{
		PLATEID, LATITUDE, LONGITUDE, ANGLE, FIXED
	};
}

namespace {

	/**
	 * Struct to build the following table of file dialog filters / options.
	 * Typedef for the resulting QMap.
	 */
	struct FileDialogFilterOption
	{
		const char *text;
		const GPlatesGui::CsvExport::ExportOptions options;
	};
	typedef QMap<QString, GPlatesGui::CsvExport::ExportOptions> FileDialogFilterMapType;
	
	/**
	 * Table of filter options to present to the user when exporting CSV.
	 */
	static FileDialogFilterOption file_dialog_filter_table[] = {
		{ "CSV file, comma-delimited (*.csv)", { ',' } },
		{ "CSV file, semicolon-delimited (*.csv)", { ';' } },
		{ "CSV file, tab-delimited (*.csv)", { '\t' } },
	};
	
	/**
	 * This map is built for a quick, easy way to get back the CSV options
	 * based on what filter the QFileDialog says was selected.
	 */
	const FileDialogFilterMapType &
	build_export_filter_map()
	{
		static FileDialogFilterMapType map;
		FileDialogFilterOption *begin = file_dialog_filter_table;
		FileDialogFilterOption *end = begin + NUM_ELEMS(file_dialog_filter_table);
		for (; begin != end; ++begin) {
			map[QObject::tr(begin->text)] = begin->options;
		}
		return map;
	}
	

	/**
	 * Anon namespace function to create and set up a QFileDialog
	 * for getting names of CSV files to export. Called once
	 * from @a handle_export() and assigned to a static member.
	 * Memory will be managed by Qt if you supply a valid @a parent.
	 */
	QFileDialog *
	create_export_file_dialog(
			QWidget *parent)
	{
		QFileDialog *dialog = new QFileDialog(parent, QObject::tr("Export tablular data"));
		
		// Use "Save As"-like behaviour.
		dialog->setFileMode(QFileDialog::AnyFile);
		dialog->setAcceptMode(QFileDialog::AcceptSave);
		
		// Set up different filters for different CSV options.
		QStringList filters;
		FileDialogFilterOption *begin = file_dialog_filter_table;
		FileDialogFilterOption *end = begin + NUM_ELEMS(file_dialog_filter_table);
		for (; begin != end; ++begin) {
			filters << QObject::tr(begin->text);
		}
		dialog->setFilters(filters);
		dialog->setDefaultSuffix("csv");
		
		return dialog;
	}




	QString
	make_string_from_rotation(
		GPlatesMaths::FiniteRotation &rotation)
	{
		QString result;

		const GPlatesMaths::UnitQuaternion3D &uq = rotation.unit_quat();
		const boost::optional<GPlatesMaths::UnitVector3D> &axis_hint = rotation.axis_hint();

		if (GPlatesMaths::represents_identity_rotation(uq)) {
			result.append(QObject::tr("-- indeterminate pole --\t  angle: 0.00"));
		} else {

			using namespace GPlatesMaths;
			UnitQuaternion3D::RotationParams params = uq.get_rotation_params(axis_hint);

			PointOnSphere euler_pole(params.axis);
			LatLonPoint llp = make_lat_lon_point(euler_pole);

			QLocale locale_;
			
			QString lat_string = locale_.toString(llp.latitude(),'f',2);
			QString lon_string = locale_.toString(llp.longitude(),'f',2);

			const double &angle = radiansToDegrees(params.angle).dval();
			
			QString angle_string = locale_.toString(angle,'f',2);
	
			result.append(QObject::tr("lat: "));
			result.append(lat_string);
			result.append(QObject::tr("\tlon: "));
			result.append(lon_string);
			result.append(QObject::tr("\t  angle: "));
			result.append(angle_string);
		}
		return result;
	}

	void
	fill_tree_item(
		QTreeWidgetItem* item,
		GPlatesModel::ReconstructionTreeEdge::non_null_ptr_type edge)
	{

		int column = item->columnCount();
		QString moving_string;
		moving_string.setNum(edge->moving_plate());
		item->setText(column++,moving_string);

		QString fixed_string;
		fixed_string.setNum(edge->fixed_plate());

		QString edge_string;

		edge_string.append(fixed_string);

		GPlatesMaths::FiniteRotation relative_rotation = edge->relative_rotation();
		QString relative_rotation_string = make_string_from_rotation(relative_rotation);

		GPlatesMaths::FiniteRotation composed_rotation = edge->composed_absolute_rotation();
		QString composed_rotation_string = make_string_from_rotation(composed_rotation);

		item->setText(column++,edge_string);
		item->setText(column++,relative_rotation_string);
		item->setText(column,composed_rotation_string);
	}


	void
	add_children_of_edge_to_tree_item(
		GPlatesModel::ReconstructionTreeEdge::non_null_ptr_type edge,
		QTreeWidgetItem *item)
	{
		std::vector<GPlatesModel::ReconstructionTreeEdge::non_null_ptr_type>::iterator it;
		std::vector<GPlatesModel::ReconstructionTreeEdge::non_null_ptr_type>::iterator 
			it_begin = edge->children_in_built_tree().begin();
		std::vector<GPlatesModel::ReconstructionTreeEdge::non_null_ptr_type>::iterator 
			it_end = edge->children_in_built_tree().end();

		for(it = it_begin; it != it_end ; it++)
		{
			QTreeWidgetItem* child_item = new QTreeWidgetItem(item,0);
			fill_tree_item(child_item,*it);
			add_children_of_edge_to_tree_item(*it,child_item);
		}
	}

} // anonymous namespace

GPlatesQtWidgets::TotalReconstructionPolesDialog::TotalReconstructionPolesDialog(
		GPlatesPresentation::ViewState &view_state,
		QWidget *parent_):
	QDialog(parent_),
	d_reconstruct_ptr(&view_state.get_reconstruct())
{
	setupUi(this);


	QHeaderView *equivalent_header = table_equivalent->horizontalHeader();

	equivalent_header->setResizeMode(ColumnNames::PLATEID, QHeaderView::Fixed);
	equivalent_header->setResizeMode(ColumnNames::LONGITUDE, QHeaderView::Fixed);
	equivalent_header->setResizeMode(ColumnNames::LATITUDE, QHeaderView::Fixed);
	equivalent_header->setResizeMode(ColumnNames::ANGLE, QHeaderView::Fixed);

	QHeaderView *equivalent_vertical = table_equivalent->verticalHeader();
	equivalent_vertical->hide();

	QHeaderView *relative_header = table_relative->horizontalHeader();

	relative_header->setResizeMode(ColumnNames::PLATEID, QHeaderView::Fixed);
	relative_header->setResizeMode(ColumnNames::LONGITUDE, QHeaderView::Fixed);
	relative_header->setResizeMode(ColumnNames::LATITUDE, QHeaderView::Fixed);
	relative_header->setResizeMode(ColumnNames::ANGLE, QHeaderView::Fixed);
	relative_header->setResizeMode(ColumnNames::FIXED, QHeaderView::Fixed);

	QHeaderView *relative_vertical = table_relative->verticalHeader();
	relative_vertical->hide();

	QHeaderView *tree_reconstruction_header = tree_reconstruction->header();
	tree_reconstruction_header->setResizeMode(0,QHeaderView::ResizeToContents);
	tree_reconstruction_header->setResizeMode(1,QHeaderView::Fixed);
	tree_reconstruction_header->setResizeMode(2,QHeaderView::Fixed);
	tree_reconstruction_header->setResizeMode(3,QHeaderView::Fixed);
	tree_reconstruction_header->setMovable(false);

	tree_reconstruction_header->resizeSection(1,100);
	tree_reconstruction_header->resizeSection(2,270);
	tree_reconstruction_header->resizeSection(3,270);


	QHeaderView *tree_circuit_header = tree_circuit->header();
	tree_circuit_header->setResizeMode(0,QHeaderView::ResizeToContents);
	tree_circuit_header->setResizeMode(1,QHeaderView::Fixed);
	tree_circuit_header->setResizeMode(2,QHeaderView::Fixed);
	tree_circuit_header->setResizeMode(3,QHeaderView::Fixed);
	tree_circuit_header->setMovable(false);

	tree_circuit_header->resizeSection(1,100);
	tree_circuit_header->resizeSection(2,270);
	tree_circuit_header->resizeSection(3,270);

	set_time(d_reconstruct_ptr->get_current_reconstruction_time());
	set_plate(d_reconstruct_ptr->get_current_anchored_plate_id());

	QObject::connect(button_export_relative_rotations,SIGNAL(clicked()),this,SLOT(export_relative()));
	QObject::connect(button_export_equiv_rotations,SIGNAL(clicked()),this,SLOT(export_equivalent()));
}

void
GPlatesQtWidgets::TotalReconstructionPolesDialog::set_plate(
	unsigned long plate)
{
	d_plate = plate;
	QString s;
	s.setNum(plate);
	field_reference_plate->setText(s);
}

void
GPlatesQtWidgets::TotalReconstructionPolesDialog::set_time(
	double time)
{
	d_time = time;
	QString s;
	s.setNum(time);
	field_time->setText(s);
}


/**
 * Fill the QTableWidget in tab1 with a list of Plate-ids and their corresponding 
 *	composite Euler poles.
 */
void
GPlatesQtWidgets::TotalReconstructionPolesDialog::fill_equivalent_table()
{
	table_equivalent->clearContents();
	table_equivalent->setRowCount(0);

	std::multimap<GPlatesModel::integer_plate_id_type,
		GPlatesModel::ReconstructionTreeEdge::non_null_ptr_type>::const_iterator it;
	std::multimap<GPlatesModel::integer_plate_id_type,
		GPlatesModel::ReconstructionTreeEdge::non_null_ptr_type>::const_iterator it_begin = 
			d_reconstruct_ptr->get_current_reconstruction().reconstruction_tree().edge_map_begin();
	std::multimap<GPlatesModel::integer_plate_id_type,
		GPlatesModel::ReconstructionTreeEdge::non_null_ptr_type>::const_iterator it_end = 
			d_reconstruct_ptr->get_current_reconstruction().reconstruction_tree().edge_map_end();


	for(it = it_begin; it != it_end ; ++it)
	{
		int num_row = table_equivalent->rowCount();
		table_equivalent->insertRow(num_row);

	// insert the plate id into the first column of the table
		QString id_as_string;

		id_as_string.setNum(it->first);
		QTableWidgetItem* id_item = new QTableWidgetItem(id_as_string);
		id_item->setFlags(Qt::ItemIsEnabled);

		table_equivalent->setItem(num_row,ColumnNames::PLATEID,id_item);

		GPlatesMaths::FiniteRotation fr = it->second->composed_absolute_rotation();
		const GPlatesMaths::UnitQuaternion3D &uq = fr.unit_quat();
		if (GPlatesMaths::represents_identity_rotation(uq)) {
			QString indeterminate_string(QObject::tr("Indeterminate"));
			QTableWidgetItem*  latitude_item = new QTableWidgetItem(indeterminate_string);
			latitude_item->setFlags(Qt::ItemIsEnabled);
			table_equivalent->setItem(num_row,ColumnNames::LATITUDE,latitude_item);

			QTableWidgetItem*  longitude_item = new QTableWidgetItem(indeterminate_string);
			longitude_item->setFlags(Qt::ItemIsEnabled);
			table_equivalent->setItem(num_row,ColumnNames::LONGITUDE,longitude_item);

			QTableWidgetItem* angle_item = new QTableWidgetItem();
			angle_item->setFlags(Qt::ItemIsEnabled);
			angle_item->setData(Qt::DisplayRole, QVariant(0.0));
			table_equivalent->setItem(num_row,ColumnNames::ANGLE,angle_item);
		} else {

			using namespace GPlatesMaths;
			UnitQuaternion3D::RotationParams params = uq.get_rotation_params(fr.axis_hint());

			PointOnSphere euler_pole(params.axis);
			LatLonPoint llp = make_lat_lon_point(euler_pole);

			QLocale locale_;
			QString euler_pole_lat = locale_.toString(llp.latitude());
			QString euler_pole_lon = locale_.toString(llp.longitude());

			QTableWidgetItem* latitude_item = new QTableWidgetItem(euler_pole_lat);
			table_equivalent->setItem(num_row,ColumnNames::LATITUDE,latitude_item);

			QTableWidgetItem* longitude_item = new QTableWidgetItem(euler_pole_lon);
			longitude_item->setFlags(Qt::ItemIsEnabled);
			table_equivalent->setItem(num_row,ColumnNames::LONGITUDE,longitude_item);

			const double &angle = radiansToDegrees(params.angle).dval();
			
			QString angle_string = locale_.toString(angle);
			QTableWidgetItem* angle_item = new QTableWidgetItem(angle_string);
			angle_item->setFlags(Qt::ItemIsEnabled);
			table_equivalent->setItem(num_row,ColumnNames::ANGLE,angle_item);
		}
	}

}

void
GPlatesQtWidgets::TotalReconstructionPolesDialog::fill_relative_table()
{
	table_relative->clearContents();
	table_relative->setRowCount(0);

	std::multimap<GPlatesModel::integer_plate_id_type,
		GPlatesModel::ReconstructionTreeEdge::non_null_ptr_type>::const_iterator it;
	std::multimap<GPlatesModel::integer_plate_id_type,
		GPlatesModel::ReconstructionTreeEdge::non_null_ptr_type>::const_iterator it_begin = 
			d_reconstruct_ptr->get_current_reconstruction().reconstruction_tree().edge_map_begin();
	std::multimap<GPlatesModel::integer_plate_id_type,
		GPlatesModel::ReconstructionTreeEdge::non_null_ptr_type>::const_iterator it_end = 
			d_reconstruct_ptr->get_current_reconstruction().reconstruction_tree().edge_map_end();


	for(it = it_begin; it != it_end ; ++it)
	{
		int num_row = table_relative->rowCount();
		table_relative->insertRow(num_row);

	// insert the plate id into the first column of the table
		QString id_as_string;
		id_as_string.setNum(it->first);
		QTableWidgetItem* id_item = new QTableWidgetItem(id_as_string);
		id_item->setFlags(Qt::ItemIsEnabled);

		table_relative->setItem(num_row,ColumnNames::PLATEID,id_item);

		GPlatesMaths::FiniteRotation fr = it->second->relative_rotation();
		const GPlatesMaths::UnitQuaternion3D &uq = fr.unit_quat();
		if (GPlatesMaths::represents_identity_rotation(uq)) {
			QString indeterminate_string(QObject::tr("Indeterminate"));
			QTableWidgetItem*  latitude_item = new QTableWidgetItem(indeterminate_string);
			latitude_item->setFlags(Qt::ItemIsEnabled);
			table_relative->setItem(num_row,ColumnNames::LATITUDE,latitude_item);

			QTableWidgetItem*  longitude_item = new QTableWidgetItem(indeterminate_string);
			longitude_item->setFlags(Qt::ItemIsEnabled);
			table_relative->setItem(num_row,ColumnNames::LONGITUDE,longitude_item);

			QTableWidgetItem* angle_item = new QTableWidgetItem();
			angle_item->setFlags(Qt::ItemIsEnabled);
			angle_item->setData(Qt::DisplayRole, QVariant(0.0));
			table_relative->setItem(num_row,ColumnNames::ANGLE,angle_item);
		} else {

			using namespace GPlatesMaths;
			UnitQuaternion3D::RotationParams params = uq.get_rotation_params(fr.axis_hint());

			PointOnSphere euler_pole(params.axis);
			LatLonPoint llp = make_lat_lon_point(euler_pole);

			QLocale locale_;
			QString euler_pole_lat = locale_.toString(llp.latitude());
			QString euler_pole_lon = locale_.toString(llp.longitude());

			QTableWidgetItem* latitude_item = new QTableWidgetItem(euler_pole_lat);
			table_relative->setItem(num_row,ColumnNames::LATITUDE,latitude_item);

			QTableWidgetItem* longitude_item = new QTableWidgetItem(euler_pole_lon);
			longitude_item->setFlags(Qt::ItemIsEnabled);
			table_relative->setItem(num_row,ColumnNames::LONGITUDE,longitude_item);

			const double &angle = radiansToDegrees(params.angle).dval();
			
			QString angle_string = locale_.toString(angle);
			QTableWidgetItem* angle_item = new QTableWidgetItem(angle_string);
			angle_item->setFlags(Qt::ItemIsEnabled);
			table_relative->setItem(num_row,ColumnNames::ANGLE,angle_item);
		}

		// insert fixed plate into last column of table
		GPlatesModel::integer_plate_id_type fixed_id = it->second->fixed_plate();
		QString fixed_string;
		fixed_string.setNum(fixed_id);
		QTableWidgetItem* fixed_item = new QTableWidgetItem(fixed_string);
		fixed_item->setFlags(Qt::ItemIsEnabled);
		table_relative->setItem(num_row,ColumnNames::FIXED,fixed_item);
	}

}

/**
 * Fill the QTreeWidget in the second tab with data from the Reconstruction Tree
 */
void
GPlatesQtWidgets::TotalReconstructionPolesDialog::fill_reconstruction_tree()
{
	tree_reconstruction->clear();

	GPlatesModel::ReconstructionTree& tree =
			d_reconstruct_ptr->get_current_reconstruction().reconstruction_tree();

	std::vector<GPlatesModel::ReconstructionTreeEdge::non_null_ptr_type>::iterator it;
	std::vector<GPlatesModel::ReconstructionTreeEdge::non_null_ptr_type>::iterator it_begin = 
		tree.rootmost_edges_begin();
	std::vector<GPlatesModel::ReconstructionTreeEdge::non_null_ptr_type>::iterator it_end = 
		tree.rootmost_edges_end();

	for(it = it_begin; it != it_end; ++it)
	{
		//std::cerr << (*it)->moving_plate() << std::endl;
		
		// Create a QTreeWidgetItem for each of the rootmost edges, and iteratively 
		// add its children to the tree 


		QTreeWidgetItem *item = new QTreeWidgetItem(tree_reconstruction,0);
		fill_tree_item(item,*it);
		add_children_of_edge_to_tree_item(*it,item);

	}
}

/*
 * Fill the QTreeWidget in the third tab with the circuit-to-stationary-plate for each plate-id. 
 */
void
GPlatesQtWidgets::TotalReconstructionPolesDialog::fill_circuit_tree()
{
	tree_circuit->clear();

	std::multimap<GPlatesModel::integer_plate_id_type,
		GPlatesModel::ReconstructionTreeEdge::non_null_ptr_type>::const_iterator it;
	std::multimap<GPlatesModel::integer_plate_id_type,
		GPlatesModel::ReconstructionTreeEdge::non_null_ptr_type>::const_iterator it_begin = 
			d_reconstruct_ptr->get_current_reconstruction().reconstruction_tree().edge_map_begin();
	std::multimap<GPlatesModel::integer_plate_id_type,
		GPlatesModel::ReconstructionTreeEdge::non_null_ptr_type>::const_iterator it_end = 
			d_reconstruct_ptr->get_current_reconstruction().reconstruction_tree().edge_map_end();


	for(it = it_begin; it != it_end ; ++it)
	{
		// get plate id and add it to the top level of the tree
		QTreeWidgetItem *item = new QTreeWidgetItem(tree_circuit,0);
		QString id_as_string;
		id_as_string.setNum(it->first);
		item->setText(0,id_as_string);

		//std::cerr << it->first << std::endl;

		// go up the rotation tree using the parent, until 
		// we come to the stationary plate.
	
		GPlatesModel::ReconstructionTreeEdge::non_null_ptr_type edge = it->second;

		while (edge->parent_edge()) {
			QTreeWidgetItem *child = new QTreeWidgetItem(item,0);
			fill_tree_item(child,edge);

			edge = GPlatesModel::ReconstructionTreeEdge::non_null_ptr_type(
					edge->parent_edge(),
					GPlatesUtils::NullIntrusivePointerHandler());
		}
		// and finally add the edge from the last plate to the stationary plate
		QTreeWidgetItem *child = new QTreeWidgetItem(item,0);
		fill_tree_item(child,edge);		

	}


///////////////////////////////////////////////////////////////////////////////


}

void
GPlatesQtWidgets::TotalReconstructionPolesDialog::update()
{
	set_time(d_reconstruct_ptr->get_current_reconstruction_time());
	set_plate(d_reconstruct_ptr->get_current_anchored_plate_id());
	fill_equivalent_table();
	fill_relative_table();
	fill_reconstruction_tree();
	fill_circuit_tree();
}

void
GPlatesQtWidgets::TotalReconstructionPolesDialog::export_relative()
{
	handle_export(*table_relative);
#if 0
	QString filename = QFileDialog::getSaveFileName(this,
			tr("Save As"), "", tr("CSV file (*.csv)"));

	if (filename.isEmpty()){
		return;
	}

	GPlatesGui::CsvExport::export_table(filename,table_relative);

	return;
#endif
}

void
GPlatesQtWidgets::TotalReconstructionPolesDialog::export_equivalent()
{
	handle_export(*table_equivalent);
#if 0
	QString filename = QFileDialog::getSaveFileName(this,
			tr("Save As"), "", tr("CSV file (*.csv)"));

	if (filename.isEmpty()){
		return;
	}

	GPlatesGui::CsvExport::export_table(filename,table_equivalent);

	return;
#endif
}


void
GPlatesQtWidgets::TotalReconstructionPolesDialog::handle_export(
		const QTableWidget &table)
{
	// Build the dialog we present to the user. Keep the same instance so that
	// the dialog will remember where the user last exported to, etc.
	// Parented to this dialog; memory managed by Qt.
	static QFileDialog *file_dialog = create_export_file_dialog(this);
	// Build a map to let us look up the options the user wants based on what
	// file filter was selected in the dialog.
	static const FileDialogFilterMapType &filter_map = build_export_filter_map();
	
	// Pop up and ask for file.
	if (file_dialog->exec()) {
		QStringList filenames = file_dialog->selectedFiles();
		if (filenames.size() == 1 && ! filenames.front().isEmpty()) {
			QString filename = filenames.front();
			QString filter = file_dialog->selectedFilter();
			if (filter_map.contains(filter)) {
				GPlatesGui::CsvExport::ExportOptions options = filter_map.value(filter);
				GPlatesGui::CsvExport::export_table(filename, options, table);
			} else {
				// Somehow, user chose filter that we didn't put in there.
				QMessageBox::critical(this, tr("Invalid export filter"), tr("Please specify a CSV file format variant in the save dialog."));
			}
		}
	}
}

