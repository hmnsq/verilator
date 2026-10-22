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

#که_نه_تعریف_شوی VERILATOR_V3CONFIG_H_
#تعریف_کړه VERILATOR_V3CONFIG_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"
#ضمن "V3Error.h"
#ضمن "V3FileLine.h"
#ضمن "V3Mutex.h"

//######################################################################

class V3Control final {
عام:
    بنية Fsm_ثبتوونکی_Wrapper final {
        string moduleName;
        string d;
        string q;
        string clock;
        string reset;
        string reset_ارزښت;
    };

    تعداد class Var_Spec_ډول : uint8_t {
        PARAM,  // Select only matching parameters
        پورټ,  // Select only matching ports
        VAR  // Select any matching AstVar (including params and ports)
    };

    ساكن فراغ جمع_کړه_Case_ډک(ثابت string& file, صحيح lineno);
    ساكن فراغ جمع_کړه_Case_Parallel(ثابت string& file, صحيح lineno);
    ساكن فراغ جمع_کړه_Coverage_بلاک_Off(ثابت string& file, صحيح lineno);
    ساكن فراغ جمع_کړه_Coverage_بلاک_Off(ثابت string& module, ثابت string& blockname);
    ساكن فراغ جمع_کړه_Hier_Workers(File_کرښه* fl, ثابت string& model, صحيح workers);
    ساكن فراغ جمع_کړه_Fsm_ثبتوونکی_Wrapper(File_کرښه* fl, ثابت string& module, ثابت string& d,
                                      ثابت string& q, ثابت string& clock, ثابت string& reset,
                                      ثابت string& reset_ارزښت);
    ساكن فراغ جمع_کړه_Ignore(V3_Error_کوډ کوډ, bool on, ثابت string& filename, صحيح min, صحيح max);
    ساكن فراغ جمع_کړه_Ignore_سمون(V3_Error_کوډ کوډ, ثابت string& filename, ثابت string& contents,
                               ثابت string& سمون);
    ساكن فراغ جمع_کړه_Inline(File_کرښه* fl, ثابت string& module, ثابت string& ftask, bool on);
    ساكن فراغ جمع_کړه_Module_Pragma(ثابت string& module, VPragmaType pragma);
    ساكن فراغ جمع_کړه_Profile_معلومات(File_کرښه* fl, ثابت string& hierDpi, uint64_t لګښت);
    ساكن فراغ جمع_کړه_Profile_معلومات(File_کرښه* fl, ثابت string& model, ثابت string& کلی,
                               uint64_t لګښت);
    ساكن فراغ جمع_کړه_Scope_Trace_On(bool on, ثابت string& scope, صحيح پوړونه);
    ساكن فراغ جمع_کړه_Var_Attr(File_کرښه* fl, ثابت string& module, ثابت string& ftask,
                           Var_Spec_ډول ډول, ثابت string& نمونه_2, VAttrType type,
                           Ast_Sen_ونه* nodep);

    ساكن فراغ applyCase(AstCase* nodep);
    ساكن فراغ apply_Coverage_بلاک(Ast_غوټه_Module* modulep, AstBegin* nodep);
    ساكن فراغ apply_Coverage_بلاک(Ast_غوټه_Module* modulep, Ast_Gen_بلاک* nodep);
    ساكن فراغ applyFTask(Ast_غوټه_Module* modulep, Ast_غوټه_FTask* ftaskp);
    ساكن فراغ applyIgnores(File_کرښه* filelinep);
    ساكن فراغ applyModule(Ast_غوټه_Module* modulep);
    ساكن فراغ applyVarAttr(ثابت Ast_غوټه_Module* modulep, ثابت Ast_غوټه_FTask* ftaskp,
                             AstVar* varp);

    ساكن صحيح getHierWorkers(ثابت string& model);
    ساكن File_کرښه* get_Hier_Workers_File_کرښه(ثابت string& model);
    ساكن ثابت Fsm_ثبتوونکی_Wrapper* get_Fsm_ثبتوونکی_Wrapper(ثابت string& module);
    ساكن uint64_t get_Profile_معلومات(ثابت string& hierDpi);
    ساكن uint64_t get_Profile_معلومات(ثابت string& model, ثابت string& کلی);
    ساكن File_کرښه* get_Profile_معلومات_File_کرښه();
    ساكن bool getScopeTraceOn(ثابت string& scope);

    ساكن فراغ contents_ټېل_متن(ثابت string& متن);

    ساكن bool لري_MTask_Profile_معلومات();
    ساكن uint64_t get_Current_Hier_بلاک_لګښت();

    ساكن bool waive(ثابت File_کرښه* filelinep, V3_Error_کوډ کوډ, ثابت string& پیغام);

    ساكن فراغ selfTest();
};

#د_که_پای  // Guard
