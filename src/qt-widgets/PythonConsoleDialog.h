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
 
#ifndef GPLATES_QTWIDGETS_PYTHONCONSOLEDIALOG_H
#define GPLATES_QTWIDGETS_PYTHONCONSOLEDIALOG_H
#include <boost/scoped_ptr.hpp>
#include <QDialog>
#include <QWidget>
#include <QString>
#include <QPlainTextEdit>
#include <QMutex>
#include <QWaitCondition>
#include <QMenu>
#include <QMessageBox>

#include "PythonConsoleDialogUi.h"

#include "OpenFileDialog.h"
#include "SaveFileDialog.h"

#include "api/AbstractConsole.h"
#include "api/ConsoleReader.h"
#include "api/ConsoleWriter.h"
#include "api/Sleeper.h"
#include "gui/EventBlackout.h"
#include "gui/PythonManager.h"

namespace GPlatesApi
{
	class PythonExecutionThread;
}

namespace GPlatesAppLogic
{
	class ApplicationState;


}

namespace GPlatesGui
{
	class PythonConsoleHistory;
}

namespace GPlatesPresentation
{
	class ViewState;
}

#if !defined(GPLATES_NO_PYTHON)
namespace GPlatesQtWidgets
{
	// Forward declarations.
	class ConsoleTextEdit;
	class PythonExecutionMonitorWidget;
	class PythonReadlineDialog;
	class ViewportWindow;

	/**
	 * PythonConsoleDialog is a dialog that allows for the interactive input of
	 * statements into the Python intepreter and displays the corresponding output.
	 * Python's stdout is redirected to this dialog, so the output of any
	 * background scripts is displayed here as well.
	 */
	class PythonConsoleDialog : 
			public QDialog,
			public GPlatesApi::AbstractConsole,
			protected Ui_PythonConsoleDialog 
	{
		Q_OBJECT

	public:

		PythonConsoleDialog(
				GPlatesAppLogic::ApplicationState &application_state,
				GPlatesPresentation::ViewState &view_state,
				ViewportWindow *viewport_window,
				QWidget *parent_ = NULL);

		/**
		 * Appends the given @a text to the console. The @a error flag indicates
		 * whether it should be decorated as an error message or not.
		 *
		 * This is the implementation of the pure virtual function in the base class
		 * @a AbstractConsole.
		 *
		 * Note that it is safe to call this function from any thread, even if it is
		 * not the GUI thread.
		 */
		virtual
		void
		append_text(
				const QString &text,
				bool error = false);


		/**
		 * Appends the stringified version of @a obj to the console. The @a error
		 * flag indicates whether it should be decorated as an error message or not.
		 *
		 * This is the implementation of the pure virtual function in the base class
		 * @a AbstractConsole.
		 *
		 * Note that it is safe to call this function from any thread, even if it is
		 * not the GUI thread.
		 */
		virtual
		void
		append_text(
				const boost::python::object &obj,
				bool error = false);


		/**
		 * Prompts the user for a line of input. This function pops up a modal dialog
		 * over the @a PythonConsoleDialog if it is visible, or if it is not, over the
		 * top of @a ViewportWindow.
		 *
		 * This is the implementation of the pure virtual function in the base class
		 * @a AbstractConsole.
		 *
		 * Note that it is safe to call this function from any thread, even if it is
		 * not the GUI thread.
		 */
		virtual
		QString
		read_line();

		/**
		 * Returns a menu that is populated with recent scripts that can be run again.
		 */
		QMenu *
		get_recent_scripts_menu()
		{
			return d_recent_scripts_menu;
		}

		/**
		 * Returns the last line in the console that is not blank. If no such line
		 * exists, returns the empty string.
		 */
		QString
		get_last_non_blank_line() const;

		QWidget*
		show_cancel_widget();

		QWidget* 
		hide_cancel_widget();

	public Q_SLOTS:

		/**
		 * Prompts the user to select a Python script, and runs that Python script.
		 * Adds the script chosen to a MRU list of scripts.
		 */
		void
		run_script();

		/**
		 * Clears the output textedit.
		 */
		void
		clear();

	Q_SIGNALS:

		/**
		 * Emitted when the text has been added to the console via stdout or stderr.
		 */
		void
		text_changed();

	protected:

		virtual
		void
		showEvent(
				QShowEvent *ev);

		virtual
		void
		keyPressEvent(
				QKeyEvent *ev);

		virtual
		void
		closeEvent(
				QCloseEvent *ev);

	private Q_SLOTS:

		void
		handle_return_pressed(
				QString line);

		void
		handle_control_c_pressed(
				QString line);

		void
		handle_save_button_clicked();

		void
		handle_system_exit_exception_raised(
				int exit_status,
				QString exit_error_message);

		void
		handle_recent_script_action_triggered(
				QAction *action);

	private:

		void
		make_signal_slot_connections();

		void
		print_banner();

		void
		do_append_text(
				const QString &text,
				bool error);

		void
		do_append_object(
				const boost::python::object &obj,
				bool error);

		QString
		do_read_line();

		void
		run_script(
				const QString &filename);

		GPlatesAppLogic::ApplicationState &d_application_state;
		GPlatesApi::PythonExecutionThread *d_python_execution_thread;
		GPlatesGui::PythonManager& d_python_manager;
		ViewportWindow *d_viewport_window;

		/**
		 * The widget that echoes inputs and displays outputs.
		 */
		ConsoleTextEdit *d_output_textedit;

		/**
		 * To let the user choose a Python script to run.
		 */
		OpenFileDialog d_open_file_dialog;

		/**
		 * To let the user choose a file name when they click the "Save" button.
		 */
		SaveFileDialog d_save_file_dialog;

		/**
		 * Any text buffered and not yet sent to the Python interpreter.
		 */
		QString d_buffered_lines;

		/**
		 * Redirects writes to Python's sys.stdout to this dialog.
		 */
		GPlatesApi::ConsoleWriter d_stdout_writer;

		/**
		 * A modal dialog to read a line of input from the user.
		 *
		 * Note that this must be constructed before, and destructed after,
		 * @a d_stdin_reader.
		 *
		 * Memory managed by Qt.
		 */
		PythonReadlineDialog *d_readline_dialog;

		/**
		 * Redirects attempts to read from sys.stdin to a custom modal dialog.
		 */
		GPlatesApi::ConsoleReader d_stdin_reader;

		/**
		 * Redirects writes to Python's sys.stderr to this dialog.
		 */
		GPlatesApi::ConsoleWriter d_stderr_writer;

		/**
		 * If true, close events are rejected.
		 */
		bool d_disable_close;

		

		/**
		 * A menu that allows the user to run recently-run scripts.
		 */
		QMenu *d_recent_scripts_menu;
				
		/**
		 * Allows the user to cancel execution with a GUI widget.
		 */
		PythonExecutionMonitorWidget *d_monitor_widget;

		/**
		 * The number of lines in the output textedit that are banner text.
		 */
		int d_num_banner_lines;

		/**
		 * Used to display messages telling the user about SystemExit exceptions.
		 */
		QMessageBox *d_system_exit_messagebox;
	};


