// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Ordering constraint graph
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
//
// OrderGraph is a bipartite graph, with the two parts being formed of only OrderLogicVertex and
// OrderVarVertex vertices respectively (i.e.: edges are always between OrderLogicVertex and
// OrderVarVertex, and never between two OrderLogicVertex or OrderVarVertex). The graph represents
// both fine-grained dependencies, and additional ordering constraints between logic blocks and
// variables. The fact that OrderGraph is bipartite is important and we take advantage of this fact
// in various algorithms, so this property must be maintained.
//
// Both OrderLogicVertex and OrderVarVertex derives from OrderEitherVertex, so OrderGraph is
// composed only of OrderEitherVertex vertices.
//
// OrderLogicVertex holds a 'logic block', which is just some computational construct that is
// ordered as a single unit. Ordering of these logic blocks is determined by the variables they
// read and write, which is represented by the edges between OrderLogicVertex and OrderVarVertex
// instances (and hence the graph is bipartite).
//
// OrderVarVertex is abstract, and has various concrete subtypes that represent various ordering
// constraints imposed by variables accessed by logic blocks. The concrete subtypes and their
// roles are:
//
// OrderVarStdVertex:   Data dependencies for combinational logic and delayed assignment
//                      updates (AlwaysPost).
// OrderVarPostVertex:  Ensures all sequential logic blocks reading a signal do so before any
//                      combinational or delayed assignments update that signal.
// OrderVarPordVertex:  Ensures a _d = _q AlwaysPre used to implement delayed (non-blocking)
//                      assignments is the first write of a _d, before any sequential blocks
//                      write to that _d.
// OrderVarPreVertex:   This is an optimization. Try to ensure that a _d = _q AlwaysPre is the
//                      last read of a _q, after all reads of that _q by sequential logic. The
//                      model is still correct if we cannot satisfy this due to other interfering
//                      constraints. If respecting this constraint is possible, then combined
//                      with the OrderVarPordVertex constraint we get that all writes to _d are
//                      after all reads of a _q, which then allows us to eliminate the _d
//                      completely and assign to the _q directly. This means these delayed
//                      assignments can be implemented without temporary storage (the redundant
//                      storage is eliminated in V3LifePost).
//
// Ordering constraints are represented by directed edges, where the source of an edge needs to be
// ordered before the sink of an edge. A constraint can be either hard (must be satisfied),
// represented by a non-cutable edge, or a constraint can be soft (ideally should be satisfied, but
// is ok not to if other hard constraints interfere), represented by a cutable edge. Edges
// otherwise carry no additional information. TODO: what about weight?
//
// Note: It is required for hard (non-cutable) constraints to form a DAG, but together with the
// soft constraints the graph can be arbitrary so long as it remains bipartite.
//
//*************************************************************************

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3ORDERGRAPH_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3ORDERGRAPH_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Graph.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Sched.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <set>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

class ਕ੍ਰਮ_Logic_ਸ਼ੀਰਸ਼;
class ਕ੍ਰਮ_Var_ਸ਼ੀਰਸ਼;

//======================================================================

ਗਿਣਤੀ ਕ੍ਰਮ_Weights : uint8_t {
    ਭਾਰ_COMBO = 1,  // Breakable combo logic
    ਭਾਰ_POST = 2,  // Post-delayed used var
    ਭਾਰ_PRE = 3,  // Breakable pre-delayed used var
    ਭਾਰ_MEDIUM = 8,  // Medium weight just so dot graph looks nice
    ਭਾਰ_ਸਾਧਾਰਨ = 32  // High weight just so dot graph looks nice
};

//======================================================================
// Graph type

// The sensitivity domain of a vertex: the set of trigger indices,
// any of which being set means the logic needs to be executed.
// Instances are interned in the OrderGraph, so can compare pointers.
class ਕ੍ਰਮ_Domain final {
    ਅਚਲ std::ਸੈੱਟ<uint32_t> m_triggers;  // The trigger indices

ਜਨਤਕ:
    explicit ਕ੍ਰਮ_Domain(ਅਚਲ std::ਸੈੱਟ<uint32_t>& triggers)
        : m_triggers{triggers} {}

