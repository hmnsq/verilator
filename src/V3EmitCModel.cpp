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

वग्ग EmitCModel final : सार्वजनिक EmitCFunc {
    // TYPES
    using CFuncVector = std::vector<ध्रुव AstCFunc*>;

    // MEMBERS
    V3UniqueNames m_uniqueNames;  // For generating unique file names

    // METHODS

    CFuncVector findFuncps(std::function<तार्किक(ध्रुव AstCFunc*)> cb) {
        CFuncVector funcps;
        अत्थाय (Ast_ग्रन्थिः* nodep = m_modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            सचे (ध्रुव AstCFunc* ध्रुव funcp = VN_CAST(nodep, CFunc)) {
                सचे (cb(funcp)) funcps.प्रक्षिप_back(funcp);
            }
        }
        stable_sort(funcps.begin(), funcps.end(),
                    [](ध्रुव Ast_ग्रन्थिः* ap, ध्रुव Ast_ग्रन्थिः* bp) { पटिनिवत्त ap->name() < bp->name(); });
        पटिनिवत्त funcps;
    }

    // Whether 'funcp' is a DPI function with arguments of unpacked structures
    स्थिर तार्किक isDpiPassingStructs(ध्रुव AstCFunc* funcp) {
        सचे (!funcp->dpiImportPrototype() && !funcp->dpiExportDispatcher()) पटिनिवत्त मिच्छा;
        अत्थाय (ध्रुव Ast_ग्रन्थिः* argp = funcp->argsp(); argp; argp = argp->nextp()) {
            ध्रुव AstVar* ध्रुव varp = VN_CAST(argp, Var);
            सचे (varp && varp->dtypep()->containsUnpackedStruct()) पटिनिवत्त सच्च;
        }
        पटिनिवत्त मिच्छा;
    }

    शून्य putSectionDelimiter(ध्रुव string& name) {
        puts("\n");
        puts("//============================================================\n");
        puts("// " + name + "\n");
    }

    शून्य emitHeader(Ast_ग्रन्थिः_Module* modp) {
        openNewOutputHeaderFile(EmitCUtil::शिखरम्_Class_Name(), "Primary model header");
        puts("//\n");
        puts("// This header should be included by all source files instantiating the design.\n");
        puts("// The class here is then constructed to instantiate the design.\n");
        puts("// See the Verilator manual for examples.\n");

        ofp()->putsGuard();

        // Include files
        puts("\n");
        ofp()->puts_Int_शिखरम्_Include();

        puts("#include \"verilated.h\"\n");
        सचे (v3Global.opt.systemC()) puts("#include \"verilated_sc.h\"\n");
        सचे (v3Global.opt.mtasks()) puts("#include \"verilated_threads.h\"\n");
        सचे (v3Global.opt.savable()) puts("#include \"verilated_save.h\"\n");
        सचे (v3Global.opt.coverage()) puts("#include \"verilated_cov.h\"\n");
        सचे (v3Global.opt.coverage() || v3Global.useCovergroup())
            puts("#include \"verilated_covergroup.h\"\n");
        सचे (v3Global.dpi()) puts("#include \"svdpi.h\"\n");
        // The model needs the C structures DPI functions pass, which the DPI header declares
        सचे (!findFuncps(isDpiPassingStructs).empty()) {
            puts("#include \"" + EmitCUtil::शिखरम्_Class_Name() + "__Dpi.h\"\n");
        }

        // Declare foreign instances up front to make C++ happy
        puts("\n");
        puts("class " + EmitCUtil::symClassName() + ";\n");
        puts("class " + EmitCUtil::prefixNameProtect(modp) + ";\n");  // For rootp pointer only
        अत्थाय (ध्रुव string& base : v3Global.opt.traceClassLangs()) puts("class " + base + ";\n");
        emitModCUse(modp, VUseType::INT_FWD_CLASS);  // Note: This is needed for cell forwarding

        puts("\n");

        puts("// This class is the main interface to the Verilated model\n");
        putns(modp, "class alignas(VL_CACHE_LINE_BYTES) " + EmitCUtil::शिखरम्_Class_Name()
                        + " VL_NOT_FINAL : ");
        सचे (optSystemC()) {
            // SC_MODULE, but with multiple-inheritance of VerilatedModel
            puts("public ::sc_core::sc_module, ");
        }
        puts("public VerilatedModel {\n");
        // The symbol table references the model's evaluation state
        puts("friend class " + EmitCUtil::symClassName() + ";\n");

        ofp()->resetPrivate();
        ofp()->putsPrivate(सच्च);  // private:

        puts("// Symbol table holding complete model state (owned by this class)\n");
        puts(EmitCUtil::symClassName() + "* const vlSymsp;\n");
        puts("// Evaluation loop\n");
        puts("VerilatedEvalLoop m_evalLoop;\n");

        puts("\n");
        ofp()->putsPrivate(मिच्छा);  // public:

        puts("\n// CONSTEXPR CAPABILITIES\n");
        puts("// Verilated with --trace?\n");
        puts("static constexpr bool traceCapable = "s
             + (v3Global.opt.trace() ? "true;\n"s : "false;\n"s));

        // User accessible IO
        puts("\n// PORTS\n"
             "// The application code writes and reads these signals to\n"
             "// propagate new values into/out from the Verilated model.\n");
        अत्थाय (ध्रुव Ast_ग्रन्थिः* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            सचे (ध्रुव AstVar* ध्रुव varp = VN_CAST(nodep, Var)) {
                सचे (varp->isPrimaryIO()) {  //
                    emitVarDecl(varp, /* asRef: */ सच्च);
                }
            }
        }
        सचे (v3Global.opt.emitAccessors()) {
            puts("\n// ACCESSORS\n"
                 "// The application code should use these methods to\n"
                 "// propagate new values into/out from the Verilated model\n"
                 "// instead of using signal variables directly.\n");
            अत्थाय (ध्रुव Ast_ग्रन्थिः* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                सचे (ध्रुव AstVar* ध्रुव varp = VN_CAST(nodep, Var)) {
                    सचे (varp->isPrimaryIO()) emitVarAccessors(varp);
                }
            }
        }

        // Cells instantiated by the top level (for access to /* verilator public */)
        puts("\n// CELLS\n"
             "// Public to allow access to /* verilator public */ items.\n"
             "// Otherwise the application code can consider these internals.\n");
        अत्थाय (Ast_ग्रन्थिः* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            सचे (ध्रुव AstCell* ध्रुव cellp = VN_CAST(nodep, Cell)) {
                सचे (cellp->modp()->isConstPool()) अनुवर्त;  // Special emit rules
                putns(cellp, EmitCUtil::prefixNameProtect(cellp->modp()) + "* const "
                                 + cellp->nameProtect() + ";\n");
            }
        }

        // root instance pointer (for access to internals, including public_flat items).
        puts("\n// Root instance pointer to allow access to model internals,\n"
             "// including inlined /* verilator public_flat_* */ items.\n");
        puts(EmitCUtil::prefixNameProtect(modp) + "* const rootp;\n");

        puts("\n");
        ofp()->putsPrivate(मिच्छा);  // public:
        puts("// CONSTRUCTORS\n");
        सचे (optSystemC()) {
            puts("SC_CTOR(" + EmitCUtil::शिखरम्_Class_Name() + ");\n");
            puts("virtual ~" + EmitCUtil::शिखरम्_Class_Name() + "();\n");
        } अञ्ञथा {
            puts("/// Construct the model; called by application code\n");
            puts("/// If contextp is null, then the model will use the default global "
                 "context\n");
            puts("/// If name is \"\", then makes a wrapper with a\n");
            puts("/// single model invisible with respect to DPI scope names.\n");
            puts("explicit " + EmitCUtil::शिखरम्_Class_Name() + "(VerilatedContext* contextp,"
                 + " const char* name = \"TOP\");\n");
            puts("explicit " + EmitCUtil::शिखरम्_Class_Name() + "(const char* name = \"TOP\");\n");
            puts("/// Destroy the model; called (often implicitly) by application code\n");
            puts("virtual ~" + EmitCUtil::शिखरम्_Class_Name() + "();\n");
        }
        ofp()->putsPrivate(सच्च);
        puts("VL_UNCOPYABLE(" + EmitCUtil::शिखरम्_Class_Name() + ");  ///< Copying not allowed\n");

        puts("\n");
        ofp()->putsPrivate(मिच्छा);  // public:
        puts("// API METHODS\n");
        ध्रुव string callEvalEndStep
            = (v3Global.needTraceDumper() && !optSystemC()) ? "eval_end_step(); " : "";
        सचे (optSystemC()) {
            ofp()->putsPrivate(सच्च);  ///< eval() is invoked by our sensitive() calls.
        }
        सचे (!optSystemC()) {
            puts("/// Evaluate the model.  Application must call when inputs change.\n");
        }
        सचे (optSystemC() && v3Global.usesTiming()) {
            puts("void eval();\n");
        } अञ्ञथा {
            puts("void eval() { eval_step(); " + callEvalEndStep + "}\n");
        }
        सचे (!optSystemC()) {
            puts("/// Evaluate when calling multiple units/models per time step.\n");
        }
        puts("void eval_step();\n");
        सचे (!optSystemC()) {
            puts("/// Evaluate at end of a timestep for tracing, when using eval_step().\n");
            puts("/// Application must call after all eval() and before time changes.\n");
            puts("void eval_end_step()");
            सचे (callEvalEndStep == "") {
                puts(" {}\n");
            } अञ्ञथा {
                puts(";\n");
            }
        }
        सचे (!optSystemC()) {
            puts("/// Simulation complete, run final blocks.  Application "
                 "must call on completion.\n");
        }
        ofp()->putsPrivate(मिच्छा);  // public:
        puts("void final();\n");

        puts("/// Are there scheduled events to handle?\n");
        puts("bool eventsPending();\n");
        puts("/// Returns time at next time slot. Aborts if !eventsPending()\n");
        puts("uint64_t nextTimeSlot();\n");

        सचे (v3Global.opt.trace() || !optSystemC()) {
            puts("/// Trace signals in the model; called by application code\n");
            // Backward-compatible usage of calling trace() on the model - now part of context
            puts("void trace(VerilatedTraceBaseC* tfp, int levels, int options = 0) {"
                 " contextp()->trace(tfp, levels, options); }\n");
        }
        सचे (v3Global.opt.trace() && optSystemC()) {
            puts("/// SC tracing; avoid overloaded virtual function lint warning\n");
            puts("void trace(sc_core::sc_trace_file* tfp) const override { "
                 "::sc_core::sc_module::trace(tfp); }\n");
        }

        सचे (!optSystemC()) {
            puts("/// Retrieve name of this model instance (as passed to constructor).\n");
            puts("const char* name() const;\n");
        }

        // Emit DPI export dispatcher declarations
        {
            ध्रुव CFuncVector funcps
                = findFuncps([](ध्रुव AstCFunc* nodep) { पटिनिवत्त nodep->dpiExportDispatcher(); });
            सचे (!funcps.empty()) {
                puts("\n/// DPI Export functions\n");
                अत्थाय (ध्रुव AstCFunc* funcp : funcps) emitCFuncDecl(funcp, modp);
            }
        }

        सचे (v3Global.opt.savable()) {
            puts("\n");
            puts("// Serialization functions\n");
            puts("friend VerilatedSerialize& operator<<(VerilatedSerialize& os, "  //
                 + EmitCUtil::शिखरम्_Class_Name() + "& rhs);\n");
            puts("friend VerilatedDeserialize& operator>>(VerilatedDeserialize& os, "
                 + EmitCUtil::शिखरम्_Class_Name() + "& rhs);\n");
        }

        puts("\n// Abstract methods from VerilatedModel\n");
        puts("const char* hierName() const override final;\n");
        puts("const char* modelName() const override final;\n");
        puts("unsigned threads() const override final;\n");
        puts("/// Prepare for cloning the model at the process level (e.g. fork in Linux)\n");
        puts("/// Release necessary resources. Called before cloning.\n");
        puts("void prepareClone() const;\n");
        puts("/// Re-init after cloning the model at the process level (e.g. fork in Linux)\n");
        puts("/// Re-allocate necessary resources. Called after cloning.\n");
        puts("void atClone() const;\n");
        सचे (v3Global.opt.trace()) {
            puts("std::unique_ptr<VerilatedTraceConfig> traceConfig() const override final;\n");
        }

        ofp()->putsPrivate(सच्च);  // private:
        puts("\n// Internal functions - the model's evaluation entry points\n");
        puts("void evalBegin() override final;\n");
        puts("void evalEnd() override final;\n");
        अत्थाय (पूर्णाङ्क i = 0; i < VEval::_ENUM_END; ++i) {
            ध्रुव VEval eval{i};
            // Only the iterated regions report whether they did any work
            puts(eval.isIterated() ? "bool "s : "void "s);
            puts(eval.evalMethod() + (eval.firstIteration() ? "(bool firstIteration)"s : "()"s));
            puts(" override final;\n");
        }
        अत्थाय (पूर्णाङ्क i = 0; i < VEval::_ENUM_END; ++i) {
            ध्रुव VEval eval{i};
            सचे (!eval.hasTriggers()) अनुवर्त;
            puts("void " + eval.dumpTriggersMethod() + "() override final;\n");
        }

        puts("\n// Internal functions - trace registration\n");
        puts("void traceBaseModel(VerilatedTraceBaseC* tfp, int levels, int options);\n");

        puts("};\n");

        ofp()->putsEndGuard();

        closeOutputFile();
    }

    शून्य emitConstructorImplementation(Ast_ग्रन्थिः_Module* modp) {
        putSectionDelimiter("Constructors");

        puts("\n");
        putns(modp, EmitCUtil::शिखरम्_Class_Name() + "::" + EmitCUtil::शिखरम्_Class_Name());
        सचे (optSystemC()) {
            puts("(sc_core::sc_module_name /* unused */)\n");
            puts("    : VerilatedModel{*Verilated::threadContextp()}\n");
            puts("    , vlSymsp{new " + EmitCUtil::symClassName()
                 + "(contextp(), name(), this)}\n");
        } अञ्ञथा {
            puts("(VerilatedContext* _vcontextp__, const char* _vcname__)\n");
            puts("    : VerilatedModel{*_vcontextp__}\n");
            puts("    , vlSymsp{new " + EmitCUtil::symClassName()
                 + "(contextp(), _vcname__, this)}\n");
        }
        puts("    , m_evalLoop{*this, /*convergeLimit:*/ "s
             + cvtToStr(v3Global.opt.convergeLimit()) + "}\n");

        // Set up IO references
        अत्थाय (ध्रुव Ast_ग्रन्थिः* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            सचे (ध्रुव AstVar* ध्रुव varp = VN_CAST(nodep, Var)) {
                सचे (varp->isPrimaryIO()) {
                    ध्रुव string protName = varp->nameProtect();
                    puts("    , ");
                    putns(varp, protName + "{vlSymsp->vlInst.TOP." + varName(varp) + "}\n");
                }
            }
        }

        // Setup cell pointers
        अत्थाय (Ast_ग्रन्थिः* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            सचे (ध्रुव AstCell* ध्रुव cellp = VN_CAST(nodep, Cell)) {
                सचे (cellp->modp()->isConstPool()) अनुवर्त;  // Special emit rules
                ध्रुव string protName = cellp->nameProtect();
                puts("    , ");
                putns(cellp, protName + "{vlSymsp->vlInst.TOP." + protName + "}\n");
            }
        }

        // Setup rootp root instance pointer,
        puts("    , rootp{&(vlSymsp->vlInst.TOP)}\n");

        puts("{\n");
        puts("// Register model with the context\n");
        puts("contextp()->addModel(this);\n");
        सचे (v3Global.opt.profExec()) {
            putsDecoration(nullptr, "// Profile the evaluation loop's region loops\n");
            ध्रुव तार्किक शिखरम्_Level = !v3Global.opt.hierChild() && v3Global.opt.libCreate().empty();
            puts("m_evalLoop.profiler(vlSymsp->__Vm_executionProfilerp, /*topLevel:*/ "s
                 + (शिखरम्_Level ? "true" : "false") + ");\n");
        }
        सचे (v3Global.opt.trace())
            puts("contextp()->traceBaseModelCbAdd(\n"
                 "[this](VerilatedTraceBaseC* tfp, int levels, int options) {"
                 " traceBaseModel(tfp, levels, options); });\n");

        सचे (optSystemC()) {
            // Create sensitivity list for when to evaluate the model.
            putsDecoration(nullptr, "// Sensitivities on all clocks and combinational inputs\n");
            puts("SC_METHOD(eval);\n");
            सचे (v3Global.usesTiming()) {
                putsDecoration(nullptr, "// Notified by pending time delays/DPI exports\n");
                puts("sensitive << vlSymsp->__Vm_wakeEvent;\n");
            }
            अत्थाय (Ast_ग्रन्थिः* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                सचे (ध्रुव AstVar* ध्रुव varp = VN_CAST(nodep, Var)) {
                    सचे (varp->isNonOutput() && (varp->isScSensitive() || varp->isPrimaryClock())) {
                        पूर्णाङ्क vects = 0;
                        // This isn't very robust and may need cleanup for other data types
                        अत्थाय (AstUnpackArrayDType* arrayp
                             = VN_CAST(varp->dtypeSkipRefp(), UnpackArrayDType);
                             arrayp;
                             arrayp = VN_CAST(arrayp->subDTypep()->skipRefp(), UnpackArrayDType)) {
                            ध्रुव पूर्णाङ्क vecnum = vects++;
                            UASSERT_OBJ(arrayp->hi() >= arrayp->lo(), varp,
                                        "Should have swapped msb & lsb earlier.");
                            ध्रुव string ivar = "__Vi"s + cvtToStr(vecnum);
                            putns(varp, "for (int __Vi" + cvtToStr(vecnum) + " = "
                                            + cvtToStr(arrayp->lo()));
                            puts("; " + ivar + " <= " + cvtToStr(arrayp->hi()));
                            puts("; ++" + ivar + ") {\n");
                        }
                        putns(varp, "sensitive << " + varp->nameProtect());
                        अत्थाय (पूर्णाङ्क v = 0; v < vects; ++v) puts("[__Vi" + cvtToStr(v) + "]");
                        puts(";\n");
                        अत्थाय (पूर्णाङ्क v = 0; v < vects; ++v) puts("}\n");
                    }
                }
            }
            puts("\n");
        }

        puts("}\n");

        सचे (!optSystemC()) {
            puts("\n");
            puts(EmitCUtil::शिखरम्_Class_Name() + "::" + EmitCUtil::शिखरम्_Class_Name()
                 + "(const char* _vcname__)\n");
            puts("    : " + EmitCUtil::शिखरम्_Class_Name()
                 + "(Verilated::threadContextp(), _vcname__)\n{\n}\n");
        }
    }

    शून्य emitDestructorImplementation() {
        putSectionDelimiter("Destructor");

        puts("\n");
        puts(EmitCUtil::शिखरम्_Class_Name() + "::~" + EmitCUtil::शिखरम्_Class_Name() + "() {\n");
        puts("delete vlSymsp;\n");
        puts("}\n");
    }

    शून्य emitStandardMethods1(Ast_ग्रन्थिः_Module* modp) {
        UASSERT_OBJ(modp->is_शिखरम्(), modp, "Attempting to emitWrapEval for non-top class");

        ध्रुव string शिखरम्_Mod_Name_Protected = EmitCUtil::prefixNameProtect(modp);
        ध्रुव string selfDecl = "(" + शिखरम्_Mod_Name_Protected + "* vlSelf)";

        putSectionDelimiter("Evaluation function");

        // Forward declarations
        puts("\n");
        puts("#ifdef VL_DEBUG\n");
        puts("void " + शिखरम्_Mod_Name_Protected + "__" + protect("_eval_debug_assertions") + selfDecl
             + ";\n");
        puts("#endif  // VL_DEBUG\n");
        AstNetlist* ध्रुव netlistp = v3Global.rootp();
        अत्थाय (पूर्णाङ्क i = 0; i < VEval::_ENUM_END; ++i) {
            emitCFuncDecl(netlistp->evalFuncp(VEval{i}), modp);
        }
        अत्थाय (पूर्णाङ्क i = 0; i < VEval::_ENUM_END; ++i) {
            ध्रुव VEval eval{i};
            सचे (eval.hasTriggers()) emitCFuncDecl(netlistp->dumpTriggersFuncp(eval), modp);
        }

        सचे (optSystemC() && v3Global.usesTiming()) {
            // ::eval
            puts("\nvoid " + EmitCUtil::शिखरम्_Class_Name() + "::eval() {\n");
            puts("eval_step();\n");
            puts("if (eventsPending()) {\n");
            puts("sc_core::sc_time dt = sc_core::sc_time::from_value(nextTimeSlot() - "
                 "contextp()->time());\n");
            puts("vlSymsp->__Vm_wakeEvent.notify(dt);\n");
            puts("}\n");
            puts("}\n");
        }

        // ::eval_step - the run-time library drives the whole time step
        puts("\nvoid " + EmitCUtil::शिखरम्_Class_Name() + "::eval_step() {\n");
        puts("VL_DEBUG_IF(VL_DBG_MSGF(\"+++++TOP Evaluate " + EmitCUtil::शिखरम्_Class_Name()
             + "::eval_step\\n\"); );\n");
        puts("m_evalLoop.eval();\n");
        puts("}\n");

        // ::evalBegin - prepare the model for a time step
        puts("\nvoid " + EmitCUtil::शिखरम्_Class_Name() + "::evalBegin() {\n");
        puts("#ifdef VL_DEBUG\n");
        putsDecoration(nullptr, "// Debug assertions\n");
        puts(शिखरम्_Mod_Name_Protected + "__" + protect("_eval_debug_assertions")
             + "(&(vlSymsp->vlInst.TOP));\n");
        puts("#endif  // VL_DEBUG\n");

        सचे (v3Global.opt.trace()) puts("vlSymsp->__Vm_activity = true;\n");

        सचे (v3Global.hasEvents()) puts("vlSymsp->clearTriggeredEvents();\n");
        सचे (v3Global.hasClasses()) puts("vlSymsp->__Vm_deleter.deleteAll();\n");

        puts("}\n");

        // ::evalEnd - the time step is complete
        puts("\nvoid " + EmitCUtil::शिखरम्_Class_Name() + "::evalEnd() {\n");
        putsDecoration(nullptr, "// Evaluate cleanup\n");
        puts("Verilated::endOfEval(vlSymsp->__Vm_evalMsgQp);\n");
        सचे (AstVar* ध्रुव delaySchedp = v3Global.rootp()->delaySchedulerp()) {
            puts("vlSymsp->vlInst.TOP.");
            puts(varName(delaySchedp));
            puts(".cleanupForevered();\n");
        }

        puts("}\n");

        // Evaluation entry points
        अत्थाय (पूर्णाङ्क i = 0; i < VEval::_ENUM_END; ++i) {
            ध्रुव VEval eval{i};
            AstCFunc* ध्रुव funcp = v3Global.rootp()->evalFuncp(eval);
            puts(eval.isIterated() ? "\nbool " : "\nvoid ");
            puts(EmitCUtil::शिखरम्_Class_Name() + "::" + eval.evalMethod() + "(");
            सचे (eval.firstIteration()) puts("bool firstIteration");
            puts(") {\n");
            सचे (eval.isIterated()) puts("return ");
            puts(funcNameProtect(funcp, m_modp) + "(&(vlSymsp->vlInst.TOP)");
            सचे (eval.firstIteration()) puts(", firstIteration");
            puts(");\n");
            puts("}\n");
        }

        // The trigger dump of each region
        अत्थाय (पूर्णाङ्क i = 0; i < VEval::_ENUM_END; ++i) {
            ध्रुव VEval eval{i};
            सचे (!eval.hasTriggers()) अनुवर्त;
            puts("\nVL_ATTR_COLD void " + EmitCUtil::शिखरम्_Class_Name()
                 + "::" + eval.dumpTriggersMethod() + "() {\n");
            puts(funcNameProtect(v3Global.rootp()->dumpTriggersFuncp(eval), m_modp)
                 + "(&(vlSymsp->vlInst.TOP));\n");
            puts("}\n");
        }
    }

    शून्य emitStandardMethods2(Ast_ग्रन्थिः_Module* modp) {
        // ::eval_end_step
        सचे (v3Global.needTraceDumper() && !optSystemC()) {
            puts("\n");
            putns(modp, "void " + EmitCUtil::शिखरम्_Class_Name() + "::eval_end_step() {\n");
            puts("VL_DEBUG_IF(VL_DBG_MSGF(\"+eval_end_step " + EmitCUtil::शिखरम्_Class_Name()
                 + "::eval_end_step\\n\"); );\n");
            puts("#ifdef VM_TRACE\n");
            putsDecoration(nullptr, "// Tracing\n");
            // SystemC's eval loop deals with calling trace, not us
            puts("if (VL_UNLIKELY(vlSymsp->__Vm_dumping)) vlSymsp->_traceDump();\n");
            puts("#endif  // VM_TRACE\n");
            puts("}\n");
        }

        putSectionDelimiter("Events and timing");
        सचे (auto* ध्रुव delaySchedp = v3Global.rootp()->delaySchedulerp()) {
            putns(modp, "bool " + EmitCUtil::शिखरम्_Class_Name()
                            + "::eventsPending() { return !vlSymsp->vlInst.TOP.");
            puts(varName(delaySchedp));
            puts(".empty() && !contextp()->gotFinish(); }\n\n");

            putns(modp, "uint64_t " + EmitCUtil::शिखरम्_Class_Name()
                            + "::nextTimeSlot() { return vlSymsp->vlInst.TOP.");
            puts(varName(delaySchedp));
            puts(".nextTimeSlot(); }\n");
        } अञ्ञथा {
            putns(modp,
                  "bool " + EmitCUtil::शिखरम्_Class_Name() + "::eventsPending() { return false; }\n\n");
            puts("uint64_t " + EmitCUtil::शिखरम्_Class_Name() + "::nextTimeSlot() {\n");
            puts("VL_FATAL_MT(__FILE__, __LINE__, \"\", \"No delays in the design\");\n");
            puts("return 0;\n}\n");
        }

        putSectionDelimiter("Utilities");

        सचे (!optSystemC()) {
            // ::name
            puts("\n");
            putns(modp, "const char* " + EmitCUtil::शिखरम्_Class_Name() + "::name() const {\n");
            puts(/**/ "return vlSymsp->name();\n");
            puts("}\n");
        }

        putSectionDelimiter("Invoke final blocks");
        // ::final
        puts("\nVL_ATTR_COLD void " + EmitCUtil::शिखरम्_Class_Name() + "::final() {\n");
        puts("contextp()->executingFinal(true);\n");
        puts("evalFinal();\n");
        puts("contextp()->executingFinal(false);\n");
        puts("}\n");

        putSectionDelimiter("Implementations of abstract methods from VerilatedModel\n");
        putns(modp, "const char* " + EmitCUtil::शिखरम्_Class_Name()
                        + "::hierName() const { return vlSymsp->name(); }\n");
        putns(modp, "const char* " + EmitCUtil::शिखरम्_Class_Name() + "::modelName() const { return \""
                        + EmitCUtil::शिखरम्_Class_Name() + "\"; }\n");
        ध्रुव पूर्णाङ्क threads = v3Global.opt.hierChild()
                                ? v3Global.opt.threads()
                                : std::max(v3Global.opt.threads(), v3Global.opt.hierThreads());
        putns(modp, "unsigned " + EmitCUtil::शिखरम्_Class_Name() + "::threads() const { return "
                        + cvtToStr(threads) + "; }\n");
        putns(modp, "void " + EmitCUtil::शिखरम्_Class_Name()
                        + "::prepareClone() const { contextp()->prepareClone(); }\n");
        putns(modp, "void " + EmitCUtil::शिखरम्_Class_Name() + "::atClone() const {\n");
        सचे (v3Global.opt.threads() > 1) {
            puts("vlSymsp->__Vm_threadPoolp = static_cast<VlThreadPool*>(");
        }
        puts("contextp()->threadPoolpOnClone()");
        सचे (v3Global.opt.threads() > 1) puts(")");
        puts(";\n}\n");

        सचे (v3Global.opt.trace()) {
            putns(modp, "std::unique_ptr<VerilatedTraceConfig> " + EmitCUtil::शिखरम्_Class_Name()
                            + "::traceConfig() const {\n");
            puts("return std::unique_ptr<VerilatedTraceConfig>{new VerilatedTraceConfig{");
            puts(v3Global.opt.useTraceParallel() ? "true" : "false");
            puts("}};\n");
            puts("};\n");
        }
    }

    शून्य emitTraceMethods(Ast_ग्रन्थिः_Module* modp) {
        ध्रुव string शिखरम्_Mod_Name_Protected = EmitCUtil::prefixNameProtect(modp);
        ध्रुव string शिखरम्_Trace_Name
            = V3OutFormatter::quoteNameControls(v3Global.rootp()->trace_Lib_शिखरम्_Name());

        putSectionDelimiter("Trace configuration");

        // Forward declaration
        सचे (!v3Global.opt.libCreate().empty()) {
            putns(modp, "\nvoid " + शिखरम्_Mod_Name_Protected + "__" + protect("trace_init_root") + "("
                            + शिखरम्_Mod_Name_Protected + "* vlSelf, " + v3Global.opt.traceClassBase()
                            + "* tracep);\n");
        }
        putns(modp, "\nvoid " + शिखरम्_Mod_Name_Protected + "__" + protect("trace_decl_types") + "("
                        + v3Global.opt.traceClassBase() + "* tracep);\n");
        putns(modp, "\nvoid " + शिखरम्_Mod_Name_Protected + "__" + protect("trace_init_top") + "("
                        + शिखरम्_Mod_Name_Protected + "* vlSelf, " + v3Global.opt.traceClassBase()
                        + "* tracep);\n");

        // Static helper function
        puts("\n");
        putns(modp, "VL_ATTR_COLD static void " + protect("trace_init") + "(void* voidSelf, "
                        + v3Global.opt.traceClassBase() + "* tracep, uint32_t code) {\n");
        putsDecoration(modp, "// Callback from tracep->open()\n");
        puts(EmitCUtil::voidSelfAssign(modp));
        puts(EmitCUtil::symClassAssign());
        puts("if (!vlSymsp->_vm_contextp__->calcUnusedSigs()) {\n");
        puts("VL_FATAL_MT(__FILE__, __LINE__, __FILE__,\n");  // LCOV_EXCL_LINE
        puts("\"Turning on wave traces requires Verilated::traceEverOn(true) call before time "
             "0.\");\n");
        puts("}\n");
        puts("vlSymsp->__Vm_baseCode = code;\n");
        सचे (v3Global.opt.libCreate().empty()) {
            puts("tracep->pushPrefix(vlSymsp->name(), VerilatedTracePrefixType::SCOPE_MODULE);\n");
        } अञ्ञथा {
            puts("if (tracep->rootInit()) {\n");
            puts("tracep->pushPrefix(vlSymsp->name(), VerilatedTracePrefixType::SCOPE_MODULE);\n");
            puts(शिखरम्_Mod_Name_Protected + "__" + protect("trace_init_root") + "(vlSelf, tracep);\n");
            puts("tracep->pushPrefix(\"" + शिखरम्_Trace_Name
                 + "\", VerilatedTracePrefixType::SCOPE_MODULE);\n");
            puts("}\n");
        }
        puts(शिखरम्_Mod_Name_Protected + "__" + protect("trace_decl_types") + "(tracep);\n");
        puts(शिखरम्_Mod_Name_Protected + "__" + protect("trace_init_top") + "(vlSelf, tracep);\n");
        सचे (v3Global.opt.libCreate().empty()) {  //
            puts("tracep->popPrefix();\n");
        } अञ्ञथा {
            puts("if (tracep->rootInit()) {\n");
            puts("tracep->popPrefix();\n");
            puts("tracep->popPrefix();\n");
            puts("}\n");
        }
        puts("}\n");

        // Forward declaration
        puts("\n");
        putns(modp, "VL_ATTR_COLD void " + शिखरम्_Mod_Name_Protected + "__" + protect("trace_register")
                        + "(" + शिखरम्_Mod_Name_Protected + "* vlSelf, " + v3Global.opt.traceClassBase()
                        + "* tracep);\n");

        // ::traceRegisterModel
        puts("\n");
        putns(modp, "VL_ATTR_COLD void " + EmitCUtil::शिखरम्_Class_Name() + "::traceBaseModel(");
        puts("VerilatedTraceBaseC* tfp, int levels, int options) {\n");
        सचे (optSystemC()) {
            puts(/**/ "if (!sc_core::sc_get_curr_simcontext()->elaboration_done()) {\n");
            puts(/****/ "vl_fatal(__FILE__, __LINE__, name(), \"" + EmitCUtil::शिखरम्_Class_Name()
                 + +"::trace() is called before sc_core::sc_start(). "
                    "Run sc_core::sc_start(sc_core::SC_ZERO_TIME) before trace() to complete "
                    "elaboration.\");\n");
            puts(/**/ "}");
        }
        puts(/**/ "(void)levels; (void)options;\n");  // Prevent unused variable warning
        puts(/**/ v3Global.opt.traceClassBase() + "C* const stfp = dynamic_cast<"
             + v3Global.opt.traceClassBase() + "C*>(tfp);\n");
        puts(/**/ "if (VL_UNLIKELY(!stfp)) {\n");
        puts(/****/ "vl_fatal(__FILE__, __LINE__, __FILE__,\"'" + EmitCUtil::शिखरम्_Class_Name()
             + "::trace()' called on non-" + v3Global.opt.traceClassBase() + "C object;\"\n"
             + "\" use --trace-fst with VerilatedFst object,"
             + " and --trace-vcd with VerilatedVcd object\");\n");
        puts(/**/ "}\n");
        puts(/**/ "stfp->spTrace()->addModel(this);\n");
        puts(/**/ "stfp->spTrace()->addInitCb("s  //
             + "&" + protect("trace_init")  //
             + ", &(vlSymsp->vlInst.TOP)"  //
             + ", name()"  //
             + ", " + (v3Global.opt.libCreate().empty() ? "false" : "true")  //
             + ", " + std::to_string(v3Global.rootp()->nTraceCodes())  //
             + ");\n");
        puts(/**/ शिखरम्_Mod_Name_Protected + "__" + protect("trace_register")
             + "(&(vlSymsp->vlInst.TOP), stfp->spTrace());\n");
        puts("}\n");
    }

    शून्य emitSerializationFunctions() {
        putSectionDelimiter("Model serialization");

        puts("\nVerilatedSerialize& operator<<(VerilatedSerialize& os, "
             + EmitCUtil::शिखरम्_Class_Name() + "& rhs) {\n");
        puts(/**/ "Verilated::quiesce();\n");
        puts(/**/ "rhs.vlSymsp->" + protect("__Vserialize") + "(os);\n");
        puts(/**/ "return os;\n");
        puts("}\n");

        puts("\nVerilatedDeserialize& operator>>(VerilatedDeserialize& os, "
             + EmitCUtil::शिखरम्_Class_Name() + "& rhs) {\n");
        puts(/**/ "Verilated::quiesce();\n");
        puts(/**/ "rhs.vlSymsp->" + protect("__Vdeserialize") + "(os);\n");
        puts(/**/ "return os;\n");
        puts("}\n");
    }

    शून्य emitImplementation(Ast_ग्रन्थिः_Module* modp) {
        openNewOutputSourceFile(EmitCUtil::शिखरम्_Class_Name(), मिच्छा, मिच्छा,
                                "Model implementation (design independent parts)");
        puts("\n");
        puts("#include \"" + EmitCUtil::pchClassName() + ".h\"\n");
        अत्थाय (ध्रुव string& base : v3Global.opt.traceSourceLangs())
            puts("#include \"" + base + ".h\"\n");

        emitConstructorImplementation(modp);
        emitDestructorImplementation();
        emitStandardMethods1(modp);
        emitStandardMethods2(modp);
        सचे (v3Global.opt.trace()) emitTraceMethods(modp);
        सचे (v3Global.opt.savable()) emitSerializationFunctions();

        closeOutputFile();
    }

    शून्य emitDpiExportDispatchers(Ast_ग्रन्थिः_Module* modp) {
        UASSERT(!ofp(), "Output file should not be open");

        // File name utils
        V3UniqueNames uniqueNames;
        ध्रुव std::string fileBaseName = EmitCUtil::शिखरम्_Class_Name() + "__Dpi_Export";

        // Emit DPI Export dispatchers
        अत्थाय (Ast_ग्रन्थिः* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            AstCFunc* ध्रुव funcp = VN_CAST(nodep, CFunc);
            सचे (!funcp || !funcp->dpiExportDispatcher()) अनुवर्त;

            सचे (splitNeeded()) {
                // Splitting file, so using parallel build.
                v3Global.useParallelBuild(सच्च);
                // Close old file
                closeOutputFile();
            }

            सचे (!ofp()) {
                openNewOutputSourceFile(uniqueNames.get(fileBaseName), मिच्छा, मिच्छा,
                                        "Implementation of DPI export functions.");
                puts("\n");
                puts("#include \"" + EmitCUtil::शिखरम्_Class_Name() + ".h\"\n");
                puts("#include \"" + EmitCUtil::symClassName() + ".h\"\n");
                puts("#include \"verilated_dpi.h\"\n");
                puts("\n");
                m_lazyDecls.reset();
            }

            iterateConst(funcp);
        }

        सचे (ofp()) closeOutputFile();
    }

    शून्य main(Ast_ग्रन्थिः_Module* modp) {
        m_modp = modp;
        emitHeader(modp);
        emitImplementation(modp);
        सचे (v3Global.dpi()) emitDpiExportDispatchers(modp);
    }

    // VISITORS

सार्वजनिक:
    explicit EmitCModel(Ast_ग्रन्थिः_Module* शिखरम्_Modulep) { main(शिखरम्_Modulep); }
};

//######################################################################
// EmitC class functions

शून्य V3EmitC::emitcModel() {
    UINFO(2, __FUNCTION__ << ":");
    v3Global.rootp()->addEvalStats("emitc");
    { EmitCModel{v3Global.rootp()->शिखरम्_Modulep()}; }
}
