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

#تعریف_کرو VL_MT_DISABLED_کوڈ_UNIT 1

#ضمن "config_build.h"
#ضمن "verilatedos.h"

#ضمن "V3Global.h"
#ضمن "V3Graph.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
//######################################################################
// Test class

صنف V3_گراف_Test VL_NOT_FINAL {
محمي:
    // MEMBERS
    V3_گراف m_گراف;

    // METHODS - for children
    مجازی فراغ لگاتار_Test() = 0;  // Run the test
    مجازی string name() = 0;  // Name of the test

    // Utilities
    فراغ dumpSelf() {
        إذا (dump_گراف_Level() >= 9) m_گراف.dumpDotFilePrefixed("v3graphtest_" + name());
    }

عام:
    V3_گراف_Test() = افتراضي;
    مجازی ~V3_گراف_Test() = افتراضي;

    فراغ لگاتار() { لگاتار_Test(); }
};

//######################################################################
//######################################################################
// Vertices and nodes

صنف V3_گراف_Test_راس VL_NOT_FINAL : عام V3_گراف_راس {
    VL_RTTI_IMPL(V3_گراف_Test_راس, V3_گراف_راس)
    ثابت string m_name;

عام:
    V3_گراف_Test_راس(V3_گراف* graphp, ثابت string& name)
        : V3_گراف_راس{graphp}
        , m_name{name} {}
    ~V3_گراف_Test_راس() override = افتراضي;
    // ACCESSORS
    string name() ثابت override VL_MT_STABLE { أرجع m_name; }
};

صنف V3_گراف_Test_Var_راس final : عام V3_گراف_Test_راس {
عام:
    V3_گراف_Test_Var_راس(V3_گراف* graphp, ثابت string& name)
        : V3_گراف_Test_راس{graphp, name} {}
    ~V3_گراف_Test_Var_راس() override = افتراضي;
    // ACCESSORS
    string dotColor() ثابت override { أرجع "blue"; }
};

//######################################################################
//######################################################################
// Test vertices and nodes

صنف V3_گراف_Test_Strong final : عام V3_گراف_Test {
عام:
    string name() override { أرجع "strong"; }
    فراغ لگاتار_Test() override {
        V3_گراف* gp = &m_گراف;
        // Verify we break edges at a good point
        // A simple alg would make 3 breaks, below only requires b->i to break
        V3_گراف_Test_راس* i = جديد V3_گراف_Test_Var_راس{gp, "*INPUTS*"};
        V3_گراف_Test_راس* a = جديد V3_گراف_Test_Var_راس{gp, "a"};
        V3_گراف_Test_راس* b = جديد V3_گراف_Test_Var_راس{gp, "b"};
        V3_گراف_Test_راس* g1 = جديد V3_گراف_Test_Var_راس{gp, "g1"};
        V3_گراف_Test_راس* g2 = جديد V3_گراف_Test_Var_راس{gp, "g2"};
        V3_گراف_Test_راس* g3 = جديد V3_گراف_Test_Var_راس{gp, "g3"};
        V3_گراف_Test_راس* q = جديد V3_گراف_Test_Var_راس{gp, "q"};
        جديد V3_گراف_کنارا{gp, i, a, 2, سچ};
        جديد V3_گراف_کنارا{gp, a, b, 2, سچ};
        جديد V3_گراف_کنارا{gp, b, g1, 2, سچ};
        جديد V3_گراف_کنارا{gp, b, g2, 2, سچ};
        جديد V3_گراف_کنارا{gp, b, g3, 2, سچ};
        جديد V3_گراف_کنارا{gp, g1, a, 2, سچ};
        جديد V3_گراف_کنارا{gp, g3, g2, 2, سچ};
        جديد V3_گراف_کنارا{gp, g2, g3, 2, سچ};
        جديد V3_گراف_کنارا{gp, g1, q, 2, سچ};
        جديد V3_گراف_کنارا{gp, g2, q, 2, سچ};
        جديد V3_گراف_کنارا{gp, g3, q, 2, سچ};

        gp->stronglyConnected(&V3_گراف_کنارا::followAlwaysTrue);
        dumpSelf();

        UASSERT(i->color() != a->color() && a->color() != g2->color() && g2->color() != q->color(),
                "SelfTest: Separate colors not assigned");
        UASSERT(a->color() == b->color() && a->color() == g1->color(),
                "SelfTest: Strongly connected nodes not colored together");
        UASSERT(g2->color() == g3->color(),
                "SelfTest: Strongly connected nodes not colored together");
    }
};

