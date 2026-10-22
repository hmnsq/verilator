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

#جيڪڏهن_وصف_نه_ٿيل VERILATOR_V3ORDERGRAPH_H_
#وصف_ڪريو VERILATOR_V3ORDERGRAPH_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"
#ضمن "V3Graph.h"
#ضمن "V3Sched.h"

#ضمن <set>
#ضمن <vector>

class ترتيب_Logic_راس;
class ترتيب_Var_راس;

//======================================================================

تعداد ترتيب_Weights : uint8_t {
    وزن_COMBO = 1,  // Breakable combo logic
    وزن_POST = 2,  // Post-delayed used var
    وزن_PRE = 3,  // Breakable pre-delayed used var
    وزن_MEDIUM = 8,  // Medium weight just so dot graph looks nice
    وزن_عام = 32  // High weight just so dot graph looks nice
};

//======================================================================
// Graph type

// The sensitivity domain of a vertex: the set of trigger indices,
// any of which being set means the logic needs to be executed.
// Instances are interned in the OrderGraph, so can compare pointers.
class ترتيب_Domain final {
    ثابت std::سيٽ<uint32_t> m_triggers;  // The trigger indices

عام:
    explicit ترتيب_Domain(ثابت std::سيٽ<uint32_t>& triggers)
        : m_triggers{triggers} {}

    ثابت std::سيٽ<uint32_t>& triggers() ثابت { أرجع m_triggers; }
    bool عامل<(ثابت ترتيب_Domain& other) ثابت { أرجع m_triggers < other.m_triggers; }
};

class ترتيب_گراف final : عام V3_گراف {
    ثابت V3Sched::TriggerKit& m_trigKit;  // The TriggerKit of the triggers
    AstVarScope* ثابت m_trigVscp;  // The trigger vector for this graph
    std::سيٽ<ترتيب_Domain> m_domains;  // The interned domains

عام:
    // CONSTRUCTOR
    ترتيب_گراف(ثابت V3Sched::TriggerKit& trigKit, AstVarScope* trigVscp)
        : m_trigKit{trigKit}
        , m_trigVscp{trigVscp} {}

    // METHODS
    AstVarScope* trigVscp() ثابت { أرجع m_trigVscp; }

    // The AstSenItem the given trigger index stands for, nullptr if none
    ثابت Ast_Sen_شي* trigSenItemp(uint32_t اشاريو) ثابت {
        أرجع m_trigKit.trig_Idx2_Sen_شي().at(اشاريو);
    }

    // Get the interned instance of the given domain
    ثابت ترتيب_Domain* domainp(ثابت std::سيٽ<uint32_t>& triggers) {
        أرجع &*m_domains.emplace(triggers).پهريون;
    }

    // Methods to add edges representing constraints, utilizing the type system to help us ensure
    // the graph remains bipartite.
    مضمن فراغ جوڙيو_Hard_ڪنارو(ترتيب_Logic_راس* fromp, ترتيب_Var_راس* چوٽي,
                            صحيح وزن_2) VL_MT_DISABLED;
    مضمن فراغ جوڙيو_Hard_ڪنارو(ترتيب_Var_راس* fromp, ترتيب_Logic_راس* چوٽي,
                            صحيح وزن_2) VL_MT_DISABLED;
    مضمن فراغ جوڙيو_Soft_ڪنارو(ترتيب_Logic_راس* fromp, ترتيب_Var_راس* چوٽي,
                            صحيح وزن_2) VL_MT_DISABLED;
    مضمن فراغ جوڙيو_Soft_ڪنارو(ترتيب_Var_راس* fromp, ترتيب_Logic_راس* چوٽي,
                            صحيح وزن_2) VL_MT_DISABLED;
};

//======================================================================
// Vertex types

class ترتيب_Either_راس VL_NOT_FINAL : عام V3_گراف_راس {
    VL_RTTI_IMPL(ترتيب_Either_راس, V3_گراف_راس)
    // Event domain of vertex. For OrderLogicVertex this represents the conditions when the logic
    // block must be executed. For OrderVarVertex, this is the union of the domains of all the
    // OrderLogicVertex vertices that drive the variable. If initially set to nullptr (e.g.: all
    // OrderVarVertex and those OrderLogicVertices that represent combinational logic), then the
    // ordering algorithm will compute the domain automatically based on the edges representing
    // data-flow (those between OrderLogicVertex and OrderVarStdVertex), otherwise the domain is
    // as given (e.g.: for those OrderLogicVertices that represent clocked logic).
    ثابت ترتيب_Domain* m_domainp;

محمي:
    // CONSTRUCTOR
    ترتيب_Either_راس(ترتيب_گراف* graphp, ثابت ترتيب_Domain* domainp) VL_MT_DISABLED
        : V3_گراف_راس{graphp},
          m_domainp{domainp} {}
    ~ترتيب_Either_راس() override = افتراضي;

عام:
    // METHODS
    مجازي bool domainMatters() = 0;

