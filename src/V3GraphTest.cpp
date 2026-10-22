// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Graph tests
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

#define VL_MT_DISABLED_CODE_UNIT 1

#شامل "config_build.h"
#شامل "verilatedos.h"

#شامل "V3Global.h"
#شامل "V3Graph.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
//######################################################################
// Test class

رده V3GraphTest VL_NOT_FINAL {
محافظتشده:
    // MEMBERS
    V3Graph m_graph;

    // METHODS - for children
    مجازی تهی runTest() = 0;  // Run the test
    مجازی string name() = 0;  // Name of the test

    // Utilities
    تهی dumpSelf() {
        اگر (dumpGraphLevel() >= 9) m_graph.dumpDotFilePrefixed("v3graphtest_" + name());
    }

عمومی:
    V3GraphTest() = پیشفرض;
    مجازی ~V3GraphTest() = پیشفرض;

    تهی run() { runTest(); }
};

//######################################################################
//######################################################################
// Vertices and nodes

رده V3GraphTestVertex VL_NOT_FINAL : عمومی V3GraphVertex {
    VL_RTTI_IMPL(V3GraphTestVertex, V3GraphVertex)
    ثابت string m_name;

عمومی:
    V3GraphTestVertex(V3Graph* graphp, ثابت string& name)
        : V3GraphVertex{graphp}
        , m_name{name} {}
    ~V3GraphTestVertex() override = پیشفرض;
    // ACCESSORS
    string name() ثابت override VL_MT_STABLE { بازگشت m_name; }
};

رده V3GraphTestVarVertex final : عمومی V3GraphTestVertex {
عمومی:
    V3GraphTestVarVertex(V3Graph* graphp, ثابت string& name)
        : V3GraphTestVertex{graphp, name} {}
    ~V3GraphTestVarVertex() override = پیشفرض;
    // ACCESSORS
    string dotColor() ثابت override { بازگشت "blue"; }
};

//######################################################################
//######################################################################
// Test vertices and nodes

رده V3GraphTestStrong final : عمومی V3GraphTest {
عمومی:
    string name() override { بازگشت "strong"; }
    تهی runTest() override {
        V3Graph* gp = &m_graph;
        // Verify we break edges at a good point
        // A simple alg would make 3 breaks, below only requires b->i to break
        V3GraphTestVertex* i = جدید V3GraphTestVarVertex{gp, "*INPUTS*"};
        V3GraphTestVertex* a = جدید V3GraphTestVarVertex{gp, "a"};
        V3GraphTestVertex* b = جدید V3GraphTestVarVertex{gp, "b"};
        V3GraphTestVertex* g1 = جدید V3GraphTestVarVertex{gp, "g1"};
        V3GraphTestVertex* g2 = جدید V3GraphTestVarVertex{gp, "g2"};
        V3GraphTestVertex* g3 = جدید V3GraphTestVarVertex{gp, "g3"};
        V3GraphTestVertex* q = جدید V3GraphTestVarVertex{gp, "q"};
        جدید V3GraphEdge{gp, i, a, 2, درست};
        جدید V3GraphEdge{gp, a, b, 2, درست};
        جدید V3GraphEdge{gp, b, g1, 2, درست};
        جدید V3GraphEdge{gp, b, g2, 2, درست};
        جدید V3GraphEdge{gp, b, g3, 2, درست};
        جدید V3GraphEdge{gp, g1, a, 2, درست};
        جدید V3GraphEdge{gp, g3, g2, 2, درست};
        جدید V3GraphEdge{gp, g2, g3, 2, درست};
        جدید V3GraphEdge{gp, g1, q, 2, درست};
        جدید V3GraphEdge{gp, g2, q, 2, درست};
        جدید V3GraphEdge{gp, g3, q, 2, درست};

        gp->stronglyConnected(&V3GraphEdge::followAlwaysTrue);
        dumpSelf();

        UASSERT(i->color() != a->color() && a->color() != g2->color() && g2->color() != q->color(),
                "SelfTest: Separate colors not assigned");
        UASSERT(a->color() == b->color() && a->color() == g1->color(),
                "SelfTest: Strongly connected nodes not colored together");
        UASSERT(g2->color() == g3->color(),
                "SelfTest: Strongly connected nodes not colored together");
    }
};

