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

#include <cstdio>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <QFont>
#include <QKeyEvent>
#include <QTextCharFormat>
#include <QBrush>
#include <QTextCursor>
#include <QScrollBar>
#include <QPalette>
#include <QFontMetrics>
#include <QKeyEvent>
#include <QThread>
#include <QEvent>
#include <QCoreApplication>
#include <QPainter>
#include <QFile>
#include <QTextBlock>
#include <QAction>
#include <QFileInfo>
#include <QDebug>

#include "PythonConsoleDialog.h"

#include "PythonExecutionMonitorWidget.h"
#include "PythonReadlineDialog.h"
#include "QtWidgetUtils.h"
#include "ViewportWindow.h"

#include "api/PythonExecutionMonitor.h"
#include "api/PythonExecutionThread.h"
#include "api/PythonRunner.h"
#include "api/PythonUtils.h"
#include "api/Sleeper.h"

#include "app-logic/ApplicationState.h"

#include "global/AssertionFailureException.h"
#include "global/Constants.h"
#include "global/GPlatesAssert.h"
#include "global/SubversionInfo.h"
#include "global/python.h"

#include "gui/PythonConsoleHistory.h"
#include "gui/PythonManager.h"

#include "presentation/ViewState.h"

#include "utils/DeferredCallEvent.h"

#if !defined(GPLATES_NO_PYTHON)
namespace
{
	const char *START_PROMPT_TEXT = QT_TR_NOOP(">>>\t");
	const char *CONTINUATION_PROMPT_TEXT = QT_TR_NOOP("...\t");

	QFont
	build_fixed_width_font()
	{
		// FIXME: Improve on this.
#if defined(Q_WS_X11)
		QFont font("Droid Sans Mono");
#else
		QFont font("Consolas");
#endif
		font.setStyleHint(QFont::Courier);
#if defined(Q_WS_MAC)
		font.setPointSize(14);
#else
		font.setPointSize(9);
#endif
		return font;
	}

	const QFont &
	get_fixed_width_font()
	{
		static const QFont FIXED_WIDTH_FONT = build_fixed_width_font();
		return FIXED_WIDTH_FONT;
	}

	QTextCharFormat
	build_prompt_format()
	{
		QTextCharFormat format;
		format.setForeground(QBrush(Qt::gray));
		return format;
	}

	const QTextCharFormat &
	get_prompt_format()
	{
		static const QTextCharFormat PROMPT_FORMAT = build_prompt_format();
		return PROMPT_FORMAT;
	}

	QTextCharFormat
	build_command_format()
	{
		QTextCharFormat format;
		format.setForeground(QBrush(Qt::darkMagenta));
		return format;
	}

	const QTextCharFormat &
	get_command_format()
	{
		static const QTextCharFormat COMMAND_FORMAT = build_command_format();
		return COMMAND_FORMAT;
	}

	QTextCharFormat
	build_normal_text_format()
	{
		QTextCharFormat format;
		format.setForeground(QBrush(Qt::black));
		return format;
	}

	const QTextCharFormat &
	get_normal_text_format()
	{
		static const QTextCharFormat NORMAL_TEXT_FORMAT = build_normal_text_format();
		return NORMAL_TEXT_FORMAT;
	}

	QTextCharFormat
	build_error_text_format()
	{
		QTextCharFormat format;
		format.setForeground(QBrush(Qt::darkCyan));
		return format;
	}

	const QTextCharFormat &
	get_error_text_format()
	{
		static const QTextCharFormat ERROR_TEXT_FORMAT = build_error_text_format();
		return ERROR_TEXT_FORMAT;
	}

	int
	get_tab_stop_width()
	{
		static const int TAB_STOP_WIDTH = QFontMetrics(get_fixed_width_font()).width("    ");
		return TAB_STOP_WIDTH;
	}

	//const char *BUILT_WITHOUT_PYTHON = QT_TR_NOOP("This version of GPlates was built without Python support");

	const char *SAVE_FILE_DIALOG_TITLE = QT_TR_NOOP("Save Python Console Buffer");

	GPlatesQtWidgets::SaveFileDialog::filter_list_type
	get_save_file_dialog_filters()
	{
		GPlatesQtWidgets::SaveFileDialog::filter_list_type result;

		GPlatesQtWidgets::FileDialogFilter html_filter(
				GPlatesQtWidgets::PythonConsoleDialog::tr("HTML Document"),
				"html");
		html_filter.add_extension("htm");
		result.push_back(html_filter);

		GPlatesQtWidgets::FileDialogFilter txt_filter(
				GPlatesQtWidgets::PythonConsoleDialog::tr("Text Document"),
				"txt");
		result.push_back(txt_filter);

		return result;
	}

