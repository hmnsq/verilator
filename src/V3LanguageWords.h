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

#यदि_अपरिभाषितम् VERILATOR_V3LANGUAGEWORDS_H_
#परिभाषय VERILATOR_V3LANGUAGEWORDS_H_

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव <map>

//============================================================================

class V3_Language_शब्दाः final {
    // List of common reserved keywords
गुप्त:
    using KeywordMap = std::map<ध्रुव string, std::string>;
    संरचना Singleton final {
        KeywordMap s_kwdMap;  // List of keywords, and what language applies
        Singleton() { init(); }
        शून्य सङ्कलय_Kwd(ध्रुव string& kwd, ध्रुव string& why) { s_kwdMap.emplace(kwd, why); }
        शून्य init();
    };

सार्वजनिक:
    using const_iterator = KeywordMap::const_iterator;
    // METHODS
    स्थिर const_iterator begin() { प्रत्यागम s().s_kwdMap.begin(); }
    स्थिर const_iterator अन्तः_2() { प्रत्यागम s().s_kwdMap.अन्तः_2(); }
    स्थिर string isKeyword(ध्रुव string& kwd) {
        ध्रुव स्वचालित it = vlstd::as_const(s().s_kwdMap).find(kwd);
        यदि (it == s().s_kwdMap.अन्तः_2()) प्रत्यागम "";
        प्रत्यागम it->second;
    }

गुप्त:
    स्थिर Singleton& s() {
        स्थिर Singleton s_s;  // LCOV_EXCL_BR_LINE
        प्रत्यागम s_s;
    }
};

