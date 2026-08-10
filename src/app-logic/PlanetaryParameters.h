/*
 * Copyright (C) 2026 The GPlates developers
 *
 * This file is part of GPlates.
 *
 * GPlates is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 */

#ifndef GPLATES_APP_LOGIC_PLANETARYPARAMETERS_H
#define GPLATES_APP_LOGIC_PLANETARYPARAMETERS_H

#include <boost/noncopyable.hpp>
#include <QObject>
#include <QString>


namespace GPlatesAppLogic
{
	class ProjectDocumentRegistry;

	/**
	 * Supplies project-wide physical planetary parameters to application services.
	 */
	class PlanetaryParameters :
			public QObject,
			private boost::noncopyable
	{
		Q_OBJECT

	public:
		enum RadiusSource
		{
			PROJECT_MARKDOWN,
			EARTH_DEFAULT,
			INVALID_PROJECT_METADATA_USING_EARTH_DEFAULT
		};
		Q_ENUM(RadiusSource)

		explicit
		PlanetaryParameters(
				ProjectDocumentRegistry &document_registry,
				QObject *parent = NULL);

		double
		effective_radius_metres() const;

		double
		effective_radius_kilometres() const;

		RadiusSource
		radius_source() const;

		const QString &
		radius_diagnostic() const;

	public Q_SLOTS:
		void
		reload_from_primary_document();

	Q_SIGNALS:
		void
		effective_radius_changed(
				double radius_metres);

		void
		metadata_changed();

	private:
		void
		use_earth_default(
				RadiusSource source,
				const QString &diagnostic = QString());

		void
		set_radius(
				double radius_metres,
				RadiusSource source,
				const QString &diagnostic);

		ProjectDocumentRegistry *d_document_registry;
		double d_effective_radius_metres;
		RadiusSource d_radius_source;
		QString d_radius_diagnostic;
	};
}

#endif // GPLATES_APP_LOGIC_PLANETARYPARAMETERS_H
