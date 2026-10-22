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

#جے_بنا_تعریف VERILATOR_V3ORDERGRAPH_H_
#تعریف_کرو VERILATOR_V3ORDERGRAPH_H_

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Ast.h"
#ضمن "V3Graph.h"
#ضمن "V3Sched.h"

#ضمن <set>
#ضمن <vector>

class ترتیب_Logic_راس;
class ترتیب_Var_راس;

//======================================================================

تعداد ترتیب_Weights : uint8_t {
    بھار_COMBO = 1,  // Breakable combo logic
    بھار_POST = 2,  // Post-delayed used var
    بھار_PRE = 3,  // Breakable pre-delayed used var
    بھار_MEDIUM = 8,  // Medium weight just so dot graph looks nice
    بھار_عام = 32  // High weight just so dot graph looks nice
};

//======================================================================
// Graph type

// The sensitivity domain of a vertex: the set of trigger indices,
// any of which being set means the logic needs to be executed.
// Instances are interned in the OrderGraph, so can compare pointers.
class ترتیب_Domain final {
    ثابت std::سیٹ<uint32_t> m_triggers;  // The trigger indices

عام:
    explicit ترتیب_Domain(ثابت std::سیٹ<uint32_t>& triggers)
        : m_triggers{triggers} {}

    ثابت std::سیٹ<uint32_t>& triggers() ثابت { أرجع m_triggers; }
    bool عامل<(ثابت ترتیب_Domain& other) ثابت { أرجع m_triggers < other.m_triggers; }
};

class ترتیب_گراف final : عام V3_گراف {
    ثابت V3Sched::TriggerKit& m_trigKit;  // The TriggerKit of the triggers
    AstVarScope* ثابت m_trigVscp;  // The trigger vector for this graph
    std::سیٹ<ترتیب_Domain> m_domains;  // The interned domains

عام:
    // CONSTRUCTOR
    ترتیب_گراف(ثابت V3Sched::TriggerKit& trigKit, AstVarScope* trigVscp)
        : m_trigKit{trigKit}
        , m_trigVscp{trigVscp} {}

    // METHODS
    AstVarScope* trigVscp() ثابت { أرجع m_trigVscp; }

    // The AstSenItem the given trigger index stands for, nullptr if none
    ثابت Ast_Sen_شے* trigSenItemp(uint32_t اشاریہ) ثابت {
        أرجع m_trigKit.trig_Idx2_Sen_شے().at(اشاریہ);
    }

    // Get the interned instance of the given domain
    ثابت ترتیب_Domain* domainp(ثابت std::سیٹ<uint32_t>& triggers) {
        أرجع &*m_domains.emplace(triggers).پہلا;
    }

    // Methods to add edges representing constraints, utilizing the type system to help us ensure
    // the graph remains bipartite.
    مضمن فراغ جوڑو_Hard_کنارا(ترتیب_Logic_راس* fromp, ترتیب_Var_راس* چوٹی,
                            صحيح بھار_2) VL_MT_DISABLED;
    مضمن فراغ جوڑو_Hard_کنارا(ترتیب_Var_راس* fromp, ترتیب_Logic_راس* چوٹی,
                            صحيح بھار_2) VL_MT_DISABLED;
    مضمن فراغ جوڑو_Soft_کنارا(ترتیب_Logic_راس* fromp, ترتیب_Var_راس* چوٹی,
                            صحيح بھار_2) VL_MT_DISABLED;
    مضمن فراغ جوڑو_Soft_کنارا(ترتیب_Var_راس* fromp, ترتیب_Logic_راس* چوٹی,
                            صحيح بھار_2) VL_MT_DISABLED;
};

//======================================================================
// Vertex types

class ترتیب_Either_راس VL_NOT_FINAL : عام V3_گراف_راس {
    VL_RTTI_IMPL(ترتیب_Either_راس, V3_گراف_راس)
    // Event domain of vertex. For OrderLogicVertex this represents the conditions when the logic
    // block must be executed. For OrderVarVertex, this is the union of the domains of all the
    // OrderLogicVertex vertices that drive the variable. If initially set to nullptr (e.g.: all
    // OrderVarVertex and those OrderLogicVertices that represent combinational logic), then the
    // ordering algorithm will compute the domain automatically based on the edges representing
    // data-flow (those between OrderLogicVertex and OrderVarStdVertex), otherwise the domain is
    // as given (e.g.: for those OrderLogicVertices that represent clocked logic).
    ثابت ترتیب_Domain* m_domainp;

محمي:
    // CONSTRUCTOR
    ترتیب_Either_راس(ترتیب_گراف* graphp, ثابت ترتیب_Domain* domainp) VL_MT_DISABLED
        : V3_گراف_راس{graphp},
          m_domainp{domainp} {}
    ~ترتیب_Either_راس() override = افتراضي;

عام:
    // METHODS
    مجازی bool domainMatters() = 0;

