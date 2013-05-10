/* $Id: MeshDialog.cc 12059 2011-08-09 03:30:03Z jcannon $ */
 
/**
 * \file 
 * $Revision: 12059 $
 * $Date: 2011-08-09 13:30:03 +1000 (Tue, 09 Aug 2011) $
 * 
 * Copyright (C) 2010, 2011 The University of Sydney, Australia
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
#include <boost/foreach.hpp>
#include <sstream>
#include <string>

#include <QDir>
#include <QDebug>
#include <QFont>
#include <QMessageBox>
#include <QValidator>

#include "MetadataDialog.h"
#include "property-values/GpmlMetadata.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlTotalReconstructionPole.h"
#include "model/ModelUtils.h"

class RejectAllEdit : 
	public QValidator
{
public:
	virtual
	State 
	validate ( 
			QString & input, 
			int & pos ) const
	{
		return Invalid;
	}
};

const QString GPlatesQtWidgets::MetadataTextEditor::DELETE_MARK("@%{GPLATES_ABOUT_TO_BE_DELETED@%{");

GPlatesQtWidgets::MetadataDialog::MetadataDialog(
		QWidget *parent_):
	QDialog(parent_, Qt::Window),
	d_contributor_item(NULL),
	d_grot_proxy(NULL)
{
	setupUi(this);
	setWindowModality(Qt::NonModal);
	setModal(false);

	meta_table->setHorizontalHeaderItem(0,new QTableWidgetItem("Name"));
	meta_table->setHorizontalHeaderItem(1,new QTableWidgetItem("Value"));
	meta_table->horizontalHeader()->setMinimumSectionSize(100);

	meta_table->verticalHeader()->hide();
	meta_table->resizeColumnsToContents();

	add_simple_entry_group->hide();
	
	meta_tree->header()->close();
	connect(
			meta_tree,
			SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)),
			this,
			SLOT(handle_current_item_changed(QTreeWidgetItem *, QTreeWidgetItem *)));
	connect(
			add_simple_entry_button,
			SIGNAL(clicked()),
			this,
			SLOT(handle_add_simple_entry_clicked()));
	
	connect(
			remove_button,
			SIGNAL(clicked()),
			this,
			SLOT(handle_remove_button_clicked()));

	d_func_map[CREATOR] = &MetadataDialog::show_creator;
	d_func_map[DC] = &MetadataDialog::show_dc;
	d_func_map[RIGHTS] = &MetadataDialog::show_rights;
	d_func_map[GPML_META] = &MetadataDialog::show_header_metadata;
	d_func_map[DATE] = &MetadataDialog::show_date;
	d_func_map[COVERAGE] = &MetadataDialog::show_coverage;
	d_func_map[SEQUENCE_META] = &MetadataDialog::show_mprs;
	d_func_map[MPRS_DATA] = &MetadataDialog::show_mprs_only_data;
	d_func_map[DEFAULT_POLE_DATA] = &MetadataDialog::show_default_pole_data;
	d_func_map[POLE_META] = &MetadataDialog::show_pole;
	d_func_map[CONTRIBUTORS] = &MetadataDialog::show_contributors;
	d_func_map[GEOTIMESCALE] = &MetadataDialog::show_timescales;
	d_func_map[BIBINFO] = &MetadataDialog::show_bibinfo;
	d_func_map[POLE_META_GTS] = &MetadataDialog::show_gts;
	d_func_map[POLE_META_HELL] = &MetadataDialog::show_hell;
	d_func_map[POLE_META_AU] = &MetadataDialog::show_au;

	d_add_GTS_widget = new AddGTSWidget(this);
	QHBoxLayout *layout_ = new QHBoxLayout(widget_placeholder);
	layout_->addWidget(d_add_GTS_widget);
	d_add_GTS_widget->hide();
	connect(
			d_add_GTS_widget->add_button,
			SIGNAL(clicked()),
			this,
			SLOT(handle_add_gts_clicked()));

	d_add_contr_widget = new AddContributorWidget(this);
	layout_->addWidget(d_add_contr_widget);
	d_add_contr_widget->hide();
	connect(
			d_add_contr_widget->add_contr_button,
			SIGNAL(clicked()),
			this,
			SLOT(handle_add_contributor_clicked()));

	d_add_creator_widget = new AddCreatorWidget(this);
	layout_->addWidget(d_add_creator_widget);
	d_add_creator_widget->hide();
	connect(
		d_add_creator_widget->add_button,
		SIGNAL(clicked()),
		this,
		SLOT(handle_add_creator_clicked()));
	
}


void
GPlatesQtWidgets::MetadataDialog::handle_current_item_changed(
		QTreeWidgetItem *current,
		QTreeWidgetItem *previous)
{
	if(!current || !(current->flags() & Qt::ItemIsSelectable))
	{
		meta_table->clear();
		set_meta_table_style();
		hide_all_opt_gui_widget();
		return;
	}
	
	if(d_func_map.find(current->type()) != d_func_map.end())
	{
		(this->*d_func_map[current->type()])();
	}
	return;
}


void
GPlatesQtWidgets::MetadataDialog::refresh()
{
	meta_table->clear();
	meta_tree->clear();

	switch (d_type)
	{
	case FC:
		populate_fc_meta();
		break;
	case MPRS:
		polulate_mprs();
		break;
	case POLE:
		populate_pole();
		break;
	case EMPTY:
	default:
		break;
	}
}


void
GPlatesQtWidgets::MetadataDialog::populate_fc_meta()
{
	QTreeWidgetItem *dc_item = new QTreeWidgetItem(meta_tree,DC);
	dc_item->setText(0,"Dublin Core");
	meta_tree->addTopLevelItem(dc_item);

	d_creator_item = new QTreeWidgetItem(dc_item, CREATOR);
	d_creator_item->setText(0,"Creators");

	BOOST_FOREACH(GPlatesModel::DublinCoreMetadata::Creator &creator, d_fc_meta.get_dc_data().creators)
	{
		(new QTreeWidgetItem(d_creator_item, CREATOR))->setText(0,creator.name);
	}

	d_contributor_item = new QTreeWidgetItem(dc_item, CONTRIBUTORS);
	d_contributor_item->setText(0,"Contributors");
	//d_contributor_item->setFlags(Qt::ItemIsEnabled);

	BOOST_FOREACH(GPlatesModel::DublinCoreMetadata::Contributor& contr,  d_fc_meta.get_dc_data().contributors)
	{
		(new QTreeWidgetItem(d_contributor_item, CONTRIBUTORS))->setText(0,contr.id);
	}

	QTreeWidgetItem *dc_rights = new QTreeWidgetItem(dc_item, RIGHTS);
	dc_rights->setText(0,"Rights");

	QTreeWidgetItem *dc_date = new QTreeWidgetItem(dc_item, DATE);
	dc_date->setText(0,"Date");

	QTreeWidgetItem *dc_coverage = new QTreeWidgetItem(dc_item, COVERAGE);
	dc_coverage->setText(0,"Coverage");

	QTreeWidgetItem *gpml_meta = new QTreeWidgetItem(meta_tree,GPML_META);
	gpml_meta->setText(0,"GPML Metadata");
	meta_tree->addTopLevelItem(gpml_meta);

	QTreeWidgetItem *bibinfo = new QTreeWidgetItem(meta_tree, BIBINFO);
	bibinfo->setText(0,"BIBINFO");
	meta_tree->addTopLevelItem(bibinfo);

	d_gts_item = new QTreeWidgetItem(meta_tree, GEOTIMESCALE);
	d_gts_item->setText(0,"GEOTIMESCALEs");
	meta_tree->addTopLevelItem(d_gts_item);
	//d_gts_item->setFlags(Qt::ItemIsEnabled);

	BOOST_FOREACH(GPlatesModel::GeoTimeScale& scale, d_fc_meta.get_geo_time_scales())
	{
		(new QTreeWidgetItem(d_gts_item, GEOTIMESCALE))->setText(0,scale.id);
	}

	meta_tree->expandAll();
	//dc_timescale->setExpanded(false);
	//dc_contr->setExpanded(false);
	dc_item->setSelected(true);
	handle_current_item_changed(dc_item,NULL);
}


void
GPlatesQtWidgets::MetadataDialog::polulate_mprs()
{
	QTreeWidgetItem *item = new QTreeWidgetItem(meta_tree, SEQUENCE_META);
	if(d_trs_dlg_current_item)
	{
		item->setText(0,d_trs_dlg_current_item->text(0));
	}
	else
	{
		item->setText(0,"MPRS Metadata");
	}
	meta_tree->addTopLevelItem(item);
	
	QTreeWidgetItem *mprs_item = new QTreeWidgetItem(item, MPRS_DATA);
	mprs_item->setText(0,"MPRS Data");
	mprs_item->setToolTip(0,"Moving Plate Rotation Sequence Data");

	QTreeWidgetItem *tmp_item = new QTreeWidgetItem(item, DEFAULT_POLE_DATA);
	tmp_item->setText(0,"Default Pole Data");

	meta_tree->expandAll();

	item->setFlags(Qt::ItemIsEnabled);
	//item->setSelected(true);
	
	mprs_item->setSelected(true);
	handle_current_item_changed(mprs_item,NULL);
}


void
GPlatesQtWidgets::MetadataDialog::populate_pole()
{
	meta_tree->clear();
	QTreeWidgetItem *item = new QTreeWidgetItem(meta_tree,POLE_META);
	if(d_trs_dlg_current_item)
	{
		QString pole_str = QString("%1  %2  %3  %4")
			.arg(d_trs_dlg_current_item->text(1))
			.arg(d_trs_dlg_current_item->text(2))
			.arg(d_trs_dlg_current_item->text(3))
			.arg(d_trs_dlg_current_item->text(4));
		item->setText(0,pole_str);
	}
	else
	{
		item->setText(0,"Pole Metadata");
	}
	meta_tree->addTopLevelItem(item);
	//item->setFlags(Qt::ItemIsEnabled);

	GPlatesModel::MetadataContainer 
		pole_data = get_pole_metadata(d_mprs_data, d_pole_data),
		gst_data = find_all("GTS", pole_data);
	QTreeWidgetItem *tmp = NULL;
	
	BOOST_FOREACH(GPlatesModel::MetadataContainer::value_type v, gst_data)
	{
		if(!v->get_content().simplified().isEmpty())
		{
			if(!tmp)
			{
				tmp = new QTreeWidgetItem(item, POLE_META_GTS);
				tmp->setText(0,"GTS");
				tmp->setToolTip(0,"Geological Time Scale");
				tmp->setFlags(Qt::ItemIsEnabled);
			}
			(new QTreeWidgetItem(tmp, POLE_META_GTS))->setText(0,v->get_content());
		}
	}

	bool show_hell_flag = false;
	BOOST_FOREACH(const GPlatesModel::MetadataContainer::value_type val, pole_data)
	{
		if(val->get_name().startsWith("HELL") && !val->get_content().isEmpty())
		{
			show_hell_flag = true;
		}
	}
	if(show_hell_flag)
	{
		tmp = new QTreeWidgetItem(item, POLE_META_HELL);
		tmp->setText(0, "HELL");
		tmp->setToolTip(0, "Uncertainty Parameters");
	}

	tmp = NULL;
	GPlatesModel::MetadataContainer au_data = find_all("AU",pole_data);
	BOOST_FOREACH(GPlatesModel::MetadataContainer::value_type val, au_data)
	{
		if(!val->get_content().simplified().isEmpty())
		{
			if(!tmp)
			{
				tmp = new QTreeWidgetItem(item, POLE_META_AU);
				tmp->setText(0,"Authors");
				tmp->setToolTip(0,"Author Information");
				tmp->setFlags(Qt::ItemIsEnabled);
			}
			(new QTreeWidgetItem(tmp, POLE_META_AU))->setText(0,val->get_content());
		}
	}

	meta_tree->expandAll();
	meta_tree->setCurrentItem(item);

	//item->setSelected(true);
	//handle_current_item_changed(item,NULL);
}


void
GPlatesQtWidgets::MetadataDialog::show_creator()
{
	using namespace GPlatesModel;
	meta_table->clear();
	hide_all_opt_gui_widget();
	std::vector<DublinCoreMetadata::Creator> &creators = d_fc_meta.get_dc_data().creators;
	meta_table->setColumnCount(2);

	QString selected_name = meta_tree->currentItem()->text(0).simplified();

	if(selected_name == "Creators")
	{
		meta_table->setRowCount(1);
		meta_table->setItem(0,0,new QTableWidgetItem("The number of creators"));
		meta_table->setCellWidget(0,1,new MetadataTextEditor(QString().setNum(creators.size()),this,false,true));
		d_add_creator_widget->show();
	}
	else
	{
		BOOST_FOREACH(DublinCoreMetadata::Creator& creator, creators)
		{
			if(creator.name == selected_name)
			{
				meta_table->setRowCount(4);
				meta_table->setItem(0,0,new QTableWidgetItem("Name"));
				meta_table->setCellWidget(0,1,new MetadataTextEditor(creator.name,this,false,true));
				meta_table->setItem(1,0,new QTableWidgetItem("E-mail"));
				meta_table->setCellWidget(1,1,new MetadataTextEditor(creator.email,this));
				meta_table->setItem(2,0,new QTableWidgetItem("URL"));
				meta_table->setCellWidget(2,1,new MetadataTextEditor(creator.url,this));
				meta_table->setItem(3,0,new QTableWidgetItem("Affiliation"));
				meta_table->setCellWidget(3,1,new MetadataTextEditor(creator.affiliation,this));
				break;
			}
		}
		remove_button->show();
	}
	set_meta_table_style();
}


void
GPlatesQtWidgets::MetadataDialog::show_dc()
{
	meta_table->clear();
	
	meta_table->setRowCount(4);
	meta_table->setColumnCount(2);
	
	meta_table->setItem(0,0,new QTableWidgetItem("Title"));
	meta_table->setCellWidget(0,1,new MetadataTextEditor(d_fc_meta.get_dc_data().title,this));
	
	meta_table->setItem(1,0,new QTableWidgetItem("Namespace"));
	meta_table->setCellWidget(1,1,new MetadataTextEditor(d_fc_meta.get_dc_data().dc_namespace,this));
	
	meta_table->setItem(2,0,new QTableWidgetItem("Bibliographic Citation"));
	meta_table->setCellWidget(2,1,new MetadataTextEditor(d_fc_meta.get_dc_data().bibliographicCitation,this));
	
	meta_table->setItem(3,0,new QTableWidgetItem("Description"));
	meta_table->setCellWidget(3,1,new MetadataTextEditor(d_fc_meta.get_dc_data().description,this));
	
	set_meta_table_style();
	hide_all_opt_gui_widget();
}


void
GPlatesQtWidgets::MetadataDialog::show_contributors()
{
	using namespace GPlatesModel;

	meta_table->clear();
	hide_all_opt_gui_widget();
	std::vector<DublinCoreMetadata::Contributor>& contributors = d_fc_meta.get_dc_data().contributors;
	meta_table->setColumnCount(2);

	QString selected_id = meta_tree->currentItem()->text(0).simplified();

	if(selected_id == "Contributors")
	{
		meta_table->setRowCount(1);
		meta_table->setItem(0,0,new QTableWidgetItem("The number of contributors"));
		meta_table->setCellWidget(0,1,new MetadataTextEditor(QString().setNum(contributors.size()),this,false,true));
		d_add_contr_widget->show();
	}
	else
	{
		BOOST_FOREACH(DublinCoreMetadata::Contributor &contr, contributors)
		{
			if(contr.id == selected_id)
			{
				show_contributor(contr);
				break;
			}
		}
		remove_button->show();
	}
	set_meta_table_style();
}


void
GPlatesQtWidgets::MetadataDialog::show_rights()
{
	meta_table->clear();

	meta_table->setRowCount(2);
	meta_table->setColumnCount(2);
	
	meta_table->setItem(0,0,new QTableWidgetItem("License"));
	meta_table->setCellWidget(0,1,new MetadataTextEditor(d_fc_meta.get_dc_data().rights.license,this));
	
	meta_table->setItem(1,0,new QTableWidgetItem("URL"));
	meta_table->setCellWidget(1,1,new MetadataTextEditor(d_fc_meta.get_dc_data().rights.url,this));
	set_meta_table_style();
	hide_all_opt_gui_widget();
}


void
GPlatesQtWidgets::MetadataDialog::show_date()
{
	meta_table->clear();

	meta_table->setRowCount(d_fc_meta.get_dc_data().date.modified.size()+1);
	meta_table->setColumnCount(2);
	
	meta_table->setItem(0,0,new QTableWidgetItem("Created Date"));
	meta_table->setCellWidget(0,1,new MetadataTextEditor(d_fc_meta.get_dc_data().date.created,this));
	
	int count=1;
	typedef boost::shared_ptr<QString> qstring_shared_ptr;
	BOOST_FOREACH(qstring_shared_ptr date_m, d_fc_meta.get_dc_data().date.modified)
	{
		meta_table->setItem(count,0,new QTableWidgetItem("Modified Date"));
		meta_table->setCellWidget(count,1,new MetadataTextEditor(*date_m,this,true));
		count++;
	}
	set_meta_table_style();
	hide_all_opt_gui_widget();

	//set up the add new entry combobox.
	meta_name_combobox->clear();
	meta_name_combobox->addItem("Modified Date");
	add_simple_entry_group->show();
}


void
GPlatesQtWidgets::MetadataDialog::show_coverage()
{
	meta_table->clear();

	meta_table->setRowCount(1);
	meta_table->setColumnCount(2);
	meta_table->setItem(0,0,new QTableWidgetItem("Temporal"));
	meta_table->setCellWidget(
			0,1,new MetadataTextEditor(
					d_fc_meta.get_dc_data().coverage.temporal,
					this));
	set_meta_table_style();
	hide_all_opt_gui_widget();
}


void
GPlatesQtWidgets::MetadataDialog::show_mprs()
{
	using namespace GPlatesModel;
	meta_table->clear();
	
	std::vector<Metadata::shared_ptr_type> data_ = d_mprs_data;

	meta_table->setColumnCount(2);
	meta_table->setRowCount(data_.size());

	for(std::size_t i=0; i<data_.size(); i++)
	{
		meta_table->setItem(i,0,new QTableWidgetItem(data_[i]->get_name()));
		meta_table->setCellWidget(i,1,new MetadataTextEditor(data_[i]->get_content(),this,true));
	}

	//set up the add new entry combobox.
	using namespace GPlatesFileIO;
	RotationMetadataRegistry::MetadataAttrMap the_map = 
		PlatesRotationFileProxy::get_metadata_registry().get(MetadataType::POLE);
	meta_name_combobox->clear();
	BOOST_FOREACH(RotationMetadataRegistry::MetadataAttrMap::value_type v, the_map)
	{
		if(v.first.startsWith("HELL"))
		{
			continue;
		}
		if(v.second.type_flag & MetadataType::MULTI_OCCUR)
		{
			meta_name_combobox->addItem(v.first);
		}
		else if(!(v.second.type_flag & MetadataType::MANDATORY))
		{
			Metadata::shared_ptr_type tmp(new Metadata(v.first,""));
			if((std::find_if(
						d_mprs_data.begin(), 
						d_mprs_data.end(), 
						std::bind1st(std::ptr_fun(&is_same_meta), tmp)) == d_mprs_data.end()))
			{
				meta_name_combobox->addItem(v.first);
			}
		}
	}

	set_meta_table_style();
	hide_all_opt_gui_widget();
	add_simple_entry_group->show();
}


void
GPlatesQtWidgets::MetadataDialog::show_pole()
{
	using namespace GPlatesModel;
	using namespace GPlatesFileIO;

	meta_table->clear();
	meta_table->setColumnCount(2);

	std::vector<Metadata::shared_ptr_type> 
		pole_data = get_pole_metadata(d_mprs_data,d_pole_data);
	int num_of_mprs = pole_data.size() - d_pole_data.size();
	
	/* //This maybe not a good idea. Comment it out for now.
	//if pole data doesn't exist, add an empty one.
	BOOST_FOREACH(RotationMetadataRegistry::MetadataAttrMap::value_type v, 
		PlatesRotationFileProxy::get_metadata_registry().get(MetadataType::POLE + MetadataType::MANDATORY))
	{
		Metadata::shared_ptr_type tmp(new Metadata(v.first,""));
		if((std::find_if(
					pole_data.begin(), 
					pole_data.end(), 
					std::bind1st(std::ptr_fun(&is_same_meta), tmp)) == pole_data.end()) )
		{
			pole_data.push_back(tmp);
			d_pole_data.push_back(tmp); //keep the metadata object alive.
		}
	}
	*/
	meta_table->setRowCount(pole_data.size());
	//add mprs entries with names in bold font.
	for(int i=0; i<num_of_mprs; i++)
	{
		QTableWidgetItem *item = new QTableWidgetItem(pole_data[i]->get_name());
		QFont f; f.setBold(true); f.setItalic(true);
		item->setFont(f);
		meta_table->setItem(i,0,item);
		meta_table->setCellWidget(i,1,new MetadataTextEditor(
				pole_data[i]->get_content(),
				this,
				false,
				true));
	}
	//add individual pole data entry.
	for(std::size_t i=num_of_mprs; i<pole_data.size(); i++)
	{
		QString &name = pole_data[i]->get_name(), &content = pole_data[i]->get_content();
		meta_table->setItem(i,0,new QTableWidgetItem(name));
		MetadataAttribute meta_attr = PlatesRotationFileProxy::get_metadata_registry().get(name);
		if(meta_attr.type_flag & MetadataType::REFERENCE)
		{
			meta_table->setCellWidget(i,1,new MetadataTextEditor(content,this,true,true)); //readonly
		}
		else
		{
			meta_table->setCellWidget(i,1,new MetadataTextEditor(content,this,true));
		}
	}

	//setup "add new metadata entry" gui.
	refresh_add_new_entry_combobox();
	add_simple_entry_group->show();

	set_meta_table_style();
	hide_all_opt_gui_widget();
	add_simple_entry_group->show();
}


