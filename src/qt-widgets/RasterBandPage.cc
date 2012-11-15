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

#include <set>
#include <QString>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QItemDelegate>
#include <QModelIndex>
#include <QLineEdit>
#include <QDebug>

#include "RasterBandPage.h"


#include <boost/foreach.hpp>
namespace
{
	using GPlatesQtWidgets::RasterBandPageInternals::BandNameComboBox;

	class BandNameDelegate :
			public QItemDelegate
	{
	public:

		BandNameDelegate(
				QTableWidget *parent_) :
			QItemDelegate(parent_),
			d_table(parent_)
		{  }

		virtual
		QWidget *
		createEditor(
				QWidget *parent_,
				const QStyleOptionViewItem &option,
				const QModelIndex &index) const
		{
			QString existing = d_table->item(index.row(), index.column())->text();

			BandNameComboBox *combobox = new BandNameComboBox(d_table, parent_);
			combobox->addItem(existing);
			combobox->setEditable(true);

			return combobox;
		}

		virtual
		void
		setEditorData(
				QWidget *editor,
				const QModelIndex &index) const
		{
			BandNameComboBox *combobox = dynamic_cast<BandNameComboBox *>(editor);
			if (combobox)
			{
				combobox->set_model_index(index);
			}
		}

		virtual
		void
		setModelData(
				QWidget *editor,
				QAbstractItemModel *model,
				const QModelIndex &index) const
		{
			BandNameComboBox *combobox = dynamic_cast<BandNameComboBox *>(editor);
			if (combobox)
			{
				QString text = combobox->currentText();
				d_table->setItem(index.row(), index.column(), new QTableWidgetItem(text));
			}
		}

	private:

		QTableWidget *d_table;
	};
}


GPlatesQtWidgets::RasterBandPage::RasterBandPage(
		std::vector<QString> &band_names,
		QWidget *parent_) :
	QWizardPage(parent_),
	d_band_names(band_names)
{
	setupUi(this);

	setTitle("Raster Band Names");
	setSubTitle("Assign unique names to the bands in the raster.");

	band_names_table->verticalHeader()->hide();
	band_names_table->horizontalHeader()->setStretchLastSection(true);
	band_names_table->horizontalHeader()->setHighlightSections(false);

	band_names_table->setItemDelegateForColumn(1, new BandNameDelegate(band_names_table));
	
	make_signal_slot_connections();

	isComplete();
}


void
GPlatesQtWidgets::RasterBandPage::initializePage()
{
	populate_table();
}


bool
GPlatesQtWidgets::RasterBandPage::isComplete() const
{
	// Rules: no empty band names, no duplicate band names.
	std::set<QString> band_name_set;
	
	BOOST_FOREACH(const QString &band_name, d_band_names)
	{
		QString trimmed = band_name.trimmed();
		if (trimmed.isEmpty())
		{
			warning_container_widget->show();
			warning_label->setText(tr("Band names cannot be empty."));
			return false;
		}

		if (!band_name_set.insert(trimmed).second)
		{
			// Already in set.
			warning_container_widget->show();
			warning_label->setText(tr("Two bands cannot be assigned the same name."));
			return false;
		}
	}

	warning_container_widget->hide();
	return true;
}


void
GPlatesQtWidgets::RasterBandPage::handle_table_cell_changed(
		int row,
		int column)
{
	if (column == 1 /* band name is second column */)
	{
		QString text = band_names_table->item(row, 1)->text();
		d_band_names[row] = text.trimmed();

		emit completeChanged();
	}
}


void
GPlatesQtWidgets::RasterBandPage::make_signal_slot_connections()
{
	QObject::connect(
			band_names_table,
			SIGNAL(cellChanged(int, int)),
			this,
			SLOT(handle_table_cell_changed(int, int)));
}


void
GPlatesQtWidgets::RasterBandPage::populate_table()
{
	band_names_table->setRowCount(d_band_names.size());

	for (unsigned int i = 0; i != d_band_names.size(); ++i)
	{
		QTableWidgetItem *band_number_item = new QTableWidgetItem(QString::number(i + 1));
		band_number_item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
		band_number_item->setFlags(Qt::ItemIsEnabled);
		band_names_table->setItem(i, 0, band_number_item);

		// Seems we need to close the editor before opening a new one otherwise
		// changing the sort order will only affect the filenames and not depths.
		if (band_names_table->item(i, 1))
		{
			band_names_table->closePersistentEditor(band_names_table->item(i, 1));
		}
		QTableWidgetItem *band_name_item = new QTableWidgetItem(d_band_names[i]);
		band_name_item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsEditable);
		band_names_table->setItem(i, 1, band_name_item);

		band_names_table->openPersistentEditor(band_name_item);
	}
}


GPlatesQtWidgets::RasterBandPageInternals::BandNameComboBox::BandNameComboBox(
		QTableWidget *table,
		QWidget *parent_) :
	QComboBox(parent_),
	d_table(table)
{
	QObject::connect(
			this,
			SIGNAL(editTextChanged(const QString &)),
			this,
			SLOT(handle_text_changed(const QString &)));
}


void
GPlatesQtWidgets::RasterBandPageInternals::BandNameComboBox::set_model_index(
		const QModelIndex &model_index)
{
	d_model_index = model_index;
}


void
GPlatesQtWidgets::RasterBandPageInternals::BandNameComboBox::handle_text_changed(
		const QString &text)
{
	d_table->setItem(
			d_model_index.row(),
			d_model_index.column(),
			new QTableWidgetItem(text));
}

