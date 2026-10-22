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
সর্বজনীন:
    // TYPES
    class ActionIfs;
    // Functor to register options to V3OptionParser
    class AppendHelper;
    কাঠামো Impl;

ব্যক্তিগত:
    // MEMBERS
    ধ্রুবক std::unique_ptr<Impl> m_pimpl;  // Implementation class

    // METHODS
    ActionIfs* find(ধ্রুবক অক্ষর* optp) VL_MT_DISABLED;
    template <typename T_Act, typename T_Arg>
    ActionIfs& add(ধ্রুবক string& opt, T_Arg arg) VL_MT_DISABLED;
    // Returns true if strp starts with "-fno"
    স্থির bool hasPrefixFNo(ধ্রুবক অক্ষর* strp) VL_MT_DISABLED;
    // Returns true if strp starts with "-no"
    স্থির bool hasPrefixNo(ধ্রুবক অক্ষর* strp) VL_MT_DISABLED;

সর্বজনীন:
    // METHODS
    // Returns how many args are consumed. 0 means not match.
    // Also bool, true iff option is needed for rerun with --dump-inputs
    std::pair<পূর্ণসংখ্যা, bool> parse(পূর্ণসংখ্যা idx, পূর্ণসংখ্যা argc, অক্ষর* argv[]) VL_MT_DISABLED;
    // Find the most similar option
    string getSuggestion(ধ্রুবক অক্ষর* str) ধ্রুবক VL_MT_DISABLED;
    শূন্য addSuggestionCandidate(ধ্রুবক string& s) VL_MT_DISABLED;
    // Call this function after all options are registered.
    শূন্য finalize() VL_MT_DISABLED;
    শূন্য dumpOptions() VL_MT_DISABLED;

    // CONSTRUCTORS
    V3OptionParser() VL_MT_DISABLED;
    ~V3OptionParser() VL_MT_DISABLED;
};

class V3OptionParser::ActionIfs VL_NOT_FINAL {
সর্বজনীন:
    আভাসী ~ActionIfs() = পূর্বনির্ধারিত;
    আভাসী bool is_মান_Needed() ধ্রুবক = 0;  // Need val of "-opt val"
    আভাসী bool isFOnOffAllowed() ধ্রুবক = 0;  // true if "-fno-opt" is allowed
    আভাসী bool isNotForRerun() ধ্রুবক = 0;  // Will not be dumped with --dump-inputs
    আভাসী bool isOnOffAllowed() ধ্রুবক = 0;  // true if "-no-opt" is allowed
    আভাসী bool isPartialMatchAllowed() ধ্রুবক = 0;  // true if "-Wno-" matches "-Wno-fatal"
    আভাসী bool isUndocumented() ধ্রুবক = 0;  // Will not be suggested in typo
    // Set a value or run callback
    আভাসী শূন্য exec(ধ্রুবক অক্ষর* optp, ধ্রুবক অক্ষর* valp) = 0;
    // Mark this option undocumented. (Exclude this option from suggestion list).
    আভাসী ActionIfs& undocumented() = 0;
    // Mark this as not needed for rerunning with preprocessed sources
    আভাসী ActionIfs& notForRerun() = 0;
};

// A helper class to register options
class V3OptionParser::AppendHelper final {
সর্বজনীন:
    // TYPES
    // Tag to specify which operator() to call
    কাঠামো FOnOff {};  // For ActionFOnOff
    কাঠামো OnOff {};  // For ActionOnOff
    কাঠামো Set {};  // For ActionSet

