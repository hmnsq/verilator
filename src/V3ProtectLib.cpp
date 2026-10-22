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
        AstVar* inp = خالی_سوچک;  // Resolved input value and original public port
        AstVar* outp = خالی_سوچک;  // Library drive value
        AstVar* enp = خالی_سوچک;  // Per-bit drive enable
    };

    // MEMBERS
    AstVFile* m_vfilep = خالی_سوچک;  // DPI-enabled Verilog wrapper
    AstCFile* m_cfilep = خالی_سوچک;  // C implementation of DPI functions
    // Verilog text blocks
    Ast_متن_بلاک* m_modPortsp = خالی_سوچک;  // Module port list
    Ast_متن_بلاک* m_comboPortsp = خالی_سوچک;  // Combo function port list
    Ast_متن_بلاک* m_seqPortsp = خالی_سوچک;  // Sequential function port list
    Ast_متن_بلاک* m_comboIgnorePortsp = خالی_سوچک;  // Combo ignore function port list
    Ast_متن_بلاک* m_comboDeclsp = خالی_سوچک;  // Combo signal declaration list
    Ast_متن_بلاک* m_seqDeclsp = خالی_سوچک;  // Sequential signal declaration list
    Ast_متن_بلاک* m_tmpDeclsp = خالی_سوچک;  // Temporary signal declaration list
    Ast_متن_بلاک* m_inoutDeclsp = خالی_سوچک;  // Lowered inout signals and tristate drivers
    Ast_متن_بلاک* m_hashValuep = خالی_سوچک;  // CPP hash value
    Ast_متن_بلاک* m_comboParamsp = خالی_سوچک;  // Combo function parameter list
    Ast_متن_بلاک* m_clkSensp = خالی_سوچک;  // Clock sensitivity list
    Ast_متن_بلاک* m_comboIgnoreParamsp = خالی_سوچک;  // Combo ignore parameter list
    Ast_متن_بلاک* m_seqParamsp = خالی_سوچک;  // Sequential parameter list
    Ast_متن_بلاک* m_nbAssignsp = خالی_سوچک;  // Non-blocking assignment list
    Ast_متن_بلاک* m_seqAssignsp = خالی_سوچک;  // Sequential assignment list
    Ast_متن_بلاک* m_comboAssignsp = خالی_سوچک;  // Combo assignment list
    // C text blocks
    Ast_متن_بلاک* m_cHashValuep = خالی_سوچک;  // CPP hash value
    Ast_متن_بلاک* m_cComboParamsp = خالی_سوچک;  // Combo function parameter list
    Ast_متن_بلاک* m_cComboInsp = خالی_سوچک;  // Combo input copy list
    Ast_متن_بلاک* m_cComboOutsp = خالی_سوچک;  // Combo output copy list
    Ast_متن_بلاک* m_cSeqParamsp = خالی_سوچک;  // Sequential parameter list
    Ast_متن_بلاک* m_cSeqClksp = خالی_سوچک;  // Sequential clock copy list
    Ast_متن_بلاک* m_cSeqOutsp = خالی_سوچک;  // Sequential output copy list
    Ast_متن_بلاک* m_cIgnoreParamsp = خالی_سوچک;  // Combo ignore parameter list
    ثابت string m_libName;
    ثابت string m_چوٹی_Name;
    منطقي m_لبھیا_چوٹی = خطأ;  // Have seen the top module
    منطقي m_hasClk = خطأ;  // True if the top module has sequential logic
    std::map<صحيح, InoutPorts> m_inouts;  // Lowered ports grouped by original inout

    // VISITORS
    فراغ ویکھو(AstNetlist* nodep) override {
        m_vfilep
            = جديد AstVFile{nodep->fileline(), v3Global.opt.makeDir() + "/" + m_libName + ".sv"};
        nodep->جوڑو_Filesp(m_vfilep);
        m_cfilep
            = جديد AstCFile{nodep->fileline(), v3Global.opt.makeDir() + "/" + m_libName + ".cpp"};
        nodep->جوڑو_Filesp(m_cfilep);
        iterateChildren(nodep);
    }

    فراغ ویکھو(Ast_گرہ_Module* nodep) override {
        إذا (!nodep->is_چوٹی()) {
            أرجع;
        } وإلا {
            UASSERT_OBJ(!m_لبھیا_چوٹی, nodep, "Multiple root modules");
        }
        File_سطر* ثابت fl = nodep->fileline();
        // Need to know the existence of clk before createSvFile()
        m_hasClk = checkIfClockExists(nodep);
        createSvFile(fl, nodep);
        createCppFile(fl);

        iterateChildren(nodep);

        restoreInouts();

        // cppcheck-suppress unreadVariable
        ثابت V3Hash hash = V3Hasher::uncachedHash(m_cfilep);
        m_hashValuep->جوڑو(cvt_منزل_Str(hash.قدر()) + ";\n");
        m_cHashValuep->جوڑو(cvt_منزل_Str(hash.قدر()) + "U;\n");
        m_لبھیا_چوٹی = سچ;
    }

    فراغ جوڑو_Comment(Ast_متن_بلاک* txtp, File_سطر* fl, ثابت string& comment) {
        txtp->جوڑو(جديد AstComment{fl, comment});
    }

    فراغ configSection(Ast_گرہ_Module* /*modp*/, Ast_متن_بلاک* txtp, File_سطر* /*fl*/) {
        txtp->جوڑو("\n`ifdef VERILATOR\n");
        txtp->جوڑو("`verilator_config\n");

        txtp->جوڑو("verilator_lib -module \"" + m_libName + "\"\n");

        txtp->جوڑو("profile_data -hier-dpi \"" + m_libName + "_protectlib_combo_update\" -cost 64'd"
                  + std::منزل_string(v3Global.current_Hier_بلاک_لاگت()) + "\n");
        txtp->جوڑو("profile_data -hier-dpi \"" + m_libName + "_protectlib_seq_update\" -cost 64'd"
                  + std::منزل_string(v3Global.current_Hier_بلاک_لاگت()) + "\n");

        // Mark remaining NBA protectlib wrapper DPIs as non-hazardous by deliberately forwarding
        // them with non-zero cost.
        // Also, specify hierarchical workers for those tasks for scheduling.
        txtp->جوڑو("profile_data -hier-dpi \"" + m_libName
                  + "_protectlib_combo_ignore\" -cost 64'd1\n");

        txtp->جوڑو("hier_workers -hier-dpi \"" + m_libName
                  + "_protectlib_combo_update\" -workers 16'd"
                  + std::منزل_string(V3Control::getHierWorkers(m_libName)) + "\n");
        txtp->جوڑو("hier_workers -hier-dpi \"" + m_libName
                  + "_protectlib_seq_update\" -workers 16'd"
                  + std::منزل_string(V3Control::getHierWorkers(m_libName)) + "\n");
        // No workers for combo_ignore
        txtp->جوڑو("`verilog\n");
        txtp->جوڑو("`endif\n");
    }

    فراغ hashComment(Ast_متن_بلاک* txtp, File_سطر* fl) {
        جوڑو_Comment(txtp, fl, "Checks to make sure the .sv wrapper and library agree");
    }

    فراغ initialComment(Ast_متن_بلاک* txtp, File_سطر* fl) {
        جوڑو_Comment(txtp, fl, "Creates an instance of the library module at initial-time");
        جوڑو_Comment(txtp, fl, "(one for each instance in the user's design)");
    }

    فراغ comboComment(Ast_متن_بلاک* txtp, File_سطر* fl) {
        جوڑو_Comment(txtp, fl, "Updates all non-clock inputs and retrieves the results");
    }

    فراغ seqComment(Ast_متن_بلاک* txtp, File_سطر* fl) {
        جوڑو_Comment(txtp, fl, "Updates all clocks and retrieves the results");
    }

    فراغ comboIgnoreComment(Ast_متن_بلاک* txtp, File_سطر* fl) {
        جوڑو_Comment(txtp, fl, "Need to convince some simulators that the input to the module");
        جوڑو_Comment(txtp, fl, "must be evaluated before evaluating the clock edge");
    }

    فراغ finalComment(Ast_متن_بلاک* txtp, File_سطر* fl) {
        جوڑو_Comment(txtp, fl, "Evaluates the library module's final process");
    }

    فراغ createSvFile(File_سطر* fl, Ast_گرہ_Module* modp) {
        // Comments
        Ast_متن_بلاک* ثابت txtp = جديد Ast_متن_بلاک{fl};
        جوڑو_Comment(txtp, fl, "Wrapper module for DPI protected library");
        جوڑو_Comment(txtp, fl,
                   "This module requires lib" + m_libName + ".a or lib" + m_libName
                       + ".so to work");
        جوڑو_Comment(txtp, fl,
                   "See instructions in your simulator for how"
                   " to use DPI libraries\n");

        // Module declaration
        // Wrapper ranges preserve the original RTL, including intentional ascending ranges.
        txtp->جوڑو("/*verilator lint_save*/\n");
        txtp->جوڑو("/*verilator lint_off ASCRANGE*/\n");
        m_modPortsp = جديد Ast_متن_بلاک{fl, "module " + m_libName + " (\n", ", ", ");\n\n"};
        txtp->جوڑو(m_modPortsp);

        // Timescale
        إذا (v3Global.opt.hier_بچہ() && v3Global.rootp()->timescaleSpecified()) {
            // Emit timescale for hierarchical Verilation if input HDL specifies timespec
            txtp->جوڑو("timeunit "s + modp->timeunit().ascii() + ";\n");
            txtp->جوڑو("timeprecision "s + +v3Global.rootp()->timeprecision().ascii() + ";\n");
        } وإلا {
            جوڑو_Comment(txtp, fl,
                       "Precision of submodule"
                       " (commented out to avoid requiring timescale on all modules)");
            جوڑو_Comment(txtp, fl, "timeunit "s + v3Global.rootp()->timeunit().ascii() + ";");
            جوڑو_Comment(txtp, fl,
                       "timeprecision "s + v3Global.rootp()->timeprecision().ascii() + ";\n");
        }

        // DPI declarations
        hashComment(txtp, fl);
        txtp->جوڑو("import \"DPI-C\" function void " + m_libName
                  + "_protectlib_check_hash(int protectlib_hash__V);\n\n");
        initialComment(txtp, fl);
        txtp->جوڑو("import \"DPI-C\" function chandle " + m_libName
                  + "_protectlib_create(string scope__V);\n\n");
        comboComment(txtp, fl);
        m_comboPortsp = جديد Ast_متن_بلاک{fl,  //
                                         "import \"DPI-C\" function longint " + m_libName
                                             + "_protectlib_combo_update(\n",  //
                                         ", ",  //
                                         ");\n\n"};
        m_comboPortsp->جوڑو("chandle handle__V\n");
        txtp->جوڑو(m_comboPortsp);
        seqComment(txtp, fl);
        إذا (m_hasClk) {
            m_seqPortsp = جديد Ast_متن_بلاک{fl,  //
                                           "import \"DPI-C\" function longint " + m_libName
                                               + "_protectlib_seq_update(\n",  //
                                           ", ",  //
                                           ");\n\n"};
            m_seqPortsp->جوڑو("chandle handle__V\n");
            txtp->جوڑو(m_seqPortsp);
        }
        comboIgnoreComment(txtp, fl);
        m_comboIgnorePortsp = جديد Ast_متن_بلاک{fl,  //
                                               "import \"DPI-C\" function void " + m_libName
                                                   + "_protectlib_combo_ignore(\n",  //
                                               ", ",  //
                                               ");\n\n"};
        m_comboIgnorePortsp->جوڑو("chandle handle__V\n");
        txtp->جوڑو(m_comboIgnorePortsp);

        finalComment(txtp, fl);
        txtp->جوڑو("import \"DPI-C\" function void " + m_libName
                  + "_protectlib_final(chandle handle__V);\n\n");

        // Local variables
        // Avoid tracing handle, as it is not a stable value, so breaks wavediff
        // Likewise other internals aren't interesting to the user
        txtp->جوڑو("// verilator tracing_off\n");

        txtp->جوڑو("chandle handle__V;\n");
        txtp->جوڑو("time last_combo_seqnum__V;\n");
        إذا (m_hasClk) txtp->جوڑو("time last_seq_seqnum__V;\n");
        txtp->جوڑو("\n");

        m_comboDeclsp = جديد Ast_متن_بلاک{fl};
        txtp->جوڑو(m_comboDeclsp);
        m_seqDeclsp = جديد Ast_متن_بلاک{fl};
        txtp->جوڑو(m_seqDeclsp);
        m_tmpDeclsp = جديد Ast_متن_بلاک{fl};
        txtp->جوڑو(m_tmpDeclsp);
        m_inoutDeclsp = جديد Ast_متن_بلاک{fl};
        txtp->جوڑو(m_inoutDeclsp);

        // CPP hash value
        جوڑو_Comment(txtp, fl, "Hash value to make sure this file and the corresponding");
        جوڑو_Comment(txtp, fl, "library agree");
        m_hashValuep = جديد Ast_متن_بلاک{
            fl, "localparam int protectlib_hash__V /*verilator public_flat_rd*/ = 32'd", "", "\n"};
        txtp->جوڑو(m_hashValuep);

        // Initial
        txtp->جوڑو("initial begin\n");
        txtp->جوڑو(m_libName + "_protectlib_check_hash(protectlib_hash__V);\n");
        txtp->جوڑو("if (handle__V == null) handle__V = " + m_libName
                  + "_protectlib_create"
                    "($sformatf(\"%m\"));\n");
        txtp->جوڑو("end\n\n");

        // Combinatorial process
        جوڑو_Comment(txtp, fl, "Combinatorially evaluate changes to inputs");
        txtp->جوڑو("always_comb begin\n");
        m_comboParamsp = جديد Ast_متن_بلاک{fl,  //
                                          "last_combo_seqnum__V = " + m_libName
                                              + "_protectlib_combo_update(\n",  //
                                          ",\n",  //
                                          "\n);\n"};
        m_comboParamsp->جوڑو("handle__V");
        txtp->جوڑو(m_comboParamsp);
        txtp->جوڑو("end\n\n");

        // Sequential process
        إذا (m_hasClk) {
            جوڑو_Comment(txtp, fl, "Evaluate clock edges");
            m_clkSensp = جديد Ast_متن_بلاک{fl, "always @(", " or ", ")"};
            txtp->جوڑو(m_clkSensp);
            txtp->جوڑو(" begin\n");
            m_comboIgnoreParamsp
                = جديد Ast_متن_بلاک{fl,  //
                                   m_libName + "_protectlib_combo_ignore(\n", ",\n", "\n);\n"};
            m_comboIgnoreParamsp->جوڑو("handle__V");
            txtp->جوڑو(m_comboIgnoreParamsp);
            m_seqParamsp = جديد Ast_متن_بلاک{fl,  //
                                            "last_seq_seqnum__V <= " + m_libName
                                                + "_protectlib_seq_update(\n",  //
                                            ",\n",  //
                                            "\n);\n"};
            m_seqParamsp->جوڑو("handle__V");
            txtp->جوڑو(m_seqParamsp);
            m_nbAssignsp = جديد Ast_متن_بلاک{fl};
            txtp->جوڑو(m_nbAssignsp);
            txtp->جوڑو("end\n\n");
        }

        // Select between combinatorial and sequential results
        جوڑو_Comment(txtp, fl, "Select between combinatorial and sequential results");
        txtp->جوڑو("always_comb begin\n");
        إذا (m_hasClk) {
            txtp->جوڑو("if (last_seq_seqnum__V > last_combo_seqnum__V) begin\n");
            m_seqAssignsp = جديد Ast_متن_بلاک{fl};
            txtp->جوڑو(m_seqAssignsp);
            txtp->جوڑو("end else begin\n");
            m_comboAssignsp = جديد Ast_متن_بلاک{fl};
            txtp->جوڑو(m_comboAssignsp);
            txtp->جوڑو("end\n");
        } وإلا {
            m_comboAssignsp = جديد Ast_متن_بلاک{fl};
            txtp->جوڑو(m_comboAssignsp);
        }
        txtp->جوڑو("end\n\n");

        // Final
        txtp->جوڑو("final " + m_libName + "_protectlib_final(handle__V);\n\n");

        txtp->جوڑو("endmodule\n");
        txtp->جوڑو("/*verilator lint_restore*/\n");

        configSection(modp, txtp, fl);

        m_vfilep->tblockp(txtp);
    }

    فراغ castPtr(File_سطر* /*fl*/, Ast_متن_بلاک* txtp) {
        txtp->جوڑو(m_چوٹی_Name
                  + "_container* const handlep__V = "  // LCOV_EXCL_LINE  // lcov bug
                    "static_cast<"
                  + m_چوٹی_Name + "_container*>(vhandlep__V);\n");
    }

    فراغ createCppFile(File_سطر* fl) {
        // Comments
        Ast_متن_بلاک* ثابت txtp = جديد Ast_متن_بلاک{fl};
        جوڑو_Comment(txtp, fl, "Wrapper functions for DPI protected library\n");

        // Includes
        txtp->جوڑو("#include \"" + m_چوٹی_Name + ".h\"\n");
        txtp->جوڑو("#include \"verilated_dpi.h\"\n\n");
        txtp->جوڑو("#include <cstdio>\n");
        txtp->جوڑو("#include <cstdlib>\n\n");

        // Verilated module plus sequence number
        جوڑو_Comment(txtp, fl, "Container class to house verilated object and sequence number");
        txtp->جوڑو("class " + m_چوٹی_Name + "_container: public " + m_چوٹی_Name + " {\n");
        txtp->جوڑو("public:\n");
        txtp->جوڑو("long long m_seqnum;\n");
        txtp->جوڑو(m_چوٹی_Name + "_container(const char* scopep__V):\n");
        txtp->جوڑو(m_چوٹی_Name + "(scopep__V) {}\n");
        txtp->جوڑو("};\n\n");

        // Extern C
        txtp->جوڑو("extern \"C\" {\n\n");

        // Hash check
        hashComment(txtp, fl);
        txtp->جوڑو("void " + m_libName
                  + "_protectlib_check_hash"
                    "(int protectlib_hash__V) {\n");
        m_cHashValuep = جديد Ast_متن_بلاک{fl, "const int expected_hash__V = ", "", ""};
        txtp->جوڑو(m_cHashValuep);
        txtp->جوڑو(/**/ "if (protectlib_hash__V != expected_hash__V) {\n");
        txtp->جوڑو(/****/ "fprintf(stderr, \"%%Error: cannot use " + m_libName
                  + " library, "
                    "Verliog (%u) and library (%u) hash values do not "
                    "agree\\n\", protectlib_hash__V, expected_hash__V);\n");
        txtp->جوڑو(/****/ "std::exit(EXIT_FAILURE);\n");
        txtp->جوڑو(/**/ "}\n");
        txtp->جوڑو("}\n\n");

        // Initial
        initialComment(txtp, fl);
        txtp->جوڑو("void* " + m_libName + "_protectlib_create(const char* scopep__V) {\n");
        txtp->جوڑو(/**/ m_چوٹی_Name + "_container* const handlep__V = new " + m_چوٹی_Name
                  + "_container{scopep__V};\n");
        txtp->جوڑو(/**/ "return handlep__V;\n");
        txtp->جوڑو("}\n\n");

        // Updates
        comboComment(txtp, fl);
        m_cComboParamsp = جديد Ast_متن_بلاک{
            fl, "long long " + m_libName + "_protectlib_combo_update(\n", ",\n", "\n) {\n"};
        m_cComboParamsp->جوڑو("void* vhandlep__V");
        txtp->جوڑو(m_cComboParamsp);
        m_cComboInsp = جديد Ast_متن_بلاک{fl};
        castPtr(fl, m_cComboInsp);
        txtp->جوڑو(m_cComboInsp);
        txtp->جوڑو("handlep__V->eval();\n");
        m_cComboOutsp = جديد Ast_متن_بلاک{fl};
        txtp->جوڑو(m_cComboOutsp);
        txtp->جوڑو("return handlep__V->m_seqnum++;\n");
        txtp->جوڑو("}\n\n");

        إذا (m_hasClk) {
            seqComment(txtp, fl);
            m_cSeqParamsp = جديد Ast_متن_بلاک{
                fl, "long long " + m_libName + "_protectlib_seq_update(\n", ",\n", "\n) {\n"};
            m_cSeqParamsp->جوڑو("void* vhandlep__V");
            txtp->جوڑو(m_cSeqParamsp);
            m_cSeqClksp = جديد Ast_متن_بلاک{fl};
            castPtr(fl, m_cSeqClksp);
            txtp->جوڑو(m_cSeqClksp);
            txtp->جوڑو("handlep__V->eval();\n");
            m_cSeqOutsp = جديد Ast_متن_بلاک{fl};
            txtp->جوڑو(m_cSeqOutsp);
            txtp->جوڑو("return handlep__V->m_seqnum++;\n");
            txtp->جوڑو("}\n\n");
        }

        comboIgnoreComment(txtp, fl);
        m_cIgnoreParamsp = جديد Ast_متن_بلاک{
            fl, "void " + m_libName + "_protectlib_combo_ignore(\n", ",\n", "\n)\n{ }\n\n"};
        m_cIgnoreParamsp->جوڑو("void* vhandlep__V");
        txtp->جوڑو(m_cIgnoreParamsp);

        // Final
        finalComment(txtp, fl);
        txtp->جوڑو("void " + m_libName + "_protectlib_final(void* vhandlep__V) {\n");
        castPtr(fl, txtp);
        txtp->جوڑو(/**/ "handlep__V->final();\n");
        txtp->جوڑو(/**/ "delete handlep__V;\n");
        txtp->جوڑو("}\n\n");

        txtp->جوڑو("}\n");
        m_cfilep->tblockp(txtp);
    }

    فراغ ویکھو(AstVar* nodep) override {
        إذا (!nodep->isIO()) أرجع;
        إذا (restoreInout(nodep)) {
            InoutPorts& ports = m_inouts[nodep->libInoutId()];
            إذا (nodep->is_داخلہ()) {
                ports.inp = nodep;
            } وإلا إذا (nodep->libInoutEnable()) {
                ports.enp = nodep;
            } وإلا {
                ports.outp = nodep;
            }
        }
        إذا (nodep->direction() == VDirection::داخلہ) {
            إذا (nodep->isPrimaryClock()) {
                UASSERT_OBJ(m_hasClk, nodep, "checkIfClockExists() didn't find this clock");
                handleClock(nodep);
            } وإلا {
                handle_انکڑے_داخلہ(nodep);
            }
        } وإلا {
            // Ref ports were rejected during linking, and tristate lowering split all inouts.
            UASSERT_OBJ(nodep->direction() == VDirection::OUTPUT, nodep,
                        "Unexpected library port direction");
            handleOutput(nodep);
        }
    }

    فراغ ویکھو(Ast_گرہ*) override {}

    string c_داخلہ_Connection(AstVar* varp) {
        أرجع V3Task::سونپو_Dpi_منزل_اندرونی("handlep__V->" + varp->name(), varp);
    }

    // The wrapper's module ports are connected by name, so they must use the user's
    // names rather than the hashed form of long names used internally
    ساكن string sv_پورٹ_Name(ثابت AstVar* varp) {
        أرجع Ast_گرہ::prettyName(VName::dehash(varp->name()));
    }

    ساكن AstVar* new_Sv_پورٹ_Var(AstVar* varp) {
        AstVar* ثابت newVarp = varp->clone_درخت(خطأ);
        newVarp->name(VName::dehash(varp->name()));
        أرجع newVarp;
    }

    فراغ handleClock(AstVar* varp) {
        handle_داخلہ(varp);
        m_seqPortsp->جوڑو(varp->clone_درخت(خطأ));
        إذا (m_hasClk) {
            ثابت std::string pname = sv_پورٹ_Name(varp);
            m_seqParamsp->جوڑو(pname);
            m_clkSensp->جوڑو(pname);
        }
        m_cSeqParamsp->جوڑو(varp->dpiArgType(سچ, خطأ));
        m_cSeqClksp->جوڑو(c_داخلہ_Connection(varp));
    }

    فراغ handle_انکڑے_داخلہ(AstVar* varp) {
        handle_داخلہ(varp);
        m_comboPortsp->جوڑو(varp->clone_درخت(خطأ));
        m_comboParamsp->جوڑو(sv_پورٹ_Name(varp));
        m_comboIgnorePortsp->جوڑو(varp->clone_درخت(خطأ));
        إذا (m_hasClk) m_comboIgnoreParamsp->جوڑو(sv_پورٹ_Name(varp));
        m_cComboParamsp->جوڑو(varp->dpiArgType(سچ, خطأ));
        m_cComboInsp->جوڑو(c_داخلہ_Connection(varp));
        m_cIgnoreParamsp->جوڑو(varp->dpiArgType(سچ, خطأ));
    }

    ساكن منطقي restoreInout(ثابت AstVar* varp) {
        أرجع varp->libInoutId() && !v3Global.opt.pinsInoutEnables();
    }

    فراغ handle_داخلہ(AstVar* varp) {
        AstVar* ثابت portp = new_Sv_پورٹ_Var(varp);
        إذا (restoreInout(varp)) {
            portp->direction(VDirection::INOUT);
            portp->varType(VVarType::WIRE);
        }
        m_modPortsp->جوڑو(portp);
    }

    فراغ restoreInouts() {
        لكل (ثابت خودکار& اندراج : m_inouts) {
            ثابت InoutPorts& ports = اندراج.دوجا;
            UASSERT_OBJ(ports.inp && ports.outp && ports.enp, m_vfilep,
                        "Incomplete lowered library inout");
            ثابت string وچ = sv_پورٹ_Name(ports.inp);
            ثابت string باہرلا = ports.outp->prettyName();
            ثابت string en = ports.enp->prettyName();
            std::سمتیہ<std::pair<صحيح, صحيح>> dimensions;
            ثابت Ast_گرہ_DType* dtypep = ports.inp->dtypep()->skipRefp();
            طالما (ثابت Ast_گرہ_Array_DType* ثابت arrayp = VN_CAST(dtypep, گرہ_Array_DType)) {
                // V3Slice currently rejects lowered unpacked inouts before this pass.
                // TODO: Support unpacked inouts once V3Slice supports their lowered form.
                UASSERT_OBJ(!VN_IS(arrayp, UnpackArrayDType), ports.inp,
                            "Unpacked library inout should have been rejected by V3Slice");
                dimensions.emplace_پٹھ(arrayp->lo(), arrayp->hi());
                dtypep = arrayp->subDTypep()->skipRefp();
            }
            ثابت AstBasicDType* ثابت basicp = dtypep->basicp();
            UASSERT_OBJ(basicp, ports.inp, "Library inout must have a packed integral type");
            إذا (basicp->isRanged()) dimensions.emplace_پٹھ(basicp->lo(), basicp->hi());
            إذا (dimensions.خالی_تھاواں()) {
                m_inoutDeclsp->جوڑو("assign " + وچ + " = " + en + " ? " + باہرلا + " : 1'bz;\n");
            } وإلا {
                string lhs = وچ;
                string rhsEn = en;
                string rhs_باہرلا = باہرلا;
                لكل (ناپ_t dim = 0; dim < dimensions.ناپ(); ++dim) {
                    ثابت string اشاریہ
                        = "__VlibInout" + cvt_منزل_Str(اندراج.پہلا) + "_" + cvt_منزل_Str(dim);
                    ثابت صحيح کم = dimensions[dim].پہلا;
                    ثابت صحيح بلند = dimensions[dim].دوجا;
                    ثابت صحيح ناپ = بلند - کم + 1;
                    m_inoutDeclsp->جوڑو("for (genvar " + اشاریہ + " = 0; " + اشاریہ + " < "
                                       + cvt_منزل_Str(ناپ) + "; ++" + اشاریہ + ") begin\n");
                    ثابت string select = "[" + cvt_منزل_Str(کم) + " + " + اشاریہ + "]";
                    lhs += select;
                    rhsEn += select;
                    rhs_باہرلا += select;
                }
                m_inoutDeclsp->جوڑو("assign " + lhs + " = " + rhsEn + " ? " + rhs_باہرلا
                                   + " : 1'bz;\n");
                لكل (ناپ_t dim = 0; dim < dimensions.ناپ(); ++dim) m_inoutDeclsp->جوڑو("end\n");
            }
        }
    }

    ساكن فراغ جوڑو_Local_Variable(Ast_متن_بلاک* textp, ثابت AstVar* varp, ثابت حرف* suffix) {
        AstVar* ثابت newVarp
            = جديد AstVar{varp->fileline(), VVarType::VAR, varp->name() + suffix, varp->dtypep()};
        textp->جوڑو(newVarp);
    }

    فراغ handleOutput(AstVar* ثابت varp) {
        ثابت std::string pname = varp->prettyName();
        // Restored inout drivers are local signals and retain their internal names.
        ثابت std::string پورٹ_Name = restoreInout(varp) ? pname : sv_پورٹ_Name(varp);
        إذا (restoreInout(varp)) {
            AstVar* ثابت localp = varp->clone_درخت(خطأ);
            localp->direction(VDirection::NONE);
            m_inoutDeclsp->جوڑو(localp);
        } وإلا {
            m_modPortsp->جوڑو(new_Sv_پورٹ_Var(varp));
        }
        m_comboPortsp->جوڑو(varp->clone_درخت(خطأ));
        m_comboParamsp->جوڑو(pname + "_combo__V");
        إذا (m_hasClk) {
            m_seqPortsp->جوڑو(varp->clone_درخت(خطأ));
            m_seqParamsp->جوڑو(pname + "_tmp__V");
        }

        جوڑو_Local_Variable(m_comboDeclsp, varp, "_combo__V");

        إذا (m_hasClk) {
            جوڑو_Local_Variable(m_seqDeclsp, varp, "_seq__V");
            جوڑو_Local_Variable(m_tmpDeclsp, varp, "_tmp__V");

            m_nbAssignsp->جوڑو(pname + "_seq__V <= " + pname + "_tmp__V;\n");
            m_seqAssignsp->جوڑو(پورٹ_Name + " = " + pname + "_seq__V;\n");
        }
        m_comboAssignsp->جوڑو(پورٹ_Name + " = " + pname + "_combo__V;\n");
        m_cComboParamsp->جوڑو(varp->dpiArgType(سچ, خطأ));
        m_cComboOutsp->جوڑو(V3Task::سونپو_اندرونی_منزل_Dpi(varp, سچ, "", "", "handlep__V->") + "\n");
        إذا (m_hasClk) {
            m_cSeqParamsp->جوڑو(varp->dpiArgType(سچ, خطأ));
            m_cSeqOutsp->جوڑو(V3Task::سونپو_اندرونی_منزل_Dpi(varp, سچ, "", "", "handlep__V->")
                             + "\n");
        }
    }

    ساكن منطقي checkIfClockExists(ثابت Ast_گرہ_Module* modp) {
        لكل (ثابت Ast_گرہ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (ثابت AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                إذا (varp->direction() == VDirection::داخلہ && varp->isPrimaryClock()) {
                    أرجع سچ;
                }
            }
        }
        أرجع خطأ;
    }

عام:
    واضح ProtectVisitor(Ast_گرہ* nodep)
        : m_libName{v3Global.opt.libCreate()}
        , m_چوٹی_Name{v3Global.opt.اگیتر()} {
        iterate(nodep);
    }
};

//######################################################################
// ProtectLib class functions

فراغ V3ProtectLib::protect() {
    UINFO(2, __FUNCTION__ << ":");
    ProtectVisitor{v3Global.rootp()};
}
