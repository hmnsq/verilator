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
    ध्रुव string m_libName;
    ध्रुव string m_शीर्ष_Name;
    तार्किक m_found_शीर्ष = असत्य;  // Have seen the top module
    तार्किक m_hasClk = असत्य;  // True if the top module has sequential logic
    std::map<पूर्णाङ्क, InoutPorts> m_inouts;  // Lowered ports grouped by original inout

    // VISITORS
    शून्य visit(AstNetlist* nodep) override {
        m_vfilep
            = नूतन AstVFile{nodep->fileline(), v3Global.opt.makeDir() + "/" + m_libName + ".sv"};
        nodep->addFilesp(m_vfilep);
        m_cfilep
            = नूतन AstCFile{nodep->fileline(), v3Global.opt.makeDir() + "/" + m_libName + ".cpp"};
        nodep->addFilesp(m_cfilep);
        iterateChildren(nodep);
    }

    शून्य visit(Ast_गाँठ_Module* nodep) override {
        यदि (!nodep->is_शीर्ष()) {
            प्रत्यागम;
        } अन्यथा {
            UASSERT_OBJ(!m_found_शीर्ष, nodep, "Multiple root modules");
        }
        File_पंक्ति* ध्रुव fl = nodep->fileline();
        // Need to know the existence of clk before createSvFile()
        m_hasClk = checkIfClockExists(nodep);
        createSvFile(fl, nodep);
        createCppFile(fl);

        iterateChildren(nodep);

        restoreInouts();

        // cppcheck-suppress unreadVariable
        ध्रुव V3Hash hash = V3Hasher::uncachedHash(m_cfilep);
        m_hashValuep->add(cvtToStr(hash.मूल्य()) + ";\n");
        m_cHashValuep->add(cvtToStr(hash.मूल्य()) + "U;\n");
        m_found_शीर्ष = सत्य;
    }

    शून्य addComment(AstTextBlock* txtp, File_पंक्ति* fl, ध्रुव string& comment) {
        txtp->add(नूतन AstComment{fl, comment});
    }

    शून्य configSection(Ast_गाँठ_Module* /*modp*/, AstTextBlock* txtp, File_पंक्ति* /*fl*/) {
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

    शून्य hashComment(AstTextBlock* txtp, File_पंक्ति* fl) {
        addComment(txtp, fl, "Checks to make sure the .sv wrapper and library agree");
    }

    शून्य initialComment(AstTextBlock* txtp, File_पंक्ति* fl) {
        addComment(txtp, fl, "Creates an instance of the library module at initial-time");
        addComment(txtp, fl, "(one for each instance in the user's design)");
    }

    शून्य comboComment(AstTextBlock* txtp, File_पंक्ति* fl) {
        addComment(txtp, fl, "Updates all non-clock inputs and retrieves the results");
    }

    शून्य seqComment(AstTextBlock* txtp, File_पंक्ति* fl) {
        addComment(txtp, fl, "Updates all clocks and retrieves the results");
    }

    शून्य comboIgnoreComment(AstTextBlock* txtp, File_पंक्ति* fl) {
        addComment(txtp, fl, "Need to convince some simulators that the input to the module");
        addComment(txtp, fl, "must be evaluated before evaluating the clock edge");
    }

    शून्य finalComment(AstTextBlock* txtp, File_पंक्ति* fl) {
        addComment(txtp, fl, "Evaluates the library module's final process");
    }

    शून्य createSvFile(File_पंक्ति* fl, Ast_गाँठ_Module* modp) {
        // Comments
        AstTextBlock* ध्रुव txtp = नूतन AstTextBlock{fl};
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
        m_modPortsp = नूतन AstTextBlock{fl, "module " + m_libName + " (\n", ", ", ");\n\n"};
        txtp->add(m_modPortsp);

        // Timescale
        यदि (v3Global.opt.hierChild() && v3Global.rootp()->timescaleSpecified()) {
            // Emit timescale for hierarchical Verilation if input HDL specifies timespec
            txtp->add("timeunit "s + modp->timeunit().ascii() + ";\n");
            txtp->add("timeprecision "s + +v3Global.rootp()->timeprecision().ascii() + ";\n");
        } अन्यथा {
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
        m_comboPortsp = नूतन AstTextBlock{fl,  //
                                         "import \"DPI-C\" function longint " + m_libName
                                             + "_protectlib_combo_update(\n",  //
                                         ", ",  //
                                         ");\n\n"};
        m_comboPortsp->add("chandle handle__V\n");
        txtp->add(m_comboPortsp);
        seqComment(txtp, fl);
        यदि (m_hasClk) {
            m_seqPortsp = नूतन AstTextBlock{fl,  //
                                           "import \"DPI-C\" function longint " + m_libName
                                               + "_protectlib_seq_update(\n",  //
                                           ", ",  //
                                           ");\n\n"};
            m_seqPortsp->add("chandle handle__V\n");
            txtp->add(m_seqPortsp);
        }
        comboIgnoreComment(txtp, fl);
        m_comboIgnorePortsp = नूतन AstTextBlock{fl,  //
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
        यदि (m_hasClk) txtp->add("time last_seq_seqnum__V;\n");
        txtp->add("\n");

        m_comboDeclsp = नूतन AstTextBlock{fl};
        txtp->add(m_comboDeclsp);
        m_seqDeclsp = नूतन AstTextBlock{fl};
        txtp->add(m_seqDeclsp);
        m_tmpDeclsp = नूतन AstTextBlock{fl};
        txtp->add(m_tmpDeclsp);
        m_inoutDeclsp = नूतन AstTextBlock{fl};
        txtp->add(m_inoutDeclsp);

        // CPP hash value
        addComment(txtp, fl, "Hash value to make sure this file and the corresponding");
        addComment(txtp, fl, "library agree");
        m_hashValuep = नूतन AstTextBlock{
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
        m_comboParamsp = नूतन AstTextBlock{fl,  //
                                          "last_combo_seqnum__V = " + m_libName
                                              + "_protectlib_combo_update(\n",  //
                                          ",\n",  //
                                          "\n);\n"};
        m_comboParamsp->add("handle__V");
        txtp->add(m_comboParamsp);
        txtp->add("end\n\n");

        // Sequential process
        यदि (m_hasClk) {
            addComment(txtp, fl, "Evaluate clock edges");
            m_clkSensp = नूतन AstTextBlock{fl, "always @(", " or ", ")"};
            txtp->add(m_clkSensp);
            txtp->add(" begin\n");
            m_comboIgnoreParamsp
                = नूतन AstTextBlock{fl,  //
                                   m_libName + "_protectlib_combo_ignore(\n", ",\n", "\n);\n"};
            m_comboIgnoreParamsp->add("handle__V");
            txtp->add(m_comboIgnoreParamsp);
            m_seqParamsp = नूतन AstTextBlock{fl,  //
                                            "last_seq_seqnum__V <= " + m_libName
                                                + "_protectlib_seq_update(\n",  //
                                            ",\n",  //
                                            "\n);\n"};
            m_seqParamsp->add("handle__V");
            txtp->add(m_seqParamsp);
            m_nbAssignsp = नूतन AstTextBlock{fl};
            txtp->add(m_nbAssignsp);
            txtp->add("end\n\n");
        }

        // Select between combinatorial and sequential results
        addComment(txtp, fl, "Select between combinatorial and sequential results");
        txtp->add("always_comb begin\n");
        यदि (m_hasClk) {
            txtp->add("if (last_seq_seqnum__V > last_combo_seqnum__V) begin\n");
            m_seqAssignsp = नूतन AstTextBlock{fl};
            txtp->add(m_seqAssignsp);
            txtp->add("end else begin\n");
            m_comboAssignsp = नूतन AstTextBlock{fl};
            txtp->add(m_comboAssignsp);
            txtp->add("end\n");
        } अन्यथा {
            m_comboAssignsp = नूतन AstTextBlock{fl};
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

    शून्य castPtr(File_पंक्ति* /*fl*/, AstTextBlock* txtp) {
        txtp->add(m_शीर्ष_Name
                  + "_container* const handlep__V = "  // LCOV_EXCL_LINE  // lcov bug
                    "static_cast<"
                  + m_शीर्ष_Name + "_container*>(vhandlep__V);\n");
    }

    शून्य createCppFile(File_पंक्ति* fl) {
        // Comments
        AstTextBlock* ध्रुव txtp = नूतन AstTextBlock{fl};
        addComment(txtp, fl, "Wrapper functions for DPI protected library\n");

        // Includes
        txtp->add("#include \"" + m_शीर्ष_Name + ".h\"\n");
        txtp->add("#include \"verilated_dpi.h\"\n\n");
        txtp->add("#include <cstdio>\n");
        txtp->add("#include <cstdlib>\n\n");

        // Verilated module plus sequence number
        addComment(txtp, fl, "Container class to house verilated object and sequence number");
        txtp->add("class " + m_शीर्ष_Name + "_container: public " + m_शीर्ष_Name + " {\n");
        txtp->add("public:\n");
        txtp->add("long long m_seqnum;\n");
        txtp->add(m_शीर्ष_Name + "_container(const char* scopep__V):\n");
        txtp->add(m_शीर्ष_Name + "(scopep__V) {}\n");
        txtp->add("};\n\n");

        // Extern C
        txtp->add("extern \"C\" {\n\n");

        // Hash check
        hashComment(txtp, fl);
        txtp->add("void " + m_libName
                  + "_protectlib_check_hash"
                    "(int protectlib_hash__V) {\n");
        m_cHashValuep = नूतन AstTextBlock{fl, "const int expected_hash__V = ", "", ""};
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
        txtp->add(/**/ m_शीर्ष_Name + "_container* const handlep__V = new " + m_शीर्ष_Name
                  + "_container{scopep__V};\n");
        txtp->add(/**/ "return handlep__V;\n");
        txtp->add("}\n\n");

        // Updates
        comboComment(txtp, fl);
        m_cComboParamsp = नूतन AstTextBlock{
            fl, "long long " + m_libName + "_protectlib_combo_update(\n", ",\n", "\n) {\n"};
        m_cComboParamsp->add("void* vhandlep__V");
        txtp->add(m_cComboParamsp);
        m_cComboInsp = नूतन AstTextBlock{fl};
        castPtr(fl, m_cComboInsp);
        txtp->add(m_cComboInsp);
        txtp->add("handlep__V->eval();\n");
        m_cComboOutsp = नूतन AstTextBlock{fl};
        txtp->add(m_cComboOutsp);
        txtp->add("return handlep__V->m_seqnum++;\n");
        txtp->add("}\n\n");

        यदि (m_hasClk) {
            seqComment(txtp, fl);
            m_cSeqParamsp = नूतन AstTextBlock{
                fl, "long long " + m_libName + "_protectlib_seq_update(\n", ",\n", "\n) {\n"};
            m_cSeqParamsp->add("void* vhandlep__V");
            txtp->add(m_cSeqParamsp);
            m_cSeqClksp = नूतन AstTextBlock{fl};
            castPtr(fl, m_cSeqClksp);
            txtp->add(m_cSeqClksp);
            txtp->add("handlep__V->eval();\n");
            m_cSeqOutsp = नूतन AstTextBlock{fl};
            txtp->add(m_cSeqOutsp);
            txtp->add("return handlep__V->m_seqnum++;\n");
            txtp->add("}\n\n");
        }

        comboIgnoreComment(txtp, fl);
        m_cIgnoreParamsp = नूतन AstTextBlock{
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

    शून्य visit(AstVar* nodep) override {
        यदि (!nodep->isIO()) प्रत्यागम;
        यदि (restoreInout(nodep)) {
            InoutPorts& ports = m_inouts[nodep->libInoutId()];
            यदि (nodep->is_निवेश()) {
                ports.inp = nodep;
            } अन्यथा यदि (nodep->libInoutEnable()) {
                ports.enp = nodep;
            } अन्यथा {
                ports.outp = nodep;
            }
        }
        यदि (nodep->direction() == VDirection::निवेश) {
            यदि (nodep->isPrimaryClock()) {
                UASSERT_OBJ(m_hasClk, nodep, "checkIfClockExists() didn't find this clock");
                handleClock(nodep);
            } अन्यथा {
                handle_Data_निवेश(nodep);
            }
        } अन्यथा {
            // Ref ports were rejected during linking, and tristate lowering split all inouts.
            UASSERT_OBJ(nodep->direction() == VDirection::OUTPUT, nodep,
                        "Unexpected library port direction");
            handleOutput(nodep);
        }
    }

    शून्य visit(Ast_गाँठ*) override {}

    string c_निवेश_Connection(AstVar* varp) {
        प्रत्यागम V3Task::assignDpiToInternal("handlep__V->" + varp->name(), varp);
    }

    // The wrapper's module ports are connected by name, so they must use the user's
    // names rather than the hashed form of long names used internally
    स्थिर string svPortName(ध्रुव AstVar* varp) {
        प्रत्यागम Ast_गाँठ::prettyName(VName::dehash(varp->name()));
    }

    स्थिर AstVar* newSvPortVar(AstVar* varp) {
        AstVar* ध्रुव newVarp = varp->clone_वृक्ष(असत्य);
        newVarp->name(VName::dehash(varp->name()));
        प्रत्यागम newVarp;
    }

    शून्य handleClock(AstVar* varp) {
        handle_निवेश(varp);
        m_seqPortsp->add(varp->clone_वृक्ष(असत्य));
        यदि (m_hasClk) {
            ध्रुव std::string pname = svPortName(varp);
            m_seqParamsp->add(pname);
            m_clkSensp->add(pname);
        }
        m_cSeqParamsp->add(varp->dpiArgType(सत्य, असत्य));
        m_cSeqClksp->add(c_निवेश_Connection(varp));
    }

    शून्य handle_Data_निवेश(AstVar* varp) {
        handle_निवेश(varp);
        m_comboPortsp->add(varp->clone_वृक्ष(असत्य));
        m_comboParamsp->add(svPortName(varp));
        m_comboIgnorePortsp->add(varp->clone_वृक्ष(असत्य));
        यदि (m_hasClk) m_comboIgnoreParamsp->add(svPortName(varp));
        m_cComboParamsp->add(varp->dpiArgType(सत्य, असत्य));
        m_cComboInsp->add(c_निवेश_Connection(varp));
        m_cIgnoreParamsp->add(varp->dpiArgType(सत्य, असत्य));
    }

    स्थिर तार्किक restoreInout(ध्रुव AstVar* varp) {
        प्रत्यागम varp->libInoutId() && !v3Global.opt.pinsInoutEnables();
    }

    शून्य handle_निवेश(AstVar* varp) {
        AstVar* ध्रुव portp = newSvPortVar(varp);
        यदि (restoreInout(varp)) {
            portp->direction(VDirection::INOUT);
            portp->varType(VVarType::WIRE);
        }
        m_modPortsp->add(portp);
    }

    शून्य restoreInouts() {
        प्रति (ध्रुव स्वतः& entry : m_inouts) {
            ध्रुव InoutPorts& ports = entry.second;
            UASSERT_OBJ(ports.inp && ports.outp && ports.enp, m_vfilep,
                        "Incomplete lowered library inout");
            ध्रुव string in = svPortName(ports.inp);
            ध्रुव string out = ports.outp->prettyName();
            ध्रुव string en = ports.enp->prettyName();
            std::vector<std::pair<पूर्णाङ्क, पूर्णाङ्क>> dimensions;
            ध्रुव Ast_गाँठ_DType* dtypep = ports.inp->dtypep()->skipRefp();
            यावत् (ध्रुव Ast_गाँठ_Array_DType* ध्रुव arrayp = VN_CAST(dtypep, गाँठ_Array_DType)) {
                // V3Slice currently rejects lowered unpacked inouts before this pass.
                // TODO: Support unpacked inouts once V3Slice supports their lowered form.
                UASSERT_OBJ(!VN_IS(arrayp, UnpackArrayDType), ports.inp,
                            "Unpacked library inout should have been rejected by V3Slice");
                dimensions.emplace_back(arrayp->lo(), arrayp->hi());
                dtypep = arrayp->subDTypep()->skipRefp();
            }
            ध्रुव AstBasicDType* ध्रुव basicp = dtypep->basicp();
            UASSERT_OBJ(basicp, ports.inp, "Library inout must have a packed integral type");
            यदि (basicp->isRanged()) dimensions.emplace_back(basicp->lo(), basicp->hi());
            यदि (dimensions.empty()) {
                m_inoutDeclsp->add("assign " + in + " = " + en + " ? " + out + " : 1'bz;\n");
            } अन्यथा {
                string lhs = in;
                string rhsEn = en;
                string rhsOut = out;
                प्रति (माप_प्रकार dim = 0; dim < dimensions.size(); ++dim) {
                    ध्रुव string index
                        = "__VlibInout" + cvtToStr(entry.first) + "_" + cvtToStr(dim);
                    ध्रुव पूर्णाङ्क निचला = dimensions[dim].first;
                    ध्रुव पूर्णाङ्क ऊपरी = dimensions[dim].second;
                    ध्रुव पूर्णाङ्क size = ऊपरी - निचला + 1;
                    m_inoutDeclsp->add("for (genvar " + index + " = 0; " + index + " < "
                                       + cvtToStr(size) + "; ++" + index + ") begin\n");
                    ध्रुव string select = "[" + cvtToStr(निचला) + " + " + index + "]";
                    lhs += select;
                    rhsEn += select;
                    rhsOut += select;
                }
                m_inoutDeclsp->add("assign " + lhs + " = " + rhsEn + " ? " + rhsOut
                                   + " : 1'bz;\n");
                प्रति (माप_प्रकार dim = 0; dim < dimensions.size(); ++dim) m_inoutDeclsp->add("end\n");
            }
        }
    }

    स्थिर शून्य addLocalVariable(AstTextBlock* textp, ध्रुव AstVar* varp, ध्रुव वर्ण* suffix) {
        AstVar* ध्रुव newVarp
            = नूतन AstVar{varp->fileline(), VVarType::VAR, varp->name() + suffix, varp->dtypep()};
        textp->add(newVarp);
    }

    शून्य handleOutput(AstVar* ध्रुव varp) {
        ध्रुव std::string pname = varp->prettyName();
        // Restored inout drivers are local signals and retain their internal names.
        ध्रुव std::string portName = restoreInout(varp) ? pname : svPortName(varp);
        यदि (restoreInout(varp)) {
            AstVar* ध्रुव localp = varp->clone_वृक्ष(असत्य);
            localp->direction(VDirection::NONE);
            m_inoutDeclsp->add(localp);
        } अन्यथा {
            m_modPortsp->add(newSvPortVar(varp));
        }
        m_comboPortsp->add(varp->clone_वृक्ष(असत्य));
        m_comboParamsp->add(pname + "_combo__V");
        यदि (m_hasClk) {
            m_seqPortsp->add(varp->clone_वृक्ष(असत्य));
            m_seqParamsp->add(pname + "_tmp__V");
        }

        addLocalVariable(m_comboDeclsp, varp, "_combo__V");

        यदि (m_hasClk) {
            addLocalVariable(m_seqDeclsp, varp, "_seq__V");
            addLocalVariable(m_tmpDeclsp, varp, "_tmp__V");

            m_nbAssignsp->add(pname + "_seq__V <= " + pname + "_tmp__V;\n");
            m_seqAssignsp->add(portName + " = " + pname + "_seq__V;\n");
        }
        m_comboAssignsp->add(portName + " = " + pname + "_combo__V;\n");
        m_cComboParamsp->add(varp->dpiArgType(सत्य, असत्य));
        m_cComboOutsp->add(V3Task::assignInternalToDpi(varp, सत्य, "", "", "handlep__V->") + "\n");
        यदि (m_hasClk) {
            m_cSeqParamsp->add(varp->dpiArgType(सत्य, असत्य));
            m_cSeqOutsp->add(V3Task::assignInternalToDpi(varp, सत्य, "", "", "handlep__V->")
                             + "\n");
        }
    }

    स्थिर तार्किक checkIfClockExists(ध्रुव Ast_गाँठ_Module* modp) {
        प्रति (ध्रुव Ast_गाँठ* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            यदि (ध्रुव AstVar* ध्रुव varp = VN_CAST(stmtp, Var)) {
                यदि (varp->direction() == VDirection::निवेश && varp->isPrimaryClock()) {
                    प्रत्यागम सत्य;
                }
            }
        }
        प्रत्यागम असत्य;
    }

सार्वजनिक:
    explicit ProtectVisitor(Ast_गाँठ* nodep)
        : m_libName{v3Global.opt.libCreate()}
        , m_शीर्ष_Name{v3Global.opt.prefix()} {
        iterate(nodep);
    }
};

//######################################################################
// ProtectLib class functions

शून्य V3ProtectLib::protect() {
    UINFO(2, __FUNCTION__ << ":");
    ProtectVisitor{v3Global.rootp()};
}
