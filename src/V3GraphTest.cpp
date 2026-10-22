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

#घोषणा VL_MT_DISABLED_CODE_UNIT 1

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश "V3Global.h"
#समावेश "V3Graph.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
//######################################################################
// Test class

श्रेणी V3GraphTest VL_NOT_FINAL {
रक्षित:
    // MEMBERS
    V3Graph m_graph;

    // METHODS - for children
    भव व्योम runTest() = 0;  // Run the test
    भव string name() = 0;  // Name of the test

    // Utilities
    व्योम dumpSelf() {
        अगर (dumpGraphLevel() >= 9) m_graph.dumpDotFilePrefixed("v3graphtest_" + name());
    }

खुला:
    V3GraphTest() = शेष;
    भव ~V3GraphTest() = शेष;

    व्योम run() { runTest(); }
};

//######################################################################
//######################################################################
// Vertices and nodes

श्रेणी V3GraphTestVertex VL_NOT_FINAL : खुला V3GraphVertex {
    VL_RTTI_IMPL(V3GraphTestVertex, V3GraphVertex)
    स्थिर string m_name;

खुला:
    V3GraphTestVertex(V3Graph* graphp, स्थिर string& name)
        : V3GraphVertex{graphp}
        , m_name{name} {}
    ~V3GraphTestVertex() override = शेष;
    // ACCESSORS
    string name() स्थिर override VL_MT_STABLE { वापस m_name; }
};

श्रेणी V3GraphTestVarVertex final : खुला V3GraphTestVertex {
खुला:
    V3GraphTestVarVertex(V3Graph* graphp, स्थिर string& name)
        : V3GraphTestVertex{graphp, name} {}
    ~V3GraphTestVarVertex() override = शेष;
    // ACCESSORS
    string dotColor() स्थिर override { वापस "blue"; }
};

//######################################################################
//######################################################################
// Test vertices and nodes