	const char *OPEN_FILE_DIALOG_TITLE = QT_TR_NOOP("Run Python Script");

	const char *OPEN_FILE_DIALOG_FILTER = QT_TR_NOOP("Python Script (*.py *.pyw);;All Files (*)");
}


GPlatesQtWidgets::PythonConsoleDialog::PythonConsoleDialog(
		GPlatesAppLogic::ApplicationState &application_state,
		GPlatesPresentation::ViewState &view_state,
		ViewportWindow *viewport_window,
		QWidget *parent_)
	: QDialog(parent_, Qt::Window)
	, d_application_state(application_state)
	, d_python_manager(view_state.get_python_manager())
	, d_viewport_window(viewport_window)
	, d_output_textedit(new ConsoleTextEdit(this))
	, d_open_file_dialog(
			this,
			tr(OPEN_FILE_DIALOG_TITLE),
			tr(OPEN_FILE_DIALOG_FILTER),
			view_state)
	, d_save_file_dialog(
			this,
			tr(SAVE_FILE_DIALOG_TITLE),
			get_save_file_dialog_filters(),
			view_state)
	, d_stdout_writer(false /* not stderr */, this)
	, d_readline_dialog(new PythonReadlineDialog(this))
	, d_stdin_reader(this)
	// stderr replacement must be last. If there was an error during the above and
	// we call PyErr_Print(), we want it going to the actual stderr, not the
	// replacement (because the console dialog isn't ready yet!).
	, d_stderr_writer(true /* is stderr */, this)
	, d_disable_close(false)
	, d_recent_scripts_menu(new QMenu(tr("R&un Recent Script"), this))
	, d_monitor_widget(NULL)
	, d_num_banner_lines(0)
	, d_system_exit_messagebox(
			new QMessageBox(
				QMessageBox::Critical,
				tr("Python Exception"),
				QString(),
				QMessageBox::Ok,
				this))
{
	setupUi(this);
	run_script_button->setMenu(d_recent_scripts_menu);

	QtWidgetUtils::add_widget_to_placeholder(
			d_output_textedit,
			output_placeholder_widget);

	d_python_execution_thread = d_python_manager.get_python_execution_thread();

	make_signal_slot_connections();

	print_banner();

}


void
GPlatesQtWidgets::PythonConsoleDialog::make_signal_slot_connections()
{
	// Output textedit signals.
	QObject::connect(
			d_output_textedit,
			SIGNAL(return_pressed(QString)),
			this,
			SLOT(handle_return_pressed(QString)));
	QObject::connect(
			d_output_textedit,
			SIGNAL(control_c_pressed(QString)),
			this,
			SLOT(handle_control_c_pressed(QString)));

	// Button signals.
	QObject::connect(
			run_script_button,
			SIGNAL(clicked()),
			this,
			SLOT(run_script()));
	QObject::connect(
			save_button,
			SIGNAL(clicked()),
			this,
			SLOT(handle_save_button_clicked()));
	QObject::connect(
			clear_button,
			SIGNAL(clicked()),
			this,
			SLOT(clear()));
	QObject::connect(
			d_recent_scripts_menu,
			SIGNAL(triggered(QAction *)),
			this,
			SLOT(handle_recent_script_action_triggered(QAction *)));

	// Thread signals.
	QObject::connect(
			d_python_execution_thread,
			SIGNAL(system_exit_exception_raised(int, QString )),
			this,
			SLOT(handle_system_exit_exception_raised(int, QString )));

	QObject::connect(
			&GPlatesApi::PythonUtils::python_manager(),
			SIGNAL(system_exit_exception_raised(int, QString )),
			this,
			SLOT(handle_system_exit_exception_raised(int, QString )));
}


void
GPlatesQtWidgets::PythonConsoleDialog::print_banner()
{
	QString banner_text;
	banner_text += GPlatesGlobal::VersionString;
	banner_text += tr(" (r");
	QString version_number = GPlatesGlobal::SubversionInfo::get_working_copy_version_number();
	if (version_number.isEmpty())
	{
		version_number = tr("<unknown>");
	}
	banner_text += version_number;
	banner_text += tr(") with Python ");
	banner_text += Py_GetVersion();
	banner_text += tr(" on ");
	banner_text += Py_GetPlatform();
	banner_text.remove('\n');
	banner_text += "\nType \"help\" for more information.\n";
	append_text(banner_text);
	d_num_banner_lines = 2;
}


