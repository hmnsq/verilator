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

#অন্তর্ভাৱ "config_build.h"
#অন্তর্ভাৱ "verilatedos.h"

#অন্তর্ভাৱ "V3Ast.h"
#অন্তর্ভাৱ "V3Error.h"
#অন্তর্ভাৱ "V3FileLine.h"
#অন্তর্ভাৱ "V3Mutex.h"

//######################################################################

class V3Control final {
সার্ৱজনিক:
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

    স্থির শূন্য addCaseFull(ধ্রুৱ string& file, পূর্ণাঙ্ক lineno);
    স্থির শূন্য addCaseParallel(ধ্রুৱ string& file, পূর্ণাঙ্ক lineno);
    স্থির শূন্য addCoverageBlockOff(ধ্রুৱ string& file, পূর্ণাঙ্ক lineno);
    স্থির শূন্য addCoverageBlockOff(ধ্রুৱ string& module, ধ্রুৱ string& blockname);
    স্থির শূন্য addHierWorkers(File_সাৰি* fl, ধ্রুৱ string& model, পূর্ণাঙ্ক workers);
    স্থির শূন্য addFsmRegisterWrapper(File_সাৰি* fl, ধ্রুৱ string& module, ধ্রুৱ string& d,
                                      ধ্রুৱ string& q, ধ্রুৱ string& clock, ধ্রুৱ string& reset,
                                      ধ্রুৱ string& reset_মান);
    স্থির শূন্য addIgnore(V3ErrorCode code, bool on, ধ্রুৱ string& filename, পূর্ণাঙ্ক min, পূর্ণাঙ্ক max);
    স্থির শূন্য addIgnoreMatch(V3ErrorCode code, ধ্রুৱ string& filename, ধ্রুৱ string& contents,
                               ধ্রুৱ string& match);
    স্থির শূন্য addInline(File_সাৰি* fl, ধ্রুৱ string& module, ধ্রুৱ string& ftask, bool on);
    স্থির শূন্য addModulePragma(ধ্রুৱ string& module, VPragmaType pragma);
    স্থির শূন্য addProfileData(File_সাৰি* fl, ধ্রুৱ string& hierDpi, uint64_t cost);
    স্থির শূন্য addProfileData(File_সাৰি* fl, ধ্রুৱ string& model, ধ্রুৱ string& চাবি,
                               uint64_t cost);
    স্থির শূন্য addScopeTraceOn(bool on, ধ্রুৱ string& scope, পূর্ণাঙ্ক levels);
    স্থির শূন্য addVarAttr(File_সাৰি* fl, ধ্রুৱ string& module, ধ্রুৱ string& ftask,
                           VarSpecKind kind, ধ্রুৱ string& pattern, VAttrType type,
                           Ast_Sen_বৃক্ষ* nodep);

    স্থির শূন্য applyCase(AstCase* nodep);
    স্থির শূন্য applyCoverageBlock(Ast_গ্ৰন্থি_Module* modulep, AstBegin* nodep);
    স্থির শূন্য applyCoverageBlock(Ast_গ্ৰন্থি_Module* modulep, AstGenBlock* nodep);
    স্থির শূন্য applyFTask(Ast_গ্ৰন্থি_Module* modulep, Ast_গ্ৰন্থি_FTask* ftaskp);
    স্থির শূন্য applyIgnores(File_সাৰি* filelinep);
    স্থির শূন্য applyModule(Ast_গ্ৰন্থি_Module* modulep);
    স্থির শূন্য applyVarAttr(ধ্রুৱ Ast_গ্ৰন্থি_Module* modulep, ধ্রুৱ Ast_গ্ৰন্থি_FTask* ftaskp,
                             AstVar* varp);

    স্থির পূর্ণাঙ্ক getHierWorkers(ধ্রুৱ string& model);
    স্থির File_সাৰি* get_Hier_Workers_File_সাৰি(ধ্রুৱ string& model);
    স্থির ধ্রুৱ FsmRegisterWrapper* getFsmRegisterWrapper(ধ্রুৱ string& module);
    স্থির uint64_t getProfileData(ধ্রুৱ string& hierDpi);
    স্থির uint64_t getProfileData(ধ্রুৱ string& model, ধ্রুৱ string& চাবি);
    স্থির File_সাৰি* get_Profile_Data_File_সাৰি();
    স্থির bool getScopeTraceOn(ধ্রুৱ string& scope);

    স্থির শূন্য contents_ঠেলো_Text(ধ্রুৱ string& text);

    স্থির bool containsMTaskProfileData();
    স্থির uint64_t getCurrentHierBlockCost();

    স্থির bool waive(ধ্রুৱ File_সাৰি* filelinep, V3ErrorCode code, ধ্রুৱ string& message);

    স্থির শূন্য selfTest();
};

#endif  // Guard
