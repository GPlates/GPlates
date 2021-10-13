#!/usr/bin/env python
#

import sys
import os
import string
import os.path
import re
import codecs
from optparse import OptionParser

HELP_USAGE = "Usage: list_external_includes.py include_trace_file internal_ignore_path\n"

__usage__ = "%prog [options] [-h --help] root_source_dir root_build_dir [platform]"
__description__ = "Reads Visual Studio 'BuildLog.htm' files build with '/showIncludes' compiler option and "\
                  "writes list of '#include's to '*_pch.h' pre-compiled header files.\n\n"\
                  "'root_source_dir' is root directory of GPlates source tree (has 'src/' as subdirectory).\n"\
                  "'root_build_dir' is root directory of GPlates build tree (same as root source dir if in-place build).\n"\
                  "'platform' currently can only be 'MSVC' (default) and is optional."

class IncludeParser:
    def parse_includes(self, build_type, root_source_dir, root_build_tree_dir):
        pass

class ParserMSVC(IncludeParser):
    def __init__(self):
    
        self.locations = [
            ('src', 'gplates'),
            ('src', 'gplates-cli'),
            ('src', 'gplates-unit-test'),
			('src/api', 'api'),
			('src/app-logic', 'app-logic'),
            ('src/canvas-tools', 'canvas-tools'),
			('src/cli', 'cli'),
			('src/data-mining', 'data-mining'),
            ('src/feature-visitors', 'feature-visitors'),
            ('src/file-io', 'file-io'),
            ('src/global', 'global'),
            ('src/gui', 'gui'),
            ('src/maths', 'maths'),
            ('src/model', 'model'),
            ('src/opengl', 'opengl'),
            ('src/presentation', 'presentation'),
            ('src/property-values', 'property-values'),
            ('src/qt-resources', 'qt-resources'),
            ('src/qt-widgets', 'qt-widgets'),
            ('src/unit-test', 'unit-test'),
            ('src/utils', 'utils'),
            ('src/view-operations', 'view-operations'),
        ]
        
        # Include headers that look different across platforms or
        # that need something like 'extern "C" { }'.
        # First part matches part between angle brackets in '#include <>'
        # and second part is list of lines to replace '#include' statement with.
        # NOTE: make first part lower case.
        self.include_varies_with_platform = [
            ('windows.h', [
                '#ifdef __WINDOWS__',
                '#include <windows.h>',
                '#endif // __WINDOWS__'
                ]),
            ('gl/gl.h', [
                'extern "C" {',
                '#if defined(__APPLE__)',
                '#include <OpenGL/gl.h>',
                '#elif defined(__WINDOWS__)',
                '#include <windows.h>',
                '#include <GL/gl.h>',
                '#else',
                '#include <GL/gl.h>',
                '#endif',
                '}'
                ]),
            ('gl/glu.h', [
                'extern "C" {',
                '#if defined(__APPLE__)',
                '#include <OpenGL/glu.h>',
                '#elif defined(__WINDOWS__)',
                '#include <windows.h>',
                '#include <GL/glu.h>',
                '#else',
                '#include <GL/glu.h>',
                '#endif',
                '}'
            	]),
        ]
        # Convert search part of list to lower case.
        for i in range(len(self.include_varies_with_platform)):
            search, replace = self.include_varies_with_platform[i]
            self.include_varies_with_platform[i] = (search.lower(), replace)
        
        # If any item in list are a substring in '#include' header then surround with 2nd and 3rd entries.
        # NOTE: make lower case.
        self.include_exceptions = [
            # Some linux systems have issues with boost due to bug in anonymous namespaces with pre-compiled headers (see http://gcc.gnu.org/bugzilla/show_bug.cgi?id=10591).
            ('boost/', '#ifdef __WINDOWS__', '#endif // __WINDOWS__'),
            # Causes compiler error on windows platforms when included in precompiled headers.
            ('QtOpenGL/qgl.h', '#if 0', '#endif')
        ]
        # Convert to lower case.
        for i in range(len(self.include_exceptions)):
            search, prefix, suffix = self.include_exceptions[i]
            self.include_exceptions[i] = (search.lower(), prefix, suffix)

    def read_build_log(self, filename):
        """ Reads Visual Studio generated 'BuildLog.htm' file. """
        
        # Let user know if file doesn't exist before we open file and throw exception.
        if not os.path.exists(filename):
            print >>sys.stderr, "File " + filename + " doesn't exist."
        
        # See if file starts with BOM (byte-order marker).
        file_has_bom = 0
        f = open(filename, 'r')
        bom = f.read(3)
        if bom[:2] == codecs.BOM_UTF16_BE:
            encoding = 'utf-16-be'
            file_has_bom = 2
        elif bom[:2] == codecs.BOM_UTF16_LE:
            encoding = 'utf-16-le'
            file_has_bom = 2
        elif bom == codecs.BOM_UTF8:
            encoding = 'utf-8'
            file_has_bom = 3
        else:
            encoding = 'ascii'

        # The Visual Studio 'BuildLog.htm' files are encoded in UTF16 little-endian.
        f = codecs.open(filename, "r", encoding)

        # Remove BOM marker if has one.
        if file_has_bom:
            f.read(file_has_bom)

        lines = f.readlines()

        # Convert from unicode to ascii.
        for line in lines:
            line = line.encode('ascii')

        f.close()
        
        return lines

    def convert_path_to_forward_slashes(self, path):
        path_regexp  = re.compile('\\\\')
        return path_regexp.sub('/', path)

    def convert_msvc_include_to_relative_include(self, msvc_include):
        #  Because MSVC uses '\' in its paths and any '#include' lines in our sources files use '/' in their paths
        # we can easily strip off the include paths
		# An example of an include path being stripped is:
		# "C:\SDK\boost\boost_1_36_0\boost/cstdint.hpp" converted to "boost/cstdint.hpp".
        msvc_include_regexp  = re.compile('\\\\([^\\\\]+)$')
        msvc_include_match = msvc_include_regexp.search(msvc_include)
        if not msvc_include_match:
			# Some includes are not in MSVC format (ie, they only use '/' in their path).
            # Currently these are some boost headers (see bottom of 'cmake/modules/Config_h.cmake') that
            # are in 'src/system-fixes' that indirectly include the real boost installation header files
            # (if installed boost version greater than 1.34).
			# NOTE: Currently we'll need to go through the generated pch files and strip off the include paths by hand.
			# For example, "C:/SDK/boost/boost_1_36_0/boost/cstdint.hpp" needs to be converted to "boost/cstdint.hpp".
            #
            # TODO: Test if 'src/global/config.h' file exists and if so look for those boost include paths and use that
            # as a prefix here that can be removed from the path to make it relative.
            # For example, to remove "C:/SDK/boost/boost_1_36_0/".
			return msvc_include
        
        return msvc_include_match.group(1)

    def output_platform_independent_include(self, include, output_file):
        include_lower = include.lower()
        
        # See if it's in the list of exceptions and if so surround with prefix and suffix lines in the output file.
        for include_exception, include_prefix, include_suffix in self.include_exceptions:
            if string.find(include_lower, include_exception) != -1:
                print >>output_file, include_prefix
                print >>output_file, '#include <' + include + '>'
                print >>output_file, include_suffix
                return

        # See if we need to replace the include with something else.
        for include_search, include_replace in self.include_varies_with_platform:
            if string.find(include_lower, include_search) != -1:
                for line in include_replace:
                    print >>output_file, line
                return

        print >>output_file, '#include <' + include + '>'

    def parse_include(self, input_filename, output_filename, root_source_dir_lower_and_forward_slashes, root_build_tree_dir_lower_and_forward_slashes):
        
        lines = self.read_build_log(input_filename)
        
        output_file = open(output_filename, 'w')
        
        msvc_include_regexp  = re.compile('^Note\: including file\:(\s+)(.*)')
        
        # Use a large number so that the next indent will be less than it.
        indent_of_last_toplevel_external_include = 1000
        
        msvc_include_set = set()
        
        line_number = 0;
        for line in lines:
            line_number += 1
            msvc_include_match = msvc_include_regexp.search(line)
            if msvc_include_match:
                indent = len(msvc_include_match.group(1))-1

                # Some definitions:
                #
                # Internal header - an include header file that is part of the GPlates source code.
                #
                # External header - an include header file that is *not* part of the GPlates source code.
                # These are the system headers and headers of dependency libraries.
                #
                # Internal headers can include both internal and external headers.
                # External headers can only include external headers (external headers don't know about GPlates).
                #
                # A toplevel external include header is an external header file that is included by an internal header
                # or internal ".cc" file. These are the headers we want to put in the PCH file.
                
                # If the indent of the current line is greater than the indent of the last toplevel external include
                # then we are at an external include that is not a toplevel and hence should be ignored.
                # We ignore these non-toplevel external includes because if our PCH file includes the toplevel include file
                # then that will automatically include all its dependency includes anyway. Also we don't want to include
                # header files that are included by the toplevel external include because they are implementation headers
                # that could be dependent on the version of the library or operating system installed on.
                # For example <boost/bimap/bimap.hpp> includes <boost/bimap/detail/user_interface_config.hpp> -
                # the former is a stable interface header filename whereas the latter might not be.
                if indent > indent_of_last_toplevel_external_include:
                    continue

                # If we found an internal path then start looking for external paths at any indent level.
                # Any include path that matches the root source dir or root build directory is considered an
                # internal header and won't be included in the PCH.
                else:
                    msvc_include = string.strip(msvc_include_match.group(2))
                    msvc_include_lower_and_forward_slashes = self.convert_path_to_forward_slashes(msvc_include.lower())
                    
                    # Compare all paths using forward slashes in the pathname.
                    if string.find(msvc_include_lower_and_forward_slashes, root_source_dir_lower_and_forward_slashes) != -1 or \
                        string.find(msvc_include_lower_and_forward_slashes, root_build_tree_dir_lower_and_forward_slashes) != -1:
                        # The current include is an internal include so any external include after this will
                        # be the next toplevel external include.
                        # Use a large number so that the next indent will be less than it.
                        indent_of_last_toplevel_external_include = 1000
                    
                    # If we get here then we're an external include.
                    # See if we've been included by an internal include.
                    # If so then add to the list of include files for our PCH.
                    # We can simply compare the current indent with the indent of the last toplevel external include.
                    # If it's not larger then it means the current external include is a toplevel external include.
                    elif indent <= indent_of_last_toplevel_external_include:
                        indent_of_last_toplevel_external_include = indent
                        # Only need to add each header filename once.
                        if msvc_include not in msvc_include_set:
                            msvc_include_set.add(msvc_include)
                            include = self.convert_msvc_include_to_relative_include(msvc_include)
                            self.output_platform_independent_include(include, output_file)

            # There was no match so we must have reached a new ".cc" file so start anew.
            else:
                # Use a large number so that the next indent will be less than it.
                indent_of_last_toplevel_external_include = 1000
                
        output_file.close()

    def parse_includes(self, build_type, root_source_dir, root_build_tree_dir):
        # Any include path that matches the root source dir or root build directory is considered an internal header and won't be included in the PCH.
        root_source_dir_lower_and_forward_slashes = self.convert_path_to_forward_slashes(os.path.abspath(root_source_dir).lower())
        root_build_tree_dir_lower_and_forward_slashes = self.convert_path_to_forward_slashes(os.path.abspath(root_build_tree_dir).lower())
        
        for base_dir, proj in self.locations:
            input_filename = os.path.join(root_build_tree_dir, base_dir, proj + '.dir', build_type, 'BuildLog.htm')
            output_filename = os.path.join(root_source_dir, base_dir, proj + '_pch.h')
            self.parse_include(input_filename, output_filename, root_source_dir_lower_and_forward_slashes, root_build_tree_dir_lower_and_forward_slashes)

def main(argv):
    # Parse the command-line options. 
    parser = OptionParser(usage = __usage__,
                          description = __description__)
    # Add option MSVC build type used to generated 'BuildLog.htm' files.
    parser.add_option("--msvc-build-type", action="store", type="string",\
                      dest="build_type",\
                      help="MSVC build type used to show included headers "\
                        "('debug', 'release', 'relwithdebinfo' or 'minsizerel' - default is 'debug')")
    # Defaults for options.
    parser.set_defaults(build_type="debug")
    platform = "MSVC"
    
    # Parse command-line options.
    (options, args) = parser.parse_args()
    if len(args) > 3 or len(args) < 2:
        parser.error("incorrect number of arguments")

    root_source_dir = args[0]
    root_build_dir = args[1]
    
    if len(args) == 3:
        platform = args[2]

    if platform == "MSVC":
        include_parser = ParserMSVC()
    else:
        parser.error("currently only 'MSVC' platform is supported")
        
    include_parser.parse_includes(options.build_type, root_source_dir, root_build_dir)

if __name__ == "__main__":
    main( sys.argv )
