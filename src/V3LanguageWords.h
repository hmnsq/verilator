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

#నిర్వచితం_కాకపోతే VERILATOR_V3LANGUAGEWORDS_H_
#నిర్వచించు VERILATOR_V3LANGUAGEWORDS_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ <map>

//============================================================================

class V3_Language_పదాలు final {
    // List of common reserved keywords
అంతరంగ:
    using KeywordMap = std::map<స్థిరరాశి string, std::string>;
    నిర్మాణం Singleton final {
        KeywordMap s_kwdMap;  // List of keywords, and what language applies
        Singleton() { init(); }
        శూన్య కూడు_Kwd(స్థిరరాశి string& kwd, స్థిరరాశి string& why) { s_kwdMap.emplace(kwd, why); }
        శూన్య init();
    };

బహిరంగ:
    using const_iterator = KeywordMap::const_iterator;
    // METHODS
    స్థిర const_iterator begin() { తిరిగి s().s_kwdMap.begin(); }
    స్థిర const_iterator చివర() { తిరిగి s().s_kwdMap.చివర(); }
    స్థిర string isKeyword(స్థిరరాశి string& kwd) {
        స్థిరరాశి స్వయంచాలక it = vlstd::as_const(s().s_kwdMap).కనుగొను(kwd);
        ఒకవేళ (it == s().s_kwdMap.చివర()) తిరిగి "";
        తిరిగి it->రెండవ;
    }

అంతరంగ:
    స్థిర Singleton& s() {
        స్థిర Singleton s_s;  // LCOV_EXCL_BR_LINE
        తిరిగి s_s;
    }
};

