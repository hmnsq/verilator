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

#నిర్వచించు VL_MT_DISABLED_సంకేతం_UNIT 1

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Global.h"
#అన్తర్భావ "V3Graph.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
//######################################################################
// Test class

తరగతి V3_లేఖాచిత్రం_Test VL_NOT_FINAL {
రక్షిత:
    // MEMBERS
    V3_లేఖాచిత్రం m_లేఖాచిత్రం;

    // METHODS - for children
    మాయా శూన్య నిరంతరం_Test() = 0;  // Run the test
    మాయా string name() = 0;  // Name of the test

    // Utilities
    శూన్య dumpSelf() {
        ఒకవేళ (dump_లేఖాచిత్రం_Level() >= 9) m_లేఖాచిత్రం.dumpDotFilePrefixed("v3graphtest_" + name());
    }

బహిరంగ:
    V3_లేఖాచిత్రం_Test() = అప్రమేయ;
    మాయా ~V3_లేఖాచిత్రం_Test() = అప్రమేయ;

    శూన్య నిరంతరం() { నిరంతరం_Test(); }
};

//######################################################################
//######################################################################
// Vertices and nodes

తరగతి V3_లేఖాచిత్రం_Test_శీర్షం VL_NOT_FINAL : బహిరంగ V3_లేఖాచిత్రం_శీర్షం {
    VL_RTTI_IMPL(V3_లేఖాచిత్రం_Test_శీర్షం, V3_లేఖాచిత్రం_శీర్షం)
    స్థిరరాశి string m_name;

బహిరంగ:
    V3_లేఖాచిత్రం_Test_శీర్షం(V3_లేఖాచిత్రం* graphp, స్థిరరాశి string& name)
        : V3_లేఖాచిత్రం_శీర్షం{graphp}
        , m_name{name} {}
    ~V3_లేఖాచిత్రం_Test_శీర్షం() override = అప్రమేయ;
    // ACCESSORS
    string name() స్థిరరాశి override VL_MT_STABLE { తిరిగి m_name; }
};

తరగతి V3_లేఖాచిత్రం_Test_Var_శీర్షం final : బహిరంగ V3_లేఖాచిత్రం_Test_శీర్షం {
బహిరంగ:
    V3_లేఖాచిత్రం_Test_Var_శీర్షం(V3_లేఖాచిత్రం* graphp, స్థిరరాశి string& name)
        : V3_లేఖాచిత్రం_Test_శీర్షం{graphp, name} {}
    ~V3_లేఖాచిత్రం_Test_Var_శీర్షం() override = అప్రమేయ;
    // ACCESSORS
    string dotColor() స్థిరరాశి override { తిరిగి "blue"; }
};

//######################################################################
//######################################################################
// Test vertices and nodes

తరగతి V3_లేఖాచిత్రం_Test_Strong final : బహిరంగ V3_లేఖాచిత్రం_Test {
బహిరంగ:
    string name() override { తిరిగి "strong"; }
    శూన్య నిరంతరం_Test() override {
        V3_లేఖాచిత్రం* gp = &m_లేఖాచిత్రం;
        // Verify we break edges at a good point
        // A simple alg would make 3 breaks, below only requires b->i to break
        V3_లేఖాచిత్రం_Test_శీర్షం* i = కొత్త V3_లేఖాచిత్రం_Test_Var_శీర్షం{gp, "*INPUTS*"};
        V3_లేఖాచిత్రం_Test_శీర్షం* a = కొత్త V3_లేఖాచిత్రం_Test_Var_శీర్షం{gp, "a"};
        V3_లేఖాచిత్రం_Test_శీర్షం* b = కొత్త V3_లేఖాచిత్రం_Test_Var_శీర్షం{gp, "b"};
        V3_లేఖాచిత్రం_Test_శీర్షం* g1 = కొత్త V3_లేఖాచిత్రం_Test_Var_శీర్షం{gp, "g1"};
        V3_లేఖాచిత్రం_Test_శీర్షం* g2 = కొత్త V3_లేఖాచిత్రం_Test_Var_శీర్షం{gp, "g2"};
        V3_లేఖాచిత్రం_Test_శీర్షం* g3 = కొత్త V3_లేఖాచిత్రం_Test_Var_శీర్షం{gp, "g3"};
        V3_లేఖాచిత్రం_Test_శీర్షం* q = కొత్త V3_లేఖాచిత్రం_Test_Var_శీర్షం{gp, "q"};
        కొత్త V3_లేఖాచిత్రం_అంచు{gp, i, a, 2, నిజం};
        కొత్త V3_లేఖాచిత్రం_అంచు{gp, a, b, 2, నిజం};
        కొత్త V3_లేఖాచిత్రం_అంచు{gp, b, g1, 2, నిజం};
        కొత్త V3_లేఖాచిత్రం_అంచు{gp, b, g2, 2, నిజం};
        కొత్త V3_లేఖాచిత్రం_అంచు{gp, b, g3, 2, నిజం};
        కొత్త V3_లేఖాచిత్రం_అంచు{gp, g1, a, 2, నిజం};
        కొత్త V3_లేఖాచిత్రం_అంచు{gp, g3, g2, 2, నిజం};
        కొత్త V3_లేఖాచిత్రం_అంచు{gp, g2, g3, 2, నిజం};
        కొత్త V3_లేఖాచిత్రం_అంచు{gp, g1, q, 2, నిజం};
        కొత్త V3_లేఖాచిత్రం_అంచు{gp, g2, q, 2, నిజం};
        కొత్త V3_లేఖాచిత్రం_అంచు{gp, g3, q, 2, నిజం};

        gp->stronglyConnected(&V3_లేఖాచిత్రం_అంచు::followAlwaysTrue);
        dumpSelf();

        UASSERT(i->color() != a->color() && a->color() != g2->color() && g2->color() != q->color(),
                "SelfTest: Separate colors not assigned");
        UASSERT(a->color() == b->color() && a->color() == g1->color(),
                "SelfTest: Strongly connected nodes not colored together");
        UASSERT(g2->color() == g3->color(),
                "SelfTest: Strongly connected nodes not colored together");
    }
};

