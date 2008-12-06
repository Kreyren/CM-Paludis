/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2005, 2006, 2007, 2008 Ciaran McCreesh
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

#include <paludis/dep_list.hh>
#include <paludis/dep_list_exceptions.hh>
#include <paludis/query_visitor.hh>
#include <paludis/range_rewriter.hh>
#include <paludis/show_suggest_visitor.hh>
#include <paludis/condition_tracker.hh>
#include <paludis/handled_information.hh>

#include <paludis/dep_spec.hh>
#include <paludis/user_dep_spec.hh>
#include <paludis/action.hh>
#include <paludis/dep_spec_flattener.hh>
#include <paludis/distribution.hh>
#include <paludis/match_package.hh>
#include <paludis/metadata_key.hh>
#include <paludis/package_id.hh>
#include <paludis/version_requirements.hh>
#include <paludis/selection.hh>
#include <paludis/generator.hh>
#include <paludis/filter.hh>
#include <paludis/filtered_generator.hh>
#include <paludis/choice.hh>

#include <paludis/util/join.hh>
#include <paludis/util/log.hh>
#include <paludis/util/private_implementation_pattern-impl.hh>
#include <paludis/util/make_shared_ptr.hh>
#include <paludis/util/iterator_funcs.hh>
#include <paludis/util/save.hh>
#include <paludis/util/member_iterator.hh>
#include <paludis/util/set.hh>
#include <paludis/util/sequence.hh>
#include <paludis/util/stringify.hh>
#include <paludis/util/tokeniser.hh>
#include <paludis/util/visitor-impl.hh>
#include <paludis/util/wrapped_forward_iterator-impl.hh>
#include <paludis/util/hashes.hh>

#include <algorithm>
#include <functional>
#include <vector>
#include <list>
#include <set>
#include <tr1/functional>
#include <tr1/unordered_map>

using namespace paludis;

typedef std::list<std::tr1::shared_ptr<ActiveDependencyLabels> > LabelsStack;

template class Sequence<std::tr1::function<bool (const PackageID &, const Mask &)> >;
template class WrappedForwardIterator<DepList::IteratorTag, DepListEntry>;
template class WrappedForwardIterator<DepList::ConstIteratorTag, const DepListEntry>;

template class PrivateImplementationPattern<DepList>;

#include <paludis/dep_list-sr.cc>

DepListOptions::DepListOptions() :
    reinstall(dl_reinstall_never),
    reinstall_scm(dl_reinstall_scm_never),
    target_type(dl_target_package),
    upgrade(dl_upgrade_always),
    downgrade(dl_downgrade_as_needed),
    new_slots(dl_new_slots_always),
    fall_back(dl_fall_back_as_needed_except_targets),
    installed_deps_pre(dl_deps_discard),
    installed_deps_runtime(dl_deps_try_post),
    installed_deps_post(dl_deps_try_post),
    uninstalled_deps_pre(dl_deps_pre),
    uninstalled_deps_runtime(dl_deps_pre_or_post),
    uninstalled_deps_post(dl_deps_post),
    uninstalled_deps_suggested(dl_deps_try_post),
    suggested(dl_suggested_show),
    circular(dl_circular_error),
    use(dl_use_deps_standard),
    blocks(dl_blocks_accumulate),
    dependency_tags(false),
    match_package_options()
{
    /* when changing the above, also see src/paludis/command_line.cc. */
}

namespace paludis
{
    typedef std::list<DepListEntry> MergeList;
    typedef std::tr1::unordered_multimap<QualifiedPackageName, MergeList::iterator, Hash<QualifiedPackageName> > MergeListIndex;

    template<>
    struct Implementation<DepList>
    {
        const Environment * const env;
        std::tr1::shared_ptr<DepListOptions> opts;

        MergeList merge_list;
        MergeList::const_iterator current_merge_list_entry;
        MergeList::iterator merge_list_insert_position;
        long merge_list_generation;

        MergeListIndex merge_list_index;

        SetSpecTree::ConstItem * current_top_level_target;

        bool throw_on_blocker;

        LabelsStack labels;

        const std::tr1::shared_ptr<const PackageID> current_package_id() const
        {
            if (current_merge_list_entry != merge_list.end())
                return current_merge_list_entry->package_id;
            return std::tr1::shared_ptr<const PackageID>();
        }

        Implementation(const Environment * const e, const DepListOptions & o) :
            env(e),
            opts(new DepListOptions(o)),
            current_merge_list_entry(merge_list.end()),
            merge_list_insert_position(merge_list.end()),
            merge_list_generation(0),
            current_top_level_target(0),
            throw_on_blocker(o.blocks == dl_blocks_error)
        {
            labels.push_front(make_shared_ptr(new ActiveDependencyLabels(*make_shared_ptr(new DependencyLabelSequence))));
        }
    };
}

namespace
{
    struct GenerationGreaterThan
    {
        long g;

        GenerationGreaterThan(long gg) :
            g(gg)
        {
        }

        template <typename T_>
        bool operator() (const T_ & e) const
        {
            return e.generation > g;
        }
    };

    struct RemoveTagsWithGenerationGreaterThan
    {
        long g;

        RemoveTagsWithGenerationGreaterThan(long gg) :
            g(gg)
        {
        }

        void operator() (DepListEntry & e) const
        {
            /* see EffSTL 9 for why this is so painful */
            if (e.tags->empty())
                return;
            std::tr1::shared_ptr<DepListEntryTags> t(new DepListEntryTags);
            GenerationGreaterThan pred(g);
            for (DepListEntryTags::ConstIterator i(e.tags->begin()), i_end(e.tags->end()) ;
                    i != i_end ; ++i)
                if (! pred(*i))
                    t->insert(*i);
            std::swap(e.tags, t);
        }
    };

    class DepListTransaction
    {
        protected:
            MergeList & _list;
            MergeListIndex & _index;
            long & _generation;
            int _initial_generation;
            bool _committed;

        public:
            DepListTransaction(MergeList & l, MergeListIndex & i, long & g) :
                _list(l),
                _index(i),
                _generation(g),
                _initial_generation(g),
                _committed(false)
            {
                ++_generation;
            }

            void commit()
            {
                _committed = true;
            }

            ~DepListTransaction()
            {
                if (_committed)
                    return;

                /* See EffSTL 9 */
                GenerationGreaterThan pred(_initial_generation);
                for (MergeList::iterator i(_list.begin()) ; i != _list.end() ; )
                {
                    if (! pred(*i))
                        ++i;
                    else
                    {
                        for (std::pair<MergeListIndex::iterator, MergeListIndex::iterator> p(
                                    _index.equal_range(i->package_id->name())) ; p.first != p.second ; )
                            if (p.first->second == i)
                                _index.erase(p.first++);
                            else
                                ++p.first;

                        _list.erase(i++);
                    }
                }

                std::for_each(_list.begin(), _list.end(),
                        RemoveTagsWithGenerationGreaterThan(_initial_generation));
            }
    };

    struct MatchDepListEntryAgainstPackageDepSpec
    {
        const Environment * const env;
        const PackageDepSpec & a;
        const MatchPackageOptions & o;

        MatchDepListEntryAgainstPackageDepSpec(const Environment * const ee,
                const PackageDepSpec & aa, const MatchPackageOptions & oo) :
            env(ee),
            a(aa),
            o(oo)
        {
        }

        bool operator() (const std::pair<const QualifiedPackageName, MergeList::const_iterator> & e)
        {
            switch (e.second->kind)
            {
                case dlk_virtual:
                case dlk_package:
                case dlk_provided:
                case dlk_already_installed:
                case dlk_subpackage:
                    return match_package(*env, a, *e.second->package_id, o);

                case dlk_block:
                case dlk_masked:
                case dlk_suggested:
                    return false;

                case last_dlk:
                    ;
            }

            throw InternalError(PALUDIS_HERE, "Bad e.second->kind");
        }
    };

