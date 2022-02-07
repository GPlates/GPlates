/* $Id$ */

/**
 * \file
 * $Revision$
 * $Date$
 *
 * Copyright (C) 2015 The University of Sydney, Australia
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

#include <QStandardItemModel>
#include <QListView>
#include <QDomDocument>
#include <QFile>
#include <QFileInfo>
#include <QBrush>
#include <QColor>
#include <QDebug>

#include "gui/Completionist.h"
#include "gui/CustomCompleter.h"


namespace
{
	enum ModelColumnName
	{
		MODEL_COLUMN_COMPLETION, MODEL_COLUMN_POPUP
	};
	
	QColor
	appropriate_fg_colour(
			const QColor &bg_colour)
	{
		if (bg_colour.lightness() < 100) {
			return QColor(Qt::white);
		} else {
			return QColor(Qt::black);
		}
	}
	
	
	QDomDocument
	load_xml_completion_resource(
			const QString &resource_path)
	{
		QFileInfo info(resource_path);
		QDomDocument dom(info.baseName());	// Document name will be e.g. ICC2012
		QFile file(resource_path);
		if ( ! file.open(QIODevice::ReadOnly)) {
			return dom;
		}
		if ( ! dom.setContent(&file)) {
			// Ideally, we'd raise an error.
			file.close();
			return dom;
		}
		file.close();
		return dom;
	}
	
	
	/**
	 * Recursively transform our XML Timescale document into a QAbstractItemModel for completion purposes.
	 */
	void
	add_child_groups_to_model(
			int &row,
			int depth,
			QString indent,
			const QDomNode node,
			QAbstractItemModel *model)
	{
		if (node.isNull() || ! node.isElement()) {
			return;
		}
		// Visit the node itself first.
		QDomElement elem = node.toElement();
		if (elem.tagName() == "Group") {
			QModelIndex completion_idx = model->index(row, MODEL_COLUMN_COMPLETION);
			QModelIndex popup_idx = model->index(row, MODEL_COLUMN_POPUP);
			// Nothing we do seems to persuade QCompleter to use EditRole.
			// So instead, we make a model with two columns; one containing the raw data that QCompleter
			// searches within and inserts, the other containing purely visual data for display.
			// The primary driving force for doing it this way was to display nicely indented names
			// of timescale bands; for some insane reason QCompleter does not match with substrings,
			// only prefixes. Anyway, now that we do have display data separate from search/insert data,
			// we can do other nice things like adding extra information to the side of the thing that
			// we're completing.
			QString name = elem.attribute("name");
			model->setData(completion_idx, name, Qt::DisplayRole);
			model->setData(popup_idx, QString("%1%2").arg(indent).arg(name), Qt::DisplayRole);
			
			// Some of these timescales may have pleasing background colours assigned in the XML.
			QString html_colour = elem.attribute("colour");
			if ( ! html_colour.isNull()) {
				QColor bg_colour = QColor(html_colour);
				QColor fg_colour = appropriate_fg_colour(bg_colour);
				model->setData(popup_idx, QBrush(bg_colour), Qt::BackgroundRole);
				model->setData(popup_idx, QBrush(fg_colour), Qt::ForegroundRole);
			}

			// We've visited a Group element. Increment things that need incrementing.
			row++;                  // This is a reference, and the value will go up monotonically.
			indent.append(" ");     // This is also passed by value and reflects call depth.
		}
		
		// Then visit the children, if any.
		QDomNode child = node.firstChild();
		while ( ! child.isNull()) {
			add_child_groups_to_model(row, depth + 1, indent, child, model);
			child = child.nextSibling();
		}
	}
	
	
	/**
	 * Creates a new AbstractItemModel corresponding to the given QDomDocument presuming it is a GPlatesTimescale document.
	 * You are responsible for parenting the pointer properly.
	 * Returns NULL if it feels like it, which shouldn't happen in our compiled-in resource setup but could happen later
	 * down the track when we're loading these more dynamically, perhaps nearer the GPlatesModel layer.
	 */
	QAbstractItemModel *
	create_model_from_timescale_xml(
			const QDomDocument &dom)
	{
		// Basically, we expect a GPlatesTimescale document element, with a bunch of nested Group entries.
		// Those Group elements have a name attribute, amongst other things.
		QDomElement elem_GPlatesTimescale = dom.documentElement();
		if (elem_GPlatesTimescale.tagName() != "GPlatesTimescale") {
			return NULL;
		}
		
		int rows = dom.elementsByTagName("Group").count();
		QStandardItemModel *model = new QStandardItemModel(rows, 2, NULL);
		
		// Populate the model recursively, indenting the items according to their depth in the tree.
		int i = 0;
		int depth = 0;
		QString indent = "";
		add_child_groups_to_model(i, depth, indent, elem_GPlatesTimescale, model);

		return model;
	}
	
}



void
GPlatesGui::Completionist::install_completer(
		QLineEdit &widget)
{
	// We first need to construct our model and completer, and glue them together.
	//
	// For now, we are hard-coding the ICC2012 dictionary - we need a better way to specify the desired
	// dictionary when calling this method, and perhaps GPlates could maintain those separately 
	// somewhere down in the Model layer, or thereabouts.
	//
	// It should be further noted that in an ideal world, we might want to switch the Completer for the
	// EditAgeWidget with different ones depending on the timescale selected, if any. It should certainly
	// default to whatever timescale is the "GPlates Standard", but it might be nice to be able to complete
	// names from user-selected timescales too, provided GPlates is aware of them.
	QAbstractItemModel *model = get_model_for_dictionary(":gpgim/timescales/ICC2012.xml");
	CustomCompleter *completer = new CustomCompleter(&widget);
	completer->setModel(model);
	
	// Setting the completion mode to PopupCompletion may work better for e.g. lists of mineral names,
	// but specifically for Timescale completion, I think it works better to use UnfilteredPopupCompletion.
	completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
	completer->setCaseSensitivity(Qt::CaseInsensitive);
	completer->setMaxVisibleItems(12);
	completer->setCompletionColumn(MODEL_COLUMN_COMPLETION);
	
	// I give up on trying to persuade it to use the correct Role for completion; We'll use a custom QTreeView
	// and some smoke and mirrors to make it work the way we want.
	completer->set_custom_popup();
	
	// Finally we can set the completer on the given widget.
	widget.setCompleter(completer);
}


QAbstractItemModel *
GPlatesGui::Completionist::get_model_for_dictionary(
		const QString &name)
{
	// Did we already generate it? Then just return it.
	if (d_models.contains(name)) {
		return d_models.value(name).data();
	}

	// Otherwise, we need to generate our Model based on some XML file or something like that, then store it here.
	QDomDocument dom = load_xml_completion_resource(name);
	QAbstractItemModel *model = create_model_from_timescale_xml(dom);
	
	// We may need to return NULL if no such dictionary exists (and therefore no model can be built)
	if (model == NULL) {
		return NULL;
	}

	// Save it for next time.
	d_models.insert(name, QSharedPointer<QAbstractItemModel>(model));
	return model;
}

