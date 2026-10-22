// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Waves tracing
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
// V3TraceDecl's Transformations:
//      Create trace init CFunc
//      For each VarScope
//          If appropriate type of signal, create a TraceDecl
//
//*************************************************************************

#ضمن "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ضمن "V3TraceDecl.h"

#ضمن "V3Ast.h"
#ضمن "V3Control.h"
#ضمن "V3EmitCBase.h"
#ضمن "V3Error.h"
#ضمن "V3File.h"
#ضمن "V3Global.h"
#ضمن "V3Number.h"
#ضمن "V3Stats.h"
#ضمن "V3UniqueNames.h"

#ضمن <cstdint>
#ضمن <functional>
#ضمن <limits>
#ضمن <tuple>
#ضمن <unordered_map>
#ضمن <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Utility class to emit path adjustments

صنف PathAdjustor final {
    File_قۇر* ثابت m_flp;  // FileLine used for created nodes
    std::function<فراغ(Ast_تۈگۈن_Stmt*)> m_emit;  // Function called with adjustment statements
    std::vector<std::string> m_دۆۋە{""};  // Stack of current paths

    ساكن constexpr حرف SEPARATOR = ' ';

عام:
    explicit PathAdjustor(File_قۇر* flp, std::function<فراغ(Ast_تۈگۈن_Stmt*)> emit)
        : m_flp{flp}
        , m_emit{emit} {}

    // Emit Prefix adjustments until the current path is 'newPath'
    فراغ adjust(ثابت string& newPath, AstCell* cellp, AstVarScope* vscp) {
        // Move up to enclosing path
        طالما (!VString::startsWith(newPath, m_دۆۋە.back())) {
            m_emit(جديد Ast_Trace_چىقىرىش_Prefix{m_flp});
            m_دۆۋە.چىقىرىش_back();
        }

        إذا (newPath == m_دۆۋە.back()) أرجع;

        ثابت VTracePrefixType lastScopeType =  //
            (cellp && VN_IS(cellp->modp(), Iface))
                    || (vscp && VN_IS(vscp->dtypep(), IfaceRefDType))
                ? VTracePrefixType::SCOPE_INTERFACE
                : VTracePrefixType::SCOPE_MODULE;
        ثابت std::string extraPrefix = newPath.substr(m_دۆۋە.back().size());
        size_t begin = 0;
        ثابت size_t last = extraPrefix.rfind(SEPARATOR);
        // Move down, one path element at a time
        طالما (true) {
            ثابت size_t end = extraPrefix.find(SEPARATOR, begin);
            إذا (end == string::npos) اكسر;
            ثابت string& extra = extraPrefix.substr(begin, end - begin);
            إذا (end == last) {
                m_emit(جديد Ast_Trace_ئىتتىرىش_Prefix{m_flp, extra, lastScopeType});
            } وإلا {
                m_emit(جديد Ast_Trace_ئىتتىرىش_Prefix{m_flp, extra, VTracePrefixType::SCOPE_MODULE});
            }
            m_دۆۋە.ئىتتىرىش_back(m_دۆۋە.back() + extra + SEPARATOR);
            begin = end + 1;
        }
        UASSERT(begin == extraPrefix.size(), "Should have consumed all of extraPrefix");
    }

    // Emit Prefix adjustments to unwind the path back to its original state
    فراغ unwind() {
        طالما (m_دۆۋە.size() > 1) {
            m_emit(جديد Ast_Trace_چىقىرىش_Prefix{m_flp});
            m_دۆۋە.چىقىرىش_back();
        }
    }
};

//######################################################################
// TraceDecl state, as a visitor of each AstNode

