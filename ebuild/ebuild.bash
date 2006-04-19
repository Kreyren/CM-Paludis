#!/bin/bash
# vim: set sw=4 sts=4 et :

# Copyright (c) 2006 Ciaran McCreesh <ciaran.mccreesh@blueyonder.co.uk>
#
# Based in part upon ebuild.sh from Portage, which is Copyright 1995-2005
# Gentoo Foundation and distributed under the terms of the GNU General
# Public License v2.
#
# This file is part of the Paludis package manager. Paludis is free software;
# you can redistribute it and/or modify it under the terms of the GNU General
# Public License as published by the Free Software Foundation; either version
# 2 of the License, or (at your option) any later version.
#
# Paludis is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc., 59 Temple
# Place, Suite 330, Boston, MA  02111-1307  USA

unalias -a
unset GZIP BZIP BZIP2 CDPATH GREP_OPTIONS GREP_COLOR GLOBIGNORE
eval unset LANG ${!LC_*}

shopt -s expand_aliases

EBUILD_KILL_PID=$$
alias die='diefunc "$FUNCNAME" "$LINENO"'
alias assert='_pipestatus="${PIPESTATUS[*]}"; [[ -z "${_pipestatus//[ 0]/}" ]] || diefunc "$FUNCNAME" "$LINENO" "$_pipestatus"'
trap 'echo "die trap: exiting with error." 1>&2 ; exit 250' 15

export EBUILD_PROGRAM_NAME="$0"

diefunc()
{
    local func="$1" line="$2"
    shift 2
    echo 1>&2
    echo "ERROR in ${CATEGORY:-?}/${PF:-?}:" 1>&2
    echo "!!! In ${func:-?} at line ${line:-?}" 1>&2
    echo "!!! ${*:-(no message provided)}" 1>&2
    echo 1>&2

    kill ${EBUILD_KILL_PID}
    exit 249
}

EBUILD_MODULES_DIR=$(readlink -f $(dirname $0 ) )
[[ -d ${EBUILD_MODULES_DIR} ]] || die "${EBUILD_MODULES_DIR} is not a directory"
export PALUDIS_EBUILD_MODULES_DIR="${EBUILD_MODULES_DIR}"

ebuild_load_module()
{
    source "${EBUILD_MODULES_DIR}/${1}.bash" || die "Error loading module ${1}"
}

source /sbin/functions.sh || die "Couldn't source functions.sh"
ebuild_load_module echo_functions
ebuild_load_module sandbox_stubs
ebuild_load_module portage_stubs
ebuild_load_module list_functions
ebuild_load_module multilib_functions
ebuild_load_module install_functions
ebuild_load_module build_functions
ebuild_load_module unpack_functions
ebuild_load_module eclass_functions

ebuild_source_profile()
{
    if [[ -f ${1}/parent ]] ; then
        while read line; do
            ebuild_source_profile $(readlink -f ${1}/${line} )
        done <${1}/parent
    fi

    if [[ -f ${1}/make.defaults ]] ; then
        eval "$(sed -e 's/^\([a-zA-Z0-9\-_]\+=\)/export \1/' ${1}/make.defaults )" || die "Couldn't source ${1}/make.defaults"
    fi

    if [[ -f ${1}/bashrc ]] ; then
        source ${1}/bashrc || die "Couldn't source ${1}/bashrc"
    fi
}

save_vars="USE USE_EXPAND"

for var in ${save_vars} ; do
    eval "export save_var_${var}='${!var}'"
done

if [[ -n "${PALUDIS_PROFILE_DIR}" ]] ; then
    ebuild_source_profile $(readlink -f "${PALUDIS_PROFILE_DIR}")
fi

unset ${save_vars}

for f in ${PALUDIS_BASHRC_FILES} ; do
    if [[ -f ${f} ]] ; then
        ebuild_notice "debug" "Loading bashrc file ${f}"
        source ${f}
    else
        ebuild_notice "debug" "Skipping bashrc file ${f}"
    fi
done

for var in ${save_vars}; do
    if [[ -n ${!var} ]]; then
        die "${var} should not be set in bashrc. Aborting."
    fi
done

for var in ${save_vars} ; do
    eval "export ${var}=\${save_var_${var}}"
done

ebuild_load_ebuild()
{
    export EBUILD="${1}"
    if [[ "${CATEGORY}" == "virtual" ]] ; then
        if [[ -f "${1}" ]] ; then
            source ${1} || die "Error sourcing ebuild '${1}'"
        elif [[ -e "${1}" ]] ; then
            die "'${1}' exists but is not a regular file"
        fi
    else
        [[ -f "${1}" ]] || die "Ebuild '${1}' is not a file"
        source ${1} || die "Error sourcing ebuild '${1}'"
    fi
    [[ ${RDEPEND-unset} == "unset" ]] && RDEPEND="${DEPEND}"

    IUSE="${IUSE} ${E_IUSE}"
    DEPEND="${DEPEND} ${E_DEPEND}"
    RDEPEND="${RDEPEND} ${E_RDEPEND}"
    PDEPEND="${PDEPEND} ${E_PDEPEND}"
    KEYWORDS="${KEYWORDS} ${E_KEYWORDS}"
}

ebuild_main()
{
    local action ebuild="$1"
    shift

    for action in $@ ; do
        case ${action} in
            metadata)
                ebuild_load_module depend
                ;;

            init|fetch|merge|unmerge|tidyup|updateenv|strip)
                ebuild_load_module builtin_${action}
            ;;

            unpack|compile|install|test)
                ebuild_load_module src_${action}
            ;;

            setup|config|nofetch|preinst|postinst|prerm|postrm)
                ebuild_load_module pkg_${action}
            ;;

            *)
                ebuild_load_module usage_error
                ebuild_f_usage_error "Unknown action '${action}'"
                exit 1
            ;;
        esac
    done

    if [[ $1 == metadata ]] ; then
        for f in cut tr ; do
            eval "export ebuild_real_${f}=\"$(which $f )\""
            eval "${f}() { ebuild_notice qa 'global scope ${f}' ; $(which $f ) \"\$@\" ; }"
        done
        PATH="" ebuild_load_ebuild "${ebuild}"
        ebuild_f_depend || die "${1} failed"
    else
        ebuild_load_ebuild "${ebuild}"
        for action in $@ ; do
            ebuild_f_${action} || die "${action} failed"
            if [[ ${action} == "init" ]] ; then
                ebuild_load_ebuild "${ebuild}"
            fi
        done
    fi
}

ebuild_main $@

