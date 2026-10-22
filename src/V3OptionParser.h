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

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ <functional>
#ಅನ್ತರ್ಭಾವ <memory>
#ಅನ್ತರ್ಭಾವ <string>

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
ಸಾರ್ವಜನಿಕ:
    // TYPES
    class ActionIfs;
    // Functor to register options to V3OptionParser
    class AppendHelper;
    ಸಂರಚನಾ Impl;

ಗುಪ್ತ:
    // MEMBERS
    ಧ್ರುವ std::unique_ptr<Impl> m_pimpl;  // Implementation class

    // METHODS
    ActionIfs* find(ಧ್ರುವ ವರ್ಣ* optp) VL_MT_DISABLED;
    template <typename T_Act, typename T_Arg>
    ActionIfs& add(ಧ್ರುವ string& opt, T_Arg arg) VL_MT_DISABLED;
    // Returns true if strp starts with "-fno"
    ಸ್ಥಿರ bool hasPrefixFNo(ಧ್ರುವ ವರ್ಣ* strp) VL_MT_DISABLED;
    // Returns true if strp starts with "-no"
    ಸ್ಥಿರ bool hasPrefixNo(ಧ್ರುವ ವರ್ಣ* strp) VL_MT_DISABLED;

ಸಾರ್ವಜನಿಕ:
    // METHODS
    // Returns how many args are consumed. 0 means not match.
    // Also bool, true iff option is needed for rerun with --dump-inputs
    std::pair<ಪೂರ್ಣಾಙ್ಕ, bool> parse(ಪೂರ್ಣಾಙ್ಕ idx, ಪೂರ್ಣಾಙ್ಕ argc, ವರ್ಣ* argv[]) VL_MT_DISABLED;
    // Find the most similar option
    string getSuggestion(ಧ್ರುವ ವರ್ಣ* str) ಧ್ರುವ VL_MT_DISABLED;
    ಶೂನ್ಯ addSuggestionCandidate(ಧ್ರುವ string& s) VL_MT_DISABLED;
    // Call this function after all options are registered.
    ಶೂನ್ಯ finalize() VL_MT_DISABLED;
    ಶೂನ್ಯ dumpOptions() VL_MT_DISABLED;

    // CONSTRUCTORS
    V3OptionParser() VL_MT_DISABLED;
    ~V3OptionParser() VL_MT_DISABLED;
};

class V3OptionParser::ActionIfs VL_NOT_FINAL {
ಸಾರ್ವಜನಿಕ:
    ಆಭಾಸ ~ActionIfs() = ಸ್ವಭಾವ;
    ಆಭಾಸ bool is_ಮೌಲ್ಯ_Needed() ಧ್ರುವ = 0;  // Need val of "-opt val"
    ಆಭಾಸ bool isFOnOffAllowed() ಧ್ರುವ = 0;  // true if "-fno-opt" is allowed
    ಆಭಾಸ bool isNotForRerun() ಧ್ರುವ = 0;  // Will not be dumped with --dump-inputs
    ಆಭಾಸ bool isOnOffAllowed() ಧ್ರುವ = 0;  // true if "-no-opt" is allowed
    ಆಭಾಸ bool isPartialMatchAllowed() ಧ್ರುವ = 0;  // true if "-Wno-" matches "-Wno-fatal"
    ಆಭಾಸ bool isUndocumented() ಧ್ರುವ = 0;  // Will not be suggested in typo
    // Set a value or run callback
    ಆಭಾಸ ಶೂನ್ಯ exec(ಧ್ರುವ ವರ್ಣ* optp, ಧ್ರುವ ವರ್ಣ* valp) = 0;
    // Mark this option undocumented. (Exclude this option from suggestion list).
    ಆಭಾಸ ActionIfs& undocumented() = 0;
    // Mark this as not needed for rerunning with preprocessed sources
    ಆಭಾಸ ActionIfs& notForRerun() = 0;
};

// A helper class to register options
class V3OptionParser::AppendHelper final {
ಸಾರ್ವಜನಿಕ:
    // TYPES
    // Tag to specify which operator() to call
    ಸಂರಚನಾ FOnOff {};  // For ActionFOnOff
    ಸಂರಚನಾ OnOff {};  // For ActionOnOff
    ಸಂರಚನಾ Set {};  // For ActionSet

