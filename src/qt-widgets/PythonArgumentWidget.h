/* $Id: ColourScaleWidget.h 11071 2011-02-18 03:17:55Z elau $ */

/**
 * \file
 * Contains the definition of the class ElidedLabel.
 *
 * $Revision: 11071 $
 * $Date: 2011-02-18 14:17:55 +1100 (Fri, 18 Feb 2011) $ 
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
 
#ifndef GPLATES_QTWIDGETS_PYTHONARGUMENTWIDGET_H
#define GPLATES_QTWIDGETS_PYTHONARGUMENTWIDGET_H
#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QWidget>
#include <QtGui/QColorDialog>
#include <QtGui/QFileDialog>
#include <QtGui/QLineEdit>
#include <QPalette>
#include "gui/PythonConfiguration.h"

#include "global/python.h"
#if !defined(GPLATES_NO_PYTHON)

namespace GPlatesQtWidgets
{
	class PythonArgumentWidget : public QWidget
	{
		Q_OBJECT

	public:
		explicit
		PythonArgumentWidget(
				QWidget *parent_ = NULL) :
			QWidget(parent_)
		{ }

		Q_SIGNALS:
		void
		configuration_changed();
	};

	class PythonArgDefaultWidget : 
		public PythonArgumentWidget
	{
		Q_OBJECT

	public:
		explicit
		PythonArgDefaultWidget(
				GPlatesGui::PythonCfgItem* cfg_item,
				QWidget *parent_ = NULL) :
		PythonArgumentWidget(parent_),
		d_cfg_item(cfg_item)
		{
			QHBoxLayout*hboxLayout = new QHBoxLayout(this);
			hboxLayout->setSpacing(2);
			hboxLayout->setContentsMargins(1, 1, 1, 1);
			QLineEdit* line_edit = new QLineEdit(this);
			hboxLayout->addWidget(line_edit);
			line_edit->setText(d_cfg_item->get_value());

			QObject::connect(
					line_edit,
					SIGNAL(textChanged(const QString&)),
					this,
					SLOT(handle_string_changed(const QString&)));
			
			QObject::connect(
					line_edit,
					SIGNAL(editingFinished()),
					this,
					SLOT(handle_editing_finished()));
			 
		}
	private Q_SLOTS:
		
		void
		handle_string_changed(
				const QString& str)
		{
			d_cfg_item->set_value(str.trimmed());
		}

		void
		handle_editing_finished()
		{
			Q_EMIT configuration_changed();
		}


	private:
		GPlatesGui::PythonCfgItem*  d_cfg_item;
	};

	class PythonArgColorWidget : 
		public PythonArgumentWidget
	{
		Q_OBJECT

	public:
		explicit
		PythonArgColorWidget(
				GPlatesGui::PythonCfgItem* cfg_item,
				QWidget *parent_ = NULL) :
			PythonArgumentWidget(parent_),
			d_cfg_item(cfg_item)
		{
			hboxLayout = new QHBoxLayout(this);
			hboxLayout->setSpacing(2);
			hboxLayout->setContentsMargins(1, 1, 1, 1);
			hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
			

			color_name = new QLineEdit(this);
			color_name->setEnabled(false);

			color_name->setText(cfg_item->get_value());

			QString s = "background-color: ";
			color_name->setStyleSheet(s + cfg_item->get_value());

			choose_button = new QPushButton("choose...",this);
			hboxLayout->addWidget(color_name);
			hboxLayout->addWidget(choose_button);
			

			QObject::connect(
					color_name,
					SIGNAL(textChanged(const QString&)),
					this,
					SLOT(handle_color_name_changed(const QString&)));

			QObject::connect(
					choose_button,
					SIGNAL(clicked(bool)),
					this,
					SLOT(handle_choose_button_clicked(bool)));
		}
		private Q_SLOTS:
			void
			handle_choose_button_clicked(bool b)
			{
				QColor selected_colour = QColorDialog::getColor();
				if (selected_colour.isValid())
				{
					color_name->setText(selected_colour.name());

					QString s = "background-color: ";
					color_name->setStyleSheet(s + selected_colour.name());
				}
			}

			void
			handle_color_name_changed(const QString& _color_name)
			{
				d_cfg_item->set_value(_color_name);
				Q_EMIT configuration_changed();
			}

	private:
		QHBoxLayout* hboxLayout;
		QLineEdit *color_name;
		QPushButton* choose_button;
		GPlatesGui::PythonCfgItem*  d_cfg_item;
	};

	class PythonArgPaletteWidget : 
		public PythonArgumentWidget
	{
		Q_OBJECT

	public:
		explicit
		PythonArgPaletteWidget(
				GPlatesGui::PythonCfgItem* cfg_item,
				QWidget *parent_ = NULL) :
			PythonArgumentWidget(parent_),
			d_cfg_item(cfg_item)
		{
			hboxLayout = new QHBoxLayout(this);
			hboxLayout->setSpacing(2);
			hboxLayout->setContentsMargins(1, 1, 1, 1);
			hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
			
			line_edit = new QLineEdit(this);
			choose_button = new QPushButton("open...",this);
			
			line_edit->setText(d_cfg_item->get_value());
			line_edit->setEnabled(false);
			
			spacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
			//hboxLayout->addItem(spacer);
			hboxLayout->addWidget(line_edit);
			hboxLayout->addWidget(choose_button);
			

			QObject::connect(
					choose_button,
					SIGNAL(clicked(bool)),
					this,
					SLOT(handle_choose_button_clicked(bool)));

			QObject::connect(
					line_edit,
					SIGNAL(textChanged(const QString&)),
					this,
					SLOT(handle_cpt_file_changed(const QString&)));
		}
		private Q_SLOTS:
			void
			handle_choose_button_clicked(bool b)
			{
				QStringList file_names = QFileDialog::getOpenFileNames(
						this,
						QT_TR_NOOP("Open Files"),
						d_last_open_directory,
						QT_TR_NOOP("CPT files (*.cpt);;All files (*)"));

				if (!file_names.isEmpty())
				{
					d_last_open_directory = QFileInfo(file_names.first()).path();
					line_edit->setText(file_names.first());
				}
			}

			void
			handle_cpt_file_changed(const QString& cpt_file)
			{
				d_cfg_item->set_value(cpt_file);
				Q_EMIT configuration_changed();
			}
	private:
		QHBoxLayout* hboxLayout;
		QLineEdit* line_edit;
		QPushButton* choose_button;
		QSpacerItem* spacer;
		QString d_last_open_directory;
		GPlatesGui::PythonCfgItem*  d_cfg_item;
	};
}

#endif

#endif  // GPLATES_QTWIDGETS_PYTHONARGUMENTWIDGET_H


