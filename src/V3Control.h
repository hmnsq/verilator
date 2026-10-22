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

#ifndef VERILATOR_V3CONFIG_H_
#define VERILATOR_V3CONFIG_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3Error.h"
#ಅನ್ತರ್ಭಾವ "V3FileLine.h"
#ಅನ್ತರ್ಭಾವ "V3Mutex.h"

//######################################################################

class V3Control final {
ಸಾರ್ವಜನಿಕ:
    ಸಂರಚನಾ FsmRegisterWrapper final {
        string moduleName;
        string d;
        string q;
        string clock;
        string reset;
        string reset_ಮೌಲ್ಯ;
    };

    ಗಣನಾ class VarSpecKind : uint8_t {
        PARAM,  // Select only matching parameters
        PORT,  // Select only matching ports
        VAR  // Select any matching AstVar (including params and ports)
    };

    ಸ್ಥಿರ ಶೂನ್ಯ addCaseFull(ಧ್ರುವ string& file, ಪೂರ್ಣಾಙ್ಕ lineno);
    ಸ್ಥಿರ ಶೂನ್ಯ addCaseParallel(ಧ್ರುವ string& file, ಪೂರ್ಣಾಙ್ಕ lineno);
    ಸ್ಥಿರ ಶೂನ್ಯ addCoverageBlockOff(ಧ್ರುವ string& file, ಪೂರ್ಣಾಙ್ಕ lineno);
    ಸ್ಥಿರ ಶೂನ್ಯ addCoverageBlockOff(ಧ್ರುವ string& module, ಧ್ರುವ string& blockname);
    ಸ್ಥಿರ ಶೂನ್ಯ addHierWorkers(File_ಸಾಲು* fl, ಧ್ರುವ string& model, ಪೂರ್ಣಾಙ್ಕ workers);
    ಸ್ಥಿರ ಶೂನ್ಯ addFsmRegisterWrapper(File_ಸಾಲು* fl, ಧ್ರುವ string& module, ಧ್ರುವ string& d,
                                      ಧ್ರುವ string& q, ಧ್ರುವ string& clock, ಧ್ರುವ string& reset,
                                      ಧ್ರುವ string& reset_ಮೌಲ್ಯ);
    ಸ್ಥಿರ ಶೂನ್ಯ addIgnore(V3ErrorCode code, bool on, ಧ್ರುವ string& filename, ಪೂರ್ಣಾಙ್ಕ min, ಪೂರ್ಣಾಙ್ಕ max);
    ಸ್ಥಿರ ಶೂನ್ಯ addIgnoreMatch(V3ErrorCode code, ಧ್ರುವ string& filename, ಧ್ರುವ string& contents,
                               ಧ್ರುವ string& match);
    ಸ್ಥಿರ ಶೂನ್ಯ addInline(File_ಸಾಲು* fl, ಧ್ರುವ string& module, ಧ್ರುವ string& ftask, bool on);
    ಸ್ಥಿರ ಶೂನ್ಯ addModulePragma(ಧ್ರುವ string& module, VPragmaType pragma);
    ಸ್ಥಿರ ಶೂನ್ಯ addProfileData(File_ಸಾಲು* fl, ಧ್ರುವ string& hierDpi, uint64_t cost);
    ಸ್ಥಿರ ಶೂನ್ಯ addProfileData(File_ಸಾಲು* fl, ಧ್ರುವ string& model, ಧ್ರುವ string& ಕೀಲಿ,
                               uint64_t cost);
    ಸ್ಥಿರ ಶೂನ್ಯ addScopeTraceOn(bool on, ಧ್ರುವ string& scope, ಪೂರ್ಣಾಙ್ಕ levels);
    ಸ್ಥಿರ ಶೂನ್ಯ addVarAttr(File_ಸಾಲು* fl, ಧ್ರುವ string& module, ಧ್ರುವ string& ftask,
                           VarSpecKind kind, ಧ್ರುವ string& pattern, VAttrType type,
                           Ast_Sen_ವೃಕ್ಷ* nodep);

    ಸ್ಥಿರ ಶೂನ್ಯ applyCase(AstCase* nodep);
    ಸ್ಥಿರ ಶೂನ್ಯ applyCoverageBlock(Ast_ಗಂಟು_Module* modulep, AstBegin* nodep);
    ಸ್ಥಿರ ಶೂನ್ಯ applyCoverageBlock(Ast_ಗಂಟು_Module* modulep, AstGenBlock* nodep);
    ಸ್ಥಿರ ಶೂನ್ಯ applyFTask(Ast_ಗಂಟು_Module* modulep, Ast_ಗಂಟು_FTask* ftaskp);
    ಸ್ಥಿರ ಶೂನ್ಯ applyIgnores(File_ಸಾಲು* filelinep);
    ಸ್ಥಿರ ಶೂನ್ಯ applyModule(Ast_ಗಂಟು_Module* modulep);
    ಸ್ಥಿರ ಶೂನ್ಯ applyVarAttr(ಧ್ರುವ Ast_ಗಂಟು_Module* modulep, ಧ್ರುವ Ast_ಗಂಟು_FTask* ftaskp,
                             AstVar* varp);

    ಸ್ಥಿರ ಪೂರ್ಣಾಙ್ಕ getHierWorkers(ಧ್ರುವ string& model);
    ಸ್ಥಿರ File_ಸಾಲು* get_Hier_Workers_File_ಸಾಲು(ಧ್ರುವ string& model);
    ಸ್ಥಿರ ಧ್ರುವ FsmRegisterWrapper* getFsmRegisterWrapper(ಧ್ರುವ string& module);
    ಸ್ಥಿರ uint64_t getProfileData(ಧ್ರುವ string& hierDpi);
    ಸ್ಥಿರ uint64_t getProfileData(ಧ್ರುವ string& model, ಧ್ರುವ string& ಕೀಲಿ);
    ಸ್ಥಿರ File_ಸಾಲು* get_Profile_Data_File_ಸಾಲು();
    ಸ್ಥಿರ bool getScopeTraceOn(ಧ್ರುವ string& scope);

    ಸ್ಥಿರ ಶೂನ್ಯ contents_ತಳ್ಳು_Text(ಧ್ರುವ string& text);

    ಸ್ಥಿರ bool containsMTaskProfileData();
    ಸ್ಥಿರ uint64_t getCurrentHierBlockCost();

    ಸ್ಥಿರ bool waive(ಧ್ರುವ File_ಸಾಲು* filelinep, V3ErrorCode code, ಧ್ರುವ string& message);

    ಸ್ಥಿರ ಶೂನ್ಯ selfTest();
};

#endif  // Guard