    bool is_interesting_any_child(const Environment & env,
            const DependencySpecTree::ConstItem & i)
    {
        const PackageDepSpec * const u(get_const_item(i)->as_package_dep_spec());
        if (0 != u && u->package_ptr())
        {
            return ! env[selection::SomeArbitraryVersion(
                    generator::Package(*u->package_ptr()) |
                    filter::SupportsAction<InstalledAction>())]->empty();
        }
        else
            return false;
    }
}

struct DepList::AddVisitor :
    ConstVisitor<DependencySpecTree>
{
    DepList * const d;
    std::tr1::shared_ptr<const DestinationsSet> destinations;
    std::tr1::shared_ptr<ConstTreeSequence<DependencySpecTree, AllDepSpec> > conditions;
    std::set<SetName> recursing_sets;
    const bool only_if_not_suggested_label;

    AddVisitor(DepList * const dd, bool l, std::tr1::shared_ptr<const DestinationsSet> ddd,
               std::tr1::shared_ptr<ConstTreeSequence<DependencySpecTree, AllDepSpec> > c =
               (std::tr1::shared_ptr<ConstTreeSequence<DependencySpecTree, AllDepSpec> >(
                   new ConstTreeSequence<DependencySpecTree, AllDepSpec>(
                       std::tr1::shared_ptr<AllDepSpec>(new AllDepSpec))))) :
        d(dd),
        destinations(ddd),
        conditions(c),
        only_if_not_suggested_label(l)
    {
    }

    void visit_sequence(const AllDepSpec &,
            DependencySpecTree::ConstSequenceIterator,
            DependencySpecTree::ConstSequenceIterator);

    void visit_sequence(const AnyDepSpec &,
            DependencySpecTree::ConstSequenceIterator,
            DependencySpecTree::ConstSequenceIterator);

    void visit_sequence(const ConditionalDepSpec &,
            DependencySpecTree::ConstSequenceIterator,
            DependencySpecTree::ConstSequenceIterator);

    void visit_leaf(const PackageDepSpec &);

    void visit_leaf(const BlockDepSpec &);

    void visit_leaf(const DependencyLabelsDepSpec &);

    void visit_leaf(const NamedSetDepSpec &);
};

namespace
{
    struct SuggestActiveVisitor :
        ConstVisitor<DependencySuggestLabelVisitorTypes>
    {
        bool result;

        SuggestActiveVisitor() :
            result(false)
        {
        }

        void visit(const DependencyRecommendedLabel &)
        {
        }

        void visit(const DependencySuggestedLabel &)
        {
            result = true;
        }

        void visit(const DependencyRequiredLabel &)
        {
        }
    };
}