अन्तःस्थ शून्य V3_Language_शब्दाः::Singleton::init() {
    // C++ keywords
    // clang-format off
    सङ्कलय_Kwd("abort",                 "C++ common word");
    सङ्कलय_Kwd("alignas",               "C++11 keyword");
    सङ्कलय_Kwd("alignof",               "C++11 keyword");
    सङ्कलय_Kwd("and",                   "C++11 keyword");
    सङ्कलय_Kwd("and_eq",                "C++11 keyword");
    सङ्कलय_Kwd("asm",                   "C++ common word");
    सङ्कलय_Kwd("atomic_cancel",         "C++ TM TS keyword");
    सङ्कलय_Kwd("atomic_commit",         "C++ TM TS keyword");
    सङ्कलय_Kwd("atomic_noexcept",       "C++ TM TS keyword");
    सङ्कलय_Kwd("auto",                  "C++ keyword");
    सङ्कलय_Kwd("bit_vector",            "C++ common word");
    सङ्कलय_Kwd("bitand",                "C++ keyword");
    सङ्कलय_Kwd("bitor",                 "C++ keyword");
    सङ्कलय_Kwd("bool",                  "C++ keyword");
    सङ्कलय_Kwd("break",                 "C++ keyword");
    सङ्कलय_Kwd("case",                  "C++ keyword");
    सङ्कलय_Kwd("catch",                 "C++ keyword");
    सङ्कलय_Kwd("cdecl",                 "C++ common word");
    सङ्कलय_Kwd("char",                  "C++ keyword");
    सङ्कलय_Kwd("char16_t",              "C++11 keyword");
    सङ्कलय_Kwd("char32_t",              "C++11 keyword");
    सङ्कलय_Kwd("class",                 "C++11 keyword");
    सङ्कलय_Kwd("compl",                 "C++11 keyword");
    सङ्कलय_Kwd("complex",               "C++ common word");
    सङ्कलय_Kwd("concept",               "C++20 keyword");
    सङ्कलय_Kwd("const",                 "C++ keyword");
    सङ्कलय_Kwd("const_cast",            "C++ common word");
    सङ्कलय_Kwd("const_iterator",        "C++ common word");
    सङ्कलय_Kwd("const_reference ",      "C++ common word");
    सङ्कलय_Kwd("constexpr",             "C++11 keyword");
    सङ्कलय_Kwd("continue",              "C++ keyword");
    सङ्कलय_Kwd("decltype",              "C++11 keyword");
    सङ्कलय_Kwd("default",               "C++ keyword");
    सङ्कलय_Kwd("delete",                "C++ keyword");
    सङ्कलय_Kwd("do",                    "C++ keyword");
    सङ्कलय_Kwd("double",                "C++ keyword");
    सङ्कलय_Kwd("dynamic_cast",          "C++ keyword");
    सङ्कलय_Kwd("else",                  "C++ keyword");
    सङ्कलय_Kwd("enum",                  "C++ keyword");
    सङ्कलय_Kwd("explicit",              "C++ keyword");
    सङ्कलय_Kwd("export",                "C++ keyword");
    सङ्कलय_Kwd("extern",                "C++ keyword");
    सङ्कलय_Kwd("false",                 "C++ keyword");
    सङ्कलय_Kwd("far",                   "C++ common word");
    सङ्कलय_Kwd("float",                 "C++ keyword");
    सङ्कलय_Kwd("for",                   "C++ keyword");
    सङ्कलय_Kwd("friend",                "C++ keyword");
    सङ्कलय_Kwd("goto",                  "C++ keyword");
    सङ्कलय_Kwd("huge",                  "C++ keyword");
    सङ्कलय_Kwd("if",                    "C++ keyword");
    सङ्कलय_Kwd("import",                "C++ modules TS keyword");
    सङ्कलय_Kwd("inline",                "C++ keyword");
    सङ्कलय_Kwd("int",                   "C++ keyword");
    सङ्कलय_Kwd("interrupt",             "C++ common word");
    सङ्कलय_Kwd("iterator",              "C++ common word");
    सङ्कलय_Kwd("long",                  "C++ keyword");
    सङ्कलय_Kwd("module",                "C++ modules TS keyword");
    सङ्कलय_Kwd("mutable",               "C++ keyword");
    सङ्कलय_Kwd("namespace",             "C++ keyword");
    सङ्कलय_Kwd("near",                  "C++ common word");
    सङ्कलय_Kwd("new",                   "C++ keyword");
    सङ्कलय_Kwd("noexcept",              "C++11 keyword");
    सङ्कलय_Kwd("not",                   "C++ keyword");
    सङ्कलय_Kwd("not_eq",                "C++ keyword");
    सङ्कलय_Kwd("nullptr",               "C++ common word");
    सङ्कलय_Kwd("nullptr",               "C++11 keyword");
    सङ्कलय_Kwd("operator",              "C++ keyword");
    सङ्कलय_Kwd("or",                    "C++ keyword");
    सङ्कलय_Kwd("or_eq",                 "C++ keyword");
    सङ्कलय_Kwd("override",              "C++ common word");
    सङ्कलय_Kwd("pascal",                "C++ keyword");
    सङ्कलय_Kwd("private",               "C++ keyword");
    सङ्कलय_Kwd("protected",             "C++ keyword");
    सङ्कलय_Kwd("public",                "C++ keyword");
    सङ्कलय_Kwd("queue",                 "C++ common word");
    सङ्कलय_Kwd("reference",             "C++ common word");
    सङ्कलय_Kwd("register",              "C++ keyword");
    सङ्कलय_Kwd("reinterpret_cast ",     "C++ keyword");
    सङ्कलय_Kwd("requires",              "C++20 keyword");
    सङ्कलय_Kwd("restrict",              "C++ keyword");
    सङ्कलय_Kwd("return",                "C++ keyword");
    सङ्कलय_Kwd("short",                 "C++ keyword");
    सङ्कलय_Kwd("signed",                "C++ keyword");
    सङ्कलय_Kwd("sizeof",                "C++ keyword");
    सङ्कलय_Kwd("stack",                 "C++ common word");
    सङ्कलय_Kwd("static",                "C++ keyword");
    सङ्कलय_Kwd("static_assert",         "C++11 keyword");
    सङ्कलय_Kwd("static_cast",           "C++ keyword");
    सङ्कलय_Kwd("std::deque",            "C++ common word");
    सङ्कलय_Kwd("std::list",             "C++ common word");
    सङ्कलय_Kwd("std::map",              "C++ common word");
    सङ्कलय_Kwd("std::multimap",         "C++ common word");
    सङ्कलय_Kwd("std::multiset",         "C++ common word");
    सङ्कलय_Kwd("std::set",              "C++ common word");
    सङ्कलय_Kwd("std::vector",           "C++ common word");
    सङ्कलय_Kwd("struct",                "C++ keyword");
    सङ्कलय_Kwd("switch",                "C++ keyword");
    सङ्कलय_Kwd("synchronized",          "C++ TM TS keyword");
    सङ्कलय_Kwd("template",              "C++ keyword");
    सङ्कलय_Kwd("this",                  "C++ keyword");
    सङ्कलय_Kwd("thread_local",          "C++11 keyword");
    सङ्कलय_Kwd("throw",                 "C++ keyword");
    सङ्कलय_Kwd("transaction_safe",      "C++ common word");
    सङ्कलय_Kwd("transaction_safe_dynamic",      "C++ common word");
    सङ्कलय_Kwd("true",                  "C++ keyword");
    सङ्कलय_Kwd("try",                   "C++ keyword");
    सङ्कलय_Kwd("type_info",             "C++ common word");
    सङ्कलय_Kwd("typedef",               "C++ keyword");
    सङ्कलय_Kwd("typeid",                "C++ keyword");
    सङ्कलय_Kwd("typename",              "C++ keyword");
    सङ्कलय_Kwd("uint16_t",              "C++ common word");
    सङ्कलय_Kwd("uint32_t",              "C++ common word");
    सङ्कलय_Kwd("uint8_t",               "C++ common word");
    सङ्कलय_Kwd("union",                 "C++ keyword");
    सङ्कलय_Kwd("unsigned",              "C++ keyword");
    सङ्कलय_Kwd("using",                 "C++ keyword");
    सङ्कलय_Kwd("virtual",               "C++ keyword");
    सङ्कलय_Kwd("void",                  "C++ keyword");
    सङ्कलय_Kwd("volatile",              "C++ keyword");
    सङ्कलय_Kwd("wchar_t",               "C++ keyword");
    सङ्कलय_Kwd("while",                 "C++ keyword");
    सङ्कलय_Kwd("xor",                   "C++ keyword");
    सङ्कलय_Kwd("xor_eq",                "C++ keyword");
    // This conflicts with header functions, so is ignored
    //dKwd("final",                 "C++11 keyword");
    // SystemC
    सङ्कलय_Kwd("sc_clock",              "SystemC common word");
    सङ्कलय_Kwd("sc_in",                 "SystemC common word");
    सङ्कलय_Kwd("sc_inout",              "SystemC common word");
    सङ्कलय_Kwd("sc_out",                "SystemC common word");
    सङ्कलय_Kwd("sc_signal",             "SystemC common word");
    सङ्कलय_Kwd("sensitive",             "SystemC common word");
    सङ्कलय_Kwd("sensitive_neg",         "SystemC common word");
    सङ्कलय_Kwd("sensitive_pos",         "SystemC common word");
    // Preprocessor defined words
    सङ्कलय_Kwd("`__FILE__",                 "Verilog preprocessor directive");
    सङ्कलय_Kwd("`__LINE__",                 "Verilog preprocessor directive");
    सङ्कलय_Kwd("`accelerate",               "Verilog-XL preprocessor directive");
    सङ्कलय_Kwd("`autoexpand_vectornets",    "Verilog-XL preprocessor directive");
    सङ्कलय_Kwd("`begin_keywords",           "Verilog preprocessor directive");
    सङ्कलय_Kwd("`celldefine",               "Verilog preprocessor directive");
    सङ्कलय_Kwd("`default_decay_time",       "Verilog preprocessor directive");
    सङ्कलय_Kwd("`default_nettype",          "Verilog preprocessor directive");
    सङ्कलय_Kwd("`default_trireg_strength",  "Verilog preprocessor directive");
    सङ्कलय_Kwd("`define",                   "Verilog preprocessor directive");
    सङ्कलय_Kwd("`delay_mode_distributed",   "Verilog preprocessor directive");
    सङ्कलय_Kwd("`delay_mode_path",          "Verilog preprocessor directive");
    सङ्कलय_Kwd("`delay_mode_unit",          "Verilog preprocessor directive");
    सङ्कलय_Kwd("`delay_mode_zero",          "Verilog preprocessor directive");
    सङ्कलय_Kwd("`disable_portfaults",       "Verilog-XL preprocessor directive");
    सङ्कलय_Kwd("`else",                     "Verilog preprocessor directive");
    सङ्कलय_Kwd("`elsif",                    "Verilog preprocessor directive");
    सङ्कलय_Kwd("`enable_portfaults",        "Verilog-XL preprocessor directive");
    सङ्कलय_Kwd("`end_keywords",             "Verilog preprocessor directive");
    सङ्कलय_Kwd("`endcelldefine",            "Verilog preprocessor directive");
    सङ्कलय_Kwd("`endif",                    "Verilog preprocessor directive");
    सङ्कलय_Kwd("`endprotect",               "Verilog preprocessor directive");
    सङ्कलय_Kwd("`endprotected",             "Verilog preprocessor directive");
    सङ्कलय_Kwd("`error",                    "Verilog preprocessor directive");
    सङ्कलय_Kwd("`expand_vectornets",        "Verilog-XL preprocessor directive");
    सङ्कलय_Kwd("`ifdef",                    "Verilog preprocessor directive");
    सङ्कलय_Kwd("`ifndef",                   "Verilog preprocessor directive");
    सङ्कलय_Kwd("`include",                  "Verilog preprocessor directive");
    सङ्कलय_Kwd("`inline",                   "Verilog preprocessor directive");
    सङ्कलय_Kwd("`line",                     "Verilog preprocessor directive");
    सङ्कलय_Kwd("`noaccelerate",             "Verilog-XL preprocessor directive");
    सङ्कलय_Kwd("`noexpand_vectornets",      "Verilog-XL preprocessor directive");
    सङ्कलय_Kwd("`noremove_gatenames",       "Verilog-XL preprocessor directive");
    सङ्कलय_Kwd("`noremove_netnames",        "Verilog-XL preprocessor directive");
    सङ्कलय_Kwd("`nosuppress_faults",        "Verilog-XL preprocessor directive");
    सङ्कलय_Kwd("`nounconnected_drive",      "Verilog-XL preprocessor directive");
    सङ्कलय_Kwd("`portcoerce",               "Verilog preprocessor directive");
    सङ्कलय_Kwd("`pragma",                   "Verilog preprocessor directive");
    सङ्कलय_Kwd("`protect",                  "Verilog preprocessor directive");
    सङ्कलय_Kwd("`protected",                "Verilog preprocessor directive");
    सङ्कलय_Kwd("`remove_gatenames",         "Verilog-XL preprocessor directive");
    सङ्कलय_Kwd("`remove_netnames",          "Verilog-XL preprocessor directive");
    सङ्कलय_Kwd("`resetall",                 "Verilog preprocessor directive");
    सङ्कलय_Kwd("`suppress_faults",          "Verilog-XL preprocessor directive");
    सङ्कलय_Kwd("`systemc_ctor",             "Verilator preprocessor directive");
    सङ्कलय_Kwd("`systemc_dtor",             "Verilator preprocessor directive");
    सङ्कलय_Kwd("`systemc_header",           "Verilator preprocessor directive");
    सङ्कलय_Kwd("`systemc_header_post",      "Verilator preprocessor directive");
    सङ्कलय_Kwd("`systemc_imp_header",       "Verilator preprocessor directive");
    सङ्कलय_Kwd("`systemc_implementation",   "Verilator preprocessor directive");
    सङ्कलय_Kwd("`systemc_interface",        "Verilator preprocessor directive");
    सङ्कलय_Kwd("`timescale",                "Verilog preprocessor directive");
    सङ्कलय_Kwd("`undef",                    "Verilog preprocessor directive");
    सङ्कलय_Kwd("`undefineall",              "Verilog preprocessor directive");
    सङ्कलय_Kwd("`verilator_config",         "Verilator preprocessor directive");
    सङ्कलय_Kwd("`verilog",                  "Verilator preprocessor directive");
    // clang-format on
}

#यदि_अन्तः  // Guard
