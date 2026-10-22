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

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ <functional>
#അന്തര്ഭാവ <memory>
#അന്തര്ഭാവ <string>

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
പൊതു:
    // TYPES
    class ActionIfs;
    // Functor to register options to V3OptionParser
    class AppendHelper;
    ഘടന Impl;

സ്വകാര്യ:
    // MEMBERS
    സ്ഥിരം std::unique_ptr<Impl> m_pimpl;  // Implementation class

    // METHODS
    ActionIfs* find(സ്ഥിരം അക്ഷരം* optp) VL_MT_DISABLED;
    template <typename T_Act, typename T_Arg>
    ActionIfs& add(സ്ഥിരം string& opt, T_Arg arg) VL_MT_DISABLED;
    // Returns true if strp starts with "-fno"
    സ്ഥിതം bool hasPrefixFNo(സ്ഥിരം അക്ഷരം* strp) VL_MT_DISABLED;
    // Returns true if strp starts with "-no"
    സ്ഥിതം bool hasPrefixNo(സ്ഥിരം അക്ഷരം* strp) VL_MT_DISABLED;

പൊതു:
    // METHODS
    // Returns how many args are consumed. 0 means not match.
    // Also bool, true iff option is needed for rerun with --dump-inputs
    std::pair<പൂർണ്ണസംഖ്യ, bool> parse(പൂർണ്ണസംഖ്യ idx, പൂർണ്ണസംഖ്യ argc, അക്ഷരം* argv[]) VL_MT_DISABLED;
    // Find the most similar option
    string getSuggestion(സ്ഥിരം അക്ഷരം* str) സ്ഥിരം VL_MT_DISABLED;
    ശൂന്യം addSuggestionCandidate(സ്ഥിരം string& s) VL_MT_DISABLED;
    // Call this function after all options are registered.
    ശൂന്യം finalize() VL_MT_DISABLED;
    ശൂന്യം dumpOptions() VL_MT_DISABLED;

    // CONSTRUCTORS
    V3OptionParser() VL_MT_DISABLED;
    ~V3OptionParser() VL_MT_DISABLED;
};

class V3OptionParser::ActionIfs VL_NOT_FINAL {
പൊതു:
    മായാ ~ActionIfs() = സ്ഥിരസ്ഥിതി;
    മായാ bool is_മൂല്യം_Needed() സ്ഥിരം = 0;  // Need val of "-opt val"
    മായാ bool isFOnOffAllowed() സ്ഥിരം = 0;  // true if "-fno-opt" is allowed
    മായാ bool isNotForRerun() സ്ഥിരം = 0;  // Will not be dumped with --dump-inputs
    മായാ bool isOnOffAllowed() സ്ഥിരം = 0;  // true if "-no-opt" is allowed
    മായാ bool isPartialMatchAllowed() സ്ഥിരം = 0;  // true if "-Wno-" matches "-Wno-fatal"
    മായാ bool isUndocumented() സ്ഥിരം = 0;  // Will not be suggested in typo
    // Set a value or run callback
    മായാ ശൂന്യം exec(സ്ഥിരം അക്ഷരം* optp, സ്ഥിരം അക്ഷരം* valp) = 0;
    // Mark this option undocumented. (Exclude this option from suggestion list).
    മായാ ActionIfs& undocumented() = 0;
    // Mark this as not needed for rerunning with preprocessed sources
    മായാ ActionIfs& notForRerun() = 0;
};

// A helper class to register options
class V3OptionParser::AppendHelper final {
പൊതു:
    // TYPES
    // Tag to specify which operator() to call
    ഘടന FOnOff {};  // For ActionFOnOff
    ഘടന OnOff {};  // For ActionOnOff
    ഘടന Set {};  // For ActionSet

