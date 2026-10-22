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

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব <functional>
#অন্তর্ভাব <memory>
#অন্তর্ভাব <string>

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
সার্বজনিক:
    // TYPES
    class ActionIfs;
    // Functor to register options to V3OptionParser
    class AppendHelper;
    সংরচনা Impl;

গুপ্ত:
    // MEMBERS
    ধ্রুব std::unique_ptr<Impl> m_pimpl;  // Implementation class

    // METHODS
    ActionIfs* find(ধ্রুব বর্ণ* optp) VL_MT_DISABLED;
    template <typename T_Act, typename T_Arg>
    ActionIfs& add(ধ্রুব string& opt, T_Arg arg) VL_MT_DISABLED;
    // Returns true if strp starts with "-fno"
    স্থির bool hasPrefixFNo(ধ্রুব বর্ণ* strp) VL_MT_DISABLED;
    // Returns true if strp starts with "-no"
    স্থির bool hasPrefixNo(ধ্রুব বর্ণ* strp) VL_MT_DISABLED;

সার্বজনিক:
    // METHODS
    // Returns how many args are consumed. 0 means not match.
    // Also bool, true iff option is needed for rerun with --dump-inputs
    std::pair<পূর্ণাঙ্ক, bool> parse(পূর্ণাঙ্ক idx, পূর্ণাঙ্ক argc, বর্ণ* argv[]) VL_MT_DISABLED;
    // Find the most similar option
    string getSuggestion(ধ্রুব বর্ণ* str) ধ্রুব VL_MT_DISABLED;
    শূন্য addSuggestionCandidate(ধ্রুব string& s) VL_MT_DISABLED;
    // Call this function after all options are registered.
    শূন্য finalize() VL_MT_DISABLED;
    শূন্য dumpOptions() VL_MT_DISABLED;

    // CONSTRUCTORS
    V3OptionParser() VL_MT_DISABLED;
    ~V3OptionParser() VL_MT_DISABLED;
};

class V3OptionParser::ActionIfs VL_NOT_FINAL {
সার্বজনিক:
    আভাস ~ActionIfs() = স্বভাব;
    আভাস bool is_মান_Needed() ধ্রুব = 0;  // Need val of "-opt val"
    আভাস bool isFOnOffAllowed() ধ্রুব = 0;  // true if "-fno-opt" is allowed
    আভাস bool isNotForRerun() ধ্রুব = 0;  // Will not be dumped with --dump-inputs
    আভাস bool isOnOffAllowed() ধ্রুব = 0;  // true if "-no-opt" is allowed
    আভাস bool isPartialMatchAllowed() ধ্রুব = 0;  // true if "-Wno-" matches "-Wno-fatal"
    আভাস bool isUndocumented() ধ্রুব = 0;  // Will not be suggested in typo
    // Set a value or run callback
    আভাস শূন্য exec(ধ্রুব বর্ণ* optp, ধ্রুব বর্ণ* valp) = 0;
    // Mark this option undocumented. (Exclude this option from suggestion list).
    আভাস ActionIfs& undocumented() = 0;
    // Mark this as not needed for rerunning with preprocessed sources
    আভাস ActionIfs& notForRerun() = 0;
};

// A helper class to register options
class V3OptionParser::AppendHelper final {
সার্বজনিক:
    // TYPES
    // Tag to specify which operator() to call
    সংরচনা FOnOff {};  // For ActionFOnOff
    সংরচনা OnOff {};  // For ActionOnOff
    সংরচনা Set {};  // For ActionSet

