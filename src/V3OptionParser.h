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

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ <functional>
#અન્તર્ભાવ <memory>
#અન્તર્ભાવ <string>

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
જાહેર:
    // TYPES
    class ActionIfs;
    // Functor to register options to V3OptionParser
    class AppendHelper;
    સંરચના Impl;

ખાનગી:
    // MEMBERS
    અચલ std::unique_ptr<Impl> m_pimpl;  // Implementation class

    // METHODS
    ActionIfs* find(અચલ અક્ષર* optp) VL_MT_DISABLED;
    template <typename T_Act, typename T_Arg>
    ActionIfs& add(અચલ string& opt, T_Arg arg) VL_MT_DISABLED;
    // Returns true if strp starts with "-fno"
    સ્થિર bool hasPrefixFNo(અચલ અક્ષર* strp) VL_MT_DISABLED;
    // Returns true if strp starts with "-no"
    સ્થિર bool hasPrefixNo(અચલ અક્ષર* strp) VL_MT_DISABLED;

જાહેર:
    // METHODS
    // Returns how many args are consumed. 0 means not match.
    // Also bool, true iff option is needed for rerun with --dump-inputs
    std::pair<પૂર્ણાંક, bool> parse(પૂર્ણાંક idx, પૂર્ણાંક argc, અક્ષર* argv[]) VL_MT_DISABLED;
    // Find the most similar option
    string getSuggestion(અચલ અક્ષર* str) અચલ VL_MT_DISABLED;
    શૂન્ય addSuggestionCandidate(અચલ string& s) VL_MT_DISABLED;
    // Call this function after all options are registered.
    શૂન્ય finalize() VL_MT_DISABLED;
    શૂન્ય dumpOptions() VL_MT_DISABLED;

    // CONSTRUCTORS
    V3OptionParser() VL_MT_DISABLED;
    ~V3OptionParser() VL_MT_DISABLED;
};

class V3OptionParser::ActionIfs VL_NOT_FINAL {
જાહેર:
    આભાસી ~ActionIfs() = મૂળભૂત;
    આભાસી bool is_કિંમત_Needed() અચલ = 0;  // Need val of "-opt val"
    આભાસી bool isFOnOffAllowed() અચલ = 0;  // true if "-fno-opt" is allowed
    આભાસી bool isNotForRerun() અચલ = 0;  // Will not be dumped with --dump-inputs
    આભાસી bool isOnOffAllowed() અચલ = 0;  // true if "-no-opt" is allowed
    આભાસી bool isPartialMatchAllowed() અચલ = 0;  // true if "-Wno-" matches "-Wno-fatal"
    આભાસી bool isUndocumented() અચલ = 0;  // Will not be suggested in typo
    // Set a value or run callback
    આભાસી શૂન્ય exec(અચલ અક્ષર* optp, અચલ અક્ષર* valp) = 0;
    // Mark this option undocumented. (Exclude this option from suggestion list).
    આભાસી ActionIfs& undocumented() = 0;
    // Mark this as not needed for rerunning with preprocessed sources
    આભાસી ActionIfs& notForRerun() = 0;
};

// A helper class to register options
class V3OptionParser::AppendHelper final {
જાહેર:
    // TYPES
    // Tag to specify which operator() to call
    સંરચના FOnOff {};  // For ActionFOnOff
    સંરચના OnOff {};  // For ActionOnOff
    સંરચના Set {};  // For ActionSet

    સંરચના CbCall {};  // For ActionCbCall
    સંરચના CbFOnOff {};  // For ActionCbFOnOff
    સંરચના CbOnOff {};  // For ActionCbOnOff
    સંરચના CbPartialMatch {};  // For ActionCbPartialMatch
    સંરચના CbPartialMatchVal {};  // For ActionCbPartialMatchVal
    સંરચના CbVal {};  // For ActionCbVal

ખાનગી:
    // MEMBERS
    V3OptionParser& m_parser;  // The actual option registry

જાહેર:
    // METHODS
    ActionIfs& operator()(અચલ અક્ષર* optp, Set, bool*) અચલ VL_MT_DISABLED;
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
    ActionIfs& operator()(અચલ અક્ષર* optp, Set, VOptionBool*) અચલ VL_MT_DISABLED;
#endif
    ActionIfs& operator()(અચલ અક્ષર* optp, Set, પૂર્ણાંક*) અચલ VL_MT_DISABLED;
    ActionIfs& operator()(અચલ અક્ષર* optp, Set, string*) અચલ VL_MT_DISABLED;

    ActionIfs& operator()(અચલ અક્ષર* optp, FOnOff, bool*) અચલ VL_MT_DISABLED;
    ActionIfs& operator()(અચલ અક્ષર* optp, OnOff, bool*) અચલ VL_MT_DISABLED;
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
    ActionIfs& operator()(અચલ અક્ષર* optp, OnOff, VOptionBool*) અચલ VL_MT_DISABLED;
#endif

    ActionIfs& operator()(અચલ અક્ષર* optp, CbCall,
                          std::function<શૂન્ય(શૂન્ય)>) અચલ VL_MT_DISABLED;
    ActionIfs& operator()(અચલ અક્ષર* optp, CbFOnOff,
                          std::function<શૂન્ય(bool)>) અચલ VL_MT_DISABLED;
    ActionIfs& operator()(અચલ અક્ષર* optp, CbOnOff,
                          std::function<શૂન્ય(bool)>) અચલ VL_MT_DISABLED;
    ActionIfs& operator()(અચલ અક્ષર* optp, CbVal, std::function<શૂન્ય(પૂર્ણાંક)>) અચલ VL_MT_DISABLED;
    ActionIfs& operator()(અચલ અક્ષર* optp, CbVal,
                          std::function<શૂન્ય(અચલ અક્ષર*)>) અચલ VL_MT_DISABLED;

    ActionIfs& operator()(અચલ અક્ષર* optp, CbPartialMatch,
                          std::function<શૂન્ય(અચલ અક્ષર*)>) અચલ VL_MT_DISABLED;
    ActionIfs& operator()(અચલ અક્ષર* optp, CbPartialMatchVal,
                          std::function<શૂન્ય(અચલ અક્ષર*, અચલ અક્ષર*)>) અચલ VL_MT_DISABLED;

    // CONSTRUCTORS
    explicit AppendHelper(V3OptionParser& parser)
        : m_parser{parser} {}
};

#define V3OPTION_PARSER_DECL_TAGS \
    અચલ auto Set VL_ATTR_UNUSED = V3OptionParser::AppendHelper::Set{}; \
    અચલ auto FOnOff VL_ATTR_UNUSED = V3OptionParser::AppendHelper::FOnOff{}; \
    અચલ auto OnOff VL_ATTR_UNUSED = V3OptionParser::AppendHelper::OnOff{}; \
    અચલ auto CbCall VL_ATTR_UNUSED = V3OptionParser::AppendHelper::CbCall{}; \
    અચલ auto CbFOnOff VL_ATTR_UNUSED = V3OptionParser::AppendHelper::CbFOnOff{}; \
    અચલ auto CbOnOff VL_ATTR_UNUSED = V3OptionParser::AppendHelper::CbOnOff{}; \
    અચલ auto CbPartialMatch VL_ATTR_UNUSED = V3OptionParser::AppendHelper::CbPartialMatch{}; \
    અચલ auto CbPartialMatchVal VL_ATTR_UNUSED \
        = V3OptionParser::AppendHelper::CbPartialMatchVal{}; \
    અચલ auto CbVal VL_ATTR_UNUSED = V3OptionParser::AppendHelper::CbVal{};

//######################################################################

#endif  // guard
