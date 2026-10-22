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

#অন্তর্ভাৱ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#অন্তর্ভাৱ "V3TraceDecl.h"

#অন্তর্ভাৱ "V3Ast.h"
#অন্তর্ভাৱ "V3Control.h"
#অন্তর্ভাৱ "V3EmitCBase.h"
#অন্তর্ভাৱ "V3Error.h"
#অন্তর্ভাৱ "V3File.h"
#অন্তর্ভাৱ "V3Global.h"
#অন্তর্ভাৱ "V3Number.h"
#অন্তর্ভাৱ "V3Stats.h"
#অন্তর্ভাৱ "V3UniqueNames.h"

#অন্তর্ভাৱ <cstdint>
#অন্তর্ভাৱ <functional>
#অন্তর্ভাৱ <limits>
#অন্তর্ভাৱ <tuple>
#অন্তর্ভাৱ <unordered_map>
#অন্তর্ভাৱ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Utility class to emit path adjustments

শ্ৰেণী PathAdjustor final {
    File_সাৰি* ধ্রুৱ m_flp;  // FileLine used for created nodes
    std::function<শূন্য(Ast_গ্ৰন্থি_Stmt*)> m_emit;  // Function called with adjustment statements
    std::vector<std::string> m_স্তূপ{""};  // Stack of current paths

    স্থির constexpr ৱর্ণ SEPARATOR = ' ';

সার্ৱজনিক:
    explicit PathAdjustor(File_সাৰি* flp, std::function<শূন্য(Ast_গ্ৰন্থি_Stmt*)> emit)
        : m_flp{flp}
        , m_emit{emit} {}

    // Emit Prefix adjustments until the current path is 'newPath'
    শূন্য adjust(ধ্রুৱ string& newPath, AstCell* cellp, AstVarScope* vscp) {
        // Move up to enclosing path
        যেতিয়ালৈ (!VString::startsWith(newPath, m_স্তূপ.back())) {
            m_emit(নূতন Ast_Trace_বেৰ_কৰো_Prefix{m_flp});
            m_স্তূপ.বেৰ_কৰো_back();
        }

        যদি (newPath == m_স্তূপ.back()) ঘূৰাই;

        ধ্রুৱ VTracePrefixType lastScopeType =  //
            (cellp && VN_IS(cellp->modp(), Iface))
                    || (vscp && VN_IS(vscp->dtypep(), IfaceRefDType))
                ? VTracePrefixType::SCOPE_INTERFACE
                : VTracePrefixType::SCOPE_MODULE;
        ধ্রুৱ std::string extraPrefix = newPath.substr(m_স্তূপ.back().size());
        size_t begin = 0;
        ধ্রুৱ size_t last = extraPrefix.rfind(SEPARATOR);
        // Move down, one path element at a time
        যেতিয়ালৈ (সঁচা) {
            ধ্রুৱ size_t end = extraPrefix.find(SEPARATOR, begin);
            যদি (end == string::npos) ভঙ্গ;
            ধ্রুৱ string& extra = extraPrefix.substr(begin, end - begin);
            যদি (end == last) {
                m_emit(নূতন Ast_Trace_ঠেলো_Prefix{m_flp, extra, lastScopeType});
            } অন্যথা {
                m_emit(নূতন Ast_Trace_ঠেলো_Prefix{m_flp, extra, VTracePrefixType::SCOPE_MODULE});
            }
            m_স্তূপ.ঠেলো_back(m_স্তূপ.back() + extra + SEPARATOR);
            begin = end + 1;
        }
        UASSERT(begin == extraPrefix.size(), "Should have consumed all of extraPrefix");
    }

    // Emit Prefix adjustments to unwind the path back to its original state
    শূন্য unwind() {
        যেতিয়ালৈ (m_স্তূপ.size() > 1) {
            m_emit(নূতন Ast_Trace_বেৰ_কৰো_Prefix{m_flp});
            m_স্তূপ.বেৰ_কৰো_back();
        }
    }
};

//######################################################################
// TraceDecl state, as a visitor of each AstNode

