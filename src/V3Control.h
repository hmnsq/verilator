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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3Ast.h"
#அந்தர்பாவ "V3Error.h"
#அந்தர்பாவ "V3FileLine.h"
#அந்தர்பாவ "V3Mutex.h"

//######################################################################

class V3Control final {
பொது:
    கட்டமைப்பு FsmRegisterWrapper final {
        string moduleName;
        string d;
        string q;
        string clock;
        string reset;
        string reset_மதிப்பு;
    };

    எண்ணிக்கை class VarSpecKind : uint8_t {
        PARAM,  // Select only matching parameters
        PORT,  // Select only matching ports
        VAR  // Select any matching AstVar (including params and ports)
    };

    நிலையான வெறுமை addCaseFull(மாறிலி string& file, முழு lineno);
    நிலையான வெறுமை addCaseParallel(மாறிலி string& file, முழு lineno);
    நிலையான வெறுமை addCoverageBlockOff(மாறிலி string& file, முழு lineno);
    நிலையான வெறுமை addCoverageBlockOff(மாறிலி string& module, மாறிலி string& blockname);
    நிலையான வெறுமை addHierWorkers(File_வரி* fl, மாறிலி string& model, முழு workers);
    நிலையான வெறுமை addFsmRegisterWrapper(File_வரி* fl, மாறிலி string& module, மாறிலி string& d,
                                      மாறிலி string& q, மாறிலி string& clock, மாறிலி string& reset,
                                      மாறிலி string& reset_மதிப்பு);
    நிலையான வெறுமை addIgnore(V3ErrorCode code, bool on, மாறிலி string& filename, முழு min, முழு max);
    நிலையான வெறுமை addIgnoreMatch(V3ErrorCode code, மாறிலி string& filename, மாறிலி string& contents,
                               மாறிலி string& match);
    நிலையான வெறுமை addInline(File_வரி* fl, மாறிலி string& module, மாறிலி string& ftask, bool on);
    நிலையான வெறுமை addModulePragma(மாறிலி string& module, VPragmaType pragma);
    நிலையான வெறுமை addProfileData(File_வரி* fl, மாறிலி string& hierDpi, uint64_t cost);
    நிலையான வெறுமை addProfileData(File_வரி* fl, மாறிலி string& model, மாறிலி string& சாவி,
                               uint64_t cost);
    நிலையான வெறுமை addScopeTraceOn(bool on, மாறிலி string& scope, முழு levels);
    நிலையான வெறுமை addVarAttr(File_வரி* fl, மாறிலி string& module, மாறிலி string& ftask,
                           VarSpecKind kind, மாறிலி string& pattern, VAttrType type,
                           Ast_Sen_மரம்* nodep);

    நிலையான வெறுமை applyCase(AstCase* nodep);
    நிலையான வெறுமை applyCoverageBlock(Ast_முனை_Module* modulep, AstBegin* nodep);
    நிலையான வெறுமை applyCoverageBlock(Ast_முனை_Module* modulep, AstGenBlock* nodep);
    நிலையான வெறுமை applyFTask(Ast_முனை_Module* modulep, Ast_முனை_FTask* ftaskp);
    நிலையான வெறுமை applyIgnores(File_வரி* filelinep);
    நிலையான வெறுமை applyModule(Ast_முனை_Module* modulep);
    நிலையான வெறுமை applyVarAttr(மாறிலி Ast_முனை_Module* modulep, மாறிலி Ast_முனை_FTask* ftaskp,
                             AstVar* varp);

    நிலையான முழு getHierWorkers(மாறிலி string& model);
    நிலையான File_வரி* get_Hier_Workers_File_வரி(மாறிலி string& model);
    நிலையான மாறிலி FsmRegisterWrapper* getFsmRegisterWrapper(மாறிலி string& module);
    நிலையான uint64_t getProfileData(மாறிலி string& hierDpi);
    நிலையான uint64_t getProfileData(மாறிலி string& model, மாறிலி string& சாவி);
    நிலையான File_வரி* get_Profile_Data_File_வரி();
    நிலையான bool getScopeTraceOn(மாறிலி string& scope);

    நிலையான வெறுமை contents_தள்ளு_Text(மாறிலி string& text);

    நிலையான bool containsMTaskProfileData();
    நிலையான uint64_t getCurrentHierBlockCost();

    நிலையான bool waive(மாறிலி File_வரி* filelinep, V3ErrorCode code, மாறிலி string& message);

    நிலையான வெறுமை selfTest();
};

#endif  // Guard
