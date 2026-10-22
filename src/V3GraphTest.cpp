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

#تعریف_کړه VL_MT_DISABLED_کوډ_UNIT 1

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Global.h"
#ضمن "V3Graph.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
//######################################################################
// Test class

صنف V3_ګراف_Test VL_NOT_FINAL {
محمي:
    // MEMBERS
    V3_ګراف m_ګراف;

    // METHODS - for children
    مجازي فراغ پرلپسې_Test() = 0;  // Run the test
    مجازي string name() = 0;  // Name of the test

    // Utilities
    فراغ dumpSelf() {
        إذا (dump_ګراف_Level() >= 9) m_ګراف.dumpDotFilePrefixed("v3graphtest_" + name());
    }

عام:
    V3_ګراف_Test() = افتراضي;
    مجازي ~V3_ګراف_Test() = افتراضي;

    فراغ پرلپسې() { پرلپسې_Test(); }
};

//######################################################################
//######################################################################
// Vertices and nodes

صنف V3_ګراف_Test_راس VL_NOT_FINAL : عام V3_ګراف_راس {
    VL_RTTI_IMPL(V3_ګراف_Test_راس, V3_ګراف_راس)
    ثابت string m_name;

عام:
    V3_ګراف_Test_راس(V3_ګراف* graphp, ثابت string& name)
        : V3_ګراف_راس{graphp}
        , m_name{name} {}
    ~V3_ګراف_Test_راس() override = افتراضي;
    // ACCESSORS
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
};

صنف V3_ګراف_Test_Var_راس final : عام V3_ګراف_Test_راس {
عام:
    V3_ګراف_Test_Var_راس(V3_ګراف* graphp, ثابت string& name)
        : V3_ګراف_Test_راس{graphp, name} {}
    ~V3_ګراف_Test_Var_راس() override = افتراضي;
    // ACCESSORS
    string dotColor() ثابت override { أرجع "blue"; }
};

//######################################################################
//######################################################################
// Test vertices and nodes

صنف V3_ګراف_Test_Strong final : عام V3_ګراف_Test {
عام:
    string name() override { أرجع "strong"; }
    فراغ پرلپسې_Test() override {
        V3_ګراف* gp = &m_ګراف;
        // Verify we break edges at a good point
        // A simple alg would make 3 breaks, below only requires b->i to break
        V3_ګراف_Test_راس* i = جديد V3_ګراف_Test_Var_راس{gp, "*INPUTS*"};
        V3_ګراف_Test_راس* a = جديد V3_ګراف_Test_Var_راس{gp, "a"};
        V3_ګراف_Test_راس* b = جديد V3_ګراف_Test_Var_راس{gp, "b"};
        V3_ګراف_Test_راس* g1 = جديد V3_ګراف_Test_Var_راس{gp, "g1"};
        V3_ګراف_Test_راس* g2 = جديد V3_ګراف_Test_Var_راس{gp, "g2"};
        V3_ګراف_Test_راس* g3 = جديد V3_ګراف_Test_Var_راس{gp, "g3"};
        V3_ګراف_Test_راس* q = جديد V3_ګراف_Test_Var_راس{gp, "q"};
        جديد V3_ګراف_څنډه{gp, i, a, 2, ریښتیا};
        جديد V3_ګراف_څنډه{gp, a, b, 2, ریښتیا};
        جديد V3_ګراف_څنډه{gp, b, g1, 2, ریښتیا};
        جديد V3_ګراف_څنډه{gp, b, g2, 2, ریښتیا};
        جديد V3_ګراف_څنډه{gp, b, g3, 2, ریښتیا};
        جديد V3_ګراف_څنډه{gp, g1, a, 2, ریښتیا};
        جديد V3_ګراف_څنډه{gp, g3, g2, 2, ریښتیا};
        جديد V3_ګراف_څنډه{gp, g2, g3, 2, ریښتیا};
        جديد V3_ګراف_څنډه{gp, g1, q, 2, ریښتیا};
        جديد V3_ګراف_څنډه{gp, g2, q, 2, ریښتیا};
        جديد V3_ګراف_څنډه{gp, g3, q, 2, ریښتیا};

        gp->stronglyConnected(&V3_ګراف_څنډه::followAlwaysTrue);
        dumpSelf();

        UASSERT(i->color() != a->color() && a->color() != g2->color() && g2->color() != q->color(),
                "SelfTest: Separate colors not assigned");
        UASSERT(a->color() == b->color() && a->color() == g1->color(),
                "SelfTest: Strongly connected nodes not colored together");
        UASSERT(g2->color() == g3->color(),
                "SelfTest: Strongly connected nodes not colored together");
    }
};

