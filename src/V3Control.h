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

#جے_بنا_تعریف VERILATOR_V3CONFIG_H_
#تعریف_کرو VERILATOR_V3CONFIG_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"
#ضمن "V3Error.h"
#ضمن "V3FileLine.h"
#ضمن "V3Mutex.h"

//######################################################################

class V3Control final {
عام:
    بنية Fsm_رجسٹر_Wrapper final {
        string moduleName;
        string d;
        string q;
        string clock;
        string reset;
        string reset_قدر;
    };

    تعداد class Var_Spec_قسم : uint8_t {
        PARAM,  // Select only matching parameters
        پورٹ,  // Select only matching ports
        VAR  // Select any matching AstVar (including params and ports)
    };

    ساكن فراغ جوڑو_Case_بھریا(ثابت string& file, صحيح lineno);
    ساكن فراغ جوڑو_Case_Parallel(ثابت string& file, صحيح lineno);
    ساكن فراغ جوڑو_Coverage_بلاک_Off(ثابت string& file, صحيح lineno);
    ساكن فراغ جوڑو_Coverage_بلاک_Off(ثابت string& module, ثابت string& blockname);
    ساكن فراغ جوڑو_Hier_Workers(File_سطر* fl, ثابت string& model, صحيح workers);
    ساكن فراغ جوڑو_Fsm_رجسٹر_Wrapper(File_سطر* fl, ثابت string& module, ثابت string& d,
                                      ثابت string& q, ثابت string& clock, ثابت string& reset,
                                      ثابت string& reset_قدر);
    ساكن فراغ جوڑو_Ignore(V3_Error_کوڈ کوڈ, bool on, ثابت string& filename, صحيح min, صحيح max);
    ساكن فراغ جوڑو_Ignore_میل(V3_Error_کوڈ کوڈ, ثابت string& filename, ثابت string& contents,
                               ثابت string& میل);
    ساكن فراغ جوڑو_Inline(File_سطر* fl, ثابت string& module, ثابت string& ftask, bool on);
    ساكن فراغ جوڑو_Module_Pragma(ثابت string& module, VPragmaType pragma);
    ساكن فراغ جوڑو_Profile_انکڑے(File_سطر* fl, ثابت string& hierDpi, uint64_t لاگت);
    ساكن فراغ جوڑو_Profile_انکڑے(File_سطر* fl, ثابت string& model, ثابت string& کلید,
                               uint64_t لاگت);
    ساكن فراغ جوڑو_Scope_Trace_On(bool on, ثابت string& scope, صحيح پدھراں);
    ساكن فراغ جوڑو_Var_Attr(File_سطر* fl, ثابت string& module, ثابت string& ftask,
                           Var_Spec_قسم قسم, ثابت string& نمونہ_2, VAttrType type,
                           Ast_Sen_درخت* nodep);

    ساكن فراغ applyCase(AstCase* nodep);
    ساكن فراغ apply_Coverage_بلاک(Ast_گرہ_Module* modulep, AstBegin* nodep);
    ساكن فراغ apply_Coverage_بلاک(Ast_گرہ_Module* modulep, Ast_Gen_بلاک* nodep);
    ساكن فراغ applyFTask(Ast_گرہ_Module* modulep, Ast_گرہ_FTask* ftaskp);
    ساكن فراغ applyIgnores(File_سطر* filelinep);
    ساكن فراغ applyModule(Ast_گرہ_Module* modulep);
    ساكن فراغ applyVarAttr(ثابت Ast_گرہ_Module* modulep, ثابت Ast_گرہ_FTask* ftaskp,
                             AstVar* varp);

    ساكن صحيح getHierWorkers(ثابت string& model);
    ساكن File_سطر* get_Hier_Workers_File_سطر(ثابت string& model);
    ساكن ثابت Fsm_رجسٹر_Wrapper* get_Fsm_رجسٹر_Wrapper(ثابت string& module);
    ساكن uint64_t get_Profile_انکڑے(ثابت string& hierDpi);
    ساكن uint64_t get_Profile_انکڑے(ثابت string& model, ثابت string& کلید);
    ساكن File_سطر* get_Profile_انکڑے_File_سطر();
    ساكن bool getScopeTraceOn(ثابت string& scope);

    ساكن فراغ contents_ڈالو_متن(ثابت string& متن);

    ساكن bool شامل_اے_MTask_Profile_انکڑے();
    ساكن uint64_t get_Current_Hier_بلاک_لاگت();

    ساكن bool waive(ثابت File_سطر* filelinep, V3_Error_کوڈ کوڈ, ثابت string& سنیہا);

    ساكن فراغ selfTest();
};

#جے_انت  // Guard