	/**
	 * ConsoleInputTextEdit is a widget for the input of one line of Python.
	 */
	class ConsoleInputTextEdit :
			public QPlainTextEdit
	{
		Q_OBJECT

	public:

		enum Prompt
		{
			START_PROMPT,
			CONTINUATION_PROMPT
		};

		explicit
		ConsoleInputTextEdit(
				QWidget *parent_ = NULL);

		void
		set_prompt(
				Prompt prompt);

		void
		set_text(
				const QString &text);

		virtual
		QSize
		sizeHint() const;

		void
		set_vertical_padding(
				int padding);

		void
		handle_key_press_event(
				QKeyEvent *ev);

		const QString &
		get_prompt() const;

	Q_SIGNALS:

		void
		return_pressed(
				QString line);

		void
		up_pressed(
				QString line);

		void
		down_pressed(
				QString line);

		void
		control_c_pressed(
				QString line);

	protected:

		virtual
		void
		keyPressEvent(
				QKeyEvent *ev);

		virtual
		void
		mousePressEvent(
				QMouseEvent *ev);

		virtual
		bool
		viewportEvent(
				QEvent *ev);

		virtual
		bool
		canInsertFromMimeData(
				const QMimeData *source) const
		{
			return false;
		}

	private Q_SLOTS:

		void
		handle_text_changed();

		void
		check_cursor_position();

		void
		handle_internal_scrolling(
				int value);

	private:

		void
		set_prompt(
				const QString &prompt);

		QString
		get_text() const;

		bool d_inside_handle_text_changed;
		QString d_prompt;
		int d_vertical_padding;
	};


	/**
	 * ConsoleTextEdit is the widget that echoes inputs and displays outputs.
	 */
	class ConsoleTextEdit :
			public QPlainTextEdit
	{
		Q_OBJECT

	public:

		explicit
		ConsoleTextEdit(
				QWidget *parent_ = NULL);

		virtual
		~ConsoleTextEdit();

		void
		append_text(
				const QString &text,
				bool error = false);

		void
		append_text(
				const QString &prompt,
				const QString &text);

		void
		focus_on_input_widget();

		void
		set_input_prompt(
				ConsoleInputTextEdit::Prompt prompt);

		void
		set_input_widget_visible(
				bool visible);

		QString
		get_last_non_blank_line(
				int num_banner_lines) const;

	Q_SIGNALS:

		void
		return_pressed(
				QString line);

		void
		control_c_pressed(
				QString line);

	protected:

		virtual
		void
		keyPressEvent(
				QKeyEvent *ev);

		virtual
		void
		resizeEvent(
				QResizeEvent *ev);

		virtual
		void
		mousePressEvent(
				QMouseEvent *ev);

		virtual
		bool
		eventFilter(
				QObject *watched,
				QEvent *ev);

	private Q_SLOTS:

		void
		handle_text_changed();

		void
		handle_return_pressed(
				QString line);

		void
		handle_up_pressed(
				QString line);

		void
		handle_down_pressed(
				QString line);

		void
		handle_control_c_pressed(
				QString line);

		void
		reposition_input_widget();

	private:

		void
		scroll_to_bottom();

		ConsoleInputTextEdit *d_input_textedit;
		int d_vertical_padding;
		boost::scoped_ptr<GPlatesGui::PythonConsoleHistory> d_console_history;
		bool d_on_blank_line;
	};
}
#endif  //GPLATES_NO_PYTHON
#endif  // GPLATES_QTWIDGETS_PYTHONCONSOLEDIALOG_H
