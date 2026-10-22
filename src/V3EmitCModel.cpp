// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Emit C++ for model entry point class
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

#अन्तर्भाव "V3EmitC.h"
#अन्तर्भाव "V3EmitCFunc.h"
#अन्तर्भाव "V3UniqueNames.h"

#अन्तर्भाव <algorithm>
#अन्तर्भाव <functional>
#अन्तर्भाव <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

वर्ग EmitCModel final : सार्वजनिक EmitCFunc {
    // TYPES
    उपयुज्य CFunc_सदिशम् = std::सदिशम्<ध्रुव AstCFunc*>;

    // MEMBERS
    V3_Unique_नामानि m_unique_नामानि;  // For generating unique file names

    // METHODS

    CFunc_सदिशम् findFuncps(std::function<तार्किक(ध्रुव AstCFunc*)> cb) {
        CFunc_सदिशम् funcps;
        प्रति (Ast_ग्रन्थिः* nodep = m_modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            यदि (ध्रुव AstCFunc* ध्रुव funcp = VN_CAST(nodep, CFunc)) {
                यदि (cb(funcp)) funcps.प्रक्षिप_पृष्ठभागः(funcp);
            }
        }
        stable_क्रमय(funcps.begin(), funcps.अन्तः_2(),
                    [](ध्रुव Ast_ग्रन्थिः* ap, ध्रुव Ast_ग्रन्थिः* bp) { प्रत्यागम ap->name() < bp->name(); });
        प्रत्यागम funcps;
    }

    // Whether 'funcp' is a DPI function with arguments of unpacked structures
    स्थिर तार्किक isDpiPassingStructs(ध्रुव AstCFunc* funcp) {
        यदि (!funcp->dpiImportPrototype() && !funcp->dpiExportDispatcher()) प्रत्यागम असत्य;
        प्रति (ध्रुव Ast_ग्रन्थिः* argp = funcp->argsp(); argp; argp = argp->nextp()) {
            ध्रुव AstVar* ध्रुव varp = VN_CAST(argp, Var);
            यदि (varp && varp->dtypep()->अन्तर्भवति_Unpacked_Struct()) प्रत्यागम सत्य;
        }
        प्रत्यागम असत्य;
    }

    शून्य putSectionDelimiter(ध्रुव string& name) {
        पङ्क्तिं_लिख("\n");
        पङ्क्तिं_लिख("//============================================================\n");
        पङ्क्तिं_लिख("// " + name + "\n");
    }

    शून्य emit_शीर्षकम्(Ast_ग्रन्थिः_Module* modp) {
        open_New_Output_शीर्षकम्_File(EmitCUtil::शिखरम्_Class_Name(), "Primary model header");
        पङ्क्तिं_लिख("//\n");
        पङ्क्तिं_लिख("// This header should be included by all source files instantiating the design.\n");
        पङ्क्तिं_लिख("// The class here is then constructed to instantiate the design.\n");
        पङ्क्तिं_लिख("// See the Verilator manual for examples.\n");

        ofp()->putsGuard();

        // Include files
        पङ्क्तिं_लिख("\n");
        ofp()->puts_Int_शिखरम्_Include();

        पङ्क्तिं_लिख("#include \"verilated.h\"\n");
        यदि (v3Global.opt.systemC()) पङ्क्तिं_लिख("#include \"verilated_sc.h\"\n");
        यदि (v3Global.opt.mtasks()) पङ्क्तिं_लिख("#include \"verilated_threads.h\"\n");
        यदि (v3Global.opt.savable()) पङ्क्तिं_लिख("#include \"verilated_save.h\"\n");
        यदि (v3Global.opt.coverage()) पङ्क्तिं_लिख("#include \"verilated_cov.h\"\n");
        यदि (v3Global.opt.coverage() || v3Global.useCovergroup())
            पङ्क्तिं_लिख("#include \"verilated_covergroup.h\"\n");
        यदि (v3Global.dpi()) पङ्क्तिं_लिख("#include \"svdpi.h\"\n");
        // The model needs the C structures DPI functions pass, which the DPI header declares
        यदि (!findFuncps(isDpiPassingStructs).रिक्तानि()) {
            पङ्क्तिं_लिख("#include \"" + EmitCUtil::शिखरम्_Class_Name() + "__Dpi.h\"\n");
        }

        // Declare foreign instances up front to make C++ happy
        पङ्क्तिं_लिख("\n");
        पङ्क्तिं_लिख("class " + EmitCUtil::symClassName() + ";\n");
        पङ्क्तिं_लिख("class " + EmitCUtil::उपसर्गः_Name_Protect(modp) + ";\n");  // For rootp pointer only
        प्रति (ध्रुव string& आधारः : v3Global.opt.traceClassLangs()) पङ्क्तिं_लिख("class " + आधारः + ";\n");
        emitModCUse(modp, VUseType::INT_FWD_CLASS);  // Note: This is needed for cell forwarding

        पङ्क्तिं_लिख("\n");

        पङ्क्तिं_लिख("// This class is the main interface to the Verilated model\n");
        putns(modp, "class alignas(VL_CACHE_LINE_BYTES) " + EmitCUtil::शिखरम्_Class_Name()
                        + " VL_NOT_FINAL : ");
        यदि (optSystemC()) {
            // SC_MODULE, but with multiple-inheritance of VerilatedModel
            पङ्क्तिं_लिख("public ::sc_core::sc_module, ");
        }
        पङ्क्तिं_लिख("public VerilatedModel {\n");
        // The symbol table references the model's evaluation state
        पङ्क्तिं_लिख("friend class " + EmitCUtil::symClassName() + ";\n");

        ofp()->reset_वैयक्तिकम्();
        ofp()->puts_वैयक्तिकम्(सत्य);  // private:

        पङ्क्तिं_लिख("// Symbol table holding complete model state (owned by this class)\n");
        पङ्क्तिं_लिख(EmitCUtil::symClassName() + "* const vlSymsp;\n");
        पङ्क्तिं_लिख("// Evaluation loop\n");
        पङ्क्तिं_लिख("VerilatedEvalLoop m_evalLoop;\n");

        पङ्क्तिं_लिख("\n");
        ofp()->puts_वैयक्तिकम्(असत्य);  // public:

        पङ्क्तिं_लिख("\n// CONSTEXPR CAPABILITIES\n");
        पङ्क्तिं_लिख("// Verilated with --trace?\n");
        पङ्क्तिं_लिख("static constexpr bool traceCapable = "s
             + (v3Global.opt.trace() ? "true;\n"s : "false;\n"s));

        // User accessible IO
        पङ्क्तिं_लिख("\n// PORTS\n"
             "// The application code writes and reads these signals to\n"
             "// propagate new values into/out from the Verilated model.\n");
        प्रति (ध्रुव Ast_ग्रन्थिः* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            यदि (ध्रुव AstVar* ध्रुव varp = VN_CAST(nodep, Var)) {
                यदि (varp->isPrimaryIO()) {  //
                    emitVarDecl(varp, /* asRef: */ सत्य);
                }
            }
        }
        यदि (v3Global.opt.emitAccessors()) {
            पङ्क्तिं_लिख("\n// ACCESSORS\n"
                 "// The application code should use these methods to\n"
                 "// propagate new values into/out from the Verilated model\n"
                 "// instead of using signal variables directly.\n");
            प्रति (ध्रुव Ast_ग्रन्थिः* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                यदि (ध्रुव AstVar* ध्रुव varp = VN_CAST(nodep, Var)) {
                    यदि (varp->isPrimaryIO()) emitVarAccessors(varp);
                }
            }
        }

        // Cells instantiated by the top level (for access to /* verilator public */)
        पङ्क्तिं_लिख("\n// CELLS\n"
             "// Public to allow access to /* verilator public */ items.\n"
             "// Otherwise the application code can consider these internals.\n");
        प्रति (Ast_ग्रन्थिः* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            यदि (ध्रुव AstCell* ध्रुव cellp = VN_CAST(nodep, Cell)) {
                यदि (cellp->modp()->isConstPool()) अनुवर्त;  // Special emit rules
                putns(cellp, EmitCUtil::उपसर्गः_Name_Protect(cellp->modp()) + "* const "
                                 + cellp->nameProtect() + ";\n");
            }
        }

        // root instance pointer (for access to internals, including public_flat items).
        पङ्क्तिं_लिख("\n// Root instance pointer to allow access to model internals,\n"
             "// including inlined /* verilator public_flat_* */ items.\n");
        पङ्क्तिं_लिख(EmitCUtil::उपसर्गः_Name_Protect(modp) + "* const rootp;\n");

        पङ्क्तिं_लिख("\n");
        ofp()->puts_वैयक्तिकम्(असत्य);  // public:
        पङ्क्तिं_लिख("// CONSTRUCTORS\n");
        यदि (optSystemC()) {
            पङ्क्तिं_लिख("SC_CTOR(" + EmitCUtil::शिखरम्_Class_Name() + ");\n");
            पङ्क्तिं_लिख("virtual ~" + EmitCUtil::शिखरम्_Class_Name() + "();\n");
        } अन्यथा {
            पङ्क्तिं_लिख("/// Construct the model; called by application code\n");
            पङ्क्तिं_लिख("/// If contextp is null, then the model will use the default global "
                 "context\n");
            पङ्क्तिं_लिख("/// If name is \"\", then makes a wrapper with a\n");
            पङ्क्तिं_लिख("/// single model invisible with respect to DPI scope names.\n");
            पङ्क्तिं_लिख("explicit " + EmitCUtil::शिखरम्_Class_Name() + "(VerilatedContext* contextp,"
                 + " const char* name = \"TOP\");\n");
            पङ्क्तिं_लिख("explicit " + EmitCUtil::शिखरम्_Class_Name() + "(const char* name = \"TOP\");\n");
            पङ्क्तिं_लिख("/// Destroy the model; called (often implicitly) by application code\n");
            पङ्क्तिं_लिख("virtual ~" + EmitCUtil::शिखरम्_Class_Name() + "();\n");
        }
        ofp()->puts_वैयक्तिकम्(सत्य);
        पङ्क्तिं_लिख("VL_UNCOPYABLE(" + EmitCUtil::शिखरम्_Class_Name() + ");  ///< Copying not allowed\n");

        पङ्क्तिं_लिख("\n");
        ofp()->puts_वैयक्तिकम्(असत्य);  // public:
        पङ्क्तिं_लिख("// API METHODS\n");
        ध्रुव string call_Eval_अन्तः_चरणः
            = (v3Global.needTraceDumper() && !optSystemC()) ? "eval_end_step(); " : "";
        यदि (optSystemC()) {
            ofp()->puts_वैयक्तिकम्(सत्य);  ///< eval() is invoked by our sensitive() calls.
        }
        यदि (!optSystemC()) {
            पङ्क्तिं_लिख("/// Evaluate the model.  Application must call when inputs change.\n");
        }
        यदि (optSystemC() && v3Global.उपयुङ्क्ते_Timing()) {
            पङ्क्तिं_लिख("void eval();\n");
        } अन्यथा {
            पङ्क्तिं_लिख("void eval() { eval_step(); " + call_Eval_अन्तः_चरणः + "}\n");
        }
        यदि (!optSystemC()) {
            पङ्क्तिं_लिख("/// Evaluate when calling multiple units/models per time step.\n");
        }
        पङ्क्तिं_लिख("void eval_step();\n");
        यदि (!optSystemC()) {
            पङ्क्तिं_लिख("/// Evaluate at end of a timestep for tracing, when using eval_step().\n");
            पङ्क्तिं_लिख("/// Application must call after all eval() and before time changes.\n");
            पङ्क्तिं_लिख("void eval_end_step()");
            यदि (call_Eval_अन्तः_चरणः == "") {
                पङ्क्तिं_लिख(" {}\n");
            } अन्यथा {
                पङ्क्तिं_लिख(";\n");
            }
        }
        यदि (!optSystemC()) {
            पङ्क्तिं_लिख("/// Simulation complete, run final blocks.  Application "
                 "must call on completion.\n");
        }
        ofp()->puts_वैयक्तिकम्(असत्य);  // public:
        पङ्क्तिं_लिख("void final();\n");

        पङ्क्तिं_लिख("/// Are there scheduled events to handle?\n");
        पङ्क्तिं_लिख("bool eventsPending();\n");
        पङ्क्तिं_लिख("/// Returns time at next time slot. Aborts if !eventsPending()\n");
        पङ्क्तिं_लिख("uint64_t nextTimeSlot();\n");

        यदि (v3Global.opt.trace() || !optSystemC()) {
            पङ्क्तिं_लिख("/// Trace signals in the model; called by application code\n");
            // Backward-compatible usage of calling trace() on the model - now part of context
            पङ्क्तिं_लिख("void trace(VerilatedTraceBaseC* tfp, int levels, int options = 0) {"
                 " contextp()->trace(tfp, levels, options); }\n");
        }
        यदि (v3Global.opt.trace() && optSystemC()) {
            पङ्क्तिं_लिख("/// SC tracing; avoid overloaded virtual function lint warning\n");
            पङ्क्तिं_लिख("void trace(sc_core::sc_trace_file* tfp) const override { "
                 "::sc_core::sc_module::trace(tfp); }\n");
        }

        यदि (!optSystemC()) {
            पङ्क्तिं_लिख("/// Retrieve name of this model instance (as passed to constructor).\n");
            पङ्क्तिं_लिख("const char* name() const;\n");
        }

        // Emit DPI export dispatcher declarations
        {
            ध्रुव CFunc_सदिशम् funcps
                = findFuncps([](ध्रुव AstCFunc* nodep) { प्रत्यागम nodep->dpiExportDispatcher(); });
            यदि (!funcps.रिक्तानि()) {
                पङ्क्तिं_लिख("\n/// DPI Export functions\n");
                प्रति (ध्रुव AstCFunc* funcp : funcps) emitCFuncDecl(funcp, modp);
            }
        }

        यदि (v3Global.opt.savable()) {
            पङ्क्तिं_लिख("\n");
            पङ्क्तिं_लिख("// Serialization functions\n");
            पङ्क्तिं_लिख("friend VerilatedSerialize& operator<<(VerilatedSerialize& os, "  //
                 + EmitCUtil::शिखरम्_Class_Name() + "& rhs);\n");
            पङ्क्तिं_लिख("friend VerilatedDeserialize& operator>>(VerilatedDeserialize& os, "
                 + EmitCUtil::शिखरम्_Class_Name() + "& rhs);\n");
        }

        पङ्क्तिं_लिख("\n// Abstract methods from VerilatedModel\n");
        पङ्क्तिं_लिख("const char* hierName() const override final;\n");
        पङ्क्तिं_लिख("const char* modelName() const override final;\n");
        पङ्क्तिं_लिख("unsigned threads() const override final;\n");
        पङ्क्तिं_लिख("/// Prepare for cloning the model at the process level (e.g. fork in Linux)\n");
        पङ्क्तिं_लिख("/// Release necessary resources. Called before cloning.\n");
        पङ्क्तिं_लिख("void prepareClone() const;\n");
        पङ्क्तिं_लिख("/// Re-init after cloning the model at the process level (e.g. fork in Linux)\n");
        पङ्क्तिं_लिख("/// Re-allocate necessary resources. Called after cloning.\n");
        पङ्क्तिं_लिख("void atClone() const;\n");
        यदि (v3Global.opt.trace()) {
            पङ्क्तिं_लिख("std::unique_ptr<VerilatedTraceConfig> traceConfig() const override final;\n");
        }

        ofp()->puts_वैयक्तिकम्(सत्य);  // private:
        पङ्क्तिं_लिख("\n// Internal functions - the model's evaluation entry points\n");
        पङ्क्तिं_लिख("void evalBegin() override final;\n");
        पङ्क्तिं_लिख("void evalEnd() override final;\n");
        प्रति (पूर्णाङ्क i = 0; i < VEval::ENUM_अन्तः; ++i) {
            ध्रुव VEval eval{i};
            // Only the iterated regions report whether they did any work
            पङ्क्तिं_लिख(eval.isIterated() ? "bool "s : "void "s);
            पङ्क्तिं_लिख(eval.eval_विधानम्() + (eval.firstIteration() ? "(bool firstIteration)"s : "()"s));
            पङ्क्तिं_लिख(" override final;\n");
        }
        प्रति (पूर्णाङ्क i = 0; i < VEval::ENUM_अन्तः; ++i) {
            ध्रुव VEval eval{i};
            यदि (!eval.hasTriggers()) अनुवर्त;
            पङ्क्तिं_लिख("void " + eval.dump_Triggers_विधानम्() + "() override final;\n");
        }

        पङ्क्तिं_लिख("\n// Internal functions - trace registration\n");
        पङ्क्तिं_लिख("void traceBaseModel(VerilatedTraceBaseC* tfp, int levels, int options);\n");

        पङ्क्तिं_लिख("};\n");

        ofp()->puts_अन्तः_Guard();

        closeOutputFile();
    }

    शून्य emitConstructorImplementation(Ast_ग्रन्थिः_Module* modp) {
        putSectionDelimiter("Constructors");

        पङ्क्तिं_लिख("\n");
        putns(modp, EmitCUtil::शिखरम्_Class_Name() + "::" + EmitCUtil::शिखरम्_Class_Name());
        यदि (optSystemC()) {
            पङ्क्तिं_लिख("(sc_core::sc_module_name /* unused */)\n");
            पङ्क्तिं_लिख("    : VerilatedModel{*Verilated::threadContextp()}\n");
            पङ्क्तिं_लिख("    , vlSymsp{new " + EmitCUtil::symClassName()
                 + "(contextp(), name(), this)}\n");
        } अन्यथा {
            पङ्क्तिं_लिख("(VerilatedContext* _vcontextp__, const char* _vcname__)\n");
            पङ्क्तिं_लिख("    : VerilatedModel{*_vcontextp__}\n");
            पङ्क्तिं_लिख("    , vlSymsp{new " + EmitCUtil::symClassName()
                 + "(contextp(), _vcname__, this)}\n");
        }
        पङ्क्तिं_लिख("    , m_evalLoop{*this, /*convergeLimit:*/ "s
             + cvt_गन्तव्यम्_Str(v3Global.opt.converge_सीमा()) + "}\n");

        // Set up IO references
        प्रति (ध्रुव Ast_ग्रन्थिः* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            यदि (ध्रुव AstVar* ध्रुव varp = VN_CAST(nodep, Var)) {
                यदि (varp->isPrimaryIO()) {
                    ध्रुव string protName = varp->nameProtect();
                    पङ्क्तिं_लिख("    , ");
                    putns(varp, protName + "{vlSymsp->vlInst.TOP." + varName(varp) + "}\n");
                }
            }
        }

        // Setup cell pointers
        प्रति (Ast_ग्रन्थिः* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            यदि (ध्रुव AstCell* ध्रुव cellp = VN_CAST(nodep, Cell)) {
                यदि (cellp->modp()->isConstPool()) अनुवर्त;  // Special emit rules
                ध्रुव string protName = cellp->nameProtect();
                पङ्क्तिं_लिख("    , ");
                putns(cellp, protName + "{vlSymsp->vlInst.TOP." + protName + "}\n");
            }
        }

        // Setup rootp root instance pointer,
        पङ्क्तिं_लिख("    , rootp{&(vlSymsp->vlInst.TOP)}\n");

        पङ्क्तिं_लिख("{\n");
        पङ्क्तिं_लिख("// Register model with the context\n");
        पङ्क्तिं_लिख("contextp()->addModel(this);\n");
        यदि (v3Global.opt.profExec()) {
            putsDecoration(शून्यसूचकः, "// Profile the evaluation loop's region loops\n");
            ध्रुव तार्किक शिखरम्_Level = !v3Global.opt.hierChild() && v3Global.opt.libCreate().रिक्तानि();
            पङ्क्तिं_लिख("m_evalLoop.profiler(vlSymsp->__Vm_executionProfilerp, /*topLevel:*/ "s
                 + (शिखरम्_Level ? "true" : "false") + ");\n");
        }
        यदि (v3Global.opt.trace())
            पङ्क्तिं_लिख("contextp()->traceBaseModelCbAdd(\n"
                 "[this](VerilatedTraceBaseC* tfp, int levels, int options) {"
                 " traceBaseModel(tfp, levels, options); });\n");

        यदि (optSystemC()) {
            // Create sensitivity list for when to evaluate the model.
            putsDecoration(शून्यसूचकः, "// Sensitivities on all clocks and combinational inputs\n");
            पङ्क्तिं_लिख("SC_METHOD(eval);\n");
            यदि (v3Global.उपयुङ्क्ते_Timing()) {
                putsDecoration(शून्यसूचकः, "// Notified by pending time delays/DPI exports\n");
                पङ्क्तिं_लिख("sensitive << vlSymsp->__Vm_wakeEvent;\n");
            }
            प्रति (Ast_ग्रन्थिः* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                यदि (ध्रुव AstVar* ध्रुव varp = VN_CAST(nodep, Var)) {
                    यदि (varp->isNonOutput() && (varp->isScSensitive() || varp->isPrimaryClock())) {
                        पूर्णाङ्क vects = 0;
                        // This isn't very robust and may need cleanup for other data types
                        प्रति (AstUnpackArrayDType* arrayp
                             = VN_CAST(varp->dtypeSkipRefp(), UnpackArrayDType);
                             arrayp;
                             arrayp = VN_CAST(arrayp->subDTypep()->skipRefp(), UnpackArrayDType)) {
                            ध्रुव पूर्णाङ्क vecnum = vects++;
                            UASSERT_OBJ(arrayp->hi() >= arrayp->lo(), varp,
                                        "Should have swapped msb & lsb earlier.");
                            ध्रुव string ivar = "__Vi"s + cvt_गन्तव्यम्_Str(vecnum);
                            putns(varp, "for (int __Vi" + cvt_गन्तव्यम्_Str(vecnum) + " = "
                                            + cvt_गन्तव्यम्_Str(arrayp->lo()));
                            पङ्क्तिं_लिख("; " + ivar + " <= " + cvt_गन्तव्यम्_Str(arrayp->hi()));
                            पङ्क्तिं_लिख("; ++" + ivar + ") {\n");
                        }
                        putns(varp, "sensitive << " + varp->nameProtect());
                        प्रति (पूर्णाङ्क v = 0; v < vects; ++v) पङ्क्तिं_लिख("[__Vi" + cvt_गन्तव्यम्_Str(v) + "]");
                        पङ्क्तिं_लिख(";\n");
                        प्रति (पूर्णाङ्क v = 0; v < vects; ++v) पङ्क्तिं_लिख("}\n");
                    }
                }
            }
            पङ्क्तिं_लिख("\n");
        }

        पङ्क्तिं_लिख("}\n");

        यदि (!optSystemC()) {
            पङ्क्तिं_लिख("\n");
            पङ्क्तिं_लिख(EmitCUtil::शिखरम्_Class_Name() + "::" + EmitCUtil::शिखरम्_Class_Name()
                 + "(const char* _vcname__)\n");
            पङ्क्तिं_लिख("    : " + EmitCUtil::शिखरम्_Class_Name()
                 + "(Verilated::threadContextp(), _vcname__)\n{\n}\n");
        }
    }

    शून्य emitDestructorImplementation() {
        putSectionDelimiter("Destructor");

        पङ्क्तिं_लिख("\n");
        पङ्क्तिं_लिख(EmitCUtil::शिखरम्_Class_Name() + "::~" + EmitCUtil::शिखरम्_Class_Name() + "() {\n");
        पङ्क्तिं_लिख("delete vlSymsp;\n");
        पङ्क्तिं_लिख("}\n");
    }

    शून्य emitStandardMethods1(Ast_ग्रन्थिः_Module* modp) {
        UASSERT_OBJ(modp->is_शिखरम्(), modp, "Attempting to emitWrapEval for non-top class");

        ध्रुव string शिखरम्_Mod_Name_Protected = EmitCUtil::उपसर्गः_Name_Protect(modp);
        ध्रुव string selfDecl = "(" + शिखरम्_Mod_Name_Protected + "* vlSelf)";

        putSectionDelimiter("Evaluation function");

        // Forward declarations
        पङ्क्तिं_लिख("\n");
        पङ्क्तिं_लिख("#ifdef VL_DEBUG\n");
        पङ्क्तिं_लिख("void " + शिखरम्_Mod_Name_Protected + "__" + protect("_eval_debug_assertions") + selfDecl
             + ";\n");
        पङ्क्तिं_लिख("#endif  // VL_DEBUG\n");
        AstNetlist* ध्रुव netlistp = v3Global.rootp();
        प्रति (पूर्णाङ्क i = 0; i < VEval::ENUM_अन्तः; ++i) {
            emitCFuncDecl(netlistp->evalFuncp(VEval{i}), modp);
        }
        प्रति (पूर्णाङ्क i = 0; i < VEval::ENUM_अन्तः; ++i) {
            ध्रुव VEval eval{i};
            यदि (eval.hasTriggers()) emitCFuncDecl(netlistp->dumpTriggersFuncp(eval), modp);
        }

        यदि (optSystemC() && v3Global.उपयुङ्क्ते_Timing()) {
            // ::eval
            पङ्क्तिं_लिख("\nvoid " + EmitCUtil::शिखरम्_Class_Name() + "::eval() {\n");
            पङ्क्तिं_लिख("eval_step();\n");
            पङ्क्तिं_लिख("if (eventsPending()) {\n");
            पङ्क्तिं_लिख("sc_core::sc_time dt = sc_core::sc_time::from_value(nextTimeSlot() - "
                 "contextp()->time());\n");
            पङ्क्तिं_लिख("vlSymsp->__Vm_wakeEvent.notify(dt);\n");
            पङ्क्तिं_लिख("}\n");
            पङ्क्तिं_लिख("}\n");
        }

        // ::eval_step - the run-time library drives the whole time step
        पङ्क्तिं_लिख("\nvoid " + EmitCUtil::शिखरम्_Class_Name() + "::eval_step() {\n");
        पङ्क्तिं_लिख("VL_DEBUG_IF(VL_DBG_MSGF(\"+++++TOP Evaluate " + EmitCUtil::शिखरम्_Class_Name()
             + "::eval_step\\n\"); );\n");
        पङ्क्तिं_लिख("m_evalLoop.eval();\n");
        पङ्क्तिं_लिख("}\n");

        // ::evalBegin - prepare the model for a time step
        पङ्क्तिं_लिख("\nvoid " + EmitCUtil::शिखरम्_Class_Name() + "::evalBegin() {\n");
        पङ्क्तिं_लिख("#ifdef VL_DEBUG\n");
        putsDecoration(शून्यसूचकः, "// Debug assertions\n");
        पङ्क्तिं_लिख(शिखरम्_Mod_Name_Protected + "__" + protect("_eval_debug_assertions")
             + "(&(vlSymsp->vlInst.TOP));\n");
        पङ्क्तिं_लिख("#endif  // VL_DEBUG\n");

        यदि (v3Global.opt.trace()) पङ्क्तिं_लिख("vlSymsp->__Vm_activity = true;\n");

        यदि (v3Global.hasEvents()) पङ्क्तिं_लिख("vlSymsp->clearTriggeredEvents();\n");
        यदि (v3Global.hasClasses()) पङ्क्तिं_लिख("vlSymsp->__Vm_deleter.deleteAll();\n");

        पङ्क्तिं_लिख("}\n");

        // ::evalEnd - the time step is complete
        पङ्क्तिं_लिख("\nvoid " + EmitCUtil::शिखरम्_Class_Name() + "::evalEnd() {\n");
        putsDecoration(शून्यसूचकः, "// Evaluate cleanup\n");
        पङ्क्तिं_लिख("Verilated::endOfEval(vlSymsp->__Vm_evalMsgQp);\n");
        यदि (AstVar* ध्रुव delaySchedp = v3Global.rootp()->delaySchedulerp()) {
            पङ्क्तिं_लिख("vlSymsp->vlInst.TOP.");
            पङ्क्तिं_लिख(varName(delaySchedp));
            पङ्क्तिं_लिख(".cleanupForevered();\n");
        }

        पङ्क्तिं_लिख("}\n");

        // Evaluation entry points
        प्रति (पूर्णाङ्क i = 0; i < VEval::ENUM_अन्तः; ++i) {
            ध्रुव VEval eval{i};
            AstCFunc* ध्रुव funcp = v3Global.rootp()->evalFuncp(eval);
            पङ्क्तिं_लिख(eval.isIterated() ? "\nbool " : "\nvoid ");
            पङ्क्तिं_लिख(EmitCUtil::शिखरम्_Class_Name() + "::" + eval.eval_विधानम्() + "(");
            यदि (eval.firstIteration()) पङ्क्तिं_लिख("bool firstIteration");
            पङ्क्तिं_लिख(") {\n");
            यदि (eval.isIterated()) पङ्क्तिं_लिख("return ");
            पङ्क्तिं_लिख(funcNameProtect(funcp, m_modp) + "(&(vlSymsp->vlInst.TOP)");
            यदि (eval.firstIteration()) पङ्क्तिं_लिख(", firstIteration");
            पङ्क्तिं_लिख(");\n");
            पङ्क्तिं_लिख("}\n");
        }

        // The trigger dump of each region
        प्रति (पूर्णाङ्क i = 0; i < VEval::ENUM_अन्तः; ++i) {
            ध्रुव VEval eval{i};
            यदि (!eval.hasTriggers()) अनुवर्त;
            पङ्क्तिं_लिख("\nVL_ATTR_COLD void " + EmitCUtil::शिखरम्_Class_Name()
                 + "::" + eval.dump_Triggers_विधानम्() + "() {\n");
            पङ्क्तिं_लिख(funcNameProtect(v3Global.rootp()->dumpTriggersFuncp(eval), m_modp)
                 + "(&(vlSymsp->vlInst.TOP));\n");
            पङ्क्तिं_लिख("}\n");
        }
    }

    शून्य emitStandardMethods2(Ast_ग्रन्थिः_Module* modp) {
        // ::eval_end_step
        यदि (v3Global.needTraceDumper() && !optSystemC()) {
            पङ्क्तिं_लिख("\n");
            putns(modp, "void " + EmitCUtil::शिखरम्_Class_Name() + "::eval_end_step() {\n");
            पङ्क्तिं_लिख("VL_DEBUG_IF(VL_DBG_MSGF(\"+eval_end_step " + EmitCUtil::शिखरम्_Class_Name()
                 + "::eval_end_step\\n\"); );\n");
            पङ्क्तिं_लिख("#ifdef VM_TRACE\n");
            putsDecoration(शून्यसूचकः, "// Tracing\n");
            // SystemC's eval loop deals with calling trace, not us
            पङ्क्तिं_लिख("if (VL_UNLIKELY(vlSymsp->__Vm_dumping)) vlSymsp->_traceDump();\n");
            पङ्क्तिं_लिख("#endif  // VM_TRACE\n");
            पङ्क्तिं_लिख("}\n");
        }

        putSectionDelimiter("Events and timing");
        यदि (स्वचालित* ध्रुव delaySchedp = v3Global.rootp()->delaySchedulerp()) {
            putns(modp, "bool " + EmitCUtil::शिखरम्_Class_Name()
                            + "::eventsPending() { return !vlSymsp->vlInst.TOP.");
            पङ्क्तिं_लिख(varName(delaySchedp));
            पङ्क्तिं_लिख(".empty() && !contextp()->gotFinish(); }\n\n");

            putns(modp, "uint64_t " + EmitCUtil::शिखरम्_Class_Name()
                            + "::nextTimeSlot() { return vlSymsp->vlInst.TOP.");
            पङ्क्तिं_लिख(varName(delaySchedp));
            पङ्क्तिं_लिख(".nextTimeSlot(); }\n");
        } अन्यथा {
            putns(modp,
                  "bool " + EmitCUtil::शिखरम्_Class_Name() + "::eventsPending() { return false; }\n\n");
            पङ्क्तिं_लिख("uint64_t " + EmitCUtil::शिखरम्_Class_Name() + "::nextTimeSlot() {\n");
            पङ्क्तिं_लिख("VL_FATAL_MT(__FILE__, __LINE__, \"\", \"No delays in the design\");\n");
            पङ्क्तिं_लिख("return 0;\n}\n");
        }

        putSectionDelimiter("Utilities");

        यदि (!optSystemC()) {
            // ::name
            पङ्क्तिं_लिख("\n");
            putns(modp, "const char* " + EmitCUtil::शिखरम्_Class_Name() + "::name() const {\n");
            पङ्क्तिं_लिख(/**/ "return vlSymsp->name();\n");
            पङ्क्तिं_लिख("}\n");
        }

        putSectionDelimiter("Invoke final blocks");
        // ::final
        पङ्क्तिं_लिख("\nVL_ATTR_COLD void " + EmitCUtil::शिखरम्_Class_Name() + "::final() {\n");
        पङ्क्तिं_लिख("contextp()->executingFinal(true);\n");
        पङ्क्तिं_लिख("evalFinal();\n");
        पङ्क्तिं_लिख("contextp()->executingFinal(false);\n");
        पङ्क्तिं_लिख("}\n");

        putSectionDelimiter("Implementations of abstract methods from VerilatedModel\n");
        putns(modp, "const char* " + EmitCUtil::शिखरम्_Class_Name()
                        + "::hierName() const { return vlSymsp->name(); }\n");
        putns(modp, "const char* " + EmitCUtil::शिखरम्_Class_Name() + "::modelName() const { return \""
                        + EmitCUtil::शिखरम्_Class_Name() + "\"; }\n");
        ध्रुव पूर्णाङ्क तन्तवः = v3Global.opt.hierChild()
                                ? v3Global.opt.तन्तवः()
                                : std::max(v3Global.opt.तन्तवः(), v3Global.opt.hier_तन्तवः());
        putns(modp, "unsigned " + EmitCUtil::शिखरम्_Class_Name() + "::threads() const { return "
                        + cvt_गन्तव्यम्_Str(तन्तवः) + "; }\n");
        putns(modp, "void " + EmitCUtil::शिखरम्_Class_Name()
                        + "::prepareClone() const { contextp()->prepareClone(); }\n");
        putns(modp, "void " + EmitCUtil::शिखरम्_Class_Name() + "::atClone() const {\n");
        यदि (v3Global.opt.तन्तवः() > 1) {
            पङ्क्तिं_लिख("vlSymsp->__Vm_threadPoolp = static_cast<VlThreadPool*>(");
        }
        पङ्क्तिं_लिख("contextp()->threadPoolpOnClone()");
        यदि (v3Global.opt.तन्तवः() > 1) पङ्क्तिं_लिख(")");
        पङ्क्तिं_लिख(";\n}\n");

        यदि (v3Global.opt.trace()) {
            putns(modp, "std::unique_ptr<VerilatedTraceConfig> " + EmitCUtil::शिखरम्_Class_Name()
                            + "::traceConfig() const {\n");
            पङ्क्तिं_लिख("return std::unique_ptr<VerilatedTraceConfig>{new VerilatedTraceConfig{");
            पङ्क्तिं_लिख(v3Global.opt.useTraceParallel() ? "true" : "false");
            पङ्क्तिं_लिख("}};\n");
            पङ्क्तिं_लिख("};\n");
        }
    }

    शून्य emitTraceMethods(Ast_ग्रन्थिः_Module* modp) {
        ध्रुव string शिखरम्_Mod_Name_Protected = EmitCUtil::उपसर्गः_Name_Protect(modp);
        ध्रुव string शिखरम्_Trace_Name
            = V3_बहिर्गमनम्_Formatter::quoteNameControls(v3Global.rootp()->trace_Lib_शिखरम्_Name());

        putSectionDelimiter("Trace configuration");

        // Forward declaration
        यदि (!v3Global.opt.libCreate().रिक्तानि()) {
            putns(modp, "\nvoid " + शिखरम्_Mod_Name_Protected + "__" + protect("trace_init_root") + "("
                            + शिखरम्_Mod_Name_Protected + "* vlSelf, " + v3Global.opt.trace_Class_आधारः()
                            + "* tracep);\n");
        }
        putns(modp, "\nvoid " + शिखरम्_Mod_Name_Protected + "__" + protect("trace_decl_types") + "("
                        + v3Global.opt.trace_Class_आधारः() + "* tracep);\n");
        putns(modp, "\nvoid " + शिखरम्_Mod_Name_Protected + "__" + protect("trace_init_top") + "("
                        + शिखरम्_Mod_Name_Protected + "* vlSelf, " + v3Global.opt.trace_Class_आधारः()
                        + "* tracep);\n");

        // Static helper function
        पङ्क्तिं_लिख("\n");
        putns(modp, "VL_ATTR_COLD static void " + protect("trace_init") + "(void* voidSelf, "
                        + v3Global.opt.trace_Class_आधारः() + "* tracep, uint32_t code) {\n");
        putsDecoration(modp, "// Callback from tracep->open()\n");
        पङ्क्तिं_लिख(EmitCUtil::void_Self_नियोजय(modp));
        पङ्क्तिं_लिख(EmitCUtil::sym_Class_नियोजय());
        पङ्क्तिं_लिख("if (!vlSymsp->_vm_contextp__->calcUnusedSigs()) {\n");
        पङ्क्तिं_लिख("VL_FATAL_MT(__FILE__, __LINE__, __FILE__,\n");  // LCOV_EXCL_LINE
        पङ्क्तिं_लिख("\"Turning on wave traces requires Verilated::traceEverOn(true) call before time "
             "0.\");\n");
        पङ्क्तिं_लिख("}\n");
        पङ्क्तिं_लिख("vlSymsp->__Vm_baseCode = code;\n");
        यदि (v3Global.opt.libCreate().रिक्तानि()) {
            पङ्क्तिं_लिख("tracep->pushPrefix(vlSymsp->name(), VerilatedTracePrefixType::SCOPE_MODULE);\n");
        } अन्यथा {
            पङ्क्तिं_लिख("if (tracep->rootInit()) {\n");
            पङ्क्तिं_लिख("tracep->pushPrefix(vlSymsp->name(), VerilatedTracePrefixType::SCOPE_MODULE);\n");
            पङ्क्तिं_लिख(शिखरम्_Mod_Name_Protected + "__" + protect("trace_init_root") + "(vlSelf, tracep);\n");
            पङ्क्तिं_लिख("tracep->pushPrefix(\"" + शिखरम्_Trace_Name
                 + "\", VerilatedTracePrefixType::SCOPE_MODULE);\n");
            पङ्क्तिं_लिख("}\n");
        }
        पङ्क्तिं_लिख(शिखरम्_Mod_Name_Protected + "__" + protect("trace_decl_types") + "(tracep);\n");
        पङ्क्तिं_लिख(शिखरम्_Mod_Name_Protected + "__" + protect("trace_init_top") + "(vlSelf, tracep);\n");
        यदि (v3Global.opt.libCreate().रिक्तानि()) {  //
            पङ्क्तिं_लिख("tracep->popPrefix();\n");
        } अन्यथा {
            पङ्क्तिं_लिख("if (tracep->rootInit()) {\n");
            पङ्क्तिं_लिख("tracep->popPrefix();\n");
            पङ्क्तिं_लिख("tracep->popPrefix();\n");
            पङ्क्तिं_लिख("}\n");
        }
        पङ्क्तिं_लिख("}\n");

        // Forward declaration
        पङ्क्तिं_लिख("\n");
        putns(modp, "VL_ATTR_COLD void " + शिखरम्_Mod_Name_Protected + "__" + protect("trace_register")
                        + "(" + शिखरम्_Mod_Name_Protected + "* vlSelf, " + v3Global.opt.trace_Class_आधारः()
                        + "* tracep);\n");

        // ::traceRegisterModel
        पङ्क्तिं_लिख("\n");
        putns(modp, "VL_ATTR_COLD void " + EmitCUtil::शिखरम्_Class_Name() + "::traceBaseModel(");
        पङ्क्तिं_लिख("VerilatedTraceBaseC* tfp, int levels, int options) {\n");
        यदि (optSystemC()) {
            पङ्क्तिं_लिख(/**/ "if (!sc_core::sc_get_curr_simcontext()->elaboration_done()) {\n");
            पङ्क्तिं_लिख(/****/ "vl_fatal(__FILE__, __LINE__, name(), \"" + EmitCUtil::शिखरम्_Class_Name()
                 + +"::trace() is called before sc_core::sc_start(). "
                    "Run sc_core::sc_start(sc_core::SC_ZERO_TIME) before trace() to complete "
                    "elaboration.\");\n");
            पङ्क्तिं_लिख(/**/ "}");
        }
        पङ्क्तिं_लिख(/**/ "(void)levels; (void)options;\n");  // Prevent unused variable warning
        पङ्क्तिं_लिख(/**/ v3Global.opt.trace_Class_आधारः() + "C* const stfp = dynamic_cast<"
             + v3Global.opt.trace_Class_आधारः() + "C*>(tfp);\n");
        पङ्क्तिं_लिख(/**/ "if (VL_UNLIKELY(!stfp)) {\n");
        पङ्क्तिं_लिख(/****/ "vl_fatal(__FILE__, __LINE__, __FILE__,\"'" + EmitCUtil::शिखरम्_Class_Name()
             + "::trace()' called on non-" + v3Global.opt.trace_Class_आधारः() + "C object;\"\n"
             + "\" use --trace-fst with VerilatedFst object,"
             + " and --trace-vcd with VerilatedVcd object\");\n");
        पङ्क्तिं_लिख(/**/ "}\n");
        पङ्क्तिं_लिख(/**/ "stfp->spTrace()->addModel(this);\n");
        पङ्क्तिं_लिख(/**/ "stfp->spTrace()->addInitCb("s  //
             + "&" + protect("trace_init")  //
             + ", &(vlSymsp->vlInst.TOP)"  //
             + ", name()"  //
             + ", " + (v3Global.opt.libCreate().रिक्तानि() ? "false" : "true")  //
             + ", " + std::गन्तव्यम्_string(v3Global.rootp()->nTraceCodes())  //
             + ");\n");
        पङ्क्तिं_लिख(/**/ शिखरम्_Mod_Name_Protected + "__" + protect("trace_register")
             + "(&(vlSymsp->vlInst.TOP), stfp->spTrace());\n");
        पङ्क्तिं_लिख("}\n");
    }

    शून्य emitSerializationFunctions() {
        putSectionDelimiter("Model serialization");

        पङ्क्तिं_लिख("\nVerilatedSerialize& operator<<(VerilatedSerialize& os, "
             + EmitCUtil::शिखरम्_Class_Name() + "& rhs) {\n");
        पङ्क्तिं_लिख(/**/ "Verilated::quiesce();\n");
        पङ्क्तिं_लिख(/**/ "rhs.vlSymsp->" + protect("__Vserialize") + "(os);\n");
        पङ्क्तिं_लिख(/**/ "return os;\n");
        पङ्क्तिं_लिख("}\n");

        पङ्क्तिं_लिख("\nVerilatedDeserialize& operator>>(VerilatedDeserialize& os, "
             + EmitCUtil::शिखरम्_Class_Name() + "& rhs) {\n");
        पङ्क्तिं_लिख(/**/ "Verilated::quiesce();\n");
        पङ्क्तिं_लिख(/**/ "rhs.vlSymsp->" + protect("__Vdeserialize") + "(os);\n");
        पङ्क्तिं_लिख(/**/ "return os;\n");
        पङ्क्तिं_लिख("}\n");
    }

    शून्य emitImplementation(Ast_ग्रन्थिः_Module* modp) {
        openNewOutputSourceFile(EmitCUtil::शिखरम्_Class_Name(), असत्य, असत्य,
                                "Model implementation (design independent parts)");
        पङ्क्तिं_लिख("\n");
        पङ्क्तिं_लिख("#include \"" + EmitCUtil::pchClassName() + ".h\"\n");
        प्रति (ध्रुव string& आधारः : v3Global.opt.traceSourceLangs())
            पङ्क्तिं_लिख("#include \"" + आधारः + ".h\"\n");

        emitConstructorImplementation(modp);
        emitDestructorImplementation();
        emitStandardMethods1(modp);
        emitStandardMethods2(modp);
        यदि (v3Global.opt.trace()) emitTraceMethods(modp);
        यदि (v3Global.opt.savable()) emitSerializationFunctions();

        closeOutputFile();
    }

    शून्य emitDpiExportDispatchers(Ast_ग्रन्थिः_Module* modp) {
        UASSERT(!ofp(), "Output file should not be open");

        // File name utils
        V3_Unique_नामानि unique_नामानि;
        ध्रुव std::string file_आधारः_Name = EmitCUtil::शिखरम्_Class_Name() + "__Dpi_Export";

        // Emit DPI Export dispatchers
        प्रति (Ast_ग्रन्थिः* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            AstCFunc* ध्रुव funcp = VN_CAST(nodep, CFunc);
            यदि (!funcp || !funcp->dpiExportDispatcher()) अनुवर्त;

            यदि (विभज_Needed()) {
                // Splitting file, so using parallel build.
                v3Global.useParallelBuild(सत्य);
                // Close old file
                closeOutputFile();
            }

            यदि (!ofp()) {
                openNewOutputSourceFile(unique_नामानि.get(file_आधारः_Name), असत्य, असत्य,
                                        "Implementation of DPI export functions.");
                पङ्क्तिं_लिख("\n");
                पङ्क्तिं_लिख("#include \"" + EmitCUtil::शिखरम्_Class_Name() + ".h\"\n");
                पङ्क्तिं_लिख("#include \"" + EmitCUtil::symClassName() + ".h\"\n");
                पङ्क्तिं_लिख("#include \"verilated_dpi.h\"\n");
                पङ्क्तिं_लिख("\n");
                m_lazyDecls.reset();
            }

            iterateConst(funcp);
        }

        यदि (ofp()) closeOutputFile();
    }

    शून्य मुख्यम्(Ast_ग्रन्थिः_Module* modp) {
        m_modp = modp;
        emit_शीर्षकम्(modp);
        emitImplementation(modp);
        यदि (v3Global.dpi()) emitDpiExportDispatchers(modp);
    }

    // VISITORS

सार्वजनिक:
    स्पष्ट EmitCModel(Ast_ग्रन्थिः_Module* शिखरम्_Modulep) { मुख्यम्(शिखरम्_Modulep); }
};

//######################################################################
// EmitC class functions

शून्य V3EmitC::emitcModel() {
    UINFO(2, __FUNCTION__ << ":");
    v3Global.rootp()->सङ्कलय_Eval_Stats("emitc");
    { EmitCModel{v3Global.rootp()->शिखरम्_Modulep()}; }
}
