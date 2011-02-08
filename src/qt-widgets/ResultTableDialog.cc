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

#include <QtGui/QDialogButtonBox>
#include <QtGui/QHeaderView>
#include <QtGui/QPushButton>
#include <boost/optional.hpp> 
#include "ResultTableDialog.h"

#include "utils/ExportTemplateFilenameSequence.h"

using namespace GPlatesQtWidgets;

const QString ResultTableDialog::filter_csv(QObject::tr("CSV (*.csv)"));
const QString ResultTableDialog::filter_csv_ext(QObject::tr("csv"));
const QString ResultTableDialog::page_label_format("Page: %d/%d ");

	
void
ResultTableDialog::update_page_label()
{
	char tmp[20];
	sprintf(
			tmp, 
			page_label_format.toStdString().c_str(), 
			d_page_index + 1, 
			d_page_num);
	page_label->setText(
			QApplication::translate(
					"ResultTableDialog", 
					tmp, 
					0, 
					QApplication::UnicodeUTF8));
}

void 
ResultTableDialog::update_time_label()
{
	char tmp[50];
	sprintf(
			tmp, 
			"Reconstruction time: %f Ma", 
			d_data_tables[d_page_index].reconstruction_time());
	time_label->setText(
			QApplication::translate(
					"ResultTableDialog", 
					tmp, 
					0, 
					QApplication::UnicodeUTF8));
}

ResultTableDialog::ResultTableDialog(
		const std::vector< DataTable > data_tables,
		GPlatesPresentation::ViewState &view_state,
		QWidget *parent_,
		bool old_version) :
	d_data_tables(data_tables),
	d_view_state(view_state),
	d_page_index(0),
	d_old_version(old_version)
{
	setupUi(this);
	setModal(false);
	table_view = new ResultTableView(this);
	table_view->setObjectName(QString::fromUtf8("table_view"));
    table_view->setSelectionMode(QAbstractItemView::SingleSelection);
    table_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_view->setVerticalScrollMode(QAbstractItemView::ScrollPerItem);
    table_view->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
	table_view->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
	table_view->horizontalHeader()->setStretchLastSection(false);
	//table_view->setContextMenuPolicy(Qt::ActionsContextMenu);
	d_page_num = data_tables.size();
	if(d_page_num > 0)
	{
		d_table_model_prt.reset(
				new ResultTableModel(
						d_data_tables.at(d_page_index), 
						this));
	}

	if(d_data_tables.size() > d_page_index)
	{
		table_view->setModel(d_table_model_prt.get());
	}
	table_view->resizeColumnsToContents(); 
	vboxLayout->addWidget(table_view);

	if(d_old_version)
	{
		init_controls();
	}
	else
	{
		QHBoxLayout* hboxLayout;
		QSpacerItem* spacerItem;
		hboxLayout = new QHBoxLayout();
		hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
		spacerItem = new QSpacerItem(91, 25, QSizePolicy::Expanding, QSizePolicy::Minimum);
		hboxLayout->addItem(spacerItem);
		QPushButton* pushButton_close = new QPushButton(this);
		pushButton_close->setObjectName(QString::fromUtf8("pushButton_close"));
		hboxLayout->addWidget(pushButton_close);
		vboxLayout->addLayout(hboxLayout);
		pushButton_close->setText(QApplication::translate("ResultTableDialog", "close", 0, QApplication::UnicodeUTF8));
		QObject::connect(pushButton_close, SIGNAL(clicked()), this, SLOT(reject()));
	}
	
	update();
}	


