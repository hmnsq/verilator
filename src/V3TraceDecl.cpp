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

वर्ग मार्गः_Adjustor final {
    File_पङ्क्तिः* ध्रुव m_flp;  // FileLine used for created nodes
    std::function<शून्य(Ast_ग्रन्थिः_Stmt*)> m_emit;  // Function called with adjustment statements
    std::सदिशम्<std::string> m_राशिः{""};  // Stack of current paths

    स्थिर constexpr वर्ण SEPARATOR = ' ';

सार्वजनिक:
    स्पष्ट मार्गः_Adjustor(File_पङ्क्तिः* flp, std::function<शून्य(Ast_ग्रन्थिः_Stmt*)> emit)
        : m_flp{flp}
        , m_emit{emit} {}

    // Emit Prefix adjustments until the current path is 'newPath'
    शून्य adjust(ध्रुव string& new_मार्गः, AstCell* cellp, AstVarScope* vscp) {
        // Move up to enclosing path
        यावत् (!VString::startsWith(new_मार्गः, m_राशिः.पृष्ठभागः())) {
            m_emit(नूतन Ast_Trace_निष्कासय_उपसर्गः{m_flp});
            m_राशिः.निष्कासय_पृष्ठभागः();
        }

        यदि (new_मार्गः == m_राशिः.पृष्ठभागः()) प्रत्यागम;

        ध्रुव VTrace_उपसर्गः_Type चरमः_Scope_Type =  //
            (cellp && VN_IS(cellp->modp(), Iface))
                    || (vscp && VN_IS(vscp->dtypep(), IfaceRefDType))
                ? VTrace_उपसर्गः_Type::SCOPE_INTERFACE
                : VTrace_उपसर्गः_Type::SCOPE_MODULE;
        ध्रुव std::string extra_उपसर्गः = new_मार्गः.substr(m_राशिः.पृष्ठभागः().आकार());
        आकार_t begin = 0;
        ध्रुव आकार_t चरमः = extra_उपसर्गः.rfind(SEPARATOR);
        // Move down, one path element at a time
        यावत् (सत्य) {
            ध्रुव आकार_t अन्तः_2 = extra_उपसर्गः.find(SEPARATOR, begin);
            यदि (अन्तः_2 == string::npos) भङ्ग;
            ध्रुव string& extra = extra_उपसर्गः.substr(begin, अन्तः_2 - begin);
            यदि (अन्तः_2 == चरमः) {
                m_emit(नूतन Ast_Trace_प्रक्षिप_उपसर्गः{m_flp, extra, चरमः_Scope_Type});
            } अन्यथा {
                m_emit(नूतन Ast_Trace_प्रक्षिप_उपसर्गः{m_flp, extra, VTrace_उपसर्गः_Type::SCOPE_MODULE});
            }
            m_राशिः.प्रक्षिप_पृष्ठभागः(m_राशिः.पृष्ठभागः() + extra + SEPARATOR);
            begin = अन्तः_2 + 1;
        }
        UASSERT(begin == extra_उपसर्गः.आकार(), "Should have consumed all of extraPrefix");
    }

    // Emit Prefix adjustments to unwind the path back to its original state
    शून्य unwind() {
        यावत् (m_राशिः.आकार() > 1) {
            m_emit(नूतन Ast_Trace_निष्कासय_उपसर्गः{m_flp});
            m_राशिः.निष्कासय_पृष्ठभागः();
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
    Ast_शिखरम्_Scope* ध्रुव m_शिखरम्_Scopep;  // The singleton AstTopScope
    ध्रुव AstScope* m_currScopep = शून्यसूचकः;  // Current scope being visited

    std::सदिशम्<AstCFunc*> m_शिखरम्_Funcps;  // Top level trace initialization functions
    std::सदिशम्<AstCFunc*> m_subFuncps;  // Trace sub functions for this scope
    std::समुच्चयः<ध्रुव AstTraceDecl*> m_declUncalledps;  // Declarations not called
    // Functions per type and variable kind (wire vs logic etc.)
    संरचना Dtype_Func_कुञ्चिका final {
        ध्रुव Ast_ग्रन्थिः_DType* dtypep;
        VVarType varType;
        तार्किक संक्रियकः==(ध्रुव Dtype_Func_कुञ्चिका& other) ध्रुव {
            प्रत्यागम dtypep == other.dtypep && varType == other.varType;
        }
    };
    संरचना Dtype_Func_कुञ्चिका_Hash final {
        आकार_t संक्रियकः()(ध्रुव Dtype_Func_कुञ्चिका& कुञ्चिका) ध्रुव {
            प्रत्यागम std::hash<ध्रुव Ast_ग्रन्थिः_DType*>{}(कुञ्चिका.dtypep)
                   ^ (std::hash<uint8_t>{}(कुञ्चिका.varType) << 1);
        }
    };
    std::unordered_map<Dtype_Func_कुञ्चिका, AstCFunc*, Dtype_Func_कुञ्चिका_Hash>
        m_dtypeFuncs;  // Functions per type+kind
    AstCFunc* m_dtypeFuncp = शून्यसूचकः;  // Current type func
    AstCFunc* m_dtypeSubFuncp = शून्यसूचकः;  // Current type sub func
    ध्रुव string m_dtype_तर्काः{", const char* name, uint32_t fidx, uint32_t c, "
                             "VerilatedTraceSigDirection direction"};  // Type func args
    AstTraceDecl* m_dtypeDeclp = शून्यसूचकः;  // Current type func decl
    V3_Unique_नामानि m_dtype_नामानि{""};  // Unique names for dtype funcs
    तार्किक m_skipDtypeFunc = असत्य;  // Don't create a type func
    uint32_t m_विस्थापनम् = std::numeric_limits<uint32_t>::max();  // Offset for types
    पूर्णाङ्क m_शिखरम्_Func_आकार = 0;  // Size of the top function currently being built
    पूर्णाङ्क m_sub_Func_आकार = 0;  // Size of the sub function currently being built
    तार्किक m_force_New_पर्णम् = असत्य;  // Force next addToSubFunc to start a fresh leaf
    आकार_t m_शिखरम्_Scope_मूलम्_Func_गणनम् = 0;  // Top-scope init functions used only for wrapper IOs
    तार्किक m_शिखरम्_Scope_मूलम्_Phase = असत्य;  // Emitting top-scope wrapper IO declarations
    ध्रुव पूर्णाङ्क m_func_आकार_सीमा  // Maximum size of a function
        = v3Global.opt.output_विभज_CTrace() ? v3Global.opt.output_विभज_CTrace()
                                           : std::numeric_limits<पूर्णाङ्क>::max();
    // Trace init functions to for each scope
    std::unordered_map<ध्रुव AstScope*, std::सदिशम्<AstCFunc*>> m_scopeInitFuncps;
    // Map from hierarchical scope name to the corresponding AstScope. Note that
    // this is a many-to-one mapping for interfaces, due to interface refs.
    std::unordered_map<std::string, ध्रुव AstScope*> m_मार्गः_गन्तव्यम्_Scopep;
    // Cell initialization placeholders:
    // (parent scope, cell under parent scope, statement)
    std::सदिशम्<std::tuple<AstScope*, AstCell*, Ast_ग्रन्थिः_Stmt*>> m_cellInitPlaceholders;
    // Interface refs initialization placeholders:
    // (Interface ref variable, placeholder statement)
    std::सदिशम्<std::tuple<AstVarScope*, Ast_ग्रन्थिः_Stmt*>> m_ifaceRefInitPlaceholders;

    // A trace entry under a scope is either:
    // - A variable (including interface references)
    // - A sub scope (stored as the cell corresponding to the sub scope)
    // Note: members are non-const to allow copy during sorting
    वर्ग Trace_प्रविष्टिः final {
        AstVarScope* m_vscp = शून्यसूचकः;  // AstVarScope under scope being traced
        AstCell* m_cellp = शून्यसूचकः;  // Sub scope (as AstCell) under scope being traced
        std::string m_मार्गः;  // Path to enclosing module in original hierarchy
        std::string m_name;  // Name of signal/subscope

        शून्य init(ध्रुव std::string& name, Ast_ग्रन्थिः* nodep, तार्किक अन्तर्_शिखरम्_Scope) {
            // Compute path in hierarchy and item name
            ध्रुव std::string& vcdName = Ast_ग्रन्थिः::vcdName(name);
            AstVar* ध्रुव varp = VN_CAST(nodep, Var);
            यदि (VN_IS(nodep, Cell) || VN_IS(varp->dtypep(), IfaceRefDType)) {
                // Cell or interface reference
                m_मार्गः = vcdName + " ";
                m_name.clear();
            } अन्यथा यदि (varp->isPrimaryIO()) {
                // Primary IO variable
                m_मार्गः = "$rootio ";
                m_name = vcdName;
            } अन्यथा {
                // Other Variable
                ध्रुव आकार_t pos = vcdName.rfind(' ');
                ध्रुव आकार_t मार्गः_Len = pos == std::string::npos ? 0 : pos + 1;
                m_मार्गः = vcdName.substr(0, मार्गः_Len);
                m_name = vcdName.substr(मार्गः_Len);
            }

            // When creating a --lib-create library, drop the name of the selected top module.
            // This will be replaced by the instance name in the model that uses the library, or
            // restored at runtime if the library itself is traced as the root model. Other top
            // level entities ($unit, packages, ...) keep a '$libroot' wrapper so they still have
            // a stable location in the dump.
            यदि (अन्तर्_शिखरम्_Scope && !v3Global.opt.libCreate().रिक्तानि()) {
                ध्रुव आकार_t start = m_मार्गः.find(' ');
                // Must have a prefix in the top scope with lib, as top wrapper signals not traced
                UASSERT_OBJ(start != std::string::npos, nodep, "No prefix with --lib-create");
                ध्रुव std::string उपसर्गः = m_मार्गः.substr(0, start);
                // Wrapper primary IOs stay under $rootio so a root-traced library can restore
                // them under the runtime C++ instance name without affecting embedded use.
                यदि (उपसर्गः == "$rootio") प्रत्यागम;
                m_मार्गः = m_मार्गः.substr(start + 1);
                यदि (v3Global.rootp()->trace_Lib_शिखरम्_Name() != उपसर्गः) m_मार्गः = "$libroot " + m_मार्गः;
            }
        }

    सार्वजनिक:
        स्पष्ट Trace_प्रविष्टिः(ध्रुव AstScope* scopep, AstVarScope* vscp)
            : m_vscp{vscp} {
            init(vscp->varp()->name(), vscp->varp(), scopep->is_शिखरम्());
        }
        स्पष्ट Trace_प्रविष्टिः(ध्रुव AstScope* scopep, AstCell* cellp)
            : m_cellp{cellp} {
            init(cellp->name(), cellp, scopep->is_शिखरम्());
        }
        पूर्णाङ्क operatorCompare(ध्रुव Trace_प्रविष्टिः& b) ध्रुव {
            यदि (ध्रुव पूर्णाङ्क cmp = मार्गः().compare(b.मार्गः())) प्रत्यागम cmp < 0;
            यदि (ध्रुव पूर्णाङ्क cmp = fileline().operatorCompare(b.fileline())) प्रत्यागम cmp < 0;
            प्रत्यागम name() < b.name();
        }
        AstVarScope* vscp() ध्रुव { प्रत्यागम m_vscp; }
        AstCell* cellp() ध्रुव { प्रत्यागम m_cellp; }
        ध्रुव std::string& मार्गः() ध्रुव { प्रत्यागम m_मार्गः; }
        शून्य मार्गः(ध्रुव std::string& मार्गः) { m_मार्गः = मार्गः; }
        ध्रुव std::string& name() ध्रुव { प्रत्यागम m_name; }
        File_पङ्क्तिः& fileline() ध्रुव { प्रत्यागम m_vscp ? *m_vscp->fileline() : *m_cellp->fileline(); }
    };
    std::सदिशम्<Trace_प्रविष्टिः> m_entries;  // Trace entries under current scope
    AstVarScope* m_traVscp = शून्यसूचकः;  // Current AstVarScope we are constructing AstTraceDecls for
    Ast_ग्रन्थिः_व्यञ्जकम्* m_traValuep = शून्यसूचकः;  // Value expression for current signal
    string m_traName;  // Name component for current signal

    VDouble0 m_statSigs;  // Statistic tracking
    VDouble0 m_statIgnSigs;  // Statistic tracking

    // METHODS

    string vscIgnoreTrace(ध्रुव AstVarScope* nodep) {
        // Return true if this shouldn't be traced
        // See also similar rule in V3Coverage::varIgnoreToggle
        ध्रुव AstVar* ध्रुव varp = nodep->varp();
        यदि (!varp->isTrace()) प्रत्यागम "Verilator trace_off";
        यदि (!nodep->isTrace()) प्रत्यागम "Verilator instance trace_off";
        // Automatics (typically, excluding forks) have no persistance over
        // time, and may optimize differently when multithreadeded or hierarchical.
        // Class automatics refer to being in a class but might still be pointed
        // to by a static, so are ok.
        यदि (varp->lifetime().isAutomatic() && !varp->isClassMember() && !varp->isParam())
            प्रत्यागम "Automatic variable";

        ध्रुव पूर्णाङ्क विस्तारः = recurse_DType_विस्तारः(nodep->varp()->dtypep());
        यदि (v3Global.opt.trace_Max_विस्तारः() && विस्तारः > v3Global.opt.trace_Max_विस्तारः())
            प्रत्यागम "Width " + cvt_गन्तव्यम्_Str(विस्तारः) + " > --trace-max-width";

        ध्रुव string prettyName = nodep->prettyName();
        यदि (!V3Control::getScopeTraceOn(prettyName)) प्रत्यागम "Vlt scope trace_off";
        यदि (!v3Global.opt.traceUnderscore()) {
            यदि (!prettyName.रिक्तानि() && prettyName[0] == '_') प्रत्यागम "Leading underscore";
            यदि (prettyName.find("._") != string::npos) प्रत्यागम "Inlined leading underscore";
        }
        प्रत्यागम ""s;
    }

    पूर्णाङ्क recurse_DType_विस्तारः(ध्रुव Ast_ग्रन्थिः_DType* nodep) {
        यदि (ध्रुव Ast_ग्रन्थिः_Array_DType* adtypep = VN_CAST(nodep, ग्रन्थिः_Array_DType))
            प्रत्यागम recurse_DType_विस्तारः(adtypep->subDTypep()) * adtypep->declRange().elements();
        प्रत्यागम nodep->विस्तारः();
    }

    AstCFunc* newCFunc(File_पङ्क्तिः* flp, ध्रुव string& name) {
        AstScope* ध्रुव शिखरम्_Scopep = m_शिखरम्_Scopep->scopep();
        AstCFunc* ध्रुव funcp = नूतन AstCFunc{flp, name, शिखरम्_Scopep};
        funcp->argTypes(v3Global.opt.trace_Class_आधारः() + "* tracep");
        funcp->isTrace(सत्य);
        funcp->isStatic(असत्य);
        funcp->isLoose(सत्य);
        funcp->slow(सत्य);
        शिखरम्_Scopep->सङ्कलय_Blocksp(funcp);
        प्रत्यागम funcp;
    }

    शून्य सङ्कलय_गन्तव्यम्_शिखरम्_Func(Ast_ग्रन्थिः_Stmt* stmtp) {
        यदि (m_शिखरम्_Func_आकार > m_func_आकार_सीमा || m_शिखरम्_Funcps.रिक्तानि()) {
            m_शिखरम्_Func_आकार = 0;
            //
            ध्रुव string n = cvt_गन्तव्यम्_Str(m_शिखरम्_Funcps.आकार());
            ध्रुव string name{"trace_init_top__" + n};
            AstCFunc* ध्रुव funcp = newCFunc(m_शिखरम्_Scopep->fileline(), name);
            m_शिखरम्_Funcps.प्रक्षिप_पृष्ठभागः(funcp);
        }
        m_शिखरम्_Funcps.पृष्ठभागः()->सङ्कलय_Stmtsp(stmtp);
        m_शिखरम्_Func_आकार += stmtp->ग्रन्थिः_गणनम्();
    }

    शून्य सङ्कलय_गन्तव्यम्_Sub_Func(Ast_ग्रन्थिः_Stmt* stmtp) {
        // TODO (maybe) -- sub funcs for dtype components
        यदि (m_dtypeSubFuncp) {
            यदि (m_sub_Func_आकार > m_func_आकार_सीमा) newDeclSubFunc();
            m_dtypeSubFuncp->सङ्कलय_Stmtsp(stmtp);
            m_sub_Func_आकार += stmtp->ग्रन्थिः_गणनम्();
            प्रत्यागम;
        }
        // Defer trace splitting until V3Trace
        यदि (m_subFuncps.रिक्तानि() || m_force_New_पर्णम्) {
            m_force_New_पर्णम् = असत्य;
            File_पङ्क्तिः* ध्रुव flp = m_currScopep->fileline();
            ध्रुव string n = cvt_गन्तव्यम्_Str(m_subFuncps.आकार());
            ध्रुव string name
                = m_currScopep == m_शिखरम्_Scopep->scopep() && !v3Global.opt.libCreate().रिक्तानि()
                      ? (m_शिखरम्_Scope_मूलम्_Phase ? "trace_init_leaf_root__" : "trace_init_leaf_top__")
                            + n
                      : "trace_init_sub__" + m_currScopep->nameDotless() + "__" + n;
            AstCFunc* ध्रुव funcp = newCFunc(flp, name);
            funcp->सङ्कलय_Stmtsp(नूतन AstCStmt{flp, "const int c = vlSymsp->__Vm_baseCode;"});
            m_subFuncps.प्रक्षिप_पृष्ठभागः(funcp);
        }
        m_subFuncps.पृष्ठभागः()->सङ्कलय_Stmtsp(stmtp);
        m_sub_Func_आकार += stmtp->ग्रन्थिः_गणनम्();
    }

    AstTraceDecl* सङ्कलय_Trace_Decl(ध्रुव VNumRange& arrayRange,
                               पूर्णाङ्क विस्तारः_Override,  // If !=0, is packed struct/array where basicp
                                                   // size misreflects one element
                               AstCCall* ध्रुव dtypeCallp = शून्यसूचकः) {
        VNumRange bitRange;
        यदि (विस्तारः_Override) {
            bitRange = VNumRange{विस्तारः_Override - 1, 0};
        } अन्यथा यदि (ध्रुव AstBasicDType* ध्रुव bdtypep = m_traValuep->dtypep()->basicp()) {
            bitRange = bdtypep->nrange();
        }
        File_पङ्क्तिः* ध्रुव flp = m_traVscp->fileline();
        Ast_ग्रन्थिः_व्यञ्जकम्* valuep = m_traValuep->clone_वृक्षः(असत्य);
        ध्रुव तार्किक valid_विस्थापनम् = m_विस्थापनम् != std::numeric_limits<uint32_t>::max();
        AstTraceDecl* ध्रुव newp
            = नूतन AstTraceDecl{flp,      m_traName,  m_traVscp->varp(), valuep,
                               bitRange, arrayRange, dtypeCallp,        valid_विस्थापनम्};
        यदि (valid_विस्थापनम्) {
            newp->संहिता(m_विस्थापनम्);
            यदि (!dtypeCallp) { m_विस्थापनम् += newp->संहिता_Inc(); }
            valuep->foreach([&](AstVarRef* ध्रुव refp) {
                UASSERT_OBJ(refp->varScopep() == m_traVscp, refp,
                            "Trace decl expression references unexpected var");
                refp->replaceWith(नूतन AstCExpr{flp, "__VdtypeVar", m_traVscp->विस्तारः()});
                VL_DO_DANGLING(refp->delete_वृक्षः(), refp);
            });
        } अन्यथा {
            newp->dtypeDeclp(m_dtypeDeclp);
        }
        m_declUncalledps.emplace(newp);
        सङ्कलय_गन्तव्यम्_Sub_Func(newp);

        प्रत्यागम newp;
    }

    शून्य सङ्कलय_Ignore(ध्रुव string& why) {
        ++m_statIgnSigs;
        ध्रुव std::string cmt = "Tracing: "s + m_traName + " // Ignored: " + why;
        यदि (debug() > 3 && m_traVscp) std::निर्गमः << "- " << m_traVscp->fileline() << cmt << '\n';
    }

    शून्य fixupPlaceholder(ध्रुव std::string& मार्गः, Ast_ग्रन्थिः_Stmt* placeholderp) {
        // Find the scope for the path. As we are working based on cell names,
        // it is possible there is no corresponding scope (e.g.: for an empty
        // module).
        ध्रुव स्वचालित it = m_मार्गः_गन्तव्यम्_Scopep.find(Ast_ग्रन्थिः::prettyName(मार्गः));
        यदि (it != m_मार्गः_गन्तव्यम्_Scopep.अन्तः_2()) {
            ध्रुव AstScope* ध्रुव scopep = it->second;
            File_पङ्क्तिः* ध्रुव flp = placeholderp->fileline();

            // Call the initialization functions for the scope
            Ast_ग्रन्थिः* stmtp = शून्यसूचकः;
            प्रति (AstCFunc* ध्रुव subFuncp : m_scopeInitFuncps.at(scopep)) {
                AstCCall* ध्रुव callp = नूतन AstCCall{flp, subFuncp};
                callp->dtype_समुच्चयः_Void();
                callp->argTypes("tracep");
                stmtp = Ast_ग्रन्थिः::सङ्कलय_अग्र(stmtp, callp->makeStmt());
            }

            // Add after the placeholder
            यदि (stmtp) placeholderp->सङ्कलय_अग्र_अत्र(stmtp);
        }
        // Delete the placeholder
        placeholderp->unlink_Fr_पृष्ठभागः();
        VL_DO_DANGLING(placeholderp->delete_वृक्षः(), placeholderp);
    }

    शून्य fixupLibStub(ध्रुव std::string& मार्गः, Ast_ग्रन्थिः_Stmt* placeholderp) {
        File_पङ्क्तिः* ध्रुव flp = placeholderp->fileline();

        // The library instance's model is named after its %m within the parent model,
        // which does not include the "l2-name" (top module) of the library
        std::string name = Ast_ग्रन्थिः::prettyName(मार्गः);
        यदि (!v3Global.opt.libCreate().रिक्तानि()) {
            ध्रुव std::string शिखरम्_उपसर्गः = v3Global.rootp()->trace_Lib_शिखरम्_Name() + ".";
            यदि (VString::startsWith(name, शिखरम्_उपसर्गः)) name = name.substr(शिखरम्_उपसर्गः.आकार());
        }

        // Call the initialization function for the library instance
        AstCStmt* ध्रुव initp = नूतन AstCStmt{flp};
        initp->सङ्कलय("{\n");
        initp->सङ्कलय("std::string __VlibName = vlSymsp->name();\n");
        initp->सङ्कलय("if (!__VlibName.empty()) __VlibName += '.';\n");
        initp->सङ्कलय("__VlibName += ");
        initp->सङ्कलय(नूतन AstConst{flp, AstConst::String{}, name});
        initp->सङ्कलय(";\n");
        initp->सङ्कलय("tracep->initLib(__VlibName);\n");
        initp->सङ्कलय("}\n");

        placeholderp->सङ्कलय_अग्र_अत्र(initp);
        // Delete the placeholder
        VL_DO_DANGLING(placeholderp->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), placeholderp);
    }

    शून्य fixupPlaceholders() {
        // Fix up cell initialization placehodlers
        UINFO(9, "fixupPlaceholders()");
        प्रति (ध्रुव स्वचालित& item : m_cellInitPlaceholders) {
            ध्रुव AstScope* ध्रुव parentp = std::get<0>(item);
            ध्रुव AstCell* ध्रुव cellp = std::get<1>(item);
            Ast_ग्रन्थिः_Stmt* ध्रुव placeholderp = std::get<2>(item);
            ध्रुव std::string मार्गः = parentp->name() + "__DOT__" + cellp->name();
            यदि (cellp->modp()->verilatorLib()) {
                fixupLibStub(मार्गः, placeholderp);
            } अन्यथा {
                fixupPlaceholder(मार्गः, placeholderp);
            }
        }

        // Fix up interface reference initialization placeholders
        प्रति (ध्रुव स्वचालित& item : m_ifaceRefInitPlaceholders) {
            ध्रुव AstVarScope* ध्रुव vscp = std::get<0>(item);
            Ast_ग्रन्थिः_Stmt* ध्रुव placeholderp = std::get<1>(item);
            ध्रुव std::string मार्गः = vscp->scopep()->name() + "__DOT__" + vscp->varp()->name();
            fixupPlaceholder(मार्गः, placeholderp);
        }
    }

    शून्य remove_Redundant_उपसर्गः_प्रक्षिप_निष्कासय() {
        प्रति (ध्रुव स्वचालित& pair : m_scopeInitFuncps) {
            // cppcheck-suppress constVariablePointer
            प्रति (AstCFunc* ध्रुव funcp : pair.second) {
                Ast_ग्रन्थिः* prevp = शून्यसूचकः;
                Ast_ग्रन्थिः* currp = funcp->stmtsp();
                यावत् (currp) {
                    Ast_ग्रन्थिः* ध्रुव nextp = currp->nextp();
                    यदि (VN_IS(prevp, Trace_प्रक्षिप_उपसर्गः) && VN_IS(currp, Trace_निष्कासय_उपसर्गः)) {
                        VL_DO_DANGLING(prevp->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), prevp);
                        VL_DO_DANGLING(currp->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), currp);
                    }
                    यदि (!nextp) भङ्ग;
                    prevp = nextp->backp();
                    currp = nextp;
                }
            }
        }
    }

    शून्य checkCalls(ध्रुव AstCFunc* funcp) {
        यदि (!v3Global.opt.debugCheck()) प्रत्यागम;
        checkCallsRecurse(funcp);
        यदि (!m_declUncalledps.रिक्तानि()) {  // LCOV_EXCL_START
            प्रति (स्वचालित tracep : m_declUncalledps) UINFO(0, "-nodep " << tracep);
            (*(m_declUncalledps.begin()))->v3fatalSrc("Created TraceDecl which is never called");
        }  // LCOV_EXCL_STOP
    }
    शून्य checkCallsRecurse(ध्रुव AstCFunc* funcp) {
        funcp->foreach([एतत्](ध्रुव Ast_ग्रन्थिः* nodep) {
            यदि (ध्रुव AstTraceDecl* ध्रुव declp = VN_CAST(nodep, TraceDecl)) {
                m_declUncalledps.erase(declp);
            } अन्यथा यदि (ध्रुव AstCCall* ध्रुव ccallp = VN_CAST(nodep, CCall)) {
                checkCallsRecurse(ccallp->funcp());
            }
        });
    }

    तार्किक isBasicIO() { प्रत्यागम m_traVscp->varp()->isVLIO(); }
    शून्य newDeclSubFunc() {
        File_पङ्क्तिः* ध्रुव flp = m_dtypeFuncp->fileline();
        AstCFunc* ध्रुव subFuncp = newCFunc(flp, m_dtype_नामानि.get("trace_init_dtype_sub__"));
        subFuncp->argTypes(subFuncp->argTypes() + m_dtype_तर्काः);
        AstCCall* ध्रुव subCallp = नूतन AstCCall{flp, subFuncp};
        subCallp->dtype_समुच्चयः_Void();
        subCallp->argTypes("tracep, name, fidx, c, direction");
        m_dtypeFuncp->सङ्कलय_Stmtsp(subCallp->makeStmt());
        m_dtypeSubFuncp = subFuncp;
        m_sub_Func_आकार = 0;
    }
    शून्य newDeclFunc(Ast_ग्रन्थिः_DType* nodep) {
        Ast_ग्रन्थिः_DType* ध्रुव skipTypep = nodep->skipRefp();
        // offset and direction args added in EmitCImp
        std::string call_तर्काः{"tracep, \"" + VIdProtect::protect(m_traName) + "\""};
        VL_RESTORER_COPY(m_traName);
        File_पङ्क्तिः* ध्रुव flp = skipTypep->fileline();

        ध्रुव Dtype_Func_कुञ्चिका dtype_कुञ्चिका{skipTypep, m_traVscp->varp()->varType()};
        स्वचालित pair = m_dtypeFuncs.emplace(dtype_कुञ्चिका, शून्यसूचकः);
        AstCFunc** funcpp = &pair.first->second;
        यदि (pair.second) {
            *funcpp = newCFunc(flp, m_dtype_नामानि.get("trace_init_dtype__"));
            (*funcpp)->argTypes((*funcpp)->argTypes() + m_dtype_तर्काः);
            (*funcpp)->user2p(m_traVscp);
        }

        AstCCall* ध्रुव callp = नूतन AstCCall{flp, *funcpp};
        callp->dtype_समुच्चयः_Void();
        callp->argTypes(call_तर्काः);
        m_dtypeDeclp = सङ्कलय_Trace_Decl(VNumRange{}, skipTypep->विस्तारः(), callp);
        सङ्कलय_गन्तव्यम्_Sub_Func(callp->makeStmt());

        यदि (pair.second) {
            VL_RESTORER(m_विस्थापनम्);
            m_विस्थापनम् = 0;

            VL_RESTORER(m_dtypeFuncp);
            VL_RESTORER(m_dtypeSubFuncp);
            VL_RESTORER(m_sub_Func_आकार);
            m_dtypeFuncp = *funcpp;
            newDeclSubFunc();
            यदि (AstStructDType* ध्रुव dtypep = VN_CAST(skipTypep, StructDType)) {
                declStruct(dtypep, सत्य);
            } अन्यथा यदि (AstUnpackArrayDType* ध्रुव dtypep = VN_CAST(skipTypep, UnpackArrayDType)) {
                declUnpackedArray(dtypep, सत्य);
            } अन्यथा यदि (AstPackArrayDType* ध्रुव dtypep = VN_CAST(skipTypep, PackArrayDType)) {
                declPackedArray(dtypep, सत्य);
            } अन्यथा {
                UASSERT_OBJ(असत्य, skipTypep, "Creating a trace function for an unexpected type");
            }
            m_dtypeFuncp->user1(m_विस्थापनम्);
        }

        m_dtypeDeclp->संहिता_Inc((*funcpp)->user1());
        m_विस्थापनम् += m_dtypeDeclp->संहिता_Inc();
    }
    शून्य declUnpackedArray(AstUnpackArrayDType* ध्रुव nodep, तार्किक newFunc) {
        string उपसर्गः_Name(newFunc ? "name" : m_traName);

        VL_RESTORER_COPY(m_traName);
        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();

        सङ्कलय_गन्तव्यम्_Sub_Func(नूतन Ast_Trace_प्रक्षिप_उपसर्गः{flp, उपसर्गः_Name, VTrace_उपसर्गः_Type::ARRAY_UNPACKED,
                                            nodep->वामतः(), nodep->दक्षिणतः(), !newFunc});

        यदि (VN_IS(nodep->subDTypep()->skip_Ref_गन्तव्यम्_Enump(),
                  BasicDType)  // Nothing lower than this array
            && m_traVscp->dtypep()->skip_Ref_गन्तव्यम्_Enump() == nodep) {  // Nothing above this array
            // Simple 1-D array, use existing V3EmitC runtime loop rather than unrolling
            // This will put "(index)" at end of signal name for us
            यदि (m_traVscp->dtypep()->skip_Ref_गन्तव्यम्_Enump()->isString()) {
                सङ्कलय_Ignore("Unsupported: strings");
            } अन्यथा {
                m_traName = "";
                सङ्कलय_Trace_Decl(nodep->declRange(), 0);
            }
        } अन्यथा {
            Ast_ग्रन्थिः_DType* ध्रुव subtypep = nodep->subDTypep()->skip_Ref_गन्तव्यम्_Enump();
            // Always iterate left index to right index
            प्रति (ध्रुव पूर्णाङ्क i : nodep->declRange().seq_वामः_गन्तव्यम्_दक्षिणः()) {
                VL_RESTORER(m_traValuep);
                m_traName = '[' + std::गन्तव्यम्_string(i) + ']';
                m_traValuep = m_traValuep->clone_वृक्षः(असत्य);
                m_traValuep = नूतन AstArraySel{flp, m_traValuep, i - nodep->lo()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_वृक्षः(), m_traValuep);
            }
        }

        सङ्कलय_गन्तव्यम्_Sub_Func(नूतन Ast_Trace_निष्कासय_उपसर्गः{flp});
    }
    शून्य declPackedArray(AstPackArrayDType* ध्रुव nodep, तार्किक newFunc) {
        string उपसर्गः_Name(newFunc ? "name" : m_traName);
        Ast_ग्रन्थिः_DType* ध्रुव subtypep = nodep->subDTypep()->skip_Ref_गन्तव्यम्_Enump();

        VL_RESTORER_COPY(m_traName);
        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();

        सङ्कलय_गन्तव्यम्_Sub_Func(नूतन Ast_Trace_प्रक्षिप_उपसर्गः{flp, उपसर्गः_Name, VTrace_उपसर्गः_Type::ARRAY_PACKED,
                                            nodep->वामतः(), nodep->दक्षिणतः(), !newFunc});

        // Always iterate left index to right index
        प्रति (ध्रुव पूर्णाङ्क i : nodep->declRange().seq_वामः_गन्तव्यम्_दक्षिणः()) {
            VL_RESTORER(m_traValuep);
            m_traName = '[' + std::गन्तव्यम्_string(i) + ']';
            ध्रुव पूर्णाङ्क lsb = (i - nodep->lo()) * subtypep->विस्तारः();
            m_traValuep = m_traValuep->clone_वृक्षः(असत्य);
            m_traValuep = नूतन AstSel{flp, m_traValuep, lsb, subtypep->विस्तारः()};
            m_traValuep->dtypep(subtypep);
            iterate(subtypep);
            VL_DO_CLEAR(m_traValuep->delete_वृक्षः(), m_traValuep = शून्यसूचकः);
        }

        सङ्कलय_गन्तव्यम्_Sub_Func(नूतन Ast_Trace_निष्कासय_उपसर्गः{flp});
    }
    शून्य declStruct(AstStructDType* ध्रुव nodep, तार्किक newFunc) {
        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
        string उपसर्गः_Name(newFunc ? "name" : m_traName);
        पूर्णाङ्क nMembers = 0;
        प्रति (Ast_ग्रन्थिः* mp = nodep->membersp(); mp; mp = mp->nextp()) ++nMembers;
        यदि (!nodep->packed()) {
            सङ्कलय_गन्तव्यम्_Sub_Func(नूतन Ast_Trace_प्रक्षिप_उपसर्गः{flp, उपसर्गः_Name,  //
                                                VTrace_उपसर्गः_Type::STRUCT_UNPACKED, nMembers, 0,
                                                !newFunc});
            प्रति (ध्रुव AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_ग्रन्थिः_DType* ध्रुव subtypep = itemp->subDTypep()->skip_Ref_गन्तव्यम्_Enump();
                m_traName = itemp->prettyName();
                VL_RESTORER(m_traValuep);
                m_traValuep = m_traValuep->clone_वृक्षः(असत्य);
                m_traValuep = नूतन AstStructSel{flp, m_traValuep, itemp->name()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_वृक्षः(), m_traValuep);
            }
            सङ्कलय_गन्तव्यम्_Sub_Func(नूतन Ast_Trace_निष्कासय_उपसर्गः{flp});
        } अन्यथा {
            सङ्कलय_गन्तव्यम्_Sub_Func(नूतन Ast_Trace_प्रक्षिप_उपसर्गः{flp, उपसर्गः_Name,  //
                                                VTrace_उपसर्गः_Type::STRUCT_PACKED, nMembers, 0,
                                                !newFunc});
            प्रति (ध्रुव AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_ग्रन्थिः_DType* ध्रुव subtypep = itemp->subDTypep()->skip_Ref_गन्तव्यम्_Enump();
                m_traName = itemp->prettyName();
                VL_RESTORER(m_traValuep);
                m_traValuep = m_traValuep->clone_वृक्षः(असत्य);
                m_traValuep = नूतन AstSel{flp, m_traValuep, itemp->lsb(), subtypep->विस्तारः()};
                m_traValuep->dtypep(subtypep);
                iterate(subtypep);
                VL_DO_DANGLING(m_traValuep->delete_वृक्षः(), m_traValuep);
            }
            सङ्कलय_गन्तव्यम्_Sub_Func(नूतन Ast_Trace_निष्कासय_उपसर्गः{flp});
        }
    }

    // VISITORS
    शून्य दर्शय(AstScope* nodep) override {
        UINFO(9, "visit " << nodep);
        UASSERT_OBJ(!m_currScopep, nodep, "Should not nest");
        UASSERT_OBJ(m_subFuncps.रिक्तानि(), nodep, "Should not nest");
        UASSERT_OBJ(m_entries.रिक्तानि(), nodep, "Should not nest");
        UASSERT_OBJ(!m_traVscp, nodep, "Should not nest");
        UASSERT_OBJ(!m_traValuep, nodep, "Should not nest");
        UASSERT_OBJ(m_traName.रिक्तानि(), nodep, "Should not nest");

        // If this is a stub for a --lib-create library, skip.
        यदि (nodep->modp()->verilatorLib()) प्रत्यागम;

        VL_RESTORER(m_currScopep);
        m_currScopep = nodep;

        // Gather signals under this scope
        iterateChildrenConst(nodep);

        // Gather cells under this scope
        प्रति (Ast_ग्रन्थिः* stmtp = nodep->modp()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            यदि (AstCell* ध्रुव cellp = VN_CAST(stmtp, Cell)) m_entries.emplace_पृष्ठभागः(nodep, cellp);
        }

        यदि (!m_entries.रिक्तानि()) {
            // Sort trace entries, by enclosing instance (necessary for single traversal of
            // hierarchy during initialization), then by source location, then by name.
            std::stable_क्रमय(
                m_entries.begin(), m_entries.अन्तः_2(),
                [](ध्रुव Trace_प्रविष्टिः& a, ध्रुव Trace_प्रविष्टिः& b) { प्रत्यागम a.operatorCompare(b); });

            File_पङ्क्तिः* ध्रुव flp = nodep->fileline();
            मार्गः_Adjustor मार्गः_Adjustor_2{flp, [&](Ast_ग्रन्थिः_Stmt* stmtp) { सङ्कलय_गन्तव्यम्_Sub_Func(stmtp); }};
            ध्रुव तार्किक विभज_मूलम्_Primary_Ios = nodep->is_शिखरम्() && !v3Global.opt.libCreate().रिक्तानि();
            ध्रुव स्वचालित emit_प्रविष्टिः = [&](ध्रुव Trace_प्रविष्टिः& प्रविष्टिः) {
                AstVarScope* ध्रुव vscp = प्रविष्टिः.vscp();
                // Adjust name prefix based on path in hierarchy
                UINFO(9, "path='" << प्रविष्टिः.मार्गः() << "' name='" << प्रविष्टिः.name() << "' "
                                  << (प्रविष्टिः.cellp() ? static_cast<Ast_ग्रन्थिः*>(प्रविष्टिः.cellp())
                                                    : static_cast<Ast_ग्रन्थिः*>(प्रविष्टिः.vscp())));
                मार्गः_Adjustor_2.adjust(प्रविष्टिः.मार्गः(), प्रविष्टिः.cellp(), प्रविष्टिः.vscp());

                m_traName = प्रविष्टिः.name();

                यदि (vscp) {
                    // This is a signal: build AstTraceDecl for it
                    m_traVscp = vscp;
                    ध्रुव string& ignoreReason = vscIgnoreTrace(m_traVscp);
                    यदि (!ignoreReason.रिक्तानि()) {
                        सङ्कलय_Ignore(ignoreReason);
                    } अन्यथा {
                        ++m_statSigs;
                        // Create reference to whole signal. We will operate on this during the
                        // traversal.
                        m_traValuep
                            = नूतन AstVarRef{m_traVscp->fileline(), m_traVscp, VAccess::READ};
                        // Recurse into data type of the signal. The visit methods will add
                        // AstTraceDecls.
                        iterate(m_traVscp->varp()->dtypep()->skip_Ref_गन्तव्यम्_Enump());
                        // Delete reference created above. Traversal cloned it as required.
                        यदि (m_traValuep) {
                            VL_DO_DANGLING(m_traValuep->delete_वृक्षः(), m_traValuep);
                            // Note: Sometimes VL_DANGLING is a no-op, but we have assertions
                            // on m_traValuep being nullptr, so make sure it is.
                            m_traValuep = शून्यसूचकः;
                        }
                    }
                } अन्यथा {
                    // This is a subscope: insert a placeholder to be fixed up later
                    AstCell* ध्रुव cellp = प्रविष्टिः.cellp();
                    Ast_ग्रन्थिः_Stmt* ध्रुव stmtp = नूतन AstComment{
                        cellp->fileline(), "Instance init for: " + cellp->prettyName()};
                    सङ्कलय_गन्तव्यम्_Sub_Func(stmtp);
                    m_cellInitPlaceholders.emplace_पृष्ठभागः(nodep, cellp, stmtp);
                }
            };
            यदि (विभज_मूलम्_Primary_Ios) {
                m_शिखरम्_Scope_मूलम्_Phase = सत्य;
                प्रति (ध्रुव Trace_प्रविष्टिः& प्रविष्टिः : m_entries) {
                    AstVarScope* ध्रुव vscp = प्रविष्टिः.vscp();
                    यदि (!(vscp && vscp->varp()->isPrimaryIO())) अनुवर्त;
                    emit_प्रविष्टिः(प्रविष्टिः);
                }
                m_शिखरम्_Scope_मूलम्_Phase = असत्य;
                मार्गः_Adjustor_2.unwind();
                m_शिखरम्_Scope_मूलम्_Func_गणनम् = m_subFuncps.आकार();
                यदि (m_शिखरम्_Scope_मूलम्_Func_गणनम्) {
                    // Force the next addToSubFunc to start a fresh leaf so
                    // root-phase decls and top-phase decls live in separate
                    // functions. Without this, both phases land in the same
                    // leaf and `trace_init_top` ends up emitting both the
                    // $rootio-prefixed IOs and the regular-scope decls, so
                    // each IO gets traced twice when the lib is consumed
                    // via initLib(name).
                    m_sub_Func_आकार = 0;
                    m_force_New_पर्णम् = सत्य;
                }
            }
            प्रति (ध्रुव Trace_प्रविष्टिः& प्रविष्टिः : m_entries) {
                AstVarScope* ध्रुव vscp = प्रविष्टिः.vscp();
                यदि (विभज_मूलम्_Primary_Ios && vscp && vscp->varp()->isPrimaryIO()) अनुवर्त;
                emit_प्रविष्टिः(प्रविष्टिः);
            }
            मार्गः_Adjustor_2.unwind();
            m_traVscp = शून्यसूचकः;
            m_traName.clear();
            UASSERT_OBJ(!m_traValuep, nodep, "Should have been deleted");
            m_entries.clear();
        }

        // Save the initialization functions of this scope
        m_scopeInitFuncps.emplace(nodep, std::move(m_subFuncps));

        // Save the hierarchical name of this scope
        ध्रुव std::string मार्गः = nodep->prettyName();
        m_मार्गः_गन्तव्यम्_Scopep.emplace(मार्गः, nodep);

        // Save the hierarchical names of interface references that reference this scope
        ध्रुव AstCell* ध्रुव cellp = nodep->aboveCellp();
        यदि (cellp
            && VN_IS(cellp->modp(), Iface)
            // Exclude classes that were inside interfaces, the cell's modp is an interface,
            // but the nodep->module is the Class
            && VN_IS(nodep->modp(), Iface)) {
            ध्रुव आकार_t चरमः_Dot = मार्गः.find_चरमः_सम्बन्धि('.');
            UASSERT_OBJ(चरमः_Dot != string::npos, nodep,
                        "Expected an interface scope name to have at least one dot: " << मार्गः);
            ध्रुव std::string parent_मार्गः = मार्गः.substr(0, चरमः_Dot + 1);

            प्रति (AstIntfRef *intfRefp = cellp->intfRefsp(), *nextp; intfRefp; intfRefp = nextp) {
                nextp = VN_AS(intfRefp->nextp(), IntfRef);

                ध्रुव std::string refName = intfRefp->prettyName();

                // Assume only references under the same parent scope reference
                // the same interface.
                // TODO: This is not actually correct. An interface can propagate
                //       upwards and sideways when passed to a port via a downward
                //       hierarchical reference, which we will miss here.
                यदि (!VString::startsWith(refName, parent_मार्गः)) अनुवर्त;

                // Save the mapping from the path of the reference to the scope
                m_मार्गः_गन्तव्यम्_Scopep.emplace(refName, nodep);

                // No more need for AstIntfRef, unless V3EmitCSyms wants it for VPI
                यदि (!v3Global.opt.vpi()) {
                    VL_DO_DANGLING(intfRefp->unlink_Fr_पृष्ठभागः()->delete_वृक्षः(), intfRefp);
                }
            }
        }
    }
    शून्य दर्शय(AstVarScope* nodep) override {
        UASSERT_OBJ(m_currScopep, nodep, "AstVarScope not under AstScope");

        // Prefilter - things that get added to m_vscps will either get traced or get a comment as
        // to why they are not traced. Generally these conditions doesn't need updating, instead
        // use varp->isTrace() and/or vscIgnoreTrace.
        यदि (nodep->varp()->is_अस्थायी() && !nodep->varp()->isTrace()) प्रत्यागम;
        यदि (nodep->varp()->isClassMember()) प्रत्यागम;
        यदि (nodep->varp()->isFuncLocal()) प्रत्यागम;

        // When creating a --lib-create library ...
        यदि (!v3Global.opt.libCreate().रिक्तानि()) {
            // Ignore parameters in packages. These will be traced at the top level.
            यदि (nodep->varp()->isParam() && VN_IS(nodep->scopep()->modp(), Package)) प्रत्यागम;
        }

        // Add to traced signal list
        m_entries.emplace_पृष्ठभागः(m_currScopep, nodep);
    }

    // VISITORS - Data types when tracing
    शून्य दर्शय(AstConstDType* nodep) override {
        यदि (!m_traVscp) प्रत्यागम;
        VL_RESTORER(m_विस्थापनम्);
        VL_RESTORER(m_skipDtypeFunc);
        m_skipDtypeFunc = सत्य;
        iterate(nodep->subDTypep()->skip_Ref_गन्तव्यम्_Enump());
    }
    शून्य दर्शय(AstRefDType* nodep) override {
        यदि (!m_traVscp) प्रत्यागम;
        VL_RESTORER(m_विस्थापनम्);
        iterate(nodep->subDTypep()->skip_Ref_गन्तव्यम्_Enump());
    }
    शून्य दर्शय(AstIfaceRefDType* /*nodep*/) override {
        यदि (!m_traVscp) प्रत्यागम;
        // Insert a placeholder to be fixed up later
        File_पङ्क्तिः* ध्रुव flp = m_traVscp->fileline();
        Ast_ग्रन्थिः_Stmt* ध्रुव stmtp
            = नूतन AstComment{flp, "Interface ref init for: " + m_traVscp->prettyName()};
        सङ्कलय_गन्तव्यम्_Sub_Func(stmtp);
        m_ifaceRefInitPlaceholders.emplace_पृष्ठभागः(m_traVscp, stmtp);
    }
    शून्य दर्शय(AstUnpackArrayDType* nodep) override {
        // Note more specific dtypes above
        यदि (!m_traVscp) प्रत्यागम;

        यदि (v3Global.opt.traceMaxArray()
            && static_cast<पूर्णाङ्क>(nodep->arrayUnpackedElements()) > v3Global.opt.traceMaxArray()) {
            सङ्कलय_Ignore("Wide memory > --trace-max-array ents");
            प्रत्यागम;
        }

        // Skip unpacked arrays of non-traceable leaf types (e.g. strings)
        यदि (VN_IS(nodep->subDTypep()->skip_Ref_गन्तव्यम्_Enump(), BasicDType)
            && nodep->subDTypep()->skip_Ref_गन्तव्यम्_Enump()->isString()) {
            सङ्कलय_Ignore("Unsupported: strings");
            प्रत्यागम;
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        यदि (isBasicIO()) m_skipDtypeFunc = सत्य;

        यदि (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_विस्थापनम्);
            newDeclFunc(nodep);
        }
        declUnpackedArray(nodep, असत्य);
    }
    शून्य दर्शय(AstPackArrayDType* nodep) override {
        यदि (!m_traVscp) प्रत्यागम;

        यदि (!v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit.
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            सङ्कलय_Trace_Decl(VNumRange{}, nodep->विस्तारः());
            प्रत्यागम;
        }

        Ast_ग्रन्थिः_DType* ध्रुव subtypep = nodep->subDTypep()->skip_Ref_गन्तव्यम्_Enump();

        // Do not unroll if the elements are simple 'bit', or 'logic'
        यदि (AstBasicDType* ध्रुव basicp = VN_CAST(subtypep->skipRefp(), BasicDType)) {
            यदि (basicp->isBitLogic() && !basicp->isRanged()) {
                सङ्कलय_Trace_Decl(VNumRange{}, nodep->विस्तारः());
                प्रत्यागम;
            }
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        यदि (isBasicIO()) m_skipDtypeFunc = सत्य;

        यदि (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_विस्थापनम्);
            newDeclFunc(nodep);
        }
        declPackedArray(nodep, असत्य);
    }
    शून्य दर्शय(AstStructDType* nodep) override {
        यदि (!m_traVscp) प्रत्यागम;

        यदि (nodep->packed() && !v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            सङ्कलय_Trace_Decl(VNumRange{}, nodep->विस्तारः());
            प्रत्यागम;
        }

        VL_RESTORER(m_skipDtypeFunc);
        VL_RESTORER(m_dtypeDeclp);
        यदि (isBasicIO()) m_skipDtypeFunc = सत्य;

        // Only create sub functions for top-level structs, i.e. don't have struct funcs
        // call other struct funcs for child types.  This could easily be done for decl funcs
        // but full / chg funcs would require copying / aligning data for child types or more
        // complicated / wonky / generalized data access.
        यदि (!(m_skipDtypeFunc || m_dtypeDeclp)) {
            VL_RESTORER(m_विस्थापनम्);
            newDeclFunc(nodep);
        }
        declStruct(nodep, असत्य);
    }
    शून्य दर्शय(AstUnionDType* nodep) override {
        यदि (!m_traVscp) प्रत्यागम;

        VL_RESTORER(m_skipDtypeFunc);
        m_skipDtypeFunc = सत्य;

        यदि (nodep->packed() && !v3Global.opt.traceStructs()) {
            // Everything downstream is packed, so deal with as one trace unit
            // This may not be the nicest for user presentation, but is
            // a much faster way to trace
            सङ्कलय_Trace_Decl(VNumRange{}, nodep->विस्तारः());
            प्रत्यागम;
        }

        VL_RESTORER_COPY(m_traName);
        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();

        पूर्णाङ्क nMembers = 0;
        प्रति (Ast_ग्रन्थिः* mp = nodep->membersp(); mp; mp = mp->nextp()) ++nMembers;

        यदि (!nodep->packed()) {
            सङ्कलय_Ignore("Unsupported: Unpacked union");
        } अन्यथा {
            सङ्कलय_गन्तव्यम्_Sub_Func(नूतन Ast_Trace_प्रक्षिप_उपसर्गः{flp, m_traName,  //
                                                VTrace_उपसर्गः_Type::UNION_PACKED, nMembers});
            प्रति (ध्रुव AstMemberDType *itemp = nodep->membersp(), *nextp; itemp; itemp = nextp) {
                nextp = VN_AS(itemp->nextp(), MemberDType);
                Ast_ग्रन्थिः_DType* ध्रुव subtypep = itemp->subDTypep()->skip_Ref_गन्तव्यम्_Enump();
                m_traName = itemp->prettyName();
                iterate(subtypep);
            }
            सङ्कलय_गन्तव्यम्_Sub_Func(नूतन Ast_Trace_निष्कासय_उपसर्गः{flp});
        }
    }
    शून्य दर्शय(AstBasicDType* nodep) override {
        यदि (!m_traVscp) प्रत्यागम;
        यदि (nodep->isString()) {
            सङ्कलय_Ignore("Unsupported: strings");
        } अन्यथा {
            सङ्कलय_Trace_Decl(VNumRange{}, 0);
        }
    }
    शून्य दर्शय(AstEnumDType* nodep) override { iterate(nodep->skipRefp()); }
    शून्य दर्शय(Ast_ग्रन्थिः_DType*) override {
        // Note more specific dtypes above
        यदि (!m_traVscp) प्रत्यागम;
        सङ्कलय_Ignore("Unsupported: data type");
    }

    //--------------------
    शून्य दर्शय(Ast_ग्रन्थिः* nodep) override { iterateChildren(nodep); }

