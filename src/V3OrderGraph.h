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

#ವ್ಯಾಖ್ಯಾನಿತವಾಗದಿದ್ದರೆ VERILATOR_V3ORDERGRAPH_H_
#ವ್ಯಾಖ್ಯಾನಿಸು VERILATOR_V3ORDERGRAPH_H_

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Ast.h"
#ಅನ್ತರ್ಭಾವ "V3Graph.h"
#ಅನ್ತರ್ಭಾವ "V3Sched.h"

#ಅನ್ತರ್ಭಾವ <set>
#ಅನ್ತರ್ಭಾವ <vector>

class ಕ್ರಮ_Logic_ಶೃಂಗ;
class ಕ್ರಮ_Var_ಶೃಂಗ;

//======================================================================

ಎಣಿಕೆ ಕ್ರಮ_Weights : uint8_t {
    ತೂಕ_COMBO = 1,  // Breakable combo logic
    ತೂಕ_POST = 2,  // Post-delayed used var
    ತೂಕ_PRE = 3,  // Breakable pre-delayed used var
    ತೂಕ_MEDIUM = 8,  // Medium weight just so dot graph looks nice
    ತೂಕ_ಸಾಮಾನ್ಯ = 32  // High weight just so dot graph looks nice
};

//======================================================================
// Graph type

// The sensitivity domain of a vertex: the set of trigger indices,
// any of which being set means the logic needs to be executed.
// Instances are interned in the OrderGraph, so can compare pointers.
class ಕ್ರಮ_Domain final {
    ಸ್ಥಿರಾಂಕ std::ಗಣ<uint32_t> m_triggers;  // The trigger indices

ಸಾರ್ವಜನಿಕ:
    explicit ಕ್ರಮ_Domain(ಸ್ಥಿರಾಂಕ std::ಗಣ<uint32_t>& triggers)
        : m_triggers{triggers} {}

    ಸ್ಥಿರಾಂಕ std::ಗಣ<uint32_t>& triggers() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_triggers; }
    bool ಕ್ರಿಯಾಕಾರಕ<(ಸ್ಥಿರಾಂಕ ಕ್ರಮ_Domain& other) ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_triggers < other.m_triggers; }
};

class ಕ್ರಮ_ನಕ್ಷೆ final : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ {
    ಸ್ಥಿರಾಂಕ V3Sched::TriggerKit& m_trigKit;  // The TriggerKit of the triggers
    AstVarScope* ಸ್ಥಿರಾಂಕ m_trigVscp;  // The trigger vector for this graph
    std::ಗಣ<ಕ್ರಮ_Domain> m_domains;  // The interned domains

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTOR
    ಕ್ರಮ_ನಕ್ಷೆ(ಸ್ಥಿರಾಂಕ V3Sched::TriggerKit& trigKit, AstVarScope* trigVscp)
        : m_trigKit{trigKit}
        , m_trigVscp{trigVscp} {}

    // METHODS
    AstVarScope* trigVscp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_trigVscp; }

    // The AstSenItem the given trigger index stands for, nullptr if none
    ಸ್ಥಿರಾಂಕ Ast_Sen_ಅಂಶ* trigSenItemp(uint32_t ಸೂಚ್ಯಂಕ) ಸ್ಥಿರಾಂಕ {
        ಹಿಂತಿರುಗಿ m_trigKit.trig_Idx2_Sen_ಅಂಶ().at(ಸೂಚ್ಯಂಕ);
    }

    // Get the interned instance of the given domain
    ಸ್ಥಿರಾಂಕ ಕ್ರಮ_Domain* domainp(ಸ್ಥಿರಾಂಕ std::ಗಣ<uint32_t>& triggers) {
        ಹಿಂತಿರುಗಿ &*m_domains.emplace(triggers).ಮೊದಲ;
    }

    // Methods to add edges representing constraints, utilizing the type system to help us ensure
    // the graph remains bipartite.
    ಸಾಲೊಳಗೆ ಶೂನ್ಯ ಕೂಡಿಸು_Hard_ಅಂಚು(ಕ್ರಮ_Logic_ಶೃಂಗ* fromp, ಕ್ರಮ_Var_ಶೃಂಗ* ತುದಿ,
                            ಪೂರ್ಣಾಂಕ ತೂಕ_2) VL_MT_DISABLED;
    ಸಾಲೊಳಗೆ ಶೂನ್ಯ ಕೂಡಿಸು_Hard_ಅಂಚು(ಕ್ರಮ_Var_ಶೃಂಗ* fromp, ಕ್ರಮ_Logic_ಶೃಂಗ* ತುದಿ,
                            ಪೂರ್ಣಾಂಕ ತೂಕ_2) VL_MT_DISABLED;
    ಸಾಲೊಳಗೆ ಶೂನ್ಯ ಕೂಡಿಸು_Soft_ಅಂಚು(ಕ್ರಮ_Logic_ಶೃಂಗ* fromp, ಕ್ರಮ_Var_ಶೃಂಗ* ತುದಿ,
                            ಪೂರ್ಣಾಂಕ ತೂಕ_2) VL_MT_DISABLED;
    ಸಾಲೊಳಗೆ ಶೂನ್ಯ ಕೂಡಿಸು_Soft_ಅಂಚು(ಕ್ರಮ_Var_ಶೃಂಗ* fromp, ಕ್ರಮ_Logic_ಶೃಂಗ* ತುದಿ,
                            ಪೂರ್ಣಾಂಕ ತೂಕ_2) VL_MT_DISABLED;
};

