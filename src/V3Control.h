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

#অন্তর্ভাব "config_build.h"
#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব "V3Ast.h"
#অন্তর্ভাব "V3Error.h"
#অন্তর্ভাব "V3FileLine.h"
#অন্তর্ভাব "V3Mutex.h"

//######################################################################

class V3Control final {
সর্বজনীন:
    কাঠামো FsmRegisterWrapper final {
        string moduleName;
        string d;
        string q;
        string clock;
        string reset;
        string reset_মান;
    };

    গণনা class VarSpecKind : uint8_t {
        PARAM,  // Select only matching parameters
        PORT,  // Select only matching ports
        VAR  // Select any matching AstVar (including params and ports)
    };

    স্থির শূন্য addCaseFull(ধ্রুবক string& file, পূর্ণসংখ্যা lineno);
    স্থির শূন্য addCaseParallel(ধ্রুবক string& file, পূর্ণসংখ্যা lineno);
    স্থির শূন্য addCoverageBlockOff(ধ্রুবক string& file, পূর্ণসংখ্যা lineno);
    স্থির শূন্য addCoverageBlockOff(ধ্রুবক string& module, ধ্রুবক string& blockname);
    স্থির শূন্য addHierWorkers(File_সারি* fl, ধ্রুবক string& model, পূর্ণসংখ্যা workers);
    স্থির শূন্য addFsmRegisterWrapper(File_সারি* fl, ধ্রুবক string& module, ধ্রুবক string& d,
                                      ধ্রুবক string& q, ধ্রুবক string& clock, ধ্রুবক string& reset,
                                      ধ্রুবক string& reset_মান);
    স্থির শূন্য addIgnore(V3ErrorCode code, bool on, ধ্রুবক string& filename, পূর্ণসংখ্যা min, পূর্ণসংখ্যা max);
    স্থির শূন্য addIgnoreMatch(V3ErrorCode code, ধ্রুবক string& filename, ধ্রুবক string& contents,
                               ধ্রুবক string& match);
    স্থির শূন্য addInline(File_সারি* fl, ধ্রুবক string& module, ধ্রুবক string& ftask, bool on);
    স্থির শূন্য addModulePragma(ধ্রুবক string& module, VPragmaType pragma);
    স্থির শূন্য add_Profile_তথ্য(File_সারি* fl, ধ্রুবক string& hierDpi, uint64_t cost);
    স্থির শূন্য add_Profile_তথ্য(File_সারি* fl, ধ্রুবক string& model, ধ্রুবক string& চাবি,
                               uint64_t cost);
    স্থির শূন্য addScopeTraceOn(bool on, ধ্রুবক string& scope, পূর্ণসংখ্যা levels);
    স্থির শূন্য addVarAttr(File_সারি* fl, ধ্রুবক string& module, ধ্রুবক string& ftask,
                           VarSpecKind kind, ধ্রুবক string& pattern, VAttrType type,
                           Ast_Sen_বৃক্ষ* nodep);

    স্থির শূন্য applyCase(AstCase* nodep);
    স্থির শূন্য applyCoverageBlock(Ast_গ্রন্থি_Module* modulep, AstBegin* nodep);
    স্থির শূন্য applyCoverageBlock(Ast_গ্রন্থি_Module* modulep, AstGenBlock* nodep);
    স্থির শূন্য applyFTask(Ast_গ্রন্থি_Module* modulep, Ast_গ্রন্থি_FTask* ftaskp);
    স্থির শূন্য applyIgnores(File_সারি* filelinep);
    স্থির শূন্য applyModule(Ast_গ্রন্থি_Module* modulep);
    স্থির শূন্য applyVarAttr(ধ্রুবক Ast_গ্রন্থি_Module* modulep, ধ্রুবক Ast_গ্রন্থি_FTask* ftaskp,
                             AstVar* varp);

    স্থির পূর্ণসংখ্যা getHierWorkers(ধ্রুবক string& model);
    স্থির File_সারি* get_Hier_Workers_File_সারি(ধ্রুবক string& model);
    স্থির ধ্রুবক FsmRegisterWrapper* getFsmRegisterWrapper(ধ্রুবক string& module);
    স্থির uint64_t get_Profile_তথ্য(ধ্রুবক string& hierDpi);
    স্থির uint64_t get_Profile_তথ্য(ধ্রুবক string& model, ধ্রুবক string& চাবি);
    স্থির File_সারি* get_Profile_তথ্য_File_সারি();
    স্থির bool getScopeTraceOn(ধ্রুবক string& scope);

    স্থির শূন্য contents_ঠেলো_Text(ধ্রুবক string& text);

    স্থির bool contains_MTask_Profile_তথ্য();
    স্থির uint64_t getCurrentHierBlockCost();

    স্থির bool waive(ধ্রুবক File_সারি* filelinep, V3ErrorCode code, ধ্রুবক string& message);

    স্থির শূন্য selfTest();
};

#endif  // Guard
