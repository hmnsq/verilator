// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Build DPI protected C++ and SV
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2003-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3ProtectLib.h"

#ضمن "V3Control.h"
#ضمن "V3Hasher.h"
#ضمن "V3InstrCount.h"
#ضمن "V3String.h"
#ضمن "V3Task.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// ProtectLib top-level visitor

صنف ProtectVisitor final : عام VNVisitor {
    // TYPES
    بنية InoutPorts final {
        AstVar* inp = تش_اشاره;  // Resolved input value and original public port
        AstVar* outp = تش_اشاره;  // Library drive value
        AstVar* enp = تش_اشاره;  // Per-bit drive enable
    };

    // MEMBERS
    AstVFile* m_vfilep = تش_اشاره;  // DPI-enabled Verilog wrapper
    AstCFile* m_cfilep = تش_اشاره;  // C implementation of DPI functions
    // Verilog text blocks
    Ast_متن_بلاک* m_modPortsp = تش_اشاره;  // Module port list
    Ast_متن_بلاک* m_comboPortsp = تش_اشاره;  // Combo function port list
    Ast_متن_بلاک* m_seqPortsp = تش_اشاره;  // Sequential function port list
    Ast_متن_بلاک* m_comboIgnorePortsp = تش_اشاره;  // Combo ignore function port list
    Ast_متن_بلاک* m_comboDeclsp = تش_اشاره;  // Combo signal declaration list
    Ast_متن_بلاک* m_seqDeclsp = تش_اشاره;  // Sequential signal declaration list
    Ast_متن_بلاک* m_tmpDeclsp = تش_اشاره;  // Temporary signal declaration list
    Ast_متن_بلاک* m_inoutDeclsp = تش_اشاره;  // Lowered inout signals and tristate drivers
    Ast_متن_بلاک* m_hashValuep = تش_اشاره;  // CPP hash value
    Ast_متن_بلاک* m_comboParamsp = تش_اشاره;  // Combo function parameter list
    Ast_متن_بلاک* m_clkSensp = تش_اشاره;  // Clock sensitivity list
    Ast_متن_بلاک* m_comboIgnoreParamsp = تش_اشاره;  // Combo ignore parameter list
    Ast_متن_بلاک* m_seqParamsp = تش_اشاره;  // Sequential parameter list
    Ast_متن_بلاک* m_nbAssignsp = تش_اشاره;  // Non-blocking assignment list
    Ast_متن_بلاک* m_seqAssignsp = تش_اشاره;  // Sequential assignment list
    Ast_متن_بلاک* m_comboAssignsp = تش_اشاره;  // Combo assignment list
    // C text blocks
    Ast_متن_بلاک* m_cHashValuep = تش_اشاره;  // CPP hash value
    Ast_متن_بلاک* m_cComboParamsp = تش_اشاره;  // Combo function parameter list
    Ast_متن_بلاک* m_cComboInsp = تش_اشاره;  // Combo input copy list
    Ast_متن_بلاک* m_cComboOutsp = تش_اشاره;  // Combo output copy list
    Ast_متن_بلاک* m_cSeqParamsp = تش_اشاره;  // Sequential parameter list
    Ast_متن_بلاک* m_cSeqClksp = تش_اشاره;  // Sequential clock copy list
    Ast_متن_بلاک* m_cSeqOutsp = تش_اشاره;  // Sequential output copy list
    Ast_متن_بلاک* m_cIgnoreParamsp = تش_اشاره;  // Combo ignore parameter list
    ثابت string m_libName;
    ثابت string m_سر_Name;
    منطقي m_وموندل_سر = خطأ;  // Have seen the top module
    منطقي m_hasClk = خطأ;  // True if the top module has sequential logic
    std::map<صحيح, InoutPorts> m_inouts;  // Lowered ports grouped by original inout

    // VISITORS
    فراغ لیدنه(AstNetlist* nodep) override {
        m_vfilep
            = جديد AstVFile{nodep->fileline(), v3Global.opt.makeDir() + "/" + m_libName + ".sv"};
        nodep->جمع_کړه_Filesp(m_vfilep);
        m_cfilep
            = جديد AstCFile{nodep->fileline(), v3Global.opt.makeDir() + "/" + m_libName + ".cpp"};
        nodep->جمع_کړه_Filesp(m_cfilep);
        iterateChildren(nodep);
    }

    فراغ لیدنه(Ast_غوټه_Module* nodep) override {
        إذا (!nodep->is_سر()) {
            أرجع;
        } وإلا {
            UASSERT_OBJ(!m_وموندل_سر, nodep, "Multiple root modules");
        }
        File_کرښه* ثابت fl = nodep->fileline();
        // Need to know the existence of clk before createSvFile()
        m_hasClk = checkIfClockExists(nodep);
        createSvFile(fl, nodep);
        createCppFile(fl);

        iterateChildren(nodep);

        restoreInouts();

        // cppcheck-suppress unreadVariable
        ثابت V3Hash hash = V3Hasher::uncachedHash(m_cfilep);
        m_hashValuep->جمع_کړه(cvt_منزل_Str(hash.ارزښت()) + ";\n");
        m_cHashValuep->جمع_کړه(cvt_منزل_Str(hash.ارزښت()) + "U;\n");
        m_وموندل_سر = ریښتیا;
    }

    فراغ جمع_کړه_Comment(Ast_متن_بلاک* txtp, File_کرښه* fl, ثابت string& comment) {
        txtp->جمع_کړه(جديد AstComment{fl, comment});
    }

    فراغ configSection(Ast_غوټه_Module* /*modp*/, Ast_متن_بلاک* txtp, File_کرښه* /*fl*/) {
        txtp->جمع_کړه("\n`ifdef VERILATOR\n");
        txtp->جمع_کړه("`verilator_config\n");

        txtp->جمع_کړه("verilator_lib -module \"" + m_libName + "\"\n");

        txtp->جمع_کړه("profile_data -hier-dpi \"" + m_libName + "_protectlib_combo_update\" -cost 64'd"
                  + std::منزل_string(v3Global.current_Hier_بلاک_لګښت()) + "\n");
        txtp->جمع_کړه("profile_data -hier-dpi \"" + m_libName + "_protectlib_seq_update\" -cost 64'd"
                  + std::منزل_string(v3Global.current_Hier_بلاک_لګښت()) + "\n");

        // Mark remaining NBA protectlib wrapper DPIs as non-hazardous by deliberately forwarding
        // them with non-zero cost.
        // Also, specify hierarchical workers for those tasks for scheduling.
        txtp->جمع_کړه("profile_data -hier-dpi \"" + m_libName
                  + "_protectlib_combo_ignore\" -cost 64'd1\n");

        txtp->جمع_کړه("hier_workers -hier-dpi \"" + m_libName
                  + "_protectlib_combo_update\" -workers 16'd"
                  + std::منزل_string(V3Control::getHierWorkers(m_libName)) + "\n");
        txtp->جمع_کړه("hier_workers -hier-dpi \"" + m_libName
                  + "_protectlib_seq_update\" -workers 16'd"
                  + std::منزل_string(V3Control::getHierWorkers(m_libName)) + "\n");
        // No workers for combo_ignore
        txtp->جمع_کړه("`verilog\n");
        txtp->جمع_کړه("`endif\n");
    }

    فراغ hashComment(Ast_متن_بلاک* txtp, File_کرښه* fl) {
        جمع_کړه_Comment(txtp, fl, "Checks to make sure the .sv wrapper and library agree");
    }

    فراغ initialComment(Ast_متن_بلاک* txtp, File_کرښه* fl) {
        جمع_کړه_Comment(txtp, fl, "Creates an instance of the library module at initial-time");
        جمع_کړه_Comment(txtp, fl, "(one for each instance in the user's design)");
    }

    فراغ comboComment(Ast_متن_بلاک* txtp, File_کرښه* fl) {
        جمع_کړه_Comment(txtp, fl, "Updates all non-clock inputs and retrieves the results");
    }

    فراغ seqComment(Ast_متن_بلاک* txtp, File_کرښه* fl) {
        جمع_کړه_Comment(txtp, fl, "Updates all clocks and retrieves the results");
    }

    فراغ comboIgnoreComment(Ast_متن_بلاک* txtp, File_کرښه* fl) {
        جمع_کړه_Comment(txtp, fl, "Need to convince some simulators that the input to the module");
        جمع_کړه_Comment(txtp, fl, "must be evaluated before evaluating the clock edge");
    }

    فراغ finalComment(Ast_متن_بلاک* txtp, File_کرښه* fl) {
        جمع_کړه_Comment(txtp, fl, "Evaluates the library module's final process");
    }

    فراغ createSvFile(File_کرښه* fl, Ast_غوټه_Module* modp) {
        // Comments
        Ast_متن_بلاک* ثابت txtp = جديد Ast_متن_بلاک{fl};
        جمع_کړه_Comment(txtp, fl, "Wrapper module for DPI protected library");
        جمع_کړه_Comment(txtp, fl,
                   "This module requires lib" + m_libName + ".a or lib" + m_libName
                       + ".so to work");
        جمع_کړه_Comment(txtp, fl,
                   "See instructions in your simulator for how"
                   " to use DPI libraries\n");

        // Module declaration
        // Wrapper ranges preserve the original RTL, including intentional ascending ranges.
        txtp->جمع_کړه("/*verilator lint_save*/\n");
        txtp->جمع_کړه("/*verilator lint_off ASCRANGE*/\n");
        m_modPortsp = جديد Ast_متن_بلاک{fl, "module " + m_libName + " (\n", ", ", ");\n\n"};
        txtp->جمع_کړه(m_modPortsp);

        // Timescale
        إذا (v3Global.opt.hier_ماشوم() && v3Global.rootp()->timescaleSpecified()) {
            // Emit timescale for hierarchical Verilation if input HDL specifies timespec
            txtp->جمع_کړه("timeunit "s + modp->timeunit().ascii() + ";\n");
            txtp->جمع_کړه("timeprecision "s + +v3Global.rootp()->timeprecision().ascii() + ";\n");
        } وإلا {
            جمع_کړه_Comment(txtp, fl,
                       "Precision of submodule"
                       " (commented out to avoid requiring timescale on all modules)");
            جمع_کړه_Comment(txtp, fl, "timeunit "s + v3Global.rootp()->timeunit().ascii() + ";");
            جمع_کړه_Comment(txtp, fl,
                       "timeprecision "s + v3Global.rootp()->timeprecision().ascii() + ";\n");
        }

        // DPI declarations
        hashComment(txtp, fl);
        txtp->جمع_کړه("import \"DPI-C\" function void " + m_libName
                  + "_protectlib_check_hash(int protectlib_hash__V);\n\n");
        initialComment(txtp, fl);
        txtp->جمع_کړه("import \"DPI-C\" function chandle " + m_libName
                  + "_protectlib_create(string scope__V);\n\n");
        comboComment(txtp, fl);
        m_comboPortsp = جديد Ast_متن_بلاک{fl,  //
                                         "import \"DPI-C\" function longint " + m_libName
                                             + "_protectlib_combo_update(\n",  //
                                         ", ",  //
                                         ");\n\n"};
        m_comboPortsp->جمع_کړه("chandle handle__V\n");
        txtp->جمع_کړه(m_comboPortsp);
        seqComment(txtp, fl);
        إذا (m_hasClk) {
            m_seqPortsp = جديد Ast_متن_بلاک{fl,  //
                                           "import \"DPI-C\" function longint " + m_libName
                                               + "_protectlib_seq_update(\n",  //
                                           ", ",  //
                                           ");\n\n"};
            m_seqPortsp->جمع_کړه("chandle handle__V\n");
            txtp->جمع_کړه(m_seqPortsp);
        }
        comboIgnoreComment(txtp, fl);
        m_comboIgnorePortsp = جديد Ast_متن_بلاک{fl,  //
                                               "import \"DPI-C\" function void " + m_libName
                                                   + "_protectlib_combo_ignore(\n",  //
                                               ", ",  //
                                               ");\n\n"};
        m_comboIgnorePortsp->جمع_کړه("chandle handle__V\n");
        txtp->جمع_کړه(m_comboIgnorePortsp);

        finalComment(txtp, fl);
        txtp->جمع_کړه("import \"DPI-C\" function void " + m_libName
                  + "_protectlib_final(chandle handle__V);\n\n");

        // Local variables
        // Avoid tracing handle, as it is not a stable value, so breaks wavediff
        // Likewise other internals aren't interesting to the user
        txtp->جمع_کړه("// verilator tracing_off\n");

        txtp->جمع_کړه("chandle handle__V;\n");
        txtp->جمع_کړه("time last_combo_seqnum__V;\n");
        إذا (m_hasClk) txtp->جمع_کړه("time last_seq_seqnum__V;\n");
        txtp->جمع_کړه("\n");

        m_comboDeclsp = جديد Ast_متن_بلاک{fl};
        txtp->جمع_کړه(m_comboDeclsp);
        m_seqDeclsp = جديد Ast_متن_بلاک{fl};
        txtp->جمع_کړه(m_seqDeclsp);
        m_tmpDeclsp = جديد Ast_متن_بلاک{fl};
        txtp->جمع_کړه(m_tmpDeclsp);
        m_inoutDeclsp = جديد Ast_متن_بلاک{fl};
        txtp->جمع_کړه(m_inoutDeclsp);

        // CPP hash value
        جمع_کړه_Comment(txtp, fl, "Hash value to make sure this file and the corresponding");
        جمع_کړه_Comment(txtp, fl, "library agree");
        m_hashValuep = جديد Ast_متن_بلاک{
            fl, "localparam int protectlib_hash__V /*verilator public_flat_rd*/ = 32'd", "", "\n"};
        txtp->جمع_کړه(m_hashValuep);

        // Initial
        txtp->جمع_کړه("initial begin\n");
        txtp->جمع_کړه(m_libName + "_protectlib_check_hash(protectlib_hash__V);\n");
        txtp->جمع_کړه("if (handle__V == null) handle__V = " + m_libName
                  + "_protectlib_create"
                    "($sformatf(\"%m\"));\n");
        txtp->جمع_کړه("end\n\n");

        // Combinatorial process
        جمع_کړه_Comment(txtp, fl, "Combinatorially evaluate changes to inputs");
        txtp->جمع_کړه("always_comb begin\n");
        m_comboParamsp = جديد Ast_متن_بلاک{fl,  //
                                          "last_combo_seqnum__V = " + m_libName
                                              + "_protectlib_combo_update(\n",  //
                                          ",\n",  //
                                          "\n);\n"};
        m_comboParamsp->جمع_کړه("handle__V");
        txtp->جمع_کړه(m_comboParamsp);
        txtp->جمع_کړه("end\n\n");

        // Sequential process
        إذا (m_hasClk) {
            جمع_کړه_Comment(txtp, fl, "Evaluate clock edges");
            m_clkSensp = جديد Ast_متن_بلاک{fl, "always @(", " or ", ")"};
            txtp->جمع_کړه(m_clkSensp);
            txtp->جمع_کړه(" begin\n");
            m_comboIgnoreParamsp
                = جديد Ast_متن_بلاک{fl,  //
                                   m_libName + "_protectlib_combo_ignore(\n", ",\n", "\n);\n"};
            m_comboIgnoreParamsp->جمع_کړه("handle__V");
            txtp->جمع_کړه(m_comboIgnoreParamsp);
            m_seqParamsp = جديد Ast_متن_بلاک{fl,  //
                                            "last_seq_seqnum__V <= " + m_libName
                                                + "_protectlib_seq_update(\n",  //
                                            ",\n",  //
                                            "\n);\n"};
            m_seqParamsp->جمع_کړه("handle__V");
            txtp->جمع_کړه(m_seqParamsp);
            m_nbAssignsp = جديد Ast_متن_بلاک{fl};
            txtp->جمع_کړه(m_nbAssignsp);
            txtp->جمع_کړه("end\n\n");
        }

        // Select between combinatorial and sequential results
        جمع_کړه_Comment(txtp, fl, "Select between combinatorial and sequential results");
        txtp->جمع_کړه("always_comb begin\n");
        إذا (m_hasClk) {
            txtp->جمع_کړه("if (last_seq_seqnum__V > last_combo_seqnum__V) begin\n");
            m_seqAssignsp = جديد Ast_متن_بلاک{fl};
            txtp->جمع_کړه(m_seqAssignsp);
            txtp->جمع_کړه("end else begin\n");
            m_comboAssignsp = جديد Ast_متن_بلاک{fl};
            txtp->جمع_کړه(m_comboAssignsp);
            txtp->جمع_کړه("end\n");
        } وإلا {
            m_comboAssignsp = جديد Ast_متن_بلاک{fl};
            txtp->جمع_کړه(m_comboAssignsp);
        }
        txtp->جمع_کړه("end\n\n");

        // Final
        txtp->جمع_کړه("final " + m_libName + "_protectlib_final(handle__V);\n\n");

        txtp->جمع_کړه("endmodule\n");
        txtp->جمع_کړه("/*verilator lint_restore*/\n");

        configSection(modp, txtp, fl);

        m_vfilep->tblockp(txtp);
    }

    فراغ castPtr(File_کرښه* /*fl*/, Ast_متن_بلاک* txtp) {
        txtp->جمع_کړه(m_سر_Name
                  + "_container* const handlep__V = "  // LCOV_EXCL_LINE  // lcov bug
                    "static_cast<"
                  + m_سر_Name + "_container*>(vhandlep__V);\n");
    }

    فراغ createCppFile(File_کرښه* fl) {
        // Comments
        Ast_متن_بلاک* ثابت txtp = جديد Ast_متن_بلاک{fl};
        جمع_کړه_Comment(txtp, fl, "Wrapper functions for DPI protected library\n");

        // Includes
        txtp->جمع_کړه("#include \"" + m_سر_Name + ".h\"\n");
        txtp->جمع_کړه("#include \"verilated_dpi.h\"\n\n");
        txtp->جمع_کړه("#include <cstdio>\n");
        txtp->جمع_کړه("#include <cstdlib>\n\n");

        // Verilated module plus sequence number
        جمع_کړه_Comment(txtp, fl, "Container class to house verilated object and sequence number");
        txtp->جمع_کړه("class " + m_سر_Name + "_container: public " + m_سر_Name + " {\n");
        txtp->جمع_کړه("public:\n");
        txtp->جمع_کړه("long long m_seqnum;\n");
        txtp->جمع_کړه(m_سر_Name + "_container(const char* scopep__V):\n");
        txtp->جمع_کړه(m_سر_Name + "(scopep__V) {}\n");
        txtp->جمع_کړه("};\n\n");

        // Extern C
        txtp->جمع_کړه("extern \"C\" {\n\n");

        // Hash check
        hashComment(txtp, fl);
        txtp->جمع_کړه("void " + m_libName
                  + "_protectlib_check_hash"
                    "(int protectlib_hash__V) {\n");
        m_cHashValuep = جديد Ast_متن_بلاک{fl, "const int expected_hash__V = ", "", ""};
        txtp->جمع_کړه(m_cHashValuep);
        txtp->جمع_کړه(/**/ "if (protectlib_hash__V != expected_hash__V) {\n");
        txtp->جمع_کړه(/****/ "fprintf(stderr, \"%%Error: cannot use " + m_libName
                  + " library, "
                    "Verliog (%u) and library (%u) hash values do not "
                    "agree\\n\", protectlib_hash__V, expected_hash__V);\n");
        txtp->جمع_کړه(/****/ "std::exit(EXIT_FAILURE);\n");
        txtp->جمع_کړه(/**/ "}\n");
        txtp->جمع_کړه("}\n\n");

        // Initial
        initialComment(txtp, fl);
        txtp->جمع_کړه("void* " + m_libName + "_protectlib_create(const char* scopep__V) {\n");
        txtp->جمع_کړه(/**/ m_سر_Name + "_container* const handlep__V = new " + m_سر_Name
                  + "_container{scopep__V};\n");
        txtp->جمع_کړه(/**/ "return handlep__V;\n");
        txtp->جمع_کړه("}\n\n");

        // Updates
        comboComment(txtp, fl);
        m_cComboParamsp = جديد Ast_متن_بلاک{
            fl, "long long " + m_libName + "_protectlib_combo_update(\n", ",\n", "\n) {\n"};
        m_cComboParamsp->جمع_کړه("void* vhandlep__V");
        txtp->جمع_کړه(m_cComboParamsp);
        m_cComboInsp = جديد Ast_متن_بلاک{fl};
        castPtr(fl, m_cComboInsp);
        txtp->جمع_کړه(m_cComboInsp);
        txtp->جمع_کړه("handlep__V->eval();\n");
        m_cComboOutsp = جديد Ast_متن_بلاک{fl};
        txtp->جمع_کړه(m_cComboOutsp);
        txtp->جمع_کړه("return handlep__V->m_seqnum++;\n");
        txtp->جمع_کړه("}\n\n");

        إذا (m_hasClk) {
            seqComment(txtp, fl);
            m_cSeqParamsp = جديد Ast_متن_بلاک{
                fl, "long long " + m_libName + "_protectlib_seq_update(\n", ",\n", "\n) {\n"};
            m_cSeqParamsp->جمع_کړه("void* vhandlep__V");
            txtp->جمع_کړه(m_cSeqParamsp);
            m_cSeqClksp = جديد Ast_متن_بلاک{fl};
            castPtr(fl, m_cSeqClksp);
            txtp->جمع_کړه(m_cSeqClksp);
            txtp->جمع_کړه("handlep__V->eval();\n");
            m_cSeqOutsp = جديد Ast_متن_بلاک{fl};
            txtp->جمع_کړه(m_cSeqOutsp);
            txtp->جمع_کړه("return handlep__V->m_seqnum++;\n");
            txtp->جمع_کړه("}\n\n");
        }

        comboIgnoreComment(txtp, fl);
        m_cIgnoreParamsp = جديد Ast_متن_بلاک{
            fl, "void " + m_libName + "_protectlib_combo_ignore(\n", ",\n", "\n)\n{ }\n\n"};
        m_cIgnoreParamsp->جمع_کړه("void* vhandlep__V");
        txtp->جمع_کړه(m_cIgnoreParamsp);

        // Final
        finalComment(txtp, fl);
        txtp->جمع_کړه("void " + m_libName + "_protectlib_final(void* vhandlep__V) {\n");
        castPtr(fl, txtp);
        txtp->جمع_کړه(/**/ "handlep__V->final();\n");
        txtp->جمع_کړه(/**/ "delete handlep__V;\n");
        txtp->جمع_کړه("}\n\n");

        txtp->جمع_کړه("}\n");
        m_cfilep->tblockp(txtp);
    }

    فراغ لیدنه(AstVar* nodep) override {
        إذا (!nodep->isIO()) أرجع;
        إذا (restoreInout(nodep)) {
            InoutPorts& ports = m_inouts[nodep->libInoutId()];
            إذا (nodep->is_ننوت()) {
                ports.inp = nodep;
            } وإلا إذا (nodep->libInoutEnable()) {
                ports.enp = nodep;
            } وإلا {
                ports.outp = nodep;
            }
        }
        إذا (nodep->direction() == VDirection::ننوت) {
            إذا (nodep->isPrimaryClock()) {
                UASSERT_OBJ(m_hasClk, nodep, "checkIfClockExists() didn't find this clock");
                handleClock(nodep);
            } وإلا {
                handle_معلومات_ننوت(nodep);
            }
        } وإلا {
            // Ref ports were rejected during linking, and tristate lowering split all inouts.
            UASSERT_OBJ(nodep->direction() == VDirection::OUTPUT, nodep,
                        "Unexpected library port direction");
            handleOutput(nodep);
        }
    }

    فراغ لیدنه(Ast_غوټه*) override {}

    string c_ننوت_Connection(AstVar* varp) {
        أرجع V3Task::ټاکل_Dpi_منزل_دننی("handlep__V->" + varp->name(), varp);
    }

    // The wrapper's module ports are connected by name, so they must use the user's
    // names rather than the hashed form of long names used internally
    ساكن string sv_پورټ_Name(ثابت AstVar* varp) {
        أرجع Ast_غوټه::prettyName(VName::dehash(varp->name()));
    }

    ساكن AstVar* new_Sv_پورټ_Var(AstVar* varp) {
        AstVar* ثابت newVarp = varp->clone_ونه(خطأ);
        newVarp->name(VName::dehash(varp->name()));
        أرجع newVarp;
    }

    فراغ handleClock(AstVar* varp) {
        handle_ننوت(varp);
        m_seqPortsp->جمع_کړه(varp->clone_ونه(خطأ));
        إذا (m_hasClk) {
            ثابت std::string pname = sv_پورټ_Name(varp);
            m_seqParamsp->جمع_کړه(pname);
            m_clkSensp->جمع_کړه(pname);
        }
        m_cSeqParamsp->جمع_کړه(varp->dpiArgType(ریښتیا, خطأ));
        m_cSeqClksp->جمع_کړه(c_ننوت_Connection(varp));
    }

    فراغ handle_معلومات_ننوت(AstVar* varp) {
        handle_ننوت(varp);
        m_comboPortsp->جمع_کړه(varp->clone_ونه(خطأ));
        m_comboParamsp->جمع_کړه(sv_پورټ_Name(varp));
        m_comboIgnorePortsp->جمع_کړه(varp->clone_ونه(خطأ));
        إذا (m_hasClk) m_comboIgnoreParamsp->جمع_کړه(sv_پورټ_Name(varp));
        m_cComboParamsp->جمع_کړه(varp->dpiArgType(ریښتیا, خطأ));
        m_cComboInsp->جمع_کړه(c_ننوت_Connection(varp));
        m_cIgnoreParamsp->جمع_کړه(varp->dpiArgType(ریښتیا, خطأ));
    }

    ساكن منطقي restoreInout(ثابت AstVar* varp) {
        أرجع varp->libInoutId() && !v3Global.opt.pinsInoutEnables();
    }

    فراغ handle_ننوت(AstVar* varp) {
        AstVar* ثابت portp = new_Sv_پورټ_Var(varp);
        إذا (restoreInout(varp)) {
            portp->direction(VDirection::INOUT);
            portp->varType(VVarType::WIRE);
        }
        m_modPortsp->جمع_کړه(portp);
    }

    فراغ restoreInouts() {
        لكل (ثابت خپلکاری& ثبت : m_inouts) {
            ثابت InoutPorts& ports = ثبت.دوهم;
            UASSERT_OBJ(ports.inp && ports.outp && ports.enp, m_vfilep,
                        "Incomplete lowered library inout");
            ثابت string دننه = sv_پورټ_Name(ports.inp);
            ثابت string بهر = ports.outp->prettyName();
            ثابت string en = ports.enp->prettyName();
            std::ویکټور<std::pair<صحيح, صحيح>> dimensions;
            ثابت Ast_غوټه_DType* dtypep = ports.inp->dtypep()->skipRefp();
            طالما (ثابت Ast_غوټه_Array_DType* ثابت arrayp = VN_CAST(dtypep, غوټه_Array_DType)) {
                // V3Slice currently rejects lowered unpacked inouts before this pass.
                // TODO: Support unpacked inouts once V3Slice supports their lowered form.
                UASSERT_OBJ(!VN_IS(arrayp, UnpackArrayDType), ports.inp,
                            "Unpacked library inout should have been rejected by V3Slice");
                dimensions.emplace_شا(arrayp->lo(), arrayp->hi());
                dtypep = arrayp->subDTypep()->skipRefp();
            }
            ثابت AstBasicDType* ثابت basicp = dtypep->basicp();
            UASSERT_OBJ(basicp, ports.inp, "Library inout must have a packed integral type");
            إذا (basicp->isRanged()) dimensions.emplace_شا(basicp->lo(), basicp->hi());
            إذا (dimensions.تشه()) {
                m_inoutDeclsp->جمع_کړه("assign " + دننه + " = " + en + " ? " + بهر + " : 1'bz;\n");
            } وإلا {
                string lhs = دننه;
                string rhsEn = en;
                string rhs_بهر = بهر;
                لكل (اندازه_t dim = 0; dim < dimensions.اندازه(); ++dim) {
                    ثابت string شاخص
                        = "__VlibInout" + cvt_منزل_Str(ثبت.لومړی) + "_" + cvt_منزل_Str(dim);
                    ثابت صحيح ټیټ = dimensions[dim].لومړی;
                    ثابت صحيح لوړ = dimensions[dim].دوهم;
                    ثابت صحيح اندازه = لوړ - ټیټ + 1;
                    m_inoutDeclsp->جمع_کړه("for (genvar " + شاخص + " = 0; " + شاخص + " < "
                                       + cvt_منزل_Str(اندازه) + "; ++" + شاخص + ") begin\n");
                    ثابت string select = "[" + cvt_منزل_Str(ټیټ) + " + " + شاخص + "]";
                    lhs += select;
                    rhsEn += select;
                    rhs_بهر += select;
                }
                m_inoutDeclsp->جمع_کړه("assign " + lhs + " = " + rhsEn + " ? " + rhs_بهر
                                   + " : 1'bz;\n");
                لكل (اندازه_t dim = 0; dim < dimensions.اندازه(); ++dim) m_inoutDeclsp->جمع_کړه("end\n");
            }
        }
    }

    ساكن فراغ جمع_کړه_Local_Variable(Ast_متن_بلاک* textp, ثابت AstVar* varp, ثابت حرف* suffix) {
        AstVar* ثابت newVarp
            = جديد AstVar{varp->fileline(), VVarType::VAR, varp->name() + suffix, varp->dtypep()};
        textp->جمع_کړه(newVarp);
    }

    فراغ handleOutput(AstVar* ثابت varp) {
        ثابت std::string pname = varp->prettyName();
        // Restored inout drivers are local signals and retain their internal names.
        ثابت std::string پورټ_Name = restoreInout(varp) ? pname : sv_پورټ_Name(varp);
        إذا (restoreInout(varp)) {
            AstVar* ثابت localp = varp->clone_ونه(خطأ);
            localp->direction(VDirection::NONE);
            m_inoutDeclsp->جمع_کړه(localp);
        } وإلا {
            m_modPortsp->جمع_کړه(new_Sv_پورټ_Var(varp));
        }
        m_comboPortsp->جمع_کړه(varp->clone_ونه(خطأ));
        m_comboParamsp->جمع_کړه(pname + "_combo__V");
        إذا (m_hasClk) {
            m_seqPortsp->جمع_کړه(varp->clone_ونه(خطأ));
            m_seqParamsp->جمع_کړه(pname + "_tmp__V");
        }

        جمع_کړه_Local_Variable(m_comboDeclsp, varp, "_combo__V");

        إذا (m_hasClk) {
            جمع_کړه_Local_Variable(m_seqDeclsp, varp, "_seq__V");
            جمع_کړه_Local_Variable(m_tmpDeclsp, varp, "_tmp__V");

            m_nbAssignsp->جمع_کړه(pname + "_seq__V <= " + pname + "_tmp__V;\n");
            m_seqAssignsp->جمع_کړه(پورټ_Name + " = " + pname + "_seq__V;\n");
        }
        m_comboAssignsp->جمع_کړه(پورټ_Name + " = " + pname + "_combo__V;\n");
        m_cComboParamsp->جمع_کړه(varp->dpiArgType(ریښتیا, خطأ));
        m_cComboOutsp->جمع_کړه(V3Task::ټاکل_دننی_منزل_Dpi(varp, ریښتیا, "", "", "handlep__V->") + "\n");
        إذا (m_hasClk) {
            m_cSeqParamsp->جمع_کړه(varp->dpiArgType(ریښتیا, خطأ));
            m_cSeqOutsp->جمع_کړه(V3Task::ټاکل_دننی_منزل_Dpi(varp, ریښتیا, "", "", "handlep__V->")
                             + "\n");
        }
    }

    ساكن منطقي checkIfClockExists(ثابت Ast_غوټه_Module* modp) {
        لكل (ثابت Ast_غوټه* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (ثابت AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                إذا (varp->direction() == VDirection::ننوت && varp->isPrimaryClock()) {
                    أرجع ریښتیا;
                }
            }
        }
        أرجع خطأ;
    }

عام:
    څرګند ProtectVisitor(Ast_غوټه* nodep)
        : m_libName{v3Global.opt.libCreate()}
        , m_سر_Name{v3Global.opt.مختاړی()} {
        iterate(nodep);
    }
};

//######################################################################
// ProtectLib class functions

فراغ V3ProtectLib::protect() {
    UINFO(2, __FUNCTION__ << ":");
    ProtectVisitor{v3Global.rootp()};
}
