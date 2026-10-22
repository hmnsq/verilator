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

#اگر_تعریف_نشده VERILATOR_V3LANGUAGEWORDS_H_
#تعریف_کن VERILATOR_V3LANGUAGEWORDS_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن <map>

//============================================================================

class V3_Language_واژگان final {
    // List of common reserved keywords
خاص:
    using KeywordMap = std::map<ثابت string, std::string>;
    بنية Singleton final {
        KeywordMap s_kwdMap;  // List of keywords, and what language applies
        Singleton() { init(); }
        فراغ جمع_کن_Kwd(ثابت string& kwd, ثابت string& why) { s_kwdMap.emplace(kwd, why); }
        فراغ init();
    };

عام:
    using const_iterator = KeywordMap::const_iterator;
    // METHODS
    ساكن const_iterator begin() { أرجع s().s_kwdMap.begin(); }
    ساكن const_iterator انتها() { أرجع s().s_kwdMap.انتها(); }
    ساكن string isKeyword(ثابت string& kwd) {
        ثابت خودکار it = vlstd::as_const(s().s_kwdMap).پیدا_کن(kwd);
        إذا (it == s().s_kwdMap.انتها()) أرجع "";
        أرجع it->دومی;
    }

خاص:
    ساكن Singleton& s() {
        ساكن Singleton s_s;  // LCOV_EXCL_BR_LINE
        أرجع s_s;
    }
};