    // ACCESSORS
    ثابت ترتیب_Domain* domainp() ثابت VL_MT_STABLE { أرجع m_domainp; }
    فراغ domainp(ثابت ترتیب_Domain* domainp) VL_MT_DISABLED {
#إذا VL_DEBUG
        UASSERT(!m_domainp, "Domain should only be set once");
#جے_انت
        m_domainp = domainp;
    }
};

class ترتیب_Logic_راس final : عام ترتیب_Either_راس {
    VL_RTTI_IMPL(ترتیب_Logic_راس, ترتیب_Either_راس)

عام:
    // Variable access record
    بنية VarAccess final {
        AstVarScope* m_vscp;  // The variable accessed
        VAccess m_access;  // The kind of access, as in the AST
    };

خاص:
    Ast_گرہ* ثابت m_nodep;  // The logic this vertex represents
    AstScope* ثابت m_scopep;  // Scope the logic is under
    ثابت ترتیب_Domain* ثابت m_hybridp;  // Additional sensitivities for hybrid combinational logic
    ثابت bool m_comb;  // Is from combinational logic
    // Every variable accessed by this logic, in order of first access, at most one record per
    // variable. Only populated for multi-threaded ordering.
    std::سمتیہ<VarAccess> m_varAccesses;

عام:
    // CONSTRUCTOR
    ترتیب_Logic_راس(ترتیب_گراف* graphp, AstScope* scopep, ثابت ترتیب_Domain* domainp,
                     ثابت ترتیب_Domain* hybridp, Ast_گرہ* nodep) VL_MT_DISABLED
        : ترتیب_Either_راس{graphp, domainp},
          m_nodep{nodep},
          m_scopep{scopep},
          m_hybridp{hybridp},
          m_comb{!domainp && !hybridp} {
        UASSERT_OBJ(scopep, nodep, "Must not be null");
        UASSERT_OBJ(!(domainp && hybridp), nodep, "Cannot have bot domainp and hybridp set");
    }
    ~ترتیب_Logic_راس() override = افتراضي;

    // METHODS
    bool domainMatters() override { أرجع true; }

    // ACCESSORS
    Ast_گرہ* nodep() ثابت VL_MT_STABLE { أرجع m_nodep; }
    AstScope* scopep() ثابت VL_MT_STABLE { أرجع m_scopep; }
    ثابت ترتیب_Domain* hybridp() ثابت { أرجع m_hybridp; }
    bool comb() ثابت { أرجع m_comb; }
    ثابت std::سمتیہ<VarAccess>& varAccesses() ثابت { أرجع m_varAccesses; }
    فراغ جوڑو_Var_Access(AstVarScope* vscp, VAccess access) {
        m_varAccesses.ڈالو_پٹھ({vscp, access});
    }

    // LCOV_EXCL_START // Debug code
    string name() ثابت override VL_MT_STABLE {
        أرجع (cvt_منزل_Hex(m_nodep) + "\\n " + cvt_منزل_Str(nodep()->typeName()));
    }
    string dotShape() ثابت override { أرجع VN_IS(m_nodep, سرگرم) ? "doubleoctagon" : "rect"; }
    // LCOV_EXCL_STOP
};

class ترتیب_Var_راس VL_NOT_FINAL : عام ترتیب_Either_راس {
    VL_RTTI_IMPL(ترتیب_Var_راس, ترتیب_Either_راس)
    AstVarScope* ثابت m_vscp;

عام:
    // CONSTRUCTOR
    ترتیب_Var_راس(ترتیب_گراف* graphp, AstVarScope* vscp) VL_MT_DISABLED
        : ترتیب_Either_راس{graphp, nullptr},
          m_vscp{vscp} {}
    ~ترتیب_Var_راس() override = افتراضي;

    // ACCESSORS
    AstVarScope* vscp() ثابت { أرجع m_vscp; }

    // LCOV_EXCL_START // Debug code
    string dotShape() ثابت override final { أرجع "ellipse"; }
    مجازی string nameSuffix() ثابت VL_MT_محفوظ = 0;
    string name() ثابت override final VL_MT_STABLE {
        أرجع cvt_منزل_Hex(m_vscp) + " " + nameSuffix() + "\\n " + m_vscp->name();
    }
    // LCOV_EXCL_STOP
};

class ترتیب_Var_Std_راس final : عام ترتیب_Var_راس {
    VL_RTTI_IMPL(ترتیب_Var_Std_راس, ترتیب_Var_راس)
عام:
    // CONSTRUCTOR
    ترتیب_Var_Std_راس(ترتیب_گراف* graphp, AstVarScope* vscp) VL_MT_DISABLED
        : ترتیب_Var_راس{graphp, vscp} {}
    ~ترتیب_Var_Std_راس() override = افتراضي;

    // METHODS
    bool domainMatters() override { أرجع true; }