void
GPlatesQtWidgets::PythonConsoleDialog::append_text(
		const QString &text,
		bool error)
{
	// If called from the GUI thread, calls do_append_text straight away.
	// If not called from the GUI thread:
	// Post an event on the GUI thread to have the appending of text done there,
	// and then block until it is done.
	GPlatesUtils::DeferCall<void>::defer_call(
			boost::bind(
				&PythonConsoleDialog::do_append_text,
				boost::ref(*this),
				text,
				error),
			true /* blocking */);
}


void
GPlatesQtWidgets::PythonConsoleDialog::append_text(
		const boost::python::object &obj,
		bool error)
{
	// If called from the GUI thread, calls do_append_object straight away.
	// If not called from the GUI thread:
	// Post an event on the GUI thread to have the appending of text done there,
	// and then block until it is done.
	GPlatesUtils::DeferCall<void>::defer_call(
			boost::bind(
				&PythonConsoleDialog::do_append_object,
				boost::ref(*this),
				obj,
				error),
			true /* blocking */);
}


void
GPlatesQtWidgets::PythonConsoleDialog::do_append_text(
		const QString &text,
		bool error)
{
	// This must only be called from the GUI thread.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			QThread::currentThread() == thread(),
			GPLATES_ASSERTION_SOURCE);

	d_output_textedit->append_text(text, error);

	emit text_changed();
}


void
GPlatesQtWidgets::PythonConsoleDialog::do_append_object(
		const boost::python::object &obj,
		bool error)
{
	do_append_text(GPlatesApi::PythonUtils::to_QString(obj), error); 
}


QString
GPlatesQtWidgets::PythonConsoleDialog::read_line()
{
	// If called from the GUI thread, calls do_read_line straight away.
	// If not called from the GUI thread:
	// Post an event on the GUI thread to have the read line done there,
	// and then block until it is done.
	boost::function< QString () > bound_function = boost::bind(
			boost::mem_fn(&PythonConsoleDialog::do_read_line),
			this);
	return GPlatesUtils::DeferCall<QString>::defer_call(bound_function);
}


QString
GPlatesQtWidgets::PythonConsoleDialog::do_read_line()
{
	// This must only be called from the GUI thread.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			QThread::currentThread() == thread(),
			GPLATES_ASSERTION_SOURCE);

	// The prompt is the last line of text in the output text edit.
	QString prompt = d_output_textedit->document()->lastBlock().text();

	return d_readline_dialog->get_line(prompt);
}


QString
GPlatesQtWidgets::PythonConsoleDialog::get_last_non_blank_line() const
{
	return d_output_textedit->get_last_non_blank_line(d_num_banner_lines);
}


void
GPlatesQtWidgets::PythonConsoleDialog::run_script()
{
	QString file_name = d_open_file_dialog.get_open_file_name();
	if (!file_name.isEmpty())
	{
		run_script(file_name);
	}
}


void
GPlatesQtWidgets::PythonConsoleDialog::handle_recent_script_action_triggered(
		QAction *action)
{
	run_script(action->data().toString());
}


void
GPlatesQtWidgets::PythonConsoleDialog::run_script(
		const QString &filename)
{
	d_python_execution_thread->exec_file(filename, "utf-8"); // FIXME: hard coded coding
	
	// Check whether the file name is already associated with a menu item.
	QAction *first = NULL;
	BOOST_FOREACH(QAction *action, d_recent_scripts_menu->actions())
	{
		if (first == NULL)
		{
			first = action;
		}
		if (action->data().toString() == filename)
		{
			if (first != action)
			{
				d_recent_scripts_menu->removeAction(action);
				d_recent_scripts_menu->insertAction(first, action);
			}
			return;
		}
	}

	// Put the new script at the top of the menu.
	QAction *new_action = new QAction(QFileInfo(filename).fileName(), this);
	QVariant qv;
	qv.setValue(filename);
	new_action->setData(qv);
	d_recent_scripts_menu->insertAction(first, new_action);

	// Check that the menu isn't too full.
	static const int MAX_RECENT_SCRIPTS = 8;
	if (d_recent_scripts_menu->actions().count() > MAX_RECENT_SCRIPTS)
	{
		QAction *last_action = d_recent_scripts_menu->actions().last();
		d_recent_scripts_menu->removeAction(last_action);
	}
}


