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
        string clock;
        string reset;
        string reset_मूल्य;
    };

    गणना class VarSpecKind : uint8_t {
        PARAM,  // Select only matching parameters
        PORT,  // Select only matching ports
        VAR  // Select any matching AstVar (including params and ports)
    };

    स्थिर शून्य addCaseFull(ध्रुव string& file, पूर्णाङ्क lineno);
    स्थिर शून्य addCaseParallel(ध्रुव string& file, पूर्णाङ्क lineno);
    स्थिर शून्य addCoverageBlockOff(ध्रुव string& file, पूर्णाङ्क lineno);
    स्थिर शून्य addCoverageBlockOff(ध्रुव string& module, ध्रुव string& blockname);
    स्थिर शून्य addHierWorkers(File_पंक्ति* fl, ध्रुव string& model, पूर्णाङ्क workers);
    स्थिर शून्य addFsmRegisterWrapper(File_पंक्ति* fl, ध्रुव string& module, ध्रुव string& d,
                                      ध्रुव string& q, ध्रुव string& clock, ध्रुव string& reset,
                                      ध्रुव string& reset_मूल्य);
    स्थिर शून्य addIgnore(V3ErrorCode code, bool on, ध्रुव string& filename, पूर्णाङ्क min, पूर्णाङ्क max);
    स्थिर शून्य addIgnoreMatch(V3ErrorCode code, ध्रुव string& filename, ध्रुव string& contents,
                               ध्रुव string& match);
    स्थिर शून्य addInline(File_पंक्ति* fl, ध्रुव string& module, ध्रुव string& ftask, bool on);
    स्थिर शून्य addModulePragma(ध्रुव string& module, VPragmaType pragma);
    स्थिर शून्य addProfileData(File_पंक्ति* fl, ध्रुव string& hierDpi, uint64_t cost);
    स्थिर शून्य addProfileData(File_पंक्ति* fl, ध्रुव string& model, ध्रुव string& कुंजी,
                               uint64_t cost);
    स्थिर शून्य addScopeTraceOn(bool on, ध्रुव string& scope, पूर्णाङ्क levels);
    स्थिर शून्य addVarAttr(File_पंक्ति* fl, ध्रुव string& module, ध्रुव string& ftask,
                           VarSpecKind kind, ध्रुव string& pattern, VAttrType type,
                           Ast_Sen_वृक्ष* nodep);

    स्थिर शून्य applyCase(AstCase* nodep);
    स्थिर शून्य applyCoverageBlock(Ast_गाँठ_Module* modulep, AstBegin* nodep);
    स्थिर शून्य applyCoverageBlock(Ast_गाँठ_Module* modulep, AstGenBlock* nodep);
    स्थिर शून्य applyFTask(Ast_गाँठ_Module* modulep, Ast_गाँठ_FTask* ftaskp);
    स्थिर शून्य applyIgnores(File_पंक्ति* filelinep);
    स्थिर शून्य applyModule(Ast_गाँठ_Module* modulep);
    स्थिर शून्य applyVarAttr(ध्रुव Ast_गाँठ_Module* modulep, ध्रुव Ast_गाँठ_FTask* ftaskp,
                             AstVar* varp);

    स्थिर पूर्णाङ्क getHierWorkers(ध्रुव string& model);
    स्थिर File_पंक्ति* get_Hier_Workers_File_पंक्ति(ध्रुव string& model);
    स्थिर ध्रुव FsmRegisterWrapper* getFsmRegisterWrapper(ध्रुव string& module);
    स्थिर uint64_t getProfileData(ध्रुव string& hierDpi);
    स्थिर uint64_t getProfileData(ध्रुव string& model, ध्रुव string& कुंजी);
    स्थिर File_पंक्ति* get_Profile_Data_File_पंक्ति();
    स्थिर bool getScopeTraceOn(ध्रुव string& scope);

    स्थिर शून्य contents_धकेलो_Text(ध्रुव string& text);

    स्थिर bool containsMTaskProfileData();
    स्थिर uint64_t getCurrentHierBlockCost();

    स्थिर bool waive(ध्रुव File_पंक्ति* filelinep, V3ErrorCode code, ध्रुव string& message);

    स्थिर शून्य selfTest();
};

#endif  // Guard
