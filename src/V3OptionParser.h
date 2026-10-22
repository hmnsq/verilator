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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव <functional>
#अन्तर्भाव <memory>
#अन्तर्भाव <string>

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
सार्वजनिक:
    // TYPES
    class ActionIfs;
    // Functor to register options to V3OptionParser
    class AppendHelper;
    संरचना Impl;

गुप्त:
    // MEMBERS
    अचर std::unique_ptr<Impl> m_pimpl;  // Implementation class

    // METHODS
    ActionIfs* find(अचर वर्ण* optp) VL_MT_DISABLED;
    ढाँचा <typename T_Act, typename T_Arg>
    ActionIfs& add(अचर string& opt, T_Arg arg) VL_MT_DISABLED;
    // Returns true if strp starts with "-fno"
    स्थिर bool hasPrefixFNo(अचर वर्ण* strp) VL_MT_DISABLED;
    // Returns true if strp starts with "-no"
    स्थिर bool hasPrefixNo(अचर वर्ण* strp) VL_MT_DISABLED;

सार्वजनिक:
    // METHODS
    // Returns how many args are consumed. 0 means not match.
    // Also bool, true iff option is needed for rerun with --dump-inputs
    std::pair<पूर्णाङ्क, bool> parse(पूर्णाङ्क idx, पूर्णाङ्क argc, वर्ण* argv[]) VL_MT_DISABLED;
    // Find the most similar option
    string getSuggestion(अचर वर्ण* str) अचर VL_MT_DISABLED;
    शून्य addSuggestionCandidate(अचर string& s) VL_MT_DISABLED;
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
    आभास bool is_मान_Needed() अचर = 0;  // Need val of "-opt val"
    आभास bool isFOnOffAllowed() अचर = 0;  // true if "-fno-opt" is allowed
    आभास bool isNotForRerun() अचर = 0;  // Will not be dumped with --dump-inputs
    आभास bool isOnOffAllowed() अचर = 0;  // true if "-no-opt" is allowed
    आभास bool isPartialMatchAllowed() अचर = 0;  // true if "-Wno-" matches "-Wno-fatal"
    आभास bool isUndocumented() अचर = 0;  // Will not be suggested in typo
    // Set a value or run callback
    आभास शून्य exec(अचर वर्ण* optp, अचर वर्ण* valp) = 0;
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
    संरचना Set {};  // For ActionSet

    संरचना CbCall {};  // For ActionCbCall
    संरचना CbFOnOff {};  // For ActionCbFOnOff
    संरचना CbOnOff {};  // For ActionCbOnOff
    संरचना CbPartialMatch {};  // For ActionCbPartialMatch
    संरचना CbPartialMatchVal {};  // For ActionCbPartialMatchVal
    संरचना CbVal {};  // For ActionCbVal

गुप्त:
    // MEMBERS
    V3OptionParser& m_parser;  // The actual option registry

सार्वजनिक:
    // METHODS
    ActionIfs& चालक()(अचर वर्ण* optp, Set, bool*) अचर VL_MT_DISABLED;
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
    ActionIfs& चालक()(अचर वर्ण* optp, Set, VOptionBool*) अचर VL_MT_DISABLED;
#endif
    ActionIfs& चालक()(अचर वर्ण* optp, Set, पूर्णाङ्क*) अचर VL_MT_DISABLED;
    ActionIfs& चालक()(अचर वर्ण* optp, Set, string*) अचर VL_MT_DISABLED;

    ActionIfs& चालक()(अचर वर्ण* optp, FOnOff, bool*) अचर VL_MT_DISABLED;
    ActionIfs& चालक()(अचर वर्ण* optp, OnOff, bool*) अचर VL_MT_DISABLED;
#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
    ActionIfs& चालक()(अचर वर्ण* optp, OnOff, VOptionBool*) अचर VL_MT_DISABLED;
#endif

    ActionIfs& चालक()(अचर वर्ण* optp, CbCall,
                          std::function<शून्य(शून्य)>) अचर VL_MT_DISABLED;
    ActionIfs& चालक()(अचर वर्ण* optp, CbFOnOff,
                          std::function<शून्य(bool)>) अचर VL_MT_DISABLED;
    ActionIfs& चालक()(अचर वर्ण* optp, CbOnOff,
                          std::function<शून्य(bool)>) अचर VL_MT_DISABLED;
    ActionIfs& चालक()(अचर वर्ण* optp, CbVal, std::function<शून्य(पूर्णाङ्क)>) अचर VL_MT_DISABLED;
    ActionIfs& चालक()(अचर वर्ण* optp, CbVal,
                          std::function<शून्य(अचर वर्ण*)>) अचर VL_MT_DISABLED;

    ActionIfs& चालक()(अचर वर्ण* optp, CbPartialMatch,
                          std::function<शून्य(अचर वर्ण*)>) अचर VL_MT_DISABLED;
    ActionIfs& चालक()(अचर वर्ण* optp, CbPartialMatchVal,
                          std::function<शून्य(अचर वर्ण*, अचर वर्ण*)>) अचर VL_MT_DISABLED;

    // CONSTRUCTORS
    explicit AppendHelper(V3OptionParser& parser)
        : m_parser{parser} {}
};

#define V3OPTION_PARSER_DECL_TAGS \
    अचर स्वतः Set VL_ATTR_UNUSED = V3OptionParser::AppendHelper::Set{}; \
    अचर स्वतः FOnOff VL_ATTR_UNUSED = V3OptionParser::AppendHelper::FOnOff{}; \
    अचर स्वतः OnOff VL_ATTR_UNUSED = V3OptionParser::AppendHelper::OnOff{}; \
    अचर स्वतः CbCall VL_ATTR_UNUSED = V3OptionParser::AppendHelper::CbCall{}; \
    अचर स्वतः CbFOnOff VL_ATTR_UNUSED = V3OptionParser::AppendHelper::CbFOnOff{}; \
    अचर स्वतः CbOnOff VL_ATTR_UNUSED = V3OptionParser::AppendHelper::CbOnOff{}; \
    अचर स्वतः CbPartialMatch VL_ATTR_UNUSED = V3OptionParser::AppendHelper::CbPartialMatch{}; \
    अचर स्वतः CbPartialMatchVal VL_ATTR_UNUSED \
        = V3OptionParser::AppendHelper::CbPartialMatchVal{}; \
    अचर स्वतः CbVal VL_ATTR_UNUSED = V3OptionParser::AppendHelper::CbVal{};

//######################################################################

#endif  // guard
