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
        AstVar* inp = خالي_اشارو;  // Resolved input value and original public port
        AstVar* outp = خالي_اشارو;  // Library drive value
        AstVar* enp = خالي_اشارو;  // Per-bit drive enable
    };

    // MEMBERS
    AstVFile* m_vfilep = خالي_اشارو;  // DPI-enabled Verilog wrapper
    AstCFile* m_cfilep = خالي_اشارو;  // C implementation of DPI functions
    // Verilog text blocks
    Ast_متن_بلاڪ* m_modPortsp = خالي_اشارو;  // Module port list
    Ast_متن_بلاڪ* m_comboPortsp = خالي_اشارو;  // Combo function port list
    Ast_متن_بلاڪ* m_seqPortsp = خالي_اشارو;  // Sequential function port list
    Ast_متن_بلاڪ* m_comboIgnorePortsp = خالي_اشارو;  // Combo ignore function port list
    Ast_متن_بلاڪ* m_comboDeclsp = خالي_اشارو;  // Combo signal declaration list
    Ast_متن_بلاڪ* m_seqDeclsp = خالي_اشارو;  // Sequential signal declaration list
    Ast_متن_بلاڪ* m_tmpDeclsp = خالي_اشارو;  // Temporary signal declaration list
    Ast_متن_بلاڪ* m_inoutDeclsp = خالي_اشارو;  // Lowered inout signals and tristate drivers
    Ast_متن_بلاڪ* m_hashValuep = خالي_اشارو;  // CPP hash value
    Ast_متن_بلاڪ* m_comboParamsp = خالي_اشارو;  // Combo function parameter list
    Ast_متن_بلاڪ* m_clkSensp = خالي_اشارو;  // Clock sensitivity list
    Ast_متن_بلاڪ* m_comboIgnoreParamsp = خالي_اشارو;  // Combo ignore parameter list
    Ast_متن_بلاڪ* m_seqParamsp = خالي_اشارو;  // Sequential parameter list
    Ast_متن_بلاڪ* m_nbAssignsp = خالي_اشارو;  // Non-blocking assignment list
    Ast_متن_بلاڪ* m_seqAssignsp = خالي_اشارو;  // Sequential assignment list
    Ast_متن_بلاڪ* m_comboAssignsp = خالي_اشارو;  // Combo assignment list
    // C text blocks
    Ast_متن_بلاڪ* m_cHashValuep = خالي_اشارو;  // CPP hash value
    Ast_متن_بلاڪ* m_cComboParamsp = خالي_اشارو;  // Combo function parameter list
    Ast_متن_بلاڪ* m_cComboInsp = خالي_اشارو;  // Combo input copy list
    Ast_متن_بلاڪ* m_cComboOutsp = خالي_اشارو;  // Combo output copy list
    Ast_متن_بلاڪ* m_cSeqParamsp = خالي_اشارو;  // Sequential parameter list
    Ast_متن_بلاڪ* m_cSeqClksp = خالي_اشارو;  // Sequential clock copy list
    Ast_متن_بلاڪ* m_cSeqOutsp = خالي_اشارو;  // Sequential output copy list
    Ast_متن_بلاڪ* m_cIgnoreParamsp = خالي_اشارو;  // Combo ignore parameter list
    ثابت string m_libName;
    ثابت string m_چوٽي_Name;
    منطقي m_مليو_چوٽي = خطأ;  // Have seen the top module
    منطقي m_hasClk = خطأ;  // True if the top module has sequential logic
    std::map<صحيح, InoutPorts> m_inouts;  // Lowered ports grouped by original inout

    // VISITORS
    فراغ گهمو(AstNetlist* nodep) override {
        m_vfilep
            = جديد AstVFile{nodep->fileline(), v3Global.opt.makeDir() + "/" + m_libName + ".sv"};
        nodep->جوڙيو_Filesp(m_vfilep);
        m_cfilep
            = جديد AstCFile{nodep->fileline(), v3Global.opt.makeDir() + "/" + m_libName + ".cpp"};
        nodep->جوڙيو_Filesp(m_cfilep);
        iterateChildren(nodep);
    }

    فراغ گهمو(Ast_ڳنڍ_Module* nodep) override {
        إذا (!nodep->is_چوٽي()) {
            أرجع;
        } وإلا {
            UASSERT_OBJ(!m_مليو_چوٽي, nodep, "Multiple root modules");
        }
        File_سٽ* ثابت fl = nodep->fileline();
        // Need to know the existence of clk before createSvFile()
        m_hasClk = checkIfClockExists(nodep);
        createSvFile(fl, nodep);
        createCppFile(fl);

        iterateChildren(nodep);

        restoreInouts();

        // cppcheck-suppress unreadVariable
        ثابت V3Hash hash = V3Hasher::uncachedHash(m_cfilep);
        m_hashValuep->جوڙيو(cvt_منزل_Str(hash.قدر()) + ";\n");
        m_cHashValuep->جوڙيو(cvt_منزل_Str(hash.قدر()) + "U;\n");
        m_مليو_چوٽي = سچ;
    }

    فراغ جوڙيو_Comment(Ast_متن_بلاڪ* txtp, File_سٽ* fl, ثابت string& comment) {
        txtp->جوڙيو(جديد AstComment{fl, comment});
    }

    فراغ configSection(Ast_ڳنڍ_Module* /*modp*/, Ast_متن_بلاڪ* txtp, File_سٽ* /*fl*/) {
        txtp->جوڙيو("\n`ifdef VERILATOR\n");
        txtp->جوڙيو("`verilator_config\n");

        txtp->جوڙيو("verilator_lib -module \"" + m_libName + "\"\n");

        txtp->جوڙيو("profile_data -hier-dpi \"" + m_libName + "_protectlib_combo_update\" -cost 64'd"
                  + std::منزل_string(v3Global.current_Hier_بلاڪ_لاڳت()) + "\n");
        txtp->جوڙيو("profile_data -hier-dpi \"" + m_libName + "_protectlib_seq_update\" -cost 64'd"
                  + std::منزل_string(v3Global.current_Hier_بلاڪ_لاڳت()) + "\n");

        // Mark remaining NBA protectlib wrapper DPIs as non-hazardous by deliberately forwarding
        // them with non-zero cost.
        // Also, specify hierarchical workers for those tasks for scheduling.
        txtp->جوڙيو("profile_data -hier-dpi \"" + m_libName
                  + "_protectlib_combo_ignore\" -cost 64'd1\n");

        txtp->جوڙيو("hier_workers -hier-dpi \"" + m_libName
                  + "_protectlib_combo_update\" -workers 16'd"
                  + std::منزل_string(V3Control::getHierWorkers(m_libName)) + "\n");
        txtp->جوڙيو("hier_workers -hier-dpi \"" + m_libName
                  + "_protectlib_seq_update\" -workers 16'd"
                  + std::منزل_string(V3Control::getHierWorkers(m_libName)) + "\n");
        // No workers for combo_ignore
        txtp->جوڙيو("`verilog\n");
        txtp->جوڙيو("`endif\n");
    }

    فراغ hashComment(Ast_متن_بلاڪ* txtp, File_سٽ* fl) {
        جوڙيو_Comment(txtp, fl, "Checks to make sure the .sv wrapper and library agree");
    }

    فراغ initialComment(Ast_متن_بلاڪ* txtp, File_سٽ* fl) {
        جوڙيو_Comment(txtp, fl, "Creates an instance of the library module at initial-time");
        جوڙيو_Comment(txtp, fl, "(one for each instance in the user's design)");
    }

    فراغ comboComment(Ast_متن_بلاڪ* txtp, File_سٽ* fl) {
        جوڙيو_Comment(txtp, fl, "Updates all non-clock inputs and retrieves the results");
    }

    فراغ seqComment(Ast_متن_بلاڪ* txtp, File_سٽ* fl) {
        جوڙيو_Comment(txtp, fl, "Updates all clocks and retrieves the results");
    }

    فراغ comboIgnoreComment(Ast_متن_بلاڪ* txtp, File_سٽ* fl) {
        جوڙيو_Comment(txtp, fl, "Need to convince some simulators that the input to the module");
        جوڙيو_Comment(txtp, fl, "must be evaluated before evaluating the clock edge");
    }

    فراغ finalComment(Ast_متن_بلاڪ* txtp, File_سٽ* fl) {
        جوڙيو_Comment(txtp, fl, "Evaluates the library module's final process");
    }

    فراغ createSvFile(File_سٽ* fl, Ast_ڳنڍ_Module* modp) {
        // Comments
        Ast_متن_بلاڪ* ثابت txtp = جديد Ast_متن_بلاڪ{fl};
        جوڙيو_Comment(txtp, fl, "Wrapper module for DPI protected library");
        جوڙيو_Comment(txtp, fl,
                   "This module requires lib" + m_libName + ".a or lib" + m_libName
                       + ".so to work");
        جوڙيو_Comment(txtp, fl,
                   "See instructions in your simulator for how"
                   " to use DPI libraries\n");

        // Module declaration
        // Wrapper ranges preserve the original RTL, including intentional ascending ranges.
        txtp->جوڙيو("/*verilator lint_save*/\n");
        txtp->جوڙيو("/*verilator lint_off ASCRANGE*/\n");
        m_modPortsp = جديد Ast_متن_بلاڪ{fl, "module " + m_libName + " (\n", ", ", ");\n\n"};
        txtp->جوڙيو(m_modPortsp);

        // Timescale
        إذا (v3Global.opt.hier_ٻار() && v3Global.rootp()->timescaleSpecified()) {
            // Emit timescale for hierarchical Verilation if input HDL specifies timespec
            txtp->جوڙيو("timeunit "s + modp->timeunit().ascii() + ";\n");
            txtp->جوڙيو("timeprecision "s + +v3Global.rootp()->timeprecision().ascii() + ";\n");
        } وإلا {
            جوڙيو_Comment(txtp, fl,
                       "Precision of submodule"
                       " (commented out to avoid requiring timescale on all modules)");
            جوڙيو_Comment(txtp, fl, "timeunit "s + v3Global.rootp()->timeunit().ascii() + ";");
            جوڙيو_Comment(txtp, fl,
                       "timeprecision "s + v3Global.rootp()->timeprecision().ascii() + ";\n");
        }

        // DPI declarations
        hashComment(txtp, fl);
        txtp->جوڙيو("import \"DPI-C\" function void " + m_libName
                  + "_protectlib_check_hash(int protectlib_hash__V);\n\n");
        initialComment(txtp, fl);
        txtp->جوڙيو("import \"DPI-C\" function chandle " + m_libName
                  + "_protectlib_create(string scope__V);\n\n");
        comboComment(txtp, fl);
        m_comboPortsp = جديد Ast_متن_بلاڪ{fl,  //
                                         "import \"DPI-C\" function longint " + m_libName
                                             + "_protectlib_combo_update(\n",  //
                                         ", ",  //
                                         ");\n\n"};
        m_comboPortsp->جوڙيو("chandle handle__V\n");
        txtp->جوڙيو(m_comboPortsp);
        seqComment(txtp, fl);
        إذا (m_hasClk) {
            m_seqPortsp = جديد Ast_متن_بلاڪ{fl,  //
                                           "import \"DPI-C\" function longint " + m_libName
                                               + "_protectlib_seq_update(\n",  //
                                           ", ",  //
                                           ");\n\n"};
            m_seqPortsp->جوڙيو("chandle handle__V\n");
            txtp->جوڙيو(m_seqPortsp);
        }
        comboIgnoreComment(txtp, fl);
        m_comboIgnorePortsp = جديد Ast_متن_بلاڪ{fl,  //
                                               "import \"DPI-C\" function void " + m_libName
                                                   + "_protectlib_combo_ignore(\n",  //
                                               ", ",  //
                                               ");\n\n"};
        m_comboIgnorePortsp->جوڙيو("chandle handle__V\n");
        txtp->جوڙيو(m_comboIgnorePortsp);

        finalComment(txtp, fl);
        txtp->جوڙيو("import \"DPI-C\" function void " + m_libName
                  + "_protectlib_final(chandle handle__V);\n\n");

        // Local variables
        // Avoid tracing handle, as it is not a stable value, so breaks wavediff
        // Likewise other internals aren't interesting to the user
        txtp->جوڙيو("// verilator tracing_off\n");

        txtp->جوڙيو("chandle handle__V;\n");
        txtp->جوڙيو("time last_combo_seqnum__V;\n");
        إذا (m_hasClk) txtp->جوڙيو("time last_seq_seqnum__V;\n");
        txtp->جوڙيو("\n");

        m_comboDeclsp = جديد Ast_متن_بلاڪ{fl};
        txtp->جوڙيو(m_comboDeclsp);
        m_seqDeclsp = جديد Ast_متن_بلاڪ{fl};
        txtp->جوڙيو(m_seqDeclsp);
        m_tmpDeclsp = جديد Ast_متن_بلاڪ{fl};
        txtp->جوڙيو(m_tmpDeclsp);
        m_inoutDeclsp = جديد Ast_متن_بلاڪ{fl};
        txtp->جوڙيو(m_inoutDeclsp);

        // CPP hash value
        جوڙيو_Comment(txtp, fl, "Hash value to make sure this file and the corresponding");
        جوڙيو_Comment(txtp, fl, "library agree");
        m_hashValuep = جديد Ast_متن_بلاڪ{
            fl, "localparam int protectlib_hash__V /*verilator public_flat_rd*/ = 32'd", "", "\n"};
        txtp->جوڙيو(m_hashValuep);

        // Initial
        txtp->جوڙيو("initial begin\n");
        txtp->جوڙيو(m_libName + "_protectlib_check_hash(protectlib_hash__V);\n");
        txtp->جوڙيو("if (handle__V == null) handle__V = " + m_libName
                  + "_protectlib_create"
                    "($sformatf(\"%m\"));\n");
        txtp->جوڙيو("end\n\n");

        // Combinatorial process
        جوڙيو_Comment(txtp, fl, "Combinatorially evaluate changes to inputs");
        txtp->جوڙيو("always_comb begin\n");
        m_comboParamsp = جديد Ast_متن_بلاڪ{fl,  //
                                          "last_combo_seqnum__V = " + m_libName
                                              + "_protectlib_combo_update(\n",  //
                                          ",\n",  //
                                          "\n);\n"};
        m_comboParamsp->جوڙيو("handle__V");
        txtp->جوڙيو(m_comboParamsp);
        txtp->جوڙيو("end\n\n");

        // Sequential process
        إذا (m_hasClk) {
            جوڙيو_Comment(txtp, fl, "Evaluate clock edges");
            m_clkSensp = جديد Ast_متن_بلاڪ{fl, "always @(", " or ", ")"};
            txtp->جوڙيو(m_clkSensp);
            txtp->جوڙيو(" begin\n");
            m_comboIgnoreParamsp
                = جديد Ast_متن_بلاڪ{fl,  //
                                   m_libName + "_protectlib_combo_ignore(\n", ",\n", "\n);\n"};
            m_comboIgnoreParamsp->جوڙيو("handle__V");
            txtp->جوڙيو(m_comboIgnoreParamsp);
            m_seqParamsp = جديد Ast_متن_بلاڪ{fl,  //
                                            "last_seq_seqnum__V <= " + m_libName
                                                + "_protectlib_seq_update(\n",  //
                                            ",\n",  //
                                            "\n);\n"};
            m_seqParamsp->جوڙيو("handle__V");
            txtp->جوڙيو(m_seqParamsp);
            m_nbAssignsp = جديد Ast_متن_بلاڪ{fl};
            txtp->جوڙيو(m_nbAssignsp);
            txtp->جوڙيو("end\n\n");
        }

        // Select between combinatorial and sequential results
        جوڙيو_Comment(txtp, fl, "Select between combinatorial and sequential results");
        txtp->جوڙيو("always_comb begin\n");
        إذا (m_hasClk) {
            txtp->جوڙيو("if (last_seq_seqnum__V > last_combo_seqnum__V) begin\n");
            m_seqAssignsp = جديد Ast_متن_بلاڪ{fl};
            txtp->جوڙيو(m_seqAssignsp);
            txtp->جوڙيو("end else begin\n");
            m_comboAssignsp = جديد Ast_متن_بلاڪ{fl};
            txtp->جوڙيو(m_comboAssignsp);
            txtp->جوڙيو("end\n");
        } وإلا {
            m_comboAssignsp = جديد Ast_متن_بلاڪ{fl};
            txtp->جوڙيو(m_comboAssignsp);
        }
        txtp->جوڙيو("end\n\n");

        // Final
        txtp->جوڙيو("final " + m_libName + "_protectlib_final(handle__V);\n\n");

        txtp->جوڙيو("endmodule\n");
        txtp->جوڙيو("/*verilator lint_restore*/\n");

        configSection(modp, txtp, fl);

        m_vfilep->tblockp(txtp);
    }

    فراغ castPtr(File_سٽ* /*fl*/, Ast_متن_بلاڪ* txtp) {
        txtp->جوڙيو(m_چوٽي_Name
                  + "_container* const handlep__V = "  // LCOV_EXCL_LINE  // lcov bug
                    "static_cast<"
                  + m_چوٽي_Name + "_container*>(vhandlep__V);\n");
    }

    فراغ createCppFile(File_سٽ* fl) {
        // Comments
        Ast_متن_بلاڪ* ثابت txtp = جديد Ast_متن_بلاڪ{fl};
        جوڙيو_Comment(txtp, fl, "Wrapper functions for DPI protected library\n");

        // Includes
        txtp->جوڙيو("#include \"" + m_چوٽي_Name + ".h\"\n");
        txtp->جوڙيو("#include \"verilated_dpi.h\"\n\n");
        txtp->جوڙيو("#include <cstdio>\n");
        txtp->جوڙيو("#include <cstdlib>\n\n");

        // Verilated module plus sequence number
        جوڙيو_Comment(txtp, fl, "Container class to house verilated object and sequence number");
        txtp->جوڙيو("class " + m_چوٽي_Name + "_container: public " + m_چوٽي_Name + " {\n");
        txtp->جوڙيو("public:\n");
        txtp->جوڙيو("long long m_seqnum;\n");
        txtp->جوڙيو(m_چوٽي_Name + "_container(const char* scopep__V):\n");
        txtp->جوڙيو(m_چوٽي_Name + "(scopep__V) {}\n");
        txtp->جوڙيو("};\n\n");

        // Extern C
        txtp->جوڙيو("extern \"C\" {\n\n");

        // Hash check
        hashComment(txtp, fl);
        txtp->جوڙيو("void " + m_libName
                  + "_protectlib_check_hash"
                    "(int protectlib_hash__V) {\n");
        m_cHashValuep = جديد Ast_متن_بلاڪ{fl, "const int expected_hash__V = ", "", ""};
        txtp->جوڙيو(m_cHashValuep);
        txtp->جوڙيو(/**/ "if (protectlib_hash__V != expected_hash__V) {\n");
        txtp->جوڙيو(/****/ "fprintf(stderr, \"%%Error: cannot use " + m_libName
                  + " library, "
                    "Verliog (%u) and library (%u) hash values do not "
                    "agree\\n\", protectlib_hash__V, expected_hash__V);\n");
        txtp->جوڙيو(/****/ "std::exit(EXIT_FAILURE);\n");
        txtp->جوڙيو(/**/ "}\n");
        txtp->جوڙيو("}\n\n");

        // Initial
        initialComment(txtp, fl);
        txtp->جوڙيو("void* " + m_libName + "_protectlib_create(const char* scopep__V) {\n");
        txtp->جوڙيو(/**/ m_چوٽي_Name + "_container* const handlep__V = new " + m_چوٽي_Name
                  + "_container{scopep__V};\n");
        txtp->جوڙيو(/**/ "return handlep__V;\n");
        txtp->جوڙيو("}\n\n");

        // Updates
        comboComment(txtp, fl);
        m_cComboParamsp = جديد Ast_متن_بلاڪ{
            fl, "long long " + m_libName + "_protectlib_combo_update(\n", ",\n", "\n) {\n"};
        m_cComboParamsp->جوڙيو("void* vhandlep__V");
        txtp->جوڙيو(m_cComboParamsp);
        m_cComboInsp = جديد Ast_متن_بلاڪ{fl};
        castPtr(fl, m_cComboInsp);
        txtp->جوڙيو(m_cComboInsp);
        txtp->جوڙيو("handlep__V->eval();\n");
        m_cComboOutsp = جديد Ast_متن_بلاڪ{fl};
        txtp->جوڙيو(m_cComboOutsp);
        txtp->جوڙيو("return handlep__V->m_seqnum++;\n");
        txtp->جوڙيو("}\n\n");

        إذا (m_hasClk) {
            seqComment(txtp, fl);
            m_cSeqParamsp = جديد Ast_متن_بلاڪ{
                fl, "long long " + m_libName + "_protectlib_seq_update(\n", ",\n", "\n) {\n"};
            m_cSeqParamsp->جوڙيو("void* vhandlep__V");
            txtp->جوڙيو(m_cSeqParamsp);
            m_cSeqClksp = جديد Ast_متن_بلاڪ{fl};
            castPtr(fl, m_cSeqClksp);
            txtp->جوڙيو(m_cSeqClksp);
            txtp->جوڙيو("handlep__V->eval();\n");
            m_cSeqOutsp = جديد Ast_متن_بلاڪ{fl};
            txtp->جوڙيو(m_cSeqOutsp);
            txtp->جوڙيو("return handlep__V->m_seqnum++;\n");
            txtp->جوڙيو("}\n\n");
        }

        comboIgnoreComment(txtp, fl);
        m_cIgnoreParamsp = جديد Ast_متن_بلاڪ{
            fl, "void " + m_libName + "_protectlib_combo_ignore(\n", ",\n", "\n)\n{ }\n\n"};
        m_cIgnoreParamsp->جوڙيو("void* vhandlep__V");
        txtp->جوڙيو(m_cIgnoreParamsp);

        // Final
        finalComment(txtp, fl);
        txtp->جوڙيو("void " + m_libName + "_protectlib_final(void* vhandlep__V) {\n");
        castPtr(fl, txtp);
        txtp->جوڙيو(/**/ "handlep__V->final();\n");
        txtp->جوڙيو(/**/ "delete handlep__V;\n");
        txtp->جوڙيو("}\n\n");

        txtp->جوڙيو("}\n");
        m_cfilep->tblockp(txtp);
    }

    فراغ گهمو(AstVar* nodep) override {
        إذا (!nodep->isIO()) أرجع;
        إذا (restoreInout(nodep)) {
            InoutPorts& ports = m_inouts[nodep->libInoutId()];
            إذا (nodep->is_داخلا()) {
                ports.inp = nodep;
            } وإلا إذا (nodep->libInoutEnable()) {
                ports.enp = nodep;
            } وإلا {
                ports.outp = nodep;
            }
        }
        إذا (nodep->direction() == VDirection::داخلا) {
            إذا (nodep->isPrimaryClock()) {
                UASSERT_OBJ(m_hasClk, nodep, "checkIfClockExists() didn't find this clock");
                handleClock(nodep);
            } وإلا {
                handle_معلومات_داخلا(nodep);
            }
        } وإلا {
            // Ref ports were rejected during linking, and tristate lowering split all inouts.
            UASSERT_OBJ(nodep->direction() == VDirection::OUTPUT, nodep,
                        "Unexpected library port direction");
            handleOutput(nodep);
        }
    }

    فراغ گهمو(Ast_ڳنڍ*) override {}

    string c_داخلا_Connection(AstVar* varp) {
        أرجع V3Task::سونپيو_Dpi_منزل_اندروني("handlep__V->" + varp->name(), varp);
    }

    // The wrapper's module ports are connected by name, so they must use the user's
    // names rather than the hashed form of long names used internally
    ساكن string sv_پورٽ_Name(ثابت AstVar* varp) {
        أرجع Ast_ڳنڍ::prettyName(VName::dehash(varp->name()));
    }

    ساكن AstVar* new_Sv_پورٽ_Var(AstVar* varp) {
        AstVar* ثابت newVarp = varp->clone_وڻ(خطأ);
        newVarp->name(VName::dehash(varp->name()));
        أرجع newVarp;
    }

    فراغ handleClock(AstVar* varp) {
        handle_داخلا(varp);
        m_seqPortsp->جوڙيو(varp->clone_وڻ(خطأ));
        إذا (m_hasClk) {
            ثابت std::string pname = sv_پورٽ_Name(varp);
            m_seqParamsp->جوڙيو(pname);
            m_clkSensp->جوڙيو(pname);
        }
        m_cSeqParamsp->جوڙيو(varp->dpiArgType(سچ, خطأ));
        m_cSeqClksp->جوڙيو(c_داخلا_Connection(varp));
    }

    فراغ handle_معلومات_داخلا(AstVar* varp) {
        handle_داخلا(varp);
        m_comboPortsp->جوڙيو(varp->clone_وڻ(خطأ));
        m_comboParamsp->جوڙيو(sv_پورٽ_Name(varp));
        m_comboIgnorePortsp->جوڙيو(varp->clone_وڻ(خطأ));
        إذا (m_hasClk) m_comboIgnoreParamsp->جوڙيو(sv_پورٽ_Name(varp));
        m_cComboParamsp->جوڙيو(varp->dpiArgType(سچ, خطأ));
        m_cComboInsp->جوڙيو(c_داخلا_Connection(varp));
        m_cIgnoreParamsp->جوڙيو(varp->dpiArgType(سچ, خطأ));
    }

    ساكن منطقي restoreInout(ثابت AstVar* varp) {
        أرجع varp->libInoutId() && !v3Global.opt.pinsInoutEnables();
    }

    فراغ handle_داخلا(AstVar* varp) {
        AstVar* ثابت portp = new_Sv_پورٽ_Var(varp);
        إذا (restoreInout(varp)) {
            portp->direction(VDirection::INOUT);
            portp->varType(VVarType::WIRE);
        }
        m_modPortsp->جوڙيو(portp);
    }

    فراغ restoreInouts() {
        لكل (ثابت خودڪار& داخلا_2 : m_inouts) {
            ثابت InoutPorts& ports = داخلا_2.ٻيو;
            UASSERT_OBJ(ports.inp && ports.outp && ports.enp, m_vfilep,
                        "Incomplete lowered library inout");
            ثابت string اندريون = sv_پورٽ_Name(ports.inp);
            ثابت string ٻاهريون = ports.outp->prettyName();
            ثابت string en = ports.enp->prettyName();
            std::ويڪٽر<std::pair<صحيح, صحيح>> dimensions;
            ثابت Ast_ڳنڍ_DType* dtypep = ports.inp->dtypep()->skipRefp();
            طالما (ثابت Ast_ڳنڍ_Array_DType* ثابت arrayp = VN_CAST(dtypep, ڳنڍ_Array_DType)) {
                // V3Slice currently rejects lowered unpacked inouts before this pass.
                // TODO: Support unpacked inouts once V3Slice supports their lowered form.
                UASSERT_OBJ(!VN_IS(arrayp, UnpackArrayDType), ports.inp,
                            "Unpacked library inout should have been rejected by V3Slice");
                dimensions.emplace_پٺ(arrayp->lo(), arrayp->hi());
                dtypep = arrayp->subDTypep()->skipRefp();
            }
            ثابت AstBasicDType* ثابت basicp = dtypep->basicp();
            UASSERT_OBJ(basicp, ports.inp, "Library inout must have a packed integral type");
            إذا (basicp->isRanged()) dimensions.emplace_پٺ(basicp->lo(), basicp->hi());
            إذا (dimensions.خالي_جايون()) {
                m_inoutDeclsp->جوڙيو("assign " + اندريون + " = " + en + " ? " + ٻاهريون + " : 1'bz;\n");
            } وإلا {
                string lhs = اندريون;
                string rhsEn = en;
                string rhs_ٻاهريون = ٻاهريون;
                لكل (سائيز_t dim = 0; dim < dimensions.سائيز(); ++dim) {
                    ثابت string اشاريو
                        = "__VlibInout" + cvt_منزل_Str(داخلا_2.پهريون) + "_" + cvt_منزل_Str(dim);
                    ثابت صحيح هيٺيون = dimensions[dim].پهريون;
                    ثابت صحيح مٿاهون = dimensions[dim].ٻيو;
                    ثابت صحيح سائيز = مٿاهون - هيٺيون + 1;
                    m_inoutDeclsp->جوڙيو("for (genvar " + اشاريو + " = 0; " + اشاريو + " < "
                                       + cvt_منزل_Str(سائيز) + "; ++" + اشاريو + ") begin\n");
                    ثابت string select = "[" + cvt_منزل_Str(هيٺيون) + " + " + اشاريو + "]";
                    lhs += select;
                    rhsEn += select;
                    rhs_ٻاهريون += select;
                }
                m_inoutDeclsp->جوڙيو("assign " + lhs + " = " + rhsEn + " ? " + rhs_ٻاهريون
                                   + " : 1'bz;\n");
                لكل (سائيز_t dim = 0; dim < dimensions.سائيز(); ++dim) m_inoutDeclsp->جوڙيو("end\n");
            }
        }
    }

    ساكن فراغ جوڙيو_Local_Variable(Ast_متن_بلاڪ* textp, ثابت AstVar* varp, ثابت حرف* suffix) {
        AstVar* ثابت newVarp
            = جديد AstVar{varp->fileline(), VVarType::VAR, varp->name() + suffix, varp->dtypep()};
        textp->جوڙيو(newVarp);
    }

    فراغ handleOutput(AstVar* ثابت varp) {
        ثابت std::string pname = varp->prettyName();
        // Restored inout drivers are local signals and retain their internal names.
        ثابت std::string پورٽ_Name = restoreInout(varp) ? pname : sv_پورٽ_Name(varp);
        إذا (restoreInout(varp)) {
            AstVar* ثابت localp = varp->clone_وڻ(خطأ);
            localp->direction(VDirection::NONE);
            m_inoutDeclsp->جوڙيو(localp);
        } وإلا {
            m_modPortsp->جوڙيو(new_Sv_پورٽ_Var(varp));
        }
        m_comboPortsp->جوڙيو(varp->clone_وڻ(خطأ));
        m_comboParamsp->جوڙيو(pname + "_combo__V");
        إذا (m_hasClk) {
            m_seqPortsp->جوڙيو(varp->clone_وڻ(خطأ));
            m_seqParamsp->جوڙيو(pname + "_tmp__V");
        }

        جوڙيو_Local_Variable(m_comboDeclsp, varp, "_combo__V");

        إذا (m_hasClk) {
            جوڙيو_Local_Variable(m_seqDeclsp, varp, "_seq__V");
            جوڙيو_Local_Variable(m_tmpDeclsp, varp, "_tmp__V");

            m_nbAssignsp->جوڙيو(pname + "_seq__V <= " + pname + "_tmp__V;\n");
            m_seqAssignsp->جوڙيو(پورٽ_Name + " = " + pname + "_seq__V;\n");
        }
        m_comboAssignsp->جوڙيو(پورٽ_Name + " = " + pname + "_combo__V;\n");
        m_cComboParamsp->جوڙيو(varp->dpiArgType(سچ, خطأ));
        m_cComboOutsp->جوڙيو(V3Task::سونپيو_اندروني_منزل_Dpi(varp, سچ, "", "", "handlep__V->") + "\n");
        إذا (m_hasClk) {
            m_cSeqParamsp->جوڙيو(varp->dpiArgType(سچ, خطأ));
            m_cSeqOutsp->جوڙيو(V3Task::سونپيو_اندروني_منزل_Dpi(varp, سچ, "", "", "handlep__V->")
                             + "\n");
        }
    }

    ساكن منطقي checkIfClockExists(ثابت Ast_ڳنڍ_Module* modp) {
        لكل (ثابت Ast_ڳنڍ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (ثابت AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                إذا (varp->direction() == VDirection::داخلا && varp->isPrimaryClock()) {
                    أرجع سچ;
                }
            }
        }
        أرجع خطأ;
    }

عام:
    واضح ProtectVisitor(Ast_ڳنڍ* nodep)
        : m_libName{v3Global.opt.libCreate()}
        , m_چوٽي_Name{v3Global.opt.اڳياڙي()} {
        iterate(nodep);
    }
};

//######################################################################
// ProtectLib class functions

فراغ V3ProtectLib::protect() {
    UINFO(2, __FUNCTION__ << ":");
    ProtectVisitor{v3Global.rootp()};
}
