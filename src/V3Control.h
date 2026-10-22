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

#അന്തര്ഭാവ "config_build.h"
#അന്തര്ഭാവ "verilatedos.h"

#അന്തര്ഭാവ "V3Ast.h"
#അന്തര്ഭാവ "V3Error.h"
#അന്തര്ഭാവ "V3FileLine.h"
#അന്തര്ഭാവ "V3Mutex.h"

//######################################################################

class V3Control final {
പൊതു:
    ഘടന FsmRegisterWrapper final {
        string moduleName;
        string d;
        string q;
        string clock;
        string reset;
        string reset_മൂല്യം;
    };

    എണ്ണൽ class VarSpecKind : uint8_t {
        PARAM,  // Select only matching parameters
        PORT,  // Select only matching ports
        VAR  // Select any matching AstVar (including params and ports)
    };

    സ്ഥിതം ശൂന്യം addCaseFull(സ്ഥിരം string& file, പൂർണ്ണസംഖ്യ lineno);
    സ്ഥിതം ശൂന്യം addCaseParallel(സ്ഥിരം string& file, പൂർണ്ണസംഖ്യ lineno);
    സ്ഥിതം ശൂന്യം addCoverageBlockOff(സ്ഥിരം string& file, പൂർണ്ണസംഖ്യ lineno);
    സ്ഥിതം ശൂന്യം addCoverageBlockOff(സ്ഥിരം string& module, സ്ഥിരം string& blockname);
    സ്ഥിതം ശൂന്യം addHierWorkers(File_വരി* fl, സ്ഥിരം string& model, പൂർണ്ണസംഖ്യ workers);
    സ്ഥിതം ശൂന്യം addFsmRegisterWrapper(File_വരി* fl, സ്ഥിരം string& module, സ്ഥിരം string& d,
                                      സ്ഥിരം string& q, സ്ഥിരം string& clock, സ്ഥിരം string& reset,
                                      സ്ഥിരം string& reset_മൂല്യം);
    സ്ഥിതം ശൂന്യം addIgnore(V3ErrorCode code, bool on, സ്ഥിരം string& filename, പൂർണ്ണസംഖ്യ min, പൂർണ്ണസംഖ്യ max);
    സ്ഥിതം ശൂന്യം addIgnoreMatch(V3ErrorCode code, സ്ഥിരം string& filename, സ്ഥിരം string& contents,
                               സ്ഥിരം string& match);
    സ്ഥിതം ശൂന്യം addInline(File_വരി* fl, സ്ഥിരം string& module, സ്ഥിരം string& ftask, bool on);
    സ്ഥിതം ശൂന്യം addModulePragma(സ്ഥിരം string& module, VPragmaType pragma);
    സ്ഥിതം ശൂന്യം addProfileData(File_വരി* fl, സ്ഥിരം string& hierDpi, uint64_t cost);
    സ്ഥിതം ശൂന്യം addProfileData(File_വരി* fl, സ്ഥിരം string& model, സ്ഥിരം string& താക്കോൽ,
                               uint64_t cost);
    സ്ഥിതം ശൂന്യം addScopeTraceOn(bool on, സ്ഥിരം string& scope, പൂർണ്ണസംഖ്യ levels);
    സ്ഥിതം ശൂന്യം addVarAttr(File_വരി* fl, സ്ഥിരം string& module, സ്ഥിരം string& ftask,
                           VarSpecKind kind, സ്ഥിരം string& pattern, VAttrType type,
                           Ast_Sen_വൃക്ഷം* nodep);

    സ്ഥിതം ശൂന്യം applyCase(AstCase* nodep);
    സ്ഥിതം ശൂന്യം applyCoverageBlock(Ast_കണ്ണി_Module* modulep, AstBegin* nodep);
    സ്ഥിതം ശൂന്യം applyCoverageBlock(Ast_കണ്ണി_Module* modulep, AstGenBlock* nodep);
    സ്ഥിതം ശൂന്യം applyFTask(Ast_കണ്ണി_Module* modulep, Ast_കണ്ണി_FTask* ftaskp);
    സ്ഥിതം ശൂന്യം applyIgnores(File_വരി* filelinep);
    സ്ഥിതം ശൂന്യം applyModule(Ast_കണ്ണി_Module* modulep);
    സ്ഥിതം ശൂന്യം applyVarAttr(സ്ഥിരം Ast_കണ്ണി_Module* modulep, സ്ഥിരം Ast_കണ്ണി_FTask* ftaskp,
                             AstVar* varp);

    സ്ഥിതം പൂർണ്ണസംഖ്യ getHierWorkers(സ്ഥിരം string& model);
    സ്ഥിതം File_വരി* get_Hier_Workers_File_വരി(സ്ഥിരം string& model);
    സ്ഥിതം സ്ഥിരം FsmRegisterWrapper* getFsmRegisterWrapper(സ്ഥിരം string& module);
    സ്ഥിതം uint64_t getProfileData(സ്ഥിരം string& hierDpi);
    സ്ഥിതം uint64_t getProfileData(സ്ഥിരം string& model, സ്ഥിരം string& താക്കോൽ);
    സ്ഥിതം File_വരി* get_Profile_Data_File_വരി();
    സ്ഥിതം bool getScopeTraceOn(സ്ഥിരം string& scope);

    സ്ഥിതം ശൂന്യം contents_തള്ളുക_Text(സ്ഥിരം string& text);

    സ്ഥിതം bool containsMTaskProfileData();
    സ്ഥിതം uint64_t getCurrentHierBlockCost();

    സ്ഥിതം bool waive(സ്ഥിരം File_വരി* filelinep, V3ErrorCode code, സ്ഥിരം string& message);

    സ്ഥിതം ശൂന്യം selfTest();
};

#endif  // Guard
