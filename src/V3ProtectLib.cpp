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
        AstVar* inp = اشاره_تهی;  // Resolved input value and original public port
        AstVar* outp = اشاره_تهی;  // Library drive value
        AstVar* enp = اشاره_تهی;  // Per-bit drive enable
    };

    // MEMBERS
    AstVFile* m_vfilep = اشاره_تهی;  // DPI-enabled Verilog wrapper
    AstCFile* m_cfilep = اشاره_تهی;  // C implementation of DPI functions
    // Verilog text blocks
    Ast_متن_بلوک* m_modPortsp = اشاره_تهی;  // Module port list
    Ast_متن_بلوک* m_comboPortsp = اشاره_تهی;  // Combo function port list
    Ast_متن_بلوک* m_seqPortsp = اشاره_تهی;  // Sequential function port list
    Ast_متن_بلوک* m_comboIgnorePortsp = اشاره_تهی;  // Combo ignore function port list
    Ast_متن_بلوک* m_comboDeclsp = اشاره_تهی;  // Combo signal declaration list
    Ast_متن_بلوک* m_seqDeclsp = اشاره_تهی;  // Sequential signal declaration list
    Ast_متن_بلوک* m_tmpDeclsp = اشاره_تهی;  // Temporary signal declaration list
    Ast_متن_بلوک* m_inoutDeclsp = اشاره_تهی;  // Lowered inout signals and tristate drivers
    Ast_متن_بلوک* m_hashValuep = اشاره_تهی;  // CPP hash value
    Ast_متن_بلوک* m_comboParamsp = اشاره_تهی;  // Combo function parameter list
    Ast_متن_بلوک* m_clkSensp = اشاره_تهی;  // Clock sensitivity list
    Ast_متن_بلوک* m_comboIgnoreParamsp = اشاره_تهی;  // Combo ignore parameter list
    Ast_متن_بلوک* m_seqParamsp = اشاره_تهی;  // Sequential parameter list
    Ast_متن_بلوک* m_nbAssignsp = اشاره_تهی;  // Non-blocking assignment list
    Ast_متن_بلوک* m_seqAssignsp = اشاره_تهی;  // Sequential assignment list
    Ast_متن_بلوک* m_comboAssignsp = اشاره_تهی;  // Combo assignment list
    // C text blocks
    Ast_متن_بلوک* m_cHashValuep = اشاره_تهی;  // CPP hash value
    Ast_متن_بلوک* m_cComboParamsp = اشاره_تهی;  // Combo function parameter list
    Ast_متن_بلوک* m_cComboInsp = اشاره_تهی;  // Combo input copy list
    Ast_متن_بلوک* m_cComboOutsp = اشاره_تهی;  // Combo output copy list
    Ast_متن_بلوک* m_cSeqParamsp = اشاره_تهی;  // Sequential parameter list
    Ast_متن_بلوک* m_cSeqClksp = اشاره_تهی;  // Sequential clock copy list
    Ast_متن_بلوک* m_cSeqOutsp = اشاره_تهی;  // Sequential output copy list
    Ast_متن_بلوک* m_cIgnoreParamsp = اشاره_تهی;  // Combo ignore parameter list
    ثابت string m_libName;
    ثابت string m_بالاترین_Name;
    منطقي m_یافت_بالاترین = خطأ;  // Have seen the top module
    منطقي m_hasClk = خطأ;  // True if the top module has sequential logic
    std::map<صحيح, InoutPorts> m_inouts;  // Lowered ports grouped by original inout

    // VISITORS
    فراغ بازدید(AstNetlist* nodep) override {
        m_vfilep
            = جديد AstVFile{nodep->fileline(), v3Global.opt.makeDir() + "/" + m_libName + ".sv"};
        nodep->جمع_کن_Filesp(m_vfilep);
        m_cfilep
            = جديد AstCFile{nodep->fileline(), v3Global.opt.makeDir() + "/" + m_libName + ".cpp"};
        nodep->جمع_کن_Filesp(m_cfilep);
        iterateChildren(nodep);
    }

    فراغ بازدید(Ast_گره_Module* nodep) override {
        إذا (!nodep->is_بالاترین()) {
            أرجع;
        } وإلا {
            UASSERT_OBJ(!m_یافت_بالاترین, nodep, "Multiple root modules");
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
        m_hashValuep->جمع_کن(cvt_مقصد_Str(hash.مقدار()) + ";\n");
        m_cHashValuep->جمع_کن(cvt_مقصد_Str(hash.مقدار()) + "U;\n");
        m_یافت_بالاترین = درست;
    }

    فراغ جمع_کن_Comment(Ast_متن_بلوک* txtp, File_سطر* fl, ثابت string& comment) {
        txtp->جمع_کن(جديد AstComment{fl, comment});
    }

    فراغ configSection(Ast_گره_Module* /*modp*/, Ast_متن_بلوک* txtp, File_سطر* /*fl*/) {
        txtp->جمع_کن("\n`ifdef VERILATOR\n");
        txtp->جمع_کن("`verilator_config\n");

        txtp->جمع_کن("verilator_lib -module \"" + m_libName + "\"\n");

        txtp->جمع_کن("profile_data -hier-dpi \"" + m_libName + "_protectlib_combo_update\" -cost 64'd"
                  + std::مقصد_string(v3Global.current_Hier_بلوک_هزینه()) + "\n");
        txtp->جمع_کن("profile_data -hier-dpi \"" + m_libName + "_protectlib_seq_update\" -cost 64'd"
                  + std::مقصد_string(v3Global.current_Hier_بلوک_هزینه()) + "\n");

        // Mark remaining NBA protectlib wrapper DPIs as non-hazardous by deliberately forwarding
        // them with non-zero cost.
        // Also, specify hierarchical workers for those tasks for scheduling.
        txtp->جمع_کن("profile_data -hier-dpi \"" + m_libName
                  + "_protectlib_combo_ignore\" -cost 64'd1\n");

        txtp->جمع_کن("hier_workers -hier-dpi \"" + m_libName
                  + "_protectlib_combo_update\" -workers 16'd"
                  + std::مقصد_string(V3Control::getHierWorkers(m_libName)) + "\n");
        txtp->جمع_کن("hier_workers -hier-dpi \"" + m_libName
                  + "_protectlib_seq_update\" -workers 16'd"
                  + std::مقصد_string(V3Control::getHierWorkers(m_libName)) + "\n");
        // No workers for combo_ignore
        txtp->جمع_کن("`verilog\n");
        txtp->جمع_کن("`endif\n");
    }

    فراغ hashComment(Ast_متن_بلوک* txtp, File_سطر* fl) {
        جمع_کن_Comment(txtp, fl, "Checks to make sure the .sv wrapper and library agree");
    }

    فراغ initialComment(Ast_متن_بلوک* txtp, File_سطر* fl) {
        جمع_کن_Comment(txtp, fl, "Creates an instance of the library module at initial-time");
        جمع_کن_Comment(txtp, fl, "(one for each instance in the user's design)");
    }

    فراغ comboComment(Ast_متن_بلوک* txtp, File_سطر* fl) {
        جمع_کن_Comment(txtp, fl, "Updates all non-clock inputs and retrieves the results");
    }

    فراغ seqComment(Ast_متن_بلوک* txtp, File_سطر* fl) {
        جمع_کن_Comment(txtp, fl, "Updates all clocks and retrieves the results");
    }

    فراغ comboIgnoreComment(Ast_متن_بلوک* txtp, File_سطر* fl) {
        جمع_کن_Comment(txtp, fl, "Need to convince some simulators that the input to the module");
        جمع_کن_Comment(txtp, fl, "must be evaluated before evaluating the clock edge");
    }

    فراغ finalComment(Ast_متن_بلوک* txtp, File_سطر* fl) {
        جمع_کن_Comment(txtp, fl, "Evaluates the library module's final process");
    }

    فراغ createSvFile(File_سطر* fl, Ast_گره_Module* modp) {
        // Comments
        Ast_متن_بلوک* ثابت txtp = جديد Ast_متن_بلوک{fl};
        جمع_کن_Comment(txtp, fl, "Wrapper module for DPI protected library");
        جمع_کن_Comment(txtp, fl,
                   "This module requires lib" + m_libName + ".a or lib" + m_libName
                       + ".so to work");
        جمع_کن_Comment(txtp, fl,
                   "See instructions in your simulator for how"
                   " to use DPI libraries\n");

        // Module declaration
        // Wrapper ranges preserve the original RTL, including intentional ascending ranges.
        txtp->جمع_کن("/*verilator lint_save*/\n");
        txtp->جمع_کن("/*verilator lint_off ASCRANGE*/\n");
        m_modPortsp = جديد Ast_متن_بلوک{fl, "module " + m_libName + " (\n", ", ", ");\n\n"};
        txtp->جمع_کن(m_modPortsp);

        // Timescale
        إذا (v3Global.opt.hier_فرزند() && v3Global.rootp()->timescaleSpecified()) {
            // Emit timescale for hierarchical Verilation if input HDL specifies timespec
            txtp->جمع_کن("timeunit "s + modp->timeunit().ascii() + ";\n");
            txtp->جمع_کن("timeprecision "s + +v3Global.rootp()->timeprecision().ascii() + ";\n");
        } وإلا {
            جمع_کن_Comment(txtp, fl,
                       "Precision of submodule"
                       " (commented out to avoid requiring timescale on all modules)");
            جمع_کن_Comment(txtp, fl, "timeunit "s + v3Global.rootp()->timeunit().ascii() + ";");
            جمع_کن_Comment(txtp, fl,
                       "timeprecision "s + v3Global.rootp()->timeprecision().ascii() + ";\n");
        }

        // DPI declarations
        hashComment(txtp, fl);
        txtp->جمع_کن("import \"DPI-C\" function void " + m_libName
                  + "_protectlib_check_hash(int protectlib_hash__V);\n\n");
        initialComment(txtp, fl);
        txtp->جمع_کن("import \"DPI-C\" function chandle " + m_libName
                  + "_protectlib_create(string scope__V);\n\n");
        comboComment(txtp, fl);
        m_comboPortsp = جديد Ast_متن_بلوک{fl,  //
                                         "import \"DPI-C\" function longint " + m_libName
                                             + "_protectlib_combo_update(\n",  //
                                         ", ",  //
                                         ");\n\n"};
        m_comboPortsp->جمع_کن("chandle handle__V\n");
        txtp->جمع_کن(m_comboPortsp);
        seqComment(txtp, fl);
        إذا (m_hasClk) {
            m_seqPortsp = جديد Ast_متن_بلوک{fl,  //
                                           "import \"DPI-C\" function longint " + m_libName
                                               + "_protectlib_seq_update(\n",  //
                                           ", ",  //
                                           ");\n\n"};
            m_seqPortsp->جمع_کن("chandle handle__V\n");
            txtp->جمع_کن(m_seqPortsp);
        }
        comboIgnoreComment(txtp, fl);
        m_comboIgnorePortsp = جديد Ast_متن_بلوک{fl,  //
                                               "import \"DPI-C\" function void " + m_libName
                                                   + "_protectlib_combo_ignore(\n",  //
                                               ", ",  //
                                               ");\n\n"};
        m_comboIgnorePortsp->جمع_کن("chandle handle__V\n");
        txtp->جمع_کن(m_comboIgnorePortsp);

        finalComment(txtp, fl);
        txtp->جمع_کن("import \"DPI-C\" function void " + m_libName
                  + "_protectlib_final(chandle handle__V);\n\n");

        // Local variables
        // Avoid tracing handle, as it is not a stable value, so breaks wavediff
        // Likewise other internals aren't interesting to the user
        txtp->جمع_کن("// verilator tracing_off\n");

        txtp->جمع_کن("chandle handle__V;\n");
        txtp->جمع_کن("time last_combo_seqnum__V;\n");
        إذا (m_hasClk) txtp->جمع_کن("time last_seq_seqnum__V;\n");
        txtp->جمع_کن("\n");

        m_comboDeclsp = جديد Ast_متن_بلوک{fl};
        txtp->جمع_کن(m_comboDeclsp);
        m_seqDeclsp = جديد Ast_متن_بلوک{fl};
        txtp->جمع_کن(m_seqDeclsp);
        m_tmpDeclsp = جديد Ast_متن_بلوک{fl};
        txtp->جمع_کن(m_tmpDeclsp);
        m_inoutDeclsp = جديد Ast_متن_بلوک{fl};
        txtp->جمع_کن(m_inoutDeclsp);

        // CPP hash value
        جمع_کن_Comment(txtp, fl, "Hash value to make sure this file and the corresponding");
        جمع_کن_Comment(txtp, fl, "library agree");
        m_hashValuep = جديد Ast_متن_بلوک{
            fl, "localparam int protectlib_hash__V /*verilator public_flat_rd*/ = 32'd", "", "\n"};
        txtp->جمع_کن(m_hashValuep);

        // Initial
        txtp->جمع_کن("initial begin\n");
        txtp->جمع_کن(m_libName + "_protectlib_check_hash(protectlib_hash__V);\n");
        txtp->جمع_کن("if (handle__V == null) handle__V = " + m_libName
                  + "_protectlib_create"
                    "($sformatf(\"%m\"));\n");
        txtp->جمع_کن("end\n\n");

        // Combinatorial process
        جمع_کن_Comment(txtp, fl, "Combinatorially evaluate changes to inputs");
        txtp->جمع_کن("always_comb begin\n");
        m_comboParamsp = جديد Ast_متن_بلوک{fl,  //
                                          "last_combo_seqnum__V = " + m_libName
                                              + "_protectlib_combo_update(\n",  //
                                          ",\n",  //
                                          "\n);\n"};
        m_comboParamsp->جمع_کن("handle__V");
        txtp->جمع_کن(m_comboParamsp);
        txtp->جمع_کن("end\n\n");

        // Sequential process
        إذا (m_hasClk) {
            جمع_کن_Comment(txtp, fl, "Evaluate clock edges");
            m_clkSensp = جديد Ast_متن_بلوک{fl, "always @(", " or ", ")"};
            txtp->جمع_کن(m_clkSensp);
            txtp->جمع_کن(" begin\n");
            m_comboIgnoreParamsp
                = جديد Ast_متن_بلوک{fl,  //
                                   m_libName + "_protectlib_combo_ignore(\n", ",\n", "\n);\n"};
            m_comboIgnoreParamsp->جمع_کن("handle__V");
            txtp->جمع_کن(m_comboIgnoreParamsp);
            m_seqParamsp = جديد Ast_متن_بلوک{fl,  //
                                            "last_seq_seqnum__V <= " + m_libName
                                                + "_protectlib_seq_update(\n",  //
                                            ",\n",  //
                                            "\n);\n"};
            m_seqParamsp->جمع_کن("handle__V");
            txtp->جمع_کن(m_seqParamsp);
            m_nbAssignsp = جديد Ast_متن_بلوک{fl};
            txtp->جمع_کن(m_nbAssignsp);
            txtp->جمع_کن("end\n\n");
        }

        // Select between combinatorial and sequential results
        جمع_کن_Comment(txtp, fl, "Select between combinatorial and sequential results");
        txtp->جمع_کن("always_comb begin\n");
        إذا (m_hasClk) {
            txtp->جمع_کن("if (last_seq_seqnum__V > last_combo_seqnum__V) begin\n");
            m_seqAssignsp = جديد Ast_متن_بلوک{fl};
            txtp->جمع_کن(m_seqAssignsp);
            txtp->جمع_کن("end else begin\n");
            m_comboAssignsp = جديد Ast_متن_بلوک{fl};
            txtp->جمع_کن(m_comboAssignsp);
            txtp->جمع_کن("end\n");
        } وإلا {
            m_comboAssignsp = جديد Ast_متن_بلوک{fl};
            txtp->جمع_کن(m_comboAssignsp);
        }
        txtp->جمع_کن("end\n\n");

        // Final
        txtp->جمع_کن("final " + m_libName + "_protectlib_final(handle__V);\n\n");

        txtp->جمع_کن("endmodule\n");
        txtp->جمع_کن("/*verilator lint_restore*/\n");

        configSection(modp, txtp, fl);

        m_vfilep->tblockp(txtp);
    }

    فراغ castPtr(File_سطر* /*fl*/, Ast_متن_بلوک* txtp) {
        txtp->جمع_کن(m_بالاترین_Name
                  + "_container* const handlep__V = "  // LCOV_EXCL_LINE  // lcov bug
                    "static_cast<"
                  + m_بالاترین_Name + "_container*>(vhandlep__V);\n");
    }

    فراغ createCppFile(File_سطر* fl) {
        // Comments
        Ast_متن_بلوک* ثابت txtp = جديد Ast_متن_بلوک{fl};
        جمع_کن_Comment(txtp, fl, "Wrapper functions for DPI protected library\n");

        // Includes
        txtp->جمع_کن("#include \"" + m_بالاترین_Name + ".h\"\n");
        txtp->جمع_کن("#include \"verilated_dpi.h\"\n\n");
        txtp->جمع_کن("#include <cstdio>\n");
        txtp->جمع_کن("#include <cstdlib>\n\n");

        // Verilated module plus sequence number
        جمع_کن_Comment(txtp, fl, "Container class to house verilated object and sequence number");
        txtp->جمع_کن("class " + m_بالاترین_Name + "_container: public " + m_بالاترین_Name + " {\n");
        txtp->جمع_کن("public:\n");
        txtp->جمع_کن("long long m_seqnum;\n");
        txtp->جمع_کن(m_بالاترین_Name + "_container(const char* scopep__V):\n");
        txtp->جمع_کن(m_بالاترین_Name + "(scopep__V) {}\n");
        txtp->جمع_کن("};\n\n");

        // Extern C
        txtp->جمع_کن("extern \"C\" {\n\n");

        // Hash check
        hashComment(txtp, fl);
        txtp->جمع_کن("void " + m_libName
                  + "_protectlib_check_hash"
                    "(int protectlib_hash__V) {\n");
        m_cHashValuep = جديد Ast_متن_بلوک{fl, "const int expected_hash__V = ", "", ""};
        txtp->جمع_کن(m_cHashValuep);
        txtp->جمع_کن(/**/ "if (protectlib_hash__V != expected_hash__V) {\n");
        txtp->جمع_کن(/****/ "fprintf(stderr, \"%%Error: cannot use " + m_libName
                  + " library, "
                    "Verliog (%u) and library (%u) hash values do not "
                    "agree\\n\", protectlib_hash__V, expected_hash__V);\n");
        txtp->جمع_کن(/****/ "std::exit(EXIT_FAILURE);\n");
        txtp->جمع_کن(/**/ "}\n");
        txtp->جمع_کن("}\n\n");

        // Initial
        initialComment(txtp, fl);
        txtp->جمع_کن("void* " + m_libName + "_protectlib_create(const char* scopep__V) {\n");
        txtp->جمع_کن(/**/ m_بالاترین_Name + "_container* const handlep__V = new " + m_بالاترین_Name
                  + "_container{scopep__V};\n");
        txtp->جمع_کن(/**/ "return handlep__V;\n");
        txtp->جمع_کن("}\n\n");

        // Updates
        comboComment(txtp, fl);
        m_cComboParamsp = جديد Ast_متن_بلوک{
            fl, "long long " + m_libName + "_protectlib_combo_update(\n", ",\n", "\n) {\n"};
        m_cComboParamsp->جمع_کن("void* vhandlep__V");
        txtp->جمع_کن(m_cComboParamsp);
        m_cComboInsp = جديد Ast_متن_بلوک{fl};
        castPtr(fl, m_cComboInsp);
        txtp->جمع_کن(m_cComboInsp);
        txtp->جمع_کن("handlep__V->eval();\n");
        m_cComboOutsp = جديد Ast_متن_بلوک{fl};
        txtp->جمع_کن(m_cComboOutsp);
        txtp->جمع_کن("return handlep__V->m_seqnum++;\n");
        txtp->جمع_کن("}\n\n");

        إذا (m_hasClk) {
            seqComment(txtp, fl);
            m_cSeqParamsp = جديد Ast_متن_بلوک{
                fl, "long long " + m_libName + "_protectlib_seq_update(\n", ",\n", "\n) {\n"};
            m_cSeqParamsp->جمع_کن("void* vhandlep__V");
            txtp->جمع_کن(m_cSeqParamsp);
            m_cSeqClksp = جديد Ast_متن_بلوک{fl};
            castPtr(fl, m_cSeqClksp);
            txtp->جمع_کن(m_cSeqClksp);
            txtp->جمع_کن("handlep__V->eval();\n");
            m_cSeqOutsp = جديد Ast_متن_بلوک{fl};
            txtp->جمع_کن(m_cSeqOutsp);
            txtp->جمع_کن("return handlep__V->m_seqnum++;\n");
            txtp->جمع_کن("}\n\n");
        }

        comboIgnoreComment(txtp, fl);
        m_cIgnoreParamsp = جديد Ast_متن_بلوک{
            fl, "void " + m_libName + "_protectlib_combo_ignore(\n", ",\n", "\n)\n{ }\n\n"};
        m_cIgnoreParamsp->جمع_کن("void* vhandlep__V");
        txtp->جمع_کن(m_cIgnoreParamsp);

        // Final
        finalComment(txtp, fl);
        txtp->جمع_کن("void " + m_libName + "_protectlib_final(void* vhandlep__V) {\n");
        castPtr(fl, txtp);
        txtp->جمع_کن(/**/ "handlep__V->final();\n");
        txtp->جمع_کن(/**/ "delete handlep__V;\n");
        txtp->جمع_کن("}\n\n");

        txtp->جمع_کن("}\n");
        m_cfilep->tblockp(txtp);
    }

    فراغ بازدید(AstVar* nodep) override {
        إذا (!nodep->isIO()) أرجع;
        إذا (restoreInout(nodep)) {
            InoutPorts& ports = m_inouts[nodep->libInoutId()];
            إذا (nodep->is_درون_داد()) {
                ports.inp = nodep;
            } وإلا إذا (nodep->libInoutEnable()) {
                ports.enp = nodep;
            } وإلا {
                ports.outp = nodep;
            }
        }
        إذا (nodep->direction() == VDirection::درون_داد) {
            إذا (nodep->isPrimaryClock()) {
                UASSERT_OBJ(m_hasClk, nodep, "checkIfClockExists() didn't find this clock");
                handleClock(nodep);
            } وإلا {
                handle_داده_درون_داد(nodep);
            }
        } وإلا {
            // Ref ports were rejected during linking, and tristate lowering split all inouts.
            UASSERT_OBJ(nodep->direction() == VDirection::OUTPUT, nodep,
                        "Unexpected library port direction");
            handleOutput(nodep);
        }
    }

    فراغ بازدید(Ast_گره*) override {}

    string c_درون_داد_Connection(AstVar* varp) {
        أرجع V3Task::تخصیص_بده_Dpi_مقصد_داخلی("handlep__V->" + varp->name(), varp);
    }

    // The wrapper's module ports are connected by name, so they must use the user's
    // names rather than the hashed form of long names used internally
    ساكن string sv_پورت_Name(ثابت AstVar* varp) {
        أرجع Ast_گره::prettyName(VName::dehash(varp->name()));
    }

    ساكن AstVar* new_Sv_پورت_Var(AstVar* varp) {
        AstVar* ثابت newVarp = varp->clone_درخت(خطأ);
        newVarp->name(VName::dehash(varp->name()));
        أرجع newVarp;
    }

    فراغ handleClock(AstVar* varp) {
        handle_درون_داد(varp);
        m_seqPortsp->جمع_کن(varp->clone_درخت(خطأ));
        إذا (m_hasClk) {
            ثابت std::string pname = sv_پورت_Name(varp);
            m_seqParamsp->جمع_کن(pname);
            m_clkSensp->جمع_کن(pname);
        }
        m_cSeqParamsp->جمع_کن(varp->dpiArgType(درست, خطأ));
        m_cSeqClksp->جمع_کن(c_درون_داد_Connection(varp));
    }

    فراغ handle_داده_درون_داد(AstVar* varp) {
        handle_درون_داد(varp);
        m_comboPortsp->جمع_کن(varp->clone_درخت(خطأ));
        m_comboParamsp->جمع_کن(sv_پورت_Name(varp));
        m_comboIgnorePortsp->جمع_کن(varp->clone_درخت(خطأ));
        إذا (m_hasClk) m_comboIgnoreParamsp->جمع_کن(sv_پورت_Name(varp));
        m_cComboParamsp->جمع_کن(varp->dpiArgType(درست, خطأ));
        m_cComboInsp->جمع_کن(c_درون_داد_Connection(varp));
        m_cIgnoreParamsp->جمع_کن(varp->dpiArgType(درست, خطأ));
    }

    ساكن منطقي restoreInout(ثابت AstVar* varp) {
        أرجع varp->libInoutId() && !v3Global.opt.pinsInoutEnables();
    }

    فراغ handle_درون_داد(AstVar* varp) {
        AstVar* ثابت portp = new_Sv_پورت_Var(varp);
        إذا (restoreInout(varp)) {
            portp->direction(VDirection::INOUT);
            portp->varType(VVarType::WIRE);
        }
        m_modPortsp->جمع_کن(portp);
    }

    فراغ restoreInouts() {
        لكل (ثابت خودکار& مدخل : m_inouts) {
            ثابت InoutPorts& ports = مدخل.دومی;
            UASSERT_OBJ(ports.inp && ports.outp && ports.enp, m_vfilep,
                        "Incomplete lowered library inout");
            ثابت string درون = sv_پورت_Name(ports.inp);
            ثابت string بیرون = ports.outp->prettyName();
            ثابت string en = ports.enp->prettyName();
            std::وکتور<std::pair<صحيح, صحيح>> dimensions;
            ثابت Ast_گره_DType* dtypep = ports.inp->dtypep()->skipRefp();
            طالما (ثابت Ast_گره_Array_DType* ثابت arrayp = VN_CAST(dtypep, گره_Array_DType)) {
                // V3Slice currently rejects lowered unpacked inouts before this pass.
                // TODO: Support unpacked inouts once V3Slice supports their lowered form.
                UASSERT_OBJ(!VN_IS(arrayp, UnpackArrayDType), ports.inp,
                            "Unpacked library inout should have been rejected by V3Slice");
                dimensions.emplace_پشت(arrayp->lo(), arrayp->hi());
                dtypep = arrayp->subDTypep()->skipRefp();
            }
            ثابت AstBasicDType* ثابت basicp = dtypep->basicp();
            UASSERT_OBJ(basicp, ports.inp, "Library inout must have a packed integral type");
            إذا (basicp->isRanged()) dimensions.emplace_پشت(basicp->lo(), basicp->hi());
            إذا (dimensions.خالی()) {
                m_inoutDeclsp->جمع_کن("assign " + درون + " = " + en + " ? " + بیرون + " : 1'bz;\n");
            } وإلا {
                string lhs = درون;
                string rhsEn = en;
                string rhs_بیرون = بیرون;
                لكل (اندازه_t dim = 0; dim < dimensions.اندازه(); ++dim) {
                    ثابت string شاخص
                        = "__VlibInout" + cvt_مقصد_Str(مدخل.اولی) + "_" + cvt_مقصد_Str(dim);
                    ثابت صحيح کم = dimensions[dim].اولی;
                    ثابت صحيح بالا = dimensions[dim].دومی;
                    ثابت صحيح اندازه = بالا - کم + 1;
                    m_inoutDeclsp->جمع_کن("for (genvar " + شاخص + " = 0; " + شاخص + " < "
                                       + cvt_مقصد_Str(اندازه) + "; ++" + شاخص + ") begin\n");
                    ثابت string select = "[" + cvt_مقصد_Str(کم) + " + " + شاخص + "]";
                    lhs += select;
                    rhsEn += select;
                    rhs_بیرون += select;
                }
                m_inoutDeclsp->جمع_کن("assign " + lhs + " = " + rhsEn + " ? " + rhs_بیرون
                                   + " : 1'bz;\n");
                لكل (اندازه_t dim = 0; dim < dimensions.اندازه(); ++dim) m_inoutDeclsp->جمع_کن("end\n");
            }
        }
    }

    ساكن فراغ جمع_کن_Local_Variable(Ast_متن_بلوک* textp, ثابت AstVar* varp, ثابت حرف* suffix) {
        AstVar* ثابت newVarp
            = جديد AstVar{varp->fileline(), VVarType::VAR, varp->name() + suffix, varp->dtypep()};
        textp->جمع_کن(newVarp);
    }

    فراغ handleOutput(AstVar* ثابت varp) {
        ثابت std::string pname = varp->prettyName();
        // Restored inout drivers are local signals and retain their internal names.
        ثابت std::string پورت_Name = restoreInout(varp) ? pname : sv_پورت_Name(varp);
        إذا (restoreInout(varp)) {
            AstVar* ثابت localp = varp->clone_درخت(خطأ);
            localp->direction(VDirection::NONE);
            m_inoutDeclsp->جمع_کن(localp);
        } وإلا {
            m_modPortsp->جمع_کن(new_Sv_پورت_Var(varp));
        }
        m_comboPortsp->جمع_کن(varp->clone_درخت(خطأ));
        m_comboParamsp->جمع_کن(pname + "_combo__V");
        إذا (m_hasClk) {
            m_seqPortsp->جمع_کن(varp->clone_درخت(خطأ));
            m_seqParamsp->جمع_کن(pname + "_tmp__V");
        }

        جمع_کن_Local_Variable(m_comboDeclsp, varp, "_combo__V");

        إذا (m_hasClk) {
            جمع_کن_Local_Variable(m_seqDeclsp, varp, "_seq__V");
            جمع_کن_Local_Variable(m_tmpDeclsp, varp, "_tmp__V");

            m_nbAssignsp->جمع_کن(pname + "_seq__V <= " + pname + "_tmp__V;\n");
            m_seqAssignsp->جمع_کن(پورت_Name + " = " + pname + "_seq__V;\n");
        }
        m_comboAssignsp->جمع_کن(پورت_Name + " = " + pname + "_combo__V;\n");
        m_cComboParamsp->جمع_کن(varp->dpiArgType(درست, خطأ));
        m_cComboOutsp->جمع_کن(V3Task::تخصیص_بده_داخلی_مقصد_Dpi(varp, درست, "", "", "handlep__V->") + "\n");
        إذا (m_hasClk) {
            m_cSeqParamsp->جمع_کن(varp->dpiArgType(درست, خطأ));
            m_cSeqOutsp->جمع_کن(V3Task::تخصیص_بده_داخلی_مقصد_Dpi(varp, درست, "", "", "handlep__V->")
                             + "\n");
        }
    }

    ساكن منطقي checkIfClockExists(ثابت Ast_گره_Module* modp) {
        لكل (ثابت Ast_گره* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (ثابت AstVar* ثابت varp = VN_CAST(stmtp, Var)) {
                إذا (varp->direction() == VDirection::درون_داد && varp->isPrimaryClock()) {
                    أرجع درست;
                }
            }
        }
        أرجع خطأ;
    }

عام:
    صریح ProtectVisitor(Ast_گره* nodep)
        : m_libName{v3Global.opt.libCreate()}
        , m_بالاترین_Name{v3Global.opt.پیشوند()} {
        iterate(nodep);
    }
};

//######################################################################
// ProtectLib class functions

فراغ V3ProtectLib::protect() {
    UINFO(2, __FUNCTION__ << ":");
    ProtectVisitor{v3Global.rootp()};
}
