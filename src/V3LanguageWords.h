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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3LANGUAGEWORDS_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3LANGUAGEWORDS_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <map>

//============================================================================

class V3_Language_ਸ਼ਬਦ final {
    // List of common reserved keywords
ਨਿਜੀ:
    using KeywordMap = std::map<ਅਚਲ string, std::string>;
    ਬਣਤਰ Singleton final {
        KeywordMap s_kwdMap;  // List of keywords, and what language applies
        Singleton() { init(); }
        ਖਾਲੀ ਜੋੜੋ_Kwd(ਅਚਲ string& kwd, ਅਚਲ string& why) { s_kwdMap.emplace(kwd, why); }
        ਖਾਲੀ init();
    };

ਜਨਤਕ:
    using const_iterator = KeywordMap::const_iterator;
    // METHODS
    ਸਥਿਰ const_iterator begin() { ਵਾਪਸ s().s_kwdMap.begin(); }
    ਸਥਿਰ const_iterator ਸਿਰਾ() { ਵਾਪਸ s().s_kwdMap.ਸਿਰਾ(); }
    ਸਥਿਰ string isKeyword(ਅਚਲ string& kwd) {
        ਅਚਲ ਸਵੈਚਾਲਿਤ it = vlstd::as_const(s().s_kwdMap).ਲੱਭੋ(kwd);
        ਜੇ (it == s().s_kwdMap.ਸਿਰਾ()) ਵਾਪਸ "";
        ਵਾਪਸ it->ਦੂਜਾ;
    }

ਨਿਜੀ:
    ਸਥਿਰ Singleton& s() {
        ਸਥਿਰ Singleton s_s;  // LCOV_EXCL_BR_LINE
        ਵਾਪਸ s_s;
    }
};