শ্ৰেণী TraceDeclVisitor final : সার্ৱজনিক VNVisitor {
    // NODE STATE
    // AstCFunc::user1()                // uint32_t.  code offset for current type
    // AstCFunc::user2()                // VarScope* for dtype functions

    // STATE
    Ast_শীৰ্ষ_Scope* ধ্রুৱ m_শীৰ্ষ_Scopep;  // The singleton AstTopScope
    ধ্রুৱ AstScope* m_currScopep = nullptr;  // Current scope being visited

    std::vector<AstCFunc*> m_শীৰ্ষ_Funcps;  // Top level trace initialization functions
    std::vector<AstCFunc*> m_subFuncps;  // Trace sub functions for this scope
    std::set<ধ্রুৱ AstTraceDecl*> m_declUncalledps;  // Declarations not called
    // Functions per type and variable kind (wire vs logic etc.)
    সংরচনা Dtype_Func_চাবি final {
        ধ্রুৱ Ast_গ্ৰন্থি_DType* dtypep;
        VVarType varType;
        তার্কিক operator==(ধ্রুৱ Dtype_Func_চাবি& other) ধ্রুৱ {
            ঘূৰাই dtypep == other.dtypep && varType == other.varType;
        }
    };
    সংরচনা Dtype_Func_চাবি_Hash final {
        size_t operator()(ধ্রুৱ Dtype_Func_চাবি& চাবি) ধ্রুৱ {
            ঘূৰাই std::hash<ধ্রুৱ Ast_গ্ৰন্থি_DType*>{}(চাবি.dtypep)
                   ^ (std::hash<uint8_t>{}(চাবি.varType) << 1);
        }
    };
    std::unordered_map<Dtype_Func_চাবি, AstCFunc*, Dtype_Func_চাবি_Hash>
        m_dtypeFuncs;  // Functions per type+kind
    AstCFunc* m_dtypeFuncp = nullptr;  // Current type func
    AstCFunc* m_dtypeSubFuncp = nullptr;  // Current type sub func
    ধ্রুৱ string m_dtypeArgs{", const char* name, uint32_t fidx, uint32_t c, "
                             "VerilatedTraceSigDirection direction"};  // Type func args
    AstTraceDecl* m_dtypeDeclp = nullptr;  // Current type func decl
    V3UniqueNames m_dtypeNames{""};  // Unique names for dtype funcs
    তার্কিক m_skipDtypeFunc = মিছা;  // Don't create a type func
    uint32_t m_offset = std::numeric_limits<uint32_t>::max();  // Offset for types
    পূর্ণাঙ্ক m_শীৰ্ষ_Func_Size = 0;  // Size of the top function currently being built
    পূর্ণাঙ্ক m_subFuncSize = 0;  // Size of the sub function currently being built
    তার্কিক m_forceNewLeaf = মিছা;  // Force next addToSubFunc to start a fresh leaf
    size_t m_শীৰ্ষ_Scope_মূল_Func_গুনতি = 0;  // Top-scope init functions used only for wrapper IOs
    তার্কিক m_শীৰ্ষ_Scope_মূল_Phase = মিছা;  // Emitting top-scope wrapper IO declarations
    ধ্রুৱ পূর্ণাঙ্ক m_funcSizeLimit  // Maximum size of a function
        = v3Global.opt.outputSplitCTrace() ? v3Global.opt.outputSplitCTrace()
                                           : std::numeric_limits<পূর্ণাঙ্ক>::max();
    // Trace init functions to for each scope
    std::unordered_map<ধ্রুৱ AstScope*, std::vector<AstCFunc*>> m_scopeInitFuncps;
    // Map from hierarchical scope name to the corresponding AstScope. Note that
    // this is a many-to-one mapping for interfaces, due to interface refs.
    std::unordered_map<std::string, ধ্রুৱ AstScope*> m_pathToScopep;
    // Cell initialization placeholders:
    // (parent scope, cell under parent scope, statement)
    std::vector<std::tuple<AstScope*, AstCell*, Ast_গ্ৰন্থি_Stmt*>> m_cellInitPlaceholders;
    // Interface refs initialization placeholders:
    // (Interface ref variable, placeholder statement)
    std::vector<std::tuple<AstVarScope*, Ast_গ্ৰন্থি_Stmt*>> m_ifaceRefInitPlaceholders;

    // A trace entry under a scope is either:
    // - A variable (including interface references)
    // - A sub scope (stored as the cell corresponding to the sub scope)
    // Note: members are non-const to allow copy during sorting
    শ্ৰেণী TraceEntry final {
        AstVarScope* m_vscp = nullptr;  // AstVarScope under scope being traced
        AstCell* m_cellp = nullptr;  // Sub scope (as AstCell) under scope being traced
        std::string m_path;  // Path to enclosing module in original hierarchy
        std::string m_name;  // Name of signal/subscope

        শূন্য init(ধ্রুৱ std::string& name, Ast_গ্ৰন্থি* nodep, তার্কিক in_শীৰ্ষ_Scope) {
            // Compute path in hierarchy and item name
            ধ্রুৱ std::string& vcdName = Ast_গ্ৰন্থি::vcdName(name);
            AstVar* ধ্রুৱ varp = VN_CAST(nodep, Var);
            যদি (VN_IS(nodep, Cell) || VN_IS(varp->dtypep(), IfaceRefDType)) {
                // Cell or interface reference
                m_path = vcdName + " ";
                m_name.clear();
            } অন্যথা যদি (varp->isPrimaryIO()) {
                // Primary IO variable
                m_path = "$rootio ";
                m_name = vcdName;
            } অন্যথা {
                // Other Variable
                ধ্রুৱ size_t pos = vcdName.rfind(' ');
                ধ্রুৱ size_t pathLen = pos == std::string::npos ? 0 : pos + 1;
                m_path = vcdName.substr(0, pathLen);
                m_name = vcdName.substr(pathLen);
            }

            // When creating a --lib-create library, drop the name of the selected top module.
            // This will be replaced by the instance name in the model that uses the library, or
            // restored at runtime if the library itself is traced as the root model. Other top
            // level entities ($unit, packages, ...) keep a '$libroot' wrapper so they still have
            // a stable location in the dump.
            যদি (in_শীৰ্ষ_Scope && !v3Global.opt.libCreate().empty()) {
                ধ্রুৱ size_t start = m_path.find(' ');
                // Must have a prefix in the top scope with lib, as top wrapper signals not traced
                UASSERT_OBJ(start != std::string::npos, nodep, "No prefix with --lib-create");
                ধ্রুৱ std::string prefix = m_path.substr(0, start);
                // Wrapper primary IOs stay under $rootio so a root-traced library can restore
                // them under the runtime C++ instance name without affecting embedded use.
                যদি (prefix == "$rootio") ঘূৰাই;
                m_path = m_path.substr(start + 1);
                যদি (v3Global.rootp()->trace_Lib_শীৰ্ষ_Name() != prefix) m_path = "$libroot " + m_path;
            }
        }

    সার্ৱজনিক:
        explicit TraceEntry(ধ্রুৱ AstScope* scopep, AstVarScope* vscp)
            : m_vscp{vscp} {
            init(vscp->varp()->name(), vscp->varp(), scopep->is_শীৰ্ষ());
        }
        explicit TraceEntry(ধ্রুৱ AstScope* scopep, AstCell* cellp)
            : m_cellp{cellp} {
            init(cellp->name(), cellp, scopep->is_শীৰ্ষ());
        }
        পূর্ণাঙ্ক operatorCompare(ধ্রুৱ TraceEntry& b) ধ্রুৱ {
            যদি (ধ্রুৱ পূর্ণাঙ্ক cmp = path().compare(b.path())) ঘূৰাই cmp < 0;
            যদি (ধ্রুৱ পূর্ণাঙ্ক cmp = fileline().operatorCompare(b.fileline())) ঘূৰাই cmp < 0;
            ঘূৰাই name() < b.name();
        }
        AstVarScope* vscp() ধ্রুৱ { ঘূৰাই m_vscp; }
        AstCell* cellp() ধ্রুৱ { ঘূৰাই m_cellp; }
        ধ্রুৱ std::string& path() ধ্রুৱ { ঘূৰাই m_path; }
        শূন্য path(ধ্রুৱ std::string& path) { m_path = path; }
        ধ্রুৱ std::string& name() ধ্রুৱ { ঘূৰাই m_name; }
        File_সাৰি& fileline() ধ্রুৱ { ঘূৰাই m_vscp ? *m_vscp->fileline() : *m_cellp->fileline(); }
    };
    std::vector<TraceEntry> m_entries;  // Trace entries under current scope
    AstVarScope* m_traVscp = nullptr;  // Current AstVarScope we are constructing AstTraceDecls for
    Ast_গ্ৰন্থি_ৰাশিমালা* m_traValuep = nullptr;  // Value expression for current signal
    string m_traName;  // Name component for current signal

    VDouble0 m_statSigs;  // Statistic tracking
    VDouble0 m_statIgnSigs;  // Statistic tracking

    // METHODS

    string vscIgnoreTrace(ধ্রুৱ AstVarScope* nodep) {
        // Return true if this shouldn't be traced
        // See also similar rule in V3Coverage::varIgnoreToggle
        ধ্রুৱ AstVar* ধ্রুৱ varp = nodep->varp();
        যদি (!varp->isTrace()) ঘূৰাই "Verilator trace_off";
        যদি (!nodep->isTrace()) ঘূৰাই "Verilator instance trace_off";
        // Automatics (typically, excluding forks) have no persistance over
        // time, and may optimize differently when multithreadeded or hierarchical.
        // Class automatics refer to being in a class but might still be pointed
        // to by a static, so are ok.
        যদি (varp->lifetime().isAutomatic() && !varp->isClassMember() && !varp->isParam())
            ঘূৰাই "Automatic variable";

        ধ্রুৱ পূর্ণাঙ্ক width = recurseDTypeWidth(nodep->varp()->dtypep());
        যদি (v3Global.opt.traceMaxWidth() && width > v3Global.opt.traceMaxWidth())
            ঘূৰাই "Width " + cvtToStr(width) + " > --trace-max-width";

        ধ্রুৱ string prettyName = nodep->prettyName();
        যদি (!V3Control::getScopeTraceOn(prettyName)) ঘূৰাই "Vlt scope trace_off";
        যদি (!v3Global.opt.traceUnderscore()) {
            যদি (!prettyName.empty() && prettyName[0] == '_') ঘূৰাই "Leading underscore";
            যদি (prettyName.find("._") != string::npos) ঘূৰাই "Inlined leading underscore";
        }
        ঘূৰাই ""s;
    }

    পূর্ণাঙ্ক recurseDTypeWidth(ধ্রুৱ Ast_গ্ৰন্থি_DType* nodep) {
        যদি (ধ্রুৱ Ast_গ্ৰন্থি_Array_DType* adtypep = VN_CAST(nodep, গ্ৰন্থি_Array_DType))
            ঘূৰাই recurseDTypeWidth(adtypep->subDTypep()) * adtypep->declRange().elements();
        ঘূৰাই nodep->width();
    }

    AstCFunc* newCFunc(File_সাৰি* flp, ধ্রুৱ string& name) {
        AstScope* ধ্রুৱ শীৰ্ষ_Scopep = m_শীৰ্ষ_Scopep->scopep();
        AstCFunc* ধ্রুৱ funcp = নূতন AstCFunc{flp, name, শীৰ্ষ_Scopep};
        funcp->argTypes(v3Global.opt.traceClassBase() + "* tracep");
        funcp->isTrace(সঁচা);
        funcp->isStatic(মিছা);
        funcp->isLoose(সঁচা);
        funcp->slow(সঁচা);
        শীৰ্ষ_Scopep->addBlocksp(funcp);
        ঘূৰাই funcp;
    }

    শূন্য add_To_শীৰ্ষ_Func(Ast_গ্ৰন্থি_Stmt* stmtp) {
        যদি (m_শীৰ্ষ_Func_Size > m_funcSizeLimit || m_শীৰ্ষ_Funcps.empty()) {
            m_শীৰ্ষ_Func_Size = 0;
            //
            ধ্রুৱ string n = cvtToStr(m_শীৰ্ষ_Funcps.size());
            ধ্রুৱ string name{"trace_init_top__" + n};
            AstCFunc* ধ্রুৱ funcp = newCFunc(m_শীৰ্ষ_Scopep->fileline(), name);
            m_শীৰ্ষ_Funcps.ঠেলো_back(funcp);
        }
        m_শীৰ্ষ_Funcps.back()->addStmtsp(stmtp);
        m_শীৰ্ষ_Func_Size += stmtp->গ্ৰন্থি_গুনতি();
    }

    শূন্য addToSubFunc(Ast_গ্ৰন্থি_Stmt* stmtp) {
        // TODO (maybe) -- sub funcs for dtype components
        যদি (m_dtypeSubFuncp) {
            যদি (m_subFuncSize > m_funcSizeLimit) newDeclSubFunc();
            m_dtypeSubFuncp->addStmtsp(stmtp);
            m_subFuncSize += stmtp->গ্ৰন্থি_গুনতি();
            ঘূৰাই;
        }
        // Defer trace splitting until V3Trace
        যদি (m_subFuncps.empty() || m_forceNewLeaf) {
            m_forceNewLeaf = মিছা;
            File_সাৰি* ধ্রুৱ flp = m_currScopep->fileline();
            ধ্রুৱ string n = cvtToStr(m_subFuncps.size());
            ধ্রুৱ string name
                = m_currScopep == m_শীৰ্ষ_Scopep->scopep() && !v3Global.opt.libCreate().empty()
                      ? (m_শীৰ্ষ_Scope_মূল_Phase ? "trace_init_leaf_root__" : "trace_init_leaf_top__")
                            + n
                      : "trace_init_sub__" + m_currScopep->nameDotless() + "__" + n;
            AstCFunc* ধ্রুৱ funcp = newCFunc(flp, name);
            funcp->addStmtsp(নূতন AstCStmt{flp, "const int c = vlSymsp->__Vm_baseCode;"});
            m_subFuncps.ঠেলো_back(funcp);
        }
        m_subFuncps.back()->addStmtsp(stmtp);
        m_subFuncSize += stmtp->গ্ৰন্থি_গুনতি();
    }

    AstTraceDecl* addTraceDecl(ধ্রুৱ VNumRange& arrayRange,
                               পূর্ণাঙ্ক widthOverride,  // If !=0, is packed struct/array where basicp
                                                   // size misreflects one element
                               AstCCall* ধ্রুৱ dtypeCallp = nullptr) {
        VNumRange bitRange;
        যদি (widthOverride) {
            bitRange = VNumRange{widthOverride - 1, 0};
        } অন্যথা যদি (ধ্রুৱ AstBasicDType* ধ্রুৱ bdtypep = m_traValuep->dtypep()->basicp()) {
            bitRange = bdtypep->nrange();
        }
        File_সাৰি* ধ্রুৱ flp = m_traVscp->fileline();
        Ast_গ্ৰন্থি_ৰাশিমালা* valuep = m_traValuep->clone_বৃক্ষ(মিছা);
        ধ্রুৱ তার্কিক validOffset = m_offset != std::numeric_limits<uint32_t>::max();
        AstTraceDecl* ধ্রুৱ newp
            = নূতন AstTraceDecl{flp,      m_traName,  m_traVscp->varp(), valuep,
                               bitRange, arrayRange, dtypeCallp,        validOffset};
        যদি (validOffset) {
            newp->code(m_offset);
            যদি (!dtypeCallp) { m_offset += newp->codeInc(); }
            valuep->foreach([&](AstVarRef* ধ্রুৱ refp) {
                UASSERT_OBJ(refp->varScopep() == m_traVscp, refp,
                            "Trace decl expression references unexpected var");
                refp->replaceWith(নূতন AstCExpr{flp, "__VdtypeVar", m_traVscp->width()});
                VL_DO_DANGLING(refp->delete_বৃক্ষ(), refp);
            });
        } অন্যথা {
            newp->dtypeDeclp(m_dtypeDeclp);
        }
        m_declUncalledps.emplace(newp);
        addToSubFunc(newp);

        ঘূৰাই newp;
    }

    শূন্য addIgnore(ধ্রুৱ string& why) {
        ++m_statIgnSigs;
        ধ্রুৱ std::string cmt = "Tracing: "s + m_traName + " // Ignored: " + why;
        যদি (debug() > 3 && m_traVscp) std::cout << "- " << m_traVscp->fileline() << cmt << '\n';
    }

    শূন্য fixupPlaceholder(ধ্রুৱ std::string& path, Ast_গ্ৰন্থি_Stmt* placeholderp) {
        // Find the scope for the path. As we are working based on cell names,
        // it is possible there is no corresponding scope (e.g.: for an empty
        // module).
        ধ্রুৱ auto it = m_pathToScopep.find(Ast_গ্ৰন্থি::prettyName(path));
        যদি (it != m_pathToScopep.end()) {
            ধ্রুৱ AstScope* ধ্রুৱ scopep = it->second;
            File_সাৰি* ধ্রুৱ flp = placeholderp->fileline();

            // Call the initialization functions for the scope
            Ast_গ্ৰন্থি* stmtp = nullptr;
            বাবে (AstCFunc* ধ্রুৱ subFuncp : m_scopeInitFuncps.at(scopep)) {
                AstCCall* ধ্রুৱ callp = নূতন AstCCall{flp, subFuncp};
                callp->dtypeSetVoid();
                callp->argTypes("tracep");
                stmtp = Ast_গ্ৰন্থি::addNext(stmtp, callp->makeStmt());
            }

            // Add after the placeholder
            যদি (stmtp) placeholderp->addNextHere(stmtp);
        }
        // Delete the placeholder
        placeholderp->unlinkFrBack();
        VL_DO_DANGLING(placeholderp->delete_বৃক্ষ(), placeholderp);
    }

    শূন্য fixupLibStub(ধ্রুৱ std::string& path, Ast_গ্ৰন্থি_Stmt* placeholderp) {
        File_সাৰি* ধ্রুৱ flp = placeholderp->fileline();

        // The library instance's model is named after its %m within the parent model,
        // which does not include the "l2-name" (top module) of the library
        std::string name = Ast_গ্ৰন্থি::prettyName(path);
        যদি (!v3Global.opt.libCreate().empty()) {
            ধ্রুৱ std::string শীৰ্ষ_Prefix = v3Global.rootp()->trace_Lib_শীৰ্ষ_Name() + ".";
            যদি (VString::startsWith(name, শীৰ্ষ_Prefix)) name = name.substr(শীৰ্ষ_Prefix.size());
        }

        // Call the initialization function for the library instance
        AstCStmt* ধ্রুৱ initp = নূতন AstCStmt{flp};
        initp->add("{\n");
        initp->add("std::string __VlibName = vlSymsp->name();\n");
        initp->add("if (!__VlibName.empty()) __VlibName += '.';\n");
        initp->add("__VlibName += ");
        initp->add(নূতন AstConst{flp, AstConst::String{}, name});
        initp->add(";\n");
        initp->add("tracep->initLib(__VlibName);\n");
        initp->add("}\n");

        placeholderp->addNextHere(initp);
        // Delete the placeholder
        VL_DO_DANGLING(placeholderp->unlinkFrBack()->delete_বৃক্ষ(), placeholderp);
    }

    শূন্য fixupPlaceholders() {
        // Fix up cell initialization placehodlers
        UINFO(9, "fixupPlaceholders()");
        বাবে (ধ্রুৱ auto& item : m_cellInitPlaceholders) {
            ধ্রুৱ AstScope* ধ্রুৱ parentp = std::get<0>(item);
            ধ্রুৱ AstCell* ধ্রুৱ cellp = std::get<1>(item);
            Ast_গ্ৰন্থি_Stmt* ধ্রুৱ placeholderp = std::get<2>(item);
            ধ্রুৱ std::string path = parentp->name() + "__DOT__" + cellp->name();
            যদি (cellp->modp()->verilatorLib()) {
                fixupLibStub(path, placeholderp);
            } অন্যথা {
                fixupPlaceholder(path, placeholderp);
            }
        }

        // Fix up interface reference initialization placeholders
        বাবে (ধ্রুৱ auto& item : m_ifaceRefInitPlaceholders) {
            ধ্রুৱ AstVarScope* ধ্রুৱ vscp = std::get<0>(item);
            Ast_গ্ৰন্থি_Stmt* ধ্রুৱ placeholderp = std::get<1>(item);
            ধ্রুৱ std::string path = vscp->scopep()->name() + "__DOT__" + vscp->varp()->name();
            fixupPlaceholder(path, placeholderp);
        }
    }

    শূন্য remove_Redundant_Prefix_ঠেলো_বেৰ_কৰো() {
        বাবে (ধ্রুৱ auto& pair : m_scopeInitFuncps) {
            // cppcheck-suppress constVariablePointer
            বাবে (AstCFunc* ধ্রুৱ funcp : pair.second) {
                Ast_গ্ৰন্থি* prevp = nullptr;
                Ast_গ্ৰন্থি* currp = funcp->stmtsp();
                যেতিয়ালৈ (currp) {
                    Ast_গ্ৰন্থি* ধ্রুৱ nextp = currp->nextp();
                    যদি (VN_IS(prevp, Trace_ঠেলো_Prefix) && VN_IS(currp, Trace_বেৰ_কৰো_Prefix)) {
                        VL_DO_DANGLING(prevp->unlinkFrBack()->delete_বৃক্ষ(), prevp);
                        VL_DO_DANGLING(currp->unlinkFrBack()->delete_বৃক্ষ(), currp);
                    }
                    যদি (!nextp) ভঙ্গ;
                    prevp = nextp->backp();
                    currp = nextp;
                }
            }
        }
    }

    শূন্য checkCalls(ধ্রুৱ AstCFunc* funcp) {
        যদি (!v3Global.opt.debugCheck()) ঘূৰাই;
        checkCallsRecurse(funcp);
        যদি (!m_declUncalledps.empty()) {  // LCOV_EXCL_START
            বাবে (auto tracep : m_declUncalledps) UINFO(0, "-nodep " << tracep);
            (*(m_declUncalledps.begin()))->v3fatalSrc("Created TraceDecl which is never called");
        }  // LCOV_EXCL_STOP
    }
    শূন্য checkCallsRecurse(ধ্রুৱ AstCFunc* funcp) {
        funcp->foreach([এতত্](ধ্রুৱ Ast_গ্ৰন্থি* nodep) {
            যদি (ধ্রুৱ AstTraceDecl* ধ্রুৱ declp = VN_CAST(nodep, TraceDecl)) {
                m_declUncalledps.erase(declp);
            } অন্যথা যদি (ধ্রুৱ AstCCall* ধ্রুৱ ccallp = VN_CAST(nodep, CCall)) {
                checkCallsRecurse(ccallp->funcp());
            }
        });
    }

    তার্কিক isBasicIO() { ঘূৰাই m_traVscp->varp()->isVLIO(); }
    শূন্য newDeclSubFunc() {
        File_সাৰি* ধ্রুৱ flp = m_dtypeFuncp->fileline();
        AstCFunc* ধ্রুৱ subFuncp = newCFunc(flp, m_dtypeNames.get("trace_init_dtype_sub__"));
        subFuncp->argTypes(subFuncp->argTypes() + m_dtypeArgs);
        AstCCall* ধ্রুৱ subCallp = নূতন AstCCall{flp, subFuncp};
        subCallp->dtypeSetVoid();
        subCallp->argTypes("tracep, name, fidx, c, direction");
        m_dtypeFuncp->addStmtsp(subCallp->makeStmt());
        m_dtypeSubFuncp = subFuncp;
        m_subFuncSize = 0;
    }
    শূন্য newDeclFunc(Ast_গ্ৰন্থি_DType* nodep) {
        Ast_গ্ৰন্থি_DType* ধ্রুৱ skipTypep = nodep->skipRefp();
        // offset and direction args added in EmitCImp
        std::string callArgs{"tracep, \"" + VIdProtect::protect(m_traName) + "\""};
        VL_RESTORER_COPY(m_traName);
        File_সাৰি* ধ্রুৱ flp = skipTypep->fileline();

        ধ্রুৱ Dtype_Func_চাবি dtype_চাবি{skipTypep, m_traVscp->varp()->varType()};
        auto pair = m_dtypeFuncs.emplace(dtype_চাবি, nullptr);
        AstCFunc** funcpp = &pair.first->second;
        যদি (pair.second) {
            *funcpp = newCFunc(flp, m_dtypeNames.get("trace_init_dtype__"));
            (*funcpp)->argTypes((*funcpp)->argTypes() + m_dtypeArgs);
            (*funcpp)->user2p(m_traVscp);
        }

        AstCCall* ধ্রুৱ callp = নূতন AstCCall{flp, *funcpp};
        callp->dtypeSetVoid();
        callp->argTypes(callArgs);
        m_dtypeDeclp = addTraceDecl(VNumRange{}, skipTypep->width(), callp);
        addToSubFunc(callp->makeStmt());

        যদি (pair.second) {
            VL_RESTORER(m_offset);
            m_offset = 0;

            VL_RESTORER(m_dtypeFuncp);
            VL_RESTORER(m_dtypeSubFuncp);
            VL_RESTORER(m_subFuncSize);
            m_dtypeFuncp = *funcpp;
            newDeclSubFunc();
            যদি (AstStructDType* ধ্রুৱ dtypep = VN_CAST(skipTypep, StructDType)) {
                declStruct(dtypep, সঁচা);
            } অন্যথা যদি (AstUnpackArrayDType* ধ্রুৱ dtypep = VN_CAST(skipTypep, UnpackArrayDType)) {
                declUnpackedArray(dtypep, সঁচা);
            } অন্যথা যদি (AstPackArrayDType* ধ্রুৱ dtypep = VN_CAST(skipTypep, PackArrayDType)) {
                declPackedArray(dtypep, সঁচা);
            } অন্যথা {
                UASSERT_OBJ(মিছা, skipTypep, "Creating a trace function for an unexpected type");
            }
            m_dtypeFuncp->user1(m_offset);
        }

        m_dtypeDeclp->codeInc((*funcpp)->user1());
        m_offset += m_dtypeDeclp->codeInc();
    }
    শূন্য declUnpackedArray(AstUnpackArrayDType* ধ্রুৱ nodep, তার্কিক newFunc) {
        string prefixName(newFunc ? "name" : m_traName);

        VL_RESTORER_COPY(m_traName);
        File_সাৰি* ধ্রুৱ flp = nodep->fileline();

        addToSubFunc(নূতন Ast_Trace_ঠেলো_Prefix{flp, prefixName, VTracePrefixType::ARRAY_UNPACKED,
                                            nodep->বাম(), nodep->ডান(), !newFunc});

        যদি (VN_IS(nodep->subDTypep()->skipRefToEnump(),
                  BasicDType)  // Nothing lower than this array
            && m_traVscp->dtypep()->skipRefToEnump() == nodep) {  // Nothing above this array
            // Simple 1-D array, use existing V3EmitC runtime loop rather than unrolling
            // This will put "(index)" at end of signal name for us
            যদি (m_traVscp->dtypep()->skipRefToEnump()->isString()) {
                addIgnore("Unsupported: strings");
            } অন্যথা {
                m_traName = "";
                addTraceDecl(nodep->declRange(), 0);
            }
        } অন্যথা {
            Ast_গ্ৰন্থি_DType* ধ্রুৱ subtypep = nodep->subDTypep()->skipRefToEnump();
            // Always iterate left index to right index
            বাবে (ধ্রুৱ পূর্ণাঙ্ক i : nodep->declRange().seq_বাম_To_ডান()) {
                VL_RESTORER(m_traValuep);
                m_traName = '[' + std::to_string(i) + ']';
                m_traValuep = m_traValuep->clone_বৃক্ষ(মিছা);
                m_traValuep = নূতন AstArraySel{flp, m_traValuep, i - nodep->lo()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_বৃক্ষ(), m_traValuep);
            }
        }

        addToSubFunc(নূতন Ast_Trace_বেৰ_কৰো_Prefix{flp});
    }
    শূন্য declPackedArray(AstPackArrayDType* ধ্রুৱ nodep, তার্কিক newFunc) {
        string prefixName(newFunc ? "name" : m_traName);
        Ast_গ্ৰন্থি_DType* ধ্রুৱ subtypep = nodep->subDTypep()->skipRefToEnump();

        VL_RESTORER_COPY(m_traName);
        File_সাৰি* ধ্রুৱ flp = nodep->fileline();

        addToSubFunc(নূতন Ast_Trace_ঠেলো_Prefix{flp, prefixName, VTracePrefixType::ARRAY_PACKED,
                                            nodep->বাম(), nodep->ডান(), !newFunc});

        // Always iterate left index to right index
        বাবে (ধ্রুৱ পূর্ণাঙ্ক i : nodep->declRange().seq_বাম_To_ডান()) {
            VL_RESTORER(m_traValuep);
            m_traName = '[' + std::to_string(i) + ']';
            ধ্রুৱ পূর্ণাঙ্ক lsb = (i - nodep->lo()) * subtypep->width();
            m_traValuep = m_traValuep->clone_বৃক্ষ(মিছা);
            m_traValuep = নূতন AstSel{flp, m_traValuep, lsb, subtypep->width()};
            m_traValuep->dtypep(subtypep);
            iterate(subtypep);
            VL_DO_CLEAR(m_traValuep->delete_বৃক্ষ(), m_traValuep = nullptr);
        }

        addToSubFunc(নূতন Ast_Trace_বেৰ_কৰো_Prefix{flp});
    }
    শূন্য declStruct(AstStructDType* ধ্রুৱ nodep, তার্কিক newFunc) {
        File_সাৰি* ধ্রুৱ flp = nodep->fileline();
        string prefixName(newFunc ? "name" : m_traName);
        পূর্ণাঙ্ক nMembers = 0;
        বাবে (Ast_গ্ৰন্থি* mp = nodep->membersp(); mp; mp = mp->nextp()) ++nMembers;
        যদি (!nodep->packed()) {
            addToSubFunc(নূতন Ast_Trace_ঠেলো_Prefix{flp, prefixName,  //
                                                VTracePrefixType::STRUCT_UNPACKED, nMembers, 0,
                                                !newFunc});
            বাবে (ধ্রুৱ AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_গ্ৰন্থি_DType* ধ্রুৱ subtypep = itemp->subDTypep()->skipRefToEnump();
                m_traName = itemp->prettyName();
                VL_RESTORER(m_traValuep);
                m_traValuep = m_traValuep->clone_বৃক্ষ(মিছা);
                m_traValuep = নূতন AstStructSel{flp, m_traValuep, itemp->name()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_বৃক্ষ(), m_traValuep);
            }
            addToSubFunc(নূতন Ast_Trace_বেৰ_কৰো_Prefix{flp});
        } অন্যথা {
            addToSubFunc(নূতন Ast_Trace_ঠেলো_Prefix{flp, prefixName,  //
                                                VTracePrefixType::STRUCT_PACKED, nMembers, 0,
                                                !newFunc});
            বাবে (ধ্রুৱ AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_গ্ৰন্থি_DType* ধ্রুৱ subtypep = itemp->subDTypep()->skipRefToEnump();
                m_traName = itemp->prettyName();
                VL_RESTORER(m_traValuep);
                m_traValuep = m_traValuep->clone_বৃক্ষ(মিছা);
                m_traValuep = নূতন AstSel{flp, m_traValuep, itemp->lsb(), subtypep->width()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_বৃক্ষ(), m_traValuep);
            }
            addToSubFunc(নূতন Ast_Trace_বেৰ_কৰো_Prefix{flp});
        }
    }

    // VISITORS
    শূন্য visit(AstScope* nodep) override {
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(!m_currScopep, nodep, "Should not nest");
        UASSERT_OBJ(m_subFuncps.empty(), nodep, "Should not nest");
        UASSERT_OBJ(m_entries.empty(), nodep, "Should not nest");
        UASSERT_OBJ(!m_traVscp, nodep, "Should not nest");
        UASSERT_OBJ(!m_traValuep, nodep, "Should not nest");
        UASSERT_OBJ(m_traName.empty(), nodep, "Should not nest");

        // If this is a stub for a --lib-create library, skip.
        যদি (nodep->modp()->verilatorLib()) ঘূৰাই;

        VL_RESTORER(m_currScopep);
        m_currScopep = nodep;

        // Gather signals under this scope
        iterateChildrenConst(nodep);

        // Gather cells under this scope
        বাবে (Ast_গ্ৰন্থি* stmtp = nodep->modp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            যদি (AstCell* ধ্রুৱ cellp = VN_CAST(stmtp, Cell)) m_entries.emplace_back(nodep, cellp);
        }

        যদি (!m_entries.empty()) {
            // Sort trace entries, by enclosing instance (necessary for single traversal of
            // hierarchy during initialization), then by source location, then by name.
            std::stable_sort(
                m_entries.begin(), m_entries.end(),
                [](ধ্রুৱ TraceEntry& a, ধ্রুৱ TraceEntry& b) { ঘূৰাই a.operatorCompare(b); });

            File_সাৰি* ধ্রুৱ flp = nodep->fileline();
            PathAdjustor pathAdjustor{flp, [&](Ast_গ্ৰন্থি_Stmt* stmtp) { addToSubFunc(stmtp); }};
            ধ্রুৱ তার্কিক split_মূল_Primary_Ios = nodep->is_শীৰ্ষ() && !v3Global.opt.libCreate().empty();
            ধ্রুৱ auto emitEntry = [&](ধ্রুৱ TraceEntry& entry) {
                AstVarScope* ধ্রুৱ vscp = entry.vscp();
                // Adjust name prefix based on path in hierarchy
                UINFO(9, "path='" << entry.path() << "' name='" << entry.name() << "' "
                                  << (entry.cellp() ? static_cast<Ast_গ্ৰন্থি*>(entry.cellp())
                                                    : static_cast<Ast_গ্ৰন্থি*>(entry.vscp())));
                pathAdjustor.adjust(entry.path(), entry.cellp(), entry.vscp());

                m_traName = entry.name();

                যদি (vscp) {
                    // This is a signal: build AstTraceDecl for it
                    m_traVscp = vscp;
                    ধ্রুৱ string& ignoreReason = vscIgnoreTrace(m_traVscp);
                    যদি (!ignoreReason.empty()) {
                        addIgnore(ignoreReason);
                    } অন্যথা {
                        ++m_statSigs;
                        // Create reference to whole signal. We will operate on this during the
                        // traversal.
                        m_traValuep
                            = নূতন AstVarRef{m_traVscp->fileline(), m_traVscp, VAccess::READ};
                        // Recurse into data type of the signal. The visit methods will add
                        // AstTraceDecls.
                        iterate(m_traVscp->varp()->dtypep()->skipRefToEnump());
                        // Delete reference created above. Traversal cloned it as required.
                        যদি (m_traValuep) {
                            VL_DO_DANGLING(m_traValuep->delete_বৃক্ষ(), m_traValuep);
                            // Note: Sometimes VL_DANGLING is a no-op, but we have assertions
                            // on m_traValuep being nullptr, so make sure it is.
                            m_traValuep = nullptr;
                        }
                    }
                } অন্যথা {
                    // This is a subscope: insert a placeholder to be fixed up later
                    AstCell* ধ্রুৱ cellp = entry.cellp();
                    Ast_গ্ৰন্থি_Stmt* ধ্রুৱ stmtp = নূতন AstComment{
                        cellp->fileline(), "Instance init for: " + cellp->prettyName()};
                    addToSubFunc(stmtp);
                    m_cellInitPlaceholders.emplace_back(nodep, cellp, stmtp);
                }
            };
            যদি (split_মূল_Primary_Ios) {
                m_শীৰ্ষ_Scope_মূল_Phase = সঁচা;
                বাবে (ধ্রুৱ TraceEntry& entry : m_entries) {
                    AstVarScope* ধ্রুৱ vscp = entry.vscp();
                    যদি (!(vscp && vscp->varp()->isPrimaryIO())) অনুৱর্ত;
                    emitEntry(entry);
                }
                m_শীৰ্ষ_Scope_মূল_Phase = মিছা;
                pathAdjustor.unwind();
                m_শীৰ্ষ_Scope_মূল_Func_গুনতি = m_subFuncps.size();
                যদি (m_শীৰ্ষ_Scope_মূল_Func_গুনতি) {
                    // Force the next addToSubFunc to start a fresh leaf so
                    // root-phase decls and top-phase decls live in separate
                    // functions. Without this, both phases land in the same
                    // leaf and `trace_init_top` ends up emitting both the
                    // $rootio-prefixed IOs and the regular-scope decls, so
                    // each IO gets traced twice when the lib is consumed
                    // via initLib(name).
                    m_subFuncSize = 0;
                    m_forceNewLeaf = সঁচা;
                }
            }
            বাবে (ধ্রুৱ TraceEntry& entry : m_entries) {
                AstVarScope* ধ্রুৱ vscp = entry.vscp();
                যদি (split_মূল_Primary_Ios && vscp && vscp->varp()->isPrimaryIO()) অনুৱর্ত;
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
        ধ্রুৱ std::string path = nodep->prettyName();
        m_pathToScopep.emplace(path, nodep);

        // Save the hierarchical names of interface references that reference this scope
        ধ্রুৱ AstCell* ধ্রুৱ cellp = nodep->aboveCellp();
        যদি (cellp
            && VN_IS(cellp->modp(), Iface)
            // Exclude classes that were inside interfaces, the cell's modp is an interface,
            // but the nodep->module is the Class
            && VN_IS(nodep->modp(), Iface)) {
            ধ্রুৱ size_t lastDot = path.find_last_of('.');
            UASSERT_OBJ(lastDot != string::npos, nodep,
                        "Expected an interface scope name to have at least one dot: " << path);
            ধ্রুৱ std::string parentPath = path.substr(0, lastDot + 1);

            বাবে (AstIntfRef *intfRefp = cellp->intfRefsp(), *nextp; intfRefp; intfRefp = nextp) {
                nextp = VN_AS(intfRefp->nextp(), IntfRef);

                ধ্রুৱ std::string refName = intfRefp->prettyName();

                // Assume only references under the same parent scope reference
                // the same interface.
                // TODO: This is not actually correct. An interface can propagate
                //       upwards and sideways when passed to a port via a downward
                //       hierarchical reference, which we will miss here.
                যদি (!VString::startsWith(refName, parentPath)) অনুৱর্ত;

                // Save the mapping from the path of the reference to the scope
                m_pathToScopep.emplace(refName, nodep);

                // No more need for AstIntfRef, unless V3EmitCSyms wants it for VPI
                যদি (!v3Global.opt.vpi()) {
                    VL_DO_DANGLING(intfRefp->unlinkFrBack()->delete_বৃক্ষ(), intfRefp);
                }
            }
        }
    }
    শূন্য visit(AstVarScope* nodep) override {
        UASSERT_OBJ(m_currScopep, nodep, "AstVarScope not under AstScope");

        // Prefilter - things that get added to m_vscps will either get traced or get a comment as
        // to why they are not traced. Generally these conditions doesn't need updating, instead
        // use varp->isTrace() and/or vscIgnoreTrace.
        যদি (nodep->varp()->isTemp() && !nodep->varp()->isTrace()) ঘূৰাই;
        যদি (nodep->varp()->isClassMember()) ঘূৰাই;
        যদি (nodep->varp()->isFuncLocal()) ঘূৰাই;

        // When creating a --lib-create library ...
        যদি (!v3Global.opt.libCreate().empty()) {
            // Ignore parameters in packages. These will be traced at the top level.
            যদি (nodep->varp()->isParam() && VN_IS(nodep->scopep()->modp(), Package)) ঘূৰাই;
        }

        // Add to traced signal list
        m_entries.emplace_back(m_currScopep, nodep);
    }

    // VISITORS - Data types when tracing
    শূন্য visit(AstConstDType* nodep) override {
        যদি (!m_traVscp) ঘূৰাই;
        VL_RESTORER(m_offset);
        VL_RESTORER(m_skipDtypeFunc);
        m_skipDtypeFunc = সঁচা;
        iterate(nodep->subDTypep()->skipRefToEnump());
    }
    শূন্য visit(AstRefDType* nodep) override {
        যদি (!m_traVscp) ঘূৰাই;
        VL_RESTORER(m_offset);
        iterate(nodep->subDTypep()->skipRefToEnump());
    }
    শূন্য visit(AstIfaceRefDType* /*nodep*/) override {
        যদি (!m_traVscp) ঘূৰাই;
        // Insert a placeholder to be fixed up later
        File_সাৰি* ধ্রুৱ flp = m_traVscp->fileline();
        Ast_গ্ৰন্থি_Stmt* ধ্রুৱ stmtp
            = নূতন AstComment{flp, "Interface ref init for: " + m_traVscp->prettyName()};
        addToSubFunc(stmtp);
        m_ifaceRefInitPlaceholders.emplace_back(m_traVscp, stmtp);
    }
    শূন্য visit(AstUnpackArrayDType* nodep) override {
        // Note more specific dtypes above
        যদি (!m_traVscp) ঘূৰাই;

        যদি (v3Global.opt.traceMaxArray()
            && static_cast<পূর্ণাঙ্ক>(nodep->arrayUnpackedElements()) > v3Global.opt.traceMaxArray()) {
            addIgnore("Wide memory > --trace-max-array ents");
            ঘূৰাই;
        }

        // Skip unpacked arrays of non-traceable leaf types (e.g. strings)
        যদি (VN_IS(nodep->subDTypep()->skipRefToEnump(), BasicDType)
            && nodep->subDTypep()->skipRefToEnump()->isString()) {
            addIgnore("Unsupported: strings");
            ঘূৰাই;
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        যদি (isBasicIO()) m_skipDtypeFunc = সঁচা;

        যদি (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_offset);
            newDeclFunc(nodep);
        }
        declUnpackedArray(nodep, মিছা);
    }
    শূন্য visit(AstPackArrayDType* nodep) override {
        যদি (!m_traVscp) ঘূৰাই;

        যদি (!v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit.
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            addTraceDecl(VNumRange{}, nodep->width());
            ঘূৰাই;
        }

        Ast_গ্ৰন্থি_DType* ধ্রুৱ subtypep = nodep->subDTypep()->skipRefToEnump();

        // Do not unroll if the elements are simple 'bit', or 'logic'
        যদি (AstBasicDType* ধ্রুৱ basicp = VN_CAST(subtypep->skipRefp(), BasicDType)) {
            যদি (basicp->isBitLogic() && !basicp->isRanged()) {
                addTraceDecl(VNumRange{}, nodep->width());
                ঘূৰাই;
            }
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        যদি (isBasicIO()) m_skipDtypeFunc = সঁচা;

        যদি (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_offset);
            newDeclFunc(nodep);
        }
        declPackedArray(nodep, মিছা);
    }
    শূন্য visit(AstStructDType* nodep) override {
        যদি (!m_traVscp) ঘূৰাই;

        যদি (nodep->packed() && !v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            addTraceDecl(VNumRange{}, nodep->width());
            ঘূৰাই;
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        যদি (isBasicIO()) m_skipDtypeFunc = সঁচা;

        // Only create sub functions for top-level structs, i.e. don't have struct funcs
        // call other struct funcs for child types.  This could easily be done for decl funcs
        // but full / chg funcs would require copying / aligning data for child types or more
        // complicated / wonky / generalized data access.
        যদি (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_offset);
            newDeclFunc(nodep);
        }
        declStruct(nodep, মিছা);
    }
    শূন্য visit(AstUnionDType* nodep) override {
        যদি (!m_traVscp) ঘূৰাই;

        VL_RESTORER(m_skipDtypeFunc);
        m_skipDtypeFunc = সঁচা;

        যদি (nodep->packed() && !v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            addTraceDecl(VNumRange{}, nodep->width());
            ঘূৰাই;
        }

        VL_RESTORER_COPY(m_traName);
        File_সাৰি* ধ্রুৱ flp = nodep->fileline();

        পূর্ণাঙ্ক nMembers = 0;
        বাবে (Ast_গ্ৰন্থি* mp = nodep->membersp(); mp; mp = mp->nextp()) ++nMembers;

        যদি (!nodep->packed()) {
            addIgnore("Unsupported: Unpacked union");
        } অন্যথা {
            addToSubFunc(নূতন Ast_Trace_ঠেলো_Prefix{flp, m_traName,  //
                                                VTracePrefixType::UNION_PACKED, nMembers});
            বাবে (ধ্রুৱ AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_গ্ৰন্থি_DType* ধ্রুৱ subtypep = itemp->subDTypep()->skipRefToEnump();
                m_traName = itemp->prettyName();
                iterate(subtypep);
            }
            addToSubFunc(নূতন Ast_Trace_বেৰ_কৰো_Prefix{flp});
        }
    }
    শূন্য visit(AstBasicDType* nodep) override {
        যদি (!m_traVscp) ঘূৰাই;
        যদি (nodep->isString()) {
            addIgnore("Unsupported: strings");
        } অন্যথা {
            addTraceDecl(VNumRange{}, 0);
        }
    }
    শূন্য visit(AstEnumDType* nodep) override { iterate(nodep->skipRefp()); }
    শূন্য visit(Ast_গ্ৰন্থি_DType*) override {
        // Note more specific dtypes above
        যদি (!m_traVscp) ঘূৰাই;
        addIgnore("Unsupported: data type");
    }

    //--------------------
    শূন্য visit(Ast_গ্ৰন্থি* nodep) override { iterateChildren(nodep); }

সার্ৱজনিক:
    // CONSTRUCTORS
    explicit TraceDeclVisitor(AstNetlist* nodep)
        : m_শীৰ্ষ_Scopep{nodep->শীৰ্ষ_Scopep()} {
        File_সাৰি* ধ্রুৱ flp = nodep->fileline();

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
        remove_Redundant_Prefix_ঠেলো_বেৰ_কৰো();

        ধ্রুৱ std::vector<AstCFunc*>& শীৰ্ষ_Scope_Funcps = m_scopeInitFuncps.at(m_শীৰ্ষ_Scopep->scopep());
        AstCFunc* মূল_Funcp = nullptr;
        যদি (!v3Global.opt.libCreate().empty()) {
            মূল_Funcp = newCFunc(flp, "trace_init_root");
            মূল_Funcp->entryPoint(সঁচা);
            বাবে (size_t i = 0; i < m_শীৰ্ষ_Scope_মূল_Func_গুনতি; ++i) {
                AstCCall* ধ্রুৱ callp = নূতন AstCCall{flp, শীৰ্ষ_Scope_Funcps.at(i)};
                callp->dtypeSetVoid();
                callp->argTypes("tracep");
                মূল_Funcp->addStmtsp(callp->makeStmt());
            }
            যদি (!m_শীৰ্ষ_Scope_মূল_Func_গুনতি) মূল_Funcp->addStmtsp(নূতন AstComment{flp, "Empty"});
        }

        // Call the non-wrapper initialization functions of the root scope from the top function
        বাবে (size_t i = m_শীৰ্ষ_Scope_মূল_Func_গুনতি; i < শীৰ্ষ_Scope_Funcps.size(); ++i) {
            AstCFunc* ধ্রুৱ funcp = শীৰ্ষ_Scope_Funcps.at(i);
            AstCCall* ধ্রুৱ callp = নূতন AstCCall{flp, funcp};
            callp->dtypeSetVoid();
            callp->argTypes("tracep");
            add_To_শীৰ্ষ_Func(callp->makeStmt());
        }

        // Ensure a top function exists, in case there was nothing to trace at all
        যদি (m_শীৰ্ষ_Funcps.empty()) add_To_শীৰ্ষ_Func(নূতন AstComment{flp, "Empty"});

        // Create single top level function, if more than one exists
        যদি (m_শীৰ্ষ_Funcps.size() > 1) {
            AstCFunc* ধ্রুৱ শীৰ্ষ_Funcp = newCFunc(flp, "");
            বাবে (AstCFunc* funcp : m_শীৰ্ষ_Funcps) {
                AstCCall* ধ্রুৱ callp = নূতন AstCCall{flp, funcp};
                callp->dtypeSetVoid();
                callp->argTypes("tracep");
                শীৰ্ষ_Funcp->addStmtsp(callp->makeStmt());
            }
            m_শীৰ্ষ_Funcps.clear();
            m_শীৰ্ষ_Funcps.ঠেলো_back(শীৰ্ষ_Funcp);
        }

        // Set name of top level function
        AstCFunc* ধ্রুৱ শীৰ্ষ_Funcp = m_শীৰ্ষ_Funcps.front();
        শীৰ্ষ_Funcp->name("trace_init_top");
        শীৰ্ষ_Funcp->entryPoint(সঁচা);

        যদি (মূল_Funcp && v3Global.opt.debugCheck()) checkCallsRecurse(মূল_Funcp);
        checkCalls(শীৰ্ষ_Funcp);
    }
    ~TraceDeclVisitor() override {
        V3Stats::addStat("Tracing, Traced signals", m_statSigs);
        V3Stats::addStat("Tracing, Ignored signals", m_statIgnSigs);
    }
};

//######################################################################
// Trace class functions

শূন্য V3TraceDecl::traceDeclAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TraceDeclVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_বৃক্ষ("tracedecl", 0, dump_বৃক্ষ_Either_Level() >= 3);
}
