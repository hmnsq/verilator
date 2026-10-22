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

#ଅନ୍ତର୍ଭାଵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ଅନ୍ତର୍ଭାଵ "V3TraceDecl.h"

#ଅନ୍ତର୍ଭାଵ "V3Ast.h"
#ଅନ୍ତର୍ଭାଵ "V3Control.h"
#ଅନ୍ତର୍ଭାଵ "V3EmitCBase.h"
#ଅନ୍ତର୍ଭାଵ "V3Error.h"
#ଅନ୍ତର୍ଭାଵ "V3File.h"
#ଅନ୍ତର୍ଭାଵ "V3Global.h"
#ଅନ୍ତର୍ଭାଵ "V3Number.h"
#ଅନ୍ତର୍ଭାଵ "V3Stats.h"
#ଅନ୍ତର୍ଭାଵ "V3UniqueNames.h"

#ଅନ୍ତର୍ଭାଵ <cstdint>
#ଅନ୍ତର୍ଭାଵ <functional>
#ଅନ୍ତର୍ଭାଵ <limits>
#ଅନ୍ତର୍ଭାଵ <tuple>
#ଅନ୍ତର୍ଭାଵ <unordered_map>
#ଅନ୍ତର୍ଭାଵ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Utility class to emit path adjustments

ଶ୍ରେଣୀ PathAdjustor final {
    File_ଧାଡ଼ି* ଅଚଳ m_flp;  // FileLine used for created nodes
    std::function<ଶୂନ୍ୟ(Ast_ଗଣ୍ଠି_Stmt*)> m_emit;  // Function called with adjustment statements
    std::vector<std::string> m_ଗଦା{""};  // Stack of current paths

    ସ୍ଥିର constexpr ଅକ୍ଷର SEPARATOR = ' ';

ସର୍ବସାଧାରଣ:
    explicit PathAdjustor(File_ଧାଡ଼ି* flp, std::function<ଶୂନ୍ୟ(Ast_ଗଣ୍ଠି_Stmt*)> emit)
        : m_flp{flp}
        , m_emit{emit} {}

    // Emit Prefix adjustments until the current path is 'newPath'
    ଶୂନ୍ୟ adjust(ଅଚଳ string& newPath, AstCell* cellp, AstVarScope* vscp) {
        // Move up to enclosing path
        ଯେତେବେଳେ (!VString::startsWith(newPath, m_ଗଦା.back())) {
            m_emit(ନୂତନ Ast_Trace_କାଢ଼_Prefix{m_flp});
            m_ଗଦା.କାଢ଼_back();
        }

        ଯଦି (newPath == m_ଗଦା.back()) ଫେରନ୍ତୁ;

        ଅଚଳ VTracePrefixType lastScopeType =  //
            (cellp && VN_IS(cellp->modp(), Iface))
                    || (vscp && VN_IS(vscp->dtypep(), IfaceRefDType))
                ? VTracePrefixType::SCOPE_INTERFACE
                : VTracePrefixType::SCOPE_MODULE;
        ଅଚଳ std::string extraPrefix = newPath.substr(m_ଗଦା.back().size());
        size_t begin = 0;
        ଅଚଳ size_t last = extraPrefix.rfind(SEPARATOR);
        // Move down, one path element at a time
        ଯେତେବେଳେ (ସତ୍ୟ) {
            ଅଚଳ size_t end = extraPrefix.find(SEPARATOR, begin);
            ଯଦି (end == string::npos) ଭାଙ୍ଗ;
            ଅଚଳ string& extra = extraPrefix.substr(begin, end - begin);
            ଯଦି (end == last) {
                m_emit(ନୂତନ Ast_Trace_ଠେଲ_Prefix{m_flp, extra, lastScopeType});
            } ଅନ୍ୟଥା {
                m_emit(ନୂତନ Ast_Trace_ଠେଲ_Prefix{m_flp, extra, VTracePrefixType::SCOPE_MODULE});
            }
            m_ଗଦା.ଠେଲ_back(m_ଗଦା.back() + extra + SEPARATOR);
            begin = end + 1;
        }
        UASSERT(begin == extraPrefix.size(), "Should have consumed all of extraPrefix");
    }

    // Emit Prefix adjustments to unwind the path back to its original state
    ଶୂନ୍ୟ unwind() {
        ଯେତେବେଳେ (m_ଗଦା.size() > 1) {
            m_emit(ନୂତନ Ast_Trace_କାଢ଼_Prefix{m_flp});
            m_ଗଦା.କାଢ଼_back();
        }
    }
};

//######################################################################
// TraceDecl state, as a visitor of each AstNode