    ಸಂರಚನಾ CbCall {};  // For ActionCbCall
    ಸಂರಚನಾ CbFOnOff {};  // For ActionCbFOnOff
    ಸಂರಚನಾ CbOnOff {};  // For ActionCbOnOff
    ಸಂರಚನಾ CbPartialMatch {};  // For ActionCbPartialMatch
    ಸಂರಚನಾ CbPartialMatchVal {};  // For ActionCbPartialMatchVal
    ಸಂರಚನಾ CbVal {};  // For ActionCbVal

ಗುಪ್ತ:
    // MEMBERS
    V3OptionParser& m_parser;  // The actual option registry

ಸಾರ್ವಜನಿಕ:
    // METHODS
    ActionIfs& operator()(ಧ್ರುವ ವರ್ಣ* optp, Set, bool*) ಧ್ರುವ VL_MT_DISABLED;
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
    ActionIfs& operator()(ಧ್ರುವ ವರ್ಣ* optp, Set, VOptionBool*) ಧ್ರುವ VL_MT_DISABLED;
#endif
    ActionIfs& operator()(ಧ್ರುವ ವರ್ಣ* optp, Set, ಪೂರ್ಣಾಙ್ಕ*) ಧ್ರುವ VL_MT_DISABLED;
    ActionIfs& operator()(ಧ್ರುವ ವರ್ಣ* optp, Set, string*) ಧ್ರುವ VL_MT_DISABLED;

    ActionIfs& operator()(ಧ್ರುವ ವರ್ಣ* optp, FOnOff, bool*) ಧ್ರುವ VL_MT_DISABLED;
    ActionIfs& operator()(ಧ್ರುವ ವರ್ಣ* optp, OnOff, bool*) ಧ್ರುವ VL_MT_DISABLED;
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
    ActionIfs& operator()(ಧ್ರುವ ವರ್ಣ* optp, OnOff, VOptionBool*) ಧ್ರುವ VL_MT_DISABLED;
#endif

    ActionIfs& operator()(ಧ್ರುವ ವರ್ಣ* optp, CbCall,
                          std::function<ಶೂನ್ಯ(ಶೂನ್ಯ)>) ಧ್ರುವ VL_MT_DISABLED;
    ActionIfs& operator()(ಧ್ರುವ ವರ್ಣ* optp, CbFOnOff,
                          std::function<ಶೂನ್ಯ(bool)>) ಧ್ರುವ VL_MT_DISABLED;
    ActionIfs& operator()(ಧ್ರುವ ವರ್ಣ* optp, CbOnOff,
                          std::function<ಶೂನ್ಯ(bool)>) ಧ್ರುವ VL_MT_DISABLED;
    ActionIfs& operator()(ಧ್ರುವ ವರ್ಣ* optp, CbVal, std::function<ಶೂನ್ಯ(ಪೂರ್ಣಾಙ್ಕ)>) ಧ್ರುವ VL_MT_DISABLED;
    ActionIfs& operator()(ಧ್ರುವ ವರ್ಣ* optp, CbVal,
                          std::function<ಶೂನ್ಯ(ಧ್ರುವ ವರ್ಣ*)>) ಧ್ರುವ VL_MT_DISABLED;

    ActionIfs& operator()(ಧ್ರುವ ವರ್ಣ* optp, CbPartialMatch,
                          std::function<ಶೂನ್ಯ(ಧ್ರುವ ವರ್ಣ*)>) ಧ್ರುವ VL_MT_DISABLED;
    ActionIfs& operator()(ಧ್ರುವ ವರ್ಣ* optp, CbPartialMatchVal,
                          std::function<ಶೂನ್ಯ(ಧ್ರುವ ವರ್ಣ*, ಧ್ರುವ ವರ್ಣ*)>) ಧ್ರುವ VL_MT_DISABLED;

    // CONSTRUCTORS
    explicit AppendHelper(V3OptionParser& parser)
        : m_parser{parser} {}
};

#define V3OPTION_PARSER_DECL_TAGS \
    ಧ್ರುವ auto Set VL_ATTR_UNUSED = V3OptionParser::AppendHelper::Set{}; \
    ಧ್ರುವ auto FOnOff VL_ATTR_UNUSED = V3OptionParser::AppendHelper::FOnOff{}; \
    ಧ್ರುವ auto OnOff VL_ATTR_UNUSED = V3OptionParser::AppendHelper::OnOff{}; \
    ಧ್ರುವ auto CbCall VL_ATTR_UNUSED = V3OptionParser::AppendHelper::CbCall{}; \
    ಧ್ರುವ auto CbFOnOff VL_ATTR_UNUSED = V3OptionParser::AppendHelper::CbFOnOff{}; \
    ಧ್ರುವ auto CbOnOff VL_ATTR_UNUSED = V3OptionParser::AppendHelper::CbOnOff{}; \
    ಧ್ರುವ auto CbPartialMatch VL_ATTR_UNUSED = V3OptionParser::AppendHelper::CbPartialMatch{}; \
    ಧ್ರುವ auto CbPartialMatchVal VL_ATTR_UNUSED \
        = V3OptionParser::AppendHelper::CbPartialMatchVal{}; \
    ಧ್ರುವ auto CbVal VL_ATTR_UNUSED = V3OptionParser::AppendHelper::CbVal{};

//######################################################################

#endif  // guard
