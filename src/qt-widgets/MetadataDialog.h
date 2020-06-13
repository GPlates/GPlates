/* $Id: MeshDialog.h 12448 2011-10-24 04:47:04Z mchin $ */
 
/**
 * \file 
 * $Revision: 12448 $
 * $Date: 2011-10-24 15:47:04 +1100 (Mon, 24 Oct 2011) $
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
#ifndef METADATA_DIALOG_H
#define METADATA_DIALOG_H

#include <boost/shared_ptr.hpp>
#include <iostream>
#include <QComboBox>
#include <QObject>
#include <QDialog>
#include <QPushButton>
#include <QTextBrowser>
#include <QTextEdit>

#include "ui_AddContributorWidgetUi.h"
#include "ui_AddCreatorWidgetUi.h"
#include "ui_AddGTSWidgetUi.h"
#include "ui_MetadataDialogUi.h"

#include "file-io/PlatesRotationFileProxy.h"

#include "model/FeatureHandle.h"
#include "model/Metadata.h"


namespace GPlatesModel
{
	class Metadata;
}

namespace GPlatesPropertyValues
{
	class GpmlFiniteRotation;
}

namespace GPlatesQtWidgets
{
	class MetadataDialog;
	
	class DataEdit: 
		public QTextEdit
	{
		Q_OBJECT
	public:
		DataEdit(
				QWidget* parent_):
			QTextEdit(parent_)
		{ }


	Q_SIGNALS:
			void edit_finished();
	protected:
		virtual
		void 
		focusOutEvent ( 
				QFocusEvent * event_ )
		{
			Q_EMIT edit_finished();
		}
	};

	class MetadataTextEditor :
		public QWidget
	{
		Q_OBJECT

	public:
		MetadataTextEditor(
				QString &text,
				MetadataDialog *dlg,
				bool opt = false,
				bool readonly = false):
			QWidget(NULL),
			d_txt(text),
			d_dlg(dlg),
			d_optional(opt),
			d_readonly(readonly)
		{
			setup_ui();
			d_editor->setVisible(false);
			setup_browser();
			setup_editor();
		
			d_edit_button->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
			d_edit_button->setFixedHeight(20);
			d_edit_button->setFixedWidth(40);
			d_del_button->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
			d_del_button->setFixedHeight(20);
			d_del_button->setFixedWidth(40);
			if(!d_optional)
			{
				d_del_button->setDisabled(true);
			}

			if(readonly)
			{
				d_edit_button->setDisabled(true);
			}

			QObject::connect(
					d_edit_button,
					SIGNAL(clicked()),
					this,
					SLOT(edit_button_clicked()));

			QObject::connect(
					d_del_button,
					SIGNAL(clicked()),
					this,
					SLOT(del_button_clicked()));

			QObject::connect(
					d_editor,
					SIGNAL(textChanged ()),
					this,
					SLOT(editor_text_changed()));

			QObject::connect(
					d_editor,
					SIGNAL(edit_finished()),
					this,
					SLOT(handle_edit_finished()));
		}

	protected Q_SLOTS:
	
		void
		edit_button_clicked()
		{
			d_edit_button->setDisabled(true);
			d_editor->setVisible(true);
			d_browser->setVisible(false);
			QTextCursor c = d_editor->textCursor();
			c.setPosition(0);
			c.setPosition(d_txt.length(), QTextCursor::KeepAnchor);
			d_editor->setTextCursor(c);
			d_editor->setFocus();
		}

		void
		del_button_clicked();

		void
		editor_text_changed()
		{
			d_txt = d_editor->toPlainText();
		}

		void
		handle_edit_finished();

	protected:
	
		void
		setup_browser()
		{
			QTextDocument* doc = new QTextDocument(d_txt);
			if(d_txt.startsWith("http://"))
			{
				QString html_str = QString("<html><body><a href=\"%1\">%1</a></html></body>").arg(d_txt);
				doc->setHtml(html_str);
				d_browser->setOpenExternalLinks(true);
			}
			d_browser->setDocument(doc);
			d_browser->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
			d_browser->setFixedWidth(350);
			if(d_txt.size()>60 || d_txt.indexOf('\n')!=-1)
			{
				d_browser->setFixedHeight(75);
			}
			else
			{
				d_browser->setFixedHeight(25);
				d_browser->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
			}
			//d_browser->setFixedHeight(d_browser->document()->size().height()); //+ d_browser->contentsMargins().top()*2);
			QPalette p = d_browser->palette();
			p.setColor(QPalette::Base, QColor(228,228,228));
			d_browser->setPalette(p);
		}

		void
		setup_editor()
		{
			d_editor->setDocument(new QTextDocument(d_txt));
		}

		void
		setup_ui()
		{
			h_layout = new QHBoxLayout(this);
			d_editor = new DataEdit(this);
			d_browser = new QTextBrowser(this);
			h_layout->addWidget(d_editor);
			h_layout->addWidget(d_browser);
		
			d_edit_button = new QPushButton("Edit",this);
			d_del_button = new QPushButton("Del",this);
			h_layout->addWidget(d_edit_button);
			h_layout->addWidget(d_del_button);
		}
		QHBoxLayout *h_layout;
		DataEdit *d_editor;
		QTextBrowser *d_browser;
		QPushButton *d_edit_button, *d_del_button;
		QString &d_txt;
		MetadataDialog *d_dlg;
		bool d_optional, d_readonly;
	};

	class AddContributorWidget :
		public QWidget,
		public Ui_AddContributorWidget
	{
		public:
		AddContributorWidget(QWidget *parent_=NULL):
			  QWidget(parent_)
		{ setupUi(this); }
	};

	class AddGTSWidget :
		public QWidget,
		public Ui_AddGTSWidget
	{
		public:
		AddGTSWidget(QWidget *parent_=NULL):
		  QWidget(parent_)
		  { setupUi(this); }
	};


	class AddCreatorWidget :
		public QWidget,
		public Ui_AddCreatorWidget
	{
	public:
		AddCreatorWidget(QWidget *parent_=NULL):
		  QWidget(parent_)
		  { setupUi(this); }
	};


	class MetadataDialog: 
		public QDialog,
		protected Ui_MetadataDialog 
	{

		Q_OBJECT
	public:
		MetadataDialog(
				QWidget *parent_ = NULL);

		enum MetaType
		{
			FC,
			MPRS,
			POLE,
			EMPTY
		};

		void
		set_data(
				const GPlatesModel::FeatureCollectionMetadata &d)
		{
			d_fc_meta = d;
		}

		void
		set_data(
				GPlatesModel::FeatureHandle::iterator iter);

		void
		set_data(
				GPlatesModel::FeatureHandle::iterator iter,
				QTreeWidgetItem*);

		void
		set_data(
				GPlatesModel::FeatureHandle::weak_ref f,
				QTreeWidgetItem*);

		void
		clear_data()
		{
			d_type = EMPTY;
			refresh();
		}

		void
		set_grot_proxy(
				GPlatesFileIO::PlatesRotationFileProxy *proxy)
		{
			d_grot_proxy = proxy;
		}

		void
		save()
		{
			switch (d_type)
			{
			case FC:
				save_fc_meta();
				break;
			case MPRS:
				save_mprs_meta();
				break;
			case POLE:
				save_pole_meta();
				break;
			case EMPTY:
			default:
				break;
			}
		}

		void
		delete_row(
				MetadataTextEditor*);

		void
		refresh();

		void
		refresh_metadata_table()
		{
			QTreeWidgetItem *current = meta_tree->currentItem();
			if(current)
			{
				int current_type = current->type();
				if(d_func_map.find(current_type) != d_func_map.end())
				{
					(this->*d_func_map[current_type])();
				}
			}
		}

		void
		refresh_add_new_entry_combobox();

	public Q_SLOTS:
		void
		handle_current_item_changed(
				QTreeWidgetItem *current,
				QTreeWidgetItem *previous);

		void
		handle_add_simple_entry_clicked();

		void
		handle_add_contributor_clicked();

		void
		handle_add_gts_clicked();

		void
		handle_add_creator_clicked();

		void
		handle_remove_button_clicked();

	protected:
		
		void
		set_meta_table_style();

		void
		show_creator();

		void
		show_dc();

		void
		show_rights();

		void
		show_header_metadata();

		void
		show_date();

		void
		show_coverage();

		void
		show_mprs();

		void
		show_default_pole_data();

		/*
		* show the data which only applies to the rotation sequence.
		*/
		void
		show_mprs_only_data();

		void
		show_pole();

		void
		show_contributors();

		void
		show_timescales();

		void
		show_bibinfo();

		void
		show_data();

		void
		show_gts();
		
		void
		show_hell();

		void
		show_au();
		
		void
		populate_fc_meta();
		
		void
		polulate_mprs();
			
		void
		populate_pole();

		void
		save_fc_meta();
		
		void
		save_mprs_meta();
		
		void
		save_pole_meta();

		void
		show_gts(
				GPlatesModel::GeoTimeScale&,
				bool readonly = false);

		void
		show_contributor(
				GPlatesModel::DublinCoreMetadata::Contributor &contr, 
				bool readonly = false);

		/*
		* Merge d_mprs_data and d_pole_data to get metadata applying to the pole.
		* The metadata in d_pole_data overwrites metadata in d_mprs_data.
		*/
		GPlatesModel::MetadataContainer
		get_pole_metadata(
				const GPlatesModel::MetadataContainer&,
				const GPlatesModel::MetadataContainer&);

		GPlatesPropertyValues::GpmlFiniteRotation *
		get_gpml_total_reconstruction_pole(
				GPlatesModel::PropertyValue::non_null_ptr_type);

		void
		hide_all_opt_gui_widget()
		{
			d_add_GTS_widget->hide();
			d_add_contr_widget->hide();
			add_simple_entry_group->hide();
			d_add_creator_widget->hide();
			remove_button->hide();
		}

		QString
		valid_unique_name(
				const QString &name, 
				const std::vector<QString> &name_vec);

		
		GPlatesModel::MetadataContainer::iterator
		default_pole_data_begin();

		GPlatesModel::MetadataContainer
		get_mprs_only_data()
		{
			return GPlatesModel::MetadataContainer(d_mprs_data.begin(), default_pole_data_begin()); 
		}

		GPlatesModel::MetadataContainer
		get_default_pole_data()
		{
			return GPlatesModel::MetadataContainer(default_pole_data_begin(), d_mprs_data.end()); 
		}

		bool
		is_the_contributor_name(
				const GPlatesModel::DublinCoreMetadata::Contributor &contr, 
				const QString &name)
		{
			return (contr.id == name);
		}

		void
		remove_contributor(
				QString &name)
		{
			std::remove_if(
					d_fc_meta.get_dc_data().contributors.begin(),
					d_fc_meta.get_dc_data().contributors.end(),
					boost::bind(&MetadataDialog::is_the_contributor_name,this,_1,name));
			d_fc_meta.get_dc_data().contributors.pop_back();
			save();
			meta_tree->clear();
			populate_fc_meta();
			meta_tree->setCurrentItem(d_contributor_item);
			
		}
		
		bool
		is_the_gts_name(
				const GPlatesModel::GeoTimeScale &scale, 
				const QString &name)
		{
			return (scale.id == name);
		}

		void
		remove_gts(
				QString &name)
		{
			std::remove_if(
					d_fc_meta.get_geo_time_scales().begin(),
					d_fc_meta.get_geo_time_scales().end(),
					boost::bind(&MetadataDialog::is_the_gts_name,this,_1,name));
			d_fc_meta.get_geo_time_scales().pop_back();
			save();
			meta_tree->clear();
			populate_fc_meta();
			meta_tree->setCurrentItem(d_gts_item);
		}
		
		bool
		is_the_creator_name(
				const GPlatesModel::DublinCoreMetadata::Creator &c, 
				const QString &name)
		{
			return (c.name == name);
		}

		void
		remove_creator(
				QString &name)
		{
			std::remove_if(
					d_fc_meta.get_dc_data().creators.begin(),
					d_fc_meta.get_dc_data().creators.end(),
					boost::bind(&MetadataDialog::is_the_creator_name,this,_1,name));
			d_fc_meta.get_dc_data().creators.pop_back();
			save();
			meta_tree->clear();
			populate_fc_meta();
			meta_tree->setCurrentItem(d_creator_item);
		}

		~MetadataDialog(){}
		
		AddGTSWidget *d_add_GTS_widget;
		AddContributorWidget *d_add_contr_widget;
		AddCreatorWidget *d_add_creator_widget;
	private:
		enum ItemType
		{
			DC=1,
			CREATOR,
			CONTRIBUTORS,
			RIGHTS,
			GPML_META,
			DATE,
			COVERAGE,
			GEOTIMESCALE,
			BIBINFO,
			SEQUENCE_META,
			MPRS_DATA,
			DEFAULT_POLE_DATA,
			POLE_META,
			POLE_META_PP,
			POLE_META_C,
			POLE_META_GTS,
			POLE_META_AU,
			POLE_META_DOI,
			POLE_META_REF,
			POLE_META_HELL,
			POLE_META_T,
			POLE_META_CHRONID,
			NUM_OF_TYPES
		};

		GPlatesModel::MetadataContainer d_mprs_data, d_pole_data;
		GPlatesModel::FeatureCollectionMetadata d_fc_meta;
		typedef void (MetadataDialog::*function)();
		std::map<int, function> d_func_map;
		MetaType d_type;
		GPlatesModel::FeatureHandle::iterator d_feature_iter;
		GPlatesModel::FeatureHandle::weak_ref d_feature_ref;
		QTreeWidgetItem *d_trs_dlg_current_item, *d_contributor_item, *d_gts_item, *d_creator_item;
		GPlatesFileIO::PlatesRotationFileProxy *d_grot_proxy;
		QString d_moving_plate_id;
	};
}

#endif //METADATA_DIALOG_H



