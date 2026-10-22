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

#अगर_अघोषित VERILATOR_V3OPTION_PARSER_H_
#घोषणा VERILATOR_V3OPTION_PARSER_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश <functional>
#समावेश <memory>
#समावेश <string>

// Not to include V3Option.h here so that VlcMain or other executables can use this file easily.
#अगर_अघोषित V3OPTION_PARSER_NO_VOPTION_BOOL
class VOptionBool;
#पूर्ण_अगर

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
खुला:
    // TYPES
    class ActionIfs;
    // Functor to register options to V3OptionParser
    class AppendHelper;
    काष्ठा Impl;

निजी:
    // MEMBERS
    स्थिर std::unique_ptr<Impl> m_pimpl;  // Implementation class

    // METHODS
    ActionIfs* find(स्थिर अक्षर* optp) VL_MT_DISABLED;
    ढाँचा <typename T_Act, typename T_Arg>
    ActionIfs& add(स्थिर string& opt, T_Arg arg) VL_MT_DISABLED;
    // Returns true if strp starts with "-fno"
    अटल bool hasPrefixFNo(स्थिर अक्षर* strp) VL_MT_DISABLED;
    // Returns true if strp starts with "-no"
    अटल bool hasPrefixNo(स्थिर अक्षर* strp) VL_MT_DISABLED;

खुला:
    // METHODS
    // Returns how many args are consumed. 0 means not match.
    // Also bool, true iff option is needed for rerun with --dump-inputs
    std::pair<पूर्णांक, bool> parse(पूर्णांक idx, पूर्णांक argc, अक्षर* argv[]) VL_MT_DISABLED;
    // Find the most similar option
    string getSuggestion(स्थिर अक्षर* str) स्थिर VL_MT_DISABLED;
    व्योम addSuggestionCandidate(स्थिर string& s) VL_MT_DISABLED;
    // Call this function after all options are registered.
    व्योम finalize() VL_MT_DISABLED;
    व्योम dumpOptions() VL_MT_DISABLED;

    // CONSTRUCTORS
    V3OptionParser() VL_MT_DISABLED;
    ~V3OptionParser() VL_MT_DISABLED;
};

class V3OptionParser::ActionIfs VL_NOT_FINAL {
खुला:
    भव ~ActionIfs() = शेष;
    भव bool is_मूल्य_Needed() स्थिर = 0;  // Need val of "-opt val"
    भव bool isFOnOffAllowed() स्थिर = 0;  // true if "-fno-opt" is allowed
    भव bool isNotForRerun() स्थिर = 0;  // Will not be dumped with --dump-inputs
    भव bool isOnOffAllowed() स्थिर = 0;  // true if "-no-opt" is allowed
    भव bool isPartialMatchAllowed() स्थिर = 0;  // true if "-Wno-" matches "-Wno-fatal"
    भव bool isUndocumented() स्थिर = 0;  // Will not be suggested in typo
    // Set a value or run callback
    भव व्योम exec(स्थिर अक्षर* optp, स्थिर अक्षर* valp) = 0;
    // Mark this option undocumented. (Exclude this option from suggestion list).
    भव ActionIfs& undocumented() = 0;
    // Mark this as not needed for rerunning with preprocessed sources
    भव ActionIfs& notForRerun() = 0;
};

// A helper class to register options
class V3OptionParser::AppendHelper final {
खुला:
    // TYPES
    // Tag to specify which operator() to call
    काष्ठा FOnOff {};  // For ActionFOnOff
    काष्ठा OnOff {};  // For ActionOnOff
    काष्ठा Set {};  // For ActionSet

