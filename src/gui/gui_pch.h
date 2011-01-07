#include <iosfwd>
#ifdef __WINDOWS__
#include <windows.h>
#endif // __WINDOWS__
extern "C" {
#if defined(__APPLE__)
#include <OpenGL/gl.h>
#elif defined(__WINDOWS__)
#include <windows.h>
#include <GL/gl.h>
#else
#include <GL/gl.h>
#endif
}
extern "C" {
#if defined(__APPLE__)
#include <OpenGL/glu.h>
#elif defined(__WINDOWS__)
#include <windows.h>
#include <GL/glu.h>
#else
#include <GL/glu.h>
#endif
}
#ifdef __WINDOWS__
#include <boost/intrusive_ptr.hpp>
#endif // __WINDOWS__
#include <vector>
#include <string>
#include <map>
#include <QString>
#ifdef __WINDOWS__
#include <boost/optional.hpp>
#endif // __WINDOWS__
#include <set>
#ifdef __WINDOWS__
#include <boost/config.hpp>
#endif // __WINDOWS__
#ifdef __WINDOWS__
#include <boost/assert.hpp>
#endif // __WINDOWS__
#ifdef __WINDOWS__
#include <boost/detail/workaround.hpp>
#endif // __WINDOWS__
#include <iostream>
#include <cmath>
#include <list>
#ifdef __WINDOWS__
#include <boost/current_function.hpp>
#endif // __WINDOWS__
#include <QtCore/QTimer>
#include <QCloseEvent>
#include <QStringList>
#include <QUndoGroup>
#include <QFileInfo>
#ifdef __WINDOWS__
#include <boost/shared_ptr.hpp>
#endif // __WINDOWS__
#include <sstream>
#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QDockWidget>
#include <QtGui/QHBoxLayout>
#include <QtGui/QMainWindow>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
#include <QtGui/QStatusBar>
#include <QtGui/QTabWidget>
#include <QtGui/QTableView>
#include <QtGui/QToolBar>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>
#if 0
#include <QtOpenGL/qgl.h>
#endif
#ifdef __WINDOWS__
#include <boost/scoped_ptr.hpp>
#endif // __WINDOWS__
#include <QObject>
#ifdef __WINDOWS__
#include <boost/none.hpp>
#endif // __WINDOWS__
#include <QWidget>
#include <QtGui/QDoubleSpinBox>
#include <QtGui/QFrame>
#include <QtGui/QGridLayout>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QSlider>
#include <QtGui/QSpacerItem>
#include <QDialog>
#include <QtGui/QDialog>
#include <QtGui/QSpinBox>
#include <QTimer>
#include <QtGui/QCheckBox>
#include <QtGui/QGroupBox>
#include <QtGui/QTextEdit>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QLineEdit>
#include <QtGui/QTreeWidget>
#include <QtGui/QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QAbstractTableModel>
#include <QItemSelection>
#include <QHeaderView>
#include <QtGui/QTableWidget>
#include <QDebug>
#include <QTreeWidget>
#include <QUndoStack>
#include <QGridLayout>
#include <Qt>
#include <QFileDialog>
#include <QPainter>
#include <QSvgGenerator>
#include <limits>
#include <cstdlib>
#include <queue>
#include <functional>
#include <algorithm>
#include <QApplication>
#include <QLocale>
#include <QFont>
#include <QFontMetrics>
#include <QVariant>
#include <fstream>
#include <QMessageBox>
#include <QTableWidget>
