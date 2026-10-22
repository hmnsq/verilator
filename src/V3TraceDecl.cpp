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

#અન્તર્ભાવ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#અન્તર્ભાવ "V3TraceDecl.h"

#અન્તર્ભાવ "V3Ast.h"
#અન્તર્ભાવ "V3Control.h"
#અન્તર્ભાવ "V3EmitCBase.h"
#અન્તર્ભાવ "V3Error.h"
#અન્તર્ભાવ "V3File.h"
#અન્તર્ભાવ "V3Global.h"
#અન્તર્ભાવ "V3Number.h"
#અન્તર્ભાવ "V3Stats.h"
#અન્તર્ભાવ "V3UniqueNames.h"

#અન્તર્ભાવ <cstdint>
#અન્તર્ભાવ <functional>
#અન્તર્ભાવ <limits>
#અન્તર્ભાવ <tuple>
#અન્તર્ભાવ <unordered_map>
#અન્તર્ભાવ <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Utility class to emit path adjustments

વર્ગ PathAdjustor final {
    File_લીટી* અચલ m_flp;  // FileLine used for created nodes
    std::function<શૂન્ય(Ast_ગાંઠ_Stmt*)> m_emit;  // Function called with adjustment statements
    std::vector<std::string> m_ઢગલો{""};  // Stack of current paths

    સ્થિર constexpr અક્ષર SEPARATOR = ' ';

જાહેર:
    explicit PathAdjustor(File_લીટી* flp, std::function<શૂન્ય(Ast_ગાંઠ_Stmt*)> emit)
        : m_flp{flp}
        , m_emit{emit} {}

    // Emit Prefix adjustments until the current path is 'newPath'
    શૂન્ય adjust(અચલ string& newPath, AstCell* cellp, AstVarScope* vscp) {
        // Move up to enclosing path
        જ્યારે (!VString::startsWith(newPath, m_ઢગલો.back())) {
            m_emit(નવું Ast_Trace_કાઢો_Prefix{m_flp});
            m_ઢગલો.કાઢો_back();
        }

        જો (newPath == m_ઢગલો.back()) પાછા;

        અચલ VTracePrefixType lastScopeType =  //
            (cellp && VN_IS(cellp->modp(), Iface))
                    || (vscp && VN_IS(vscp->dtypep(), IfaceRefDType))
                ? VTracePrefixType::SCOPE_INTERFACE
                : VTracePrefixType::SCOPE_MODULE;
        અચલ std::string extraPrefix = newPath.substr(m_ઢગલો.back().size());
        size_t begin = 0;
        અચલ size_t last = extraPrefix.rfind(SEPARATOR);
        // Move down, one path element at a time
        જ્યારે (સાચું) {
            અચલ size_t end = extraPrefix.find(SEPARATOR, begin);
            જો (end == string::npos) તોડો;
            અચલ string& extra = extraPrefix.substr(begin, end - begin);
            જો (end == last) {
                m_emit(નવું Ast_Trace_ધકેલો_Prefix{m_flp, extra, lastScopeType});
            } નહીંતર {
                m_emit(નવું Ast_Trace_ધકેલો_Prefix{m_flp, extra, VTracePrefixType::SCOPE_MODULE});
            }
            m_ઢગલો.ધકેલો_back(m_ઢગલો.back() + extra + SEPARATOR);
            begin = end + 1;
        }
        UASSERT(begin == extraPrefix.size(), "Should have consumed all of extraPrefix");
    }

    // Emit Prefix adjustments to unwind the path back to its original state
    શૂન્ય unwind() {
        જ્યારે (m_ઢગલો.size() > 1) {
            m_emit(નવું Ast_Trace_કાઢો_Prefix{m_flp});
            m_ઢગલો.કાઢો_back();
        }
    }
};

//######################################################################
// TraceDecl state, as a visitor of each AstNode

