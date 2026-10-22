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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ <functional>
#ଅନ୍ତର୍ଭାଵ <memory>
#ଅନ୍ତର୍ଭାଵ <string>

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
ସର୍ବସାଧାରଣ:
    // TYPES
    class ActionIfs;
    // Functor to register options to V3OptionParser
    class AppendHelper;
    ସଂରଚନା Impl;

ବ୍ୟକ୍ତିଗତ:
    // MEMBERS
    ଅଚଳ std::unique_ptr<Impl> m_pimpl;  // Implementation class

    // METHODS
    ActionIfs* find(ଅଚଳ ଅକ୍ଷର* optp) VL_MT_DISABLED;
    template <typename T_Act, typename T_Arg>
    ActionIfs& add(ଅଚଳ string& opt, T_Arg arg) VL_MT_DISABLED;
    // Returns true if strp starts with "-fno"
    ସ୍ଥିର bool hasPrefixFNo(ଅଚଳ ଅକ୍ଷର* strp) VL_MT_DISABLED;
    // Returns true if strp starts with "-no"
    ସ୍ଥିର bool hasPrefixNo(ଅଚଳ ଅକ୍ଷର* strp) VL_MT_DISABLED;

ସର୍ବସାଧାରଣ:
    // METHODS
    // Returns how many args are consumed. 0 means not match.
    // Also bool, true iff option is needed for rerun with --dump-inputs
    std::pair<ପୂର୍ଣ୍ଣସଂଖ୍ୟା, bool> parse(ପୂର୍ଣ୍ଣସଂଖ୍ୟା idx, ପୂର୍ଣ୍ଣସଂଖ୍ୟା argc, ଅକ୍ଷର* argv[]) VL_MT_DISABLED;
    // Find the most similar option
    string getSuggestion(ଅଚଳ ଅକ୍ଷର* str) ଅଚଳ VL_MT_DISABLED;
    ଶୂନ୍ୟ addSuggestionCandidate(ଅଚଳ string& s) VL_MT_DISABLED;
    // Call this function after all options are registered.
    ଶୂନ୍ୟ finalize() VL_MT_DISABLED;
    ଶୂନ୍ୟ dumpOptions() VL_MT_DISABLED;

    // CONSTRUCTORS
    V3OptionParser() VL_MT_DISABLED;
    ~V3OptionParser() VL_MT_DISABLED;
};

class V3OptionParser::ActionIfs VL_NOT_FINAL {
ସର୍ବସାଧାରଣ:
    ଆଭାସୀ ~ActionIfs() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ଆଭାସୀ bool is_ମୂଲ୍ୟ_Needed() ଅଚଳ = 0;  // Need val of "-opt val"
    ଆଭାସୀ bool isFOnOffAllowed() ଅଚଳ = 0;  // true if "-fno-opt" is allowed
    ଆଭାସୀ bool isNotForRerun() ଅଚଳ = 0;  // Will not be dumped with --dump-inputs
    ଆଭାସୀ bool isOnOffAllowed() ଅଚଳ = 0;  // true if "-no-opt" is allowed
    ଆଭାସୀ bool isPartialMatchAllowed() ଅଚଳ = 0;  // true if "-Wno-" matches "-Wno-fatal"
    ଆଭାସୀ bool isUndocumented() ଅଚଳ = 0;  // Will not be suggested in typo
    // Set a value or run callback
    ଆଭାସୀ ଶୂନ୍ୟ exec(ଅଚଳ ଅକ୍ଷର* optp, ଅଚଳ ଅକ୍ଷର* valp) = 0;
    // Mark this option undocumented. (Exclude this option from suggestion list).
    ଆଭାସୀ ActionIfs& undocumented() = 0;
    // Mark this as not needed for rerunning with preprocessed sources
    ଆଭାସୀ ActionIfs& notForRerun() = 0;
};

// A helper class to register options
class V3OptionParser::AppendHelper final {
ସର୍ବସାଧାରଣ:
    // TYPES
    // Tag to specify which operator() to call
    ସଂରଚନା FOnOff {};  // For ActionFOnOff
    ସଂରଚନା OnOff {};  // For ActionOnOff
    ସଂରଚନା Set {};  // For ActionSet