తరగతి V3_లేఖాచిత్రం_Test_Acyc final : బహిరంగ V3_లేఖాచిత్రం_Test {
బహిరంగ:
    string name() override { తిరిగి "acyc"; }
    శూన్య నిరంతరం_Test() override {
        V3_లేఖాచిత్రం* gp = &m_లేఖాచిత్రం;
        // Verify we break edges at a good point
        // A simple alg would make 3 breaks, below only requires b->i to break
        V3_లేఖాచిత్రం_Test_శీర్షం* i = కొత్త V3_లేఖాచిత్రం_Test_Var_శీర్షం{gp, "*INPUTS*"};
        V3_లేఖాచిత్రం_Test_శీర్షం* a = కొత్త V3_లేఖాచిత్రం_Test_Var_శీర్షం{gp, "a"};
        V3_లేఖాచిత్రం_Test_శీర్షం* b = కొత్త V3_లేఖాచిత్రం_Test_Var_శీర్షం{gp, "b"};
        V3_లేఖాచిత్రం_Test_శీర్షం* g1 = కొత్త V3_లేఖాచిత్రం_Test_Var_శీర్షం{gp, "g1"};
        V3_లేఖాచిత్రం_Test_శీర్షం* g2 = కొత్త V3_లేఖాచిత్రం_Test_Var_శీర్షం{gp, "g2"};
        V3_లేఖాచిత్రం_Test_శీర్షం* g3 = కొత్త V3_లేఖాచిత్రం_Test_Var_శీర్షం{gp, "g3"};
        కొత్త V3_లేఖాచిత్రం_అంచు{gp, i, a, 2, నిజం};
        కొత్త V3_లేఖాచిత్రం_అంచు{gp, a, b, 2, నిజం};
        కొత్త V3_లేఖాచిత్రం_అంచు{gp, b, g1, 2, నిజం};
        కొత్త V3_లేఖాచిత్రం_అంచు{gp, b, g2, 2, నిజం};
        కొత్త V3_లేఖాచిత్రం_అంచు{gp, b, g3, 2, నిజం};
        కొత్త V3_లేఖాచిత్రం_అంచు{gp, g1, a, 2, నిజం};
        కొత్త V3_లేఖాచిత్రం_అంచు{gp, g2, a, 2, నిజం};
        కొత్త V3_లేఖాచిత్రం_అంచు{gp, g3, a, 2, నిజం};

        gp->acyclic(&V3_లేఖాచిత్రం_అంచు::followAlwaysTrue);
        gp->క్రమం();
        dumpSelf();
    }
};

