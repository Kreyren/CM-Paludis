/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2006, 2007 Ciaran McCreesh <ciaranm@ciaranm.org>
 *
 * This file is part of the Paludis package manager. Paludis is free software;
 * you can redistribute it and/or modify it under the terms of the GNU General
 * Public License version 2, as published by the Free Software Foundation.
 *
 * Paludis is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <paludis/about.hh>
#include <paludis/repositories/gentoo/ebuild.hh>
#include <paludis/util/system.hh>
#include <paludis/util/strip.hh>
#include <paludis/util/pstream.hh>
#include <paludis/util/log.hh>
#include <paludis/environment.hh>
#include <paludis/config_file.hh>
#include <paludis/portage_dep_parser.hh>
#include <sys/resource.h>
#include <sys/time.h>
#include <unistd.h>

/** \file
 * Implementation for ebuild.hh things.
 *
 * \ingroup grpebuildinterface
 */

using namespace paludis;

#include <paludis/repositories/gentoo/ebuild-se.cc>
#include <paludis/repositories/gentoo/ebuild-sr.cc>

EbuildCommand::EbuildCommand(const EbuildCommandParams & p) :
    params(p)
{
}

EbuildCommand::~EbuildCommand()
{
}

bool
EbuildCommand::success()
{
    return true;
}

bool
EbuildCommand::use_sandbox() const
{
    return true;
}

bool
EbuildCommand::failure()
{
    return false;
}

bool
EbuildCommand::operator() ()
{
    Command cmd(getenv_with_default("PALUDIS_EBUILD_DIR", LIBEXECDIR "/paludis") +
            "/ebuild.bash '" +
            stringify(params.ebuild_dir) + "/" +
            stringify(params.db_entry->name.package) + "-" +
            stringify(params.db_entry->version) +
            ".ebuild' " + commands());

    if (use_sandbox())
        cmd.with_sandbox();

    cmd = extend_command(cmd
            .with_setenv("P", stringify(params.db_entry->name.package) + "-" +
                stringify(params.db_entry->version.remove_revision()))
            .with_setenv("PV", stringify(params.db_entry->version.remove_revision()))
            .with_setenv("PR", stringify(params.db_entry->version.revision_only()))
            .with_setenv("PN", stringify(params.db_entry->name.package))
            .with_setenv("PVR", stringify(params.db_entry->version))
            .with_setenv("PF", stringify(params.db_entry->name.package) + "-" +
                stringify(params.db_entry->version))
            .with_setenv("CATEGORY", stringify(params.db_entry->name.category))
            .with_setenv("REPOSITORY", stringify(params.db_entry->repository))
            .with_setenv("FILESDIR", stringify(params.files_dir))
            .with_setenv("ECLASSDIR", stringify(*params.eclassdirs->begin()))
            .with_setenv("ECLASSDIRS", join(params.eclassdirs->begin(),
                    params.eclassdirs->end(), " "))
            .with_setenv("PORTDIR", stringify(params.portdir))
            .with_setenv("DISTDIR", stringify(params.distdir))
            .with_setenv("PKGMANAGER", PALUDIS_PACKAGE "-" + stringify(PALUDIS_VERSION_MAJOR) + "." +
                stringify(PALUDIS_VERSION_MINOR) + "." +
                stringify(PALUDIS_VERSION_MICRO) +
                (std::string(PALUDIS_SUBVERSION_REVISION).empty() ?
                 std::string("") : "-r" + std::string(PALUDIS_SUBVERSION_REVISION)))
            .with_setenv("PALUDIS_TMPDIR", stringify(params.buildroot))
            .with_setenv("PALUDIS_CONFIG_DIR", SYSCONFDIR "/paludis/")
            .with_setenv("PALUDIS_BASHRC_FILES", params.environment->bashrc_files())
            .with_setenv("PALUDIS_HOOK_DIRS", params.environment->hook_dirs())
            .with_setenv("PALUDIS_FETCHERS_DIRS", params.environment->fetchers_dirs())
            .with_setenv("PALUDIS_SYNCERS_DIRS", params.environment->syncers_dirs())
            .with_setenv("PALUDIS_COMMAND", params.environment->paludis_command())
            .with_setenv("KV", kernel_version())
            .with_setenv("PALUDIS_EBUILD_LOG_LEVEL", stringify(Log::get_instance()->log_level()))
            .with_setenv("PALUDIS_EBUILD_DIR", getenv_with_default("PALUDIS_EBUILD_DIR", LIBEXECDIR "/paludis")));

    if (do_run_command(add_portage_vars(cmd)))
        return success();
    else
        return failure();
}

