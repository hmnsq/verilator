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

#ಅನ್ತರ್ಭಾವ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ಅನ್ತರ್ಭಾವ "V3ProtectLib.h"

#ಅನ್ತರ್ಭಾವ "V3Control.h"
#ಅನ್ತರ್ಭಾವ "V3Hasher.h"
#ಅನ್ತರ್ಭಾವ "V3InstrCount.h"
#ಅನ್ತರ್ಭಾವ "V3String.h"
#ಅನ್ತರ್ಭಾವ "V3Task.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// ProtectLib top-level visitor

ವರ್ಗ ProtectVisitor final : ಸಾರ್ವಜನಿಕ VNVisitor {
    // TYPES
    ರಚನೆ InoutPorts final {
        AstVar* inp = ಶೂನ್ಯಸೂಚಕ;  // Resolved input value and original public port
        AstVar* outp = ಶೂನ್ಯಸೂಚಕ;  // Library drive value
        AstVar* enp = ಶೂನ್ಯಸೂಚಕ;  // Per-bit drive enable
    };

    // MEMBERS
    AstVFile* m_vfilep = ಶೂನ್ಯಸೂಚಕ;  // DPI-enabled Verilog wrapper
    AstCFile* m_cfilep = ಶೂನ್ಯಸೂಚಕ;  // C implementation of DPI functions
    // Verilog text blocks
    Ast_ಪಠ್ಯ_ಖಂಡ* m_modPortsp = ಶೂನ್ಯಸೂಚಕ;  // Module port list
    Ast_ಪಠ್ಯ_ಖಂಡ* m_comboPortsp = ಶೂನ್ಯಸೂಚಕ;  // Combo function port list
    Ast_ಪಠ್ಯ_ಖಂಡ* m_seqPortsp = ಶೂನ್ಯಸೂಚಕ;  // Sequential function port list
    Ast_ಪಠ್ಯ_ಖಂಡ* m_comboIgnorePortsp = ಶೂನ್ಯಸೂಚಕ;  // Combo ignore function port list
    Ast_ಪಠ್ಯ_ಖಂಡ* m_comboDeclsp = ಶೂನ್ಯಸೂಚಕ;  // Combo signal declaration list
    Ast_ಪಠ್ಯ_ಖಂಡ* m_seqDeclsp = ಶೂನ್ಯಸೂಚಕ;  // Sequential signal declaration list
    Ast_ಪಠ್ಯ_ಖಂಡ* m_tmpDeclsp = ಶೂನ್ಯಸೂಚಕ;  // Temporary signal declaration list
    Ast_ಪಠ್ಯ_ಖಂಡ* m_inoutDeclsp = ಶೂನ್ಯಸೂಚಕ;  // Lowered inout signals and tristate drivers
    Ast_ಪಠ್ಯ_ಖಂಡ* m_hashValuep = ಶೂನ್ಯಸೂಚಕ;  // CPP hash value
    Ast_ಪಠ್ಯ_ಖಂಡ* m_comboParamsp = ಶೂನ್ಯಸೂಚಕ;  // Combo function parameter list
    Ast_ಪಠ್ಯ_ಖಂಡ* m_clkSensp = ಶೂನ್ಯಸೂಚಕ;  // Clock sensitivity list
    Ast_ಪಠ್ಯ_ಖಂಡ* m_comboIgnoreParamsp = ಶೂನ್ಯಸೂಚಕ;  // Combo ignore parameter list
    Ast_ಪಠ್ಯ_ಖಂಡ* m_seqParamsp = ಶೂನ್ಯಸೂಚಕ;  // Sequential parameter list
    Ast_ಪಠ್ಯ_ಖಂಡ* m_nbAssignsp = ಶೂನ್ಯಸೂಚಕ;  // Non-blocking assignment list
    Ast_ಪಠ್ಯ_ಖಂಡ* m_seqAssignsp = ಶೂನ್ಯಸೂಚಕ;  // Sequential assignment list
    Ast_ಪಠ್ಯ_ಖಂಡ* m_comboAssignsp = ಶೂನ್ಯಸೂಚಕ;  // Combo assignment list
    // C text blocks
    Ast_ಪಠ್ಯ_ಖಂಡ* m_cHashValuep = ಶೂನ್ಯಸೂಚಕ;  // CPP hash value
    Ast_ಪಠ್ಯ_ಖಂಡ* m_cComboParamsp = ಶೂನ್ಯಸೂಚಕ;  // Combo function parameter list
    Ast_ಪಠ್ಯ_ಖಂಡ* m_cComboInsp = ಶೂನ್ಯಸೂಚಕ;  // Combo input copy list
    Ast_ಪಠ್ಯ_ಖಂಡ* m_cComboOutsp = ಶೂನ್ಯಸೂಚಕ;  // Combo output copy list
    Ast_ಪಠ್ಯ_ಖಂಡ* m_cSeqParamsp = ಶೂನ್ಯಸೂಚಕ;  // Sequential parameter list
    Ast_ಪಠ್ಯ_ಖಂಡ* m_cSeqClksp = ಶೂನ್ಯಸೂಚಕ;  // Sequential clock copy list
    Ast_ಪಠ್ಯ_ಖಂಡ* m_cSeqOutsp = ಶೂನ್ಯಸೂಚಕ;  // Sequential output copy list
    Ast_ಪಠ್ಯ_ಖಂಡ* m_cIgnoreParamsp = ಶೂನ್ಯಸೂಚಕ;  // Combo ignore parameter list
    ಸ್ಥಿರಾಂಕ string m_libName;
    ಸ್ಥಿರಾಂಕ string m_ತುದಿ_Name;
    ತಾರ್ಕಿಕ m_ಸಿಕ್ಕಿತು_ತುದಿ = ಸುಳ್ಳು;  // Have seen the top module
    ತಾರ್ಕಿಕ m_hasClk = ಸುಳ್ಳು;  // True if the top module has sequential logic
    std::map<ಪೂರ್ಣಾಂಕ, InoutPorts> m_inouts;  // Lowered ports grouped by original inout

    // VISITORS
    ಶೂನ್ಯ ಭೇಟಿ(AstNetlist* nodep) override {
        m_vfilep
            = ಹೊಸ AstVFile{nodep->fileline(), v3Global.opt.makeDir() + "/" + m_libName + ".sv"};
        nodep->ಕೂಡಿಸು_Filesp(m_vfilep);
        m_cfilep
            = ಹೊಸ AstCFile{nodep->fileline(), v3Global.opt.makeDir() + "/" + m_libName + ".cpp"};
        nodep->ಕೂಡಿಸು_Filesp(m_cfilep);
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು_Module* nodep) override {
        ಆದರೆ (!nodep->is_ತುದಿ()) {
            ಹಿಂತಿರುಗಿ;
        } ಇಲ್ಲದಿದ್ದರೆ {
            UASSERT_OBJ(!m_ಸಿಕ್ಕಿತು_ತುದಿ, nodep, "Multiple root modules");
        }
        File_ಸಾಲು* ಸ್ಥಿರಾಂಕ fl = nodep->fileline();
        // Need to know the existence of clk before createSvFile()
        m_hasClk = checkIfClockExists(nodep);
        createSvFile(fl, nodep);
        createCppFile(fl);

        iterateChildren(nodep);

        restoreInouts();

        // cppcheck-suppress unreadVariable
        ಸ್ಥಿರಾಂಕ V3Hash hash = V3Hasher::uncachedHash(m_cfilep);
        m_hashValuep->ಕೂಡಿಸು(cvt_ಗಮ್ಯ_Str(hash.ಮೌಲ್ಯ()) + ";\n");
        m_cHashValuep->ಕೂಡಿಸು(cvt_ಗಮ್ಯ_Str(hash.ಮೌಲ್ಯ()) + "U;\n");
        m_ಸಿಕ್ಕಿತು_ತುದಿ = ನಿಜ;
    }

    ಶೂನ್ಯ ಕೂಡಿಸು_Comment(Ast_ಪಠ್ಯ_ಖಂಡ* txtp, File_ಸಾಲು* fl, ಸ್ಥಿರಾಂಕ string& comment) {
        txtp->ಕೂಡಿಸು(ಹೊಸ AstComment{fl, comment});
    }

    ಶೂನ್ಯ configSection(Ast_ಗಂಟು_Module* /*modp*/, Ast_ಪಠ್ಯ_ಖಂಡ* txtp, File_ಸಾಲು* /*fl*/) {
        txtp->ಕೂಡಿಸು("\n`ifdef VERILATOR\n");
        txtp->ಕೂಡಿಸು("`verilator_config\n");

        txtp->ಕೂಡಿಸು("verilator_lib -module \"" + m_libName + "\"\n");

        txtp->ಕೂಡಿಸು("profile_data -hier-dpi \"" + m_libName + "_protectlib_combo_update\" -cost 64'd"
                  + std::ಗಮ್ಯ_string(v3Global.current_Hier_ಖಂಡ_ವೆಚ್ಚ()) + "\n");
        txtp->ಕೂಡಿಸು("profile_data -hier-dpi \"" + m_libName + "_protectlib_seq_update\" -cost 64'd"
                  + std::ಗಮ್ಯ_string(v3Global.current_Hier_ಖಂಡ_ವೆಚ್ಚ()) + "\n");

        // Mark remaining NBA protectlib wrapper DPIs as non-hazardous by deliberately forwarding
        // them with non-zero cost.
        // Also, specify hierarchical workers for those tasks for scheduling.
        txtp->ಕೂಡಿಸು("profile_data -hier-dpi \"" + m_libName
                  + "_protectlib_combo_ignore\" -cost 64'd1\n");

        txtp->ಕೂಡಿಸು("hier_workers -hier-dpi \"" + m_libName
                  + "_protectlib_combo_update\" -workers 16'd"
                  + std::ಗಮ್ಯ_string(V3Control::getHierWorkers(m_libName)) + "\n");
        txtp->ಕೂಡಿಸು("hier_workers -hier-dpi \"" + m_libName
                  + "_protectlib_seq_update\" -workers 16'd"
                  + std::ಗಮ್ಯ_string(V3Control::getHierWorkers(m_libName)) + "\n");
        // No workers for combo_ignore
        txtp->ಕೂಡಿಸು("`verilog\n");
        txtp->ಕೂಡಿಸು("`endif\n");
    }

    ಶೂನ್ಯ hashComment(Ast_ಪಠ್ಯ_ಖಂಡ* txtp, File_ಸಾಲು* fl) {
        ಕೂಡಿಸು_Comment(txtp, fl, "Checks to make sure the .sv wrapper and library agree");
    }

    ಶೂನ್ಯ initialComment(Ast_ಪಠ್ಯ_ಖಂಡ* txtp, File_ಸಾಲು* fl) {
        ಕೂಡಿಸು_Comment(txtp, fl, "Creates an instance of the library module at initial-time");
        ಕೂಡಿಸು_Comment(txtp, fl, "(one for each instance in the user's design)");
    }

    ಶೂನ್ಯ comboComment(Ast_ಪಠ್ಯ_ಖಂಡ* txtp, File_ಸಾಲು* fl) {
        ಕೂಡಿಸು_Comment(txtp, fl, "Updates all non-clock inputs and retrieves the results");
    }

    ಶೂನ್ಯ seqComment(Ast_ಪಠ್ಯ_ಖಂಡ* txtp, File_ಸಾಲು* fl) {
        ಕೂಡಿಸು_Comment(txtp, fl, "Updates all clocks and retrieves the results");
    }

    ಶೂನ್ಯ comboIgnoreComment(Ast_ಪಠ್ಯ_ಖಂಡ* txtp, File_ಸಾಲು* fl) {
        ಕೂಡಿಸು_Comment(txtp, fl, "Need to convince some simulators that the input to the module");
        ಕೂಡಿಸು_Comment(txtp, fl, "must be evaluated before evaluating the clock edge");
    }

    ಶೂನ್ಯ finalComment(Ast_ಪಠ್ಯ_ಖಂಡ* txtp, File_ಸಾಲು* fl) {
        ಕೂಡಿಸು_Comment(txtp, fl, "Evaluates the library module's final process");
    }

    ಶೂನ್ಯ createSvFile(File_ಸಾಲು* fl, Ast_ಗಂಟು_Module* modp) {
        // Comments
        Ast_ಪಠ್ಯ_ಖಂಡ* ಸ್ಥಿರಾಂಕ txtp = ಹೊಸ Ast_ಪಠ್ಯ_ಖಂಡ{fl};
        ಕೂಡಿಸು_Comment(txtp, fl, "Wrapper module for DPI protected library");
        ಕೂಡಿಸು_Comment(txtp, fl,
                   "This module requires lib" + m_libName + ".a or lib" + m_libName
                       + ".so to work");
        ಕೂಡಿಸು_Comment(txtp, fl,
                   "See instructions in your simulator for how"
                   " to use DPI libraries\n");

        // Module declaration
        // Wrapper ranges preserve the original RTL, including intentional ascending ranges.
        txtp->ಕೂಡಿಸು("/*verilator lint_save*/\n");
        txtp->ಕೂಡಿಸು("/*verilator lint_off ASCRANGE*/\n");
        m_modPortsp = ಹೊಸ Ast_ಪಠ್ಯ_ಖಂಡ{fl, "module " + m_libName + " (\n", ", ", ");\n\n"};
        txtp->ಕೂಡಿಸು(m_modPortsp);

        // Timescale
        ಆದರೆ (v3Global.opt.hier_ಮಗು() && v3Global.rootp()->timescaleSpecified()) {
            // Emit timescale for hierarchical Verilation if input HDL specifies timespec
            txtp->ಕೂಡಿಸು("timeunit "s + modp->timeunit().ascii() + ";\n");
            txtp->ಕೂಡಿಸು("timeprecision "s + +v3Global.rootp()->timeprecision().ascii() + ";\n");
        } ಇಲ್ಲದಿದ್ದರೆ {
            ಕೂಡಿಸು_Comment(txtp, fl,
                       "Precision of submodule"
                       " (commented out to avoid requiring timescale on all modules)");
            ಕೂಡಿಸು_Comment(txtp, fl, "timeunit "s + v3Global.rootp()->timeunit().ascii() + ";");
            ಕೂಡಿಸು_Comment(txtp, fl,
                       "timeprecision "s + v3Global.rootp()->timeprecision().ascii() + ";\n");
        }

        // DPI declarations
        hashComment(txtp, fl);
        txtp->ಕೂಡಿಸು("import \"DPI-C\" function void " + m_libName
                  + "_protectlib_check_hash(int protectlib_hash__V);\n\n");
        initialComment(txtp, fl);
        txtp->ಕೂಡಿಸು("import \"DPI-C\" function chandle " + m_libName
                  + "_protectlib_create(string scope__V);\n\n");
        comboComment(txtp, fl);
        m_comboPortsp = ಹೊಸ Ast_ಪಠ್ಯ_ಖಂಡ{fl,  //
                                         "import \"DPI-C\" function longint " + m_libName
                                             + "_protectlib_combo_update(\n",  //
                                         ", ",  //
                                         ");\n\n"};
        m_comboPortsp->ಕೂಡಿಸು("chandle handle__V\n");
        txtp->ಕೂಡಿಸು(m_comboPortsp);
        seqComment(txtp, fl);
        ಆದರೆ (m_hasClk) {
            m_seqPortsp = ಹೊಸ Ast_ಪಠ್ಯ_ಖಂಡ{fl,  //
                                           "import \"DPI-C\" function longint " + m_libName
                                               + "_protectlib_seq_update(\n",  //
                                           ", ",  //
                                           ");\n\n"};
            m_seqPortsp->ಕೂಡಿಸು("chandle handle__V\n");
            txtp->ಕೂಡಿಸು(m_seqPortsp);
        }
        comboIgnoreComment(txtp, fl);
        m_comboIgnorePortsp = ಹೊಸ Ast_ಪಠ್ಯ_ಖಂಡ{fl,  //
                                               "import \"DPI-C\" function void " + m_libName
                                                   + "_protectlib_combo_ignore(\n",  //
                                               ", ",  //
                                               ");\n\n"};
        m_comboIgnorePortsp->ಕೂಡಿಸು("chandle handle__V\n");
        txtp->ಕೂಡಿಸು(m_comboIgnorePortsp);

        finalComment(txtp, fl);
        txtp->ಕೂಡಿಸು("import \"DPI-C\" function void " + m_libName
                  + "_protectlib_final(chandle handle__V);\n\n");

        // Local variables
        // Avoid tracing handle, as it is not a stable value, so breaks wavediff
        // Likewise other internals aren't interesting to the user
        txtp->ಕೂಡಿಸು("// verilator tracing_off\n");

        txtp->ಕೂಡಿಸು("chandle handle__V;\n");
        txtp->ಕೂಡಿಸು("time last_combo_seqnum__V;\n");
        ಆದರೆ (m_hasClk) txtp->ಕೂಡಿಸು("time last_seq_seqnum__V;\n");
        txtp->ಕೂಡಿಸು("\n");

        m_comboDeclsp = ಹೊಸ Ast_ಪಠ್ಯ_ಖಂಡ{fl};
        txtp->ಕೂಡಿಸು(m_comboDeclsp);
        m_seqDeclsp = ಹೊಸ Ast_ಪಠ್ಯ_ಖಂಡ{fl};
        txtp->ಕೂಡಿಸು(m_seqDeclsp);
        m_tmpDeclsp = ಹೊಸ Ast_ಪಠ್ಯ_ಖಂಡ{fl};
        txtp->ಕೂಡಿಸು(m_tmpDeclsp);
        m_inoutDeclsp = ಹೊಸ Ast_ಪಠ್ಯ_ಖಂಡ{fl};
        txtp->ಕೂಡಿಸು(m_inoutDeclsp);

        // CPP hash value
        ಕೂಡಿಸು_Comment(txtp, fl, "Hash value to make sure this file and the corresponding");
        ಕೂಡಿಸು_Comment(txtp, fl, "library agree");
        m_hashValuep = ಹೊಸ Ast_ಪಠ್ಯ_ಖಂಡ{
            fl, "localparam int protectlib_hash__V /*verilator public_flat_rd*/ = 32'd", "", "\n"};
        txtp->ಕೂಡಿಸು(m_hashValuep);

        // Initial
        txtp->ಕೂಡಿಸು("initial begin\n");
        txtp->ಕೂಡಿಸು(m_libName + "_protectlib_check_hash(protectlib_hash__V);\n");
        txtp->ಕೂಡಿಸು("if (handle__V == null) handle__V = " + m_libName
                  + "_protectlib_create"
                    "($sformatf(\"%m\"));\n");
        txtp->ಕೂಡಿಸು("end\n\n");

        // Combinatorial process
        ಕೂಡಿಸು_Comment(txtp, fl, "Combinatorially evaluate changes to inputs");
        txtp->ಕೂಡಿಸು("always_comb begin\n");
        m_comboParamsp = ಹೊಸ Ast_ಪಠ್ಯ_ಖಂಡ{fl,  //
                                          "last_combo_seqnum__V = " + m_libName
                                              + "_protectlib_combo_update(\n",  //
                                          ",\n",  //
                                          "\n);\n"};
        m_comboParamsp->ಕೂಡಿಸು("handle__V");
        txtp->ಕೂಡಿಸು(m_comboParamsp);
        txtp->ಕೂಡಿಸು("end\n\n");

        // Sequential process
        ಆದರೆ (m_hasClk) {
            ಕೂಡಿಸು_Comment(txtp, fl, "Evaluate clock edges");
            m_clkSensp = ಹೊಸ Ast_ಪಠ್ಯ_ಖಂಡ{fl, "always @(", " or ", ")"};
            txtp->ಕೂಡಿಸು(m_clkSensp);
            txtp->ಕೂಡಿಸು(" begin\n");
            m_comboIgnoreParamsp
                = ಹೊಸ Ast_ಪಠ್ಯ_ಖಂಡ{fl,  //
                                   m_libName + "_protectlib_combo_ignore(\n", ",\n", "\n);\n"};
            m_comboIgnoreParamsp->ಕೂಡಿಸು("handle__V");
            txtp->ಕೂಡಿಸು(m_comboIgnoreParamsp);
            m_seqParamsp = ಹೊಸ Ast_ಪಠ್ಯ_ಖಂಡ{fl,  //
                                            "last_seq_seqnum__V <= " + m_libName
                                                + "_protectlib_seq_update(\n",  //
                                            ",\n",  //
                                            "\n);\n"};
            m_seqParamsp->ಕೂಡಿಸು("handle__V");
            txtp->ಕೂಡಿಸು(m_seqParamsp);
            m_nbAssignsp = ಹೊಸ Ast_ಪಠ್ಯ_ಖಂಡ{fl};
            txtp->ಕೂಡಿಸು(m_nbAssignsp);
            txtp->ಕೂಡಿಸು("end\n\n");
        }

        // Select between combinatorial and sequential results
        ಕೂಡಿಸು_Comment(txtp, fl, "Select between combinatorial and sequential results");
        txtp->ಕೂಡಿಸು("always_comb begin\n");
        ಆದರೆ (m_hasClk) {
            txtp->ಕೂಡಿಸು("if (last_seq_seqnum__V > last_combo_seqnum__V) begin\n");
            m_seqAssignsp = ಹೊಸ Ast_ಪಠ್ಯ_ಖಂಡ{fl};
            txtp->ಕೂಡಿಸು(m_seqAssignsp);
            txtp->ಕೂಡಿಸು("end else begin\n");
            m_comboAssignsp = ಹೊಸ Ast_ಪಠ್ಯ_ಖಂಡ{fl};
            txtp->ಕೂಡಿಸು(m_comboAssignsp);
            txtp->ಕೂಡಿಸು("end\n");
        } ಇಲ್ಲದಿದ್ದರೆ {
            m_comboAssignsp = ಹೊಸ Ast_ಪಠ್ಯ_ಖಂಡ{fl};
            txtp->ಕೂಡಿಸು(m_comboAssignsp);
        }
        txtp->ಕೂಡಿಸು("end\n\n");

        // Final
        txtp->ಕೂಡಿಸು("final " + m_libName + "_protectlib_final(handle__V);\n\n");

        txtp->ಕೂಡಿಸು("endmodule\n");
        txtp->ಕೂಡಿಸು("/*verilator lint_restore*/\n");

        configSection(modp, txtp, fl);

        m_vfilep->tblockp(txtp);
    }

    ಶೂನ್ಯ castPtr(File_ಸಾಲು* /*fl*/, Ast_ಪಠ್ಯ_ಖಂಡ* txtp) {
        txtp->ಕೂಡಿಸು(m_ತುದಿ_Name
                  + "_container* const handlep__V = "  // LCOV_EXCL_LINE  // lcov bug
                    "static_cast<"
                  + m_ತುದಿ_Name + "_container*>(vhandlep__V);\n");
    }

    ಶೂನ್ಯ createCppFile(File_ಸಾಲು* fl) {
        // Comments
        Ast_ಪಠ್ಯ_ಖಂಡ* ಸ್ಥಿರಾಂಕ txtp = ಹೊಸ Ast_ಪಠ್ಯ_ಖಂಡ{fl};
        ಕೂಡಿಸು_Comment(txtp, fl, "Wrapper functions for DPI protected library\n");

        // Includes
        txtp->ಕೂಡಿಸು("#include \"" + m_ತುದಿ_Name + ".h\"\n");
        txtp->ಕೂಡಿಸು("#include \"verilated_dpi.h\"\n\n");
        txtp->ಕೂಡಿಸು("#include <cstdio>\n");
        txtp->ಕೂಡಿಸು("#include <cstdlib>\n\n");

        // Verilated module plus sequence number
        ಕೂಡಿಸು_Comment(txtp, fl, "Container class to house verilated object and sequence number");
        txtp->ಕೂಡಿಸು("class " + m_ತುದಿ_Name + "_container: public " + m_ತುದಿ_Name + " {\n");
        txtp->ಕೂಡಿಸು("public:\n");
        txtp->ಕೂಡಿಸು("long long m_seqnum;\n");
        txtp->ಕೂಡಿಸು(m_ತುದಿ_Name + "_container(const char* scopep__V):\n");
        txtp->ಕೂಡಿಸು(m_ತುದಿ_Name + "(scopep__V) {}\n");
        txtp->ಕೂಡಿಸು("};\n\n");

        // Extern C
        txtp->ಕೂಡಿಸು("extern \"C\" {\n\n");

        // Hash check
        hashComment(txtp, fl);
        txtp->ಕೂಡಿಸು("void " + m_libName
                  + "_protectlib_check_hash"
                    "(int protectlib_hash__V) {\n");
        m_cHashValuep = ಹೊಸ Ast_ಪಠ್ಯ_ಖಂಡ{fl, "const int expected_hash__V = ", "", ""};
        txtp->ಕೂಡಿಸು(m_cHashValuep);
        txtp->ಕೂಡಿಸು(/**/ "if (protectlib_hash__V != expected_hash__V) {\n");
        txtp->ಕೂಡಿಸು(/****/ "fprintf(stderr, \"%%Error: cannot use " + m_libName
                  + " library, "
                    "Verliog (%u) and library (%u) hash values do not "
                    "agree\\n\", protectlib_hash__V, expected_hash__V);\n");
        txtp->ಕೂಡಿಸು(/****/ "std::exit(EXIT_FAILURE);\n");
        txtp->ಕೂಡಿಸು(/**/ "}\n");
        txtp->ಕೂಡಿಸು("}\n\n");

        // Initial
        initialComment(txtp, fl);
        txtp->ಕೂಡಿಸು("void* " + m_libName + "_protectlib_create(const char* scopep__V) {\n");
        txtp->ಕೂಡಿಸು(/**/ m_ತುದಿ_Name + "_container* const handlep__V = new " + m_ತುದಿ_Name
                  + "_container{scopep__V};\n");
        txtp->ಕೂಡಿಸು(/**/ "return handlep__V;\n");
        txtp->ಕೂಡಿಸು("}\n\n");

        // Updates
        comboComment(txtp, fl);
        m_cComboParamsp = ಹೊಸ Ast_ಪಠ್ಯ_ಖಂಡ{
            fl, "long long " + m_libName + "_protectlib_combo_update(\n", ",\n", "\n) {\n"};
        m_cComboParamsp->ಕೂಡಿಸು("void* vhandlep__V");
        txtp->ಕೂಡಿಸು(m_cComboParamsp);
        m_cComboInsp = ಹೊಸ Ast_ಪಠ್ಯ_ಖಂಡ{fl};
        castPtr(fl, m_cComboInsp);
        txtp->ಕೂಡಿಸು(m_cComboInsp);
        txtp->ಕೂಡಿಸು("handlep__V->eval();\n");
        m_cComboOutsp = ಹೊಸ Ast_ಪಠ್ಯ_ಖಂಡ{fl};
        txtp->ಕೂಡಿಸು(m_cComboOutsp);
        txtp->ಕೂಡಿಸು("return handlep__V->m_seqnum++;\n");
        txtp->ಕೂಡಿಸು("}\n\n");

        ಆದರೆ (m_hasClk) {
            seqComment(txtp, fl);
            m_cSeqParamsp = ಹೊಸ Ast_ಪಠ್ಯ_ಖಂಡ{
                fl, "long long " + m_libName + "_protectlib_seq_update(\n", ",\n", "\n) {\n"};
            m_cSeqParamsp->ಕೂಡಿಸು("void* vhandlep__V");
            txtp->ಕೂಡಿಸು(m_cSeqParamsp);
            m_cSeqClksp = ಹೊಸ Ast_ಪಠ್ಯ_ಖಂಡ{fl};
            castPtr(fl, m_cSeqClksp);
            txtp->ಕೂಡಿಸು(m_cSeqClksp);
            txtp->ಕೂಡಿಸು("handlep__V->eval();\n");
            m_cSeqOutsp = ಹೊಸ Ast_ಪಠ್ಯ_ಖಂಡ{fl};
            txtp->ಕೂಡಿಸು(m_cSeqOutsp);
            txtp->ಕೂಡಿಸು("return handlep__V->m_seqnum++;\n");
            txtp->ಕೂಡಿಸು("}\n\n");
        }

        comboIgnoreComment(txtp, fl);
        m_cIgnoreParamsp = ಹೊಸ Ast_ಪಠ್ಯ_ಖಂಡ{
            fl, "void " + m_libName + "_protectlib_combo_ignore(\n", ",\n", "\n)\n{ }\n\n"};
        m_cIgnoreParamsp->ಕೂಡಿಸು("void* vhandlep__V");
        txtp->ಕೂಡಿಸು(m_cIgnoreParamsp);

        // Final
        finalComment(txtp, fl);
        txtp->ಕೂಡಿಸು("void " + m_libName + "_protectlib_final(void* vhandlep__V) {\n");
        castPtr(fl, txtp);
        txtp->ಕೂಡಿಸು(/**/ "handlep__V->final();\n");
        txtp->ಕೂಡಿಸು(/**/ "delete handlep__V;\n");
        txtp->ಕೂಡಿಸು("}\n\n");

        txtp->ಕೂಡಿಸು("}\n");
        m_cfilep->tblockp(txtp);
    }

    ಶೂನ್ಯ ಭೇಟಿ(AstVar* nodep) override {
        ಆದರೆ (!nodep->isIO()) ಹಿಂತಿರುಗಿ;
        ಆದರೆ (restoreInout(nodep)) {
            InoutPorts& ports = m_inouts[nodep->libInoutId()];
            ಆದರೆ (nodep->is_ಒಳಹರಿವು()) {
                ports.inp = nodep;
            } ಇಲ್ಲದಿದ್ದರೆ ಆದರೆ (nodep->libInoutEnable()) {
                ports.enp = nodep;
            } ಇಲ್ಲದಿದ್ದರೆ {
                ports.outp = nodep;
            }
        }
        ಆದರೆ (nodep->direction() == VDirection::ಒಳಹರಿವು) {
            ಆದರೆ (nodep->isPrimaryClock()) {
                UASSERT_OBJ(m_hasClk, nodep, "checkIfClockExists() didn't find this clock");
                handleClock(nodep);
            } ಇಲ್ಲದಿದ್ದರೆ {
                handle_ದತ್ತಾಂಶ_ಒಳಹರಿವು(nodep);
            }
        } ಇಲ್ಲದಿದ್ದರೆ {
            // Ref ports were rejected during linking, and tristate lowering split all inouts.
            UASSERT_OBJ(nodep->direction() == VDirection::OUTPUT, nodep,
                        "Unexpected library port direction");
            handleOutput(nodep);
        }
    }

    ಶೂನ್ಯ ಭೇಟಿ(Ast_ಗಂಟು*) override {}

    string c_ಒಳಹರಿವು_Connection(AstVar* varp) {
        ಹಿಂತಿರುಗಿ V3Task::ನಿಯೋಜಿಸು_Dpi_ಗಮ್ಯ_ಆಂತರಿಕ("handlep__V->" + varp->name(), varp);
    }

    // The wrapper's module ports are connected by name, so they must use the user's
    // names rather than the hashed form of long names used internally
    ಸ್ಥಿರ string sv_ಪೋರ್ಟ್_Name(ಸ್ಥಿರಾಂಕ AstVar* varp) {
        ಹಿಂತಿರುಗಿ Ast_ಗಂಟು::prettyName(VName::dehash(varp->name()));
    }

    ಸ್ಥಿರ AstVar* new_Sv_ಪೋರ್ಟ್_Var(AstVar* varp) {
        AstVar* ಸ್ಥಿರಾಂಕ newVarp = varp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
        newVarp->name(VName::dehash(varp->name()));
        ಹಿಂತಿರುಗಿ newVarp;
    }

    ಶೂನ್ಯ handleClock(AstVar* varp) {
        handle_ಒಳಹರಿವು(varp);
        m_seqPortsp->ಕೂಡಿಸು(varp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು));
        ಆದರೆ (m_hasClk) {
            ಸ್ಥಿರಾಂಕ std::string pname = sv_ಪೋರ್ಟ್_Name(varp);
            m_seqParamsp->ಕೂಡಿಸು(pname);
            m_clkSensp->ಕೂಡಿಸು(pname);
        }
        m_cSeqParamsp->ಕೂಡಿಸು(varp->dpiArgType(ನಿಜ, ಸುಳ್ಳು));
        m_cSeqClksp->ಕೂಡಿಸು(c_ಒಳಹರಿವು_Connection(varp));
    }

    ಶೂನ್ಯ handle_ದತ್ತಾಂಶ_ಒಳಹರಿವು(AstVar* varp) {
        handle_ಒಳಹರಿವು(varp);
        m_comboPortsp->ಕೂಡಿಸು(varp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು));
        m_comboParamsp->ಕೂಡಿಸು(sv_ಪೋರ್ಟ್_Name(varp));
        m_comboIgnorePortsp->ಕೂಡಿಸು(varp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು));
        ಆದರೆ (m_hasClk) m_comboIgnoreParamsp->ಕೂಡಿಸು(sv_ಪೋರ್ಟ್_Name(varp));
        m_cComboParamsp->ಕೂಡಿಸು(varp->dpiArgType(ನಿಜ, ಸುಳ್ಳು));
        m_cComboInsp->ಕೂಡಿಸು(c_ಒಳಹರಿವು_Connection(varp));
        m_cIgnoreParamsp->ಕೂಡಿಸು(varp->dpiArgType(ನಿಜ, ಸುಳ್ಳು));
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ restoreInout(ಸ್ಥಿರಾಂಕ AstVar* varp) {
        ಹಿಂತಿರುಗಿ varp->libInoutId() && !v3Global.opt.pinsInoutEnables();
    }

    ಶೂನ್ಯ handle_ಒಳಹರಿವು(AstVar* varp) {
        AstVar* ಸ್ಥಿರಾಂಕ portp = new_Sv_ಪೋರ್ಟ್_Var(varp);
        ಆದರೆ (restoreInout(varp)) {
            portp->direction(VDirection::INOUT);
            portp->varType(VVarType::WIRE);
        }
        m_modPortsp->ಕೂಡಿಸು(portp);
    }

    ಶೂನ್ಯ restoreInouts() {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ ಸ್ವಯಂಚಾಲಿತ& ನಮೂದು : m_inouts) {
            ಸ್ಥಿರಾಂಕ InoutPorts& ports = ನಮೂದು.ಎರಡನೇ;
            UASSERT_OBJ(ports.inp && ports.outp && ports.enp, m_vfilep,
                        "Incomplete lowered library inout");
            ಸ್ಥಿರಾಂಕ string ಒಳ = sv_ಪೋರ್ಟ್_Name(ports.inp);
            ಸ್ಥಿರಾಂಕ string ಹೊರಹೋಗು = ports.outp->prettyName();
            ಸ್ಥಿರಾಂಕ string en = ports.enp->prettyName();
            std::ಸದಿಶ<std::pair<ಪೂರ್ಣಾಂಕ, ಪೂರ್ಣಾಂಕ>> dimensions;
            ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_DType* dtypep = ports.inp->dtypep()->skipRefp();
            ಆಗ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Array_DType* ಸ್ಥಿರಾಂಕ arrayp = VN_CAST(dtypep, ಗಂಟು_Array_DType)) {
                // V3Slice currently rejects lowered unpacked inouts before this pass.
                // TODO: Support unpacked inouts once V3Slice supports their lowered form.
                UASSERT_OBJ(!VN_IS(arrayp, UnpackArrayDType), ports.inp,
                            "Unpacked library inout should have been rejected by V3Slice");
                dimensions.emplace_ಹಿಂಭಾಗ(arrayp->lo(), arrayp->hi());
                dtypep = arrayp->subDTypep()->skipRefp();
            }
            ಸ್ಥಿರಾಂಕ AstBasicDType* ಸ್ಥಿರಾಂಕ basicp = dtypep->basicp();
            UASSERT_OBJ(basicp, ports.inp, "Library inout must have a packed integral type");
            ಆದರೆ (basicp->isRanged()) dimensions.emplace_ಹಿಂಭಾಗ(basicp->lo(), basicp->hi());
            ಆದರೆ (dimensions.ಖಾಲಿಗಳು()) {
                m_inoutDeclsp->ಕೂಡಿಸು("assign " + ಒಳ + " = " + en + " ? " + ಹೊರಹೋಗು + " : 1'bz;\n");
            } ಇಲ್ಲದಿದ್ದರೆ {
                string lhs = ಒಳ;
                string rhsEn = en;
                string rhs_ಹೊರಹೋಗು = ಹೊರಹೋಗು;
                ಗಾಗಿ (ಗಾತ್ರ_t dim = 0; dim < dimensions.ಗಾತ್ರ_2(); ++dim) {
                    ಸ್ಥಿರಾಂಕ string ಸೂಚ್ಯಂಕ
                        = "__VlibInout" + cvt_ಗಮ್ಯ_Str(ನಮೂದು.ಮೊದಲ) + "_" + cvt_ಗಮ್ಯ_Str(dim);
                    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಕೆಳಗಿನ = dimensions[dim].ಮೊದಲ;
                    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಮೇಲಿನ = dimensions[dim].ಎರಡನೇ;
                    ಸ್ಥಿರಾಂಕ ಪೂರ್ಣಾಂಕ ಗಾತ್ರ_2 = ಮೇಲಿನ - ಕೆಳಗಿನ + 1;
                    m_inoutDeclsp->ಕೂಡಿಸು("for (genvar " + ಸೂಚ್ಯಂಕ + " = 0; " + ಸೂಚ್ಯಂಕ + " < "
                                       + cvt_ಗಮ್ಯ_Str(ಗಾತ್ರ_2) + "; ++" + ಸೂಚ್ಯಂಕ + ") begin\n");
                    ಸ್ಥಿರಾಂಕ string select = "[" + cvt_ಗಮ್ಯ_Str(ಕೆಳಗಿನ) + " + " + ಸೂಚ್ಯಂಕ + "]";
                    lhs += select;
                    rhsEn += select;
                    rhs_ಹೊರಹೋಗು += select;
                }
                m_inoutDeclsp->ಕೂಡಿಸು("assign " + lhs + " = " + rhsEn + " ? " + rhs_ಹೊರಹೋಗು
                                   + " : 1'bz;\n");
                ಗಾಗಿ (ಗಾತ್ರ_t dim = 0; dim < dimensions.ಗಾತ್ರ_2(); ++dim) m_inoutDeclsp->ಕೂಡಿಸು("end\n");
            }
        }
    }

    ಸ್ಥಿರ ಶೂನ್ಯ ಕೂಡಿಸು_Local_Variable(Ast_ಪಠ್ಯ_ಖಂಡ* textp, ಸ್ಥಿರಾಂಕ AstVar* varp, ಸ್ಥಿರಾಂಕ ಅಕ್ಷರ* suffix) {
        AstVar* ಸ್ಥಿರಾಂಕ newVarp
            = ಹೊಸ AstVar{varp->fileline(), VVarType::VAR, varp->name() + suffix, varp->dtypep()};
        textp->ಕೂಡಿಸು(newVarp);
    }

    ಶೂನ್ಯ handleOutput(AstVar* ಸ್ಥಿರಾಂಕ varp) {
        ಸ್ಥಿರಾಂಕ std::string pname = varp->prettyName();
        // Restored inout drivers are local signals and retain their internal names.
        ಸ್ಥಿರಾಂಕ std::string ಪೋರ್ಟ್_Name = restoreInout(varp) ? pname : sv_ಪೋರ್ಟ್_Name(varp);
        ಆದರೆ (restoreInout(varp)) {
            AstVar* ಸ್ಥಿರಾಂಕ localp = varp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು);
            localp->direction(VDirection::NONE);
            m_inoutDeclsp->ಕೂಡಿಸು(localp);
        } ಇಲ್ಲದಿದ್ದರೆ {
            m_modPortsp->ಕೂಡಿಸು(new_Sv_ಪೋರ್ಟ್_Var(varp));
        }
        m_comboPortsp->ಕೂಡಿಸು(varp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು));
        m_comboParamsp->ಕೂಡಿಸು(pname + "_combo__V");
        ಆದರೆ (m_hasClk) {
            m_seqPortsp->ಕೂಡಿಸು(varp->clone_ವೃಕ್ಷ(ಸುಳ್ಳು));
            m_seqParamsp->ಕೂಡಿಸು(pname + "_tmp__V");
        }

        ಕೂಡಿಸು_Local_Variable(m_comboDeclsp, varp, "_combo__V");

        ಆದರೆ (m_hasClk) {
            ಕೂಡಿಸು_Local_Variable(m_seqDeclsp, varp, "_seq__V");
            ಕೂಡಿಸು_Local_Variable(m_tmpDeclsp, varp, "_tmp__V");

            m_nbAssignsp->ಕೂಡಿಸು(pname + "_seq__V <= " + pname + "_tmp__V;\n");
            m_seqAssignsp->ಕೂಡಿಸು(ಪೋರ್ಟ್_Name + " = " + pname + "_seq__V;\n");
        }
        m_comboAssignsp->ಕೂಡಿಸು(ಪೋರ್ಟ್_Name + " = " + pname + "_combo__V;\n");
        m_cComboParamsp->ಕೂಡಿಸು(varp->dpiArgType(ನಿಜ, ಸುಳ್ಳು));
        m_cComboOutsp->ಕೂಡಿಸು(V3Task::ನಿಯೋಜಿಸು_ಆಂತರಿಕ_ಗಮ್ಯ_Dpi(varp, ನಿಜ, "", "", "handlep__V->") + "\n");
        ಆದರೆ (m_hasClk) {
            m_cSeqParamsp->ಕೂಡಿಸು(varp->dpiArgType(ನಿಜ, ಸುಳ್ಳು));
            m_cSeqOutsp->ಕೂಡಿಸು(V3Task::ನಿಯೋಜಿಸು_ಆಂತರಿಕ_ಗಮ್ಯ_Dpi(varp, ನಿಜ, "", "", "handlep__V->")
                             + "\n");
        }
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ checkIfClockExists(ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು_Module* modp) {
        ಗಾಗಿ (ಸ್ಥಿರಾಂಕ Ast_ಗಂಟು* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ಆದರೆ (ಸ್ಥಿರಾಂಕ AstVar* ಸ್ಥಿರಾಂಕ varp = VN_CAST(stmtp, Var)) {
                ಆದರೆ (varp->direction() == VDirection::ಒಳಹರಿವು && varp->isPrimaryClock()) {
                    ಹಿಂತಿರುಗಿ ನಿಜ;
                }
            }
        }
        ಹಿಂತಿರುಗಿ ಸುಳ್ಳು;
    }

ಸಾರ್ವಜನಿಕ:
    ಸ್ಪಷ್ಟ ProtectVisitor(Ast_ಗಂಟು* nodep)
        : m_libName{v3Global.opt.libCreate()}
        , m_ತುದಿ_Name{v3Global.opt.ಪೂರ್ವಪ್ರತ್ಯಯ()} {
        iterate(nodep);
    }
};

//######################################################################
// ProtectLib class functions

ಶೂನ್ಯ V3ProtectLib::protect() {
    UINFO(2, __FUNCTION__ << ":");
    ProtectVisitor{v3Global.rootp()};
}
