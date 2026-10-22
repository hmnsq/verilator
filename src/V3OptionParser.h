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

#यदि_अपरिभाषितम् VERILATOR_V3OPTION_PARSER_H_
#परिभाषय VERILATOR_V3OPTION_PARSER_H_

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव <functional>
#अन्तर्भाव <memory>
#अन्तर्भाव <string>

// Not to include V3Option.h here so that VlcMain or other executables can use this file easily.
#यदि_अपरिभाषितम् V3OPTION_PARSER_NO_VOPTION_BOOL
class VOptionBool;
#यदि_अन्तः

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
सार्वजनिक:
    // TYPES
    class ActionIfs;
    // Functor to register options to V3OptionParser
    class AppendHelper;
    संरचना Impl;

गुप्त:
    // MEMBERS
    ध्रुव std::unique_ptr<Impl> m_pimpl;  // Implementation class

    // METHODS
    ActionIfs* find(ध्रुव वर्ण* optp) VL_MT_DISABLED;
    प्रतिरूपम् <typename T_Act, typename T_Arg>
    ActionIfs& सङ्कलय(ध्रुव string& opt, T_Arg arg) VL_MT_DISABLED;
    // Returns true if strp starts with "-fno"
    स्थिर bool has_उपसर्गः_FNo(ध्रुव वर्ण* strp) VL_MT_DISABLED;
    // Returns true if strp starts with "-no"
    स्थिर bool has_उपसर्गः_No(ध्रुव वर्ण* strp) VL_MT_DISABLED;

सार्वजनिक:
    // METHODS
    // Returns how many args are consumed. 0 means not match.
    // Also bool, true iff option is needed for rerun with --dump-inputs
    std::pair<पूर्णाङ्क, bool> parse(पूर्णाङ्क idx, पूर्णाङ्क argc, वर्ण* argv[]) VL_MT_DISABLED;
    // Find the most similar option
    string getSuggestion(ध्रुव वर्ण* str) ध्रुव VL_MT_DISABLED;
    शून्य सङ्कलय_Suggestion_Candidate(ध्रुव string& s) VL_MT_DISABLED;
    // Call this function after all options are registered.
    शून्य finalize() VL_MT_DISABLED;
    शून्य dumpOptions() VL_MT_DISABLED;

    // CONSTRUCTORS
    V3OptionParser() VL_MT_DISABLED;
    ~V3OptionParser() VL_MT_DISABLED;
};

class V3OptionParser::ActionIfs VL_NOT_FINAL {
सार्वजनिक:
    आभास ~ActionIfs() = स्वभाव;
    आभास bool is_मानम्_Needed() ध्रुव = 0;  // Need val of "-opt val"
    आभास bool isFOnOffAllowed() ध्रुव = 0;  // true if "-fno-opt" is allowed
    आभास bool isNotForRerun() ध्रुव = 0;  // Will not be dumped with --dump-inputs
    आभास bool isOnOffAllowed() ध्रुव = 0;  // true if "-no-opt" is allowed
    आभास bool is_Partial_मेलनम्_Allowed() ध्रुव = 0;  // true if "-Wno-" matches "-Wno-fatal"
    आभास bool isUndocumented() ध्रुव = 0;  // Will not be suggested in typo
    // Set a value or run callback
    आभास शून्य exec(ध्रुव वर्ण* optp, ध्रुव वर्ण* valp) = 0;
    // Mark this option undocumented. (Exclude this option from suggestion list).
    आभास ActionIfs& undocumented() = 0;
    // Mark this as not needed for rerunning with preprocessed sources
    आभास ActionIfs& notForRerun() = 0;
};

// A helper class to register options
class V3OptionParser::AppendHelper final {
सार्वजनिक:
    // TYPES
    // Tag to specify which operator() to call
    संरचना FOnOff {};  // For ActionFOnOff
    संरचना OnOff {};  // For ActionOnOff
    संरचना समुच्चयः_2 {};  // For ActionSet