void
GPlatesQtWidgets::PythonConsoleDialog::showEvent(
		QShowEvent *ev)
{
	d_output_textedit->focus_on_input_widget();
}


void
GPlatesQtWidgets::PythonConsoleDialog::keyPressEvent(
		QKeyEvent *ev)
{
	// Eat the Esc key so that it doesn't close the dialog.
	if (ev->key() != Qt::Key_Escape)
	{
		QDialog::keyPressEvent(ev);
	}
}


void
GPlatesQtWidgets::PythonConsoleDialog::closeEvent(
		QCloseEvent *ev)
{
	if (d_disable_close)
	{
		ev->ignore();
	}
}


namespace
{
	bool
	is_all_whitespace(
			const QString &line)
	{
		for (QString::const_iterator iter = line.begin(); iter != line.end(); ++iter)
		{
			if (!iter->isSpace())
			{
				return false;
			}
		}
		return true;
	}
}


void
GPlatesQtWidgets::PythonConsoleDialog::handle_return_pressed(
		QString line)
{
	if (!line.isEmpty() && is_all_whitespace(line))
	{
		d_buffered_lines += line;
		d_buffered_lines += "\n";
		d_output_textedit->set_input_prompt(ConsoleInputTextEdit::CONTINUATION_PROMPT);
		return;
	}

	d_python_execution_thread->exec_interactive_command(d_buffered_lines + line);
	
	d_output_textedit->set_input_prompt(
			d_python_execution_thread->continue_interactive_input() ?
			ConsoleInputTextEdit::CONTINUATION_PROMPT :
			ConsoleInputTextEdit::START_PROMPT);

	d_buffered_lines.clear();
}





void
GPlatesQtWidgets::PythonConsoleDialog::handle_system_exit_exception_raised(
		int exit_status,
		QString exit_error_message)
{
	// Only show a warning if the exit status is not 0. 0 usually means success
	// so let's not scare the user!
	if (exit_status != 0)
	{
		QString warning;
		if (!exit_error_message.isEmpty())
		{
			warning = tr("A Python script raised an unhandled SystemExit exception \"%1\" with exit status %2.")
				.arg(exit_error_message).arg(exit_status);
		}
		else
		{
			warning = tr("A Python script raised an unhandled SystemExit exception with exit status %1.")
				.arg(exit_status);
		}
		warning += tr("\nThis is a serious error that usually results in program termination. "
			"Please consider saving your work and restarting GPlates.");

		d_system_exit_messagebox->setText(warning);
		d_system_exit_messagebox->exec();
	}
}


void
GPlatesQtWidgets::PythonConsoleDialog::handle_control_c_pressed(
		QString line)
{
	d_python_execution_thread->reset_interactive_buffer();
	d_buffered_lines.clear();
	d_output_textedit->set_input_prompt(ConsoleInputTextEdit::START_PROMPT);
	d_output_textedit->append_text("KeyboardInterrupt\n", true);
}


void
GPlatesQtWidgets::PythonConsoleDialog::handle_save_button_clicked()
{
	boost::optional<QString> file_name = d_save_file_dialog.get_file_name();
	if (file_name)
	{
		QFile output_file(*file_name);
		if (!output_file.open(QIODevice::WriteOnly))
		{
			QMessageBox::critical(
					this,
					tr(SAVE_FILE_DIALOG_TITLE),
					tr("GPlates could not write to the chosen location. Please choose another location."));
			return;
		}

		QString file_contents = 
			(file_name->endsWith("html") || file_name->endsWith("htm")) ?
			d_output_textedit->document()->toHtml("utf-8") :
			d_output_textedit->toPlainText();
		output_file.write(file_contents.toUtf8());

		output_file.close();
	}
}


void
GPlatesQtWidgets::PythonConsoleDialog::clear()
{
	d_output_textedit->clear();
	d_output_textedit->focus_on_input_widget();
	d_num_banner_lines = 0;
}


