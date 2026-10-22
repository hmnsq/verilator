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

#அந்தர்பாவ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#அந்தர்பாவ "V3ProtectLib.h"

#அந்தர்பாவ "V3Control.h"
#அந்தர்பாவ "V3Hasher.h"
#அந்தர்பாவ "V3InstrCount.h"
#அந்தர்பாவ "V3String.h"
#அந்தர்பாவ "V3Task.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// ProtectLib top-level visitor

வகுப்பு ProtectVisitor final : பொது VNVisitor {
    // TYPES
    கட்டமைப்பு InoutPorts final {
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
    மாறிலி string m_libName;
    மாறிலி string m_உச்சி_Name;
    தருக்க m_found_உச்சி = பொய்;  // Have seen the top module
    தருக்க m_hasClk = பொய்;  // True if the top module has sequential logic
    std::map<முழு, InoutPorts> m_inouts;  // Lowered ports grouped by original inout

    // VISITORS
    வெறுமை visit(AstNetlist* nodep) override {
        m_vfilep
            = புதிய AstVFile{nodep->fileline(), v3Global.opt.makeDir() + "/" + m_libName + ".sv"};
        nodep->addFilesp(m_vfilep);
        m_cfilep
            = புதிய AstCFile{nodep->fileline(), v3Global.opt.makeDir() + "/" + m_libName + ".cpp"};
        nodep->addFilesp(m_cfilep);
        iterateChildren(nodep);
    }

    வெறுமை visit(Ast_முனை_Module* nodep) override {
        எனில் (!nodep->is_உச்சி()) {
            திரும்பு;
        } இல்லையெனில் {
            UASSERT_OBJ(!m_found_உச்சி, nodep, "Multiple root modules");
        }
        File_வரி* மாறிலி fl = nodep->fileline();
        // Need to know the existence of clk before createSvFile()
        m_hasClk = checkIfClockExists(nodep);
        createSvFile(fl, nodep);
        createCppFile(fl);

        iterateChildren(nodep);

        restoreInouts();

        // cppcheck-suppress unreadVariable
        மாறிலி V3Hash hash = V3Hasher::uncachedHash(m_cfilep);
        m_hashValuep->add(cvtToStr(hash.மதிப்பு()) + ";\n");
        m_cHashValuep->add(cvtToStr(hash.மதிப்பு()) + "U;\n");
        m_found_உச்சி = உண்மை;
    }

    வெறுமை addComment(AstTextBlock* txtp, File_வரி* fl, மாறிலி string& comment) {
        txtp->add(புதிய AstComment{fl, comment});
    }

    வெறுமை configSection(Ast_முனை_Module* /*modp*/, AstTextBlock* txtp, File_வரி* /*fl*/) {
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

    வெறுமை hashComment(AstTextBlock* txtp, File_வரி* fl) {
        addComment(txtp, fl, "Checks to make sure the .sv wrapper and library agree");
    }

    வெறுமை initialComment(AstTextBlock* txtp, File_வரி* fl) {
        addComment(txtp, fl, "Creates an instance of the library module at initial-time");
        addComment(txtp, fl, "(one for each instance in the user's design)");
    }

    வெறுமை comboComment(AstTextBlock* txtp, File_வரி* fl) {
        addComment(txtp, fl, "Updates all non-clock inputs and retrieves the results");
    }

    வெறுமை seqComment(AstTextBlock* txtp, File_வரி* fl) {
        addComment(txtp, fl, "Updates all clocks and retrieves the results");
    }

    வெறுமை comboIgnoreComment(AstTextBlock* txtp, File_வரி* fl) {
        addComment(txtp, fl, "Need to convince some simulators that the input to the module");
        addComment(txtp, fl, "must be evaluated before evaluating the clock edge");
    }

    வெறுமை finalComment(AstTextBlock* txtp, File_வரி* fl) {
        addComment(txtp, fl, "Evaluates the library module's final process");
    }

    வெறுமை createSvFile(File_வரி* fl, Ast_முனை_Module* modp) {
        // Comments
        AstTextBlock* மாறிலி txtp = புதிய AstTextBlock{fl};
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
        m_modPortsp = புதிய AstTextBlock{fl, "module " + m_libName + " (\n", ", ", ");\n\n"};
        txtp->add(m_modPortsp);

        // Timescale
        எனில் (v3Global.opt.hierChild() && v3Global.rootp()->timescaleSpecified()) {
            // Emit timescale for hierarchical Verilation if input HDL specifies timespec
            txtp->add("timeunit "s + modp->timeunit().ascii() + ";\n");
            txtp->add("timeprecision "s + +v3Global.rootp()->timeprecision().ascii() + ";\n");
        } இல்லையெனில் {
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
        m_comboPortsp = புதிய AstTextBlock{fl,  //
                                         "import \"DPI-C\" function longint " + m_libName
                                             + "_protectlib_combo_update(\n",  //
                                         ", ",  //
                                         ");\n\n"};
        m_comboPortsp->add("chandle handle__V\n");
        txtp->add(m_comboPortsp);
        seqComment(txtp, fl);
        எனில் (m_hasClk) {
            m_seqPortsp = புதிய AstTextBlock{fl,  //
                                           "import \"DPI-C\" function longint " + m_libName
                                               + "_protectlib_seq_update(\n",  //
                                           ", ",  //
                                           ");\n\n"};
            m_seqPortsp->add("chandle handle__V\n");
            txtp->add(m_seqPortsp);
        }
        comboIgnoreComment(txtp, fl);
        m_comboIgnorePortsp = புதிய AstTextBlock{fl,  //
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
        எனில் (m_hasClk) txtp->add("time last_seq_seqnum__V;\n");
        txtp->add("\n");

        m_comboDeclsp = புதிய AstTextBlock{fl};
        txtp->add(m_comboDeclsp);
        m_seqDeclsp = புதிய AstTextBlock{fl};
        txtp->add(m_seqDeclsp);
        m_tmpDeclsp = புதிய AstTextBlock{fl};
        txtp->add(m_tmpDeclsp);
        m_inoutDeclsp = புதிய AstTextBlock{fl};
        txtp->add(m_inoutDeclsp);

        // CPP hash value
        addComment(txtp, fl, "Hash value to make sure this file and the corresponding");
        addComment(txtp, fl, "library agree");
        m_hashValuep = புதிய AstTextBlock{
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
        m_comboParamsp = புதிய AstTextBlock{fl,  //
                                          "last_combo_seqnum__V = " + m_libName
                                              + "_protectlib_combo_update(\n",  //
                                          ",\n",  //
                                          "\n);\n"};
        m_comboParamsp->add("handle__V");
        txtp->add(m_comboParamsp);
        txtp->add("end\n\n");

        // Sequential process
        எனில் (m_hasClk) {
            addComment(txtp, fl, "Evaluate clock edges");
            m_clkSensp = புதிய AstTextBlock{fl, "always @(", " or ", ")"};
            txtp->add(m_clkSensp);
            txtp->add(" begin\n");
            m_comboIgnoreParamsp
                = புதிய AstTextBlock{fl,  //
                                   m_libName + "_protectlib_combo_ignore(\n", ",\n", "\n);\n"};
            m_comboIgnoreParamsp->add("handle__V");
            txtp->add(m_comboIgnoreParamsp);
            m_seqParamsp = புதிய AstTextBlock{fl,  //
                                            "last_seq_seqnum__V <= " + m_libName
                                                + "_protectlib_seq_update(\n",  //
                                            ",\n",  //
                                            "\n);\n"};
            m_seqParamsp->add("handle__V");
            txtp->add(m_seqParamsp);
            m_nbAssignsp = புதிய AstTextBlock{fl};
            txtp->add(m_nbAssignsp);
            txtp->add("end\n\n");
        }

        // Select between combinatorial and sequential results
        addComment(txtp, fl, "Select between combinatorial and sequential results");
        txtp->add("always_comb begin\n");
        எனில் (m_hasClk) {
            txtp->add("if (last_seq_seqnum__V > last_combo_seqnum__V) begin\n");
            m_seqAssignsp = புதிய AstTextBlock{fl};
            txtp->add(m_seqAssignsp);
            txtp->add("end else begin\n");
            m_comboAssignsp = புதிய AstTextBlock{fl};
            txtp->add(m_comboAssignsp);
            txtp->add("end\n");
        } இல்லையெனில் {
            m_comboAssignsp = புதிய AstTextBlock{fl};
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

    வெறுமை castPtr(File_வரி* /*fl*/, AstTextBlock* txtp) {
        txtp->add(m_உச்சி_Name
                  + "_container* const handlep__V = "  // LCOV_EXCL_LINE  // lcov bug
                    "static_cast<"
                  + m_உச்சி_Name + "_container*>(vhandlep__V);\n");
    }

    வெறுமை createCppFile(File_வரி* fl) {
        // Comments
        AstTextBlock* மாறிலி txtp = புதிய AstTextBlock{fl};
        addComment(txtp, fl, "Wrapper functions for DPI protected library\n");

        // Includes
        txtp->add("#include \"" + m_உச்சி_Name + ".h\"\n");
        txtp->add("#include \"verilated_dpi.h\"\n\n");
        txtp->add("#include <cstdio>\n");
        txtp->add("#include <cstdlib>\n\n");

        // Verilated module plus sequence number
        addComment(txtp, fl, "Container class to house verilated object and sequence number");
        txtp->add("class " + m_உச்சி_Name + "_container: public " + m_உச்சி_Name + " {\n");
        txtp->add("public:\n");
        txtp->add("long long m_seqnum;\n");
        txtp->add(m_உச்சி_Name + "_container(const char* scopep__V):\n");
        txtp->add(m_உச்சி_Name + "(scopep__V) {}\n");
        txtp->add("};\n\n");

        // Extern C
        txtp->add("extern \"C\" {\n\n");

        // Hash check
        hashComment(txtp, fl);
        txtp->add("void " + m_libName
                  + "_protectlib_check_hash"
                    "(int protectlib_hash__V) {\n");
        m_cHashValuep = புதிய AstTextBlock{fl, "const int expected_hash__V = ", "", ""};
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
        txtp->add(/**/ m_உச்சி_Name + "_container* const handlep__V = new " + m_உச்சி_Name
                  + "_container{scopep__V};\n");
        txtp->add(/**/ "return handlep__V;\n");
        txtp->add("}\n\n");

        // Updates
        comboComment(txtp, fl);
        m_cComboParamsp = புதிய AstTextBlock{
            fl, "long long " + m_libName + "_protectlib_combo_update(\n", ",\n", "\n) {\n"};
        m_cComboParamsp->add("void* vhandlep__V");
        txtp->add(m_cComboParamsp);
        m_cComboInsp = புதிய AstTextBlock{fl};
        castPtr(fl, m_cComboInsp);
        txtp->add(m_cComboInsp);
        txtp->add("handlep__V->eval();\n");
        m_cComboOutsp = புதிய AstTextBlock{fl};
        txtp->add(m_cComboOutsp);
        txtp->add("return handlep__V->m_seqnum++;\n");
        txtp->add("}\n\n");

        எனில் (m_hasClk) {
            seqComment(txtp, fl);
            m_cSeqParamsp = புதிய AstTextBlock{
                fl, "long long " + m_libName + "_protectlib_seq_update(\n", ",\n", "\n) {\n"};
            m_cSeqParamsp->add("void* vhandlep__V");
            txtp->add(m_cSeqParamsp);
            m_cSeqClksp = புதிய AstTextBlock{fl};
            castPtr(fl, m_cSeqClksp);
            txtp->add(m_cSeqClksp);
            txtp->add("handlep__V->eval();\n");
            m_cSeqOutsp = புதிய AstTextBlock{fl};
            txtp->add(m_cSeqOutsp);
            txtp->add("return handlep__V->m_seqnum++;\n");
            txtp->add("}\n\n");
        }

        comboIgnoreComment(txtp, fl);
        m_cIgnoreParamsp = புதிய AstTextBlock{
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

    வெறுமை visit(AstVar* nodep) override {
        எனில் (!nodep->isIO()) திரும்பு;
        எனில் (restoreInout(nodep)) {
            InoutPorts& ports = m_inouts[nodep->libInoutId()];
            எனில் (nodep->is_உள்ளீடு()) {
                ports.inp = nodep;
            } இல்லையெனில் எனில் (nodep->libInoutEnable()) {
                ports.enp = nodep;
            } இல்லையெனில் {
                ports.outp = nodep;
            }
        }
        எனில் (nodep->direction() == VDirection::உள்ளீடு) {
            எனில் (nodep->isPrimaryClock()) {
                UASSERT_OBJ(m_hasClk, nodep, "checkIfClockExists() didn't find this clock");
                handleClock(nodep);
            } இல்லையெனில் {
                handle_Data_உள்ளீடு(nodep);
            }
        } இல்லையெனில் {
            // Ref ports were rejected during linking, and tristate lowering split all inouts.
            UASSERT_OBJ(nodep->direction() == VDirection::OUTPUT, nodep,
                        "Unexpected library port direction");
            handleOutput(nodep);
        }
    }

    வெறுமை visit(Ast_முனை*) override {}

    string c_உள்ளீடு_Connection(AstVar* varp) {
        திரும்பு V3Task::assignDpiToInternal("handlep__V->" + varp->name(), varp);
    }

    // The wrapper's module ports are connected by name, so they must use the user's
    // names rather than the hashed form of long names used internally
    நிலையான string svPortName(மாறிலி AstVar* varp) {
        திரும்பு Ast_முனை::prettyName(VName::dehash(varp->name()));
    }

    நிலையான AstVar* newSvPortVar(AstVar* varp) {
        AstVar* மாறிலி newVarp = varp->clone_மரம்(பொய்);
        newVarp->name(VName::dehash(varp->name()));
        திரும்பு newVarp;
    }

    வெறுமை handleClock(AstVar* varp) {
        handle_உள்ளீடு(varp);
        m_seqPortsp->add(varp->clone_மரம்(பொய்));
        எனில் (m_hasClk) {
            மாறிலி std::string pname = svPortName(varp);
            m_seqParamsp->add(pname);
            m_clkSensp->add(pname);
        }
        m_cSeqParamsp->add(varp->dpiArgType(உண்மை, பொய்));
        m_cSeqClksp->add(c_உள்ளீடு_Connection(varp));
    }

    வெறுமை handle_Data_உள்ளீடு(AstVar* varp) {
        handle_உள்ளீடு(varp);
        m_comboPortsp->add(varp->clone_மரம்(பொய்));
        m_comboParamsp->add(svPortName(varp));
        m_comboIgnorePortsp->add(varp->clone_மரம்(பொய்));
        எனில் (m_hasClk) m_comboIgnoreParamsp->add(svPortName(varp));
        m_cComboParamsp->add(varp->dpiArgType(உண்மை, பொய்));
        m_cComboInsp->add(c_உள்ளீடு_Connection(varp));
        m_cIgnoreParamsp->add(varp->dpiArgType(உண்மை, பொய்));
    }

    நிலையான தருக்க restoreInout(மாறிலி AstVar* varp) {
        திரும்பு varp->libInoutId() && !v3Global.opt.pinsInoutEnables();
    }

    வெறுமை handle_உள்ளீடு(AstVar* varp) {
        AstVar* மாறிலி portp = newSvPortVar(varp);
        எனில் (restoreInout(varp)) {
            portp->direction(VDirection::INOUT);
            portp->varType(VVarType::WIRE);
        }
        m_modPortsp->add(portp);
    }

    வெறுமை restoreInouts() {
        க்கு (மாறிலி auto& entry : m_inouts) {
            மாறிலி InoutPorts& ports = entry.second;
            UASSERT_OBJ(ports.inp && ports.outp && ports.enp, m_vfilep,
                        "Incomplete lowered library inout");
            மாறிலி string in = svPortName(ports.inp);
            மாறிலி string out = ports.outp->prettyName();
            மாறிலி string en = ports.enp->prettyName();
            std::vector<std::pair<முழு, முழு>> dimensions;
            மாறிலி Ast_முனை_DType* dtypep = ports.inp->dtypep()->skipRefp();
            போது (மாறிலி Ast_முனை_Array_DType* மாறிலி arrayp = VN_CAST(dtypep, முனை_Array_DType)) {
                // V3Slice currently rejects lowered unpacked inouts before this pass.
                // TODO: Support unpacked inouts once V3Slice supports their lowered form.
                UASSERT_OBJ(!VN_IS(arrayp, UnpackArrayDType), ports.inp,
                            "Unpacked library inout should have been rejected by V3Slice");
                dimensions.emplace_back(arrayp->lo(), arrayp->hi());
                dtypep = arrayp->subDTypep()->skipRefp();
            }
            மாறிலி AstBasicDType* மாறிலி basicp = dtypep->basicp();
            UASSERT_OBJ(basicp, ports.inp, "Library inout must have a packed integral type");
            எனில் (basicp->isRanged()) dimensions.emplace_back(basicp->lo(), basicp->hi());
            எனில் (dimensions.empty()) {
                m_inoutDeclsp->add("assign " + in + " = " + en + " ? " + out + " : 1'bz;\n");
            } இல்லையெனில் {
                string lhs = in;
                string rhsEn = en;
                string rhsOut = out;
                க்கு (size_t dim = 0; dim < dimensions.size(); ++dim) {
                    மாறிலி string index
                        = "__VlibInout" + cvtToStr(entry.first) + "_" + cvtToStr(dim);
                    மாறிலி முழு கீழ் = dimensions[dim].first;
                    மாறிலி முழு உயர்வு = dimensions[dim].second;
                    மாறிலி முழு size = உயர்வு - கீழ் + 1;
                    m_inoutDeclsp->add("for (genvar " + index + " = 0; " + index + " < "
                                       + cvtToStr(size) + "; ++" + index + ") begin\n");
                    மாறிலி string select = "[" + cvtToStr(கீழ்) + " + " + index + "]";
                    lhs += select;
                    rhsEn += select;
                    rhsOut += select;
                }
                m_inoutDeclsp->add("assign " + lhs + " = " + rhsEn + " ? " + rhsOut
                                   + " : 1'bz;\n");
                க்கு (size_t dim = 0; dim < dimensions.size(); ++dim) m_inoutDeclsp->add("end\n");
            }
        }
    }

    நிலையான வெறுமை addLocalVariable(AstTextBlock* textp, மாறிலி AstVar* varp, மாறிலி எழுத்து* suffix) {
        AstVar* மாறிலி newVarp
            = புதிய AstVar{varp->fileline(), VVarType::VAR, varp->name() + suffix, varp->dtypep()};
        textp->add(newVarp);
    }

    வெறுமை handleOutput(AstVar* மாறிலி varp) {
        மாறிலி std::string pname = varp->prettyName();
        // Restored inout drivers are local signals and retain their internal names.
        மாறிலி std::string portName = restoreInout(varp) ? pname : svPortName(varp);
        எனில் (restoreInout(varp)) {
            AstVar* மாறிலி localp = varp->clone_மரம்(பொய்);
            localp->direction(VDirection::NONE);
            m_inoutDeclsp->add(localp);
        } இல்லையெனில் {
            m_modPortsp->add(newSvPortVar(varp));
        }
        m_comboPortsp->add(varp->clone_மரம்(பொய்));
        m_comboParamsp->add(pname + "_combo__V");
        எனில் (m_hasClk) {
            m_seqPortsp->add(varp->clone_மரம்(பொய்));
            m_seqParamsp->add(pname + "_tmp__V");
        }

        addLocalVariable(m_comboDeclsp, varp, "_combo__V");

        எனில் (m_hasClk) {
            addLocalVariable(m_seqDeclsp, varp, "_seq__V");
            addLocalVariable(m_tmpDeclsp, varp, "_tmp__V");

            m_nbAssignsp->add(pname + "_seq__V <= " + pname + "_tmp__V;\n");
            m_seqAssignsp->add(portName + " = " + pname + "_seq__V;\n");
        }
        m_comboAssignsp->add(portName + " = " + pname + "_combo__V;\n");
        m_cComboParamsp->add(varp->dpiArgType(உண்மை, பொய்));
        m_cComboOutsp->add(V3Task::assignInternalToDpi(varp, உண்மை, "", "", "handlep__V->") + "\n");
        எனில் (m_hasClk) {
            m_cSeqParamsp->add(varp->dpiArgType(உண்மை, பொய்));
            m_cSeqOutsp->add(V3Task::assignInternalToDpi(varp, உண்மை, "", "", "handlep__V->")
                             + "\n");
        }
    }

    நிலையான தருக்க checkIfClockExists(மாறிலி Ast_முனை_Module* modp) {
        க்கு (மாறிலி Ast_முனை* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            எனில் (மாறிலி AstVar* மாறிலி varp = VN_CAST(stmtp, Var)) {
                எனில் (varp->direction() == VDirection::உள்ளீடு && varp->isPrimaryClock()) {
                    திரும்பு உண்மை;
                }
            }
        }
        திரும்பு பொய்;
    }

பொது:
    explicit ProtectVisitor(Ast_முனை* nodep)
        : m_libName{v3Global.opt.libCreate()}
        , m_உச்சி_Name{v3Global.opt.prefix()} {
        iterate(nodep);
    }
};

//######################################################################
// ProtectLib class functions

வெறுமை V3ProtectLib::protect() {
    UINFO(2, __FUNCTION__ << ":");
    ProtectVisitor{v3Global.rootp()};
}
