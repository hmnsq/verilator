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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3OPTION_PARSER_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3OPTION_PARSER_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <functional>
#ਅਨ੍ਤਰ੍ਭਾਵ <memory>
#ਅਨ੍ਤਰ੍ਭਾਵ <string>

// Not to include V3Option.h here so that VlcMain or other executables can use this file easily.
#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ V3OPTION_PARSER_NO_VOPTION_BOOL
class VOptionBool;
#ਜੇ_ਅੰਤ

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
ਜਨਤਕ:
    // TYPES
    class ActionIfs;
    // Functor to register options to V3OptionParser
    class AppendHelper;
    ਬਣਤਰ Impl;

ਨਿਜੀ:
    // MEMBERS
    ਅਚਲ std::unique_ptr<Impl> m_pimpl;  // Implementation class

    // METHODS
    ActionIfs* ਲੱਭੋ(ਅਚਲ ਅੱਖਰ* optp) VL_MT_DISABLED;
    ਸਾਂਚਾ <typename T_Act, typename T_Arg>
    ActionIfs& ਜੋੜੋ(ਅਚਲ string& opt, T_Arg arg) VL_MT_DISABLED;
    // Returns true if strp starts with "-fno"
    ਸਥਿਰ bool has_ਅਗੇਤਰ_FNo(ਅਚਲ ਅੱਖਰ* strp) VL_MT_DISABLED;
    // Returns true if strp starts with "-no"
    ਸਥਿਰ bool has_ਅਗੇਤਰ_No(ਅਚਲ ਅੱਖਰ* strp) VL_MT_DISABLED;

ਜਨਤਕ:
    // METHODS
    // Returns how many args are consumed. 0 means not match.
    // Also bool, true iff option is needed for rerun with --dump-inputs
    std::pair<ਪੂਰਨ, bool> parse(ਪੂਰਨ idx, ਪੂਰਨ argc, ਅੱਖਰ* argv[]) VL_MT_DISABLED;
    // Find the most similar option
    string getSuggestion(ਅਚਲ ਅੱਖਰ* str) ਅਚਲ VL_MT_DISABLED;
    ਖਾਲੀ ਜੋੜੋ_Suggestion_Candidate(ਅਚਲ string& s) VL_MT_DISABLED;
    // Call this function after all options are registered.
    ਖਾਲੀ finalize() VL_MT_DISABLED;
    ਖਾਲੀ dumpOptions() VL_MT_DISABLED;

    // CONSTRUCTORS
    V3OptionParser() VL_MT_DISABLED;
    ~V3OptionParser() VL_MT_DISABLED;
};

class V3OptionParser::ActionIfs VL_NOT_FINAL {
ਜਨਤਕ:
    ਆਭਾਸੀ ~ActionIfs() = ਮੂਲ;
    ਆਭਾਸੀ bool is_ਮੁੱਲ_Needed() ਅਚਲ = 0;  // Need val of "-opt val"
    ਆਭਾਸੀ bool isFOnOffAllowed() ਅਚਲ = 0;  // true if "-fno-opt" is allowed
    ਆਭਾਸੀ bool isNotForRerun() ਅਚਲ = 0;  // Will not be dumped with --dump-inputs
    ਆਭਾਸੀ bool isOnOffAllowed() ਅਚਲ = 0;  // true if "-no-opt" is allowed
    ਆਭਾਸੀ bool is_Partial_ਮੇਲ_Allowed() ਅਚਲ = 0;  // true if "-Wno-" matches "-Wno-fatal"
    ਆਭਾਸੀ bool isUndocumented() ਅਚਲ = 0;  // Will not be suggested in typo
    // Set a value or run callback
    ਆਭਾਸੀ ਖਾਲੀ exec(ਅਚਲ ਅੱਖਰ* optp, ਅਚਲ ਅੱਖਰ* valp) = 0;
    // Mark this option undocumented. (Exclude this option from suggestion list).
    ਆਭਾਸੀ ActionIfs& undocumented() = 0;
    // Mark this as not needed for rerunning with preprocessed sources
    ਆਭਾਸੀ ActionIfs& notForRerun() = 0;
};

// A helper class to register options
class V3OptionParser::AppendHelper final {
ਜਨਤਕ:
    // TYPES
    // Tag to specify which operator() to call
    ਬਣਤਰ FOnOff {};  // For ActionFOnOff
    ਬਣਤਰ OnOff {};  // For ActionOnOff
    ਬਣਤਰ ਸੈੱਟ_2 {};  // For ActionSet

    ਬਣਤਰ CbCall {};  // For ActionCbCall
    ਬਣਤਰ CbFOnOff {};  // For ActionCbFOnOff
    ਬਣਤਰ CbOnOff {};  // For ActionCbOnOff
    ਬਣਤਰ Cb_Partial_ਮੇਲ {};  // For ActionCbPartialMatch
    ਬਣਤਰ Cb_Partial_ਮੇਲ_Val {};  // For ActionCbPartialMatchVal
    ਬਣਤਰ CbVal {};  // For ActionCbVal

ਨਿਜੀ:
    // MEMBERS
    V3OptionParser& m_parser;  // The actual option registry

ਜਨਤਕ:
    // METHODS
    ActionIfs& ਸੰਚਾਲਕ()(ਅਚਲ ਅੱਖਰ* optp, ਸੈੱਟ_2, bool*) ਅਚਲ VL_MT_DISABLED;
#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ V3OPTION_PARSER_NO_VOPTION_BOOL
    ActionIfs& ਸੰਚਾਲਕ()(ਅਚਲ ਅੱਖਰ* optp, ਸੈੱਟ_2, VOptionBool*) ਅਚਲ VL_MT_DISABLED;
#ਜੇ_ਅੰਤ
    ActionIfs& ਸੰਚਾਲਕ()(ਅਚਲ ਅੱਖਰ* optp, ਸੈੱਟ_2, ਪੂਰਨ*) ਅਚਲ VL_MT_DISABLED;
    ActionIfs& ਸੰਚਾਲਕ()(ਅਚਲ ਅੱਖਰ* optp, ਸੈੱਟ_2, string*) ਅਚਲ VL_MT_DISABLED;

