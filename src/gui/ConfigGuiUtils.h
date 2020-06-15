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

#ifndef GPLATES_GUI_CONFIGGUIUTILS_H
#define GPLATES_GUI_CONFIGGUIUTILS_H

#include <QAbstractButton>
#include <QButtonGroup>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QPointer>
#include <QSpinBox>
#include <QTableView>
#include <QVariant>
#include <QWidget>

//class QMultiMap;


namespace GPlatesUtils
{
	class ConfigInterface;
}

namespace GPlatesQtWidgets
{
	class ConfigTableView;
}

namespace GPlatesGui
{
	namespace ConfigGuiUtils
	{

		class MapValueEquals:
				public std::unary_function<QString,bool>
		{
		public:
			explicit
			MapValueEquals(
					QString value):
				d_value(value)
			{}

			bool
			operator()(
					const QString &value) const
			{
				return (value == d_value);
			}
		private:
			QString d_value;
		};


		/**
		 * Given a ConfigBundle (or UserPreferences) and parent widget, create a QTableView that
		 * is linked to the ConfigBundle; changes in one will be reflected in the other.
		 *
		 * @param bundle - the bundle of key/value pairs.
		 * @param parent - a QWidget to serve as the parent for the returned QTableView widget.
		 *                 This is to ensure memory will be cleaned up appropriately; it is up to
		 *                 you to insert the widget into a layout somewhere.
		 */
		GPlatesQtWidgets::ConfigTableView *
		link_config_interface_to_table(
				GPlatesUtils::ConfigInterface &config,
				bool use_icons,
				QWidget *parent);
		
		
		/**
		 * Given an existing widget (of a small number of supported types), set up signal/slot
		 * connections so that the value of the widget is synchronised with a UserPreferences
		 * key.
		 */
		void
		link_widget_to_preference(
				QLineEdit *widget,
				GPlatesUtils::ConfigInterface &config,
				const QString &key,
				QAbstractButton *reset_button);

		void
		link_widget_to_preference(
				QCheckBox *widget,
				GPlatesUtils::ConfigInterface &config,
				const QString &key,
				QAbstractButton *reset_button);

		void
		link_widget_to_preference(
				QSpinBox *widget,
				GPlatesUtils::ConfigInterface &config,
				const QString &key,
				QAbstractButton *reset_button);

		void
		link_widget_to_preference(
				QDoubleSpinBox *widget,
				GPlatesUtils::ConfigInterface &config,
				const QString &key,
				QAbstractButton *reset_button);



		class ConfigWidgetAdapter :
				public QObject
		{
			Q_OBJECT
		public:
			explicit
			ConfigWidgetAdapter(
					QWidget *widget,
					GPlatesUtils::ConfigInterface &config,
					const QString &key);
			
			virtual
			~ConfigWidgetAdapter()
			{  }

		Q_SIGNALS:
			
			void
			value_changed(
					const QString &value);

			void
			value_changed(
					bool value);

			void
			value_changed(
					int value);

			void
			value_changed(
					double value);

		public Q_SLOTS:
			
			void
			handle_key_value_updated(
					QString key);

			void
			handle_widget_value_updated(
					QString value);

			void
			handle_widget_value_updated(
					bool value);

			void
			handle_widget_value_updated(
					int value);

			void
			handle_widget_value_updated(
					double value);

			// Because QLineEdit::editingFinished() doesn't also provide the text.
			// May be needed for other widget 'finished editing (void)' signals.
			void
			handle_widget_editing_finished();
			
			void
			handle_reset_clicked();
			
			
		private:
			QPointer<QWidget> d_widget_ptr;
			GPlatesUtils::ConfigInterface &d_config;
			QString d_key;
		};

		/**
		 * @brief The ConfigButtonGroupAdapter class
		 * - this is an awkward workaround for storing values from a group of radio buttons
		 * in preferences.
		 */
		class ConfigButtonGroupAdapter :
				public QObject
		{
			Q_OBJECT
		public:

			typedef QMap<int,QString> button_enum_to_description_map_type;

			explicit
			ConfigButtonGroupAdapter(
					QButtonGroup *button_group,
					GPlatesUtils::ConfigInterface &config,
					const QString &key,
					const button_enum_to_description_map_type &button_to_description_map);

			virtual
			~ConfigButtonGroupAdapter()
			{  }

		Q_SIGNALS:


			void
			value_changed(
					int value);


		public Q_SLOTS:

			void
			handle_key_value_updated(
					QString key);

			void
			handle_checked_button_changed(
					int index);

			void
			set_checked_button(
					int index);

			void
			handle_reset_clicked();


		private:
			QPointer<QButtonGroup> d_button_group_ptr;
			GPlatesUtils::ConfigInterface &d_config;
			QString d_key;
			const button_enum_to_description_map_type &d_button_to_description_map;
		};


		void
		link_button_group_to_preference(
				QButtonGroup *button_group,
				GPlatesUtils::ConfigInterface &config,
				const QString &key,
				const GPlatesGui::ConfigGuiUtils::ConfigButtonGroupAdapter::button_enum_to_description_map_type &map,
				QAbstractButton *reset_button);
	}
}

#endif // GPLATES_GUI_CONFIGGUIUTILS_H