void
GPlatesQtWidgets::MetadataDialog::refresh_add_new_entry_combobox()
{
	QTreeWidgetItem *current = meta_tree->currentItem();
	if(current)
	{
		using namespace GPlatesFileIO;
		RotationMetadataRegistry::MetadataAttrMap the_pole_attr_map = 
			PlatesRotationFileProxy::get_metadata_registry().get(MetadataType::POLE);
		int current_type = current->type();
		if(current_type == POLE_META)
		{
			meta_name_combobox->clear();
			BOOST_FOREACH(RotationMetadataRegistry::MetadataAttrMap::value_type v, the_pole_attr_map)
			{
				if((v.second.type_flag & MetadataType::MULTI_OCCUR) ||
					find_first_of(v.first,d_pole_data) == d_pole_data.end())
				{
					meta_name_combobox->addItem(v.first);
				}
			}
		}
		else if(current_type == DEFAULT_POLE_DATA)
		{
			meta_name_combobox->clear();
			BOOST_FOREACH(RotationMetadataRegistry::MetadataAttrMap::value_type v, the_pole_attr_map)
			{
				if(v.first.startsWith("HELL"))
				{
					continue;
				}
				if(v.second.type_flag & MetadataType::MULTI_OCCUR)
				{
					meta_name_combobox->addItem(v.first);
				}
				else 
				{
					using namespace GPlatesModel;
					Metadata::shared_ptr_type tmp(new Metadata(v.first,""));
					if((std::find_if(
							d_mprs_data.begin(), 
							d_mprs_data.end(), 
							std::bind1st(std::ptr_fun(&is_same_meta), tmp)) == d_mprs_data.end()))
					{
						meta_name_combobox->addItem(v.first);
					}
				}
			}
		}
	}
}


