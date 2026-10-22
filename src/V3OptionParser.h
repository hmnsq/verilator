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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3OPTION_PARSER_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3OPTION_PARSER_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ <functional>
#ಅನ್ತರ್ಭಾವ <memory>
#ಅನ್ತರ್ಭಾವ <string>

// Not to include V3Option.h here so that VlcMain or other executables can use this file easily.
#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ V3OPTION_PARSER_NO_VOPTION_BOOL
class VOptionBool;
#ಆದರೆ_ಅಂತ್ಯ

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
    ರಚನೆ Impl;

ಖಾಸಗಿ:
    // MEMBERS
    ಸ್ಥಿರಾಂಕ std::unique_ptr<Impl> m_pimpl;  // Implementation class

    // METHODS
    ActionIfs* ಕಂಡುಹಿಡಿ(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* optp) VL_MT_DISABLED;
    ಮಾದರಿ <typename T_Act, typename T_Arg>
    ActionIfs& ಕೂಡಿಸು(ಸ್ಥಿರಾಂಕ string& opt, T_Arg arg) VL_MT_DISABLED;
    // Returns true if strp starts with "-fno"
    ಸ್ಥಿರ bool has_ಪೂರ್ವಪ್ರತ್ಯಯ_FNo(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* strp) VL_MT_DISABLED;
    // Returns true if strp starts with "-no"
    ಸ್ಥಿರ bool has_ಪೂರ್ವಪ್ರತ್ಯಯ_No(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* strp) VL_MT_DISABLED;

ಸಾರ್ವಜನಿಕ:
    // METHODS
    // Returns how many args are consumed. 0 means not match.
    // Also bool, true iff option is needed for rerun with --dump-inputs
    std::pair<ಪೂರ್ಣಾಂಕ, bool> parse(ಪೂರ್ಣಾಂಕ idx, ಪೂರ್ಣಾಂಕ argc, ಅಕ್ಷರ* argv[]) VL_MT_DISABLED;
    // Find the most similar option
    string getSuggestion(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* str) ಸ್ಥಿರಾಂಕ VL_MT_DISABLED;
    ಶೂನ್ಯ ಕೂಡಿಸು_Suggestion_Candidate(ಸ್ಥಿರಾಂಕ string& s) VL_MT_DISABLED;
    // Call this function after all options are registered.
    ಶೂನ್ಯ finalize() VL_MT_DISABLED;
    ಶೂನ್ಯ dumpOptions() VL_MT_DISABLED;

    // CONSTRUCTORS
    V3OptionParser() VL_MT_DISABLED;
    ~V3OptionParser() VL_MT_DISABLED;
};

class V3OptionParser::ActionIfs VL_NOT_FINAL {
ಸಾರ್ವಜನಿಕ:
    ವಾಸ್ತವಿಕ ~ActionIfs() = ಪೂರ್ವನಿಯೋಜಿತ;
    ವಾಸ್ತವಿಕ bool is_ಮೌಲ್ಯ_Needed() ಸ್ಥಿರಾಂಕ = 0;  // Need val of "-opt val"
    ವಾಸ್ತವಿಕ bool isFOnOffAllowed() ಸ್ಥಿರಾಂಕ = 0;  // true if "-fno-opt" is allowed
    ವಾಸ್ತವಿಕ bool isNotForRerun() ಸ್ಥಿರಾಂಕ = 0;  // Will not be dumped with --dump-inputs
    ವಾಸ್ತವಿಕ bool isOnOffAllowed() ಸ್ಥಿರಾಂಕ = 0;  // true if "-no-opt" is allowed
    ವಾಸ್ತವಿಕ bool is_Partial_ಹೊಂದಿಕೆ_Allowed() ಸ್ಥಿರಾಂಕ = 0;  // true if "-Wno-" matches "-Wno-fatal"
    ವಾಸ್ತವಿಕ bool isUndocumented() ಸ್ಥಿರಾಂಕ = 0;  // Will not be suggested in typo
    // Set a value or run callback
    ವಾಸ್ತವಿಕ ಶೂನ್ಯ exec(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* optp, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* valp) = 0;
    // Mark this option undocumented. (Exclude this option from suggestion list).
    ವಾಸ್ತವಿಕ ActionIfs& undocumented() = 0;
    // Mark this as not needed for rerunning with preprocessed sources
    ವಾಸ್ತವಿಕ ActionIfs& notForRerun() = 0;
};

// A helper class to register options
class V3OptionParser::AppendHelper final {
ಸಾರ್ವಜನಿಕ:
    // TYPES
    // Tag to specify which operator() to call
    ರಚನೆ FOnOff {};  // For ActionFOnOff
    ರಚನೆ OnOff {};  // For ActionOnOff
    ರಚನೆ ಗಣ_2 {};  // For ActionSet