    ਅਚਲ std::ਸੈੱਟ<uint32_t>& triggers() ਅਚਲ { ਵਾਪਸ m_triggers; }
    bool ਸੰਚਾਲਕ<(ਅਚਲ ਕ੍ਰਮ_Domain& other) ਅਚਲ { ਵਾਪਸ m_triggers < other.m_triggers; }
};

class ਕ੍ਰਮ_ਗ੍ਰਾਫ final : ਜਨਤਕ V3_ਗ੍ਰਾਫ {
    ਅਚਲ V3Sched::TriggerKit& m_trigKit;  // The TriggerKit of the triggers
    AstVarScope* ਅਚਲ m_trigVscp;  // The trigger vector for this graph
    std::ਸੈੱਟ<ਕ੍ਰਮ_Domain> m_domains;  // The interned domains

ਜਨਤਕ:
    // CONSTRUCTOR
    ਕ੍ਰਮ_ਗ੍ਰਾਫ(ਅਚਲ V3Sched::TriggerKit& trigKit, AstVarScope* trigVscp)
        : m_trigKit{trigKit}
        , m_trigVscp{trigVscp} {}

    // METHODS
    AstVarScope* trigVscp() ਅਚਲ { ਵਾਪਸ m_trigVscp; }

    // The AstSenItem the given trigger index stands for, nullptr if none
    ਅਚਲ Ast_Sen_ਵਸਤੂ* trigSenItemp(uint32_t ਸੂਚਕਾਂਕ) ਅਚਲ {
        ਵਾਪਸ m_trigKit.trig_Idx2_Sen_ਵਸਤੂ().at(ਸੂਚਕਾਂਕ);
    }

    // Get the interned instance of the given domain
    ਅਚਲ ਕ੍ਰਮ_Domain* domainp(ਅਚਲ std::ਸੈੱਟ<uint32_t>& triggers) {
        ਵਾਪਸ &*m_domains.emplace(triggers).ਪਹਿਲਾ;
    }

    // Methods to add edges representing constraints, utilizing the type system to help us ensure
    // the graph remains bipartite.
    ਸਤਰਅੰਦਰ ਖਾਲੀ ਜੋੜੋ_Hard_ਕਿਨਾਰਾ(ਕ੍ਰਮ_Logic_ਸ਼ੀਰਸ਼* fromp, ਕ੍ਰਮ_Var_ਸ਼ੀਰਸ਼* ਸਿਖਰ,
                            ਪੂਰਨ ਭਾਰ_2) VL_MT_DISABLED;
    ਸਤਰਅੰਦਰ ਖਾਲੀ ਜੋੜੋ_Hard_ਕਿਨਾਰਾ(ਕ੍ਰਮ_Var_ਸ਼ੀਰਸ਼* fromp, ਕ੍ਰਮ_Logic_ਸ਼ੀਰਸ਼* ਸਿਖਰ,
                            ਪੂਰਨ ਭਾਰ_2) VL_MT_DISABLED;
    ਸਤਰਅੰਦਰ ਖਾਲੀ ਜੋੜੋ_Soft_ਕਿਨਾਰਾ(ਕ੍ਰਮ_Logic_ਸ਼ੀਰਸ਼* fromp, ਕ੍ਰਮ_Var_ਸ਼ੀਰਸ਼* ਸਿਖਰ,
                            ਪੂਰਨ ਭਾਰ_2) VL_MT_DISABLED;
    ਸਤਰਅੰਦਰ ਖਾਲੀ ਜੋੜੋ_Soft_ਕਿਨਾਰਾ(ਕ੍ਰਮ_Var_ਸ਼ੀਰਸ਼* fromp, ਕ੍ਰਮ_Logic_ਸ਼ੀਰਸ਼* ਸਿਖਰ,
                            ਪੂਰਨ ਭਾਰ_2) VL_MT_DISABLED;
};

//======================================================================
// Vertex types

