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

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3Ast.h"
#ଅନ୍ତର୍ଭାଵ "V3Error.h"
#ଅନ୍ତର୍ଭାଵ "V3FileLine.h"
#ଅନ୍ତର୍ଭାଵ "V3Mutex.h"

//######################################################################

class V3Control final {
ସର୍ବସାଧାରଣ:
    ସଂରଚନା FsmRegisterWrapper final {
        string moduleName;
        string d;
        string q;
        string clock;
        string reset;
        string reset_ମୂଲ୍ୟ;
    };

    ଗଣନା class VarSpecKind : uint8_t {
        PARAM,  // Select only matching parameters
        PORT,  // Select only matching ports
        VAR  // Select any matching AstVar (including params and ports)
    };

    ସ୍ଥିର ଶୂନ୍ୟ addCaseFull(ଅଚଳ string& file, ପୂର୍ଣ୍ଣସଂଖ୍ୟା lineno);
    ସ୍ଥିର ଶୂନ୍ୟ addCaseParallel(ଅଚଳ string& file, ପୂର୍ଣ୍ଣସଂଖ୍ୟା lineno);
    ସ୍ଥିର ଶୂନ୍ୟ addCoverageBlockOff(ଅଚଳ string& file, ପୂର୍ଣ୍ଣସଂଖ୍ୟା lineno);
    ସ୍ଥିର ଶୂନ୍ୟ addCoverageBlockOff(ଅଚଳ string& module, ଅଚଳ string& blockname);
    ସ୍ଥିର ଶୂନ୍ୟ addHierWorkers(File_ଧାଡ଼ି* fl, ଅଚଳ string& model, ପୂର୍ଣ୍ଣସଂଖ୍ୟା workers);
    ସ୍ଥିର ଶୂନ୍ୟ addFsmRegisterWrapper(File_ଧାଡ଼ି* fl, ଅଚଳ string& module, ଅଚଳ string& d,
                                      ଅଚଳ string& q, ଅଚଳ string& clock, ଅଚଳ string& reset,
                                      ଅଚଳ string& reset_ମୂଲ୍ୟ);
    ସ୍ଥିର ଶୂନ୍ୟ addIgnore(V3ErrorCode code, bool on, ଅଚଳ string& filename, ପୂର୍ଣ୍ଣସଂଖ୍ୟା min, ପୂର୍ଣ୍ଣସଂଖ୍ୟା max);
    ସ୍ଥିର ଶୂନ୍ୟ addIgnoreMatch(V3ErrorCode code, ଅଚଳ string& filename, ଅଚଳ string& contents,
                               ଅଚଳ string& match);
    ସ୍ଥିର ଶୂନ୍ୟ addInline(File_ଧାଡ଼ି* fl, ଅଚଳ string& module, ଅଚଳ string& ftask, bool on);
    ସ୍ଥିର ଶୂନ୍ୟ addModulePragma(ଅଚଳ string& module, VPragmaType pragma);
    ସ୍ଥିର ଶୂନ୍ୟ addProfileData(File_ଧାଡ଼ି* fl, ଅଚଳ string& hierDpi, uint64_t cost);
    ସ୍ଥିର ଶୂନ୍ୟ addProfileData(File_ଧାଡ଼ି* fl, ଅଚଳ string& model, ଅଚଳ string& ଚାବି,
                               uint64_t cost);
    ସ୍ଥିର ଶୂନ୍ୟ addScopeTraceOn(bool on, ଅଚଳ string& scope, ପୂର୍ଣ୍ଣସଂଖ୍ୟା levels);
    ସ୍ଥିର ଶୂନ୍ୟ addVarAttr(File_ଧାଡ଼ି* fl, ଅଚଳ string& module, ଅଚଳ string& ftask,
                           VarSpecKind kind, ଅଚଳ string& pattern, VAttrType type,
                           Ast_Sen_ବୃକ୍ଷ* nodep);

    ସ୍ଥିର ଶୂନ୍ୟ applyCase(AstCase* nodep);
    ସ୍ଥିର ଶୂନ୍ୟ applyCoverageBlock(Ast_ଗଣ୍ଠି_Module* modulep, AstBegin* nodep);
    ସ୍ଥିର ଶୂନ୍ୟ applyCoverageBlock(Ast_ଗଣ୍ଠି_Module* modulep, AstGenBlock* nodep);
    ସ୍ଥିର ଶୂନ୍ୟ applyFTask(Ast_ଗଣ୍ଠି_Module* modulep, Ast_ଗଣ୍ଠି_FTask* ftaskp);
    ସ୍ଥିର ଶୂନ୍ୟ applyIgnores(File_ଧାଡ଼ି* filelinep);
    ସ୍ଥିର ଶୂନ୍ୟ applyModule(Ast_ଗଣ୍ଠି_Module* modulep);
    ସ୍ଥିର ଶୂନ୍ୟ applyVarAttr(ଅଚଳ Ast_ଗଣ୍ଠି_Module* modulep, ଅଚଳ Ast_ଗଣ୍ଠି_FTask* ftaskp,
                             AstVar* varp);

    ସ୍ଥିର ପୂର୍ଣ୍ଣସଂଖ୍ୟା getHierWorkers(ଅଚଳ string& model);
    ସ୍ଥିର File_ଧାଡ଼ି* get_Hier_Workers_File_ଧାଡ଼ି(ଅଚଳ string& model);
    ସ୍ଥିର ଅଚଳ FsmRegisterWrapper* getFsmRegisterWrapper(ଅଚଳ string& module);
    ସ୍ଥିର uint64_t getProfileData(ଅଚଳ string& hierDpi);
    ସ୍ଥିର uint64_t getProfileData(ଅଚଳ string& model, ଅଚଳ string& ଚାବି);
    ସ୍ଥିର File_ଧାଡ଼ି* get_Profile_Data_File_ଧାଡ଼ି();
    ସ୍ଥିର bool getScopeTraceOn(ଅଚଳ string& scope);

    ସ୍ଥିର ଶୂନ୍ୟ contents_ଠେଲ_Text(ଅଚଳ string& text);

    ସ୍ଥିର bool containsMTaskProfileData();
    ସ୍ଥିର uint64_t getCurrentHierBlockCost();

    ସ୍ଥିର bool waive(ଅଚଳ File_ଧାଡ଼ି* filelinep, V3ErrorCode code, ଅଚଳ string& message);

    ସ୍ଥିର ଶୂନ୍ୟ selfTest();
};

#endif  // Guard
