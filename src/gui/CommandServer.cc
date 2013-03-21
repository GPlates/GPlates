/* $Id:  $ */

/**
 * @file 
 * Contains the implementation of the CommandServer class.
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
#include <boost/tuple/tuple.hpp>

#include "CommandServer.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/CoRegistrationLayerProxy.h"
#include "app-logic/UserPreferences.h"
#include "data-mining/DataMiningUtils.h"
#include "global/LogException.h"
#include "gui/AnimationController.h"
#include "opengl/GLContext.h"
#include "opengl/GLRenderer.h"
#include "presentation/Application.h"
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


	std::vector<GPlatesAppLogic::CoRegistrationLayerProxy::non_null_ptr_type>
	get_all_coreg_proxies()
	{
		using namespace GPlatesAppLogic;
		using namespace GPlatesPresentation;

		ApplicationState &state = Application::instance().get_application_state();

		// Get the current reconstruction (of all (enabled) layers).
		const Reconstruction &reconstruction =	state.get_current_reconstruction();

		// Get the co-registration layer outputs (likely only one layer but could be more).
		std::vector<CoRegistrationLayerProxy::non_null_ptr_type> co_registration_layer_outputs;
		reconstruction.get_active_layer_outputs<CoRegistrationLayerProxy>(co_registration_layer_outputs);
		return co_registration_layer_outputs;
	}


	GPlatesAppLogic::CoRegistrationLayerProxy::non_null_ptr_type
	get_coreg_proxy()
	{
		std::vector<GPlatesAppLogic::CoRegistrationLayerProxy::non_null_ptr_type> 
			co_registration_layer_outputs = get_all_coreg_proxies();
		
		if(co_registration_layer_outputs.empty())
		{
			throw GPlatesGlobal::LogException(
					GPLATES_EXCEPTION_SOURCE,
					"Cannot find any co-registration layer." );
		}
		else
		{
			if(co_registration_layer_outputs.size() != 1)
			{
				qWarning() << "More than one co-registration layers found. Only return the first one. ";
			}
			return co_registration_layer_outputs[0];
		}
	}


	GPlatesAppLogic::CoRegistrationLayerProxy::non_null_ptr_type
	get_coreg_proxy(
			const QString& layer_name)
	{
		using namespace GPlatesPresentation;
		using namespace GPlatesAppLogic;

		if(layer_name.isEmpty())
		{
			return get_coreg_proxy();
		}

		VisualLayers& layers = Application::instance().get_view_state().get_visual_layers();
		boost::weak_ptr<VisualLayer> layer;
		boost::shared_ptr<VisualLayer> locked_visual_layer;

		for(size_t i=0; i < layers.size(); i++)
		{
			layer = layers.visual_layer_at(i);
			if (locked_visual_layer = layer.lock())
			if(locked_visual_layer && (locked_visual_layer->get_name() == layer_name))
			{
				break;
			}
			locked_visual_layer.reset();
		}
		
		if(locked_visual_layer)
		{
			boost::optional<GPlatesAppLogic::LayerProxy::non_null_ptr_type> layer_proxy = 
				locked_visual_layer->get_reconstruct_graph_layer().get_layer_output();
			if(layer_proxy)
			{
				CoRegistrationLayerProxy* coreg_proxy = dynamic_cast<CoRegistrationLayerProxy*>((*layer_proxy).get());
				if(coreg_proxy)
				{
					return CoRegistrationLayerProxy::non_null_ptr_type(coreg_proxy);
				}
			}
		}
		throw GPlatesGlobal::LogException(
				GPLATES_EXCEPTION_SOURCE,
				"Cannot find co-registration layer with name: "+ layer_name);
	}


	GPlatesModel::FeatureHandle::non_null_ptr_type
	get_feature_by_id(
			const QString& id)
	{
		GPlatesPresentation::Application &app = GPlatesPresentation::Application::instance();
		std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> files =
			app.get_application_state().get_feature_collection_file_state().get_loaded_files();

		BOOST_FOREACH(GPlatesAppLogic::FeatureCollectionFileState::file_reference& file_ref, files)
		{
			GPlatesModel::FeatureCollectionHandle::weak_ref fc = file_ref.get_file().get_feature_collection();
			BOOST_FOREACH(GPlatesModel::FeatureHandle::non_null_ptr_type feature, *fc)
			{
				if(feature->feature_id().get().qstring() == id)
				{
					return feature;
				}
			}
		}
		throw GPlatesGlobal::LogException(
				GPLATES_EXCEPTION_SOURCE,
				"Cannot find feature with id: " + id ); 
	}
}


GPlatesGui::CommandServer::CommandServer(
		unsigned port, 
		QObject* _parent) :
	QTcpServer(_parent),
	d_disabled(false),
	d_command(""),
	d_timeout(false),
	d_timer(new QTimer(this))
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
	d_command_map["GetSeeds"] = create_get_seeds_command;
	d_command_map["GetTimeSetting"] = create_get_time_setting_command;
	d_command_map["GetBeginTime"] = create_get_begin_time_command;
	d_command_map["GetAssociations"] = create_get_associations_command;
	d_command_map["GetAssociationData"] = create_get_association_data_command;
	d_command_map["SetReconstructionTime"] = create_set_reconstruction_time_command;
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
				return d_command_map[name](reader);
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

	try{
		BOOST_FOREACH(
			const GPlatesModel::FeatureHandle::weak_ref f, 
			DataMiningUtils::get_all_seed_features(get_coreg_proxy(d_layer_name)))
		{
			if(f.is_valid())
				feature_id_set.insert(f->feature_id().get().qstring());
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
		GPlatesGui::AnimationController&  controller =
			GPlatesPresentation::Application::instance().get_view_state().get_animation_controller();
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
		GPlatesModel::FeatureHandle::non_null_ptr_type feature = get_feature_by_id(d_feature_id);
		boost::optional<GPlatesMaths::Real> begin_time = GPlatesUtils::get_begin_time(feature.get());
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
	}catch(GPlatesGlobal::Exception& ex)
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
	try{
		const GPlatesDataMining::CoRegConfigurationTable& table = 
			get_coreg_proxy(d_layer_name)->get_current_coregistration_configuration_table();

		
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
			os << GPlatesDataMining::to_string(map_it->second);
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
		GPlatesPresentation::Application::instance().get_main_window().
		reconstruction_view_widget().globe_and_map_widget().get_active_gl_context();

	// Make sure the context is currently active.
	gl_context->make_current();

	const unsigned int viewport_width =
			GPlatesPresentation::Application::instance().get_view_state().get_main_viewport_dimensions().first/*width*/;
	const unsigned int viewport_height =
			GPlatesPresentation::Application::instance().get_view_state().get_main_viewport_dimensions().second/*height*/;

	// Start a begin_render/end_render scope.
	// NOTE: Before calling this, OpenGL should be in the default OpenGL state.
	GPlatesOpenGL::GLRenderer::non_null_ptr_type renderer = gl_context->create_renderer();
	GPlatesOpenGL::GLRenderer::RenderScope render_scope(*renderer, viewport_width, viewport_height);
	try{
		boost::optional<GPlatesAppLogic::CoRegistrationData::non_null_ptr_type> coregistration_data = 
			get_coreg_proxy(d_layer_name)->get_coregistration_data(*renderer, d_time);
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
	bool ret = true;
	QTextStream os(socket);
	os.setAutoDetectUnicode(true);
	try{

		GPlatesGui::AnimationController&  controller =
			GPlatesPresentation::Application::instance().get_view_state().get_animation_controller();
	
		if(d_time < 0)
			throw GPlatesGlobal::LogException(GPLATES_EXCEPTION_SOURCE, "Invalid input reconstruction time.");
		
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
		ret = false;
	}
	os.flush();
	return ret;
}







