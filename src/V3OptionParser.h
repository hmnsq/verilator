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

#నిర్వచితం_కాకపోతే VERILATOR_V3OPTION_PARSER_H_
#నిర్వచించు VERILATOR_V3OPTION_PARSER_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ <functional>
#అన్తర్భావ <memory>
#అన్తర్భావ <string>

// Not to include V3Option.h here so that VlcMain or other executables can use this file easily.
#నిర్వచితం_కాకపోతే V3OPTION_PARSER_NO_VOPTION_BOOL
class VOptionBool;
#అయితే_ముగింపు

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
బహిరంగ:
    // TYPES
    class ActionIfs;
    // Functor to register options to V3OptionParser
    class AppendHelper;
    నిర్మాణం Impl;

అంతరంగ:
    // MEMBERS
    స్థిరరాశి std::unique_ptr<Impl> m_pimpl;  // Implementation class

    // METHODS
    ActionIfs* కనుగొను(స్థిరరాశి అక్షరం* optp) VL_MT_DISABLED;
    మూస <typename T_Act, typename T_Arg>
    ActionIfs& కూడు(స్థిరరాశి string& opt, T_Arg arg) VL_MT_DISABLED;
    // Returns true if strp starts with "-fno"
    స్థిర bool has_ఉపసర్గ_FNo(స్థిరరాశి అక్షరం* strp) VL_MT_DISABLED;
    // Returns true if strp starts with "-no"
    స్థిర bool has_ఉపసర్గ_No(స్థిరరాశి అక్షరం* strp) VL_MT_DISABLED;

బహిరంగ:
    // METHODS
    // Returns how many args are consumed. 0 means not match.
    // Also bool, true iff option is needed for rerun with --dump-inputs
    std::pair<పూర్ణాంకం, bool> parse(పూర్ణాంకం idx, పూర్ణాంకం argc, అక్షరం* argv[]) VL_MT_DISABLED;
    // Find the most similar option
    string getSuggestion(స్థిరరాశి అక్షరం* str) స్థిరరాశి VL_MT_DISABLED;
    శూన్య కూడు_Suggestion_Candidate(స్థిరరాశి string& s) VL_MT_DISABLED;
    // Call this function after all options are registered.
    శూన్య finalize() VL_MT_DISABLED;
    శూన్య dumpOptions() VL_MT_DISABLED;

    // CONSTRUCTORS
    V3OptionParser() VL_MT_DISABLED;
    ~V3OptionParser() VL_MT_DISABLED;
};

class V3OptionParser::ActionIfs VL_NOT_FINAL {
బహిరంగ:
    మాయా ~ActionIfs() = అప్రమేయ;
    మాయా bool is_విలువ_Needed() స్థిరరాశి = 0;  // Need val of "-opt val"
    మాయా bool isFOnOffAllowed() స్థిరరాశి = 0;  // true if "-fno-opt" is allowed
    మాయా bool isNotForRerun() స్థిరరాశి = 0;  // Will not be dumped with --dump-inputs
    మాయా bool isOnOffAllowed() స్థిరరాశి = 0;  // true if "-no-opt" is allowed
    మాయా bool is_Partial_సరిపోలిక_Allowed() స్థిరరాశి = 0;  // true if "-Wno-" matches "-Wno-fatal"
    మాయా bool isUndocumented() స్థిరరాశి = 0;  // Will not be suggested in typo
    // Set a value or run callback
    మాయా శూన్య exec(స్థిరరాశి అక్షరం* optp, స్థిరరాశి అక్షరం* valp) = 0;
    // Mark this option undocumented. (Exclude this option from suggestion list).
    మాయా ActionIfs& undocumented() = 0;
    // Mark this as not needed for rerunning with preprocessed sources
    మాయా ActionIfs& notForRerun() = 0;
};

// A helper class to register options
class V3OptionParser::AppendHelper final {
బహిరంగ:
    // TYPES
    // Tag to specify which operator() to call
    నిర్మాణం FOnOff {};  // For ActionFOnOff
    నిర్మాణం OnOff {};  // For ActionOnOff
    నిర్మాణం సమితి_2 {};  // For ActionSet

    నిర్మాణం CbCall {};  // For ActionCbCall
    నిర్మాణం CbFOnOff {};  // For ActionCbFOnOff
    నిర్మాణం CbOnOff {};  // For ActionCbOnOff
    నిర్మాణం Cb_Partial_సరిపోలిక {};  // For ActionCbPartialMatch
    నిర్మాణం Cb_Partial_సరిపోలిక_Val {};  // For ActionCbPartialMatchVal
    నిర్మాణం CbVal {};  // For ActionCbVal

అంతరంగ:
    // MEMBERS
    V3OptionParser& m_parser;  // The actual option registry

బహిరంగ:
    // METHODS
    ActionIfs& క్రియాకారకం()(స్థిరరాశి అక్షరం* optp, సమితి_2, bool*) స్థిరరాశి VL_MT_DISABLED;
#నిర్వచితం_కాకపోతే V3OPTION_PARSER_NO_VOPTION_BOOL
    ActionIfs& క్రియాకారకం()(స్థిరరాశి అక్షరం* optp, సమితి_2, VOptionBool*) స్థిరరాశి VL_MT_DISABLED;
#అయితే_ముగింపు
    ActionIfs& క్రియాకారకం()(స్థిరరాశి అక్షరం* optp, సమితి_2, పూర్ణాంకం*) స్థిరరాశి VL_MT_DISABLED;
    ActionIfs& క్రియాకారకం()(స్థిరరాశి అక్షరం* optp, సమితి_2, string*) స్థిరరాశి VL_MT_DISABLED;

