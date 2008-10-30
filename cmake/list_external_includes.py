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
            ('src', 'gplates-no-gui'),
            ('src/canvas-tools', 'canvas-tools'),
            ('src/feature-visitors', 'feature-visitors'),
            ('src/file-io', 'file-io'),
            ('src/global', 'global'),
            ('src/gui', 'gui'),
            ('src/maths', 'maths'),
            ('src/model', 'model'),
            ('src/property-values', 'property-values'),
            ('src/qt-resources', 'qt-resources'),
            ('src/qt-widgets', 'qt-widgets'),
            ('src/utils', 'utils'),
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

    def convert_msvc_include(self, msvc_include):
        #  Because MSVC uses '\' in its paths and any '#include' lines in our sources files use '/' in their paths
        # we can easily strip off the include paths
        msvc_include_regexp  = re.compile('\\\\([^\\\\]+)$')
        msvc_include_match = msvc_include_regexp.search(msvc_include)
        if not msvc_include_match:
            raise SyntaxError, "Line cannot be parsed '%s'" % msvc_include
        
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

    def parse_include(self, input_filename, output_filename, root_source_dir_lower, root_build_tree_dir_lower):
        
        lines = self.read_build_log(input_filename)
        
        output_file = open(output_filename, 'w')
        
        msvc_include_regexp  = re.compile('^Note\: including file\:(\s+)(.*)')
        
        last_indent_first_external_include = 1000
        
        msvc_include_set = set()
        
        line_number = 0;
        for line in lines:
            line_number += 1
            msvc_include_match = msvc_include_regexp.search(line)
            if msvc_include_match:
                indent = len(msvc_include_match.group(1))-1
                msvc_include = string.strip(msvc_include_match.group(2))
                
                # If we found an internal path then start looking for external paths at any indent level.
                # Any include path that matches the root source dir or root build directory is considered an internal header and won't be included in the PCH.
                msvc_include_lower = msvc_include.lower()
                if string.find(msvc_include_lower, root_source_dir_lower) != -1 or \
                    string.find(msvc_include_lower, root_build_tree_dir_lower) != -1:
                    last_indent_first_external_include = 1000
                
                # If we're an external include and we've been included by an internal include then print to output.
                elif indent <= last_indent_first_external_include:
                    last_indent_first_external_include = indent
                    if msvc_include not in msvc_include_set:
                        msvc_include_set.add(msvc_include)
                        include = self.convert_msvc_include(msvc_include)
                        self.output_platform_independent_include(include, output_file)
            else:
                last_indent_first_external_include = 1000
                
        output_file.close()

    def parse_includes(self, build_type, root_source_dir, root_build_tree_dir):
        # Any include path that matches the root source dir or root build directory is considered an internal header and won't be included in the PCH.
        root_source_dir_lower = os.path.abspath(root_source_dir).lower()
        root_build_tree_dir_lower = os.path.abspath(root_build_tree_dir).lower()
        
        for base_dir, proj in self.locations:
            input_filename = os.path.join(root_build_tree_dir, base_dir, proj + '.dir', build_type, 'BuildLog.htm')
            output_filename = os.path.join(root_source_dir, base_dir, proj + '_pch.h')
            self.parse_include(input_filename, output_filename, root_source_dir_lower, root_build_tree_dir_lower)

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
