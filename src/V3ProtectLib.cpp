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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3ProtectLib.h"

#અન્તર્ભાવ "V3Control.h"
#અન્તર્ભાવ "V3Hasher.h"
#અન્તર્ભાવ "V3InstrCount.h"
#અન્તર્ભાવ "V3String.h"
#અન્તર્ભાવ "V3Task.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// ProtectLib top-level visitor

વર્ગ ProtectVisitor final : જાહેર VNVisitor {
    // TYPES
    સંરચના InoutPorts final {
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
    અચલ string m_libName;
    અચલ string m_ટોચ_Name;
    તાર્કિક m_found_ટોચ = ખોટું;  // Have seen the top module
    તાર્કિક m_hasClk = ખોટું;  // True if the top module has sequential logic
    std::map<પૂર્ણાંક, InoutPorts> m_inouts;  // Lowered ports grouped by original inout

    // VISITORS
    શૂન્ય visit(AstNetlist* nodep) override {
        m_vfilep
            = નવું AstVFile{nodep->fileline(), v3Global.opt.makeDir() + "/" + m_libName + ".sv"};
        nodep->addFilesp(m_vfilep);
        m_cfilep
            = નવું AstCFile{nodep->fileline(), v3Global.opt.makeDir() + "/" + m_libName + ".cpp"};
        nodep->addFilesp(m_cfilep);
        iterateChildren(nodep);
    }

    શૂન્ય visit(Ast_ગાંઠ_Module* nodep) override {
        જો (!nodep->is_ટોચ()) {
            પાછા;
        } નહીંતર {
            UASSERT_OBJ(!m_found_ટોચ, nodep, "Multiple root modules");
        }
        File_લીટી* અચલ fl = nodep->fileline();
        // Need to know the existence of clk before createSvFile()
        m_hasClk = checkIfClockExists(nodep);
        createSvFile(fl, nodep);
        createCppFile(fl);

        iterateChildren(nodep);

        restoreInouts();

        // cppcheck-suppress unreadVariable
        અચલ V3Hash hash = V3Hasher::uncachedHash(m_cfilep);
        m_hashValuep->add(cvtToStr(hash.કિંમત()) + ";\n");
        m_cHashValuep->add(cvtToStr(hash.કિંમત()) + "U;\n");
        m_found_ટોચ = સાચું;
    }

    શૂન્ય addComment(AstTextBlock* txtp, File_લીટી* fl, અચલ string& comment) {
        txtp->add(નવું AstComment{fl, comment});
    }

    શૂન્ય configSection(Ast_ગાંઠ_Module* /*modp*/, AstTextBlock* txtp, File_લીટી* /*fl*/) {
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

    શૂન્ય hashComment(AstTextBlock* txtp, File_લીટી* fl) {
        addComment(txtp, fl, "Checks to make sure the .sv wrapper and library agree");
    }

    શૂન્ય initialComment(AstTextBlock* txtp, File_લીટી* fl) {
        addComment(txtp, fl, "Creates an instance of the library module at initial-time");
        addComment(txtp, fl, "(one for each instance in the user's design)");
    }

    શૂન્ય comboComment(AstTextBlock* txtp, File_લીટી* fl) {
        addComment(txtp, fl, "Updates all non-clock inputs and retrieves the results");
    }

    શૂન્ય seqComment(AstTextBlock* txtp, File_લીટી* fl) {
        addComment(txtp, fl, "Updates all clocks and retrieves the results");
    }

    શૂન્ય comboIgnoreComment(AstTextBlock* txtp, File_લીટી* fl) {
        addComment(txtp, fl, "Need to convince some simulators that the input to the module");
        addComment(txtp, fl, "must be evaluated before evaluating the clock edge");
    }

    શૂન્ય finalComment(AstTextBlock* txtp, File_લીટી* fl) {
        addComment(txtp, fl, "Evaluates the library module's final process");
    }

    શૂન્ય createSvFile(File_લીટી* fl, Ast_ગાંઠ_Module* modp) {
        // Comments
        AstTextBlock* અચલ txtp = નવું AstTextBlock{fl};
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
        m_modPortsp = નવું AstTextBlock{fl, "module " + m_libName + " (\n", ", ", ");\n\n"};
        txtp->add(m_modPortsp);

        // Timescale
        જો (v3Global.opt.hierChild() && v3Global.rootp()->timescaleSpecified()) {
            // Emit timescale for hierarchical Verilation if input HDL specifies timespec
            txtp->add("timeunit "s + modp->timeunit().ascii() + ";\n");
            txtp->add("timeprecision "s + +v3Global.rootp()->timeprecision().ascii() + ";\n");
        } નહીંતર {
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
        m_comboPortsp = નવું AstTextBlock{fl,  //
                                         "import \"DPI-C\" function longint " + m_libName
                                             + "_protectlib_combo_update(\n",  //
                                         ", ",  //
                                         ");\n\n"};
        m_comboPortsp->add("chandle handle__V\n");
        txtp->add(m_comboPortsp);
        seqComment(txtp, fl);
        જો (m_hasClk) {
            m_seqPortsp = નવું AstTextBlock{fl,  //
                                           "import \"DPI-C\" function longint " + m_libName
                                               + "_protectlib_seq_update(\n",  //
                                           ", ",  //
                                           ");\n\n"};
            m_seqPortsp->add("chandle handle__V\n");
            txtp->add(m_seqPortsp);
        }
        comboIgnoreComment(txtp, fl);
        m_comboIgnorePortsp = નવું AstTextBlock{fl,  //
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
        જો (m_hasClk) txtp->add("time last_seq_seqnum__V;\n");
        txtp->add("\n");

        m_comboDeclsp = નવું AstTextBlock{fl};
        txtp->add(m_comboDeclsp);
        m_seqDeclsp = નવું AstTextBlock{fl};
        txtp->add(m_seqDeclsp);
        m_tmpDeclsp = નવું AstTextBlock{fl};
        txtp->add(m_tmpDeclsp);
        m_inoutDeclsp = નવું AstTextBlock{fl};
        txtp->add(m_inoutDeclsp);

        // CPP hash value
        addComment(txtp, fl, "Hash value to make sure this file and the corresponding");
        addComment(txtp, fl, "library agree");
        m_hashValuep = નવું AstTextBlock{
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
        m_comboParamsp = નવું AstTextBlock{fl,  //
                                          "last_combo_seqnum__V = " + m_libName
                                              + "_protectlib_combo_update(\n",  //
                                          ",\n",  //
                                          "\n);\n"};
        m_comboParamsp->add("handle__V");
        txtp->add(m_comboParamsp);
        txtp->add("end\n\n");

        // Sequential process
        જો (m_hasClk) {
            addComment(txtp, fl, "Evaluate clock edges");
            m_clkSensp = નવું AstTextBlock{fl, "always @(", " or ", ")"};
            txtp->add(m_clkSensp);
            txtp->add(" begin\n");
            m_comboIgnoreParamsp
                = નવું AstTextBlock{fl,  //
                                   m_libName + "_protectlib_combo_ignore(\n", ",\n", "\n);\n"};
            m_comboIgnoreParamsp->add("handle__V");
            txtp->add(m_comboIgnoreParamsp);
            m_seqParamsp = નવું AstTextBlock{fl,  //
                                            "last_seq_seqnum__V <= " + m_libName
                                                + "_protectlib_seq_update(\n",  //
                                            ",\n",  //
                                            "\n);\n"};
            m_seqParamsp->add("handle__V");
            txtp->add(m_seqParamsp);
            m_nbAssignsp = નવું AstTextBlock{fl};
            txtp->add(m_nbAssignsp);
            txtp->add("end\n\n");
        }

        // Select between combinatorial and sequential results
        addComment(txtp, fl, "Select between combinatorial and sequential results");
        txtp->add("always_comb begin\n");
        જો (m_hasClk) {
            txtp->add("if (last_seq_seqnum__V > last_combo_seqnum__V) begin\n");
            m_seqAssignsp = નવું AstTextBlock{fl};
            txtp->add(m_seqAssignsp);
            txtp->add("end else begin\n");
            m_comboAssignsp = નવું AstTextBlock{fl};
            txtp->add(m_comboAssignsp);
            txtp->add("end\n");
        } નહીંતર {
            m_comboAssignsp = નવું AstTextBlock{fl};
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

    શૂન્ય castPtr(File_લીટી* /*fl*/, AstTextBlock* txtp) {
        txtp->add(m_ટોચ_Name
                  + "_container* const handlep__V = "  // LCOV_EXCL_LINE  // lcov bug
                    "static_cast<"
                  + m_ટોચ_Name + "_container*>(vhandlep__V);\n");
    }

    શૂન્ય createCppFile(File_લીટી* fl) {
        // Comments
        AstTextBlock* અચલ txtp = નવું AstTextBlock{fl};
        addComment(txtp, fl, "Wrapper functions for DPI protected library\n");

        // Includes
        txtp->add("#include \"" + m_ટોચ_Name + ".h\"\n");
        txtp->add("#include \"verilated_dpi.h\"\n\n");
        txtp->add("#include <cstdio>\n");
        txtp->add("#include <cstdlib>\n\n");

        // Verilated module plus sequence number
        addComment(txtp, fl, "Container class to house verilated object and sequence number");
        txtp->add("class " + m_ટોચ_Name + "_container: public " + m_ટોચ_Name + " {\n");
        txtp->add("public:\n");
        txtp->add("long long m_seqnum;\n");
        txtp->add(m_ટોચ_Name + "_container(const char* scopep__V):\n");
        txtp->add(m_ટોચ_Name + "(scopep__V) {}\n");
        txtp->add("};\n\n");

        // Extern C
        txtp->add("extern \"C\" {\n\n");

        // Hash check
        hashComment(txtp, fl);
        txtp->add("void " + m_libName
                  + "_protectlib_check_hash"
                    "(int protectlib_hash__V) {\n");
        m_cHashValuep = નવું AstTextBlock{fl, "const int expected_hash__V = ", "", ""};
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
        txtp->add(/**/ m_ટોચ_Name + "_container* const handlep__V = new " + m_ટોચ_Name
                  + "_container{scopep__V};\n");
        txtp->add(/**/ "return handlep__V;\n");
        txtp->add("}\n\n");

        // Updates
        comboComment(txtp, fl);
        m_cComboParamsp = નવું AstTextBlock{
            fl, "long long " + m_libName + "_protectlib_combo_update(\n", ",\n", "\n) {\n"};
        m_cComboParamsp->add("void* vhandlep__V");
        txtp->add(m_cComboParamsp);
        m_cComboInsp = નવું AstTextBlock{fl};
        castPtr(fl, m_cComboInsp);
        txtp->add(m_cComboInsp);
        txtp->add("handlep__V->eval();\n");
        m_cComboOutsp = નવું AstTextBlock{fl};
        txtp->add(m_cComboOutsp);
        txtp->add("return handlep__V->m_seqnum++;\n");
        txtp->add("}\n\n");

        જો (m_hasClk) {
            seqComment(txtp, fl);
            m_cSeqParamsp = નવું AstTextBlock{
                fl, "long long " + m_libName + "_protectlib_seq_update(\n", ",\n", "\n) {\n"};
            m_cSeqParamsp->add("void* vhandlep__V");
            txtp->add(m_cSeqParamsp);
            m_cSeqClksp = નવું AstTextBlock{fl};
            castPtr(fl, m_cSeqClksp);
            txtp->add(m_cSeqClksp);
            txtp->add("handlep__V->eval();\n");
            m_cSeqOutsp = નવું AstTextBlock{fl};
            txtp->add(m_cSeqOutsp);
            txtp->add("return handlep__V->m_seqnum++;\n");
            txtp->add("}\n\n");
        }

        comboIgnoreComment(txtp, fl);
        m_cIgnoreParamsp = નવું AstTextBlock{
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

    શૂન્ય visit(AstVar* nodep) override {
        જો (!nodep->isIO()) પાછા;
        જો (restoreInout(nodep)) {
            InoutPorts& ports = m_inouts[nodep->libInoutId()];
            જો (nodep->is_આગત()) {
                ports.inp = nodep;
            } નહીંતર જો (nodep->libInoutEnable()) {
                ports.enp = nodep;
            } નહીંતર {
                ports.outp = nodep;
            }
        }
        જો (nodep->direction() == VDirection::આગત) {
            જો (nodep->isPrimaryClock()) {
                UASSERT_OBJ(m_hasClk, nodep, "checkIfClockExists() didn't find this clock");
                handleClock(nodep);
            } નહીંતર {
                handle_Data_આગત(nodep);
            }
        } નહીંતર {
            // Ref ports were rejected during linking, and tristate lowering split all inouts.
            UASSERT_OBJ(nodep->direction() == VDirection::OUTPUT, nodep,
                        "Unexpected library port direction");
            handleOutput(nodep);
        }
    }

    શૂન્ય visit(Ast_ગાંઠ*) override {}

    string c_આગત_Connection(AstVar* varp) {
        પાછા V3Task::assignDpiToInternal("handlep__V->" + varp->name(), varp);
    }

    // The wrapper's module ports are connected by name, so they must use the user's
    // names rather than the hashed form of long names used internally
    સ્થિર string svPortName(અચલ AstVar* varp) {
        પાછા Ast_ગાંઠ::prettyName(VName::dehash(varp->name()));
    }

    સ્થિર AstVar* newSvPortVar(AstVar* varp) {
        AstVar* અચલ newVarp = varp->clone_વૃક્ષ(ખોટું);
        newVarp->name(VName::dehash(varp->name()));
        પાછા newVarp;
    }

    શૂન્ય handleClock(AstVar* varp) {
        handle_આગત(varp);
        m_seqPortsp->add(varp->clone_વૃક્ષ(ખોટું));
        જો (m_hasClk) {
            અચલ std::string pname = svPortName(varp);
            m_seqParamsp->add(pname);
            m_clkSensp->add(pname);
        }
        m_cSeqParamsp->add(varp->dpiArgType(સાચું, ખોટું));
        m_cSeqClksp->add(c_આગત_Connection(varp));
    }

    શૂન્ય handle_Data_આગત(AstVar* varp) {
        handle_આગત(varp);
        m_comboPortsp->add(varp->clone_વૃક્ષ(ખોટું));
        m_comboParamsp->add(svPortName(varp));
        m_comboIgnorePortsp->add(varp->clone_વૃક્ષ(ખોટું));
        જો (m_hasClk) m_comboIgnoreParamsp->add(svPortName(varp));
        m_cComboParamsp->add(varp->dpiArgType(સાચું, ખોટું));
        m_cComboInsp->add(c_આગત_Connection(varp));
        m_cIgnoreParamsp->add(varp->dpiArgType(સાચું, ખોટું));
    }

    સ્થિર તાર્કિક restoreInout(અચલ AstVar* varp) {
        પાછા varp->libInoutId() && !v3Global.opt.pinsInoutEnables();
    }

    શૂન્ય handle_આગત(AstVar* varp) {
        AstVar* અચલ portp = newSvPortVar(varp);
        જો (restoreInout(varp)) {
            portp->direction(VDirection::INOUT);
            portp->varType(VVarType::WIRE);
        }
        m_modPortsp->add(portp);
    }

    શૂન્ય restoreInouts() {
        માટે (અચલ auto& entry : m_inouts) {
            અચલ InoutPorts& ports = entry.second;
            UASSERT_OBJ(ports.inp && ports.outp && ports.enp, m_vfilep,
                        "Incomplete lowered library inout");
            અચલ string in = svPortName(ports.inp);
            અચલ string out = ports.outp->prettyName();
            અચલ string en = ports.enp->prettyName();
            std::vector<std::pair<પૂર્ણાંક, પૂર્ણાંક>> dimensions;
            અચલ Ast_ગાંઠ_DType* dtypep = ports.inp->dtypep()->skipRefp();
            જ્યારે (અચલ Ast_ગાંઠ_Array_DType* અચલ arrayp = VN_CAST(dtypep, ગાંઠ_Array_DType)) {
                // V3Slice currently rejects lowered unpacked inouts before this pass.
                // TODO: Support unpacked inouts once V3Slice supports their lowered form.
                UASSERT_OBJ(!VN_IS(arrayp, UnpackArrayDType), ports.inp,
                            "Unpacked library inout should have been rejected by V3Slice");
                dimensions.emplace_back(arrayp->lo(), arrayp->hi());
                dtypep = arrayp->subDTypep()->skipRefp();
            }
            અચલ AstBasicDType* અચલ basicp = dtypep->basicp();
            UASSERT_OBJ(basicp, ports.inp, "Library inout must have a packed integral type");
            જો (basicp->isRanged()) dimensions.emplace_back(basicp->lo(), basicp->hi());
            જો (dimensions.empty()) {
                m_inoutDeclsp->add("assign " + in + " = " + en + " ? " + out + " : 1'bz;\n");
            } નહીંતર {
                string lhs = in;
                string rhsEn = en;
                string rhsOut = out;
                માટે (size_t dim = 0; dim < dimensions.size(); ++dim) {
                    અચલ string index
                        = "__VlibInout" + cvtToStr(entry.first) + "_" + cvtToStr(dim);
                    અચલ પૂર્ણાંક નીચલું = dimensions[dim].first;
                    અચલ પૂર્ણાંક ઉપલું = dimensions[dim].second;
                    અચલ પૂર્ણાંક size = ઉપલું - નીચલું + 1;
                    m_inoutDeclsp->add("for (genvar " + index + " = 0; " + index + " < "
                                       + cvtToStr(size) + "; ++" + index + ") begin\n");
                    અચલ string select = "[" + cvtToStr(નીચલું) + " + " + index + "]";
                    lhs += select;
                    rhsEn += select;
                    rhsOut += select;
                }
                m_inoutDeclsp->add("assign " + lhs + " = " + rhsEn + " ? " + rhsOut
                                   + " : 1'bz;\n");
                માટે (size_t dim = 0; dim < dimensions.size(); ++dim) m_inoutDeclsp->add("end\n");
            }
        }
    }

    સ્થિર શૂન્ય addLocalVariable(AstTextBlock* textp, અચલ AstVar* varp, અચલ અક્ષર* suffix) {
        AstVar* અચલ newVarp
            = નવું AstVar{varp->fileline(), VVarType::VAR, varp->name() + suffix, varp->dtypep()};
        textp->add(newVarp);
    }

    શૂન્ય handleOutput(AstVar* અચલ varp) {
        અચલ std::string pname = varp->prettyName();
        // Restored inout drivers are local signals and retain their internal names.
        અચલ std::string portName = restoreInout(varp) ? pname : svPortName(varp);
        જો (restoreInout(varp)) {
            AstVar* અચલ localp = varp->clone_વૃક્ષ(ખોટું);
            localp->direction(VDirection::NONE);
            m_inoutDeclsp->add(localp);
        } નહીંતર {
            m_modPortsp->add(newSvPortVar(varp));
        }
        m_comboPortsp->add(varp->clone_વૃક્ષ(ખોટું));
        m_comboParamsp->add(pname + "_combo__V");
        જો (m_hasClk) {
            m_seqPortsp->add(varp->clone_વૃક્ષ(ખોટું));
            m_seqParamsp->add(pname + "_tmp__V");
        }

        addLocalVariable(m_comboDeclsp, varp, "_combo__V");

        જો (m_hasClk) {
            addLocalVariable(m_seqDeclsp, varp, "_seq__V");
            addLocalVariable(m_tmpDeclsp, varp, "_tmp__V");

            m_nbAssignsp->add(pname + "_seq__V <= " + pname + "_tmp__V;\n");
            m_seqAssignsp->add(portName + " = " + pname + "_seq__V;\n");
        }
        m_comboAssignsp->add(portName + " = " + pname + "_combo__V;\n");
        m_cComboParamsp->add(varp->dpiArgType(સાચું, ખોટું));
        m_cComboOutsp->add(V3Task::assignInternalToDpi(varp, સાચું, "", "", "handlep__V->") + "\n");
        જો (m_hasClk) {
            m_cSeqParamsp->add(varp->dpiArgType(સાચું, ખોટું));
            m_cSeqOutsp->add(V3Task::assignInternalToDpi(varp, સાચું, "", "", "handlep__V->")
                             + "\n");
        }
    }

    સ્થિર તાર્કિક checkIfClockExists(અચલ Ast_ગાંઠ_Module* modp) {
        માટે (અચલ Ast_ગાંઠ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            જો (અચલ AstVar* અચલ varp = VN_CAST(stmtp, Var)) {
                જો (varp->direction() == VDirection::આગત && varp->isPrimaryClock()) {
                    પાછા સાચું;
                }
            }
        }
        પાછા ખોટું;
    }

જાહેર:
    explicit ProtectVisitor(Ast_ગાંઠ* nodep)
        : m_libName{v3Global.opt.libCreate()}
        , m_ટોચ_Name{v3Global.opt.prefix()} {
        iterate(nodep);
    }
};

//######################################################################
// ProtectLib class functions

શૂન્ય V3ProtectLib::protect() {
    UINFO(2, __FUNCTION__ << ":");
    ProtectVisitor{v3Global.rootp()};
}