    ସଂରଚନା CbCall {};  // For ActionCbCall
    ସଂରଚନା CbFOnOff {};  // For ActionCbFOnOff
    ସଂରଚନା CbOnOff {};  // For ActionCbOnOff
    ସଂରଚନା CbPartialMatch {};  // For ActionCbPartialMatch
    ସଂରଚନା CbPartialMatchVal {};  // For ActionCbPartialMatchVal
    ସଂରଚନା CbVal {};  // For ActionCbVal

ବ୍ୟକ୍ତିଗତ:
    // MEMBERS
    V3OptionParser& m_parser;  // The actual option registry

ସର୍ବସାଧାରଣ:
    // METHODS
    ActionIfs& operator()(ଅଚଳ ଅକ୍ଷର* optp, Set, bool*) ଅଚଳ VL_MT_DISABLED;
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
    ActionIfs& operator()(ଅଚଳ ଅକ୍ଷର* optp, Set, VOptionBool*) ଅଚଳ VL_MT_DISABLED;
#endif
    ActionIfs& operator()(ଅଚଳ ଅକ୍ଷର* optp, Set, ପୂର୍ଣ୍ଣସଂଖ୍ୟା*) ଅଚଳ VL_MT_DISABLED;
    ActionIfs& operator()(ଅଚଳ ଅକ୍ଷର* optp, Set, string*) ଅଚଳ VL_MT_DISABLED;

    ActionIfs& operator()(ଅଚଳ ଅକ୍ଷର* optp, FOnOff, bool*) ଅଚଳ VL_MT_DISABLED;
    ActionIfs& operator()(ଅଚଳ ଅକ୍ଷର* optp, OnOff, bool*) ଅଚଳ VL_MT_DISABLED;
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
    ActionIfs& operator()(ଅଚଳ ଅକ୍ଷର* optp, OnOff, VOptionBool*) ଅଚଳ VL_MT_DISABLED;
#endif

    ActionIfs& operator()(ଅଚଳ ଅକ୍ଷର* optp, CbCall,
                          std::function<ଶୂନ୍ୟ(ଶୂନ୍ୟ)>) ଅଚଳ VL_MT_DISABLED;
    ActionIfs& operator()(ଅଚଳ ଅକ୍ଷର* optp, CbFOnOff,
                          std::function<ଶୂନ୍ୟ(bool)>) ଅଚଳ VL_MT_DISABLED;
    ActionIfs& operator()(ଅଚଳ ଅକ୍ଷର* optp, CbOnOff,
                          std::function<ଶୂନ୍ୟ(bool)>) ଅଚଳ VL_MT_DISABLED;
    ActionIfs& operator()(ଅଚଳ ଅକ୍ଷର* optp, CbVal, std::function<ଶୂନ୍ୟ(ପୂର୍ଣ୍ଣସଂଖ୍ୟା)>) ଅଚଳ VL_MT_DISABLED;
    ActionIfs& operator()(ଅଚଳ ଅକ୍ଷର* optp, CbVal,
                          std::function<ଶୂନ୍ୟ(ଅଚଳ ଅକ୍ଷର*)>) ଅଚଳ VL_MT_DISABLED;

    ActionIfs& operator()(ଅଚଳ ଅକ୍ଷର* optp, CbPartialMatch,
                          std::function<ଶୂନ୍ୟ(ଅଚଳ ଅକ୍ଷର*)>) ଅଚଳ VL_MT_DISABLED;
    ActionIfs& operator()(ଅଚଳ ଅକ୍ଷର* optp, CbPartialMatchVal,
                          std::function<ଶୂନ୍ୟ(ଅଚଳ ଅକ୍ଷର*, ଅଚଳ ଅକ୍ଷର*)>) ଅଚଳ VL_MT_DISABLED;

    // CONSTRUCTORS
    explicit AppendHelper(V3OptionParser& parser)
        : m_parser{parser} {}
};

#define V3OPTION_PARSER_DECL_TAGS \
    ଅଚଳ auto Set VL_ATTR_UNUSED = V3OptionParser::AppendHelper::Set{}; \
    ଅଚଳ auto FOnOff VL_ATTR_UNUSED = V3OptionParser::AppendHelper::FOnOff{}; \
    ଅଚଳ auto OnOff VL_ATTR_UNUSED = V3OptionParser::AppendHelper::OnOff{}; \
    ଅଚଳ auto CbCall VL_ATTR_UNUSED = V3OptionParser::AppendHelper::CbCall{}; \
    ଅଚଳ auto CbFOnOff VL_ATTR_UNUSED = V3OptionParser::AppendHelper::CbFOnOff{}; \
    ଅଚଳ auto CbOnOff VL_ATTR_UNUSED = V3OptionParser::AppendHelper::CbOnOff{}; \
    ଅଚଳ auto CbPartialMatch VL_ATTR_UNUSED = V3OptionParser::AppendHelper::CbPartialMatch{}; \
    ଅଚଳ auto CbPartialMatchVal VL_ATTR_UNUSED \
        = V3OptionParser::AppendHelper::CbPartialMatchVal{}; \
    ଅଚଳ auto CbVal VL_ATTR_UNUSED = V3OptionParser::AppendHelper::CbVal{};

//######################################################################

#endif  // guard