    ActionIfs& క్రియాకారకం()(స్థిరరాశి అక్షరం* optp, FOnOff, bool*) స్థిరరాశి VL_MT_DISABLED;
    ActionIfs& క్రియాకారకం()(స్థిరరాశి అక్షరం* optp, OnOff, bool*) స్థిరరాశి VL_MT_DISABLED;
#నిర్వచితం_కాకపోతే V3OPTION_PARSER_NO_VOPTION_BOOL
    ActionIfs& క్రియాకారకం()(స్థిరరాశి అక్షరం* optp, OnOff, VOptionBool*) స్థిరరాశి VL_MT_DISABLED;
#అయితే_ముగింపు

    ActionIfs& క్రియాకారకం()(స్థిరరాశి అక్షరం* optp, CbCall,
                          std::function<శూన్య(శూన్య)>) స్థిరరాశి VL_MT_DISABLED;
    ActionIfs& క్రియాకారకం()(స్థిరరాశి అక్షరం* optp, CbFOnOff,
                          std::function<శూన్య(bool)>) స్థిరరాశి VL_MT_DISABLED;
    ActionIfs& క్రియాకారకం()(స్థిరరాశి అక్షరం* optp, CbOnOff,
                          std::function<శూన్య(bool)>) స్థిరరాశి VL_MT_DISABLED;
    ActionIfs& క్రియాకారకం()(స్థిరరాశి అక్షరం* optp, CbVal, std::function<శూన్య(పూర్ణాంకం)>) స్థిరరాశి VL_MT_DISABLED;
    ActionIfs& క్రియాకారకం()(స్థిరరాశి అక్షరం* optp, CbVal,
                          std::function<శూన్య(స్థిరరాశి అక్షరం*)>) స్థిరరాశి VL_MT_DISABLED;

    ActionIfs& క్రియాకారకం()(స్థిరరాశి అక్షరం* optp, Cb_Partial_సరిపోలిక,
                          std::function<శూన్య(స్థిరరాశి అక్షరం*)>) స్థిరరాశి VL_MT_DISABLED;
    ActionIfs& క్రియాకారకం()(స్థిరరాశి అక్షరం* optp, Cb_Partial_సరిపోలిక_Val,
                          std::function<శూన్య(స్థిరరాశి అక్షరం*, స్థిరరాశి అక్షరం*)>) స్థిరరాశి VL_MT_DISABLED;

    // CONSTRUCTORS
    explicit AppendHelper(V3OptionParser& parser)
        : m_parser{parser} {}
};

#నిర్వచించు V3_OPTION_PARSER_DECL_ట్యాగులు \
    స్థిరరాశి స్వయంచాలక సమితి_2 VL_ATTR_వాడని = V3OptionParser::AppendHelper::సమితి_2{}; \
    స్థిరరాశి స్వయంచాలక FOnOff VL_ATTR_వాడని = V3OptionParser::AppendHelper::FOnOff{}; \
    స్థిరరాశి స్వయంచాలక OnOff VL_ATTR_వాడని = V3OptionParser::AppendHelper::OnOff{}; \
    స్థిరరాశి స్వయంచాలక CbCall VL_ATTR_వాడని = V3OptionParser::AppendHelper::CbCall{}; \
    స్థిరరాశి స్వయంచాలక CbFOnOff VL_ATTR_వాడని = V3OptionParser::AppendHelper::CbFOnOff{}; \
    స్థిరరాశి స్వయంచాలక CbOnOff VL_ATTR_వాడని = V3OptionParser::AppendHelper::CbOnOff{}; \
    స్థిరరాశి స్వయంచాలక Cb_Partial_సరిపోలిక VL_ATTR_వాడని = V3OptionParser::AppendHelper::Cb_Partial_సరిపోలిక{}; \
    స్థిరరాశి స్వయంచాలక Cb_Partial_సరిపోలిక_Val VL_ATTR_వాడని \
        = V3OptionParser::AppendHelper::Cb_Partial_సరిపోలిక_Val{}; \
    స్థిరరాశి స్వయంచాలక CbVal VL_ATTR_వాడని = V3OptionParser::AppendHelper::CbVal{};

//######################################################################

#అయితే_ముగింపు  // guard
