// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Language rules
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2005-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************

#که_نه_تعریف_شوی VERILATOR_V3LANGUAGEWORDS_H_
#تعریف_کړه VERILATOR_V3LANGUAGEWORDS_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن <map>

//============================================================================

class V3_Language_ټکي final {
    // List of common reserved keywords
خاص:
    using KeywordMap = std::map<ثابت string, std::string>;
    بنية Singleton final {
        KeywordMap s_kwdMap;  // List of keywords, and what language applies
        Singleton() { init(); }
        فراغ جمع_کړه_Kwd(ثابت string& kwd, ثابت string& why) { s_kwdMap.emplace(kwd, why); }
        فراغ init();
    };

عام:
    using const_iterator = KeywordMap::const_iterator;
    // METHODS
    ساكن const_iterator begin() { أرجع s().s_kwdMap.begin(); }
    ساكن const_iterator پای_ټکی() { أرجع s().s_kwdMap.پای_ټکی(); }
    ساكن string isKeyword(ثابت string& kwd) {
        ثابت خپلکاری it = vlstd::as_const(s().s_kwdMap).پیدا_کړه(kwd);
        إذا (it == s().s_kwdMap.پای_ټکی()) أرجع "";
        أرجع it->دوهم;
    }

خاص:
    ساكن Singleton& s() {
        ساكن Singleton s_s;  // LCOV_EXCL_BR_LINE
        أرجع s_s;
    }
};

