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

#اگر_غیر_تعریف_شدہ VERILATOR_V3LANGUAGEWORDS_H_
#تعریف_کرو VERILATOR_V3LANGUAGEWORDS_H_

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل <map>

//============================================================================

class V3_Language_الفاظ final {
    // List of common reserved keywords
نجی:
    using KeywordMap = std::map<ثابت string, std::string>;
    ساخت Singleton final {
        KeywordMap s_kwdMap;  // List of keywords, and what language applies
        Singleton() { init(); }
        خالی جوڑو_Kwd(ثابت string& kwd, ثابت string& why) { s_kwdMap.emplace(kwd, why); }
        خالی init();
    };

عوامی:
    using const_iterator = KeywordMap::const_iterator;
    // METHODS
    ساکن const_iterator begin() { واپس s().s_kwdMap.begin(); }
    ساکن const_iterator انتہا() { واپس s().s_kwdMap.انتہا(); }
    ساکن string isKeyword(ثابت string& kwd) {
        ثابت خودکار it = vlstd::as_const(s().s_kwdMap).ڈھونڈو(kwd);
        اگر (it == s().s_kwdMap.انتہا()) واپس "";
        واپس it->دوسرا;
    }

نجی:
    ساکن Singleton& s() {
        ساکن Singleton s_s;  // LCOV_EXCL_BR_LINE
        واپس s_s;
    }
};