class ਕ੍ਰਮ_Either_ਸ਼ੀਰਸ਼ VL_NOT_FINAL : ਜਨਤਕ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼ {
    VL_RTTI_IMPL(ਕ੍ਰਮ_Either_ਸ਼ੀਰਸ਼, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼)
    // Event domain of vertex. For OrderLogicVertex this represents the conditions when the logic
    // block must be executed. For OrderVarVertex, this is the union of the domains of all the
    // OrderLogicVertex vertices that drive the variable. If initially set to nullptr (e.g.: all
    // OrderVarVertex and those OrderLogicVertices that represent combinational logic), then the
    // ordering algorithm will compute the domain automatically based on the edges representing
    // data-flow (those between OrderLogicVertex and OrderVarStdVertex), otherwise the domain is
    // as given (e.g.: for those OrderLogicVertices that represent clocked logic).
    ਅਚਲ ਕ੍ਰਮ_Domain* m_domainp;

ਸੁਰੱਖਿਅਤ:
    // CONSTRUCTOR
    ਕ੍ਰਮ_Either_ਸ਼ੀਰਸ਼(ਕ੍ਰਮ_ਗ੍ਰਾਫ* graphp, ਅਚਲ ਕ੍ਰਮ_Domain* domainp) VL_MT_DISABLED
        : V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼{graphp},
          m_domainp{domainp} {}
    ~ਕ੍ਰਮ_Either_ਸ਼ੀਰਸ਼() override = ਮੂਲ;

ਜਨਤਕ:
    // METHODS
    ਆਭਾਸੀ bool domainMatters() = 0;

    // ACCESSORS
    ਅਚਲ ਕ੍ਰਮ_Domain* domainp() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ m_domainp; }
    ਖਾਲੀ domainp(ਅਚਲ ਕ੍ਰਮ_Domain* domainp) VL_MT_DISABLED {
#ਜੇ VL_DEBUG
        UASSERT(!m_domainp, "Domain should only be set once");
#ਜੇ_ਅੰਤ
        m_domainp = domainp;
    }
};

class ਕ੍ਰਮ_Logic_ਸ਼ੀਰਸ਼ final : ਜਨਤਕ ਕ੍ਰਮ_Either_ਸ਼ੀਰਸ਼ {
    VL_RTTI_IMPL(ਕ੍ਰਮ_Logic_ਸ਼ੀਰਸ਼, ਕ੍ਰਮ_Either_ਸ਼ੀਰਸ਼)

ਜਨਤਕ:
    // Variable access record
    ਬਣਤਰ VarAccess final {
        AstVarScope* m_vscp;  // The variable accessed
        VAccess m_access;  // The kind of access, as in the AST
    };

ਨਿਜੀ:
    Ast_ਗੰਢ* ਅਚਲ m_nodep;  // The logic this vertex represents
    AstScope* ਅਚਲ m_scopep;  // Scope the logic is under
    ਅਚਲ ਕ੍ਰਮ_Domain* ਅਚਲ m_hybridp;  // Additional sensitivities for hybrid combinational logic
    ਅਚਲ bool m_comb;  // Is from combinational logic
    // Every variable accessed by this logic, in order of first access, at most one record per
    // variable. Only populated for multi-threaded ordering.
    std::ਸਦਿਸ਼<VarAccess> m_varAccesses;

ਜਨਤਕ:
    // CONSTRUCTOR
    ਕ੍ਰਮ_Logic_ਸ਼ੀਰਸ਼(ਕ੍ਰਮ_ਗ੍ਰਾਫ* graphp, AstScope* scopep, ਅਚਲ ਕ੍ਰਮ_Domain* domainp,
                     ਅਚਲ ਕ੍ਰਮ_Domain* hybridp, Ast_ਗੰਢ* nodep) VL_MT_DISABLED
        : ਕ੍ਰਮ_Either_ਸ਼ੀਰਸ਼{graphp, domainp},
          m_nodep{nodep},
          m_scopep{scopep},
          m_hybridp{hybridp},
          m_comb{!domainp && !hybridp} {
        UASSERT_OBJ(scopep, nodep, "Must not be null");
        UASSERT_OBJ(!(domainp && hybridp), nodep, "Cannot have bot domainp and hybridp set");
    }
    ~ਕ੍ਰਮ_Logic_ਸ਼ੀਰਸ਼() override = ਮੂਲ;

