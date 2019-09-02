//
// Workaround for a bug in <CGAL/Delaunay_triangulation_2.h>.
//
// http://cgal-discuss.949826.n4.nabble.com/Natural-neighbor-coordinates-infinite-loop-tp4659522p4664137.html
//
// By defining CGAL_DT2_USE_RECURSIVE_PROPAGATE_CONFLICTS we can use an alternative piece of code
// in that file to avoid the bug. The alternate code uses recursive propagation for all depths and
// does not switch to non-recursive code at depth 100. However there is a risk that we could exceed
// our C stack memory doing this if the conflict region is large enough (spans many faces).
// But typical deforming networks in GPlates are not really large enough for this to be a problem.
//
// In any case, this workaround is only employed for CGAL < 4.12.2 and CGAL 4.13.0.
// The bug was fixed in 4.12.2, 4.13.1 and 4.14 (according to the link above).
//
// NOTE: This workaround needs to be placed at the top of ".cc" file to prior to any direct or
//       indirect include of <CGAL/Delaunay_triangulation_2.h>.
//       It should also be placed at the top of the PCH header used for this (app-logic) project.
//
#include <CGAL/config.h>
#if (CGAL_VERSION_NR < CGAL_VERSION_NUMBER(4, 12, 2)) || (CGAL_VERSION_NR == CGAL_VERSION_NUMBER(4, 13, 0))
#	define CGAL_DT2_USE_RECURSIVE_PROPAGATE_CONFLICTS
#endif

#ifdef __WINDOWS__
#include <boost/foreach.hpp>
#endif // __WINDOWS__
#include <vector>
#ifdef __WINDOWS__
#include <boost/optional.hpp>
#endif // __WINDOWS__
#ifdef __WINDOWS__
#include <boost/shared_ptr.hpp>
#endif // __WINDOWS__
#include <QString>
#include <map>
#ifdef __WINDOWS__
#include <boost/mpl/assert.hpp>
#endif // __WINDOWS__
#ifdef __WINDOWS__
#include <boost/mpl/contains.hpp>
#endif // __WINDOWS__
#ifdef __WINDOWS__
#include <boost/utility/enable_if.hpp>
#endif // __WINDOWS__
#ifdef __WINDOWS__
#include <boost/operators.hpp>
#endif // __WINDOWS__
#ifdef __WINDOWS__
#include <boost/weak_ptr.hpp>
#endif // __WINDOWS__
#ifdef __WINDOWS__
#include <boost/tuple/tuple.hpp>
#endif // __WINDOWS__
#ifdef __WINDOWS__
#include <boost/noncopyable.hpp>
#endif // __WINDOWS__
#include <QObject>
#include <QFileInfo>
#include <QMap>
#ifdef __WINDOWS__
#include <boost/any.hpp>
#endif // __WINDOWS__
#ifdef __WINDOWS__
#include <boost/scoped_ptr.hpp>
#endif // __WINDOWS__
#include <set>
#ifdef __WINDOWS__
#include <boost/intrusive_ptr.hpp>
#endif // __WINDOWS__
#ifdef __WINDOWS__
#include <boost/config.hpp>
#endif // __WINDOWS__
#ifdef __WINDOWS__
#include <boost/assert.hpp>
#endif // __WINDOWS__
#ifdef __WINDOWS__
#include <boost/detail/workaround.hpp>
#endif // __WINDOWS__
#ifdef __WINDOWS__
#include <boost/function.hpp>
#endif // __WINDOWS__
#include <ctime>
#ifdef __WINDOWS__
#include <boost/cstdint.hpp>
#endif // __WINDOWS__
#include <QStringList>
#include <QLocale>
#ifdef __WINDOWS__
#include <boost/mpl/vector.hpp>
#endif // __WINDOWS__
#ifdef __WINDOWS__
#include <boost/variant.hpp>
#endif // __WINDOWS__
#ifdef __WINDOWS__
#include <boost/iterator/transform_iterator.hpp>
#endif // __WINDOWS__
#ifdef __WINDOWS__
#include <boost/lambda/construct.hpp>
#endif // __WINDOWS__
#ifdef __WINDOWS__
#include <boost/math/special_functions/fpclassify.hpp>
#endif // __WINDOWS__
#ifdef __WINDOWS__
#include <boost/type_traits/add_const.hpp>
#endif // __WINDOWS__
#ifdef __WINDOWS__
#include <boost/type_traits/remove_const.hpp>
#endif // __WINDOWS__
#ifdef __WINDOWS__
#include <boost/bind.hpp>
#endif // __WINDOWS__
#include <iostream>
#ifdef __WINDOWS__
#include <boost/scoped_array.hpp>
#endif // __WINDOWS__
#include <QRect>
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
#include <QColor>
#include <QVariant>
#include <QSettings>
#include <QPointer>
#include <QList>
#include <QCoreApplication>
#include <QSet>
#include <QDateTime>
#include <iterator>
#include <list>
#include <cstddef>
#include <utility>
#include <QUrl>
#include <QtGlobal>
#include <stack>
#include <QDebug>
#include <bitset>
#include <cmath>
#include <limits>
#ifdef __WINDOWS__
#include <boost/none.hpp>
#endif // __WINDOWS__
#ifdef __WINDOWS__
#include <boost/type_traits/remove_pointer.hpp>
#endif // __WINDOWS__
#include <CGAL/Origin.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Triangulation_2.h>
#include <CGAL/Constrained_Delaunay_triangulation_2.h>
#include <CGAL/Delaunay_triangulation_2.h>
#include <CGAL/Delaunay_triangulation_3.h>
#include <CGAL/Triangulation_hierarchy_3.h>
#include <CGAL/Triangulation_conformer_2.h>
#include <CGAL/Delaunay_mesher_2.h>
#include <CGAL/Delaunay_mesh_face_base_2.h>
#include <CGAL/Delaunay_mesh_size_criteria_2.h>
#include <CGAL/Interpolation_traits_2.h>
#include <CGAL/natural_neighbor_coordinates_2.h>
#include <CGAL/surface_neighbor_coordinates_3.h>
#include <CGAL/interpolation_functions.h>
#include <CGAL/Interpolation_gradient_fitting_traits_2.h>
#include <CGAL/sibson_gradient_fitting.h>
#include <QtAlgorithms>
#include <algorithm>
#ifdef __WINDOWS__
#include <boost/cast.hpp>
#endif // __WINDOWS__
#include <string>
#ifdef __WINDOWS__
#include <boost/checked_delete.hpp>
#endif // __WINDOWS__
#include <deque>
#include <ostream>
#ifdef __WINDOWS__
#include <boost/static_assert.hpp>
#endif // __WINDOWS__
#include <functional>
#ifdef __WINDOWS__
#include <boost/lambda/bind.hpp>
#endif // __WINDOWS__
#ifdef __WINDOWS__
#include <boost/lambda/lambda.hpp>
#endif // __WINDOWS__
#ifdef __WINDOWS__
#include <boost/mpl/set.hpp>
#endif // __WINDOWS__
#include <QProgressBar>
#include <memory>
#include <ogrsf_frmts.h>
#include <sstream>
#ifdef __WINDOWS__
#include <boost/range/concepts.hpp>
#endif // __WINDOWS__
#ifdef __WINDOWS__
#include <boost/variant/get.hpp>
#endif // __WINDOWS__
#include <Qt>
#include <QListView>
#include <QComboBox>
#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QDialog>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QGridLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QListWidget>
#include <QtGui/QPushButton>
#include <QtGui/QRadioButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QTableWidget>
#include <QtGui/QVBoxLayout>
#include <QWidget>
