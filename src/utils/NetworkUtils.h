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

#ifndef GPLATES_UTILS_NETWORKUTILS_H
#define GPLATES_UTILS_NETWORKUTILS_H

#include <QUrl>
#include <QNetworkProxy>


namespace GPlatesUtils
{
	namespace NetworkUtils
	{
	
		/**
		 * Returns a url which approximates the parameters of a QNetworkProxy.
		 * It is not exact, and will pretend schemes like "socks5://" exist.
		 * This is used by UserPreferences to store details on a user-configured proxy.
		 */
		QUrl
		get_url_for_proxy(
				const QNetworkProxy &proxy);
		

		/**
		 * Returns a QNetworkProxy object constructed from a url.
		 * The given url will not really be a proper url, and might pretend schemes like
		 * "socks5://" exist.
		 * This can be used to get a user-configured proxy out of UserPreferences.
		 */
		QNetworkProxy
		get_proxy_for_url(
				const QUrl &url);
		
		
	}
}

#endif //GPLATES_UTILS_NETWORKUTILS_H
