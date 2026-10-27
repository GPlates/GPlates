#
# Downloads the source archives the pyGPlates wheel dependencies are built from, checking each
# against the SHA-256 pinned beside its version in 'versions.sh'. Sourced - not executed - by the
# Linux image ('manylinux_2_28.dockerfile', in each build step), and by the macOS and Windows
# dependency builds ('build_macos_deps.sh', 'build_windows_deps.sh'):
#
#   . "$(dirname "$0")/download.sh"
#
# Without the checksum, every wheel would compile whatever the download host served into
# published wheels.
#
# Keep this file to plain sh, plus 'local' (which bash and dash both accept) - the Docker build
# steps source it with /bin/sh.

# Download an archive and extract it into the current directory:
#
#   download_and_extract <sha256> "<url> [<url> ...]" <tar options>
#
# eg, 'download_and_extract "${CGAL_SHA256}" "https://..." xJ --strip-components=1'.
#
# The URLs are tried in turn until one serves an archive with the expected SHA-256. A list,
# because one host being down would otherwise fail a long job: ftp.gnu.org timed out twice, and
# on the second occasion ftpmirror.gnu.org (GNU's mirror redirector) was down with it - both are
# on the FSF's own network - so a fallback has to be on another network (eg, mirrors.kernel.org).
# With the checksum, any host is as safe as the project's own.
#
# Each URL is itself retried. Plain '--retry' covers timeouts and HTTP 408/429/500/502-504, and
# '--retry-connrefused' adds refused connections ('--retry-all-errors' would cover more, but needs
# curl 7.71, and the Linux image's EL8 has 7.61). The timeouts are what let a dead host give way
# to the next one: curl's own connect timeout is minutes, which over six attempts would spend half
# an hour on one host, and it has no transfer timeout at all, so a host that stalls mid-transfer
# would hang the job - '--speed-limit'/'--speed-time' abort (and retry) a transfer that has slowed
# to under 1 KB/s for a minute. '--fail' makes an HTTP error page an error, rather than something
# to checksum. The archive goes to a file, not straight into tar, because a retry after a partial
# transfer would feed tar the start of the archive a second time - and because it must be checked
# before anything is extracted.
#
# The file is DOWNLOAD_ARCHIVE if the sourcing script sets it, a fixed path either way, so a
# failed download is overwritten by the next run rather than left behind. Tar reads it from
# stdin, since GNU tar takes a 'C:/...' archive name for a remote host.
download_and_extract() {
    local expected_sha256=$1
    local urls=$2
    shift 2
    local archive=${DOWNLOAD_ARCHIVE:-/tmp/pygplates-download.tmp}
    local url actual_sha256

    # An unset pin (eg, a new platform with no checksum for its sccache build) would otherwise
    # report every host as serving the wrong archive.
    if [ -z "${expected_sha256}" ]; then
        echo "error: no SHA-256 pinned for ${urls}" >&2
        return 1
    fi

    for url in ${urls}; do
        rm -f "${archive}"
        if ! curl -fsSL --retry 5 --retry-connrefused --retry-delay 10 --connect-timeout 30 \
                --speed-limit 1024 --speed-time 60 -o "${archive}" "${url}"; then
            echo "warning: downloading ${url} failed" >&2
            continue
        fi
        # macOS has no 'sha256sum'.
        if command -v sha256sum > /dev/null 2>&1; then
            actual_sha256=$(sha256sum < "${archive}" | cut -d ' ' -f 1)
        else
            actual_sha256=$(shasum -a 256 < "${archive}" | cut -d ' ' -f 1)
        fi
        if [ "${actual_sha256}" != "${expected_sha256}" ]; then
            echo "warning: ${url} has SHA-256 ${actual_sha256}, but ${expected_sha256} is pinned" >&2
            continue
        fi
        # (The failure is returned explicitly, because the Docker build steps chain commands with
        #  '&&' rather than running under 'set -e'.)
        if ! tar "$@" -f - < "${archive}"; then
            rm -f "${archive}"
            return 1
        fi
        rm -f "${archive}"
        return 0
    done

    rm -f "${archive}"
    echo "error: no URL served the pinned archive (SHA-256 ${expected_sha256}): ${urls}" >&2
    return 1
}
