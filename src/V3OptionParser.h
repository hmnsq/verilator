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

#اگر_غیر_تعریف_شدہ VERILATOR_V3OPTION_PARSER_H_
#تعریف_کرو VERILATOR_V3OPTION_PARSER_H_

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل <functional>
#شامل <memory>
#شامل <string>

// Not to include V3Option.h here so that VlcMain or other executables can use this file easily.
#اگر_غیر_تعریف_شدہ V3OPTION_PARSER_NO_VOPTION_BOOL
class VOptionBool;
#اگر_ختم

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
عوامی:
    // TYPES
    class ActionIfs;
    // Functor to register options to V3OptionParser
    class AppendHelper;
    ساخت Impl;

نجی:
    // MEMBERS
    ثابت std::unique_ptr<Impl> m_pimpl;  // Implementation class

    // METHODS
    ActionIfs* ڈھونڈو(ثابت حرف* optp) VL_MT_DISABLED;
    سانچہ <typename T_Act, typename T_Arg>
    ActionIfs& جوڑو(ثابت string& opt, T_Arg arg) VL_MT_DISABLED;
    // Returns true if strp starts with "-fno"
    ساکن bool has_سابقہ_FNo(ثابت حرف* strp) VL_MT_DISABLED;
    // Returns true if strp starts with "-no"
    ساکن bool has_سابقہ_No(ثابت حرف* strp) VL_MT_DISABLED;

عوامی:
    // METHODS
    // Returns how many args are consumed. 0 means not match.
    // Also bool, true iff option is needed for rerun with --dump-inputs
    std::pair<صحیح, bool> parse(صحیح idx, صحیح argc, حرف* argv[]) VL_MT_DISABLED;
    // Find the most similar option
    string getSuggestion(ثابت حرف* str) ثابت VL_MT_DISABLED;
    خالی جوڑو_Suggestion_Candidate(ثابت string& s) VL_MT_DISABLED;
    // Call this function after all options are registered.
    خالی finalize() VL_MT_DISABLED;
    خالی dumpOptions() VL_MT_DISABLED;

    // CONSTRUCTORS
    V3OptionParser() VL_MT_DISABLED;
    ~V3OptionParser() VL_MT_DISABLED;
};

class V3OptionParser::ActionIfs VL_NOT_FINAL {
عوامی:
    مجازی ~ActionIfs() = طےشدہ;
    مجازی bool is_قدر_Needed() ثابت = 0;  // Need val of "-opt val"
    مجازی bool isFOnOffAllowed() ثابت = 0;  // true if "-fno-opt" is allowed
    مجازی bool isNotForRerun() ثابت = 0;  // Will not be dumped with --dump-inputs
    مجازی bool isOnOffAllowed() ثابت = 0;  // true if "-no-opt" is allowed
    مجازی bool is_Partial_مطابقت_Allowed() ثابت = 0;  // true if "-Wno-" matches "-Wno-fatal"
    مجازی bool isUndocumented() ثابت = 0;  // Will not be suggested in typo
    // Set a value or run callback
    مجازی خالی exec(ثابت حرف* optp, ثابت حرف* valp) = 0;
    // Mark this option undocumented. (Exclude this option from suggestion list).
    مجازی ActionIfs& undocumented() = 0;
    // Mark this as not needed for rerunning with preprocessed sources
    مجازی ActionIfs& notForRerun() = 0;
};

// A helper class to register options
class V3OptionParser::AppendHelper final {
عوامی:
    // TYPES
    // Tag to specify which operator() to call
    ساخت FOnOff {};  // For ActionFOnOff
    ساخت OnOff {};  // For ActionOnOff
    ساخت سیٹ_2 {};  // For ActionSet