    সংরচনা CbCall {};  // For ActionCbCall
    সংরচনা CbFOnOff {};  // For ActionCbFOnOff
    সংরচনা CbOnOff {};  // For ActionCbOnOff
    সংরচনা CbPartialMatch {};  // For ActionCbPartialMatch
    সংরচনা CbPartialMatchVal {};  // For ActionCbPartialMatchVal
    সংরচনা CbVal {};  // For ActionCbVal

গুপ্ত:
    // MEMBERS
    V3OptionParser& m_parser;  // The actual option registry

সার্বজনিক:
    // METHODS
    ActionIfs& operator()(ধ্রুব বর্ণ* optp, Set, bool*) ধ্রুব VL_MT_DISABLED;
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
    ActionIfs& operator()(ধ্রুব বর্ণ* optp, Set, VOptionBool*) ধ্রুব VL_MT_DISABLED;
#endif
    ActionIfs& operator()(ধ্রুব বর্ণ* optp, Set, পূর্ণাঙ্ক*) ধ্রুব VL_MT_DISABLED;
    ActionIfs& operator()(ধ্রুব বর্ণ* optp, Set, string*) ধ্রুব VL_MT_DISABLED;

    ActionIfs& operator()(ধ্রুব বর্ণ* optp, FOnOff, bool*) ধ্রুব VL_MT_DISABLED;
    ActionIfs& operator()(ধ্রুব বর্ণ* optp, OnOff, bool*) ধ্রুব VL_MT_DISABLED;
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
    ActionIfs& operator()(ধ্রুব বর্ণ* optp, OnOff, VOptionBool*) ধ্রুব VL_MT_DISABLED;
#endif

    ActionIfs& operator()(ধ্রুব বর্ণ* optp, CbCall,
                          std::function<শূন্য(শূন্য)>) ধ্রুব VL_MT_DISABLED;
    ActionIfs& operator()(ধ্রুব বর্ণ* optp, CbFOnOff,
                          std::function<শূন্য(bool)>) ধ্রুব VL_MT_DISABLED;
    ActionIfs& operator()(ধ্রুব বর্ণ* optp, CbOnOff,
                          std::function<শূন্য(bool)>) ধ্রুব VL_MT_DISABLED;
    ActionIfs& operator()(ধ্রুব বর্ণ* optp, CbVal, std::function<শূন্য(পূর্ণাঙ্ক)>) ধ্রুব VL_MT_DISABLED;
    ActionIfs& operator()(ধ্রুব বর্ণ* optp, CbVal,
                          std::function<শূন্য(ধ্রুব বর্ণ*)>) ধ্রুব VL_MT_DISABLED;

    ActionIfs& operator()(ধ্রুব বর্ণ* optp, CbPartialMatch,
                          std::function<শূন্য(ধ্রুব বর্ণ*)>) ধ্রুব VL_MT_DISABLED;
    ActionIfs& operator()(ধ্রুব বর্ণ* optp, CbPartialMatchVal,
                          std::function<শূন্য(ধ্রুব বর্ণ*, ধ্রুব বর্ণ*)>) ধ্রুব VL_MT_DISABLED;

    // CONSTRUCTORS
    explicit AppendHelper(V3OptionParser& parser)
        : m_parser{parser} {}
};

#define V3OPTION_PARSER_DECL_TAGS \
    ধ্রুব auto Set VL_ATTR_UNUSED = V3OptionParser::AppendHelper::Set{}; \
    ধ্রুব auto FOnOff VL_ATTR_UNUSED = V3OptionParser::AppendHelper::FOnOff{}; \
    ধ্রুব auto OnOff VL_ATTR_UNUSED = V3OptionParser::AppendHelper::OnOff{}; \
    ধ্রুব auto CbCall VL_ATTR_UNUSED = V3OptionParser::AppendHelper::CbCall{}; \
    ধ্রুব auto CbFOnOff VL_ATTR_UNUSED = V3OptionParser::AppendHelper::CbFOnOff{}; \
    ধ্রুব auto CbOnOff VL_ATTR_UNUSED = V3OptionParser::AppendHelper::CbOnOff{}; \
    ধ্রুব auto CbPartialMatch VL_ATTR_UNUSED = V3OptionParser::AppendHelper::CbPartialMatch{}; \
    ধ্রুব auto CbPartialMatchVal VL_ATTR_UNUSED \
        = V3OptionParser::AppendHelper::CbPartialMatchVal{}; \
    ধ্রুব auto CbVal VL_ATTR_UNUSED = V3OptionParser::AppendHelper::CbVal{};

//######################################################################

#endif  // guard