مضمن فراغ V3_Language_ټکي::Singleton::init() {
    // C++ keywords
    // clang-format off
    جمع_کړه_Kwd("abort",                 "C++ common word");
    جمع_کړه_Kwd("alignas",               "C++11 keyword");
    جمع_کړه_Kwd("alignof",               "C++11 keyword");
    جمع_کړه_Kwd("and",                   "C++11 keyword");
    جمع_کړه_Kwd("and_eq",                "C++11 keyword");
    جمع_کړه_Kwd("asm",                   "C++ common word");
    جمع_کړه_Kwd("atomic_cancel",         "C++ TM TS keyword");
    جمع_کړه_Kwd("atomic_commit",         "C++ TM TS keyword");
    جمع_کړه_Kwd("atomic_noexcept",       "C++ TM TS keyword");
    جمع_کړه_Kwd("auto",                  "C++ keyword");
    جمع_کړه_Kwd("bit_vector",            "C++ common word");
    جمع_کړه_Kwd("bitand",                "C++ keyword");
    جمع_کړه_Kwd("bitor",                 "C++ keyword");
    جمع_کړه_Kwd("bool",                  "C++ keyword");
    جمع_کړه_Kwd("break",                 "C++ keyword");
    جمع_کړه_Kwd("case",                  "C++ keyword");
    جمع_کړه_Kwd("catch",                 "C++ keyword");
    جمع_کړه_Kwd("cdecl",                 "C++ common word");
    جمع_کړه_Kwd("char",                  "C++ keyword");
    جمع_کړه_Kwd("char16_t",              "C++11 keyword");
    جمع_کړه_Kwd("char32_t",              "C++11 keyword");
    جمع_کړه_Kwd("class",                 "C++11 keyword");
    جمع_کړه_Kwd("compl",                 "C++11 keyword");
    جمع_کړه_Kwd("complex",               "C++ common word");
    جمع_کړه_Kwd("concept",               "C++20 keyword");
    جمع_کړه_Kwd("const",                 "C++ keyword");
    جمع_کړه_Kwd("const_cast",            "C++ common word");
    جمع_کړه_Kwd("const_iterator",        "C++ common word");
    جمع_کړه_Kwd("const_reference ",      "C++ common word");
    جمع_کړه_Kwd("constexpr",             "C++11 keyword");
    جمع_کړه_Kwd("continue",              "C++ keyword");
    جمع_کړه_Kwd("decltype",              "C++11 keyword");
    جمع_کړه_Kwd("default",               "C++ keyword");
    جمع_کړه_Kwd("delete",                "C++ keyword");
    جمع_کړه_Kwd("do",                    "C++ keyword");
    جمع_کړه_Kwd("double",                "C++ keyword");
    جمع_کړه_Kwd("dynamic_cast",          "C++ keyword");
    جمع_کړه_Kwd("else",                  "C++ keyword");
    جمع_کړه_Kwd("enum",                  "C++ keyword");
    جمع_کړه_Kwd("explicit",              "C++ keyword");
    جمع_کړه_Kwd("export",                "C++ keyword");
    جمع_کړه_Kwd("extern",                "C++ keyword");
    جمع_کړه_Kwd("false",                 "C++ keyword");
    جمع_کړه_Kwd("far",                   "C++ common word");
    جمع_کړه_Kwd("float",                 "C++ keyword");
    جمع_کړه_Kwd("for",                   "C++ keyword");
    جمع_کړه_Kwd("friend",                "C++ keyword");
    جمع_کړه_Kwd("goto",                  "C++ keyword");
    جمع_کړه_Kwd("huge",                  "C++ keyword");
    جمع_کړه_Kwd("if",                    "C++ keyword");
    جمع_کړه_Kwd("import",                "C++ modules TS keyword");
    جمع_کړه_Kwd("inline",                "C++ keyword");
    جمع_کړه_Kwd("int",                   "C++ keyword");
    جمع_کړه_Kwd("interrupt",             "C++ common word");
    جمع_کړه_Kwd("iterator",              "C++ common word");
    جمع_کړه_Kwd("long",                  "C++ keyword");
    جمع_کړه_Kwd("module",                "C++ modules TS keyword");
    جمع_کړه_Kwd("mutable",               "C++ keyword");
    جمع_کړه_Kwd("namespace",             "C++ keyword");
    جمع_کړه_Kwd("near",                  "C++ common word");
    جمع_کړه_Kwd("new",                   "C++ keyword");
    جمع_کړه_Kwd("noexcept",              "C++11 keyword");
    جمع_کړه_Kwd("not",                   "C++ keyword");
    جمع_کړه_Kwd("not_eq",                "C++ keyword");
    جمع_کړه_Kwd("nullptr",               "C++ common word");
    جمع_کړه_Kwd("nullptr",               "C++11 keyword");
    جمع_کړه_Kwd("operator",              "C++ keyword");
    جمع_کړه_Kwd("or",                    "C++ keyword");
    جمع_کړه_Kwd("or_eq",                 "C++ keyword");
    جمع_کړه_Kwd("override",              "C++ common word");
    جمع_کړه_Kwd("pascal",                "C++ keyword");
    جمع_کړه_Kwd("private",               "C++ keyword");
    جمع_کړه_Kwd("protected",             "C++ keyword");
    جمع_کړه_Kwd("public",                "C++ keyword");
    جمع_کړه_Kwd("queue",                 "C++ common word");
    جمع_کړه_Kwd("reference",             "C++ common word");
    جمع_کړه_Kwd("register",              "C++ keyword");
    جمع_کړه_Kwd("reinterpret_cast ",     "C++ keyword");
    جمع_کړه_Kwd("requires",              "C++20 keyword");
    جمع_کړه_Kwd("restrict",              "C++ keyword");
    جمع_کړه_Kwd("return",                "C++ keyword");
    جمع_کړه_Kwd("short",                 "C++ keyword");
    جمع_کړه_Kwd("signed",                "C++ keyword");
    جمع_کړه_Kwd("sizeof",                "C++ keyword");
    جمع_کړه_Kwd("stack",                 "C++ common word");
    جمع_کړه_Kwd("static",                "C++ keyword");
    جمع_کړه_Kwd("static_assert",         "C++11 keyword");
    جمع_کړه_Kwd("static_cast",           "C++ keyword");
    جمع_کړه_Kwd("std::deque",            "C++ common word");
    جمع_کړه_Kwd("std::list",             "C++ common word");
    جمع_کړه_Kwd("std::map",              "C++ common word");
    جمع_کړه_Kwd("std::multimap",         "C++ common word");
    جمع_کړه_Kwd("std::multiset",         "C++ common word");
    جمع_کړه_Kwd("std::set",              "C++ common word");
    جمع_کړه_Kwd("std::vector",           "C++ common word");
    جمع_کړه_Kwd("struct",                "C++ keyword");
    جمع_کړه_Kwd("switch",                "C++ keyword");
    جمع_کړه_Kwd("synchronized",          "C++ TM TS keyword");
    جمع_کړه_Kwd("template",              "C++ keyword");
    جمع_کړه_Kwd("this",                  "C++ keyword");
    جمع_کړه_Kwd("thread_local",          "C++11 keyword");
    جمع_کړه_Kwd("throw",                 "C++ keyword");
    جمع_کړه_Kwd("transaction_safe",      "C++ common word");
    جمع_کړه_Kwd("transaction_safe_dynamic",      "C++ common word");
    جمع_کړه_Kwd("true",                  "C++ keyword");
    جمع_کړه_Kwd("try",                   "C++ keyword");
    جمع_کړه_Kwd("type_info",             "C++ common word");
    جمع_کړه_Kwd("typedef",               "C++ keyword");
    جمع_کړه_Kwd("typeid",                "C++ keyword");
    جمع_کړه_Kwd("typename",              "C++ keyword");
    جمع_کړه_Kwd("uint16_t",              "C++ common word");
    جمع_کړه_Kwd("uint32_t",              "C++ common word");
    جمع_کړه_Kwd("uint8_t",               "C++ common word");
    جمع_کړه_Kwd("union",                 "C++ keyword");
    جمع_کړه_Kwd("unsigned",              "C++ keyword");
    جمع_کړه_Kwd("using",                 "C++ keyword");
    جمع_کړه_Kwd("virtual",               "C++ keyword");
    جمع_کړه_Kwd("void",                  "C++ keyword");
    جمع_کړه_Kwd("volatile",              "C++ keyword");
    جمع_کړه_Kwd("wchar_t",               "C++ keyword");
    جمع_کړه_Kwd("while",                 "C++ keyword");
    جمع_کړه_Kwd("xor",                   "C++ keyword");
    جمع_کړه_Kwd("xor_eq",                "C++ keyword");
    // This conflicts with header functions, so is ignored
    //dKwd("final",                 "C++11 keyword");
    // SystemC
    جمع_کړه_Kwd("sc_clock",              "SystemC common word");
    جمع_کړه_Kwd("sc_in",                 "SystemC common word");
    جمع_کړه_Kwd("sc_inout",              "SystemC common word");
    جمع_کړه_Kwd("sc_out",                "SystemC common word");
    جمع_کړه_Kwd("sc_signal",             "SystemC common word");
    جمع_کړه_Kwd("sensitive",             "SystemC common word");
    جمع_کړه_Kwd("sensitive_neg",         "SystemC common word");
    جمع_کړه_Kwd("sensitive_pos",         "SystemC common word");
    // Preprocessor defined words
    جمع_کړه_Kwd("`__FILE__",                 "Verilog preprocessor directive");
    جمع_کړه_Kwd("`__LINE__",                 "Verilog preprocessor directive");
    جمع_کړه_Kwd("`accelerate",               "Verilog-XL preprocessor directive");
    جمع_کړه_Kwd("`autoexpand_vectornets",    "Verilog-XL preprocessor directive");
    جمع_کړه_Kwd("`begin_keywords",           "Verilog preprocessor directive");
    جمع_کړه_Kwd("`celldefine",               "Verilog preprocessor directive");
    جمع_کړه_Kwd("`default_decay_time",       "Verilog preprocessor directive");
    جمع_کړه_Kwd("`default_nettype",          "Verilog preprocessor directive");
    جمع_کړه_Kwd("`default_trireg_strength",  "Verilog preprocessor directive");
    جمع_کړه_Kwd("`define",                   "Verilog preprocessor directive");
    جمع_کړه_Kwd("`delay_mode_distributed",   "Verilog preprocessor directive");
    جمع_کړه_Kwd("`delay_mode_path",          "Verilog preprocessor directive");
    جمع_کړه_Kwd("`delay_mode_unit",          "Verilog preprocessor directive");
    جمع_کړه_Kwd("`delay_mode_zero",          "Verilog preprocessor directive");
    جمع_کړه_Kwd("`disable_portfaults",       "Verilog-XL preprocessor directive");
    جمع_کړه_Kwd("`else",                     "Verilog preprocessor directive");
    جمع_کړه_Kwd("`elsif",                    "Verilog preprocessor directive");
    جمع_کړه_Kwd("`enable_portfaults",        "Verilog-XL preprocessor directive");
    جمع_کړه_Kwd("`end_keywords",             "Verilog preprocessor directive");
    جمع_کړه_Kwd("`endcelldefine",            "Verilog preprocessor directive");
    جمع_کړه_Kwd("`endif",                    "Verilog preprocessor directive");
    جمع_کړه_Kwd("`endprotect",               "Verilog preprocessor directive");
    جمع_کړه_Kwd("`endprotected",             "Verilog preprocessor directive");
    جمع_کړه_Kwd("`error",                    "Verilog preprocessor directive");
    جمع_کړه_Kwd("`expand_vectornets",        "Verilog-XL preprocessor directive");
    جمع_کړه_Kwd("`ifdef",                    "Verilog preprocessor directive");
    جمع_کړه_Kwd("`ifndef",                   "Verilog preprocessor directive");
    جمع_کړه_Kwd("`include",                  "Verilog preprocessor directive");
    جمع_کړه_Kwd("`inline",                   "Verilog preprocessor directive");
    جمع_کړه_Kwd("`line",                     "Verilog preprocessor directive");
    جمع_کړه_Kwd("`noaccelerate",             "Verilog-XL preprocessor directive");
    جمع_کړه_Kwd("`noexpand_vectornets",      "Verilog-XL preprocessor directive");
    جمع_کړه_Kwd("`noremove_gatenames",       "Verilog-XL preprocessor directive");
    جمع_کړه_Kwd("`noremove_netnames",        "Verilog-XL preprocessor directive");
    جمع_کړه_Kwd("`nosuppress_faults",        "Verilog-XL preprocessor directive");
    جمع_کړه_Kwd("`nounconnected_drive",      "Verilog-XL preprocessor directive");
    جمع_کړه_Kwd("`portcoerce",               "Verilog preprocessor directive");
    جمع_کړه_Kwd("`pragma",                   "Verilog preprocessor directive");
    جمع_کړه_Kwd("`protect",                  "Verilog preprocessor directive");
    جمع_کړه_Kwd("`protected",                "Verilog preprocessor directive");
    جمع_کړه_Kwd("`remove_gatenames",         "Verilog-XL preprocessor directive");
    جمع_کړه_Kwd("`remove_netnames",          "Verilog-XL preprocessor directive");
    جمع_کړه_Kwd("`resetall",                 "Verilog preprocessor directive");
    جمع_کړه_Kwd("`suppress_faults",          "Verilog-XL preprocessor directive");
    جمع_کړه_Kwd("`systemc_ctor",             "Verilator preprocessor directive");
    جمع_کړه_Kwd("`systemc_dtor",             "Verilator preprocessor directive");
    جمع_کړه_Kwd("`systemc_header",           "Verilator preprocessor directive");
    جمع_کړه_Kwd("`systemc_header_post",      "Verilator preprocessor directive");
    جمع_کړه_Kwd("`systemc_imp_header",       "Verilator preprocessor directive");
    جمع_کړه_Kwd("`systemc_implementation",   "Verilator preprocessor directive");
    جمع_کړه_Kwd("`systemc_interface",        "Verilator preprocessor directive");
    جمع_کړه_Kwd("`timescale",                "Verilog preprocessor directive");
    جمع_کړه_Kwd("`undef",                    "Verilog preprocessor directive");
    جمع_کړه_Kwd("`undefineall",              "Verilog preprocessor directive");
    جمع_کړه_Kwd("`verilator_config",         "Verilator preprocessor directive");
    جمع_کړه_Kwd("`verilog",                  "Verilator preprocessor directive");
    // clang-format on
}

#د_که_پای  // Guard