void
DepList::AddVisitor::visit_leaf(const PackageDepSpec & a)
{
    Context context("When adding PackageDepSpec '" + stringify(a) + "':");

    if (only_if_not_suggested_label)
    {
        SuggestActiveVisitor v;
        for (DependencySuggestLabelSequence::ConstIterator
                i((*d->_imp->labels.begin())->suggest_labels()->begin()),
                i_end((*d->_imp->labels.begin())->suggest_labels()->end()) ;
                i != i_end ; ++i)
            (*i)->accept(v);

        if (v.result)
        {
            Log::get_instance()->message("dep_list.skipping_suggested", ll_debug, lc_context) << "Skipping dep '"
                << a << "' because suggested label is active";
            return;
        }
    }

    Save<std::tr1::shared_ptr<ConstTreeSequence<DependencySpecTree, AllDepSpec> > > save_c(
        &conditions, d->_imp->opts->dependency_tags ? ConditionTracker(conditions).add_condition(a) : conditions);

    /* find already installed things */
    // TODO: check destinations
    std::tr1::shared_ptr<const PackageIDSequence> already_installed((*d->_imp->env)[selection::AllVersionsSorted(
                generator::Matches(a, d->_imp->opts->match_package_options) | filter::SupportsAction<InstalledAction>())]);

    /* are we already on the merge list? */
    std::pair<MergeListIndex::iterator, MergeListIndex::iterator> q;
    if (a.package_ptr())
        q = d->_imp->merge_list_index.equal_range(*a.package_ptr());
    else
        q = std::make_pair(d->_imp->merge_list_index.begin(), d->_imp->merge_list_index.end());

    MergeListIndex::iterator qq(std::find_if(q.first, q.second,
                MatchDepListEntryAgainstPackageDepSpec(d->_imp->env, a, d->_imp->opts->match_package_options)));

    MergeList::iterator existing_merge_list_entry(qq == q.second ? d->_imp->merge_list.end() : qq->second);
    if (existing_merge_list_entry != d->_imp->merge_list.end())
    {
        /* tag it */
        if (a.tag())
            existing_merge_list_entry->tags->insert(DepTagEntry::create()
                    .tag(a.tag())
                    .generation(d->_imp->merge_list_generation));

        if (d->_imp->opts->dependency_tags && d->_imp->current_package_id())
            existing_merge_list_entry->tags->insert(DepTagEntry::create()
                    .tag(std::tr1::shared_ptr<DepTag>(new DependencyDepTag(d->_imp->current_package_id(), a, conditions)))
                    .generation(d->_imp->merge_list_generation));

        /* add an appropriate destination */
        // TODO

        /* have our deps been merged already, or is this a circular dep? */
        if (dle_no_deps == existing_merge_list_entry->state)
        {
            /* is a sufficiently good version installed? */
            if (! already_installed->empty())
                return;

            if (d->_imp->opts->circular == dl_circular_discard)
            {
                Log::get_instance()->message("dep_list.dropping_circular", ll_qa, lc_context)
                    << "Dropping circular dependency on '" << *existing_merge_list_entry->package_id << "'";
                return;
            }
            else if (d->_imp->opts->circular == dl_circular_discard_silently)
                return;

            throw CircularDependencyError("Atom '" + stringify(a) + "' matched by merge list entry '" +
                    stringify(*existing_merge_list_entry->package_id) + "', which does not yet have its "
                    "dependencies installed");
        }
        else
            return;
    }

    /* find installable candidates, and find the best visible candidate */
    std::tr1::shared_ptr<const PackageID> best_visible_candidate;
    std::tr1::shared_ptr<const PackageIDSequence> installable_candidates(
            (*d->_imp->env)[selection::AllVersionsSorted(generator::Matches(a, d->_imp->opts->match_package_options) &
                generator::SomeIDsMightSupportAction<InstallAction>())]);

    for (PackageIDSequence::ReverseConstIterator p(installable_candidates->rbegin()),
            p_end(installable_candidates->rend()) ; p != p_end ; ++p)
        if ((*p)->supports_action(SupportsActionTest<InstallAction>()) && ! (*p)->masked())
        {
            best_visible_candidate = *p;
            break;
        }

    if (! best_visible_candidate && ! already_installed->empty() &&
        (*already_installed->last())->transient_key() && (*already_installed->last())->transient_key()->value() &&
        (dl_target_package != d->_imp->opts->target_type || ! d->is_top_level_target(**already_installed->last())))
    {
            Log::get_instance()->message("dep_list.no_visible.transient", ll_debug, lc_context) << "No visible packages matching '"
                << a << "', silently falling back to installed package '" << **already_installed->last() << "' as it is transient";
            d->add_already_installed_package(*already_installed->last(), a.tag(), a, conditions, destinations);
            return;
    }

    /* are we allowed to override mask reasons? */
    if (! best_visible_candidate && d->_imp->opts->override_masks)
    {
        for (DepListOverrideMasksFunctions::ConstIterator of(d->_imp->opts->override_masks->begin()),
                of_end(d->_imp->opts->override_masks->end()) ; of != of_end ; ++of)
        {
            if (best_visible_candidate)
                break;

            for (PackageIDSequence::ReverseConstIterator p(installable_candidates->rbegin()),
                    p_end(installable_candidates->rend()) ; p != p_end ; ++p)
            {
                if (! (*p)->supports_action(SupportsActionTest<InstallAction>()))
                    continue;

                bool success(true);
                for (PackageID::MasksConstIterator m((*p)->begin_masks()), m_end((*p)->end_masks()) ;
                        m != m_end ; ++m)
                {
                    bool local_success(false);
                    for (DepListOverrideMasksFunctions::ConstIterator o(d->_imp->opts->override_masks->begin()),
                            o_end(next(of)) ; o != o_end ; ++o)
                        if ((*o)(**p, **m))
                            local_success = true;

                    success &= local_success;
                    if (! success)
                        break;
                }

                if (success)
                {
                    d->add_error_package(*p, dlk_masked, a, conditions);
                    best_visible_candidate = *p;
                }
            }
        }
    }

    /* no installable candidates. if we're already installed, that's ok (except for top level
     * package targets), otherwise error. */
    if (! best_visible_candidate)
    {
        bool can_fall_back;
        do
        {
            switch (d->_imp->opts->fall_back)
            {
                case dl_fall_back_never:
                    can_fall_back = false;
                    continue;

                case dl_fall_back_as_needed_except_targets:
                    if (! d->_imp->current_package_id())
                        can_fall_back = false;
                    else if (already_installed->empty())
                        can_fall_back = true;
                    else
                        can_fall_back = ! d->is_top_level_target(**already_installed->last());

                    continue;

                case dl_fall_back_as_needed:
                    can_fall_back = true;
                    continue;

                case last_dl_fall_back:
                    ;
            }

            throw InternalError(PALUDIS_HERE, "Bad fall_back value '" + stringify(d->_imp->opts->fall_back) + "'");
        } while (false);

        if (already_installed->empty() || ! can_fall_back)
        {
            if (! a.additional_requirements_ptr())
                throw AllMaskedError(a);

            std::tr1::shared_ptr<const PackageIDSequence> match_except_reqs((*d->_imp->env)[selection::AllVersionsUnsorted(
                        generator::Matches(*a.without_additional_requirements(), d->_imp->opts->match_package_options))]);

            for (PackageIDSequence::ConstIterator i(match_except_reqs->begin()),
                    i_end(match_except_reqs->end()) ; i != i_end ; ++i)
                if (! (*i)->masked())
                    throw AdditionalRequirementsNotMetError(a);

            throw AllMaskedError(a);
        }
        else
        {
            Log::get_instance()->message("dep_list.no_visible", ll_warning, lc_context) << "No visible packages matching '"
                << a << "', falling back to installed package '" << **already_installed->last() << "'";
            d->add_already_installed_package(*already_installed->last(), a.tag(), a, conditions, destinations);
            return;
        }
    }

    std::tr1::shared_ptr<PackageIDSequence> already_installed_in_same_slot(new PackageIDSequence);
    for (PackageIDSequence::ConstIterator aa(already_installed->begin()),
            aa_end(already_installed->end()) ; aa != aa_end ; ++aa)
        if ((*aa)->slot() == best_visible_candidate->slot())
            already_installed_in_same_slot->push_back(*aa);
    /* no need to sort already_installed_in_same_slot here, although if the above is
     * changed then check that this still holds... */

    /* we have an already installed version. do we want to use it? */
    if (! already_installed_in_same_slot->empty())
    {
        if (d->prefer_installed_over_uninstalled(**already_installed_in_same_slot->last(), *best_visible_candidate))
        {
            Log::get_instance()->message("dep_list.installed_over_best_visible", ll_debug, lc_context)
                << "Taking installed package '" << **already_installed_in_same_slot->last() << "' over '"
                << *best_visible_candidate << "'";
            d->add_already_installed_package(*already_installed_in_same_slot->last(), a.tag(), a, conditions, destinations);
            return;
        }
        else
            Log::get_instance()->message("dep_list.best_visible_over_installed", ll_debug, lc_context)
                << "Not taking installed package '" << **already_installed_in_same_slot->last() << "' over '"
                << *best_visible_candidate << "'";
    }
    else if ((! already_installed->empty()) && (dl_new_slots_as_needed == d->_imp->opts->new_slots))
    {
        /* we have an already installed, but not in the same slot, and our options
         * allow us to take this. */
        if (d->prefer_installed_over_uninstalled(**already_installed->last(), *best_visible_candidate))
        {
            Log::get_instance()->message("dep_list.installed_over_slot", ll_debug, lc_context) <<
                "Taking installed package '" << **already_installed->last() << "' over '" << *best_visible_candidate <<
                "' (in different slot)";
            d->add_already_installed_package(*already_installed->last(), a.tag(), a, conditions, destinations);
            return;
        }
        else
            Log::get_instance()->message("dep_list.slot_over_installed", ll_debug, lc_context) <<
                "Not taking installed package '" << **already_installed->last() << "' over '" <<
                *best_visible_candidate << "' (in different slot)";
    }
    else
        Log::get_instance()->message("dep_list.no_installed", ll_debug, lc_context) << "No installed packages in SLOT '"
            << best_visible_candidate->slot() << "', taking uninstalled package '"
            << *best_visible_candidate << "'";

    /* if this is a downgrade, make sure that that's ok */
    switch (d->_imp->opts->downgrade)
    {
        case dl_downgrade_as_needed:
            break;

        case dl_downgrade_error:
        case dl_downgrade_warning:
            {
                std::tr1::shared_ptr<const PackageIDSequence> are_we_downgrading(
                        (*d->_imp->env)[selection::AllVersionsSorted(
                            generator::Matches(make_package_dep_spec()
                                .package(best_visible_candidate->name())
                                .slot_requirement(make_shared_ptr(new UserSlotExactRequirement(best_visible_candidate->slot()))),
                                d->_imp->opts->match_package_options) |
                            filter::SupportsAction<InstalledAction>())]);

                if (are_we_downgrading->empty())
                    break;

                if ((*are_we_downgrading->last())->version() <= best_visible_candidate->version())
                    break;

                if (d->_imp->opts->downgrade == dl_downgrade_error)
                    throw DowngradeNotAllowedError(stringify(*best_visible_candidate),
                            stringify(**are_we_downgrading->last()));

                Log::get_instance()->message("dep_list.downgrade", ll_warning, lc_context) << "Downgrade to '"
                    << *best_visible_candidate << "' from '" << **are_we_downgrading->last() << "' forced";
            }
            break;

        case last_dl_downgrade:
            ;
    }

    d->add_package(best_visible_candidate, a.tag(), a, conditions, destinations);
}

void
DepList::AddVisitor::visit_leaf(const NamedSetDepSpec & a)
{
    Context context("When adding NamedSetDepSpec '" + stringify(a) + "':");

    std::tr1::shared_ptr<const SetSpecTree::ConstItem> set(d->_imp->env->set(a.name()));

    if (! set)
        throw NoSuchSetError(stringify(a.name()));

    if (! recursing_sets.insert(a.name()).second)
    {
        Log::get_instance()->message("dep_list.recursive_set", ll_warning, lc_context) << "Recursively defined set '" << a.name() << "'";
        throw RecursivelyDefinedSetError(stringify(a.name()));
    }

    set->accept(*this);

    recursing_sets.erase(a.name());
}

void
DepList::AddVisitor::visit_sequence(const AllDepSpec &,
        DependencySpecTree::ConstSequenceIterator cur,
        DependencySpecTree::ConstSequenceIterator end)
{
    d->_imp->labels.push_front(make_shared_ptr(new ActiveDependencyLabels(**d->_imp->labels.begin())));
    RunOnDestruction restore_labels(std::tr1::bind(std::tr1::mem_fn(&LabelsStack::pop_front), &d->_imp->labels));

    std::for_each(cur, end, accept_visitor(*this));
}