    काष्ठा CbCall {};  // For ActionCbCall
    काष्ठा CbFOnOff {};  // For ActionCbFOnOff
    काष्ठा CbOnOff {};  // For ActionCbOnOff
    काष्ठा CbPartialMatch {};  // For ActionCbPartialMatch
    काष्ठा CbPartialMatchVal {};  // For ActionCbPartialMatchVal
    काष्ठा CbVal {};  // For ActionCbVal

निजी:
    // MEMBERS
    V3OptionParser& m_parser;  // The actual option registry

खुला:
    // METHODS
    ActionIfs& चालक()(स्थिर अक्षर* optp, Set, bool*) स्थिर VL_MT_DISABLED;
#अगर_अघोषित V3OPTION_PARSER_NO_VOPTION_BOOL
    ActionIfs& चालक()(स्थिर अक्षर* optp, Set, VOptionBool*) स्थिर VL_MT_DISABLED;
#पूर्ण_अगर
    ActionIfs& चालक()(स्थिर अक्षर* optp, Set, पूर्णांक*) स्थिर VL_MT_DISABLED;
    ActionIfs& चालक()(स्थिर अक्षर* optp, Set, string*) स्थिर VL_MT_DISABLED;

    ActionIfs& चालक()(स्थिर अक्षर* optp, FOnOff, bool*) स्थिर VL_MT_DISABLED;
    ActionIfs& चालक()(स्थिर अक्षर* optp, OnOff, bool*) स्थिर VL_MT_DISABLED;
#अगर_अघोषित V3OPTION_PARSER_NO_VOPTION_BOOL
    ActionIfs& चालक()(स्थिर अक्षर* optp, OnOff, VOptionBool*) स्थिर VL_MT_DISABLED;
#पूर्ण_अगर

    ActionIfs& चालक()(स्थिर अक्षर* optp, CbCall,
                          std::function<व्योम(व्योम)>) स्थिर VL_MT_DISABLED;
    ActionIfs& चालक()(स्थिर अक्षर* optp, CbFOnOff,
                          std::function<व्योम(bool)>) स्थिर VL_MT_DISABLED;
    ActionIfs& चालक()(स्थिर अक्षर* optp, CbOnOff,
                          std::function<व्योम(bool)>) स्थिर VL_MT_DISABLED;
    ActionIfs& चालक()(स्थिर अक्षर* optp, CbVal, std::function<व्योम(पूर्णांक)>) स्थिर VL_MT_DISABLED;
    ActionIfs& चालक()(स्थिर अक्षर* optp, CbVal,
                          std::function<व्योम(स्थिर अक्षर*)>) स्थिर VL_MT_DISABLED;

    ActionIfs& चालक()(स्थिर अक्षर* optp, CbPartialMatch,
                          std::function<व्योम(स्थिर अक्षर*)>) स्थिर VL_MT_DISABLED;
    ActionIfs& चालक()(स्थिर अक्षर* optp, CbPartialMatchVal,
                          std::function<व्योम(स्थिर अक्षर*, स्थिर अक्षर*)>) स्थिर VL_MT_DISABLED;

    // CONSTRUCTORS
    explicit AppendHelper(V3OptionParser& parser)
        : m_parser{parser} {}
};

#घोषणा V3OPTION_PARSER_DECL_TAGS \
    स्थिर स्वतः Set VL_ATTR_UNUSED = V3OptionParser::AppendHelper::Set{}; \
    स्थिर स्वतः FOnOff VL_ATTR_UNUSED = V3OptionParser::AppendHelper::FOnOff{}; \
    स्थिर स्वतः OnOff VL_ATTR_UNUSED = V3OptionParser::AppendHelper::OnOff{}; \
    स्थिर स्वतः CbCall VL_ATTR_UNUSED = V3OptionParser::AppendHelper::CbCall{}; \
    स्थिर स्वतः CbFOnOff VL_ATTR_UNUSED = V3OptionParser::AppendHelper::CbFOnOff{}; \
    स्थिर स्वतः CbOnOff VL_ATTR_UNUSED = V3OptionParser::AppendHelper::CbOnOff{}; \
    स्थिर स्वतः CbPartialMatch VL_ATTR_UNUSED = V3OptionParser::AppendHelper::CbPartialMatch{}; \
    स्थिर स्वतः CbPartialMatchVal VL_ATTR_UNUSED \
        = V3OptionParser::AppendHelper::CbPartialMatchVal{}; \
    स्थिर स्वतः CbVal VL_ATTR_UNUSED = V3OptionParser::AppendHelper::CbVal{};

//######################################################################

#पूर्ण_अगर  // guard