void
GPlatesQtWidgets::MetadataDialog::show_header_metadata()
{
	meta_table->clear();
	meta_table->setRowCount(d_fc_meta.get_header_metadata().REVISIONHIST.size() + 3);
	meta_table->setColumnCount(2);

	meta_table->setItem(0,0,new QTableWidgetItem("GPlates Rotation File Version"));
	meta_table->setCellWidget(0,1,new MetadataTextEditor(
			d_fc_meta.get_header_metadata().GPLATESROTATIONFILE_version,
			this));

	meta_table->setItem(1,0,new QTableWidgetItem("GPlates Rotation File Documentation"));
	meta_table->setCellWidget(1,1,new MetadataTextEditor(
			d_fc_meta.get_header_metadata().GPLATESROTATIONFILE_documentation,
			this));

	meta_table->setItem(2,0,new QTableWidgetItem("GPML Namespace"));
	meta_table->setCellWidget(2,1,new MetadataTextEditor(
			d_fc_meta.get_header_metadata().GPML_namespace,
			this));

	int count=3;
	typedef boost::shared_ptr<QString> qstring_shared_ptr;
	BOOST_FOREACH(qstring_shared_ptr his, d_fc_meta.get_header_metadata().REVISIONHIST)
	{
		meta_table->setItem(count,0,new QTableWidgetItem("Revision History"));
		meta_table->setCellWidget(count,1,new MetadataTextEditor(*his,this,true));
		count++;
	}
	set_meta_table_style();
	hide_all_opt_gui_widget();

	//set up the add new entry combobox.
	meta_name_combobox->clear();
	meta_name_combobox->addItem("Revision History");
	add_simple_entry_group->show();
}