GPlatesQtWidgets::ConsoleInputTextEdit::ConsoleInputTextEdit(
		QWidget *parent_) :
	QPlainTextEdit(parent_),
	d_inside_handle_text_changed(false),
	d_vertical_padding(0)
{
	viewport()->setAutoFillBackground(false);
	document()->setUndoRedoEnabled(false);

	setFrameStyle(0);
	setTabStopWidth(get_tab_stop_width());
	setWordWrapMode(QTextOption::NoWrap);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	QObject::connect(
			this,
			SIGNAL(textChanged()),
			this,
			SLOT(handle_text_changed()));
	QObject::connect(
			this,
			SIGNAL(cursorPositionChanged()),
			this,
			SLOT(check_cursor_position()));
	QObject::connect(
			this,
			SIGNAL(selectionChanged()),
			this,
			SLOT(check_cursor_position()));
	QObject::connect(
			verticalScrollBar(),
			SIGNAL(valueChanged(int)),
			this,
			SLOT(handle_internal_scrolling(int)));

	set_prompt(START_PROMPT);
}


void
GPlatesQtWidgets::ConsoleInputTextEdit::set_prompt(
		Prompt prompt)
{
	switch (prompt)
	{
		case START_PROMPT:
			set_prompt(tr(START_PROMPT_TEXT));
			break;

		case CONTINUATION_PROMPT:
			set_prompt(tr(CONTINUATION_PROMPT_TEXT));
			break;
	}
}


QSize
GPlatesQtWidgets::ConsoleInputTextEdit::sizeHint() const
{
	// Account for the padding within the edit widget.
	QFontMetrics font_metrics(get_fixed_width_font());
	QSize size_hint = QPlainTextEdit::sizeHint();
	size_hint.setHeight((std::max)(font_metrics.lineSpacing(), font_metrics.height()) +
			2 * d_vertical_padding);
	return size_hint;
}


void
GPlatesQtWidgets::ConsoleInputTextEdit::set_vertical_padding(
		int padding)
{
	d_vertical_padding = padding;
	resize(sizeHint());
}


void
GPlatesQtWidgets::ConsoleInputTextEdit::handle_key_press_event(
		QKeyEvent *ev)
{
	keyPressEvent(ev);
}


const QString &
GPlatesQtWidgets::ConsoleInputTextEdit::get_prompt() const
{
	return d_prompt;
}


void
GPlatesQtWidgets::ConsoleInputTextEdit::keyPressEvent(
		QKeyEvent *ev)
{
	// Regardless of where the cursor is, if the user press return/enter, a
	// newline is not inserted at the cursor, but instead, we pretend that the
	// user pressed return/enter at the end of the line.
	if (ev->key() == Qt::Key_Return || ev->key() == Qt::Key_Enter)
	{
		emit return_pressed(get_text());
	}
	else if (ev->key() == Qt::Key_Up)
	{
		emit up_pressed(get_text());
	}
	else if (ev->key() == Qt::Key_Down)
	{
		emit down_pressed(get_text());
	}
	else if (QtWidgetUtils::is_control_c(ev))
	{
#if !defined(Q_WS_MAC)
		// If there is a selection, interpret the Ctrl+C as usual.
		if (textCursor().hasSelection())
		{
			QPlainTextEdit::keyPressEvent(ev);
			return;
		}
#endif
		emit control_c_pressed(get_text());
	}
#if defined(Q_WS_MAC)
	else if (ev->key() == Qt::Key_Backspace && ev->modifiers() == Qt::ControlModifier)
	{
		// Delete to front of line.
		QTextCursor text_cursor = textCursor();
		while (text_cursor.position() > d_prompt.length())
		{
			text_cursor.deletePreviousChar();
		}
		setTextCursor(text_cursor);
	}
#endif
	else
	{
		QPlainTextEdit::keyPressEvent(ev);
	}
}


void
GPlatesQtWidgets::ConsoleInputTextEdit::mousePressEvent(
		QMouseEvent *ev)
{
	QPlainTextEdit::mousePressEvent(ev);
	check_cursor_position();
}


bool
GPlatesQtWidgets::ConsoleInputTextEdit::viewportEvent(
		QEvent *ev)
{
	if (ev->type() == QEvent::Paint)
	{
		// Paint the yellow highlight behind the text.
		QPainter painter(viewport());
		painter.setBrush(QBrush(Qt::yellow));
		painter.setPen(QPen(Qt::NoPen));
		QRect cursor_rect = cursorRect();
		QRect region(0, cursor_rect.y(), width(), cursor_rect.height() + 1);
		painter.drawRect(region);
	}

	return QPlainTextEdit::viewportEvent(ev);
}


