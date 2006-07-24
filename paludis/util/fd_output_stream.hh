/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2006 Ciaran McCreesh <ciaran.mccreesh@blueyonder.co.uk>
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

#ifndef PALUDIS_GUARD_PALUDIS_UTIL_FD_OUTPUT_STREAM_HH
#define PALUDIS_GUARD_PALUDIS_UTIL_FD_OUTPUT_STREAM_HH 1

#include <ostream>
#include <unistd.h>

namespace paludis
{
    /**
     * Output stream buffer class that's opened via an FD.
     *
     * See \ref TCppSL Ch. 13.13 for what we're doing here. The buffer code is
     * based upon the "io/outbuf2.hpp" example in section 13.13.3.
     *
     * \ingroup grpfdotputstream
     */
    class FDOutputStreamBuf :
        public std::streambuf
    {
        protected:
            int fd;

            virtual int_type
            overflow(int_type c)
            {
                if (c != EOF)
                {
                    char z = c;
                    if (1 != write(fd, &z, 1))
                        return EOF;
                }
                return c;
            }

            virtual std::streamsize
            xsputn(const char * s, std::streamsize num)
            {
                return write(fd, s, num);
            }

        public:
            FDOutputStreamBuf(const int f) :
                fd(f)
            {
            }
    };

    /**
     * Member from base initialisation for FDOutputStream.
     *
     * \ingroup grpfdotputstream
     */
    class FDOutputStreamBase
    {
        protected:
            FDOutputStreamBuf buf;

        public:
            FDOutputStreamBase(const int fd) :
                buf(fd)
            {
            }
    };

    /**
     * Output stream buffer class that's opened via an FD.
     *
     * \ingroup grpfdotputstream
     */
    class FDOutputStream :
        protected FDOutputStreamBase,
        public std::ostream
    {
        public:
            FDOutputStream(const int fd) :
                FDOutputStreamBase(fd),
                std::ostream(&buf)
        {
        }
    };
}

#endif
