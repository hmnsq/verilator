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

#నిర్వచితం_కాకపోతే VERILATOR_V3ORDERGRAPH_H_
#నిర్వచించు VERILATOR_V3ORDERGRAPH_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Ast.h"
#అన్తర్భావ "V3Graph.h"
#అన్తర్భావ "V3Sched.h"

#అన్తర్భావ <set>
#అన్తర్భావ <vector>

class క్రమం_Logic_శీర్షం;
class క్రమం_Var_శీర్షం;

//======================================================================

గణన క్రమం_Weights : uint8_t {
    బరువు_COMBO = 1,  // Breakable combo logic
    బరువు_POST = 2,  // Post-delayed used var
    బరువు_PRE = 3,  // Breakable pre-delayed used var
    బరువు_MEDIUM = 8,  // Medium weight just so dot graph looks nice
    బరువు_సాధారణ = 32  // High weight just so dot graph looks nice
};

//======================================================================
// Graph type

// The sensitivity domain of a vertex: the set of trigger indices,
// any of which being set means the logic needs to be executed.
// Instances are interned in the OrderGraph, so can compare pointers.
class క్రమం_Domain final {
    స్థిరరాశి std::సమితి<uint32_t> m_triggers;  // The trigger indices

బహిరంగ:
    explicit క్రమం_Domain(స్థిరరాశి std::సమితి<uint32_t>& triggers)
        : m_triggers{triggers} {}

    స్థిరరాశి std::సమితి<uint32_t>& triggers() స్థిరరాశి { తిరిగి m_triggers; }
    bool క్రియాకారకం<(స్థిరరాశి క్రమం_Domain& other) స్థిరరాశి { తిరిగి m_triggers < other.m_triggers; }
};

class క్రమం_లేఖాచిత్రం final : బహిరంగ V3_లేఖాచిత్రం {
    స్థిరరాశి V3Sched::TriggerKit& m_trigKit;  // The TriggerKit of the triggers
    AstVarScope* స్థిరరాశి m_trigVscp;  // The trigger vector for this graph
    std::సమితి<క్రమం_Domain> m_domains;  // The interned domains

బహిరంగ:
    // CONSTRUCTOR
    క్రమం_లేఖాచిత్రం(స్థిరరాశి V3Sched::TriggerKit& trigKit, AstVarScope* trigVscp)
        : m_trigKit{trigKit}
        , m_trigVscp{trigVscp} {}

    // METHODS
    AstVarScope* trigVscp() స్థిరరాశి { తిరిగి m_trigVscp; }

    // The AstSenItem the given trigger index stands for, nullptr if none
    స్థిరరాశి Ast_Sen_అంశం* trigSenItemp(uint32_t సూచీ) స్థిరరాశి {
        తిరిగి m_trigKit.trig_Idx2_Sen_అంశం().at(సూచీ);
    }

    // Get the interned instance of the given domain
    స్థిరరాశి క్రమం_Domain* domainp(స్థిరరాశి std::సమితి<uint32_t>& triggers) {
        తిరిగి &*m_domains.emplace(triggers).మొదటి;
    }

    // Methods to add edges representing constraints, utilizing the type system to help us ensure
    // the graph remains bipartite.
    అంతఃవరుస శూన్య కూడు_Hard_అంచు(క్రమం_Logic_శీర్షం* fromp, క్రమం_Var_శీర్షం* శిఖరం,
                            పూర్ణాంకం బరువు_2) VL_MT_DISABLED;
    అంతఃవరుస శూన్య కూడు_Hard_అంచు(క్రమం_Var_శీర్షం* fromp, క్రమం_Logic_శీర్షం* శిఖరం,
                            పూర్ణాంకం బరువు_2) VL_MT_DISABLED;
    అంతఃవరుస శూన్య కూడు_Soft_అంచు(క్రమం_Logic_శీర్షం* fromp, క్రమం_Var_శీర్షం* శిఖరం,
                            పూర్ణాంకం బరువు_2) VL_MT_DISABLED;
    అంతఃవరుస శూన్య కూడు_Soft_అంచు(క్రమం_Var_శీర్షం* fromp, క్రమం_Logic_శీర్షం* శిఖరం,
                            పూర్ణాంకం బరువు_2) VL_MT_DISABLED;
};

//======================================================================
// Vertex types