Command
EbuildCommand::add_portage_vars(const Command & cmd) const
{
    return Command(cmd)
        .with_setenv("PORTAGE_ACTUAL_DISTDIR", stringify(params.distdir))
        .with_setenv("PORTAGE_BASHRC", "/dev/null")
        .with_setenv("PORTAGE_BUILDDIR", stringify(params.buildroot) + "/" +
             stringify(params.db_entry->name.category) + "/" +
             stringify(params.db_entry->name.package) + "-" +
             stringify(params.db_entry->version))
        .with_setenv("PORTAGE_CALLER", params.environment->paludis_command())
        .with_setenv("PORTAGE_GID", "0")
        .with_setenv("PORTAGE_INST_GID", "0")
        .with_setenv("PORTAGE_INST_UID", "0")
        .with_setenv("PORTAGE_MASTER_PID", stringify(::getpid()))
        .with_setenv("PORTAGE_NICENCESS", stringify(::getpriority(PRIO_PROCESS, 0)))
        .with_setenv("PORTAGE_TMPDIR", stringify(params.buildroot))
        .with_setenv("PORTAGE_TMPFS", "/dev/shm")
        .with_setenv("PORTAGE_WORKDIR_MODE", "0700");
}

bool
EbuildCommand::do_run_command(const Command & cmd)
{
    return 0 == run_command(cmd);
}

EbuildMetadataCommand::EbuildMetadataCommand(const EbuildCommandParams & p) :
    EbuildCommand(p)
{
}

std::string
EbuildMetadataCommand::commands() const
{
    return "metadata";
}

bool
EbuildMetadataCommand::failure()
{
    return EbuildCommand::failure();
}

Command
EbuildMetadataCommand::extend_command(const Command & cmd)
{
    return cmd;
}

bool
EbuildMetadataCommand::do_run_command(const Command & cmd)
{
    PStream prog(cmd);
    KeyValueConfigFile f(&prog);
    _metadata.reset(new EbuildVersionMetadata);

    bool ok(false);
    try
    {
        _metadata->build_depend_string = f.get("DEPEND");
        _metadata->run_depend_string = f.get("RDEPEND");
        _metadata->slot = SlotName(f.get("SLOT"));
        _metadata->src_uri = f.get("SRC_URI");
        _metadata->restrict_string = f.get("RESTRICT");
        _metadata->homepage = f.get("HOMEPAGE");
        _metadata->license_interface->license_string = f.get("LICENSE");
        _metadata->description = f.get("DESCRIPTION");
        _metadata->keywords = f.get("KEYWORDS");
        _metadata->eclass_keywords = f.get("E_KEYWORDS");
        _metadata->inherited = f.get("INHERITED");
        _metadata->iuse = f.get("IUSE");
        _metadata->post_depend_string = f.get("PDEPEND");
        _metadata->provide_string = f.get("PROVIDE");
        _metadata->eapi = f.get("EAPI");

        if (0 == prog.exit_status())
            ok = true;
    }
    catch (const NameError & e)
    {
        Log::get_instance()->message(ll_warning, lc_context, "Caught exception '" +
                stringify(e.message()) + "' (" + stringify(e.what()) +
                ") when generating cache for '" + stringify(*params.db_entry) + "'");
    }

    if (ok)
        return true;
    else
    {
        Log::get_instance()->message(ll_warning, lc_context, "Could not generate cache for '"
                + stringify(*params.db_entry) + "'");
        _metadata->eapi = "UNKNOWN";

        return false;
    }
}

EbuildVariableCommand::EbuildVariableCommand(const EbuildCommandParams & p,
        const std::string & var) :
    EbuildCommand(p),
    _var(var)
{
}

std::string
EbuildVariableCommand::commands() const
{
    return "variable";
}

bool
EbuildVariableCommand::failure()
{
    return EbuildCommand::failure();
}

Command
EbuildVariableCommand::extend_command(const Command & cmd)
{
    return Command(cmd).with_setenv("PALUDIS_VARIABLE", _var);
}

bool
EbuildVariableCommand::do_run_command(const Command & cmd)
{
    PStream prog(cmd);
    _result = strip_trailing_string(
            std::string((std::istreambuf_iterator<char>(prog)),
                std::istreambuf_iterator<char>()), "\n");

    return (0 == prog.exit_status());
}

std::string
EbuildFetchCommand::commands() const
{
    if (fetch_params.no_fetch)
        return "nofetch";
    else
        return "fetch";
}

bool
EbuildFetchCommand::failure()
{
    throw PackageFetchActionError("Fetch failed for '" + stringify(
                *params.db_entry) + "'");
}

