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

#अन्तर्भाव "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#अन्तर्भाव "V3TraceDecl.h"

#अन्तर्भाव "V3Ast.h"
#अन्तर्भाव "V3Control.h"
#अन्तर्भाव "V3EmitCBase.h"
#अन्तर्भाव "V3Error.h"
#अन्तर्भाव "V3File.h"
#अन्तर्भाव "V3Global.h"
#अन्तर्भाव "V3Number.h"
#अन्तर्भाव "V3Stats.h"
#अन्तर्भाव "V3UniqueNames.h"

#अन्तर्भाव <cstdint>
#अन्तर्भाव <functional>
#अन्तर्भाव <limits>
#अन्तर्भाव <tuple>
#अन्तर्भाव <unordered_map>
#अन्तर्भाव <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Utility class to emit path adjustments

वर्ग PathAdjustor final {
    File_पङ्क्ति* अचर m_flp;  // FileLine used for created nodes
    std::function<शून्य(Ast_गाँठो_Stmt*)> m_emit;  // Function called with adjustment statements
    std::vector<std::string> m_थाक{""};  // Stack of current paths

    स्थिर constexpr वर्ण SEPARATOR = ' ';

सार्वजनिक:
    explicit PathAdjustor(File_पङ्क्ति* flp, std::function<शून्य(Ast_गाँठो_Stmt*)> emit)
        : m_flp{flp}
        , m_emit{emit} {}

    // Emit Prefix adjustments until the current path is 'newPath'
    शून्य adjust(अचर string& newPath, AstCell* cellp, AstVarScope* vscp) {
        // Move up to enclosing path
        जबसम्म (!VString::startsWith(newPath, m_थाक.back())) {
            m_emit(नूतन Ast_Trace_झिक_Prefix{m_flp});
            m_थाक.झिक_back();
        }

        यदि (newPath == m_थाक.back()) फर्काऊ;

        अचर VTracePrefixType lastScopeType =  //
            (cellp && VN_IS(cellp->modp(), Iface))
                    || (vscp && VN_IS(vscp->dtypep(), IfaceRefDType))
                ? VTracePrefixType::SCOPE_INTERFACE
                : VTracePrefixType::SCOPE_MODULE;
        अचर std::string extraPrefix = newPath.substr(m_थाक.back().size());
        माप_प्रकार begin = 0;
        अचर माप_प्रकार last = extraPrefix.rfind(SEPARATOR);
        // Move down, one path element at a time
        जबसम्म (सत्य) {
            अचर माप_प्रकार end = extraPrefix.find(SEPARATOR, begin);
            यदि (end == string::npos) रोक;
            अचर string& extra = extraPrefix.substr(begin, end - begin);
            यदि (end == last) {
                m_emit(नूतन Ast_Trace_थप_Prefix{m_flp, extra, lastScopeType});
            } अन्यथा {
                m_emit(नूतन Ast_Trace_थप_Prefix{m_flp, extra, VTracePrefixType::SCOPE_MODULE});
            }
            m_थाक.थप_back(m_थाक.back() + extra + SEPARATOR);
            begin = end + 1;
        }
        UASSERT(begin == extraPrefix.size(), "Should have consumed all of extraPrefix");
    }

    // Emit Prefix adjustments to unwind the path back to its original state
    शून्य unwind() {
        जबसम्म (m_थाक.size() > 1) {
            m_emit(नूतन Ast_Trace_झिक_Prefix{m_flp});
            m_थाक.झिक_back();
        }
    }
};

//######################################################################
// TraceDecl state, as a visitor of each AstNode