//======================================================================
// Vertex types

class ಕ್ರಮ_Either_ಶೃಂಗ VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ_ಶೃಂಗ {
    VL_RTTI_IMPL(ಕ್ರಮ_Either_ಶೃಂಗ, V3_ನಕ್ಷೆ_ಶೃಂಗ)
    // Event domain of vertex. For OrderLogicVertex this represents the conditions when the logic
    // block must be executed. For OrderVarVertex, this is the union of the domains of all the
    // OrderLogicVertex vertices that drive the variable. If initially set to nullptr (e.g.: all
    // OrderVarVertex and those OrderLogicVertices that represent combinational logic), then the
    // ordering algorithm will compute the domain automatically based on the edges representing
    // data-flow (those between OrderLogicVertex and OrderVarStdVertex), otherwise the domain is
    // as given (e.g.: for those OrderLogicVertices that represent clocked logic).
    ಸ್ಥಿರಾಂಕ ಕ್ರಮ_Domain* m_domainp;

ಸಂರಕ್ಷಿತ:
    // CONSTRUCTOR
    ಕ್ರಮ_Either_ಶೃಂಗ(ಕ್ರಮ_ನಕ್ಷೆ* graphp, ಸ್ಥಿರಾಂಕ ಕ್ರಮ_Domain* domainp) VL_MT_DISABLED
        : V3_ನಕ್ಷೆ_ಶೃಂಗ{graphp},
          m_domainp{domainp} {}
    ~ಕ್ರಮ_Either_ಶೃಂಗ() override = ಪೂರ್ವನಿಯೋಜಿತ;

ಸಾರ್ವಜನಿಕ:
    // METHODS
    ವಾಸ್ತವಿಕ bool domainMatters() = 0;

    // ACCESSORS
    ಸ್ಥಿರಾಂಕ ಕ್ರಮ_Domain* domainp() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_domainp; }
    ಶೂನ್ಯ domainp(ಸ್ಥಿರಾಂಕ ಕ್ರಮ_Domain* domainp) VL_MT_DISABLED {
#ಆದರೆ VL_DEBUG
        UASSERT(!m_domainp, "Domain should only be set once");
#ಆದರೆ_ಅಂತ್ಯ
        m_domainp = domainp;
    }
};