void
GPlatesQtWidgets::MetadataDialog::show_timescales()
{
	meta_table->clear();
	hide_all_opt_gui_widget();
	std::vector<GPlatesModel::GeoTimeScale>& timescales = 
		d_fc_meta.get_geo_time_scales();
	meta_table->setColumnCount(2);
	QString selected_id = meta_tree->currentItem()->text(0).simplified();

	if(selected_id.toLower() == "geotimescales")
	{
		meta_table->setRowCount(1);
		meta_table->setItem(0,0,new QTableWidgetItem("The number of geo-timescales"));
		meta_table->setCellWidget(0,1,new MetadataTextEditor(
				QString().setNum(timescales.size()),this,false,true));
		d_add_GTS_widget->show();
	}
	else
	{
		BOOST_FOREACH(GPlatesModel::GeoTimeScale& scale, timescales)
		{
			if(scale.id == selected_id)
			{
				show_gts(scale);
			}
		}
		remove_button->show();
	}
	set_meta_table_style();
}


void
GPlatesQtWidgets::MetadataDialog::show_bibinfo()
{
	meta_table->clear();

	meta_table->setRowCount(2);
	meta_table->setColumnCount(2);
	
	meta_table->setItem(0,0,new QTableWidgetItem("Bibliography File"));
	meta_table->setCellWidget(0,1,new MetadataTextEditor(d_fc_meta.get_bibinfo().bibfile,this));
	
	meta_table->setItem(1,0,new QTableWidgetItem("DOI base"));
	meta_table->setCellWidget(1,1,new MetadataTextEditor(d_fc_meta.get_bibinfo().doibase,this));
	set_meta_table_style();
	hide_all_opt_gui_widget();
}


void
GPlatesQtWidgets::MetadataDialog::set_meta_table_style()
{
	meta_table->setHorizontalHeaderItem(0,new QTableWidgetItem("Name"));
	meta_table->setHorizontalHeaderItem(1,new QTableWidgetItem("Value"));
	meta_table->horizontalHeader()->setMinimumSectionSize(100);

	meta_table->resizeColumnsToContents();
	meta_table->resizeRowsToContents();
	meta_table->setShowGrid(false);
	meta_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	meta_table->setSelectionMode(QAbstractItemView::NoSelection);
	meta_table->setFocusPolicy(Qt::NoFocus);
	
}

namespace
{
	using namespace GPlatesModel;
	using namespace GPlatesPropertyValues;