అంతఃవరుస శూన్య V3_Language_పదాలు::Singleton::init() {
    // C++ keywords
    // clang-format off
    కూడు_Kwd("abort",                 "C++ common word");
    కూడు_Kwd("alignas",               "C++11 keyword");
    కూడు_Kwd("alignof",               "C++11 keyword");
    కూడు_Kwd("and",                   "C++11 keyword");
    కూడు_Kwd("and_eq",                "C++11 keyword");
    కూడు_Kwd("asm",                   "C++ common word");
    కూడు_Kwd("atomic_cancel",         "C++ TM TS keyword");
    కూడు_Kwd("atomic_commit",         "C++ TM TS keyword");
    కూడు_Kwd("atomic_noexcept",       "C++ TM TS keyword");
    కూడు_Kwd("auto",                  "C++ keyword");
    కూడు_Kwd("bit_vector",            "C++ common word");
    కూడు_Kwd("bitand",                "C++ keyword");
    కూడు_Kwd("bitor",                 "C++ keyword");
    కూడు_Kwd("bool",                  "C++ keyword");
    కూడు_Kwd("break",                 "C++ keyword");
    కూడు_Kwd("case",                  "C++ keyword");
    కూడు_Kwd("catch",                 "C++ keyword");
    కూడు_Kwd("cdecl",                 "C++ common word");
    కూడు_Kwd("char",                  "C++ keyword");
    కూడు_Kwd("char16_t",              "C++11 keyword");
    కూడు_Kwd("char32_t",              "C++11 keyword");
    కూడు_Kwd("class",                 "C++11 keyword");
    కూడు_Kwd("compl",                 "C++11 keyword");
    కూడు_Kwd("complex",               "C++ common word");
    కూడు_Kwd("concept",               "C++20 keyword");
    కూడు_Kwd("const",                 "C++ keyword");
    కూడు_Kwd("const_cast",            "C++ common word");
    కూడు_Kwd("const_iterator",        "C++ common word");
    కూడు_Kwd("const_reference ",      "C++ common word");
    కూడు_Kwd("constexpr",             "C++11 keyword");
    కూడు_Kwd("continue",              "C++ keyword");
    కూడు_Kwd("decltype",              "C++11 keyword");
    కూడు_Kwd("default",               "C++ keyword");
    కూడు_Kwd("delete",                "C++ keyword");
    కూడు_Kwd("do",                    "C++ keyword");
    కూడు_Kwd("double",                "C++ keyword");
    కూడు_Kwd("dynamic_cast",          "C++ keyword");
    కూడు_Kwd("else",                  "C++ keyword");
    కూడు_Kwd("enum",                  "C++ keyword");
    కూడు_Kwd("explicit",              "C++ keyword");
    కూడు_Kwd("export",                "C++ keyword");
    కూడు_Kwd("extern",                "C++ keyword");
    కూడు_Kwd("false",                 "C++ keyword");
    కూడు_Kwd("far",                   "C++ common word");
    కూడు_Kwd("float",                 "C++ keyword");
    కూడు_Kwd("for",                   "C++ keyword");
    కూడు_Kwd("friend",                "C++ keyword");
    కూడు_Kwd("goto",                  "C++ keyword");
    కూడు_Kwd("huge",                  "C++ keyword");
    కూడు_Kwd("if",                    "C++ keyword");
    కూడు_Kwd("import",                "C++ modules TS keyword");
    కూడు_Kwd("inline",                "C++ keyword");
    కూడు_Kwd("int",                   "C++ keyword");
    కూడు_Kwd("interrupt",             "C++ common word");
    కూడు_Kwd("iterator",              "C++ common word");
    కూడు_Kwd("long",                  "C++ keyword");
    కూడు_Kwd("module",                "C++ modules TS keyword");
    కూడు_Kwd("mutable",               "C++ keyword");
    కూడు_Kwd("namespace",             "C++ keyword");
    కూడు_Kwd("near",                  "C++ common word");
    కూడు_Kwd("new",                   "C++ keyword");
    కూడు_Kwd("noexcept",              "C++11 keyword");
    కూడు_Kwd("not",                   "C++ keyword");
    కూడు_Kwd("not_eq",                "C++ keyword");
    కూడు_Kwd("nullptr",               "C++ common word");
    కూడు_Kwd("nullptr",               "C++11 keyword");
    కూడు_Kwd("operator",              "C++ keyword");
    కూడు_Kwd("or",                    "C++ keyword");
    కూడు_Kwd("or_eq",                 "C++ keyword");
    కూడు_Kwd("override",              "C++ common word");
    కూడు_Kwd("pascal",                "C++ keyword");
    కూడు_Kwd("private",               "C++ keyword");
    కూడు_Kwd("protected",             "C++ keyword");
    కూడు_Kwd("public",                "C++ keyword");
    కూడు_Kwd("queue",                 "C++ common word");
    కూడు_Kwd("reference",             "C++ common word");
    కూడు_Kwd("register",              "C++ keyword");
    కూడు_Kwd("reinterpret_cast ",     "C++ keyword");
    కూడు_Kwd("requires",              "C++20 keyword");
    కూడు_Kwd("restrict",              "C++ keyword");
    కూడు_Kwd("return",                "C++ keyword");
    కూడు_Kwd("short",                 "C++ keyword");
    కూడు_Kwd("signed",                "C++ keyword");
    కూడు_Kwd("sizeof",                "C++ keyword");
    కూడు_Kwd("stack",                 "C++ common word");
    కూడు_Kwd("static",                "C++ keyword");
    కూడు_Kwd("static_assert",         "C++11 keyword");
    కూడు_Kwd("static_cast",           "C++ keyword");
    కూడు_Kwd("std::deque",            "C++ common word");
    కూడు_Kwd("std::list",             "C++ common word");
    కూడు_Kwd("std::map",              "C++ common word");
    కూడు_Kwd("std::multimap",         "C++ common word");
    కూడు_Kwd("std::multiset",         "C++ common word");
    కూడు_Kwd("std::set",              "C++ common word");
    కూడు_Kwd("std::vector",           "C++ common word");
    కూడు_Kwd("struct",                "C++ keyword");
    కూడు_Kwd("switch",                "C++ keyword");
    కూడు_Kwd("synchronized",          "C++ TM TS keyword");
    కూడు_Kwd("template",              "C++ keyword");
    కూడు_Kwd("this",                  "C++ keyword");
    కూడు_Kwd("thread_local",          "C++11 keyword");
    కూడు_Kwd("throw",                 "C++ keyword");
    కూడు_Kwd("transaction_safe",      "C++ common word");
    కూడు_Kwd("transaction_safe_dynamic",      "C++ common word");
    కూడు_Kwd("true",                  "C++ keyword");
    కూడు_Kwd("try",                   "C++ keyword");
    కూడు_Kwd("type_info",             "C++ common word");
    కూడు_Kwd("typedef",               "C++ keyword");
    కూడు_Kwd("typeid",                "C++ keyword");
    కూడు_Kwd("typename",              "C++ keyword");
    కూడు_Kwd("uint16_t",              "C++ common word");
    కూడు_Kwd("uint32_t",              "C++ common word");
    కూడు_Kwd("uint8_t",               "C++ common word");
    కూడు_Kwd("union",                 "C++ keyword");
    కూడు_Kwd("unsigned",              "C++ keyword");
    కూడు_Kwd("using",                 "C++ keyword");
    కూడు_Kwd("virtual",               "C++ keyword");
    కూడు_Kwd("void",                  "C++ keyword");
    కూడు_Kwd("volatile",              "C++ keyword");
    కూడు_Kwd("wchar_t",               "C++ keyword");
    కూడు_Kwd("while",                 "C++ keyword");
    కూడు_Kwd("xor",                   "C++ keyword");
    కూడు_Kwd("xor_eq",                "C++ keyword");
    // This conflicts with header functions, so is ignored
    //dKwd("final",                 "C++11 keyword");
    // SystemC
    కూడు_Kwd("sc_clock",              "SystemC common word");
    కూడు_Kwd("sc_in",                 "SystemC common word");
    కూడు_Kwd("sc_inout",              "SystemC common word");
    కూడు_Kwd("sc_out",                "SystemC common word");
    కూడు_Kwd("sc_signal",             "SystemC common word");
    కూడు_Kwd("sensitive",             "SystemC common word");
    కూడు_Kwd("sensitive_neg",         "SystemC common word");
    కూడు_Kwd("sensitive_pos",         "SystemC common word");
    // Preprocessor defined words
    కూడు_Kwd("`__FILE__",                 "Verilog preprocessor directive");
    కూడు_Kwd("`__LINE__",                 "Verilog preprocessor directive");
    కూడు_Kwd("`accelerate",               "Verilog-XL preprocessor directive");
    కూడు_Kwd("`autoexpand_vectornets",    "Verilog-XL preprocessor directive");
    కూడు_Kwd("`begin_keywords",           "Verilog preprocessor directive");
    కూడు_Kwd("`celldefine",               "Verilog preprocessor directive");
    కూడు_Kwd("`default_decay_time",       "Verilog preprocessor directive");
    కూడు_Kwd("`default_nettype",          "Verilog preprocessor directive");
    కూడు_Kwd("`default_trireg_strength",  "Verilog preprocessor directive");
    కూడు_Kwd("`define",                   "Verilog preprocessor directive");
    కూడు_Kwd("`delay_mode_distributed",   "Verilog preprocessor directive");
    కూడు_Kwd("`delay_mode_path",          "Verilog preprocessor directive");
    కూడు_Kwd("`delay_mode_unit",          "Verilog preprocessor directive");
    కూడు_Kwd("`delay_mode_zero",          "Verilog preprocessor directive");
    కూడు_Kwd("`disable_portfaults",       "Verilog-XL preprocessor directive");
    కూడు_Kwd("`else",                     "Verilog preprocessor directive");
    కూడు_Kwd("`elsif",                    "Verilog preprocessor directive");
    కూడు_Kwd("`enable_portfaults",        "Verilog-XL preprocessor directive");
    కూడు_Kwd("`end_keywords",             "Verilog preprocessor directive");
    కూడు_Kwd("`endcelldefine",            "Verilog preprocessor directive");
    కూడు_Kwd("`endif",                    "Verilog preprocessor directive");
    కూడు_Kwd("`endprotect",               "Verilog preprocessor directive");
    కూడు_Kwd("`endprotected",             "Verilog preprocessor directive");
    కూడు_Kwd("`error",                    "Verilog preprocessor directive");
    కూడు_Kwd("`expand_vectornets",        "Verilog-XL preprocessor directive");
    కూడు_Kwd("`ifdef",                    "Verilog preprocessor directive");
    కూడు_Kwd("`ifndef",                   "Verilog preprocessor directive");
    కూడు_Kwd("`include",                  "Verilog preprocessor directive");
    కూడు_Kwd("`inline",                   "Verilog preprocessor directive");
    కూడు_Kwd("`line",                     "Verilog preprocessor directive");
    కూడు_Kwd("`noaccelerate",             "Verilog-XL preprocessor directive");
    కూడు_Kwd("`noexpand_vectornets",      "Verilog-XL preprocessor directive");
    కూడు_Kwd("`noremove_gatenames",       "Verilog-XL preprocessor directive");
    కూడు_Kwd("`noremove_netnames",        "Verilog-XL preprocessor directive");
    కూడు_Kwd("`nosuppress_faults",        "Verilog-XL preprocessor directive");
    కూడు_Kwd("`nounconnected_drive",      "Verilog-XL preprocessor directive");
    కూడు_Kwd("`portcoerce",               "Verilog preprocessor directive");
    కూడు_Kwd("`pragma",                   "Verilog preprocessor directive");
    కూడు_Kwd("`protect",                  "Verilog preprocessor directive");
    కూడు_Kwd("`protected",                "Verilog preprocessor directive");
    కూడు_Kwd("`remove_gatenames",         "Verilog-XL preprocessor directive");
    కూడు_Kwd("`remove_netnames",          "Verilog-XL preprocessor directive");
    కూడు_Kwd("`resetall",                 "Verilog preprocessor directive");
    కూడు_Kwd("`suppress_faults",          "Verilog-XL preprocessor directive");
    కూడు_Kwd("`systemc_ctor",             "Verilator preprocessor directive");
    కూడు_Kwd("`systemc_dtor",             "Verilator preprocessor directive");
    కూడు_Kwd("`systemc_header",           "Verilator preprocessor directive");
    కూడు_Kwd("`systemc_header_post",      "Verilator preprocessor directive");
    కూడు_Kwd("`systemc_imp_header",       "Verilator preprocessor directive");
    కూడు_Kwd("`systemc_implementation",   "Verilator preprocessor directive");
    కూడు_Kwd("`systemc_interface",        "Verilator preprocessor directive");
    కూడు_Kwd("`timescale",                "Verilog preprocessor directive");
    కూడు_Kwd("`undef",                    "Verilog preprocessor directive");
    కూడు_Kwd("`undefineall",              "Verilog preprocessor directive");
    కూడు_Kwd("`verilator_config",         "Verilator preprocessor directive");
    కూడు_Kwd("`verilog",                  "Verilator preprocessor directive");
    // clang-format on
}

#అయితే_ముగింపు  // Guard