Command
EbuildFetchCommand::extend_command(const Command & cmd)
{
    Command result(Command(cmd)
            .with_setenv("A", fetch_params.a)
            .with_setenv("AA", fetch_params.aa)
            .with_setenv("USE", fetch_params.use)
            .with_setenv("USE_EXPAND", fetch_params.use_expand)
            .with_setenv("FLAT_SRC_URI", fetch_params.flat_src_uri)
            .with_setenv("ROOT", fetch_params.root)
            .with_setenv("PALUDIS_USE_SAFE_RESUME", fetch_params.safe_resume ? "oohyesplease" : "")
            .with_setenv("PALUDIS_PROFILE_DIR", stringify(*fetch_params.profiles->begin()))
            .with_setenv("PALUDIS_PROFILE_DIRS", join(fetch_params.profiles->begin(),
                    fetch_params.profiles->end(), " ")));

    for (AssociativeCollection<std::string, std::string>::Iterator
            i(fetch_params.expand_vars->begin()),
            j(fetch_params.expand_vars->end()) ; i != j ; ++i)
        result.with_setenv(i->first, i->second);

    return result;
}

EbuildFetchCommand::EbuildFetchCommand(const EbuildCommandParams & p,
        const EbuildFetchCommandParams & f) :
    EbuildCommand(p),
    fetch_params(f)
{
}

std::string
EbuildInstallCommand::commands() const
{
    switch (install_params.phase)
    {
        case ip_build:
            return "init setup unpack compile test install saveenv";

        case ip_preinstall:
            return "loadenv strip preinst saveenv";

        case ip_postinstall:
            return "loadenv postinst saveenv";

        case ip_tidyup:
            return "tidyup";

        case last_ip:
            ;
    };

    throw InternalError(PALUDIS_HERE, "Bad phase");
}

bool
EbuildInstallCommand::failure()
{
    throw PackageInstallActionError("Install failed for '" + stringify(
                *params.db_entry) + "'");
}

Command
EbuildInstallCommand::extend_command(const Command & cmd)
{
    std::string debug_build;
    do
    {
        switch (install_params.debug_build)
        {
            case ido_none:
                debug_build = "none";
                continue;

            case ido_split:
                debug_build = "split";
                continue;

            case ido_internal:
                debug_build = "internal";
                continue;
        }

        throw InternalError(PALUDIS_HERE, "Bad debug_build value");
    }
    while (false);

    Command result(Command(cmd)
            .with_setenv("A", install_params.a)
            .with_setenv("AA", install_params.aa)
            .with_setenv("USE", install_params.use)
            .with_setenv("USE_EXPAND", install_params.use_expand)
            .with_setenv("ROOT", install_params.root)
            .with_setenv("PALUDIS_LOADSAVEENV_DIR", stringify(install_params.loadsaveenv_dir))
            .with_setenv("PALUDIS_CONFIG_PROTECT", install_params.config_protect)
            .with_setenv("PALUDIS_CONFIG_PROTECT_MASK", install_params.config_protect_mask)
            .with_setenv("PALUDIS_EBUILD_OVERRIDE_CONFIG_PROTECT_MASK",
                install_params.disable_cfgpro ? "/" : "")
            .with_setenv("PALUDIS_DEBUG_BUILD", debug_build)
            .with_setenv("PALUDIS_PROFILE_DIR", stringify(*install_params.profiles->begin()))
            .with_setenv("PALUDIS_PROFILE_DIRS", join(install_params.profiles->begin(),
                                          install_params.profiles->end(), " "))
            .with_setenv("SLOT", stringify(install_params.slot)));

    for (AssociativeCollection<std::string, std::string>::Iterator
            i(install_params.expand_vars->begin()),
            j(install_params.expand_vars->end()) ; i != j ; ++i)
        result.with_setenv(i->first, i->second);

    return result;
}

EbuildInstallCommand::EbuildInstallCommand(const EbuildCommandParams & p,
        const EbuildInstallCommandParams & f) :
    EbuildCommand(p),
    install_params(f)
{
}

std::string
EbuildUninstallCommand::commands() const
{
    switch (uninstall_params.phase)
    {
        case up_preremove:
            return "prerm saveenv";

        case up_postremove:
            return "loadenv postrm";

        case last_up:
            ;
    }

    throw InternalError(PALUDIS_HERE, "Bad phase value");
}

bool
EbuildUninstallCommand::failure()
{
    throw PackageUninstallActionError("Uninstall failed for '" + stringify(
                *params.db_entry) + "'");
}