void
ResultTableDialog::init_controls()
{
	QHBoxLayout* hboxLayout_1;
	QSpacerItem* spacerItem_1;
	QPushButton* pushButton_goto;
	hboxLayout_1 = new QHBoxLayout();
    hboxLayout_1->setObjectName(QString::fromUtf8("hboxLayout_1"));
	time_label = new QLabel();
	hboxLayout_1->addWidget(time_label);
	update_time_label();
    spacerItem_1 = new QSpacerItem(91, 25, QSizePolicy::Expanding, QSizePolicy::Minimum);
    hboxLayout_1->addItem(spacerItem_1);

	pushButton_goto = new QPushButton(this);
    pushButton_goto->setObjectName(QString::fromUtf8("pushButton_goto"));
    hboxLayout_1->addWidget(pushButton_goto);
	
	spinBox_page = new QSpinBox();
	spinBox_page->setObjectName(QString::fromUtf8("spinBox_page"));
    spinBox_page->setMaximum(d_page_num);
	spinBox_page->setMinimum(1);
	hboxLayout_1->addWidget(spinBox_page);
	
	page_label = new QLabel();
	hboxLayout_1->addWidget(page_label);
	update_page_label();

	vboxLayout->addLayout(hboxLayout_1);

	QHBoxLayout* hboxLayout;
	QPushButton* pushButton_close;
	QSpacerItem* spacerItem;
	QPushButton* pushButton_save;
	QPushButton* pushButton_save_all;

	hboxLayout = new QHBoxLayout();
    hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
    spacerItem = new QSpacerItem(91, 25, QSizePolicy::Expanding, QSizePolicy::Minimum);

    pushButton_close = new QPushButton(this);
    pushButton_close->setObjectName(QString::fromUtf8("pushButton_close"));
    hboxLayout->addWidget(pushButton_close);

    pushButton_save = new QPushButton(this);
    pushButton_save->setObjectName(QString::fromUtf8("pushButton_save"));
    hboxLayout->addWidget(pushButton_save);

	pushButton_save_all = new QPushButton(this);
    pushButton_save_all->setObjectName(QString::fromUtf8("pushButton_save_all"));
    hboxLayout->addWidget(pushButton_save_all);

    hboxLayout->addItem(spacerItem);

    pushButton_previous = new QPushButton(this);
    pushButton_previous->setObjectName(QString::fromUtf8("pushButton_previous"));
    hboxLayout->addWidget(pushButton_previous);

	pushButton_next = new QPushButton(this);
    pushButton_next->setObjectName(QString::fromUtf8("pushButton_next"));
	hboxLayout->addWidget(pushButton_next);

    vboxLayout->addLayout(hboxLayout);

	pushButton_close->setText(QApplication::translate("ResultTableDialog", "close", 0, QApplication::UnicodeUTF8));
    pushButton_save->setText(QApplication::translate("ResultTableDialog", "Save", 0, QApplication::UnicodeUTF8));
    pushButton_next->setText(QApplication::translate("ResultTableDialog", "next page", 0, QApplication::UnicodeUTF8));
	pushButton_previous->setText(QApplication::translate("ResultTableDialog", "previous page", 0, QApplication::UnicodeUTF8));
	pushButton_goto->setText(QApplication::translate("ResultTableDialog", "goto page", 0, QApplication::UnicodeUTF8));
	pushButton_save_all->setText(QApplication::translate("ResultTableDialog", "save all", 0, QApplication::UnicodeUTF8));

	QObject::connect(pushButton_close, SIGNAL(clicked()), this, SLOT(reject()));
	QObject::connect(pushButton_save, SIGNAL(clicked()), this, SLOT(accept()));
	QObject::connect(pushButton_next, SIGNAL(clicked()), this, SLOT(handle_next_page()));
	QObject::connect(pushButton_previous, SIGNAL(clicked()), this, SLOT(handle_previous_page()));
	QObject::connect(pushButton_goto, SIGNAL(clicked()), this, SLOT(handle_goto_page()));
	QObject::connect(pushButton_save_all, SIGNAL(clicked()), this, SLOT(handle_save_all()));
	return;
}


void
ResultTableDialog::reject()
{
	d_page_index = 0;
	d_page_num = 0;
	done(QDialog::Rejected);
	d_data_tables.clear();
}