తరగతి V3_లేఖాచిత్రం_Test_Vars final : బహిరంగ V3_లేఖాచిత్రం_Test {
బహిరంగ:
    string name() override { తిరిగి "vars"; }
    శూన్య నిరంతరం_Test() override {
        V3_లేఖాచిత్రం* స్థిరరాశి gp = &m_లేఖాచిత్రం;

        V3_లేఖాచిత్రం_Test_శీర్షం* స్థిరరాశి clk = కొత్త V3_లేఖాచిత్రం_Test_Var_శీర్షం{gp, "$clk"};

        V3_లేఖాచిత్రం_Test_శీర్షం* స్థిరరాశి a = కొత్త V3_లేఖాచిత్రం_Test_Var_శీర్షం{gp, "$a"};
        V3_లేఖాచిత్రం_Test_శీర్షం* స్థిరరాశి a_dly = కొత్త V3_లేఖాచిత్రం_Test_Var_శీర్షం{gp, "$a_dly"};
        V3_లేఖాచిత్రం_Test_శీర్షం* స్థిరరాశి a_dlyblk = కొత్త V3_లేఖాచిత్రం_Test_Var_శీర్షం{gp, "$a_dlyblk"};
        V3_లేఖాచిత్రం_Test_శీర్షం* స్థిరరాశి b = కొత్త V3_లేఖాచిత్రం_Test_Var_శీర్షం{gp, "$b"};
        V3_లేఖాచిత్రం_Test_శీర్షం* స్థిరరాశి b_dly = కొత్త V3_లేఖాచిత్రం_Test_Var_శీర్షం{gp, "$b_dly"};
        V3_లేఖాచిత్రం_Test_శీర్షం* స్థిరరాశి b_dlyblk = కొత్త V3_లేఖాచిత్రం_Test_Var_శీర్షం{gp, "$b_dlyblk"};
        V3_లేఖాచిత్రం_Test_శీర్షం* స్థిరరాశి c = కొత్త V3_లేఖాచిత్రం_Test_Var_శీర్షం{gp, "$c"};
        V3_లేఖాచిత్రం_Test_శీర్షం* స్థిరరాశి i = కొత్త V3_లేఖాచిత్రం_Test_Var_శీర్షం{gp, "$i"};

        V3_లేఖాచిత్రం_Test_శీర్షం* స్థిరరాశి ap = కొత్త V3_లేఖాచిత్రం_Test_Var_శీర్షం{gp, "$a_pre"};
        V3_లేఖాచిత్రం_Test_శీర్షం* స్థిరరాశి bp = కొత్త V3_లేఖాచిత్రం_Test_Var_శీర్షం{gp, "$b_pre"};
        V3_లేఖాచిత్రం_Test_శీర్షం* స్థిరరాశి cp = కొత్త V3_లేఖాచిత్రం_Test_Var_శీర్షం{gp, "$c_pre"};

        V3_లేఖాచిత్రం_Test_శీర్షం* n;

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

        n = కొత్త V3_లేఖాచిత్రం_Test_శీర్షం{gp, "*INPUTS*"};
        {
            కొత్త V3_లేఖాచిత్రం_అంచు{gp, n, clk, 2};
            కొత్త V3_లేఖాచిత్రం_అంచు{gp, n, i, 2};
        }

        V3_లేఖాచిత్రం_Test_శీర్షం* స్థిరరాశి posedge = n = కొత్త V3_లేఖాచిత్రం_Test_శీర్షం{gp, "*posedge clk*"};
        { కొత్త V3_లేఖాచిత్రం_అంచు{gp, clk, n, 2}; }

        // AlwaysPre's     VarRefs on LHS:  generate special BLK
        //    normal:      VarRefs on LHS:  generate normal
        //    underSBlock: VarRefs on RHS:  consume 'pre' (required to save cutable tests)
        n = కొత్త V3_లేఖాచిత్రం_Test_శీర్షం{gp, "a_dly<PRE=a"};
        {
            కొత్త V3_లేఖాచిత్రం_అంచు{gp, n, a_dlyblk, 2};  // Block ordering
            కొత్త V3_లేఖాచిత్రం_అంచు{gp, n, a_dly, 2};
            కొత్త V3_లేఖాచిత్రం_అంచు{gp, ap, n, 2, నిజం};  // DESIRED delayed ordering (inp is required)
            కొత్త V3_లేఖాచిత్రం_అంచు{gp, posedge, n, 2};
        }
        n = కొత్త V3_లేఖాచిత్రం_Test_శీర్షం{gp, "b_dly<PRE=b"};
        {
            కొత్త V3_లేఖాచిత్రం_అంచు{gp, n, b_dlyblk, 2};  // Block ordering
            కొత్త V3_లేఖాచిత్రం_అంచు{gp, n, b_dly, 2};
            కొత్త V3_లేఖాచిత్రం_అంచు{gp, bp, n, 2, నిజం};  // DESIRED delayed ordering
            కొత్త V3_లేఖాచిత్రం_అంచు{gp, posedge, n, 2};
        }

        // AssignDly's     VarRefs on LHS:  consume special BLK
        //    normal:      VarRefs on LHS:  generate normal
        //    underSBlock: VarRefs on RHS:  generate 'pre' signals (cutable)
        //    SenItems:    consume CLOCK dependency
        n = కొత్త V3_లేఖాచిత్రం_Test_శీర్షం{gp, "a_dly<=b|c"};
        {
            కొత్త V3_లేఖాచిత్రం_అంచు{gp, a_dlyblk, n, 2};  // Block ordering in
            కొత్త V3_లేఖాచిత్రం_అంచు{gp, n, a_dly, 2};
            // Note we don't include ap as we're generating a_dly
            కొత్త V3_లేఖాచిత్రం_అంచు{gp, n, bp, 2};  // DESIRED delayed usage
            కొత్త V3_లేఖాచిత్రం_అంచు{gp, n, cp, 2};  // DESIRED delayed usage
            కొత్త V3_లేఖాచిత్రం_అంచు{gp, posedge, n, 2};
        }
        n = కొత్త V3_లేఖాచిత్రం_Test_శీర్షం{gp, "b_dly<=a"};
        {
            కొత్త V3_లేఖాచిత్రం_అంచు{gp, b_dlyblk, n, 2};  // Block ordering in
            కొత్త V3_లేఖాచిత్రం_అంచు{gp, n, b_dly, 2};
            కొత్త V3_లేఖాచిత్రం_అంచు{gp, n, ap, 2};  // DESIRED delayed usage
            కొత్త V3_లేఖాచిత్రం_అంచు{gp, posedge, n, 2};
        }

        // AlwaysPost's
        //    normal:      VarRefs on LHS:  generate normal
        //    underSBlock: VarRefs on RHS:  consume normal
        n = కొత్త V3_లేఖాచిత్రం_Test_శీర్షం{gp, "a=POST=a_dly"};
        {
            కొత్త V3_లేఖాచిత్రం_అంచు{gp, n, a, 3};
            కొత్త V3_లేఖాచిత్రం_అంచు{gp, a_dly, n, 3};
            కొత్త V3_లేఖాచిత్రం_అంచు{gp, posedge, n, 2};
        }
        n = కొత్త V3_లేఖాచిత్రం_Test_శీర్షం{gp, "b=POST=b_dly"};
        {
            కొత్త V3_లేఖాచిత్రం_అంచు{gp, n, b, 3};
            కొత్త V3_లేఖాచిత్రం_అంచు{gp, b_dly, n, 3};
            కొత్త V3_లేఖాచిత్రం_అంచు{gp, posedge, n, 2};
        }

        // COMBO
        // Inbound edges are always uncutable, because we must put combo logic after sequential
        // Outbound are cutable, as we may need to evaluate multiple times

        {
            n = కొత్త V3_లేఖాచిత్రం_Test_శీర్షం{gp, "c=a|b|i"};
            కొత్త V3_లేఖాచిత్రం_అంచు{gp, n, c, 1, నిజం};
            కొత్త V3_లేఖాచిత్రం_అంచు{gp, a, n, 1, అబద్ధం};
            కొత్త V3_లేఖాచిత్రం_అంచు{gp, b, n, 1, అబద్ధం};
            కొత్త V3_లేఖాచిత్రం_అంచు{gp, i, n, 1, అబద్ధం};
        }

        gp->acyclic(&V3_లేఖాచిత్రం_అంచు::followAlwaysTrue);
        gp->క్రమం();

        // Test debug function
        gp->dump_అంచులు(std::బయటకు, *ap);

        dumpSelf();
    }
};