	std::vector<boost::shared_ptr<Metadata> >
	convert_mprs_metadata_to_vector(
			GpmlKeyValueDictionary::non_null_ptr_type dict)
	{
		std::vector<boost::shared_ptr<Metadata> > ret;
		BOOST_FOREACH(const GpmlKeyValueDictionaryElement& ele, dict->elements())
		{
			const XsString* val = 
				dynamic_cast<const XsString*>(ele.value().get());
			if(val)
			{
				QString name = ele.key()->value().get().qstring();
				ret.push_back(boost::shared_ptr<Metadata>(
						new PoleMetadata(
								name, 
								val->value().get().qstring())));
			}
		}
		return ret;
	}
}


void
GPlatesQtWidgets::MetadataDialog::set_data(
		GPlatesModel::FeatureHandle::iterator iter)
{
	d_type = FC;
	boost::optional<GPlatesModel::PropertyValue::non_null_ptr_to_const_type> value = 
		GPlatesModel::ModelUtils::get_property_value(**iter);
	if(value)
	{
		const GPlatesPropertyValues::GpmlMetadata* gpml_metadata =
			dynamic_cast<const GPlatesPropertyValues::GpmlMetadata*>((*value).get());
		if(gpml_metadata)
		{
			set_data(gpml_metadata->get_data());
		}
	}
	d_feature_iter = iter;
	refresh();
}

void
GPlatesQtWidgets::MetadataDialog::set_data(
		GPlatesModel::FeatureHandle::iterator iter,
		QTreeWidgetItem *current_item)
{
	using namespace GPlatesPropertyValues;
	using namespace  GPlatesModel;

	d_type =MPRS;
	d_trs_dlg_current_item = current_item;
	QString tmp = current_item->text(0);
	d_moving_plate_id = tmp.left(tmp.indexOf(" "));
	const TopLevelPropertyInline *p_inline = 
		dynamic_cast<const TopLevelPropertyInline*>((*iter).get());
	if(p_inline && p_inline->size() >= 1)
	{
		const GpmlKeyValueDictionary* const_dictionary = 
			dynamic_cast<const GpmlKeyValueDictionary*>((*p_inline->begin()).get());
		if(const_dictionary)
		{
			GpmlKeyValueDictionary* dictionary = 
				const_cast<GpmlKeyValueDictionary*>(const_dictionary);
			std::vector<boost::shared_ptr<Metadata> > data_ =
				convert_mprs_metadata_to_vector(
						GpmlKeyValueDictionary::non_null_ptr_type(dictionary));
			d_mprs_data = data_;
			refresh();
		}
	}
	d_feature_iter = iter;
}


void
GPlatesQtWidgets::MetadataDialog::set_data(
		GPlatesModel::FeatureHandle::weak_ref feature_ref,
		QTreeWidgetItem *item)
{
	using namespace GPlatesModel;
	using namespace GPlatesPropertyValues;
	d_type = POLE;
	d_trs_dlg_current_item = item;
	d_feature_ref = feature_ref;
	d_pole_data.clear();
	d_mprs_data.clear();

	static const  PropertyName mprs_attrs = 
		PropertyName::create_gpml(QString("mprsAttributes"));

	FeatureHandle::iterator it = feature_ref->begin();
	for(;it != feature_ref->end(); it++)
	{
		if((*it)->property_name() == mprs_attrs)
		{
			const TopLevelPropertyInline *p_inline = 
				dynamic_cast<const TopLevelPropertyInline*>((*it).get());
			if(p_inline && p_inline->size() >= 1)
			{
				const GpmlKeyValueDictionary* const_dictionary = 
					dynamic_cast<const GpmlKeyValueDictionary*>((*p_inline->begin()).get());
				if(const_dictionary)
				{
					GpmlKeyValueDictionary* dictionary = 
						const_cast<GpmlKeyValueDictionary*>(const_dictionary);
					d_mprs_data = convert_mprs_metadata_to_vector(
							GpmlKeyValueDictionary::non_null_ptr_type(dictionary));
					break;
				}
			}
		}
	}

	std::vector<FeatureHandle::iterator> iters = ModelUtils::get_top_level_property_ref(
			PropertyName::create_gpml(QString("totalReconstructionPole")),
			d_feature_ref);
	if(iters.size()!=1)
	{
		qWarning() << "There should be always one totalReconstructionPole in the feature.";
		return;
	}
	GPlatesPropertyValues::GpmlTotalReconstructionPole *trs = 
		get_gpml_total_reconstruction_pole(*ModelUtils::get_property_value(**iters[0]));
	if(trs)
	{
		d_pole_data = trs->metadata();
	}
	refresh();
}


void
GPlatesQtWidgets::MetadataTextEditor::handle_edit_finished()
{
	if(d_dlg)
	{
		d_txt = d_editor->toPlainText();
		d_dlg->save();
	}
	d_edit_button->setEnabled(true);
	d_editor->setVisible(false);
	setup_browser();
	d_browser->setVisible(true);
	//d_dlg->refresh();
}


void
GPlatesQtWidgets::MetadataTextEditor::del_button_clicked()
{
	d_txt = DELETE_MARK;
	d_dlg->delete_row(this);
	d_dlg->save();
	d_dlg->refresh_add_new_entry_combobox();
}


void
GPlatesQtWidgets::MetadataDialog::save_fc_meta()
{
	typedef boost::shared_ptr<QString> qstring_shared_ptr;
	std::vector<qstring_shared_ptr> tmp;
	BOOST_FOREACH(qstring_shared_ptr str, d_fc_meta.get_header_metadata().REVISIONHIST)
	{
		if(*str != MetadataTextEditor::DELETE_MARK)
		{
			tmp.push_back(str);
		}
	}
	d_fc_meta.get_header_metadata().REVISIONHIST = tmp;
	tmp.clear();
	BOOST_FOREACH(qstring_shared_ptr str, d_fc_meta.get_dc_data().date.modified)
	{
		if(*str != MetadataTextEditor::DELETE_MARK)
		{
			tmp.push_back(str);
		}
	}
	d_fc_meta.get_dc_data().date.modified = tmp;
	*d_feature_iter = GPlatesModel::TopLevelPropertyInline::create(
		GPlatesModel::PropertyName::create_gpml("metadata"),
		GPlatesPropertyValues::GpmlMetadata::create(d_fc_meta));

	d_grot_proxy->update_header_metadata(d_fc_meta);
}


void
GPlatesQtWidgets::MetadataDialog::save_mprs_meta()
{
	using namespace GPlatesModel;
	using namespace GPlatesPropertyValues;

	GpmlKeyValueDictionary::non_null_ptr_type dictionary = 
		GpmlKeyValueDictionary::create();

	std::vector<Metadata::shared_ptr_type> tmp;
	BOOST_FOREACH(Metadata::shared_ptr_type d, d_mprs_data)
	{
		if(d->get_content() != MetadataTextEditor::DELETE_MARK)
		{
			tmp.push_back(d);
		}
	}
	d_mprs_data = tmp;
	BOOST_FOREACH(Metadata::shared_ptr_type data_, d_mprs_data)
	{
		XsString::non_null_ptr_type 
			key = XsString::create(
					GPlatesUtils::make_icu_string_from_qstring(
							data_->get_name())),
		 val = XsString::create(
				GPlatesUtils::make_icu_string_from_qstring(
						data_->get_content()));

		GpmlKeyValueDictionaryElement new_element(
				key, 
				val,
				StructuralType::create_xsi("string"));
		dictionary->elements().push_back(new_element);
	}
	if(dictionary->num_elements() > 0)
	{
		*d_feature_iter = 
			TopLevelPropertyInline::create(
					PropertyName::create_gpml("mprsAttributes"),
					dictionary);
	}
	d_grot_proxy->update_MPRS_metadata(
			get_mprs_only_data(),
			get_default_pole_data(),
			d_moving_plate_id);
}


