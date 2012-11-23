/* $Id: $ */

/**
 * @file 
 * Contains the definition of the CommandServer class.
 *
 * Most recent change:
 *   $Date: $
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
#include <QtXml/QXmlStreamReader>

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
				const QString& layer_name):
			d_layer_name(layer_name)
		{	}

		bool
		execute(
				QTcpSocket* socket);
	private:
		QString d_layer_name;
	};


	class GetTimeSettingCommand : 
		public Command
	{
	public:
		GetTimeSettingCommand()
		{	}

		bool
		execute(
				QTcpSocket* socket);
	private:
		
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
				const QString& layer_name):
			d_layer_name(layer_name)
		{	}

		bool
		execute(
				QTcpSocket* socket);
	private:
		QString d_layer_name;
	};


	class GetAssociationDataCommand : 
		public Command
	{
	public:
		GetAssociationDataCommand(
				double time,
				const QString& layer_name,
				bool is_invalid = false):
			d_time(time),
			d_layer_name(layer_name),
			d_invalid_time(is_invalid)
		{	}

		bool
		execute(
				QTcpSocket* socket);
	private:
		double d_time;
		QString d_layer_name;
		bool d_invalid_time;
	};


	class SetReconstructionTimeCommand : 
		public Command
	{
	public:
		SetReconstructionTimeCommand(
				double time):
			d_time(time)
		{	}

		bool
		execute(
				QTcpSocket* socket);
	private:
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
	

	inline
	boost::shared_ptr<Command>
	create_get_seeds_command(
			QXmlStreamReader& reader)
	{
		QString layer_name = read_next_element_txt(reader, "LayerName");

		return boost::shared_ptr<Command>(new GetSeedsCommand(layer_name));
	}


	inline
	boost::shared_ptr<Command>
	create_get_time_setting_command(
			QXmlStreamReader& reader)
	{
		return boost::shared_ptr<Command>(new GetTimeSettingCommand());
	}


	inline
	boost::shared_ptr<Command>
	create_get_begin_time_command(
			QXmlStreamReader& reader)
	{
		QString id;
		if(reader.readNextStartElement() && ("FeatureID" == reader.name()))
		{
			id = reader.readElementText().simplified();
		}
		return boost::shared_ptr<Command>(new GetBeginTimeCommand(id));
	}


	inline
	boost::shared_ptr<Command>
	create_get_associations_command(
			QXmlStreamReader& reader)
	{
		QString layer_name = read_next_element_txt(reader, "LayerName");
		return boost::shared_ptr<Command>(new GetAssociationsCommand(layer_name));
	}


	inline
	boost::shared_ptr<Command>
	create_get_association_data_command(
			QXmlStreamReader& reader)
	{
		double time = 0.0;
		bool is_invalid = false;
		QString reconstruct_time = read_next_element_txt(reader, "ReconstructionTime");
		QString layer_name = read_next_element_txt(reader, "LayerName");
		bool ok;
		time = reconstruct_time.toDouble(&ok);
		if(!ok)
		{
			is_invalid = true;
			qWarning() << "The reconstructionTime is not a number: " << reconstruct_time;
		}
		return boost::shared_ptr<Command>(new GetAssociationDataCommand(time, layer_name, is_invalid));
	}


	inline
	boost::shared_ptr<Command>
	create_set_reconstruction_time_command(
			QXmlStreamReader& reader)
	{
		double time = 0.0;
		QString time_str = read_next_element_txt(reader, "ReconstructionTime");
		bool ok;
		time = time_str.toDouble(&ok);
		if(!ok)
		{
			time = -1;
			qWarning() << "Invalid reconstructionTime: " << time_str;
		}
		return boost::shared_ptr<Command>(new SetReconstructionTimeCommand(time));
	}


	class CommandServer : 
		public QTcpServer 
	{
		Q_OBJECT
	public:
		CommandServer(
				unsigned port = 0, 
				QObject* _parent = 0); 

		void 
		incomingConnection(
				int socket)
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
//			qDebug() << "New Connection " ;
		}

		 void pause()
		 {
			 d_disabled = true;
		 }

		 void resume()
		 {
			 d_disabled = false;
		 }

	private slots:
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

		typedef boost::shared_ptr<Command> (*CreateFun)(QXmlStreamReader&);
		bool d_disabled;
		std::map<QString, CreateFun> d_command_map;
		QString d_command;
		bool d_timeout;
		QTimer *d_timer; 
	};
}

#endif  // GPLATES_GUI_COMMANDSERVER_H



