// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Verilator Control Files (.vlt) handling
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2010-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3CONFIG_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3CONFIG_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3Error.h"
#ಅನ್ತರ್ಭಾವ "V3FileLine.h"
#ಅನ್ತರ್ಭಾವ "V3Mutex.h"

//######################################################################

class V3Control final {
ಸಾರ್ವಜನಿಕ:
    ರಚನೆ Fsm_ರಿಜಿಸ್ಟರ್_Wrapper final {
        string moduleName;
        string d;
        string q;
        string clock;
        string reset;
        string reset_ಮೌಲ್ಯ;
    };

    ಎಣಿಕೆ class Var_Spec_ಬಗೆ : uint8_t {
        PARAM,  // Select only matching parameters
        ಪೋರ್ಟ್,  // Select only matching ports
        VAR  // Select any matching AstVar (including params and ports)
    };

    ಸ್ಥಿರ ಶೂನ್ಯ ಕೂಡಿಸು_Case_ತುಂಬಿದ(ಸ್ಥಿರಾಂಕ string& file, ಪೂರ್ಣಾಂಕ lineno);
    ಸ್ಥಿರ ಶೂನ್ಯ ಕೂಡಿಸು_Case_Parallel(ಸ್ಥಿರಾಂಕ string& file, ಪೂರ್ಣಾಂಕ lineno);
    ಸ್ಥಿರ ಶೂನ್ಯ ಕೂಡಿಸು_Coverage_ಖಂಡ_Off(ಸ್ಥಿರಾಂಕ string& file, ಪೂರ್ಣಾಂಕ lineno);
    ಸ್ಥಿರ ಶೂನ್ಯ ಕೂಡಿಸು_Coverage_ಖಂಡ_Off(ಸ್ಥಿರಾಂಕ string& module, ಸ್ಥಿರಾಂಕ string& blockname);
    ಸ್ಥಿರ ಶೂನ್ಯ ಕೂಡಿಸು_Hier_Workers(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& model, ಪೂರ್ಣಾಂಕ workers);
    ಸ್ಥಿರ ಶೂನ್ಯ ಕೂಡಿಸು_Fsm_ರಿಜಿಸ್ಟರ್_Wrapper(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& module, ಸ್ಥಿರಾಂಕ string& d,
                                      ಸ್ಥಿರಾಂಕ string& q, ಸ್ಥಿರಾಂಕ string& clock, ಸ್ಥಿರಾಂಕ string& reset,
                                      ಸ್ಥಿರಾಂಕ string& reset_ಮೌಲ್ಯ);
    ಸ್ಥಿರ ಶೂನ್ಯ ಕೂಡಿಸು_Ignore(V3_Error_ಸಂಕೇತ ಸಂಕೇತ_2, bool on, ಸ್ಥಿರಾಂಕ string& filename, ಪೂರ್ಣಾಂಕ min, ಪೂರ್ಣಾಂಕ max);
    ಸ್ಥಿರ ಶೂನ್ಯ ಕೂಡಿಸು_Ignore_ಹೊಂದಿಕೆ(V3_Error_ಸಂಕೇತ ಸಂಕೇತ_2, ಸ್ಥಿರಾಂಕ string& filename, ಸ್ಥಿರಾಂಕ string& contents,
                               ಸ್ಥಿರಾಂಕ string& ಹೊಂದಿಕೆ);
    ಸ್ಥಿರ ಶೂನ್ಯ ಕೂಡಿಸು_Inline(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& module, ಸ್ಥಿರಾಂಕ string& ftask, bool on);
    ಸ್ಥಿರ ಶೂನ್ಯ ಕೂಡಿಸು_Module_Pragma(ಸ್ಥಿರಾಂಕ string& module, VPragmaType pragma);
    ಸ್ಥಿರ ಶೂನ್ಯ ಕೂಡಿಸು_Profile_ದತ್ತಾಂಶ(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& hierDpi, uint64_t ವೆಚ್ಚ);
    ಸ್ಥಿರ ಶೂನ್ಯ ಕೂಡಿಸು_Profile_ದತ್ತಾಂಶ(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& model, ಸ್ಥಿರಾಂಕ string& ಕೀಲಿ,
                               uint64_t ವೆಚ್ಚ);
    ಸ್ಥಿರ ಶೂನ್ಯ ಕೂಡಿಸು_Scope_Trace_On(bool on, ಸ್ಥಿರಾಂಕ string& scope, ಪೂರ್ಣಾಂಕ ಮಟ್ಟಗಳು);
    ಸ್ಥಿರ ಶೂನ್ಯ ಕೂಡಿಸು_Var_Attr(File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& module, ಸ್ಥಿರಾಂಕ string& ftask,
                           Var_Spec_ಬಗೆ ಬಗೆ, ಸ್ಥಿರಾಂಕ string& ನಮೂನೆ_2, VAttrType type,
                           Ast_Sen_ವೃಕ್ಷ* nodep);