    // ACCESSORS
    ثابت ترتيب_Domain* domainp() ثابت VL_MT_STABLE { أرجع m_domainp; }
    فراغ domainp(ثابت ترتيب_Domain* domainp) VL_MT_DISABLED {
#إذا VL_DEBUG
        UASSERT(!m_domainp, "Domain should only be set once");
#جيڪڏهن_پورو
        m_domainp = domainp;
    }
};

class ترتيب_Logic_راس final : عام ترتيب_Either_راس {
    VL_RTTI_IMPL(ترتيب_Logic_راس, ترتيب_Either_راس)

عام:
    // Variable access record
    بنية VarAccess final {
        AstVarScope* m_vscp;  // The variable accessed
        VAccess m_access;  // The kind of access, as in the AST
    };

خاص:
    Ast_ڳنڍ* ثابت m_nodep;  // The logic this vertex represents
    AstScope* ثابت m_scopep;  // Scope the logic is under
    ثابت ترتيب_Domain* ثابت m_hybridp;  // Additional sensitivities for hybrid combinational logic
    ثابت bool m_comb;  // Is from combinational logic
    // Every variable accessed by this logic, in order of first access, at most one record per
    // variable. Only populated for multi-threaded ordering.
    std::ويڪٽر<VarAccess> m_varAccesses;

عام:
    // CONSTRUCTOR
    ترتيب_Logic_راس(ترتيب_گراف* graphp, AstScope* scopep, ثابت ترتيب_Domain* domainp,
                     ثابت ترتيب_Domain* hybridp, Ast_ڳنڍ* nodep) VL_MT_DISABLED
        : ترتيب_Either_راس{graphp, domainp},
          m_nodep{nodep},
          m_scopep{scopep},
          m_hybridp{hybridp},
          m_comb{!domainp && !hybridp} {
        UASSERT_OBJ(scopep, nodep, "Must not be null");
        UASSERT_OBJ(!(domainp && hybridp), nodep, "Cannot have bot domainp and hybridp set");
    }
    ~ترتيب_Logic_راس() override = افتراضي;

    // METHODS
    bool domainMatters() override { أرجع true; }

    // ACCESSORS
    Ast_ڳنڍ* nodep() ثابت VL_MT_STABLE { أرجع m_nodep; }
    AstScope* scopep() ثابت VL_MT_STABLE { أرجع m_scopep; }
    ثابت ترتيب_Domain* hybridp() ثابت { أرجع m_hybridp; }
    bool comb() ثابت { أرجع m_comb; }
    ثابت std::ويڪٽر<VarAccess>& varAccesses() ثابت { أرجع m_varAccesses; }
    فراغ جوڙيو_Var_Access(AstVarScope* vscp, VAccess access) {
        m_varAccesses.ڌڪو_پٺ({vscp, access});
    }

    // LCOV_EXCL_START // Debug code
    string name() ثابت override VL_MT_STABLE {
        أرجع (cvt_منزل_Hex(m_nodep) + "\\n " + cvt_منزل_Str(nodep()->typeName()));
    }
    string dotShape() ثابت override { أرجع VN_IS(m_nodep, سرگرم) ? "doubleoctagon" : "rect"; }
    // LCOV_EXCL_STOP
};

class ترتيب_Var_راس VL_NOT_FINAL : عام ترتيب_Either_راس {
    VL_RTTI_IMPL(ترتيب_Var_راس, ترتيب_Either_راس)
    AstVarScope* ثابت m_vscp;

عام:
    // CONSTRUCTOR
    ترتيب_Var_راس(ترتيب_گراف* graphp, AstVarScope* vscp) VL_MT_DISABLED
        : ترتيب_Either_راس{graphp, nullptr},
          m_vscp{vscp} {}
    ~ترتيب_Var_راس() override = افتراضي;

    // ACCESSORS
    AstVarScope* vscp() ثابت { أرجع m_vscp; }

    // LCOV_EXCL_START // Debug code
    string dotShape() ثابت override final { أرجع "ellipse"; }
    مجازي string nameSuffix() ثابت VL_MT_محفوظ = 0;
    string name() ثابت override final VL_MT_STABLE {
        أرجع cvt_منزل_Hex(m_vscp) + " " + nameSuffix() + "\\n " + m_vscp->name();
    }
    // LCOV_EXCL_STOP
};

class ترتيب_Var_Std_راس final : عام ترتيب_Var_راس {
    VL_RTTI_IMPL(ترتيب_Var_Std_راس, ترتيب_Var_راس)
عام:
    // CONSTRUCTOR
    ترتيب_Var_Std_راس(ترتيب_گراف* graphp, AstVarScope* vscp) VL_MT_DISABLED
        : ترتيب_Var_راس{graphp, vscp} {}
    ~ترتيب_Var_Std_راس() override = افتراضي;

    // METHODS
    bool domainMatters() override { أرجع true; }