class ಕ್ರಮ_Logic_ಶೃಂಗ final : ಸಾರ್ವಜನಿಕ ಕ್ರಮ_Either_ಶೃಂಗ {
    VL_RTTI_IMPL(ಕ್ರಮ_Logic_ಶೃಂಗ, ಕ್ರಮ_Either_ಶೃಂಗ)

ಸಾರ್ವಜನಿಕ:
    // Variable access record
    ರಚನೆ VarAccess final {
        AstVarScope* m_vscp;  // The variable accessed
        VAccess m_access;  // The kind of access, as in the AST
    };

ಖಾಸಗಿ:
    Ast_ಗಂಟು* ಸ್ಥಿರಾಂಕ m_nodep;  // The logic this vertex represents
    AstScope* ಸ್ಥಿರಾಂಕ m_scopep;  // Scope the logic is under
    ಸ್ಥಿರಾಂಕ ಕ್ರಮ_Domain* ಸ್ಥಿರಾಂಕ m_hybridp;  // Additional sensitivities for hybrid combinational logic
    ಸ್ಥಿರಾಂಕ bool m_comb;  // Is from combinational logic
    // Every variable accessed by this logic, in order of first access, at most one record per
    // variable. Only populated for multi-threaded ordering.
    std::ಸದಿಶ<VarAccess> m_varAccesses;

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTOR
    ಕ್ರಮ_Logic_ಶೃಂಗ(ಕ್ರಮ_ನಕ್ಷೆ* graphp, AstScope* scopep, ಸ್ಥಿರಾಂಕ ಕ್ರಮ_Domain* domainp,
                     ಸ್ಥಿರಾಂಕ ಕ್ರಮ_Domain* hybridp, Ast_ಗಂಟು* nodep) VL_MT_DISABLED
        : ಕ್ರಮ_Either_ಶೃಂಗ{graphp, domainp},
          m_nodep{nodep},
          m_scopep{scopep},
          m_hybridp{hybridp},
          m_comb{!domainp && !hybridp} {
        UASSERT_OBJ(scopep, nodep, "Must not be null");
        UASSERT_OBJ(!(domainp && hybridp), nodep, "Cannot have bot domainp and hybridp set");
    }
    ~ಕ್ರಮ_Logic_ಶೃಂಗ() override = ಪೂರ್ವನಿಯೋಜಿತ;

    // METHODS
    bool domainMatters() override { ಹಿಂತಿರುಗಿ true; }

    // ACCESSORS
    Ast_ಗಂಟು* nodep() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_nodep; }
    AstScope* scopep() ಸ್ಥಿರಾಂಕ VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_scopep; }
    ಸ್ಥಿರಾಂಕ ಕ್ರಮ_Domain* hybridp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_hybridp; }
    bool comb() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_comb; }
    ಸ್ಥಿರಾಂಕ std::ಸದಿಶ<VarAccess>& varAccesses() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_varAccesses; }
    ಶೂನ್ಯ ಕೂಡಿಸು_Var_Access(AstVarScope* vscp, VAccess access) {
        m_varAccesses.ತಳ್ಳು_ಹಿಂಭಾಗ({vscp, access});
    }

    // LCOV_EXCL_START // Debug code
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE {
        ಹಿಂತಿರುಗಿ (cvt_ಗಮ್ಯ_Hex(m_nodep) + "\\n " + cvt_ಗಮ್ಯ_Str(nodep()->typeName()));
    }
    string dotShape() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ VN_IS(m_nodep, ಸಕ್ರಿಯ) ? "doubleoctagon" : "rect"; }
    // LCOV_EXCL_STOP
};

class ಕ್ರಮ_Var_ಶೃಂಗ VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ ಕ್ರಮ_Either_ಶೃಂಗ {
    VL_RTTI_IMPL(ಕ್ರಮ_Var_ಶೃಂಗ, ಕ್ರಮ_Either_ಶೃಂಗ)
    AstVarScope* ಸ್ಥಿರಾಂಕ m_vscp;

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTOR
    ಕ್ರಮ_Var_ಶೃಂಗ(ಕ್ರಮ_ನಕ್ಷೆ* graphp, AstVarScope* vscp) VL_MT_DISABLED
        : ಕ್ರಮ_Either_ಶೃಂಗ{graphp, nullptr},
          m_vscp{vscp} {}
    ~ಕ್ರಮ_Var_ಶೃಂಗ() override = ಪೂರ್ವನಿಯೋಜಿತ;

    // ACCESSORS
    AstVarScope* vscp() ಸ್ಥಿರಾಂಕ { ಹಿಂತಿರುಗಿ m_vscp; }

