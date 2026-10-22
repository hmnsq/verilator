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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3ProtectLib.h"

#ଅନ୍ତର୍ଭାଵ "V3Control.h"
#ଅନ୍ତର୍ଭାଵ "V3Hasher.h"
#ଅନ୍ତର୍ଭାଵ "V3InstrCount.h"
#ଅନ୍ତର୍ଭାଵ "V3String.h"
#ଅନ୍ତର୍ଭାଵ "V3Task.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// ProtectLib top-level visitor

ଶ୍ରେଣୀ ProtectVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // TYPES
    ସଂରଚନା InoutPorts final {
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
    ଅଚଳ string m_libName;
    ଅଚଳ string m_ଶୀର୍ଷ_Name;
    ତାର୍କିକ m_found_ଶୀର୍ଷ = ମିଥ୍ୟା;  // Have seen the top module
    ତାର୍କିକ m_hasClk = ମିଥ୍ୟା;  // True if the top module has sequential logic
    std::map<ପୂର୍ଣ୍ଣସଂଖ୍ୟା, InoutPorts> m_inouts;  // Lowered ports grouped by original inout

    // VISITORS
    ଶୂନ୍ୟ visit(AstNetlist* nodep) override {
        m_vfilep
            = ନୂତନ AstVFile{nodep->fileline(), v3Global.opt.makeDir() + "/" + m_libName + ".sv"};
        nodep->addFilesp(m_vfilep);
        m_cfilep
            = ନୂତନ AstCFile{nodep->fileline(), v3Global.opt.makeDir() + "/" + m_libName + ".cpp"};
        nodep->addFilesp(m_cfilep);
        iterateChildren(nodep);
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_Module* nodep) override {
        ଯଦି (!nodep->is_ଶୀର୍ଷ()) {
            ଫେରନ୍ତୁ;
        } ଅନ୍ୟଥା {
            UASSERT_OBJ(!m_found_ଶୀର୍ଷ, nodep, "Multiple root modules");
        }
        File_ଧାଡ଼ି* ଅଚଳ fl = nodep->fileline();
        // Need to know the existence of clk before createSvFile()
        m_hasClk = checkIfClockExists(nodep);
        createSvFile(fl, nodep);
        createCppFile(fl);

        iterateChildren(nodep);

        restoreInouts();

        // cppcheck-suppress unreadVariable
        ଅଚଳ V3Hash hash = V3Hasher::uncachedHash(m_cfilep);
        m_hashValuep->add(cvtToStr(hash.ମୂଲ୍ୟ()) + ";\n");
        m_cHashValuep->add(cvtToStr(hash.ମୂଲ୍ୟ()) + "U;\n");
        m_found_ଶୀର୍ଷ = ସତ୍ୟ;
    }

    ଶୂନ୍ୟ addComment(AstTextBlock* txtp, File_ଧାଡ଼ି* fl, ଅଚଳ string& comment) {
        txtp->add(ନୂତନ AstComment{fl, comment});
    }

    ଶୂନ୍ୟ configSection(Ast_ଗଣ୍ଠି_Module* /*modp*/, AstTextBlock* txtp, File_ଧାଡ଼ି* /*fl*/) {
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

    ଶୂନ୍ୟ hashComment(AstTextBlock* txtp, File_ଧାଡ଼ି* fl) {
        addComment(txtp, fl, "Checks to make sure the .sv wrapper and library agree");
    }

    ଶୂନ୍ୟ initialComment(AstTextBlock* txtp, File_ଧାଡ଼ି* fl) {
        addComment(txtp, fl, "Creates an instance of the library module at initial-time");
        addComment(txtp, fl, "(one for each instance in the user's design)");
    }

    ଶୂନ୍ୟ comboComment(AstTextBlock* txtp, File_ଧାଡ଼ି* fl) {
        addComment(txtp, fl, "Updates all non-clock inputs and retrieves the results");
    }

    ଶୂନ୍ୟ seqComment(AstTextBlock* txtp, File_ଧାଡ଼ି* fl) {
        addComment(txtp, fl, "Updates all clocks and retrieves the results");
    }

    ଶୂନ୍ୟ comboIgnoreComment(AstTextBlock* txtp, File_ଧାଡ଼ି* fl) {
        addComment(txtp, fl, "Need to convince some simulators that the input to the module");
        addComment(txtp, fl, "must be evaluated before evaluating the clock edge");
    }

    ଶୂନ୍ୟ finalComment(AstTextBlock* txtp, File_ଧାଡ଼ି* fl) {
        addComment(txtp, fl, "Evaluates the library module's final process");
    }

    ଶୂନ୍ୟ createSvFile(File_ଧାଡ଼ି* fl, Ast_ଗଣ୍ଠି_Module* modp) {
        // Comments
        AstTextBlock* ଅଚଳ txtp = ନୂତନ AstTextBlock{fl};
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
        m_modPortsp = ନୂତନ AstTextBlock{fl, "module " + m_libName + " (\n", ", ", ");\n\n"};
        txtp->add(m_modPortsp);

        // Timescale
        ଯଦି (v3Global.opt.hierChild() && v3Global.rootp()->timescaleSpecified()) {
            // Emit timescale for hierarchical Verilation if input HDL specifies timespec
            txtp->add("timeunit "s + modp->timeunit().ascii() + ";\n");
            txtp->add("timeprecision "s + +v3Global.rootp()->timeprecision().ascii() + ";\n");
        } ଅନ୍ୟଥା {
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
        m_comboPortsp = ନୂତନ AstTextBlock{fl,  //
                                         "import \"DPI-C\" function longint " + m_libName
                                             + "_protectlib_combo_update(\n",  //
                                         ", ",  //
                                         ");\n\n"};
        m_comboPortsp->add("chandle handle__V\n");
        txtp->add(m_comboPortsp);
        seqComment(txtp, fl);
        ଯଦି (m_hasClk) {
            m_seqPortsp = ନୂତନ AstTextBlock{fl,  //
                                           "import \"DPI-C\" function longint " + m_libName
                                               + "_protectlib_seq_update(\n",  //
                                           ", ",  //
                                           ");\n\n"};
            m_seqPortsp->add("chandle handle__V\n");
            txtp->add(m_seqPortsp);
        }
        comboIgnoreComment(txtp, fl);
        m_comboIgnorePortsp = ନୂତନ AstTextBlock{fl,  //
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
        ଯଦି (m_hasClk) txtp->add("time last_seq_seqnum__V;\n");
        txtp->add("\n");

        m_comboDeclsp = ନୂତନ AstTextBlock{fl};
        txtp->add(m_comboDeclsp);
        m_seqDeclsp = ନୂତନ AstTextBlock{fl};
        txtp->add(m_seqDeclsp);
        m_tmpDeclsp = ନୂତନ AstTextBlock{fl};
        txtp->add(m_tmpDeclsp);
        m_inoutDeclsp = ନୂତନ AstTextBlock{fl};
        txtp->add(m_inoutDeclsp);

        // CPP hash value
        addComment(txtp, fl, "Hash value to make sure this file and the corresponding");
        addComment(txtp, fl, "library agree");
        m_hashValuep = ନୂତନ AstTextBlock{
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
        m_comboParamsp = ନୂତନ AstTextBlock{fl,  //
                                          "last_combo_seqnum__V = " + m_libName
                                              + "_protectlib_combo_update(\n",  //
                                          ",\n",  //
                                          "\n);\n"};
        m_comboParamsp->add("handle__V");
        txtp->add(m_comboParamsp);
        txtp->add("end\n\n");

        // Sequential process
        ଯଦି (m_hasClk) {
            addComment(txtp, fl, "Evaluate clock edges");
            m_clkSensp = ନୂତନ AstTextBlock{fl, "always @(", " or ", ")"};
            txtp->add(m_clkSensp);
            txtp->add(" begin\n");
            m_comboIgnoreParamsp
                = ନୂତନ AstTextBlock{fl,  //
                                   m_libName + "_protectlib_combo_ignore(\n", ",\n", "\n);\n"};
            m_comboIgnoreParamsp->add("handle__V");
            txtp->add(m_comboIgnoreParamsp);
            m_seqParamsp = ନୂତନ AstTextBlock{fl,  //
                                            "last_seq_seqnum__V <= " + m_libName
                                                + "_protectlib_seq_update(\n",  //
                                            ",\n",  //
                                            "\n);\n"};
            m_seqParamsp->add("handle__V");
            txtp->add(m_seqParamsp);
            m_nbAssignsp = ନୂତନ AstTextBlock{fl};
            txtp->add(m_nbAssignsp);
            txtp->add("end\n\n");
        }

        // Select between combinatorial and sequential results
        addComment(txtp, fl, "Select between combinatorial and sequential results");
        txtp->add("always_comb begin\n");
        ଯଦି (m_hasClk) {
            txtp->add("if (last_seq_seqnum__V > last_combo_seqnum__V) begin\n");
            m_seqAssignsp = ନୂତନ AstTextBlock{fl};
            txtp->add(m_seqAssignsp);
            txtp->add("end else begin\n");
            m_comboAssignsp = ନୂତନ AstTextBlock{fl};
            txtp->add(m_comboAssignsp);
            txtp->add("end\n");
        } ଅନ୍ୟଥା {
            m_comboAssignsp = ନୂତନ AstTextBlock{fl};
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

    ଶୂନ୍ୟ castPtr(File_ଧାଡ଼ି* /*fl*/, AstTextBlock* txtp) {
        txtp->add(m_ଶୀର୍ଷ_Name
                  + "_container* const handlep__V = "  // LCOV_EXCL_LINE  // lcov bug
                    "static_cast<"
                  + m_ଶୀର୍ଷ_Name + "_container*>(vhandlep__V);\n");
    }

    ଶୂନ୍ୟ createCppFile(File_ଧାଡ଼ି* fl) {
        // Comments
        AstTextBlock* ଅଚଳ txtp = ନୂତନ AstTextBlock{fl};
        addComment(txtp, fl, "Wrapper functions for DPI protected library\n");

        // Includes
        txtp->add("#include \"" + m_ଶୀର୍ଷ_Name + ".h\"\n");
        txtp->add("#include \"verilated_dpi.h\"\n\n");
        txtp->add("#include <cstdio>\n");
        txtp->add("#include <cstdlib>\n\n");

        // Verilated module plus sequence number
        addComment(txtp, fl, "Container class to house verilated object and sequence number");
        txtp->add("class " + m_ଶୀର୍ଷ_Name + "_container: public " + m_ଶୀର୍ଷ_Name + " {\n");
        txtp->add("public:\n");
        txtp->add("long long m_seqnum;\n");
        txtp->add(m_ଶୀର୍ଷ_Name + "_container(const char* scopep__V):\n");
        txtp->add(m_ଶୀର୍ଷ_Name + "(scopep__V) {}\n");
        txtp->add("};\n\n");

        // Extern C
        txtp->add("extern \"C\" {\n\n");

        // Hash check
        hashComment(txtp, fl);
        txtp->add("void " + m_libName
                  + "_protectlib_check_hash"
                    "(int protectlib_hash__V) {\n");
        m_cHashValuep = ନୂତନ AstTextBlock{fl, "const int expected_hash__V = ", "", ""};
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
        txtp->add(/**/ m_ଶୀର୍ଷ_Name + "_container* const handlep__V = new " + m_ଶୀର୍ଷ_Name
                  + "_container{scopep__V};\n");
        txtp->add(/**/ "return handlep__V;\n");
        txtp->add("}\n\n");

        // Updates
        comboComment(txtp, fl);
        m_cComboParamsp = ନୂତନ AstTextBlock{
            fl, "long long " + m_libName + "_protectlib_combo_update(\n", ",\n", "\n) {\n"};
        m_cComboParamsp->add("void* vhandlep__V");
        txtp->add(m_cComboParamsp);
        m_cComboInsp = ନୂତନ AstTextBlock{fl};
        castPtr(fl, m_cComboInsp);
        txtp->add(m_cComboInsp);
        txtp->add("handlep__V->eval();\n");
        m_cComboOutsp = ନୂତନ AstTextBlock{fl};
        txtp->add(m_cComboOutsp);
        txtp->add("return handlep__V->m_seqnum++;\n");
        txtp->add("}\n\n");

        ଯଦି (m_hasClk) {
            seqComment(txtp, fl);
            m_cSeqParamsp = ନୂତନ AstTextBlock{
                fl, "long long " + m_libName + "_protectlib_seq_update(\n", ",\n", "\n) {\n"};
            m_cSeqParamsp->add("void* vhandlep__V");
            txtp->add(m_cSeqParamsp);
            m_cSeqClksp = ନୂତନ AstTextBlock{fl};
            castPtr(fl, m_cSeqClksp);
            txtp->add(m_cSeqClksp);
            txtp->add("handlep__V->eval();\n");
            m_cSeqOutsp = ନୂତନ AstTextBlock{fl};
            txtp->add(m_cSeqOutsp);
            txtp->add("return handlep__V->m_seqnum++;\n");
            txtp->add("}\n\n");
        }

        comboIgnoreComment(txtp, fl);
        m_cIgnoreParamsp = ନୂତନ AstTextBlock{
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

    ଶୂନ୍ୟ visit(AstVar* nodep) override {
        ଯଦି (!nodep->isIO()) ଫେରନ୍ତୁ;
        ଯଦି (restoreInout(nodep)) {
            InoutPorts& ports = m_inouts[nodep->libInoutId()];
            ଯଦି (nodep->is_ନିବେଶ()) {
                ports.inp = nodep;
            } ଅନ୍ୟଥା ଯଦି (nodep->libInoutEnable()) {
                ports.enp = nodep;
            } ଅନ୍ୟଥା {
                ports.outp = nodep;
            }
        }
        ଯଦି (nodep->direction() == VDirection::ନିବେଶ) {
            ଯଦି (nodep->isPrimaryClock()) {
                UASSERT_OBJ(m_hasClk, nodep, "checkIfClockExists() didn't find this clock");
                handleClock(nodep);
            } ଅନ୍ୟଥା {
                handle_Data_ନିବେଶ(nodep);
            }
        } ଅନ୍ୟଥା {
            // Ref ports were rejected during linking, and tristate lowering split all inouts.
            UASSERT_OBJ(nodep->direction() == VDirection::OUTPUT, nodep,
                        "Unexpected library port direction");
            handleOutput(nodep);
        }
    }

    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି*) override {}

    string c_ନିବେଶ_Connection(AstVar* varp) {
        ଫେରନ୍ତୁ V3Task::assignDpiToInternal("handlep__V->" + varp->name(), varp);
    }

    // The wrapper's module ports are connected by name, so they must use the user's
    // names rather than the hashed form of long names used internally
    ସ୍ଥିର string svPortName(ଅଚଳ AstVar* varp) {
        ଫେରନ୍ତୁ Ast_ଗଣ୍ଠି::prettyName(VName::dehash(varp->name()));
    }

    ସ୍ଥିର AstVar* newSvPortVar(AstVar* varp) {
        AstVar* ଅଚଳ newVarp = varp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
        newVarp->name(VName::dehash(varp->name()));
        ଫେରନ୍ତୁ newVarp;
    }

    ଶୂନ୍ୟ handleClock(AstVar* varp) {
        handle_ନିବେଶ(varp);
        m_seqPortsp->add(varp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା));
        ଯଦି (m_hasClk) {
            ଅଚଳ std::string pname = svPortName(varp);
            m_seqParamsp->add(pname);
            m_clkSensp->add(pname);
        }
        m_cSeqParamsp->add(varp->dpiArgType(ସତ୍ୟ, ମିଥ୍ୟା));
        m_cSeqClksp->add(c_ନିବେଶ_Connection(varp));
    }

    ଶୂନ୍ୟ handle_Data_ନିବେଶ(AstVar* varp) {
        handle_ନିବେଶ(varp);
        m_comboPortsp->add(varp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା));
        m_comboParamsp->add(svPortName(varp));
        m_comboIgnorePortsp->add(varp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା));
        ଯଦି (m_hasClk) m_comboIgnoreParamsp->add(svPortName(varp));
        m_cComboParamsp->add(varp->dpiArgType(ସତ୍ୟ, ମିଥ୍ୟା));
        m_cComboInsp->add(c_ନିବେଶ_Connection(varp));
        m_cIgnoreParamsp->add(varp->dpiArgType(ସତ୍ୟ, ମିଥ୍ୟା));
    }

    ସ୍ଥିର ତାର୍କିକ restoreInout(ଅଚଳ AstVar* varp) {
        ଫେରନ୍ତୁ varp->libInoutId() && !v3Global.opt.pinsInoutEnables();
    }

    ଶୂନ୍ୟ handle_ନିବେଶ(AstVar* varp) {
        AstVar* ଅଚଳ portp = newSvPortVar(varp);
        ଯଦି (restoreInout(varp)) {
            portp->direction(VDirection::INOUT);
            portp->varType(VVarType::WIRE);
        }
        m_modPortsp->add(portp);
    }

    ଶୂନ୍ୟ restoreInouts() {
        ପାଇଁ (ଅଚଳ auto& entry : m_inouts) {
            ଅଚଳ InoutPorts& ports = entry.second;
            UASSERT_OBJ(ports.inp && ports.outp && ports.enp, m_vfilep,
                        "Incomplete lowered library inout");
            ଅଚଳ string in = svPortName(ports.inp);
            ଅଚଳ string out = ports.outp->prettyName();
            ଅଚଳ string en = ports.enp->prettyName();
            std::vector<std::pair<ପୂର୍ଣ୍ଣସଂଖ୍ୟା, ପୂର୍ଣ୍ଣସଂଖ୍ୟା>> dimensions;
            ଅଚଳ Ast_ଗଣ୍ଠି_DType* dtypep = ports.inp->dtypep()->skipRefp();
            ଯେତେବେଳେ (ଅଚଳ Ast_ଗଣ୍ଠି_Array_DType* ଅଚଳ arrayp = VN_CAST(dtypep, ଗଣ୍ଠି_Array_DType)) {
                // V3Slice currently rejects lowered unpacked inouts before this pass.
                // TODO: Support unpacked inouts once V3Slice supports their lowered form.
                UASSERT_OBJ(!VN_IS(arrayp, UnpackArrayDType), ports.inp,
                            "Unpacked library inout should have been rejected by V3Slice");
                dimensions.emplace_back(arrayp->lo(), arrayp->hi());
                dtypep = arrayp->subDTypep()->skipRefp();
            }
            ଅଚଳ AstBasicDType* ଅଚଳ basicp = dtypep->basicp();
            UASSERT_OBJ(basicp, ports.inp, "Library inout must have a packed integral type");
            ଯଦି (basicp->isRanged()) dimensions.emplace_back(basicp->lo(), basicp->hi());
            ଯଦି (dimensions.empty()) {
                m_inoutDeclsp->add("assign " + in + " = " + en + " ? " + out + " : 1'bz;\n");
            } ଅନ୍ୟଥା {
                string lhs = in;
                string rhsEn = en;
                string rhsOut = out;
                ପାଇଁ (size_t dim = 0; dim < dimensions.size(); ++dim) {
                    ଅଚଳ string index
                        = "__VlibInout" + cvtToStr(entry.first) + "_" + cvtToStr(dim);
                    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା ତଳ = dimensions[dim].first;
                    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା ଉପର = dimensions[dim].second;
                    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା size = ଉପର - ତଳ + 1;
                    m_inoutDeclsp->add("for (genvar " + index + " = 0; " + index + " < "
                                       + cvtToStr(size) + "; ++" + index + ") begin\n");
                    ଅଚଳ string select = "[" + cvtToStr(ତଳ) + " + " + index + "]";
                    lhs += select;
                    rhsEn += select;
                    rhsOut += select;
                }
                m_inoutDeclsp->add("assign " + lhs + " = " + rhsEn + " ? " + rhsOut
                                   + " : 1'bz;\n");
                ପାଇଁ (size_t dim = 0; dim < dimensions.size(); ++dim) m_inoutDeclsp->add("end\n");
            }
        }
    }

    ସ୍ଥିର ଶୂନ୍ୟ addLocalVariable(AstTextBlock* textp, ଅଚଳ AstVar* varp, ଅଚଳ ଅକ୍ଷର* suffix) {
        AstVar* ଅଚଳ newVarp
            = ନୂତନ AstVar{varp->fileline(), VVarType::VAR, varp->name() + suffix, varp->dtypep()};
        textp->add(newVarp);
    }

    ଶୂନ୍ୟ handleOutput(AstVar* ଅଚଳ varp) {
        ଅଚଳ std::string pname = varp->prettyName();
        // Restored inout drivers are local signals and retain their internal names.
        ଅଚଳ std::string portName = restoreInout(varp) ? pname : svPortName(varp);
        ଯଦି (restoreInout(varp)) {
            AstVar* ଅଚଳ localp = varp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
            localp->direction(VDirection::NONE);
            m_inoutDeclsp->add(localp);
        } ଅନ୍ୟଥା {
            m_modPortsp->add(newSvPortVar(varp));
        }
        m_comboPortsp->add(varp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା));
        m_comboParamsp->add(pname + "_combo__V");
        ଯଦି (m_hasClk) {
            m_seqPortsp->add(varp->clone_ବୃକ୍ଷ(ମିଥ୍ୟା));
            m_seqParamsp->add(pname + "_tmp__V");
        }

        addLocalVariable(m_comboDeclsp, varp, "_combo__V");

        ଯଦି (m_hasClk) {
            addLocalVariable(m_seqDeclsp, varp, "_seq__V");
            addLocalVariable(m_tmpDeclsp, varp, "_tmp__V");

            m_nbAssignsp->add(pname + "_seq__V <= " + pname + "_tmp__V;\n");
            m_seqAssignsp->add(portName + " = " + pname + "_seq__V;\n");
        }
        m_comboAssignsp->add(portName + " = " + pname + "_combo__V;\n");
        m_cComboParamsp->add(varp->dpiArgType(ସତ୍ୟ, ମିଥ୍ୟା));
        m_cComboOutsp->add(V3Task::assignInternalToDpi(varp, ସତ୍ୟ, "", "", "handlep__V->") + "\n");
        ଯଦି (m_hasClk) {
            m_cSeqParamsp->add(varp->dpiArgType(ସତ୍ୟ, ମିଥ୍ୟା));
            m_cSeqOutsp->add(V3Task::assignInternalToDpi(varp, ସତ୍ୟ, "", "", "handlep__V->")
                             + "\n");
        }
    }

    ସ୍ଥିର ତାର୍କିକ checkIfClockExists(ଅଚଳ Ast_ଗଣ୍ଠି_Module* modp) {
        ପାଇଁ (ଅଚଳ Ast_ଗଣ୍ଠି* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ଯଦି (ଅଚଳ AstVar* ଅଚଳ varp = VN_CAST(stmtp, Var)) {
                ଯଦି (varp->direction() == VDirection::ନିବେଶ && varp->isPrimaryClock()) {
                    ଫେରନ୍ତୁ ସତ୍ୟ;
                }
            }
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }

ସର୍ବସାଧାରଣ:
    explicit ProtectVisitor(Ast_ଗଣ୍ଠି* nodep)
        : m_libName{v3Global.opt.libCreate()}
        , m_ଶୀର୍ଷ_Name{v3Global.opt.prefix()} {
        iterate(nodep);
    }
};

//######################################################################
// ProtectLib class functions

ଶୂନ୍ୟ V3ProtectLib::protect() {
    UINFO(2, __FUNCTION__ << ":");
    ProtectVisitor{v3Global.rootp()};
}