ਸਤਰਅੰਦਰ ਖਾਲੀ V3_Language_ਸ਼ਬਦ::Singleton::init() {
    // C++ keywords
    // clang-format off
    ਜੋੜੋ_Kwd("abort",                 "C++ common word");
    ਜੋੜੋ_Kwd("alignas",               "C++11 keyword");
    ਜੋੜੋ_Kwd("alignof",               "C++11 keyword");
    ਜੋੜੋ_Kwd("and",                   "C++11 keyword");
    ਜੋੜੋ_Kwd("and_eq",                "C++11 keyword");
    ਜੋੜੋ_Kwd("asm",                   "C++ common word");
    ਜੋੜੋ_Kwd("atomic_cancel",         "C++ TM TS keyword");
    ਜੋੜੋ_Kwd("atomic_commit",         "C++ TM TS keyword");
    ਜੋੜੋ_Kwd("atomic_noexcept",       "C++ TM TS keyword");
    ਜੋੜੋ_Kwd("auto",                  "C++ keyword");
    ਜੋੜੋ_Kwd("bit_vector",            "C++ common word");
    ਜੋੜੋ_Kwd("bitand",                "C++ keyword");
    ਜੋੜੋ_Kwd("bitor",                 "C++ keyword");
    ਜੋੜੋ_Kwd("bool",                  "C++ keyword");
    ਜੋੜੋ_Kwd("break",                 "C++ keyword");
    ਜੋੜੋ_Kwd("case",                  "C++ keyword");
    ਜੋੜੋ_Kwd("catch",                 "C++ keyword");
    ਜੋੜੋ_Kwd("cdecl",                 "C++ common word");
    ਜੋੜੋ_Kwd("char",                  "C++ keyword");
    ਜੋੜੋ_Kwd("char16_t",              "C++11 keyword");
    ਜੋੜੋ_Kwd("char32_t",              "C++11 keyword");
    ਜੋੜੋ_Kwd("class",                 "C++11 keyword");
    ਜੋੜੋ_Kwd("compl",                 "C++11 keyword");
    ਜੋੜੋ_Kwd("complex",               "C++ common word");
    ਜੋੜੋ_Kwd("concept",               "C++20 keyword");
    ਜੋੜੋ_Kwd("const",                 "C++ keyword");
    ਜੋੜੋ_Kwd("const_cast",            "C++ common word");
    ਜੋੜੋ_Kwd("const_iterator",        "C++ common word");
    ਜੋੜੋ_Kwd("const_reference ",      "C++ common word");
    ਜੋੜੋ_Kwd("constexpr",             "C++11 keyword");
    ਜੋੜੋ_Kwd("continue",              "C++ keyword");
    ਜੋੜੋ_Kwd("decltype",              "C++11 keyword");
    ਜੋੜੋ_Kwd("default",               "C++ keyword");
    ਜੋੜੋ_Kwd("delete",                "C++ keyword");
    ਜੋੜੋ_Kwd("do",                    "C++ keyword");
    ਜੋੜੋ_Kwd("double",                "C++ keyword");
    ਜੋੜੋ_Kwd("dynamic_cast",          "C++ keyword");
    ਜੋੜੋ_Kwd("else",                  "C++ keyword");
    ਜੋੜੋ_Kwd("enum",                  "C++ keyword");
    ਜੋੜੋ_Kwd("explicit",              "C++ keyword");
    ਜੋੜੋ_Kwd("export",                "C++ keyword");
    ਜੋੜੋ_Kwd("extern",                "C++ keyword");
    ਜੋੜੋ_Kwd("false",                 "C++ keyword");
    ਜੋੜੋ_Kwd("far",                   "C++ common word");
    ਜੋੜੋ_Kwd("float",                 "C++ keyword");
    ਜੋੜੋ_Kwd("for",                   "C++ keyword");
    ਜੋੜੋ_Kwd("friend",                "C++ keyword");
    ਜੋੜੋ_Kwd("goto",                  "C++ keyword");
    ਜੋੜੋ_Kwd("huge",                  "C++ keyword");
    ਜੋੜੋ_Kwd("if",                    "C++ keyword");
    ਜੋੜੋ_Kwd("import",                "C++ modules TS keyword");
    ਜੋੜੋ_Kwd("inline",                "C++ keyword");
    ਜੋੜੋ_Kwd("int",                   "C++ keyword");
    ਜੋੜੋ_Kwd("interrupt",             "C++ common word");
    ਜੋੜੋ_Kwd("iterator",              "C++ common word");
    ਜੋੜੋ_Kwd("long",                  "C++ keyword");
    ਜੋੜੋ_Kwd("module",                "C++ modules TS keyword");
    ਜੋੜੋ_Kwd("mutable",               "C++ keyword");
    ਜੋੜੋ_Kwd("namespace",             "C++ keyword");
    ਜੋੜੋ_Kwd("near",                  "C++ common word");
    ਜੋੜੋ_Kwd("new",                   "C++ keyword");
    ਜੋੜੋ_Kwd("noexcept",              "C++11 keyword");
    ਜੋੜੋ_Kwd("not",                   "C++ keyword");
    ਜੋੜੋ_Kwd("not_eq",                "C++ keyword");
    ਜੋੜੋ_Kwd("nullptr",               "C++ common word");
    ਜੋੜੋ_Kwd("nullptr",               "C++11 keyword");
    ਜੋੜੋ_Kwd("operator",              "C++ keyword");
    ਜੋੜੋ_Kwd("or",                    "C++ keyword");
    ਜੋੜੋ_Kwd("or_eq",                 "C++ keyword");
    ਜੋੜੋ_Kwd("override",              "C++ common word");
    ਜੋੜੋ_Kwd("pascal",                "C++ keyword");
    ਜੋੜੋ_Kwd("private",               "C++ keyword");
    ਜੋੜੋ_Kwd("protected",             "C++ keyword");
    ਜੋੜੋ_Kwd("public",                "C++ keyword");
    ਜੋੜੋ_Kwd("queue",                 "C++ common word");
    ਜੋੜੋ_Kwd("reference",             "C++ common word");
    ਜੋੜੋ_Kwd("register",              "C++ keyword");
    ਜੋੜੋ_Kwd("reinterpret_cast ",     "C++ keyword");
    ਜੋੜੋ_Kwd("requires",              "C++20 keyword");
    ਜੋੜੋ_Kwd("restrict",              "C++ keyword");
    ਜੋੜੋ_Kwd("return",                "C++ keyword");
    ਜੋੜੋ_Kwd("short",                 "C++ keyword");
    ਜੋੜੋ_Kwd("signed",                "C++ keyword");
    ਜੋੜੋ_Kwd("sizeof",                "C++ keyword");
    ਜੋੜੋ_Kwd("stack",                 "C++ common word");
    ਜੋੜੋ_Kwd("static",                "C++ keyword");
    ਜੋੜੋ_Kwd("static_assert",         "C++11 keyword");
    ਜੋੜੋ_Kwd("static_cast",           "C++ keyword");
    ਜੋੜੋ_Kwd("std::deque",            "C++ common word");
    ਜੋੜੋ_Kwd("std::list",             "C++ common word");
    ਜੋੜੋ_Kwd("std::map",              "C++ common word");
    ਜੋੜੋ_Kwd("std::multimap",         "C++ common word");
    ਜੋੜੋ_Kwd("std::multiset",         "C++ common word");
    ਜੋੜੋ_Kwd("std::set",              "C++ common word");
    ਜੋੜੋ_Kwd("std::vector",           "C++ common word");
    ਜੋੜੋ_Kwd("struct",                "C++ keyword");
    ਜੋੜੋ_Kwd("switch",                "C++ keyword");
    ਜੋੜੋ_Kwd("synchronized",          "C++ TM TS keyword");
    ਜੋੜੋ_Kwd("template",              "C++ keyword");
    ਜੋੜੋ_Kwd("this",                  "C++ keyword");
    ਜੋੜੋ_Kwd("thread_local",          "C++11 keyword");
    ਜੋੜੋ_Kwd("throw",                 "C++ keyword");
    ਜੋੜੋ_Kwd("transaction_safe",      "C++ common word");
    ਜੋੜੋ_Kwd("transaction_safe_dynamic",      "C++ common word");
    ਜੋੜੋ_Kwd("true",                  "C++ keyword");
    ਜੋੜੋ_Kwd("try",                   "C++ keyword");
    ਜੋੜੋ_Kwd("type_info",             "C++ common word");
    ਜੋੜੋ_Kwd("typedef",               "C++ keyword");
    ਜੋੜੋ_Kwd("typeid",                "C++ keyword");
    ਜੋੜੋ_Kwd("typename",              "C++ keyword");
    ਜੋੜੋ_Kwd("uint16_t",              "C++ common word");
    ਜੋੜੋ_Kwd("uint32_t",              "C++ common word");
    ਜੋੜੋ_Kwd("uint8_t",               "C++ common word");
    ਜੋੜੋ_Kwd("union",                 "C++ keyword");
    ਜੋੜੋ_Kwd("unsigned",              "C++ keyword");
    ਜੋੜੋ_Kwd("using",                 "C++ keyword");
    ਜੋੜੋ_Kwd("virtual",               "C++ keyword");
    ਜੋੜੋ_Kwd("void",                  "C++ keyword");
    ਜੋੜੋ_Kwd("volatile",              "C++ keyword");
    ਜੋੜੋ_Kwd("wchar_t",               "C++ keyword");
    ਜੋੜੋ_Kwd("while",                 "C++ keyword");
    ਜੋੜੋ_Kwd("xor",                   "C++ keyword");
    ਜੋੜੋ_Kwd("xor_eq",                "C++ keyword");
    // This conflicts with header functions, so is ignored
    //dKwd("final",                 "C++11 keyword");
    // SystemC
    ਜੋੜੋ_Kwd("sc_clock",              "SystemC common word");
    ਜੋੜੋ_Kwd("sc_in",                 "SystemC common word");
    ਜੋੜੋ_Kwd("sc_inout",              "SystemC common word");
    ਜੋੜੋ_Kwd("sc_out",                "SystemC common word");
    ਜੋੜੋ_Kwd("sc_signal",             "SystemC common word");
    ਜੋੜੋ_Kwd("sensitive",             "SystemC common word");
    ਜੋੜੋ_Kwd("sensitive_neg",         "SystemC common word");
    ਜੋੜੋ_Kwd("sensitive_pos",         "SystemC common word");
    // Preprocessor defined words
    ਜੋੜੋ_Kwd("`__FILE__",                 "Verilog preprocessor directive");
    ਜੋੜੋ_Kwd("`__LINE__",                 "Verilog preprocessor directive");
    ਜੋੜੋ_Kwd("`accelerate",               "Verilog-XL preprocessor directive");
    ਜੋੜੋ_Kwd("`autoexpand_vectornets",    "Verilog-XL preprocessor directive");
    ਜੋੜੋ_Kwd("`begin_keywords",           "Verilog preprocessor directive");
    ਜੋੜੋ_Kwd("`celldefine",               "Verilog preprocessor directive");
    ਜੋੜੋ_Kwd("`default_decay_time",       "Verilog preprocessor directive");
    ਜੋੜੋ_Kwd("`default_nettype",          "Verilog preprocessor directive");
    ਜੋੜੋ_Kwd("`default_trireg_strength",  "Verilog preprocessor directive");
    ਜੋੜੋ_Kwd("`define",                   "Verilog preprocessor directive");
    ਜੋੜੋ_Kwd("`delay_mode_distributed",   "Verilog preprocessor directive");
    ਜੋੜੋ_Kwd("`delay_mode_path",          "Verilog preprocessor directive");
    ਜੋੜੋ_Kwd("`delay_mode_unit",          "Verilog preprocessor directive");
    ਜੋੜੋ_Kwd("`delay_mode_zero",          "Verilog preprocessor directive");
    ਜੋੜੋ_Kwd("`disable_portfaults",       "Verilog-XL preprocessor directive");
    ਜੋੜੋ_Kwd("`else",                     "Verilog preprocessor directive");
    ਜੋੜੋ_Kwd("`elsif",                    "Verilog preprocessor directive");
    ਜੋੜੋ_Kwd("`enable_portfaults",        "Verilog-XL preprocessor directive");
    ਜੋੜੋ_Kwd("`end_keywords",             "Verilog preprocessor directive");
    ਜੋੜੋ_Kwd("`endcelldefine",            "Verilog preprocessor directive");
    ਜੋੜੋ_Kwd("`endif",                    "Verilog preprocessor directive");
    ਜੋੜੋ_Kwd("`endprotect",               "Verilog preprocessor directive");
    ਜੋੜੋ_Kwd("`endprotected",             "Verilog preprocessor directive");
    ਜੋੜੋ_Kwd("`error",                    "Verilog preprocessor directive");
    ਜੋੜੋ_Kwd("`expand_vectornets",        "Verilog-XL preprocessor directive");
    ਜੋੜੋ_Kwd("`ifdef",                    "Verilog preprocessor directive");
    ਜੋੜੋ_Kwd("`ifndef",                   "Verilog preprocessor directive");
    ਜੋੜੋ_Kwd("`include",                  "Verilog preprocessor directive");
    ਜੋੜੋ_Kwd("`inline",                   "Verilog preprocessor directive");
    ਜੋੜੋ_Kwd("`line",                     "Verilog preprocessor directive");
    ਜੋੜੋ_Kwd("`noaccelerate",             "Verilog-XL preprocessor directive");
    ਜੋੜੋ_Kwd("`noexpand_vectornets",      "Verilog-XL preprocessor directive");
    ਜੋੜੋ_Kwd("`noremove_gatenames",       "Verilog-XL preprocessor directive");
    ਜੋੜੋ_Kwd("`noremove_netnames",        "Verilog-XL preprocessor directive");
    ਜੋੜੋ_Kwd("`nosuppress_faults",        "Verilog-XL preprocessor directive");
    ਜੋੜੋ_Kwd("`nounconnected_drive",      "Verilog-XL preprocessor directive");
    ਜੋੜੋ_Kwd("`portcoerce",               "Verilog preprocessor directive");
    ਜੋੜੋ_Kwd("`pragma",                   "Verilog preprocessor directive");
    ਜੋੜੋ_Kwd("`protect",                  "Verilog preprocessor directive");
    ਜੋੜੋ_Kwd("`protected",                "Verilog preprocessor directive");
    ਜੋੜੋ_Kwd("`remove_gatenames",         "Verilog-XL preprocessor directive");
    ਜੋੜੋ_Kwd("`remove_netnames",          "Verilog-XL preprocessor directive");
    ਜੋੜੋ_Kwd("`resetall",                 "Verilog preprocessor directive");
    ਜੋੜੋ_Kwd("`suppress_faults",          "Verilog-XL preprocessor directive");
    ਜੋੜੋ_Kwd("`systemc_ctor",             "Verilator preprocessor directive");
    ਜੋੜੋ_Kwd("`systemc_dtor",             "Verilator preprocessor directive");
    ਜੋੜੋ_Kwd("`systemc_header",           "Verilator preprocessor directive");
    ਜੋੜੋ_Kwd("`systemc_header_post",      "Verilator preprocessor directive");
    ਜੋੜੋ_Kwd("`systemc_imp_header",       "Verilator preprocessor directive");
    ਜੋੜੋ_Kwd("`systemc_implementation",   "Verilator preprocessor directive");
    ਜੋੜੋ_Kwd("`systemc_interface",        "Verilator preprocessor directive");
    ਜੋੜੋ_Kwd("`timescale",                "Verilog preprocessor directive");
    ਜੋੜੋ_Kwd("`undef",                    "Verilog preprocessor directive");
    ਜੋੜੋ_Kwd("`undefineall",              "Verilog preprocessor directive");
    ਜੋੜੋ_Kwd("`verilator_config",         "Verilator preprocessor directive");
    ਜੋੜੋ_Kwd("`verilog",                  "Verilator preprocessor directive");
    // clang-format on
}

#ਜੇ_ਅੰਤ  // Guard
