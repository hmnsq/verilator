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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3LANGUAGEWORDS_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3LANGUAGEWORDS_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ <map>

//============================================================================

class V3_Language_ಪದಗಳು final {
    // List of common reserved keywords
ಖಾಸಗಿ:
    using KeywordMap = std::map<ಸ್ಥಿರಾಂಕ string, std::string>;
    ರಚನೆ Singleton final {
        KeywordMap s_kwdMap;  // List of keywords, and what language applies
        Singleton() { init(); }
        ಶೂನ್ಯ ಕೂಡಿಸು_Kwd(ಸ್ಥಿರಾಂಕ string& kwd, ಸ್ಥಿರಾಂಕ string& why) { s_kwdMap.emplace(kwd, why); }
        ಶೂನ್ಯ init();
    };

ಸಾರ್ವಜನಿಕ:
    using const_iterator = KeywordMap::const_iterator;
    // METHODS
    ಸ್ಥಿರ const_iterator begin() { ಹಿಂತಿರುಗಿ s().s_kwdMap.begin(); }
    ಸ್ಥಿರ const_iterator ಕೊನೆ() { ಹಿಂತಿರುಗಿ s().s_kwdMap.ಕೊನೆ(); }
    ಸ್ಥಿರ string isKeyword(ಸ್ಥಿರಾಂಕ string& kwd) {
        ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ it = vlstd::as_const(s().s_kwdMap).ಕಂಡುಹಿಡಿ(kwd);
        ಆದರೆ (it == s().s_kwdMap.ಕೊನೆ()) ಹಿಂತಿರುಗಿ "";
        ಹಿಂತಿರುಗಿ it->ಎರಡನೇ;
    }

ಖಾಸಗಿ:
    ಸ್ಥಿರ Singleton& s() {
        ಸ್ಥಿರ Singleton s_s;  // LCOV_EXCL_BR_LINE
        ಹಿಂತಿರುಗಿ s_s;
    }
};

