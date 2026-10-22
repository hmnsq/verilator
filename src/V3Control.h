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

#અન્તર્ભાવ "config_build.h"
#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ "V3Ast.h"
#અન્તર્ભાવ "V3Error.h"
#અન્તર્ભાવ "V3FileLine.h"
#અન્તર્ભાવ "V3Mutex.h"

//######################################################################

class V3Control final {
જાહેર:
    સંરચના FsmRegisterWrapper final {
        string moduleName;
        string d;
        string q;
        string clock;
        string reset;
        string reset_કિંમત;
    };

    ગણના class VarSpecKind : uint8_t {
        PARAM,  // Select only matching parameters
        PORT,  // Select only matching ports
        VAR  // Select any matching AstVar (including params and ports)
    };

    સ્થિર શૂન્ય addCaseFull(અચલ string& file, પૂર્ણાંક lineno);
    સ્થિર શૂન્ય addCaseParallel(અચલ string& file, પૂર્ણાંક lineno);
    સ્થિર શૂન્ય addCoverageBlockOff(અચલ string& file, પૂર્ણાંક lineno);
    સ્થિર શૂન્ય addCoverageBlockOff(અચલ string& module, અચલ string& blockname);
    સ્થિર શૂન્ય addHierWorkers(File_લીટી* fl, અચલ string& model, પૂર્ણાંક workers);
    સ્થિર શૂન્ય addFsmRegisterWrapper(File_લીટી* fl, અચલ string& module, અચલ string& d,
                                      અચલ string& q, અચલ string& clock, અચલ string& reset,
                                      અચલ string& reset_કિંમત);
    સ્થિર શૂન્ય addIgnore(V3ErrorCode code, bool on, અચલ string& filename, પૂર્ણાંક min, પૂર્ણાંક max);
    સ્થિર શૂન્ય addIgnoreMatch(V3ErrorCode code, અચલ string& filename, અચલ string& contents,
                               અચલ string& match);
    સ્થિર શૂન્ય addInline(File_લીટી* fl, અચલ string& module, અચલ string& ftask, bool on);
    સ્થિર શૂન્ય addModulePragma(અચલ string& module, VPragmaType pragma);
    સ્થિર શૂન્ય addProfileData(File_લીટી* fl, અચલ string& hierDpi, uint64_t cost);
    સ્થિર શૂન્ય addProfileData(File_લીટી* fl, અચલ string& model, અચલ string& ચાવી,
                               uint64_t cost);
    સ્થિર શૂન્ય addScopeTraceOn(bool on, અચલ string& scope, પૂર્ણાંક levels);
    સ્થિર શૂન્ય addVarAttr(File_લીટી* fl, અચલ string& module, અચલ string& ftask,
                           VarSpecKind kind, અચલ string& pattern, VAttrType type,
                           Ast_Sen_વૃક્ષ* nodep);

    સ્થિર શૂન્ય applyCase(AstCase* nodep);
    સ્થિર શૂન્ય applyCoverageBlock(Ast_ગાંઠ_Module* modulep, AstBegin* nodep);
    સ્થિર શૂન્ય applyCoverageBlock(Ast_ગાંઠ_Module* modulep, AstGenBlock* nodep);
    સ્થિર શૂન્ય applyFTask(Ast_ગાંઠ_Module* modulep, Ast_ગાંઠ_FTask* ftaskp);
    સ્થિર શૂન્ય applyIgnores(File_લીટી* filelinep);
    સ્થિર શૂન્ય applyModule(Ast_ગાંઠ_Module* modulep);
    સ્થિર શૂન્ય applyVarAttr(અચલ Ast_ગાંઠ_Module* modulep, અચલ Ast_ગાંઠ_FTask* ftaskp,
                             AstVar* varp);

    સ્થિર પૂર્ણાંક getHierWorkers(અચલ string& model);
    સ્થિર File_લીટી* get_Hier_Workers_File_લીટી(અચલ string& model);
    સ્થિર અચલ FsmRegisterWrapper* getFsmRegisterWrapper(અચલ string& module);
    સ્થિર uint64_t getProfileData(અચલ string& hierDpi);
    સ્થિર uint64_t getProfileData(અચલ string& model, અચલ string& ચાવી);
    સ્થિર File_લીટી* get_Profile_Data_File_લીટી();
    સ્થિર bool getScopeTraceOn(અચલ string& scope);

    સ્થિર શૂન્ય contents_ધકેલો_Text(અચલ string& text);

    સ્થિર bool containsMTaskProfileData();
    સ્થિર uint64_t getCurrentHierBlockCost();

    સ્થિર bool waive(અચલ File_લીટી* filelinep, V3ErrorCode code, અચલ string& message);

    સ્થિર શૂન્ય selfTest();
};

#endif  // Guard