صنف TraceDeclVisitor final : عام VNVisitor {
    // NODE STATE
    // AstCFunc::user1()                // uint32_t.  code offset for current type
    // AstCFunc::user2()                // VarScope* for dtype functions

    // STATE
    Ast_چوققا_Scope* ثابت m_چوققا_Scopep;  // The singleton AstTopScope
    ثابت AstScope* m_currScopep = nullptr;  // Current scope being visited

    std::vector<AstCFunc*> m_چوققا_Funcps;  // Top level trace initialization functions
    std::vector<AstCFunc*> m_subFuncps;  // Trace sub functions for this scope
    std::set<ثابت AstTraceDecl*> m_declUncalledps;  // Declarations not called
    // Functions per type and variable kind (wire vs logic etc.)
    بنية Dtype_Func_ئاچقۇچ final {
        ثابت Ast_تۈگۈن_DType* dtypep;
        VVarType varType;
        منطقي operator==(ثابت Dtype_Func_ئاچقۇچ& other) ثابت {
            أرجع dtypep == other.dtypep && varType == other.varType;
        }
    };
    بنية Dtype_Func_ئاچقۇچ_Hash final {
        size_t operator()(ثابت Dtype_Func_ئاچقۇچ& ئاچقۇچ) ثابت {
            أرجع std::hash<ثابت Ast_تۈگۈن_DType*>{}(ئاچقۇچ.dtypep)
                   ^ (std::hash<uint8_t>{}(ئاچقۇچ.varType) << 1);
        }
    };
    std::unordered_map<Dtype_Func_ئاچقۇچ, AstCFunc*, Dtype_Func_ئاچقۇچ_Hash>
        m_dtypeFuncs;  // Functions per type+kind
    AstCFunc* m_dtypeFuncp = nullptr;  // Current type func
    AstCFunc* m_dtypeSubFuncp = nullptr;  // Current type sub func
    ثابت string m_dtypeArgs{", const char* name, uint32_t fidx, uint32_t c, "
                             "VerilatedTraceSigDirection direction"};  // Type func args
    AstTraceDecl* m_dtypeDeclp = nullptr;  // Current type func decl
    V3UniqueNames m_dtypeNames{""};  // Unique names for dtype funcs
    منطقي m_skipDtypeFunc = خطأ;  // Don't create a type func
    uint32_t m_offset = std::numeric_limits<uint32_t>::max();  // Offset for types
    صحيح m_چوققا_Func_Size = 0;  // Size of the top function currently being built
    صحيح m_subFuncSize = 0;  // Size of the sub function currently being built
    منطقي m_forceNewLeaf = خطأ;  // Force next addToSubFunc to start a fresh leaf
    size_t m_چوققا_Scope_يىلتىز_Func_سانى = 0;  // Top-scope init functions used only for wrapper IOs
    منطقي m_چوققا_Scope_يىلتىز_Phase = خطأ;  // Emitting top-scope wrapper IO declarations
    ثابت صحيح m_funcSizeLimit  // Maximum size of a function
        = v3Global.opt.outputSplitCTrace() ? v3Global.opt.outputSplitCTrace()
                                           : std::numeric_limits<صحيح>::max();
    // Trace init functions to for each scope
    std::unordered_map<ثابت AstScope*, std::vector<AstCFunc*>> m_scopeInitFuncps;
    // Map from hierarchical scope name to the corresponding AstScope. Note that
    // this is a many-to-one mapping for interfaces, due to interface refs.
    std::unordered_map<std::string, ثابت AstScope*> m_pathToScopep;
    // Cell initialization placeholders:
    // (parent scope, cell under parent scope, statement)
    std::vector<std::tuple<AstScope*, AstCell*, Ast_تۈگۈن_Stmt*>> m_cellInitPlaceholders;
    // Interface refs initialization placeholders:
    // (Interface ref variable, placeholder statement)
    std::vector<std::tuple<AstVarScope*, Ast_تۈگۈن_Stmt*>> m_ifaceRefInitPlaceholders;

    // A trace entry under a scope is either:
    // - A variable (including interface references)
    // - A sub scope (stored as the cell corresponding to the sub scope)
    // Note: members are non-const to allow copy during sorting
    صنف TraceEntry final {
        AstVarScope* m_vscp = nullptr;  // AstVarScope under scope being traced
        AstCell* m_cellp = nullptr;  // Sub scope (as AstCell) under scope being traced
        std::string m_path;  // Path to enclosing module in original hierarchy
        std::string m_name;  // Name of signal/subscope

        فراغ init(ثابت std::string& name, Ast_تۈگۈن* nodep, منطقي in_چوققا_Scope) {
            // Compute path in hierarchy and item name
            ثابت std::string& vcdName = Ast_تۈگۈن::vcdName(name);
            AstVar* ثابت varp = VN_CAST(nodep, Var);
            إذا (VN_IS(nodep, Cell) || VN_IS(varp->dtypep(), IfaceRefDType)) {
                // Cell or interface reference
                m_path = vcdName + " ";
                m_name.clear();
            } وإلا إذا (varp->isPrimaryIO()) {
                // Primary IO variable
                m_path = "$rootio ";
                m_name = vcdName;
            } وإلا {
                // Other Variable
                ثابت size_t pos = vcdName.rfind(' ');
                ثابت size_t pathLen = pos == std::string::npos ? 0 : pos + 1;
                m_path = vcdName.substr(0, pathLen);
                m_name = vcdName.substr(pathLen);
            }

            // When creating a --lib-create library, drop the name of the selected top module.
            // This will be replaced by the instance name in the model that uses the library, or
            // restored at runtime if the library itself is traced as the root model. Other top
            // level entities ($unit, packages, ...) keep a '$libroot' wrapper so they still have
            // a stable location in the dump.
            إذا (in_چوققا_Scope && !v3Global.opt.libCreate().empty()) {
                ثابت size_t start = m_path.find(' ');
                // Must have a prefix in the top scope with lib, as top wrapper signals not traced
                UASSERT_OBJ(start != std::string::npos, nodep, "No prefix with --lib-create");
                ثابت std::string prefix = m_path.substr(0, start);
                // Wrapper primary IOs stay under $rootio so a root-traced library can restore
                // them under the runtime C++ instance name without affecting embedded use.
                إذا (prefix == "$rootio") أرجع;
                m_path = m_path.substr(start + 1);
                إذا (v3Global.rootp()->trace_Lib_چوققا_Name() != prefix) m_path = "$libroot " + m_path;
            }
        }

    عام:
        explicit TraceEntry(ثابت AstScope* scopep, AstVarScope* vscp)
            : m_vscp{vscp} {
            init(vscp->varp()->name(), vscp->varp(), scopep->is_چوققا());
        }
        explicit TraceEntry(ثابت AstScope* scopep, AstCell* cellp)
            : m_cellp{cellp} {
            init(cellp->name(), cellp, scopep->is_چوققا());
        }
        صحيح operatorCompare(ثابت TraceEntry& b) ثابت {
            إذا (ثابت صحيح cmp = path().compare(b.path())) أرجع cmp < 0;
            إذا (ثابت صحيح cmp = fileline().operatorCompare(b.fileline())) أرجع cmp < 0;
            أرجع name() < b.name();
        }
        AstVarScope* vscp() ثابت { أرجع m_vscp; }
        AstCell* cellp() ثابت { أرجع m_cellp; }
        ثابت std::string& path() ثابت { أرجع m_path; }
        فراغ path(ثابت std::string& path) { m_path = path; }
        ثابت std::string& name() ثابت { أرجع m_name; }
        File_قۇر& fileline() ثابت { أرجع m_vscp ? *m_vscp->fileline() : *m_cellp->fileline(); }
    };
    std::vector<TraceEntry> m_entries;  // Trace entries under current scope
    AstVarScope* m_traVscp = nullptr;  // Current AstVarScope we are constructing AstTraceDecls for
    Ast_تۈگۈن_ئىپادە* m_traValuep = nullptr;  // Value expression for current signal
    string m_traName;  // Name component for current signal

    VDouble0 m_statSigs;  // Statistic tracking
    VDouble0 m_statIgnSigs;  // Statistic tracking

    // METHODS

    string vscIgnoreTrace(ثابت AstVarScope* nodep) {
        // Return true if this shouldn't be traced
        // See also similar rule in V3Coverage::varIgnoreToggle
        ثابت AstVar* ثابت varp = nodep->varp();
        إذا (!varp->isTrace()) أرجع "Verilator trace_off";
        إذا (!nodep->isTrace()) أرجع "Verilator instance trace_off";
        // Automatics (typically, excluding forks) have no persistance over
        // time, and may optimize differently when multithreadeded or hierarchical.
        // Class automatics refer to being in a class but might still be pointed
        // to by a static, so are ok.
        إذا (varp->lifetime().isAutomatic() && !varp->isClassMember() && !varp->isParam())
            أرجع "Automatic variable";

        ثابت صحيح width = recurseDTypeWidth(nodep->varp()->dtypep());
        إذا (v3Global.opt.traceMaxWidth() && width > v3Global.opt.traceMaxWidth())
            أرجع "Width " + cvtToStr(width) + " > --trace-max-width";

        ثابت string prettyName = nodep->prettyName();
        إذا (!V3Control::getScopeTraceOn(prettyName)) أرجع "Vlt scope trace_off";
        إذا (!v3Global.opt.traceUnderscore()) {
            إذا (!prettyName.empty() && prettyName[0] == '_') أرجع "Leading underscore";
            إذا (prettyName.find("._") != string::npos) أرجع "Inlined leading underscore";
        }
        أرجع ""s;
    }

    صحيح recurseDTypeWidth(ثابت Ast_تۈگۈن_DType* nodep) {
        إذا (ثابت Ast_تۈگۈن_Array_DType* adtypep = VN_CAST(nodep, تۈگۈن_Array_DType))
            أرجع recurseDTypeWidth(adtypep->subDTypep()) * adtypep->declRange().elements();
        أرجع nodep->width();
    }

    AstCFunc* newCFunc(File_قۇر* flp, ثابت string& name) {
        AstScope* ثابت چوققا_Scopep = m_چوققا_Scopep->scopep();
        AstCFunc* ثابت funcp = جديد AstCFunc{flp, name, چوققا_Scopep};
        funcp->argTypes(v3Global.opt.traceClassBase() + "* tracep");
        funcp->isTrace(true);
        funcp->isStatic(خطأ);
        funcp->isLoose(true);
        funcp->slow(true);
        چوققا_Scopep->addBlocksp(funcp);
        أرجع funcp;
    }

    فراغ add_To_چوققا_Func(Ast_تۈگۈن_Stmt* stmtp) {
        إذا (m_چوققا_Func_Size > m_funcSizeLimit || m_چوققا_Funcps.empty()) {
            m_چوققا_Func_Size = 0;
            //
            ثابت string n = cvtToStr(m_چوققا_Funcps.size());
            ثابت string name{"trace_init_top__" + n};
            AstCFunc* ثابت funcp = newCFunc(m_چوققا_Scopep->fileline(), name);
            m_چوققا_Funcps.ئىتتىرىش_back(funcp);
        }
        m_چوققا_Funcps.back()->addStmtsp(stmtp);
        m_چوققا_Func_Size += stmtp->تۈگۈن_سانى();
    }

    فراغ addToSubFunc(Ast_تۈگۈن_Stmt* stmtp) {
        // TODO (maybe) -- sub funcs for dtype components
        إذا (m_dtypeSubFuncp) {
            إذا (m_subFuncSize > m_funcSizeLimit) newDeclSubFunc();
            m_dtypeSubFuncp->addStmtsp(stmtp);
            m_subFuncSize += stmtp->تۈگۈن_سانى();
            أرجع;
        }
        // Defer trace splitting until V3Trace
        إذا (m_subFuncps.empty() || m_forceNewLeaf) {
            m_forceNewLeaf = خطأ;
            File_قۇر* ثابت flp = m_currScopep->fileline();
            ثابت string n = cvtToStr(m_subFuncps.size());
            ثابت string name
                = m_currScopep == m_چوققا_Scopep->scopep() && !v3Global.opt.libCreate().empty()
                      ? (m_چوققا_Scope_يىلتىز_Phase ? "trace_init_leaf_root__" : "trace_init_leaf_top__")
                            + n
                      : "trace_init_sub__" + m_currScopep->nameDotless() + "__" + n;
            AstCFunc* ثابت funcp = newCFunc(flp, name);
            funcp->addStmtsp(جديد AstCStmt{flp, "const int c = vlSymsp->__Vm_baseCode;"});
            m_subFuncps.ئىتتىرىش_back(funcp);
        }
        m_subFuncps.back()->addStmtsp(stmtp);
        m_subFuncSize += stmtp->تۈگۈن_سانى();
    }

    AstTraceDecl* addTraceDecl(ثابت VNumRange& arrayRange,
                               صحيح widthOverride,  // If !=0, is packed struct/array where basicp
                                                   // size misreflects one element
                               AstCCall* ثابت dtypeCallp = nullptr) {
        VNumRange bitRange;
        إذا (widthOverride) {
            bitRange = VNumRange{widthOverride - 1, 0};
        } وإلا إذا (ثابت AstBasicDType* ثابت bdtypep = m_traValuep->dtypep()->basicp()) {
            bitRange = bdtypep->nrange();
        }
        File_قۇر* ثابت flp = m_traVscp->fileline();
        Ast_تۈگۈن_ئىپادە* valuep = m_traValuep->clone_دەرەخ(خطأ);
        ثابت منطقي validOffset = m_offset != std::numeric_limits<uint32_t>::max();
        AstTraceDecl* ثابت newp
            = جديد AstTraceDecl{flp,      m_traName,  m_traVscp->varp(), valuep,
                               bitRange, arrayRange, dtypeCallp,        validOffset};
        إذا (validOffset) {
            newp->code(m_offset);
            إذا (!dtypeCallp) { m_offset += newp->codeInc(); }
            valuep->foreach([&](AstVarRef* ثابت refp) {
                UASSERT_OBJ(refp->varScopep() == m_traVscp, refp,
                            "Trace decl expression references unexpected var");
                refp->replaceWith(جديد AstCExpr{flp, "__VdtypeVar", m_traVscp->width()});
                VL_DO_DANGLING(refp->delete_دەرەخ(), refp);
            });
        } وإلا {
            newp->dtypeDeclp(m_dtypeDeclp);
        }
        m_declUncalledps.emplace(newp);
        addToSubFunc(newp);

        أرجع newp;
    }

    فراغ addIgnore(ثابت string& why) {
        ++m_statIgnSigs;
        ثابت std::string cmt = "Tracing: "s + m_traName + " // Ignored: " + why;
        إذا (debug() > 3 && m_traVscp) std::cout << "- " << m_traVscp->fileline() << cmt << '\n';
    }

    فراغ fixupPlaceholder(ثابت std::string& path, Ast_تۈگۈن_Stmt* placeholderp) {
        // Find the scope for the path. As we are working based on cell names,
        // it is possible there is no corresponding scope (e.g.: for an empty
        // module).
        ثابت auto it = m_pathToScopep.find(Ast_تۈگۈن::prettyName(path));
        إذا (it != m_pathToScopep.end()) {
            ثابت AstScope* ثابت scopep = it->second;
            File_قۇر* ثابت flp = placeholderp->fileline();

            // Call the initialization functions for the scope
            Ast_تۈگۈن* stmtp = nullptr;
            لكل (AstCFunc* ثابت subFuncp : m_scopeInitFuncps.at(scopep)) {
                AstCCall* ثابت callp = جديد AstCCall{flp, subFuncp};
                callp->dtypeSetVoid();
                callp->argTypes("tracep");
                stmtp = Ast_تۈگۈن::addNext(stmtp, callp->makeStmt());
            }

            // Add after the placeholder
            إذا (stmtp) placeholderp->addNextHere(stmtp);
        }
        // Delete the placeholder
        placeholderp->unlinkFrBack();
        VL_DO_DANGLING(placeholderp->delete_دەرەخ(), placeholderp);
    }

    فراغ fixupLibStub(ثابت std::string& path, Ast_تۈگۈن_Stmt* placeholderp) {
        File_قۇر* ثابت flp = placeholderp->fileline();

        // The library instance's model is named after its %m within the parent model,
        // which does not include the "l2-name" (top module) of the library
        std::string name = Ast_تۈگۈن::prettyName(path);
        إذا (!v3Global.opt.libCreate().empty()) {
            ثابت std::string چوققا_Prefix = v3Global.rootp()->trace_Lib_چوققا_Name() + ".";
            إذا (VString::startsWith(name, چوققا_Prefix)) name = name.substr(چوققا_Prefix.size());
        }

        // Call the initialization function for the library instance
        AstCStmt* ثابت initp = جديد AstCStmt{flp};
        initp->add("{\n");
        initp->add("std::string __VlibName = vlSymsp->name();\n");
        initp->add("if (!__VlibName.empty()) __VlibName += '.';\n");
        initp->add("__VlibName += ");
        initp->add(جديد AstConst{flp, AstConst::String{}, name});
        initp->add(";\n");
        initp->add("tracep->initLib(__VlibName);\n");
        initp->add("}\n");

        placeholderp->addNextHere(initp);
        // Delete the placeholder
        VL_DO_DANGLING(placeholderp->unlinkFrBack()->delete_دەرەخ(), placeholderp);
    }

    فراغ fixupPlaceholders() {
        // Fix up cell initialization placehodlers
        UINFO(9, "fixupPlaceholders()");
        لكل (ثابت auto& item : m_cellInitPlaceholders) {
            ثابت AstScope* ثابت parentp = std::get<0>(item);
            ثابت AstCell* ثابت cellp = std::get<1>(item);
            Ast_تۈگۈن_Stmt* ثابت placeholderp = std::get<2>(item);
            ثابت std::string path = parentp->name() + "__DOT__" + cellp->name();
            إذا (cellp->modp()->verilatorLib()) {
                fixupLibStub(path, placeholderp);
            } وإلا {
                fixupPlaceholder(path, placeholderp);
            }
        }

        // Fix up interface reference initialization placeholders
        لكل (ثابت auto& item : m_ifaceRefInitPlaceholders) {
            ثابت AstVarScope* ثابت vscp = std::get<0>(item);
            Ast_تۈگۈن_Stmt* ثابت placeholderp = std::get<1>(item);
            ثابت std::string path = vscp->scopep()->name() + "__DOT__" + vscp->varp()->name();
            fixupPlaceholder(path, placeholderp);
        }
    }

    فراغ remove_Redundant_Prefix_ئىتتىرىش_چىقىرىش() {
        لكل (ثابت auto& pair : m_scopeInitFuncps) {
            // cppcheck-suppress constVariablePointer
            لكل (AstCFunc* ثابت funcp : pair.second) {
                Ast_تۈگۈن* prevp = nullptr;
                Ast_تۈگۈن* currp = funcp->stmtsp();
                طالما (currp) {
                    Ast_تۈگۈن* ثابت nextp = currp->nextp();
                    إذا (VN_IS(prevp, Trace_ئىتتىرىش_Prefix) && VN_IS(currp, Trace_چىقىرىش_Prefix)) {
                        VL_DO_DANGLING(prevp->unlinkFrBack()->delete_دەرەخ(), prevp);
                        VL_DO_DANGLING(currp->unlinkFrBack()->delete_دەرەخ(), currp);
                    }
                    إذا (!nextp) اكسر;
                    prevp = nextp->backp();
                    currp = nextp;
                }
            }
        }
    }

    فراغ checkCalls(ثابت AstCFunc* funcp) {
        إذا (!v3Global.opt.debugCheck()) أرجع;
        checkCallsRecurse(funcp);
        إذا (!m_declUncalledps.empty()) {  // LCOV_EXCL_START
            لكل (auto tracep : m_declUncalledps) UINFO(0, "-nodep " << tracep);
            (*(m_declUncalledps.begin()))->v3fatalSrc("Created TraceDecl which is never called");
        }  // LCOV_EXCL_STOP
    }
    فراغ checkCallsRecurse(ثابت AstCFunc* funcp) {
        funcp->foreach([هذا](ثابت Ast_تۈگۈن* nodep) {
            إذا (ثابت AstTraceDecl* ثابت declp = VN_CAST(nodep, TraceDecl)) {
                m_declUncalledps.erase(declp);
            } وإلا إذا (ثابت AstCCall* ثابت ccallp = VN_CAST(nodep, CCall)) {
                checkCallsRecurse(ccallp->funcp());
            }
        });
    }

    منطقي isBasicIO() { أرجع m_traVscp->varp()->isVLIO(); }
    فراغ newDeclSubFunc() {
        File_قۇر* ثابت flp = m_dtypeFuncp->fileline();
        AstCFunc* ثابت subFuncp = newCFunc(flp, m_dtypeNames.get("trace_init_dtype_sub__"));
        subFuncp->argTypes(subFuncp->argTypes() + m_dtypeArgs);
        AstCCall* ثابت subCallp = جديد AstCCall{flp, subFuncp};
        subCallp->dtypeSetVoid();
        subCallp->argTypes("tracep, name, fidx, c, direction");
        m_dtypeFuncp->addStmtsp(subCallp->makeStmt());
        m_dtypeSubFuncp = subFuncp;
        m_subFuncSize = 0;
    }
    فراغ newDeclFunc(Ast_تۈگۈن_DType* nodep) {
        Ast_تۈگۈن_DType* ثابت skipTypep = nodep->skipRefp();
        // offset and direction args added in EmitCImp
        std::string callArgs{"tracep, \"" + VIdProtect::protect(m_traName) + "\""};
        VL_RESTORER_COPY(m_traName);
        File_قۇر* ثابت flp = skipTypep->fileline();

        ثابت Dtype_Func_ئاچقۇچ dtype_ئاچقۇچ{skipTypep, m_traVscp->varp()->varType()};
        auto pair = m_dtypeFuncs.emplace(dtype_ئاچقۇچ, nullptr);
        AstCFunc** funcpp = &pair.first->second;
        إذا (pair.second) {
            *funcpp = newCFunc(flp, m_dtypeNames.get("trace_init_dtype__"));
            (*funcpp)->argTypes((*funcpp)->argTypes() + m_dtypeArgs);
            (*funcpp)->user2p(m_traVscp);
        }

        AstCCall* ثابت callp = جديد AstCCall{flp, *funcpp};
        callp->dtypeSetVoid();
        callp->argTypes(callArgs);
        m_dtypeDeclp = addTraceDecl(VNumRange{}, skipTypep->width(), callp);
        addToSubFunc(callp->makeStmt());

        إذا (pair.second) {
            VL_RESTORER(m_offset);
            m_offset = 0;

            VL_RESTORER(m_dtypeFuncp);
            VL_RESTORER(m_dtypeSubFuncp);
            VL_RESTORER(m_subFuncSize);
            m_dtypeFuncp = *funcpp;
            newDeclSubFunc();
            إذا (AstStructDType* ثابت dtypep = VN_CAST(skipTypep, StructDType)) {
                declStruct(dtypep, true);
            } وإلا إذا (AstUnpackArrayDType* ثابت dtypep = VN_CAST(skipTypep, UnpackArrayDType)) {
                declUnpackedArray(dtypep, true);
            } وإلا إذا (AstPackArrayDType* ثابت dtypep = VN_CAST(skipTypep, PackArrayDType)) {
                declPackedArray(dtypep, true);
            } وإلا {
                UASSERT_OBJ(خطأ, skipTypep, "Creating a trace function for an unexpected type");
            }
            m_dtypeFuncp->user1(m_offset);
        }

        m_dtypeDeclp->codeInc((*funcpp)->user1());
        m_offset += m_dtypeDeclp->codeInc();
    }
    فراغ declUnpackedArray(AstUnpackArrayDType* ثابت nodep, منطقي newFunc) {
        string prefixName(newFunc ? "name" : m_traName);

        VL_RESTORER_COPY(m_traName);
        File_قۇر* ثابت flp = nodep->fileline();

        addToSubFunc(جديد Ast_Trace_ئىتتىرىش_Prefix{flp, prefixName, VTracePrefixType::ARRAY_UNPACKED,
                                            nodep->سول(), nodep->ئوڭ(), !newFunc});

        إذا (VN_IS(nodep->subDTypep()->skipRefToEnump(),
                  BasicDType)  // Nothing lower than this array
            && m_traVscp->dtypep()->skipRefToEnump() == nodep) {  // Nothing above this array
            // Simple 1-D array, use existing V3EmitC runtime loop rather than unrolling
            // This will put "(index)" at end of signal name for us
            إذا (m_traVscp->dtypep()->skipRefToEnump()->isString()) {
                addIgnore("Unsupported: strings");
            } وإلا {
                m_traName = "";
                addTraceDecl(nodep->declRange(), 0);
            }
        } وإلا {
            Ast_تۈگۈن_DType* ثابت subtypep = nodep->subDTypep()->skipRefToEnump();
            // Always iterate left index to right index
            لكل (ثابت صحيح i : nodep->declRange().seq_سول_To_ئوڭ()) {
                VL_RESTORER(m_traValuep);
                m_traName = '[' + std::to_string(i) + ']';
                m_traValuep = m_traValuep->clone_دەرەخ(خطأ);
                m_traValuep = جديد AstArraySel{flp, m_traValuep, i - nodep->lo()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_دەرەخ(), m_traValuep);
            }
        }

        addToSubFunc(جديد Ast_Trace_چىقىرىش_Prefix{flp});
    }
    فراغ declPackedArray(AstPackArrayDType* ثابت nodep, منطقي newFunc) {
        string prefixName(newFunc ? "name" : m_traName);
        Ast_تۈگۈن_DType* ثابت subtypep = nodep->subDTypep()->skipRefToEnump();

        VL_RESTORER_COPY(m_traName);
        File_قۇر* ثابت flp = nodep->fileline();

        addToSubFunc(جديد Ast_Trace_ئىتتىرىش_Prefix{flp, prefixName, VTracePrefixType::ARRAY_PACKED,
                                            nodep->سول(), nodep->ئوڭ(), !newFunc});

        // Always iterate left index to right index
        لكل (ثابت صحيح i : nodep->declRange().seq_سول_To_ئوڭ()) {
            VL_RESTORER(m_traValuep);
            m_traName = '[' + std::to_string(i) + ']';
            ثابت صحيح lsb = (i - nodep->lo()) * subtypep->width();
            m_traValuep = m_traValuep->clone_دەرەخ(خطأ);
            m_traValuep = جديد AstSel{flp, m_traValuep, lsb, subtypep->width()};
            m_traValuep->dtypep(subtypep);
            iterate(subtypep);
            VL_DO_CLEAR(m_traValuep->delete_دەرەخ(), m_traValuep = nullptr);
        }

        addToSubFunc(جديد Ast_Trace_چىقىرىش_Prefix{flp});
    }
    فراغ declStruct(AstStructDType* ثابت nodep, منطقي newFunc) {
        File_قۇر* ثابت flp = nodep->fileline();
        string prefixName(newFunc ? "name" : m_traName);
        صحيح nMembers = 0;
        لكل (Ast_تۈگۈن* mp = nodep->membersp(); mp; mp = mp->nextp()) ++nMembers;
        إذا (!nodep->packed()) {
            addToSubFunc(جديد Ast_Trace_ئىتتىرىش_Prefix{flp, prefixName,  //
                                                VTracePrefixType::STRUCT_UNPACKED, nMembers, 0,
                                                !newFunc});
            لكل (ثابت AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_تۈگۈن_DType* ثابت subtypep = itemp->subDTypep()->skipRefToEnump();
                m_traName = itemp->prettyName();
                VL_RESTORER(m_traValuep);
                m_traValuep = m_traValuep->clone_دەرەخ(خطأ);
                m_traValuep = جديد AstStructSel{flp, m_traValuep, itemp->name()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_دەرەخ(), m_traValuep);
            }
            addToSubFunc(جديد Ast_Trace_چىقىرىش_Prefix{flp});
        } وإلا {
            addToSubFunc(جديد Ast_Trace_ئىتتىرىش_Prefix{flp, prefixName,  //
                                                VTracePrefixType::STRUCT_PACKED, nMembers, 0,
                                                !newFunc});
            لكل (ثابت AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_تۈگۈن_DType* ثابت subtypep = itemp->subDTypep()->skipRefToEnump();
                m_traName = itemp->prettyName();
                VL_RESTORER(m_traValuep);
                m_traValuep = m_traValuep->clone_دەرەخ(خطأ);
                m_traValuep = جديد AstSel{flp, m_traValuep, itemp->lsb(), subtypep->width()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_دەرەخ(), m_traValuep);
            }
            addToSubFunc(جديد Ast_Trace_چىقىرىش_Prefix{flp});
        }
    }

    // VISITORS
    فراغ visit(AstScope* nodep) override {
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(!m_currScopep, nodep, "Should not nest");
        UASSERT_OBJ(m_subFuncps.empty(), nodep, "Should not nest");
        UASSERT_OBJ(m_entries.empty(), nodep, "Should not nest");
        UASSERT_OBJ(!m_traVscp, nodep, "Should not nest");
        UASSERT_OBJ(!m_traValuep, nodep, "Should not nest");
        UASSERT_OBJ(m_traName.empty(), nodep, "Should not nest");

        // If this is a stub for a --lib-create library, skip.
        إذا (nodep->modp()->verilatorLib()) أرجع;

        VL_RESTORER(m_currScopep);
        m_currScopep = nodep;

        // Gather signals under this scope
        iterateChildrenConst(nodep);

        // Gather cells under this scope
        لكل (Ast_تۈگۈن* stmtp = nodep->modp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            إذا (AstCell* ثابت cellp = VN_CAST(stmtp, Cell)) m_entries.emplace_back(nodep, cellp);
        }

        إذا (!m_entries.empty()) {
            // Sort trace entries, by enclosing instance (necessary for single traversal of
            // hierarchy during initialization), then by source location, then by name.
            std::stable_sort(
                m_entries.begin(), m_entries.end(),
                [](ثابت TraceEntry& a, ثابت TraceEntry& b) { أرجع a.operatorCompare(b); });

            File_قۇر* ثابت flp = nodep->fileline();
            PathAdjustor pathAdjustor{flp, [&](Ast_تۈگۈن_Stmt* stmtp) { addToSubFunc(stmtp); }};
            ثابت منطقي split_يىلتىز_Primary_Ios = nodep->is_چوققا() && !v3Global.opt.libCreate().empty();
            ثابت auto emitEntry = [&](ثابت TraceEntry& entry) {
                AstVarScope* ثابت vscp = entry.vscp();
                // Adjust name prefix based on path in hierarchy
                UINFO(9, "path='" << entry.path() << "' name='" << entry.name() << "' "
                                  << (entry.cellp() ? static_cast<Ast_تۈگۈن*>(entry.cellp())
                                                    : static_cast<Ast_تۈگۈن*>(entry.vscp())));
                pathAdjustor.adjust(entry.path(), entry.cellp(), entry.vscp());

                m_traName = entry.name();

                إذا (vscp) {
                    // This is a signal: build AstTraceDecl for it
                    m_traVscp = vscp;
                    ثابت string& ignoreReason = vscIgnoreTrace(m_traVscp);
                    إذا (!ignoreReason.empty()) {
                        addIgnore(ignoreReason);
                    } وإلا {
                        ++m_statSigs;
                        // Create reference to whole signal. We will operate on this during the
                        // traversal.
                        m_traValuep
                            = جديد AstVarRef{m_traVscp->fileline(), m_traVscp, VAccess::READ};
                        // Recurse into data type of the signal. The visit methods will add
                        // AstTraceDecls.
                        iterate(m_traVscp->varp()->dtypep()->skipRefToEnump());
                        // Delete reference created above. Traversal cloned it as required.
                        إذا (m_traValuep) {
                            VL_DO_DANGLING(m_traValuep->delete_دەرەخ(), m_traValuep);
                            // Note: Sometimes VL_DANGLING is a no-op, but we have assertions
                            // on m_traValuep being nullptr, so make sure it is.
                            m_traValuep = nullptr;
                        }
                    }
                } وإلا {
                    // This is a subscope: insert a placeholder to be fixed up later
                    AstCell* ثابت cellp = entry.cellp();
                    Ast_تۈگۈن_Stmt* ثابت stmtp = جديد AstComment{
                        cellp->fileline(), "Instance init for: " + cellp->prettyName()};
                    addToSubFunc(stmtp);
                    m_cellInitPlaceholders.emplace_back(nodep, cellp, stmtp);
                }
            };
            إذا (split_يىلتىز_Primary_Ios) {
                m_چوققا_Scope_يىلتىز_Phase = true;
                لكل (ثابت TraceEntry& entry : m_entries) {
                    AstVarScope* ثابت vscp = entry.vscp();
                    إذا (!(vscp && vscp->varp()->isPrimaryIO())) استمر;
                    emitEntry(entry);
                }
                m_چوققا_Scope_يىلتىز_Phase = خطأ;
                pathAdjustor.unwind();
                m_چوققا_Scope_يىلتىز_Func_سانى = m_subFuncps.size();
                إذا (m_چوققا_Scope_يىلتىز_Func_سانى) {
                    // Force the next addToSubFunc to start a fresh leaf so
                    // root-phase decls and top-phase decls live in separate
                    // functions. Without this, both phases land in the same
                    // leaf and `trace_init_top` ends up emitting both the
                    // $rootio-prefixed IOs and the regular-scope decls, so
                    // each IO gets traced twice when the lib is consumed
                    // via initLib(name).
                    m_subFuncSize = 0;
                    m_forceNewLeaf = true;
                }
            }
            لكل (ثابت TraceEntry& entry : m_entries) {
                AstVarScope* ثابت vscp = entry.vscp();
                إذا (split_يىلتىز_Primary_Ios && vscp && vscp->varp()->isPrimaryIO()) استمر;
                emitEntry(entry);
            }
            pathAdjustor.unwind();
            m_traVscp = nullptr;
            m_traName.clear();
            UASSERT_OBJ(!m_traValuep, nodep, "Should have been deleted");
            m_entries.clear();
        }

        // Save the initialization functions of this scope
        m_scopeInitFuncps.emplace(nodep, std::move(m_subFuncps));

        // Save the hierarchical name of this scope
        ثابت std::string path = nodep->prettyName();
        m_pathToScopep.emplace(path, nodep);

        // Save the hierarchical names of interface references that reference this scope
        ثابت AstCell* ثابت cellp = nodep->aboveCellp();
        إذا (cellp
            && VN_IS(cellp->modp(), Iface)
            // Exclude classes that were inside interfaces, the cell's modp is an interface,
            // but the nodep->module is the Class
            && VN_IS(nodep->modp(), Iface)) {
            ثابت size_t lastDot = path.find_last_of('.');
            UASSERT_OBJ(lastDot != string::npos, nodep,
                        "Expected an interface scope name to have at least one dot: " << path);
            ثابت std::string parentPath = path.substr(0, lastDot + 1);

            لكل (AstIntfRef *intfRefp = cellp->intfRefsp(), *nextp; intfRefp; intfRefp = nextp) {
                nextp = VN_AS(intfRefp->nextp(), IntfRef);

                ثابت std::string refName = intfRefp->prettyName();

                // Assume only references under the same parent scope reference
                // the same interface.
                // TODO: This is not actually correct. An interface can propagate
                //       upwards and sideways when passed to a port via a downward
                //       hierarchical reference, which we will miss here.
                إذا (!VString::startsWith(refName, parentPath)) استمر;

                // Save the mapping from the path of the reference to the scope
                m_pathToScopep.emplace(refName, nodep);

                // No more need for AstIntfRef, unless V3EmitCSyms wants it for VPI
                إذا (!v3Global.opt.vpi()) {
                    VL_DO_DANGLING(intfRefp->unlinkFrBack()->delete_دەرەخ(), intfRefp);
                }
            }
        }
    }
    فراغ visit(AstVarScope* nodep) override {
        UASSERT_OBJ(m_currScopep, nodep, "AstVarScope not under AstScope");

        // Prefilter - things that get added to m_vscps will either get traced or get a comment as
        // to why they are not traced. Generally these conditions doesn't need updating, instead
        // use varp->isTrace() and/or vscIgnoreTrace.
        إذا (nodep->varp()->isTemp() && !nodep->varp()->isTrace()) أرجع;
        إذا (nodep->varp()->isClassMember()) أرجع;
        إذا (nodep->varp()->isFuncLocal()) أرجع;

        // When creating a --lib-create library ...
        إذا (!v3Global.opt.libCreate().empty()) {
            // Ignore parameters in packages. These will be traced at the top level.
            إذا (nodep->varp()->isParam() && VN_IS(nodep->scopep()->modp(), Package)) أرجع;
        }

        // Add to traced signal list
        m_entries.emplace_back(m_currScopep, nodep);
    }

    // VISITORS - Data types when tracing
    فراغ visit(AstConstDType* nodep) override {
        إذا (!m_traVscp) أرجع;
        VL_RESTORER(m_offset);
        VL_RESTORER(m_skipDtypeFunc);
        m_skipDtypeFunc = true;
        iterate(nodep->subDTypep()->skipRefToEnump());
    }
    فراغ visit(AstRefDType* nodep) override {
        إذا (!m_traVscp) أرجع;
        VL_RESTORER(m_offset);
        iterate(nodep->subDTypep()->skipRefToEnump());
    }
    فراغ visit(AstIfaceRefDType* /*nodep*/) override {
        إذا (!m_traVscp) أرجع;
        // Insert a placeholder to be fixed up later
        File_قۇر* ثابت flp = m_traVscp->fileline();
        Ast_تۈگۈن_Stmt* ثابت stmtp
            = جديد AstComment{flp, "Interface ref init for: " + m_traVscp->prettyName()};
        addToSubFunc(stmtp);
        m_ifaceRefInitPlaceholders.emplace_back(m_traVscp, stmtp);
    }
    فراغ visit(AstUnpackArrayDType* nodep) override {
        // Note more specific dtypes above
        إذا (!m_traVscp) أرجع;

        إذا (v3Global.opt.traceMaxArray()
            && static_cast<صحيح>(nodep->arrayUnpackedElements()) > v3Global.opt.traceMaxArray()) {
            addIgnore("Wide memory > --trace-max-array ents");
            أرجع;
        }

        // Skip unpacked arrays of non-traceable leaf types (e.g. strings)
        إذا (VN_IS(nodep->subDTypep()->skipRefToEnump(), BasicDType)
            && nodep->subDTypep()->skipRefToEnump()->isString()) {
            addIgnore("Unsupported: strings");
            أرجع;
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        إذا (isBasicIO()) m_skipDtypeFunc = true;

        إذا (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_offset);
            newDeclFunc(nodep);
        }
        declUnpackedArray(nodep, خطأ);
    }
    فراغ visit(AstPackArrayDType* nodep) override {
        إذا (!m_traVscp) أرجع;

        إذا (!v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit.
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            addTraceDecl(VNumRange{}, nodep->width());
            أرجع;
        }

        Ast_تۈگۈن_DType* ثابت subtypep = nodep->subDTypep()->skipRefToEnump();

        // Do not unroll if the elements are simple 'bit', or 'logic'
        إذا (AstBasicDType* ثابت basicp = VN_CAST(subtypep->skipRefp(), BasicDType)) {
            إذا (basicp->isBitLogic() && !basicp->isRanged()) {
                addTraceDecl(VNumRange{}, nodep->width());
                أرجع;
            }
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        إذا (isBasicIO()) m_skipDtypeFunc = true;

        إذا (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_offset);
            newDeclFunc(nodep);
        }
        declPackedArray(nodep, خطأ);
    }
    فراغ visit(AstStructDType* nodep) override {
        إذا (!m_traVscp) أرجع;

        إذا (nodep->packed() && !v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            addTraceDecl(VNumRange{}, nodep->width());
            أرجع;
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        إذا (isBasicIO()) m_skipDtypeFunc = true;

        // Only create sub functions for top-level structs, i.e. don't have struct funcs
        // call other struct funcs for child types.  This could easily be done for decl funcs
        // but full / chg funcs would require copying / aligning data for child types or more
        // complicated / wonky / generalized data access.
        إذا (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_offset);
            newDeclFunc(nodep);
        }
        declStruct(nodep, خطأ);
    }
    فراغ visit(AstUnionDType* nodep) override {
        إذا (!m_traVscp) أرجع;

        VL_RESTORER(m_skipDtypeFunc);
        m_skipDtypeFunc = true;

        إذا (nodep->packed() && !v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            addTraceDecl(VNumRange{}, nodep->width());
            أرجع;
        }

        VL_RESTORER_COPY(m_traName);
        File_قۇر* ثابت flp = nodep->fileline();

        صحيح nMembers = 0;
        لكل (Ast_تۈگۈن* mp = nodep->membersp(); mp; mp = mp->nextp()) ++nMembers;

        إذا (!nodep->packed()) {
            addIgnore("Unsupported: Unpacked union");
        } وإلا {
            addToSubFunc(جديد Ast_Trace_ئىتتىرىش_Prefix{flp, m_traName,  //
                                                VTracePrefixType::UNION_PACKED, nMembers});
            لكل (ثابت AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_تۈگۈن_DType* ثابت subtypep = itemp->subDTypep()->skipRefToEnump();
                m_traName = itemp->prettyName();
                iterate(subtypep);
            }
            addToSubFunc(جديد Ast_Trace_چىقىرىش_Prefix{flp});
        }
    }
    فراغ visit(AstBasicDType* nodep) override {
        إذا (!m_traVscp) أرجع;
        إذا (nodep->isString()) {
            addIgnore("Unsupported: strings");
        } وإلا {
            addTraceDecl(VNumRange{}, 0);
        }
    }
    فراغ visit(AstEnumDType* nodep) override { iterate(nodep->skipRefp()); }
    فراغ visit(Ast_تۈگۈن_DType*) override {
        // Note more specific dtypes above
        إذا (!m_traVscp) أرجع;
        addIgnore("Unsupported: data type");
    }

    //--------------------
    فراغ visit(Ast_تۈگۈن* nodep) override { iterateChildren(nodep); }

عام:
    // CONSTRUCTORS
    explicit TraceDeclVisitor(AstNetlist* nodep)
        : m_چوققا_Scopep{nodep->چوققا_Scopep()} {
        File_قۇر* ثابت flp = nodep->fileline();

        // Iterate modules to build per scope initialization functions
        iterateAndNextConstNull(nodep->modulesp());
        UASSERT_OBJ(m_subFuncps.empty(), nodep, "Should have been emptied");

        // Fix up the placeholders in the initialization functions
        fixupPlaceholders();

        // Now that we have everything ready, remove redundant pushPrefix/popPrefix
        // pairs. While functionally this is not really necessary (the trace files
        // might have some empty scope declarations), we do it to preserve previous
        // behavior. Note: unfortunately generating these without the redundant
        // push/pop pairs is a bit hard. It is cleaner to remove them.
        remove_Redundant_Prefix_ئىتتىرىش_چىقىرىش();

        ثابت std::vector<AstCFunc*>& چوققا_Scope_Funcps = m_scopeInitFuncps.at(m_چوققا_Scopep->scopep());
        AstCFunc* يىلتىز_Funcp = nullptr;
        إذا (!v3Global.opt.libCreate().empty()) {
            يىلتىز_Funcp = newCFunc(flp, "trace_init_root");
            يىلتىز_Funcp->entryPoint(true);
            لكل (size_t i = 0; i < m_چوققا_Scope_يىلتىز_Func_سانى; ++i) {
                AstCCall* ثابت callp = جديد AstCCall{flp, چوققا_Scope_Funcps.at(i)};
                callp->dtypeSetVoid();
                callp->argTypes("tracep");
                يىلتىز_Funcp->addStmtsp(callp->makeStmt());
            }
            إذا (!m_چوققا_Scope_يىلتىز_Func_سانى) يىلتىز_Funcp->addStmtsp(جديد AstComment{flp, "Empty"});
        }

        // Call the non-wrapper initialization functions of the root scope from the top function
        لكل (size_t i = m_چوققا_Scope_يىلتىز_Func_سانى; i < چوققا_Scope_Funcps.size(); ++i) {
            AstCFunc* ثابت funcp = چوققا_Scope_Funcps.at(i);
            AstCCall* ثابت callp = جديد AstCCall{flp, funcp};
            callp->dtypeSetVoid();
            callp->argTypes("tracep");
            add_To_چوققا_Func(callp->makeStmt());
        }

        // Ensure a top function exists, in case there was nothing to trace at all
        إذا (m_چوققا_Funcps.empty()) add_To_چوققا_Func(جديد AstComment{flp, "Empty"});

        // Create single top level function, if more than one exists
        إذا (m_چوققا_Funcps.size() > 1) {
            AstCFunc* ثابت چوققا_Funcp = newCFunc(flp, "");
            لكل (AstCFunc* funcp : m_چوققا_Funcps) {
                AstCCall* ثابت callp = جديد AstCCall{flp, funcp};
                callp->dtypeSetVoid();
                callp->argTypes("tracep");
                چوققا_Funcp->addStmtsp(callp->makeStmt());
            }
            m_چوققا_Funcps.clear();
            m_چوققا_Funcps.ئىتتىرىش_back(چوققا_Funcp);
        }

        // Set name of top level function
        AstCFunc* ثابت چوققا_Funcp = m_چوققا_Funcps.front();
        چوققا_Funcp->name("trace_init_top");
        چوققا_Funcp->entryPoint(true);

        إذا (يىلتىز_Funcp && v3Global.opt.debugCheck()) checkCallsRecurse(يىلتىز_Funcp);
        checkCalls(چوققا_Funcp);
    }
    ~TraceDeclVisitor() override {
        V3Stats::addStat("Tracing, Traced signals", m_statSigs);
        V3Stats::addStat("Tracing, Ignored signals", m_statIgnSigs);
    }
};

//######################################################################
// Trace class functions

فراغ V3TraceDecl::traceDeclAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TraceDeclVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_دەرەخ("tracedecl", 0, dump_دەرەخ_Either_Level() >= 3);
}