    // METHODS
    bool domainMatters() override { ਵਾਪਸ true; }

    // ACCESSORS
    Ast_ਗੰਢ* nodep() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ m_nodep; }
    AstScope* scopep() ਅਚਲ VL_MT_STABLE { ਵਾਪਸ m_scopep; }
    ਅਚਲ ਕ੍ਰਮ_Domain* hybridp() ਅਚਲ { ਵਾਪਸ m_hybridp; }
    bool comb() ਅਚਲ { ਵਾਪਸ m_comb; }
    ਅਚਲ std::ਸਦਿਸ਼<VarAccess>& varAccesses() ਅਚਲ { ਵਾਪਸ m_varAccesses; }
    ਖਾਲੀ ਜੋੜੋ_Var_Access(AstVarScope* vscp, VAccess access) {
        m_varAccesses.ਧੱਕੋ_ਪਿੱਠ({vscp, access});
    }

    // LCOV_EXCL_START // Debug code
    string name() ਅਚਲ override VL_MT_STABLE {
        ਵਾਪਸ (cvt_ਮੰਜ਼ਿਲ_Hex(m_nodep) + "\\n " + cvt_ਮੰਜ਼ਿਲ_Str(nodep()->typeName()));
    }
    string dotShape() ਅਚਲ override { ਵਾਪਸ VN_IS(m_nodep, ਸਰਗਰਮ) ? "doubleoctagon" : "rect"; }
    // LCOV_EXCL_STOP
};

class ਕ੍ਰਮ_Var_ਸ਼ੀਰਸ਼ VL_NOT_FINAL : ਜਨਤਕ ਕ੍ਰਮ_Either_ਸ਼ੀਰਸ਼ {
    VL_RTTI_IMPL(ਕ੍ਰਮ_Var_ਸ਼ੀਰਸ਼, ਕ੍ਰਮ_Either_ਸ਼ੀਰਸ਼)
    AstVarScope* ਅਚਲ m_vscp;

ਜਨਤਕ:
    // CONSTRUCTOR
    ਕ੍ਰਮ_Var_ਸ਼ੀਰਸ਼(ਕ੍ਰਮ_ਗ੍ਰਾਫ* graphp, AstVarScope* vscp) VL_MT_DISABLED
        : ਕ੍ਰਮ_Either_ਸ਼ੀਰਸ਼{graphp, nullptr},
          m_vscp{vscp} {}
    ~ਕ੍ਰਮ_Var_ਸ਼ੀਰਸ਼() override = ਮੂਲ;

    // ACCESSORS
    AstVarScope* vscp() ਅਚਲ { ਵਾਪਸ m_vscp; }

    // LCOV_EXCL_START // Debug code
    string dotShape() ਅਚਲ override final { ਵਾਪਸ "ellipse"; }
    ਆਭਾਸੀ string nameSuffix() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ = 0;
    string name() ਅਚਲ override final VL_MT_STABLE {
        ਵਾਪਸ cvt_ਮੰਜ਼ਿਲ_Hex(m_vscp) + " " + nameSuffix() + "\\n " + m_vscp->name();
    }
    // LCOV_EXCL_STOP
};

class ਕ੍ਰਮ_Var_Std_ਸ਼ੀਰਸ਼ final : ਜਨਤਕ ਕ੍ਰਮ_Var_ਸ਼ੀਰਸ਼ {
    VL_RTTI_IMPL(ਕ੍ਰਮ_Var_Std_ਸ਼ੀਰਸ਼, ਕ੍ਰਮ_Var_ਸ਼ੀਰਸ਼)
ਜਨਤਕ:
    // CONSTRUCTOR
    ਕ੍ਰਮ_Var_Std_ਸ਼ੀਰਸ਼(ਕ੍ਰਮ_ਗ੍ਰਾਫ* graphp, AstVarScope* vscp) VL_MT_DISABLED
        : ਕ੍ਰਮ_Var_ਸ਼ੀਰਸ਼{graphp, vscp} {}
    ~ਕ੍ਰਮ_Var_Std_ਸ਼ੀਰਸ਼() override = ਮੂਲ;