رده V3GraphTestAcyc final : عمومی V3GraphTest {
عمومی:
    string name() override { بازگشت "acyc"; }
    تهی runTest() override {
        V3Graph* gp = &m_graph;
        // Verify we break edges at a good point
        // A simple alg would make 3 breaks, below only requires b->i to break
        V3GraphTestVertex* i = جدید V3GraphTestVarVertex{gp, "*INPUTS*"};
        V3GraphTestVertex* a = جدید V3GraphTestVarVertex{gp, "a"};
        V3GraphTestVertex* b = جدید V3GraphTestVarVertex{gp, "b"};
        V3GraphTestVertex* g1 = جدید V3GraphTestVarVertex{gp, "g1"};
        V3GraphTestVertex* g2 = جدید V3GraphTestVarVertex{gp, "g2"};
        V3GraphTestVertex* g3 = جدید V3GraphTestVarVertex{gp, "g3"};
        جدید V3GraphEdge{gp, i, a, 2, درست};
        جدید V3GraphEdge{gp, a, b, 2, درست};
        جدید V3GraphEdge{gp, b, g1, 2, درست};
        جدید V3GraphEdge{gp, b, g2, 2, درست};
        جدید V3GraphEdge{gp, b, g3, 2, درست};
        جدید V3GraphEdge{gp, g1, a, 2, درست};
        جدید V3GraphEdge{gp, g2, a, 2, درست};
        جدید V3GraphEdge{gp, g3, a, 2, درست};

        gp->acyclic(&V3GraphEdge::followAlwaysTrue);
        gp->order();
        dumpSelf();
    }
};

