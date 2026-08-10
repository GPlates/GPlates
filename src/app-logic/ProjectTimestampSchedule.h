/*
 * Copyright (C) 2026 CaliTarheel
 *
 * This file is part of GPlates.
 *
 * GPlates is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 */

#ifndef GPLATES_APP_LOGIC_PROJECTTIMESTAMPSCHEDULE_H
#define GPLATES_APP_LOGIC_PROJECTTIMESTAMPSCHEDULE_H

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <cstddef>
#include <QObject>
#include <QString>
#include <vector>


namespace GPlatesAppLogic
{
	class ProjectDocumentRegistry;

	/**
	 * Supplies the exact Project Timestamp schedule from the Primary Project
	 * Document while retaining the uniform builder used by existing generators.
	 */
	class ProjectTimestampSchedule :
			public QObject,
			private boost::noncopyable
	{
		Q_OBJECT

	public:
		enum Source
		{
			NO_PROJECT_TIMESTAMPS,
			PROJECT_MARKDOWN,
			INVALID_PROJECT_METADATA
		};
		Q_ENUM(Source)

		explicit
		ProjectTimestampSchedule(
				ProjectDocumentRegistry &document_registry,
				QObject *parent = NULL);

		/**
		 * Existing safe inclusive uniform builder used by D8 and D10.
		 */
		static
		std::vector<double>
		build(
				double youngest_time,
				double oldest_time,
				double step,
				std::size_t maximum_timestamps = 100000);

		const std::vector<double> &
		timestamps_older_to_younger() const;

		boost::optional<double>
		next_older_timestamp(
				double current_time) const;

		boost::optional<double>
		next_younger_timestamp(
				double current_time) const;

		boost::optional<double>
		default_older_bound(
				double current_time) const;

		Source
		source() const;

		const QString &
		diagnostic() const;

	public Q_SLOTS:
		void
		reload_from_primary_document();

	Q_SIGNALS:
		void
		schedule_changed();

	private:
		void
		set_state(
				const std::vector<double> &timestamps,
				Source source,
				const QString &diagnostic);

		ProjectDocumentRegistry *d_document_registry;
		std::vector<double> d_timestamps_older_to_younger;
		Source d_source;
		QString d_diagnostic;
	};
}

#endif