ଶ୍ରେଣୀ TraceDeclVisitor final : ସର୍ବସାଧାରଣ VNVisitor {
    // NODE STATE
    // AstCFunc::user1()                // uint32_t.  code offset for current type
    // AstCFunc::user2()                // VarScope* for dtype functions

    // STATE
    Ast_ଶୀର୍ଷ_Scope* ଅଚଳ m_ଶୀର୍ଷ_Scopep;  // The singleton AstTopScope
    ଅଚଳ AstScope* m_currScopep = nullptr;  // Current scope being visited

    std::vector<AstCFunc*> m_ଶୀର୍ଷ_Funcps;  // Top level trace initialization functions
    std::vector<AstCFunc*> m_subFuncps;  // Trace sub functions for this scope
    std::set<ଅଚଳ AstTraceDecl*> m_declUncalledps;  // Declarations not called
    // Functions per type and variable kind (wire vs logic etc.)
    ସଂରଚନା Dtype_Func_ଚାବି final {
        ଅଚଳ Ast_ଗଣ୍ଠି_DType* dtypep;
        VVarType varType;
        ତାର୍କିକ operator==(ଅଚଳ Dtype_Func_ଚାବି& other) ଅଚଳ {
            ଫେରନ୍ତୁ dtypep == other.dtypep && varType == other.varType;
        }
    };
    ସଂରଚନା Dtype_Func_ଚାବି_Hash final {
        size_t operator()(ଅଚଳ Dtype_Func_ଚାବି& ଚାବି) ଅଚଳ {
            ଫେରନ୍ତୁ std::hash<ଅଚଳ Ast_ଗଣ୍ଠି_DType*>{}(ଚାବି.dtypep)
                   ^ (std::hash<uint8_t>{}(ଚାବି.varType) << 1);
        }
    };
    std::unordered_map<Dtype_Func_ଚାବି, AstCFunc*, Dtype_Func_ଚାବି_Hash>
        m_dtypeFuncs;  // Functions per type+kind
    AstCFunc* m_dtypeFuncp = nullptr;  // Current type func
    AstCFunc* m_dtypeSubFuncp = nullptr;  // Current type sub func
    ଅଚଳ string m_dtypeArgs{", const char* name, uint32_t fidx, uint32_t c, "
                             "VerilatedTraceSigDirection direction"};  // Type func args
    AstTraceDecl* m_dtypeDeclp = nullptr;  // Current type func decl
    V3UniqueNames m_dtypeNames{""};  // Unique names for dtype funcs
    ତାର୍କିକ m_skipDtypeFunc = ମିଥ୍ୟା;  // Don't create a type func
    uint32_t m_offset = std::numeric_limits<uint32_t>::max();  // Offset for types
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_ଶୀର୍ଷ_Func_Size = 0;  // Size of the top function currently being built
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_subFuncSize = 0;  // Size of the sub function currently being built
    ତାର୍କିକ m_forceNewLeaf = ମିଥ୍ୟା;  // Force next addToSubFunc to start a fresh leaf
    size_t m_ଶୀର୍ଷ_Scope_ମୂଳ_Func_ଗଣନା = 0;  // Top-scope init functions used only for wrapper IOs
    ତାର୍କିକ m_ଶୀର୍ଷ_Scope_ମୂଳ_Phase = ମିଥ୍ୟା;  // Emitting top-scope wrapper IO declarations
    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_funcSizeLimit  // Maximum size of a function
        = v3Global.opt.outputSplitCTrace() ? v3Global.opt.outputSplitCTrace()
                                           : std::numeric_limits<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>::max();
    // Trace init functions to for each scope
    std::unordered_map<ଅଚଳ AstScope*, std::vector<AstCFunc*>> m_scopeInitFuncps;
    // Map from hierarchical scope name to the corresponding AstScope. Note that
    // this is a many-to-one mapping for interfaces, due to interface refs.
    std::unordered_map<std::string, ଅଚଳ AstScope*> m_pathToScopep;
    // Cell initialization placeholders:
    // (parent scope, cell under parent scope, statement)
    std::vector<std::tuple<AstScope*, AstCell*, Ast_ଗଣ୍ଠି_Stmt*>> m_cellInitPlaceholders;
    // Interface refs initialization placeholders:
    // (Interface ref variable, placeholder statement)
    std::vector<std::tuple<AstVarScope*, Ast_ଗଣ୍ଠି_Stmt*>> m_ifaceRefInitPlaceholders;

    // A trace entry under a scope is either:
    // - A variable (including interface references)
    // - A sub scope (stored as the cell corresponding to the sub scope)
    // Note: members are non-const to allow copy during sorting
    ଶ୍ରେଣୀ TraceEntry final {
        AstVarScope* m_vscp = nullptr;  // AstVarScope under scope being traced
        AstCell* m_cellp = nullptr;  // Sub scope (as AstCell) under scope being traced
        std::string m_path;  // Path to enclosing module in original hierarchy
        std::string m_name;  // Name of signal/subscope

        ଶୂନ୍ୟ init(ଅଚଳ std::string& name, Ast_ଗଣ୍ଠି* nodep, ତାର୍କିକ in_ଶୀର୍ଷ_Scope) {
            // Compute path in hierarchy and item name
            ଅଚଳ std::string& vcdName = Ast_ଗଣ୍ଠି::vcdName(name);
            AstVar* ଅଚଳ varp = VN_CAST(nodep, Var);
            ଯଦି (VN_IS(nodep, Cell) || VN_IS(varp->dtypep(), IfaceRefDType)) {
                // Cell or interface reference
                m_path = vcdName + " ";
                m_name.clear();
            } ଅନ୍ୟଥା ଯଦି (varp->isPrimaryIO()) {
                // Primary IO variable
                m_path = "$rootio ";
                m_name = vcdName;
            } ଅନ୍ୟଥା {
                // Other Variable
                ଅଚଳ size_t pos = vcdName.rfind(' ');
                ଅଚଳ size_t pathLen = pos == std::string::npos ? 0 : pos + 1;
                m_path = vcdName.substr(0, pathLen);
                m_name = vcdName.substr(pathLen);
            }

            // When creating a --lib-create library, drop the name of the selected top module.
            // This will be replaced by the instance name in the model that uses the library, or
            // restored at runtime if the library itself is traced as the root model. Other top
            // level entities ($unit, packages, ...) keep a '$libroot' wrapper so they still have
            // a stable location in the dump.
            ଯଦି (in_ଶୀର୍ଷ_Scope && !v3Global.opt.libCreate().empty()) {
                ଅଚଳ size_t start = m_path.find(' ');
                // Must have a prefix in the top scope with lib, as top wrapper signals not traced
                UASSERT_OBJ(start != std::string::npos, nodep, "No prefix with --lib-create");
                ଅଚଳ std::string prefix = m_path.substr(0, start);
                // Wrapper primary IOs stay under $rootio so a root-traced library can restore
                // them under the runtime C++ instance name without affecting embedded use.
                ଯଦି (prefix == "$rootio") ଫେରନ୍ତୁ;
                m_path = m_path.substr(start + 1);
                ଯଦି (v3Global.rootp()->trace_Lib_ଶୀର୍ଷ_Name() != prefix) m_path = "$libroot " + m_path;
            }
        }

    ସର୍ବସାଧାରଣ:
        explicit TraceEntry(ଅଚଳ AstScope* scopep, AstVarScope* vscp)
            : m_vscp{vscp} {
            init(vscp->varp()->name(), vscp->varp(), scopep->is_ଶୀର୍ଷ());
        }
        explicit TraceEntry(ଅଚଳ AstScope* scopep, AstCell* cellp)
            : m_cellp{cellp} {
            init(cellp->name(), cellp, scopep->is_ଶୀର୍ଷ());
        }
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା operatorCompare(ଅଚଳ TraceEntry& b) ଅଚଳ {
            ଯଦି (ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା cmp = path().compare(b.path())) ଫେରନ୍ତୁ cmp < 0;
            ଯଦି (ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା cmp = fileline().operatorCompare(b.fileline())) ଫେରନ୍ତୁ cmp < 0;
            ଫେରନ୍ତୁ name() < b.name();
        }
        AstVarScope* vscp() ଅଚଳ { ଫେରନ୍ତୁ m_vscp; }
        AstCell* cellp() ଅଚଳ { ଫେରନ୍ତୁ m_cellp; }
        ଅଚଳ std::string& path() ଅଚଳ { ଫେରନ୍ତୁ m_path; }
        ଶୂନ୍ୟ path(ଅଚଳ std::string& path) { m_path = path; }
        ଅଚଳ std::string& name() ଅଚଳ { ଫେରନ୍ତୁ m_name; }
        File_ଧାଡ଼ି& fileline() ଅଚଳ { ଫେରନ୍ତୁ m_vscp ? *m_vscp->fileline() : *m_cellp->fileline(); }
    };
    std::vector<TraceEntry> m_entries;  // Trace entries under current scope
    AstVarScope* m_traVscp = nullptr;  // Current AstVarScope we are constructing AstTraceDecls for
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* m_traValuep = nullptr;  // Value expression for current signal
    string m_traName;  // Name component for current signal

    VDouble0 m_statSigs;  // Statistic tracking
    VDouble0 m_statIgnSigs;  // Statistic tracking

    // METHODS

    string vscIgnoreTrace(ଅଚଳ AstVarScope* nodep) {
        // Return true if this shouldn't be traced
        // See also similar rule in V3Coverage::varIgnoreToggle
        ଅଚଳ AstVar* ଅଚଳ varp = nodep->varp();
        ଯଦି (!varp->isTrace()) ଫେରନ୍ତୁ "Verilator trace_off";
        ଯଦି (!nodep->isTrace()) ଫେରନ୍ତୁ "Verilator instance trace_off";
        // Automatics (typically, excluding forks) have no persistance over
        // time, and may optimize differently when multithreadeded or hierarchical.
        // Class automatics refer to being in a class but might still be pointed
        // to by a static, so are ok.
        ଯଦି (varp->lifetime().isAutomatic() && !varp->isClassMember() && !varp->isParam())
            ଫେରନ୍ତୁ "Automatic variable";

        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା width = recurseDTypeWidth(nodep->varp()->dtypep());
        ଯଦି (v3Global.opt.traceMaxWidth() && width > v3Global.opt.traceMaxWidth())
            ଫେରନ୍ତୁ "Width " + cvtToStr(width) + " > --trace-max-width";

        ଅଚଳ string prettyName = nodep->prettyName();
        ଯଦି (!V3Control::getScopeTraceOn(prettyName)) ଫେରନ୍ତୁ "Vlt scope trace_off";
        ଯଦି (!v3Global.opt.traceUnderscore()) {
            ଯଦି (!prettyName.empty() && prettyName[0] == '_') ଫେରନ୍ତୁ "Leading underscore";
            ଯଦି (prettyName.find("._") != string::npos) ଫେରନ୍ତୁ "Inlined leading underscore";
        }
        ଫେରନ୍ତୁ ""s;
    }

    ପୂର୍ଣ୍ଣସଂଖ୍ୟା recurseDTypeWidth(ଅଚଳ Ast_ଗଣ୍ଠି_DType* nodep) {
        ଯଦି (ଅଚଳ Ast_ଗଣ୍ଠି_Array_DType* adtypep = VN_CAST(nodep, ଗଣ୍ଠି_Array_DType))
            ଫେରନ୍ତୁ recurseDTypeWidth(adtypep->subDTypep()) * adtypep->declRange().elements();
        ଫେରନ୍ତୁ nodep->width();
    }

    AstCFunc* newCFunc(File_ଧାଡ଼ି* flp, ଅଚଳ string& name) {
        AstScope* ଅଚଳ ଶୀର୍ଷ_Scopep = m_ଶୀର୍ଷ_Scopep->scopep();
        AstCFunc* ଅଚଳ funcp = ନୂତନ AstCFunc{flp, name, ଶୀର୍ଷ_Scopep};
        funcp->argTypes(v3Global.opt.traceClassBase() + "* tracep");
        funcp->isTrace(ସତ୍ୟ);
        funcp->isStatic(ମିଥ୍ୟା);
        funcp->isLoose(ସତ୍ୟ);
        funcp->slow(ସତ୍ୟ);
        ଶୀର୍ଷ_Scopep->addBlocksp(funcp);
        ଫେରନ୍ତୁ funcp;
    }

    ଶୂନ୍ୟ add_To_ଶୀର୍ଷ_Func(Ast_ଗଣ୍ଠି_Stmt* stmtp) {
        ଯଦି (m_ଶୀର୍ଷ_Func_Size > m_funcSizeLimit || m_ଶୀର୍ଷ_Funcps.empty()) {
            m_ଶୀର୍ଷ_Func_Size = 0;
            //
            ଅଚଳ string n = cvtToStr(m_ଶୀର୍ଷ_Funcps.size());
            ଅଚଳ string name{"trace_init_top__" + n};
            AstCFunc* ଅଚଳ funcp = newCFunc(m_ଶୀର୍ଷ_Scopep->fileline(), name);
            m_ଶୀର୍ଷ_Funcps.ଠେଲ_back(funcp);
        }
        m_ଶୀର୍ଷ_Funcps.back()->addStmtsp(stmtp);
        m_ଶୀର୍ଷ_Func_Size += stmtp->ଗଣ୍ଠି_ଗଣନା();
    }

    ଶୂନ୍ୟ addToSubFunc(Ast_ଗଣ୍ଠି_Stmt* stmtp) {
        // TODO (maybe) -- sub funcs for dtype components
        ଯଦି (m_dtypeSubFuncp) {
            ଯଦି (m_subFuncSize > m_funcSizeLimit) newDeclSubFunc();
            m_dtypeSubFuncp->addStmtsp(stmtp);
            m_subFuncSize += stmtp->ଗଣ୍ଠି_ଗଣନା();
            ଫେରନ୍ତୁ;
        }
        // Defer trace splitting until V3Trace
        ଯଦି (m_subFuncps.empty() || m_forceNewLeaf) {
            m_forceNewLeaf = ମିଥ୍ୟା;
            File_ଧାଡ଼ି* ଅଚଳ flp = m_currScopep->fileline();
            ଅଚଳ string n = cvtToStr(m_subFuncps.size());
            ଅଚଳ string name
                = m_currScopep == m_ଶୀର୍ଷ_Scopep->scopep() && !v3Global.opt.libCreate().empty()
                      ? (m_ଶୀର୍ଷ_Scope_ମୂଳ_Phase ? "trace_init_leaf_root__" : "trace_init_leaf_top__")
                            + n
                      : "trace_init_sub__" + m_currScopep->nameDotless() + "__" + n;
            AstCFunc* ଅଚଳ funcp = newCFunc(flp, name);
            funcp->addStmtsp(ନୂତନ AstCStmt{flp, "const int c = vlSymsp->__Vm_baseCode;"});
            m_subFuncps.ଠେଲ_back(funcp);
        }
        m_subFuncps.back()->addStmtsp(stmtp);
        m_subFuncSize += stmtp->ଗଣ୍ଠି_ଗଣନା();
    }

    AstTraceDecl* addTraceDecl(ଅଚଳ VNumRange& arrayRange,
                               ପୂର୍ଣ୍ଣସଂଖ୍ୟା widthOverride,  // If !=0, is packed struct/array where basicp
                                                   // size misreflects one element
                               AstCCall* ଅଚଳ dtypeCallp = nullptr) {
        VNumRange bitRange;
        ଯଦି (widthOverride) {
            bitRange = VNumRange{widthOverride - 1, 0};
        } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstBasicDType* ଅଚଳ bdtypep = m_traValuep->dtypep()->basicp()) {
            bitRange = bdtypep->nrange();
        }
        File_ଧାଡ଼ି* ଅଚଳ flp = m_traVscp->fileline();
        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* valuep = m_traValuep->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
        ଅଚଳ ତାର୍କିକ validOffset = m_offset != std::numeric_limits<uint32_t>::max();
        AstTraceDecl* ଅଚଳ newp
            = ନୂତନ AstTraceDecl{flp,      m_traName,  m_traVscp->varp(), valuep,
                               bitRange, arrayRange, dtypeCallp,        validOffset};
        ଯଦି (validOffset) {
            newp->code(m_offset);
            ଯଦି (!dtypeCallp) { m_offset += newp->codeInc(); }
            valuep->foreach([&](AstVarRef* ଅଚଳ refp) {
                UASSERT_OBJ(refp->varScopep() == m_traVscp, refp,
                            "Trace decl expression references unexpected var");
                refp->replaceWith(ନୂତନ AstCExpr{flp, "__VdtypeVar", m_traVscp->width()});
                VL_DO_DANGLING(refp->delete_ବୃକ୍ଷ(), refp);
            });
        } ଅନ୍ୟଥା {
            newp->dtypeDeclp(m_dtypeDeclp);
        }
        m_declUncalledps.emplace(newp);
        addToSubFunc(newp);

        ଫେରନ୍ତୁ newp;
    }

    ଶୂନ୍ୟ addIgnore(ଅଚଳ string& why) {
        ++m_statIgnSigs;
        ଅଚଳ std::string cmt = "Tracing: "s + m_traName + " // Ignored: " + why;
        ଯଦି (debug() > 3 && m_traVscp) std::cout << "- " << m_traVscp->fileline() << cmt << '\n';
    }

    ଶୂନ୍ୟ fixupPlaceholder(ଅଚଳ std::string& path, Ast_ଗଣ୍ଠି_Stmt* placeholderp) {
        // Find the scope for the path. As we are working based on cell names,
        // it is possible there is no corresponding scope (e.g.: for an empty
        // module).
        ଅଚଳ auto it = m_pathToScopep.find(Ast_ଗଣ୍ଠି::prettyName(path));
        ଯଦି (it != m_pathToScopep.end()) {
            ଅଚଳ AstScope* ଅଚଳ scopep = it->second;
            File_ଧାଡ଼ି* ଅଚଳ flp = placeholderp->fileline();

            // Call the initialization functions for the scope
            Ast_ଗଣ୍ଠି* stmtp = nullptr;
            ପାଇଁ (AstCFunc* ଅଚଳ subFuncp : m_scopeInitFuncps.at(scopep)) {
                AstCCall* ଅଚଳ callp = ନୂତନ AstCCall{flp, subFuncp};
                callp->dtypeSetVoid();
                callp->argTypes("tracep");
                stmtp = Ast_ଗଣ୍ଠି::addNext(stmtp, callp->makeStmt());
            }

            // Add after the placeholder
            ଯଦି (stmtp) placeholderp->addNextHere(stmtp);
        }
        // Delete the placeholder
        placeholderp->unlinkFrBack();
        VL_DO_DANGLING(placeholderp->delete_ବୃକ୍ଷ(), placeholderp);
    }

    ଶୂନ୍ୟ fixupLibStub(ଅଚଳ std::string& path, Ast_ଗଣ୍ଠି_Stmt* placeholderp) {
        File_ଧାଡ଼ି* ଅଚଳ flp = placeholderp->fileline();

        // The library instance's model is named after its %m within the parent model,
        // which does not include the "l2-name" (top module) of the library
        std::string name = Ast_ଗଣ୍ଠି::prettyName(path);
        ଯଦି (!v3Global.opt.libCreate().empty()) {
            ଅଚଳ std::string ଶୀର୍ଷ_Prefix = v3Global.rootp()->trace_Lib_ଶୀର୍ଷ_Name() + ".";
            ଯଦି (VString::startsWith(name, ଶୀର୍ଷ_Prefix)) name = name.substr(ଶୀର୍ଷ_Prefix.size());
        }

        // Call the initialization function for the library instance
        AstCStmt* ଅଚଳ initp = ନୂତନ AstCStmt{flp};
        initp->add("{\n");
        initp->add("std::string __VlibName = vlSymsp->name();\n");
        initp->add("if (!__VlibName.empty()) __VlibName += '.';\n");
        initp->add("__VlibName += ");
        initp->add(ନୂତନ AstConst{flp, AstConst::String{}, name});
        initp->add(";\n");
        initp->add("tracep->initLib(__VlibName);\n");
        initp->add("}\n");

        placeholderp->addNextHere(initp);
        // Delete the placeholder
        VL_DO_DANGLING(placeholderp->unlinkFrBack()->delete_ବୃକ୍ଷ(), placeholderp);
    }

    ଶୂନ୍ୟ fixupPlaceholders() {
        // Fix up cell initialization placehodlers
        UINFO(9, "fixupPlaceholders()");
        ପାଇଁ (ଅଚଳ auto& item : m_cellInitPlaceholders) {
            ଅଚଳ AstScope* ଅଚଳ parentp = std::get<0>(item);
            ଅଚଳ AstCell* ଅଚଳ cellp = std::get<1>(item);
            Ast_ଗଣ୍ଠି_Stmt* ଅଚଳ placeholderp = std::get<2>(item);
            ଅଚଳ std::string path = parentp->name() + "__DOT__" + cellp->name();
            ଯଦି (cellp->modp()->verilatorLib()) {
                fixupLibStub(path, placeholderp);
            } ଅନ୍ୟଥା {
                fixupPlaceholder(path, placeholderp);
            }
        }

        // Fix up interface reference initialization placeholders
        ପାଇଁ (ଅଚଳ auto& item : m_ifaceRefInitPlaceholders) {
            ଅଚଳ AstVarScope* ଅଚଳ vscp = std::get<0>(item);
            Ast_ଗଣ୍ଠି_Stmt* ଅଚଳ placeholderp = std::get<1>(item);
            ଅଚଳ std::string path = vscp->scopep()->name() + "__DOT__" + vscp->varp()->name();
            fixupPlaceholder(path, placeholderp);
        }
    }

    ଶୂନ୍ୟ remove_Redundant_Prefix_ଠେଲ_କାଢ଼() {
        ପାଇଁ (ଅଚଳ auto& pair : m_scopeInitFuncps) {
            // cppcheck-suppress constVariablePointer
            ପାଇଁ (AstCFunc* ଅଚଳ funcp : pair.second) {
                Ast_ଗଣ୍ଠି* prevp = nullptr;
                Ast_ଗଣ୍ଠି* currp = funcp->stmtsp();
                ଯେତେବେଳେ (currp) {
                    Ast_ଗଣ୍ଠି* ଅଚଳ nextp = currp->nextp();
                    ଯଦି (VN_IS(prevp, Trace_ଠେଲ_Prefix) && VN_IS(currp, Trace_କାଢ଼_Prefix)) {
                        VL_DO_DANGLING(prevp->unlinkFrBack()->delete_ବୃକ୍ଷ(), prevp);
                        VL_DO_DANGLING(currp->unlinkFrBack()->delete_ବୃକ୍ଷ(), currp);
                    }
                    ଯଦି (!nextp) ଭାଙ୍ଗ;
                    prevp = nextp->backp();
                    currp = nextp;
                }
            }
        }
    }

    ଶୂନ୍ୟ checkCalls(ଅଚଳ AstCFunc* funcp) {
        ଯଦି (!v3Global.opt.debugCheck()) ଫେରନ୍ତୁ;
        checkCallsRecurse(funcp);
        ଯଦି (!m_declUncalledps.empty()) {  // LCOV_EXCL_START
            ପାଇଁ (auto tracep : m_declUncalledps) UINFO(0, "-nodep " << tracep);
            (*(m_declUncalledps.begin()))->v3fatalSrc("Created TraceDecl which is never called");
        }  // LCOV_EXCL_STOP
    }
    ଶୂନ୍ୟ checkCallsRecurse(ଅଚଳ AstCFunc* funcp) {
        funcp->foreach([ଏହି](ଅଚଳ Ast_ଗଣ୍ଠି* nodep) {
            ଯଦି (ଅଚଳ AstTraceDecl* ଅଚଳ declp = VN_CAST(nodep, TraceDecl)) {
                m_declUncalledps.erase(declp);
            } ଅନ୍ୟଥା ଯଦି (ଅଚଳ AstCCall* ଅଚଳ ccallp = VN_CAST(nodep, CCall)) {
                checkCallsRecurse(ccallp->funcp());
            }
        });
    }

    ତାର୍କିକ isBasicIO() { ଫେରନ୍ତୁ m_traVscp->varp()->isVLIO(); }
    ଶୂନ୍ୟ newDeclSubFunc() {
        File_ଧାଡ଼ି* ଅଚଳ flp = m_dtypeFuncp->fileline();
        AstCFunc* ଅଚଳ subFuncp = newCFunc(flp, m_dtypeNames.get("trace_init_dtype_sub__"));
        subFuncp->argTypes(subFuncp->argTypes() + m_dtypeArgs);
        AstCCall* ଅଚଳ subCallp = ନୂତନ AstCCall{flp, subFuncp};
        subCallp->dtypeSetVoid();
        subCallp->argTypes("tracep, name, fidx, c, direction");
        m_dtypeFuncp->addStmtsp(subCallp->makeStmt());
        m_dtypeSubFuncp = subFuncp;
        m_subFuncSize = 0;
    }
    ଶୂନ୍ୟ newDeclFunc(Ast_ଗଣ୍ଠି_DType* nodep) {
        Ast_ଗଣ୍ଠି_DType* ଅଚଳ skipTypep = nodep->skipRefp();
        // offset and direction args added in EmitCImp
        std::string callArgs{"tracep, \"" + VIdProtect::protect(m_traName) + "\""};
        VL_RESTORER_COPY(m_traName);
        File_ଧାଡ଼ି* ଅଚଳ flp = skipTypep->fileline();

        ଅଚଳ Dtype_Func_ଚାବି dtype_ଚାବି{skipTypep, m_traVscp->varp()->varType()};
        auto pair = m_dtypeFuncs.emplace(dtype_ଚାବି, nullptr);
        AstCFunc** funcpp = &pair.first->second;
        ଯଦି (pair.second) {
            *funcpp = newCFunc(flp, m_dtypeNames.get("trace_init_dtype__"));
            (*funcpp)->argTypes((*funcpp)->argTypes() + m_dtypeArgs);
            (*funcpp)->user2p(m_traVscp);
        }

        AstCCall* ଅଚଳ callp = ନୂତନ AstCCall{flp, *funcpp};
        callp->dtypeSetVoid();
        callp->argTypes(callArgs);
        m_dtypeDeclp = addTraceDecl(VNumRange{}, skipTypep->width(), callp);
        addToSubFunc(callp->makeStmt());

        ଯଦି (pair.second) {
            VL_RESTORER(m_offset);
            m_offset = 0;

            VL_RESTORER(m_dtypeFuncp);
            VL_RESTORER(m_dtypeSubFuncp);
            VL_RESTORER(m_subFuncSize);
            m_dtypeFuncp = *funcpp;
            newDeclSubFunc();
            ଯଦି (AstStructDType* ଅଚଳ dtypep = VN_CAST(skipTypep, StructDType)) {
                declStruct(dtypep, ସତ୍ୟ);
            } ଅନ୍ୟଥା ଯଦି (AstUnpackArrayDType* ଅଚଳ dtypep = VN_CAST(skipTypep, UnpackArrayDType)) {
                declUnpackedArray(dtypep, ସତ୍ୟ);
            } ଅନ୍ୟଥା ଯଦି (AstPackArrayDType* ଅଚଳ dtypep = VN_CAST(skipTypep, PackArrayDType)) {
                declPackedArray(dtypep, ସତ୍ୟ);
            } ଅନ୍ୟଥା {
                UASSERT_OBJ(ମିଥ୍ୟା, skipTypep, "Creating a trace function for an unexpected type");
            }
            m_dtypeFuncp->user1(m_offset);
        }

        m_dtypeDeclp->codeInc((*funcpp)->user1());
        m_offset += m_dtypeDeclp->codeInc();
    }
    ଶୂନ୍ୟ declUnpackedArray(AstUnpackArrayDType* ଅଚଳ nodep, ତାର୍କିକ newFunc) {
        string prefixName(newFunc ? "name" : m_traName);

        VL_RESTORER_COPY(m_traName);
        File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();

        addToSubFunc(ନୂତନ Ast_Trace_ଠେଲ_Prefix{flp, prefixName, VTracePrefixType::ARRAY_UNPACKED,
                                            nodep->ବାମ(), nodep->ଡାହାଣ(), !newFunc});

        ଯଦି (VN_IS(nodep->subDTypep()->skipRefToEnump(),
                  BasicDType)  // Nothing lower than this array
            && m_traVscp->dtypep()->skipRefToEnump() == nodep) {  // Nothing above this array
            // Simple 1-D array, use existing V3EmitC runtime loop rather than unrolling
            // This will put "(index)" at end of signal name for us
            ଯଦି (m_traVscp->dtypep()->skipRefToEnump()->isString()) {
                addIgnore("Unsupported: strings");
            } ଅନ୍ୟଥା {
                m_traName = "";
                addTraceDecl(nodep->declRange(), 0);
            }
        } ଅନ୍ୟଥା {
            Ast_ଗଣ୍ଠି_DType* ଅଚଳ subtypep = nodep->subDTypep()->skipRefToEnump();
            // Always iterate left index to right index
            ପାଇଁ (ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା i : nodep->declRange().seq_ବାମ_To_ଡାହାଣ()) {
                VL_RESTORER(m_traValuep);
                m_traName = '[' + std::to_string(i) + ']';
                m_traValuep = m_traValuep->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
                m_traValuep = ନୂତନ AstArraySel{flp, m_traValuep, i - nodep->lo()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_ବୃକ୍ଷ(), m_traValuep);
            }
        }

        addToSubFunc(ନୂତନ Ast_Trace_କାଢ଼_Prefix{flp});
    }
    ଶୂନ୍ୟ declPackedArray(AstPackArrayDType* ଅଚଳ nodep, ତାର୍କିକ newFunc) {
        string prefixName(newFunc ? "name" : m_traName);
        Ast_ଗଣ୍ଠି_DType* ଅଚଳ subtypep = nodep->subDTypep()->skipRefToEnump();

        VL_RESTORER_COPY(m_traName);
        File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();

        addToSubFunc(ନୂତନ Ast_Trace_ଠେଲ_Prefix{flp, prefixName, VTracePrefixType::ARRAY_PACKED,
                                            nodep->ବାମ(), nodep->ଡାହାଣ(), !newFunc});

        // Always iterate left index to right index
        ପାଇଁ (ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା i : nodep->declRange().seq_ବାମ_To_ଡାହାଣ()) {
            VL_RESTORER(m_traValuep);
            m_traName = '[' + std::to_string(i) + ']';
            ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା lsb = (i - nodep->lo()) * subtypep->width();
            m_traValuep = m_traValuep->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
            m_traValuep = ନୂତନ AstSel{flp, m_traValuep, lsb, subtypep->width()};
            m_traValuep->dtypep(subtypep);
            iterate(subtypep);
            VL_DO_CLEAR(m_traValuep->delete_ବୃକ୍ଷ(), m_traValuep = nullptr);
        }

        addToSubFunc(ନୂତନ Ast_Trace_କାଢ଼_Prefix{flp});
    }
    ଶୂନ୍ୟ declStruct(AstStructDType* ଅଚଳ nodep, ତାର୍କିକ newFunc) {
        File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
        string prefixName(newFunc ? "name" : m_traName);
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା nMembers = 0;
        ପାଇଁ (Ast_ଗଣ୍ଠି* mp = nodep->membersp(); mp; mp = mp->nextp()) ++nMembers;
        ଯଦି (!nodep->packed()) {
            addToSubFunc(ନୂତନ Ast_Trace_ଠେଲ_Prefix{flp, prefixName,  //
                                                VTracePrefixType::STRUCT_UNPACKED, nMembers, 0,
                                                !newFunc});
            ପାଇଁ (ଅଚଳ AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_ଗଣ୍ଠି_DType* ଅଚଳ subtypep = itemp->subDTypep()->skipRefToEnump();
                m_traName = itemp->prettyName();
                VL_RESTORER(m_traValuep);
                m_traValuep = m_traValuep->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
                m_traValuep = ନୂତନ AstStructSel{flp, m_traValuep, itemp->name()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_ବୃକ୍ଷ(), m_traValuep);
            }
            addToSubFunc(ନୂତନ Ast_Trace_କାଢ଼_Prefix{flp});
        } ଅନ୍ୟଥା {
            addToSubFunc(ନୂତନ Ast_Trace_ଠେଲ_Prefix{flp, prefixName,  //
                                                VTracePrefixType::STRUCT_PACKED, nMembers, 0,
                                                !newFunc});
            ପାଇଁ (ଅଚଳ AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_ଗଣ୍ଠି_DType* ଅଚଳ subtypep = itemp->subDTypep()->skipRefToEnump();
                m_traName = itemp->prettyName();
                VL_RESTORER(m_traValuep);
                m_traValuep = m_traValuep->clone_ବୃକ୍ଷ(ମିଥ୍ୟା);
                m_traValuep = ନୂତନ AstSel{flp, m_traValuep, itemp->lsb(), subtypep->width()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_ବୃକ୍ଷ(), m_traValuep);
            }
            addToSubFunc(ନୂତନ Ast_Trace_କାଢ଼_Prefix{flp});
        }
    }

    // VISITORS
    ଶୂନ୍ୟ visit(AstScope* nodep) override {
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(!m_currScopep, nodep, "Should not nest");
        UASSERT_OBJ(m_subFuncps.empty(), nodep, "Should not nest");
        UASSERT_OBJ(m_entries.empty(), nodep, "Should not nest");
        UASSERT_OBJ(!m_traVscp, nodep, "Should not nest");
        UASSERT_OBJ(!m_traValuep, nodep, "Should not nest");
        UASSERT_OBJ(m_traName.empty(), nodep, "Should not nest");

        // If this is a stub for a --lib-create library, skip.
        ଯଦି (nodep->modp()->verilatorLib()) ଫେରନ୍ତୁ;

        VL_RESTORER(m_currScopep);
        m_currScopep = nodep;

        // Gather signals under this scope
        iterateChildrenConst(nodep);

        // Gather cells under this scope
        ପାଇଁ (Ast_ଗଣ୍ଠି* stmtp = nodep->modp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            ଯଦି (AstCell* ଅଚଳ cellp = VN_CAST(stmtp, Cell)) m_entries.emplace_back(nodep, cellp);
        }

        ଯଦି (!m_entries.empty()) {
            // Sort trace entries, by enclosing instance (necessary for single traversal of
            // hierarchy during initialization), then by source location, then by name.
            std::stable_sort(
                m_entries.begin(), m_entries.end(),
                [](ଅଚଳ TraceEntry& a, ଅଚଳ TraceEntry& b) { ଫେରନ୍ତୁ a.operatorCompare(b); });

            File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
            PathAdjustor pathAdjustor{flp, [&](Ast_ଗଣ୍ଠି_Stmt* stmtp) { addToSubFunc(stmtp); }};
            ଅଚଳ ତାର୍କିକ split_ମୂଳ_Primary_Ios = nodep->is_ଶୀର୍ଷ() && !v3Global.opt.libCreate().empty();
            ଅଚଳ auto emitEntry = [&](ଅଚଳ TraceEntry& entry) {
                AstVarScope* ଅଚଳ vscp = entry.vscp();
                // Adjust name prefix based on path in hierarchy
                UINFO(9, "path='" << entry.path() << "' name='" << entry.name() << "' "
                                  << (entry.cellp() ? static_cast<Ast_ଗଣ୍ଠି*>(entry.cellp())
                                                    : static_cast<Ast_ଗଣ୍ଠି*>(entry.vscp())));
                pathAdjustor.adjust(entry.path(), entry.cellp(), entry.vscp());

                m_traName = entry.name();

                ଯଦି (vscp) {
                    // This is a signal: build AstTraceDecl for it
                    m_traVscp = vscp;
                    ଅଚଳ string& ignoreReason = vscIgnoreTrace(m_traVscp);
                    ଯଦି (!ignoreReason.empty()) {
                        addIgnore(ignoreReason);
                    } ଅନ୍ୟଥା {
                        ++m_statSigs;
                        // Create reference to whole signal. We will operate on this during the
                        // traversal.
                        m_traValuep
                            = ନୂତନ AstVarRef{m_traVscp->fileline(), m_traVscp, VAccess::READ};
                        // Recurse into data type of the signal. The visit methods will add
                        // AstTraceDecls.
                        iterate(m_traVscp->varp()->dtypep()->skipRefToEnump());
                        // Delete reference created above. Traversal cloned it as required.
                        ଯଦି (m_traValuep) {
                            VL_DO_DANGLING(m_traValuep->delete_ବୃକ୍ଷ(), m_traValuep);
                            // Note: Sometimes VL_DANGLING is a no-op, but we have assertions
                            // on m_traValuep being nullptr, so make sure it is.
                            m_traValuep = nullptr;
                        }
                    }
                } ଅନ୍ୟଥା {
                    // This is a subscope: insert a placeholder to be fixed up later
                    AstCell* ଅଚଳ cellp = entry.cellp();
                    Ast_ଗଣ୍ଠି_Stmt* ଅଚଳ stmtp = ନୂତନ AstComment{
                        cellp->fileline(), "Instance init for: " + cellp->prettyName()};
                    addToSubFunc(stmtp);
                    m_cellInitPlaceholders.emplace_back(nodep, cellp, stmtp);
                }
            };
            ଯଦି (split_ମୂଳ_Primary_Ios) {
                m_ଶୀର୍ଷ_Scope_ମୂଳ_Phase = ସତ୍ୟ;
                ପାଇଁ (ଅଚଳ TraceEntry& entry : m_entries) {
                    AstVarScope* ଅଚଳ vscp = entry.vscp();
                    ଯଦି (!(vscp && vscp->varp()->isPrimaryIO())) ଜାରି;
                    emitEntry(entry);
                }
                m_ଶୀର୍ଷ_Scope_ମୂଳ_Phase = ମିଥ୍ୟା;
                pathAdjustor.unwind();
                m_ଶୀର୍ଷ_Scope_ମୂଳ_Func_ଗଣନା = m_subFuncps.size();
                ଯଦି (m_ଶୀର୍ଷ_Scope_ମୂଳ_Func_ଗଣନା) {
                    // Force the next addToSubFunc to start a fresh leaf so
                    // root-phase decls and top-phase decls live in separate
                    // functions. Without this, both phases land in the same
                    // leaf and `trace_init_top` ends up emitting both the
                    // $rootio-prefixed IOs and the regular-scope decls, so
                    // each IO gets traced twice when the lib is consumed
                    // via initLib(name).
                    m_subFuncSize = 0;
                    m_forceNewLeaf = ସତ୍ୟ;
                }
            }
            ପାଇଁ (ଅଚଳ TraceEntry& entry : m_entries) {
                AstVarScope* ଅଚଳ vscp = entry.vscp();
                ଯଦି (split_ମୂଳ_Primary_Ios && vscp && vscp->varp()->isPrimaryIO()) ଜାରି;
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
        ଅଚଳ std::string path = nodep->prettyName();
        m_pathToScopep.emplace(path, nodep);

        // Save the hierarchical names of interface references that reference this scope
        ଅଚଳ AstCell* ଅଚଳ cellp = nodep->aboveCellp();
        ଯଦି (cellp
            && VN_IS(cellp->modp(), Iface)
            // Exclude classes that were inside interfaces, the cell's modp is an interface,
            // but the nodep->module is the Class
            && VN_IS(nodep->modp(), Iface)) {
            ଅଚଳ size_t lastDot = path.find_last_of('.');
            UASSERT_OBJ(lastDot != string::npos, nodep,
                        "Expected an interface scope name to have at least one dot: " << path);
            ଅଚଳ std::string parentPath = path.substr(0, lastDot + 1);

            ପାଇଁ (AstIntfRef *intfRefp = cellp->intfRefsp(), *nextp; intfRefp; intfRefp = nextp) {
                nextp = VN_AS(intfRefp->nextp(), IntfRef);

                ଅଚଳ std::string refName = intfRefp->prettyName();

                // Assume only references under the same parent scope reference
                // the same interface.
                // TODO: This is not actually correct. An interface can propagate
                //       upwards and sideways when passed to a port via a downward
                //       hierarchical reference, which we will miss here.
                ଯଦି (!VString::startsWith(refName, parentPath)) ଜାରି;

                // Save the mapping from the path of the reference to the scope
                m_pathToScopep.emplace(refName, nodep);

                // No more need for AstIntfRef, unless V3EmitCSyms wants it for VPI
                ଯଦି (!v3Global.opt.vpi()) {
                    VL_DO_DANGLING(intfRefp->unlinkFrBack()->delete_ବୃକ୍ଷ(), intfRefp);
                }
            }
        }
    }
    ଶୂନ୍ୟ visit(AstVarScope* nodep) override {
        UASSERT_OBJ(m_currScopep, nodep, "AstVarScope not under AstScope");

        // Prefilter - things that get added to m_vscps will either get traced or get a comment as
        // to why they are not traced. Generally these conditions doesn't need updating, instead
        // use varp->isTrace() and/or vscIgnoreTrace.
        ଯଦି (nodep->varp()->isTemp() && !nodep->varp()->isTrace()) ଫେରନ୍ତୁ;
        ଯଦି (nodep->varp()->isClassMember()) ଫେରନ୍ତୁ;
        ଯଦି (nodep->varp()->isFuncLocal()) ଫେରନ୍ତୁ;

        // When creating a --lib-create library ...
        ଯଦି (!v3Global.opt.libCreate().empty()) {
            // Ignore parameters in packages. These will be traced at the top level.
            ଯଦି (nodep->varp()->isParam() && VN_IS(nodep->scopep()->modp(), Package)) ଫେରନ୍ତୁ;
        }

        // Add to traced signal list
        m_entries.emplace_back(m_currScopep, nodep);
    }

    // VISITORS - Data types when tracing
    ଶୂନ୍ୟ visit(AstConstDType* nodep) override {
        ଯଦି (!m_traVscp) ଫେରନ୍ତୁ;
        VL_RESTORER(m_offset);
        VL_RESTORER(m_skipDtypeFunc);
        m_skipDtypeFunc = ସତ୍ୟ;
        iterate(nodep->subDTypep()->skipRefToEnump());
    }
    ଶୂନ୍ୟ visit(AstRefDType* nodep) override {
        ଯଦି (!m_traVscp) ଫେରନ୍ତୁ;
        VL_RESTORER(m_offset);
        iterate(nodep->subDTypep()->skipRefToEnump());
    }
    ଶୂନ୍ୟ visit(AstIfaceRefDType* /*nodep*/) override {
        ଯଦି (!m_traVscp) ଫେରନ୍ତୁ;
        // Insert a placeholder to be fixed up later
        File_ଧାଡ଼ି* ଅଚଳ flp = m_traVscp->fileline();
        Ast_ଗଣ୍ଠି_Stmt* ଅଚଳ stmtp
            = ନୂତନ AstComment{flp, "Interface ref init for: " + m_traVscp->prettyName()};
        addToSubFunc(stmtp);
        m_ifaceRefInitPlaceholders.emplace_back(m_traVscp, stmtp);
    }
    ଶୂନ୍ୟ visit(AstUnpackArrayDType* nodep) override {
        // Note more specific dtypes above
        ଯଦି (!m_traVscp) ଫେରନ୍ତୁ;

        ଯଦି (v3Global.opt.traceMaxArray()
            && static_cast<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>(nodep->arrayUnpackedElements()) > v3Global.opt.traceMaxArray()) {
            addIgnore("Wide memory > --trace-max-array ents");
            ଫେରନ୍ତୁ;
        }

        // Skip unpacked arrays of non-traceable leaf types (e.g. strings)
        ଯଦି (VN_IS(nodep->subDTypep()->skipRefToEnump(), BasicDType)
            && nodep->subDTypep()->skipRefToEnump()->isString()) {
            addIgnore("Unsupported: strings");
            ଫେରନ୍ତୁ;
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        ଯଦି (isBasicIO()) m_skipDtypeFunc = ସତ୍ୟ;

        ଯଦି (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_offset);
            newDeclFunc(nodep);
        }
        declUnpackedArray(nodep, ମିଥ୍ୟା);
    }
    ଶୂନ୍ୟ visit(AstPackArrayDType* nodep) override {
        ଯଦି (!m_traVscp) ଫେରନ୍ତୁ;

        ଯଦି (!v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit.
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            addTraceDecl(VNumRange{}, nodep->width());
            ଫେରନ୍ତୁ;
        }

        Ast_ଗଣ୍ଠି_DType* ଅଚଳ subtypep = nodep->subDTypep()->skipRefToEnump();

        // Do not unroll if the elements are simple 'bit', or 'logic'
        ଯଦି (AstBasicDType* ଅଚଳ basicp = VN_CAST(subtypep->skipRefp(), BasicDType)) {
            ଯଦି (basicp->isBitLogic() && !basicp->isRanged()) {
                addTraceDecl(VNumRange{}, nodep->width());
                ଫେରନ୍ତୁ;
            }
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        ଯଦି (isBasicIO()) m_skipDtypeFunc = ସତ୍ୟ;

        ଯଦି (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_offset);
            newDeclFunc(nodep);
        }
        declPackedArray(nodep, ମିଥ୍ୟା);
    }
    ଶୂନ୍ୟ visit(AstStructDType* nodep) override {
        ଯଦି (!m_traVscp) ଫେରନ୍ତୁ;

        ଯଦି (nodep->packed() && !v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            addTraceDecl(VNumRange{}, nodep->width());
            ଫେରନ୍ତୁ;
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        ଯଦି (isBasicIO()) m_skipDtypeFunc = ସତ୍ୟ;

        // Only create sub functions for top-level structs, i.e. don't have struct funcs
        // call other struct funcs for child types.  This could easily be done for decl funcs
        // but full / chg funcs would require copying / aligning data for child types or more
        // complicated / wonky / generalized data access.
        ଯଦି (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_offset);
            newDeclFunc(nodep);
        }
        declStruct(nodep, ମିଥ୍ୟା);
    }
    ଶୂନ୍ୟ visit(AstUnionDType* nodep) override {
        ଯଦି (!m_traVscp) ଫେରନ୍ତୁ;

        VL_RESTORER(m_skipDtypeFunc);
        m_skipDtypeFunc = ସତ୍ୟ;

        ଯଦି (nodep->packed() && !v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            addTraceDecl(VNumRange{}, nodep->width());
            ଫେରନ୍ତୁ;
        }

        VL_RESTORER_COPY(m_traName);
        File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();

        ପୂର୍ଣ୍ଣସଂଖ୍ୟା nMembers = 0;
        ପାଇଁ (Ast_ଗଣ୍ଠି* mp = nodep->membersp(); mp; mp = mp->nextp()) ++nMembers;

        ଯଦି (!nodep->packed()) {
            addIgnore("Unsupported: Unpacked union");
        } ଅନ୍ୟଥା {
            addToSubFunc(ନୂତନ Ast_Trace_ଠେଲ_Prefix{flp, m_traName,  //
                                                VTracePrefixType::UNION_PACKED, nMembers});
            ପାଇଁ (ଅଚଳ AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_ଗଣ୍ଠି_DType* ଅଚଳ subtypep = itemp->subDTypep()->skipRefToEnump();
                m_traName = itemp->prettyName();
                iterate(subtypep);
            }
            addToSubFunc(ନୂତନ Ast_Trace_କାଢ଼_Prefix{flp});
        }
    }
    ଶୂନ୍ୟ visit(AstBasicDType* nodep) override {
        ଯଦି (!m_traVscp) ଫେରନ୍ତୁ;
        ଯଦି (nodep->isString()) {
            addIgnore("Unsupported: strings");
        } ଅନ୍ୟଥା {
            addTraceDecl(VNumRange{}, 0);
        }
    }
    ଶୂନ୍ୟ visit(AstEnumDType* nodep) override { iterate(nodep->skipRefp()); }
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି_DType*) override {
        // Note more specific dtypes above
        ଯଦି (!m_traVscp) ଫେରନ୍ତୁ;
        addIgnore("Unsupported: data type");
    }

    //--------------------
    ଶୂନ୍ୟ visit(Ast_ଗଣ୍ଠି* nodep) override { iterateChildren(nodep); }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    explicit TraceDeclVisitor(AstNetlist* nodep)
        : m_ଶୀର୍ଷ_Scopep{nodep->ଶୀର୍ଷ_Scopep()} {
        File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();

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
        remove_Redundant_Prefix_ଠେଲ_କାଢ଼();

        ଅଚଳ std::vector<AstCFunc*>& ଶୀର୍ଷ_Scope_Funcps = m_scopeInitFuncps.at(m_ଶୀର୍ଷ_Scopep->scopep());
        AstCFunc* ମୂଳ_Funcp = nullptr;
        ଯଦି (!v3Global.opt.libCreate().empty()) {
            ମୂଳ_Funcp = newCFunc(flp, "trace_init_root");
            ମୂଳ_Funcp->entryPoint(ସତ୍ୟ);
            ପାଇଁ (size_t i = 0; i < m_ଶୀର୍ଷ_Scope_ମୂଳ_Func_ଗଣନା; ++i) {
                AstCCall* ଅଚଳ callp = ନୂତନ AstCCall{flp, ଶୀର୍ଷ_Scope_Funcps.at(i)};
                callp->dtypeSetVoid();
                callp->argTypes("tracep");
                ମୂଳ_Funcp->addStmtsp(callp->makeStmt());
            }
            ଯଦି (!m_ଶୀର୍ଷ_Scope_ମୂଳ_Func_ଗଣନା) ମୂଳ_Funcp->addStmtsp(ନୂତନ AstComment{flp, "Empty"});
        }

        // Call the non-wrapper initialization functions of the root scope from the top function
        ପାଇଁ (size_t i = m_ଶୀର୍ଷ_Scope_ମୂଳ_Func_ଗଣନା; i < ଶୀର୍ଷ_Scope_Funcps.size(); ++i) {
            AstCFunc* ଅଚଳ funcp = ଶୀର୍ଷ_Scope_Funcps.at(i);
            AstCCall* ଅଚଳ callp = ନୂତନ AstCCall{flp, funcp};
            callp->dtypeSetVoid();
            callp->argTypes("tracep");
            add_To_ଶୀର୍ଷ_Func(callp->makeStmt());
        }

        // Ensure a top function exists, in case there was nothing to trace at all
        ଯଦି (m_ଶୀର୍ଷ_Funcps.empty()) add_To_ଶୀର୍ଷ_Func(ନୂତନ AstComment{flp, "Empty"});

        // Create single top level function, if more than one exists
        ଯଦି (m_ଶୀର୍ଷ_Funcps.size() > 1) {
            AstCFunc* ଅଚଳ ଶୀର୍ଷ_Funcp = newCFunc(flp, "");
            ପାଇଁ (AstCFunc* funcp : m_ଶୀର୍ଷ_Funcps) {
                AstCCall* ଅଚଳ callp = ନୂତନ AstCCall{flp, funcp};
                callp->dtypeSetVoid();
                callp->argTypes("tracep");
                ଶୀର୍ଷ_Funcp->addStmtsp(callp->makeStmt());
            }
            m_ଶୀର୍ଷ_Funcps.clear();
            m_ଶୀର୍ଷ_Funcps.ଠେଲ_back(ଶୀର୍ଷ_Funcp);
        }

        // Set name of top level function
        AstCFunc* ଅଚଳ ଶୀର୍ଷ_Funcp = m_ଶୀର୍ଷ_Funcps.front();
        ଶୀର୍ଷ_Funcp->name("trace_init_top");
        ଶୀର୍ଷ_Funcp->entryPoint(ସତ୍ୟ);

        ଯଦି (ମୂଳ_Funcp && v3Global.opt.debugCheck()) checkCallsRecurse(ମୂଳ_Funcp);
        checkCalls(ଶୀର୍ଷ_Funcp);
    }
    ~TraceDeclVisitor() override {
        V3Stats::addStat("Tracing, Traced signals", m_statSigs);
        V3Stats::addStat("Tracing, Ignored signals", m_statIgnSigs);
    }
};

//######################################################################
// Trace class functions

ଶୂନ୍ୟ V3TraceDecl::traceDeclAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TraceDeclVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_ବୃକ୍ଷ("tracedecl", 0, dump_ବୃକ୍ଷ_Either_Level() >= 3);
}
