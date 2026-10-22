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

#ವ್ಯಾಖ್ಯಾನಿಸು VL_MT_DISABLED_ಸಂಕೇತ_UNIT 1

#ಅನ್ತರ್ಭಾವ "config_build.h"
#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Global.h"
#ಅನ್ತರ್ಭಾವ "V3Graph.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
//######################################################################
// Test class

ವರ್ಗ V3_ನಕ್ಷೆ_Test VL_NOT_FINAL {
ಸಂರಕ್ಷಿತ:
    // MEMBERS
    V3_ನಕ್ಷೆ m_ನಕ್ಷೆ;

    // METHODS - for children
    ವಾಸ್ತವಿಕ ಶೂನ್ಯ ಸತತ_Test() = 0;  // Run the test
    ವಾಸ್ತವಿಕ string name() = 0;  // Name of the test

    // Utilities
    ಶೂನ್ಯ dumpSelf() {
        ಆದರೆ (dump_ನಕ್ಷೆ_Level() >= 9) m_ನಕ್ಷೆ.dumpDotFilePrefixed("v3graphtest_" + name());
    }

ಸಾರ್ವಜನಿಕ:
    V3_ನಕ್ಷೆ_Test() = ಪೂರ್ವನಿಯೋಜಿತ;
    ವಾಸ್ತವಿಕ ~V3_ನಕ್ಷೆ_Test() = ಪೂರ್ವನಿಯೋಜಿತ;

    ಶೂನ್ಯ ಸತತ() { ಸತತ_Test(); }
};

//######################################################################
//######################################################################
// Vertices and nodes

ವರ್ಗ V3_ನಕ್ಷೆ_Test_ಶೃಂಗ VL_NOT_FINAL : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ_ಶೃಂಗ {
    VL_RTTI_IMPL(V3_ನಕ್ಷೆ_Test_ಶೃಂಗ, V3_ನಕ್ಷೆ_ಶೃಂಗ)
    ಸ್ಥಿರಾಂಕ string m_name;

ಸಾರ್ವಜನಿಕ:
    V3_ನಕ್ಷೆ_Test_ಶೃಂಗ(V3_ನಕ್ಷೆ* graphp, ಸ್ಥಿರಾಂಕ string& name)
        : V3_ನಕ್ಷೆ_ಶೃಂಗ{graphp}
        , m_name{name} {}
    ~V3_ನಕ್ಷೆ_Test_ಶೃಂಗ() override = ಪೂರ್ವನಿಯೋಜಿತ;
    // ACCESSORS
    string name() ಸ್ಥಿರಾಂಕ override VL_MT_STABLE { ಹಿಂತಿರುಗಿ m_name; }
};

ವರ್ಗ V3_ನಕ್ಷೆ_Test_Var_ಶೃಂಗ final : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ_Test_ಶೃಂಗ {
ಸಾರ್ವಜನಿಕ:
    V3_ನಕ್ಷೆ_Test_Var_ಶೃಂಗ(V3_ನಕ್ಷೆ* graphp, ಸ್ಥಿರಾಂಕ string& name)
        : V3_ನಕ್ಷೆ_Test_ಶೃಂಗ{graphp, name} {}
    ~V3_ನಕ್ಷೆ_Test_Var_ಶೃಂಗ() override = ಪೂರ್ವನಿಯೋಜಿತ;
    // ACCESSORS
    string dotColor() ಸ್ಥಿರಾಂಕ override { ಹಿಂತಿರುಗಿ "blue"; }
};

//######################################################################
//######################################################################
// Test vertices and nodes

