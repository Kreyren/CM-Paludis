#!/usr/bin/env bash
# vim: set sw=4 sts=4 et :

# Copyright (c) 2006, 2007 Ciaran McCreesh
#
# Based in part upon ebuild.sh from Portage, which is Copyright 1995-2005
# Gentoo Foundation and distributed under the terms of the GNU General
# Public License v2.
#
# This file is part of the Paludis package manager. Paludis is free software;
# you can redistribute it and/or modify it under the terms of the GNU General
# Public License, version 2, as published by the Free Software Foundation.
#
# Paludis is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc., 59 Temple
# Place, Suite 330, Boston, MA  02111-1307  USA

pkg_postinst()
{
    :
}

ebuild_f_postinst()
{
    local old_sandbox_write="${SANDBOX_WRITE}"
    [[ -z "${PALUDIS_DO_NOTHING_SANDBOXY}" ]] && SANDBOX_WRITE="${SANDBOX_WRITE+${SANDBOX_WRITE}:}${ROOT%/}/"

    if hasq "postinst" ${SKIP_FUNCTIONS} ; then
        ebuild_section "Skipping pkg_postinst (SKIP_FUNCTIONS)"
    else
        if [[ $(type -t pre_pkg_postinst ) == "function" ]] ; then
            ebuild_section "Starting pre_pkg_postinst"
            pre_pkg_postinst
            ebuild_section "Done pre_pkg_postinst"
        fi

        ebuild_section "Starting pkg_postinst"
        pkg_postinst
        ebuild_section "Done pkg_postinst"

        if [[ $(type -t post_pkg_postinst ) == "function" ]] ; then
            ebuild_section "Starting post_pkg_postinst"
            post_pkg_postinst
            ebuild_section "Done post_pkg_postinst"
        fi
    fi

    [[ -z "${PALUDIS_DO_NOTHING_SANDBOXY}" ]] && SANDBOX_WRITE="${old_sandbox_write}"
    true
}

