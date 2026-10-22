// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Command line option parser
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2003-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************

#اگر_تعریف_نشده VERILATOR_V3OPTION_PARSER_H_
#تعریف_کن VERILATOR_V3OPTION_PARSER_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن <functional>
#ضمن <memory>
#ضمن <string>

// Not to include V3Option.h here so that VlcMain or other executables can use this file easily.
#اگر_تعریف_نشده V3OPTION_PARSER_NO_VOPTION_BOOL
class VOptionBool;
#پایان_اگر

// Typical usage would look as below.
// See also V3Options::parseOptsList() in V3Options.cpp for more detailed usage.
//
//    V3OptionParser parser;
//    V3OptionParser::AppendHelper DECL_OPTION{parser};
//    V3OPTION_PARSER_DECL_TAGS;
//
//    DECL_OPPTION("-help", CbCall, []() { showHelp(); });
//    DECL_OPPTION("-some_opt", Set, &m_intMember});
//    parser.finalize();
//    for (int i = 1; i < argc;) {
//        if (int consumed = parser.parse(i, argc, argv)) {
//            i += consumed;
//        } else {  // error
//            std::cerr << parser.getSuggestion(argv[i]) << std::endl;
//        }
//    }
//

//######################################################################
// V3 OptionParser

class V3OptionParser final {
عام:
    // TYPES
    class ActionIfs;
    // Functor to register options to V3OptionParser
    class AppendHelper;
    بنية Impl;

خاص:
    // MEMBERS
    ثابت std::unique_ptr<Impl> m_pimpl;  // Implementation class

    // METHODS
    ActionIfs* پیدا_کن(ثابت حرف* optp) VL_MT_DISABLED;
    قالب <typename T_Act, typename T_Arg>
    ActionIfs& جمع_کن(ثابت string& opt, T_Arg arg) VL_MT_DISABLED;
    // Returns true if strp starts with "-fno"
    ساكن bool has_پیشوند_FNo(ثابت حرف* strp) VL_MT_DISABLED;
    // Returns true if strp starts with "-no"
    ساكن bool has_پیشوند_No(ثابت حرف* strp) VL_MT_DISABLED;

عام:
    // METHODS
    // Returns how many args are consumed. 0 means not match.
    // Also bool, true iff option is needed for rerun with --dump-inputs
    std::pair<صحيح, bool> parse(صحيح idx, صحيح argc, حرف* argv[]) VL_MT_DISABLED;
    // Find the most similar option
    string getSuggestion(ثابت حرف* str) ثابت VL_MT_DISABLED;
    فراغ جمع_کن_Suggestion_Candidate(ثابت string& s) VL_MT_DISABLED;
    // Call this function after all options are registered.
    فراغ finalize() VL_MT_DISABLED;
    فراغ dumpOptions() VL_MT_DISABLED;

    // CONSTRUCTORS
    V3OptionParser() VL_MT_DISABLED;
    ~V3OptionParser() VL_MT_DISABLED;
};

class V3OptionParser::ActionIfs VL_NOT_FINAL {
عام:
    مجازی ~ActionIfs() = افتراضي;
    مجازی bool is_مقدار_Needed() ثابت = 0;  // Need val of "-opt val"
    مجازی bool isFOnOffAllowed() ثابت = 0;  // true if "-fno-opt" is allowed
    مجازی bool isNotForRerun() ثابت = 0;  // Will not be dumped with --dump-inputs
    مجازی bool isOnOffAllowed() ثابت = 0;  // true if "-no-opt" is allowed
    مجازی bool is_Partial_تطابق_Allowed() ثابت = 0;  // true if "-Wno-" matches "-Wno-fatal"
    مجازی bool isUndocumented() ثابت = 0;  // Will not be suggested in typo
    // Set a value or run callback
    مجازی فراغ exec(ثابت حرف* optp, ثابت حرف* valp) = 0;
    // Mark this option undocumented. (Exclude this option from suggestion list).
    مجازی ActionIfs& undocumented() = 0;
    // Mark this as not needed for rerunning with preprocessed sources
    مجازی ActionIfs& notForRerun() = 0;
};

// A helper class to register options
class V3OptionParser::AppendHelper final {
عام:
    // TYPES
    // Tag to specify which operator() to call
    بنية FOnOff {};  // For ActionFOnOff
    بنية OnOff {};  // For ActionOnOff
    بنية مجموعه_2 {};  // For ActionSet