ವರ್ಗ V3_ನಕ್ಷೆ_Test_Strong final : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ_Test {
ಸಾರ್ವಜನಿಕ:
    string name() override { ಹಿಂತಿರುಗಿ "strong"; }
    ಶೂನ್ಯ ಸತತ_Test() override {
        V3_ನಕ್ಷೆ* gp = &m_ನಕ್ಷೆ;
        // Verify we break edges at a good point
        // A simple alg would make 3 breaks, below only requires b->i to break
        V3_ನಕ್ಷೆ_Test_ಶೃಂಗ* i = ಹೊಸ V3_ನಕ್ಷೆ_Test_Var_ಶೃಂಗ{gp, "*INPUTS*"};
        V3_ನಕ್ಷೆ_Test_ಶೃಂಗ* a = ಹೊಸ V3_ನಕ್ಷೆ_Test_Var_ಶೃಂಗ{gp, "a"};
        V3_ನಕ್ಷೆ_Test_ಶೃಂಗ* b = ಹೊಸ V3_ನಕ್ಷೆ_Test_Var_ಶೃಂಗ{gp, "b"};
        V3_ನಕ್ಷೆ_Test_ಶೃಂಗ* g1 = ಹೊಸ V3_ನಕ್ಷೆ_Test_Var_ಶೃಂಗ{gp, "g1"};
        V3_ನಕ್ಷೆ_Test_ಶೃಂಗ* g2 = ಹೊಸ V3_ನಕ್ಷೆ_Test_Var_ಶೃಂಗ{gp, "g2"};
        V3_ನಕ್ಷೆ_Test_ಶೃಂಗ* g3 = ಹೊಸ V3_ನಕ್ಷೆ_Test_Var_ಶೃಂಗ{gp, "g3"};
        V3_ನಕ್ಷೆ_Test_ಶೃಂಗ* q = ಹೊಸ V3_ನಕ್ಷೆ_Test_Var_ಶೃಂಗ{gp, "q"};
        ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{gp, i, a, 2, ನಿಜ};
        ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{gp, a, b, 2, ನಿಜ};
        ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{gp, b, g1, 2, ನಿಜ};
        ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{gp, b, g2, 2, ನಿಜ};
        ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{gp, b, g3, 2, ನಿಜ};
        ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{gp, g1, a, 2, ನಿಜ};
        ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{gp, g3, g2, 2, ನಿಜ};
        ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{gp, g2, g3, 2, ನಿಜ};
        ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{gp, g1, q, 2, ನಿಜ};
        ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{gp, g2, q, 2, ನಿಜ};
        ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{gp, g3, q, 2, ನಿಜ};

        gp->stronglyConnected(&V3_ನಕ್ಷೆ_ಅಂಚು::followAlwaysTrue);
        dumpSelf();

        UASSERT(i->color() != a->color() && a->color() != g2->color() && g2->color() != q->color(),
                "SelfTest: Separate colors not assigned");
        UASSERT(a->color() == b->color() && a->color() == g1->color(),
                "SelfTest: Strongly connected nodes not colored together");
        UASSERT(g2->color() == g3->color(),
                "SelfTest: Strongly connected nodes not colored together");
    }
};

ವರ್ಗ V3_ನಕ್ಷೆ_Test_Acyc final : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ_Test {
ಸಾರ್ವಜನಿಕ:
    string name() override { ಹಿಂತಿರುಗಿ "acyc"; }
    ಶೂನ್ಯ ಸತತ_Test() override {
        V3_ನಕ್ಷೆ* gp = &m_ನಕ್ಷೆ;
        // Verify we break edges at a good point
        // A simple alg would make 3 breaks, below only requires b->i to break
        V3_ನಕ್ಷೆ_Test_ಶೃಂಗ* i = ಹೊಸ V3_ನಕ್ಷೆ_Test_Var_ಶೃಂಗ{gp, "*INPUTS*"};
        V3_ನಕ್ಷೆ_Test_ಶೃಂಗ* a = ಹೊಸ V3_ನಕ್ಷೆ_Test_Var_ಶೃಂಗ{gp, "a"};
        V3_ನಕ್ಷೆ_Test_ಶೃಂಗ* b = ಹೊಸ V3_ನಕ್ಷೆ_Test_Var_ಶೃಂಗ{gp, "b"};
        V3_ನಕ್ಷೆ_Test_ಶೃಂಗ* g1 = ಹೊಸ V3_ನಕ್ಷೆ_Test_Var_ಶೃಂಗ{gp, "g1"};
        V3_ನಕ್ಷೆ_Test_ಶೃಂಗ* g2 = ಹೊಸ V3_ನಕ್ಷೆ_Test_Var_ಶೃಂಗ{gp, "g2"};
        V3_ನಕ್ಷೆ_Test_ಶೃಂಗ* g3 = ಹೊಸ V3_ನಕ್ಷೆ_Test_Var_ಶೃಂಗ{gp, "g3"};
        ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{gp, i, a, 2, ನಿಜ};
        ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{gp, a, b, 2, ನಿಜ};
        ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{gp, b, g1, 2, ನಿಜ};
        ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{gp, b, g2, 2, ನಿಜ};
        ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{gp, b, g3, 2, ನಿಜ};
        ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{gp, g1, a, 2, ನಿಜ};
        ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{gp, g2, a, 2, ನಿಜ};
        ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{gp, g3, a, 2, ನಿಜ};

        gp->acyclic(&V3_ನಕ್ಷೆ_ಅಂಚು::followAlwaysTrue);
        gp->ಕ್ರಮ();
        dumpSelf();
    }
};