void
GPlatesQtWidgets::ConsoleInputTextEdit::handle_text_changed()
{
	if (d_inside_handle_text_changed)
	{
		return;
	}

	d_inside_handle_text_changed = true;

	// Check whether the user deleted part of the prompt.
	QTextCursor text_cursor = textCursor();
	text_cursor.movePosition(QTextCursor::Start);
	QString full_text = toPlainText();
	for (int i = 0; i != d_prompt.length(); ++i)
	{
		if (i >= full_text.length() || d_prompt.at(i) != full_text.at(i))
		{
			text_cursor.insertText(d_prompt.right(d_prompt.length() - i));
			setTextCursor(text_cursor);
			break;
		}
		text_cursor.movePosition(QTextCursor::NextCharacter);
	}

	// This handles the case where the user pastes in text that contains linebreaks.
	QString text = get_text();
	QStringList lines = text.split('\n');
	if (lines.count() >= 2)
	{
		set_text(QString());

		// Emit signal for each line except the last.
		for (int i = 0; i != lines.count() - 1; ++i)
		{
			emit return_pressed(lines.at(i));
		}

		// Set the last line as the text remaining in the edit box.
		set_text(lines.at(lines.count() - 1));
	}

	d_inside_handle_text_changed = false;
}


void
GPlatesQtWidgets::ConsoleInputTextEdit::check_cursor_position()
{
	QTextCursor text_cursor = textCursor();
	int start = text_cursor.selectionStart();
	int end = text_cursor.selectionEnd();

	if (start < d_prompt.length())
	{
		text_cursor.movePosition(QTextCursor::Start);

		if (start == text_cursor.position())
		{
			// Selection goes forwards.
			text_cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::MoveAnchor, d_prompt.length());
			int selection_length = end - d_prompt.length();
			if (selection_length > 0)
			{
				text_cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, selection_length);
			}
		}
		else
		{
			// Selection goes backwards.
			if (end < d_prompt.length())
			{
				end = d_prompt.length();
			}
			text_cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::MoveAnchor, end);
			int selection_length = end - d_prompt.length();
			if (selection_length > 0)
			{
				text_cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor, selection_length);
			}
		}

		setTextCursor(text_cursor);
	}
}


void
GPlatesQtWidgets::ConsoleInputTextEdit::handle_internal_scrolling(
		int value)
{
	if (value != verticalScrollBar()->minimum())
	{
		verticalScrollBar()->triggerAction(QAbstractSlider::SliderToMinimum);
	}
}


void
GPlatesQtWidgets::ConsoleInputTextEdit::set_prompt(
		const QString &prompt)
{
	d_prompt = prompt;
	setPlainText(prompt);
}


void
GPlatesQtWidgets::ConsoleInputTextEdit::set_text(
		const QString &text)
{
	setPlainText(d_prompt + text);
	moveCursor(QTextCursor::End);
}


QString
GPlatesQtWidgets::ConsoleInputTextEdit::get_text() const
{
	QString full_text = toPlainText();
	return full_text.right(full_text.length() - d_prompt.length());
}


GPlatesQtWidgets::ConsoleTextEdit::ConsoleTextEdit(
		QWidget *parent_) :
	QPlainTextEdit(parent_),
	d_input_textedit(new ConsoleInputTextEdit(this)),
	d_vertical_padding(0),
	d_console_history(new GPlatesGui::PythonConsoleHistory()),
	d_on_blank_line(true)
{
	setReadOnly(true);
	setFrameStyle(0);
	setTabStopWidth(get_tab_stop_width());
	setFont(get_fixed_width_font());
	setWordWrapMode(QTextOption::WrapAnywhere);

	QPalette this_palette = palette();
	this_palette.setColor(QPalette::Active, QPalette::Window, this_palette.color(QPalette::Active, QPalette::Base));
	this_palette.setColor(QPalette::Inactive, QPalette::Window, this_palette.color(QPalette::Inactive, QPalette::Base));
	this_palette.setColor(QPalette::Disabled, QPalette::Window, this_palette.color(QPalette::Disabled, QPalette::Base));
	setPalette(this_palette);
	setAutoFillBackground(true);

	d_input_textedit->setFont(get_fixed_width_font());
	d_vertical_padding = static_cast<int>(contentOffset().y());
	d_input_textedit->set_vertical_padding(d_vertical_padding);
	d_input_textedit->raise();
	d_input_textedit->installEventFilter(this);

	document()->setUndoRedoEnabled(false);

	QObject::connect(
			this,
			SIGNAL(textChanged()),
			this,
			SLOT(handle_text_changed()));
	QObject::connect(
			d_input_textedit,
			SIGNAL(return_pressed(QString)),
			this,
			SLOT(handle_return_pressed(QString)));
	QObject::connect(
			d_input_textedit,
			SIGNAL(up_pressed(QString)),
			this,
			SLOT(handle_up_pressed(QString)));
	QObject::connect(
			d_input_textedit,
			SIGNAL(down_pressed(QString)),
			this,
			SLOT(handle_down_pressed(QString)));
	QObject::connect(
			d_input_textedit,
			SIGNAL(control_c_pressed(QString)),
			this,
			SLOT(handle_control_c_pressed(QString)));

	QObject::connect(
			verticalScrollBar(),
			SIGNAL(valueChanged(int)),
			this,
			SLOT(reposition_input_widget()));
}


