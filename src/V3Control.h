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

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"
#ضمن "V3Error.h"
#ضمن "V3FileLine.h"
#ضمن "V3Mutex.h"

//######################################################################

class V3Control final {
عام:
    بنية FsmRegisterWrapper final {
        string moduleName;
        string d;
        string q;
        string clock;
        string reset;
        string resetValue;
    };

    تعداد class VarSpecKind : uint8_t {
        PARAM,  // Select only matching parameters
        PORT,  // Select only matching ports
        VAR  // Select any matching AstVar (including params and ports)
    };

    ساكن فراغ addCaseFull(ثابت string& file, صحيح lineno);
    ساكن فراغ addCaseParallel(ثابت string& file, صحيح lineno);
    ساكن فراغ addCoverageBlockOff(ثابت string& file, صحيح lineno);
    ساكن فراغ addCoverageBlockOff(ثابت string& module, ثابت string& blockname);
    ساكن فراغ addHierWorkers(FileLine* fl, ثابت string& model, صحيح workers);
    ساكن فراغ addFsmRegisterWrapper(FileLine* fl, ثابت string& module, ثابت string& d,
                                      ثابت string& q, ثابت string& clock, ثابت string& reset,
                                      ثابت string& resetValue);
    ساكن فراغ addIgnore(V3ErrorCode code, bool on, ثابت string& filename, صحيح min, صحيح max);
    ساكن فراغ addIgnoreMatch(V3ErrorCode code, ثابت string& filename, ثابت string& contents,
                               ثابت string& match);
    ساكن فراغ addInline(FileLine* fl, ثابت string& module, ثابت string& ftask, bool on);
    ساكن فراغ addModulePragma(ثابت string& module, VPragmaType pragma);
    ساكن فراغ addProfileData(FileLine* fl, ثابت string& hierDpi, uint64_t cost);
    ساكن فراغ addProfileData(FileLine* fl, ثابت string& model, ثابت string& key,
                               uint64_t cost);
    ساكن فراغ addScopeTraceOn(bool on, ثابت string& scope, صحيح levels);
    ساكن فراغ addVarAttr(FileLine* fl, ثابت string& module, ثابت string& ftask,
                           VarSpecKind kind, ثابت string& pattern, VAttrType type,
                           AstSenTree* nodep);

    ساكن فراغ applyCase(AstCase* nodep);
    ساكن فراغ applyCoverageBlock(AstNodeModule* modulep, AstBegin* nodep);
    ساكن فراغ applyCoverageBlock(AstNodeModule* modulep, AstGenBlock* nodep);
    ساكن فراغ applyFTask(AstNodeModule* modulep, AstNodeFTask* ftaskp);
    ساكن فراغ applyIgnores(FileLine* filelinep);
    ساكن فراغ applyModule(AstNodeModule* modulep);
    ساكن فراغ applyVarAttr(ثابت AstNodeModule* modulep, ثابت AstNodeFTask* ftaskp,
                             AstVar* varp);

    ساكن صحيح getHierWorkers(ثابت string& model);
    ساكن FileLine* getHierWorkersFileLine(ثابت string& model);
    ساكن ثابت FsmRegisterWrapper* getFsmRegisterWrapper(ثابت string& module);
    ساكن uint64_t getProfileData(ثابت string& hierDpi);
    ساكن uint64_t getProfileData(ثابت string& model, ثابت string& key);
    ساكن FileLine* getProfileDataFileLine();
    ساكن bool getScopeTraceOn(ثابت string& scope);

    ساكن فراغ contentsPushText(ثابت string& text);

    ساكن bool containsMTaskProfileData();
    ساكن uint64_t getCurrentHierBlockCost();

    ساكن bool waive(ثابت FileLine* filelinep, V3ErrorCode code, ثابت string& message);

    ساكن فراغ selfTest();
};

#endif  // Guard