class క్రమం_Either_శీర్షం VL_NOT_FINAL : బహిరంగ V3_లేఖాచిత్రం_శీర్షం {
    VL_RTTI_IMPL(క్రమం_Either_శీర్షం, V3_లేఖాచిత్రం_శీర్షం)
    // Event domain of vertex. For OrderLogicVertex this represents the conditions when the logic
    // block must be executed. For OrderVarVertex, this is the union of the domains of all the
    // OrderLogicVertex vertices that drive the variable. If initially set to nullptr (e.g.: all
    // OrderVarVertex and those OrderLogicVertices that represent combinational logic), then the
    // ordering algorithm will compute the domain automatically based on the edges representing
    // data-flow (those between OrderLogicVertex and OrderVarStdVertex), otherwise the domain is
    // as given (e.g.: for those OrderLogicVertices that represent clocked logic).
    స్థిరరాశి క్రమం_Domain* m_domainp;

రక్షిత:
    // CONSTRUCTOR
    క్రమం_Either_శీర్షం(క్రమం_లేఖాచిత్రం* graphp, స్థిరరాశి క్రమం_Domain* domainp) VL_MT_DISABLED
        : V3_లేఖాచిత్రం_శీర్షం{graphp},
          m_domainp{domainp} {}
    ~క్రమం_Either_శీర్షం() override = అప్రమేయ;

బహిరంగ:
    // METHODS
    మాయా bool domainMatters() = 0;

    // ACCESSORS
    స్థిరరాశి క్రమం_Domain* domainp() స్థిరరాశి VL_MT_STABLE { తిరిగి m_domainp; }
    శూన్య domainp(స్థిరరాశి క్రమం_Domain* domainp) VL_MT_DISABLED {
#ఒకవేళ VL_DEBUG
        UASSERT(!m_domainp, "Domain should only be set once");
#అయితే_ముగింపు
        m_domainp = domainp;
    }
};

class క్రమం_Logic_శీర్షం final : బహిరంగ క్రమం_Either_శీర్షం {
    VL_RTTI_IMPL(క్రమం_Logic_శీర్షం, క్రమం_Either_శీర్షం)

బహిరంగ:
    // Variable access record
    నిర్మాణం VarAccess final {
        AstVarScope* m_vscp;  // The variable accessed
        VAccess m_access;  // The kind of access, as in the AST
    };

అంతరంగ:
    Ast_కణుపు* స్థిరరాశి m_nodep;  // The logic this vertex represents
    AstScope* స్థిరరాశి m_scopep;  // Scope the logic is under
    స్థిరరాశి క్రమం_Domain* స్థిరరాశి m_hybridp;  // Additional sensitivities for hybrid combinational logic
    స్థిరరాశి bool m_comb;  // Is from combinational logic
    // Every variable accessed by this logic, in order of first access, at most one record per
    // variable. Only populated for multi-threaded ordering.
    std::సదిశ<VarAccess> m_varAccesses;

బహిరంగ:
    // CONSTRUCTOR
    క్రమం_Logic_శీర్షం(క్రమం_లేఖాచిత్రం* graphp, AstScope* scopep, స్థిరరాశి క్రమం_Domain* domainp,
                     స్థిరరాశి క్రమం_Domain* hybridp, Ast_కణుపు* nodep) VL_MT_DISABLED
        : క్రమం_Either_శీర్షం{graphp, domainp},
          m_nodep{nodep},
          m_scopep{scopep},
          m_hybridp{hybridp},
          m_comb{!domainp && !hybridp} {
        UASSERT_OBJ(scopep, nodep, "Must not be null");
        UASSERT_OBJ(!(domainp && hybridp), nodep, "Cannot have bot domainp and hybridp set");
    }
    ~క్రమం_Logic_శీర్షం() override = అప్రమేయ;

    // METHODS
    bool domainMatters() override { తిరిగి true; }

    // ACCESSORS
    Ast_కణుపు* nodep() స్థిరరాశి VL_MT_STABLE { తిరిగి m_nodep; }
    AstScope* scopep() స్థిరరాశి VL_MT_STABLE { తిరిగి m_scopep; }
    స్థిరరాశి క్రమం_Domain* hybridp() స్థిరరాశి { తిరిగి m_hybridp; }
    bool comb() స్థిరరాశి { తిరిగి m_comb; }
    స్థిరరాశి std::సదిశ<VarAccess>& varAccesses() స్థిరరాశి { తిరిగి m_varAccesses; }
    శూన్య కూడు_Var_Access(AstVarScope* vscp, VAccess access) {
        m_varAccesses.నెట్టు_వెనుక({vscp, access});
    }

