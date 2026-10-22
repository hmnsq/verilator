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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3ProtectLib.h"

#अन्तर्भाव "V3Control.h"
#अन्तर्भाव "V3Hasher.h"
#अन्तर्भाव "V3InstrCount.h"
#अन्तर्भाव "V3String.h"
#अन्तर्भाव "V3Task.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// ProtectLib top-level visitor

वर्ग ProtectVisitor final : सार्वजनिक VNVisitor {
    // TYPES
    संरचना InoutPorts final {
        AstVar* inp = शून्यसूचकः;  // Resolved input value and original public port
        AstVar* outp = शून्यसूचकः;  // Library drive value
        AstVar* enp = शून्यसूचकः;  // Per-bit drive enable
    };

    // MEMBERS
    AstVFile* m_vfilep = शून्यसूचकः;  // DPI-enabled Verilog wrapper
    AstCFile* m_cfilep = शून्यसूचकः;  // C implementation of DPI functions
    // Verilog text blocks
    Ast_Text_खण्डम्* m_modPortsp = शून्यसूचकः;  // Module port list
    Ast_Text_खण्डम्* m_comboPortsp = शून्यसूचकः;  // Combo function port list
    Ast_Text_खण्डम्* m_seqPortsp = शून्यसूचकः;  // Sequential function port list
    Ast_Text_खण्डम्* m_comboIgnorePortsp = शून्यसूचकः;  // Combo ignore function port list
    Ast_Text_खण्डम्* m_comboDeclsp = शून्यसूचकः;  // Combo signal declaration list
    Ast_Text_खण्डम्* m_seqDeclsp = शून्यसूचकः;  // Sequential signal declaration list
    Ast_Text_खण्डम्* m_tmpDeclsp = शून्यसूचकः;  // Temporary signal declaration list
    Ast_Text_खण्डम्* m_inoutDeclsp = शून्यसूचकः;  // Lowered inout signals and tristate drivers
    Ast_Text_खण्डम्* m_hashValuep = शून्यसूचकः;  // CPP hash value
    Ast_Text_खण्डम्* m_comboParamsp = शून्यसूचकः;  // Combo function parameter list
    Ast_Text_खण्डम्* m_clkSensp = शून्यसूचकः;  // Clock sensitivity list
    Ast_Text_खण्डम्* m_comboIgnoreParamsp = शून्यसूचकः;  // Combo ignore parameter list
    Ast_Text_खण्डम्* m_seqParamsp = शून्यसूचकः;  // Sequential parameter list
    Ast_Text_खण्डम्* m_nbAssignsp = शून्यसूचकः;  // Non-blocking assignment list
    Ast_Text_खण्डम्* m_seqAssignsp = शून्यसूचकः;  // Sequential assignment list
    Ast_Text_खण्डम्* m_comboAssignsp = शून्यसूचकः;  // Combo assignment list
    // C text blocks
    Ast_Text_खण्डम्* m_cHashValuep = शून्यसूचकः;  // CPP hash value
    Ast_Text_खण्डम्* m_cComboParamsp = शून्यसूचकः;  // Combo function parameter list
    Ast_Text_खण्डम्* m_cComboInsp = शून्यसूचकः;  // Combo input copy list
    Ast_Text_खण्डम्* m_cComboOutsp = शून्यसूचकः;  // Combo output copy list
    Ast_Text_खण्डम्* m_cSeqParamsp = शून्यसूचकः;  // Sequential parameter list
    Ast_Text_खण्डम्* m_cSeqClksp = शून्यसूचकः;  // Sequential clock copy list
    Ast_Text_खण्डम्* m_cSeqOutsp = शून्यसूचकः;  // Sequential output copy list
    Ast_Text_खण्डम्* m_cIgnoreParamsp = शून्यसूचकः;  // Combo ignore parameter list
    ध्रुव string m_libName;
    ध्रुव string m_शिखरम्_Name;
    तार्किक m_प्राप्तम्_शिखरम् = असत्य;  // Have seen the top module
    तार्किक m_hasClk = असत्य;  // True if the top module has sequential logic
    std::map<पूर्णाङ्क, InoutPorts> m_inouts;  // Lowered ports grouped by original inout

    // VISITORS
    शून्य दर्शय(AstNetlist* nodep) override {
        m_vfilep
            = नूतन AstVFile{nodep->fileline(), v3Global.opt.makeDir() + "/" + m_libName + ".sv"};
        nodep->सङ्कलय_Filesp(m_vfilep);
        m_cfilep
            = नूतन AstCFile{nodep->fileline(), v3Global.opt.makeDir() + "/" + m_libName + ".cpp"};
        nodep->सङ्कलय_Filesp(m_cfilep);
        iterateChildren(nodep);
    }

    शून्य दर्शय(Ast_ग्रन्थिः_Module* nodep) override {
        यदि (!nodep->is_शिखरम्()) {
            प्रत्यागम;
        } अन्यथा {
            UASSERT_OBJ(!m_प्राप्तम्_शिखरम्, nodep, "Multiple root modules");
        }
        File_पङ्क्तिः* ध्रुव fl = nodep->fileline();
        // Need to know the existence of clk before createSvFile()
        m_hasClk = checkIfClockExists(nodep);
        createSvFile(fl, nodep);
        createCppFile(fl);

        iterateChildren(nodep);

        restoreInouts();

        // cppcheck-suppress unreadVariable
        ध्रुव V3Hash hash = V3Hasher::uncachedHash(m_cfilep);
        m_hashValuep->सङ्कलय(cvt_गन्तव्यम्_Str(hash.मानम्()) + ";\n");
        m_cHashValuep->सङ्कलय(cvt_गन्तव्यम्_Str(hash.मानम्()) + "U;\n");
        m_प्राप्तम्_शिखरम् = सत्य;
    }

    शून्य सङ्कलय_Comment(Ast_Text_खण्डम्* txtp, File_पङ्क्तिः* fl, ध्रुव string& comment) {
        txtp->सङ्कलय(नूतन AstComment{fl, comment});
    }

    शून्य configSection(Ast_ग्रन्थिः_Module* /*modp*/, Ast_Text_खण्डम्* txtp, File_पङ्क्तिः* /*fl*/) {
        txtp->सङ्कलय("\n`ifdef VERILATOR\n");
        txtp->सङ्कलय("`verilator_config\n");

        txtp->सङ्कलय("verilator_lib -module \"" + m_libName + "\"\n");

        txtp->सङ्कलय("profile_data -hier-dpi \"" + m_libName + "_protectlib_combo_update\" -cost 64'd"
                  + std::गन्तव्यम्_string(v3Global.current_Hier_खण्डम्_Cost()) + "\n");
        txtp->सङ्कलय("profile_data -hier-dpi \"" + m_libName + "_protectlib_seq_update\" -cost 64'd"
                  + std::गन्तव्यम्_string(v3Global.current_Hier_खण्डम्_Cost()) + "\n");

        // Mark remaining NBA protectlib wrapper DPIs as non-hazardous by deliberately forwarding
        // them with non-zero cost.
        // Also, specify hierarchical workers for those tasks for scheduling.
        txtp->सङ्कलय("profile_data -hier-dpi \"" + m_libName
                  + "_protectlib_combo_ignore\" -cost 64'd1\n");

        txtp->सङ्कलय("hier_workers -hier-dpi \"" + m_libName
                  + "_protectlib_combo_update\" -workers 16'd"
                  + std::गन्तव्यम्_string(V3Control::getHierWorkers(m_libName)) + "\n");
        txtp->सङ्कलय("hier_workers -hier-dpi \"" + m_libName
                  + "_protectlib_seq_update\" -workers 16'd"
                  + std::गन्तव्यम्_string(V3Control::getHierWorkers(m_libName)) + "\n");
        // No workers for combo_ignore
        txtp->सङ्कलय("`verilog\n");
        txtp->सङ्कलय("`endif\n");
    }

    शून्य hashComment(Ast_Text_खण्डम्* txtp, File_पङ्क्तिः* fl) {
        सङ्कलय_Comment(txtp, fl, "Checks to make sure the .sv wrapper and library agree");
    }

    शून्य initialComment(Ast_Text_खण्डम्* txtp, File_पङ्क्तिः* fl) {
        सङ्कलय_Comment(txtp, fl, "Creates an instance of the library module at initial-time");
        सङ्कलय_Comment(txtp, fl, "(one for each instance in the user's design)");
    }

    शून्य comboComment(Ast_Text_खण्डम्* txtp, File_पङ्क्तिः* fl) {
        सङ्कलय_Comment(txtp, fl, "Updates all non-clock inputs and retrieves the results");
    }

    शून्य seqComment(Ast_Text_खण्डम्* txtp, File_पङ्क्तिः* fl) {
        सङ्कलय_Comment(txtp, fl, "Updates all clocks and retrieves the results");
    }

    शून्य comboIgnoreComment(Ast_Text_खण्डम्* txtp, File_पङ्क्तिः* fl) {
        सङ्कलय_Comment(txtp, fl, "Need to convince some simulators that the input to the module");
        सङ्कलय_Comment(txtp, fl, "must be evaluated before evaluating the clock edge");
    }

    शून्य finalComment(Ast_Text_खण्डम्* txtp, File_पङ्क्तिः* fl) {
        सङ्कलय_Comment(txtp, fl, "Evaluates the library module's final process");
    }

    शून्य createSvFile(File_पङ्क्तिः* fl, Ast_ग्रन्थिः_Module* modp) {
        // Comments
        Ast_Text_खण्डम्* ध्रुव txtp = नूतन Ast_Text_खण्डम्{fl};
        सङ्कलय_Comment(txtp, fl, "Wrapper module for DPI protected library");
        सङ्कलय_Comment(txtp, fl,
                   "This module requires lib" + m_libName + ".a or lib" + m_libName
                       + ".so to work");
        सङ्कलय_Comment(txtp, fl,
                   "See instructions in your simulator for how"
                   " to use DPI libraries\n");

        // Module declaration
        // Wrapper ranges preserve the original RTL, including intentional ascending ranges.
        txtp->सङ्कलय("/*verilator lint_save*/\n");
        txtp->सङ्कलय("/*verilator lint_off ASCRANGE*/\n");
        m_modPortsp = नूतन Ast_Text_खण्डम्{fl, "module " + m_libName + " (\n", ", ", ");\n\n"};
        txtp->सङ्कलय(m_modPortsp);

        // Timescale
        यदि (v3Global.opt.hierChild() && v3Global.rootp()->timescaleSpecified()) {
            // Emit timescale for hierarchical Verilation if input HDL specifies timespec
            txtp->सङ्कलय("timeunit "s + modp->timeunit().ascii() + ";\n");
            txtp->सङ्कलय("timeprecision "s + +v3Global.rootp()->timeprecision().ascii() + ";\n");
        } अन्यथा {
            सङ्कलय_Comment(txtp, fl,
                       "Precision of submodule"
                       " (commented out to avoid requiring timescale on all modules)");
            सङ्कलय_Comment(txtp, fl, "timeunit "s + v3Global.rootp()->timeunit().ascii() + ";");
            सङ्कलय_Comment(txtp, fl,
                       "timeprecision "s + v3Global.rootp()->timeprecision().ascii() + ";\n");
        }

        // DPI declarations
        hashComment(txtp, fl);
        txtp->सङ्कलय("import \"DPI-C\" function void " + m_libName
                  + "_protectlib_check_hash(int protectlib_hash__V);\n\n");
        initialComment(txtp, fl);
        txtp->सङ्कलय("import \"DPI-C\" function chandle " + m_libName
                  + "_protectlib_create(string scope__V);\n\n");
        comboComment(txtp, fl);
        m_comboPortsp = नूतन Ast_Text_खण्डम्{fl,  //
                                         "import \"DPI-C\" function longint " + m_libName
                                             + "_protectlib_combo_update(\n",  //
                                         ", ",  //
                                         ");\n\n"};
        m_comboPortsp->सङ्कलय("chandle handle__V\n");
        txtp->सङ्कलय(m_comboPortsp);
        seqComment(txtp, fl);
        यदि (m_hasClk) {
            m_seqPortsp = नूतन Ast_Text_खण्डम्{fl,  //
                                           "import \"DPI-C\" function longint " + m_libName
                                               + "_protectlib_seq_update(\n",  //
                                           ", ",  //
                                           ");\n\n"};
            m_seqPortsp->सङ्कलय("chandle handle__V\n");
            txtp->सङ्कलय(m_seqPortsp);
        }
        comboIgnoreComment(txtp, fl);
        m_comboIgnorePortsp = नूतन Ast_Text_खण्डम्{fl,  //
                                               "import \"DPI-C\" function void " + m_libName
                                                   + "_protectlib_combo_ignore(\n",  //
                                               ", ",  //
                                               ");\n\n"};
        m_comboIgnorePortsp->सङ्कलय("chandle handle__V\n");
        txtp->सङ्कलय(m_comboIgnorePortsp);

        finalComment(txtp, fl);
        txtp->सङ्कलय("import \"DPI-C\" function void " + m_libName
                  + "_protectlib_final(chandle handle__V);\n\n");

        // Local variables
        // Avoid tracing handle, as it is not a stable value, so breaks wavediff
        // Likewise other internals aren't interesting to the user
        txtp->सङ्कलय("// verilator tracing_off\n");

        txtp->सङ्कलय("chandle handle__V;\n");
        txtp->सङ्कलय("time last_combo_seqnum__V;\n");
        यदि (m_hasClk) txtp->सङ्कलय("time last_seq_seqnum__V;\n");
        txtp->सङ्कलय("\n");

        m_comboDeclsp = नूतन Ast_Text_खण्डम्{fl};
        txtp->सङ्कलय(m_comboDeclsp);
        m_seqDeclsp = नूतन Ast_Text_खण्डम्{fl};
        txtp->सङ्कलय(m_seqDeclsp);
        m_tmpDeclsp = नूतन Ast_Text_खण्डम्{fl};
        txtp->सङ्कलय(m_tmpDeclsp);
        m_inoutDeclsp = नूतन Ast_Text_खण्डम्{fl};
        txtp->सङ्कलय(m_inoutDeclsp);

        // CPP hash value
        सङ्कलय_Comment(txtp, fl, "Hash value to make sure this file and the corresponding");
        सङ्कलय_Comment(txtp, fl, "library agree");
        m_hashValuep = नूतन Ast_Text_खण्डम्{
            fl, "localparam int protectlib_hash__V /*verilator public_flat_rd*/ = 32'd", "", "\n"};
        txtp->सङ्कलय(m_hashValuep);

        // Initial
        txtp->सङ्कलय("initial begin\n");
        txtp->सङ्कलय(m_libName + "_protectlib_check_hash(protectlib_hash__V);\n");
        txtp->सङ्कलय("if (handle__V == null) handle__V = " + m_libName
                  + "_protectlib_create"
                    "($sformatf(\"%m\"));\n");
        txtp->सङ्कलय("end\n\n");

        // Combinatorial process
        सङ्कलय_Comment(txtp, fl, "Combinatorially evaluate changes to inputs");
        txtp->सङ्कलय("always_comb begin\n");
        m_comboParamsp = नूतन Ast_Text_खण्डम्{fl,  //
                                          "last_combo_seqnum__V = " + m_libName
                                              + "_protectlib_combo_update(\n",  //
                                          ",\n",  //
                                          "\n);\n"};
        m_comboParamsp->सङ्कलय("handle__V");
        txtp->सङ्कलय(m_comboParamsp);
        txtp->सङ्कलय("end\n\n");

        // Sequential process
        यदि (m_hasClk) {
            सङ्कलय_Comment(txtp, fl, "Evaluate clock edges");
            m_clkSensp = नूतन Ast_Text_खण्डम्{fl, "always @(", " or ", ")"};
            txtp->सङ्कलय(m_clkSensp);
            txtp->सङ्कलय(" begin\n");
            m_comboIgnoreParamsp
                = नूतन Ast_Text_खण्डम्{fl,  //
                                   m_libName + "_protectlib_combo_ignore(\n", ",\n", "\n);\n"};
            m_comboIgnoreParamsp->सङ्कलय("handle__V");
            txtp->सङ्कलय(m_comboIgnoreParamsp);
            m_seqParamsp = नूतन Ast_Text_खण्डम्{fl,  //
                                            "last_seq_seqnum__V <= " + m_libName
                                                + "_protectlib_seq_update(\n",  //
                                            ",\n",  //
                                            "\n);\n"};
            m_seqParamsp->सङ्कलय("handle__V");
            txtp->सङ्कलय(m_seqParamsp);
            m_nbAssignsp = नूतन Ast_Text_खण्डम्{fl};
            txtp->सङ्कलय(m_nbAssignsp);
            txtp->सङ्कलय("end\n\n");
        }

        // Select between combinatorial and sequential results
        सङ्कलय_Comment(txtp, fl, "Select between combinatorial and sequential results");
        txtp->सङ्कलय("always_comb begin\n");
        यदि (m_hasClk) {
            txtp->सङ्कलय("if (last_seq_seqnum__V > last_combo_seqnum__V) begin\n");
            m_seqAssignsp = नूतन Ast_Text_खण्डम्{fl};
            txtp->सङ्कलय(m_seqAssignsp);
            txtp->सङ्कलय("end else begin\n");
            m_comboAssignsp = नूतन Ast_Text_खण्डम्{fl};
            txtp->सङ्कलय(m_comboAssignsp);
            txtp->सङ्कलय("end\n");
        } अन्यथा {
            m_comboAssignsp = नूतन Ast_Text_खण्डम्{fl};
            txtp->सङ्कलय(m_comboAssignsp);
        }
        txtp->सङ्कलय("end\n\n");

        // Final
        txtp->सङ्कलय("final " + m_libName + "_protectlib_final(handle__V);\n\n");

        txtp->सङ्कलय("endmodule\n");
        txtp->सङ्कलय("/*verilator lint_restore*/\n");

        configSection(modp, txtp, fl);

        m_vfilep->tblockp(txtp);
    }

    शून्य castPtr(File_पङ्क्तिः* /*fl*/, Ast_Text_खण्डम्* txtp) {
        txtp->सङ्कलय(m_शिखरम्_Name
                  + "_container* const handlep__V = "  // LCOV_EXCL_LINE  // lcov bug
                    "static_cast<"
                  + m_शिखरम्_Name + "_container*>(vhandlep__V);\n");
    }

    शून्य createCppFile(File_पङ्क्तिः* fl) {
        // Comments
        Ast_Text_खण्डम्* ध्रुव txtp = नूतन Ast_Text_खण्डम्{fl};
        सङ्कलय_Comment(txtp, fl, "Wrapper functions for DPI protected library\n");

        // Includes
        txtp->सङ्कलय("#include \"" + m_शिखरम्_Name + ".h\"\n");
        txtp->सङ्कलय("#include \"verilated_dpi.h\"\n\n");
        txtp->सङ्कलय("#include <cstdio>\n");
        txtp->सङ्कलय("#include <cstdlib>\n\n");

        // Verilated module plus sequence number
        सङ्कलय_Comment(txtp, fl, "Container class to house verilated object and sequence number");
        txtp->सङ्कलय("class " + m_शिखरम्_Name + "_container: public " + m_शिखरम्_Name + " {\n");
        txtp->सङ्कलय("public:\n");
        txtp->सङ्कलय("long long m_seqnum;\n");
        txtp->सङ्कलय(m_शिखरम्_Name + "_container(const char* scopep__V):\n");
        txtp->सङ्कलय(m_शिखरम्_Name + "(scopep__V) {}\n");
        txtp->सङ्कलय("};\n\n");

        // Extern C
        txtp->सङ्कलय("extern \"C\" {\n\n");

        // Hash check
        hashComment(txtp, fl);
        txtp->सङ्कलय("void " + m_libName
                  + "_protectlib_check_hash"
                    "(int protectlib_hash__V) {\n");
        m_cHashValuep = नूतन Ast_Text_खण्डम्{fl, "const int expected_hash__V = ", "", ""};
        txtp->सङ्कलय(m_cHashValuep);
        txtp->सङ्कलय(/**/ "if (protectlib_hash__V != expected_hash__V) {\n");
        txtp->सङ्कलय(/****/ "fprintf(stderr, \"%%Error: cannot use " + m_libName
                  + " library, "
                    "Verliog (%u) and library (%u) hash values do not "
                    "agree\\n\", protectlib_hash__V, expected_hash__V);\n");
        txtp->सङ्कलय(/****/ "std::exit(EXIT_FAILURE);\n");
        txtp->सङ्कलय(/**/ "}\n");
        txtp->सङ्कलय("}\n\n");

        // Initial
        initialComment(txtp, fl);
        txtp->सङ्कलय("void* " + m_libName + "_protectlib_create(const char* scopep__V) {\n");
        txtp->सङ्कलय(/**/ m_शिखरम्_Name + "_container* const handlep__V = new " + m_शिखरम्_Name
                  + "_container{scopep__V};\n");
        txtp->सङ्कलय(/**/ "return handlep__V;\n");
        txtp->सङ्कलय("}\n\n");

        // Updates
        comboComment(txtp, fl);
        m_cComboParamsp = नूतन Ast_Text_खण्डम्{
            fl, "long long " + m_libName + "_protectlib_combo_update(\n", ",\n", "\n) {\n"};
        m_cComboParamsp->सङ्कलय("void* vhandlep__V");
        txtp->सङ्कलय(m_cComboParamsp);
        m_cComboInsp = नूतन Ast_Text_खण्डम्{fl};
        castPtr(fl, m_cComboInsp);
        txtp->सङ्कलय(m_cComboInsp);
        txtp->सङ्कलय("handlep__V->eval();\n");
        m_cComboOutsp = नूतन Ast_Text_खण्डम्{fl};
        txtp->सङ्कलय(m_cComboOutsp);
        txtp->सङ्कलय("return handlep__V->m_seqnum++;\n");
        txtp->सङ्कलय("}\n\n");

        यदि (m_hasClk) {
            seqComment(txtp, fl);
            m_cSeqParamsp = नूतन Ast_Text_खण्डम्{
                fl, "long long " + m_libName + "_protectlib_seq_update(\n", ",\n", "\n) {\n"};
            m_cSeqParamsp->सङ्कलय("void* vhandlep__V");
            txtp->सङ्कलय(m_cSeqParamsp);
            m_cSeqClksp = नूतन Ast_Text_खण्डम्{fl};
            castPtr(fl, m_cSeqClksp);
            txtp->सङ्कलय(m_cSeqClksp);
            txtp->सङ्कलय("handlep__V->eval();\n");
            m_cSeqOutsp = नूतन Ast_Text_खण्डम्{fl};
            txtp->सङ्कलय(m_cSeqOutsp);
            txtp->सङ्कलय("return handlep__V->m_seqnum++;\n");
            txtp->सङ्कलय("}\n\n");
        }

        comboIgnoreComment(txtp, fl);
        m_cIgnoreParamsp = नूतन Ast_Text_खण्डम्{
            fl, "void " + m_libName + "_protectlib_combo_ignore(\n", ",\n", "\n)\n{ }\n\n"};
        m_cIgnoreParamsp->सङ्कलय("void* vhandlep__V");
        txtp->सङ्कलय(m_cIgnoreParamsp);

        // Final
        finalComment(txtp, fl);
        txtp->सङ्कलय("void " + m_libName + "_protectlib_final(void* vhandlep__V) {\n");
        castPtr(fl, txtp);
        txtp->सङ्कलय(/**/ "handlep__V->final();\n");
        txtp->सङ्कलय(/**/ "delete handlep__V;\n");
        txtp->सङ्कलय("}\n\n");

        txtp->सङ्कलय("}\n");
        m_cfilep->tblockp(txtp);
    }

    शून्य दर्शय(AstVar* nodep) override {
        यदि (!nodep->isIO()) प्रत्यागम;
        यदि (restoreInout(nodep)) {
            InoutPorts& ports = m_inouts[nodep->libInoutId()];
            यदि (nodep->is_निवेशः()) {
                ports.inp = nodep;
            } अन्यथा यदि (nodep->libInoutEnable()) {
                ports.enp = nodep;
            } अन्यथा {
                ports.outp = nodep;
            }
        }
        यदि (nodep->direction() == VDirection::निवेशः) {
            यदि (nodep->isPrimaryClock()) {
                UASSERT_OBJ(m_hasClk, nodep, "checkIfClockExists() didn't find this clock");
                handleClock(nodep);
            } अन्यथा {
                handle_दत्त_निवेशः(nodep);
            }
        } अन्यथा {
            // Ref ports were rejected during linking, and tristate lowering split all inouts.
            UASSERT_OBJ(nodep->direction() == VDirection::OUTPUT, nodep,
                        "Unexpected library port direction");
            handleOutput(nodep);
        }
    }

    शून्य दर्शय(Ast_ग्रन्थिः*) override {}

    string c_निवेशः_Connection(AstVar* varp) {
        प्रत्यागम V3Task::नियोजय_Dpi_गन्तव्यम्_आन्तरिकम्("handlep__V->" + varp->name(), varp);
    }

    // The wrapper's module ports are connected by name, so they must use the user's
    // names rather than the hashed form of long names used internally
    स्थिर string sv_पत्तनम्_Name(ध्रुव AstVar* varp) {
        प्रत्यागम Ast_ग्रन्थिः::prettyName(VName::dehash(varp->name()));
    }

    स्थिर AstVar* new_Sv_पत्तनम्_Var(AstVar* varp) {
        AstVar* ध्रुव newVarp = varp->clone_वृक्षः(असत्य);
        newVarp->name(VName::dehash(varp->name()));
        प्रत्यागम newVarp;
    }

    शून्य handleClock(AstVar* varp) {
        handle_निवेशः(varp);
        m_seqPortsp->सङ्कलय(varp->clone_वृक्षः(असत्य));
        यदि (m_hasClk) {
            ध्रुव std::string pname = sv_पत्तनम्_Name(varp);
            m_seqParamsp->सङ्कलय(pname);
            m_clkSensp->सङ्कलय(pname);
        }
        m_cSeqParamsp->सङ्कलय(varp->dpiArgType(सत्य, असत्य));
        m_cSeqClksp->सङ्कलय(c_निवेशः_Connection(varp));
    }

    शून्य handle_दत्त_निवेशः(AstVar* varp) {
        handle_निवेशः(varp);
        m_comboPortsp->सङ्कलय(varp->clone_वृक्षः(असत्य));
        m_comboParamsp->सङ्कलय(sv_पत्तनम्_Name(varp));
        m_comboIgnorePortsp->सङ्कलय(varp->clone_वृक्षः(असत्य));
        यदि (m_hasClk) m_comboIgnoreParamsp->सङ्कलय(sv_पत्तनम्_Name(varp));
        m_cComboParamsp->सङ्कलय(varp->dpiArgType(सत्य, असत्य));
        m_cComboInsp->सङ्कलय(c_निवेशः_Connection(varp));
        m_cIgnoreParamsp->सङ्कलय(varp->dpiArgType(सत्य, असत्य));
    }

    स्थिर तार्किक restoreInout(ध्रुव AstVar* varp) {
        प्रत्यागम varp->libInoutId() && !v3Global.opt.pinsInoutEnables();
    }

    शून्य handle_निवेशः(AstVar* varp) {
        AstVar* ध्रुव portp = new_Sv_पत्तनम्_Var(varp);
        यदि (restoreInout(varp)) {
            portp->direction(VDirection::INOUT);
            portp->varType(VVarType::WIRE);
        }
        m_modPortsp->सङ्कलय(portp);
    }

    शून्य restoreInouts() {
        प्रति (ध्रुव स्वचालित& प्रविष्टिः : m_inouts) {
            ध्रुव InoutPorts& ports = प्रविष्टिः.second;
            UASSERT_OBJ(ports.inp && ports.outp && ports.enp, m_vfilep,
                        "Incomplete lowered library inout");
            ध्रुव string अन्तर् = sv_पत्तनम्_Name(ports.inp);
            ध्रुव string बहिर्गमनम् = ports.outp->prettyName();
            ध्रुव string en = ports.enp->prettyName();
            std::सदिशम्<std::pair<पूर्णाङ्क, पूर्णाङ्क>> dimensions;
            ध्रुव Ast_ग्रन्थिः_DType* dtypep = ports.inp->dtypep()->skipRefp();
            यावत् (ध्रुव Ast_ग्रन्थिः_Array_DType* ध्रुव arrayp = VN_CAST(dtypep, ग्रन्थिः_Array_DType)) {
                // V3Slice currently rejects lowered unpacked inouts before this pass.
                // TODO: Support unpacked inouts once V3Slice supports their lowered form.
                UASSERT_OBJ(!VN_IS(arrayp, UnpackArrayDType), ports.inp,
                            "Unpacked library inout should have been rejected by V3Slice");
                dimensions.emplace_पृष्ठभागः(arrayp->lo(), arrayp->hi());
                dtypep = arrayp->subDTypep()->skipRefp();
            }
            ध्रुव AstBasicDType* ध्रुव basicp = dtypep->basicp();
            UASSERT_OBJ(basicp, ports.inp, "Library inout must have a packed integral type");
            यदि (basicp->isRanged()) dimensions.emplace_पृष्ठभागः(basicp->lo(), basicp->hi());
            यदि (dimensions.रिक्तानि()) {
                m_inoutDeclsp->सङ्कलय("assign " + अन्तर् + " = " + en + " ? " + बहिर्गमनम् + " : 1'bz;\n");
            } अन्यथा {
                string lhs = अन्तर्;
                string rhsEn = en;
                string rhs_बहिर्गमनम् = बहिर्गमनम्;
                प्रति (आकार_t dim = 0; dim < dimensions.आकार(); ++dim) {
                    ध्रुव string सूचकांक
                        = "__VlibInout" + cvt_गन्तव्यम्_Str(प्रविष्टिः.first) + "_" + cvt_गन्तव्यम्_Str(dim);
                    ध्रुव पूर्णाङ्क नीचः = dimensions[dim].first;
                    ध्रुव पूर्णाङ्क उच्चः = dimensions[dim].second;
                    ध्रुव पूर्णाङ्क आकार = उच्चः - नीचः + 1;
                    m_inoutDeclsp->सङ्कलय("for (genvar " + सूचकांक + " = 0; " + सूचकांक + " < "
                                       + cvt_गन्तव्यम्_Str(आकार) + "; ++" + सूचकांक + ") begin\n");
                    ध्रुव string select = "[" + cvt_गन्तव्यम्_Str(नीचः) + " + " + सूचकांक + "]";
                    lhs += select;
                    rhsEn += select;
                    rhs_बहिर्गमनम् += select;
                }
                m_inoutDeclsp->सङ्कलय("assign " + lhs + " = " + rhsEn + " ? " + rhs_बहिर्गमनम्
                                   + " : 1'bz;\n");
                प्रति (आकार_t dim = 0; dim < dimensions.आकार(); ++dim) m_inoutDeclsp->सङ्कलय("end\n");
            }
        }
    }

    स्थिर शून्य सङ्कलय_Local_Variable(Ast_Text_खण्डम्* textp, ध्रुव AstVar* varp, ध्रुव वर्ण* suffix) {
        AstVar* ध्रुव newVarp
            = नूतन AstVar{varp->fileline(), VVarType::VAR, varp->name() + suffix, varp->dtypep()};
        textp->सङ्कलय(newVarp);
    }

    शून्य handleOutput(AstVar* ध्रुव varp) {
        ध्रुव std::string pname = varp->prettyName();
        // Restored inout drivers are local signals and retain their internal names.
        ध्रुव std::string पत्तनम्_Name = restoreInout(varp) ? pname : sv_पत्तनम्_Name(varp);
        यदि (restoreInout(varp)) {
            AstVar* ध्रुव localp = varp->clone_वृक्षः(असत्य);
            localp->direction(VDirection::NONE);
            m_inoutDeclsp->सङ्कलय(localp);
        } अन्यथा {
            m_modPortsp->सङ्कलय(new_Sv_पत्तनम्_Var(varp));
        }
        m_comboPortsp->सङ्कलय(varp->clone_वृक्षः(असत्य));
        m_comboParamsp->सङ्कलय(pname + "_combo__V");
        यदि (m_hasClk) {
            m_seqPortsp->सङ्कलय(varp->clone_वृक्षः(असत्य));
            m_seqParamsp->सङ्कलय(pname + "_tmp__V");
        }

        सङ्कलय_Local_Variable(m_comboDeclsp, varp, "_combo__V");

        यदि (m_hasClk) {
            सङ्कलय_Local_Variable(m_seqDeclsp, varp, "_seq__V");
            सङ्कलय_Local_Variable(m_tmpDeclsp, varp, "_tmp__V");

            m_nbAssignsp->सङ्कलय(pname + "_seq__V <= " + pname + "_tmp__V;\n");
            m_seqAssignsp->सङ्कलय(पत्तनम्_Name + " = " + pname + "_seq__V;\n");
        }
        m_comboAssignsp->सङ्कलय(पत्तनम्_Name + " = " + pname + "_combo__V;\n");
        m_cComboParamsp->सङ्कलय(varp->dpiArgType(सत्य, असत्य));
        m_cComboOutsp->सङ्कलय(V3Task::नियोजय_आन्तरिकम्_गन्तव्यम्_Dpi(varp, सत्य, "", "", "handlep__V->") + "\n");
        यदि (m_hasClk) {
            m_cSeqParamsp->सङ्कलय(varp->dpiArgType(सत्य, असत्य));
            m_cSeqOutsp->सङ्कलय(V3Task::नियोजय_आन्तरिकम्_गन्तव्यम्_Dpi(varp, सत्य, "", "", "handlep__V->")
                             + "\n");
        }
    }

    स्थिर तार्किक checkIfClockExists(ध्रुव Ast_ग्रन्थिः_Module* modp) {
        प्रति (ध्रुव Ast_ग्रन्थिः* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            यदि (ध्रुव AstVar* ध्रुव varp = VN_CAST(stmtp, Var)) {
                यदि (varp->direction() == VDirection::निवेशः && varp->isPrimaryClock()) {
                    प्रत्यागम सत्य;
                }
            }
        }
        प्रत्यागम असत्य;
    }

सार्वजनिक:
    स्पष्ट ProtectVisitor(Ast_ग्रन्थिः* nodep)
        : m_libName{v3Global.opt.libCreate()}
        , m_शिखरम्_Name{v3Global.opt.उपसर्गः()} {
        iterate(nodep);
    }
};

//######################################################################
// ProtectLib class functions

शून्य V3ProtectLib::protect() {
    UINFO(2, __FUNCTION__ << ":");
    ProtectVisitor{v3Global.rootp()};
}
