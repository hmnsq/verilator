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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ <functional>
#அந்தர்பாவ <memory>
#அந்தர்பாவ <string>

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
பொது:
    // TYPES
    class ActionIfs;
    // Functor to register options to V3OptionParser
    class AppendHelper;
    கட்டமைப்பு Impl;

தனிப்பட்ட:
    // MEMBERS
    மாறிலி std::unique_ptr<Impl> m_pimpl;  // Implementation class

    // METHODS
    ActionIfs* find(மாறிலி எழுத்து* optp) VL_MT_DISABLED;
    template <typename T_Act, typename T_Arg>
    ActionIfs& add(மாறிலி string& opt, T_Arg arg) VL_MT_DISABLED;
    // Returns true if strp starts with "-fno"
    நிலையான bool hasPrefixFNo(மாறிலி எழுத்து* strp) VL_MT_DISABLED;
    // Returns true if strp starts with "-no"
    நிலையான bool hasPrefixNo(மாறிலி எழுத்து* strp) VL_MT_DISABLED;

பொது:
    // METHODS
    // Returns how many args are consumed. 0 means not match.
    // Also bool, true iff option is needed for rerun with --dump-inputs
    std::pair<முழு, bool> parse(முழு idx, முழு argc, எழுத்து* argv[]) VL_MT_DISABLED;
    // Find the most similar option
    string getSuggestion(மாறிலி எழுத்து* str) மாறிலி VL_MT_DISABLED;
    வெறுமை addSuggestionCandidate(மாறிலி string& s) VL_MT_DISABLED;
    // Call this function after all options are registered.
    வெறுமை finalize() VL_MT_DISABLED;
    வெறுமை dumpOptions() VL_MT_DISABLED;

    // CONSTRUCTORS
    V3OptionParser() VL_MT_DISABLED;
    ~V3OptionParser() VL_MT_DISABLED;
};

class V3OptionParser::ActionIfs VL_NOT_FINAL {
பொது:
    மெய்நிகர் ~ActionIfs() = இயல்பு;
    மெய்நிகர் bool is_மதிப்பு_Needed() மாறிலி = 0;  // Need val of "-opt val"
    மெய்நிகர் bool isFOnOffAllowed() மாறிலி = 0;  // true if "-fno-opt" is allowed
    மெய்நிகர் bool isNotForRerun() மாறிலி = 0;  // Will not be dumped with --dump-inputs
    மெய்நிகர் bool isOnOffAllowed() மாறிலி = 0;  // true if "-no-opt" is allowed
    மெய்நிகர் bool isPartialMatchAllowed() மாறிலி = 0;  // true if "-Wno-" matches "-Wno-fatal"
    மெய்நிகர் bool isUndocumented() மாறிலி = 0;  // Will not be suggested in typo
    // Set a value or run callback
    மெய்நிகர் வெறுமை exec(மாறிலி எழுத்து* optp, மாறிலி எழுத்து* valp) = 0;
    // Mark this option undocumented. (Exclude this option from suggestion list).
    மெய்நிகர் ActionIfs& undocumented() = 0;
    // Mark this as not needed for rerunning with preprocessed sources
    மெய்நிகர் ActionIfs& notForRerun() = 0;
};

// A helper class to register options
class V3OptionParser::AppendHelper final {
பொது:
    // TYPES
    // Tag to specify which operator() to call
    கட்டமைப்பு FOnOff {};  // For ActionFOnOff
    கட்டமைப்பு OnOff {};  // For ActionOnOff
    கட்டமைப்பு Set {};  // For ActionSet