مضمن فراغ V3_Language_واژگان::Singleton::init() {
    // C++ keywords
    // clang-format off
    جمع_کن_Kwd("abort",                 "C++ common word");
    جمع_کن_Kwd("alignas",               "C++11 keyword");
    جمع_کن_Kwd("alignof",               "C++11 keyword");
    جمع_کن_Kwd("and",                   "C++11 keyword");
    جمع_کن_Kwd("and_eq",                "C++11 keyword");
    جمع_کن_Kwd("asm",                   "C++ common word");
    جمع_کن_Kwd("atomic_cancel",         "C++ TM TS keyword");
    جمع_کن_Kwd("atomic_commit",         "C++ TM TS keyword");
    جمع_کن_Kwd("atomic_noexcept",       "C++ TM TS keyword");
    جمع_کن_Kwd("auto",                  "C++ keyword");
    جمع_کن_Kwd("bit_vector",            "C++ common word");
    جمع_کن_Kwd("bitand",                "C++ keyword");
    جمع_کن_Kwd("bitor",                 "C++ keyword");
    جمع_کن_Kwd("bool",                  "C++ keyword");
    جمع_کن_Kwd("break",                 "C++ keyword");
    جمع_کن_Kwd("case",                  "C++ keyword");
    جمع_کن_Kwd("catch",                 "C++ keyword");
    جمع_کن_Kwd("cdecl",                 "C++ common word");
    جمع_کن_Kwd("char",                  "C++ keyword");
    جمع_کن_Kwd("char16_t",              "C++11 keyword");
    جمع_کن_Kwd("char32_t",              "C++11 keyword");
    جمع_کن_Kwd("class",                 "C++11 keyword");
    جمع_کن_Kwd("compl",                 "C++11 keyword");
    جمع_کن_Kwd("complex",               "C++ common word");
    جمع_کن_Kwd("concept",               "C++20 keyword");
    جمع_کن_Kwd("const",                 "C++ keyword");
    جمع_کن_Kwd("const_cast",            "C++ common word");
    جمع_کن_Kwd("const_iterator",        "C++ common word");
    جمع_کن_Kwd("const_reference ",      "C++ common word");
    جمع_کن_Kwd("constexpr",             "C++11 keyword");
    جمع_کن_Kwd("continue",              "C++ keyword");
    جمع_کن_Kwd("decltype",              "C++11 keyword");
    جمع_کن_Kwd("default",               "C++ keyword");
    جمع_کن_Kwd("delete",                "C++ keyword");
    جمع_کن_Kwd("do",                    "C++ keyword");
    جمع_کن_Kwd("double",                "C++ keyword");
    جمع_کن_Kwd("dynamic_cast",          "C++ keyword");
    جمع_کن_Kwd("else",                  "C++ keyword");
    جمع_کن_Kwd("enum",                  "C++ keyword");
    جمع_کن_Kwd("explicit",              "C++ keyword");
    جمع_کن_Kwd("export",                "C++ keyword");
    جمع_کن_Kwd("extern",                "C++ keyword");
    جمع_کن_Kwd("false",                 "C++ keyword");
    جمع_کن_Kwd("far",                   "C++ common word");
    جمع_کن_Kwd("float",                 "C++ keyword");
    جمع_کن_Kwd("for",                   "C++ keyword");
    جمع_کن_Kwd("friend",                "C++ keyword");
    جمع_کن_Kwd("goto",                  "C++ keyword");
    جمع_کن_Kwd("huge",                  "C++ keyword");
    جمع_کن_Kwd("if",                    "C++ keyword");
    جمع_کن_Kwd("import",                "C++ modules TS keyword");
    جمع_کن_Kwd("inline",                "C++ keyword");
    جمع_کن_Kwd("int",                   "C++ keyword");
    جمع_کن_Kwd("interrupt",             "C++ common word");
    جمع_کن_Kwd("iterator",              "C++ common word");
    جمع_کن_Kwd("long",                  "C++ keyword");
    جمع_کن_Kwd("module",                "C++ modules TS keyword");
    جمع_کن_Kwd("mutable",               "C++ keyword");
    جمع_کن_Kwd("namespace",             "C++ keyword");
    جمع_کن_Kwd("near",                  "C++ common word");
    جمع_کن_Kwd("new",                   "C++ keyword");
    جمع_کن_Kwd("noexcept",              "C++11 keyword");
    جمع_کن_Kwd("not",                   "C++ keyword");
    جمع_کن_Kwd("not_eq",                "C++ keyword");
    جمع_کن_Kwd("nullptr",               "C++ common word");
    جمع_کن_Kwd("nullptr",               "C++11 keyword");
    جمع_کن_Kwd("operator",              "C++ keyword");
    جمع_کن_Kwd("or",                    "C++ keyword");
    جمع_کن_Kwd("or_eq",                 "C++ keyword");
    جمع_کن_Kwd("override",              "C++ common word");
    جمع_کن_Kwd("pascal",                "C++ keyword");
    جمع_کن_Kwd("private",               "C++ keyword");
    جمع_کن_Kwd("protected",             "C++ keyword");
    جمع_کن_Kwd("public",                "C++ keyword");
    جمع_کن_Kwd("queue",                 "C++ common word");
    جمع_کن_Kwd("reference",             "C++ common word");
    جمع_کن_Kwd("register",              "C++ keyword");
    جمع_کن_Kwd("reinterpret_cast ",     "C++ keyword");
    جمع_کن_Kwd("requires",              "C++20 keyword");
    جمع_کن_Kwd("restrict",              "C++ keyword");
    جمع_کن_Kwd("return",                "C++ keyword");
    جمع_کن_Kwd("short",                 "C++ keyword");
    جمع_کن_Kwd("signed",                "C++ keyword");
    جمع_کن_Kwd("sizeof",                "C++ keyword");
    جمع_کن_Kwd("stack",                 "C++ common word");
    جمع_کن_Kwd("static",                "C++ keyword");
    جمع_کن_Kwd("static_assert",         "C++11 keyword");
    جمع_کن_Kwd("static_cast",           "C++ keyword");
    جمع_کن_Kwd("std::deque",            "C++ common word");
    جمع_کن_Kwd("std::list",             "C++ common word");
    جمع_کن_Kwd("std::map",              "C++ common word");
    جمع_کن_Kwd("std::multimap",         "C++ common word");
    جمع_کن_Kwd("std::multiset",         "C++ common word");
    جمع_کن_Kwd("std::set",              "C++ common word");
    جمع_کن_Kwd("std::vector",           "C++ common word");
    جمع_کن_Kwd("struct",                "C++ keyword");
    جمع_کن_Kwd("switch",                "C++ keyword");
    جمع_کن_Kwd("synchronized",          "C++ TM TS keyword");
    جمع_کن_Kwd("template",              "C++ keyword");
    جمع_کن_Kwd("this",                  "C++ keyword");
    جمع_کن_Kwd("thread_local",          "C++11 keyword");
    جمع_کن_Kwd("throw",                 "C++ keyword");
    جمع_کن_Kwd("transaction_safe",      "C++ common word");
    جمع_کن_Kwd("transaction_safe_dynamic",      "C++ common word");
    جمع_کن_Kwd("true",                  "C++ keyword");
    جمع_کن_Kwd("try",                   "C++ keyword");
    جمع_کن_Kwd("type_info",             "C++ common word");
    جمع_کن_Kwd("typedef",               "C++ keyword");
    جمع_کن_Kwd("typeid",                "C++ keyword");
    جمع_کن_Kwd("typename",              "C++ keyword");
    جمع_کن_Kwd("uint16_t",              "C++ common word");
    جمع_کن_Kwd("uint32_t",              "C++ common word");
    جمع_کن_Kwd("uint8_t",               "C++ common word");
    جمع_کن_Kwd("union",                 "C++ keyword");
    جمع_کن_Kwd("unsigned",              "C++ keyword");
    جمع_کن_Kwd("using",                 "C++ keyword");
    جمع_کن_Kwd("virtual",               "C++ keyword");
    جمع_کن_Kwd("void",                  "C++ keyword");
    جمع_کن_Kwd("volatile",              "C++ keyword");
    جمع_کن_Kwd("wchar_t",               "C++ keyword");
    جمع_کن_Kwd("while",                 "C++ keyword");
    جمع_کن_Kwd("xor",                   "C++ keyword");
    جمع_کن_Kwd("xor_eq",                "C++ keyword");
    // This conflicts with header functions, so is ignored
    //dKwd("final",                 "C++11 keyword");
    // SystemC
    جمع_کن_Kwd("sc_clock",              "SystemC common word");
    جمع_کن_Kwd("sc_in",                 "SystemC common word");
    جمع_کن_Kwd("sc_inout",              "SystemC common word");
    جمع_کن_Kwd("sc_out",                "SystemC common word");
    جمع_کن_Kwd("sc_signal",             "SystemC common word");
    جمع_کن_Kwd("sensitive",             "SystemC common word");
    جمع_کن_Kwd("sensitive_neg",         "SystemC common word");
    جمع_کن_Kwd("sensitive_pos",         "SystemC common word");
    // Preprocessor defined words
    جمع_کن_Kwd("`__FILE__",                 "Verilog preprocessor directive");
    جمع_کن_Kwd("`__LINE__",                 "Verilog preprocessor directive");
    جمع_کن_Kwd("`accelerate",               "Verilog-XL preprocessor directive");
    جمع_کن_Kwd("`autoexpand_vectornets",    "Verilog-XL preprocessor directive");
    جمع_کن_Kwd("`begin_keywords",           "Verilog preprocessor directive");
    جمع_کن_Kwd("`celldefine",               "Verilog preprocessor directive");
    جمع_کن_Kwd("`default_decay_time",       "Verilog preprocessor directive");
    جمع_کن_Kwd("`default_nettype",          "Verilog preprocessor directive");
    جمع_کن_Kwd("`default_trireg_strength",  "Verilog preprocessor directive");
    جمع_کن_Kwd("`define",                   "Verilog preprocessor directive");
    جمع_کن_Kwd("`delay_mode_distributed",   "Verilog preprocessor directive");
    جمع_کن_Kwd("`delay_mode_path",          "Verilog preprocessor directive");
    جمع_کن_Kwd("`delay_mode_unit",          "Verilog preprocessor directive");
    جمع_کن_Kwd("`delay_mode_zero",          "Verilog preprocessor directive");
    جمع_کن_Kwd("`disable_portfaults",       "Verilog-XL preprocessor directive");
    جمع_کن_Kwd("`else",                     "Verilog preprocessor directive");
    جمع_کن_Kwd("`elsif",                    "Verilog preprocessor directive");
    جمع_کن_Kwd("`enable_portfaults",        "Verilog-XL preprocessor directive");
    جمع_کن_Kwd("`end_keywords",             "Verilog preprocessor directive");
    جمع_کن_Kwd("`endcelldefine",            "Verilog preprocessor directive");
    جمع_کن_Kwd("`endif",                    "Verilog preprocessor directive");
    جمع_کن_Kwd("`endprotect",               "Verilog preprocessor directive");
    جمع_کن_Kwd("`endprotected",             "Verilog preprocessor directive");
    جمع_کن_Kwd("`error",                    "Verilog preprocessor directive");
    جمع_کن_Kwd("`expand_vectornets",        "Verilog-XL preprocessor directive");
    جمع_کن_Kwd("`ifdef",                    "Verilog preprocessor directive");
    جمع_کن_Kwd("`ifndef",                   "Verilog preprocessor directive");
    جمع_کن_Kwd("`include",                  "Verilog preprocessor directive");
    جمع_کن_Kwd("`inline",                   "Verilog preprocessor directive");
    جمع_کن_Kwd("`line",                     "Verilog preprocessor directive");
    جمع_کن_Kwd("`noaccelerate",             "Verilog-XL preprocessor directive");
    جمع_کن_Kwd("`noexpand_vectornets",      "Verilog-XL preprocessor directive");
    جمع_کن_Kwd("`noremove_gatenames",       "Verilog-XL preprocessor directive");
    جمع_کن_Kwd("`noremove_netnames",        "Verilog-XL preprocessor directive");
    جمع_کن_Kwd("`nosuppress_faults",        "Verilog-XL preprocessor directive");
    جمع_کن_Kwd("`nounconnected_drive",      "Verilog-XL preprocessor directive");
    جمع_کن_Kwd("`portcoerce",               "Verilog preprocessor directive");
    جمع_کن_Kwd("`pragma",                   "Verilog preprocessor directive");
    جمع_کن_Kwd("`protect",                  "Verilog preprocessor directive");
    جمع_کن_Kwd("`protected",                "Verilog preprocessor directive");
    جمع_کن_Kwd("`remove_gatenames",         "Verilog-XL preprocessor directive");
    جمع_کن_Kwd("`remove_netnames",          "Verilog-XL preprocessor directive");
    جمع_کن_Kwd("`resetall",                 "Verilog preprocessor directive");
    جمع_کن_Kwd("`suppress_faults",          "Verilog-XL preprocessor directive");
    جمع_کن_Kwd("`systemc_ctor",             "Verilator preprocessor directive");
    جمع_کن_Kwd("`systemc_dtor",             "Verilator preprocessor directive");
    جمع_کن_Kwd("`systemc_header",           "Verilator preprocessor directive");
    جمع_کن_Kwd("`systemc_header_post",      "Verilator preprocessor directive");
    جمع_کن_Kwd("`systemc_imp_header",       "Verilator preprocessor directive");
    جمع_کن_Kwd("`systemc_implementation",   "Verilator preprocessor directive");
    جمع_کن_Kwd("`systemc_interface",        "Verilator preprocessor directive");
    جمع_کن_Kwd("`timescale",                "Verilog preprocessor directive");
    جمع_کن_Kwd("`undef",                    "Verilog preprocessor directive");
    جمع_کن_Kwd("`undefineall",              "Verilog preprocessor directive");
    جمع_کن_Kwd("`verilator_config",         "Verilator preprocessor directive");
    جمع_کن_Kwd("`verilog",                  "Verilator preprocessor directive");
    // clang-format on
}

#پایان_اگر  // Guard
