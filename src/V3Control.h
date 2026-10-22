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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3CONFIG_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3CONFIG_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Error.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3FileLine.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Mutex.h"

//######################################################################

class V3Control final {
ਜਨਤਕ:
    ਬਣਤਰ Fsm_ਰਜਿਸਟਰ_Wrapper final {
        string moduleName;
        string d;
        string q;
        string clock;
        string reset;
        string reset_ਮੁੱਲ;
    };

    ਗਿਣਤੀ class Var_Spec_ਕਿਸਮ : uint8_t {
        PARAM,  // Select only matching parameters
        ਪੋਰਟ,  // Select only matching ports
        VAR  // Select any matching AstVar (including params and ports)
    };

    ਸਥਿਰ ਖਾਲੀ ਜੋੜੋ_Case_ਭਰਿਆ(ਅਚਲ string& file, ਪੂਰਨ lineno);
    ਸਥਿਰ ਖਾਲੀ ਜੋੜੋ_Case_Parallel(ਅਚਲ string& file, ਪੂਰਨ lineno);
    ਸਥਿਰ ਖਾਲੀ ਜੋੜੋ_Coverage_ਖੰਡ_Off(ਅਚਲ string& file, ਪੂਰਨ lineno);
    ਸਥਿਰ ਖਾਲੀ ਜੋੜੋ_Coverage_ਖੰਡ_Off(ਅਚਲ string& module, ਅਚਲ string& blockname);
    ਸਥਿਰ ਖਾਲੀ ਜੋੜੋ_Hier_Workers(File_ਸਤਰ* fl, ਅਚਲ string& model, ਪੂਰਨ workers);
    ਸਥਿਰ ਖਾਲੀ ਜੋੜੋ_Fsm_ਰਜਿਸਟਰ_Wrapper(File_ਸਤਰ* fl, ਅਚਲ string& module, ਅਚਲ string& d,
                                      ਅਚਲ string& q, ਅਚਲ string& clock, ਅਚਲ string& reset,
                                      ਅਚਲ string& reset_ਮੁੱਲ);
    ਸਥਿਰ ਖਾਲੀ ਜੋੜੋ_Ignore(V3_Error_ਕੋਡ ਕੋਡ, bool on, ਅਚਲ string& filename, ਪੂਰਨ min, ਪੂਰਨ max);
    ਸਥਿਰ ਖਾਲੀ ਜੋੜੋ_Ignore_ਮੇਲ(V3_Error_ਕੋਡ ਕੋਡ, ਅਚਲ string& filename, ਅਚਲ string& contents,
                               ਅਚਲ string& ਮੇਲ);
    ਸਥਿਰ ਖਾਲੀ ਜੋੜੋ_Inline(File_ਸਤਰ* fl, ਅਚਲ string& module, ਅਚਲ string& ftask, bool on);
    ਸਥਿਰ ਖਾਲੀ ਜੋੜੋ_Module_Pragma(ਅਚਲ string& module, VPragmaType pragma);
    ਸਥਿਰ ਖਾਲੀ ਜੋੜੋ_Profile_ਅੰਕੜੇ(File_ਸਤਰ* fl, ਅਚਲ string& hierDpi, uint64_t ਲਾਗਤ);
    ਸਥਿਰ ਖਾਲੀ ਜੋੜੋ_Profile_ਅੰਕੜੇ(File_ਸਤਰ* fl, ਅਚਲ string& model, ਅਚਲ string& ਕੁੰਜੀ,
                               uint64_t ਲਾਗਤ);
    ਸਥਿਰ ਖਾਲੀ ਜੋੜੋ_Scope_Trace_On(bool on, ਅਚਲ string& scope, ਪੂਰਨ ਪੱਧਰ);
    ਸਥਿਰ ਖਾਲੀ ਜੋੜੋ_Var_Attr(File_ਸਤਰ* fl, ਅਚਲ string& module, ਅਚਲ string& ftask,
                           Var_Spec_ਕਿਸਮ ਕਿਸਮ, ਅਚਲ string& ਨਮੂਨਾ_2, VAttrType type,
                           Ast_Sen_ਰੁੱਖ* nodep);

    ਸਥਿਰ ਖਾਲੀ applyCase(AstCase* nodep);
    ਸਥਿਰ ਖਾਲੀ apply_Coverage_ਖੰਡ(Ast_ਗੰਢ_Module* modulep, AstBegin* nodep);
    ਸਥਿਰ ਖਾਲੀ apply_Coverage_ਖੰਡ(Ast_ਗੰਢ_Module* modulep, Ast_Gen_ਖੰਡ* nodep);
    ਸਥਿਰ ਖਾਲੀ applyFTask(Ast_ਗੰਢ_Module* modulep, Ast_ਗੰਢ_FTask* ftaskp);
    ਸਥਿਰ ਖਾਲੀ applyIgnores(File_ਸਤਰ* filelinep);
    ਸਥਿਰ ਖਾਲੀ applyModule(Ast_ਗੰਢ_Module* modulep);
    ਸਥਿਰ ਖਾਲੀ applyVarAttr(ਅਚਲ Ast_ਗੰਢ_Module* modulep, ਅਚਲ Ast_ਗੰਢ_FTask* ftaskp,
                             AstVar* varp);

    ਸਥਿਰ ਪੂਰਨ getHierWorkers(ਅਚਲ string& model);
    ਸਥਿਰ File_ਸਤਰ* get_Hier_Workers_File_ਸਤਰ(ਅਚਲ string& model);
    ਸਥਿਰ ਅਚਲ Fsm_ਰਜਿਸਟਰ_Wrapper* get_Fsm_ਰਜਿਸਟਰ_Wrapper(ਅਚਲ string& module);
    ਸਥਿਰ uint64_t get_Profile_ਅੰਕੜੇ(ਅਚਲ string& hierDpi);
    ਸਥਿਰ uint64_t get_Profile_ਅੰਕੜੇ(ਅਚਲ string& model, ਅਚਲ string& ਕੁੰਜੀ);
    ਸਥਿਰ File_ਸਤਰ* get_Profile_ਅੰਕੜੇ_File_ਸਤਰ();
    ਸਥਿਰ bool getScopeTraceOn(ਅਚਲ string& scope);

    ਸਥਿਰ ਖਾਲੀ contents_ਧੱਕੋ_ਪਾਠ(ਅਚਲ string& ਪਾਠ);

    ਸਥਿਰ bool ਸ਼ਾਮਲ_ਹੈ_MTask_Profile_ਅੰਕੜੇ();
    ਸਥਿਰ uint64_t get_Current_Hier_ਖੰਡ_ਲਾਗਤ();

    ਸਥਿਰ bool waive(ਅਚਲ File_ਸਤਰ* filelinep, V3_Error_ਕੋਡ ਕੋਡ, ਅਚਲ string& ਸੁਨੇਹਾ);

    ਸਥਿਰ ਖਾਲੀ selfTest();
};

#ਜੇ_ਅੰਤ  // Guard