    ActionIfs& ਸੰਚਾਲਕ()(ਅਚਲ ਅੱਖਰ* optp, FOnOff, bool*) ਅਚਲ VL_MT_DISABLED;
    ActionIfs& ਸੰਚਾਲਕ()(ਅਚਲ ਅੱਖਰ* optp, OnOff, bool*) ਅਚਲ VL_MT_DISABLED;
#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ V3OPTION_PARSER_NO_VOPTION_BOOL
    ActionIfs& ਸੰਚਾਲਕ()(ਅਚਲ ਅੱਖਰ* optp, OnOff, VOptionBool*) ਅਚਲ VL_MT_DISABLED;
#ਜੇ_ਅੰਤ

    ActionIfs& ਸੰਚਾਲਕ()(ਅਚਲ ਅੱਖਰ* optp, CbCall,
                          std::function<ਖਾਲੀ(ਖਾਲੀ)>) ਅਚਲ VL_MT_DISABLED;
    ActionIfs& ਸੰਚਾਲਕ()(ਅਚਲ ਅੱਖਰ* optp, CbFOnOff,
                          std::function<ਖਾਲੀ(bool)>) ਅਚਲ VL_MT_DISABLED;
    ActionIfs& ਸੰਚਾਲਕ()(ਅਚਲ ਅੱਖਰ* optp, CbOnOff,
                          std::function<ਖਾਲੀ(bool)>) ਅਚਲ VL_MT_DISABLED;
    ActionIfs& ਸੰਚਾਲਕ()(ਅਚਲ ਅੱਖਰ* optp, CbVal, std::function<ਖਾਲੀ(ਪੂਰਨ)>) ਅਚਲ VL_MT_DISABLED;
    ActionIfs& ਸੰਚਾਲਕ()(ਅਚਲ ਅੱਖਰ* optp, CbVal,
                          std::function<ਖਾਲੀ(ਅਚਲ ਅੱਖਰ*)>) ਅਚਲ VL_MT_DISABLED;

    ActionIfs& ਸੰਚਾਲਕ()(ਅਚਲ ਅੱਖਰ* optp, Cb_Partial_ਮੇਲ,
                          std::function<ਖਾਲੀ(ਅਚਲ ਅੱਖਰ*)>) ਅਚਲ VL_MT_DISABLED;
    ActionIfs& ਸੰਚਾਲਕ()(ਅਚਲ ਅੱਖਰ* optp, Cb_Partial_ਮੇਲ_Val,
                          std::function<ਖਾਲੀ(ਅਚਲ ਅੱਖਰ*, ਅਚਲ ਅੱਖਰ*)>) ਅਚਲ VL_MT_DISABLED;

    // CONSTRUCTORS
    explicit AppendHelper(V3OptionParser& parser)
        : m_parser{parser} {}
};

#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ V3_OPTION_PARSER_DECL_ਟੈਗ_ਸੂਚੀ \
    ਅਚਲ ਸਵੈਚਾਲਿਤ ਸੈੱਟ_2 VL_ATTR_ਅਣਵਰਤਿਆ = V3OptionParser::AppendHelper::ਸੈੱਟ_2{}; \
    ਅਚਲ ਸਵੈਚਾਲਿਤ FOnOff VL_ATTR_ਅਣਵਰਤਿਆ = V3OptionParser::AppendHelper::FOnOff{}; \
    ਅਚਲ ਸਵੈਚਾਲਿਤ OnOff VL_ATTR_ਅਣਵਰਤਿਆ = V3OptionParser::AppendHelper::OnOff{}; \
    ਅਚਲ ਸਵੈਚਾਲਿਤ CbCall VL_ATTR_ਅਣਵਰਤਿਆ = V3OptionParser::AppendHelper::CbCall{}; \
    ਅਚਲ ਸਵੈਚਾਲਿਤ CbFOnOff VL_ATTR_ਅਣਵਰਤਿਆ = V3OptionParser::AppendHelper::CbFOnOff{}; \
    ਅਚਲ ਸਵੈਚਾਲਿਤ CbOnOff VL_ATTR_ਅਣਵਰਤਿਆ = V3OptionParser::AppendHelper::CbOnOff{}; \
    ਅਚਲ ਸਵੈਚਾਲਿਤ Cb_Partial_ਮੇਲ VL_ATTR_ਅਣਵਰਤਿਆ = V3OptionParser::AppendHelper::Cb_Partial_ਮੇਲ{}; \
    ਅਚਲ ਸਵੈਚਾਲਿਤ Cb_Partial_ਮੇਲ_Val VL_ATTR_ਅਣਵਰਤਿਆ \
        = V3OptionParser::AppendHelper::Cb_Partial_ਮੇਲ_Val{}; \
    ਅਚਲ ਸਵੈਚਾਲਿਤ CbVal VL_ATTR_ਅਣਵਰਤਿਆ = V3OptionParser::AppendHelper::CbVal{};

//######################################################################

#ਜੇ_ਅੰਤ  // guard
