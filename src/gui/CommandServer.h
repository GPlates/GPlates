/* $Id$ */

/**
 * @file 
 * Contains the definition of the CommandServer class.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_COMMANDSERVER_H
#define GPLATES_GUI_COMMANDSERVER_H
#include <set>

#include <boost/shared_ptr.hpp>

#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QStringList>
#include <QRegExp>
#include <QXmlStreamReader>

#include "app-logic/ApplicationState.h"

#include "presentation/ViewState.h"

namespace GPlatesGui
{
	inline
	QString
	escape_reserved_xml_characters(
			const QString str)
	{
		QString ret = str;
		ret.replace(QString("&"), QString("&#x26;"));
		ret.replace(QString("<"), QString("&#x60;"));
		ret.replace(QString(">"), QString("&#x62;"));
		return ret;
	}

	class Command
	{
	public:
		virtual
		bool
		execute(
				QTcpSocket*) = 0;

		virtual
		~Command(){}
	};


	class GetSeedsCommand : 
		public Command
	{
	public:
		GetSeedsCommand(
				GPlatesPresentation::ViewState &view_state,
				const QString& layer_name):
			d_view_state(view_state),
			d_layer_name(layer_name)
		{	}

		bool
		execute(
				QTcpSocket* socket);
	private:
		GPlatesPresentation::ViewState &d_view_state;
		QString d_layer_name;
	};


	class GetTimeSettingCommand : 
		public Command
	{
	public:
		GetTimeSettingCommand(
				GPlatesPresentation::ViewState &view_state):
			d_view_state(view_state)
		{	}

		bool
		execute(
				QTcpSocket* socket);
	private:
		GPlatesPresentation::ViewState &d_view_state;
	};


	class GetBeginTimeCommand : 
		public Command
	{
	public:
		GetBeginTimeCommand(
				const QString feature_id):
			d_feature_id(feature_id)
		{}

		bool
		execute(
				QTcpSocket* socket);
	private:
		QString d_feature_id;
	};


	class GetAssociationsCommand : 
		public Command
	{
	public:
		GetAssociationsCommand(
				GPlatesPresentation::ViewState &view_state,
				const QString& layer_name):
			d_view_state(view_state),
			d_layer_name(layer_name)
		{	}

		bool
		execute(
				QTcpSocket* socket);
	private:
		GPlatesPresentation::ViewState &d_view_state;
		QString d_layer_name;
	};


	class GetAssociationDataCommand : 
		public Command
	{
	public:
		GetAssociationDataCommand(
				GPlatesPresentation::ViewState &view_state,
				GPlatesQtWidgets::ViewportWindow &main_window,
				double time,
				const QString& layer_name,
				bool is_invalid = false):
			d_view_state(view_state),
			d_main_window(main_window),
			d_time(time),
			d_layer_name(layer_name),
			d_invalid_time(is_invalid)
		{	}

		bool
		execute(
				QTcpSocket* socket);
	private:
		GPlatesPresentation::ViewState &d_view_state;
		GPlatesQtWidgets::ViewportWindow &d_main_window;
		double d_time;
		QString d_layer_name;
		bool d_invalid_time;
	};


	class GetBirthAttributeCommand : 
		public Command
	{
	public:
		GetBirthAttributeCommand(
			GPlatesPresentation::ViewState &view_state,
			GPlatesQtWidgets::ViewportWindow &main_window,
			const QString& feature_id,
			const QString& layer_name):
		d_view_state(view_state),
		d_main_window(main_window),
		d_feature_id(feature_id),
		d_layer_name(layer_name)
		{	}

		bool
		execute(
				QTcpSocket* socket);
	private:
		GPlatesPresentation::ViewState &d_view_state;
		GPlatesQtWidgets::ViewportWindow &d_main_window;
		QString d_feature_id, d_layer_name;
	};

	
	class SetReconstructionTimeCommand : 
		public Command
	{
	public:
		SetReconstructionTimeCommand(
				GPlatesPresentation::ViewState &view_state,
				double time):
			d_view_state(view_state),
			d_time(time)
		{	}

		bool
		execute(
				QTcpSocket* socket);
	private:
		GPlatesPresentation::ViewState &d_view_state;
		double d_time;
	};


	inline
	QString
	read_next_element_txt(
			QXmlStreamReader& reader,
			const QString& name="")
	{
		QString txt;;
		if(reader.readNextStartElement())
		{
			if(!name.isEmpty()) 
			{
				if(reader.name() != name)
				{
					qWarning() << "The next element name is " << reader.name() << " instead of " << name;
					return "";
				}
			}
			txt = reader.readElementText().simplified();
		}
		else
		{
			qWarning() << "Reached the end of this XML document.";
		}
		return txt;
	}

	class CommandServer : 
		public QTcpServer 
	{
		Q_OBJECT
	public:
		CommandServer(
				GPlatesAppLogic::ApplicationState &application_state,
				GPlatesPresentation::ViewState &view_state,
				GPlatesQtWidgets::ViewportWindow &main_window,
				unsigned port = 0, 
				QObject* _parent = 0); 

		void 
		incomingConnection(
				qintptr socket)
		{
			 if (d_disabled)
			 {
				 return;
			 }

			// When a new client connects, the server constructs a QTcpSocket and all
			// communication with the client is done over this QTcpSocket. QTcpSocket
			// works asynchronously, this means that all the communication is done
			// in the two slots readClient() and discardClient().
			QTcpSocket* s = new QTcpSocket(this);
			connect(s, SIGNAL(readyRead()), this, SLOT(readClient()));
			connect(s, SIGNAL(disconnected()), this, SLOT(discardClient()));
			s->setSocketDescriptor(socket);
		}

		 void pause()
		 {
			 d_disabled = true;
		 }

		 void resume()
		 {
			 d_disabled = false;
		 }
	protected:
		/*
		* The following group of functions create Command objects for CommandServer.
		* The Command objects they create can be deduced easily from their function names.
		*/
		boost::shared_ptr<Command>
		create_get_seeds_command(
				QXmlStreamReader& reader);

		boost::shared_ptr<Command>
		create_get_time_setting_command(
				QXmlStreamReader& reader);
		
		boost::shared_ptr<Command>
		create_get_begin_time_command(
				QXmlStreamReader& reader);

		boost::shared_ptr<Command>
		create_get_associations_command(
				QXmlStreamReader& reader);

		boost::shared_ptr<Command>
		create_get_association_data_command(
				QXmlStreamReader& reader);

		boost::shared_ptr<Command>
		create_get_birth_attribute_command(
				QXmlStreamReader& reader);

		boost::shared_ptr<Command>
		create_set_reconstruction_time_command(
				QXmlStreamReader& reader);

	private Q_SLOTS:
		void 
		readClient();

		void discardClient()
		{
			QTcpSocket* socket = static_cast<QTcpSocket*>(sender());
			socket->deleteLater();
			//qDebug() << "Connection closed";
		}

		void
		timeout()
		{
			d_timeout = true;
		}

		boost::shared_ptr<Command>
		create_command(
				const QString& request);

	private:
		CommandServer(const CommandServer&);
		CommandServer& operator=(const CommandServer&);

		typedef boost::shared_ptr<Command> (CommandServer::*CreateFun)(QXmlStreamReader&);
		bool d_disabled;
		std::map<QString, CreateFun> d_command_map;
		QString d_command;
		bool d_timeout;
		QTimer *d_timer; 
		GPlatesAppLogic::ApplicationState &d_app_state;
		GPlatesPresentation::ViewState &d_view_state;
		GPlatesQtWidgets::ViewportWindow &d_main_window;
	};
}

#endif  // GPLATES_GUI_COMMANDSERVER_H