void
DepList::AddVisitor::visit_sequence(const ConditionalDepSpec & a,
        DependencySpecTree::ConstSequenceIterator cur,
        DependencySpecTree::ConstSequenceIterator end)
{
    Save<std::tr1::shared_ptr<ConstTreeSequence<DependencySpecTree, AllDepSpec> > > save_c(
        &conditions, d->_imp->opts->dependency_tags ? ConditionTracker(conditions).add_condition(a) : conditions);

    if (d->_imp->opts->use == dl_use_deps_standard)
    {
        d->_imp->labels.push_front(make_shared_ptr(new ActiveDependencyLabels(**d->_imp->labels.begin())));
        RunOnDestruction restore_labels(std::tr1::bind(std::tr1::mem_fn(&LabelsStack::pop_front), &d->_imp->labels));

        if (a.condition_met())
            std::for_each(cur, end, accept_visitor(*this));
    }
    else
    {
        if (a.condition_meetable())
            std::for_each(cur, end, accept_visitor(*this));
    }
}

void
DepList::AddVisitor::visit_sequence(const AnyDepSpec & a,
        DependencySpecTree::ConstSequenceIterator cur,
        DependencySpecTree::ConstSequenceIterator end)
{
    using namespace std::tr1::placeholders;

    d->_imp->labels.push_front(make_shared_ptr(new ActiveDependencyLabels(**d->_imp->labels.begin())));
    RunOnDestruction restore_labels(std::tr1::bind(std::tr1::mem_fn(&LabelsStack::pop_front), &d->_imp->labels));

    /* annoying requirement: || ( foo? ( ... ) ) resolves to empty if !foo. */
    if (end == std::find_if(cur, end, &is_viable_any_child))
        return;

    {
        RangeRewriter r;
        std::for_each(cur, end, accept_visitor(r));
        std::tr1::shared_ptr<PackageDepSpec> rewritten_spec(r.spec());
        if (rewritten_spec)
        {
            TreeLeaf<DependencySpecTree, PackageDepSpec> rr(r.spec());
            d->add_not_top_level(only_if_not_suggested_label, rr, destinations, conditions);
            return;
        }
    }

    Save<std::tr1::shared_ptr<ConstTreeSequence<DependencySpecTree, AllDepSpec> > > save_c(
        &conditions, d->_imp->opts->dependency_tags ? ConditionTracker(conditions).add_condition(a) : conditions);

    /* see if any of our children is already installed. if any is, add it so that
     * any upgrades kick in */
    for (DependencySpecTree::ConstSequenceIterator c(cur) ; c != end ; ++c)
    {
        if (! is_viable_any_child(*c))
            continue;

        if (d->already_installed(*c, destinations))
        {
            Context context("When using already installed group to resolve dependencies:");
            d->add_not_top_level(only_if_not_suggested_label, *c, destinations, conditions);
            return;
        }
    }

    /* if we have something like || ( a >=b-2 ) and b-1 is installed, try to go for
     * the b-2 bit first */
    for (DependencySpecTree::ConstSequenceIterator c(cur) ; c != end ; ++c)
    {
        if (! is_viable_any_child(*c))
            continue;
        if (! is_interesting_any_child(*d->_imp->env, *c))
            continue;

        try
        {
            Context context("When using already installed package to resolve dependencies:");

            Save<bool> save_t(&d->_imp->throw_on_blocker,
                    dl_blocks_discard_completely != d->_imp->opts->blocks);
            Save<std::tr1::shared_ptr<DepListOverrideMasksFunctions> > save_o(&d->_imp->opts->override_masks,
                    std::tr1::shared_ptr<DepListOverrideMasksFunctions>());
            d->add_not_top_level(only_if_not_suggested_label, *c, destinations, conditions);
            return;
        }
        catch (const DepListError &)
        {
        }
    }

    /* install first available viable option */
    for (DependencySpecTree::ConstSequenceIterator c(cur) ; c != end ; ++c)
    {
        if (! is_viable_any_child(*c))
            continue;

        try
        {
            Context context("When using new group to resolve dependencies:");

            Save<bool> save_t(&d->_imp->throw_on_blocker,
                    dl_blocks_discard_completely != d->_imp->opts->blocks);
            Save<std::tr1::shared_ptr<DepListOverrideMasksFunctions> > save_o(&d->_imp->opts->override_masks,
                    std::tr1::shared_ptr<DepListOverrideMasksFunctions>());
            d->add_not_top_level(only_if_not_suggested_label, *c, destinations, conditions);
            return;
        }
        catch (const DepListError &)
        {
        }
    }

    Log::get_instance()->message("dep_list.using_first_any_item", ll_debug, lc_context)
        << "No resolvable item in || ( ) block. Using first item for error message";
    {
        Context block_context("Inside || ( ) block with other options:");
        for (DependencySpecTree::ConstSequenceIterator c(cur) ; c != end ; ++c)
        {
            if (! is_viable_any_child(*c))
                continue;

            d->add_not_top_level(only_if_not_suggested_label, *c, destinations, conditions);
            return;
        }
    }
}