    ساخت CbCall {};  // For ActionCbCall
    ساخت CbFOnOff {};  // For ActionCbFOnOff
    ساخت CbOnOff {};  // For ActionCbOnOff
    ساخت Cb_Partial_مطابقت {};  // For ActionCbPartialMatch
    ساخت Cb_Partial_مطابقت_Val {};  // For ActionCbPartialMatchVal
    ساخت CbVal {};  // For ActionCbVal

نجی:
    // MEMBERS
    V3OptionParser& m_parser;  // The actual option registry

عوامی:
    // METHODS
    ActionIfs& عامل()(ثابت حرف* optp, سیٹ_2, bool*) ثابت VL_MT_DISABLED;
#اگر_غیر_تعریف_شدہ V3OPTION_PARSER_NO_VOPTION_BOOL
    ActionIfs& عامل()(ثابت حرف* optp, سیٹ_2, VOptionBool*) ثابت VL_MT_DISABLED;
#اگر_ختم
    ActionIfs& عامل()(ثابت حرف* optp, سیٹ_2, صحیح*) ثابت VL_MT_DISABLED;
    ActionIfs& عامل()(ثابت حرف* optp, سیٹ_2, string*) ثابت VL_MT_DISABLED;

    ActionIfs& عامل()(ثابت حرف* optp, FOnOff, bool*) ثابت VL_MT_DISABLED;
    ActionIfs& عامل()(ثابت حرف* optp, OnOff, bool*) ثابت VL_MT_DISABLED;
#اگر_غیر_تعریف_شدہ V3OPTION_PARSER_NO_VOPTION_BOOL
    ActionIfs& عامل()(ثابت حرف* optp, OnOff, VOptionBool*) ثابت VL_MT_DISABLED;
#اگر_ختم

    ActionIfs& عامل()(ثابت حرف* optp, CbCall,
                          std::function<خالی(خالی)>) ثابت VL_MT_DISABLED;
    ActionIfs& عامل()(ثابت حرف* optp, CbFOnOff,
                          std::function<خالی(bool)>) ثابت VL_MT_DISABLED;
    ActionIfs& عامل()(ثابت حرف* optp, CbOnOff,
                          std::function<خالی(bool)>) ثابت VL_MT_DISABLED;
    ActionIfs& عامل()(ثابت حرف* optp, CbVal, std::function<خالی(صحیح)>) ثابت VL_MT_DISABLED;
    ActionIfs& عامل()(ثابت حرف* optp, CbVal,
                          std::function<خالی(ثابت حرف*)>) ثابت VL_MT_DISABLED;

    ActionIfs& عامل()(ثابت حرف* optp, Cb_Partial_مطابقت,
                          std::function<خالی(ثابت حرف*)>) ثابت VL_MT_DISABLED;
    ActionIfs& عامل()(ثابت حرف* optp, Cb_Partial_مطابقت_Val,
                          std::function<خالی(ثابت حرف*, ثابت حرف*)>) ثابت VL_MT_DISABLED;

    // CONSTRUCTORS
    explicit AppendHelper(V3OptionParser& parser)
        : m_parser{parser} {}
};

#تعریف_کرو V3_OPTION_PARSER_DECL_لیبل_فہرست \
    ثابت خودکار سیٹ_2 VL_ATTR_غیر_مستعمل = V3OptionParser::AppendHelper::سیٹ_2{}; \
    ثابت خودکار FOnOff VL_ATTR_غیر_مستعمل = V3OptionParser::AppendHelper::FOnOff{}; \
    ثابت خودکار OnOff VL_ATTR_غیر_مستعمل = V3OptionParser::AppendHelper::OnOff{}; \
    ثابت خودکار CbCall VL_ATTR_غیر_مستعمل = V3OptionParser::AppendHelper::CbCall{}; \
    ثابت خودکار CbFOnOff VL_ATTR_غیر_مستعمل = V3OptionParser::AppendHelper::CbFOnOff{}; \
    ثابت خودکار CbOnOff VL_ATTR_غیر_مستعمل = V3OptionParser::AppendHelper::CbOnOff{}; \
    ثابت خودکار Cb_Partial_مطابقت VL_ATTR_غیر_مستعمل = V3OptionParser::AppendHelper::Cb_Partial_مطابقت{}; \
    ثابت خودکار Cb_Partial_مطابقت_Val VL_ATTR_غیر_مستعمل \
        = V3OptionParser::AppendHelper::Cb_Partial_مطابقت_Val{}; \
    ثابت خودکار CbVal VL_ATTR_غیر_مستعمل = V3OptionParser::AppendHelper::CbVal{};

//######################################################################

#اگر_ختم  // guard