    // LCOV_EXCL_START // Debug code
    string nameSuffix() ثابت override VL_MT_محفوظ { أرجع ""; }
    string dotColor() ثابت override { أرجع "grey"; }
    // LCOV_EXCL_STOP
};

class ترتیب_Var_Pre_راس final : عام ترتیب_Var_راس {
    VL_RTTI_IMPL(ترتیب_Var_Pre_راس, ترتیب_Var_راس)
عام:
    // CONSTRUCTOR
    ترتیب_Var_Pre_راس(ترتیب_گراف* graphp, AstVarScope* vscp) VL_MT_DISABLED
        : ترتیب_Var_راس{graphp, vscp} {}
    ~ترتیب_Var_Pre_راس() override = افتراضي;

    // METHODS
    bool domainMatters() override { أرجع false; }

    // LCOV_EXCL_START // Debug code
    string nameSuffix() ثابت override VL_MT_محفوظ { أرجع "PRE"; }
    string dotColor() ثابت override { أرجع "green"; }
    // LCOV_EXCL_STOP
};

class ترتیب_Var_Post_راس final : عام ترتیب_Var_راس {
    VL_RTTI_IMPL(ترتیب_Var_Post_راس, ترتیب_Var_راس)
عام:
    // CONSTRUCTOR
    ترتیب_Var_Post_راس(ترتیب_گراف* graphp, AstVarScope* vscp) VL_MT_DISABLED
        : ترتیب_Var_راس{graphp, vscp} {}
    ~ترتیب_Var_Post_راس() override = افتراضي;

    // METHODS
    bool domainMatters() override { أرجع false; }

    // LCOV_EXCL_START // Debug code
    string nameSuffix() ثابت override VL_MT_محفوظ { أرجع "POST"; }
    string dotColor() ثابت override { أرجع "red"; }
    // LCOV_EXCL_STOP
};

class ترتیب_Var_Pord_راس final : عام ترتیب_Var_راس {
    VL_RTTI_IMPL(ترتیب_Var_Pord_راس, ترتیب_Var_راس)
عام:
    // CONSTRUCTOR
    ترتیب_Var_Pord_راس(ترتیب_گراف* graphp, AstVarScope* vscp) VL_MT_DISABLED
        : ترتیب_Var_راس{graphp, vscp} {}
    ~ترتیب_Var_Pord_راس() override = افتراضي;

    // METHODS
    bool domainMatters() override { أرجع false; }

    // LCOV_EXCL_START // Debug code
    string nameSuffix() ثابت override VL_MT_محفوظ { أرجع "PORD"; }
    string dotColor() ثابت override { أرجع "blue"; }
    // LCOV_EXCL_STOP
};

//======================================================================
// Edge type

class ترتیب_کنارا final : عام V3_گراف_کنارا {
    VL_RTTI_IMPL(ترتیب_کنارا, V3_گراف_کنارا)
    یار class ترتیب_گراف;  // Only the OrderGraph can create these
    // CONSTRUCTOR
    ترتیب_کنارا(ترتیب_گراف* graphp, ترتیب_Either_راس* fromp, ترتیب_Either_راس* چوٹی, صحيح بھار_2,
              bool cutable) VL_MT_DISABLED : V3_گراف_کنارا{graphp, fromp, چوٹی, بھار_2, cutable} {}
    ~ترتیب_کنارا() override = افتراضي;

    // LCOV_EXCL_START // Debug code
    string dotColor() ثابت override { أرجع cutable() ? "green" : "red"; }
    // LCOV_EXCL_STOP
};

//======================================================================
// Inline methods

فراغ ترتیب_گراف::جوڑو_Hard_کنارا(ترتیب_Logic_راس* fromp, ترتیب_Var_راس* چوٹی,
                             صحيح بھار_2) VL_MT_DISABLED {
    new ترتیب_کنارا{this, fromp, چوٹی, بھار_2, /* cutable: */ false};
}
فراغ ترتیب_گراف::جوڑو_Hard_کنارا(ترتیب_Var_راس* fromp, ترتیب_Logic_راس* چوٹی,
                             صحيح بھار_2) VL_MT_DISABLED {
    new ترتیب_کنارا{this, fromp, چوٹی, بھار_2, /* cutable: */ false};
}
فراغ ترتیب_گراف::جوڑو_Soft_کنارا(ترتیب_Logic_راس* fromp, ترتیب_Var_راس* چوٹی,
                             صحيح بھار_2) VL_MT_DISABLED {
    new ترتیب_کنارا{this, fromp, چوٹی, بھار_2, /* cutable: */ true};
}
فراغ ترتیب_گراف::جوڑو_Soft_کنارا(ترتیب_Var_راس* fromp, ترتیب_Logic_راس* چوٹی,
                             صحيح بھار_2) VL_MT_DISABLED {
    new ترتیب_کنارا{this, fromp, چوٹی, بھار_2, /* cutable: */ true};
}

#جے_انت  // Guard
