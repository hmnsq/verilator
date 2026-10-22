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

#అన్తర్భావ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#అన్తర్భావ "V3ProtectLib.h"

#అన్తర్భావ "V3Control.h"
#అన్తర్భావ "V3Hasher.h"
#అన్తర్భావ "V3InstrCount.h"
#అన్తర్భావ "V3String.h"
#అన్తర్భావ "V3Task.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// ProtectLib top-level visitor

తరగతి ProtectVisitor final : బహిరంగ VNVisitor {
    // TYPES
    నిర్మాణం InoutPorts final {
        AstVar* inp = శూన్యసూచిక;  // Resolved input value and original public port
        AstVar* outp = శూన్యసూచిక;  // Library drive value
        AstVar* enp = శూన్యసూచిక;  // Per-bit drive enable
    };

    // MEMBERS
    AstVFile* m_vfilep = శూన్యసూచిక;  // DPI-enabled Verilog wrapper
    AstCFile* m_cfilep = శూన్యసూచిక;  // C implementation of DPI functions
    // Verilog text blocks
    Ast_పాఠ్యం_ఖండం* m_modPortsp = శూన్యసూచిక;  // Module port list
    Ast_పాఠ్యం_ఖండం* m_comboPortsp = శూన్యసూచిక;  // Combo function port list
    Ast_పాఠ్యం_ఖండం* m_seqPortsp = శూన్యసూచిక;  // Sequential function port list
    Ast_పాఠ్యం_ఖండం* m_comboIgnorePortsp = శూన్యసూచిక;  // Combo ignore function port list
    Ast_పాఠ్యం_ఖండం* m_comboDeclsp = శూన్యసూచిక;  // Combo signal declaration list
    Ast_పాఠ్యం_ఖండం* m_seqDeclsp = శూన్యసూచిక;  // Sequential signal declaration list
    Ast_పాఠ్యం_ఖండం* m_tmpDeclsp = శూన్యసూచిక;  // Temporary signal declaration list
    Ast_పాఠ్యం_ఖండం* m_inoutDeclsp = శూన్యసూచిక;  // Lowered inout signals and tristate drivers
    Ast_పాఠ్యం_ఖండం* m_hashValuep = శూన్యసూచిక;  // CPP hash value
    Ast_పాఠ్యం_ఖండం* m_comboParamsp = శూన్యసూచిక;  // Combo function parameter list
    Ast_పాఠ్యం_ఖండం* m_clkSensp = శూన్యసూచిక;  // Clock sensitivity list
    Ast_పాఠ్యం_ఖండం* m_comboIgnoreParamsp = శూన్యసూచిక;  // Combo ignore parameter list
    Ast_పాఠ్యం_ఖండం* m_seqParamsp = శూన్యసూచిక;  // Sequential parameter list
    Ast_పాఠ్యం_ఖండం* m_nbAssignsp = శూన్యసూచిక;  // Non-blocking assignment list
    Ast_పాఠ్యం_ఖండం* m_seqAssignsp = శూన్యసూచిక;  // Sequential assignment list
    Ast_పాఠ్యం_ఖండం* m_comboAssignsp = శూన్యసూచిక;  // Combo assignment list
    // C text blocks
    Ast_పాఠ్యం_ఖండం* m_cHashValuep = శూన్యసూచిక;  // CPP hash value
    Ast_పాఠ్యం_ఖండం* m_cComboParamsp = శూన్యసూచిక;  // Combo function parameter list
    Ast_పాఠ్యం_ఖండం* m_cComboInsp = శూన్యసూచిక;  // Combo input copy list
    Ast_పాఠ్యం_ఖండం* m_cComboOutsp = శూన్యసూచిక;  // Combo output copy list
    Ast_పాఠ్యం_ఖండం* m_cSeqParamsp = శూన్యసూచిక;  // Sequential parameter list
    Ast_పాఠ్యం_ఖండం* m_cSeqClksp = శూన్యసూచిక;  // Sequential clock copy list
    Ast_పాఠ్యం_ఖండం* m_cSeqOutsp = శూన్యసూచిక;  // Sequential output copy list
    Ast_పాఠ్యం_ఖండం* m_cIgnoreParamsp = శూన్యసూచిక;  // Combo ignore parameter list
    స్థిరరాశి string m_libName;
    స్థిరరాశి string m_శిఖరం_Name;
    తార్కిక m_దొరికింది_శిఖరం = అబద్ధం;  // Have seen the top module
    తార్కిక m_hasClk = అబద్ధం;  // True if the top module has sequential logic
    std::map<పూర్ణాంకం, InoutPorts> m_inouts;  // Lowered ports grouped by original inout

    // VISITORS
    శూన్య సందర్శించు(AstNetlist* nodep) override {
        m_vfilep
            = కొత్త AstVFile{nodep->fileline(), v3Global.opt.makeDir() + "/" + m_libName + ".sv"};
        nodep->కూడు_Filesp(m_vfilep);
        m_cfilep
            = కొత్త AstCFile{nodep->fileline(), v3Global.opt.makeDir() + "/" + m_libName + ".cpp"};
        nodep->కూడు_Filesp(m_cfilep);
        iterateChildren(nodep);
    }

    శూన్య సందర్శించు(Ast_కణుపు_Module* nodep) override {
        ఒకవేళ (!nodep->is_శిఖరం()) {
            తిరిగి;
        } లేకపోతే {
            UASSERT_OBJ(!m_దొరికింది_శిఖరం, nodep, "Multiple root modules");
        }
        File_పంక్తి* స్థిరరాశి fl = nodep->fileline();
        // Need to know the existence of clk before createSvFile()
        m_hasClk = checkIfClockExists(nodep);
        createSvFile(fl, nodep);
        createCppFile(fl);

        iterateChildren(nodep);

        restoreInouts();

        // cppcheck-suppress unreadVariable
        స్థిరరాశి V3Hash hash = V3Hasher::uncachedHash(m_cfilep);
        m_hashValuep->కూడు(cvt_గమ్యం_Str(hash.విలువ()) + ";\n");
        m_cHashValuep->కూడు(cvt_గమ్యం_Str(hash.విలువ()) + "U;\n");
        m_దొరికింది_శిఖరం = నిజం;
    }

    శూన్య కూడు_Comment(Ast_పాఠ్యం_ఖండం* txtp, File_పంక్తి* fl, స్థిరరాశి string& comment) {
        txtp->కూడు(కొత్త AstComment{fl, comment});
    }

    శూన్య configSection(Ast_కణుపు_Module* /*modp*/, Ast_పాఠ్యం_ఖండం* txtp, File_పంక్తి* /*fl*/) {
        txtp->కూడు("\n`ifdef VERILATOR\n");
        txtp->కూడు("`verilator_config\n");

        txtp->కూడు("verilator_lib -module \"" + m_libName + "\"\n");

        txtp->కూడు("profile_data -hier-dpi \"" + m_libName + "_protectlib_combo_update\" -cost 64'd"
                  + std::గమ్యం_string(v3Global.current_Hier_ఖండం_ఖర్చు()) + "\n");
        txtp->కూడు("profile_data -hier-dpi \"" + m_libName + "_protectlib_seq_update\" -cost 64'd"
                  + std::గమ్యం_string(v3Global.current_Hier_ఖండం_ఖర్చు()) + "\n");

        // Mark remaining NBA protectlib wrapper DPIs as non-hazardous by deliberately forwarding
        // them with non-zero cost.
        // Also, specify hierarchical workers for those tasks for scheduling.
        txtp->కూడు("profile_data -hier-dpi \"" + m_libName
                  + "_protectlib_combo_ignore\" -cost 64'd1\n");

        txtp->కూడు("hier_workers -hier-dpi \"" + m_libName
                  + "_protectlib_combo_update\" -workers 16'd"
                  + std::గమ్యం_string(V3Control::getHierWorkers(m_libName)) + "\n");
        txtp->కూడు("hier_workers -hier-dpi \"" + m_libName
                  + "_protectlib_seq_update\" -workers 16'd"
                  + std::గమ్యం_string(V3Control::getHierWorkers(m_libName)) + "\n");
        // No workers for combo_ignore
        txtp->కూడు("`verilog\n");
        txtp->కూడు("`endif\n");
    }

    శూన్య hashComment(Ast_పాఠ్యం_ఖండం* txtp, File_పంక్తి* fl) {
        కూడు_Comment(txtp, fl, "Checks to make sure the .sv wrapper and library agree");
    }

    శూన్య initialComment(Ast_పాఠ్యం_ఖండం* txtp, File_పంక్తి* fl) {
        కూడు_Comment(txtp, fl, "Creates an instance of the library module at initial-time");
        కూడు_Comment(txtp, fl, "(one for each instance in the user's design)");
    }

    శూన్య comboComment(Ast_పాఠ్యం_ఖండం* txtp, File_పంక్తి* fl) {
        కూడు_Comment(txtp, fl, "Updates all non-clock inputs and retrieves the results");
    }

    శూన్య seqComment(Ast_పాఠ్యం_ఖండం* txtp, File_పంక్తి* fl) {
        కూడు_Comment(txtp, fl, "Updates all clocks and retrieves the results");
    }

    శూన్య comboIgnoreComment(Ast_పాఠ్యం_ఖండం* txtp, File_పంక్తి* fl) {
        కూడు_Comment(txtp, fl, "Need to convince some simulators that the input to the module");
        కూడు_Comment(txtp, fl, "must be evaluated before evaluating the clock edge");
    }

    శూన్య finalComment(Ast_పాఠ్యం_ఖండం* txtp, File_పంక్తి* fl) {
        కూడు_Comment(txtp, fl, "Evaluates the library module's final process");
    }

    శూన్య createSvFile(File_పంక్తి* fl, Ast_కణుపు_Module* modp) {
        // Comments
        Ast_పాఠ్యం_ఖండం* స్థిరరాశి txtp = కొత్త Ast_పాఠ్యం_ఖండం{fl};
        కూడు_Comment(txtp, fl, "Wrapper module for DPI protected library");
        కూడు_Comment(txtp, fl,
                   "This module requires lib" + m_libName + ".a or lib" + m_libName
                       + ".so to work");
        కూడు_Comment(txtp, fl,
                   "See instructions in your simulator for how"
                   " to use DPI libraries\n");

        // Module declaration
        // Wrapper ranges preserve the original RTL, including intentional ascending ranges.
        txtp->కూడు("/*verilator lint_save*/\n");
        txtp->కూడు("/*verilator lint_off ASCRANGE*/\n");
        m_modPortsp = కొత్త Ast_పాఠ్యం_ఖండం{fl, "module " + m_libName + " (\n", ", ", ");\n\n"};
        txtp->కూడు(m_modPortsp);

        // Timescale
        ఒకవేళ (v3Global.opt.hier_బిడ్డ() && v3Global.rootp()->timescaleSpecified()) {
            // Emit timescale for hierarchical Verilation if input HDL specifies timespec
            txtp->కూడు("timeunit "s + modp->timeunit().ascii() + ";\n");
            txtp->కూడు("timeprecision "s + +v3Global.rootp()->timeprecision().ascii() + ";\n");
        } లేకపోతే {
            కూడు_Comment(txtp, fl,
                       "Precision of submodule"
                       " (commented out to avoid requiring timescale on all modules)");
            కూడు_Comment(txtp, fl, "timeunit "s + v3Global.rootp()->timeunit().ascii() + ";");
            కూడు_Comment(txtp, fl,
                       "timeprecision "s + v3Global.rootp()->timeprecision().ascii() + ";\n");
        }

        // DPI declarations
        hashComment(txtp, fl);
        txtp->కూడు("import \"DPI-C\" function void " + m_libName
                  + "_protectlib_check_hash(int protectlib_hash__V);\n\n");
        initialComment(txtp, fl);
        txtp->కూడు("import \"DPI-C\" function chandle " + m_libName
                  + "_protectlib_create(string scope__V);\n\n");
        comboComment(txtp, fl);
        m_comboPortsp = కొత్త Ast_పాఠ్యం_ఖండం{fl,  //
                                         "import \"DPI-C\" function longint " + m_libName
                                             + "_protectlib_combo_update(\n",  //
                                         ", ",  //
                                         ");\n\n"};
        m_comboPortsp->కూడు("chandle handle__V\n");
        txtp->కూడు(m_comboPortsp);
        seqComment(txtp, fl);
        ఒకవేళ (m_hasClk) {
            m_seqPortsp = కొత్త Ast_పాఠ్యం_ఖండం{fl,  //
                                           "import \"DPI-C\" function longint " + m_libName
                                               + "_protectlib_seq_update(\n",  //
                                           ", ",  //
                                           ");\n\n"};
            m_seqPortsp->కూడు("chandle handle__V\n");
            txtp->కూడు(m_seqPortsp);
        }
        comboIgnoreComment(txtp, fl);
        m_comboIgnorePortsp = కొత్త Ast_పాఠ్యం_ఖండం{fl,  //
                                               "import \"DPI-C\" function void " + m_libName
                                                   + "_protectlib_combo_ignore(\n",  //
                                               ", ",  //
                                               ");\n\n"};
        m_comboIgnorePortsp->కూడు("chandle handle__V\n");
        txtp->కూడు(m_comboIgnorePortsp);

        finalComment(txtp, fl);
        txtp->కూడు("import \"DPI-C\" function void " + m_libName
                  + "_protectlib_final(chandle handle__V);\n\n");

        // Local variables
        // Avoid tracing handle, as it is not a stable value, so breaks wavediff
        // Likewise other internals aren't interesting to the user
        txtp->కూడు("// verilator tracing_off\n");

        txtp->కూడు("chandle handle__V;\n");
        txtp->కూడు("time last_combo_seqnum__V;\n");
        ఒకవేళ (m_hasClk) txtp->కూడు("time last_seq_seqnum__V;\n");
        txtp->కూడు("\n");

        m_comboDeclsp = కొత్త Ast_పాఠ్యం_ఖండం{fl};
        txtp->కూడు(m_comboDeclsp);
        m_seqDeclsp = కొత్త Ast_పాఠ్యం_ఖండం{fl};
        txtp->కూడు(m_seqDeclsp);
        m_tmpDeclsp = కొత్త Ast_పాఠ్యం_ఖండం{fl};
        txtp->కూడు(m_tmpDeclsp);
        m_inoutDeclsp = కొత్త Ast_పాఠ్యం_ఖండం{fl};
        txtp->కూడు(m_inoutDeclsp);

        // CPP hash value
        కూడు_Comment(txtp, fl, "Hash value to make sure this file and the corresponding");
        కూడు_Comment(txtp, fl, "library agree");
        m_hashValuep = కొత్త Ast_పాఠ్యం_ఖండం{
            fl, "localparam int protectlib_hash__V /*verilator public_flat_rd*/ = 32'd", "", "\n"};
        txtp->కూడు(m_hashValuep);

        // Initial
        txtp->కూడు("initial begin\n");
        txtp->కూడు(m_libName + "_protectlib_check_hash(protectlib_hash__V);\n");
        txtp->కూడు("if (handle__V == null) handle__V = " + m_libName
                  + "_protectlib_create"
                    "($sformatf(\"%m\"));\n");
        txtp->కూడు("end\n\n");

        // Combinatorial process
        కూడు_Comment(txtp, fl, "Combinatorially evaluate changes to inputs");
        txtp->కూడు("always_comb begin\n");
        m_comboParamsp = కొత్త Ast_పాఠ్యం_ఖండం{fl,  //
                                          "last_combo_seqnum__V = " + m_libName
                                              + "_protectlib_combo_update(\n",  //
                                          ",\n",  //
                                          "\n);\n"};
        m_comboParamsp->కూడు("handle__V");
        txtp->కూడు(m_comboParamsp);
        txtp->కూడు("end\n\n");

        // Sequential process
        ఒకవేళ (m_hasClk) {
            కూడు_Comment(txtp, fl, "Evaluate clock edges");
            m_clkSensp = కొత్త Ast_పాఠ్యం_ఖండం{fl, "always @(", " or ", ")"};
            txtp->కూడు(m_clkSensp);
            txtp->కూడు(" begin\n");
            m_comboIgnoreParamsp
                = కొత్త Ast_పాఠ్యం_ఖండం{fl,  //
                                   m_libName + "_protectlib_combo_ignore(\n", ",\n", "\n);\n"};
            m_comboIgnoreParamsp->కూడు("handle__V");
            txtp->కూడు(m_comboIgnoreParamsp);
            m_seqParamsp = కొత్త Ast_పాఠ్యం_ఖండం{fl,  //
                                            "last_seq_seqnum__V <= " + m_libName
                                                + "_protectlib_seq_update(\n",  //
                                            ",\n",  //
                                            "\n);\n"};
            m_seqParamsp->కూడు("handle__V");
            txtp->కూడు(m_seqParamsp);
            m_nbAssignsp = కొత్త Ast_పాఠ్యం_ఖండం{fl};
            txtp->కూడు(m_nbAssignsp);
            txtp->కూడు("end\n\n");
        }

        // Select between combinatorial and sequential results
        కూడు_Comment(txtp, fl, "Select between combinatorial and sequential results");
        txtp->కూడు("always_comb begin\n");
        ఒకవేళ (m_hasClk) {
            txtp->కూడు("if (last_seq_seqnum__V > last_combo_seqnum__V) begin\n");
            m_seqAssignsp = కొత్త Ast_పాఠ్యం_ఖండం{fl};
            txtp->కూడు(m_seqAssignsp);
            txtp->కూడు("end else begin\n");
            m_comboAssignsp = కొత్త Ast_పాఠ్యం_ఖండం{fl};
            txtp->కూడు(m_comboAssignsp);
            txtp->కూడు("end\n");
        } లేకపోతే {
            m_comboAssignsp = కొత్త Ast_పాఠ్యం_ఖండం{fl};
            txtp->కూడు(m_comboAssignsp);
        }
        txtp->కూడు("end\n\n");

        // Final
        txtp->కూడు("final " + m_libName + "_protectlib_final(handle__V);\n\n");

        txtp->కూడు("endmodule\n");
        txtp->కూడు("/*verilator lint_restore*/\n");

        configSection(modp, txtp, fl);

        m_vfilep->tblockp(txtp);
    }

    శూన్య castPtr(File_పంక్తి* /*fl*/, Ast_పాఠ్యం_ఖండం* txtp) {
        txtp->కూడు(m_శిఖరం_Name
                  + "_container* const handlep__V = "  // LCOV_EXCL_LINE  // lcov bug
                    "static_cast<"
                  + m_శిఖరం_Name + "_container*>(vhandlep__V);\n");
    }

    శూన్య createCppFile(File_పంక్తి* fl) {
        // Comments
        Ast_పాఠ్యం_ఖండం* స్థిరరాశి txtp = కొత్త Ast_పాఠ్యం_ఖండం{fl};
        కూడు_Comment(txtp, fl, "Wrapper functions for DPI protected library\n");

        // Includes
        txtp->కూడు("#include \"" + m_శిఖరం_Name + ".h\"\n");
        txtp->కూడు("#include \"verilated_dpi.h\"\n\n");
        txtp->కూడు("#include <cstdio>\n");
        txtp->కూడు("#include <cstdlib>\n\n");

        // Verilated module plus sequence number
        కూడు_Comment(txtp, fl, "Container class to house verilated object and sequence number");
        txtp->కూడు("class " + m_శిఖరం_Name + "_container: public " + m_శిఖరం_Name + " {\n");
        txtp->కూడు("public:\n");
        txtp->కూడు("long long m_seqnum;\n");
        txtp->కూడు(m_శిఖరం_Name + "_container(const char* scopep__V):\n");
        txtp->కూడు(m_శిఖరం_Name + "(scopep__V) {}\n");
        txtp->కూడు("};\n\n");

        // Extern C
        txtp->కూడు("extern \"C\" {\n\n");

        // Hash check
        hashComment(txtp, fl);
        txtp->కూడు("void " + m_libName
                  + "_protectlib_check_hash"
                    "(int protectlib_hash__V) {\n");
        m_cHashValuep = కొత్త Ast_పాఠ్యం_ఖండం{fl, "const int expected_hash__V = ", "", ""};
        txtp->కూడు(m_cHashValuep);
        txtp->కూడు(/**/ "if (protectlib_hash__V != expected_hash__V) {\n");
        txtp->కూడు(/****/ "fprintf(stderr, \"%%Error: cannot use " + m_libName
                  + " library, "
                    "Verliog (%u) and library (%u) hash values do not "
                    "agree\\n\", protectlib_hash__V, expected_hash__V);\n");
        txtp->కూడు(/****/ "std::exit(EXIT_FAILURE);\n");
        txtp->కూడు(/**/ "}\n");
        txtp->కూడు("}\n\n");

        // Initial
        initialComment(txtp, fl);
        txtp->కూడు("void* " + m_libName + "_protectlib_create(const char* scopep__V) {\n");
        txtp->కూడు(/**/ m_శిఖరం_Name + "_container* const handlep__V = new " + m_శిఖరం_Name
                  + "_container{scopep__V};\n");
        txtp->కూడు(/**/ "return handlep__V;\n");
        txtp->కూడు("}\n\n");

        // Updates
        comboComment(txtp, fl);
        m_cComboParamsp = కొత్త Ast_పాఠ్యం_ఖండం{
            fl, "long long " + m_libName + "_protectlib_combo_update(\n", ",\n", "\n) {\n"};
        m_cComboParamsp->కూడు("void* vhandlep__V");
        txtp->కూడు(m_cComboParamsp);
        m_cComboInsp = కొత్త Ast_పాఠ్యం_ఖండం{fl};
        castPtr(fl, m_cComboInsp);
        txtp->కూడు(m_cComboInsp);
        txtp->కూడు("handlep__V->eval();\n");
        m_cComboOutsp = కొత్త Ast_పాఠ్యం_ఖండం{fl};
        txtp->కూడు(m_cComboOutsp);
        txtp->కూడు("return handlep__V->m_seqnum++;\n");
        txtp->కూడు("}\n\n");

        ఒకవేళ (m_hasClk) {
            seqComment(txtp, fl);
            m_cSeqParamsp = కొత్త Ast_పాఠ్యం_ఖండం{
                fl, "long long " + m_libName + "_protectlib_seq_update(\n", ",\n", "\n) {\n"};
            m_cSeqParamsp->కూడు("void* vhandlep__V");
            txtp->కూడు(m_cSeqParamsp);
            m_cSeqClksp = కొత్త Ast_పాఠ్యం_ఖండం{fl};
            castPtr(fl, m_cSeqClksp);
            txtp->కూడు(m_cSeqClksp);
            txtp->కూడు("handlep__V->eval();\n");
            m_cSeqOutsp = కొత్త Ast_పాఠ్యం_ఖండం{fl};
            txtp->కూడు(m_cSeqOutsp);
            txtp->కూడు("return handlep__V->m_seqnum++;\n");
            txtp->కూడు("}\n\n");
        }

        comboIgnoreComment(txtp, fl);
        m_cIgnoreParamsp = కొత్త Ast_పాఠ్యం_ఖండం{
            fl, "void " + m_libName + "_protectlib_combo_ignore(\n", ",\n", "\n)\n{ }\n\n"};
        m_cIgnoreParamsp->కూడు("void* vhandlep__V");
        txtp->కూడు(m_cIgnoreParamsp);

        // Final
        finalComment(txtp, fl);
        txtp->కూడు("void " + m_libName + "_protectlib_final(void* vhandlep__V) {\n");
        castPtr(fl, txtp);
        txtp->కూడు(/**/ "handlep__V->final();\n");
        txtp->కూడు(/**/ "delete handlep__V;\n");
        txtp->కూడు("}\n\n");

        txtp->కూడు("}\n");
        m_cfilep->tblockp(txtp);
    }

    శూన్య సందర్శించు(AstVar* nodep) override {
        ఒకవేళ (!nodep->isIO()) తిరిగి;
        ఒకవేళ (restoreInout(nodep)) {
            InoutPorts& ports = m_inouts[nodep->libInoutId()];
            ఒకవేళ (nodep->is_ఆదానం()) {
                ports.inp = nodep;
            } లేకపోతే ఒకవేళ (nodep->libInoutEnable()) {
                ports.enp = nodep;
            } లేకపోతే {
                ports.outp = nodep;
            }
        }
        ఒకవేళ (nodep->direction() == VDirection::ఆదానం) {
            ఒకవేళ (nodep->isPrimaryClock()) {
                UASSERT_OBJ(m_hasClk, nodep, "checkIfClockExists() didn't find this clock");
                handleClock(nodep);
            } లేకపోతే {
                handle_దత్తాంశం_ఆదానం(nodep);
            }
        } లేకపోతే {
            // Ref ports were rejected during linking, and tristate lowering split all inouts.
            UASSERT_OBJ(nodep->direction() == VDirection::OUTPUT, nodep,
                        "Unexpected library port direction");
            handleOutput(nodep);
        }
    }

    శూన్య సందర్శించు(Ast_కణుపు*) override {}

    string c_ఆదానం_Connection(AstVar* varp) {
        తిరిగి V3Task::కేటాయించు_Dpi_గమ్యం_అంతర్గత("handlep__V->" + varp->name(), varp);
    }

    // The wrapper's module ports are connected by name, so they must use the user's
    // names rather than the hashed form of long names used internally
    స్థిర string sv_పోర్టు_Name(స్థిరరాశి AstVar* varp) {
        తిరిగి Ast_కణుపు::prettyName(VName::dehash(varp->name()));
    }

    స్థిర AstVar* new_Sv_పోర్టు_Var(AstVar* varp) {
        AstVar* స్థిరరాశి newVarp = varp->clone_వృక్షం(అబద్ధం);
        newVarp->name(VName::dehash(varp->name()));
        తిరిగి newVarp;
    }

    శూన్య handleClock(AstVar* varp) {
        handle_ఆదానం(varp);
        m_seqPortsp->కూడు(varp->clone_వృక్షం(అబద్ధం));
        ఒకవేళ (m_hasClk) {
            స్థిరరాశి std::string pname = sv_పోర్టు_Name(varp);
            m_seqParamsp->కూడు(pname);
            m_clkSensp->కూడు(pname);
        }
        m_cSeqParamsp->కూడు(varp->dpiArgType(నిజం, అబద్ధం));
        m_cSeqClksp->కూడు(c_ఆదానం_Connection(varp));
    }

    శూన్య handle_దత్తాంశం_ఆదానం(AstVar* varp) {
        handle_ఆదానం(varp);
        m_comboPortsp->కూడు(varp->clone_వృక్షం(అబద్ధం));
        m_comboParamsp->కూడు(sv_పోర్టు_Name(varp));
        m_comboIgnorePortsp->కూడు(varp->clone_వృక్షం(అబద్ధం));
        ఒకవేళ (m_hasClk) m_comboIgnoreParamsp->కూడు(sv_పోర్టు_Name(varp));
        m_cComboParamsp->కూడు(varp->dpiArgType(నిజం, అబద్ధం));
        m_cComboInsp->కూడు(c_ఆదానం_Connection(varp));
        m_cIgnoreParamsp->కూడు(varp->dpiArgType(నిజం, అబద్ధం));
    }

    స్థిర తార్కిక restoreInout(స్థిరరాశి AstVar* varp) {
        తిరిగి varp->libInoutId() && !v3Global.opt.pinsInoutEnables();
    }

    శూన్య handle_ఆదానం(AstVar* varp) {
        AstVar* స్థిరరాశి portp = new_Sv_పోర్టు_Var(varp);
        ఒకవేళ (restoreInout(varp)) {
            portp->direction(VDirection::INOUT);
            portp->varType(VVarType::WIRE);
        }
        m_modPortsp->కూడు(portp);
    }

    శూన్య restoreInouts() {
        కొరకు (స్థిరరాశి స్వయంచాలక& నమోదు_2 : m_inouts) {
            స్థిరరాశి InoutPorts& ports = నమోదు_2.రెండవ;
            UASSERT_OBJ(ports.inp && ports.outp && ports.enp, m_vfilep,
                        "Incomplete lowered library inout");
            స్థిరరాశి string లోపల = sv_పోర్టు_Name(ports.inp);
            స్థిరరాశి string బహిర్గతం = ports.outp->prettyName();
            స్థిరరాశి string en = ports.enp->prettyName();
            std::సదిశ<std::pair<పూర్ణాంకం, పూర్ణాంకం>> dimensions;
            స్థిరరాశి Ast_కణుపు_DType* dtypep = ports.inp->dtypep()->skipRefp();
            అయితే (స్థిరరాశి Ast_కణుపు_Array_DType* స్థిరరాశి arrayp = VN_CAST(dtypep, కణుపు_Array_DType)) {
                // V3Slice currently rejects lowered unpacked inouts before this pass.
                // TODO: Support unpacked inouts once V3Slice supports their lowered form.
                UASSERT_OBJ(!VN_IS(arrayp, UnpackArrayDType), ports.inp,
                            "Unpacked library inout should have been rejected by V3Slice");
                dimensions.emplace_వెనుక(arrayp->lo(), arrayp->hi());
                dtypep = arrayp->subDTypep()->skipRefp();
            }
            స్థిరరాశి AstBasicDType* స్థిరరాశి basicp = dtypep->basicp();
            UASSERT_OBJ(basicp, ports.inp, "Library inout must have a packed integral type");
            ఒకవేళ (basicp->isRanged()) dimensions.emplace_వెనుక(basicp->lo(), basicp->hi());
            ఒకవేళ (dimensions.ఖాళీలు()) {
                m_inoutDeclsp->కూడు("assign " + లోపల + " = " + en + " ? " + బహిర్గతం + " : 1'bz;\n");
            } లేకపోతే {
                string lhs = లోపల;
                string rhsEn = en;
                string rhs_బహిర్గతం = బహిర్గతం;
                కొరకు (కొలత_t dim = 0; dim < dimensions.కొలత(); ++dim) {
                    స్థిరరాశి string సూచీ
                        = "__VlibInout" + cvt_గమ్యం_Str(నమోదు_2.మొదటి) + "_" + cvt_గమ్యం_Str(dim);
                    స్థిరరాశి పూర్ణాంకం దిగువ = dimensions[dim].మొదటి;
                    స్థిరరాశి పూర్ణాంకం ఎగువ = dimensions[dim].రెండవ;
                    స్థిరరాశి పూర్ణాంకం కొలత = ఎగువ - దిగువ + 1;
                    m_inoutDeclsp->కూడు("for (genvar " + సూచీ + " = 0; " + సూచీ + " < "
                                       + cvt_గమ్యం_Str(కొలత) + "; ++" + సూచీ + ") begin\n");
                    స్థిరరాశి string select = "[" + cvt_గమ్యం_Str(దిగువ) + " + " + సూచీ + "]";
                    lhs += select;
                    rhsEn += select;
                    rhs_బహిర్గతం += select;
                }
                m_inoutDeclsp->కూడు("assign " + lhs + " = " + rhsEn + " ? " + rhs_బహిర్గతం
                                   + " : 1'bz;\n");
                కొరకు (కొలత_t dim = 0; dim < dimensions.కొలత(); ++dim) m_inoutDeclsp->కూడు("end\n");
            }
        }
    }

    స్థిర శూన్య కూడు_Local_Variable(Ast_పాఠ్యం_ఖండం* textp, స్థిరరాశి AstVar* varp, స్థిరరాశి అక్షరం* suffix) {
        AstVar* స్థిరరాశి newVarp
            = కొత్త AstVar{varp->fileline(), VVarType::VAR, varp->name() + suffix, varp->dtypep()};
        textp->కూడు(newVarp);
    }

    శూన్య handleOutput(AstVar* స్థిరరాశి varp) {
        స్థిరరాశి std::string pname = varp->prettyName();
        // Restored inout drivers are local signals and retain their internal names.
        స్థిరరాశి std::string పోర్టు_Name = restoreInout(varp) ? pname : sv_పోర్టు_Name(varp);
        ఒకవేళ (restoreInout(varp)) {
            AstVar* స్థిరరాశి localp = varp->clone_వృక్షం(అబద్ధం);
            localp->direction(VDirection::NONE);
            m_inoutDeclsp->కూడు(localp);
        } లేకపోతే {
            m_modPortsp->కూడు(new_Sv_పోర్టు_Var(varp));
        }
        m_comboPortsp->కూడు(varp->clone_వృక్షం(అబద్ధం));
        m_comboParamsp->కూడు(pname + "_combo__V");
        ఒకవేళ (m_hasClk) {
            m_seqPortsp->కూడు(varp->clone_వృక్షం(అబద్ధం));
            m_seqParamsp->కూడు(pname + "_tmp__V");
        }

        కూడు_Local_Variable(m_comboDeclsp, varp, "_combo__V");

        ఒకవేళ (m_hasClk) {
            కూడు_Local_Variable(m_seqDeclsp, varp, "_seq__V");
            కూడు_Local_Variable(m_tmpDeclsp, varp, "_tmp__V");

            m_nbAssignsp->కూడు(pname + "_seq__V <= " + pname + "_tmp__V;\n");
            m_seqAssignsp->కూడు(పోర్టు_Name + " = " + pname + "_seq__V;\n");
        }
        m_comboAssignsp->కూడు(పోర్టు_Name + " = " + pname + "_combo__V;\n");
        m_cComboParamsp->కూడు(varp->dpiArgType(నిజం, అబద్ధం));
        m_cComboOutsp->కూడు(V3Task::కేటాయించు_అంతర్గత_గమ్యం_Dpi(varp, నిజం, "", "", "handlep__V->") + "\n");
        ఒకవేళ (m_hasClk) {
            m_cSeqParamsp->కూడు(varp->dpiArgType(నిజం, అబద్ధం));
            m_cSeqOutsp->కూడు(V3Task::కేటాయించు_అంతర్గత_గమ్యం_Dpi(varp, నిజం, "", "", "handlep__V->")
                             + "\n");
        }
    }

    స్థిర తార్కిక checkIfClockExists(స్థిరరాశి Ast_కణుపు_Module* modp) {
        కొరకు (స్థిరరాశి Ast_కణుపు* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ఒకవేళ (స్థిరరాశి AstVar* స్థిరరాశి varp = VN_CAST(stmtp, Var)) {
                ఒకవేళ (varp->direction() == VDirection::ఆదానం && varp->isPrimaryClock()) {
                    తిరిగి నిజం;
                }
            }
        }
        తిరిగి అబద్ధం;
    }

బహిరంగ:
    స్పష్టమైన ProtectVisitor(Ast_కణుపు* nodep)
        : m_libName{v3Global.opt.libCreate()}
        , m_శిఖరం_Name{v3Global.opt.ఉపసర్గ()} {
        iterate(nodep);
    }
};

//######################################################################
// ProtectLib class functions

శూన్య V3ProtectLib::protect() {
    UINFO(2, __FUNCTION__ << ":");
    ProtectVisitor{v3Global.rootp()};
}