صنف V3_ګراف_Test_Acyc final : عام V3_ګراف_Test {
عام:
    string name() override { أرجع "acyc"; }
    فراغ پرلپسې_Test() override {
        V3_ګراف* gp = &m_ګراف;
        // Verify we break edges at a good point
        // A simple alg would make 3 breaks, below only requires b->i to break
        V3_ګراف_Test_راس* i = جديد V3_ګراف_Test_Var_راس{gp, "*INPUTS*"};
        V3_ګراف_Test_راس* a = جديد V3_ګراف_Test_Var_راس{gp, "a"};
        V3_ګراف_Test_راس* b = جديد V3_ګراف_Test_Var_راس{gp, "b"};
        V3_ګراف_Test_راس* g1 = جديد V3_ګراف_Test_Var_راس{gp, "g1"};
        V3_ګراف_Test_راس* g2 = جديد V3_ګراف_Test_Var_راس{gp, "g2"};
        V3_ګراف_Test_راس* g3 = جديد V3_ګراف_Test_Var_راس{gp, "g3"};
        جديد V3_ګراف_څنډه{gp, i, a, 2, ریښتیا};
        جديد V3_ګراف_څنډه{gp, a, b, 2, ریښتیا};
        جديد V3_ګراف_څنډه{gp, b, g1, 2, ریښتیا};
        جديد V3_ګراف_څنډه{gp, b, g2, 2, ریښتیا};
        جديد V3_ګراف_څنډه{gp, b, g3, 2, ریښتیا};
        جديد V3_ګراف_څنډه{gp, g1, a, 2, ریښتیا};
        جديد V3_ګراف_څنډه{gp, g2, a, 2, ریښتیا};
        جديد V3_ګراف_څنډه{gp, g3, a, 2, ریښتیا};

        gp->acyclic(&V3_ګراف_څنډه::followAlwaysTrue);
        gp->ترتیب();
        dumpSelf();
    }
};