    கட்டமைப்பு CbCall {};  // For ActionCbCall
    கட்டமைப்பு CbFOnOff {};  // For ActionCbFOnOff
    கட்டமைப்பு CbOnOff {};  // For ActionCbOnOff
    கட்டமைப்பு CbPartialMatch {};  // For ActionCbPartialMatch
    கட்டமைப்பு CbPartialMatchVal {};  // For ActionCbPartialMatchVal
    கட்டமைப்பு CbVal {};  // For ActionCbVal

தனிப்பட்ட:
    // MEMBERS
    V3OptionParser& m_parser;  // The actual option registry

பொது:
    // METHODS
    ActionIfs& operator()(மாறிலி எழுத்து* optp, Set, bool*) மாறிலி VL_MT_DISABLED;
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
    ActionIfs& operator()(மாறிலி எழுத்து* optp, Set, VOptionBool*) மாறிலி VL_MT_DISABLED;
#endif
    ActionIfs& operator()(மாறிலி எழுத்து* optp, Set, முழு*) மாறிலி VL_MT_DISABLED;
    ActionIfs& operator()(மாறிலி எழுத்து* optp, Set, string*) மாறிலி VL_MT_DISABLED;

    ActionIfs& operator()(மாறிலி எழுத்து* optp, FOnOff, bool*) மாறிலி VL_MT_DISABLED;
    ActionIfs& operator()(மாறிலி எழுத்து* optp, OnOff, bool*) மாறிலி VL_MT_DISABLED;
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
    ActionIfs& operator()(மாறிலி எழுத்து* optp, OnOff, VOptionBool*) மாறிலி VL_MT_DISABLED;
#endif

    ActionIfs& operator()(மாறிலி எழுத்து* optp, CbCall,
                          std::function<வெறுமை(வெறுமை)>) மாறிலி VL_MT_DISABLED;
    ActionIfs& operator()(மாறிலி எழுத்து* optp, CbFOnOff,
                          std::function<வெறுமை(bool)>) மாறிலி VL_MT_DISABLED;
    ActionIfs& operator()(மாறிலி எழுத்து* optp, CbOnOff,
                          std::function<வெறுமை(bool)>) மாறிலி VL_MT_DISABLED;
    ActionIfs& operator()(மாறிலி எழுத்து* optp, CbVal, std::function<வெறுமை(முழு)>) மாறிலி VL_MT_DISABLED;
    ActionIfs& operator()(மாறிலி எழுத்து* optp, CbVal,
                          std::function<வெறுமை(மாறிலி எழுத்து*)>) மாறிலி VL_MT_DISABLED;

    ActionIfs& operator()(மாறிலி எழுத்து* optp, CbPartialMatch,
                          std::function<வெறுமை(மாறிலி எழுத்து*)>) மாறிலி VL_MT_DISABLED;
    ActionIfs& operator()(மாறிலி எழுத்து* optp, CbPartialMatchVal,
                          std::function<வெறுமை(மாறிலி எழுத்து*, மாறிலி எழுத்து*)>) மாறிலி VL_MT_DISABLED;

    // CONSTRUCTORS
    explicit AppendHelper(V3OptionParser& parser)
        : m_parser{parser} {}
};

#define V3OPTION_PARSER_DECL_TAGS \
    மாறிலி auto Set VL_ATTR_UNUSED = V3OptionParser::AppendHelper::Set{}; \
    மாறிலி auto FOnOff VL_ATTR_UNUSED = V3OptionParser::AppendHelper::FOnOff{}; \
    மாறிலி auto OnOff VL_ATTR_UNUSED = V3OptionParser::AppendHelper::OnOff{}; \
    மாறிலி auto CbCall VL_ATTR_UNUSED = V3OptionParser::AppendHelper::CbCall{}; \
    மாறிலி auto CbFOnOff VL_ATTR_UNUSED = V3OptionParser::AppendHelper::CbFOnOff{}; \
    மாறிலி auto CbOnOff VL_ATTR_UNUSED = V3OptionParser::AppendHelper::CbOnOff{}; \
    மாறிலி auto CbPartialMatch VL_ATTR_UNUSED = V3OptionParser::AppendHelper::CbPartialMatch{}; \
    மாறிலி auto CbPartialMatchVal VL_ATTR_UNUSED \
        = V3OptionParser::AppendHelper::CbPartialMatchVal{}; \
    மாறிலி auto CbVal VL_ATTR_UNUSED = V3OptionParser::AppendHelper::CbVal{};

//######################################################################

#endif  // guard