GPlatesQtWidgets::ConsoleTextEdit::~ConsoleTextEdit()
{  }


void
GPlatesQtWidgets::ConsoleTextEdit::append_text(
		const QString &text,
		bool error)
{
	if (text.isEmpty())
	{
		return;
	}

	QTextCursor text_cursor = textCursor();
	text_cursor.movePosition(QTextCursor::End);

	text_cursor.beginEditBlock();

	text_cursor.insertText(text, error ? get_error_text_format() : get_normal_text_format());
	d_on_blank_line = (text.at(text.length() - 1) == '\n');

	text_cursor.endEditBlock();

	scroll_to_bottom();
	reposition_input_widget();
}


void
GPlatesQtWidgets::ConsoleTextEdit::append_text(
		const QString &prompt,
		const QString &text)
{
	QTextCursor text_cursor = textCursor();
	text_cursor.movePosition(QTextCursor::End);

	text_cursor.beginEditBlock();

	if (!d_on_blank_line)
	{
		text_cursor.insertText("\n", get_prompt_format());
	}

	text_cursor.insertText(prompt, get_prompt_format());
	text_cursor.insertText(text, get_command_format());
	d_on_blank_line = !text.isEmpty() && (text.at(text.length() - 1) == '\n');

	text_cursor.endEditBlock();

	scroll_to_bottom();
	reposition_input_widget();
}


void
GPlatesQtWidgets::ConsoleTextEdit::focus_on_input_widget()
{
	scroll_to_bottom();
	reposition_input_widget();
	d_input_textedit->setFocus();
}


void
GPlatesQtWidgets::ConsoleTextEdit::set_input_prompt(
		ConsoleInputTextEdit::Prompt prompt)
{
	d_input_textedit->set_prompt(prompt);
}


void
GPlatesQtWidgets::ConsoleTextEdit::set_input_widget_visible(
		bool visible)
{
	d_input_textedit->setVisible(visible);
	reposition_input_widget();
	d_input_textedit->setFocus();
}


QString
GPlatesQtWidgets::ConsoleTextEdit::get_last_non_blank_line(
		int num_banner_lines) const
{
	QTextBlock block = document()->lastBlock();
	while (block.isValid() && block.blockNumber() >= num_banner_lines)
	{
		QString block_text = block.text();
		if (block_text.trimmed().isEmpty())
		{
			block = block.previous();
		}
		else
		{
			return block_text;
		}
	}

	return QString();
}


void
GPlatesQtWidgets::ConsoleTextEdit::keyPressEvent(
		QKeyEvent *ev)
{
	if (ev->modifiers() == Qt::NoModifier && d_input_textedit->isVisible())
	{
		scroll_to_bottom();
		reposition_input_widget();
		d_input_textedit->setFocus();
		d_input_textedit->handle_key_press_event(ev);
	}
	else
	{
		QPlainTextEdit::keyPressEvent(ev);
	}
}


void
GPlatesQtWidgets::ConsoleTextEdit::resizeEvent(
		QResizeEvent *ev)
{
	QPlainTextEdit::resizeEvent(ev);

	// We must reposition the input widget *after* the base implementation has
	// finished doing its job.
	reposition_input_widget();
}


void
GPlatesQtWidgets::ConsoleTextEdit::mousePressEvent(
		QMouseEvent *ev)
{
	// If the user clicks below the input widget, set focus on it.
	QRect input_geometry = d_input_textedit->geometry();
	if (ev->y() > input_geometry.bottom())
	{
		d_input_textedit->setFocus();
	}
	else
	{
		QPlainTextEdit::mousePressEvent(ev);
	}
}