    // METHODS
    bool domainMatters() override { ਵਾਪਸ true; }

    // LCOV_EXCL_START // Debug code
    string nameSuffix() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ ""; }
    string dotColor() ਅਚਲ override { ਵਾਪਸ "grey"; }
    // LCOV_EXCL_STOP
};

class ਕ੍ਰਮ_Var_Pre_ਸ਼ੀਰਸ਼ final : ਜਨਤਕ ਕ੍ਰਮ_Var_ਸ਼ੀਰਸ਼ {
    VL_RTTI_IMPL(ਕ੍ਰਮ_Var_Pre_ਸ਼ੀਰਸ਼, ਕ੍ਰਮ_Var_ਸ਼ੀਰਸ਼)
ਜਨਤਕ:
    // CONSTRUCTOR
    ਕ੍ਰਮ_Var_Pre_ਸ਼ੀਰਸ਼(ਕ੍ਰਮ_ਗ੍ਰਾਫ* graphp, AstVarScope* vscp) VL_MT_DISABLED
        : ਕ੍ਰਮ_Var_ਸ਼ੀਰਸ਼{graphp, vscp} {}
    ~ਕ੍ਰਮ_Var_Pre_ਸ਼ੀਰਸ਼() override = ਮੂਲ;

    // METHODS
    bool domainMatters() override { ਵਾਪਸ false; }

    // LCOV_EXCL_START // Debug code
    string nameSuffix() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ "PRE"; }
    string dotColor() ਅਚਲ override { ਵਾਪਸ "green"; }
    // LCOV_EXCL_STOP
};

class ਕ੍ਰਮ_Var_Post_ਸ਼ੀਰਸ਼ final : ਜਨਤਕ ਕ੍ਰਮ_Var_ਸ਼ੀਰਸ਼ {
    VL_RTTI_IMPL(ਕ੍ਰਮ_Var_Post_ਸ਼ੀਰਸ਼, ਕ੍ਰਮ_Var_ਸ਼ੀਰਸ਼)
ਜਨਤਕ:
    // CONSTRUCTOR
    ਕ੍ਰਮ_Var_Post_ਸ਼ੀਰਸ਼(ਕ੍ਰਮ_ਗ੍ਰਾਫ* graphp, AstVarScope* vscp) VL_MT_DISABLED
        : ਕ੍ਰਮ_Var_ਸ਼ੀਰਸ਼{graphp, vscp} {}
    ~ਕ੍ਰਮ_Var_Post_ਸ਼ੀਰਸ਼() override = ਮੂਲ;

    // METHODS
    bool domainMatters() override { ਵਾਪਸ false; }

    // LCOV_EXCL_START // Debug code
    string nameSuffix() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ "POST"; }
    string dotColor() ਅਚਲ override { ਵਾਪਸ "red"; }
    // LCOV_EXCL_STOP
};

class ਕ੍ਰਮ_Var_Pord_ਸ਼ੀਰਸ਼ final : ਜਨਤਕ ਕ੍ਰਮ_Var_ਸ਼ੀਰਸ਼ {
    VL_RTTI_IMPL(ਕ੍ਰਮ_Var_Pord_ਸ਼ੀਰਸ਼, ਕ੍ਰਮ_Var_ਸ਼ੀਰਸ਼)
ਜਨਤਕ:
    // CONSTRUCTOR
    ਕ੍ਰਮ_Var_Pord_ਸ਼ੀਰਸ਼(ਕ੍ਰਮ_ਗ੍ਰਾਫ* graphp, AstVarScope* vscp) VL_MT_DISABLED
        : ਕ੍ਰਮ_Var_ਸ਼ੀਰਸ਼{graphp, vscp} {}
    ~ਕ੍ਰਮ_Var_Pord_ਸ਼ੀਰਸ਼() override = ਮੂਲ;

    // METHODS
    bool domainMatters() override { ਵਾਪਸ false; }

    // LCOV_EXCL_START // Debug code
    string nameSuffix() ਅਚਲ override VL_MT_ਸੁਰੱਖਿਅਤ { ਵਾਪਸ "PORD"; }
    string dotColor() ਅਚਲ override { ਵਾਪਸ "blue"; }
    // LCOV_EXCL_STOP
};