صنف V3_گراف_Test_Acyc final : عام V3_گراف_Test {
عام:
    string name() override { أرجع "acyc"; }
    فراغ لگاتار_Test() override {
        V3_گراف* gp = &m_گراف;
        // Verify we break edges at a good point
        // A simple alg would make 3 breaks, below only requires b->i to break
        V3_گراف_Test_راس* i = جديد V3_گراف_Test_Var_راس{gp, "*INPUTS*"};
        V3_گراف_Test_راس* a = جديد V3_گراف_Test_Var_راس{gp, "a"};
        V3_گراف_Test_راس* b = جديد V3_گراف_Test_Var_راس{gp, "b"};
        V3_گراف_Test_راس* g1 = جديد V3_گراف_Test_Var_راس{gp, "g1"};
        V3_گراف_Test_راس* g2 = جديد V3_گراف_Test_Var_راس{gp, "g2"};
        V3_گراف_Test_راس* g3 = جديد V3_گراف_Test_Var_راس{gp, "g3"};
        جديد V3_گراف_کنارا{gp, i, a, 2, سچ};
        جديد V3_گراف_کنارا{gp, a, b, 2, سچ};
        جديد V3_گراف_کنارا{gp, b, g1, 2, سچ};
        جديد V3_گراف_کنارا{gp, b, g2, 2, سچ};
        جديد V3_گراف_کنارا{gp, b, g3, 2, سچ};
        جديد V3_گراف_کنارا{gp, g1, a, 2, سچ};
        جديد V3_گراف_کنارا{gp, g2, a, 2, سچ};
        جديد V3_گراف_کنارا{gp, g3, a, 2, سچ};

        gp->acyclic(&V3_گراف_کنارا::followAlwaysTrue);
        gp->ترتیب();
        dumpSelf();
    }
};