bool
GPlatesQtWidgets::ConsoleTextEdit::eventFilter(
		QObject *watched,
		QEvent *ev)
{
	if (watched == d_input_textedit && ev->type() == QEvent::FocusIn)
	{
		QTextCursor text_cursor = textCursor();
		text_cursor.movePosition(QTextCursor::End);
		setTextCursor(text_cursor);
	}

	return QPlainTextEdit::eventFilter(watched, ev);
}


void
GPlatesQtWidgets::ConsoleTextEdit::handle_text_changed()
{
	// Handle case when we get cleared.
	if (document()->isEmpty())
	{
		d_on_blank_line = true;
	}
}


void
GPlatesQtWidgets::ConsoleTextEdit::handle_return_pressed(
		QString line)
{
	append_text(d_input_textedit->get_prompt(), line + "\n");
	d_input_textedit->set_text(QString());
	d_console_history->commit_command(line);

	emit return_pressed(line);
}


void
GPlatesQtWidgets::ConsoleTextEdit::handle_up_pressed(
		QString line)
{
	boost::optional<QString> previous_command =
		d_console_history->get_previous_command(line);
	if (previous_command)
	{
		d_input_textedit->set_text(*previous_command);
	}
}


void
GPlatesQtWidgets::ConsoleTextEdit::handle_down_pressed(
		QString line)
{
	boost::optional<QString> next_command =
		d_console_history->get_next_command(line);
	if (next_command)
	{
		d_input_textedit->set_text(*next_command);
	}
}


void
GPlatesQtWidgets::ConsoleTextEdit::handle_control_c_pressed(
		QString line)
{
	append_text(d_input_textedit->get_prompt(), line + "\n");
	d_input_textedit->set_text(QString());
	d_console_history->reset_modifiable_history();

	emit control_c_pressed(line);
}


void
GPlatesQtWidgets::ConsoleTextEdit::reposition_input_widget()
{
	if (!d_input_textedit->isVisible())
	{
		return;
	}

	// Set the width of the input widget to match our width minus the width of
	// the vertical scrollbar, if it is visible.
	int input_widget_height = d_input_textedit->sizeHint().height();
	QScrollBar *scrollbar = verticalScrollBar();
	int input_widget_width = width() - (scrollbar->isVisible() ? scrollbar->width() : 0);
	d_input_textedit->resize(input_widget_width, input_widget_height);

	// The vertical position of the input widget is at the very top if the
	// document is empty. If the document is not empty, it is after the last row
	// of text, except if the last row is empty, in which case it is on top of the last row.
	int input_widget_y;
	if (document()->isEmpty())
	{
		input_widget_y = 0;
	}
	else
	{
		document()->setTextWidth(input_widget_width);
		QTextCursor text_cursor = textCursor();
		text_cursor.movePosition(QTextCursor::End);
		QRect cursor_rect = cursorRect(text_cursor);

		input_widget_y = cursor_rect.y() - d_vertical_padding;
#if 0
		// input_widget_y = cursor_rect.y() + cursor_rect.height();
		input_widget_y = cursor_rect.y() + cursor_rect.height();

		// Subtract that distance inside the input widget from the top of the
		// text to the top of the widget.
		input_widget_y = input_widget_y - d_vertical_padding;
#endif
	}

	d_input_textedit->move(0, input_widget_y);
}


void
GPlatesQtWidgets::ConsoleTextEdit::scroll_to_bottom()
{
	verticalScrollBar()->triggerAction(QAbstractSlider::SliderToMaximum);
}

QWidget *
GPlatesQtWidgets::PythonConsoleDialog::show_cancel_widget()
{
	// Because this dialog is exempt from the event blackout, we need to manually
	// disable a few things.
	d_output_textedit->set_input_widget_visible(false);
	d_output_textedit->setFocus();
	run_script_button->setEnabled(false);
	save_button->setEnabled(false);
	d_disable_close = true;
	
	d_monitor_widget = new PythonExecutionMonitorWidget(
			d_python_execution_thread,
			isVisible() ? static_cast<QWidget *>(this) : d_viewport_window);
	return d_monitor_widget;
}

QWidget *
GPlatesQtWidgets::PythonConsoleDialog::hide_cancel_widget()
{
	QWidget * ret = d_monitor_widget;
	d_output_textedit->set_input_widget_visible(true);
	run_script_button->setEnabled(true);
	save_button->setEnabled(true);
	d_disable_close = false;
	d_monitor_widget->deleteLater();
	d_monitor_widget = NULL;
	return ret;
}


#endif // GPLATES_NO_PYTHON

