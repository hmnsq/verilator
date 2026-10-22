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
    ಸಂರಚನಾ InoutPorts final {
        AstVar* inp = nullptr;  // Resolved input value and original public port
        AstVar* outp = nullptr;  // Library drive value
        AstVar* enp = nullptr;  // Per-bit drive enable
    };

    // MEMBERS
    AstVFile* m_vfilep = nullptr;  // DPI-enabled Verilog wrapper
    AstCFile* m_cfilep = nullptr;  // C implementation of DPI functions
    // Verilog text blocks
    AstTextBlock* m_modPortsp = nullptr;  // Module port list
    AstTextBlock* m_comboPortsp = nullptr;  // Combo function port list
    AstTextBlock* m_seqPortsp = nullptr;  // Sequential function port list
    AstTextBlock* m_comboIgnorePortsp = nullptr;  // Combo ignore function port list
    AstTextBlock* m_comboDeclsp = nullptr;  // Combo signal declaration list
    AstTextBlock* m_seqDeclsp = nullptr;  // Sequential signal declaration list
    AstTextBlock* m_tmpDeclsp = nullptr;  // Temporary signal declaration list
    AstTextBlock* m_inoutDeclsp = nullptr;  // Lowered inout signals and tristate drivers
    AstTextBlock* m_hashValuep = nullptr;  // CPP hash value
    AstTextBlock* m_comboParamsp = nullptr;  // Combo function parameter list
    AstTextBlock* m_clkSensp = nullptr;  // Clock sensitivity list
    AstTextBlock* m_comboIgnoreParamsp = nullptr;  // Combo ignore parameter list
    AstTextBlock* m_seqParamsp = nullptr;  // Sequential parameter list
    AstTextBlock* m_nbAssignsp = nullptr;  // Non-blocking assignment list
    AstTextBlock* m_seqAssignsp = nullptr;  // Sequential assignment list
    AstTextBlock* m_comboAssignsp = nullptr;  // Combo assignment list
    // C text blocks
    AstTextBlock* m_cHashValuep = nullptr;  // CPP hash value
    AstTextBlock* m_cComboParamsp = nullptr;  // Combo function parameter list
    AstTextBlock* m_cComboInsp = nullptr;  // Combo input copy list
    AstTextBlock* m_cComboOutsp = nullptr;  // Combo output copy list
    AstTextBlock* m_cSeqParamsp = nullptr;  // Sequential parameter list
    AstTextBlock* m_cSeqClksp = nullptr;  // Sequential clock copy list
    AstTextBlock* m_cSeqOutsp = nullptr;  // Sequential output copy list
    AstTextBlock* m_cIgnoreParamsp = nullptr;  // Combo ignore parameter list
    ಧ್ರುವ string m_libName;
    ಧ್ರುವ string m_ತುದಿ_Name;
    ತಾರ್ಕಿಕ m_found_ತುದಿ = ಅಸತ್ಯ;  // Have seen the top module
    ತಾರ್ಕಿಕ m_hasClk = ಅಸತ್ಯ;  // True if the top module has sequential logic
    std::map<ಪೂರ್ಣಾಙ್ಕ, InoutPorts> m_inouts;  // Lowered ports grouped by original inout

    // VISITORS
    ಶೂನ್ಯ visit(AstNetlist* nodep) override {
        m_vfilep
            = ನೂತನ AstVFile{nodep->fileline(), v3Global.opt.makeDir() + "/" + m_libName + ".sv"};
        nodep->addFilesp(m_vfilep);
        m_cfilep
            = ನೂತನ AstCFile{nodep->fileline(), v3Global.opt.makeDir() + "/" + m_libName + ".cpp"};
        nodep->addFilesp(m_cfilep);
        iterateChildren(nodep);
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು_Module* nodep) override {
        ಯದಿ (!nodep->is_ತುದಿ()) {
            ಪ್ರತ್ಯಾಗಮ;
        } ಅನ್ಯಥಾ {
            UASSERT_OBJ(!m_found_ತುದಿ, nodep, "Multiple root modules");
        }
        File_ಸಾಲು* ಧ್ರುವ fl = nodep->fileline();
        // Need to know the existence of clk before createSvFile()
        m_hasClk = checkIfClockExists(nodep);
        createSvFile(fl, nodep);
        createCppFile(fl);

        iterateChildren(nodep);

        restoreInouts();

        // cppcheck-suppress unreadVariable
        ಧ್ರುವ V3Hash hash = V3Hasher::uncachedHash(m_cfilep);
        m_hashValuep->add(cvtToStr(hash.ಮೌಲ್ಯ()) + ";\n");
        m_cHashValuep->add(cvtToStr(hash.ಮೌಲ್ಯ()) + "U;\n");
        m_found_ತುದಿ = ಸತ್ಯ;
    }

    ಶೂನ್ಯ addComment(AstTextBlock* txtp, File_ಸಾಲು* fl, ಧ್ರುವ string& comment) {
        txtp->add(ನೂತನ AstComment{fl, comment});
    }

    ಶೂನ್ಯ configSection(Ast_ಗಂಟು_Module* /*modp*/, AstTextBlock* txtp, File_ಸಾಲು* /*fl*/) {
        txtp->add("\n`ifdef VERILATOR\n");
        txtp->add("`verilator_config\n");

        txtp->add("verilator_lib -module \"" + m_libName + "\"\n");

        txtp->add("profile_data -hier-dpi \"" + m_libName + "_protectlib_combo_update\" -cost 64'd"
                  + std::to_string(v3Global.currentHierBlockCost()) + "\n");
        txtp->add("profile_data -hier-dpi \"" + m_libName + "_protectlib_seq_update\" -cost 64'd"
                  + std::to_string(v3Global.currentHierBlockCost()) + "\n");

        // Mark remaining NBA protectlib wrapper DPIs as non-hazardous by deliberately forwarding
        // them with non-zero cost.
        // Also, specify hierarchical workers for those tasks for scheduling.
        txtp->add("profile_data -hier-dpi \"" + m_libName
                  + "_protectlib_combo_ignore\" -cost 64'd1\n");

        txtp->add("hier_workers -hier-dpi \"" + m_libName
                  + "_protectlib_combo_update\" -workers 16'd"
                  + std::to_string(V3Control::getHierWorkers(m_libName)) + "\n");
        txtp->add("hier_workers -hier-dpi \"" + m_libName
                  + "_protectlib_seq_update\" -workers 16'd"
                  + std::to_string(V3Control::getHierWorkers(m_libName)) + "\n");
        // No workers for combo_ignore
        txtp->add("`verilog\n");
        txtp->add("`endif\n");
    }

    ಶೂನ್ಯ hashComment(AstTextBlock* txtp, File_ಸಾಲು* fl) {
        addComment(txtp, fl, "Checks to make sure the .sv wrapper and library agree");
    }

    ಶೂನ್ಯ initialComment(AstTextBlock* txtp, File_ಸಾಲು* fl) {
        addComment(txtp, fl, "Creates an instance of the library module at initial-time");
        addComment(txtp, fl, "(one for each instance in the user's design)");
    }

    ಶೂನ್ಯ comboComment(AstTextBlock* txtp, File_ಸಾಲು* fl) {
        addComment(txtp, fl, "Updates all non-clock inputs and retrieves the results");
    }

    ಶೂನ್ಯ seqComment(AstTextBlock* txtp, File_ಸಾಲು* fl) {
        addComment(txtp, fl, "Updates all clocks and retrieves the results");
    }

    ಶೂನ್ಯ comboIgnoreComment(AstTextBlock* txtp, File_ಸಾಲು* fl) {
        addComment(txtp, fl, "Need to convince some simulators that the input to the module");
        addComment(txtp, fl, "must be evaluated before evaluating the clock edge");
    }

    ಶೂನ್ಯ finalComment(AstTextBlock* txtp, File_ಸಾಲು* fl) {
        addComment(txtp, fl, "Evaluates the library module's final process");
    }

    ಶೂನ್ಯ createSvFile(File_ಸಾಲು* fl, Ast_ಗಂಟು_Module* modp) {
        // Comments
        AstTextBlock* ಧ್ರುವ txtp = ನೂತನ AstTextBlock{fl};
        addComment(txtp, fl, "Wrapper module for DPI protected library");
        addComment(txtp, fl,
                   "This module requires lib" + m_libName + ".a or lib" + m_libName
                       + ".so to work");
        addComment(txtp, fl,
                   "See instructions in your simulator for how"
                   " to use DPI libraries\n");

        // Module declaration
        // Wrapper ranges preserve the original RTL, including intentional ascending ranges.
        txtp->add("/*verilator lint_save*/\n");
        txtp->add("/*verilator lint_off ASCRANGE*/\n");
        m_modPortsp = ನೂತನ AstTextBlock{fl, "module " + m_libName + " (\n", ", ", ");\n\n"};
        txtp->add(m_modPortsp);

        // Timescale
        ಯದಿ (v3Global.opt.hierChild() && v3Global.rootp()->timescaleSpecified()) {
            // Emit timescale for hierarchical Verilation if input HDL specifies timespec
            txtp->add("timeunit "s + modp->timeunit().ascii() + ";\n");
            txtp->add("timeprecision "s + +v3Global.rootp()->timeprecision().ascii() + ";\n");
        } ಅನ್ಯಥಾ {
            addComment(txtp, fl,
                       "Precision of submodule"
                       " (commented out to avoid requiring timescale on all modules)");
            addComment(txtp, fl, "timeunit "s + v3Global.rootp()->timeunit().ascii() + ";");
            addComment(txtp, fl,
                       "timeprecision "s + v3Global.rootp()->timeprecision().ascii() + ";\n");
        }

        // DPI declarations
        hashComment(txtp, fl);
        txtp->add("import \"DPI-C\" function void " + m_libName
                  + "_protectlib_check_hash(int protectlib_hash__V);\n\n");
        initialComment(txtp, fl);
        txtp->add("import \"DPI-C\" function chandle " + m_libName
                  + "_protectlib_create(string scope__V);\n\n");
        comboComment(txtp, fl);
        m_comboPortsp = ನೂತನ AstTextBlock{fl,  //
                                         "import \"DPI-C\" function longint " + m_libName
                                             + "_protectlib_combo_update(\n",  //
                                         ", ",  //
                                         ");\n\n"};
        m_comboPortsp->add("chandle handle__V\n");
        txtp->add(m_comboPortsp);
        seqComment(txtp, fl);
        ಯದಿ (m_hasClk) {
            m_seqPortsp = ನೂತನ AstTextBlock{fl,  //
                                           "import \"DPI-C\" function longint " + m_libName
                                               + "_protectlib_seq_update(\n",  //
                                           ", ",  //
                                           ");\n\n"};
            m_seqPortsp->add("chandle handle__V\n");
            txtp->add(m_seqPortsp);
        }
        comboIgnoreComment(txtp, fl);
        m_comboIgnorePortsp = ನೂತನ AstTextBlock{fl,  //
                                               "import \"DPI-C\" function void " + m_libName
                                                   + "_protectlib_combo_ignore(\n",  //
                                               ", ",  //
                                               ");\n\n"};
        m_comboIgnorePortsp->add("chandle handle__V\n");
        txtp->add(m_comboIgnorePortsp);

        finalComment(txtp, fl);
        txtp->add("import \"DPI-C\" function void " + m_libName
                  + "_protectlib_final(chandle handle__V);\n\n");

        // Local variables
        // Avoid tracing handle, as it is not a stable value, so breaks wavediff
        // Likewise other internals aren't interesting to the user
        txtp->add("// verilator tracing_off\n");

        txtp->add("chandle handle__V;\n");
        txtp->add("time last_combo_seqnum__V;\n");
        ಯದಿ (m_hasClk) txtp->add("time last_seq_seqnum__V;\n");
        txtp->add("\n");

        m_comboDeclsp = ನೂತನ AstTextBlock{fl};
        txtp->add(m_comboDeclsp);
        m_seqDeclsp = ನೂತನ AstTextBlock{fl};
        txtp->add(m_seqDeclsp);
        m_tmpDeclsp = ನೂತನ AstTextBlock{fl};
        txtp->add(m_tmpDeclsp);
        m_inoutDeclsp = ನೂತನ AstTextBlock{fl};
        txtp->add(m_inoutDeclsp);

        // CPP hash value
        addComment(txtp, fl, "Hash value to make sure this file and the corresponding");
        addComment(txtp, fl, "library agree");
        m_hashValuep = ನೂತನ AstTextBlock{
            fl, "localparam int protectlib_hash__V /*verilator public_flat_rd*/ = 32'd", "", "\n"};
        txtp->add(m_hashValuep);

        // Initial
        txtp->add("initial begin\n");
        txtp->add(m_libName + "_protectlib_check_hash(protectlib_hash__V);\n");
        txtp->add("if (handle__V == null) handle__V = " + m_libName
                  + "_protectlib_create"
                    "($sformatf(\"%m\"));\n");
        txtp->add("end\n\n");

        // Combinatorial process
        addComment(txtp, fl, "Combinatorially evaluate changes to inputs");
        txtp->add("always_comb begin\n");
        m_comboParamsp = ನೂತನ AstTextBlock{fl,  //
                                          "last_combo_seqnum__V = " + m_libName
                                              + "_protectlib_combo_update(\n",  //
                                          ",\n",  //
                                          "\n);\n"};
        m_comboParamsp->add("handle__V");
        txtp->add(m_comboParamsp);
        txtp->add("end\n\n");

        // Sequential process
        ಯದಿ (m_hasClk) {
            addComment(txtp, fl, "Evaluate clock edges");
            m_clkSensp = ನೂತನ AstTextBlock{fl, "always @(", " or ", ")"};
            txtp->add(m_clkSensp);
            txtp->add(" begin\n");
            m_comboIgnoreParamsp
                = ನೂತನ AstTextBlock{fl,  //
                                   m_libName + "_protectlib_combo_ignore(\n", ",\n", "\n);\n"};
            m_comboIgnoreParamsp->add("handle__V");
            txtp->add(m_comboIgnoreParamsp);
            m_seqParamsp = ನೂತನ AstTextBlock{fl,  //
                                            "last_seq_seqnum__V <= " + m_libName
                                                + "_protectlib_seq_update(\n",  //
                                            ",\n",  //
                                            "\n);\n"};
            m_seqParamsp->add("handle__V");
            txtp->add(m_seqParamsp);
            m_nbAssignsp = ನೂತನ AstTextBlock{fl};
            txtp->add(m_nbAssignsp);
            txtp->add("end\n\n");
        }

        // Select between combinatorial and sequential results
        addComment(txtp, fl, "Select between combinatorial and sequential results");
        txtp->add("always_comb begin\n");
        ಯದಿ (m_hasClk) {
            txtp->add("if (last_seq_seqnum__V > last_combo_seqnum__V) begin\n");
            m_seqAssignsp = ನೂತನ AstTextBlock{fl};
            txtp->add(m_seqAssignsp);
            txtp->add("end else begin\n");
            m_comboAssignsp = ನೂತನ AstTextBlock{fl};
            txtp->add(m_comboAssignsp);
            txtp->add("end\n");
        } ಅನ್ಯಥಾ {
            m_comboAssignsp = ನೂತನ AstTextBlock{fl};
            txtp->add(m_comboAssignsp);
        }
        txtp->add("end\n\n");

        // Final
        txtp->add("final " + m_libName + "_protectlib_final(handle__V);\n\n");

        txtp->add("endmodule\n");
        txtp->add("/*verilator lint_restore*/\n");

        configSection(modp, txtp, fl);

        m_vfilep->tblockp(txtp);
    }

    ಶೂನ್ಯ castPtr(File_ಸಾಲು* /*fl*/, AstTextBlock* txtp) {
        txtp->add(m_ತುದಿ_Name
                  + "_container* const handlep__V = "  // LCOV_EXCL_LINE  // lcov bug
                    "static_cast<"
                  + m_ತುದಿ_Name + "_container*>(vhandlep__V);\n");
    }

    ಶೂನ್ಯ createCppFile(File_ಸಾಲು* fl) {
        // Comments
        AstTextBlock* ಧ್ರುವ txtp = ನೂತನ AstTextBlock{fl};
        addComment(txtp, fl, "Wrapper functions for DPI protected library\n");

        // Includes
        txtp->add("#include \"" + m_ತುದಿ_Name + ".h\"\n");
        txtp->add("#include \"verilated_dpi.h\"\n\n");
        txtp->add("#include <cstdio>\n");
        txtp->add("#include <cstdlib>\n\n");

        // Verilated module plus sequence number
        addComment(txtp, fl, "Container class to house verilated object and sequence number");
        txtp->add("class " + m_ತುದಿ_Name + "_container: public " + m_ತುದಿ_Name + " {\n");
        txtp->add("public:\n");
        txtp->add("long long m_seqnum;\n");
        txtp->add(m_ತುದಿ_Name + "_container(const char* scopep__V):\n");
        txtp->add(m_ತುದಿ_Name + "(scopep__V) {}\n");
        txtp->add("};\n\n");

        // Extern C
        txtp->add("extern \"C\" {\n\n");

        // Hash check
        hashComment(txtp, fl);
        txtp->add("void " + m_libName
                  + "_protectlib_check_hash"
                    "(int protectlib_hash__V) {\n");
        m_cHashValuep = ನೂತನ AstTextBlock{fl, "const int expected_hash__V = ", "", ""};
        txtp->add(m_cHashValuep);
        txtp->add(/**/ "if (protectlib_hash__V != expected_hash__V) {\n");
        txtp->add(/****/ "fprintf(stderr, \"%%Error: cannot use " + m_libName
                  + " library, "
                    "Verliog (%u) and library (%u) hash values do not "
                    "agree\\n\", protectlib_hash__V, expected_hash__V);\n");
        txtp->add(/****/ "std::exit(EXIT_FAILURE);\n");
        txtp->add(/**/ "}\n");
        txtp->add("}\n\n");

        // Initial
        initialComment(txtp, fl);
        txtp->add("void* " + m_libName + "_protectlib_create(const char* scopep__V) {\n");
        txtp->add(/**/ m_ತುದಿ_Name + "_container* const handlep__V = new " + m_ತುದಿ_Name
                  + "_container{scopep__V};\n");
        txtp->add(/**/ "return handlep__V;\n");
        txtp->add("}\n\n");

        // Updates
        comboComment(txtp, fl);
        m_cComboParamsp = ನೂತನ AstTextBlock{
            fl, "long long " + m_libName + "_protectlib_combo_update(\n", ",\n", "\n) {\n"};
        m_cComboParamsp->add("void* vhandlep__V");
        txtp->add(m_cComboParamsp);
        m_cComboInsp = ನೂತನ AstTextBlock{fl};
        castPtr(fl, m_cComboInsp);
        txtp->add(m_cComboInsp);
        txtp->add("handlep__V->eval();\n");
        m_cComboOutsp = ನೂತನ AstTextBlock{fl};
        txtp->add(m_cComboOutsp);
        txtp->add("return handlep__V->m_seqnum++;\n");
        txtp->add("}\n\n");

        ಯದಿ (m_hasClk) {
            seqComment(txtp, fl);
            m_cSeqParamsp = ನೂತನ AstTextBlock{
                fl, "long long " + m_libName + "_protectlib_seq_update(\n", ",\n", "\n) {\n"};
            m_cSeqParamsp->add("void* vhandlep__V");
            txtp->add(m_cSeqParamsp);
            m_cSeqClksp = ನೂತನ AstTextBlock{fl};
            castPtr(fl, m_cSeqClksp);
            txtp->add(m_cSeqClksp);
            txtp->add("handlep__V->eval();\n");
            m_cSeqOutsp = ನೂತನ AstTextBlock{fl};
            txtp->add(m_cSeqOutsp);
            txtp->add("return handlep__V->m_seqnum++;\n");
            txtp->add("}\n\n");
        }

        comboIgnoreComment(txtp, fl);
        m_cIgnoreParamsp = ನೂತನ AstTextBlock{
            fl, "void " + m_libName + "_protectlib_combo_ignore(\n", ",\n", "\n)\n{ }\n\n"};
        m_cIgnoreParamsp->add("void* vhandlep__V");
        txtp->add(m_cIgnoreParamsp);

        // Final
        finalComment(txtp, fl);
        txtp->add("void " + m_libName + "_protectlib_final(void* vhandlep__V) {\n");
        castPtr(fl, txtp);
        txtp->add(/**/ "handlep__V->final();\n");
        txtp->add(/**/ "delete handlep__V;\n");
        txtp->add("}\n\n");

        txtp->add("}\n");
        m_cfilep->tblockp(txtp);
    }

    ಶೂನ್ಯ visit(AstVar* nodep) override {
        ಯದಿ (!nodep->isIO()) ಪ್ರತ್ಯಾಗಮ;
        ಯದಿ (restoreInout(nodep)) {
            InoutPorts& ports = m_inouts[nodep->libInoutId()];
            ಯದಿ (nodep->is_ಒಳಹರಿವು()) {
                ports.inp = nodep;
            } ಅನ್ಯಥಾ ಯದಿ (nodep->libInoutEnable()) {
                ports.enp = nodep;
            } ಅನ್ಯಥಾ {
                ports.outp = nodep;
            }
        }
        ಯದಿ (nodep->direction() == VDirection::ಒಳಹರಿವು) {
            ಯದಿ (nodep->isPrimaryClock()) {
                UASSERT_OBJ(m_hasClk, nodep, "checkIfClockExists() didn't find this clock");
                handleClock(nodep);
            } ಅನ್ಯಥಾ {
                handle_Data_ಒಳಹರಿವು(nodep);
            }
        } ಅನ್ಯಥಾ {
            // Ref ports were rejected during linking, and tristate lowering split all inouts.
            UASSERT_OBJ(nodep->direction() == VDirection::OUTPUT, nodep,
                        "Unexpected library port direction");
            handleOutput(nodep);
        }
    }

    ಶೂನ್ಯ visit(Ast_ಗಂಟು*) override {}

    string c_ಒಳಹರಿವು_Connection(AstVar* varp) {
        ಪ್ರತ್ಯಾಗಮ V3Task::assignDpiToInternal("handlep__V->" + varp->name(), varp);
    }

    // The wrapper's module ports are connected by name, so they must use the user's
    // names rather than the hashed form of long names used internally
    ಸ್ಥಿರ string svPortName(ಧ್ರುವ AstVar* varp) {
        ಪ್ರತ್ಯಾಗಮ Ast_ಗಂಟು::prettyName(VName::dehash(varp->name()));
    }

    ಸ್ಥಿರ AstVar* newSvPortVar(AstVar* varp) {
        AstVar* ಧ್ರುವ newVarp = varp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
        newVarp->name(VName::dehash(varp->name()));
        ಪ್ರತ್ಯಾಗಮ newVarp;
    }

    ಶೂನ್ಯ handleClock(AstVar* varp) {
        handle_ಒಳಹರಿವು(varp);
        m_seqPortsp->add(varp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ));
        ಯದಿ (m_hasClk) {
            ಧ್ರುವ std::string pname = svPortName(varp);
            m_seqParamsp->add(pname);
            m_clkSensp->add(pname);
        }
        m_cSeqParamsp->add(varp->dpiArgType(ಸತ್ಯ, ಅಸತ್ಯ));
        m_cSeqClksp->add(c_ಒಳಹರಿವು_Connection(varp));
    }

    ಶೂನ್ಯ handle_Data_ಒಳಹರಿವು(AstVar* varp) {
        handle_ಒಳಹರಿವು(varp);
        m_comboPortsp->add(varp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ));
        m_comboParamsp->add(svPortName(varp));
        m_comboIgnorePortsp->add(varp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ));
        ಯದಿ (m_hasClk) m_comboIgnoreParamsp->add(svPortName(varp));
        m_cComboParamsp->add(varp->dpiArgType(ಸತ್ಯ, ಅಸತ್ಯ));
        m_cComboInsp->add(c_ಒಳಹರಿವು_Connection(varp));
        m_cIgnoreParamsp->add(varp->dpiArgType(ಸತ್ಯ, ಅಸತ್ಯ));
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ restoreInout(ಧ್ರುವ AstVar* varp) {
        ಪ್ರತ್ಯಾಗಮ varp->libInoutId() && !v3Global.opt.pinsInoutEnables();
    }

    ಶೂನ್ಯ handle_ಒಳಹರಿವು(AstVar* varp) {
        AstVar* ಧ್ರುವ portp = newSvPortVar(varp);
        ಯದಿ (restoreInout(varp)) {
            portp->direction(VDirection::INOUT);
            portp->varType(VVarType::WIRE);
        }
        m_modPortsp->add(portp);
    }

    ಶೂನ್ಯ restoreInouts() {
        ಪ್ರತಿ (ಧ್ರುವ auto& entry : m_inouts) {
            ಧ್ರುವ InoutPorts& ports = entry.second;
            UASSERT_OBJ(ports.inp && ports.outp && ports.enp, m_vfilep,
                        "Incomplete lowered library inout");
            ಧ್ರುವ string in = svPortName(ports.inp);
            ಧ್ರುವ string out = ports.outp->prettyName();
            ಧ್ರುವ string en = ports.enp->prettyName();
            std::vector<std::pair<ಪೂರ್ಣಾಙ್ಕ, ಪೂರ್ಣಾಙ್ಕ>> dimensions;
            ಧ್ರುವ Ast_ಗಂಟು_DType* dtypep = ports.inp->dtypep()->skipRefp();
            ಯಾವತ್ (ಧ್ರುವ Ast_ಗಂಟು_Array_DType* ಧ್ರುವ arrayp = VN_CAST(dtypep, ಗಂಟು_Array_DType)) {
                // V3Slice currently rejects lowered unpacked inouts before this pass.
                // TODO: Support unpacked inouts once V3Slice supports their lowered form.
                UASSERT_OBJ(!VN_IS(arrayp, UnpackArrayDType), ports.inp,
                            "Unpacked library inout should have been rejected by V3Slice");
                dimensions.emplace_back(arrayp->lo(), arrayp->hi());
                dtypep = arrayp->subDTypep()->skipRefp();
            }
            ಧ್ರುವ AstBasicDType* ಧ್ರುವ basicp = dtypep->basicp();
            UASSERT_OBJ(basicp, ports.inp, "Library inout must have a packed integral type");
            ಯದಿ (basicp->isRanged()) dimensions.emplace_back(basicp->lo(), basicp->hi());
            ಯದಿ (dimensions.empty()) {
                m_inoutDeclsp->add("assign " + in + " = " + en + " ? " + out + " : 1'bz;\n");
            } ಅನ್ಯಥಾ {
                string lhs = in;
                string rhsEn = en;
                string rhsOut = out;
                ಪ್ರತಿ (size_t dim = 0; dim < dimensions.size(); ++dim) {
                    ಧ್ರುವ string index
                        = "__VlibInout" + cvtToStr(entry.first) + "_" + cvtToStr(dim);
                    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ ಕೆಳಗಿನ = dimensions[dim].first;
                    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ ಮೇಲಿನ = dimensions[dim].second;
                    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ size = ಮೇಲಿನ - ಕೆಳಗಿನ + 1;
                    m_inoutDeclsp->add("for (genvar " + index + " = 0; " + index + " < "
                                       + cvtToStr(size) + "; ++" + index + ") begin\n");
                    ಧ್ರುವ string select = "[" + cvtToStr(ಕೆಳಗಿನ) + " + " + index + "]";
                    lhs += select;
                    rhsEn += select;
                    rhsOut += select;
                }
                m_inoutDeclsp->add("assign " + lhs + " = " + rhsEn + " ? " + rhsOut
                                   + " : 1'bz;\n");
                ಪ್ರತಿ (size_t dim = 0; dim < dimensions.size(); ++dim) m_inoutDeclsp->add("end\n");
            }
        }
    }

    ಸ್ಥಿರ ಶೂನ್ಯ addLocalVariable(AstTextBlock* textp, ಧ್ರುವ AstVar* varp, ಧ್ರುವ ವರ್ಣ* suffix) {
        AstVar* ಧ್ರುವ newVarp
            = ನೂತನ AstVar{varp->fileline(), VVarType::VAR, varp->name() + suffix, varp->dtypep()};
        textp->add(newVarp);
    }

    ಶೂನ್ಯ handleOutput(AstVar* ಧ್ರುವ varp) {
        ಧ್ರುವ std::string pname = varp->prettyName();
        // Restored inout drivers are local signals and retain their internal names.
        ಧ್ರುವ std::string portName = restoreInout(varp) ? pname : svPortName(varp);
        ಯದಿ (restoreInout(varp)) {
            AstVar* ಧ್ರುವ localp = varp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ);
            localp->direction(VDirection::NONE);
            m_inoutDeclsp->add(localp);
        } ಅನ್ಯಥಾ {
            m_modPortsp->add(newSvPortVar(varp));
        }
        m_comboPortsp->add(varp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ));
        m_comboParamsp->add(pname + "_combo__V");
        ಯದಿ (m_hasClk) {
            m_seqPortsp->add(varp->clone_ವೃಕ್ಷ(ಅಸತ್ಯ));
            m_seqParamsp->add(pname + "_tmp__V");
        }

        addLocalVariable(m_comboDeclsp, varp, "_combo__V");

        ಯದಿ (m_hasClk) {
            addLocalVariable(m_seqDeclsp, varp, "_seq__V");
            addLocalVariable(m_tmpDeclsp, varp, "_tmp__V");

            m_nbAssignsp->add(pname + "_seq__V <= " + pname + "_tmp__V;\n");
            m_seqAssignsp->add(portName + " = " + pname + "_seq__V;\n");
        }
        m_comboAssignsp->add(portName + " = " + pname + "_combo__V;\n");
        m_cComboParamsp->add(varp->dpiArgType(ಸತ್ಯ, ಅಸತ್ಯ));
        m_cComboOutsp->add(V3Task::assignInternalToDpi(varp, ಸತ್ಯ, "", "", "handlep__V->") + "\n");
        ಯದಿ (m_hasClk) {
            m_cSeqParamsp->add(varp->dpiArgType(ಸತ್ಯ, ಅಸತ್ಯ));
            m_cSeqOutsp->add(V3Task::assignInternalToDpi(varp, ಸತ್ಯ, "", "", "handlep__V->")
                             + "\n");
        }
    }

    ಸ್ಥಿರ ತಾರ್ಕಿಕ checkIfClockExists(ಧ್ರುವ Ast_ಗಂಟು_Module* modp) {
        ಪ್ರತಿ (ಧ್ರುವ Ast_ಗಂಟು* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ಯದಿ (ಧ್ರುವ AstVar* ಧ್ರುವ varp = VN_CAST(stmtp, Var)) {
                ಯದಿ (varp->direction() == VDirection::ಒಳಹರಿವು && varp->isPrimaryClock()) {
                    ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
                }
            }
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }

ಸಾರ್ವಜನಿಕ:
    explicit ProtectVisitor(Ast_ಗಂಟು* nodep)
        : m_libName{v3Global.opt.libCreate()}
        , m_ತುದಿ_Name{v3Global.opt.prefix()} {
        iterate(nodep);
    }
};

//######################################################################
// ProtectLib class functions

ಶೂನ್ಯ V3ProtectLib::protect() {
    UINFO(2, __FUNCTION__ << ":");
    ProtectVisitor{v3Global.rootp()};
}
