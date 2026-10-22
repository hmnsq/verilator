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

#جيڪڏهن_وصف_نه_ٿيل VERILATOR_V3CONFIG_H_
#وصف_ڪريو VERILATOR_V3CONFIG_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"
#ضمن "V3Error.h"
#ضمن "V3FileLine.h"
#ضمن "V3Mutex.h"

//######################################################################

class V3Control final {
عام:
    بنية Fsm_رجسٽر_Wrapper final {
        string moduleName;
        string d;
        string q;
        string clock;
        string reset;
        string reset_قدر;
    };

    تعداد class Var_Spec_قسم : uint8_t {
        PARAM,  // Select only matching parameters
        پورٽ,  // Select only matching ports
        VAR  // Select any matching AstVar (including params and ports)
    };

    ساكن فراغ جوڙيو_Case_ڀريل(ثابت string& file, صحيح lineno);
    ساكن فراغ جوڙيو_Case_Parallel(ثابت string& file, صحيح lineno);
    ساكن فراغ جوڙيو_Coverage_بلاڪ_Off(ثابت string& file, صحيح lineno);
    ساكن فراغ جوڙيو_Coverage_بلاڪ_Off(ثابت string& module, ثابت string& blockname);
    ساكن فراغ جوڙيو_Hier_Workers(File_سٽ* fl, ثابت string& model, صحيح workers);
    ساكن فراغ جوڙيو_Fsm_رجسٽر_Wrapper(File_سٽ* fl, ثابت string& module, ثابت string& d,
                                      ثابت string& q, ثابت string& clock, ثابت string& reset,
                                      ثابت string& reset_قدر);
    ساكن فراغ جوڙيو_Ignore(V3_Error_ڪوڊ ڪوڊ, bool on, ثابت string& filename, صحيح min, صحيح max);
    ساكن فراغ جوڙيو_Ignore_ميلاپ(V3_Error_ڪوڊ ڪوڊ, ثابت string& filename, ثابت string& contents,
                               ثابت string& ميلاپ);
    ساكن فراغ جوڙيو_Inline(File_سٽ* fl, ثابت string& module, ثابت string& ftask, bool on);
    ساكن فراغ جوڙيو_Module_Pragma(ثابت string& module, VPragmaType pragma);
    ساكن فراغ جوڙيو_Profile_معلومات(File_سٽ* fl, ثابت string& hierDpi, uint64_t لاڳت);
    ساكن فراغ جوڙيو_Profile_معلومات(File_سٽ* fl, ثابت string& model, ثابت string& ڪنجي,
                               uint64_t لاڳت);
    ساكن فراغ جوڙيو_Scope_Trace_On(bool on, ثابت string& scope, صحيح سطحون);
    ساكن فراغ جوڙيو_Var_Attr(File_سٽ* fl, ثابت string& module, ثابت string& ftask,
                           Var_Spec_قسم قسم, ثابت string& نمونو_2, VAttrType type,
                           Ast_Sen_وڻ* nodep);

    ساكن فراغ applyCase(AstCase* nodep);
    ساكن فراغ apply_Coverage_بلاڪ(Ast_ڳنڍ_Module* modulep, AstBegin* nodep);
    ساكن فراغ apply_Coverage_بلاڪ(Ast_ڳنڍ_Module* modulep, Ast_Gen_بلاڪ* nodep);
    ساكن فراغ applyFTask(Ast_ڳنڍ_Module* modulep, Ast_ڳنڍ_FTask* ftaskp);
    ساكن فراغ applyIgnores(File_سٽ* filelinep);
    ساكن فراغ applyModule(Ast_ڳنڍ_Module* modulep);
    ساكن فراغ applyVarAttr(ثابت Ast_ڳنڍ_Module* modulep, ثابت Ast_ڳنڍ_FTask* ftaskp,
                             AstVar* varp);

    ساكن صحيح getHierWorkers(ثابت string& model);
    ساكن File_سٽ* get_Hier_Workers_File_سٽ(ثابت string& model);
    ساكن ثابت Fsm_رجسٽر_Wrapper* get_Fsm_رجسٽر_Wrapper(ثابت string& module);
    ساكن uint64_t get_Profile_معلومات(ثابت string& hierDpi);
    ساكن uint64_t get_Profile_معلومات(ثابت string& model, ثابت string& ڪنجي);
    ساكن File_سٽ* get_Profile_معلومات_File_سٽ();
    ساكن bool getScopeTraceOn(ثابت string& scope);

    ساكن فراغ contents_ڌڪو_متن(ثابت string& متن);

    ساكن bool شامل_آهي_MTask_Profile_معلومات();
    ساكن uint64_t get_Current_Hier_بلاڪ_لاڳت();

    ساكن bool waive(ثابت File_سٽ* filelinep, V3_Error_ڪوڊ ڪوڊ, ثابت string& پيغام);

    ساكن فراغ selfTest();
};

#جيڪڏهن_پورو  // Guard