Command
EbuildUninstallCommand::extend_command(const Command & cmd)
{
    Command result(Command(cmd)
            .with_setenv("ROOT", uninstall_params.root)
            .with_setenv("PALUDIS_LOADSAVEENV_DIR", stringify(uninstall_params.loadsaveenv_dir))
            .with_setenv("PALUDIS_EBUILD_OVERRIDE_CONFIG_PROTECT_MASK",
                uninstall_params.disable_cfgpro ? "/" : ""));

    if (uninstall_params.load_environment)
        result
            .with_setenv("PALUDIS_LOAD_ENVIRONMENT", stringify(*uninstall_params.load_environment))
            .with_setenv("PALUDIS_SKIP_INHERIT", "yes");

    return result;
}

EbuildUninstallCommand::EbuildUninstallCommand(const EbuildCommandParams & p,
        const EbuildUninstallCommandParams & f) :
    EbuildCommand(p),
    uninstall_params(f)
{
}

EbuildVersionMetadata::EbuildVersionMetadata() :
    VersionMetadata(
            VersionMetadataBase(SlotName("unset"), "", "", "UNKNOWN"),
            VersionMetadataCapabilities::create()
            .ebuild_interface(this)
            .cran_interface(0)
            .deps_interface(this)
            .license_interface(this)
            .virtual_interface(0)
            .origins_interface(0)
            ),
    VersionMetadataEbuildInterface(),
    VersionMetadataDepsInterface(PortageDepParser::parse_depend),
    VersionMetadataLicenseInterface(PortageDepParser::parse_license)
{
}

EbuildVersionMetadata::~EbuildVersionMetadata()
{
}

std::string
EbuildConfigCommand::commands() const
{
    return "config";
}

bool
EbuildConfigCommand::failure()
{
    throw PackageConfigActionError("Configure failed for '" + stringify(
                *params.db_entry) + "'");
}

Command
EbuildConfigCommand::extend_command(const Command & cmd)
{
    Command result(Command(cmd)
            .with_setenv("ROOT", config_params.root));

    if (config_params.load_environment)
        result
            .with_setenv("PALUDIS_LOAD_ENVIRONMENT", stringify(*config_params.load_environment))
            .with_setenv("PALUDIS_SKIP_INHERIT", "yes");

    return result;
}

EbuildConfigCommand::EbuildConfigCommand(const EbuildCommandParams & p,
        const EbuildConfigCommandParams & f) :
    EbuildCommand(p),
    config_params(f)
{
}

WriteVDBEntryCommand::WriteVDBEntryCommand(const WriteVDBEntryParams & p) :
    params(p)
{
}

void
WriteVDBEntryCommand::operator() ()
{
    std::string ebuild_cmd(getenv_with_default("PALUDIS_EBUILD_DIR", LIBEXECDIR "/paludis") +
            "/write_vdb_entry.bash '" +
            stringify(params.output_directory) + "' '" +
            stringify(params.environment_file) + "'");

    Command cmd(Command(ebuild_cmd)
            .with_setenv("PKGMANAGER", PALUDIS_PACKAGE "-" + stringify(PALUDIS_VERSION_MAJOR) + "." +
                stringify(PALUDIS_VERSION_MINOR) + "." +
                stringify(PALUDIS_VERSION_MICRO) +
                (std::string(PALUDIS_SUBVERSION_REVISION).empty() ?
                 std::string("") : "-r" + std::string(PALUDIS_SUBVERSION_REVISION)))
            .with_setenv("PALUDIS_CONFIG_DIR", SYSCONFDIR "/paludis/")
            .with_setenv("PALUDIS_BASHRC_FILES", params.environment->bashrc_files())
            .with_setenv("PALUDIS_HOOK_DIRS", params.environment->hook_dirs())
            .with_setenv("PALUDIS_FETCHERS_DIRS", params.environment->fetchers_dirs())
            .with_setenv("PALUDIS_SYNCERS_DIRS", params.environment->syncers_dirs())
            .with_setenv("PALUDIS_COMMAND", params.environment->paludis_command())
            .with_setenv("PALUDIS_EBUILD_LOG_LEVEL", stringify(Log::get_instance()->log_level()))
            .with_setenv("PALUDIS_EBUILD_DIR", getenv_with_default("PALUDIS_EBUILD_DIR", LIBEXECDIR "/paludis")));

    if (0 != (run_command(cmd)))
        throw PackageInstallActionError("Write VDB Entry command failed");
}

VDBPostMergeCommand::VDBPostMergeCommand(const VDBPostMergeCommandParams & p) :
    params(p)
{
}

void
VDBPostMergeCommand::operator() ()
{
    if (! getenv_with_default("PALUDIS_NO_GLOBAL_HOOKS", "").empty())
        return;

    std::string ebuild_cmd("ldconfig -r '" + stringify(params.root) + "'");

    if (0 != (run_command(ebuild_cmd)))
        throw PackageInstallActionError("VDB Entry post merge commands failed");
}