void
DepList::AddVisitor::visit_leaf(const BlockDepSpec & a)
{
    if (dl_blocks_discard_completely == d->_imp->opts->blocks)
        return;

    // TODO: check destinations

    Context context("When checking BlockDepSpec '" + stringify(a) + "':");

    Save<std::tr1::shared_ptr<ConstTreeSequence<DependencySpecTree, AllDepSpec> > > save_c(
        &conditions, d->_imp->opts->dependency_tags ? ConditionTracker(conditions).add_condition(a) : conditions);

    bool check_whole_list(false);
    std::list<MergeList::const_iterator> will_be_installed;
    std::tr1::shared_ptr<const PackageIDSequence> already_installed;

    if (a.blocked_spec()->package_ptr())
    {
        PackageDepSpec just_package(make_package_dep_spec().package(*a.blocked_spec()->package_ptr()));
        already_installed = (*d->_imp->env)[selection::AllVersionsUnsorted(
                generator::Matches(just_package, d->_imp->opts->match_package_options) |
                filter::SupportsAction<InstalledAction>())];

        MatchDepListEntryAgainstPackageDepSpec m(d->_imp->env, just_package, d->_imp->opts->match_package_options);
        for (std::pair<MergeListIndex::const_iterator, MergeListIndex::const_iterator> p(
                    d->_imp->merge_list_index.equal_range(*a.blocked_spec()->package_ptr())) ;
                p.first != p.second ; ++p.first)
        {
            if (d->_imp->current_merge_list_entry != d->_imp->merge_list.end())
            {
                if (d->_imp->current_merge_list_entry == p.first->second)
                    continue;

                if (d->_imp->current_merge_list_entry->associated_entry == &*p.first->second)
                    continue;
            }

            if (m(*p.first))
                will_be_installed.push_back(p.first->second);
        }
    }
    else
    {
        check_whole_list = true;
        /* TODO: InstalledAtRoot? */
        already_installed = (*d->_imp->env)[selection::AllVersionsUnsorted(
                generator::All() | filter::SupportsAction<InstalledAction>())];
    }

    if (already_installed->empty() && will_be_installed.empty() && ! check_whole_list)
        return;

    for (PackageIDSequence::ConstIterator aa(already_installed->begin()),
            aa_end(already_installed->end()) ; aa != aa_end ; ++aa)
    {
        if (! match_package(*d->_imp->env, *a.blocked_spec(), **aa, d->_imp->opts->match_package_options))
            continue;

        bool replaced(false);
        for (std::list<MergeList::const_iterator>::const_iterator r(will_be_installed.begin()),
                r_end(will_be_installed.end()) ; r != r_end && ! replaced ; ++r)
            if ((*r)->package_id->slot() == (*aa)->slot())
            {
                /* if it's a virtual, it only replaces if it's the same package. */
                if ((*r)->package_id->virtual_for_key())
                {
                    if ((*r)->package_id->virtual_for_key()->value()->name() == (*aa)->name())
                        replaced = true;
                }
                else
                    replaced = true;
            }

        if (replaced)
            continue;

        /* ignore if it's a virtual/blah (not <virtual/blah-1) block and it's blocking
         * ourself */
        if (! (a.blocked_spec()->version_requirements_ptr() || a.blocked_spec()->slot_requirement_ptr()
                    || a.blocked_spec()->additional_requirements_ptr()
                    || a.blocked_spec()->from_repository_ptr()
                    || a.blocked_spec()->in_repository_ptr()
                    || a.blocked_spec()->installed_at_path_ptr()
                    || a.blocked_spec()->installable_to_repository_ptr()
                    || a.blocked_spec()->installable_to_path_ptr())
                && d->_imp->current_package_id())
        {
            if ((*aa)->name() == d->_imp->current_package_id()->name())
                continue;

            if ((*aa)->virtual_for_key() && (*aa)->virtual_for_key()->value()->name() == d->_imp->current_package_id()->name())
                continue;
        }

        switch (d->_imp->throw_on_blocker ? dl_blocks_error : d->_imp->opts->blocks)
        {
            case dl_blocks_error:
                throw BlockError(stringify(*a.blocked_spec()));

            case dl_blocks_discard:
                Log::get_instance()->message("dep_list.discarding_block", ll_warning, lc_context) << "Discarding block '" << a << "'";
                break;

            case dl_blocks_discard_completely:
                break;

            case dl_blocks_accumulate:
                d->add_error_package(*aa, dlk_block, *a.blocked_spec(), conditions);
                break;

            case last_dl_blocks:
                break;
        }
    }

    for (std::list<MergeList::const_iterator>::const_iterator r(will_be_installed.begin()),
            r_end(will_be_installed.end()) ; r != r_end ; ++r)
    {
        if (! match_package(*d->_imp->env, *a.blocked_spec(), *(*r)->package_id, d->_imp->opts->match_package_options))
            continue;

        /* ignore if it's a virtual/blah (not <virtual/blah-1) block and it's blocking
         * ourself */
        if (! (a.blocked_spec()->version_requirements_ptr() || a.blocked_spec()->slot_requirement_ptr()
                    || a.blocked_spec()->additional_requirements_ptr()
                    || a.blocked_spec()->in_repository_ptr()
                    || a.blocked_spec()->from_repository_ptr()
                    || a.blocked_spec()->installed_at_path_ptr()
                    || a.blocked_spec()->installable_to_repository_ptr()
                    || a.blocked_spec()->installable_to_path_ptr())
                && d->_imp->current_package_id())
        {
            if ((*r)->package_id->name() == d->_imp->current_package_id()->name())
                continue;

            if ((*r)->package_id->virtual_for_key() && (*r)->package_id->virtual_for_key()->value()->name() ==
                    d->_imp->current_package_id()->name())
                continue;
        }

        throw BlockError(stringify(*a.blocked_spec()));
    }

    if (check_whole_list)
    {
        for (MergeList::const_iterator r(d->_imp->merge_list.begin()),
                r_end(d->_imp->merge_list.end()) ; r != r_end ; ++r)
        {
            if (! match_package(*d->_imp->env, *a.blocked_spec(), *r->package_id, d->_imp->opts->match_package_options))
                continue;

            /* ignore if it's a virtual/blah (not <virtual/blah-1) block and it's blocking
             * ourself */
            if (! (a.blocked_spec()->version_requirements_ptr() || a.blocked_spec()->slot_requirement_ptr()
                        || a.blocked_spec()->additional_requirements_ptr()
                        || a.blocked_spec()->in_repository_ptr()
                        || a.blocked_spec()->from_repository_ptr()
                        || a.blocked_spec()->installed_at_path_ptr()
                        || a.blocked_spec()->installable_to_repository_ptr()
                        || a.blocked_spec()->installable_to_path_ptr())
                    && d->_imp->current_package_id())
            {
                if (r->package_id->name() == d->_imp->current_package_id()->name())
                    continue;

                if (r->package_id->virtual_for_key() &&
                        r->package_id->virtual_for_key()->value()->name() == d->_imp->current_package_id()->name())
                    continue;
            }

            throw BlockError(stringify(*a.blocked_spec()));
        }
    }
}

void
DepList::AddVisitor::visit_leaf(const DependencyLabelsDepSpec & spec)
{
    d->_imp->labels.begin()->reset(new ActiveDependencyLabels(**d->_imp->labels.begin(), spec));
}

DepList::DepList(const Environment * const e, const DepListOptions & o) :
    PrivateImplementationPattern<DepList>(new Implementation<DepList>(e, o))
{
}

DepList::~DepList()
{
}

std::tr1::shared_ptr<DepListOptions>
DepList::options()
{
    return _imp->opts;
}

const std::tr1::shared_ptr<const DepListOptions>
DepList::options() const
{
    return _imp->opts;
}

void
DepList::clear()
{
    DepListOptions o(*options());
    _imp.reset(new Implementation<DepList>(_imp->env, o));
}

void
DepList::add_in_role(const bool only_if_not_suggested_label, DependencySpecTree::ConstItem & spec, const std::string & role,
        const std::tr1::shared_ptr<const DestinationsSet> & destinations)
{
    Context context("When adding " + role + (only_if_not_suggested_label ? " unless under a suggested label" : "") + ":");
    add_not_top_level(only_if_not_suggested_label, spec, destinations,
        std::tr1::shared_ptr<ConstTreeSequence<DependencySpecTree, AllDepSpec> >(
            new ConstTreeSequence<DependencySpecTree, AllDepSpec>(std::tr1::shared_ptr<AllDepSpec>(new AllDepSpec))));
}

void
DepList::add_not_top_level(const bool only_if_not_suggested_label,
        DependencySpecTree::ConstItem & spec, const std::tr1::shared_ptr<const DestinationsSet> & destinations,
                           const std::tr1::shared_ptr<ConstTreeSequence<DependencySpecTree, AllDepSpec> > & conditions)
{
    DepListTransaction transaction(_imp->merge_list, _imp->merge_list_index, _imp->merge_list_generation);

    AddVisitor visitor(this, only_if_not_suggested_label, destinations, conditions);
    spec.accept(visitor);
    transaction.commit();
}

void
DepList::add(SetSpecTree::ConstItem & spec, const std::tr1::shared_ptr<const DestinationsSet> & destinations)
{
    DepListTransaction transaction(_imp->merge_list, _imp->merge_list_index, _imp->merge_list_generation);

    Save<SetSpecTree::ConstItem *> save_current_top_level_target(&_imp->current_top_level_target,
            _imp->current_top_level_target ? _imp->current_top_level_target : &spec);

    AddVisitor visitor(this, false, destinations);
    spec.accept(visitor);
    transaction.commit();
}

void
DepList::add(const PackageDepSpec & spec, const std::tr1::shared_ptr<const DestinationsSet> & destinations)
{
    TreeLeaf<SetSpecTree, PackageDepSpec> l(std::tr1::shared_ptr<PackageDepSpec>(new PackageDepSpec(spec)));
    add(l, destinations);
}