    ಸ್ಥಿರ ಶೂನ್ಯ applyCase(AstCase* nodep);
    ಸ್ಥಿರ ಶೂನ್ಯ apply_Coverage_ಖಂಡ(Ast_ಗಂಟು_Module* modulep, AstBegin* nodep);
    ಸ್ಥಿರ ಶೂನ್ಯ apply_Coverage_ಖಂಡ(Ast_ಗಂಟು_Module* modulep, Ast_Gen_ಖಂಡ* nodep);
    ಸ್ಥಿರ ಶೂನ್ಯ applyFTask(Ast_ಗಂಟು_Module* modulep, Ast_ಗಂಟು_FTask* ftaskp);
    ಸ್ಥಿರ ಶೂನ್ಯ applyIgnores(File_ಸಾಲು* filelinep);
    ಸ್ಥಿರ ಶೂನ್ಯ applyModule(Ast_ಗಂಟು_Module* modulep);
    ಸ್ಥಿರ ಶೂನ್ಯ applyVarAttr(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* modulep, ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_FTask* ftaskp,
                             AstVar* varp);

    ಸ್ಥಿರ ಪೂರ್ಣಾಂಕ getHierWorkers(ಸ್ಥಿರಾಂಕ string& model);
    ಸ್ಥಿರ File_ಸಾಲು* get_Hier_Workers_File_ಸಾಲು(ಸ್ಥಿರಾಂಕ string& model);
    ಸ್ಥಿರ ಸ್ಥಿರಾಂಕ Fsm_ರಿಜಿಸ್ಟರ್_Wrapper* get_Fsm_ರಿಜಿಸ್ಟರ್_Wrapper(ಸ್ಥಿರಾಂಕ string& module);
    ಸ್ಥಿರ uint64_t get_Profile_ದತ್ತಾಂಶ(ಸ್ಥಿರಾಂಕ string& hierDpi);
    ಸ್ಥಿರ uint64_t get_Profile_ದತ್ತಾಂಶ(ಸ್ಥಿರಾಂಕ string& model, ಸ್ಥಿರಾಂಕ string& ಕೀಲಿ);
    ಸ್ಥಿರ File_ಸಾಲು* get_Profile_ದತ್ತಾಂಶ_File_ಸಾಲು();
    ಸ್ಥಿರ bool getScopeTraceOn(ಸ್ಥಿರಾಂಕ string& scope);

    ಸ್ಥಿರ ಶೂನ್ಯ contents_ತಳ್ಳು_ಪಠ್ಯ(ಸ್ಥಿರಾಂಕ string& ಪಠ್ಯ);

    ಸ್ಥಿರ bool ಒಳಗೊಂಡಿದೆ_MTask_Profile_ದತ್ತಾಂಶ();
    ಸ್ಥಿರ uint64_t get_Current_Hier_ಖಂಡ_ವೆಚ್ಚ();

    ಸ್ಥಿರ bool waive(ಸ್ಥಿರಾಂಕ File_ಸಾಲು* filelinep, V3_Error_ಸಂಕೇತ ಸಂಕೇತ_2, ಸ್ಥಿರಾಂಕ string& ಸಂದೇಶ);

    ಸ್ಥಿರ ಶೂನ್ಯ selfTest();
};

#ಆದರೆ_ಅಂತ್ಯ  // Guard
