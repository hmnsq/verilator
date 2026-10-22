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

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3ProtectLib.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Control.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Hasher.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3InstrCount.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3String.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Task.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// ProtectLib top-level visitor

ਵਰਗ ProtectVisitor final : ਜਨਤਕ VNVisitor {
    // TYPES
    ਬਣਤਰ InoutPorts final {
        AstVar* inp = ਖਾਲੀ_ਸੂਚਕ;  // Resolved input value and original public port
        AstVar* outp = ਖਾਲੀ_ਸੂਚਕ;  // Library drive value
        AstVar* enp = ਖਾਲੀ_ਸੂਚਕ;  // Per-bit drive enable
    };

    // MEMBERS
    AstVFile* m_vfilep = ਖਾਲੀ_ਸੂਚਕ;  // DPI-enabled Verilog wrapper
    AstCFile* m_cfilep = ਖਾਲੀ_ਸੂਚਕ;  // C implementation of DPI functions
    // Verilog text blocks
    Ast_ਪਾਠ_ਖੰਡ* m_modPortsp = ਖਾਲੀ_ਸੂਚਕ;  // Module port list
    Ast_ਪਾਠ_ਖੰਡ* m_comboPortsp = ਖਾਲੀ_ਸੂਚਕ;  // Combo function port list
    Ast_ਪਾਠ_ਖੰਡ* m_seqPortsp = ਖਾਲੀ_ਸੂਚਕ;  // Sequential function port list
    Ast_ਪਾਠ_ਖੰਡ* m_comboIgnorePortsp = ਖਾਲੀ_ਸੂਚਕ;  // Combo ignore function port list
    Ast_ਪਾਠ_ਖੰਡ* m_comboDeclsp = ਖਾਲੀ_ਸੂਚਕ;  // Combo signal declaration list
    Ast_ਪਾਠ_ਖੰਡ* m_seqDeclsp = ਖਾਲੀ_ਸੂਚਕ;  // Sequential signal declaration list
    Ast_ਪਾਠ_ਖੰਡ* m_tmpDeclsp = ਖਾਲੀ_ਸੂਚਕ;  // Temporary signal declaration list
    Ast_ਪਾਠ_ਖੰਡ* m_inoutDeclsp = ਖਾਲੀ_ਸੂਚਕ;  // Lowered inout signals and tristate drivers
    Ast_ਪਾਠ_ਖੰਡ* m_hashValuep = ਖਾਲੀ_ਸੂਚਕ;  // CPP hash value
    Ast_ਪਾਠ_ਖੰਡ* m_comboParamsp = ਖਾਲੀ_ਸੂਚਕ;  // Combo function parameter list
    Ast_ਪਾਠ_ਖੰਡ* m_clkSensp = ਖਾਲੀ_ਸੂਚਕ;  // Clock sensitivity list
    Ast_ਪਾਠ_ਖੰਡ* m_comboIgnoreParamsp = ਖਾਲੀ_ਸੂਚਕ;  // Combo ignore parameter list
    Ast_ਪਾਠ_ਖੰਡ* m_seqParamsp = ਖਾਲੀ_ਸੂਚਕ;  // Sequential parameter list
    Ast_ਪਾਠ_ਖੰਡ* m_nbAssignsp = ਖਾਲੀ_ਸੂਚਕ;  // Non-blocking assignment list
    Ast_ਪਾਠ_ਖੰਡ* m_seqAssignsp = ਖਾਲੀ_ਸੂਚਕ;  // Sequential assignment list
    Ast_ਪਾਠ_ਖੰਡ* m_comboAssignsp = ਖਾਲੀ_ਸੂਚਕ;  // Combo assignment list
    // C text blocks
    Ast_ਪਾਠ_ਖੰਡ* m_cHashValuep = ਖਾਲੀ_ਸੂਚਕ;  // CPP hash value
    Ast_ਪਾਠ_ਖੰਡ* m_cComboParamsp = ਖਾਲੀ_ਸੂਚਕ;  // Combo function parameter list
    Ast_ਪਾਠ_ਖੰਡ* m_cComboInsp = ਖਾਲੀ_ਸੂਚਕ;  // Combo input copy list
    Ast_ਪਾਠ_ਖੰਡ* m_cComboOutsp = ਖਾਲੀ_ਸੂਚਕ;  // Combo output copy list
    Ast_ਪਾਠ_ਖੰਡ* m_cSeqParamsp = ਖਾਲੀ_ਸੂਚਕ;  // Sequential parameter list
    Ast_ਪਾਠ_ਖੰਡ* m_cSeqClksp = ਖਾਲੀ_ਸੂਚਕ;  // Sequential clock copy list
    Ast_ਪਾਠ_ਖੰਡ* m_cSeqOutsp = ਖਾਲੀ_ਸੂਚਕ;  // Sequential output copy list
    Ast_ਪਾਠ_ਖੰਡ* m_cIgnoreParamsp = ਖਾਲੀ_ਸੂਚਕ;  // Combo ignore parameter list
    ਅਚਲ string m_libName;
    ਅਚਲ string m_ਸਿਖਰ_Name;
    ਤਾਰਕਿਕ m_ਮਿਲਿਆ_ਸਿਖਰ = ਝੂਠ;  // Have seen the top module
    ਤਾਰਕਿਕ m_hasClk = ਝੂਠ;  // True if the top module has sequential logic
    std::map<ਪੂਰਨ, InoutPorts> m_inouts;  // Lowered ports grouped by original inout

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstNetlist* nodep) override {
        m_vfilep
            = ਨਵਾਂ AstVFile{nodep->fileline(), v3Global.opt.makeDir() + "/" + m_libName + ".sv"};
        nodep->ਜੋੜੋ_Filesp(m_vfilep);
        m_cfilep
            = ਨਵਾਂ AstCFile{nodep->fileline(), v3Global.opt.makeDir() + "/" + m_libName + ".cpp"};
        nodep->ਜੋੜੋ_Filesp(m_cfilep);
        iterateChildren(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Module* nodep) override {
        ਜੇ (!nodep->is_ਸਿਖਰ()) {
            ਵਾਪਸ;
        } ਨਹੀਂਤਾਂ {
            UASSERT_OBJ(!m_ਮਿਲਿਆ_ਸਿਖਰ, nodep, "Multiple root modules");
        }
        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
        // Need to know the existence of clk before createSvFile()
        m_hasClk = checkIfClockExists(nodep);
        createSvFile(fl, nodep);
        createCppFile(fl);

        iterateChildren(nodep);

        restoreInouts();

        // cppcheck-suppress unreadVariable
        ਅਚਲ V3Hash hash = V3Hasher::uncachedHash(m_cfilep);
        m_hashValuep->ਜੋੜੋ(cvt_ਮੰਜ਼ਿਲ_Str(hash.ਮੁੱਲ()) + ";\n");
        m_cHashValuep->ਜੋੜੋ(cvt_ਮੰਜ਼ਿਲ_Str(hash.ਮੁੱਲ()) + "U;\n");
        m_ਮਿਲਿਆ_ਸਿਖਰ = ਸੱਚ;
    }

    ਖਾਲੀ ਜੋੜੋ_Comment(Ast_ਪਾਠ_ਖੰਡ* txtp, File_ਸਤਰ* fl, ਅਚਲ string& comment) {
        txtp->ਜੋੜੋ(ਨਵਾਂ AstComment{fl, comment});
    }

    ਖਾਲੀ configSection(Ast_ਗੰਢ_Module* /*modp*/, Ast_ਪਾਠ_ਖੰਡ* txtp, File_ਸਤਰ* /*fl*/) {
        txtp->ਜੋੜੋ("\n`ifdef VERILATOR\n");
        txtp->ਜੋੜੋ("`verilator_config\n");

        txtp->ਜੋੜੋ("verilator_lib -module \"" + m_libName + "\"\n");

        txtp->ਜੋੜੋ("profile_data -hier-dpi \"" + m_libName + "_protectlib_combo_update\" -cost 64'd"
                  + std::ਮੰਜ਼ਿਲ_string(v3Global.current_Hier_ਖੰਡ_ਲਾਗਤ()) + "\n");
        txtp->ਜੋੜੋ("profile_data -hier-dpi \"" + m_libName + "_protectlib_seq_update\" -cost 64'd"
                  + std::ਮੰਜ਼ਿਲ_string(v3Global.current_Hier_ਖੰਡ_ਲਾਗਤ()) + "\n");

        // Mark remaining NBA protectlib wrapper DPIs as non-hazardous by deliberately forwarding
        // them with non-zero cost.
        // Also, specify hierarchical workers for those tasks for scheduling.
        txtp->ਜੋੜੋ("profile_data -hier-dpi \"" + m_libName
                  + "_protectlib_combo_ignore\" -cost 64'd1\n");

        txtp->ਜੋੜੋ("hier_workers -hier-dpi \"" + m_libName
                  + "_protectlib_combo_update\" -workers 16'd"
                  + std::ਮੰਜ਼ਿਲ_string(V3Control::getHierWorkers(m_libName)) + "\n");
        txtp->ਜੋੜੋ("hier_workers -hier-dpi \"" + m_libName
                  + "_protectlib_seq_update\" -workers 16'd"
                  + std::ਮੰਜ਼ਿਲ_string(V3Control::getHierWorkers(m_libName)) + "\n");
        // No workers for combo_ignore
        txtp->ਜੋੜੋ("`verilog\n");
        txtp->ਜੋੜੋ("`endif\n");
    }

    ਖਾਲੀ hashComment(Ast_ਪਾਠ_ਖੰਡ* txtp, File_ਸਤਰ* fl) {
        ਜੋੜੋ_Comment(txtp, fl, "Checks to make sure the .sv wrapper and library agree");
    }

    ਖਾਲੀ initialComment(Ast_ਪਾਠ_ਖੰਡ* txtp, File_ਸਤਰ* fl) {
        ਜੋੜੋ_Comment(txtp, fl, "Creates an instance of the library module at initial-time");
        ਜੋੜੋ_Comment(txtp, fl, "(one for each instance in the user's design)");
    }

    ਖਾਲੀ comboComment(Ast_ਪਾਠ_ਖੰਡ* txtp, File_ਸਤਰ* fl) {
        ਜੋੜੋ_Comment(txtp, fl, "Updates all non-clock inputs and retrieves the results");
    }

    ਖਾਲੀ seqComment(Ast_ਪਾਠ_ਖੰਡ* txtp, File_ਸਤਰ* fl) {
        ਜੋੜੋ_Comment(txtp, fl, "Updates all clocks and retrieves the results");
    }

    ਖਾਲੀ comboIgnoreComment(Ast_ਪਾਠ_ਖੰਡ* txtp, File_ਸਤਰ* fl) {
        ਜੋੜੋ_Comment(txtp, fl, "Need to convince some simulators that the input to the module");
        ਜੋੜੋ_Comment(txtp, fl, "must be evaluated before evaluating the clock edge");
    }

    ਖਾਲੀ finalComment(Ast_ਪਾਠ_ਖੰਡ* txtp, File_ਸਤਰ* fl) {
        ਜੋੜੋ_Comment(txtp, fl, "Evaluates the library module's final process");
    }

    ਖਾਲੀ createSvFile(File_ਸਤਰ* fl, Ast_ਗੰਢ_Module* modp) {
        // Comments
        Ast_ਪਾਠ_ਖੰਡ* ਅਚਲ txtp = ਨਵਾਂ Ast_ਪਾਠ_ਖੰਡ{fl};
        ਜੋੜੋ_Comment(txtp, fl, "Wrapper module for DPI protected library");
        ਜੋੜੋ_Comment(txtp, fl,
                   "This module requires lib" + m_libName + ".a or lib" + m_libName
                       + ".so to work");
        ਜੋੜੋ_Comment(txtp, fl,
                   "See instructions in your simulator for how"
                   " to use DPI libraries\n");

        // Module declaration
        // Wrapper ranges preserve the original RTL, including intentional ascending ranges.
        txtp->ਜੋੜੋ("/*verilator lint_save*/\n");
        txtp->ਜੋੜੋ("/*verilator lint_off ASCRANGE*/\n");
        m_modPortsp = ਨਵਾਂ Ast_ਪਾਠ_ਖੰਡ{fl, "module " + m_libName + " (\n", ", ", ");\n\n"};
        txtp->ਜੋੜੋ(m_modPortsp);

        // Timescale
        ਜੇ (v3Global.opt.hier_ਬੱਚਾ() && v3Global.rootp()->timescaleSpecified()) {
            // Emit timescale for hierarchical Verilation if input HDL specifies timespec
            txtp->ਜੋੜੋ("timeunit "s + modp->timeunit().ascii() + ";\n");
            txtp->ਜੋੜੋ("timeprecision "s + +v3Global.rootp()->timeprecision().ascii() + ";\n");
        } ਨਹੀਂਤਾਂ {
            ਜੋੜੋ_Comment(txtp, fl,
                       "Precision of submodule"
                       " (commented out to avoid requiring timescale on all modules)");
            ਜੋੜੋ_Comment(txtp, fl, "timeunit "s + v3Global.rootp()->timeunit().ascii() + ";");
            ਜੋੜੋ_Comment(txtp, fl,
                       "timeprecision "s + v3Global.rootp()->timeprecision().ascii() + ";\n");
        }

        // DPI declarations
        hashComment(txtp, fl);
        txtp->ਜੋੜੋ("import \"DPI-C\" function void " + m_libName
                  + "_protectlib_check_hash(int protectlib_hash__V);\n\n");
        initialComment(txtp, fl);
        txtp->ਜੋੜੋ("import \"DPI-C\" function chandle " + m_libName
                  + "_protectlib_create(string scope__V);\n\n");
        comboComment(txtp, fl);
        m_comboPortsp = ਨਵਾਂ Ast_ਪਾਠ_ਖੰਡ{fl,  //
                                         "import \"DPI-C\" function longint " + m_libName
                                             + "_protectlib_combo_update(\n",  //
                                         ", ",  //
                                         ");\n\n"};
        m_comboPortsp->ਜੋੜੋ("chandle handle__V\n");
        txtp->ਜੋੜੋ(m_comboPortsp);
        seqComment(txtp, fl);
        ਜੇ (m_hasClk) {
            m_seqPortsp = ਨਵਾਂ Ast_ਪਾਠ_ਖੰਡ{fl,  //
                                           "import \"DPI-C\" function longint " + m_libName
                                               + "_protectlib_seq_update(\n",  //
                                           ", ",  //
                                           ");\n\n"};
            m_seqPortsp->ਜੋੜੋ("chandle handle__V\n");
            txtp->ਜੋੜੋ(m_seqPortsp);
        }
        comboIgnoreComment(txtp, fl);
        m_comboIgnorePortsp = ਨਵਾਂ Ast_ਪਾਠ_ਖੰਡ{fl,  //
                                               "import \"DPI-C\" function void " + m_libName
                                                   + "_protectlib_combo_ignore(\n",  //
                                               ", ",  //
                                               ");\n\n"};
        m_comboIgnorePortsp->ਜੋੜੋ("chandle handle__V\n");
        txtp->ਜੋੜੋ(m_comboIgnorePortsp);

        finalComment(txtp, fl);
        txtp->ਜੋੜੋ("import \"DPI-C\" function void " + m_libName
                  + "_protectlib_final(chandle handle__V);\n\n");

        // Local variables
        // Avoid tracing handle, as it is not a stable value, so breaks wavediff
        // Likewise other internals aren't interesting to the user
        txtp->ਜੋੜੋ("// verilator tracing_off\n");

        txtp->ਜੋੜੋ("chandle handle__V;\n");
        txtp->ਜੋੜੋ("time last_combo_seqnum__V;\n");
        ਜੇ (m_hasClk) txtp->ਜੋੜੋ("time last_seq_seqnum__V;\n");
        txtp->ਜੋੜੋ("\n");

        m_comboDeclsp = ਨਵਾਂ Ast_ਪਾਠ_ਖੰਡ{fl};
        txtp->ਜੋੜੋ(m_comboDeclsp);
        m_seqDeclsp = ਨਵਾਂ Ast_ਪਾਠ_ਖੰਡ{fl};
        txtp->ਜੋੜੋ(m_seqDeclsp);
        m_tmpDeclsp = ਨਵਾਂ Ast_ਪਾਠ_ਖੰਡ{fl};
        txtp->ਜੋੜੋ(m_tmpDeclsp);
        m_inoutDeclsp = ਨਵਾਂ Ast_ਪਾਠ_ਖੰਡ{fl};
        txtp->ਜੋੜੋ(m_inoutDeclsp);

        // CPP hash value
        ਜੋੜੋ_Comment(txtp, fl, "Hash value to make sure this file and the corresponding");
        ਜੋੜੋ_Comment(txtp, fl, "library agree");
        m_hashValuep = ਨਵਾਂ Ast_ਪਾਠ_ਖੰਡ{
            fl, "localparam int protectlib_hash__V /*verilator public_flat_rd*/ = 32'd", "", "\n"};
        txtp->ਜੋੜੋ(m_hashValuep);

        // Initial
        txtp->ਜੋੜੋ("initial begin\n");
        txtp->ਜੋੜੋ(m_libName + "_protectlib_check_hash(protectlib_hash__V);\n");
        txtp->ਜੋੜੋ("if (handle__V == null) handle__V = " + m_libName
                  + "_protectlib_create"
                    "($sformatf(\"%m\"));\n");
        txtp->ਜੋੜੋ("end\n\n");

        // Combinatorial process
        ਜੋੜੋ_Comment(txtp, fl, "Combinatorially evaluate changes to inputs");
        txtp->ਜੋੜੋ("always_comb begin\n");
        m_comboParamsp = ਨਵਾਂ Ast_ਪਾਠ_ਖੰਡ{fl,  //
                                          "last_combo_seqnum__V = " + m_libName
                                              + "_protectlib_combo_update(\n",  //
                                          ",\n",  //
                                          "\n);\n"};
        m_comboParamsp->ਜੋੜੋ("handle__V");
        txtp->ਜੋੜੋ(m_comboParamsp);
        txtp->ਜੋੜੋ("end\n\n");

        // Sequential process
        ਜੇ (m_hasClk) {
            ਜੋੜੋ_Comment(txtp, fl, "Evaluate clock edges");
            m_clkSensp = ਨਵਾਂ Ast_ਪਾਠ_ਖੰਡ{fl, "always @(", " or ", ")"};
            txtp->ਜੋੜੋ(m_clkSensp);
            txtp->ਜੋੜੋ(" begin\n");
            m_comboIgnoreParamsp
                = ਨਵਾਂ Ast_ਪਾਠ_ਖੰਡ{fl,  //
                                   m_libName + "_protectlib_combo_ignore(\n", ",\n", "\n);\n"};
            m_comboIgnoreParamsp->ਜੋੜੋ("handle__V");
            txtp->ਜੋੜੋ(m_comboIgnoreParamsp);
            m_seqParamsp = ਨਵਾਂ Ast_ਪਾਠ_ਖੰਡ{fl,  //
                                            "last_seq_seqnum__V <= " + m_libName
                                                + "_protectlib_seq_update(\n",  //
                                            ",\n",  //
                                            "\n);\n"};
            m_seqParamsp->ਜੋੜੋ("handle__V");
            txtp->ਜੋੜੋ(m_seqParamsp);
            m_nbAssignsp = ਨਵਾਂ Ast_ਪਾਠ_ਖੰਡ{fl};
            txtp->ਜੋੜੋ(m_nbAssignsp);
            txtp->ਜੋੜੋ("end\n\n");
        }

        // Select between combinatorial and sequential results
        ਜੋੜੋ_Comment(txtp, fl, "Select between combinatorial and sequential results");
        txtp->ਜੋੜੋ("always_comb begin\n");
        ਜੇ (m_hasClk) {
            txtp->ਜੋੜੋ("if (last_seq_seqnum__V > last_combo_seqnum__V) begin\n");
            m_seqAssignsp = ਨਵਾਂ Ast_ਪਾਠ_ਖੰਡ{fl};
            txtp->ਜੋੜੋ(m_seqAssignsp);
            txtp->ਜੋੜੋ("end else begin\n");
            m_comboAssignsp = ਨਵਾਂ Ast_ਪਾਠ_ਖੰਡ{fl};
            txtp->ਜੋੜੋ(m_comboAssignsp);
            txtp->ਜੋੜੋ("end\n");
        } ਨਹੀਂਤਾਂ {
            m_comboAssignsp = ਨਵਾਂ Ast_ਪਾਠ_ਖੰਡ{fl};
            txtp->ਜੋੜੋ(m_comboAssignsp);
        }
        txtp->ਜੋੜੋ("end\n\n");

        // Final
        txtp->ਜੋੜੋ("final " + m_libName + "_protectlib_final(handle__V);\n\n");

        txtp->ਜੋੜੋ("endmodule\n");
        txtp->ਜੋੜੋ("/*verilator lint_restore*/\n");

        configSection(modp, txtp, fl);

        m_vfilep->tblockp(txtp);
    }

    ਖਾਲੀ castPtr(File_ਸਤਰ* /*fl*/, Ast_ਪਾਠ_ਖੰਡ* txtp) {
        txtp->ਜੋੜੋ(m_ਸਿਖਰ_Name
                  + "_container* const handlep__V = "  // LCOV_EXCL_LINE  // lcov bug
                    "static_cast<"
                  + m_ਸਿਖਰ_Name + "_container*>(vhandlep__V);\n");
    }

    ਖਾਲੀ createCppFile(File_ਸਤਰ* fl) {
        // Comments
        Ast_ਪਾਠ_ਖੰਡ* ਅਚਲ txtp = ਨਵਾਂ Ast_ਪਾਠ_ਖੰਡ{fl};
        ਜੋੜੋ_Comment(txtp, fl, "Wrapper functions for DPI protected library\n");

        // Includes
        txtp->ਜੋੜੋ("#include \"" + m_ਸਿਖਰ_Name + ".h\"\n");
        txtp->ਜੋੜੋ("#include \"verilated_dpi.h\"\n\n");
        txtp->ਜੋੜੋ("#include <cstdio>\n");
        txtp->ਜੋੜੋ("#include <cstdlib>\n\n");

        // Verilated module plus sequence number
        ਜੋੜੋ_Comment(txtp, fl, "Container class to house verilated object and sequence number");
        txtp->ਜੋੜੋ("class " + m_ਸਿਖਰ_Name + "_container: public " + m_ਸਿਖਰ_Name + " {\n");
        txtp->ਜੋੜੋ("public:\n");
        txtp->ਜੋੜੋ("long long m_seqnum;\n");
        txtp->ਜੋੜੋ(m_ਸਿਖਰ_Name + "_container(const char* scopep__V):\n");
        txtp->ਜੋੜੋ(m_ਸਿਖਰ_Name + "(scopep__V) {}\n");
        txtp->ਜੋੜੋ("};\n\n");

        // Extern C
        txtp->ਜੋੜੋ("extern \"C\" {\n\n");

        // Hash check
        hashComment(txtp, fl);
        txtp->ਜੋੜੋ("void " + m_libName
                  + "_protectlib_check_hash"
                    "(int protectlib_hash__V) {\n");
        m_cHashValuep = ਨਵਾਂ Ast_ਪਾਠ_ਖੰਡ{fl, "const int expected_hash__V = ", "", ""};
        txtp->ਜੋੜੋ(m_cHashValuep);
        txtp->ਜੋੜੋ(/**/ "if (protectlib_hash__V != expected_hash__V) {\n");
        txtp->ਜੋੜੋ(/****/ "fprintf(stderr, \"%%Error: cannot use " + m_libName
                  + " library, "
                    "Verliog (%u) and library (%u) hash values do not "
                    "agree\\n\", protectlib_hash__V, expected_hash__V);\n");
        txtp->ਜੋੜੋ(/****/ "std::exit(EXIT_FAILURE);\n");
        txtp->ਜੋੜੋ(/**/ "}\n");
        txtp->ਜੋੜੋ("}\n\n");

        // Initial
        initialComment(txtp, fl);
        txtp->ਜੋੜੋ("void* " + m_libName + "_protectlib_create(const char* scopep__V) {\n");
        txtp->ਜੋੜੋ(/**/ m_ਸਿਖਰ_Name + "_container* const handlep__V = new " + m_ਸਿਖਰ_Name
                  + "_container{scopep__V};\n");
        txtp->ਜੋੜੋ(/**/ "return handlep__V;\n");
        txtp->ਜੋੜੋ("}\n\n");

        // Updates
        comboComment(txtp, fl);
        m_cComboParamsp = ਨਵਾਂ Ast_ਪਾਠ_ਖੰਡ{
            fl, "long long " + m_libName + "_protectlib_combo_update(\n", ",\n", "\n) {\n"};
        m_cComboParamsp->ਜੋੜੋ("void* vhandlep__V");
        txtp->ਜੋੜੋ(m_cComboParamsp);
        m_cComboInsp = ਨਵਾਂ Ast_ਪਾਠ_ਖੰਡ{fl};
        castPtr(fl, m_cComboInsp);
        txtp->ਜੋੜੋ(m_cComboInsp);
        txtp->ਜੋੜੋ("handlep__V->eval();\n");
        m_cComboOutsp = ਨਵਾਂ Ast_ਪਾਠ_ਖੰਡ{fl};
        txtp->ਜੋੜੋ(m_cComboOutsp);
        txtp->ਜੋੜੋ("return handlep__V->m_seqnum++;\n");
        txtp->ਜੋੜੋ("}\n\n");

        ਜੇ (m_hasClk) {
            seqComment(txtp, fl);
            m_cSeqParamsp = ਨਵਾਂ Ast_ਪਾਠ_ਖੰਡ{
                fl, "long long " + m_libName + "_protectlib_seq_update(\n", ",\n", "\n) {\n"};
            m_cSeqParamsp->ਜੋੜੋ("void* vhandlep__V");
            txtp->ਜੋੜੋ(m_cSeqParamsp);
            m_cSeqClksp = ਨਵਾਂ Ast_ਪਾਠ_ਖੰਡ{fl};
            castPtr(fl, m_cSeqClksp);
            txtp->ਜੋੜੋ(m_cSeqClksp);
            txtp->ਜੋੜੋ("handlep__V->eval();\n");
            m_cSeqOutsp = ਨਵਾਂ Ast_ਪਾਠ_ਖੰਡ{fl};
            txtp->ਜੋੜੋ(m_cSeqOutsp);
            txtp->ਜੋੜੋ("return handlep__V->m_seqnum++;\n");
            txtp->ਜੋੜੋ("}\n\n");
        }

        comboIgnoreComment(txtp, fl);
        m_cIgnoreParamsp = ਨਵਾਂ Ast_ਪਾਠ_ਖੰਡ{
            fl, "void " + m_libName + "_protectlib_combo_ignore(\n", ",\n", "\n)\n{ }\n\n"};
        m_cIgnoreParamsp->ਜੋੜੋ("void* vhandlep__V");
        txtp->ਜੋੜੋ(m_cIgnoreParamsp);

        // Final
        finalComment(txtp, fl);
        txtp->ਜੋੜੋ("void " + m_libName + "_protectlib_final(void* vhandlep__V) {\n");
        castPtr(fl, txtp);
        txtp->ਜੋੜੋ(/**/ "handlep__V->final();\n");
        txtp->ਜੋੜੋ(/**/ "delete handlep__V;\n");
        txtp->ਜੋੜੋ("}\n\n");

        txtp->ਜੋੜੋ("}\n");
        m_cfilep->tblockp(txtp);
    }

    ਖਾਲੀ ਦੇਖੋ(AstVar* nodep) override {
        ਜੇ (!nodep->isIO()) ਵਾਪਸ;
        ਜੇ (restoreInout(nodep)) {
            InoutPorts& ports = m_inouts[nodep->libInoutId()];
            ਜੇ (nodep->is_ਨਿਵੇਸ਼()) {
                ports.inp = nodep;
            } ਨਹੀਂਤਾਂ ਜੇ (nodep->libInoutEnable()) {
                ports.enp = nodep;
            } ਨਹੀਂਤਾਂ {
                ports.outp = nodep;
            }
        }
        ਜੇ (nodep->direction() == VDirection::ਨਿਵੇਸ਼) {
            ਜੇ (nodep->isPrimaryClock()) {
                UASSERT_OBJ(m_hasClk, nodep, "checkIfClockExists() didn't find this clock");
                handleClock(nodep);
            } ਨਹੀਂਤਾਂ {
                handle_ਅੰਕੜੇ_ਨਿਵੇਸ਼(nodep);
            }
        } ਨਹੀਂਤਾਂ {
            // Ref ports were rejected during linking, and tristate lowering split all inouts.
            UASSERT_OBJ(nodep->direction() == VDirection::OUTPUT, nodep,
                        "Unexpected library port direction");
            handleOutput(nodep);
        }
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ*) override {}

    string c_ਨਿਵੇਸ਼_Connection(AstVar* varp) {
        ਵਾਪਸ V3Task::ਸੌਂਪੋ_Dpi_ਮੰਜ਼ਿਲ_ਅੰਦਰੂਨੀ("handlep__V->" + varp->name(), varp);
    }

    // The wrapper's module ports are connected by name, so they must use the user's
    // names rather than the hashed form of long names used internally
    ਸਥਿਰ string sv_ਪੋਰਟ_Name(ਅਚਲ AstVar* varp) {
        ਵਾਪਸ Ast_ਗੰਢ::prettyName(VName::dehash(varp->name()));
    }

    ਸਥਿਰ AstVar* new_Sv_ਪੋਰਟ_Var(AstVar* varp) {
        AstVar* ਅਚਲ newVarp = varp->clone_ਰੁੱਖ(ਝੂਠ);
        newVarp->name(VName::dehash(varp->name()));
        ਵਾਪਸ newVarp;
    }

    ਖਾਲੀ handleClock(AstVar* varp) {
        handle_ਨਿਵੇਸ਼(varp);
        m_seqPortsp->ਜੋੜੋ(varp->clone_ਰੁੱਖ(ਝੂਠ));
        ਜੇ (m_hasClk) {
            ਅਚਲ std::string pname = sv_ਪੋਰਟ_Name(varp);
            m_seqParamsp->ਜੋੜੋ(pname);
            m_clkSensp->ਜੋੜੋ(pname);
        }
        m_cSeqParamsp->ਜੋੜੋ(varp->dpiArgType(ਸੱਚ, ਝੂਠ));
        m_cSeqClksp->ਜੋੜੋ(c_ਨਿਵੇਸ਼_Connection(varp));
    }

    ਖਾਲੀ handle_ਅੰਕੜੇ_ਨਿਵੇਸ਼(AstVar* varp) {
        handle_ਨਿਵੇਸ਼(varp);
        m_comboPortsp->ਜੋੜੋ(varp->clone_ਰੁੱਖ(ਝੂਠ));
        m_comboParamsp->ਜੋੜੋ(sv_ਪੋਰਟ_Name(varp));
        m_comboIgnorePortsp->ਜੋੜੋ(varp->clone_ਰੁੱਖ(ਝੂਠ));
        ਜੇ (m_hasClk) m_comboIgnoreParamsp->ਜੋੜੋ(sv_ਪੋਰਟ_Name(varp));
        m_cComboParamsp->ਜੋੜੋ(varp->dpiArgType(ਸੱਚ, ਝੂਠ));
        m_cComboInsp->ਜੋੜੋ(c_ਨਿਵੇਸ਼_Connection(varp));
        m_cIgnoreParamsp->ਜੋੜੋ(varp->dpiArgType(ਸੱਚ, ਝੂਠ));
    }

    ਸਥਿਰ ਤਾਰਕਿਕ restoreInout(ਅਚਲ AstVar* varp) {
        ਵਾਪਸ varp->libInoutId() && !v3Global.opt.pinsInoutEnables();
    }

    ਖਾਲੀ handle_ਨਿਵੇਸ਼(AstVar* varp) {
        AstVar* ਅਚਲ portp = new_Sv_ਪੋਰਟ_Var(varp);
        ਜੇ (restoreInout(varp)) {
            portp->direction(VDirection::INOUT);
            portp->varType(VVarType::WIRE);
        }
        m_modPortsp->ਜੋੜੋ(portp);
    }

    ਖਾਲੀ restoreInouts() {
        ਲਈ (ਅਚਲ ਸਵੈਚਾਲਿਤ& ਇੰਦਰਾਜ : m_inouts) {
            ਅਚਲ InoutPorts& ports = ਇੰਦਰਾਜ.ਦੂਜਾ;
            UASSERT_OBJ(ports.inp && ports.outp && ports.enp, m_vfilep,
                        "Incomplete lowered library inout");
            ਅਚਲ string ਵਿੱਚ_2 = sv_ਪੋਰਟ_Name(ports.inp);
            ਅਚਲ string ਬਾਹਰਲਾ = ports.outp->prettyName();
            ਅਚਲ string en = ports.enp->prettyName();
            std::ਸਦਿਸ਼<std::pair<ਪੂਰਨ, ਪੂਰਨ>> dimensions;
            ਅਚਲ Ast_ਗੰਢ_DType* dtypep = ports.inp->dtypep()->skipRefp();
            ਜਦੋਂ (ਅਚਲ Ast_ਗੰਢ_Array_DType* ਅਚਲ arrayp = VN_CAST(dtypep, ਗੰਢ_Array_DType)) {
                // V3Slice currently rejects lowered unpacked inouts before this pass.
                // TODO: Support unpacked inouts once V3Slice supports their lowered form.
                UASSERT_OBJ(!VN_IS(arrayp, UnpackArrayDType), ports.inp,
                            "Unpacked library inout should have been rejected by V3Slice");
                dimensions.emplace_ਪਿੱਠ(arrayp->lo(), arrayp->hi());
                dtypep = arrayp->subDTypep()->skipRefp();
            }
            ਅਚਲ AstBasicDType* ਅਚਲ basicp = dtypep->basicp();
            UASSERT_OBJ(basicp, ports.inp, "Library inout must have a packed integral type");
            ਜੇ (basicp->isRanged()) dimensions.emplace_ਪਿੱਠ(basicp->lo(), basicp->hi());
            ਜੇ (dimensions.ਖਾਲੀ_ਥਾਵਾਂ()) {
                m_inoutDeclsp->ਜੋੜੋ("assign " + ਵਿੱਚ_2 + " = " + en + " ? " + ਬਾਹਰਲਾ + " : 1'bz;\n");
            } ਨਹੀਂਤਾਂ {
                string lhs = ਵਿੱਚ_2;
                string rhsEn = en;
                string rhs_ਬਾਹਰਲਾ = ਬਾਹਰਲਾ;
                ਲਈ (ਮਾਪ_t dim = 0; dim < dimensions.ਮਾਪ(); ++dim) {
                    ਅਚਲ string ਸੂਚਕਾਂਕ
                        = "__VlibInout" + cvt_ਮੰਜ਼ਿਲ_Str(ਇੰਦਰਾਜ.ਪਹਿਲਾ) + "_" + cvt_ਮੰਜ਼ਿਲ_Str(dim);
                    ਅਚਲ ਪੂਰਨ ਨੀਵਾਂ = dimensions[dim].ਪਹਿਲਾ;
                    ਅਚਲ ਪੂਰਨ ਉੱਚਾ = dimensions[dim].ਦੂਜਾ;
                    ਅਚਲ ਪੂਰਨ ਮਾਪ = ਉੱਚਾ - ਨੀਵਾਂ + 1;
                    m_inoutDeclsp->ਜੋੜੋ("for (genvar " + ਸੂਚਕਾਂਕ + " = 0; " + ਸੂਚਕਾਂਕ + " < "
                                       + cvt_ਮੰਜ਼ਿਲ_Str(ਮਾਪ) + "; ++" + ਸੂਚਕਾਂਕ + ") begin\n");
                    ਅਚਲ string select = "[" + cvt_ਮੰਜ਼ਿਲ_Str(ਨੀਵਾਂ) + " + " + ਸੂਚਕਾਂਕ + "]";
                    lhs += select;
                    rhsEn += select;
                    rhs_ਬਾਹਰਲਾ += select;
                }
                m_inoutDeclsp->ਜੋੜੋ("assign " + lhs + " = " + rhsEn + " ? " + rhs_ਬਾਹਰਲਾ
                                   + " : 1'bz;\n");
                ਲਈ (ਮਾਪ_t dim = 0; dim < dimensions.ਮਾਪ(); ++dim) m_inoutDeclsp->ਜੋੜੋ("end\n");
            }
        }
    }

    ਸਥਿਰ ਖਾਲੀ ਜੋੜੋ_Local_Variable(Ast_ਪਾਠ_ਖੰਡ* textp, ਅਚਲ AstVar* varp, ਅਚਲ ਅੱਖਰ* suffix) {
        AstVar* ਅਚਲ newVarp
            = ਨਵਾਂ AstVar{varp->fileline(), VVarType::VAR, varp->name() + suffix, varp->dtypep()};
        textp->ਜੋੜੋ(newVarp);
    }

    ਖਾਲੀ handleOutput(AstVar* ਅਚਲ varp) {
        ਅਚਲ std::string pname = varp->prettyName();
        // Restored inout drivers are local signals and retain their internal names.
        ਅਚਲ std::string ਪੋਰਟ_Name = restoreInout(varp) ? pname : sv_ਪੋਰਟ_Name(varp);
        ਜੇ (restoreInout(varp)) {
            AstVar* ਅਚਲ localp = varp->clone_ਰੁੱਖ(ਝੂਠ);
            localp->direction(VDirection::NONE);
            m_inoutDeclsp->ਜੋੜੋ(localp);
        } ਨਹੀਂਤਾਂ {
            m_modPortsp->ਜੋੜੋ(new_Sv_ਪੋਰਟ_Var(varp));
        }
        m_comboPortsp->ਜੋੜੋ(varp->clone_ਰੁੱਖ(ਝੂਠ));
        m_comboParamsp->ਜੋੜੋ(pname + "_combo__V");
        ਜੇ (m_hasClk) {
            m_seqPortsp->ਜੋੜੋ(varp->clone_ਰੁੱਖ(ਝੂਠ));
            m_seqParamsp->ਜੋੜੋ(pname + "_tmp__V");
        }

        ਜੋੜੋ_Local_Variable(m_comboDeclsp, varp, "_combo__V");

        ਜੇ (m_hasClk) {
            ਜੋੜੋ_Local_Variable(m_seqDeclsp, varp, "_seq__V");
            ਜੋੜੋ_Local_Variable(m_tmpDeclsp, varp, "_tmp__V");

            m_nbAssignsp->ਜੋੜੋ(pname + "_seq__V <= " + pname + "_tmp__V;\n");
            m_seqAssignsp->ਜੋੜੋ(ਪੋਰਟ_Name + " = " + pname + "_seq__V;\n");
        }
        m_comboAssignsp->ਜੋੜੋ(ਪੋਰਟ_Name + " = " + pname + "_combo__V;\n");
        m_cComboParamsp->ਜੋੜੋ(varp->dpiArgType(ਸੱਚ, ਝੂਠ));
        m_cComboOutsp->ਜੋੜੋ(V3Task::ਸੌਂਪੋ_ਅੰਦਰੂਨੀ_ਮੰਜ਼ਿਲ_Dpi(varp, ਸੱਚ, "", "", "handlep__V->") + "\n");
        ਜੇ (m_hasClk) {
            m_cSeqParamsp->ਜੋੜੋ(varp->dpiArgType(ਸੱਚ, ਝੂਠ));
            m_cSeqOutsp->ਜੋੜੋ(V3Task::ਸੌਂਪੋ_ਅੰਦਰੂਨੀ_ਮੰਜ਼ਿਲ_Dpi(varp, ਸੱਚ, "", "", "handlep__V->")
                             + "\n");
        }
    }

    ਸਥਿਰ ਤਾਰਕਿਕ checkIfClockExists(ਅਚਲ Ast_ਗੰਢ_Module* modp) {
        ਲਈ (ਅਚਲ Ast_ਗੰਢ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ਜੇ (ਅਚਲ AstVar* ਅਚਲ varp = VN_CAST(stmtp, Var)) {
                ਜੇ (varp->direction() == VDirection::ਨਿਵੇਸ਼ && varp->isPrimaryClock()) {
                    ਵਾਪਸ ਸੱਚ;
                }
            }
        }
        ਵਾਪਸ ਝੂਠ;
    }

ਜਨਤਕ:
    ਸਪਸ਼ਟ ProtectVisitor(Ast_ਗੰਢ* nodep)
        : m_libName{v3Global.opt.libCreate()}
        , m_ਸਿਖਰ_Name{v3Global.opt.ਅਗੇਤਰ()} {
        iterate(nodep);
    }
};

//######################################################################
// ProtectLib class functions

ਖਾਲੀ V3ProtectLib::protect() {
    UINFO(2, __FUNCTION__ << ":");
    ProtectVisitor{v3Global.rootp()};
}