void
GPlatesQtWidgets::MetadataDialog::save_pole_meta()
{
	using namespace GPlatesModel;
	std::vector<FeatureHandle::iterator> iters = ModelUtils::get_top_level_property_ref(
			PropertyName::create_gpml(QString("totalReconstructionPole")),
			d_feature_ref);
	if(iters.size()!=1)
	{
		qWarning() << "Unable to retrieve totalReconstructionPole property from the feature.";
		return;
	}
	TopLevelProperty::non_null_ptr_type trp_copy = (*iters[0])->deep_clone();
	
	GPlatesPropertyValues::GpmlTotalReconstructionPole *gpml_trp = 
		get_gpml_total_reconstruction_pole(*ModelUtils::get_property_value(*trp_copy));
	if(!gpml_trp)
	{
		qWarning() << "There is no metadata associated with this pole.";
		return;
	}
	MetadataContainer not_empty_data;
	BOOST_FOREACH(const MetadataContainer::value_type &v, d_pole_data)
	{
		if(v->get_content() != MetadataTextEditor::DELETE_MARK)
		{
			not_empty_data.push_back(v);
		}
	}
	//update model
	gpml_trp->metadata() = not_empty_data;
	
	//update grot proxy for grot file.
	if(d_grot_proxy)
	{
		GPlatesFileIO::RotationPoleData pole_data;
		QStringList tmp = d_trs_dlg_current_item->parent()->text(0).split(QRegExp("\\s+"));
		pole_data.moving_plate_id = tmp[0].toInt();
		pole_data.fix_plate_id = tmp[2].toInt();
		pole_data.time = d_trs_dlg_current_item->text(1).toDouble();
		pole_data.lat = d_trs_dlg_current_item->text(2).toDouble();//"indet" will return 0.0.
		pole_data.lon = d_trs_dlg_current_item->text(3).toDouble();
		pole_data.angle = d_trs_dlg_current_item->text(4).toDouble();
		d_grot_proxy->update_pole_metadata(not_empty_data, pole_data);
	}
	d_pole_data = not_empty_data;
	(*iters[0]) = trp_copy; //notify model the change.
}


GPlatesPropertyValues::GpmlTotalReconstructionPole*
GPlatesQtWidgets::MetadataDialog::get_gpml_total_reconstruction_pole(
		GPlatesModel::PropertyValue::non_null_ptr_to_const_type val)
{
	using namespace GPlatesPropertyValues;
	const GpmlIrregularSampling *irreg_sampling_const = dynamic_cast<const GpmlIrregularSampling *>(val.get());
	if(!irreg_sampling_const)
	{
		return NULL;
	}
	GpmlIrregularSampling *irreg_sampling = const_cast<GpmlIrregularSampling *>(irreg_sampling_const);
	QString time = d_trs_dlg_current_item->text(1), 
			lat = d_trs_dlg_current_item->text(2), 
			lon = d_trs_dlg_current_item->text(3), 
			angle=d_trs_dlg_current_item->text(4);

	std::vector<GpmlTimeSample>::iterator
		iter = irreg_sampling->time_samples().begin(),
		end = irreg_sampling->time_samples().end();
	
	static const double EPSILON = 1.0e-6; // I have to use a less tight precision because of qt.
	GpmlTotalReconstructionPole *trs = NULL;
	for ( ; iter != end; ++iter) 
	{
		if(std::fabs(iter->valid_time()->time_position().value() - time.toDouble()) < EPSILON)
		{
			trs = dynamic_cast<GpmlTotalReconstructionPole *>(iter->value().get());
			if(!trs)
			{
				qWarning() << "The time sample is not GpmlTotalReconstructionPole type.";
				return NULL;
			}
			GPlatesFileIO::RotationPoleData pole_data(trs->finite_rotation(), 0, 0, time.toDouble());
			if((std::fabs(lat.toDouble() - pole_data.lat) < EPSILON)  &&
				(std::fabs(lon.toDouble() - pole_data.lon) < EPSILON) &&
				(std::fabs(angle.toDouble() - pole_data.angle) < EPSILON))
			{
				break;
			}
		}
	}
	if(iter==end)
	{
		return NULL;
	}
	return trs;
}


void
GPlatesQtWidgets::MetadataDialog::delete_row(
		MetadataTextEditor* editor)
{
	int row_count = meta_table->rowCount();
	for(int i=0; i<row_count; i++)
	{
		MetadataTextEditor* editor_item = dynamic_cast<MetadataTextEditor*>(meta_table->cellWidget(i,1));
		if(editor_item == editor)
		{
			meta_table->removeRow(i);
			return;
		}
	}
}


void
GPlatesQtWidgets::MetadataDialog::handle_add_simple_entry_clicked()
{
	using namespace GPlatesModel;
	Metadata::shared_ptr_type md(
				new Metadata(
						meta_name_combobox->currentText(),
						value_editor->toPlainText()));
	QTreeWidgetItem *current = meta_tree->currentItem();
	if(current)
	{
		int current_type = current->type();
		switch(current_type)
		{
		case DEFAULT_POLE_DATA:
			d_mprs_data.push_back(md);
			break;
		case MPRS_DATA:
			d_mprs_data.insert(default_pole_data_begin(),md);
			break;
		case POLE_META:
			d_pole_data.push_back(md);
			break;
		case GPML_META:
			d_fc_meta.get_header_metadata().REVISIONHIST.push_back(
					boost::shared_ptr<QString>(new QString(md->get_content())));
			break;
		case DATE:
			d_fc_meta.get_dc_data().date.modified.push_back(
					boost::shared_ptr<QString>(new QString(md->get_content())));
			break;
		default:
			d_pole_data.push_back(md);
		}
		save();
		if(current_type == POLE_META)
		{
			populate_pole(); //refresh the tree widget.
		}
		refresh_metadata_table();
	}
	value_editor->clear();
	return;
}


void
GPlatesQtWidgets::MetadataDialog::handle_add_contributor_clicked()
{
	GPlatesModel::DublinCoreMetadata::Contributor contr;
	QString id = d_add_contr_widget->contr_id_value->text();
	if(id.isEmpty())
	{
		id = "New Contributor";
	}
	std::vector<QString> id_vec;
	BOOST_FOREACH(const GPlatesModel::DublinCoreMetadata::Contributor &c, 
		d_fc_meta.get_dc_data().contributors)
	{
		id_vec.push_back(c.id);
	}
	
	contr.id = valid_unique_name(id, id_vec);
	contr.name = d_add_contr_widget->contr_name_value->text();
	contr.url = d_add_contr_widget->contr_url_value->text();
	contr.email = d_add_contr_widget->contr_email_value->text();
	contr.address = d_add_contr_widget->contr_address_value->toPlainText();
	d_fc_meta.get_dc_data().contributors.push_back(contr);
	save();
	QTreeWidgetItem *tmp_item = new QTreeWidgetItem(d_contributor_item, CONTRIBUTORS);
	tmp_item->setText(0,contr.id);
	show_contributors();
}