void
DepList::add_package(const std::tr1::shared_ptr<const PackageID> & p, const std::tr1::shared_ptr<const DepTag> & tag,
        const PackageDepSpec & pds, const std::tr1::shared_ptr<DependencySpecTree::ConstItem> & conditions,
        const std::tr1::shared_ptr<const DestinationsSet> & destinations)
{
    Context context("When adding package '" + stringify(*p) + "':");

    Save<MergeList::iterator> save_merge_list_insert_position(&_imp->merge_list_insert_position);

    /* create our merge list entry. insert pre deps before ourself in the list. insert
     * post deps after ourself, and after any provides. */

    MergeList::iterator our_merge_entry_position(
            _imp->merge_list.insert(_imp->merge_list_insert_position,
                DepListEntry::create()
                .package_id(p)
                .generation(_imp->merge_list_generation)
                .state(dle_no_deps)
                .tags(std::tr1::shared_ptr<DepListEntryTags>(new DepListEntryTags))
                .destination(p->virtual_for_key() ? std::tr1::shared_ptr<Repository>() : find_destination(*p, destinations))
                .associated_entry(0)
                .handled(p->virtual_for_key() ?
                    std::tr1::shared_ptr<DepListEntryHandled>(new DepListEntryNoHandlingRequired) :
                    std::tr1::shared_ptr<DepListEntryHandled>(new DepListEntryUnhandled))
                .kind(p->virtual_for_key() ? dlk_virtual : dlk_package))),
        our_merge_entry_post_position(our_merge_entry_position);

    _imp->merge_list_index.insert(std::make_pair(p->name(), our_merge_entry_position));

    if (tag)
        our_merge_entry_position->tags->insert(DepTagEntry::create()
                .generation(_imp->merge_list_generation)
                .tag(tag));

    if (_imp->opts->dependency_tags && _imp->current_package_id())
        our_merge_entry_position->tags->insert(DepTagEntry::create()
                .tag(std::tr1::shared_ptr<DepTag>(new DependencyDepTag(_imp->current_package_id(), pds, conditions)))
                .generation(_imp->merge_list_generation));

    Save<MergeList::const_iterator> save_current_merge_list_entry(&_imp->current_merge_list_entry,
            our_merge_entry_position);

    _imp->merge_list_insert_position = our_merge_entry_position;

    /* add provides */
    if (p->provide_key())
    {
        DepSpecFlattener<ProvideSpecTree, PackageDepSpec> f(_imp->env);
        p->provide_key()->value()->accept(f);

        if (f.begin() != f.end() && ! (*DistributionData::get_instance()->distribution_from_string(
                    _imp->env->distribution())).support_old_style_virtuals())
            throw DistributionConfigurationError("Package '" + stringify(*p) + "' has PROVIDEs, but this distribution "
                    "does not support old style virtuals");

        for (DepSpecFlattener<ProvideSpecTree, PackageDepSpec>::ConstIterator i(f.begin()), i_end(f.end()) ; i != i_end ; ++i)
        {
            std::tr1::shared_ptr<PackageDepSpec> pp(new PackageDepSpec(make_package_dep_spec()
                        .package(*(*i)->package_ptr())
                        .version_requirement(VersionRequirement(vo_equal, p->version()))));

            std::pair<MergeListIndex::iterator, MergeListIndex::iterator> z;
            if (pp->package_ptr())
                z = _imp->merge_list_index.equal_range(*pp->package_ptr());
            else
                z = std::make_pair(_imp->merge_list_index.begin(), _imp->merge_list_index.end());

            MergeListIndex::iterator zz(std::find_if(z.first, z.second,
                MatchDepListEntryAgainstPackageDepSpec(_imp->env, *pp, _imp->opts->match_package_options)));

            if (zz != z.second)
                continue;

            our_merge_entry_post_position = _imp->merge_list.insert(next(our_merge_entry_post_position),
                    DepListEntry(DepListEntry::create()
                        .package_id((*_imp->env->package_database()->fetch_repository(
                                    RepositoryName("virtuals"))).make_virtuals_interface()->make_virtual_package_id(
                                QualifiedPackageName((*i)->text()), p))
                        .generation(_imp->merge_list_generation)
                        .state(dle_has_all_deps)
                        .tags(std::tr1::shared_ptr<DepListEntryTags>(new DepListEntryTags))
                        .destination(std::tr1::shared_ptr<Repository>())
                        .associated_entry(&*_imp->current_merge_list_entry)
                        .handled(make_shared_ptr(new DepListEntryNoHandlingRequired))
                        .kind(dlk_provided)));
            _imp->merge_list_index.insert(std::make_pair((*i)->text(), our_merge_entry_post_position));
        }
    }

    /* add suggests */
    if (_imp->opts->suggested == dl_suggested_show && p->suggested_dependencies_key())
    {
        Context c("When showing suggestions:");
        Save<MergeList::iterator> suggest_save_merge_list_insert_position(&_imp->merge_list_insert_position,
                next(our_merge_entry_position));
        ShowSuggestVisitor visitor(this, destinations, _imp->env, _imp->current_package_id(), _imp->opts->dependency_tags, false);
        p->suggested_dependencies_key()->value()->accept(visitor);
    }

    /* add suggests in post depend too */
    if (_imp->opts->suggested == dl_suggested_show && p->post_dependencies_key())
    {
        Context c("When showing suggestions in post dependencies key:");
        Save<MergeList::iterator> suggest_save_merge_list_insert_position(&_imp->merge_list_insert_position,
                next(our_merge_entry_position));
        ShowSuggestVisitor visitor(this, destinations, _imp->env, _imp->current_package_id(), _imp->opts->dependency_tags, true);
        p->post_dependencies_key()->value()->accept(visitor);
    }

    /* add pre dependencies */
    if (p->build_dependencies_key())
        add_predeps(*p->build_dependencies_key()->value(), _imp->opts->uninstalled_deps_pre, "build", destinations, false);
    if (p->run_dependencies_key())
        add_predeps(*p->run_dependencies_key()->value(), _imp->opts->uninstalled_deps_runtime, "run", destinations, false);
    if (p->post_dependencies_key())
        add_predeps(*p->post_dependencies_key()->value(), _imp->opts->uninstalled_deps_post, "post", destinations,
                (_imp->opts->suggested == dl_suggested_install) ? false : true);
    if (_imp->opts->suggested == dl_suggested_install && p->suggested_dependencies_key())
        add_predeps(*p->suggested_dependencies_key()->value(), _imp->opts->uninstalled_deps_suggested, "suggest", destinations, false);

    our_merge_entry_position->state = dle_has_pre_deps;
    _imp->merge_list_insert_position = next(our_merge_entry_post_position);

    /* add post dependencies */
    if (p->build_dependencies_key())
        add_postdeps(*p->build_dependencies_key()->value(), _imp->opts->uninstalled_deps_pre, "build", destinations, false);
    if (p->run_dependencies_key())
        add_postdeps(*p->run_dependencies_key()->value(), _imp->opts->uninstalled_deps_runtime, "run", destinations, false);
    if (p->post_dependencies_key())
        add_postdeps(*p->post_dependencies_key()->value(), _imp->opts->uninstalled_deps_post, "post", destinations,
                (_imp->opts->suggested == dl_suggested_install) ? false : true);

    if (_imp->opts->suggested == dl_suggested_install && p->suggested_dependencies_key())
        add_postdeps(*p->suggested_dependencies_key()->value(), _imp->opts->uninstalled_deps_suggested, "suggest", destinations, false);

    our_merge_entry_position->state = dle_has_all_deps;
}

void
DepList::add_error_package(const std::tr1::shared_ptr<const PackageID> & p, const DepListEntryKind kind,
        const PackageDepSpec & pds, const std::tr1::shared_ptr<DependencySpecTree::ConstItem> & conditions)
{
    std::pair<MergeListIndex::iterator, MergeListIndex::const_iterator> pp(
            _imp->merge_list_index.equal_range(p->name()));

    for ( ; pp.second != pp.first ; ++pp.first)
    {
        if (pp.first->second->kind == kind && *pp.first->second->package_id == *p)
        {
            if (_imp->current_package_id())
                pp.first->second->tags->insert(DepTagEntry::create()
                        .tag(std::tr1::shared_ptr<DepTag>(new DependencyDepTag(_imp->current_package_id(), pds, conditions)))
                        .generation(_imp->merge_list_generation));
            return;
        }
    }

    MergeList::iterator our_merge_entry_position(
            _imp->merge_list.insert(_imp->merge_list.begin(),
                DepListEntry::create()
                .package_id(p)
                .generation(_imp->merge_list_generation)
                .state(dle_has_all_deps)
                .tags(std::tr1::shared_ptr<DepListEntryTags>(new DepListEntryTags))
                .destination(std::tr1::shared_ptr<Repository>())
                .associated_entry(&*_imp->current_merge_list_entry)
                .handled(make_shared_ptr(new DepListEntryNoHandlingRequired))
                .kind(kind)));

    if (_imp->current_package_id())
        our_merge_entry_position->tags->insert(DepTagEntry::create()
                .tag(std::tr1::shared_ptr<DepTag>(new DependencyDepTag(_imp->current_package_id(), pds, conditions)))
                .generation(_imp->merge_list_generation));

    _imp->merge_list_index.insert(std::make_pair(p->name(), our_merge_entry_position));
}