सार्वजनिक:
    // CONSTRUCTORS
    स्पष्ट TraceDeclVisitor(AstNetlist* nodep)
        : m_शिखरम्_Scopep{nodep->शिखरम्_Scopep()} {
        File_पङ्क्तिः* ध्रुव flp = nodep->fileline();

        // Iterate modules to build per scope initialization functions
        iterate_And_अग्र_Const_Null(nodep->modulesp());
        UASSERT_OBJ(m_subFuncps.रिक्तानि(), nodep, "Should have been emptied");

        // Fix up the placeholders in the initialization functions
        fixupPlaceholders();

        // Now that we have everything ready, remove redundant pushPrefix/popPrefix
        // pairs. While functionally this is not really necessary (the trace files
        // might have some empty scope declarations), we do it to preserve previous
        // behavior. Note: unfortunately generating these without the redundant
        // push/pop pairs is a bit hard. It is cleaner to remove them.
        remove_Redundant_उपसर्गः_प्रक्षिप_निष्कासय();

        ध्रुव std::सदिशम्<AstCFunc*>& शिखरम्_Scope_Funcps = m_scopeInitFuncps.at(m_शिखरम्_Scopep->scopep());
        AstCFunc* मूलम्_Funcp = शून्यसूचकः;
        यदि (!v3Global.opt.libCreate().रिक्तानि()) {
            मूलम्_Funcp = newCFunc(flp, "trace_init_root");
            मूलम्_Funcp->प्रविष्टिः_Point(सत्य);
            प्रति (आकार_t i = 0; i < m_शिखरम्_Scope_मूलम्_Func_गणनम्; ++i) {
                AstCCall* ध्रुव callp = नूतन AstCCall{flp, शिखरम्_Scope_Funcps.at(i)};
                callp->dtype_समुच्चयः_Void();
                callp->argTypes("tracep");
                मूलम्_Funcp->सङ्कलय_Stmtsp(callp->makeStmt());
            }
            यदि (!m_शिखरम्_Scope_मूलम्_Func_गणनम्) मूलम्_Funcp->सङ्कलय_Stmtsp(नूतन AstComment{flp, "Empty"});
        }

        // Call the non-wrapper initialization functions of the root scope from the top function
        प्रति (आकार_t i = m_शिखरम्_Scope_मूलम्_Func_गणनम्; i < शिखरम्_Scope_Funcps.आकार(); ++i) {
            AstCFunc* ध्रुव funcp = शिखरम्_Scope_Funcps.at(i);
            AstCCall* ध्रुव callp = नूतन AstCCall{flp, funcp};
            callp->dtype_समुच्चयः_Void();
            callp->argTypes("tracep");
            सङ्कलय_गन्तव्यम्_शिखरम्_Func(callp->makeStmt());
        }

        // Ensure a top function exists, in case there was nothing to trace at all
        यदि (m_शिखरम्_Funcps.रिक्तानि()) सङ्कलय_गन्तव्यम्_शिखरम्_Func(नूतन AstComment{flp, "Empty"});

        // Create single top level function, if more than one exists
        यदि (m_शिखरम्_Funcps.आकार() > 1) {
            AstCFunc* ध्रुव शिखरम्_Funcp = newCFunc(flp, "");
            प्रति (AstCFunc* funcp : m_शिखरम्_Funcps) {
                AstCCall* ध्रुव callp = नूतन AstCCall{flp, funcp};
                callp->dtype_समुच्चयः_Void();
                callp->argTypes("tracep");
                शिखरम्_Funcp->सङ्कलय_Stmtsp(callp->makeStmt());
            }
            m_शिखरम्_Funcps.clear();
            m_शिखरम्_Funcps.प्रक्षिप_पृष्ठभागः(शिखरम्_Funcp);
        }

        // Set name of top level function
        AstCFunc* ध्रुव शिखरम्_Funcp = m_शिखरम्_Funcps.front();
        शिखरम्_Funcp->name("trace_init_top");
        शिखरम्_Funcp->प्रविष्टिः_Point(सत्य);

        यदि (मूलम्_Funcp && v3Global.opt.debugCheck()) checkCallsRecurse(मूलम्_Funcp);
        checkCalls(शिखरम्_Funcp);
    }
    ~TraceDeclVisitor() override {
        V3Stats::सङ्कलय_Stat("Tracing, Traced signals", m_statSigs);
        V3Stats::सङ्कलय_Stat("Tracing, Ignored signals", m_statIgnSigs);
    }
};

//######################################################################
// Trace class functions

शून्य V3TraceDecl::traceDeclAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { TraceDeclVisitor{nodep}; }  // Destruct before checking
    V3Global::dump_Check_Global_वृक्षः("tracedecl", 0, dump_वृक्षः_Either_Level() >= 3);
}