વર્ગ TraceDeclVisitor final : જાહેર VNVisitor {
    // NODE STATE
    // AstCFunc::user1()                // uint32_t.  code offset for current type
    // AstCFunc::user2()                // VarScope* for dtype functions

    // STATE
    Ast_ટોચ_Scope* અચલ m_ટોચ_Scopep;  // The singleton AstTopScope
    અચલ AstScope* m_currScopep = nullptr;  // Current scope being visited

    std::vector<AstCFunc*> m_ટોચ_Funcps;  // Top level trace initialization functions
    std::vector<AstCFunc*> m_subFuncps;  // Trace sub functions for this scope
    std::set<અચલ AstTraceDecl*> m_declUncalledps;  // Declarations not called
    // Functions per type and variable kind (wire vs logic etc.)
    સંરચના Dtype_Func_ચાવી final {
        અચલ Ast_ગાંઠ_DType* dtypep;
        VVarType varType;
        તાર્કિક operator==(અચલ Dtype_Func_ચાવી& other) અચલ {
            પાછા dtypep == other.dtypep && varType == other.varType;
        }
    };
    સંરચના Dtype_Func_ચાવી_Hash final {
        size_t operator()(અચલ Dtype_Func_ચાવી& ચાવી) અચલ {
            પાછા std::hash<અચલ Ast_ગાંઠ_DType*>{}(ચાવી.dtypep)
                   ^ (std::hash<uint8_t>{}(ચાવી.varType) << 1);
        }
    };
    std::unordered_map<Dtype_Func_ચાવી, AstCFunc*, Dtype_Func_ચાવી_Hash>
        m_dtypeFuncs;  // Functions per type+kind
    AstCFunc* m_dtypeFuncp = nullptr;  // Current type func
    AstCFunc* m_dtypeSubFuncp = nullptr;  // Current type sub func
    અચલ string m_dtypeArgs{", const char* name, uint32_t fidx, uint32_t c, "
                             "VerilatedTraceSigDirection direction"};  // Type func args
    AstTraceDecl* m_dtypeDeclp = nullptr;  // Current type func decl
    V3UniqueNames m_dtypeNames{""};  // Unique names for dtype funcs
    તાર્કિક m_skipDtypeFunc = ખોટું;  // Don't create a type func
    uint32_t m_offset = std::numeric_limits<uint32_t>::max();  // Offset for types
    પૂર્ણાંક m_ટોચ_Func_Size = 0;  // Size of the top function currently being built
    પૂર્ણાંક m_subFuncSize = 0;  // Size of the sub function currently being built
    તાર્કિક m_forceNewLeaf = ખોટું;  // Force next addToSubFunc to start a fresh leaf
    size_t m_ટોચ_Scope_મૂળ_Func_ગણતરી = 0;  // Top-scope init functions used only for wrapper IOs
    તાર્કિક m_ટોચ_Scope_મૂળ_Phase = ખોટું;  // Emitting top-scope wrapper IO declarations
    અચલ પૂર્ણાંક m_funcSizeLimit  // Maximum size of a function
        = v3Global.opt.outputSplitCTrace() ? v3Global.opt.outputSplitCTrace()
                                           : std::numeric_limits<પૂર્ણાંક>::max();
    // Trace init functions to for each scope
    std::unordered_map<અચલ AstScope*, std::vector<AstCFunc*>> m_scopeInitFuncps;
    // Map from hierarchical scope name to the corresponding AstScope. Note that
    // this is a many-to-one mapping for interfaces, due to interface refs.
    std::unordered_map<std::string, અચલ AstScope*> m_pathToScopep;
    // Cell initialization placeholders:
    // (parent scope, cell under parent scope, statement)
    std::vector<std::tuple<AstScope*, AstCell*, Ast_ગાંઠ_Stmt*>> m_cellInitPlaceholders;
    // Interface refs initialization placeholders:
    // (Interface ref variable, placeholder statement)
    std::vector<std::tuple<AstVarScope*, Ast_ગાંઠ_Stmt*>> m_ifaceRefInitPlaceholders;

    // A trace entry under a scope is either:
    // - A variable (including interface references)
    // - A sub scope (stored as the cell corresponding to the sub scope)
    // Note: members are non-const to allow copy during sorting
    વર્ગ TraceEntry final {
        AstVarScope* m_vscp = nullptr;  // AstVarScope under scope being traced
        AstCell* m_cellp = nullptr;  // Sub scope (as AstCell) under scope being traced
        std::string m_path;  // Path to enclosing module in original hierarchy
        std::string m_name;  // Name of signal/subscope

        શૂન્ય init(અચલ std::string& name, Ast_ગાંઠ* nodep, તાર્કિક in_ટોચ_Scope) {
            // Compute path in hierarchy and item name
            અચલ std::string& vcdName = Ast_ગાંઠ::vcdName(name);
            AstVar* અચલ varp = VN_CAST(nodep, Var);
            જો (VN_IS(nodep, Cell) || VN_IS(varp->dtypep(), IfaceRefDType)) {
                // Cell or interface reference
                m_path = vcdName + " ";
                m_name.clear();
            } નહીંતર જો (varp->isPrimaryIO()) {
                // Primary IO variable
                m_path = "$rootio ";
                m_name = vcdName;
            } નહીંતર {
                // Other Variable
                અચલ size_t pos = vcdName.rfind(' ');
                અચલ size_t pathLen = pos == std::string::npos ? 0 : pos + 1;
                m_path = vcdName.substr(0, pathLen);
                m_name = vcdName.substr(pathLen);
            }

            // When creating a --lib-create library, drop the name of the selected top module.
            // This will be replaced by the instance name in the model that uses the library, or
            // restored at runtime if the library itself is traced as the root model. Other top
            // level entities ($unit, packages, ...) keep a '$libroot' wrapper so they still have
            // a stable location in the dump.
            જો (in_ટોચ_Scope && !v3Global.opt.libCreate().empty()) {
                અચલ size_t start = m_path.find(' ');
                // Must have a prefix in the top scope with lib, as top wrapper signals not traced
                UASSERT_OBJ(start != std::string::npos, nodep, "No prefix with --lib-create");
                અચલ std::string prefix = m_path.substr(0, start);
                // Wrapper primary IOs stay under $rootio so a root-traced library can restore
                // them under the runtime C++ instance name without affecting embedded use.
                જો (prefix == "$rootio") પાછા;
                m_path = m_path.substr(start + 1);
                જો (v3Global.rootp()->trace_Lib_ટોચ_Name() != prefix) m_path = "$libroot " + m_path;
            }
        }

    જાહેર:
        explicit TraceEntry(અચલ AstScope* scopep, AstVarScope* vscp)
            : m_vscp{vscp} {
            init(vscp->varp()->name(), vscp->varp(), scopep->is_ટોચ());
        }
        explicit TraceEntry(અચલ AstScope* scopep, AstCell* cellp)
            : m_cellp{cellp} {
            init(cellp->name(), cellp, scopep->is_ટોચ());
        }
        પૂર્ણાંક operatorCompare(અચલ TraceEntry& b) અચલ {
            જો (અચલ પૂર્ણાંક cmp = path().compare(b.path())) પાછા cmp < 0;
            જો (અચલ પૂર્ણાંક cmp = fileline().operatorCompare(b.fileline())) પાછા cmp < 0;
            પાછા name() < b.name();
        }
        AstVarScope* vscp() અચલ { પાછા m_vscp; }
        AstCell* cellp() અચલ { પાછા m_cellp; }
        અચલ std::string& path() અચલ { પાછા m_path; }
        શૂન્ય path(અચલ std::string& path) { m_path = path; }
        અચલ std::string& name() અચલ { પાછા m_name; }
        File_લીટી& fileline() અચલ { પાછા m_vscp ? *m_vscp->fileline() : *m_cellp->fileline(); }
    };
    std::vector<TraceEntry> m_entries;  // Trace entries under current scope
    AstVarScope* m_traVscp = nullptr;  // Current AstVarScope we are constructing AstTraceDecls for
    Ast_ગાંઠ_પદાવલિ* m_traValuep = nullptr;  // Value expression for current signal
    string m_traName;  // Name component for current signal

    VDouble0 m_statSigs;  // Statistic tracking
    VDouble0 m_statIgnSigs;  // Statistic tracking

    // METHODS

    string vscIgnoreTrace(અચલ AstVarScope* nodep) {
        // Return true if this shouldn't be traced
        // See also similar rule in V3Coverage::varIgnoreToggle
        અચલ AstVar* અચલ varp = nodep->varp();
        જો (!varp->isTrace()) પાછા "Verilator trace_off";
        જો (!nodep->isTrace()) પાછા "Verilator instance trace_off";
        // Automatics (typically, excluding forks) have no persistance over
        // time, and may optimize differently when multithreadeded or hierarchical.
        // Class automatics refer to being in a class but might still be pointed
        // to by a static, so are ok.
        જો (varp->lifetime().isAutomatic() && !varp->isClassMember() && !varp->isParam())
            પાછા "Automatic variable";

        અચલ પૂર્ણાંક width = recurseDTypeWidth(nodep->varp()->dtypep());
        જો (v3Global.opt.traceMaxWidth() && width > v3Global.opt.traceMaxWidth())
            પાછા "Width " + cvtToStr(width) + " > --trace-max-width";

        અચલ string prettyName = nodep->prettyName();
        જો (!V3Control::getScopeTraceOn(prettyName)) પાછા "Vlt scope trace_off";
        જો (!v3Global.opt.traceUnderscore()) {
            જો (!prettyName.empty() && prettyName[0] == '_') પાછા "Leading underscore";
            જો (prettyName.find("._") != string::npos) પાછા "Inlined leading underscore";
        }
        પાછા ""s;
    }

    પૂર્ણાંક recurseDTypeWidth(અચલ Ast_ગાંઠ_DType* nodep) {
        જો (અચલ Ast_ગાંઠ_Array_DType* adtypep = VN_CAST(nodep, ગાંઠ_Array_DType))
            પાછા recurseDTypeWidth(adtypep->subDTypep()) * adtypep->declRange().elements();
        પાછા nodep->width();
    }

    AstCFunc* newCFunc(File_લીટી* flp, અચલ string& name) {
        AstScope* અચલ ટોચ_Scopep = m_ટોચ_Scopep->scopep();
        AstCFunc* અચલ funcp = નવું AstCFunc{flp, name, ટોચ_Scopep};
        funcp->argTypes(v3Global.opt.traceClassBase() + "* tracep");
        funcp->isTrace(સાચું);
        funcp->isStatic(ખોટું);
        funcp->isLoose(સાચું);
        funcp->slow(સાચું);
        ટોચ_Scopep->addBlocksp(funcp);
        પાછા funcp;
    }

    શૂન્ય add_To_ટોચ_Func(Ast_ગાંઠ_Stmt* stmtp) {
        જો (m_ટોચ_Func_Size > m_funcSizeLimit || m_ટોચ_Funcps.empty()) {
            m_ટોચ_Func_Size = 0;
            //
            અચલ string n = cvtToStr(m_ટોચ_Funcps.size());
            અચલ string name{"trace_init_top__" + n};
            AstCFunc* અચલ funcp = newCFunc(m_ટોચ_Scopep->fileline(), name);
            m_ટોચ_Funcps.ધકેલો_back(funcp);
        }
        m_ટોચ_Funcps.back()->addStmtsp(stmtp);
        m_ટોચ_Func_Size += stmtp->ગાંઠ_ગણતરી();
    }

    શૂન્ય addToSubFunc(Ast_ગાંઠ_Stmt* stmtp) {
        // TODO (maybe) -- sub funcs for dtype components
        જો (m_dtypeSubFuncp) {
            જો (m_subFuncSize > m_funcSizeLimit) newDeclSubFunc();
            m_dtypeSubFuncp->addStmtsp(stmtp);
            m_subFuncSize += stmtp->ગાંઠ_ગણતરી();
            પાછા;
        }
        // Defer trace splitting until V3Trace
        જો (m_subFuncps.empty() || m_forceNewLeaf) {
            m_forceNewLeaf = ખોટું;
            File_લીટી* અચલ flp = m_currScopep->fileline();
            અચલ string n = cvtToStr(m_subFuncps.size());
            અચલ string name
                = m_currScopep == m_ટોચ_Scopep->scopep() && !v3Global.opt.libCreate().empty()
                      ? (m_ટોચ_Scope_મૂળ_Phase ? "trace_init_leaf_root__" : "trace_init_leaf_top__")
                            + n
                      : "trace_init_sub__" + m_currScopep->nameDotless() + "__" + n;
            AstCFunc* અચલ funcp = newCFunc(flp, name);
            funcp->addStmtsp(નવું AstCStmt{flp, "const int c = vlSymsp->__Vm_baseCode;"});
            m_subFuncps.ધકેલો_back(funcp);
        }
        m_subFuncps.back()->addStmtsp(stmtp);
        m_subFuncSize += stmtp->ગાંઠ_ગણતરી();
    }

    AstTraceDecl* addTraceDecl(અચલ VNumRange& arrayRange,
                               પૂર્ણાંક widthOverride,  // If !=0, is packed struct/array where basicp
                                                   // size misreflects one element
                               AstCCall* અચલ dtypeCallp = nullptr) {
        VNumRange bitRange;
        જો (widthOverride) {
            bitRange = VNumRange{widthOverride - 1, 0};
        } નહીંતર જો (અચલ AstBasicDType* અચલ bdtypep = m_traValuep->dtypep()->basicp()) {
            bitRange = bdtypep->nrange();
        }
        File_લીટી* અચલ flp = m_traVscp->fileline();
        Ast_ગાંઠ_પદાવલિ* valuep = m_traValuep->clone_વૃક્ષ(ખોટું);
        અચલ તાર્કિક validOffset = m_offset != std::numeric_limits<uint32_t>::max();
        AstTraceDecl* અચલ newp
            = નવું AstTraceDecl{flp,      m_traName,  m_traVscp->varp(), valuep,
                               bitRange, arrayRange, dtypeCallp,        validOffset};
        જો (validOffset) {
            newp->code(m_offset);
            જો (!dtypeCallp) { m_offset += newp->codeInc(); }
            valuep->foreach([&](AstVarRef* અચલ refp) {
                UASSERT_OBJ(refp->varScopep() == m_traVscp, refp,
                            "Trace decl expression references unexpected var");
                refp->replaceWith(નવું AstCExpr{flp, "__VdtypeVar", m_traVscp->width()});
                VL_DO_DANGLING(refp->delete_વૃક્ષ(), refp);
            });
        } નહીંતર {
            newp->dtypeDeclp(m_dtypeDeclp);
        }
        m_declUncalledps.emplace(newp);
        addToSubFunc(newp);

        પાછા newp;
    }

    શૂન્ય addIgnore(અચલ string& why) {
        ++m_statIgnSigs;
        અચલ std::string cmt = "Tracing: "s + m_traName + " // Ignored: " + why;
        જો (debug() > 3 && m_traVscp) std::cout << "- " << m_traVscp->fileline() << cmt << '\n';
    }

    શૂન્ય fixupPlaceholder(અચલ std::string& path, Ast_ગાંઠ_Stmt* placeholderp) {
        // Find the scope for the path. As we are working based on cell names,
        // it is possible there is no corresponding scope (e.g.: for an empty
        // module).
        અચલ auto it = m_pathToScopep.find(Ast_ગાંઠ::prettyName(path));
        જો (it != m_pathToScopep.end()) {
            અચલ AstScope* અચલ scopep = it->second;
            File_લીટી* અચલ flp = placeholderp->fileline();

            // Call the initialization functions for the scope
            Ast_ગાંઠ* stmtp = nullptr;
            માટે (AstCFunc* અચલ subFuncp : m_scopeInitFuncps.at(scopep)) {
                AstCCall* અચલ callp = નવું AstCCall{flp, subFuncp};
                callp->dtypeSetVoid();
                callp->argTypes("tracep");
                stmtp = Ast_ગાંઠ::addNext(stmtp, callp->makeStmt());
            }

            // Add after the placeholder
            જો (stmtp) placeholderp->addNextHere(stmtp);
        }
        // Delete the placeholder
        placeholderp->unlinkFrBack();
        VL_DO_DANGLING(placeholderp->delete_વૃક્ષ(), placeholderp);
    }

    શૂન્ય fixupLibStub(અચલ std::string& path, Ast_ગાંઠ_Stmt* placeholderp) {
        File_લીટી* અચલ flp = placeholderp->fileline();

        // The library instance's model is named after its %m within the parent model,
        // which does not include the "l2-name" (top module) of the library
        std::string name = Ast_ગાંઠ::prettyName(path);
        જો (!v3Global.opt.libCreate().empty()) {
            અચલ std::string ટોચ_Prefix = v3Global.rootp()->trace_Lib_ટોચ_Name() + ".";
            જો (VString::startsWith(name, ટોચ_Prefix)) name = name.substr(ટોચ_Prefix.size());
        }

        // Call the initialization function for the library instance
        AstCStmt* અચલ initp = નવું AstCStmt{flp};
        initp->add("{\n");
        initp->add("std::string __VlibName = vlSymsp->name();\n");
        initp->add("if (!__VlibName.empty()) __VlibName += '.';\n");
        initp->add("__VlibName += ");
        initp->add(નવું AstConst{flp, AstConst::String{}, name});
        initp->add(";\n");
        initp->add("tracep->initLib(__VlibName);\n");
        initp->add("}\n");

        placeholderp->addNextHere(initp);
        // Delete the placeholder
        VL_DO_DANGLING(placeholderp->unlinkFrBack()->delete_વૃક્ષ(), placeholderp);
    }

    શૂન્ય fixupPlaceholders() {
        // Fix up cell initialization placehodlers
        UINFO(9, "fixupPlaceholders()");
        માટે (અચલ auto& item : m_cellInitPlaceholders) {
            અચલ AstScope* અચલ parentp = std::get<0>(item);
            અચલ AstCell* અચલ cellp = std::get<1>(item);
            Ast_ગાંઠ_Stmt* અચલ placeholderp = std::get<2>(item);
            અચલ std::string path = parentp->name() + "__DOT__" + cellp->name();
            જો (cellp->modp()->verilatorLib()) {
                fixupLibStub(path, placeholderp);
            } નહીંતર {
                fixupPlaceholder(path, placeholderp);
            }
        }

        // Fix up interface reference initialization placeholders
        માટે (અચલ auto& item : m_ifaceRefInitPlaceholders) {
            અચલ AstVarScope* અચલ vscp = std::get<0>(item);
            Ast_ગાંઠ_Stmt* અચલ placeholderp = std::get<1>(item);
            અચલ std::string path = vscp->scopep()->name() + "__DOT__" + vscp->varp()->name();
            fixupPlaceholder(path, placeholderp);
        }
    }

    શૂન્ય remove_Redundant_Prefix_ધકેલો_કાઢો() {
        માટે (અચલ auto& pair : m_scopeInitFuncps) {
            // cppcheck-suppress constVariablePointer
            માટે (AstCFunc* અચલ funcp : pair.second) {
                Ast_ગાંઠ* prevp = nullptr;
                Ast_ગાંઠ* currp = funcp->stmtsp();
                જ્યારે (currp) {
                    Ast_ગાંઠ* અચલ nextp = currp->nextp();
                    જો (VN_IS(prevp, Trace_ધકેલો_Prefix) && VN_IS(currp, Trace_કાઢો_Prefix)) {
                        VL_DO_DANGLING(prevp->unlinkFrBack()->delete_વૃક્ષ(), prevp);
                        VL_DO_DANGLING(currp->unlinkFrBack()->delete_વૃક્ષ(), currp);
                    }
                    જો (!nextp) તોડો;
                    prevp = nextp->backp();
                    currp = nextp;
                }
            }
        }
    }

    શૂન્ય checkCalls(અચલ AstCFunc* funcp) {
        જો (!v3Global.opt.debugCheck()) પાછા;
        checkCallsRecurse(funcp);
        જો (!m_declUncalledps.empty()) {  // LCOV_EXCL_START
            માટે (auto tracep : m_declUncalledps) UINFO(0, "-nodep " << tracep);
            (*(m_declUncalledps.begin()))->v3fatalSrc("Created TraceDecl which is never called");
        }  // LCOV_EXCL_STOP
    }
    શૂન્ય checkCallsRecurse(અચલ AstCFunc* funcp) {
        funcp->foreach([આ](અચલ Ast_ગાંઠ* nodep) {
            જો (અચલ AstTraceDecl* અચલ declp = VN_CAST(nodep, TraceDecl)) {
                m_declUncalledps.erase(declp);
            } નહીંતર જો (અચલ AstCCall* અચલ ccallp = VN_CAST(nodep, CCall)) {
                checkCallsRecurse(ccallp->funcp());
            }
        });
    }

    તાર્કિક isBasicIO() { પાછા m_traVscp->varp()->isVLIO(); }
    શૂન્ય newDeclSubFunc() {
        File_લીટી* અચલ flp = m_dtypeFuncp->fileline();
        AstCFunc* અચલ subFuncp = newCFunc(flp, m_dtypeNames.get("trace_init_dtype_sub__"));
        subFuncp->argTypes(subFuncp->argTypes() + m_dtypeArgs);
        AstCCall* અચલ subCallp = નવું AstCCall{flp, subFuncp};
        subCallp->dtypeSetVoid();
        subCallp->argTypes("tracep, name, fidx, c, direction");
        m_dtypeFuncp->addStmtsp(subCallp->makeStmt());
        m_dtypeSubFuncp = subFuncp;
        m_subFuncSize = 0;
    }
    શૂન્ય newDeclFunc(Ast_ગાંઠ_DType* nodep) {
        Ast_ગાંઠ_DType* અચલ skipTypep = nodep->skipRefp();
        // offset and direction args added in EmitCImp
        std::string callArgs{"tracep, \"" + VIdProtect::protect(m_traName) + "\""};
        VL_RESTORER_COPY(m_traName);
        File_લીટી* અચલ flp = skipTypep->fileline();

        અચલ Dtype_Func_ચાવી dtype_ચાવી{skipTypep, m_traVscp->varp()->varType()};
        auto pair = m_dtypeFuncs.emplace(dtype_ચાવી, nullptr);
        AstCFunc** funcpp = &pair.first->second;
        જો (pair.second) {
            *funcpp = newCFunc(flp, m_dtypeNames.get("trace_init_dtype__"));
            (*funcpp)->argTypes((*funcpp)->argTypes() + m_dtypeArgs);
            (*funcpp)->user2p(m_traVscp);
        }

        AstCCall* અચલ callp = નવું AstCCall{flp, *funcpp};
        callp->dtypeSetVoid();
        callp->argTypes(callArgs);
        m_dtypeDeclp = addTraceDecl(VNumRange{}, skipTypep->width(), callp);
        addToSubFunc(callp->makeStmt());

        જો (pair.second) {
            VL_RESTORER(m_offset);
            m_offset = 0;

            VL_RESTORER(m_dtypeFuncp);
            VL_RESTORER(m_dtypeSubFuncp);
            VL_RESTORER(m_subFuncSize);
            m_dtypeFuncp = *funcpp;
            newDeclSubFunc();
            જો (AstStructDType* અચલ dtypep = VN_CAST(skipTypep, StructDType)) {
                declStruct(dtypep, સાચું);
            } નહીંતર જો (AstUnpackArrayDType* અચલ dtypep = VN_CAST(skipTypep, UnpackArrayDType)) {
                declUnpackedArray(dtypep, સાચું);
            } નહીંતર જો (AstPackArrayDType* અચલ dtypep = VN_CAST(skipTypep, PackArrayDType)) {
                declPackedArray(dtypep, સાચું);
            } નહીંતર {
                UASSERT_OBJ(ખોટું, skipTypep, "Creating a trace function for an unexpected type");
            }
            m_dtypeFuncp->user1(m_offset);
        }

        m_dtypeDeclp->codeInc((*funcpp)->user1());
        m_offset += m_dtypeDeclp->codeInc();
    }
    શૂન્ય declUnpackedArray(AstUnpackArrayDType* અચલ nodep, તાર્કિક newFunc) {
        string prefixName(newFunc ? "name" : m_traName);

        VL_RESTORER_COPY(m_traName);
        File_લીટી* અચલ flp = nodep->fileline();

        addToSubFunc(નવું Ast_Trace_ધકેલો_Prefix{flp, prefixName, VTracePrefixType::ARRAY_UNPACKED,
                                            nodep->ડાબું(), nodep->જમણું(), !newFunc});

        જો (VN_IS(nodep->subDTypep()->skipRefToEnump(),
                  BasicDType)  // Nothing lower than this array
            && m_traVscp->dtypep()->skipRefToEnump() == nodep) {  // Nothing above this array
            // Simple 1-D array, use existing V3EmitC runtime loop rather than unrolling
            // This will put "(index)" at end of signal name for us
            જો (m_traVscp->dtypep()->skipRefToEnump()->isString()) {
                addIgnore("Unsupported: strings");
            } નહીંતર {
                m_traName = "";
                addTraceDecl(nodep->declRange(), 0);
            }
        } નહીંતર {
            Ast_ગાંઠ_DType* અચલ subtypep = nodep->subDTypep()->skipRefToEnump();
            // Always iterate left index to right index
            માટે (અચલ પૂર્ણાંક i : nodep->declRange().seq_ડાબું_To_જમણું()) {
                VL_RESTORER(m_traValuep);
                m_traName = '[' + std::to_string(i) + ']';
                m_traValuep = m_traValuep->clone_વૃક્ષ(ખોટું);
                m_traValuep = નવું AstArraySel{flp, m_traValuep, i - nodep->lo()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_વૃક્ષ(), m_traValuep);
            }
        }

        addToSubFunc(નવું Ast_Trace_કાઢો_Prefix{flp});
    }
    શૂન્ય declPackedArray(AstPackArrayDType* અચલ nodep, તાર્કિક newFunc) {
        string prefixName(newFunc ? "name" : m_traName);
        Ast_ગાંઠ_DType* અચલ subtypep = nodep->subDTypep()->skipRefToEnump();

        VL_RESTORER_COPY(m_traName);
        File_લીટી* અચલ flp = nodep->fileline();

        addToSubFunc(નવું Ast_Trace_ધકેલો_Prefix{flp, prefixName, VTracePrefixType::ARRAY_PACKED,
                                            nodep->ડાબું(), nodep->જમણું(), !newFunc});

        // Always iterate left index to right index
        માટે (અચલ પૂર્ણાંક i : nodep->declRange().seq_ડાબું_To_જમણું()) {
            VL_RESTORER(m_traValuep);
            m_traName = '[' + std::to_string(i) + ']';
            અચલ પૂર્ણાંક lsb = (i - nodep->lo()) * subtypep->width();
            m_traValuep = m_traValuep->clone_વૃક્ષ(ખોટું);
            m_traValuep = નવું AstSel{flp, m_traValuep, lsb, subtypep->width()};
            m_traValuep->dtypep(subtypep);
            iterate(subtypep);
            VL_DO_CLEAR(m_traValuep->delete_વૃક્ષ(), m_traValuep = nullptr);
        }

        addToSubFunc(નવું Ast_Trace_કાઢો_Prefix{flp});
    }
    શૂન્ય declStruct(AstStructDType* અચલ nodep, તાર્કિક newFunc) {
        File_લીટી* અચલ flp = nodep->fileline();
        string prefixName(newFunc ? "name" : m_traName);
        પૂર્ણાંક nMembers = 0;
        માટે (Ast_ગાંઠ* mp = nodep->membersp(); mp; mp = mp->nextp()) ++nMembers;
        જો (!nodep->packed()) {
            addToSubFunc(નવું Ast_Trace_ધકેલો_Prefix{flp, prefixName,  //
                                                VTracePrefixType::STRUCT_UNPACKED, nMembers, 0,
                                                !newFunc});
            માટે (અચલ AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_ગાંઠ_DType* અચલ subtypep = itemp->subDTypep()->skipRefToEnump();
                m_traName = itemp->prettyName();
                VL_RESTORER(m_traValuep);
                m_traValuep = m_traValuep->clone_વૃક્ષ(ખોટું);
                m_traValuep = નવું AstStructSel{flp, m_traValuep, itemp->name()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_વૃક્ષ(), m_traValuep);
            }
            addToSubFunc(નવું Ast_Trace_કાઢો_Prefix{flp});
        } નહીંતર {
            addToSubFunc(નવું Ast_Trace_ધકેલો_Prefix{flp, prefixName,  //
                                                VTracePrefixType::STRUCT_PACKED, nMembers, 0,
                                                !newFunc});
            માટે (અચલ AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_ગાંઠ_DType* અચલ subtypep = itemp->subDTypep()->skipRefToEnump();
                m_traName = itemp->prettyName();
                VL_RESTORER(m_traValuep);
                m_traValuep = m_traValuep->clone_વૃક્ષ(ખોટું);
                m_traValuep = નવું AstSel{flp, m_traValuep, itemp->lsb(), subtypep->width()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_વૃક્ષ(), m_traValuep);
            }
            addToSubFunc(નવું Ast_Trace_કાઢો_Prefix{flp});
        }
    }

    // VISITORS
    શૂન્ય visit(AstScope* nodep) override {
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(!m_currScopep, nodep, "Should not nest");
        UASSERT_OBJ(m_subFuncps.empty(), nodep, "Should not nest");
        UASSERT_OBJ(m_entries.empty(), nodep, "Should not nest");
        UASSERT_OBJ(!m_traVscp, nodep, "Should not nest");
        UASSERT_OBJ(!m_traValuep, nodep, "Should not nest");
        UASSERT_OBJ(m_traName.empty(), nodep, "Should not nest");

        // If this is a stub for a --lib-create library, skip.
        જો (nodep->modp()->verilatorLib()) પાછા;

        VL_RESTORER(m_currScopep);
        m_currScopep = nodep;

        // Gather signals under this scope
        iterateChildrenConst(nodep);

        // Gather cells under this scope
        માટે (Ast_ગાંઠ* stmtp = nodep->modp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            જો (AstCell* અચલ cellp = VN_CAST(stmtp, Cell)) m_entries.emplace_back(nodep, cellp);
        }

        જો (!m_entries.empty()) {
            // Sort trace entries, by enclosing instance (necessary for single traversal of
            // hierarchy during initialization), then by source location, then by name.
            std::stable_sort(
                m_entries.begin(), m_entries.end(),
                [](અચલ TraceEntry& a, અચલ TraceEntry& b) { પાછા a.operatorCompare(b); });

            File_લીટી* અચલ flp = nodep->fileline();
            PathAdjustor pathAdjustor{flp, [&](Ast_ગાંઠ_Stmt* stmtp) { addToSubFunc(stmtp); }};
            અચલ તાર્કિક split_મૂળ_Primary_Ios = nodep->is_ટોચ() && !v3Global.opt.libCreate().empty();
            અચલ auto emitEntry = [&](અચલ TraceEntry& entry) {
                AstVarScope* અચલ vscp = entry.vscp();
                // Adjust name prefix based on path in hierarchy
                UINFO(9, "path='" << entry.path() << "' name='" << entry.name() << "' "
                                  << (entry.cellp() ? static_cast<Ast_ગાંઠ*>(entry.cellp())
                                                    : static_cast<Ast_ગાંઠ*>(entry.vscp())));
                pathAdjustor.adjust(entry.path(), entry.cellp(), entry.vscp());

                m_traName = entry.name();

                જો (vscp) {
                    // This is a signal: build AstTraceDecl for it
                    m_traVscp = vscp;
                    અચલ string& ignoreReason = vscIgnoreTrace(m_traVscp);
                    જો (!ignoreReason.empty()) {
                        addIgnore(ignoreReason);
                    } નહીંતર {
                        ++m_statSigs;
                        // Create reference to whole signal. We will operate on this during the
                        // traversal.
                        m_traValuep
                            = નવું AstVarRef{m_traVscp->fileline(), m_traVscp, VAccess::READ};
                        // Recurse into data type of the signal. The visit methods will add
                        // AstTraceDecls.
                        iterate(m_traVscp->varp()->dtypep()->skipRefToEnump());
                        // Delete reference created above. Traversal cloned it as required.
                        જો (m_traValuep) {
                            VL_DO_DANGLING(m_traValuep->delete_વૃક્ષ(), m_traValuep);
                            // Note: Sometimes VL_DANGLING is a no-op, but we have assertions
                            // on m_traValuep being nullptr, so make sure it is.
                            m_traValuep = nullptr;
                        }
                    }
                } નહીંતર {
                    // This is a subscope: insert a placeholder to be fixed up later
                    AstCell* અચલ cellp = entry.cellp();
                    Ast_ગાંઠ_Stmt* અચલ stmtp = નવું AstComment{
                        cellp->fileline(), "Instance init for: " + cellp->prettyName()};
                    addToSubFunc(stmtp);
                    m_cellInitPlaceholders.emplace_back(nodep, cellp, stmtp);
                }
            };
            જો (split_મૂળ_Primary_Ios) {
                m_ટોચ_Scope_મૂળ_Phase = સાચું;
                માટે (અચલ TraceEntry& entry : m_entries) {
                    AstVarScope* અચલ vscp = entry.vscp();
                    જો (!(vscp && vscp->varp()->isPrimaryIO())) ચાલુ;
                    emitEntry(entry);
                }
                m_ટોચ_Scope_મૂળ_Phase = ખોટું;
                pathAdjustor.unwind();
                m_ટોચ_Scope_મૂળ_Func_ગણતરી = m_subFuncps.size();
                જો (m_ટોચ_Scope_મૂળ_Func_ગણતરી) {
                    // Force the next addToSubFunc to start a fresh leaf so
                    // root-phase decls and top-phase decls live in separate
                    // functions. Without this, both phases land in the same
                    // leaf and `trace_init_top` ends up emitting both the
                    // $rootio-prefixed IOs and the regular-scope decls, so
                    // each IO gets traced twice when the lib is consumed
                    // via initLib(name).
                    m_subFuncSize = 0;
                    m_forceNewLeaf = સાચું;
                }
            }
            માટે (અચલ TraceEntry& entry : m_entries) {
                AstVarScope* અચલ vscp = entry.vscp();
                જો (split_મૂળ_Primary_Ios && vscp && vscp->varp()->isPrimaryIO()) ચાલુ;
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
        અચલ std::string path = nodep->prettyName();
        m_pathToScopep.emplace(path, nodep);

        // Save the hierarchical names of interface references that reference this scope
        અચલ AstCell* અચલ cellp = nodep->aboveCellp();
        જો (cellp
            && VN_IS(cellp->modp(), Iface)
            // Exclude classes that were inside interfaces, the cell's modp is an interface,
            // but the nodep->module is the Class
            && VN_IS(nodep->modp(), Iface)) {
            અચલ size_t lastDot = path.find_last_of('.');
            UASSERT_OBJ(lastDot != string::npos, nodep,
                        "Expected an interface scope name to have at least one dot: " << path);
            અચલ std::string parentPath = path.substr(0, lastDot + 1);

            માટે (AstIntfRef *intfRefp = cellp->intfRefsp(), *nextp; intfRefp; intfRefp = nextp) {
                nextp = VN_AS(intfRefp->nextp(), IntfRef);

                અચલ std::string refName = intfRefp->prettyName();

                // Assume only references under the same parent scope reference
                // the same interface.
                // TODO: This is not actually correct. An interface can propagate
                //       upwards and sideways when passed to a port via a downward
                //       hierarchical reference, which we will miss here.
                જો (!VString::startsWith(refName, parentPath)) ચાલુ;

                // Save the mapping from the path of the reference to the scope
                m_pathToScopep.emplace(refName, nodep);

                // No more need for AstIntfRef, unless V3EmitCSyms wants it for VPI
                જો (!v3Global.opt.vpi()) {
                    VL_DO_DANGLING(intfRefp->unlinkFrBack()->delete_વૃક્ષ(), intfRefp);
                }
            }
        }
    }
    શૂન્ય visit(AstVarScope* nodep) override {
        UASSERT_OBJ(m_currScopep, nodep, "AstVarScope not under AstScope");

        // Prefilter - things that get added to m_vscps will either get traced or get a comment as
        // to why they are not traced. Generally these conditions doesn't need updating, instead
        // use varp->isTrace() and/or vscIgnoreTrace.
        જો (nodep->varp()->isTemp() && !nodep->varp()->isTrace()) પાછા;
        જો (nodep->varp()->isClassMember()) પાછા;
        જો (nodep->varp()->isFuncLocal()) પાછા;

        // When creating a --lib-create library ...
        જો (!v3Global.opt.libCreate().empty()) {
            // Ignore parameters in packages. These will be traced at the top level.
            જો (nodep->varp()->isParam() && VN_IS(nodep->scopep()->modp(), Package)) પાછા;
        }

        // Add to traced signal list
        m_entries.emplace_back(m_currScopep, nodep);
    }

    // VISITORS - Data types when tracing
    શૂન્ય visit(AstConstDType* nodep) override {
        જો (!m_traVscp) પાછા;
        VL_RESTORER(m_offset);
        VL_RESTORER(m_skipDtypeFunc);
        m_skipDtypeFunc = સાચું;
        iterate(nodep->subDTypep()->skipRefToEnump());
    }
    શૂન્ય visit(AstRefDType* nodep) override {
        જો (!m_traVscp) પાછા;
        VL_RESTORER(m_offset);
        iterate(nodep->subDTypep()->skipRefToEnump());
    }
    શૂન્ય visit(AstIfaceRefDType* /*nodep*/) override {
        જો (!m_traVscp) પાછા;
        // Insert a placeholder to be fixed up later
        File_લીટી* અચલ flp = m_traVscp->fileline();
        Ast_ગાંઠ_Stmt* અચલ stmtp
            = નવું AstComment{flp, "Interface ref init for: " + m_traVscp->prettyName()};
        addToSubFunc(stmtp);
        m_ifaceRefInitPlaceholders.emplace_back(m_traVscp, stmtp);
    }
    શૂન્ય visit(AstUnpackArrayDType* nodep) override {
        // Note more specific dtypes above
        જો (!m_traVscp) પાછા;

        જો (v3Global.opt.traceMaxArray()
            && static_cast<પૂર્ણાંક>(nodep->arrayUnpackedElements()) > v3Global.opt.traceMaxArray()) {
            addIgnore("Wide memory > --trace-max-array ents");
            પાછા;
        }

        // Skip unpacked arrays of non-traceable leaf types (e.g. strings)
        જો (VN_IS(nodep->subDTypep()->skipRefToEnump(), BasicDType)
            && nodep->subDTypep()->skipRefToEnump()->isString()) {
            addIgnore("Unsupported: strings");
            પાછા;
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        જો (isBasicIO()) m_skipDtypeFunc = સાચું;

        જો (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_offset);
            newDeclFunc(nodep);
        }
        declUnpackedArray(nodep, ખોટું);
    }
    શૂન્ય visit(AstPackArrayDType* nodep) override {
        જો (!m_traVscp) પાછા;

        જો (!v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit.
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            addTraceDecl(VNumRange{}, nodep->width());
            પાછા;
        }

        Ast_ગાંઠ_DType* અચલ subtypep = nodep->subDTypep()->skipRefToEnump();

        // Do not unroll if the elements are simple 'bit', or 'logic'
        જો (AstBasicDType* અચલ basicp = VN_CAST(subtypep->skipRefp(), BasicDType)) {
            જો (basicp->isBitLogic() && !basicp->isRanged()) {
                addTraceDecl(VNumRange{}, nodep->width());
                પાછા;
            }
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        જો (isBasicIO()) m_skipDtypeFunc = સાચું;

        જો (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_offset);
            newDeclFunc(nodep);
        }
        declPackedArray(nodep, ખોટું);
    }
    શૂન્ય visit(AstStructDType* nodep) override {
        જો (!m_traVscp) પાછા;

        જો (nodep->packed() && !v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            addTraceDecl(VNumRange{}, nodep->width());
            પાછા;
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        જો (isBasicIO()) m_skipDtypeFunc = સાચું;

        // Only create sub functions for top-level structs, i.e. don't have struct funcs
        // call other struct funcs for child types.  This could easily be done for decl funcs
        // but full / chg funcs would require copying / aligning data for child types or more
        // complicated / wonky / generalized data access.
        જો (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_offset);
            newDeclFunc(nodep);
        }
        declStruct(nodep, ખોટું);
    }
    શૂન્ય visit(AstUnionDType* nodep) override {
        જો (!m_traVscp) પાછા;

        VL_RESTORER(m_skipDtypeFunc);
        m_skipDtypeFunc = સાચું;

        જો (nodep->packed() && !v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            addTraceDecl(VNumRange{}, nodep->width());
            પાછા;
        }

        VL_RESTORER_COPY(m_traName);
        File_લીટી* અચલ flp = nodep->fileline();

        પૂર્ણાંક nMembers = 0;
        માટે (Ast_ગાંઠ* mp = nodep->membersp(); mp; mp = mp->nextp()) ++nMembers;

        જો (!nodep->packed()) {
            addIgnore("Unsupported: Unpacked union");
        } નહીંતર {
            addToSubFunc(નવું Ast_Trace_ધકેલો_Prefix{flp, m_traName,  //
                                                VTracePrefixType::UNION_PACKED, nMembers});
            માટે (અચલ AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_ગાંઠ_DType* અચલ subtypep = itemp->subDTypep()->skipRefToEnump();
                m_traName = itemp->prettyName();
                iterate(subtypep);
            }
            addToSubFunc(નવું Ast_Trace_કાઢો_Prefix{flp});
        }
    }
    શૂન્ય visit(AstBasicDType* nodep) override {
        જો (!m_traVscp) પાછા;
        જો (nodep->isString()) {
            addIgnore("Unsupported: strings");
        } નહીંતર {
            addTraceDecl(VNumRange{}, 0);
        }
    }
    શૂન્ય visit(AstEnumDType* nodep) override { iterate(nodep->skipRefp()); }
    શૂન્ય visit(Ast_ગાંઠ_DType*) override {
        // Note more specific dtypes above
        જો (!m_traVscp) પાછા;
        addIgnore("Unsupported: data type");
    }

    //--------------------
    શૂન્ય visit(Ast_ગાંઠ* nodep) override { iterateChildren(nodep); }

જાહેર:
    // CONSTRUCTORS
    explicit TraceDeclVisitor(AstNetlist* nodep)
        : m_ટોચ_Scopep{nodep->ટોચ_Scopep()} {
        File_લીટી* અચલ flp = nodep->fileline();

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
        remove_Redundant_Prefix_ધકેલો_કાઢો();

        અચલ std::vector<AstCFunc*>& ટોચ_Scope_Funcps = m_scopeInitFuncps.at(m_ટોચ_Scopep->scopep());
        AstCFunc* મૂળ_Funcp = nullptr;
        જો (!v3Global.opt.libCreate().empty()) {
            મૂળ_Funcp = newCFunc(flp, "trace_init_root");
            મૂળ_Funcp->entryPoint(સાચું);
            માટે (size_t i = 0; i < m_ટોચ_Scope_મૂળ_Func_ગણતરી; ++i) {
                AstCCall* અચલ callp = નવું AstCCall{flp, ટોચ_Scope_Funcps.at(i)};
                callp->dtypeSetVoid();
                callp->argTypes("tracep");
                મૂળ_Funcp->addStmtsp(callp->makeStmt());
            }
            જો (!m_ટોચ_Scope_મૂળ_Func_ગણતરી) મૂળ_Funcp->addStmtsp(નવું AstComment{flp, "Empty"});
        }

        // Call the non-wrapper initialization functions of the root scope from the top function
        માટે (size_t i = m_ટોચ_Scope_મૂળ_Func_ગણતરી; i < ટોચ_Scope_Funcps.size(); ++i) {
            AstCFunc* અચલ funcp = ટોચ_Scope_Funcps.at(i);
            AstCCall* અચલ callp = નવું AstCCall{flp, funcp};
            callp->dtypeSetVoid();
            callp->argTypes("tracep");
            add_To_ટોચ_Func(callp->makeStmt());
        }

        // Ensure a top function exists, in case there was nothing to trace at all
        જો (m_ટોચ_Funcps.empty()) add_To_ટોચ_Func(નવું AstComment{flp, "Empty"});

        // Create single top level function, if more than one exists
        જો (m_ટોચ_Funcps.size() > 1) {
            AstCFunc* અચલ ટોચ_Funcp = newCFunc(flp, "");
            માટે (AstCFunc* funcp : m_ટોચ_Funcps) {
                AstCCall* અચલ callp = નવું AstCCall{flp, funcp};
                callp->dtypeSetVoid();
                callp->argTypes("tracep");
                ટોચ_Funcp->addStmtsp(callp->makeStmt());
            }
            m_ટોચ_Funcps.clear();
            m_ટોચ_Funcps.ધકેલો_back(ટોચ_Funcp);
        }

        // Set name of top level function
        AstCFunc* અચલ ટોચ_Funcp = m_ટોચ_Funcps.front();
        ટોચ_Funcp->name("trace_init_top");
        ટોચ_Funcp->entryPoint(સાચું);

        જો (મૂળ_Funcp && v3Global.opt.debugCheck()) checkCallsRecurse(મૂળ_Funcp);
        checkCalls(ટોચ_Funcp);
    }
    ~TraceDeclVisitor() override {
        V3Stats::addStat("Tracing, Traced signals", m_statSigs);
        V3Stats::addStat("Tracing, Ignored signals", m_statIgnSigs);
    }
};

//######################################################################
// Trace class functions

શૂન્ય V3TraceDecl::traceDeclAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TraceDeclVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_વૃક્ષ("tracedecl", 0, dump_વૃક્ષ_Either_Level() >= 3);
}