صنف V3_ګراف_Test_Vars final : عام V3_ګراف_Test {
عام:
    string name() override { أرجع "vars"; }
    فراغ پرلپسې_Test() override {
        V3_ګراف* ثابت gp = &m_ګراف;

        V3_ګراف_Test_راس* ثابت clk = جديد V3_ګراف_Test_Var_راس{gp, "$clk"};

        V3_ګراف_Test_راس* ثابت a = جديد V3_ګراف_Test_Var_راس{gp, "$a"};
        V3_ګراف_Test_راس* ثابت a_dly = جديد V3_ګراف_Test_Var_راس{gp, "$a_dly"};
        V3_ګراف_Test_راس* ثابت a_dlyblk = جديد V3_ګراف_Test_Var_راس{gp, "$a_dlyblk"};
        V3_ګراف_Test_راس* ثابت b = جديد V3_ګراف_Test_Var_راس{gp, "$b"};
        V3_ګراف_Test_راس* ثابت b_dly = جديد V3_ګراف_Test_Var_راس{gp, "$b_dly"};
        V3_ګراف_Test_راس* ثابت b_dlyblk = جديد V3_ګراف_Test_Var_راس{gp, "$b_dlyblk"};
        V3_ګراف_Test_راس* ثابت c = جديد V3_ګراف_Test_Var_راس{gp, "$c"};
        V3_ګراف_Test_راس* ثابت i = جديد V3_ګراف_Test_Var_راس{gp, "$i"};

        V3_ګراف_Test_راس* ثابت ap = جديد V3_ګراف_Test_Var_راس{gp, "$a_pre"};
        V3_ګراف_Test_راس* ثابت bp = جديد V3_ګراف_Test_Var_راس{gp, "$b_pre"};
        V3_ګراف_Test_راس* ثابت cp = جديد V3_ګراف_Test_Var_راس{gp, "$c_pre"};

        V3_ګراف_Test_راس* n;

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

        n = جديد V3_ګراف_Test_راس{gp, "*INPUTS*"};
        {
            جديد V3_ګراف_څنډه{gp, n, clk, 2};
            جديد V3_ګراف_څنډه{gp, n, i, 2};
        }

        V3_ګراف_Test_راس* ثابت posedge = n = جديد V3_ګراف_Test_راس{gp, "*posedge clk*"};
        { جديد V3_ګراف_څنډه{gp, clk, n, 2}; }

        // AlwaysPre's     VarRefs on LHS:  generate special BLK
        //    normal:      VarRefs on LHS:  generate normal
        //    underSBlock: VarRefs on RHS:  consume 'pre' (required to save cutable tests)
        n = جديد V3_ګراف_Test_راس{gp, "a_dly<PRE=a"};
        {
            جديد V3_ګراف_څنډه{gp, n, a_dlyblk, 2};  // Block ordering
            جديد V3_ګراف_څنډه{gp, n, a_dly, 2};
            جديد V3_ګراف_څنډه{gp, ap, n, 2, ریښتیا};  // DESIRED delayed ordering (inp is required)
            جديد V3_ګراف_څنډه{gp, posedge, n, 2};
        }
        n = جديد V3_ګراف_Test_راس{gp, "b_dly<PRE=b"};
        {
            جديد V3_ګراف_څنډه{gp, n, b_dlyblk, 2};  // Block ordering
            جديد V3_ګراف_څنډه{gp, n, b_dly, 2};
            جديد V3_ګراف_څنډه{gp, bp, n, 2, ریښتیا};  // DESIRED delayed ordering
            جديد V3_ګراف_څنډه{gp, posedge, n, 2};
        }

        // AssignDly's     VarRefs on LHS:  consume special BLK
        //    normal:      VarRefs on LHS:  generate normal
        //    underSBlock: VarRefs on RHS:  generate 'pre' signals (cutable)
        //    SenItems:    consume CLOCK dependency
        n = جديد V3_ګراف_Test_راس{gp, "a_dly<=b|c"};
        {
            جديد V3_ګراف_څنډه{gp, a_dlyblk, n, 2};  // Block ordering in
            جديد V3_ګراف_څنډه{gp, n, a_dly, 2};
            // Note we don't include ap as we're generating a_dly
            جديد V3_ګراف_څنډه{gp, n, bp, 2};  // DESIRED delayed usage
            جديد V3_ګراف_څنډه{gp, n, cp, 2};  // DESIRED delayed usage
            جديد V3_ګراف_څنډه{gp, posedge, n, 2};
        }
        n = جديد V3_ګراف_Test_راس{gp, "b_dly<=a"};
        {
            جديد V3_ګراف_څنډه{gp, b_dlyblk, n, 2};  // Block ordering in
            جديد V3_ګراف_څنډه{gp, n, b_dly, 2};
            جديد V3_ګراف_څنډه{gp, n, ap, 2};  // DESIRED delayed usage
            جديد V3_ګراف_څنډه{gp, posedge, n, 2};
        }

        // AlwaysPost's
        //    normal:      VarRefs on LHS:  generate normal
        //    underSBlock: VarRefs on RHS:  consume normal
        n = جديد V3_ګراف_Test_راس{gp, "a=POST=a_dly"};
        {
            جديد V3_ګراف_څنډه{gp, n, a, 3};
            جديد V3_ګراف_څنډه{gp, a_dly, n, 3};
            جديد V3_ګراف_څنډه{gp, posedge, n, 2};
        }
        n = جديد V3_ګراف_Test_راس{gp, "b=POST=b_dly"};
        {
            جديد V3_ګراف_څنډه{gp, n, b, 3};
            جديد V3_ګراف_څنډه{gp, b_dly, n, 3};
            جديد V3_ګراف_څنډه{gp, posedge, n, 2};
        }

        // COMBO
        // Inbound edges are always uncutable, because we must put combo logic after sequential
        // Outbound are cutable, as we may need to evaluate multiple times

        {
            n = جديد V3_ګراف_Test_راس{gp, "c=a|b|i"};
            جديد V3_ګراف_څنډه{gp, n, c, 1, ریښتیا};
            جديد V3_ګراف_څنډه{gp, a, n, 1, خطأ};
            جديد V3_ګراف_څنډه{gp, b, n, 1, خطأ};
            جديد V3_ګراف_څنډه{gp, i, n, 1, خطأ};
        }

        gp->acyclic(&V3_ګراف_څنډه::followAlwaysTrue);
        gp->ترتیب();

        // Test debug function
        gp->dump_څنډې(std::وتنه, *ap);

        dumpSelf();
    }
};

//======================================================================

صنف V3_ګراف_Test_Import final : عام V3_ګراف_Test {

#که_تعریف_شوی ګراف_IMPORT
    فراغ dotImport();
#وإلا
    فراغ dotImport() {}
#د_که_پای

عام:
    string name() override { أرجع "import"; }
    فراغ پرلپسې_Test() override {
        V3_ګراف* ثابت gp = &m_ګراف;
        dotImport();
        dumpSelf();
        gp->acyclic(&V3_ګراف_څنډه::followAlwaysTrue);
        dumpSelf();
        gp->rank(&V3_ګراف_څنډه::followAlwaysTrue);
        dumpSelf();
    }
};

// clang-format off
#که_تعریف_شوی ګراف_IMPORT
# include "graph_export.cpp"
#د_که_پای
// clang-format on

//======================================================================

فراغ V3_ګراف::selfTest() {
    // Execute all of the tests
    UINFO(2, __FUNCTION__ << ":");
    { V3_ګراف_Test_Strong{}.پرلپسې(); }
    { V3_ګراف_Test_Acyc{}.پرلپسې(); }
    { V3_ګراف_Test_Vars{}.پرلپسې(); }
    { V3_ګراف_Test_Import{}.پرلپسې(); }
}