    // LCOV_EXCL_START // Debug code
    string name() స్థిరరాశి override VL_MT_STABLE {
        తిరిగి (cvt_గమ్యం_Hex(m_nodep) + "\\n " + cvt_గమ్యం_Str(nodep()->typeName()));
    }
    string dotShape() స్థిరరాశి override { తిరిగి VN_IS(m_nodep, చురుకైన) ? "doubleoctagon" : "rect"; }
    // LCOV_EXCL_STOP
};

class క్రమం_Var_శీర్షం VL_NOT_FINAL : బహిరంగ క్రమం_Either_శీర్షం {
    VL_RTTI_IMPL(క్రమం_Var_శీర్షం, క్రమం_Either_శీర్షం)
    AstVarScope* స్థిరరాశి m_vscp;

బహిరంగ:
    // CONSTRUCTOR
    క్రమం_Var_శీర్షం(క్రమం_లేఖాచిత్రం* graphp, AstVarScope* vscp) VL_MT_DISABLED
        : క్రమం_Either_శీర్షం{graphp, nullptr},
          m_vscp{vscp} {}
    ~క్రమం_Var_శీర్షం() override = అప్రమేయ;

    // ACCESSORS
    AstVarScope* vscp() స్థిరరాశి { తిరిగి m_vscp; }

    // LCOV_EXCL_START // Debug code
    string dotShape() స్థిరరాశి override final { తిరిగి "ellipse"; }
    మాయా string nameSuffix() స్థిరరాశి VL_MT_సురక్షిత = 0;
    string name() స్థిరరాశి override final VL_MT_STABLE {
        తిరిగి cvt_గమ్యం_Hex(m_vscp) + " " + nameSuffix() + "\\n " + m_vscp->name();
    }
    // LCOV_EXCL_STOP
};

class క్రమం_Var_Std_శీర్షం final : బహిరంగ క్రమం_Var_శీర్షం {
    VL_RTTI_IMPL(క్రమం_Var_Std_శీర్షం, క్రమం_Var_శీర్షం)
బహిరంగ:
    // CONSTRUCTOR
    క్రమం_Var_Std_శీర్షం(క్రమం_లేఖాచిత్రం* graphp, AstVarScope* vscp) VL_MT_DISABLED
        : క్రమం_Var_శీర్షం{graphp, vscp} {}
    ~క్రమం_Var_Std_శీర్షం() override = అప్రమేయ;

    // METHODS
    bool domainMatters() override { తిరిగి true; }

    // LCOV_EXCL_START // Debug code
    string nameSuffix() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి ""; }
    string dotColor() స్థిరరాశి override { తిరిగి "grey"; }
    // LCOV_EXCL_STOP
};

class క్రమం_Var_Pre_శీర్షం final : బహిరంగ క్రమం_Var_శీర్షం {
    VL_RTTI_IMPL(క్రమం_Var_Pre_శీర్షం, క్రమం_Var_శీర్షం)
బహిరంగ:
    // CONSTRUCTOR
    క్రమం_Var_Pre_శీర్షం(క్రమం_లేఖాచిత్రం* graphp, AstVarScope* vscp) VL_MT_DISABLED
        : క్రమం_Var_శీర్షం{graphp, vscp} {}
    ~క్రమం_Var_Pre_శీర్షం() override = అప్రమేయ;

    // METHODS
    bool domainMatters() override { తిరిగి false; }

    // LCOV_EXCL_START // Debug code
    string nameSuffix() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి "PRE"; }
    string dotColor() స్థిరరాశి override { తిరిగి "green"; }
    // LCOV_EXCL_STOP
};

class క్రమం_Var_Post_శీర్షం final : బహిరంగ క్రమం_Var_శీర్షం {
    VL_RTTI_IMPL(క్రమం_Var_Post_శీర్షం, క్రమం_Var_శీర్షం)
బహిరంగ:
    // CONSTRUCTOR
    క్రమం_Var_Post_శీర్షం(క్రమం_లేఖాచిత్రం* graphp, AstVarScope* vscp) VL_MT_DISABLED
        : క్రమం_Var_శీర్షం{graphp, vscp} {}
    ~క్రమం_Var_Post_శీర్షం() override = అప్రమేయ;

