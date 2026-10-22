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
সার্বজনিক:
    সংরচনা FsmRegisterWrapper final {
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

    স্থির শূন্য addCaseFull(ধ্রুব string& file, পূর্ণাঙ্ক lineno);
    স্থির শূন্য addCaseParallel(ধ্রুব string& file, পূর্ণাঙ্ক lineno);
    স্থির শূন্য addCoverageBlockOff(ধ্রুব string& file, পূর্ণাঙ্ক lineno);
    স্থির শূন্য addCoverageBlockOff(ধ্রুব string& module, ধ্রুব string& blockname);
    স্থির শূন্য addHierWorkers(File_সারি* fl, ধ্রুব string& model, পূর্ণাঙ্ক workers);
    স্থির শূন্য addFsmRegisterWrapper(File_সারি* fl, ধ্রুব string& module, ধ্রুব string& d,
                                      ধ্রুব string& q, ধ্রুব string& clock, ধ্রুব string& reset,
                                      ধ্রুব string& reset_মান);
    স্থির শূন্য addIgnore(V3ErrorCode code, bool on, ধ্রুব string& filename, পূর্ণাঙ্ক min, পূর্ণাঙ্ক max);
    স্থির শূন্য addIgnoreMatch(V3ErrorCode code, ধ্রুব string& filename, ধ্রুব string& contents,
                               ধ্রুব string& match);
    স্থির শূন্য addInline(File_সারি* fl, ধ্রুব string& module, ধ্রুব string& ftask, bool on);
    স্থির শূন্য addModulePragma(ধ্রুব string& module, VPragmaType pragma);
    স্থির শূন্য addProfileData(File_সারি* fl, ধ্রুব string& hierDpi, uint64_t cost);
    স্থির শূন্য addProfileData(File_সারি* fl, ধ্রুব string& model, ধ্রুব string& চাবি,
                               uint64_t cost);
    স্থির শূন্য addScopeTraceOn(bool on, ধ্রুব string& scope, পূর্ণাঙ্ক levels);
    স্থির শূন্য addVarAttr(File_সারি* fl, ধ্রুব string& module, ধ্রুব string& ftask,
                           VarSpecKind kind, ধ্রুব string& pattern, VAttrType type,
                           Ast_Sen_বৃক্ষ* nodep);

    স্থির শূন্য applyCase(AstCase* nodep);
    স্থির শূন্য applyCoverageBlock(Ast_গ্রন্থি_Module* modulep, AstBegin* nodep);
    স্থির শূন্য applyCoverageBlock(Ast_গ্রন্থি_Module* modulep, AstGenBlock* nodep);
    স্থির শূন্য applyFTask(Ast_গ্রন্থি_Module* modulep, Ast_গ্রন্থি_FTask* ftaskp);
    স্থির শূন্য applyIgnores(File_সারি* filelinep);
    স্থির শূন্য applyModule(Ast_গ্রন্থি_Module* modulep);
    স্থির শূন্য applyVarAttr(ধ্রুব Ast_গ্রন্থি_Module* modulep, ধ্রুব Ast_গ্রন্থি_FTask* ftaskp,
                             AstVar* varp);

    স্থির পূর্ণাঙ্ক getHierWorkers(ধ্রুব string& model);
    স্থির File_সারি* get_Hier_Workers_File_সারি(ধ্রুব string& model);
    স্থির ধ্রুব FsmRegisterWrapper* getFsmRegisterWrapper(ধ্রুব string& module);
    স্থির uint64_t getProfileData(ধ্রুব string& hierDpi);
    স্থির uint64_t getProfileData(ধ্রুব string& model, ধ্রুব string& চাবি);
    স্থির File_সারি* get_Profile_Data_File_সারি();
    স্থির bool getScopeTraceOn(ধ্রুব string& scope);

    স্থির শূন্য contents_ঠেলো_Text(ধ্রুব string& text);

    স্থির bool containsMTaskProfileData();
    স্থির uint64_t getCurrentHierBlockCost();

    স্থির bool waive(ধ্রুব File_সারি* filelinep, V3ErrorCode code, ধ্রুব string& message);

    স্থির শূন্য selfTest();
};

#endif  // Guard