श्रेणी V3GraphTestStrong final : खुला V3GraphTest {
खुला:
    string name() override { वापस "strong"; }
    व्योम runTest() override {
        V3Graph* gp = &m_graph;
        // Verify we break edges at a good point
        // A simple alg would make 3 breaks, below only requires b->i to break
        V3GraphTestVertex* i = नया V3GraphTestVarVertex{gp, "*INPUTS*"};
        V3GraphTestVertex* a = नया V3GraphTestVarVertex{gp, "a"};
        V3GraphTestVertex* b = नया V3GraphTestVarVertex{gp, "b"};
        V3GraphTestVertex* g1 = नया V3GraphTestVarVertex{gp, "g1"};
        V3GraphTestVertex* g2 = नया V3GraphTestVarVertex{gp, "g2"};
        V3GraphTestVertex* g3 = नया V3GraphTestVarVertex{gp, "g3"};
        V3GraphTestVertex* q = नया V3GraphTestVarVertex{gp, "q"};
        नया V3GraphEdge{gp, i, a, 2, true};
        नया V3GraphEdge{gp, a, b, 2, true};
        नया V3GraphEdge{gp, b, g1, 2, true};
        नया V3GraphEdge{gp, b, g2, 2, true};
        नया V3GraphEdge{gp, b, g3, 2, true};
        नया V3GraphEdge{gp, g1, a, 2, true};
        नया V3GraphEdge{gp, g3, g2, 2, true};
        नया V3GraphEdge{gp, g2, g3, 2, true};
        नया V3GraphEdge{gp, g1, q, 2, true};
        नया V3GraphEdge{gp, g2, q, 2, true};
        नया V3GraphEdge{gp, g3, q, 2, true};

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

श्रेणी V3GraphTestAcyc final : खुला V3GraphTest {
खुला:
    string name() override { वापस "acyc"; }
    व्योम runTest() override {
        V3Graph* gp = &m_graph;
        // Verify we break edges at a good point
        // A simple alg would make 3 breaks, below only requires b->i to break
        V3GraphTestVertex* i = नया V3GraphTestVarVertex{gp, "*INPUTS*"};
        V3GraphTestVertex* a = नया V3GraphTestVarVertex{gp, "a"};
        V3GraphTestVertex* b = नया V3GraphTestVarVertex{gp, "b"};
        V3GraphTestVertex* g1 = नया V3GraphTestVarVertex{gp, "g1"};
        V3GraphTestVertex* g2 = नया V3GraphTestVarVertex{gp, "g2"};
        V3GraphTestVertex* g3 = नया V3GraphTestVarVertex{gp, "g3"};
        नया V3GraphEdge{gp, i, a, 2, true};
        नया V3GraphEdge{gp, a, b, 2, true};
        नया V3GraphEdge{gp, b, g1, 2, true};
        नया V3GraphEdge{gp, b, g2, 2, true};
        नया V3GraphEdge{gp, b, g3, 2, true};
        नया V3GraphEdge{gp, g1, a, 2, true};
        नया V3GraphEdge{gp, g2, a, 2, true};
        नया V3GraphEdge{gp, g3, a, 2, true};

        gp->acyclic(&V3GraphEdge::followAlwaysTrue);
        gp->order();
        dumpSelf();
    }
};

श्रेणी V3GraphTestVars final : खुला V3GraphTest {
खुला:
    string name() override { वापस "vars"; }
    व्योम runTest() override {
        V3Graph* स्थिर gp = &m_graph;

        V3GraphTestVertex* स्थिर clk = नया V3GraphTestVarVertex{gp, "$clk"};

        V3GraphTestVertex* स्थिर a = नया V3GraphTestVarVertex{gp, "$a"};
        V3GraphTestVertex* स्थिर a_dly = नया V3GraphTestVarVertex{gp, "$a_dly"};
        V3GraphTestVertex* स्थिर a_dlyblk = नया V3GraphTestVarVertex{gp, "$a_dlyblk"};
        V3GraphTestVertex* स्थिर b = नया V3GraphTestVarVertex{gp, "$b"};
        V3GraphTestVertex* स्थिर b_dly = नया V3GraphTestVarVertex{gp, "$b_dly"};
        V3GraphTestVertex* स्थिर b_dlyblk = नया V3GraphTestVarVertex{gp, "$b_dlyblk"};
        V3GraphTestVertex* स्थिर c = नया V3GraphTestVarVertex{gp, "$c"};
        V3GraphTestVertex* स्थिर i = नया V3GraphTestVarVertex{gp, "$i"};

        V3GraphTestVertex* स्थिर ap = नया V3GraphTestVarVertex{gp, "$a_pre"};
        V3GraphTestVertex* स्थिर bp = नया V3GraphTestVarVertex{gp, "$b_pre"};
        V3GraphTestVertex* स्थिर cp = नया V3GraphTestVarVertex{gp, "$c_pre"};

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

        n = नया V3GraphTestVertex{gp, "*INPUTS*"};
        {
            नया V3GraphEdge{gp, n, clk, 2};
            नया V3GraphEdge{gp, n, i, 2};
        }

        V3GraphTestVertex* स्थिर posedge = n = नया V3GraphTestVertex{gp, "*posedge clk*"};
        { नया V3GraphEdge{gp, clk, n, 2}; }

        // AlwaysPre's     VarRefs on LHS:  generate special BLK
        //    normal:      VarRefs on LHS:  generate normal
        //    underSBlock: VarRefs on RHS:  consume 'pre' (required to save cutable tests)
        n = नया V3GraphTestVertex{gp, "a_dly<PRE=a"};
        {
            नया V3GraphEdge{gp, n, a_dlyblk, 2};  // Block ordering
            नया V3GraphEdge{gp, n, a_dly, 2};
            नया V3GraphEdge{gp, ap, n, 2, true};  // DESIRED delayed ordering (inp is required)
            नया V3GraphEdge{gp, posedge, n, 2};
        }
        n = नया V3GraphTestVertex{gp, "b_dly<PRE=b"};
        {
            नया V3GraphEdge{gp, n, b_dlyblk, 2};  // Block ordering
            नया V3GraphEdge{gp, n, b_dly, 2};
            नया V3GraphEdge{gp, bp, n, 2, true};  // DESIRED delayed ordering
            नया V3GraphEdge{gp, posedge, n, 2};
        }

        // AssignDly's     VarRefs on LHS:  consume special BLK
        //    normal:      VarRefs on LHS:  generate normal
        //    underSBlock: VarRefs on RHS:  generate 'pre' signals (cutable)
        //    SenItems:    consume CLOCK dependency
        n = नया V3GraphTestVertex{gp, "a_dly<=b|c"};
        {
            नया V3GraphEdge{gp, a_dlyblk, n, 2};  // Block ordering in
            नया V3GraphEdge{gp, n, a_dly, 2};
            // Note we don't include ap as we're generating a_dly
            नया V3GraphEdge{gp, n, bp, 2};  // DESIRED delayed usage
            नया V3GraphEdge{gp, n, cp, 2};  // DESIRED delayed usage
            नया V3GraphEdge{gp, posedge, n, 2};
        }
        n = नया V3GraphTestVertex{gp, "b_dly<=a"};
        {
            नया V3GraphEdge{gp, b_dlyblk, n, 2};  // Block ordering in
            नया V3GraphEdge{gp, n, b_dly, 2};
            नया V3GraphEdge{gp, n, ap, 2};  // DESIRED delayed usage
            नया V3GraphEdge{gp, posedge, n, 2};
        }

        // AlwaysPost's
        //    normal:      VarRefs on LHS:  generate normal
        //    underSBlock: VarRefs on RHS:  consume normal
        n = नया V3GraphTestVertex{gp, "a=POST=a_dly"};
        {
            नया V3GraphEdge{gp, n, a, 3};
            नया V3GraphEdge{gp, a_dly, n, 3};
            नया V3GraphEdge{gp, posedge, n, 2};
        }
        n = नया V3GraphTestVertex{gp, "b=POST=b_dly"};
        {
            नया V3GraphEdge{gp, n, b, 3};
            नया V3GraphEdge{gp, b_dly, n, 3};
            नया V3GraphEdge{gp, posedge, n, 2};
        }

        // COMBO
        // Inbound edges are always uncutable, because we must put combo logic after sequential
        // Outbound are cutable, as we may need to evaluate multiple times

        {
            n = नया V3GraphTestVertex{gp, "c=a|b|i"};
            नया V3GraphEdge{gp, n, c, 1, true};
            नया V3GraphEdge{gp, a, n, 1, false};
            नया V3GraphEdge{gp, b, n, 1, false};
            नया V3GraphEdge{gp, i, n, 1, false};
        }

        gp->acyclic(&V3GraphEdge::followAlwaysTrue);
        gp->order();

        // Test debug function
        gp->dumpEdges(std::अ_बाहर, *ap);

        dumpSelf();
    }
};

//======================================================================

श्रेणी V3GraphTestImport final : खुला V3GraphTest {

#अगर_घोषित GRAPH_IMPORT
    व्योम dotImport();
#अन्यथा
    व्योम dotImport() {}
#पूर्ण_अगर

खुला:
    string name() override { वापस "import"; }
    व्योम runTest() override {
        V3Graph* स्थिर gp = &m_graph;
        dotImport();
        dumpSelf();
        gp->acyclic(&V3GraphEdge::followAlwaysTrue);
        dumpSelf();
        gp->rank(&V3GraphEdge::followAlwaysTrue);
        dumpSelf();
    }
};

// clang-format off
#अगर_घोषित GRAPH_IMPORT
# include "graph_export.cpp"
#पूर्ण_अगर
// clang-format on

//======================================================================

व्योम V3Graph::selfTest() {
    // Execute all of the tests
    UINFO(2, __FUNCTION__ << ":");
    { V3GraphTestStrong{}.run(); }
    { V3GraphTestAcyc{}.run(); }
    { V3GraphTestVars{}.run(); }
    { V3GraphTestImport{}.run(); }
}