//======================================================================
// Edge type

class ਕ੍ਰਮ_ਕਿਨਾਰਾ final : ਜਨਤਕ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ {
    VL_RTTI_IMPL(ਕ੍ਰਮ_ਕਿਨਾਰਾ, V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ)
    ਮਿੱਤਰ class ਕ੍ਰਮ_ਗ੍ਰਾਫ;  // Only the OrderGraph can create these
    // CONSTRUCTOR
    ਕ੍ਰਮ_ਕਿਨਾਰਾ(ਕ੍ਰਮ_ਗ੍ਰਾਫ* graphp, ਕ੍ਰਮ_Either_ਸ਼ੀਰਸ਼* fromp, ਕ੍ਰਮ_Either_ਸ਼ੀਰਸ਼* ਸਿਖਰ, ਪੂਰਨ ਭਾਰ_2,
              bool cutable) VL_MT_DISABLED : V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{graphp, fromp, ਸਿਖਰ, ਭਾਰ_2, cutable} {}
    ~ਕ੍ਰਮ_ਕਿਨਾਰਾ() override = ਮੂਲ;

    // LCOV_EXCL_START // Debug code
    string dotColor() ਅਚਲ override { ਵਾਪਸ cutable() ? "green" : "red"; }
    // LCOV_EXCL_STOP
};

//======================================================================
// Inline methods

ਖਾਲੀ ਕ੍ਰਮ_ਗ੍ਰਾਫ::ਜੋੜੋ_Hard_ਕਿਨਾਰਾ(ਕ੍ਰਮ_Logic_ਸ਼ੀਰਸ਼* fromp, ਕ੍ਰਮ_Var_ਸ਼ੀਰਸ਼* ਸਿਖਰ,
                             ਪੂਰਨ ਭਾਰ_2) VL_MT_DISABLED {
    new ਕ੍ਰਮ_ਕਿਨਾਰਾ{this, fromp, ਸਿਖਰ, ਭਾਰ_2, /* cutable: */ false};
}
ਖਾਲੀ ਕ੍ਰਮ_ਗ੍ਰਾਫ::ਜੋੜੋ_Hard_ਕਿਨਾਰਾ(ਕ੍ਰਮ_Var_ਸ਼ੀਰਸ਼* fromp, ਕ੍ਰਮ_Logic_ਸ਼ੀਰਸ਼* ਸਿਖਰ,
                             ਪੂਰਨ ਭਾਰ_2) VL_MT_DISABLED {
    new ਕ੍ਰਮ_ਕਿਨਾਰਾ{this, fromp, ਸਿਖਰ, ਭਾਰ_2, /* cutable: */ false};
}
ਖਾਲੀ ਕ੍ਰਮ_ਗ੍ਰਾਫ::ਜੋੜੋ_Soft_ਕਿਨਾਰਾ(ਕ੍ਰਮ_Logic_ਸ਼ੀਰਸ਼* fromp, ਕ੍ਰਮ_Var_ਸ਼ੀਰਸ਼* ਸਿਖਰ,
                             ਪੂਰਨ ਭਾਰ_2) VL_MT_DISABLED {
    new ਕ੍ਰਮ_ਕਿਨਾਰਾ{this, fromp, ਸਿਖਰ, ਭਾਰ_2, /* cutable: */ true};
}
ਖਾਲੀ ਕ੍ਰਮ_ਗ੍ਰਾਫ::ਜੋੜੋ_Soft_ਕਿਨਾਰਾ(ਕ੍ਰਮ_Var_ਸ਼ੀਰਸ਼* fromp, ਕ੍ਰਮ_Logic_ਸ਼ੀਰਸ਼* ਸਿਖਰ,
                             ਪੂਰਨ ਭਾਰ_2) VL_MT_DISABLED {
    new ਕ੍ਰਮ_ਕਿਨਾਰਾ{this, fromp, ਸਿਖਰ, ਭਾਰ_2, /* cutable: */ true};
}

#ਜੇ_ਅੰਤ  // Guard
