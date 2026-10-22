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
    using CFuncVector = std::vector<अचर AstCFunc*>;

    // MEMBERS
    V3UniqueNames m_uniqueNames;  // For generating unique file names

    // METHODS

    CFuncVector findFuncps(std::function<तार्किक(अचर AstCFunc*)> cb) {
        CFuncVector funcps;
        लागि (Ast_गाँठो* nodep = m_modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            यदि (अचर AstCFunc* अचर funcp = VN_CAST(nodep, CFunc)) {
                यदि (cb(funcp)) funcps.थप_back(funcp);
            }
        }
        stable_sort(funcps.begin(), funcps.end(),
                    [](अचर Ast_गाँठो* ap, अचर Ast_गाँठो* bp) { फर्काऊ ap->name() < bp->name(); });
        फर्काऊ funcps;
    }

    // Whether 'funcp' is a DPI function with arguments of unpacked structures
    स्थिर तार्किक isDpiPassingStructs(अचर AstCFunc* funcp) {
        यदि (!funcp->dpiImportPrototype() && !funcp->dpiExportDispatcher()) फर्काऊ असत्य;
        लागि (अचर Ast_गाँठो* argp = funcp->argsp(); argp; argp = argp->nextp()) {
            अचर AstVar* अचर varp = VN_CAST(argp, Var);
            यदि (varp && varp->dtypep()->containsUnpackedStruct()) फर्काऊ सत्य;
        }
        फर्काऊ असत्य;
    }

    शून्य putSectionDelimiter(अचर string& name) {
        माला_दो("\n");
        माला_दो("//============================================================\n");
        माला_दो("// " + name + "\n");
    }

    शून्य emitHeader(Ast_गाँठो_Module* modp) {
        openNewOutputHeaderFile(EmitCUtil::शीर्ष_Class_Name(), "Primary model header");
        माला_दो("//\n");
        माला_दो("// This header should be included by all source files instantiating the design.\n");
        माला_दो("// The class here is then constructed to instantiate the design.\n");
        माला_दो("// See the Verilator manual for examples.\n");

        ofp()->putsGuard();

        // Include files
        माला_दो("\n");
        ofp()->puts_Int_शीर्ष_Include();

        माला_दो("#include \"verilated.h\"\n");
        यदि (v3Global.opt.systemC()) माला_दो("#include \"verilated_sc.h\"\n");
        यदि (v3Global.opt.mtasks()) माला_दो("#include \"verilated_threads.h\"\n");
        यदि (v3Global.opt.savable()) माला_दो("#include \"verilated_save.h\"\n");
        यदि (v3Global.opt.coverage()) माला_दो("#include \"verilated_cov.h\"\n");
        यदि (v3Global.opt.coverage() || v3Global.useCovergroup())
            माला_दो("#include \"verilated_covergroup.h\"\n");
        यदि (v3Global.dpi()) माला_दो("#include \"svdpi.h\"\n");
        // The model needs the C structures DPI functions pass, which the DPI header declares
        यदि (!findFuncps(isDpiPassingStructs).empty()) {
            माला_दो("#include \"" + EmitCUtil::शीर्ष_Class_Name() + "__Dpi.h\"\n");
        }

        // Declare foreign instances up front to make C++ happy
        माला_दो("\n");
        माला_दो("class " + EmitCUtil::symClassName() + ";\n");
        माला_दो("class " + EmitCUtil::prefixNameProtect(modp) + ";\n");  // For rootp pointer only
        लागि (अचर string& base : v3Global.opt.traceClassLangs()) माला_दो("class " + base + ";\n");
        emitModCUse(modp, VUseType::INT_FWD_CLASS);  // Note: This is needed for cell forwarding

        माला_दो("\n");

        माला_दो("// This class is the main interface to the Verilated model\n");
        putns(modp, "class alignas(VL_CACHE_LINE_BYTES) " + EmitCUtil::शीर्ष_Class_Name()
                        + " VL_NOT_FINAL : ");
        यदि (optSystemC()) {
            // SC_MODULE, but with multiple-inheritance of VerilatedModel
            माला_दो("public ::sc_core::sc_module, ");
        }
        माला_दो("public VerilatedModel {\n");
        // The symbol table references the model's evaluation state
        माला_दो("friend class " + EmitCUtil::symClassName() + ";\n");

        ofp()->resetPrivate();
        ofp()->putsPrivate(सत्य);  // private:

        माला_दो("// Symbol table holding complete model state (owned by this class)\n");
        माला_दो(EmitCUtil::symClassName() + "* const vlSymsp;\n");
        माला_दो("// Evaluation loop\n");
        माला_दो("VerilatedEvalLoop m_evalLoop;\n");

        माला_दो("\n");
        ofp()->putsPrivate(असत्य);  // public:

        माला_दो("\n// CONSTEXPR CAPABILITIES\n");
        माला_दो("// Verilated with --trace?\n");
        माला_दो("static constexpr bool traceCapable = "s
             + (v3Global.opt.trace() ? "true;\n"s : "false;\n"s));

        // User accessible IO
        माला_दो("\n// PORTS\n"
             "// The application code writes and reads these signals to\n"
             "// propagate new values into/out from the Verilated model.\n");
        लागि (अचर Ast_गाँठो* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            यदि (अचर AstVar* अचर varp = VN_CAST(nodep, Var)) {
                यदि (varp->isPrimaryIO()) {  //
                    emitVarDecl(varp, /* asRef: */ सत्य);
                }
            }
        }
        यदि (v3Global.opt.emitAccessors()) {
            माला_दो("\n// ACCESSORS\n"
                 "// The application code should use these methods to\n"
                 "// propagate new values into/out from the Verilated model\n"
                 "// instead of using signal variables directly.\n");
            लागि (अचर Ast_गाँठो* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                यदि (अचर AstVar* अचर varp = VN_CAST(nodep, Var)) {
                    यदि (varp->isPrimaryIO()) emitVarAccessors(varp);
                }
            }
        }

        // Cells instantiated by the top level (for access to /* verilator public */)
        माला_दो("\n// CELLS\n"
             "// Public to allow access to /* verilator public */ items.\n"
             "// Otherwise the application code can consider these internals.\n");
        लागि (Ast_गाँठो* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            यदि (अचर AstCell* अचर cellp = VN_CAST(nodep, Cell)) {
                यदि (cellp->modp()->isConstPool()) जारी;  // Special emit rules
                putns(cellp, EmitCUtil::prefixNameProtect(cellp->modp()) + "* const "
                                 + cellp->nameProtect() + ";\n");
            }
        }

        // root instance pointer (for access to internals, including public_flat items).
        माला_दो("\n// Root instance pointer to allow access to model internals,\n"
             "// including inlined /* verilator public_flat_* */ items.\n");
        माला_दो(EmitCUtil::prefixNameProtect(modp) + "* const rootp;\n");

        माला_दो("\n");
        ofp()->putsPrivate(असत्य);  // public:
        माला_दो("// CONSTRUCTORS\n");
        यदि (optSystemC()) {
            माला_दो("SC_CTOR(" + EmitCUtil::शीर्ष_Class_Name() + ");\n");
            माला_दो("virtual ~" + EmitCUtil::शीर्ष_Class_Name() + "();\n");
        } अन्यथा {
            माला_दो("/// Construct the model; called by application code\n");
            माला_दो("/// If contextp is null, then the model will use the default global "
                 "context\n");
            माला_दो("/// If name is \"\", then makes a wrapper with a\n");
            माला_दो("/// single model invisible with respect to DPI scope names.\n");
            माला_दो("explicit " + EmitCUtil::शीर्ष_Class_Name() + "(VerilatedContext* contextp,"
                 + " const char* name = \"TOP\");\n");
            माला_दो("explicit " + EmitCUtil::शीर्ष_Class_Name() + "(const char* name = \"TOP\");\n");
            माला_दो("/// Destroy the model; called (often implicitly) by application code\n");
            माला_दो("virtual ~" + EmitCUtil::शीर्ष_Class_Name() + "();\n");
        }
        ofp()->putsPrivate(सत्य);
        माला_दो("VL_UNCOPYABLE(" + EmitCUtil::शीर्ष_Class_Name() + ");  ///< Copying not allowed\n");

        माला_दो("\n");
        ofp()->putsPrivate(असत्य);  // public:
        माला_दो("// API METHODS\n");
        अचर string callEvalEndStep
            = (v3Global.needTraceDumper() && !optSystemC()) ? "eval_end_step(); " : "";
        यदि (optSystemC()) {
            ofp()->putsPrivate(सत्य);  ///< eval() is invoked by our sensitive() calls.
        }
        यदि (!optSystemC()) {
            माला_दो("/// Evaluate the model.  Application must call when inputs change.\n");
        }
        यदि (optSystemC() && v3Global.usesTiming()) {
            माला_दो("void eval();\n");
        } अन्यथा {
            माला_दो("void eval() { eval_step(); " + callEvalEndStep + "}\n");
        }
        यदि (!optSystemC()) {
            माला_दो("/// Evaluate when calling multiple units/models per time step.\n");
        }
        माला_दो("void eval_step();\n");
        यदि (!optSystemC()) {
            माला_दो("/// Evaluate at end of a timestep for tracing, when using eval_step().\n");
            माला_दो("/// Application must call after all eval() and before time changes.\n");
            माला_दो("void eval_end_step()");
            यदि (callEvalEndStep == "") {
                माला_दो(" {}\n");
            } अन्यथा {
                माला_दो(";\n");
            }
        }
        यदि (!optSystemC()) {
            माला_दो("/// Simulation complete, run final blocks.  Application "
                 "must call on completion.\n");
        }
        ofp()->putsPrivate(असत्य);  // public:
        माला_दो("void final();\n");

        माला_दो("/// Are there scheduled events to handle?\n");
        माला_दो("bool eventsPending();\n");
        माला_दो("/// Returns time at next time slot. Aborts if !eventsPending()\n");
        माला_दो("uint64_t nextTimeSlot();\n");

        यदि (v3Global.opt.trace() || !optSystemC()) {
            माला_दो("/// Trace signals in the model; called by application code\n");
            // Backward-compatible usage of calling trace() on the model - now part of context
            माला_दो("void trace(VerilatedTraceBaseC* tfp, int levels, int options = 0) {"
                 " contextp()->trace(tfp, levels, options); }\n");
        }
        यदि (v3Global.opt.trace() && optSystemC()) {
            माला_दो("/// SC tracing; avoid overloaded virtual function lint warning\n");
            माला_दो("void trace(sc_core::sc_trace_file* tfp) const override { "
                 "::sc_core::sc_module::trace(tfp); }\n");
        }

        यदि (!optSystemC()) {
            माला_दो("/// Retrieve name of this model instance (as passed to constructor).\n");
            माला_दो("const char* name() const;\n");
        }

        // Emit DPI export dispatcher declarations
        {
            अचर CFuncVector funcps
                = findFuncps([](अचर AstCFunc* nodep) { फर्काऊ nodep->dpiExportDispatcher(); });
            यदि (!funcps.empty()) {
                माला_दो("\n/// DPI Export functions\n");
                लागि (अचर AstCFunc* funcp : funcps) emitCFuncDecl(funcp, modp);
            }
        }

        यदि (v3Global.opt.savable()) {
            माला_दो("\n");
            माला_दो("// Serialization functions\n");
            माला_दो("friend VerilatedSerialize& operator<<(VerilatedSerialize& os, "  //
                 + EmitCUtil::शीर्ष_Class_Name() + "& rhs);\n");
            माला_दो("friend VerilatedDeserialize& operator>>(VerilatedDeserialize& os, "
                 + EmitCUtil::शीर्ष_Class_Name() + "& rhs);\n");
        }

        माला_दो("\n// Abstract methods from VerilatedModel\n");
        माला_दो("const char* hierName() const override final;\n");
        माला_दो("const char* modelName() const override final;\n");
        माला_दो("unsigned threads() const override final;\n");
        माला_दो("/// Prepare for cloning the model at the process level (e.g. fork in Linux)\n");
        माला_दो("/// Release necessary resources. Called before cloning.\n");
        माला_दो("void prepareClone() const;\n");
        माला_दो("/// Re-init after cloning the model at the process level (e.g. fork in Linux)\n");
        माला_दो("/// Re-allocate necessary resources. Called after cloning.\n");
        माला_दो("void atClone() const;\n");
        यदि (v3Global.opt.trace()) {
            माला_दो("std::unique_ptr<VerilatedTraceConfig> traceConfig() const override final;\n");
        }

        ofp()->putsPrivate(सत्य);  // private:
        माला_दो("\n// Internal functions - the model's evaluation entry points\n");
        माला_दो("void evalBegin() override final;\n");
        माला_दो("void evalEnd() override final;\n");
        लागि (पूर्णाङ्क i = 0; i < VEval::_ENUM_END; ++i) {
            अचर VEval eval{i};
            // Only the iterated regions report whether they did any work
            माला_दो(eval.isIterated() ? "bool "s : "void "s);
            माला_दो(eval.evalMethod() + (eval.firstIteration() ? "(bool firstIteration)"s : "()"s));
            माला_दो(" override final;\n");
        }
        लागि (पूर्णाङ्क i = 0; i < VEval::_ENUM_END; ++i) {
            अचर VEval eval{i};
            यदि (!eval.hasTriggers()) जारी;
            माला_दो("void " + eval.dumpTriggersMethod() + "() override final;\n");
        }

        माला_दो("\n// Internal functions - trace registration\n");
        माला_दो("void traceBaseModel(VerilatedTraceBaseC* tfp, int levels, int options);\n");

        माला_दो("};\n");

        ofp()->putsEndGuard();

        closeOutputFile();
    }

    शून्य emitConstructorImplementation(Ast_गाँठो_Module* modp) {
        putSectionDelimiter("Constructors");

        माला_दो("\n");
        putns(modp, EmitCUtil::शीर्ष_Class_Name() + "::" + EmitCUtil::शीर्ष_Class_Name());
        यदि (optSystemC()) {
            माला_दो("(sc_core::sc_module_name /* unused */)\n");
            माला_दो("    : VerilatedModel{*Verilated::threadContextp()}\n");
            माला_दो("    , vlSymsp{new " + EmitCUtil::symClassName()
                 + "(contextp(), name(), this)}\n");
        } अन्यथा {
            माला_दो("(VerilatedContext* _vcontextp__, const char* _vcname__)\n");
            माला_दो("    : VerilatedModel{*_vcontextp__}\n");
            माला_दो("    , vlSymsp{new " + EmitCUtil::symClassName()
                 + "(contextp(), _vcname__, this)}\n");
        }
        माला_दो("    , m_evalLoop{*this, /*convergeLimit:*/ "s
             + cvtToStr(v3Global.opt.convergeLimit()) + "}\n");

        // Set up IO references
        लागि (अचर Ast_गाँठो* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            यदि (अचर AstVar* अचर varp = VN_CAST(nodep, Var)) {
                यदि (varp->isPrimaryIO()) {
                    अचर string protName = varp->nameProtect();
                    माला_दो("    , ");
                    putns(varp, protName + "{vlSymsp->vlInst.TOP." + varName(varp) + "}\n");
                }
            }
        }

        // Setup cell pointers
        लागि (Ast_गाँठो* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            यदि (अचर AstCell* अचर cellp = VN_CAST(nodep, Cell)) {
                यदि (cellp->modp()->isConstPool()) जारी;  // Special emit rules
                अचर string protName = cellp->nameProtect();
                माला_दो("    , ");
                putns(cellp, protName + "{vlSymsp->vlInst.TOP." + protName + "}\n");
            }
        }

        // Setup rootp root instance pointer,
        माला_दो("    , rootp{&(vlSymsp->vlInst.TOP)}\n");

        माला_दो("{\n");
        माला_दो("// Register model with the context\n");
        माला_दो("contextp()->addModel(this);\n");
        यदि (v3Global.opt.profExec()) {
            putsDecoration(nullptr, "// Profile the evaluation loop's region loops\n");
            अचर तार्किक शीर्ष_Level = !v3Global.opt.hierChild() && v3Global.opt.libCreate().empty();
            माला_दो("m_evalLoop.profiler(vlSymsp->__Vm_executionProfilerp, /*topLevel:*/ "s
                 + (शीर्ष_Level ? "true" : "false") + ");\n");
        }
        यदि (v3Global.opt.trace())
            माला_दो("contextp()->traceBaseModelCbAdd(\n"
                 "[this](VerilatedTraceBaseC* tfp, int levels, int options) {"
                 " traceBaseModel(tfp, levels, options); });\n");

        यदि (optSystemC()) {
            // Create sensitivity list for when to evaluate the model.
            putsDecoration(nullptr, "// Sensitivities on all clocks and combinational inputs\n");
            माला_दो("SC_METHOD(eval);\n");
            यदि (v3Global.usesTiming()) {
                putsDecoration(nullptr, "// Notified by pending time delays/DPI exports\n");
                माला_दो("sensitive << vlSymsp->__Vm_wakeEvent;\n");
            }
            लागि (Ast_गाँठो* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
                यदि (अचर AstVar* अचर varp = VN_CAST(nodep, Var)) {
                    यदि (varp->isNonOutput() && (varp->isScSensitive() || varp->isPrimaryClock())) {
                        पूर्णाङ्क vects = 0;
                        // This isn't very robust and may need cleanup for other data types
                        लागि (AstUnpackArrayDType* arrayp
                             = VN_CAST(varp->dtypeSkipRefp(), UnpackArrayDType);
                             arrayp;
                             arrayp = VN_CAST(arrayp->subDTypep()->skipRefp(), UnpackArrayDType)) {
                            अचर पूर्णाङ्क vecnum = vects++;
                            UASSERT_OBJ(arrayp->hi() >= arrayp->lo(), varp,
                                        "Should have swapped msb & lsb earlier.");
                            अचर string ivar = "__Vi"s + cvtToStr(vecnum);
                            putns(varp, "for (int __Vi" + cvtToStr(vecnum) + " = "
                                            + cvtToStr(arrayp->lo()));
                            माला_दो("; " + ivar + " <= " + cvtToStr(arrayp->hi()));
                            माला_दो("; ++" + ivar + ") {\n");
                        }
                        putns(varp, "sensitive << " + varp->nameProtect());
                        लागि (पूर्णाङ्क v = 0; v < vects; ++v) माला_दो("[__Vi" + cvtToStr(v) + "]");
                        माला_दो(";\n");
                        लागि (पूर्णाङ्क v = 0; v < vects; ++v) माला_दो("}\n");
                    }
                }
            }
            माला_दो("\n");
        }

        माला_दो("}\n");

        यदि (!optSystemC()) {
            माला_दो("\n");
            माला_दो(EmitCUtil::शीर्ष_Class_Name() + "::" + EmitCUtil::शीर्ष_Class_Name()
                 + "(const char* _vcname__)\n");
            माला_दो("    : " + EmitCUtil::शीर्ष_Class_Name()
                 + "(Verilated::threadContextp(), _vcname__)\n{\n}\n");
        }
    }

    शून्य emitDestructorImplementation() {
        putSectionDelimiter("Destructor");

        माला_दो("\n");
        माला_दो(EmitCUtil::शीर्ष_Class_Name() + "::~" + EmitCUtil::शीर्ष_Class_Name() + "() {\n");
        माला_दो("delete vlSymsp;\n");
        माला_दो("}\n");
    }

    शून्य emitStandardMethods1(Ast_गाँठो_Module* modp) {
        UASSERT_OBJ(modp->is_शीर्ष(), modp, "Attempting to emitWrapEval for non-top class");

        अचर string शीर्ष_Mod_Name_Protected = EmitCUtil::prefixNameProtect(modp);
        अचर string selfDecl = "(" + शीर्ष_Mod_Name_Protected + "* vlSelf)";

        putSectionDelimiter("Evaluation function");

        // Forward declarations
        माला_दो("\n");
        माला_दो("#ifdef VL_DEBUG\n");
        माला_दो("void " + शीर्ष_Mod_Name_Protected + "__" + protect("_eval_debug_assertions") + selfDecl
             + ";\n");
        माला_दो("#endif  // VL_DEBUG\n");
        AstNetlist* अचर netlistp = v3Global.rootp();
        लागि (पूर्णाङ्क i = 0; i < VEval::_ENUM_END; ++i) {
            emitCFuncDecl(netlistp->evalFuncp(VEval{i}), modp);
        }
        लागि (पूर्णाङ्क i = 0; i < VEval::_ENUM_END; ++i) {
            अचर VEval eval{i};
            यदि (eval.hasTriggers()) emitCFuncDecl(netlistp->dumpTriggersFuncp(eval), modp);
        }

        यदि (optSystemC() && v3Global.usesTiming()) {
            // ::eval
            माला_दो("\nvoid " + EmitCUtil::शीर्ष_Class_Name() + "::eval() {\n");
            माला_दो("eval_step();\n");
            माला_दो("if (eventsPending()) {\n");
            माला_दो("sc_core::sc_time dt = sc_core::sc_time::from_value(nextTimeSlot() - "
                 "contextp()->time());\n");
            माला_दो("vlSymsp->__Vm_wakeEvent.notify(dt);\n");
            माला_दो("}\n");
            माला_दो("}\n");
        }

        // ::eval_step - the run-time library drives the whole time step
        माला_दो("\nvoid " + EmitCUtil::शीर्ष_Class_Name() + "::eval_step() {\n");
        माला_दो("VL_DEBUG_IF(VL_DBG_MSGF(\"+++++TOP Evaluate " + EmitCUtil::शीर्ष_Class_Name()
             + "::eval_step\\n\"); );\n");
        माला_दो("m_evalLoop.eval();\n");
        माला_दो("}\n");

        // ::evalBegin - prepare the model for a time step
        माला_दो("\nvoid " + EmitCUtil::शीर्ष_Class_Name() + "::evalBegin() {\n");
        माला_दो("#ifdef VL_DEBUG\n");
        putsDecoration(nullptr, "// Debug assertions\n");
        माला_दो(शीर्ष_Mod_Name_Protected + "__" + protect("_eval_debug_assertions")
             + "(&(vlSymsp->vlInst.TOP));\n");
        माला_दो("#endif  // VL_DEBUG\n");

        यदि (v3Global.opt.trace()) माला_दो("vlSymsp->__Vm_activity = true;\n");

        यदि (v3Global.hasEvents()) माला_दो("vlSymsp->clearTriggeredEvents();\n");
        यदि (v3Global.hasClasses()) माला_दो("vlSymsp->__Vm_deleter.deleteAll();\n");

        माला_दो("}\n");

        // ::evalEnd - the time step is complete
        माला_दो("\nvoid " + EmitCUtil::शीर्ष_Class_Name() + "::evalEnd() {\n");
        putsDecoration(nullptr, "// Evaluate cleanup\n");
        माला_दो("Verilated::endOfEval(vlSymsp->__Vm_evalMsgQp);\n");
        यदि (AstVar* अचर delaySchedp = v3Global.rootp()->delaySchedulerp()) {
            माला_दो("vlSymsp->vlInst.TOP.");
            माला_दो(varName(delaySchedp));
            माला_दो(".cleanupForevered();\n");
        }

        माला_दो("}\n");

        // Evaluation entry points
        लागि (पूर्णाङ्क i = 0; i < VEval::_ENUM_END; ++i) {
            अचर VEval eval{i};
            AstCFunc* अचर funcp = v3Global.rootp()->evalFuncp(eval);
            माला_दो(eval.isIterated() ? "\nbool " : "\nvoid ");
            माला_दो(EmitCUtil::शीर्ष_Class_Name() + "::" + eval.evalMethod() + "(");
            यदि (eval.firstIteration()) माला_दो("bool firstIteration");
            माला_दो(") {\n");
            यदि (eval.isIterated()) माला_दो("return ");
            माला_दो(funcNameProtect(funcp, m_modp) + "(&(vlSymsp->vlInst.TOP)");
            यदि (eval.firstIteration()) माला_दो(", firstIteration");
            माला_दो(");\n");
            माला_दो("}\n");
        }

        // The trigger dump of each region
        लागि (पूर्णाङ्क i = 0; i < VEval::_ENUM_END; ++i) {
            अचर VEval eval{i};
            यदि (!eval.hasTriggers()) जारी;
            माला_दो("\nVL_ATTR_COLD void " + EmitCUtil::शीर्ष_Class_Name()
                 + "::" + eval.dumpTriggersMethod() + "() {\n");
            माला_दो(funcNameProtect(v3Global.rootp()->dumpTriggersFuncp(eval), m_modp)
                 + "(&(vlSymsp->vlInst.TOP));\n");
            माला_दो("}\n");
        }
    }

    शून्य emitStandardMethods2(Ast_गाँठो_Module* modp) {
        // ::eval_end_step
        यदि (v3Global.needTraceDumper() && !optSystemC()) {
            माला_दो("\n");
            putns(modp, "void " + EmitCUtil::शीर्ष_Class_Name() + "::eval_end_step() {\n");
            माला_दो("VL_DEBUG_IF(VL_DBG_MSGF(\"+eval_end_step " + EmitCUtil::शीर्ष_Class_Name()
                 + "::eval_end_step\\n\"); );\n");
            माला_दो("#ifdef VM_TRACE\n");
            putsDecoration(nullptr, "// Tracing\n");
            // SystemC's eval loop deals with calling trace, not us
            माला_दो("if (VL_UNLIKELY(vlSymsp->__Vm_dumping)) vlSymsp->_traceDump();\n");
            माला_दो("#endif  // VM_TRACE\n");
            माला_दो("}\n");
        }

        putSectionDelimiter("Events and timing");
        यदि (स्वतः* अचर delaySchedp = v3Global.rootp()->delaySchedulerp()) {
            putns(modp, "bool " + EmitCUtil::शीर्ष_Class_Name()
                            + "::eventsPending() { return !vlSymsp->vlInst.TOP.");
            माला_दो(varName(delaySchedp));
            माला_दो(".empty() && !contextp()->gotFinish(); }\n\n");

            putns(modp, "uint64_t " + EmitCUtil::शीर्ष_Class_Name()
                            + "::nextTimeSlot() { return vlSymsp->vlInst.TOP.");
            माला_दो(varName(delaySchedp));
            माला_दो(".nextTimeSlot(); }\n");
        } अन्यथा {
            putns(modp,
                  "bool " + EmitCUtil::शीर्ष_Class_Name() + "::eventsPending() { return false; }\n\n");
            माला_दो("uint64_t " + EmitCUtil::शीर्ष_Class_Name() + "::nextTimeSlot() {\n");
            माला_दो("VL_FATAL_MT(__FILE__, __LINE__, \"\", \"No delays in the design\");\n");
            माला_दो("return 0;\n}\n");
        }

        putSectionDelimiter("Utilities");

        यदि (!optSystemC()) {
            // ::name
            माला_दो("\n");
            putns(modp, "const char* " + EmitCUtil::शीर्ष_Class_Name() + "::name() const {\n");
            माला_दो(/**/ "return vlSymsp->name();\n");
            माला_दो("}\n");
        }

        putSectionDelimiter("Invoke final blocks");
        // ::final
        माला_दो("\nVL_ATTR_COLD void " + EmitCUtil::शीर्ष_Class_Name() + "::final() {\n");
        माला_दो("contextp()->executingFinal(true);\n");
        माला_दो("evalFinal();\n");
        माला_दो("contextp()->executingFinal(false);\n");
        माला_दो("}\n");

        putSectionDelimiter("Implementations of abstract methods from VerilatedModel\n");
        putns(modp, "const char* " + EmitCUtil::शीर्ष_Class_Name()
                        + "::hierName() const { return vlSymsp->name(); }\n");
        putns(modp, "const char* " + EmitCUtil::शीर्ष_Class_Name() + "::modelName() const { return \""
                        + EmitCUtil::शीर्ष_Class_Name() + "\"; }\n");
        अचर पूर्णाङ्क threads = v3Global.opt.hierChild()
                                ? v3Global.opt.threads()
                                : std::max(v3Global.opt.threads(), v3Global.opt.hierThreads());
        putns(modp, "unsigned " + EmitCUtil::शीर्ष_Class_Name() + "::threads() const { return "
                        + cvtToStr(threads) + "; }\n");
        putns(modp, "void " + EmitCUtil::शीर्ष_Class_Name()
                        + "::prepareClone() const { contextp()->prepareClone(); }\n");
        putns(modp, "void " + EmitCUtil::शीर्ष_Class_Name() + "::atClone() const {\n");
        यदि (v3Global.opt.threads() > 1) {
            माला_दो("vlSymsp->__Vm_threadPoolp = static_cast<VlThreadPool*>(");
        }
        माला_दो("contextp()->threadPoolpOnClone()");
        यदि (v3Global.opt.threads() > 1) माला_दो(")");
        माला_दो(";\n}\n");

        यदि (v3Global.opt.trace()) {
            putns(modp, "std::unique_ptr<VerilatedTraceConfig> " + EmitCUtil::शीर्ष_Class_Name()
                            + "::traceConfig() const {\n");
            माला_दो("return std::unique_ptr<VerilatedTraceConfig>{new VerilatedTraceConfig{");
            माला_दो(v3Global.opt.useTraceParallel() ? "true" : "false");
            माला_दो("}};\n");
            माला_दो("};\n");
        }
    }

    शून्य emitTraceMethods(Ast_गाँठो_Module* modp) {
        अचर string शीर्ष_Mod_Name_Protected = EmitCUtil::prefixNameProtect(modp);
        अचर string शीर्ष_Trace_Name
            = V3OutFormatter::quoteNameControls(v3Global.rootp()->trace_Lib_शीर्ष_Name());

        putSectionDelimiter("Trace configuration");

        // Forward declaration
        यदि (!v3Global.opt.libCreate().empty()) {
            putns(modp, "\nvoid " + शीर्ष_Mod_Name_Protected + "__" + protect("trace_init_root") + "("
                            + शीर्ष_Mod_Name_Protected + "* vlSelf, " + v3Global.opt.traceClassBase()
                            + "* tracep);\n");
        }
        putns(modp, "\nvoid " + शीर्ष_Mod_Name_Protected + "__" + protect("trace_decl_types") + "("
                        + v3Global.opt.traceClassBase() + "* tracep);\n");
        putns(modp, "\nvoid " + शीर्ष_Mod_Name_Protected + "__" + protect("trace_init_top") + "("
                        + शीर्ष_Mod_Name_Protected + "* vlSelf, " + v3Global.opt.traceClassBase()
                        + "* tracep);\n");

        // Static helper function
        माला_दो("\n");
        putns(modp, "VL_ATTR_COLD static void " + protect("trace_init") + "(void* voidSelf, "
                        + v3Global.opt.traceClassBase() + "* tracep, uint32_t code) {\n");
        putsDecoration(modp, "// Callback from tracep->open()\n");
        माला_दो(EmitCUtil::voidSelfAssign(modp));
        माला_दो(EmitCUtil::symClassAssign());
        माला_दो("if (!vlSymsp->_vm_contextp__->calcUnusedSigs()) {\n");
        माला_दो("VL_FATAL_MT(__FILE__, __LINE__, __FILE__,\n");  // LCOV_EXCL_LINE
        माला_दो("\"Turning on wave traces requires Verilated::traceEverOn(true) call before time "
             "0.\");\n");
        माला_दो("}\n");
        माला_दो("vlSymsp->__Vm_baseCode = code;\n");
        यदि (v3Global.opt.libCreate().empty()) {
            माला_दो("tracep->pushPrefix(vlSymsp->name(), VerilatedTracePrefixType::SCOPE_MODULE);\n");
        } अन्यथा {
            माला_दो("if (tracep->rootInit()) {\n");
            माला_दो("tracep->pushPrefix(vlSymsp->name(), VerilatedTracePrefixType::SCOPE_MODULE);\n");
            माला_दो(शीर्ष_Mod_Name_Protected + "__" + protect("trace_init_root") + "(vlSelf, tracep);\n");
            माला_दो("tracep->pushPrefix(\"" + शीर्ष_Trace_Name
                 + "\", VerilatedTracePrefixType::SCOPE_MODULE);\n");
            माला_दो("}\n");
        }
        माला_दो(शीर्ष_Mod_Name_Protected + "__" + protect("trace_decl_types") + "(tracep);\n");
        माला_दो(शीर्ष_Mod_Name_Protected + "__" + protect("trace_init_top") + "(vlSelf, tracep);\n");
        यदि (v3Global.opt.libCreate().empty()) {  //
            माला_दो("tracep->popPrefix();\n");
        } अन्यथा {
            माला_दो("if (tracep->rootInit()) {\n");
            माला_दो("tracep->popPrefix();\n");
            माला_दो("tracep->popPrefix();\n");
            माला_दो("}\n");
        }
        माला_दो("}\n");

        // Forward declaration
        माला_दो("\n");
        putns(modp, "VL_ATTR_COLD void " + शीर्ष_Mod_Name_Protected + "__" + protect("trace_register")
                        + "(" + शीर्ष_Mod_Name_Protected + "* vlSelf, " + v3Global.opt.traceClassBase()
                        + "* tracep);\n");

        // ::traceRegisterModel
        माला_दो("\n");
        putns(modp, "VL_ATTR_COLD void " + EmitCUtil::शीर्ष_Class_Name() + "::traceBaseModel(");
        माला_दो("VerilatedTraceBaseC* tfp, int levels, int options) {\n");
        यदि (optSystemC()) {
            माला_दो(/**/ "if (!sc_core::sc_get_curr_simcontext()->elaboration_done()) {\n");
            माला_दो(/****/ "vl_fatal(__FILE__, __LINE__, name(), \"" + EmitCUtil::शीर्ष_Class_Name()
                 + +"::trace() is called before sc_core::sc_start(). "
                    "Run sc_core::sc_start(sc_core::SC_ZERO_TIME) before trace() to complete "
                    "elaboration.\");\n");
            माला_दो(/**/ "}");
        }
        माला_दो(/**/ "(void)levels; (void)options;\n");  // Prevent unused variable warning
        माला_दो(/**/ v3Global.opt.traceClassBase() + "C* const stfp = dynamic_cast<"
             + v3Global.opt.traceClassBase() + "C*>(tfp);\n");
        माला_दो(/**/ "if (VL_UNLIKELY(!stfp)) {\n");
        माला_दो(/****/ "vl_fatal(__FILE__, __LINE__, __FILE__,\"'" + EmitCUtil::शीर्ष_Class_Name()
             + "::trace()' called on non-" + v3Global.opt.traceClassBase() + "C object;\"\n"
             + "\" use --trace-fst with VerilatedFst object,"
             + " and --trace-vcd with VerilatedVcd object\");\n");
        माला_दो(/**/ "}\n");
        माला_दो(/**/ "stfp->spTrace()->addModel(this);\n");
        माला_दो(/**/ "stfp->spTrace()->addInitCb("s  //
             + "&" + protect("trace_init")  //
             + ", &(vlSymsp->vlInst.TOP)"  //
             + ", name()"  //
             + ", " + (v3Global.opt.libCreate().empty() ? "false" : "true")  //
             + ", " + std::to_string(v3Global.rootp()->nTraceCodes())  //
             + ");\n");
        माला_दो(/**/ शीर्ष_Mod_Name_Protected + "__" + protect("trace_register")
             + "(&(vlSymsp->vlInst.TOP), stfp->spTrace());\n");
        माला_दो("}\n");
    }

    शून्य emitSerializationFunctions() {
        putSectionDelimiter("Model serialization");

        माला_दो("\nVerilatedSerialize& operator<<(VerilatedSerialize& os, "
             + EmitCUtil::शीर्ष_Class_Name() + "& rhs) {\n");
        माला_दो(/**/ "Verilated::quiesce();\n");
        माला_दो(/**/ "rhs.vlSymsp->" + protect("__Vserialize") + "(os);\n");
        माला_दो(/**/ "return os;\n");
        माला_दो("}\n");

        माला_दो("\nVerilatedDeserialize& operator>>(VerilatedDeserialize& os, "
             + EmitCUtil::शीर्ष_Class_Name() + "& rhs) {\n");
        माला_दो(/**/ "Verilated::quiesce();\n");
        माला_दो(/**/ "rhs.vlSymsp->" + protect("__Vdeserialize") + "(os);\n");
        माला_दो(/**/ "return os;\n");
        माला_दो("}\n");
    }

    शून्य emitImplementation(Ast_गाँठो_Module* modp) {
        openNewOutputSourceFile(EmitCUtil::शीर्ष_Class_Name(), असत्य, असत्य,
                                "Model implementation (design independent parts)");
        माला_दो("\n");
        माला_दो("#include \"" + EmitCUtil::pchClassName() + ".h\"\n");
        लागि (अचर string& base : v3Global.opt.traceSourceLangs())
            माला_दो("#include \"" + base + ".h\"\n");

        emitConstructorImplementation(modp);
        emitDestructorImplementation();
        emitStandardMethods1(modp);
        emitStandardMethods2(modp);
        यदि (v3Global.opt.trace()) emitTraceMethods(modp);
        यदि (v3Global.opt.savable()) emitSerializationFunctions();

        closeOutputFile();
    }

    शून्य emitDpiExportDispatchers(Ast_गाँठो_Module* modp) {
        UASSERT(!ofp(), "Output file should not be open");

        // File name utils
        V3UniqueNames uniqueNames;
        अचर std::string fileBaseName = EmitCUtil::शीर्ष_Class_Name() + "__Dpi_Export";

        // Emit DPI Export dispatchers
        लागि (Ast_गाँठो* nodep = modp->stmtsp(); nodep; nodep = nodep->nextp()) {
            AstCFunc* अचर funcp = VN_CAST(nodep, CFunc);
            यदि (!funcp || !funcp->dpiExportDispatcher()) जारी;

            यदि (splitNeeded()) {
                // Splitting file, so using parallel build.
                v3Global.useParallelBuild(सत्य);
                // Close old file
                closeOutputFile();
            }

            यदि (!ofp()) {
                openNewOutputSourceFile(uniqueNames.get(fileBaseName), असत्य, असत्य,
                                        "Implementation of DPI export functions.");
                माला_दो("\n");
                माला_दो("#include \"" + EmitCUtil::शीर्ष_Class_Name() + ".h\"\n");
                माला_दो("#include \"" + EmitCUtil::symClassName() + ".h\"\n");
                माला_दो("#include \"verilated_dpi.h\"\n");
                माला_दो("\n");
                m_lazyDecls.reset();
            }

            iterateConst(funcp);
        }

        यदि (ofp()) closeOutputFile();
    }

    शून्य main(Ast_गाँठो_Module* modp) {
        m_modp = modp;
        emitHeader(modp);
        emitImplementation(modp);
        यदि (v3Global.dpi()) emitDpiExportDispatchers(modp);
    }

    // VISITORS

सार्वजनिक:
    explicit EmitCModel(Ast_गाँठो_Module* शीर्ष_Modulep) { main(शीर्ष_Modulep); }
};

//######################################################################
// EmitC class functions

शून्य V3EmitC::emitcModel() {
    UINFO(2, __FUNCTION__ << ":");
    v3Global.rootp()->addEvalStats("emitc");
    { EmitCModel{v3Global.rootp()->शीर्ष_Modulep()}; }
}
