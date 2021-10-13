/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
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

#ifndef GPLATES_GUI_CONFIGMODEL_H
#define GPLATES_GUI_CONFIGMODEL_H

#include <QAbstractTableModel>
#include <QVariant>
#include <QPointer>
#include <QString>
#include <QList>
#include <Qt>
#include <boost/noncopyable.hpp>



namespace GPlatesUtils
{
	class ConfigInterface;
}


namespace GPlatesGui
{
	/**
	 * A Qt Model class to adapt the interface of UserPreferences/ConfigBundle to a
	 * Qt TableView.
	 * See http://doc.trolltech.com/4.5/model-view-model-subclassing.html for a primer.
	 */
	class ConfigModel :
			public QAbstractTableModel,
			private boost::noncopyable
	{
		Q_OBJECT

	public:
	
		/**
		 * Custom Qt::ItemDataRole to allow ConfigValueDelegate to reset a value to the default.
		 * There is probably a better place/way to define this.
		 */
		enum CustomItemDataRole
		{
			ROLE_RESET_VALUE_TO_DEFAULT = Qt::UserRole + 1
		};
	

		/**
		 * Constructor for ConfigModel. You should not need to create these objects
		 * yourself; instead use the helper functions in ConfigBundleUtils.
		 * 
		 * Naturally, the Model needs to be a QObject, so in general it will be parented
		 * to the ConfigBundle whose interface it adapts.
		 */
		explicit
		ConfigModel(
				GPlatesUtils::ConfigInterface &_config,
				bool use_icons,
				QObject *_parent);

		virtual
		~ConfigModel();


		/**
		 * Qt Model/View accessor for data of a key or value (depending on index column).
		 */
		QVariant
		data(
				const QModelIndex &idx,
				int role = Qt::DisplayRole) const;

		/**
		 * Qt Model/View accessor for header contents and style.
		 */
		QVariant
		headerData(
				int section,
				Qt::Orientation orientation,
				int role) const;
		
		/**
		 * Qt Model/View accessor to set data of a key's value.
		 */
		bool
		setData(
				const QModelIndex &idx,
				const QVariant &value,
				int role = Qt::EditRole);

		/**
		 * Qt Model/View accessor for item flags of a key or value (depending on index column).
		 */
		Qt::ItemFlags
		flags(
				const QModelIndex &idx) const;
		
		/**
		 * Qt Model/View accessor to see how many configuration keyvalues we have.
		 */
		int
		rowCount(
				const QModelIndex &parent_idx) const;


		/**
		 * Qt Model/View accessor to see how many columns the table should have.
		 */
		int
		columnCount(
				const QModelIndex &parent_idx) const
		{
			return NUM_COLUMNS;
		}


		/**
		 * In order to effectively map the hashmap-like ConfigInterface onto a table,
		 * complete with smart widget delegates and user-friendly key names, we need
		 * a few extra bits of metadata to be stored for each key name. This might be
		 * provided at ConfigModel construction time, by some user-defined python
		 * script, or (in the case of the much more nebulous QSettings backend), it
		 * might have to be generated on the fly in response to key_value_updated()
		 * signals. Either way, these structs form the basis for an 'index' that we
		 * build and store in ConfigModel to reference each key/value pair. However
		 * we can't really call it an index, that word is overloaded here, so we'll
		 * call it a 'schema' instead.
		 */
		struct SchemaEntry
		{
			QString key;
			QString label;
		};
		typedef QList<SchemaEntry> SchemaType;


	private slots:
		
		/**
		 * When our underlying ConfigInterface gets changed, we need to emite a
		 * 'dataChanged()' signal that Qt Views can use to repaint table cells
		 * as needed.
		 */
		void
		react_key_value_updated(
				QString key);

	private:
		
		/**
		 * Return suitable QVariant-packed data for the 'name' column of a particular
		 * SchemaEntry.
		 */
		QVariant
		get_name_data_for_role(
				const SchemaEntry &entry,
				int role) const;

		/**
		 * Return suitable QVariant-packed data for the 'value' column of a particular
		 * SchemaEntry.
		 */
		QVariant
		get_value_data_for_role(
				const SchemaEntry &entry,
				int role) const;


		/**
		 * Configuration tables are only ever going to have two columns; the name and the value.
		 */
		enum ModelColumn
		{
			COLUMN_NAME,
			COLUMN_VALUE,
			NUM_COLUMNS
		};
		
		
		/**
		 * The ConfigBundle or UserPreferences backend.
		 */
		QPointer<GPlatesUtils::ConfigInterface> d_config_ptr;

				
		/**
		 * The schema is a list of SchemaEntry structs that defines two important
		 * things for the ConfigModel:-
		 *   1. Metadata to fill in table cell names, widget types, etc.
		 *   2. A stable ordering we can access by offset.
		 */
		SchemaType d_schema;

		/**
		 * The default setup for UserPreferences uses tick icons to show whether a
		 * default value has been overridden by the user. This is configurable,
		 * since e.g. python colouring probably wouldn't want it.
		 */
		bool d_use_icons_indicating_defaults;

		// Default colours, packed into QBrushes, packed into QVariants, to be returned
		// from data() accesses for table foreground/background requests.
		QVariant d_default_foreground;
		QVariant d_default_background;
		
		// Possible icons indicating a user-set value, with and without a default backing it,
		// and a blank icon for no user-set value. Once again, pre-packed into a QVariant
		// for convenience when data() gets called.
		QVariant d_user_overriding_default_icon;
		QVariant d_user_no_default_icon;
		QVariant d_default_value_icon;
		
	};
}

#endif // GPLATES_GUI_CONFIGMODEL_H
