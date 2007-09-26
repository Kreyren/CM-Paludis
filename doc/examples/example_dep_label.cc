/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/** \file
 *
 * Example \ref example_dep_label.cc "example_dep_label.cc" .
 *
 * \ingroup g_dep_spec
 */

/** \example example_dep_label.cc
 *
 * This example demonstrates how to handle dependency labels. It produces a
 * summary of distfiles for all installed packages, together with a notice of
 * whether that distfile is fetch-restricted.
 */

#include <paludis/paludis.hh>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cstdlib>
#include <list>
#include <map>

using namespace paludis;
using std::cout;
using std::endl;
using std::setw;

/* We store our results in a map from distfile name to whether it is fetch
 * restricted. */
typedef std::map<std::string, bool> ResultsMap;

namespace
{
    /* This visitor class is used to determine whether a label represents a
     * fetch restriction. */
    class IsLabelRestrictedVisitor :
        public ConstVisitor<URILabelVisitorTypes>
    {
        public:
            bool result;

            IsLabelRestrictedVisitor(const bool initial) :
                result(initial)
            {
            }

            void visit(const URIListedThenMirrorsLabel &)
            {
                result = false;
            }

            void visit(const URIListedOnlyLabel &)
            {
                result = false;
            }

            void visit(const URIMirrorsOnlyLabel &)
            {
                result = false;
            }

            void visit(const URIMirrorsThenListedLabel &)
            {
                result = false;
            }

            void visit(const URILocalMirrorsOnlyLabel &)
            {
                result = true;
            }

            void visit(const URIManualOnlyLabel &)
            {
                result = true;
            }
    };

    /* This visitor class collects src_uri entries and stores the result in
     * a provided map. Label statuses are handled by a stack. When we enter
     * a block (an AllDepSpec or a UseDepSpec), we duplicate the top item
     * of the stack, since labels recurse into subblocks. When we encounter
     * a label, we replace the top item of the stack. */
    class DistfilesCollector :
        public ConstVisitor<URISpecTree>
    {
        private:
            ResultsMap & _results;
            std::list<bool> _restricted;

        public:
            DistfilesCollector(ResultsMap & r, const bool initial) :
                _results(r)
            {
                _restricted.push_back(initial);
            }

            void visit_sequence(const AllDepSpec &,
                    URISpecTree::ConstSequenceIterator cur,
                    URISpecTree::ConstSequenceIterator end)
            {
                /* When we encounter an AllDepSpec, duplicate the top item of
                 * our restricted stack, and then recurse over all of its
                 * children, and then restore the stack. */
                _restricted.push_back(_restricted.back());
                std::for_each(cur, end, accept_visitor(*this));
                _restricted.pop_back();
            }

            void visit_sequence(const UseDepSpec &,
                    URISpecTree::ConstSequenceIterator cur,
                    URISpecTree::ConstSequenceIterator end)
            {
                /* Always recurse over a UseDepSpec's children. In real world
                 * code, we would more likely check whether the use flag is
                 * accepted. */
                _restricted.push_back(_restricted.back());
                std::for_each(cur, end, accept_visitor(*this));
                _restricted.pop_back();
            }

            void visit_leaf(const URIDepSpec & s)
            {
                /* When we encounter a URIDepSpec, store its distfile name.
                 * We handle 'a -> b' style specs by taking 'b' as the
                 * distfile name. */
                _results.insert(std::make_pair(s.renamed_url_suffix(), _restricted.back()));
            }

            void visit_leaf(const LabelsDepSpec<URILabelVisitorTypes> & l)
            {
                /* Find out whether the label represents a fetch restriction.
                 * Change the top item of the stack as appropriate. Although
                 * a LabelsDepSpec can contain multiple labels, only the last
                 * one is relevant. */
                IsLabelRestrictedVisitor v(_restricted.back());
                std::for_each(indirect_iterator(l.begin()), indirect_iterator(l.end()), accept_visitor(v));
                _restricted.back() = v.result;
            }
    };
}

int main(int, char *[])
{
    /* We start with an Environment. */
    tr1::shared_ptr<Environment> env(EnvironmentMaker::get_instance()->make_from_spec(""));

    /* Fetch package IDs for all installed packages. */
    tr1::shared_ptr<const PackageIDSequence> ids(env->package_database()->query(
                query::SupportsAction<InstalledAction>(),
                qo_whatever));

    /* Store a map from distfile name to whether it is fetch restricted. */
    ResultsMap results;

    /* For each ID: */
    for (PackageIDSet::ConstIterator i(ids->begin()), i_end(ids->end()) ;
            i != i_end ; ++i)
    {
        /* If we don't have a src_uri key, skip this package. All PackageID
         * _key() functions can potentially return zero pointers, so checking is
         * essential. */
        if (! (*i)->src_uri_key())
            continue;

        /* We need to know whether the default label for this package's src_uri
         * is restricted. */
        IsLabelRestrictedVisitor is_initial_label_restricted(false);
        (*i)->src_uri_key()->initial_label()->accept(is_initial_label_restricted);

        /* Create a visitor that will collect distfiles, and do the collecting. */
        DistfilesCollector collector(results, is_initial_label_restricted.result);
        (*i)->src_uri_key()->value()->accept(collector);
    }

    /* Display summary of results */
    cout << setw(60) << "Distfile Name" << ": " << "Fetch Restricted?" << endl;
    for (ResultsMap::const_iterator r(results.begin()), r_end(results.end()) ;
            r != r_end ; ++r)
        cout << setw(60) << r->first << ": " << (r->second ? "yes" : "no") << endl;

    return EXIT_SUCCESS;
}