اندرونی خالی V3_Language_الفاظ::Singleton::init() {
    // C++ keywords
    // clang-format off
    جوڑو_Kwd("abort",                 "C++ common word");
    جوڑو_Kwd("alignas",               "C++11 keyword");
    جوڑو_Kwd("alignof",               "C++11 keyword");
    جوڑو_Kwd("and",                   "C++11 keyword");
    جوڑو_Kwd("and_eq",                "C++11 keyword");
    جوڑو_Kwd("asm",                   "C++ common word");
    جوڑو_Kwd("atomic_cancel",         "C++ TM TS keyword");
    جوڑو_Kwd("atomic_commit",         "C++ TM TS keyword");
    جوڑو_Kwd("atomic_noexcept",       "C++ TM TS keyword");
    جوڑو_Kwd("auto",                  "C++ keyword");
    جوڑو_Kwd("bit_vector",            "C++ common word");
    جوڑو_Kwd("bitand",                "C++ keyword");
    جوڑو_Kwd("bitor",                 "C++ keyword");
    جوڑو_Kwd("bool",                  "C++ keyword");
    جوڑو_Kwd("break",                 "C++ keyword");
    جوڑو_Kwd("case",                  "C++ keyword");
    جوڑو_Kwd("catch",                 "C++ keyword");
    جوڑو_Kwd("cdecl",                 "C++ common word");
    جوڑو_Kwd("char",                  "C++ keyword");
    جوڑو_Kwd("char16_t",              "C++11 keyword");
    جوڑو_Kwd("char32_t",              "C++11 keyword");
    جوڑو_Kwd("class",                 "C++11 keyword");
    جوڑو_Kwd("compl",                 "C++11 keyword");
    جوڑو_Kwd("complex",               "C++ common word");
    جوڑو_Kwd("concept",               "C++20 keyword");
    جوڑو_Kwd("const",                 "C++ keyword");
    جوڑو_Kwd("const_cast",            "C++ common word");
    جوڑو_Kwd("const_iterator",        "C++ common word");
    جوڑو_Kwd("const_reference ",      "C++ common word");
    جوڑو_Kwd("constexpr",             "C++11 keyword");
    جوڑو_Kwd("continue",              "C++ keyword");
    جوڑو_Kwd("decltype",              "C++11 keyword");
    جوڑو_Kwd("default",               "C++ keyword");
    جوڑو_Kwd("delete",                "C++ keyword");
    جوڑو_Kwd("do",                    "C++ keyword");
    جوڑو_Kwd("double",                "C++ keyword");
    جوڑو_Kwd("dynamic_cast",          "C++ keyword");
    جوڑو_Kwd("else",                  "C++ keyword");
    جوڑو_Kwd("enum",                  "C++ keyword");
    جوڑو_Kwd("explicit",              "C++ keyword");
    جوڑو_Kwd("export",                "C++ keyword");
    جوڑو_Kwd("extern",                "C++ keyword");
    جوڑو_Kwd("false",                 "C++ keyword");
    جوڑو_Kwd("far",                   "C++ common word");
    جوڑو_Kwd("float",                 "C++ keyword");
    جوڑو_Kwd("for",                   "C++ keyword");
    جوڑو_Kwd("friend",                "C++ keyword");
    جوڑو_Kwd("goto",                  "C++ keyword");
    جوڑو_Kwd("huge",                  "C++ keyword");
    جوڑو_Kwd("if",                    "C++ keyword");
    جوڑو_Kwd("import",                "C++ modules TS keyword");
    جوڑو_Kwd("inline",                "C++ keyword");
    جوڑو_Kwd("int",                   "C++ keyword");
    جوڑو_Kwd("interrupt",             "C++ common word");
    جوڑو_Kwd("iterator",              "C++ common word");
    جوڑو_Kwd("long",                  "C++ keyword");
    جوڑو_Kwd("module",                "C++ modules TS keyword");
    جوڑو_Kwd("mutable",               "C++ keyword");
    جوڑو_Kwd("namespace",             "C++ keyword");
    جوڑو_Kwd("near",                  "C++ common word");
    جوڑو_Kwd("new",                   "C++ keyword");
    جوڑو_Kwd("noexcept",              "C++11 keyword");
    جوڑو_Kwd("not",                   "C++ keyword");
    جوڑو_Kwd("not_eq",                "C++ keyword");
    جوڑو_Kwd("nullptr",               "C++ common word");
    جوڑو_Kwd("nullptr",               "C++11 keyword");
    جوڑو_Kwd("operator",              "C++ keyword");
    جوڑو_Kwd("or",                    "C++ keyword");
    جوڑو_Kwd("or_eq",                 "C++ keyword");
    جوڑو_Kwd("override",              "C++ common word");
    جوڑو_Kwd("pascal",                "C++ keyword");
    جوڑو_Kwd("private",               "C++ keyword");
    جوڑو_Kwd("protected",             "C++ keyword");
    جوڑو_Kwd("public",                "C++ keyword");
    جوڑو_Kwd("queue",                 "C++ common word");
    جوڑو_Kwd("reference",             "C++ common word");
    جوڑو_Kwd("register",              "C++ keyword");
    جوڑو_Kwd("reinterpret_cast ",     "C++ keyword");
    جوڑو_Kwd("requires",              "C++20 keyword");
    جوڑو_Kwd("restrict",              "C++ keyword");
    جوڑو_Kwd("return",                "C++ keyword");
    جوڑو_Kwd("short",                 "C++ keyword");
    جوڑو_Kwd("signed",                "C++ keyword");
    جوڑو_Kwd("sizeof",                "C++ keyword");
    جوڑو_Kwd("stack",                 "C++ common word");
    جوڑو_Kwd("static",                "C++ keyword");
    جوڑو_Kwd("static_assert",         "C++11 keyword");
    جوڑو_Kwd("static_cast",           "C++ keyword");
    جوڑو_Kwd("std::deque",            "C++ common word");
    جوڑو_Kwd("std::list",             "C++ common word");
    جوڑو_Kwd("std::map",              "C++ common word");
    جوڑو_Kwd("std::multimap",         "C++ common word");
    جوڑو_Kwd("std::multiset",         "C++ common word");
    جوڑو_Kwd("std::set",              "C++ common word");
    جوڑو_Kwd("std::vector",           "C++ common word");
    جوڑو_Kwd("struct",                "C++ keyword");
    جوڑو_Kwd("switch",                "C++ keyword");
    جوڑو_Kwd("synchronized",          "C++ TM TS keyword");
    جوڑو_Kwd("template",              "C++ keyword");
    جوڑو_Kwd("this",                  "C++ keyword");
    جوڑو_Kwd("thread_local",          "C++11 keyword");
    جوڑو_Kwd("throw",                 "C++ keyword");
    جوڑو_Kwd("transaction_safe",      "C++ common word");
    جوڑو_Kwd("transaction_safe_dynamic",      "C++ common word");
    جوڑو_Kwd("true",                  "C++ keyword");
    جوڑو_Kwd("try",                   "C++ keyword");
    جوڑو_Kwd("type_info",             "C++ common word");
    جوڑو_Kwd("typedef",               "C++ keyword");
    جوڑو_Kwd("typeid",                "C++ keyword");
    جوڑو_Kwd("typename",              "C++ keyword");
    جوڑو_Kwd("uint16_t",              "C++ common word");
    جوڑو_Kwd("uint32_t",              "C++ common word");
    جوڑو_Kwd("uint8_t",               "C++ common word");
    جوڑو_Kwd("union",                 "C++ keyword");
    جوڑو_Kwd("unsigned",              "C++ keyword");
    جوڑو_Kwd("using",                 "C++ keyword");
    جوڑو_Kwd("virtual",               "C++ keyword");
    جوڑو_Kwd("void",                  "C++ keyword");
    جوڑو_Kwd("volatile",              "C++ keyword");
    جوڑو_Kwd("wchar_t",               "C++ keyword");
    جوڑو_Kwd("while",                 "C++ keyword");
    جوڑو_Kwd("xor",                   "C++ keyword");
    جوڑو_Kwd("xor_eq",                "C++ keyword");
    // This conflicts with header functions, so is ignored
    //dKwd("final",                 "C++11 keyword");
    // SystemC
    جوڑو_Kwd("sc_clock",              "SystemC common word");
    جوڑو_Kwd("sc_in",                 "SystemC common word");
    جوڑو_Kwd("sc_inout",              "SystemC common word");
    جوڑو_Kwd("sc_out",                "SystemC common word");
    جوڑو_Kwd("sc_signal",             "SystemC common word");
    جوڑو_Kwd("sensitive",             "SystemC common word");
    جوڑو_Kwd("sensitive_neg",         "SystemC common word");
    جوڑو_Kwd("sensitive_pos",         "SystemC common word");
    // Preprocessor defined words
    جوڑو_Kwd("`__FILE__",                 "Verilog preprocessor directive");
    جوڑو_Kwd("`__LINE__",                 "Verilog preprocessor directive");
    جوڑو_Kwd("`accelerate",               "Verilog-XL preprocessor directive");
    جوڑو_Kwd("`autoexpand_vectornets",    "Verilog-XL preprocessor directive");
    جوڑو_Kwd("`begin_keywords",           "Verilog preprocessor directive");
    جوڑو_Kwd("`celldefine",               "Verilog preprocessor directive");
    جوڑو_Kwd("`default_decay_time",       "Verilog preprocessor directive");
    جوڑو_Kwd("`default_nettype",          "Verilog preprocessor directive");
    جوڑو_Kwd("`default_trireg_strength",  "Verilog preprocessor directive");
    جوڑو_Kwd("`define",                   "Verilog preprocessor directive");
    جوڑو_Kwd("`delay_mode_distributed",   "Verilog preprocessor directive");
    جوڑو_Kwd("`delay_mode_path",          "Verilog preprocessor directive");
    جوڑو_Kwd("`delay_mode_unit",          "Verilog preprocessor directive");
    جوڑو_Kwd("`delay_mode_zero",          "Verilog preprocessor directive");
    جوڑو_Kwd("`disable_portfaults",       "Verilog-XL preprocessor directive");
    جوڑو_Kwd("`else",                     "Verilog preprocessor directive");
    جوڑو_Kwd("`elsif",                    "Verilog preprocessor directive");
    جوڑو_Kwd("`enable_portfaults",        "Verilog-XL preprocessor directive");
    جوڑو_Kwd("`end_keywords",             "Verilog preprocessor directive");
    جوڑو_Kwd("`endcelldefine",            "Verilog preprocessor directive");
    جوڑو_Kwd("`endif",                    "Verilog preprocessor directive");
    جوڑو_Kwd("`endprotect",               "Verilog preprocessor directive");
    جوڑو_Kwd("`endprotected",             "Verilog preprocessor directive");
    جوڑو_Kwd("`error",                    "Verilog preprocessor directive");
    جوڑو_Kwd("`expand_vectornets",        "Verilog-XL preprocessor directive");
    جوڑو_Kwd("`ifdef",                    "Verilog preprocessor directive");
    جوڑو_Kwd("`ifndef",                   "Verilog preprocessor directive");
    جوڑو_Kwd("`include",                  "Verilog preprocessor directive");
    جوڑو_Kwd("`inline",                   "Verilog preprocessor directive");
    جوڑو_Kwd("`line",                     "Verilog preprocessor directive");
    جوڑو_Kwd("`noaccelerate",             "Verilog-XL preprocessor directive");
    جوڑو_Kwd("`noexpand_vectornets",      "Verilog-XL preprocessor directive");
    جوڑو_Kwd("`noremove_gatenames",       "Verilog-XL preprocessor directive");
    جوڑو_Kwd("`noremove_netnames",        "Verilog-XL preprocessor directive");
    جوڑو_Kwd("`nosuppress_faults",        "Verilog-XL preprocessor directive");
    جوڑو_Kwd("`nounconnected_drive",      "Verilog-XL preprocessor directive");
    جوڑو_Kwd("`portcoerce",               "Verilog preprocessor directive");
    جوڑو_Kwd("`pragma",                   "Verilog preprocessor directive");
    جوڑو_Kwd("`protect",                  "Verilog preprocessor directive");
    جوڑو_Kwd("`protected",                "Verilog preprocessor directive");
    جوڑو_Kwd("`remove_gatenames",         "Verilog-XL preprocessor directive");
    جوڑو_Kwd("`remove_netnames",          "Verilog-XL preprocessor directive");
    جوڑو_Kwd("`resetall",                 "Verilog preprocessor directive");
    جوڑو_Kwd("`suppress_faults",          "Verilog-XL preprocessor directive");
    جوڑو_Kwd("`systemc_ctor",             "Verilator preprocessor directive");
    جوڑو_Kwd("`systemc_dtor",             "Verilator preprocessor directive");
    جوڑو_Kwd("`systemc_header",           "Verilator preprocessor directive");
    جوڑو_Kwd("`systemc_header_post",      "Verilator preprocessor directive");
    جوڑو_Kwd("`systemc_imp_header",       "Verilator preprocessor directive");
    جوڑو_Kwd("`systemc_implementation",   "Verilator preprocessor directive");
    جوڑو_Kwd("`systemc_interface",        "Verilator preprocessor directive");
    جوڑو_Kwd("`timescale",                "Verilog preprocessor directive");
    جوڑو_Kwd("`undef",                    "Verilog preprocessor directive");
    جوڑو_Kwd("`undefineall",              "Verilog preprocessor directive");
    جوڑو_Kwd("`verilator_config",         "Verilator preprocessor directive");
    جوڑو_Kwd("`verilog",                  "Verilator preprocessor directive");
    // clang-format on
}

#اگر_ختم  // Guard