void
DepList::add_suggested_package(const std::tr1::shared_ptr<const PackageID> & p,
        const PackageDepSpec & pds, const std::tr1::shared_ptr<DependencySpecTree::ConstItem> & conditions,
        const std::tr1::shared_ptr<const DestinationsSet> & destinations)
{
    std::pair<MergeListIndex::iterator, MergeListIndex::const_iterator> pp(
            _imp->merge_list_index.equal_range(p->name()));

    for ( ; pp.second != pp.first ; ++pp.first)
    {
        if ((pp.first->second->kind == dlk_suggested || pp.first->second->kind == dlk_already_installed
                    || pp.first->second->kind == dlk_package || pp.first->second->kind == dlk_provided
                    || pp.first->second->kind == dlk_subpackage) && *pp.first->second->package_id == *p)
            return;
    }

    MergeList::iterator our_merge_entry_position(
            _imp->merge_list.insert(_imp->merge_list_insert_position,
                DepListEntry::create()
                .package_id(p)
                .generation(_imp->merge_list_generation)
                .state(dle_has_all_deps)
                .tags(std::tr1::shared_ptr<DepListEntryTags>(new DepListEntryTags))
                .destination(find_destination(*p, destinations))
                .associated_entry(&*_imp->current_merge_list_entry)
                .handled(make_shared_ptr(new DepListEntryNoHandlingRequired))
                .kind(dlk_suggested)));

    if (_imp->current_package_id())
        our_merge_entry_position->tags->insert(DepTagEntry::create()
                .tag(std::tr1::shared_ptr<DepTag>(new DependencyDepTag(_imp->current_package_id(), pds, conditions)))
                .generation(_imp->merge_list_generation));

    _imp->merge_list_index.insert(std::make_pair(p->name(), our_merge_entry_position));
}

void
DepList::add_predeps(DependencySpecTree::ConstItem & d, const DepListDepsOption opt, const std::string & s,
        const std::tr1::shared_ptr<const DestinationsSet> & destinations, const bool only_if_not_suggested_label)
{
    if (dl_deps_pre == opt || dl_deps_pre_or_post == opt)
    {
        try
        {
            add_in_role(only_if_not_suggested_label, d, s + " dependencies as pre dependencies", destinations);
        }
        catch (const DepListError & e)
        {
            if (dl_deps_pre == opt)
                throw;
            else
                Log::get_instance()->message("dep_list.dropping_dependencies", ll_warning, lc_context)
                    << "Dropping " << s << " dependencies to post dependencies because of exception '"
                    << e.message() << "' (" << e.what() << ")";
        }
    }
}

void
DepList::add_postdeps(DependencySpecTree::ConstItem & d, const DepListDepsOption opt, const std::string & s,
        const std::tr1::shared_ptr<const DestinationsSet> & destinations, const bool only_if_not_suggested_label)
{
    if (dl_deps_pre_or_post == opt || dl_deps_post == opt || dl_deps_try_post == opt)
    {
        try
        {
            try
            {
                add_in_role(only_if_not_suggested_label, d, s + " dependencies as post dependencies", destinations);
            }
            catch (const CircularDependencyError &)
            {
                Save<DepListCircularOption> save_circular(&_imp->opts->circular,
                        _imp->opts->circular == dl_circular_discard_silently ?
                        dl_circular_discard_silently : dl_circular_discard);
                Save<MergeList::iterator> save_merge_list_insert_position(&_imp->merge_list_insert_position,
                        _imp->merge_list.end());
                add_in_role(only_if_not_suggested_label, d, s + " dependencies as post dependencies with cycle breaking", destinations);
            }
        }
        catch (const DepListError & e)
        {
            if (dl_deps_try_post != opt)
                throw;
            else
                Log::get_instance()->message("dep_list.ignoring_dependencies", ll_warning, lc_context)
                    << "Ignoring " << s << " dependencies due to exception '" << e.message() << "' (" << e.what() << ")";
        }
    }
}

void
DepList::add_already_installed_package(const std::tr1::shared_ptr<const PackageID> & p, const std::tr1::shared_ptr<const DepTag> & tag,
        const PackageDepSpec & pds, const std::tr1::shared_ptr<DependencySpecTree::ConstItem> & conditions,
        const std::tr1::shared_ptr<const DestinationsSet> & destinations)
{
    Context context("When adding installed package '" + stringify(*p) + "':");

    Save<MergeList::iterator> save_merge_list_insert_position(&_imp->merge_list_insert_position);

    MergeList::iterator our_merge_entry(_imp->merge_list.insert(_imp->merge_list_insert_position,
                DepListEntry::create()
                .package_id(p)
                .generation(_imp->merge_list_generation)
                .tags(std::tr1::shared_ptr<DepListEntryTags>(new DepListEntryTags))
                .state(dle_has_pre_deps)
                .destination(std::tr1::shared_ptr<Repository>())
                .associated_entry(0)
                .handled(make_shared_ptr(new DepListEntryNoHandlingRequired))
                .kind(dlk_already_installed)));
    _imp->merge_list_index.insert(std::make_pair(p->name(), our_merge_entry));

    if (tag)
        our_merge_entry->tags->insert(DepTagEntry::create()
                .generation(_imp->merge_list_generation)
                .tag(tag));

    if (_imp->opts->dependency_tags && _imp->current_package_id())
        our_merge_entry->tags->insert(DepTagEntry::create()
                .tag(std::tr1::shared_ptr<DepTag>(new DependencyDepTag(_imp->current_package_id(), pds, conditions)))
                .generation(_imp->merge_list_generation));

    Save<MergeList::const_iterator> save_current_merge_list_entry(&_imp->current_merge_list_entry,
            our_merge_entry);

    if (p->build_dependencies_key())
        add_predeps(*p->build_dependencies_key()->value(), _imp->opts->installed_deps_pre, "build", destinations, false);
    if (p->run_dependencies_key())
        add_predeps(*p->run_dependencies_key()->value(), _imp->opts->installed_deps_runtime, "run", destinations, false);
    if (p->post_dependencies_key())
        add_predeps(*p->post_dependencies_key()->value(), _imp->opts->installed_deps_post, "post", destinations, true);

    our_merge_entry->state = dle_has_pre_deps;
    _imp->merge_list_insert_position = next(our_merge_entry);

    if (p->build_dependencies_key())
        add_postdeps(*p->build_dependencies_key()->value(), _imp->opts->installed_deps_pre, "build", destinations, false);
    if (p->run_dependencies_key())
        add_postdeps(*p->run_dependencies_key()->value(), _imp->opts->installed_deps_runtime, "run", destinations, false);
    if (p->post_dependencies_key())
        add_postdeps(*p->post_dependencies_key()->value(), _imp->opts->installed_deps_post, "post", destinations, true);
}

namespace
{
    bool is_scm(const QualifiedPackageName & n)
    {
        std::string pkg(stringify(n.package()));
        switch (pkg.length())
        {
            case 0:
            case 1:
            case 2:
            case 3:
                return false;

            default:
                if (0 == pkg.compare(pkg.length() - 6, 6, "-darcs"))
                    return true;

            case 5:
                if (0 == pkg.compare(pkg.length() - 5, 5, "-live"))
                    return true;

            case 4:
                if (0 == pkg.compare(pkg.length() - 4, 4, "-cvs"))
                    return true;
                if (0 == pkg.compare(pkg.length() - 4, 4, "-svn"))
                    return true;
                return false;
        }
    }
}

