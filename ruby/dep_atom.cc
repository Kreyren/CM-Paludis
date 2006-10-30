/* vim: set sw=4 sts=4 et foldmethod=syntax : */

/*
 * Copyright (c) 2006 Ciaran McCreesh <ciaranm@ciaranm.org>
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

#include <paludis_ruby.hh>
#include <paludis/dep_atom.hh>
#include <ruby.h>

using namespace paludis;
using namespace paludis::ruby;

#define RUBY_FUNC_CAST(x) reinterpret_cast<VALUE (*)(...)>(x)

namespace
{
    static VALUE c_dep_atom;
    static VALUE c_composite_dep_atom;
    static VALUE c_package_dep_atom;
    static VALUE c_plain_text_dep_atom;
    static VALUE c_all_dep_atom;
    static VALUE c_any_dep_atom;
    static VALUE c_use_dep_atom;
    static VALUE c_block_dep_atom;
    static VALUE c_string_dep_atom;

    VALUE
    dep_atom_init_0(VALUE self)
    {
        return self;
    }

    VALUE
    dep_atom_init_1(VALUE self, VALUE)
    {
        return self;
    }

    VALUE
    dep_atom_init_2(VALUE self, VALUE, VALUE)
    {
        return self;
    }

    VALUE
    use_dep_atom_new(VALUE self, VALUE pkg, VALUE inverse)
    {
        UseDepAtom::ConstPointer * ptr(0);
        try
        {
            ptr = new UseDepAtom::ConstPointer(new UseDepAtom(
                        UseFlagName(STR2CSTR(pkg)), Qfalse != inverse && Qnil != inverse));
            VALUE tdata(Data_Wrap_Struct(self, 0, &Common<UseDepAtom::ConstPointer>::free, ptr));
            rb_obj_call_init(tdata, 2, &pkg);
            return tdata;
        }
        catch (const std::exception & e)
        {
            delete ptr;
            exception_to_ruby_exception(e);
        }
    }

    VALUE
    block_dep_atom_new(VALUE self, VALUE atom)
    {
        BlockDepAtom::ConstPointer * ptr(0);
        try
        {
            PackageDepAtom::ConstPointer pkg(value_to_package_dep_atom(atom));
            ptr = new BlockDepAtom::ConstPointer(new BlockDepAtom(pkg));
            VALUE tdata(Data_Wrap_Struct(self, 0, &Common<BlockDepAtom::ConstPointer>::free, ptr));
            rb_obj_call_init(tdata, 1, &atom);
            return tdata;
        }
        catch (const std::exception & e)
        {
            delete ptr;
            exception_to_ruby_exception(e);
        }
    }

    VALUE
    block_dep_atom_blocked_atom(VALUE self)
    {
        BlockDepAtom::ConstPointer * p;
        Data_Get_Struct(self, BlockDepAtom::ConstPointer, p);
        return dep_atom_to_value((*p)->blocked_atom());
    }

    template <typename A_>
    struct DepAtomThings
    {
        static VALUE
        dep_atom_new_1(VALUE self, VALUE s)
        {
            typename A_::ConstPointer * ptr(0);
            try
            {
                ptr = new typename A_::ConstPointer(new A_(STR2CSTR(s)));
                VALUE tdata(Data_Wrap_Struct(self, 0, &Common<typename A_::ConstPointer>::free, ptr));
                rb_obj_call_init(tdata, 1, &s);
                return tdata;
            }
            catch (const std::exception & e)
            {
                delete ptr;
                exception_to_ruby_exception(e);
            }
        }
    };

    VALUE
    use_dep_atom_flag(VALUE self)
    {
        UseDepAtom::ConstPointer * p;
        Data_Get_Struct(self, UseDepAtom::ConstPointer, p);
        return rb_str_new2(stringify((*p)->flag()).c_str());
    }

    VALUE
    use_dep_atom_inverse(VALUE self)
    {
        UseDepAtom::ConstPointer * p;
        Data_Get_Struct(self, UseDepAtom::ConstPointer, p);
        return (*p)->inverse() ? Qtrue : Qfalse;
    }

    VALUE
    composite_dep_atom_each(VALUE self)
    {
        CompositeDepAtom::Pointer * m_ptr;
        Data_Get_Struct(self, CompositeDepAtom::Pointer, m_ptr);
        for (CompositeDepAtom::Iterator i((*m_ptr)->begin()), i_end((*m_ptr)->end()) ; i != i_end ; ++i)
            rb_yield(dep_atom_to_value(*i));
        return self;
    }

    void do_register_dep_atom()
    {
        c_dep_atom = rb_define_class_under(paludis_module(), "DepAtom", rb_cObject);
        rb_funcall(c_dep_atom, rb_intern("private_class_method"), 1, rb_str_new2("new"));

        c_composite_dep_atom = rb_define_class_under(paludis_module(), "CompositeDepAtom", c_dep_atom);
        rb_funcall(c_composite_dep_atom, rb_intern("private_class_method"), 1, rb_str_new2("new"));
        rb_define_method(c_composite_dep_atom, "each", RUBY_FUNC_CAST(&composite_dep_atom_each), 0);
        rb_include_module(c_composite_dep_atom, rb_mEnumerable);

        c_all_dep_atom = rb_define_class_under(paludis_module(), "AllDepAtom", c_composite_dep_atom);
        rb_funcall(c_all_dep_atom, rb_intern("private_class_method"), 1, rb_str_new2("new"));

        c_any_dep_atom = rb_define_class_under(paludis_module(), "AnyDepAtom", c_composite_dep_atom);
        rb_funcall(c_any_dep_atom, rb_intern("private_class_method"), 1, rb_str_new2("new"));

        c_use_dep_atom = rb_define_class_under(paludis_module(), "UseDepAtom", c_composite_dep_atom);
        rb_funcall(c_use_dep_atom, rb_intern("private_class_method"), 1, rb_str_new2("new"));
        rb_define_method(c_use_dep_atom, "flag", RUBY_FUNC_CAST(&use_dep_atom_flag), 0);
        rb_define_method(c_use_dep_atom, "inverse?", RUBY_FUNC_CAST(&use_dep_atom_inverse), 0);

        c_string_dep_atom = rb_define_class_under(paludis_module(), "StringDepAtom", c_dep_atom);
        rb_funcall(c_string_dep_atom, rb_intern("private_class_method"), 1, rb_str_new2("new"));

        c_package_dep_atom = rb_define_class_under(paludis_module(), "PackageDepAtom", c_string_dep_atom);
        rb_define_singleton_method(c_package_dep_atom, "new", RUBY_FUNC_CAST(&DepAtomThings<PackageDepAtom>::dep_atom_new_1), 1);
        rb_define_method(c_package_dep_atom, "initialize", RUBY_FUNC_CAST(&dep_atom_init_1), 1);
        rb_define_method(c_package_dep_atom, "to_s", RUBY_FUNC_CAST(&Common<PackageDepAtom::ConstPointer>::to_s_via_ptr), 0);

        c_plain_text_dep_atom = rb_define_class_under(paludis_module(), "PlainTextDepAtom", c_string_dep_atom);
        rb_define_singleton_method(c_plain_text_dep_atom, "new", RUBY_FUNC_CAST(&DepAtomThings<PlainTextDepAtom>::dep_atom_new_1), 1);
        rb_define_method(c_plain_text_dep_atom, "initialize", RUBY_FUNC_CAST(&dep_atom_init_1), 1);
        rb_define_method(c_plain_text_dep_atom, "to_s", RUBY_FUNC_CAST(&Common<PlainTextDepAtom::ConstPointer>::to_s_via_ptr), 0);

        c_block_dep_atom = rb_define_class_under(paludis_module(), "BlockDepAtom", c_string_dep_atom);
        rb_define_singleton_method(c_block_dep_atom, "new", RUBY_FUNC_CAST(&block_dep_atom_new), 1);
        rb_define_method(c_block_dep_atom, "initialize", RUBY_FUNC_CAST(&dep_atom_init_1), 1);
        rb_define_method(c_block_dep_atom, "blocked_atom", RUBY_FUNC_CAST(&block_dep_atom_blocked_atom), 0);
    }
}

PackageDepAtom::ConstPointer
paludis::ruby::value_to_package_dep_atom(VALUE v)
{
    if (T_STRING == TYPE(v))
        return PackageDepAtom::ConstPointer(new PackageDepAtom(STR2CSTR(v)));
    else
    {
        PackageDepAtom::ConstPointer * v_ptr;
        Data_Get_Struct(v, PackageDepAtom::ConstPointer, v_ptr);
        return *v_ptr;
    }
}

VALUE
paludis::ruby::dep_atom_to_value(DepAtom::ConstPointer m)
{
    struct V :
        DepAtomVisitorTypes::ConstVisitor
    {
        VALUE value;
        DepAtom::ConstPointer mm;

        V(DepAtom::ConstPointer _m) :
            mm(_m)
        {
        }

        void visit(const AllDepAtom *)
        {
            value = Data_Wrap_Struct(c_all_dep_atom, 0, &Common<AllDepAtom::ConstPointer>::free,
                    new AllDepAtom::ConstPointer(mm));
        }

        void visit(const AnyDepAtom *)
        {
            value = Data_Wrap_Struct(c_any_dep_atom, 0, &Common<AnyDepAtom::ConstPointer>::free,
                    new AnyDepAtom::ConstPointer(mm));
        }

        void visit(const UseDepAtom *)
        {
            value = Data_Wrap_Struct(c_use_dep_atom, 0, &Common<UseDepAtom::ConstPointer>::free,
                    new UseDepAtom::ConstPointer(mm));
        }

        void visit(const PlainTextDepAtom *)
        {
            value = Data_Wrap_Struct(c_plain_text_dep_atom, 0, &Common<PlainTextDepAtom::ConstPointer>::free,
                    new PlainTextDepAtom::ConstPointer(mm));
        }

        void visit(const PackageDepAtom *)
        {
            value = Data_Wrap_Struct(c_package_dep_atom, 0, &Common<PackageDepAtom::ConstPointer>::free,
                    new PackageDepAtom::ConstPointer(mm));
        }

        void visit(const BlockDepAtom *)
        {
            value = Data_Wrap_Struct(c_block_dep_atom, 0, &Common<BlockDepAtom::ConstPointer>::free,
                    new BlockDepAtom::ConstPointer(mm));
        }
    };

    DepAtom::ConstPointer * m_ptr(0);
    try
    {
        V v(m);
        m->accept(&v);
        return v.value;
    }
    catch (const std::exception & e)
    {
        delete m_ptr;
        exception_to_ruby_exception(e);
    }
}

RegisterRubyClass::Register paludis_ruby_register_dep_atom PALUDIS_ATTRIBUTE((used))
    (&do_register_dep_atom);

