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

#यदि_अपरिभाषितम् VERILATOR_V3CONFIG_H_
#परिभाषय VERILATOR_V3CONFIG_H_

#अन्तर्भाव "config_build.h"
#अन्तर्भाव "verilatedos.h"

#अन्तर्भाव "V3Ast.h"
#अन्तर्भाव "V3Error.h"
#अन्तर्भाव "V3FileLine.h"
#अन्तर्भाव "V3Mutex.h"

//######################################################################

class V3Control final {
सार्वजनिक:
    संरचना Fsm_पञ्जिकाकोशः_Wrapper final {
        string moduleName;
        string d;
        string q;
        string clock;
        string reset;
        string reset_मानम्;
    };

    गणना class Var_Spec_विधा : uint8_t {
        PARAM,  // Select only matching parameters
        पत्तनम्,  // Select only matching ports
        VAR  // Select any matching AstVar (including params and ports)
    };

    स्थिर शून्य सङ्कलय_Case_पूर्णम्(ध्रुव string& file, पूर्णाङ्क lineno);
    स्थिर शून्य सङ्कलय_Case_Parallel(ध्रुव string& file, पूर्णाङ्क lineno);
    स्थिर शून्य सङ्कलय_Coverage_खण्डम्_Off(ध्रुव string& file, पूर्णाङ्क lineno);
    स्थिर शून्य सङ्कलय_Coverage_खण्डम्_Off(ध्रुव string& module, ध्रुव string& blockname);
    स्थिर शून्य सङ्कलय_Hier_Workers(File_पङ्क्तिः* fl, ध्रुव string& model, पूर्णाङ्क workers);
    स्थिर शून्य सङ्कलय_Fsm_पञ्जिकाकोशः_Wrapper(File_पङ्क्तिः* fl, ध्रुव string& module, ध्रुव string& d,
                                      ध्रुव string& q, ध्रुव string& clock, ध्रुव string& reset,
                                      ध्रुव string& reset_मानम्);
    स्थिर शून्य सङ्कलय_Ignore(V3_Error_संहिता संहिता, bool on, ध्रुव string& filename, पूर्णाङ्क min, पूर्णाङ्क max);
    स्थिर शून्य सङ्कलय_Ignore_मेलनम्(V3_Error_संहिता संहिता, ध्रुव string& filename, ध्रुव string& contents,
                               ध्रुव string& मेलनम्);
    स्थिर शून्य सङ्कलय_Inline(File_पङ्क्तिः* fl, ध्रुव string& module, ध्रुव string& ftask, bool on);
    स्थिर शून्य सङ्कलय_Module_Pragma(ध्रुव string& module, VPragmaType pragma);
    स्थिर शून्य सङ्कलय_Profile_दत्त(File_पङ्क्तिः* fl, ध्रुव string& hierDpi, uint64_t cost);
    स्थिर शून्य सङ्कलय_Profile_दत्त(File_पङ्क्तिः* fl, ध्रुव string& model, ध्रुव string& कुञ्चिका,
                               uint64_t cost);
    स्थिर शून्य सङ्कलय_Scope_Trace_On(bool on, ध्रुव string& scope, पूर्णाङ्क स्तराः);
    स्थिर शून्य सङ्कलय_Var_Attr(File_पङ्क्तिः* fl, ध्रुव string& module, ध्रुव string& ftask,
                           Var_Spec_विधा विधा, ध्रुव string& आदर्शः_2, VAttrType type,
                           Ast_Sen_वृक्षः* nodep);

    स्थिर शून्य applyCase(AstCase* nodep);
    स्थिर शून्य apply_Coverage_खण्डम्(Ast_ग्रन्थिः_Module* modulep, AstBegin* nodep);
    स्थिर शून्य apply_Coverage_खण्डम्(Ast_ग्रन्थिः_Module* modulep, Ast_Gen_खण्डम्* nodep);
    स्थिर शून्य applyFTask(Ast_ग्रन्थिः_Module* modulep, Ast_ग्रन्थिः_FTask* ftaskp);
    स्थिर शून्य applyIgnores(File_पङ्क्तिः* filelinep);
    स्थिर शून्य applyModule(Ast_ग्रन्थिः_Module* modulep);
    स्थिर शून्य applyVarAttr(ध्रुव Ast_ग्रन्थिः_Module* modulep, ध्रुव Ast_ग्रन्थिः_FTask* ftaskp,
                             AstVar* varp);

    स्थिर पूर्णाङ्क getHierWorkers(ध्रुव string& model);
    स्थिर File_पङ्क्तिः* get_Hier_Workers_File_पङ्क्तिः(ध्रुव string& model);
    स्थिर ध्रुव Fsm_पञ्जिकाकोशः_Wrapper* get_Fsm_पञ्जिकाकोशः_Wrapper(ध्रुव string& module);
    स्थिर uint64_t get_Profile_दत्त(ध्रुव string& hierDpi);
    स्थिर uint64_t get_Profile_दत्त(ध्रुव string& model, ध्रुव string& कुञ्चिका);
    स्थिर File_पङ्क्तिः* get_Profile_दत्त_File_पङ्क्तिः();
    स्थिर bool getScopeTraceOn(ध्रुव string& scope);

    स्थिर शून्य contents_प्रक्षिप_Text(ध्रुव string& text);

    स्थिर bool अन्तर्भवति_MTask_Profile_दत्त();
    स्थिर uint64_t get_Current_Hier_खण्डम्_Cost();

    स्थिर bool waive(ध्रुव File_पङ्क्तिः* filelinep, V3_Error_संहिता संहिता, ध्रुव string& सन्देशः);

    स्थिर शून्य selfTest();
};

#यदि_अन्तः  // Guard
