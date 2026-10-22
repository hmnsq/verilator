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

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Ast.h"
#شامل "V3Error.h"
#شامل "V3FileLine.h"
#شامل "V3Mutex.h"

//######################################################################

class V3Control final {
عمومی:
    ساختار FsmRegisterWrapper final {
        string moduleName;
        string d;
        string q;
        string clock;
        string reset;
        string reset_مقدار;
    };

    شمارش class VarSpecKind : uint8_t {
        PARAM,  // Select only matching parameters
        PORT,  // Select only matching ports
        VAR  // Select any matching AstVar (including params and ports)
    };

    ایستا تهی addCaseFull(ثابت string& file, صحیح lineno);
    ایستا تهی addCaseParallel(ثابت string& file, صحیح lineno);
    ایستا تهی addCoverageBlockOff(ثابت string& file, صحیح lineno);
    ایستا تهی addCoverageBlockOff(ثابت string& module, ثابت string& blockname);
    ایستا تهی addHierWorkers(File_سطر* fl, ثابت string& model, صحیح workers);
    ایستا تهی addFsmRegisterWrapper(File_سطر* fl, ثابت string& module, ثابت string& d,
                                      ثابت string& q, ثابت string& clock, ثابت string& reset,
                                      ثابت string& reset_مقدار);
    ایستا تهی addIgnore(V3ErrorCode code, bool on, ثابت string& filename, صحیح min, صحیح max);
    ایستا تهی addIgnoreMatch(V3ErrorCode code, ثابت string& filename, ثابت string& contents,
                               ثابت string& match);
    ایستا تهی addInline(File_سطر* fl, ثابت string& module, ثابت string& ftask, bool on);
    ایستا تهی addModulePragma(ثابت string& module, VPragmaType pragma);
    ایستا تهی addProfileData(File_سطر* fl, ثابت string& hierDpi, uint64_t cost);
    ایستا تهی addProfileData(File_سطر* fl, ثابت string& model, ثابت string& کلید,
                               uint64_t cost);
    ایستا تهی addScopeTraceOn(bool on, ثابت string& scope, صحیح levels);
    ایستا تهی addVarAttr(File_سطر* fl, ثابت string& module, ثابت string& ftask,
                           VarSpecKind kind, ثابت string& pattern, VAttrType type,
                           Ast_Sen_درخت* nodep);

    ایستا تهی applyCase(AstCase* nodep);
    ایستا تهی applyCoverageBlock(Ast_گره_Module* modulep, AstBegin* nodep);
    ایستا تهی applyCoverageBlock(Ast_گره_Module* modulep, AstGenBlock* nodep);
    ایستا تهی applyFTask(Ast_گره_Module* modulep, Ast_گره_FTask* ftaskp);
    ایستا تهی applyIgnores(File_سطر* filelinep);
    ایستا تهی applyModule(Ast_گره_Module* modulep);
    ایستا تهی applyVarAttr(ثابت Ast_گره_Module* modulep, ثابت Ast_گره_FTask* ftaskp,
                             AstVar* varp);

    ایستا صحیح getHierWorkers(ثابت string& model);
    ایستا File_سطر* get_Hier_Workers_File_سطر(ثابت string& model);
    ایستا ثابت FsmRegisterWrapper* getFsmRegisterWrapper(ثابت string& module);
    ایستا uint64_t getProfileData(ثابت string& hierDpi);
    ایستا uint64_t getProfileData(ثابت string& model, ثابت string& کلید);
    ایستا File_سطر* get_Profile_Data_File_سطر();
    ایستا bool getScopeTraceOn(ثابت string& scope);

    ایستا تهی contents_افزودن_Text(ثابت string& text);

    ایستا bool containsMTaskProfileData();
    ایستا uint64_t getCurrentHierBlockCost();

    ایستا bool waive(ثابت File_سطر* filelinep, V3ErrorCode code, ثابت string& message);

    ایستا تهی selfTest();
};

#endif  // Guard