void
GPlatesQtWidgets::MetadataDialog::handle_add_gts_clicked()
{
	GPlatesModel::GeoTimeScale scale;
	QString id = d_add_GTS_widget->scale_id->text();
	if(id.isEmpty())
	{
		id = "New Geographic Time Scale";
	}
	std::vector<QString> id_vec;
	BOOST_FOREACH(const GPlatesModel::GeoTimeScale &s, d_fc_meta.get_geo_time_scales())
	{
		id_vec.push_back(s.id);
	}

	scale.id = valid_unique_name(id, id_vec);
	scale.pub_id = d_add_GTS_widget->pub_id->text();
	scale.ref = d_add_GTS_widget->ref_value->text();
	scale.bib_ref = d_add_GTS_widget->bib_ref->toPlainText();
	d_fc_meta.get_geo_time_scales().push_back(scale);
	save();
	QTreeWidgetItem *tmp = new QTreeWidgetItem(d_gts_item, GEOTIMESCALE);
	tmp->setText(0,scale.id);
	show_timescales();
}


void
GPlatesQtWidgets::MetadataDialog::handle_add_creator_clicked()
{
	GPlatesModel::DublinCoreMetadata::Creator creator;
	QString name = d_add_creator_widget->name->text();
	if(name.isEmpty())
	{
		name = "New Creator";
	}
	std::vector<QString> name_vec;
	BOOST_FOREACH(
			const GPlatesModel::DublinCoreMetadata::Creator &c, 
			d_fc_meta.get_dc_data().creators)
	{
		name_vec.push_back(c.name);
	}

	creator.name = valid_unique_name(name, name_vec);
	creator.email = d_add_creator_widget->email->text();
	creator.url = d_add_creator_widget->url->text();
	creator.affiliation = d_add_creator_widget->affiliation->toPlainText();
	d_fc_meta.get_dc_data().creators.push_back(creator);
	save();
	QTreeWidgetItem *tmp = new QTreeWidgetItem(d_creator_item, CREATOR);
	tmp->setText(0,creator.name);
	show_creator();
}

void
GPlatesQtWidgets::MetadataDialog::handle_remove_button_clicked()
{
	QTreeWidgetItem *current = meta_tree->currentItem();
	if(current)
	{
		QString id_name = current->text(0);
		switch(current->type())
		{
		case CONTRIBUTORS:
			remove_contributor(id_name);
			break;
		case GEOTIMESCALE:
			remove_gts(id_name);
			break;
		case CREATOR:
			remove_creator(id_name);
			break;
		default:
			break;
		}
	}
}


QString
GPlatesQtWidgets::MetadataDialog::valid_unique_name(
		const QString &name, 
		const std::vector<QString> &name_vec)
{
	bool duplicate_id = false;
	int count = 0;
	QString tmp_name  = name;
	do{
		count++;
		duplicate_id = false;
		BOOST_FOREACH(const QString& n, name_vec)
		{
			if(n == tmp_name)
			{
				tmp_name = (name + "_%1").arg(count);
				duplicate_id = true;
				break;
			}
		}
	}while(duplicate_id);
	return tmp_name;
}


void
GPlatesQtWidgets::MetadataDialog::show_default_pole_data()
{
	using namespace GPlatesModel;
	meta_table->clear();
	MetadataContainer data_;
	
	using namespace GPlatesFileIO;
	data_.insert(data_.begin(), default_pole_data_begin(), d_mprs_data.end());

	meta_table->setColumnCount(2);
	meta_table->setRowCount(data_.size());

	for(std::size_t i=0; i<data_.size(); i++)
	{
		meta_table->setItem(i,0,new QTableWidgetItem(data_[i]->get_name()));
		meta_table->setCellWidget(i,1,new MetadataTextEditor(data_[i]->get_content(),this,true));
	}

	//set up the add new entry combobox.
	refresh_add_new_entry_combobox();

	set_meta_table_style();
	hide_all_opt_gui_widget();
	add_simple_entry_group->show();
}


void
GPlatesQtWidgets::MetadataDialog::show_mprs_only_data()
{
	using namespace GPlatesModel;
	meta_table->clear();
	MetadataContainer data_;
	
	using namespace GPlatesFileIO;
	//we always put these mandatory mprs entries at the top.
	BOOST_FOREACH(
			RotationMetadataRegistry::MetadataAttrMap::value_type v, 
			PlatesRotationFileProxy::get_metadata_registry().get(
					MetadataType::MPRS + MetadataType::MANDATORY))
	{
		MetadataContainer::iterator	iter = find_first_of(v.first, d_mprs_data);
		if(iter == d_mprs_data.end())
		{
			Metadata::shared_ptr_type m(new Metadata(v.first,""));
			data_.push_back(m);
			d_mprs_data.insert(
					d_mprs_data.begin(),
					Metadata::shared_ptr_type(new Metadata(v.first,"")));
		}
		else
		{
			data_.push_back(*iter);
		}
	}
		
	//then we process comments for mprs
	MetadataContainer comments;
	BOOST_FOREACH(MetadataContainer::value_type val, d_mprs_data)
	{
		QString name = val->get_name();
		
		if(val->get_name() == "C")
		{
			comments.push_back(val);
		}
		else if (RotationMetadataRegistry::instance().get(name).type_flag & MetadataType::POLE)
		{
			//Only comments immediately after "pid" "code" and "name" are considered comments for mprs.
			//Otherwise, the comments are default pole data.
			break;
		}
	}

	meta_table->setColumnCount(2);
	meta_table->setRowCount(data_.size()+comments.size());

	for(std::size_t i=0; i<data_.size(); i++)
	{
		meta_table->setItem(i,0,new QTableWidgetItem(data_[i]->get_name()));
		meta_table->setCellWidget(i,1,new MetadataTextEditor(data_[i]->get_content(),this,false));
	}
	for(std::size_t i=0; i<comments.size(); i++)
	{
		meta_table->setItem(i+data_.size(),0,new QTableWidgetItem(comments[i]->get_name()));
		meta_table->setCellWidget(i+data_.size(),1,new MetadataTextEditor(comments[i]->get_content(),this,true));
	}

	//set up the add new entry combobox.
	meta_name_combobox->clear();
	meta_name_combobox->addItem("C");

	set_meta_table_style();
	hide_all_opt_gui_widget();
	add_simple_entry_group->show();
}