void
ResultTableDialog::accept()
{
	GPlatesQtWidgets::SaveFileDialog::filter_list_type filters;
	filters.push_back(FileDialogFilter(filter_csv, filter_csv_ext));

	GPlatesQtWidgets::SaveFileDialog save_dialog(
			this,
			"Save as CSV",
			filters,
			d_view_state);
	boost::optional<QString> filename_opt = save_dialog.get_file_name();
	if(filename_opt)
	{
		d_table_model_prt->data_table().export_as_CSV(*filename_opt);
	}
	else
	{
		qDebug() << "No file name.";
		return;
	}
}

void
ResultTableDialog::handle_next_page()
{
	d_page_index++;
	update();
}

void
ResultTableDialog::update()
{
	if(d_page_index >= d_page_num)
	{
		d_page_index = d_page_num - 1 ;
		qWarning() << "The page index is invalid." << d_page_index;
		return;
	}

	pushButton_previous->setDisabled(false);
	pushButton_next->setDisabled(false);

	if(0 == d_page_index )
	{
		pushButton_previous->setDisabled(true);
	}
	if( d_page_num - 1 == d_page_index)
	{
		pushButton_next->setDisabled(true);
	}

	spinBox_page->setValue(d_page_index+1);
	update_page_label();
	d_table_model_prt.reset(
			new ResultTableModel(
					d_data_tables.at(d_page_index)));
	table_view->setModel(d_table_model_prt.get());
	update_time_label();
}

void
ResultTableDialog::handle_previous_page()
{
	if(0 == d_page_index)
	{
		return;
	}
	d_page_index--;
	update();
}

void
ResultTableDialog::handle_goto_page()
{
	d_page_index = spinBox_page->value() -1 ;
	update();
}

void
ResultTableDialog::handle_save_all()
{
	GPlatesQtWidgets::SaveFileDialog::filter_list_type filters;
	filters.push_back(FileDialogFilter(filter_csv, filter_csv_ext));

	GPlatesQtWidgets::SaveFileDialog save_dialog(
			this,
			"Save as CSV",
			filters,
			d_view_state);
	boost::optional<QString> filename_opt = save_dialog.get_file_name();
	QString basename;
	if(filename_opt)
	{
		int idx = (*filename_opt).lastIndexOf(".csv",-1);
		if(-1 == idx)
		{
			basename = *filename_opt;
		}
		else
		{
			basename = (*filename_opt).left(idx);
		}
	}
	else
	{
		qDebug() << "Save dialog canceled.";
		return;
	}

	if(d_page_num > 1)
	{
		basename = basename + "_" + "%f" + ".csv";
		double time_start, time_end, time_incre;
		time_start = d_data_tables[0].reconstruction_time();
		time_end = d_data_tables[d_page_num-1].reconstruction_time();
		time_incre = (time_start - time_end)/(d_page_num - 1);
		GPlatesUtils::ExportTemplateFilenameSequence filenames(
				basename,
				0,
				time_end,
				time_start,
				time_incre,
				true);
		GPlatesUtils::ExportTemplateFilenameSequence::const_iterator filename_it = filenames.begin();
		for(unsigned i = 0; i < d_page_num; i++, ++filename_it)
		{
			
			if (filename_it == filenames.end()) 
			{
				qDebug() << "Failed to get export file name when exporting result table.";
				return;
			}
			d_table_model_prt->data_table().export_as_CSV( *filename_it);
		}
	}
	else if(1 == d_page_num)
	{
		basename = basename + "_" + QString().setNum(d_data_tables[0].reconstruction_time());
		d_table_model_prt->data_table().export_as_CSV(basename);
	}
	else
	{
		qDebug() << "No data table to be exported.";
	}
	return;
}


void
ResultTableDialog::data_arrived(
		const DataTable& table)
{
	d_table_model_prt.reset(
			new ResultTableModel(table));
	table_view->setModel(d_table_model_prt.get());
	
	//table_view->setModel(new ResultTableModel(table));
}


