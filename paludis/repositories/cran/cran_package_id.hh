/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2006 Danny van Dyk <kugelfang@gentoo.org>
 * Copyright (c) 2007 Ciaran McCreesh <ciaranm@ciaranm.org>
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

#ifndef PALUDIS_GUARD_PALUDIS_CRAN_PACKAGE_ID_HH
#define PALUDIS_GUARD_PALUDIS_CRAN_PACKAGE_ID_HH 1

#include <paludis/package_id.hh>
#include <paludis/util/fs_entry.hh>

namespace paludis
{
    class CRANRepository;
    class CRANInstalledRepository;

    namespace cranrepository
    {
        class PALUDIS_VISIBLE CRANPackageID :
            public PackageID,
            private PrivateImplementationPattern<CRANPackageID>,
            public tr1::enable_shared_from_this<CRANPackageID>
        {
            private:
                Implementation<CRANPackageID> * const _imp;

            protected:
                virtual void need_keys_added() const;
                virtual void need_masks_added() const;

            public:
                CRANPackageID(const tr1::shared_ptr<const CRANRepository> &, const FSEntry &);
                CRANPackageID(const tr1::shared_ptr<const CRANRepository> &, const CRANPackageID * const, const std::string &);
                ~CRANPackageID();

                virtual const std::string canonical_form(const PackageIDCanonicalForm) const;

                virtual const QualifiedPackageName name() const;
                virtual const VersionSpec version() const;
                virtual const SlotName slot() const;
                virtual const tr1::shared_ptr<const Repository> repository() const;

                virtual const tr1::shared_ptr<const MetadataPackageIDKey> virtual_for_key() const;
                virtual const tr1::shared_ptr<const MetadataSetKey<KeywordNameSet> > keywords_key() const;
                virtual const tr1::shared_ptr<const MetadataSetKey<UseFlagNameSet> > use_key() const;
                virtual const tr1::shared_ptr<const MetadataSetKey<IUseFlagSet> > iuse_key() const;
                virtual const tr1::shared_ptr<const MetadataSetKey<InheritedSet> > inherited_key() const;
                virtual const tr1::shared_ptr<const MetadataSpecTreeKey<LicenseSpecTree> > license_key() const;
                virtual const tr1::shared_ptr<const MetadataSpecTreeKey<ProvideSpecTree> > provide_key() const;
                virtual const tr1::shared_ptr<const MetadataSpecTreeKey<DependencySpecTree> > build_dependencies_key() const;
                virtual const tr1::shared_ptr<const MetadataSpecTreeKey<DependencySpecTree> > run_dependencies_key() const;
                virtual const tr1::shared_ptr<const MetadataSpecTreeKey<DependencySpecTree> > post_dependencies_key() const;
                virtual const tr1::shared_ptr<const MetadataSpecTreeKey<DependencySpecTree> > suggested_dependencies_key() const;
                virtual const tr1::shared_ptr<const MetadataSpecTreeKey<RestrictSpecTree> > restrict_key() const;
                virtual const tr1::shared_ptr<const MetadataSpecTreeKey<URISpecTree> > src_uri_key() const;
                virtual const tr1::shared_ptr<const MetadataSpecTreeKey<URISpecTree> > bin_uri_key() const;
                virtual const tr1::shared_ptr<const MetadataSpecTreeKey<URISpecTree> > homepage_key() const;
                virtual const tr1::shared_ptr<const MetadataStringKey> short_description_key() const;
                virtual const tr1::shared_ptr<const MetadataStringKey> long_description_key() const;
                virtual const tr1::shared_ptr<const MetadataContentsKey> contents_key() const;
                virtual const tr1::shared_ptr<const MetadataTimeKey> installed_time_key() const;
                virtual const tr1::shared_ptr<const MetadataStringKey> source_origin_key() const;
                virtual const tr1::shared_ptr<const MetadataStringKey> binary_origin_key() const;
                virtual const tr1::shared_ptr<const MetadataSetKey<PackageIDSequence> > contains_key() const;
                virtual const tr1::shared_ptr<const MetadataPackageIDKey> contained_in_key() const;
                virtual const tr1::shared_ptr<const MetadataFSEntryKey> fs_location_key() const;

                virtual bool supports_action(const SupportsActionTestBase &) const PALUDIS_ATTRIBUTE((warn_unused_result));
                virtual void perform_action(Action &) const PALUDIS_ATTRIBUTE((noreturn));

                virtual bool breaks_portage() const PALUDIS_ATTRIBUTE((warn_unused_result));

                virtual bool arbitrary_less_than_comparison(const PackageID &) const
                    PALUDIS_ATTRIBUTE((warn_unused_result));

                virtual std::size_t extra_hash_value() const
                    PALUDIS_ATTRIBUTE((warn_unused_result));
        };
    }
}

#endif