    // LCOV_EXCL_START // Debug code
    string dotShape() ಸ್ಥಿರಾಂಕ override final { ಹಿಂತಿರುಗಿ "ellipse"; }
    ವಾಸ್ತವಿಕ string nameSuffix() ಸ್ಥಿರಾಂಕ VL_MT_ಸುರಕ್ಷಿತ = 0;
    string name() ಸ್ಥಿರಾಂಕ override final VL_MT_STABLE {
        ಹಿಂತಿರುಗಿ cvt_ಗಮ್ಯ_Hex(m_vscp) + " " + nameSuffix() + "\\n " + m_vscp->name();
    }
    // LCOV_EXCL_STOP
};

class ಕ್ರಮ_Var_Std_ಶೃಂಗ final : ಸಾರ್ವಜನಿಕ ಕ್ರಮ_Var_ಶೃಂಗ {
    VL_RTTI_IMPL(ಕ್ರಮ_Var_Std_ಶೃಂಗ, ಕ್ರಮ_Var_ಶೃಂಗ)
ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTOR
    ಕ್ರಮ_Var_Std_ಶೃಂಗ(ಕ್ರಮ_ನಕ್ಷೆ* graphp, AstVarScope* vscp) VL_MT_DISABLED
        : ಕ್ರಮ_Var_ಶೃಂಗ{graphp, vscp} {}
    ~ಕ್ರಮ_Var_Std_ಶೃಂಗ() override = ಪೂರ್ವನಿಯೋಜಿತ;

    // METHODS
    bool domainMatters() override { ಹಿಂತಿರುಗಿ true; }

    // LCOV_EXCL_START // Debug code
    string nameSuffix() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ ""; }
    string dotColor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "grey"; }
    // LCOV_EXCL_STOP
};

class ಕ್ರಮ_Var_Pre_ಶೃಂಗ final : ಸಾರ್ವಜನಿಕ ಕ್ರಮ_Var_ಶೃಂಗ {
    VL_RTTI_IMPL(ಕ್ರಮ_Var_Pre_ಶೃಂಗ, ಕ್ರಮ_Var_ಶೃಂಗ)
ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTOR
    ಕ್ರಮ_Var_Pre_ಶೃಂಗ(ಕ್ರಮ_ನಕ್ಷೆ* graphp, AstVarScope* vscp) VL_MT_DISABLED
        : ಕ್ರಮ_Var_ಶೃಂಗ{graphp, vscp} {}
    ~ಕ್ರಮ_Var_Pre_ಶೃಂಗ() override = ಪೂರ್ವನಿಯೋಜಿತ;

    // METHODS
    bool domainMatters() override { ಹಿಂತಿರುಗಿ false; }

    // LCOV_EXCL_START // Debug code
    string nameSuffix() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ "PRE"; }
    string dotColor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "green"; }
    // LCOV_EXCL_STOP
};

class ಕ್ರಮ_Var_Post_ಶೃಂಗ final : ಸಾರ್ವಜನಿಕ ಕ್ರಮ_Var_ಶೃಂಗ {
    VL_RTTI_IMPL(ಕ್ರಮ_Var_Post_ಶೃಂಗ, ಕ್ರಮ_Var_ಶೃಂಗ)
ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTOR
    ಕ್ರಮ_Var_Post_ಶೃಂಗ(ಕ್ರಮ_ನಕ್ಷೆ* graphp, AstVarScope* vscp) VL_MT_DISABLED
        : ಕ್ರಮ_Var_ಶೃಂಗ{graphp, vscp} {}
    ~ಕ್ರಮ_Var_Post_ಶೃಂಗ() override = ಪೂರ್ವನಿಯೋಜಿತ;

    // METHODS
    bool domainMatters() override { ಹಿಂತಿರುಗಿ false; }

    // LCOV_EXCL_START // Debug code
    string nameSuffix() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ "POST"; }
    string dotColor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "red"; }
    // LCOV_EXCL_STOP
};

class ಕ್ರಮ_Var_Pord_ಶೃಂಗ final : ಸಾರ್ವಜನಿಕ ಕ್ರಮ_Var_ಶೃಂಗ {
    VL_RTTI_IMPL(ಕ್ರಮ_Var_Pord_ಶೃಂಗ, ಕ್ರಮ_Var_ಶೃಂಗ)
ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTOR
    ಕ್ರಮ_Var_Pord_ಶೃಂಗ(ಕ್ರಮ_ನಕ್ಷೆ* graphp, AstVarScope* vscp) VL_MT_DISABLED
        : ಕ್ರಮ_Var_ಶೃಂಗ{graphp, vscp} {}
    ~ಕ್ರಮ_Var_Pord_ಶೃಂಗ() override = ಪೂರ್ವನಿಯೋಜಿತ;

