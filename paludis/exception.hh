/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2005, 2006 Ciaran McCreesh <ciaranm@gentoo.org>
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

#ifndef PALUDIS_GUARD_PALUDIS_EXCEPTION_HH
#define PALUDIS_GUARD_PALUDIS_EXCEPTION_HH 1

#include <paludis/attributes.hh>
#include <paludis/stringify.hh>

#include <string>
#include <exception>
#include <libebt/libebt.hh>

/** \file
 * Declaration for the Exception base class, the InternalError exception
 * class, the NameError class and related utilities.
 *
 * \ingroup Exception
 */

namespace paludis
{
    /**
     * Context tag for libebt.
     *
     * \ingroup Exception
     */
    struct PaludisBacktraceTag
    {
    };

    /**
     * Backtrace context class.
     *
     * \ingroup Exception
     */
    typedef libebt::BacktraceContext<PaludisBacktraceTag> Context;

    /**
     * Base exception class.
     *
     * \ingroup Exception
     */
    class Exception :
        public std::exception,
        public libebt::Backtraceable<PaludisBacktraceTag>
    {
        private:
            const std::string _message;

        protected:
            /**
             * Constructor.
             */
            Exception(const std::string & message) throw ();

        public:
            /**
             * Destructor.
             */
            virtual ~Exception() throw () PALUDIS_ATTRIBUTE((nothrow));

            /**
             * Return our descriptive error message.
             */
            const std::string & message() const throw () PALUDIS_ATTRIBUTE((nothrow));
    };

    /**
     * An InternalError is an Exception that is thrown if something that is
     * never supposed to happen happens.
     *
     * \ingroup Exception
     */
    class InternalError : public Exception
    {
        public:
            /**
             * Constructor.
             *
             * \param where Should be set to the PALUDIS_HERE macro.
             *
             * \param message A short message.
             */
            InternalError(const std::string & where, const std::string & message) throw ();

            /**
             * Constructor, with no message (deprecated).
             *
             * \param where Should be set to the PALUDIS_HERE macro.
             *
             * \deprecated Use paludis::InternalError::InternalError(const char * const,
             * const std::string &) instead.
             */
            InternalError(const std::string & where) throw () PALUDIS_ATTRIBUTE((deprecated));
    };

    /**
     * A NameError is an Exception that is thrown when some kind of invalid
     * name is encountered.
     *
     * \ingroup Exception
     * \ingroup Database
     */
    class NameError : public Exception
    {
        protected:
            /**
             * Constructor.
             *
             * \param name The invalid name encountered.
             * \param role The role for the name, for example "package name".
             */
            NameError(const std::string & name, const std::string & role) throw ();
    };

    /**
     * A ConfigurationError is thrown when an invalid configuration occurs.
     *
     * \ingroup Exception
     * \ingroup ConfigFile
     */
    class ConfigurationError : public Exception
    {
        public:
            /**
             * Constructor.
             */
            ConfigurationError(const std::string & msg) throw ();
    };
}

/** \def PALUDIS_HERE
 * Expands to the current function name, file and line, for use with
 * paludis::InternalError.
 *
 * \ingroup Exception
 */
#define PALUDIS_HERE (paludis::stringify(__PRETTY_FUNCTION__) + " at " + \
        paludis::stringify(__FILE__) + ":" + paludis::stringify(__LINE__))

#endif