bool
DepList::prefer_installed_over_uninstalled(const PackageID & installed,
        const PackageID & uninstalled)
{
    do
    {
        switch (_imp->opts->target_type)
        {
            case dl_target_package:
                if (! _imp->current_package_id())
                    return false;

                if (is_top_level_target(uninstalled))
                    return false;

                continue;

            case dl_target_set:
                continue;

            case last_dl_target:
                ;
        }

        throw InternalError(PALUDIS_HERE, "Bad target_type value '" + stringify(_imp->opts->target_type) + "'");
    } while (false);

    if (dl_reinstall_always == _imp->opts->reinstall)
            return false;

    if (dl_upgrade_as_needed == _imp->opts->upgrade)
        return true;

    if (dl_reinstall_scm_never != _imp->opts->reinstall_scm)
        if (uninstalled.version() == installed.version() &&
                (installed.version().is_scm() || is_scm(installed.name())))
        {
            static time_t current_time(time(0)); /* static to avoid weirdness */
            time_t installed_time(current_time);
            if (installed.installed_time_key())
                installed_time = installed.installed_time_key()->value();

            do
            {
                switch (_imp->opts->reinstall_scm)
                {
                    case dl_reinstall_scm_always:
                        return false;

                    case dl_reinstall_scm_daily:
                        if (current_time - installed_time > (24 * 60 * 60))
                            return false;
                        continue;

                    case dl_reinstall_scm_weekly:
                        if (current_time - installed_time > (24 * 60 * 60 * 7))
                            return false;
                        continue;

                    case dl_reinstall_scm_never:
                        ; /* nothing */

                    case last_dl_reinstall_scm:
                        ;
                }

                throw InternalError(PALUDIS_HERE, "Bad value for opts->reinstall_scm");
            } while (false);
        }

    /* use != rather than > to correctly force a downgrade when packages are
     * removed. */
    if (uninstalled.version() != installed.version())
        return false;

    if (dl_reinstall_if_use_changed == _imp->opts->reinstall)
    {
        std::set<ChoiceNameWithPrefix> common;
        if (installed.choices_key() && uninstalled.choices_key())
        {
            std::set<ChoiceNameWithPrefix> i_common, u_common;
            for (Choices::ConstIterator k(installed.choices_key()->value()->begin()),
                    k_end(installed.choices_key()->value()->end()) ;
                    k != k_end ; ++k)
            {
                if (! (*k)->consider_added_or_changed())
                    continue;

                for (Choice::ConstIterator i((*k)->begin()), i_end((*k)->end()) ;
                        i != i_end ; ++i)
                    if ((*i)->explicitly_listed())
                        i_common.insert((*i)->name_with_prefix());
            }

            for (Choices::ConstIterator k(uninstalled.choices_key()->value()->begin()),
                    k_end(uninstalled.choices_key()->value()->end()) ;
                    k != k_end ; ++k)
            {
                if (! (*k)->consider_added_or_changed())
                    continue;

                for (Choice::ConstIterator i((*k)->begin()), i_end((*k)->end()) ;
                        i != i_end ; ++i)
                    if ((*i)->explicitly_listed())
                        u_common.insert((*i)->name_with_prefix());
            }

            std::set_intersection(
                    i_common.begin(), i_common.end(),
                    u_common.begin(), u_common.end(),
                    std::inserter(common, common.begin()));
        }

        for (std::set<ChoiceNameWithPrefix>::const_iterator f(common.begin()), f_end(common.end()) ;
                f != f_end ; ++f)
            if (installed.choices_key()->value()->find_by_name_with_prefix(*f)->enabled() !=
                    uninstalled.choices_key()->value()->find_by_name_with_prefix(*f)->enabled())
                return false;
    }

    return true;
}

bool
DepList::already_installed(const DependencySpecTree::ConstItem & spec,
        const std::tr1::shared_ptr<const DestinationsSet> & destinations) const
{
    QueryVisitor visitor(this, destinations, _imp->env, _imp->current_package_id());
    spec.accept(visitor);
    return visitor.result();
}

DepList::Iterator
DepList::begin()
{
    return Iterator(_imp->merge_list.begin());
}

DepList::Iterator
DepList::end()
{
    return Iterator(_imp->merge_list.end());
}

DepList::ConstIterator
DepList::begin() const
{
    return ConstIterator(_imp->merge_list.begin());
}

DepList::ConstIterator
DepList::end() const
{
    return ConstIterator(_imp->merge_list.end());
}

bool
DepList::is_top_level_target(const PackageID & e) const
{
    if (! _imp->current_top_level_target)
        throw InternalError(PALUDIS_HERE, "current_top_level_target not set?");

    return match_package_in_set(*_imp->env, *_imp->current_top_level_target, e, _imp->opts->match_package_options);
}

namespace
{
    struct IsError
    {
        bool operator() (const DepListEntry & e) const
        {
            switch (e.kind)
            {
                case dlk_virtual:
                case dlk_package:
                case dlk_provided:
                case dlk_already_installed:
                case dlk_subpackage:
                case dlk_suggested:
                    return false;

                case dlk_block:
                case dlk_masked:
                    return true;

                case last_dlk:
                    ;
            }

            throw InternalError(PALUDIS_HERE, "Bad e.kind");
        }
    };
}

bool
DepList::has_errors() const
{
    return end() != std::find_if(begin(), end(), IsError());
}

std::tr1::shared_ptr<Repository>
DepList::find_destination(const PackageID & p,
        const std::tr1::shared_ptr<const DestinationsSet> & dd)
{
    for (DestinationsSet::ConstIterator d(dd->begin()), d_end(dd->end()) ;
             d != d_end ; ++d)
        if ((**d).destination_interface())
            if ((**d).destination_interface()->is_suitable_destination_for(p))
                return *d;

    throw NoDestinationError(p, dd);
}

bool
DepList::replaced(const PackageID & m) const
{
    std::pair<MergeListIndex::const_iterator, MergeListIndex::const_iterator> p(
            _imp->merge_list_index.equal_range(m.name()));

    PackageDepSpec spec(make_package_dep_spec().package(m.name()));
    while (p.second != ((p.first = std::find_if(p.first, p.second,
                        MatchDepListEntryAgainstPackageDepSpec(_imp->env, spec, _imp->opts->match_package_options)))))
    {
        if (p.first->second->package_id->slot() != m.slot())
            p.first = next(p.first);
        else
            return true;
    }

    return false;
}

bool
DepList::match_on_list(const PackageDepSpec & a) const
{
    std::pair<MergeListIndex::const_iterator, MergeListIndex::const_iterator> p;
    if (a.package_ptr())
        p = _imp->merge_list_index.equal_range(*a.package_ptr());
    else
        p = std::make_pair(_imp->merge_list_index.begin(), _imp->merge_list_index.end());

    return p.second != std::find_if(p.first, p.second,
            MatchDepListEntryAgainstPackageDepSpec(_imp->env, a, _imp->opts->match_package_options));
}

DepList::Iterator
DepList::push_back(const DepListEntry & e)
{
    MergeList::iterator our_merge_entry_position(_imp->merge_list.insert(_imp->merge_list.end(), e));
    _imp->merge_list_index.insert(std::make_pair(e.package_id->name(), our_merge_entry_position));
    return Iterator(our_merge_entry_position);
}

bool
paludis::is_viable_any_child(const DependencySpecTree::ConstItem & i)
{
    const ConditionalDepSpec * const u(get_const_item(i)->as_conditional_dep_spec());
    if (0 != u)
        return u->condition_met();
    else
        return true;
}