رده V3GraphTestVars final : عمومی V3GraphTest {
عمومی:
    string name() override { بازگشت "vars"; }
    تهی runTest() override {
        V3Graph* ثابت gp = &m_graph;

        V3GraphTestVertex* ثابت clk = جدید V3GraphTestVarVertex{gp, "$clk"};

        V3GraphTestVertex* ثابت a = جدید V3GraphTestVarVertex{gp, "$a"};
        V3GraphTestVertex* ثابت a_dly = جدید V3GraphTestVarVertex{gp, "$a_dly"};
        V3GraphTestVertex* ثابت a_dlyblk = جدید V3GraphTestVarVertex{gp, "$a_dlyblk"};
        V3GraphTestVertex* ثابت b = جدید V3GraphTestVarVertex{gp, "$b"};
        V3GraphTestVertex* ثابت b_dly = جدید V3GraphTestVarVertex{gp, "$b_dly"};
        V3GraphTestVertex* ثابت b_dlyblk = جدید V3GraphTestVarVertex{gp, "$b_dlyblk"};
        V3GraphTestVertex* ثابت c = جدید V3GraphTestVarVertex{gp, "$c"};
        V3GraphTestVertex* ثابت i = جدید V3GraphTestVarVertex{gp, "$i"};

        V3GraphTestVertex* ثابت ap = جدید V3GraphTestVarVertex{gp, "$a_pre"};
        V3GraphTestVertex* ثابت bp = جدید V3GraphTestVarVertex{gp, "$b_pre"};
        V3GraphTestVertex* ثابت cp = جدید V3GraphTestVarVertex{gp, "$c_pre"};

        V3GraphTestVertex* n;

        // Logical order between clk, and posedge blocks
        //   implemented by special CLK prod/cons?
        // Required order between first x_DLY<=x_pre and final x<=x_DLY
        //   implemented by producer/consumer on a_dly signals
        // Required order between first x_DLY<=x_pre and x_DLY<=setters
        //   implemented by fake dependency on _dlyblk
        // Required order between x_DLY<=setters and final x<=x_DLY
        //   implemented by producer/consumer on a_dly signals
        // Desired order between different _DLY blocks so we can elim temporaries
        //   implemented by cutable "pre" signal dependencies

        n = جدید V3GraphTestVertex{gp, "*INPUTS*"};
        {
            جدید V3GraphEdge{gp, n, clk, 2};
            جدید V3GraphEdge{gp, n, i, 2};
        }

        V3GraphTestVertex* ثابت posedge = n = جدید V3GraphTestVertex{gp, "*posedge clk*"};
        { جدید V3GraphEdge{gp, clk, n, 2}; }

        // AlwaysPre's     VarRefs on LHS:  generate special BLK
        //    normal:      VarRefs on LHS:  generate normal
        //    underSBlock: VarRefs on RHS:  consume 'pre' (required to save cutable tests)
        n = جدید V3GraphTestVertex{gp, "a_dly<PRE=a"};
        {
            جدید V3GraphEdge{gp, n, a_dlyblk, 2};  // Block ordering
            جدید V3GraphEdge{gp, n, a_dly, 2};
            جدید V3GraphEdge{gp, ap, n, 2, درست};  // DESIRED delayed ordering (inp is required)
            جدید V3GraphEdge{gp, posedge, n, 2};
        }
        n = جدید V3GraphTestVertex{gp, "b_dly<PRE=b"};
        {
            جدید V3GraphEdge{gp, n, b_dlyblk, 2};  // Block ordering
            جدید V3GraphEdge{gp, n, b_dly, 2};
            جدید V3GraphEdge{gp, bp, n, 2, درست};  // DESIRED delayed ordering
            جدید V3GraphEdge{gp, posedge, n, 2};
        }

        // AssignDly's     VarRefs on LHS:  consume special BLK
        //    normal:      VarRefs on LHS:  generate normal
        //    underSBlock: VarRefs on RHS:  generate 'pre' signals (cutable)
        //    SenItems:    consume CLOCK dependency
        n = جدید V3GraphTestVertex{gp, "a_dly<=b|c"};
        {
            جدید V3GraphEdge{gp, a_dlyblk, n, 2};  // Block ordering in
            جدید V3GraphEdge{gp, n, a_dly, 2};
            // Note we don't include ap as we're generating a_dly
            جدید V3GraphEdge{gp, n, bp, 2};  // DESIRED delayed usage
            جدید V3GraphEdge{gp, n, cp, 2};  // DESIRED delayed usage
            جدید V3GraphEdge{gp, posedge, n, 2};
        }
        n = جدید V3GraphTestVertex{gp, "b_dly<=a"};
        {
            جدید V3GraphEdge{gp, b_dlyblk, n, 2};  // Block ordering in
            جدید V3GraphEdge{gp, n, b_dly, 2};
            جدید V3GraphEdge{gp, n, ap, 2};  // DESIRED delayed usage
            جدید V3GraphEdge{gp, posedge, n, 2};
        }

        // AlwaysPost's
        //    normal:      VarRefs on LHS:  generate normal
        //    underSBlock: VarRefs on RHS:  consume normal
        n = جدید V3GraphTestVertex{gp, "a=POST=a_dly"};
        {
            جدید V3GraphEdge{gp, n, a, 3};
            جدید V3GraphEdge{gp, a_dly, n, 3};
            جدید V3GraphEdge{gp, posedge, n, 2};
        }
        n = جدید V3GraphTestVertex{gp, "b=POST=b_dly"};
        {
            جدید V3GraphEdge{gp, n, b, 3};
            جدید V3GraphEdge{gp, b_dly, n, 3};
            جدید V3GraphEdge{gp, posedge, n, 2};
        }

        // COMBO
        // Inbound edges are always uncutable, because we must put combo logic after sequential
        // Outbound are cutable, as we may need to evaluate multiple times

        {
            n = جدید V3GraphTestVertex{gp, "c=a|b|i"};
            جدید V3GraphEdge{gp, n, c, 1, درست};
            جدید V3GraphEdge{gp, a, n, 1, نادرست};
            جدید V3GraphEdge{gp, b, n, 1, نادرست};
            جدید V3GraphEdge{gp, i, n, 1, نادرست};
        }

        gp->acyclic(&V3GraphEdge::followAlwaysTrue);
        gp->order();

        // Test debug function
        gp->dumpEdges(std::cout, *ap);

        dumpSelf();
    }
};

//======================================================================

رده V3GraphTestImport final : عمومی V3GraphTest {

#ifdef GRAPH_IMPORT
    تهی dotImport();
#وگرنه
    تهی dotImport() {}
#endif

عمومی:
    string name() override { بازگشت "import"; }
    تهی runTest() override {
        V3Graph* ثابت gp = &m_graph;
        dotImport();
        dumpSelf();
        gp->acyclic(&V3GraphEdge::followAlwaysTrue);
        dumpSelf();
        gp->rank(&V3GraphEdge::followAlwaysTrue);
        dumpSelf();
    }
};

// clang-format off
#ifdef GRAPH_IMPORT
# include "graph_export.cpp"
#endif
// clang-format on

//======================================================================

تهی V3Graph::selfTest() {
    // Execute all of the tests
    UINFO(2, __FUNCTION__ << ":");
    { V3GraphTestStrong{}.run(); }
    { V3GraphTestAcyc{}.run(); }
    { V3GraphTestVars{}.run(); }
    { V3GraphTestImport{}.run(); }
}