//======================================================================

తరగతి V3_లేఖాచిత్రం_Test_Import final : బహిరంగ V3_లేఖాచిత్రం_Test {

#నిర్వచితమైతే లేఖాచిత్రం_IMPORT
    శూన్య dotImport();
#లేకపోతే
    శూన్య dotImport() {}
#అయితే_ముగింపు

బహిరంగ:
    string name() override { తిరిగి "import"; }
    శూన్య నిరంతరం_Test() override {
        V3_లేఖాచిత్రం* స్థిరరాశి gp = &m_లేఖాచిత్రం;
        dotImport();
        dumpSelf();
        gp->acyclic(&V3_లేఖాచిత్రం_అంచు::followAlwaysTrue);
        dumpSelf();
        gp->rank(&V3_లేఖాచిత్రం_అంచు::followAlwaysTrue);
        dumpSelf();
    }
};

// clang-format off
#నిర్వచితమైతే లేఖాచిత్రం_IMPORT
# include "graph_export.cpp"
#అయితే_ముగింపు
// clang-format on

//======================================================================

శూన్య V3_లేఖాచిత్రం::selfTest() {
    // Execute all of the tests
    UINFO(2, __FUNCTION__ << ":");
    { V3_లేఖాచిత్రం_Test_Strong{}.నిరంతరం(); }
    { V3_లేఖాచిత్రం_Test_Acyc{}.నిరంతరం(); }
    { V3_లేఖాచిత్రం_Test_Vars{}.నిరంతరం(); }
    { V3_లేఖాచిత్రం_Test_Import{}.నిరంతరం(); }
}