    ರಚನೆ CbCall {};  // For ActionCbCall
    ರಚನೆ CbFOnOff {};  // For ActionCbFOnOff
    ರಚನೆ CbOnOff {};  // For ActionCbOnOff
    ರಚನೆ Cb_Partial_ಹೊಂದಿಕೆ {};  // For ActionCbPartialMatch
    ರಚನೆ Cb_Partial_ಹೊಂದಿಕೆ_Val {};  // For ActionCbPartialMatchVal
    ರಚನೆ CbVal {};  // For ActionCbVal

ಖಾಸಗಿ:
    // MEMBERS
    V3OptionParser& m_parser;  // The actual option registry

ಸಾರ್ವಜನಿಕ:
    // METHODS
    ActionIfs& ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* optp, ಗಣ_2, bool*) ಸ್ಥಿರಾಂಕ VL_MT_DISABLED;
#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ V3OPTION_PARSER_NO_VOPTION_BOOL
    ActionIfs& ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* optp, ಗಣ_2, VOptionBool*) ಸ್ಥಿರಾಂಕ VL_MT_DISABLED;
#ಆದರೆ_ಅಂತ್ಯ
    ActionIfs& ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* optp, ಗಣ_2, ಪೂರ್ಣಾಂಕ*) ಸ್ಥಿರಾಂಕ VL_MT_DISABLED;
    ActionIfs& ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* optp, ಗಣ_2, string*) ಸ್ಥಿರಾಂಕ VL_MT_DISABLED;

    ActionIfs& ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* optp, FOnOff, bool*) ಸ್ಥಿರಾಂಕ VL_MT_DISABLED;
    ActionIfs& ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* optp, OnOff, bool*) ಸ್ಥಿರಾಂಕ VL_MT_DISABLED;
#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ V3OPTION_PARSER_NO_VOPTION_BOOL
    ActionIfs& ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* optp, OnOff, VOptionBool*) ಸ್ಥಿರಾಂಕ VL_MT_DISABLED;
#ಆದರೆ_ಅಂತ್ಯ

    ActionIfs& ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* optp, CbCall,
                          std::function<ಶೂನ್ಯ(ಶೂನ್ಯ)>) ಸ್ಥಿರಾಂಕ VL_MT_DISABLED;
    ActionIfs& ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* optp, CbFOnOff,
                          std::function<ಶೂನ್ಯ(bool)>) ಸ್ಥಿರಾಂಕ VL_MT_DISABLED;
    ActionIfs& ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* optp, CbOnOff,
                          std::function<ಶೂನ್ಯ(bool)>) ಸ್ಥಿರಾಂಕ VL_MT_DISABLED;
    ActionIfs& ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* optp, CbVal, std::function<ಶೂನ್ಯ(ಪೂರ್ಣಾಂಕ)>) ಸ್ಥಿರಾಂಕ VL_MT_DISABLED;
    ActionIfs& ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* optp, CbVal,
                          std::function<ಶೂನ್ಯ(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ*)>) ಸ್ಥಿರಾಂಕ VL_MT_DISABLED;

    ActionIfs& ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* optp, Cb_Partial_ಹೊಂದಿಕೆ,
                          std::function<ಶೂನ್ಯ(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ*)>) ಸ್ಥಿರಾಂಕ VL_MT_DISABLED;
    ActionIfs& ಕ್ರಿಯಾಕಾರಕ()(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* optp, Cb_Partial_ಹೊಂದಿಕೆ_Val,
                          std::function<ಶೂನ್ಯ(ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ*, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ*)>) ಸ್ಥಿರಾಂಕ VL_MT_DISABLED;

    // CONSTRUCTORS
    explicit AppendHelper(V3OptionParser& parser)
        : m_parser{parser} {}
};

#ವ್ಯಾಖ್ಯಾನಿಸು V3_OPTION_PARSER_DECL_ಟ್ಯಾಗುಗಳು \
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ ಗಣ_2 VL_ATTR_ಬಳಸದ = V3OptionParser::AppendHelper::ಗಣ_2{}; \
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ FOnOff VL_ATTR_ಬಳಸದ = V3OptionParser::AppendHelper::FOnOff{}; \
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ OnOff VL_ATTR_ಬಳಸದ = V3OptionParser::AppendHelper::OnOff{}; \
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ CbCall VL_ATTR_ಬಳಸದ = V3OptionParser::AppendHelper::CbCall{}; \
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ CbFOnOff VL_ATTR_ಬಳಸದ = V3OptionParser::AppendHelper::CbFOnOff{}; \
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ CbOnOff VL_ATTR_ಬಳಸದ = V3OptionParser::AppendHelper::CbOnOff{}; \
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ Cb_Partial_ಹೊಂದಿಕೆ VL_ATTR_ಬಳಸದ = V3OptionParser::AppendHelper::Cb_Partial_ಹೊಂದಿಕೆ{}; \
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ Cb_Partial_ಹೊಂದಿಕೆ_Val VL_ATTR_ಬಳಸದ \
        = V3OptionParser::AppendHelper::Cb_Partial_ಹೊಂದಿಕೆ_Val{}; \
    ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ CbVal VL_ATTR_ಬಳಸದ = V3OptionParser::AppendHelper::CbVal{};

//######################################################################

#ಆದರೆ_ಅಂತ್ಯ  // guard