صنف V3_گراف_Test_Vars final : عام V3_گراف_Test {
عام:
    string name() override { أرجع "vars"; }
    فراغ لگاتار_Test() override {
        V3_گراف* ثابت gp = &m_گراف;

        V3_گراف_Test_راس* ثابت clk = جديد V3_گراف_Test_Var_راس{gp, "$clk"};

        V3_گراف_Test_راس* ثابت a = جديد V3_گراف_Test_Var_راس{gp, "$a"};
        V3_گراف_Test_راس* ثابت a_dly = جديد V3_گراف_Test_Var_راس{gp, "$a_dly"};
        V3_گراف_Test_راس* ثابت a_dlyblk = جديد V3_گراف_Test_Var_راس{gp, "$a_dlyblk"};
        V3_گراف_Test_راس* ثابت b = جديد V3_گراف_Test_Var_راس{gp, "$b"};
        V3_گراف_Test_راس* ثابت b_dly = جديد V3_گراف_Test_Var_راس{gp, "$b_dly"};
        V3_گراف_Test_راس* ثابت b_dlyblk = جديد V3_گراف_Test_Var_راس{gp, "$b_dlyblk"};
        V3_گراف_Test_راس* ثابت c = جديد V3_گراف_Test_Var_راس{gp, "$c"};
        V3_گراف_Test_راس* ثابت i = جديد V3_گراف_Test_Var_راس{gp, "$i"};

        V3_گراف_Test_راس* ثابت ap = جديد V3_گراف_Test_Var_راس{gp, "$a_pre"};
        V3_گراف_Test_راس* ثابت bp = جديد V3_گراف_Test_Var_راس{gp, "$b_pre"};
        V3_گراف_Test_راس* ثابت cp = جديد V3_گراف_Test_Var_راس{gp, "$c_pre"};

        V3_گراف_Test_راس* n;

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

        n = جديد V3_گراف_Test_راس{gp, "*INPUTS*"};
        {
            جديد V3_گراف_کنارا{gp, n, clk, 2};
            جديد V3_گراف_کنارا{gp, n, i, 2};
        }

        V3_گراف_Test_راس* ثابت posedge = n = جديد V3_گراف_Test_راس{gp, "*posedge clk*"};
        { جديد V3_گراف_کنارا{gp, clk, n, 2}; }

        // AlwaysPre's     VarRefs on LHS:  generate special BLK
        //    normal:      VarRefs on LHS:  generate normal
        //    underSBlock: VarRefs on RHS:  consume 'pre' (required to save cutable tests)
        n = جديد V3_گراف_Test_راس{gp, "a_dly<PRE=a"};
        {
            جديد V3_گراف_کنارا{gp, n, a_dlyblk, 2};  // Block ordering
            جديد V3_گراف_کنارا{gp, n, a_dly, 2};
            جديد V3_گراف_کنارا{gp, ap, n, 2, سچ};  // DESIRED delayed ordering (inp is required)
            جديد V3_گراف_کنارا{gp, posedge, n, 2};
        }
        n = جديد V3_گراف_Test_راس{gp, "b_dly<PRE=b"};
        {
            جديد V3_گراف_کنارا{gp, n, b_dlyblk, 2};  // Block ordering
            جديد V3_گراف_کنارا{gp, n, b_dly, 2};
            جديد V3_گراف_کنارا{gp, bp, n, 2, سچ};  // DESIRED delayed ordering
            جديد V3_گراف_کنارا{gp, posedge, n, 2};
        }

        // AssignDly's     VarRefs on LHS:  consume special BLK
        //    normal:      VarRefs on LHS:  generate normal
        //    underSBlock: VarRefs on RHS:  generate 'pre' signals (cutable)
        //    SenItems:    consume CLOCK dependency
        n = جديد V3_گراف_Test_راس{gp, "a_dly<=b|c"};
        {
            جديد V3_گراف_کنارا{gp, a_dlyblk, n, 2};  // Block ordering in
            جديد V3_گراف_کنارا{gp, n, a_dly, 2};
            // Note we don't include ap as we're generating a_dly
            جديد V3_گراف_کنارا{gp, n, bp, 2};  // DESIRED delayed usage
            جديد V3_گراف_کنارا{gp, n, cp, 2};  // DESIRED delayed usage
            جديد V3_گراف_کنارا{gp, posedge, n, 2};
        }
        n = جديد V3_گراف_Test_راس{gp, "b_dly<=a"};
        {
            جديد V3_گراف_کنارا{gp, b_dlyblk, n, 2};  // Block ordering in
            جديد V3_گراف_کنارا{gp, n, b_dly, 2};
            جديد V3_گراف_کنارا{gp, n, ap, 2};  // DESIRED delayed usage
            جديد V3_گراف_کنارا{gp, posedge, n, 2};
        }

        // AlwaysPost's
        //    normal:      VarRefs on LHS:  generate normal
        //    underSBlock: VarRefs on RHS:  consume normal
        n = جديد V3_گراف_Test_راس{gp, "a=POST=a_dly"};
        {
            جديد V3_گراف_کنارا{gp, n, a, 3};
            جديد V3_گراف_کنارا{gp, a_dly, n, 3};
            جديد V3_گراف_کنارا{gp, posedge, n, 2};
        }
        n = جديد V3_گراف_Test_راس{gp, "b=POST=b_dly"};
        {
            جديد V3_گراف_کنارا{gp, n, b, 3};
            جديد V3_گراف_کنارا{gp, b_dly, n, 3};
            جديد V3_گراف_کنارا{gp, posedge, n, 2};
        }

        // COMBO
        // Inbound edges are always uncutable, because we must put combo logic after sequential
        // Outbound are cutable, as we may need to evaluate multiple times

        {
            n = جديد V3_گراف_Test_راس{gp, "c=a|b|i"};
            جديد V3_گراف_کنارا{gp, n, c, 1, سچ};
            جديد V3_گراف_کنارا{gp, a, n, 1, خطأ};
            جديد V3_گراف_کنارا{gp, b, n, 1, خطأ};
            جديد V3_گراف_کنارا{gp, i, n, 1, خطأ};
        }

        gp->acyclic(&V3_گراف_کنارا::followAlwaysTrue);
        gp->ترتیب();

        // Test debug function
        gp->dump_کنارے(std::باہر, *ap);

        dumpSelf();
    }
};

//======================================================================

صنف V3_گراف_Test_Import final : عام V3_گراف_Test {

#جے_تعریف_شدہ گراف_IMPORT
    فراغ dotImport();
#وإلا
    فراغ dotImport() {}
#جے_انت

عام:
    string name() override { أرجع "import"; }
    فراغ لگاتار_Test() override {
        V3_گراف* ثابت gp = &m_گراف;
        dotImport();
        dumpSelf();
        gp->acyclic(&V3_گراف_کنارا::followAlwaysTrue);
        dumpSelf();
        gp->rank(&V3_گراف_کنارا::followAlwaysTrue);
        dumpSelf();
    }
};

// clang-format off
#جے_تعریف_شدہ گراف_IMPORT
# include "graph_export.cpp"
#جے_انت
// clang-format on

//======================================================================

فراغ V3_گراف::selfTest() {
    // Execute all of the tests
    UINFO(2, __FUNCTION__ << ":");
    { V3_گراف_Test_Strong{}.لگاتار(); }
    { V3_گراف_Test_Acyc{}.لگاتار(); }
    { V3_گراف_Test_Vars{}.لگاتار(); }
    { V3_گراف_Test_Import{}.لگاتار(); }
}
