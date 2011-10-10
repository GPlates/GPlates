/* $Id$ */

/**
 * \file Miscellaneous helper functions for dealing with the network.
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

#include "NetworkUtils.h"


namespace
{
	/**
	 * Defines a mapping between Qt proxy type enum and string, unique to GPlates
	 * (i.e. not useful outside of GPlates although 'http' and 'ftp' will probably
	 * be at least recognisable).
	 */
	QMap<QNetworkProxy::ProxyType, QString>
	build_proxy_type_map()
	{
		QMap<QNetworkProxy::ProxyType, QString> map;

		// Note that only HttpProxy and Socks5Proxy make sense for a default system-wide proxy.
		map.insert(QNetworkProxy::Socks5Proxy, "socks5");
		map.insert(QNetworkProxy::HttpProxy, "http");		// Transparent proxy for anything
		map.insert(QNetworkProxy::HttpCachingProxy, "http-caching");		// Caching proxy, HTTP only.
		map.insert(QNetworkProxy::FtpCachingProxy, "ftp");		// Caching proxy, FTP only.

		return map;
	}
	
	QString
	url_scheme_for_proxy_type(
			QNetworkProxy::ProxyType type)
	{
		static const QMap<QNetworkProxy::ProxyType, QString> map = build_proxy_type_map();
		return map.value(type, "");
	}

	QNetworkProxy::ProxyType
	proxy_type_for_url_scheme(
			const QString &scheme)
	{
		static const QMap<QNetworkProxy::ProxyType, QString> map = build_proxy_type_map();
		return map.key(scheme, QNetworkProxy::NoProxy);
	}

}



QUrl
GPlatesUtils::NetworkUtils::get_url_for_proxy(
		const QNetworkProxy &proxy)
{
	QUrl url;
	url.setScheme(url_scheme_for_proxy_type(proxy.type()));
	url.setUserName(proxy.user());
	url.setPassword(proxy.password());
	url.setHost(proxy.hostName());
	url.setPort(proxy.port());
	return url;
}


QNetworkProxy
GPlatesUtils::NetworkUtils::get_proxy_for_url(
		const QUrl &url)
{
	QNetworkProxy proxy(proxy_type_for_url_scheme(url.scheme()),
			url.host(),
			url.port(),
			url.userName(),
			url.password());
	return proxy;
}