    কাঠামো CbCall {};  // For ActionCbCall
    কাঠামো CbFOnOff {};  // For ActionCbFOnOff
    কাঠামো CbOnOff {};  // For ActionCbOnOff
    কাঠামো CbPartialMatch {};  // For ActionCbPartialMatch
    কাঠামো CbPartialMatchVal {};  // For ActionCbPartialMatchVal
    কাঠামো CbVal {};  // For ActionCbVal

ব্যক্তিগত:
    // MEMBERS
    V3OptionParser& m_parser;  // The actual option registry

সর্বজনীন:
    // METHODS
    ActionIfs& operator()(ধ্রুবক অক্ষর* optp, Set, bool*) ধ্রুবক VL_MT_DISABLED;
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
    ActionIfs& operator()(ধ্রুবক অক্ষর* optp, Set, VOptionBool*) ধ্রুবক VL_MT_DISABLED;
#endif
    ActionIfs& operator()(ধ্রুবক অক্ষর* optp, Set, পূর্ণসংখ্যা*) ধ্রুবক VL_MT_DISABLED;
    ActionIfs& operator()(ধ্রুবক অক্ষর* optp, Set, string*) ধ্রুবক VL_MT_DISABLED;

    ActionIfs& operator()(ধ্রুবক অক্ষর* optp, FOnOff, bool*) ধ্রুবক VL_MT_DISABLED;
    ActionIfs& operator()(ধ্রুবক অক্ষর* optp, OnOff, bool*) ধ্রুবক VL_MT_DISABLED;
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
    ActionIfs& operator()(ধ্রুবক অক্ষর* optp, OnOff, VOptionBool*) ধ্রুবক VL_MT_DISABLED;
#endif

    ActionIfs& operator()(ধ্রুবক অক্ষর* optp, CbCall,
                          std::function<শূন্য(শূন্য)>) ধ্রুবক VL_MT_DISABLED;
    ActionIfs& operator()(ধ্রুবক অক্ষর* optp, CbFOnOff,
                          std::function<শূন্য(bool)>) ধ্রুবক VL_MT_DISABLED;
    ActionIfs& operator()(ধ্রুবক অক্ষর* optp, CbOnOff,
                          std::function<শূন্য(bool)>) ধ্রুবক VL_MT_DISABLED;
    ActionIfs& operator()(ধ্রুবক অক্ষর* optp, CbVal, std::function<শূন্য(পূর্ণসংখ্যা)>) ধ্রুবক VL_MT_DISABLED;
    ActionIfs& operator()(ধ্রুবক অক্ষর* optp, CbVal,
                          std::function<শূন্য(ধ্রুবক অক্ষর*)>) ধ্রুবক VL_MT_DISABLED;

    ActionIfs& operator()(ধ্রুবক অক্ষর* optp, CbPartialMatch,
                          std::function<শূন্য(ধ্রুবক অক্ষর*)>) ধ্রুবক VL_MT_DISABLED;
    ActionIfs& operator()(ধ্রুবক অক্ষর* optp, CbPartialMatchVal,
                          std::function<শূন্য(ধ্রুবক অক্ষর*, ধ্রুবক অক্ষর*)>) ধ্রুবক VL_MT_DISABLED;

    // CONSTRUCTORS
    explicit AppendHelper(V3OptionParser& parser)
        : m_parser{parser} {}
};

#define V3OPTION_PARSER_DECL_TAGS \
    ধ্রুবক auto Set VL_ATTR_UNUSED = V3OptionParser::AppendHelper::Set{}; \
    ধ্রুবক auto FOnOff VL_ATTR_UNUSED = V3OptionParser::AppendHelper::FOnOff{}; \
    ধ্রুবক auto OnOff VL_ATTR_UNUSED = V3OptionParser::AppendHelper::OnOff{}; \
    ধ্রুবক auto CbCall VL_ATTR_UNUSED = V3OptionParser::AppendHelper::CbCall{}; \
    ধ্রুবক auto CbFOnOff VL_ATTR_UNUSED = V3OptionParser::AppendHelper::CbFOnOff{}; \
    ধ্রুবক auto CbOnOff VL_ATTR_UNUSED = V3OptionParser::AppendHelper::CbOnOff{}; \
    ধ্রুবক auto CbPartialMatch VL_ATTR_UNUSED = V3OptionParser::AppendHelper::CbPartialMatch{}; \
    ধ্রুবক auto CbPartialMatchVal VL_ATTR_UNUSED \
        = V3OptionParser::AppendHelper::CbPartialMatchVal{}; \
    ধ্রুবক auto CbVal VL_ATTR_UNUSED = V3OptionParser::AppendHelper::CbVal{};

//######################################################################

#endif  // guard
