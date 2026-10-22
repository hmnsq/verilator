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

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Ast.h"
#अन्तर्भाव "V3Error.h"
#अन्तर्भाव "V3FileLine.h"
#अन्तर्भाव "V3Mutex.h"

//######################################################################

class V3Control final {
सार्वजनिक:
    संरचना FsmRegisterWrapper final {
        string moduleName;
        string d;
        string q;
        string घड़ी;
        string reset;
        string reset_मूल्य;
    };

    गणना class VarSpecKind : uint8_t {
        PARAM,  // Select only matching parameters
        PORT,  // Select only matching ports
        VAR  // Select any matching AstVar (including params and ports)
    };

    स्थिर शून्य addCaseFull(स्थिरांक string& file, पूर्णांक lineno);
    स्थिर शून्य addCaseParallel(स्थिरांक string& file, पूर्णांक lineno);
    स्थिर शून्य addCoverageBlockOff(स्थिरांक string& file, पूर्णांक lineno);
    स्थिर शून्य addCoverageBlockOff(स्थिरांक string& module, स्थिरांक string& blockname);
    स्थिर शून्य addHierWorkers(File_ओळ* fl, स्थिरांक string& model, पूर्णांक workers);
    स्थिर शून्य addFsmRegisterWrapper(File_ओळ* fl, स्थिरांक string& module, स्थिरांक string& d,
                                      स्थिरांक string& q, स्थिरांक string& घड़ी, स्थिरांक string& reset,
                                      स्थिरांक string& reset_मूल्य);
    स्थिर शून्य addIgnore(V3ErrorCode code, bool on, स्थिरांक string& filename, पूर्णांक min, पूर्णांक max);
    स्थिर शून्य addIgnoreMatch(V3ErrorCode code, स्थिरांक string& filename, स्थिरांक string& contents,
                               स्थिरांक string& match);
    स्थिर शून्य addInline(File_ओळ* fl, स्थिरांक string& module, स्थिरांक string& ftask, bool on);
    स्थिर शून्य addModulePragma(स्थिरांक string& module, VPragmaType pragma);
    स्थिर शून्य addProfileData(File_ओळ* fl, स्थिरांक string& hierDpi, uint64_t cost);
    स्थिर शून्य addProfileData(File_ओळ* fl, स्थिरांक string& model, स्थिरांक string& किल्ली,
                               uint64_t cost);
    स्थिर शून्य addScopeTraceOn(bool on, स्थिरांक string& scope, पूर्णांक levels);
    स्थिर शून्य addVarAttr(File_ओळ* fl, स्थिरांक string& module, स्थिरांक string& ftask,
                           VarSpecKind kind, स्थिरांक string& pattern, VAttrType type,
                           Ast_Sen_वृक्ष* nodep);

    स्थिर शून्य applyCase(AstCase* nodep);
    स्थिर शून्य applyCoverageBlock(Ast_गाठ_Module* modulep, AstBegin* nodep);
    स्थिर शून्य applyCoverageBlock(Ast_गाठ_Module* modulep, AstGenBlock* nodep);
    स्थिर शून्य applyFTask(Ast_गाठ_Module* modulep, Ast_गाठ_FTask* ftaskp);
    स्थिर शून्य applyIgnores(File_ओळ* filelinep);
    स्थिर शून्य applyModule(Ast_गाठ_Module* modulep);
    स्थिर शून्य applyVarAttr(स्थिरांक Ast_गाठ_Module* modulep, स्थिरांक Ast_गाठ_FTask* ftaskp,
                             AstVar* varp);

    स्थिर पूर्णांक getHierWorkers(स्थिरांक string& model);
    स्थिर File_ओळ* get_Hier_Workers_File_ओळ(स्थिरांक string& model);
    स्थिर स्थिरांक FsmRegisterWrapper* getFsmRegisterWrapper(स्थिरांक string& module);
    स्थिर uint64_t getProfileData(स्थिरांक string& hierDpi);
    स्थिर uint64_t getProfileData(स्थिरांक string& model, स्थिरांक string& किल्ली);
    स्थिर File_ओळ* get_Profile_Data_File_ओळ();
    स्थिर bool getScopeTraceOn(स्थिरांक string& scope);

    स्थिर शून्य contents_ढकला_Text(स्थिरांक string& text);

    स्थिर bool containsMTaskProfileData();
    स्थिर uint64_t getCurrentHierBlockCost();

    स्थिर bool waive(स्थिरांक File_ओळ* filelinep, V3ErrorCode code, स्थिरांक string& message);

    स्थिर शून्य selfTest();
};

#endif  // Guard
