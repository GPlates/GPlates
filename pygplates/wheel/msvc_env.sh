#
# Puts the MSVC toolchain (cl, nmake, the Windows SDK headers and import libraries) into the
# environment of the shell that sources this file. Sourced - not executed:
#
#   . "$(dirname "$0")/msvc_env.sh"
#
# The Windows dependency builds need it because they are not CMake builds: Boost's b2 and Qwt's
# qmake/nmake compile with whatever 'cl' the environment provides. The pyGPlates wheel build
# needs it too, since it moved from the Visual Studio generator (which locates Visual Studio by
# itself) to Ninja (which does not) - but a cibuildwheel hook cannot set the environment for the
# CMake build that follows it, so that comes from the shell cibuildwheel is started from: in CI
# the workflow imports this file's results into the whole job (see 'build-wheels.yml'), and a
# local wheel build starts from an "x64 Native Tools Command Prompt" instead.
#
# Visual Studio provides the environment as a batch file, 'vcvarsall.bat', which is only usable
# from a cmd shell - so it is run in one, and the environment it produces is imported back into
# this shell. Doing that here (rather than requiring a "x64 Native Tools Command Prompt") is
# what lets 'build_windows_deps.sh' and 'build_boost_python.sh' be run by cibuildwheel, whose
# hooks run in a plain cmd shell.
#
# Skipped when the environment is already set up (eg, when the scripts *are* run from a
# developer command prompt, when the CI workflow set one up for Ninja before starting
# cibuildwheel, or when a wheel build's 'before-build' hook follows 'before-all' in one
# already-configured shell). Either way the environment is checked before this file returns.

# The variables vcvarsall.bat sets that the builds actually read (besides PATH, which needs
# per-shell translation and so is handled on its own below). One list, exported, so the CI
# workflow's environment step hands the job exactly the set imported here - a second copy of
# this list would drift silently (the workflow's export of VCINSTALLDIR makes this file's
# import a no-op there, so its copy would be the one CI builds actually got).
export MSVC_ENV_VARS="INCLUDE LIB LIBPATH UCRTVersion VCToolsVersion VSINSTALLDIR WindowsSdkDir WindowsSDKVersion VCINSTALLDIR VSCMD_ARG_TGT_ARCH"

# ...but only if that environment targets x64, which is what everything built here is. An "x86
# Native Tools Command Prompt" also sets VCINSTALLDIR, and taking it would build Boost and Qwt
# 32-bit while 'b2 address-model=64' and the wheel expect 64-bit - which does not fail here, it
# fails much later at link, saying nothing about the shell it came from. So say it now instead.
# (An environment set up some other way may not set VSCMD_ARG_TGT_ARCH at all; that is not
# something to refuse over, so only an explicitly non-x64 target is rejected.)
if [ -n "${VCINSTALLDIR:-}" ] && [ "${VSCMD_ARG_TGT_ARCH:-x64}" != x64 ]; then
    echo "error: this shell's MSVC environment targets ${VSCMD_ARG_TGT_ARCH}, but everything here is built for x64." >&2
    echo "       Use an x64 Native Tools Command Prompt, or a plain shell (this file sets the environment up itself)." >&2
    return 1
fi

if [ -z "${VCINSTALLDIR:-}" ]; then

    # 'vswhere' ships with every Visual Studio 2017+ installation, at a fixed location, and
    # reports where Visual Studio actually is. ('-products *' so Build Tools installations -
    # which have no IDE - are found too, and '-requires' so an installation without the C++
    # toolset is not selected.)
    _vswhere="/c/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe"
    if [ ! -x "${_vswhere}" ]; then
        echo "error: ${_vswhere} not found - is Visual Studio (or Build Tools) installed?" >&2
        return 1
    fi
    _vs_install=$("${_vswhere}" -latest -products '*' \
        -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 \
        -property installationPath | tr -d '\r')
    if [ -z "${_vs_install}" ]; then
        echo "error: no Visual Studio installation with the C++ x64 toolset was found" >&2
        return 1
    fi

    # Run 'vcvarsall.bat' in a cmd shell and import the environment it leaves behind. The batch
    # file's own output goes to nul so that only 'set' output is read.
    #
    # The two commands go into a batch file of ours rather than onto cmd's command line, because
    # the Visual Studio path contains spaces and so would have to be quoted there - and quotes
    # bound for cmd do not survive the trip. A Unix-side shell quotes the arguments it hands to a
    # Windows program by the convention the C runtime parses, which cmd does not follow: the
    # quotes arrive as a literal \" and cmd reports that it cannot find the command. Nothing on
    # the command line below needs quoting: the batch file is named by a bare name, run from its
    # own directory. (It is written with CRLF line endings, as batch files are.)
    _vcvarsall=$(cygpath --windows "${_vs_install}/VC/Auxiliary/Build/vcvarsall.bat")
    _vcvars_dir=$(mktemp -d)
    printf '@call "%s" x64 > nul\r\n@set\r\n' "${_vcvarsall}" > "${_vcvars_dir}/vcvars.bat"

    # PATH is translated to the Unix form this shell needs; the rest stay Windows paths, because
    # they are read by the compiler and the linker rather than by the shell. (cmd's CRLF line
    # endings are stripped, since the values end up in Unix-side variables.)
    #
    # VCINSTALLDIR and VSCMD_ARG_TGT_ARCH are what make a second sourcing of this file a no-op.
    # Nothing here guarantees the order they arrive in - cmd's 'set' prints alphabetically, so
    # VCINSTALLDIR in fact comes early - which is why an import that ends up incomplete is caught
    # by the check for 'cl' below rather than by the guard.
    while IFS='=' read -r _name _value; do
        case "${_name}" in
            PATH)
                export PATH="$(cygpath --unix --path "${_value}")" ;;
            *)
                # The space-padded 'case' is a containment test: import _name only if it is
                # one of the MSVC_ENV_VARS words.
                case " ${MSVC_ENV_VARS} " in
                    *" ${_name} "*) export "${_name}=${_value}" ;;
                esac ;;
        esac
    done < <(cd "${_vcvars_dir}" && cmd //c vcvars.bat | tr -d '\r')

    rm -rf "${_vcvars_dir}"

    unset _vswhere _vs_install _vcvarsall _vcvars_dir _name _value
fi

# Outside the block above, so it checks the environment that was already there as well as the one
# just imported. The CI workflow sets one up before running cibuildwheel (Ninja compiles with
# whatever 'cl' it finds), and that path used to go entirely unverified - VCINSTALLDIR alone was
# taken as proof of a working toolchain.
if ! command -v cl > /dev/null; then
    echo "error: the MSVC environment names a Visual Studio installation but 'cl' is not on PATH" >&2
    return 1
fi