    ഘടന CbCall {};  // For ActionCbCall
    ഘടന CbFOnOff {};  // For ActionCbFOnOff
    ഘടന CbOnOff {};  // For ActionCbOnOff
    ഘടന CbPartialMatch {};  // For ActionCbPartialMatch
    ഘടന CbPartialMatchVal {};  // For ActionCbPartialMatchVal
    ഘടന CbVal {};  // For ActionCbVal

സ്വകാര്യ:
    // MEMBERS
    V3OptionParser& m_parser;  // The actual option registry

പൊതു:
    // METHODS
    ActionIfs& operator()(സ്ഥിരം അക്ഷരം* optp, Set, bool*) സ്ഥിരം VL_MT_DISABLED;
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
    ActionIfs& operator()(സ്ഥിരം അക്ഷരം* optp, Set, VOptionBool*) സ്ഥിരം VL_MT_DISABLED;
#endif
    ActionIfs& operator()(സ്ഥിരം അക്ഷരം* optp, Set, പൂർണ്ണസംഖ്യ*) സ്ഥിരം VL_MT_DISABLED;
    ActionIfs& operator()(സ്ഥിരം അക്ഷരം* optp, Set, string*) സ്ഥിരം VL_MT_DISABLED;

    ActionIfs& operator()(സ്ഥിരം അക്ഷരം* optp, FOnOff, bool*) സ്ഥിരം VL_MT_DISABLED;
    ActionIfs& operator()(സ്ഥിരം അക്ഷരം* optp, OnOff, bool*) സ്ഥിരം VL_MT_DISABLED;
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
    ActionIfs& operator()(സ്ഥിരം അക്ഷരം* optp, OnOff, VOptionBool*) സ്ഥിരം VL_MT_DISABLED;
#endif

    ActionIfs& operator()(സ്ഥിരം അക്ഷരം* optp, CbCall,
                          std::function<ശൂന്യം(ശൂന്യം)>) സ്ഥിരം VL_MT_DISABLED;
    ActionIfs& operator()(സ്ഥിരം അക്ഷരം* optp, CbFOnOff,
                          std::function<ശൂന്യം(bool)>) സ്ഥിരം VL_MT_DISABLED;
    ActionIfs& operator()(സ്ഥിരം അക്ഷരം* optp, CbOnOff,
                          std::function<ശൂന്യം(bool)>) സ്ഥിരം VL_MT_DISABLED;
    ActionIfs& operator()(സ്ഥിരം അക്ഷരം* optp, CbVal, std::function<ശൂന്യം(പൂർണ്ണസംഖ്യ)>) സ്ഥിരം VL_MT_DISABLED;
    ActionIfs& operator()(സ്ഥിരം അക്ഷരം* optp, CbVal,
                          std::function<ശൂന്യം(സ്ഥിരം അക്ഷരം*)>) സ്ഥിരം VL_MT_DISABLED;

    ActionIfs& operator()(സ്ഥിരം അക്ഷരം* optp, CbPartialMatch,
                          std::function<ശൂന്യം(സ്ഥിരം അക്ഷരം*)>) സ്ഥിരം VL_MT_DISABLED;
    ActionIfs& operator()(സ്ഥിരം അക്ഷരം* optp, CbPartialMatchVal,
                          std::function<ശൂന്യം(സ്ഥിരം അക്ഷരം*, സ്ഥിരം അക്ഷരം*)>) സ്ഥിരം VL_MT_DISABLED;

    // CONSTRUCTORS
    explicit AppendHelper(V3OptionParser& parser)
        : m_parser{parser} {}
};

#define V3OPTION_PARSER_DECL_TAGS \
    സ്ഥിരം auto Set VL_ATTR_UNUSED = V3OptionParser::AppendHelper::Set{}; \
    സ്ഥിരം auto FOnOff VL_ATTR_UNUSED = V3OptionParser::AppendHelper::FOnOff{}; \
    സ്ഥിരം auto OnOff VL_ATTR_UNUSED = V3OptionParser::AppendHelper::OnOff{}; \
    സ്ഥിരം auto CbCall VL_ATTR_UNUSED = V3OptionParser::AppendHelper::CbCall{}; \
    സ്ഥിരം auto CbFOnOff VL_ATTR_UNUSED = V3OptionParser::AppendHelper::CbFOnOff{}; \
    സ്ഥിരം auto CbOnOff VL_ATTR_UNUSED = V3OptionParser::AppendHelper::CbOnOff{}; \
    സ്ഥിരം auto CbPartialMatch VL_ATTR_UNUSED = V3OptionParser::AppendHelper::CbPartialMatch{}; \
    സ്ഥിരം auto CbPartialMatchVal VL_ATTR_UNUSED \
        = V3OptionParser::AppendHelper::CbPartialMatchVal{}; \
    സ്ഥിരം auto CbVal VL_ATTR_UNUSED = V3OptionParser::AppendHelper::CbVal{};

//######################################################################

#endif  // guard