    // METHODS
    bool domainMatters() override { తిరిగి false; }

    // LCOV_EXCL_START // Debug code
    string nameSuffix() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి "POST"; }
    string dotColor() స్థిరరాశి override { తిరిగి "red"; }
    // LCOV_EXCL_STOP
};

class క్రమం_Var_Pord_శీర్షం final : బహిరంగ క్రమం_Var_శీర్షం {
    VL_RTTI_IMPL(క్రమం_Var_Pord_శీర్షం, క్రమం_Var_శీర్షం)
బహిరంగ:
    // CONSTRUCTOR
    క్రమం_Var_Pord_శీర్షం(క్రమం_లేఖాచిత్రం* graphp, AstVarScope* vscp) VL_MT_DISABLED
        : క్రమం_Var_శీర్షం{graphp, vscp} {}
    ~క్రమం_Var_Pord_శీర్షం() override = అప్రమేయ;

    // METHODS
    bool domainMatters() override { తిరిగి false; }

    // LCOV_EXCL_START // Debug code
    string nameSuffix() స్థిరరాశి override VL_MT_సురక్షిత { తిరిగి "PORD"; }
    string dotColor() స్థిరరాశి override { తిరిగి "blue"; }
    // LCOV_EXCL_STOP
};

//======================================================================
// Edge type

class క్రమం_అంచు final : బహిరంగ V3_లేఖాచిత్రం_అంచు {
    VL_RTTI_IMPL(క్రమం_అంచు, V3_లేఖాచిత్రం_అంచు)
    మిత్ర class క్రమం_లేఖాచిత్రం;  // Only the OrderGraph can create these
    // CONSTRUCTOR
    క్రమం_అంచు(క్రమం_లేఖాచిత్రం* graphp, క్రమం_Either_శీర్షం* fromp, క్రమం_Either_శీర్షం* శిఖరం, పూర్ణాంకం బరువు_2,
              bool cutable) VL_MT_DISABLED : V3_లేఖాచిత్రం_అంచు{graphp, fromp, శిఖరం, బరువు_2, cutable} {}
    ~క్రమం_అంచు() override = అప్రమేయ;

    // LCOV_EXCL_START // Debug code
    string dotColor() స్థిరరాశి override { తిరిగి cutable() ? "green" : "red"; }
    // LCOV_EXCL_STOP
};

//======================================================================
// Inline methods

శూన్య క్రమం_లేఖాచిత్రం::కూడు_Hard_అంచు(క్రమం_Logic_శీర్షం* fromp, క్రమం_Var_శీర్షం* శిఖరం,
                             పూర్ణాంకం బరువు_2) VL_MT_DISABLED {
    new క్రమం_అంచు{this, fromp, శిఖరం, బరువు_2, /* cutable: */ false};
}
శూన్య క్రమం_లేఖాచిత్రం::కూడు_Hard_అంచు(క్రమం_Var_శీర్షం* fromp, క్రమం_Logic_శీర్షం* శిఖరం,
                             పూర్ణాంకం బరువు_2) VL_MT_DISABLED {
    new క్రమం_అంచు{this, fromp, శిఖరం, బరువు_2, /* cutable: */ false};
}
శూన్య క్రమం_లేఖాచిత్రం::కూడు_Soft_అంచు(క్రమం_Logic_శీర్షం* fromp, క్రమం_Var_శీర్షం* శిఖరం,
                             పూర్ణాంకం బరువు_2) VL_MT_DISABLED {
    new క్రమం_అంచు{this, fromp, శిఖరం, బరువు_2, /* cutable: */ true};
}
శూన్య క్రమం_లేఖాచిత్రం::కూడు_Soft_అంచు(క్రమం_Var_శీర్షం* fromp, క్రమం_Logic_శీర్షం* శిఖరం,
                             పూర్ణాంకం బరువు_2) VL_MT_DISABLED {
    new క్రమం_అంచు{this, fromp, శిఖరం, బరువు_2, /* cutable: */ true};
}

#అయితే_ముగింపు  // Guard