वर्ग TraceDeclVisitor final : सार्वजनिक VNVisitor {
    // NODE STATE
    // AstCFunc::user1()                // uint32_t.  code offset for current type
    // AstCFunc::user2()                // VarScope* for dtype functions

    // STATE
    Ast_शीर्ष_Scope* अचर m_शीर्ष_Scopep;  // The singleton AstTopScope
    अचर AstScope* m_currScopep = nullptr;  // Current scope being visited

    std::vector<AstCFunc*> m_शीर्ष_Funcps;  // Top level trace initialization functions
    std::vector<AstCFunc*> m_subFuncps;  // Trace sub functions for this scope
    std::set<अचर AstTraceDecl*> m_declUncalledps;  // Declarations not called
    // Functions per type and variable kind (wire vs logic etc.)
    संरचना Dtype_Func_साँचो final {
        अचर Ast_गाँठो_DType* dtypep;
        VVarType varType;
        तार्किक चालक==(अचर Dtype_Func_साँचो& other) अचर {
            फर्काऊ dtypep == other.dtypep && varType == other.varType;
        }
    };
    संरचना Dtype_Func_साँचो_Hash final {
        माप_प्रकार चालक()(अचर Dtype_Func_साँचो& साँचो) अचर {
            फर्काऊ std::hash<अचर Ast_गाँठो_DType*>{}(साँचो.dtypep)
                   ^ (std::hash<uint8_t>{}(साँचो.varType) << 1);
        }
    };
    std::unordered_map<Dtype_Func_साँचो, AstCFunc*, Dtype_Func_साँचो_Hash>
        m_dtypeFuncs;  // Functions per type+kind
    AstCFunc* m_dtypeFuncp = nullptr;  // Current type func
    AstCFunc* m_dtypeSubFuncp = nullptr;  // Current type sub func
    अचर string m_dtypeArgs{", const char* name, uint32_t fidx, uint32_t c, "
                             "VerilatedTraceSigDirection direction"};  // Type func args
    AstTraceDecl* m_dtypeDeclp = nullptr;  // Current type func decl
    V3UniqueNames m_dtypeNames{""};  // Unique names for dtype funcs
    तार्किक m_skipDtypeFunc = असत्य;  // Don't create a type func
    uint32_t m_offset = std::numeric_limits<uint32_t>::max();  // Offset for types
    पूर्णाङ्क m_शीर्ष_Func_Size = 0;  // Size of the top function currently being built
    पूर्णाङ्क m_subFuncSize = 0;  // Size of the sub function currently being built
    तार्किक m_forceNewLeaf = असत्य;  // Force next addToSubFunc to start a fresh leaf
    माप_प्रकार m_शीर्ष_Scope_जरा_Func_गन्ती = 0;  // Top-scope init functions used only for wrapper IOs
    तार्किक m_शीर्ष_Scope_जरा_Phase = असत्य;  // Emitting top-scope wrapper IO declarations
    अचर पूर्णाङ्क m_funcSizeLimit  // Maximum size of a function
        = v3Global.opt.outputSplitCTrace() ? v3Global.opt.outputSplitCTrace()
                                           : std::numeric_limits<पूर्णाङ्क>::max();
    // Trace init functions to for each scope
    std::unordered_map<अचर AstScope*, std::vector<AstCFunc*>> m_scopeInitFuncps;
    // Map from hierarchical scope name to the corresponding AstScope. Note that
    // this is a many-to-one mapping for interfaces, due to interface refs.
    std::unordered_map<std::string, अचर AstScope*> m_pathToScopep;
    // Cell initialization placeholders:
    // (parent scope, cell under parent scope, statement)
    std::vector<std::tuple<AstScope*, AstCell*, Ast_गाँठो_Stmt*>> m_cellInitPlaceholders;
    // Interface refs initialization placeholders:
    // (Interface ref variable, placeholder statement)
    std::vector<std::tuple<AstVarScope*, Ast_गाँठो_Stmt*>> m_ifaceRefInitPlaceholders;

    // A trace entry under a scope is either:
    // - A variable (including interface references)
    // - A sub scope (stored as the cell corresponding to the sub scope)
    // Note: members are non-const to allow copy during sorting
    वर्ग TraceEntry final {
        AstVarScope* m_vscp = nullptr;  // AstVarScope under scope being traced
        AstCell* m_cellp = nullptr;  // Sub scope (as AstCell) under scope being traced
        std::string m_path;  // Path to enclosing module in original hierarchy
        std::string m_name;  // Name of signal/subscope

        शून्य init(अचर std::string& name, Ast_गाँठो* nodep, तार्किक in_शीर्ष_Scope) {
            // Compute path in hierarchy and item name
            अचर std::string& vcdName = Ast_गाँठो::vcdName(name);
            AstVar* अचर varp = VN_CAST(nodep, Var);
            यदि (VN_IS(nodep, Cell) || VN_IS(varp->dtypep(), IfaceRefDType)) {
                // Cell or interface reference
                m_path = vcdName + " ";
                m_name.clear();
            } अन्यथा यदि (varp->isPrimaryIO()) {
                // Primary IO variable
                m_path = "$rootio ";
                m_name = vcdName;
            } अन्यथा {
                // Other Variable
                अचर माप_प्रकार pos = vcdName.rfind(' ');
                अचर माप_प्रकार pathLen = pos == std::string::npos ? 0 : pos + 1;
                m_path = vcdName.substr(0, pathLen);
                m_name = vcdName.substr(pathLen);
            }

            // When creating a --lib-create library, drop the name of the selected top module.
            // This will be replaced by the instance name in the model that uses the library, or
            // restored at runtime if the library itself is traced as the root model. Other top
            // level entities ($unit, packages, ...) keep a '$libroot' wrapper so they still have
            // a stable location in the dump.
            यदि (in_शीर्ष_Scope && !v3Global.opt.libCreate().empty()) {
                अचर माप_प्रकार start = m_path.find(' ');
                // Must have a prefix in the top scope with lib, as top wrapper signals not traced
                UASSERT_OBJ(start != std::string::npos, nodep, "No prefix with --lib-create");
                अचर std::string prefix = m_path.substr(0, start);
                // Wrapper primary IOs stay under $rootio so a root-traced library can restore
                // them under the runtime C++ instance name without affecting embedded use.
                यदि (prefix == "$rootio") फर्काऊ;
                m_path = m_path.substr(start + 1);
                यदि (v3Global.rootp()->trace_Lib_शीर्ष_Name() != prefix) m_path = "$libroot " + m_path;
            }
        }

    सार्वजनिक:
        explicit TraceEntry(अचर AstScope* scopep, AstVarScope* vscp)
            : m_vscp{vscp} {
            init(vscp->varp()->name(), vscp->varp(), scopep->is_शीर्ष());
        }
        explicit TraceEntry(अचर AstScope* scopep, AstCell* cellp)
            : m_cellp{cellp} {
            init(cellp->name(), cellp, scopep->is_शीर्ष());
        }
        पूर्णाङ्क operatorCompare(अचर TraceEntry& b) अचर {
            यदि (अचर पूर्णाङ्क cmp = path().compare(b.path())) फर्काऊ cmp < 0;
            यदि (अचर पूर्णाङ्क cmp = fileline().operatorCompare(b.fileline())) फर्काऊ cmp < 0;
            फर्काऊ name() < b.name();
        }
        AstVarScope* vscp() अचर { फर्काऊ m_vscp; }
        AstCell* cellp() अचर { फर्काऊ m_cellp; }
        अचर std::string& path() अचर { फर्काऊ m_path; }
        शून्य path(अचर std::string& path) { m_path = path; }
        अचर std::string& name() अचर { फर्काऊ m_name; }
        File_पङ्क्ति& fileline() अचर { फर्काऊ m_vscp ? *m_vscp->fileline() : *m_cellp->fileline(); }
    };
    std::vector<TraceEntry> m_entries;  // Trace entries under current scope
    AstVarScope* m_traVscp = nullptr;  // Current AstVarScope we are constructing AstTraceDecls for
    Ast_गाँठो_अभिव्यक्ति* m_traValuep = nullptr;  // Value expression for current signal
    string m_traName;  // Name component for current signal

    VDouble0 m_statSigs;  // Statistic tracking
    VDouble0 m_statIgnSigs;  // Statistic tracking

    // METHODS

    string vscIgnoreTrace(अचर AstVarScope* nodep) {
        // Return true if this shouldn't be traced
        // See also similar rule in V3Coverage::varIgnoreToggle
        अचर AstVar* अचर varp = nodep->varp();
        यदि (!varp->isTrace()) फर्काऊ "Verilator trace_off";
        यदि (!nodep->isTrace()) फर्काऊ "Verilator instance trace_off";
        // Automatics (typically, excluding forks) have no persistance over
        // time, and may optimize differently when multithreadeded or hierarchical.
        // Class automatics refer to being in a class but might still be pointed
        // to by a static, so are ok.
        यदि (varp->lifetime().isAutomatic() && !varp->isClassMember() && !varp->isParam())
            फर्काऊ "Automatic variable";

        अचर पूर्णाङ्क width = recurseDTypeWidth(nodep->varp()->dtypep());
        यदि (v3Global.opt.traceMaxWidth() && width > v3Global.opt.traceMaxWidth())
            फर्काऊ "Width " + cvtToStr(width) + " > --trace-max-width";

        अचर string prettyName = nodep->prettyName();
        यदि (!V3Control::getScopeTraceOn(prettyName)) फर्काऊ "Vlt scope trace_off";
        यदि (!v3Global.opt.traceUnderscore()) {
            यदि (!prettyName.empty() && prettyName[0] == '_') फर्काऊ "Leading underscore";
            यदि (prettyName.find("._") != string::npos) फर्काऊ "Inlined leading underscore";
        }
        फर्काऊ ""s;
    }

    पूर्णाङ्क recurseDTypeWidth(अचर Ast_गाँठो_DType* nodep) {
        यदि (अचर Ast_गाँठो_Array_DType* adtypep = VN_CAST(nodep, गाँठो_Array_DType))
            फर्काऊ recurseDTypeWidth(adtypep->subDTypep()) * adtypep->declRange().elements();
        फर्काऊ nodep->width();
    }

    AstCFunc* newCFunc(File_पङ्क्ति* flp, अचर string& name) {
        AstScope* अचर शीर्ष_Scopep = m_शीर्ष_Scopep->scopep();
        AstCFunc* अचर funcp = नूतन AstCFunc{flp, name, शीर्ष_Scopep};
        funcp->argTypes(v3Global.opt.traceClassBase() + "* tracep");
        funcp->isTrace(सत्य);
        funcp->isStatic(असत्य);
        funcp->isLoose(सत्य);
        funcp->slow(सत्य);
        शीर्ष_Scopep->addBlocksp(funcp);
        फर्काऊ funcp;
    }

    शून्य add_To_शीर्ष_Func(Ast_गाँठो_Stmt* stmtp) {
        यदि (m_शीर्ष_Func_Size > m_funcSizeLimit || m_शीर्ष_Funcps.empty()) {
            m_शीर्ष_Func_Size = 0;
            //
            अचर string n = cvtToStr(m_शीर्ष_Funcps.size());
            अचर string name{"trace_init_top__" + n};
            AstCFunc* अचर funcp = newCFunc(m_शीर्ष_Scopep->fileline(), name);
            m_शीर्ष_Funcps.थप_back(funcp);
        }
        m_शीर्ष_Funcps.back()->addStmtsp(stmtp);
        m_शीर्ष_Func_Size += stmtp->गाँठो_गन्ती();
    }

    शून्य addToSubFunc(Ast_गाँठो_Stmt* stmtp) {
        // TODO (maybe) -- sub funcs for dtype components
        यदि (m_dtypeSubFuncp) {
            यदि (m_subFuncSize > m_funcSizeLimit) newDeclSubFunc();
            m_dtypeSubFuncp->addStmtsp(stmtp);
            m_subFuncSize += stmtp->गाँठो_गन्ती();
            फर्काऊ;
        }
        // Defer trace splitting until V3Trace
        यदि (m_subFuncps.empty() || m_forceNewLeaf) {
            m_forceNewLeaf = असत्य;
            File_पङ्क्ति* अचर flp = m_currScopep->fileline();
            अचर string n = cvtToStr(m_subFuncps.size());
            अचर string name
                = m_currScopep == m_शीर्ष_Scopep->scopep() && !v3Global.opt.libCreate().empty()
                      ? (m_शीर्ष_Scope_जरा_Phase ? "trace_init_leaf_root__" : "trace_init_leaf_top__")
                            + n
                      : "trace_init_sub__" + m_currScopep->nameDotless() + "__" + n;
            AstCFunc* अचर funcp = newCFunc(flp, name);
            funcp->addStmtsp(नूतन AstCStmt{flp, "const int c = vlSymsp->__Vm_baseCode;"});
            m_subFuncps.थप_back(funcp);
        }
        m_subFuncps.back()->addStmtsp(stmtp);
        m_subFuncSize += stmtp->गाँठो_गन्ती();
    }

    AstTraceDecl* addTraceDecl(अचर VNumRange& arrayRange,
                               पूर्णाङ्क widthOverride,  // If !=0, is packed struct/array where basicp
                                                   // size misreflects one element
                               AstCCall* अचर dtypeCallp = nullptr) {
        VNumRange bitRange;
        यदि (widthOverride) {
            bitRange = VNumRange{widthOverride - 1, 0};
        } अन्यथा यदि (अचर AstBasicDType* अचर bdtypep = m_traValuep->dtypep()->basicp()) {
            bitRange = bdtypep->nrange();
        }
        File_पङ्क्ति* अचर flp = m_traVscp->fileline();
        Ast_गाँठो_अभिव्यक्ति* valuep = m_traValuep->clone_रूख(असत्य);
        अचर तार्किक validOffset = m_offset != std::numeric_limits<uint32_t>::max();
        AstTraceDecl* अचर newp
            = नूतन AstTraceDecl{flp,      m_traName,  m_traVscp->varp(), valuep,
                               bitRange, arrayRange, dtypeCallp,        validOffset};
        यदि (validOffset) {
            newp->code(m_offset);
            यदि (!dtypeCallp) { m_offset += newp->codeInc(); }
            valuep->foreach([&](AstVarRef* अचर refp) {
                UASSERT_OBJ(refp->varScopep() == m_traVscp, refp,
                            "Trace decl expression references unexpected var");
                refp->replaceWith(नूतन AstCExpr{flp, "__VdtypeVar", m_traVscp->width()});
                VL_DO_DANGLING(refp->delete_रूख(), refp);
            });
        } अन्यथा {
            newp->dtypeDeclp(m_dtypeDeclp);
        }
        m_declUncalledps.emplace(newp);
        addToSubFunc(newp);

        फर्काऊ newp;
    }

    शून्य addIgnore(अचर string& why) {
        ++m_statIgnSigs;
        अचर std::string cmt = "Tracing: "s + m_traName + " // Ignored: " + why;
        यदि (debug() > 3 && m_traVscp) std::cout << "- " << m_traVscp->fileline() << cmt << '\n';
    }

    शून्य fixupPlaceholder(अचर std::string& path, Ast_गाँठो_Stmt* placeholderp) {
        // Find the scope for the path. As we are working based on cell names,
        // it is possible there is no corresponding scope (e.g.: for an empty
        // module).
        अचर स्वतः it = m_pathToScopep.find(Ast_गाँठो::prettyName(path));
        यदि (it != m_pathToScopep.end()) {
            अचर AstScope* अचर scopep = it->second;
            File_पङ्क्ति* अचर flp = placeholderp->fileline();

            // Call the initialization functions for the scope
            Ast_गाँठो* stmtp = nullptr;
            लागि (AstCFunc* अचर subFuncp : m_scopeInitFuncps.at(scopep)) {
                AstCCall* अचर callp = नूतन AstCCall{flp, subFuncp};
                callp->dtypeSetVoid();
                callp->argTypes("tracep");
                stmtp = Ast_गाँठो::addNext(stmtp, callp->makeStmt());
            }

            // Add after the placeholder
            यदि (stmtp) placeholderp->addNextHere(stmtp);
        }
        // Delete the placeholder
        placeholderp->unlinkFrBack();
        VL_DO_DANGLING(placeholderp->delete_रूख(), placeholderp);
    }

    शून्य fixupLibStub(अचर std::string& path, Ast_गाँठो_Stmt* placeholderp) {
        File_पङ्क्ति* अचर flp = placeholderp->fileline();

        // The library instance's model is named after its %m within the parent model,
        // which does not include the "l2-name" (top module) of the library
        std::string name = Ast_गाँठो::prettyName(path);
        यदि (!v3Global.opt.libCreate().empty()) {
            अचर std::string शीर्ष_Prefix = v3Global.rootp()->trace_Lib_शीर्ष_Name() + ".";
            यदि (VString::startsWith(name, शीर्ष_Prefix)) name = name.substr(शीर्ष_Prefix.size());
        }

        // Call the initialization function for the library instance
        AstCStmt* अचर initp = नूतन AstCStmt{flp};
        initp->add("{\n");
        initp->add("std::string __VlibName = vlSymsp->name();\n");
        initp->add("if (!__VlibName.empty()) __VlibName += '.';\n");
        initp->add("__VlibName += ");
        initp->add(नूतन AstConst{flp, AstConst::String{}, name});
        initp->add(";\n");
        initp->add("tracep->initLib(__VlibName);\n");
        initp->add("}\n");

        placeholderp->addNextHere(initp);
        // Delete the placeholder
        VL_DO_DANGLING(placeholderp->unlinkFrBack()->delete_रूख(), placeholderp);
    }

    शून्य fixupPlaceholders() {
        // Fix up cell initialization placehodlers
        UINFO(9, "fixupPlaceholders()");
        लागि (अचर स्वतः& item : m_cellInitPlaceholders) {
            अचर AstScope* अचर parentp = std::get<0>(item);
            अचर AstCell* अचर cellp = std::get<1>(item);
            Ast_गाँठो_Stmt* अचर placeholderp = std::get<2>(item);
            अचर std::string path = parentp->name() + "__DOT__" + cellp->name();
            यदि (cellp->modp()->verilatorLib()) {
                fixupLibStub(path, placeholderp);
            } अन्यथा {
                fixupPlaceholder(path, placeholderp);
            }
        }

        // Fix up interface reference initialization placeholders
        लागि (अचर स्वतः& item : m_ifaceRefInitPlaceholders) {
            अचर AstVarScope* अचर vscp = std::get<0>(item);
            Ast_गाँठो_Stmt* अचर placeholderp = std::get<1>(item);
            अचर std::string path = vscp->scopep()->name() + "__DOT__" + vscp->varp()->name();
            fixupPlaceholder(path, placeholderp);
        }
    }

    शून्य remove_Redundant_Prefix_थप_झिक() {
        लागि (अचर स्वतः& pair : m_scopeInitFuncps) {
            // cppcheck-suppress constVariablePointer
            लागि (AstCFunc* अचर funcp : pair.second) {
                Ast_गाँठो* prevp = nullptr;
                Ast_गाँठो* currp = funcp->stmtsp();
                जबसम्म (currp) {
                    Ast_गाँठो* अचर nextp = currp->nextp();
                    यदि (VN_IS(prevp, Trace_थप_Prefix) && VN_IS(currp, Trace_झिक_Prefix)) {
                        VL_DO_DANGLING(prevp->unlinkFrBack()->delete_रूख(), prevp);
                        VL_DO_DANGLING(currp->unlinkFrBack()->delete_रूख(), currp);
                    }
                    यदि (!nextp) रोक;
                    prevp = nextp->backp();
                    currp = nextp;
                }
            }
        }
    }

    शून्य checkCalls(अचर AstCFunc* funcp) {
        यदि (!v3Global.opt.debugCheck()) फर्काऊ;
        checkCallsRecurse(funcp);
        यदि (!m_declUncalledps.empty()) {  // LCOV_EXCL_START
            लागि (स्वतः tracep : m_declUncalledps) UINFO(0, "-nodep " << tracep);
            (*(m_declUncalledps.begin()))->v3fatalSrc("Created TraceDecl which is never called");
        }  // LCOV_EXCL_STOP
    }
    शून्य checkCallsRecurse(अचर AstCFunc* funcp) {
        funcp->foreach([एतत्](अचर Ast_गाँठो* nodep) {
            यदि (अचर AstTraceDecl* अचर declp = VN_CAST(nodep, TraceDecl)) {
                m_declUncalledps.erase(declp);
            } अन्यथा यदि (अचर AstCCall* अचर ccallp = VN_CAST(nodep, CCall)) {
                checkCallsRecurse(ccallp->funcp());
            }
        });
    }

    तार्किक isBasicIO() { फर्काऊ m_traVscp->varp()->isVLIO(); }
    शून्य newDeclSubFunc() {
        File_पङ्क्ति* अचर flp = m_dtypeFuncp->fileline();
        AstCFunc* अचर subFuncp = newCFunc(flp, m_dtypeNames.get("trace_init_dtype_sub__"));
        subFuncp->argTypes(subFuncp->argTypes() + m_dtypeArgs);
        AstCCall* अचर subCallp = नूतन AstCCall{flp, subFuncp};
        subCallp->dtypeSetVoid();
        subCallp->argTypes("tracep, name, fidx, c, direction");
        m_dtypeFuncp->addStmtsp(subCallp->makeStmt());
        m_dtypeSubFuncp = subFuncp;
        m_subFuncSize = 0;
    }
    शून्य newDeclFunc(Ast_गाँठो_DType* nodep) {
        Ast_गाँठो_DType* अचर skipTypep = nodep->skipRefp();
        // offset and direction args added in EmitCImp
        std::string callArgs{"tracep, \"" + VIdProtect::protect(m_traName) + "\""};
        VL_RESTORER_COPY(m_traName);
        File_पङ्क्ति* अचर flp = skipTypep->fileline();

        अचर Dtype_Func_साँचो dtype_साँचो{skipTypep, m_traVscp->varp()->varType()};
        स्वतः pair = m_dtypeFuncs.emplace(dtype_साँचो, nullptr);
        AstCFunc** funcpp = &pair.first->second;
        यदि (pair.second) {
            *funcpp = newCFunc(flp, m_dtypeNames.get("trace_init_dtype__"));
            (*funcpp)->argTypes((*funcpp)->argTypes() + m_dtypeArgs);
            (*funcpp)->user2p(m_traVscp);
        }

        AstCCall* अचर callp = नूतन AstCCall{flp, *funcpp};
        callp->dtypeSetVoid();
        callp->argTypes(callArgs);
        m_dtypeDeclp = addTraceDecl(VNumRange{}, skipTypep->width(), callp);
        addToSubFunc(callp->makeStmt());

        यदि (pair.second) {
            VL_RESTORER(m_offset);
            m_offset = 0;

            VL_RESTORER(m_dtypeFuncp);
            VL_RESTORER(m_dtypeSubFuncp);
            VL_RESTORER(m_subFuncSize);
            m_dtypeFuncp = *funcpp;
            newDeclSubFunc();
            यदि (AstStructDType* अचर dtypep = VN_CAST(skipTypep, StructDType)) {
                declStruct(dtypep, सत्य);
            } अन्यथा यदि (AstUnpackArrayDType* अचर dtypep = VN_CAST(skipTypep, UnpackArrayDType)) {
                declUnpackedArray(dtypep, सत्य);
            } अन्यथा यदि (AstPackArrayDType* अचर dtypep = VN_CAST(skipTypep, PackArrayDType)) {
                declPackedArray(dtypep, सत्य);
            } अन्यथा {
                UASSERT_OBJ(असत्य, skipTypep, "Creating a trace function for an unexpected type");
            }
            m_dtypeFuncp->user1(m_offset);
        }

        m_dtypeDeclp->codeInc((*funcpp)->user1());
        m_offset += m_dtypeDeclp->codeInc();
    }
    शून्य declUnpackedArray(AstUnpackArrayDType* अचर nodep, तार्किक newFunc) {
        string prefixName(newFunc ? "name" : m_traName);

        VL_RESTORER_COPY(m_traName);
        File_पङ्क्ति* अचर flp = nodep->fileline();

        addToSubFunc(नूतन Ast_Trace_थप_Prefix{flp, prefixName, VTracePrefixType::ARRAY_UNPACKED,
                                            nodep->बायाँ(), nodep->दायाँ(), !newFunc});

        यदि (VN_IS(nodep->subDTypep()->skipRefToEnump(),
                  BasicDType)  // Nothing lower than this array
            && m_traVscp->dtypep()->skipRefToEnump() == nodep) {  // Nothing above this array
            // Simple 1-D array, use existing V3EmitC runtime loop rather than unrolling
            // This will put "(index)" at end of signal name for us
            यदि (m_traVscp->dtypep()->skipRefToEnump()->isString()) {
                addIgnore("Unsupported: strings");
            } अन्यथा {
                m_traName = "";
                addTraceDecl(nodep->declRange(), 0);
            }
        } अन्यथा {
            Ast_गाँठो_DType* अचर subtypep = nodep->subDTypep()->skipRefToEnump();
            // Always iterate left index to right index
            लागि (अचर पूर्णाङ्क i : nodep->declRange().seq_बायाँ_To_दायाँ()) {
                VL_RESTORER(m_traValuep);
                m_traName = '[' + std::to_string(i) + ']';
                m_traValuep = m_traValuep->clone_रूख(असत्य);
                m_traValuep = नूतन AstArraySel{flp, m_traValuep, i - nodep->lo()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_रूख(), m_traValuep);
            }
        }

        addToSubFunc(नूतन Ast_Trace_झिक_Prefix{flp});
    }
    शून्य declPackedArray(AstPackArrayDType* अचर nodep, तार्किक newFunc) {
        string prefixName(newFunc ? "name" : m_traName);
        Ast_गाँठो_DType* अचर subtypep = nodep->subDTypep()->skipRefToEnump();

        VL_RESTORER_COPY(m_traName);
        File_पङ्क्ति* अचर flp = nodep->fileline();

        addToSubFunc(नूतन Ast_Trace_थप_Prefix{flp, prefixName, VTracePrefixType::ARRAY_PACKED,
                                            nodep->बायाँ(), nodep->दायाँ(), !newFunc});

        // Always iterate left index to right index
        लागि (अचर पूर्णाङ्क i : nodep->declRange().seq_बायाँ_To_दायाँ()) {
            VL_RESTORER(m_traValuep);
            m_traName = '[' + std::to_string(i) + ']';
            अचर पूर्णाङ्क lsb = (i - nodep->lo()) * subtypep->width();
            m_traValuep = m_traValuep->clone_रूख(असत्य);
            m_traValuep = नूतन AstSel{flp, m_traValuep, lsb, subtypep->width()};
            m_traValuep->dtypep(subtypep);
            iterate(subtypep);
            VL_DO_CLEAR(m_traValuep->delete_रूख(), m_traValuep = nullptr);
        }

        addToSubFunc(नूतन Ast_Trace_झिक_Prefix{flp});
    }
    शून्य declStruct(AstStructDType* अचर nodep, तार्किक newFunc) {
        File_पङ्क्ति* अचर flp = nodep->fileline();
        string prefixName(newFunc ? "name" : m_traName);
        पूर्णाङ्क nMembers = 0;
        लागि (Ast_गाँठो* mp = nodep->membersp(); mp; mp = mp->nextp()) ++nMembers;
        यदि (!nodep->packed()) {
            addToSubFunc(नूतन Ast_Trace_थप_Prefix{flp, prefixName,  //
                                                VTracePrefixType::STRUCT_UNPACKED, nMembers, 0,
                                                !newFunc});
            लागि (अचर AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_गाँठो_DType* अचर subtypep = itemp->subDTypep()->skipRefToEnump();
                m_traName = itemp->prettyName();
                VL_RESTORER(m_traValuep);
                m_traValuep = m_traValuep->clone_रूख(असत्य);
                m_traValuep = नूतन AstStructSel{flp, m_traValuep, itemp->name()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_रूख(), m_traValuep);
            }
            addToSubFunc(नूतन Ast_Trace_झिक_Prefix{flp});
        } अन्यथा {
            addToSubFunc(नूतन Ast_Trace_थप_Prefix{flp, prefixName,  //
                                                VTracePrefixType::STRUCT_PACKED, nMembers, 0,
                                                !newFunc});
            लागि (अचर AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_गाँठो_DType* अचर subtypep = itemp->subDTypep()->skipRefToEnump();
                m_traName = itemp->prettyName();
                VL_RESTORER(m_traValuep);
                m_traValuep = m_traValuep->clone_रूख(असत्य);
                m_traValuep = नूतन AstSel{flp, m_traValuep, itemp->lsb(), subtypep->width()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_रूख(), m_traValuep);
            }
            addToSubFunc(नूतन Ast_Trace_झिक_Prefix{flp});
        }
    }

    // VISITORS
    शून्य visit(AstScope* nodep) override {
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(!m_currScopep, nodep, "Should not nest");
        UASSERT_OBJ(m_subFuncps.empty(), nodep, "Should not nest");
        UASSERT_OBJ(m_entries.empty(), nodep, "Should not nest");
        UASSERT_OBJ(!m_traVscp, nodep, "Should not nest");
        UASSERT_OBJ(!m_traValuep, nodep, "Should not nest");
        UASSERT_OBJ(m_traName.empty(), nodep, "Should not nest");

        // If this is a stub for a --lib-create library, skip.
        यदि (nodep->modp()->verilatorLib()) फर्काऊ;

        VL_RESTORER(m_currScopep);
        m_currScopep = nodep;

        // Gather signals under this scope
        iterateChildrenConst(nodep);

        // Gather cells under this scope
        लागि (Ast_गाँठो* stmtp = nodep->modp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            यदि (AstCell* अचर cellp = VN_CAST(stmtp, Cell)) m_entries.emplace_back(nodep, cellp);
        }

        यदि (!m_entries.empty()) {
            // Sort trace entries, by enclosing instance (necessary for single traversal of
            // hierarchy during initialization), then by source location, then by name.
            std::stable_sort(
                m_entries.begin(), m_entries.end(),
                [](अचर TraceEntry& a, अचर TraceEntry& b) { फर्काऊ a.operatorCompare(b); });

            File_पङ्क्ति* अचर flp = nodep->fileline();
            PathAdjustor pathAdjustor{flp, [&](Ast_गाँठो_Stmt* stmtp) { addToSubFunc(stmtp); }};
            अचर तार्किक split_जरा_Primary_Ios = nodep->is_शीर्ष() && !v3Global.opt.libCreate().empty();
            अचर स्वतः emitEntry = [&](अचर TraceEntry& entry) {
                AstVarScope* अचर vscp = entry.vscp();
                // Adjust name prefix based on path in hierarchy
                UINFO(9, "path='" << entry.path() << "' name='" << entry.name() << "' "
                                  << (entry.cellp() ? static_cast<Ast_गाँठो*>(entry.cellp())
                                                    : static_cast<Ast_गाँठो*>(entry.vscp())));
                pathAdjustor.adjust(entry.path(), entry.cellp(), entry.vscp());

                m_traName = entry.name();

                यदि (vscp) {
                    // This is a signal: build AstTraceDecl for it
                    m_traVscp = vscp;
                    अचर string& ignoreReason = vscIgnoreTrace(m_traVscp);
                    यदि (!ignoreReason.empty()) {
                        addIgnore(ignoreReason);
                    } अन्यथा {
                        ++m_statSigs;
                        // Create reference to whole signal. We will operate on this during the
                        // traversal.
                        m_traValuep
                            = नूतन AstVarRef{m_traVscp->fileline(), m_traVscp, VAccess::READ};
                        // Recurse into data type of the signal. The visit methods will add
                        // AstTraceDecls.
                        iterate(m_traVscp->varp()->dtypep()->skipRefToEnump());
                        // Delete reference created above. Traversal cloned it as required.
                        यदि (m_traValuep) {
                            VL_DO_DANGLING(m_traValuep->delete_रूख(), m_traValuep);
                            // Note: Sometimes VL_DANGLING is a no-op, but we have assertions
                            // on m_traValuep being nullptr, so make sure it is.
                            m_traValuep = nullptr;
                        }
                    }
                } अन्यथा {
                    // This is a subscope: insert a placeholder to be fixed up later
                    AstCell* अचर cellp = entry.cellp();
                    Ast_गाँठो_Stmt* अचर stmtp = नूतन AstComment{
                        cellp->fileline(), "Instance init for: " + cellp->prettyName()};
                    addToSubFunc(stmtp);
                    m_cellInitPlaceholders.emplace_back(nodep, cellp, stmtp);
                }
            };
            यदि (split_जरा_Primary_Ios) {
                m_शीर्ष_Scope_जरा_Phase = सत्य;
                लागि (अचर TraceEntry& entry : m_entries) {
                    AstVarScope* अचर vscp = entry.vscp();
                    यदि (!(vscp && vscp->varp()->isPrimaryIO())) जारी;
                    emitEntry(entry);
                }
                m_शीर्ष_Scope_जरा_Phase = असत्य;
                pathAdjustor.unwind();
                m_शीर्ष_Scope_जरा_Func_गन्ती = m_subFuncps.size();
                यदि (m_शीर्ष_Scope_जरा_Func_गन्ती) {
                    // Force the next addToSubFunc to start a fresh leaf so
                    // root-phase decls and top-phase decls live in separate
                    // functions. Without this, both phases land in the same
                    // leaf and `trace_init_top` ends up emitting both the
                    // $rootio-prefixed IOs and the regular-scope decls, so
                    // each IO gets traced twice when the lib is consumed
                    // via initLib(name).
                    m_subFuncSize = 0;
                    m_forceNewLeaf = सत्य;
                }
            }
            लागि (अचर TraceEntry& entry : m_entries) {
                AstVarScope* अचर vscp = entry.vscp();
                यदि (split_जरा_Primary_Ios && vscp && vscp->varp()->isPrimaryIO()) जारी;
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
        अचर std::string path = nodep->prettyName();
        m_pathToScopep.emplace(path, nodep);

        // Save the hierarchical names of interface references that reference this scope
        अचर AstCell* अचर cellp = nodep->aboveCellp();
        यदि (cellp
            && VN_IS(cellp->modp(), Iface)
            // Exclude classes that were inside interfaces, the cell's modp is an interface,
            // but the nodep->module is the Class
            && VN_IS(nodep->modp(), Iface)) {
            अचर माप_प्रकार lastDot = path.find_last_of('.');
            UASSERT_OBJ(lastDot != string::npos, nodep,
                        "Expected an interface scope name to have at least one dot: " << path);
            अचर std::string parentPath = path.substr(0, lastDot + 1);

            लागि (AstIntfRef *intfRefp = cellp->intfRefsp(), *nextp; intfRefp; intfRefp = nextp) {
                nextp = VN_AS(intfRefp->nextp(), IntfRef);

                अचर std::string refName = intfRefp->prettyName();

                // Assume only references under the same parent scope reference
                // the same interface.
                // TODO: This is not actually correct. An interface can propagate
                //       upwards and sideways when passed to a port via a downward
                //       hierarchical reference, which we will miss here.
                यदि (!VString::startsWith(refName, parentPath)) जारी;

                // Save the mapping from the path of the reference to the scope
                m_pathToScopep.emplace(refName, nodep);

                // No more need for AstIntfRef, unless V3EmitCSyms wants it for VPI
                यदि (!v3Global.opt.vpi()) {
                    VL_DO_DANGLING(intfRefp->unlinkFrBack()->delete_रूख(), intfRefp);
                }
            }
        }
    }
    शून्य visit(AstVarScope* nodep) override {
        UASSERT_OBJ(m_currScopep, nodep, "AstVarScope not under AstScope");

        // Prefilter - things that get added to m_vscps will either get traced or get a comment as
        // to why they are not traced. Generally these conditions doesn't need updating, instead
        // use varp->isTrace() and/or vscIgnoreTrace.
        यदि (nodep->varp()->isTemp() && !nodep->varp()->isTrace()) फर्काऊ;
        यदि (nodep->varp()->isClassMember()) फर्काऊ;
        यदि (nodep->varp()->isFuncLocal()) फर्काऊ;

        // When creating a --lib-create library ...
        यदि (!v3Global.opt.libCreate().empty()) {
            // Ignore parameters in packages. These will be traced at the top level.
            यदि (nodep->varp()->isParam() && VN_IS(nodep->scopep()->modp(), Package)) फर्काऊ;
        }

        // Add to traced signal list
        m_entries.emplace_back(m_currScopep, nodep);
    }

    // VISITORS - Data types when tracing
    शून्य visit(AstConstDType* nodep) override {
        यदि (!m_traVscp) फर्काऊ;
        VL_RESTORER(m_offset);
        VL_RESTORER(m_skipDtypeFunc);
        m_skipDtypeFunc = सत्य;
        iterate(nodep->subDTypep()->skipRefToEnump());
    }
    शून्य visit(AstRefDType* nodep) override {
        यदि (!m_traVscp) फर्काऊ;
        VL_RESTORER(m_offset);
        iterate(nodep->subDTypep()->skipRefToEnump());
    }
    शून्य visit(AstIfaceRefDType* /*nodep*/) override {
        यदि (!m_traVscp) फर्काऊ;
        // Insert a placeholder to be fixed up later
        File_पङ्क्ति* अचर flp = m_traVscp->fileline();
        Ast_गाँठो_Stmt* अचर stmtp
            = नूतन AstComment{flp, "Interface ref init for: " + m_traVscp->prettyName()};
        addToSubFunc(stmtp);
        m_ifaceRefInitPlaceholders.emplace_back(m_traVscp, stmtp);
    }
    शून्य visit(AstUnpackArrayDType* nodep) override {
        // Note more specific dtypes above
        यदि (!m_traVscp) फर्काऊ;

        यदि (v3Global.opt.traceMaxArray()
            && static_cast<पूर्णाङ्क>(nodep->arrayUnpackedElements()) > v3Global.opt.traceMaxArray()) {
            addIgnore("Wide memory > --trace-max-array ents");
            फर्काऊ;
        }

        // Skip unpacked arrays of non-traceable leaf types (e.g. strings)
        यदि (VN_IS(nodep->subDTypep()->skipRefToEnump(), BasicDType)
            && nodep->subDTypep()->skipRefToEnump()->isString()) {
            addIgnore("Unsupported: strings");
            फर्काऊ;
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        यदि (isBasicIO()) m_skipDtypeFunc = सत्य;

        यदि (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_offset);
            newDeclFunc(nodep);
        }
        declUnpackedArray(nodep, असत्य);
    }
    शून्य visit(AstPackArrayDType* nodep) override {
        यदि (!m_traVscp) फर्काऊ;

        यदि (!v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit.
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            addTraceDecl(VNumRange{}, nodep->width());
            फर्काऊ;
        }

        Ast_गाँठो_DType* अचर subtypep = nodep->subDTypep()->skipRefToEnump();

        // Do not unroll if the elements are simple 'bit', or 'logic'
        यदि (AstBasicDType* अचर basicp = VN_CAST(subtypep->skipRefp(), BasicDType)) {
            यदि (basicp->isBitLogic() && !basicp->isRanged()) {
                addTraceDecl(VNumRange{}, nodep->width());
                फर्काऊ;
            }
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        यदि (isBasicIO()) m_skipDtypeFunc = सत्य;

        यदि (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_offset);
            newDeclFunc(nodep);
        }
        declPackedArray(nodep, असत्य);
    }
    शून्य visit(AstStructDType* nodep) override {
        यदि (!m_traVscp) फर्काऊ;

        यदि (nodep->packed() && !v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            addTraceDecl(VNumRange{}, nodep->width());
            फर्काऊ;
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        यदि (isBasicIO()) m_skipDtypeFunc = सत्य;

        // Only create sub functions for top-level structs, i.e. don't have struct funcs
        // call other struct funcs for child types.  This could easily be done for decl funcs
        // but full / chg funcs would require copying / aligning data for child types or more
        // complicated / wonky / generalized data access.
        यदि (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_offset);
            newDeclFunc(nodep);
        }
        declStruct(nodep, असत्य);
    }
    शून्य visit(AstUnionDType* nodep) override {
        यदि (!m_traVscp) फर्काऊ;

        VL_RESTORER(m_skipDtypeFunc);
        m_skipDtypeFunc = सत्य;

        यदि (nodep->packed() && !v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            addTraceDecl(VNumRange{}, nodep->width());
            फर्काऊ;
        }

        VL_RESTORER_COPY(m_traName);
        File_पङ्क्ति* अचर flp = nodep->fileline();

        पूर्णाङ्क nMembers = 0;
        लागि (Ast_गाँठो* mp = nodep->membersp(); mp; mp = mp->nextp()) ++nMembers;

        यदि (!nodep->packed()) {
            addIgnore("Unsupported: Unpacked union");
        } अन्यथा {
            addToSubFunc(नूतन Ast_Trace_थप_Prefix{flp, m_traName,  //
                                                VTracePrefixType::UNION_PACKED, nMembers});
            लागि (अचर AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_गाँठो_DType* अचर subtypep = itemp->subDTypep()->skipRefToEnump();
                m_traName = itemp->prettyName();
                iterate(subtypep);
            }
            addToSubFunc(नूतन Ast_Trace_झिक_Prefix{flp});
        }
    }
    शून्य visit(AstBasicDType* nodep) override {
        यदि (!m_traVscp) फर्काऊ;
        यदि (nodep->isString()) {
            addIgnore("Unsupported: strings");
        } अन्यथा {
            addTraceDecl(VNumRange{}, 0);
        }
    }
    शून्य visit(AstEnumDType* nodep) override { iterate(nodep->skipRefp()); }
    शून्य visit(Ast_गाँठो_DType*) override {
        // Note more specific dtypes above
        यदि (!m_traVscp) फर्काऊ;
        addIgnore("Unsupported: data type");
    }

    //--------------------
    शून्य visit(Ast_गाँठो* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    explicit TraceDeclVisitor(AstNetlist* nodep)
        : m_शीर्ष_Scopep{nodep->शीर्ष_Scopep()} {
        File_पङ्क्ति* अचर flp = nodep->fileline();

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
        remove_Redundant_Prefix_थप_झिक();

        अचर std::vector<AstCFunc*>& शीर्ष_Scope_Funcps = m_scopeInitFuncps.at(m_शीर्ष_Scopep->scopep());
        AstCFunc* जरा_Funcp = nullptr;
        यदि (!v3Global.opt.libCreate().empty()) {
            जरा_Funcp = newCFunc(flp, "trace_init_root");
            जरा_Funcp->entryPoint(सत्य);
            लागि (माप_प्रकार i = 0; i < m_शीर्ष_Scope_जरा_Func_गन्ती; ++i) {
                AstCCall* अचर callp = नूतन AstCCall{flp, शीर्ष_Scope_Funcps.at(i)};
                callp->dtypeSetVoid();
                callp->argTypes("tracep");
                जरा_Funcp->addStmtsp(callp->makeStmt());
            }
            यदि (!m_शीर्ष_Scope_जरा_Func_गन्ती) जरा_Funcp->addStmtsp(नूतन AstComment{flp, "Empty"});
        }

        // Call the non-wrapper initialization functions of the root scope from the top function
        लागि (माप_प्रकार i = m_शीर्ष_Scope_जरा_Func_गन्ती; i < शीर्ष_Scope_Funcps.size(); ++i) {
            AstCFunc* अचर funcp = शीर्ष_Scope_Funcps.at(i);
            AstCCall* अचर callp = नूतन AstCCall{flp, funcp};
            callp->dtypeSetVoid();
            callp->argTypes("tracep");
            add_To_शीर्ष_Func(callp->makeStmt());
        }

        // Ensure a top function exists, in case there was nothing to trace at all
        यदि (m_शीर्ष_Funcps.empty()) add_To_शीर्ष_Func(नूतन AstComment{flp, "Empty"});

        // Create single top level function, if more than one exists
        यदि (m_शीर्ष_Funcps.size() > 1) {
            AstCFunc* अचर शीर्ष_Funcp = newCFunc(flp, "");
            लागि (AstCFunc* funcp : m_शीर्ष_Funcps) {
                AstCCall* अचर callp = नूतन AstCCall{flp, funcp};
                callp->dtypeSetVoid();
                callp->argTypes("tracep");
                शीर्ष_Funcp->addStmtsp(callp->makeStmt());
            }
            m_शीर्ष_Funcps.clear();
            m_शीर्ष_Funcps.थप_back(शीर्ष_Funcp);
        }

        // Set name of top level function
        AstCFunc* अचर शीर्ष_Funcp = m_शीर्ष_Funcps.front();
        शीर्ष_Funcp->name("trace_init_top");
        शीर्ष_Funcp->entryPoint(सत्य);

        यदि (जरा_Funcp && v3Global.opt.debugCheck()) checkCallsRecurse(जरा_Funcp);
        checkCalls(शीर्ष_Funcp);
    }
    ~TraceDeclVisitor() override {
        V3Stats::addStat("Tracing, Traced signals", m_statSigs);
        V3Stats::addStat("Tracing, Ignored signals", m_statIgnSigs);
    }
};

//######################################################################
// Trace class functions

शून्य V3TraceDecl::traceDeclAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TraceDeclVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_रूख("tracedecl", 0, dump_रूख_Either_Level() >= 3);
}