    بنية CbCall {};  // For ActionCbCall
    بنية CbFOnOff {};  // For ActionCbFOnOff
    بنية CbOnOff {};  // For ActionCbOnOff
    بنية Cb_Partial_تطابق {};  // For ActionCbPartialMatch
    بنية Cb_Partial_تطابق_Val {};  // For ActionCbPartialMatchVal
    بنية CbVal {};  // For ActionCbVal

خاص:
    // MEMBERS
    V3OptionParser& m_parser;  // The actual option registry

عام:
    // METHODS
    ActionIfs& عملگر()(ثابت حرف* optp, مجموعه_2, bool*) ثابت VL_MT_DISABLED;
#اگر_تعریف_نشده V3OPTION_PARSER_NO_VOPTION_BOOL
    ActionIfs& عملگر()(ثابت حرف* optp, مجموعه_2, VOptionBool*) ثابت VL_MT_DISABLED;
#پایان_اگر
    ActionIfs& عملگر()(ثابت حرف* optp, مجموعه_2, صحيح*) ثابت VL_MT_DISABLED;
    ActionIfs& عملگر()(ثابت حرف* optp, مجموعه_2, string*) ثابت VL_MT_DISABLED;

    ActionIfs& عملگر()(ثابت حرف* optp, FOnOff, bool*) ثابت VL_MT_DISABLED;
    ActionIfs& عملگر()(ثابت حرف* optp, OnOff, bool*) ثابت VL_MT_DISABLED;
#اگر_تعریف_نشده V3OPTION_PARSER_NO_VOPTION_BOOL
    ActionIfs& عملگر()(ثابت حرف* optp, OnOff, VOptionBool*) ثابت VL_MT_DISABLED;
#پایان_اگر

    ActionIfs& عملگر()(ثابت حرف* optp, CbCall,
                          std::function<فراغ(فراغ)>) ثابت VL_MT_DISABLED;
    ActionIfs& عملگر()(ثابت حرف* optp, CbFOnOff,
                          std::function<فراغ(bool)>) ثابت VL_MT_DISABLED;
    ActionIfs& عملگر()(ثابت حرف* optp, CbOnOff,
                          std::function<فراغ(bool)>) ثابت VL_MT_DISABLED;
    ActionIfs& عملگر()(ثابت حرف* optp, CbVal, std::function<فراغ(صحيح)>) ثابت VL_MT_DISABLED;
    ActionIfs& عملگر()(ثابت حرف* optp, CbVal,
                          std::function<فراغ(ثابت حرف*)>) ثابت VL_MT_DISABLED;

    ActionIfs& عملگر()(ثابت حرف* optp, Cb_Partial_تطابق,
                          std::function<فراغ(ثابت حرف*)>) ثابت VL_MT_DISABLED;
    ActionIfs& عملگر()(ثابت حرف* optp, Cb_Partial_تطابق_Val,
                          std::function<فراغ(ثابت حرف*, ثابت حرف*)>) ثابت VL_MT_DISABLED;

    // CONSTRUCTORS
    explicit AppendHelper(V3OptionParser& parser)
        : m_parser{parser} {}
};

#تعریف_کن V3_OPTION_PARSER_DECL_برچسبها \
    ثابت خودکار مجموعه_2 VL_ATTR_استفادهنشده = V3OptionParser::AppendHelper::مجموعه_2{}; \
    ثابت خودکار FOnOff VL_ATTR_استفادهنشده = V3OptionParser::AppendHelper::FOnOff{}; \
    ثابت خودکار OnOff VL_ATTR_استفادهنشده = V3OptionParser::AppendHelper::OnOff{}; \
    ثابت خودکار CbCall VL_ATTR_استفادهنشده = V3OptionParser::AppendHelper::CbCall{}; \
    ثابت خودکار CbFOnOff VL_ATTR_استفادهنشده = V3OptionParser::AppendHelper::CbFOnOff{}; \
    ثابت خودکار CbOnOff VL_ATTR_استفادهنشده = V3OptionParser::AppendHelper::CbOnOff{}; \
    ثابت خودکار Cb_Partial_تطابق VL_ATTR_استفادهنشده = V3OptionParser::AppendHelper::Cb_Partial_تطابق{}; \
    ثابت خودکار Cb_Partial_تطابق_Val VL_ATTR_استفادهنشده \
        = V3OptionParser::AppendHelper::Cb_Partial_تطابق_Val{}; \
    ثابت خودکار CbVal VL_ATTR_استفادهنشده = V3OptionParser::AppendHelper::CbVal{};

//######################################################################

#پایان_اگر  // guard