ಸಾಲೊಳಗೆ ಶೂನ್ಯ V3_Language_ಪದಗಳು::Singleton::init() {
    // C++ keywords
    // clang-format off
    ಕೂಡಿಸು_Kwd("abort",                 "C++ common word");
    ಕೂಡಿಸು_Kwd("alignas",               "C++11 keyword");
    ಕೂಡಿಸು_Kwd("alignof",               "C++11 keyword");
    ಕೂಡಿಸು_Kwd("and",                   "C++11 keyword");
    ಕೂಡಿಸು_Kwd("and_eq",                "C++11 keyword");
    ಕೂಡಿಸು_Kwd("asm",                   "C++ common word");
    ಕೂಡಿಸು_Kwd("atomic_cancel",         "C++ TM TS keyword");
    ಕೂಡಿಸು_Kwd("atomic_commit",         "C++ TM TS keyword");
    ಕೂಡಿಸು_Kwd("atomic_noexcept",       "C++ TM TS keyword");
    ಕೂಡಿಸು_Kwd("auto",                  "C++ keyword");
    ಕೂಡಿಸು_Kwd("bit_vector",            "C++ common word");
    ಕೂಡಿಸು_Kwd("bitand",                "C++ keyword");
    ಕೂಡಿಸು_Kwd("bitor",                 "C++ keyword");
    ಕೂಡಿಸು_Kwd("bool",                  "C++ keyword");
    ಕೂಡಿಸು_Kwd("break",                 "C++ keyword");
    ಕೂಡಿಸು_Kwd("case",                  "C++ keyword");
    ಕೂಡಿಸು_Kwd("catch",                 "C++ keyword");
    ಕೂಡಿಸು_Kwd("cdecl",                 "C++ common word");
    ಕೂಡಿಸು_Kwd("char",                  "C++ keyword");
    ಕೂಡಿಸು_Kwd("char16_t",              "C++11 keyword");
    ಕೂಡಿಸು_Kwd("char32_t",              "C++11 keyword");
    ಕೂಡಿಸು_Kwd("class",                 "C++11 keyword");
    ಕೂಡಿಸು_Kwd("compl",                 "C++11 keyword");
    ಕೂಡಿಸು_Kwd("complex",               "C++ common word");
    ಕೂಡಿಸು_Kwd("concept",               "C++20 keyword");
    ಕೂಡಿಸು_Kwd("const",                 "C++ keyword");
    ಕೂಡಿಸು_Kwd("const_cast",            "C++ common word");
    ಕೂಡಿಸು_Kwd("const_iterator",        "C++ common word");
    ಕೂಡಿಸು_Kwd("const_reference ",      "C++ common word");
    ಕೂಡಿಸು_Kwd("constexpr",             "C++11 keyword");
    ಕೂಡಿಸು_Kwd("continue",              "C++ keyword");
    ಕೂಡಿಸು_Kwd("decltype",              "C++11 keyword");
    ಕೂಡಿಸು_Kwd("default",               "C++ keyword");
    ಕೂಡಿಸು_Kwd("delete",                "C++ keyword");
    ಕೂಡಿಸು_Kwd("do",                    "C++ keyword");
    ಕೂಡಿಸು_Kwd("double",                "C++ keyword");
    ಕೂಡಿಸು_Kwd("dynamic_cast",          "C++ keyword");
    ಕೂಡಿಸು_Kwd("else",                  "C++ keyword");
    ಕೂಡಿಸು_Kwd("enum",                  "C++ keyword");
    ಕೂಡಿಸು_Kwd("explicit",              "C++ keyword");
    ಕೂಡಿಸು_Kwd("export",                "C++ keyword");
    ಕೂಡಿಸು_Kwd("extern",                "C++ keyword");
    ಕೂಡಿಸು_Kwd("false",                 "C++ keyword");
    ಕೂಡಿಸು_Kwd("far",                   "C++ common word");
    ಕೂಡಿಸು_Kwd("float",                 "C++ keyword");
    ಕೂಡಿಸು_Kwd("for",                   "C++ keyword");
    ಕೂಡಿಸು_Kwd("friend",                "C++ keyword");
    ಕೂಡಿಸು_Kwd("goto",                  "C++ keyword");
    ಕೂಡಿಸು_Kwd("huge",                  "C++ keyword");
    ಕೂಡಿಸು_Kwd("if",                    "C++ keyword");
    ಕೂಡಿಸು_Kwd("import",                "C++ modules TS keyword");
    ಕೂಡಿಸು_Kwd("inline",                "C++ keyword");
    ಕೂಡಿಸು_Kwd("int",                   "C++ keyword");
    ಕೂಡಿಸು_Kwd("interrupt",             "C++ common word");
    ಕೂಡಿಸು_Kwd("iterator",              "C++ common word");
    ಕೂಡಿಸು_Kwd("long",                  "C++ keyword");
    ಕೂಡಿಸು_Kwd("module",                "C++ modules TS keyword");
    ಕೂಡಿಸು_Kwd("mutable",               "C++ keyword");
    ಕೂಡಿಸು_Kwd("namespace",             "C++ keyword");
    ಕೂಡಿಸು_Kwd("near",                  "C++ common word");
    ಕೂಡಿಸು_Kwd("new",                   "C++ keyword");
    ಕೂಡಿಸು_Kwd("noexcept",              "C++11 keyword");
    ಕೂಡಿಸು_Kwd("not",                   "C++ keyword");
    ಕೂಡಿಸು_Kwd("not_eq",                "C++ keyword");
    ಕೂಡಿಸು_Kwd("nullptr",               "C++ common word");
    ಕೂಡಿಸು_Kwd("nullptr",               "C++11 keyword");
    ಕೂಡಿಸು_Kwd("operator",              "C++ keyword");
    ಕೂಡಿಸು_Kwd("or",                    "C++ keyword");
    ಕೂಡಿಸು_Kwd("or_eq",                 "C++ keyword");
    ಕೂಡಿಸು_Kwd("override",              "C++ common word");
    ಕೂಡಿಸು_Kwd("pascal",                "C++ keyword");
    ಕೂಡಿಸು_Kwd("private",               "C++ keyword");
    ಕೂಡಿಸು_Kwd("protected",             "C++ keyword");
    ಕೂಡಿಸು_Kwd("public",                "C++ keyword");
    ಕೂಡಿಸು_Kwd("queue",                 "C++ common word");
    ಕೂಡಿಸು_Kwd("reference",             "C++ common word");
    ಕೂಡಿಸು_Kwd("register",              "C++ keyword");
    ಕೂಡಿಸು_Kwd("reinterpret_cast ",     "C++ keyword");
    ಕೂಡಿಸು_Kwd("requires",              "C++20 keyword");
    ಕೂಡಿಸು_Kwd("restrict",              "C++ keyword");
    ಕೂಡಿಸು_Kwd("return",                "C++ keyword");
    ಕೂಡಿಸು_Kwd("short",                 "C++ keyword");
    ಕೂಡಿಸು_Kwd("signed",                "C++ keyword");
    ಕೂಡಿಸು_Kwd("sizeof",                "C++ keyword");
    ಕೂಡಿಸು_Kwd("stack",                 "C++ common word");
    ಕೂಡಿಸು_Kwd("static",                "C++ keyword");
    ಕೂಡಿಸು_Kwd("static_assert",         "C++11 keyword");
    ಕೂಡಿಸು_Kwd("static_cast",           "C++ keyword");
    ಕೂಡಿಸು_Kwd("std::deque",            "C++ common word");
    ಕೂಡಿಸು_Kwd("std::list",             "C++ common word");
    ಕೂಡಿಸು_Kwd("std::map",              "C++ common word");
    ಕೂಡಿಸು_Kwd("std::multimap",         "C++ common word");
    ಕೂಡಿಸು_Kwd("std::multiset",         "C++ common word");
    ಕೂಡಿಸು_Kwd("std::set",              "C++ common word");
    ಕೂಡಿಸು_Kwd("std::vector",           "C++ common word");
    ಕೂಡಿಸು_Kwd("struct",                "C++ keyword");
    ಕೂಡಿಸು_Kwd("switch",                "C++ keyword");
    ಕೂಡಿಸು_Kwd("synchronized",          "C++ TM TS keyword");
    ಕೂಡಿಸು_Kwd("template",              "C++ keyword");
    ಕೂಡಿಸು_Kwd("this",                  "C++ keyword");
    ಕೂಡಿಸು_Kwd("thread_local",          "C++11 keyword");
    ಕೂಡಿಸು_Kwd("throw",                 "C++ keyword");
    ಕೂಡಿಸು_Kwd("transaction_safe",      "C++ common word");
    ಕೂಡಿಸು_Kwd("transaction_safe_dynamic",      "C++ common word");
    ಕೂಡಿಸು_Kwd("true",                  "C++ keyword");
    ಕೂಡಿಸು_Kwd("try",                   "C++ keyword");
    ಕೂಡಿಸು_Kwd("type_info",             "C++ common word");
    ಕೂಡಿಸು_Kwd("typedef",               "C++ keyword");
    ಕೂಡಿಸು_Kwd("typeid",                "C++ keyword");
    ಕೂಡಿಸು_Kwd("typename",              "C++ keyword");
    ಕೂಡಿಸು_Kwd("uint16_t",              "C++ common word");
    ಕೂಡಿಸು_Kwd("uint32_t",              "C++ common word");
    ಕೂಡಿಸು_Kwd("uint8_t",               "C++ common word");
    ಕೂಡಿಸು_Kwd("union",                 "C++ keyword");
    ಕೂಡಿಸು_Kwd("unsigned",              "C++ keyword");
    ಕೂಡಿಸು_Kwd("using",                 "C++ keyword");
    ಕೂಡಿಸು_Kwd("virtual",               "C++ keyword");
    ಕೂಡಿಸು_Kwd("void",                  "C++ keyword");
    ಕೂಡಿಸು_Kwd("volatile",              "C++ keyword");
    ಕೂಡಿಸು_Kwd("wchar_t",               "C++ keyword");
    ಕೂಡಿಸು_Kwd("while",                 "C++ keyword");
    ಕೂಡಿಸು_Kwd("xor",                   "C++ keyword");
    ಕೂಡಿಸು_Kwd("xor_eq",                "C++ keyword");
    // This conflicts with header functions, so is ignored
    //dKwd("final",                 "C++11 keyword");
    // SystemC
    ಕೂಡಿಸು_Kwd("sc_clock",              "SystemC common word");
    ಕೂಡಿಸು_Kwd("sc_in",                 "SystemC common word");
    ಕೂಡಿಸು_Kwd("sc_inout",              "SystemC common word");
    ಕೂಡಿಸು_Kwd("sc_out",                "SystemC common word");
    ಕೂಡಿಸು_Kwd("sc_signal",             "SystemC common word");
    ಕೂಡಿಸು_Kwd("sensitive",             "SystemC common word");
    ಕೂಡಿಸು_Kwd("sensitive_neg",         "SystemC common word");
    ಕೂಡಿಸು_Kwd("sensitive_pos",         "SystemC common word");
    // Preprocessor defined words
    ಕೂಡಿಸು_Kwd("`__FILE__",                 "Verilog preprocessor directive");
    ಕೂಡಿಸು_Kwd("`__LINE__",                 "Verilog preprocessor directive");
    ಕೂಡಿಸು_Kwd("`accelerate",               "Verilog-XL preprocessor directive");
    ಕೂಡಿಸು_Kwd("`autoexpand_vectornets",    "Verilog-XL preprocessor directive");
    ಕೂಡಿಸು_Kwd("`begin_keywords",           "Verilog preprocessor directive");
    ಕೂಡಿಸು_Kwd("`celldefine",               "Verilog preprocessor directive");
    ಕೂಡಿಸು_Kwd("`default_decay_time",       "Verilog preprocessor directive");
    ಕೂಡಿಸು_Kwd("`default_nettype",          "Verilog preprocessor directive");
    ಕೂಡಿಸು_Kwd("`default_trireg_strength",  "Verilog preprocessor directive");
    ಕೂಡಿಸು_Kwd("`define",                   "Verilog preprocessor directive");
    ಕೂಡಿಸು_Kwd("`delay_mode_distributed",   "Verilog preprocessor directive");
    ಕೂಡಿಸು_Kwd("`delay_mode_path",          "Verilog preprocessor directive");
    ಕೂಡಿಸು_Kwd("`delay_mode_unit",          "Verilog preprocessor directive");
    ಕೂಡಿಸು_Kwd("`delay_mode_zero",          "Verilog preprocessor directive");
    ಕೂಡಿಸು_Kwd("`disable_portfaults",       "Verilog-XL preprocessor directive");
    ಕೂಡಿಸು_Kwd("`else",                     "Verilog preprocessor directive");
    ಕೂಡಿಸು_Kwd("`elsif",                    "Verilog preprocessor directive");
    ಕೂಡಿಸು_Kwd("`enable_portfaults",        "Verilog-XL preprocessor directive");
    ಕೂಡಿಸು_Kwd("`end_keywords",             "Verilog preprocessor directive");
    ಕೂಡಿಸು_Kwd("`endcelldefine",            "Verilog preprocessor directive");
    ಕೂಡಿಸು_Kwd("`endif",                    "Verilog preprocessor directive");
    ಕೂಡಿಸು_Kwd("`endprotect",               "Verilog preprocessor directive");
    ಕೂಡಿಸು_Kwd("`endprotected",             "Verilog preprocessor directive");
    ಕೂಡಿಸು_Kwd("`error",                    "Verilog preprocessor directive");
    ಕೂಡಿಸು_Kwd("`expand_vectornets",        "Verilog-XL preprocessor directive");
    ಕೂಡಿಸು_Kwd("`ifdef",                    "Verilog preprocessor directive");
    ಕೂಡಿಸು_Kwd("`ifndef",                   "Verilog preprocessor directive");
    ಕೂಡಿಸು_Kwd("`include",                  "Verilog preprocessor directive");
    ಕೂಡಿಸು_Kwd("`inline",                   "Verilog preprocessor directive");
    ಕೂಡಿಸು_Kwd("`line",                     "Verilog preprocessor directive");
    ಕೂಡಿಸು_Kwd("`noaccelerate",             "Verilog-XL preprocessor directive");
    ಕೂಡಿಸು_Kwd("`noexpand_vectornets",      "Verilog-XL preprocessor directive");
    ಕೂಡಿಸು_Kwd("`noremove_gatenames",       "Verilog-XL preprocessor directive");
    ಕೂಡಿಸು_Kwd("`noremove_netnames",        "Verilog-XL preprocessor directive");
    ಕೂಡಿಸು_Kwd("`nosuppress_faults",        "Verilog-XL preprocessor directive");
    ಕೂಡಿಸು_Kwd("`nounconnected_drive",      "Verilog-XL preprocessor directive");
    ಕೂಡಿಸು_Kwd("`portcoerce",               "Verilog preprocessor directive");
    ಕೂಡಿಸು_Kwd("`pragma",                   "Verilog preprocessor directive");
    ಕೂಡಿಸು_Kwd("`protect",                  "Verilog preprocessor directive");
    ಕೂಡಿಸು_Kwd("`protected",                "Verilog preprocessor directive");
    ಕೂಡಿಸು_Kwd("`remove_gatenames",         "Verilog-XL preprocessor directive");
    ಕೂಡಿಸು_Kwd("`remove_netnames",          "Verilog-XL preprocessor directive");
    ಕೂಡಿಸು_Kwd("`resetall",                 "Verilog preprocessor directive");
    ಕೂಡಿಸು_Kwd("`suppress_faults",          "Verilog-XL preprocessor directive");
    ಕೂಡಿಸು_Kwd("`systemc_ctor",             "Verilator preprocessor directive");
    ಕೂಡಿಸು_Kwd("`systemc_dtor",             "Verilator preprocessor directive");
    ಕೂಡಿಸು_Kwd("`systemc_header",           "Verilator preprocessor directive");
    ಕೂಡಿಸು_Kwd("`systemc_header_post",      "Verilator preprocessor directive");
    ಕೂಡಿಸು_Kwd("`systemc_imp_header",       "Verilator preprocessor directive");
    ಕೂಡಿಸು_Kwd("`systemc_implementation",   "Verilator preprocessor directive");
    ಕೂಡಿಸು_Kwd("`systemc_interface",        "Verilator preprocessor directive");
    ಕೂಡಿಸು_Kwd("`timescale",                "Verilog preprocessor directive");
    ಕೂಡಿಸು_Kwd("`undef",                    "Verilog preprocessor directive");
    ಕೂಡಿಸು_Kwd("`undefineall",              "Verilog preprocessor directive");
    ಕೂಡಿಸು_Kwd("`verilator_config",         "Verilator preprocessor directive");
    ಕೂಡಿಸು_Kwd("`verilog",                  "Verilator preprocessor directive");
    // clang-format on
}

#ಆದರೆ_ಅಂತ್ಯ  // Guard