GPlatesModel::MetadataContainer
GPlatesQtWidgets::MetadataDialog::get_pole_metadata(
		const GPlatesModel::MetadataContainer& mprs_data,
		const GPlatesModel::MetadataContainer& pole_data)
{
	MetadataContainer ret;
	using namespace GPlatesModel;
	using namespace GPlatesFileIO;
	//First, get rid of mprs pid, code, name and comments which belong only to the sequence.
	//There are two kinds of comments in mprs_data. 
	//One kind applies to the sequence and the other one is default pole comments. 
	//We assume all comments directly after mprs pid code and name are sequence comments.
	//We assume the order of metadata in mprs_data is 
	//MPRS:pid, MPRS:code, MPRS:name, Comments, default pole metadata.
	MetadataContainer tmp_mprs;
	MetadataContainer::const_iterator it = mprs_data.begin(), end = mprs_data.end();
	for(;it != end; it++)
	{
		QString name = (*it)->get_name();
		if (RotationMetadataRegistry::instance().get(name).type_flag & MetadataType::POLE)
		{
			if(name != "C")
			{
				tmp_mprs.insert(tmp_mprs.begin(), it, end);
				break;
			}
		}
	}

	//if the metadata entry exists in pole data, ignore the one in mprs.
	BOOST_FOREACH(Metadata::shared_ptr_type d, tmp_mprs)
	{
		if((std::find_if(
					pole_data.begin(), 
					pole_data.end(), 
					std::bind1st(std::ptr_fun(&is_same_meta), d)) == pole_data.end()) ||
			(RotationMetadataRegistry::instance().get(d->get_name()).type_flag & MetadataType::MULTI_OCCUR))
		{
			ret.push_back(d);
		}
	}
	ret.insert(ret.end(), pole_data.begin(),pole_data.end());
	return ret;
}


void
GPlatesQtWidgets::MetadataDialog::show_gts()
{
	QString selected_id = meta_tree->currentItem()->text(0).simplified();
	bool found = false;
	BOOST_FOREACH(GPlatesModel::GeoTimeScale& scale, d_fc_meta.get_geo_time_scales())
	{
		if(scale.id.compare(selected_id, Qt::CaseInsensitive) == 0)
		{
			show_gts(scale, true);
			found = true; 
			break;
		}
	}
	if(!found)
	{
		qWarning() << "Unable to find GTS: " << selected_id;
		GPlatesModel::GeoTimeScale gts;
		gts.id = selected_id;
		show_gts(gts, true);
	}
	set_meta_table_style();
	hide_all_opt_gui_widget();
	//add_simple_entry_group->show();
}


void
GPlatesQtWidgets::MetadataDialog::show_gts(
		GPlatesModel::GeoTimeScale &gts,
		bool readonly)
{
	meta_table->setRowCount(4);
	meta_table->setItem(0,0,new QTableWidgetItem("ID"));
	meta_table->setCellWidget(0,1,new MetadataTextEditor(gts.id, this, false, true));
	meta_table->setItem(1,0,new QTableWidgetItem("DOI/URL/ISSN"));
	meta_table->setCellWidget(1,1,new MetadataTextEditor(gts.pub_id, this, false, readonly));
	meta_table->setItem(2,0,new QTableWidgetItem("Citation"));
	meta_table->setCellWidget(2,1,new MetadataTextEditor(gts.ref, this, false, readonly));
	meta_table->setItem(3,0,new QTableWidgetItem("Bibliographic Reference"));
	meta_table->setCellWidget(3,1,new MetadataTextEditor(gts.bib_ref, this, false, readonly));
}


void
GPlatesQtWidgets::MetadataDialog::show_hell()
{
	using namespace GPlatesModel;
	
	MetadataContainer::iterator iter = find_first_of("HELL",d_pole_data);
	if((iter != d_pole_data.end()) && !(*iter)->get_content().isEmpty())
	{
		meta_table->setRowCount(1);
		meta_table->setItem(0,0,new QTableWidgetItem("HELL"));
		meta_table->setCellWidget(0,1,new MetadataTextEditor((*iter)->get_content(),this));
	}
	else
	{
		const char *tmp[] = {"HELL:r", "HELL:Ns", "HELL:dF", "HELL:kappahat", "HELL:cov"};
		std::vector<QString> hell_names(tmp, tmp+sizeof(tmp)/sizeof(char*));
		meta_table->setRowCount(hell_names.size());
		int count = 0;
		BOOST_FOREACH(const QString& name, hell_names)
		{
			Metadata::shared_ptr_type data_;
			iter = find_first_of(name,d_pole_data);
			if(iter == d_pole_data.end())
			{
				data_ = Metadata::shared_ptr_type(new Metadata(name, ""));
				d_pole_data.push_back(data_);
			}
			else
			{
				data_ = *iter;
			}
			meta_table->setItem(count,0,new QTableWidgetItem(name));
			meta_table->setCellWidget(count,1,new MetadataTextEditor(data_->get_content(),this));
			count++;
		}
	}

	set_meta_table_style();
	hide_all_opt_gui_widget();
	//add_simple_entry_group->show();
}


void
GPlatesQtWidgets::MetadataDialog::show_au()
{
	QString selected_id = meta_tree->currentItem()->text(0).simplified();
	bool found = false;
	BOOST_FOREACH(GPlatesModel::DublinCoreMetadata::Contributor& contr, d_fc_meta.get_dc_data().contributors)
	{
		if(contr.id.compare(selected_id, Qt::CaseInsensitive) == 0)
		{
			show_contributor(contr, true);
			found = true;
			break;
		}
	}
	if(!found)
	{
		qWarning() << "Unable to find contributor: " << selected_id;
		GPlatesModel::DublinCoreMetadata::Contributor c;
		c.id = selected_id;
		show_contributor(c, true);
	}
	set_meta_table_style();
	hide_all_opt_gui_widget();
}


void
GPlatesQtWidgets::MetadataDialog::show_contributor(
		GPlatesModel::DublinCoreMetadata::Contributor &contr, 
		bool readonly)
{
	meta_table->setRowCount(5);
	meta_table->setItem(0,0,new QTableWidgetItem("ID"));
	meta_table->setCellWidget(0,1,new MetadataTextEditor(contr.id,this,false,true));
	meta_table->setItem(1,0,new QTableWidgetItem("Name"));
	meta_table->setCellWidget(1,1,new MetadataTextEditor(contr.name,this,false,readonly));
	meta_table->setItem(2,0,new QTableWidgetItem("E-mail"));
	meta_table->setCellWidget(2,1,new MetadataTextEditor(contr.email,this,false,readonly));
	meta_table->setItem(3,0,new QTableWidgetItem("URL"));
	meta_table->setCellWidget(3,1,new MetadataTextEditor(contr.url,this,false,readonly));
	meta_table->setItem(4,0,new QTableWidgetItem("Address"));
	meta_table->setCellWidget(4,1,new MetadataTextEditor(contr.address,this,false,readonly));
}


GPlatesModel::MetadataContainer::iterator
GPlatesQtWidgets::MetadataDialog::default_pole_data_begin()
{
	using namespace GPlatesModel;
	using namespace GPlatesFileIO;
	MetadataContainer::iterator it = d_mprs_data.begin(), end = d_mprs_data.end(), ret = end;
	for(;it != end; it++)
	{
		QString name = (*it)->get_name();
		if (RotationMetadataRegistry::instance().get(name).type_flag & MetadataType::POLE)
		{
			if(name != "C")
			{
				ret = it;
				break;
			}
		}
	}
	return ret;
}












