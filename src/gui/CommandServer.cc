/* $Id$ */

/**
 * @file 
 * Contains the implementation of the CommandServer class.
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
#include <boost/tuple/tuple.hpp>

#include "CommandServer.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/CoRegistrationLayerProxy.h"
#include "app-logic/UserPreferences.h"

#include "data-mining/DataMiningUtils.h"

#include "global/LogException.h"

#include "gui/AnimationController.h"

#include "model/ModelUtils.h"

#include "opengl/GLContext.h"
#include "opengl/GLRenderer.h"

#include "presentation/Application.h"
#include "presentation/ViewState.h"
#include "presentation/VisualLayers.h"

#include "qt-widgets/ReconstructionViewWidget.h"
#include "qt-widgets/GlobeAndMapWidget.h"

#include "utils/FeatureUtils.h"

namespace
{
	bool 
	readNextStartElement(
			QXmlStreamReader& reader)
	{
		while (reader.readNext() != QXmlStreamReader::Invalid) {
			if (reader.isEndElement())
				return false;
			else if (reader.isStartElement())
				return true;
		}
		return false;
	}

	boost::optional<GPlatesAppLogic::CoRegistrationLayerProxy::non_null_ptr_type>
	get_coregistration_layer_proxy(
			GPlatesPresentation::ViewState &view_state,
			const QString &layer_name)
	{
		using namespace GPlatesPresentation;
		using namespace GPlatesAppLogic;

		VisualLayers& layers = view_state.get_visual_layers();
		boost::weak_ptr<VisualLayer> layer;
		boost::shared_ptr<VisualLayer> locked_visual_layer;
		for(size_t i=0; i < layers.size(); i++)
		{
			locked_visual_layer.reset();
			layer = layers.visual_layer_at(i);
			locked_visual_layer = layer.lock();
			if(locked_visual_layer)
			{
				CoRegistrationLayerProxy* coreg_proxy = NULL;
				boost::optional<GPlatesAppLogic::LayerProxy::non_null_ptr_type> layer_proxy = 
					locked_visual_layer->get_reconstruct_graph_layer().get_layer_output();
				if(layer_proxy)
				{   //First, we check if the layer is a co-registration layer.
					 coreg_proxy = dynamic_cast<CoRegistrationLayerProxy*>((*layer_proxy).get());
					if(!coreg_proxy)
					{
						//Not a co-registration layer, we continue the search.
						continue;
					}
					if(layer_name.isEmpty())
					{
						//No layer name is given. So, we just return the first one we found.
						return CoRegistrationLayerProxy::non_null_ptr_type(coreg_proxy);
					}
					else
					{
						if(locked_visual_layer->get_name() == layer_name)
						{
							// We have found the layer with the specific name.
							return CoRegistrationLayerProxy::non_null_ptr_type(coreg_proxy);
						}
						else
						{
							//Not the layer we are looking for. continue the search.
							continue;
						}
					}
				}
			}
		}
		return boost::none; //Could not find the co-registration layer.
	}
}


GPlatesGui::CommandServer::CommandServer(
		GPlatesAppLogic::ApplicationState &application_state,
		GPlatesPresentation::ViewState &view_state,
		GPlatesQtWidgets::ViewportWindow &main_window,
		unsigned port, 
		QObject* _parent) :
	QTcpServer(_parent),
	d_disabled(false),
	d_command(""),
	d_timeout(false),
	d_timer(new QTimer(this)),
	d_app_state(application_state),
	d_view_state(view_state),
	d_main_window(main_window)
{
	GPlatesAppLogic::UserPreferences pref(NULL);
	if(0 == port)
	{
		port = pref.get_value("net/server/port").toUInt();
	}

	QHostAddress addr = QHostAddress::LocalHost;
	if(!pref.get_value("net/server/local").toBool())
	{
		addr =  QHostAddress::Any;
	}

	if(!listen(addr, port))
	{
		qWarning() << "Failed to listen on port:" << port;
	}
	d_command_map["GetSeeds"] = &CommandServer::create_get_seeds_command;
	d_command_map["GetTimeSetting"] = &CommandServer::create_get_time_setting_command;
	d_command_map["GetBeginTime"] = &CommandServer::create_get_begin_time_command;
	d_command_map["GetAssociations"] = &CommandServer::create_get_associations_command;
	d_command_map["GetAssociationData"] = &CommandServer::create_get_association_data_command;
	d_command_map["GetBirthAttribute"] = &CommandServer::create_get_birth_attribute_command;
	d_command_map["SetReconstructionTime"] = &CommandServer::create_set_reconstruction_time_command;
	connect(d_timer, SIGNAL(timeout()), this, SLOT(timeout()));
}


void
GPlatesGui::CommandServer::readClient()
{
	if (d_disabled)
	{
		return;
	}

	if(d_command.isEmpty())
	{
		d_timeout = false;
		d_timer->start(1000);
	}

	// This slot is called when the client sent data to the server. 
	QTcpSocket* socket = static_cast<QTcpSocket*>(sender());
	d_command += QString(socket->readAll());

	boost::shared_ptr<Command> c = create_command(d_command);
	
	if(!d_command.contains("</Request>") && !d_timeout)//keep waiting for the whole command until timeout
	{
		return;
	}

	if(c)
	{
		c->execute(socket);
	}
	else
	{
		qWarning() << "Failed to create command for request: " << d_command;
		QTextStream os(socket);
		os.setAutoDetectUnicode(true);
		os << "<Response><ErrorMsg>Failed to create command for request.</ErrorMsg></Response>";
	}
	d_command.clear();

	if (socket->state() == QTcpSocket::UnconnectedState) 
	{
		delete socket;
		//qDebug() << "Connection closed";
	}
}


boost::shared_ptr<GPlatesGui::Command>
GPlatesGui::CommandServer::create_command(
		const QString& request)
{
	QXmlStreamReader reader(request);
	if(reader.readNextStartElement() && ("Request" == reader.name()))//assume the first element is "Request" 
	{
		if(reader.readNextStartElement() && reader.name() == "Name")
		{
			QString name = reader.readElementText().simplified();
			if(d_command_map.find(name) != d_command_map.end())
			{
				return (this->*(d_command_map[name]))(reader);
			}
		}
	}
	qWarning() << "Invalid request: " << request;
	return boost::shared_ptr<Command>();
}


bool
GPlatesGui::GetSeedsCommand::execute(
		QTcpSocket* socket)
{
	using namespace GPlatesAppLogic;
	using namespace GPlatesDataMining;

	std::set<QString> feature_id_set;
	QTextStream os(socket);
	os.setAutoDetectUnicode(true);
	boost::optional<CoRegistrationLayerProxy::non_null_ptr_type> proxy =
		get_coregistration_layer_proxy(d_view_state, d_layer_name);
	if(!proxy)
	{
		qWarning() << "Not be able to get co-registration layer.";
		return false;
	}
	try{
		BOOST_FOREACH(
				const GPlatesModel::FeatureHandle::weak_ref f, 
				(*proxy)->get_seed_features())
		{
			if(f.is_valid())
			{
				feature_id_set.insert(f->feature_id().get().qstring());
			}
		}
	}catch(GPlatesGlobal::Exception& ex)
	{
		ex.write(std::cerr);
		std::stringstream ss;
		ex.write(ss);
		os << "<Response><ErrorMsg>" << 
			escape_reserved_xml_characters(QString(ss.str().c_str())) 
			<< "</ErrorMsg></Response>";
		return false;
	}
	
	os << "<Response>";
	BOOST_FOREACH(const QString& id, feature_id_set)
	{
		os << escape_reserved_xml_characters(id) << " ";
	}
	os << "</Response>";
	os.flush();
 	return true;
}


bool
GPlatesGui::GetTimeSettingCommand::execute(
		QTcpSocket* socket)
{
	using namespace GPlatesAppLogic;
	QTextStream os(socket);
	os.setAutoDetectUnicode(true);
	double st = 140.0, et = 0.0, inc = 1.0;
	
	try{
		AnimationController&  controller = d_view_state.get_animation_controller();
		st = controller.start_time();
		et = controller.end_time();
		inc = controller.time_increment();
		
	}catch(GPlatesGlobal::Exception& ex)
	{
		ex.write(std::cerr);
		std::stringstream ss;
		ex.write(ss);
		os << "<Response><ErrorMsg>" << escape_reserved_xml_characters(QString(ss.str().c_str())) << "</ErrorMsg></Response>";
		return false;
	}

	os << "<Response>";
	os << "<BeginTime>" << st << "</BeginTime>";
	os << "<EndTime>" << et << "</EndTime>";
	os << "<Increment>" << inc << "</Increment>";
	os << "</Response>";
	os.flush();
	return true;
}

bool
GPlatesGui::GetBeginTimeCommand::execute(
		QTcpSocket* socket)
{
	QTextStream os(socket);
	os.setAutoDetectUnicode(true);
	try{
		GPlatesModel::FeatureHandle::weak_ref feature = 
			GPlatesModel::ModelUtils::find_feature(d_feature_id);
		boost::optional<GPlatesMaths::Real> begin_time = 
			GPlatesUtils::get_begin_time(feature.handle_ptr());
		QString bt_str;
		if(begin_time)
		{
			if(begin_time->is_negative_infinity())
			{
				bt_str = "-inf";
			}
			if(begin_time->is_positive_infinity())
			{
				bt_str = "inf";
			}
			else
			{
				bt_str = QString().setNum(begin_time->dval());
			}
		}
		else
		{
			bt_str = "NaN";
		}

		os << "<Response>";
		os << bt_str ;
		os << "</Response>";
	}
	catch(GPlatesGlobal::Exception& ex)//TODO
	{
		ex.write(std::cerr);
		std::stringstream ss;
		ex.write(ss);
		os << "<Response><ErrorMsg>" << escape_reserved_xml_characters(QString(ss.str().c_str())) << "</ErrorMsg></Response>";
		return false;
	}
	os.flush();
	return true;
}


bool
GPlatesGui::GetAssociationsCommand::execute(
		QTcpSocket* socket)
{
	using namespace GPlatesDataMining;
	bool ret = true;
	QTextStream os(socket);
	os.setAutoDetectUnicode(true);
	boost::optional<GPlatesAppLogic::CoRegistrationLayerProxy::non_null_ptr_type> proxy =
		get_coregistration_layer_proxy(d_view_state, d_layer_name);
	if(!proxy)
	{
		qWarning() << "Not be able to get co-registration layer.";
		return false;
	}
	try{
		const CoRegConfigurationTable& table = 
			(*proxy)->get_current_coregistration_configuration_table();

		
		std::map<unsigned, ConfigurationTableRow> sorted_map;
		std::map<unsigned, ConfigurationTableRow>::const_iterator map_it;
		CoRegConfigurationTable::const_iterator it = table.begin();
		for( ; it != table.end(); it++)
		{
			sorted_map.insert(std::pair<unsigned, ConfigurationTableRow>(it->index, *it));
		}
		map_it = sorted_map.begin();
		
		os << "<Response>";
		for( ; map_it != sorted_map.end(); map_it++)
		{
			os << to_string(map_it->second);
		}
		os << "</Response>";
	}catch(GPlatesGlobal::Exception& ex)
	{
		ex.write(std::cerr);
		std::stringstream ss;
		ex.write(ss);
		os << "<Response><ErrorMsg>" << escape_reserved_xml_characters(
			QString(ss.str().c_str())) << "</ErrorMsg></Response>";
		ret = false;
	}
	os.flush();
	return ret;
}


bool
GPlatesGui::GetAssociationDataCommand::execute(
		QTcpSocket* socket)
{
	bool ret = true;
	QTextStream os(socket);
	os.setAutoDetectUnicode(true);
	os << "<Response>";

	if(d_invalid_time)
	{
		os << "<ErrorMsg>Invalid reconstruction time.</ErrorMsg></Response>";
		return false;
	}

	GPlatesOpenGL::GLContext::non_null_ptr_type gl_context =
		d_main_window.reconstruction_view_widget().globe_and_map_widget().get_active_gl_context();

	// Make sure the context is currently active.
	gl_context->make_current();

	// Start a begin_render/end_render scope.
	// NOTE: Before calling this, OpenGL should be in the default OpenGL state.
	GPlatesOpenGL::GLRenderer::non_null_ptr_type renderer = gl_context->create_renderer();
	GPlatesOpenGL::GLRenderer::RenderScope render_scope(*renderer);
	boost::optional<GPlatesAppLogic::CoRegistrationLayerProxy::non_null_ptr_type> proxy =
		get_coregistration_layer_proxy(d_view_state, d_layer_name);
	if(!proxy)
	{
		qWarning() << "Not be able to get co-registration layer.";
		return false;
	}
	try{
		boost::optional<GPlatesAppLogic::CoRegistrationData::non_null_ptr_type> coregistration_data = 
			(*proxy)->get_coregistration_data(*renderer, d_time);
		if(coregistration_data)
		{
			std::vector<std::vector<QString> > table;
			(*coregistration_data)->data_table().to_qstring_table(table);
			if(!table.empty())
			{
				os << "<DataTable row=\"" << table.size() <<  "\" column=\"" << table[0].size() << "\" >";
				BOOST_FOREACH(const std::vector<QString>& row, table)
				{
					BOOST_FOREACH(const QString& cell, row)
					{
						os << "<c>" << escape_reserved_xml_characters(cell) << "</c>";
					}
				}
				os << "</DataTable>";
			}
		}
		os << "</Response>";
	}catch(GPlatesGlobal::Exception& ex)
	{
		ex.write(std::cerr);
		std::stringstream ss;
		ex.write(ss);
		os << "<ErrorMsg>" << escape_reserved_xml_characters(QString(ss.str().c_str())) << "</ErrorMsg></Response>";
		ret = false;
	}
	
	os.flush();
	return ret;
}


bool
GPlatesGui::GetBirthAttributeCommand::execute(
		QTcpSocket* socket)
{
	bool ret = true;
	QTextStream os(socket);
	os.setAutoDetectUnicode(true);
	os << "<Response>";

	GPlatesOpenGL::GLContext::non_null_ptr_type gl_context =
		d_main_window.reconstruction_view_widget().globe_and_map_widget().get_active_gl_context();

	// Make sure the context is currently active.
	gl_context->make_current();

	// Start a begin_render/end_render scope.
	// NOTE: Before calling this, OpenGL should be in the default OpenGL state.
	GPlatesOpenGL::GLRenderer::non_null_ptr_type renderer = gl_context->create_renderer();
	GPlatesOpenGL::GLRenderer::RenderScope render_scope(*renderer);
	try{
		//TODO: The following code should be re-factored 2013/09/23.
		//1. Catch the paticular exception, not GPlatesGlobal::Exception.h
		//2. Create a new function for converting CoRegistrationData into xml DataTable.
		boost::optional<GPlatesAppLogic::CoRegistrationLayerProxy::non_null_ptr_type> proxy =
			get_coregistration_layer_proxy(d_view_state, d_layer_name);
		if(!proxy)
		{
			qWarning() << "Not be able to get co-registration layer.";
			return false;
		}
		boost::optional<GPlatesAppLogic::CoRegistrationData::non_null_ptr_type> coregistration_data = 
			(*proxy)->get_birth_attribute_data(
					*renderer, 
					GPlatesModel::FeatureId(GPlatesUtils::UnicodeString(d_feature_id)));
		if(coregistration_data)
		{
			std::vector<std::vector<QString> > table;
			(*coregistration_data)->data_table().to_qstring_table(table);
			if(!table.empty())
			{
				os << "<DataTable row=\"" << table.size() <<  "\" column=\"" << table[0].size() << "\" >";
				BOOST_FOREACH(const std::vector<QString>& row, table)
				{
					BOOST_FOREACH(const QString& cell, row)
					{
						os << "<c>" << escape_reserved_xml_characters(cell) << "</c>";
					}
				}
				os << "</DataTable>";
			}
		}
		os << "</Response>";
	}catch(GPlatesGlobal::Exception& ex)
	{
		ex.write(std::cerr);
		std::stringstream ss;
		ex.write(ss);
		os << "<ErrorMsg>" << escape_reserved_xml_characters(QString(ss.str().c_str())) << "</ErrorMsg></Response>";
		ret = false;
	}

	os.flush();
	return ret;
}


bool
GPlatesGui::SetReconstructionTimeCommand::execute(
		QTcpSocket* socket)
{
	QTextStream os(socket);
	os.setAutoDetectUnicode(true);
	try{

		AnimationController&  controller = d_view_state.get_animation_controller();
	
		if(d_time < 0)
		{
			qWarning() << "Invalid input reconstruction time: " << d_time ;
			return false;
		}
		controller.set_view_time(d_time);
		os << "<Response>";
		os << "<Status>Succeed</Status>";
		os << "</Response>";
	}catch(GPlatesGlobal::Exception& ex)
	{
		ex.write(std::cerr);
		std::stringstream ss;
		ex.write(ss);
		os << "<Response><ErrorMsg>" << escape_reserved_xml_characters(
			QString(ss.str().c_str())) << "</ErrorMsg></Response>";
		return false;
	}
	os.flush();
	return true;
}


boost::shared_ptr<GPlatesGui::Command>
GPlatesGui::CommandServer::create_get_seeds_command(
		QXmlStreamReader& reader)
{
	QString layer_name = read_next_element_txt(reader, "LayerName");
	return boost::shared_ptr<Command>(
			new GetSeedsCommand(d_view_state, layer_name));
}

boost::shared_ptr<GPlatesGui::Command>
GPlatesGui::CommandServer::create_get_time_setting_command(
		QXmlStreamReader& reader)
{
	return boost::shared_ptr<Command>(new GetTimeSettingCommand(d_view_state));
}
		
boost::shared_ptr<GPlatesGui::Command>
GPlatesGui::CommandServer::create_get_begin_time_command(
		QXmlStreamReader& reader)
{
	QString id;
	if(reader.readNextStartElement() && ("FeatureID" == reader.name()))
	{
		id = reader.readElementText().simplified();
	}
	return boost::shared_ptr<Command>(new GetBeginTimeCommand(id));
}


boost::shared_ptr<GPlatesGui::Command>
GPlatesGui::CommandServer::create_get_associations_command(
		QXmlStreamReader& reader)
{
	QString layer_name = read_next_element_txt(reader, "LayerName");
	return boost::shared_ptr<Command>(
			new GetAssociationsCommand(d_view_state, layer_name));
}

boost::shared_ptr<GPlatesGui::Command>
GPlatesGui::CommandServer::create_get_association_data_command(
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
	return boost::shared_ptr<Command>(
			new GetAssociationDataCommand(
					d_view_state,
					d_main_window,
					time, 
					layer_name, 
					is_invalid));
}

boost::shared_ptr<GPlatesGui::Command>
GPlatesGui::CommandServer::create_get_birth_attribute_command(
		QXmlStreamReader& reader)
{
	QString feature_id = read_next_element_txt(reader, "FeatureID");
	QString layer_name = read_next_element_txt(reader, "LayerName");
	return boost::shared_ptr<Command>(
			new GetBirthAttributeCommand(
					d_view_state,
					d_main_window,
					feature_id, 
					layer_name));
}

boost::shared_ptr<GPlatesGui::Command>
GPlatesGui::CommandServer::create_set_reconstruction_time_command(
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
	return boost::shared_ptr<Command>(
			new SetReconstructionTimeCommand(
					d_view_state,
					time));
}