    // METHODS
    bool domainMatters() override { ಹಿಂತಿರುಗಿ false; }

    // LCOV_EXCL_START // Debug code
    string nameSuffix() ಸ್ಥಿರಾಂಕ override VL_MT_ಸುರಕ್ಷಿತ { ಹಿಂತಿರುಗಿ "PORD"; }
    string dotColor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "blue"; }
    // LCOV_EXCL_STOP
};

//======================================================================
// Edge type

class ಕ್ರಮ_ಅಂಚು final : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ_ಅಂಚು {
    VL_RTTI_IMPL(ಕ್ರಮ_ಅಂಚು, V3_ನಕ್ಷೆ_ಅಂಚು)
    ಮಿತ್ರ class ಕ್ರಮ_ನಕ್ಷೆ;  // Only the OrderGraph can create these
    // CONSTRUCTOR
    ಕ್ರಮ_ಅಂಚು(ಕ್ರಮ_ನಕ್ಷೆ* graphp, ಕ್ರಮ_Either_ಶೃಂಗ* fromp, ಕ್ರಮ_Either_ಶೃಂಗ* ತುದಿ, ಪೂರ್ಣಾಂಕ ತೂಕ_2,
              bool cutable) VL_MT_DISABLED : V3_ನಕ್ಷೆ_ಅಂಚು{graphp, fromp, ತುದಿ, ತೂಕ_2, cutable} {}
    ~ಕ್ರಮ_ಅಂಚು() override = ಪೂರ್ವನಿಯೋಜಿತ;

    // LCOV_EXCL_START // Debug code
    string dotColor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ cutable() ? "green" : "red"; }
    // LCOV_EXCL_STOP
};

//======================================================================
// Inline methods

ಶೂನ್ಯ ಕ್ರಮ_ನಕ್ಷೆ::ಕೂಡಿಸು_Hard_ಅಂಚು(ಕ್ರಮ_Logic_ಶೃಂಗ* fromp, ಕ್ರಮ_Var_ಶೃಂಗ* ತುದಿ,
                             ಪೂರ್ಣಾಂಕ ತೂಕ_2) VL_MT_DISABLED {
    new ಕ್ರಮ_ಅಂಚು{this, fromp, ತುದಿ, ತೂಕ_2, /* cutable: */ false};
}
ಶೂನ್ಯ ಕ್ರಮ_ನಕ್ಷೆ::ಕೂಡಿಸು_Hard_ಅಂಚು(ಕ್ರಮ_Var_ಶೃಂಗ* fromp, ಕ್ರಮ_Logic_ಶೃಂಗ* ತುದಿ,
                             ಪೂರ್ಣಾಂಕ ತೂಕ_2) VL_MT_DISABLED {
    new ಕ್ರಮ_ಅಂಚು{this, fromp, ತುದಿ, ತೂಕ_2, /* cutable: */ false};
}
ಶೂನ್ಯ ಕ್ರಮ_ನಕ್ಷೆ::ಕೂಡಿಸು_Soft_ಅಂಚು(ಕ್ರಮ_Logic_ಶೃಂಗ* fromp, ಕ್ರಮ_Var_ಶೃಂಗ* ತುದಿ,
                             ಪೂರ್ಣಾಂಕ ತೂಕ_2) VL_MT_DISABLED {
    new ಕ್ರಮ_ಅಂಚು{this, fromp, ತುದಿ, ತೂಕ_2, /* cutable: */ true};
}
ಶೂನ್ಯ ಕ್ರಮ_ನಕ್ಷೆ::ಕೂಡಿಸು_Soft_ಅಂಚು(ಕ್ರಮ_Var_ಶೃಂಗ* fromp, ಕ್ರಮ_Logic_ಶೃಂಗ* ತುದಿ,
                             ಪೂರ್ಣಾಂಕ ತೂಕ_2) VL_MT_DISABLED {
    new ಕ್ರಮ_ಅಂಚು{this, fromp, ತುದಿ, ತೂಕ_2, /* cutable: */ true};
}

#ಆದರೆ_ಅಂತ್ಯ  // Guard