    संरचना CbCall {};  // For ActionCbCall
    संरचना CbFOnOff {};  // For ActionCbFOnOff
    संरचना CbOnOff {};  // For ActionCbOnOff
    संरचना Cb_Partial_मेलनम् {};  // For ActionCbPartialMatch
    संरचना Cb_Partial_मेलनम्_Val {};  // For ActionCbPartialMatchVal
    संरचना CbVal {};  // For ActionCbVal

गुप्त:
    // MEMBERS
    V3OptionParser& m_parser;  // The actual option registry

सार्वजनिक:
    // METHODS
    ActionIfs& संक्रियकः()(ध्रुव वर्ण* optp, समुच्चयः_2, bool*) ध्रुव VL_MT_DISABLED;
#यदि_अपरिभाषितम् V3OPTION_PARSER_NO_VOPTION_BOOL
    ActionIfs& संक्रियकः()(ध्रुव वर्ण* optp, समुच्चयः_2, VOptionBool*) ध्रुव VL_MT_DISABLED;
#यदि_अन्तः
    ActionIfs& संक्रियकः()(ध्रुव वर्ण* optp, समुच्चयः_2, पूर्णाङ्क*) ध्रुव VL_MT_DISABLED;
    ActionIfs& संक्रियकः()(ध्रुव वर्ण* optp, समुच्चयः_2, string*) ध्रुव VL_MT_DISABLED;

    ActionIfs& संक्रियकः()(ध्रुव वर्ण* optp, FOnOff, bool*) ध्रुव VL_MT_DISABLED;
    ActionIfs& संक्रियकः()(ध्रुव वर्ण* optp, OnOff, bool*) ध्रुव VL_MT_DISABLED;
#यदि_अपरिभाषितम् V3OPTION_PARSER_NO_VOPTION_BOOL
    ActionIfs& संक्रियकः()(ध्रुव वर्ण* optp, OnOff, VOptionBool*) ध्रुव VL_MT_DISABLED;
#यदि_अन्तः

    ActionIfs& संक्रियकः()(ध्रुव वर्ण* optp, CbCall,
                          std::function<शून्य(शून्य)>) ध्रुव VL_MT_DISABLED;
    ActionIfs& संक्रियकः()(ध्रुव वर्ण* optp, CbFOnOff,
                          std::function<शून्य(bool)>) ध्रुव VL_MT_DISABLED;
    ActionIfs& संक्रियकः()(ध्रुव वर्ण* optp, CbOnOff,
                          std::function<शून्य(bool)>) ध्रुव VL_MT_DISABLED;
    ActionIfs& संक्रियकः()(ध्रुव वर्ण* optp, CbVal, std::function<शून्य(पूर्णाङ्क)>) ध्रुव VL_MT_DISABLED;
    ActionIfs& संक्रियकः()(ध्रुव वर्ण* optp, CbVal,
                          std::function<शून्य(ध्रुव वर्ण*)>) ध्रुव VL_MT_DISABLED;

    ActionIfs& संक्रियकः()(ध्रुव वर्ण* optp, Cb_Partial_मेलनम्,
                          std::function<शून्य(ध्रुव वर्ण*)>) ध्रुव VL_MT_DISABLED;
    ActionIfs& संक्रियकः()(ध्रुव वर्ण* optp, Cb_Partial_मेलनम्_Val,
                          std::function<शून्य(ध्रुव वर्ण*, ध्रुव वर्ण*)>) ध्रुव VL_MT_DISABLED;

    // CONSTRUCTORS
    explicit AppendHelper(V3OptionParser& parser)
        : m_parser{parser} {}
};

#परिभाषय V3_OPTION_PARSER_DECL_चिह्नकानि \
    ध्रुव स्वचालित समुच्चयः_2 VL_ATTR_अप्रयुक्तम् = V3OptionParser::AppendHelper::समुच्चयः_2{}; \
    ध्रुव स्वचालित FOnOff VL_ATTR_अप्रयुक्तम् = V3OptionParser::AppendHelper::FOnOff{}; \
    ध्रुव स्वचालित OnOff VL_ATTR_अप्रयुक्तम् = V3OptionParser::AppendHelper::OnOff{}; \
    ध्रुव स्वचालित CbCall VL_ATTR_अप्रयुक्तम् = V3OptionParser::AppendHelper::CbCall{}; \
    ध्रुव स्वचालित CbFOnOff VL_ATTR_अप्रयुक्तम् = V3OptionParser::AppendHelper::CbFOnOff{}; \
    ध्रुव स्वचालित CbOnOff VL_ATTR_अप्रयुक्तम् = V3OptionParser::AppendHelper::CbOnOff{}; \
    ध्रुव स्वचालित Cb_Partial_मेलनम् VL_ATTR_अप्रयुक्तम् = V3OptionParser::AppendHelper::Cb_Partial_मेलनम्{}; \
    ध्रुव स्वचालित Cb_Partial_मेलनम्_Val VL_ATTR_अप्रयुक्तम् \
        = V3OptionParser::AppendHelper::Cb_Partial_मेलनम्_Val{}; \
    ध्रुव स्वचालित CbVal VL_ATTR_अप्रयुक्तम् = V3OptionParser::AppendHelper::CbVal{};

//######################################################################

#यदि_अन्तः  // guard
