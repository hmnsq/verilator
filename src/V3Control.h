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

#اگر_غیر_تعریف_شدہ VERILATOR_V3CONFIG_H_
#تعریف_کرو VERILATOR_V3CONFIG_H_

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Ast.h"
#شامل "V3Error.h"
#شامل "V3FileLine.h"
#شامل "V3Mutex.h"

//######################################################################

class V3Control final {
عوامی:
    ساخت Fsm_رجسٹر_Wrapper final {
        string moduleName;
        string d;
        string q;
        string clock;
        string reset;
        string reset_قدر;
    };

    شمار class Var_Spec_قسم : uint8_t {
        PARAM,  // Select only matching parameters
        پورٹ,  // Select only matching ports
        VAR  // Select any matching AstVar (including params and ports)
    };

    ساکن خالی جوڑو_Case_بھرا(ثابت string& file, صحیح lineno);
    ساکن خالی جوڑو_Case_Parallel(ثابت string& file, صحیح lineno);
    ساکن خالی جوڑو_Coverage_بلاک_Off(ثابت string& file, صحیح lineno);
    ساکن خالی جوڑو_Coverage_بلاک_Off(ثابت string& module, ثابت string& blockname);
    ساکن خالی جوڑو_Hier_Workers(File_سطر* fl, ثابت string& model, صحیح workers);
    ساکن خالی جوڑو_Fsm_رجسٹر_Wrapper(File_سطر* fl, ثابت string& module, ثابت string& d,
                                      ثابت string& q, ثابت string& clock, ثابت string& reset,
                                      ثابت string& reset_قدر);
    ساکن خالی جوڑو_Ignore(V3_Error_رمز رمز, bool on, ثابت string& filename, صحیح min, صحیح max);
    ساکن خالی جوڑو_Ignore_مطابقت(V3_Error_رمز رمز, ثابت string& filename, ثابت string& contents,
                               ثابت string& مطابقت);
    ساکن خالی جوڑو_Inline(File_سطر* fl, ثابت string& module, ثابت string& ftask, bool on);
    ساکن خالی جوڑو_Module_Pragma(ثابت string& module, VPragmaType pragma);
    ساکن خالی جوڑو_Profile_ڈیٹا(File_سطر* fl, ثابت string& hierDpi, uint64_t لاگت);
    ساکن خالی جوڑو_Profile_ڈیٹا(File_سطر* fl, ثابت string& model, ثابت string& کلید,
                               uint64_t لاگت);
    ساکن خالی جوڑو_Scope_Trace_On(bool on, ثابت string& scope, صحیح سطحیں);
    ساکن خالی جوڑو_Var_Attr(File_سطر* fl, ثابت string& module, ثابت string& ftask,
                           Var_Spec_قسم قسم, ثابت string& نمونہ_2, VAttrType type,
                           Ast_Sen_درخت* nodep);

    ساکن خالی applyCase(AstCase* nodep);
    ساکن خالی apply_Coverage_بلاک(Ast_گرہ_Module* modulep, AstBegin* nodep);
    ساکن خالی apply_Coverage_بلاک(Ast_گرہ_Module* modulep, Ast_Gen_بلاک* nodep);
    ساکن خالی applyFTask(Ast_گرہ_Module* modulep, Ast_گرہ_FTask* ftaskp);
    ساکن خالی applyIgnores(File_سطر* filelinep);
    ساکن خالی applyModule(Ast_گرہ_Module* modulep);
    ساکن خالی applyVarAttr(ثابت Ast_گرہ_Module* modulep, ثابت Ast_گرہ_FTask* ftaskp,
                             AstVar* varp);

    ساکن صحیح getHierWorkers(ثابت string& model);
    ساکن File_سطر* get_Hier_Workers_File_سطر(ثابت string& model);
    ساکن ثابت Fsm_رجسٹر_Wrapper* get_Fsm_رجسٹر_Wrapper(ثابت string& module);
    ساکن uint64_t get_Profile_ڈیٹا(ثابت string& hierDpi);
    ساکن uint64_t get_Profile_ڈیٹا(ثابت string& model, ثابت string& کلید);
    ساکن File_سطر* get_Profile_ڈیٹا_File_سطر();
    ساکن bool getScopeTraceOn(ثابت string& scope);

    ساکن خالی contents_ڈالو_متن(ثابت string& متن);

    ساکن bool شامل_ہے_MTask_Profile_ڈیٹا();
    ساکن uint64_t get_Current_Hier_بلاک_لاگت();

    ساکن bool waive(ثابت File_سطر* filelinep, V3_Error_رمز رمز, ثابت string& پیغام);

    ساکن خالی selfTest();
};

#اگر_ختم  // Guard