    // LCOV_EXCL_START // Debug code
    string nameSuffix() ثابت override VL_MT_محفوظ { أرجع ""; }
    string dotColor() ثابت override { أرجع "grey"; }
    // LCOV_EXCL_STOP
};

class ترتيب_Var_Pre_راس final : عام ترتيب_Var_راس {
    VL_RTTI_IMPL(ترتيب_Var_Pre_راس, ترتيب_Var_راس)
عام:
    // CONSTRUCTOR
    ترتيب_Var_Pre_راس(ترتيب_گراف* graphp, AstVarScope* vscp) VL_MT_DISABLED
        : ترتيب_Var_راس{graphp, vscp} {}
    ~ترتيب_Var_Pre_راس() override = افتراضي;

    // METHODS
    bool domainMatters() override { أرجع false; }

    // LCOV_EXCL_START // Debug code
    string nameSuffix() ثابت override VL_MT_محفوظ { أرجع "PRE"; }
    string dotColor() ثابت override { أرجع "green"; }
    // LCOV_EXCL_STOP
};

class ترتيب_Var_Post_راس final : عام ترتيب_Var_راس {
    VL_RTTI_IMPL(ترتيب_Var_Post_راس, ترتيب_Var_راس)
عام:
    // CONSTRUCTOR
    ترتيب_Var_Post_راس(ترتيب_گراف* graphp, AstVarScope* vscp) VL_MT_DISABLED
        : ترتيب_Var_راس{graphp, vscp} {}
    ~ترتيب_Var_Post_راس() override = افتراضي;

    // METHODS
    bool domainMatters() override { أرجع false; }

    // LCOV_EXCL_START // Debug code
    string nameSuffix() ثابت override VL_MT_محفوظ { أرجع "POST"; }
    string dotColor() ثابت override { أرجع "red"; }
    // LCOV_EXCL_STOP
};

class ترتيب_Var_Pord_راس final : عام ترتيب_Var_راس {
    VL_RTTI_IMPL(ترتيب_Var_Pord_راس, ترتيب_Var_راس)
عام:
    // CONSTRUCTOR
    ترتيب_Var_Pord_راس(ترتيب_گراف* graphp, AstVarScope* vscp) VL_MT_DISABLED
        : ترتيب_Var_راس{graphp, vscp} {}
    ~ترتيب_Var_Pord_راس() override = افتراضي;

    // METHODS
    bool domainMatters() override { أرجع false; }

    // LCOV_EXCL_START // Debug code
    string nameSuffix() ثابت override VL_MT_محفوظ { أرجع "PORD"; }
    string dotColor() ثابت override { أرجع "blue"; }
    // LCOV_EXCL_STOP
};

//======================================================================
// Edge type

class ترتيب_ڪنارو final : عام V3_گراف_ڪنارو {
    VL_RTTI_IMPL(ترتيب_ڪنارو, V3_گراف_ڪنارو)
    دوست class ترتيب_گراف;  // Only the OrderGraph can create these
    // CONSTRUCTOR
    ترتيب_ڪنارو(ترتيب_گراف* graphp, ترتيب_Either_راس* fromp, ترتيب_Either_راس* چوٽي, صحيح وزن_2,
              bool cutable) VL_MT_DISABLED : V3_گراف_ڪنارو{graphp, fromp, چوٽي, وزن_2, cutable} {}
    ~ترتيب_ڪنارو() override = افتراضي;

    // LCOV_EXCL_START // Debug code
    string dotColor() ثابت override { أرجع cutable() ? "green" : "red"; }
    // LCOV_EXCL_STOP
};

//======================================================================
// Inline methods

فراغ ترتيب_گراف::جوڙيو_Hard_ڪنارو(ترتيب_Logic_راس* fromp, ترتيب_Var_راس* چوٽي,
                             صحيح وزن_2) VL_MT_DISABLED {
    new ترتيب_ڪنارو{this, fromp, چوٽي, وزن_2, /* cutable: */ false};
}
فراغ ترتيب_گراف::جوڙيو_Hard_ڪنارو(ترتيب_Var_راس* fromp, ترتيب_Logic_راس* چوٽي,
                             صحيح وزن_2) VL_MT_DISABLED {
    new ترتيب_ڪنارو{this, fromp, چوٽي, وزن_2, /* cutable: */ false};
}
فراغ ترتيب_گراف::جوڙيو_Soft_ڪنارو(ترتيب_Logic_راس* fromp, ترتيب_Var_راس* چوٽي,
                             صحيح وزن_2) VL_MT_DISABLED {
    new ترتيب_ڪنارو{this, fromp, چوٽي, وزن_2, /* cutable: */ true};
}
فراغ ترتيب_گراف::جوڙيو_Soft_ڪنارو(ترتيب_Var_راس* fromp, ترتيب_Logic_راس* چوٽي,
                             صحيح وزن_2) VL_MT_DISABLED {
    new ترتيب_ڪنارو{this, fromp, چوٽي, وزن_2, /* cutable: */ true};
}

#جيڪڏهن_پورو  // Guard
