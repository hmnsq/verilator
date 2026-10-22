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

#अगर_अघोषित VERILATOR_V3CONFIG_H_
#घोषणा VERILATOR_V3CONFIG_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश "V3Ast.h"
#समावेश "V3Error.h"
#समावेश "V3FileLine.h"
#समावेश "V3Mutex.h"

//######################################################################

class V3Control final {
खुला:
    काष्ठा FsmRegisterWrapper final {
        string moduleName;
        string d;
        string q;
        string घड़ी;
        string reset;
        string reset_मूल्य;
    };

    क्रमागत class VarSpecKind : uint8_t {
        PARAM,  // Select only matching parameters
        PORT,  // Select only matching ports
        VAR  // Select any matching AstVar (including params and ports)
    };

    अटल व्योम addCaseFull(स्थिर string& file, पूर्णांक lineno);
    अटल व्योम addCaseParallel(स्थिर string& file, पूर्णांक lineno);
    अटल व्योम addCoverageBlockOff(स्थिर string& file, पूर्णांक lineno);
    अटल व्योम addCoverageBlockOff(स्थिर string& module, स्थिर string& blockname);
    अटल व्योम addHierWorkers(File_पंक्ति* fl, स्थिर string& model, पूर्णांक workers);
    अटल व्योम addFsmRegisterWrapper(File_पंक्ति* fl, स्थिर string& module, स्थिर string& d,
                                      स्थिर string& q, स्थिर string& घड़ी, स्थिर string& reset,
                                      स्थिर string& reset_मूल्य);
    अटल व्योम addIgnore(V3ErrorCode code, bool on, स्थिर string& filename, पूर्णांक min, पूर्णांक max);
    अटल व्योम addIgnoreMatch(V3ErrorCode code, स्थिर string& filename, स्थिर string& contents,
                               स्थिर string& match);
    अटल व्योम addInline(File_पंक्ति* fl, स्थिर string& module, स्थिर string& ftask, bool on);
    अटल व्योम addModulePragma(स्थिर string& module, VPragmaType pragma);
    अटल व्योम add_Profile_तथ्य(File_पंक्ति* fl, स्थिर string& hierDpi, uint64_t cost);
    अटल व्योम add_Profile_तथ्य(File_पंक्ति* fl, स्थिर string& model, स्थिर string& कुंजी,
                               uint64_t cost);
    अटल व्योम addScopeTraceOn(bool on, स्थिर string& scope, पूर्णांक levels);
    अटल व्योम addVarAttr(File_पंक्ति* fl, स्थिर string& module, स्थिर string& ftask,
                           VarSpecKind kind, स्थिर string& pattern, VAttrType type,
                           Ast_Sen_वृक्ष* nodep);

    अटल व्योम applyCase(AstCase* nodep);
    अटल व्योम applyCoverageBlock(Ast_गाँठ_Module* modulep, AstBegin* nodep);
    अटल व्योम applyCoverageBlock(Ast_गाँठ_Module* modulep, AstGenBlock* nodep);
    अटल व्योम applyFTask(Ast_गाँठ_Module* modulep, Ast_गाँठ_FTask* ftaskp);
    अटल व्योम applyIgnores(File_पंक्ति* filelinep);
    अटल व्योम applyModule(Ast_गाँठ_Module* modulep);
    अटल व्योम applyVarAttr(स्थिर Ast_गाँठ_Module* modulep, स्थिर Ast_गाँठ_FTask* ftaskp,
                             AstVar* varp);

    अटल पूर्णांक getHierWorkers(स्थिर string& model);
    अटल File_पंक्ति* get_Hier_Workers_File_पंक्ति(स्थिर string& model);
    अटल स्थिर FsmRegisterWrapper* getFsmRegisterWrapper(स्थिर string& module);
    अटल uint64_t get_Profile_तथ्य(स्थिर string& hierDpi);
    अटल uint64_t get_Profile_तथ्य(स्थिर string& model, स्थिर string& कुंजी);
    अटल File_पंक्ति* get_Profile_तथ्य_File_पंक्ति();
    अटल bool getScopeTraceOn(स्थिर string& scope);

    अटल व्योम contents_धकेलो_Text(स्थिर string& text);

    अटल bool contains_MTask_Profile_तथ्य();
    अटल uint64_t getCurrentHierBlockCost();

    अटल bool waive(स्थिर File_पंक्ति* filelinep, V3ErrorCode code, स्थिर string& message);

    अटल व्योम selfTest();
};

#पूर्ण_अगर  // Guard
