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

#ifndef VERILATOR_V3OPTION_PARSER_H_
#define VERILATOR_V3OPTION_PARSER_H_

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل <functional>
#شامل <memory>
#شامل <string>

// Not to include V3Option.h here so that VlcMain or other executables can use this file easily.
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
class VOptionBool;
#endif

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
عمومی:
    // TYPES
    class ActionIfs;
    // Functor to register options to V3OptionParser
    class AppendHelper;
    ساختار Impl;

خصوصی:
    // MEMBERS
    ثابت std::unique_ptr<Impl> m_pimpl;  // Implementation class

    // METHODS
    ActionIfs* find(ثابت نویسه* optp) VL_MT_DISABLED;
    template <typename T_Act, typename T_Arg>
    ActionIfs& add(ثابت string& opt, T_Arg arg) VL_MT_DISABLED;
    // Returns true if strp starts with "-fno"
    ایستا bool hasPrefixFNo(ثابت نویسه* strp) VL_MT_DISABLED;
    // Returns true if strp starts with "-no"
    ایستا bool hasPrefixNo(ثابت نویسه* strp) VL_MT_DISABLED;

عمومی:
    // METHODS
    // Returns how many args are consumed. 0 means not match.
    // Also bool, true iff option is needed for rerun with --dump-inputs
    std::pair<صحیح, bool> parse(صحیح idx, صحیح argc, نویسه* argv[]) VL_MT_DISABLED;
    // Find the most similar option
    string getSuggestion(ثابت نویسه* str) ثابت VL_MT_DISABLED;
    تهی addSuggestionCandidate(ثابت string& s) VL_MT_DISABLED;
    // Call this function after all options are registered.
    تهی finalize() VL_MT_DISABLED;
    تهی dumpOptions() VL_MT_DISABLED;

    // CONSTRUCTORS
    V3OptionParser() VL_MT_DISABLED;
    ~V3OptionParser() VL_MT_DISABLED;
};

class V3OptionParser::ActionIfs VL_NOT_FINAL {
عمومی:
    مجازی ~ActionIfs() = پیشفرض;
    مجازی bool is_مقدار_Needed() ثابت = 0;  // Need val of "-opt val"
    مجازی bool isFOnOffAllowed() ثابت = 0;  // true if "-fno-opt" is allowed
    مجازی bool isNotForRerun() ثابت = 0;  // Will not be dumped with --dump-inputs
    مجازی bool isOnOffAllowed() ثابت = 0;  // true if "-no-opt" is allowed
    مجازی bool isPartialMatchAllowed() ثابت = 0;  // true if "-Wno-" matches "-Wno-fatal"
    مجازی bool isUndocumented() ثابت = 0;  // Will not be suggested in typo
    // Set a value or run callback
    مجازی تهی exec(ثابت نویسه* optp, ثابت نویسه* valp) = 0;
    // Mark this option undocumented. (Exclude this option from suggestion list).
    مجازی ActionIfs& undocumented() = 0;
    // Mark this as not needed for rerunning with preprocessed sources
    مجازی ActionIfs& notForRerun() = 0;
};

// A helper class to register options
class V3OptionParser::AppendHelper final {
عمومی:
    // TYPES
    // Tag to specify which operator() to call
    ساختار FOnOff {};  // For ActionFOnOff
    ساختار OnOff {};  // For ActionOnOff
    ساختار Set {};  // For ActionSet

    ساختار CbCall {};  // For ActionCbCall
    ساختار CbFOnOff {};  // For ActionCbFOnOff
    ساختار CbOnOff {};  // For ActionCbOnOff
    ساختار CbPartialMatch {};  // For ActionCbPartialMatch
    ساختار CbPartialMatchVal {};  // For ActionCbPartialMatchVal
    ساختار CbVal {};  // For ActionCbVal

خصوصی:
    // MEMBERS
    V3OptionParser& m_parser;  // The actual option registry

عمومی:
    // METHODS
    ActionIfs& operator()(ثابت نویسه* optp, Set, bool*) ثابت VL_MT_DISABLED;
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
    ActionIfs& operator()(ثابت نویسه* optp, Set, VOptionBool*) ثابت VL_MT_DISABLED;
#endif
    ActionIfs& operator()(ثابت نویسه* optp, Set, صحیح*) ثابت VL_MT_DISABLED;
    ActionIfs& operator()(ثابت نویسه* optp, Set, string*) ثابت VL_MT_DISABLED;

    ActionIfs& operator()(ثابت نویسه* optp, FOnOff, bool*) ثابت VL_MT_DISABLED;
    ActionIfs& operator()(ثابت نویسه* optp, OnOff, bool*) ثابت VL_MT_DISABLED;
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
    ActionIfs& operator()(ثابت نویسه* optp, OnOff, VOptionBool*) ثابت VL_MT_DISABLED;
#endif

    ActionIfs& operator()(ثابت نویسه* optp, CbCall,
                          std::function<تهی(تهی)>) ثابت VL_MT_DISABLED;
    ActionIfs& operator()(ثابت نویسه* optp, CbFOnOff,
                          std::function<تهی(bool)>) ثابت VL_MT_DISABLED;
    ActionIfs& operator()(ثابت نویسه* optp, CbOnOff,
                          std::function<تهی(bool)>) ثابت VL_MT_DISABLED;
    ActionIfs& operator()(ثابت نویسه* optp, CbVal, std::function<تهی(صحیح)>) ثابت VL_MT_DISABLED;
    ActionIfs& operator()(ثابت نویسه* optp, CbVal,
                          std::function<تهی(ثابت نویسه*)>) ثابت VL_MT_DISABLED;

    ActionIfs& operator()(ثابت نویسه* optp, CbPartialMatch,
                          std::function<تهی(ثابت نویسه*)>) ثابت VL_MT_DISABLED;
    ActionIfs& operator()(ثابت نویسه* optp, CbPartialMatchVal,
                          std::function<تهی(ثابت نویسه*, ثابت نویسه*)>) ثابت VL_MT_DISABLED;

    // CONSTRUCTORS
    explicit AppendHelper(V3OptionParser& parser)
        : m_parser{parser} {}
};

#define V3OPTION_PARSER_DECL_TAGS \
    ثابت auto Set VL_ATTR_UNUSED = V3OptionParser::AppendHelper::Set{}; \
    ثابت auto FOnOff VL_ATTR_UNUSED = V3OptionParser::AppendHelper::FOnOff{}; \
    ثابت auto OnOff VL_ATTR_UNUSED = V3OptionParser::AppendHelper::OnOff{}; \
    ثابت auto CbCall VL_ATTR_UNUSED = V3OptionParser::AppendHelper::CbCall{}; \
    ثابت auto CbFOnOff VL_ATTR_UNUSED = V3OptionParser::AppendHelper::CbFOnOff{}; \
    ثابت auto CbOnOff VL_ATTR_UNUSED = V3OptionParser::AppendHelper::CbOnOff{}; \
    ثابت auto CbPartialMatch VL_ATTR_UNUSED = V3OptionParser::AppendHelper::CbPartialMatch{}; \
    ثابت auto CbPartialMatchVal VL_ATTR_UNUSED \
        = V3OptionParser::AppendHelper::CbPartialMatchVal{}; \
    ثابت auto CbVal VL_ATTR_UNUSED = V3OptionParser::AppendHelper::CbVal{};

//######################################################################

#endif  // guard