ವರ್ಗ V3_ನಕ್ಷೆ_Test_Vars final : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ_Test {
ಸಾರ್ವಜನಿಕ:
    string name() override { ಹಿಂತಿರುಗಿ "vars"; }
    ಶೂನ್ಯ ಸತತ_Test() override {
        V3_ನಕ್ಷೆ* ಸ್ಥಿರಾಂಕ gp = &m_ನಕ್ಷೆ;

        V3_ನಕ್ಷೆ_Test_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ clk = ಹೊಸ V3_ನಕ್ಷೆ_Test_Var_ಶೃಂಗ{gp, "$clk"};

        V3_ನಕ್ಷೆ_Test_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ a = ಹೊಸ V3_ನಕ್ಷೆ_Test_Var_ಶೃಂಗ{gp, "$a"};
        V3_ನಕ್ಷೆ_Test_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ a_dly = ಹೊಸ V3_ನಕ್ಷೆ_Test_Var_ಶೃಂಗ{gp, "$a_dly"};
        V3_ನಕ್ಷೆ_Test_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ a_dlyblk = ಹೊಸ V3_ನಕ್ಷೆ_Test_Var_ಶೃಂಗ{gp, "$a_dlyblk"};
        V3_ನಕ್ಷೆ_Test_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ b = ಹೊಸ V3_ನಕ್ಷೆ_Test_Var_ಶೃಂಗ{gp, "$b"};
        V3_ನಕ್ಷೆ_Test_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ b_dly = ಹೊಸ V3_ನಕ್ಷೆ_Test_Var_ಶೃಂಗ{gp, "$b_dly"};
        V3_ನಕ್ಷೆ_Test_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ b_dlyblk = ಹೊಸ V3_ನಕ್ಷೆ_Test_Var_ಶೃಂಗ{gp, "$b_dlyblk"};
        V3_ನಕ್ಷೆ_Test_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ c = ಹೊಸ V3_ನಕ್ಷೆ_Test_Var_ಶೃಂಗ{gp, "$c"};
        V3_ನಕ್ಷೆ_Test_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ i = ಹೊಸ V3_ನಕ್ಷೆ_Test_Var_ಶೃಂಗ{gp, "$i"};

        V3_ನಕ್ಷೆ_Test_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ ap = ಹೊಸ V3_ನಕ್ಷೆ_Test_Var_ಶೃಂಗ{gp, "$a_pre"};
        V3_ನಕ್ಷೆ_Test_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ bp = ಹೊಸ V3_ನಕ್ಷೆ_Test_Var_ಶೃಂಗ{gp, "$b_pre"};
        V3_ನಕ್ಷೆ_Test_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ cp = ಹೊಸ V3_ನಕ್ಷೆ_Test_Var_ಶೃಂಗ{gp, "$c_pre"};

        V3_ನಕ್ಷೆ_Test_ಶೃಂಗ* n;

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

        n = ಹೊಸ V3_ನಕ್ಷೆ_Test_ಶೃಂಗ{gp, "*INPUTS*"};
        {
            ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{gp, n, clk, 2};
            ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{gp, n, i, 2};
        }

        V3_ನಕ್ಷೆ_Test_ಶೃಂಗ* ಸ್ಥಿರಾಂಕ posedge = n = ಹೊಸ V3_ನಕ್ಷೆ_Test_ಶೃಂಗ{gp, "*posedge clk*"};
        { ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{gp, clk, n, 2}; }

        // AlwaysPre's     VarRefs on LHS:  generate special BLK
        //    normal:      VarRefs on LHS:  generate normal
        //    underSBlock: VarRefs on RHS:  consume 'pre' (required to save cutable tests)
        n = ಹೊಸ V3_ನಕ್ಷೆ_Test_ಶೃಂಗ{gp, "a_dly<PRE=a"};
        {
            ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{gp, n, a_dlyblk, 2};  // Block ordering
            ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{gp, n, a_dly, 2};
            ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{gp, ap, n, 2, ನಿಜ};  // DESIRED delayed ordering (inp is required)
            ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{gp, posedge, n, 2};
        }
        n = ಹೊಸ V3_ನಕ್ಷೆ_Test_ಶೃಂಗ{gp, "b_dly<PRE=b"};
        {
            ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{gp, n, b_dlyblk, 2};  // Block ordering
            ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{gp, n, b_dly, 2};
            ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{gp, bp, n, 2, ನಿಜ};  // DESIRED delayed ordering
            ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{gp, posedge, n, 2};
        }

        // AssignDly's     VarRefs on LHS:  consume special BLK
        //    normal:      VarRefs on LHS:  generate normal
        //    underSBlock: VarRefs on RHS:  generate 'pre' signals (cutable)
        //    SenItems:    consume CLOCK dependency
        n = ಹೊಸ V3_ನಕ್ಷೆ_Test_ಶೃಂಗ{gp, "a_dly<=b|c"};
        {
            ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{gp, a_dlyblk, n, 2};  // Block ordering in
            ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{gp, n, a_dly, 2};
            // Note we don't include ap as we're generating a_dly
            ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{gp, n, bp, 2};  // DESIRED delayed usage
            ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{gp, n, cp, 2};  // DESIRED delayed usage
            ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{gp, posedge, n, 2};
        }
        n = ಹೊಸ V3_ನಕ್ಷೆ_Test_ಶೃಂಗ{gp, "b_dly<=a"};
        {
            ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{gp, b_dlyblk, n, 2};  // Block ordering in
            ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{gp, n, b_dly, 2};
            ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{gp, n, ap, 2};  // DESIRED delayed usage
            ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{gp, posedge, n, 2};
        }

        // AlwaysPost's
        //    normal:      VarRefs on LHS:  generate normal
        //    underSBlock: VarRefs on RHS:  consume normal
        n = ಹೊಸ V3_ನಕ್ಷೆ_Test_ಶೃಂಗ{gp, "a=POST=a_dly"};
        {
            ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{gp, n, a, 3};
            ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{gp, a_dly, n, 3};
            ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{gp, posedge, n, 2};
        }
        n = ಹೊಸ V3_ನಕ್ಷೆ_Test_ಶೃಂಗ{gp, "b=POST=b_dly"};
        {
            ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{gp, n, b, 3};
            ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{gp, b_dly, n, 3};
            ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{gp, posedge, n, 2};
        }

        // COMBO
        // Inbound edges are always uncutable, because we must put combo logic after sequential
        // Outbound are cutable, as we may need to evaluate multiple times

        {
            n = ಹೊಸ V3_ನಕ್ಷೆ_Test_ಶೃಂಗ{gp, "c=a|b|i"};
            ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{gp, n, c, 1, ನಿಜ};
            ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{gp, a, n, 1, ಸುಳ್ಳು};
            ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{gp, b, n, 1, ಸುಳ್ಳು};
            ಹೊಸ V3_ನಕ್ಷೆ_ಅಂಚು{gp, i, n, 1, ಸುಳ್ಳು};
        }

        gp->acyclic(&V3_ನಕ್ಷೆ_ಅಂಚು::followAlwaysTrue);
        gp->ಕ್ರಮ();

        // Test debug function
        gp->dump_ಅಂಚುಗಳು(std::ಹೊರಗೆ, *ap);

        dumpSelf();
    }
};

//======================================================================

ವರ್ಗ V3_ನಕ್ಷೆ_Test_Import final : ಸಾರ್ವಜನಿಕ V3_ನಕ್ಷೆ_Test {

#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ ನಕ್ಷೆ_IMPORT
    ಶೂನ್ಯ dotImport();
#ಇಲ್ಲದಿದ್ದರೆ
    ಶೂನ್ಯ dotImport() {}
#ಆದರೆ_ಅಂತ್ಯ

ಸಾರ್ವಜನಿಕ:
    string name() override { ಹಿಂತಿರುಗಿ "import"; }
    ಶೂನ್ಯ ಸತತ_Test() override {
        V3_ನಕ್ಷೆ* ಸ್ಥಿರಾಂಕ gp = &m_ನಕ್ಷೆ;
        dotImport();
        dumpSelf();
        gp->acyclic(&V3_ನಕ್ಷೆ_ಅಂಚು::followAlwaysTrue);
        dumpSelf();
        gp->rank(&V3_ನಕ್ಷೆ_ಅಂಚು::followAlwaysTrue);
        dumpSelf();
    }
};

// clang-format off
#ವ್ಯಾಖ್ಯಾನಿತವಾದರೆ ನಕ್ಷೆ_IMPORT
# include "graph_export.cpp"
#ಆದರೆ_ಅಂತ್ಯ
// clang-format on

//======================================================================

ಶೂನ್ಯ V3_ನಕ್ಷೆ::selfTest() {
    // Execute all of the tests
    UINFO(2, __FUNCTION__ << ":");
    { V3_ನಕ್ಷೆ_Test_Strong{}.ಸತತ(); }
    { V3_ನಕ್ಷೆ_Test_Acyc{}.ಸತತ(); }
    { V3_ನಕ್ಷೆ_Test_Vars{}.ಸತತ(); }
    { V3_ನಕ್ಷೆ_Test_Import{}.ಸತತ(); }
}
