// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Break always into sensitivity active domains
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
// V3Active's Transformations:
//
// Note this can be called multiple times.
//   Create a IACTIVE(initial), SACTIVE(combo)
//      ALWAYS: Remove any-edges from sense list
//          If no POS/NEG in senselist, Fold into SACTIVE(combo)
//          Else fold into SACTIVE(sequent).
//          OPTIMIZE: When support async clocks, fold into that active if possible
//      INITIAL: Move into IACTIVE
//      WIRE: Move into SACTIVE(combo)
//
//*************************************************************************

#ਅਨ੍ਤਰ੍ਭਾਵ "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Active.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Const.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "V3Graph.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_map>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_set>

VL_DEFINE_DEBUG_FUNCTIONS;

//***** See below for main transformation engine

//######################################################################

// Extend V3GraphVertex class for use in latch detection graph

ਵਰਗ Latch_Detect_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼ final : ਜਨਤਕ V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼ {
    VL_RTTI_IMPL(Latch_Detect_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼, V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼)
ਜਨਤਕ:
    ਗਿਣਤੀ ਸ਼ੀਰਸ਼_Type : uint8_t { VT_ਖੰਡ, VT_BRANCH, VT_OUTPUT };

ਨਿਜੀ:
    ਅਚਲ string m_name;  // Only used for .dot file generation
    ਅਚਲ ਸ਼ੀਰਸ਼_Type m_type;  // Vertex type (BLOCK/BRANCH/OUTPUT)

    string typestr() ਅਚਲ VL_MT_ਸੁਰੱਖਿਅਤ {  //   "
        ਚੋਣ (m_type) {
        ਹਾਲਤ VT_ਖੰਡ: ਵਾਪਸ "(||)";  // basic block node
        ਹਾਲਤ VT_BRANCH: ਵਾਪਸ "(&&)";  // if/else branch mode
        ਹਾਲਤ VT_OUTPUT: ਵਾਪਸ "(out)";  // var assignment
        ਮੂਲ: ਵਾਪਸ "??";  // unknown
        }
    }

ਜਨਤਕ:
    Latch_Detect_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼(V3_ਗ੍ਰਾਫ* graphp, ਅਚਲ string& name, ਸ਼ੀਰਸ਼_Type type = VT_ਖੰਡ)
        : V3_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼{graphp}
        , m_name{name}
        , m_type{type} {}
    string name() ਅਚਲ override VL_MT_STABLE { ਵਾਪਸ m_name + " " + typestr(); }
    string dotColor() ਅਚਲ override { ਵਾਪਸ user() ? "green" : "black"; }
    ਆਭਾਸੀ ਪੂਰਨ type() ਅਚਲ { ਵਾਪਸ m_type; }
};

//######################################################################
// Extend V3Graph class for use as a latch detection graph

ਵਰਗ Latch_Detect_ਗ੍ਰਾਫ final : ਜਨਤਕ V3_ਗ੍ਰਾਫ {
ਸੁਰੱਖਿਅਤ:
    Latch_Detect_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* m_curVertexp = ਖਾਲੀ_ਸੂਚਕ;  // Current latch detection graph vertex
    std::ਸਦਿਸ਼<AstVarRef*> m_outputs;  // Vector of lvalues encountered on this pass

    ਸਥਿਰ Latch_Detect_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* castVertexp(ਖਾਲੀ* vertexp) {
        ਵਾਪਸ reinterpret_cast<Latch_Detect_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼*>(vertexp);
    }

    // Recursively traverse the graph to determine whether every control 'BLOCK' has an assignment
    // to the output we are currently analyzing (the output whose 'user() is set), if so return
    // true. Where a BLOCK contains a BRANCH, both the if and else sides of the branch must return
    // true for the BRANCH to evaluate to true. A BLOCK however needs only a single one of its
    // siblings to evaluate true in order to evaluate true itself. On output vertex only evaluates
    // true if it is the vertex we are analyzing on this check

    ਤਾਰਕਿਕ latch_Check_ਅੰਦਰੂਨੀ(Latch_Detect_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* vertexp) {
        ਤਾਰਕਿਕ ਨਤੀਜਾ = ਝੂਠ;
        ਚੋਣ (vertexp->type()) {
        ਹਾਲਤ Latch_Detect_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼::VT_OUTPUT:  // Base case
            ਨਤੀਜਾ = vertexp->user();
            ਰੋਕੋ;
        ਹਾਲਤ Latch_Detect_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼::VT_ਖੰਡ:  // (OR of potentially many siblings)
            // cppcheck-suppress constVariableReference
            ਲਈ (V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ& ਕਿਨਾਰਾ : vertexp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ()) {
                ਜੇ (latch_Check_ਅੰਦਰੂਨੀ(castVertexp(ਕਿਨਾਰਾ.ਸਿਖਰ()))) {
                    ਨਤੀਜਾ = ਸੱਚ;
                    ਰੋਕੋ;
                }
            }
            ਰੋਕੋ;
        ਹਾਲਤ Latch_Detect_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼::VT_BRANCH:  // (AND of both sibling)
                                                 // A BRANCH vertex always has exactly 2 siblings
            Latch_Detect_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਅਚਲ ifp = castVertexp(vertexp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ().frontp()->ਸਿਖਰ());
            Latch_Detect_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਅਚਲ elsp = castVertexp(vertexp->ਬਾਹਰਲਾ_ਕਿਨਾਰੇ().backp()->ਸਿਖਰ());
            ਨਤੀਜਾ = latch_Check_ਅੰਦਰੂਨੀ(ifp) && latch_Check_ਅੰਦਰੂਨੀ(elsp);
            ਰੋਕੋ;
        }
        vertexp->user(ਨਤੀਜਾ);
        ਵਾਪਸ ਨਤੀਜਾ;
    }

ਜਨਤਕ:
    Latch_Detect_ਗ੍ਰਾਫ() { clear(); }
    ~Latch_Detect_ਗ੍ਰਾਫ() override { clear(); }
    // ACCESSORS
    Latch_Detect_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* currentp() { ਵਾਪਸ m_curVertexp; }
    ਖਾਲੀ currentp(Latch_Detect_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਸ਼ੀਰਸ਼) { m_curVertexp = ਸ਼ੀਰਸ਼; }
    // METHODS
    ਖਾਲੀ begin() {
        // Start a new if/else tracking graph
        m_curVertexp = ਨਵਾਂ Latch_Detect_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼{ਇਹ, "ROOT"};
    }
    // Clear out userp field of referenced outputs on destruction
    // (occurs at the end of each combinational always block)
    // cppcheck-suppress duplInheritedMember
    ਖਾਲੀ clear() {
        m_outputs.clear();
        // Calling base class clear will unlink & delete all edges & vertices
        V3_ਗ੍ਰਾਫ::clear();
        m_curVertexp = ਖਾਲੀ_ਸੂਚਕ;
    }
    // Add a new control path and connect it to its parent
    Latch_Detect_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਜੋੜੋ_ਰਸਤਾ_ਸ਼ੀਰਸ਼(Latch_Detect_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਮਾਪਾ, ਅਚਲ string& name,
                                          ਤਾਰਕਿਕ branch = ਝੂਠ) {
        m_curVertexp = ਨਵਾਂ Latch_Detect_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼{ਇਹ, name,
                                                  branch ? Latch_Detect_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼::VT_BRANCH
                                                         : Latch_Detect_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼::VT_ਖੰਡ};
        ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{ਇਹ, ਮਾਪਾ, m_curVertexp, 1};
        ਵਾਪਸ m_curVertexp;
    }
    // Add a new output variable vertex and store a pointer to it in the user1 field of the
    // variables AstNode
    Latch_Detect_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਜੋੜੋ_Output_ਸ਼ੀਰਸ਼(AstVarRef* nodep) {
        Latch_Detect_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਅਚਲ ਬਾਹਰਲਾ_Vertexp
            = ਨਵਾਂ Latch_Detect_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼{ਇਹ, nodep->name(), Latch_Detect_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼::VT_OUTPUT};
        nodep->varp()->user1p(ਬਾਹਰਲਾ_Vertexp);
        m_outputs.ਧੱਕੋ_ਪਿੱਠ(nodep);
        ਵਾਪਸ ਬਾਹਰਲਾ_Vertexp;
    }
    // Connect an output assignment to its parent control block
    ਖਾਲੀ ਜੋੜੋ_Assignment(AstVarRef* nodep) {
        Latch_Detect_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਬਾਹਰਲਾ_Vertexp;
        ਜੇ (!nodep->varp()->user1p()) {  // Not seen this output before
            ਬਾਹਰਲਾ_Vertexp = ਜੋੜੋ_Output_ਸ਼ੀਰਸ਼(nodep);
        } ਨਹੀਂਤਾਂ {
            ਬਾਹਰਲਾ_Vertexp = castVertexp(nodep->varp()->user1p());
        }
        ਨਵਾਂ V3_ਗ੍ਰਾਫ_ਕਿਨਾਰਾ{ਇਹ, m_curVertexp, ਬਾਹਰਲਾ_Vertexp, 1};
    }
    // Run latchCheckInternal on each variable assigned by the always block to see if all control
    // paths make an assignment. Detected latches are flagged in the variables AstVar
    ਖਾਲੀ latchCheck(Ast_ਗੰਢ* nodep, ਤਾਰਕਿਕ latch_expected) {
        ਤਾਰਕਿਕ latch_detected = ਝੂਠ;
        ਲਈ (ਅਚਲ AstVarRef* ਅਚਲ vrp : m_outputs) {
            Latch_Detect_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਅਚਲ vertp = castVertexp(vrp->varp()->user1p());
            vertp->user(ਸੱਚ);  // Identify the output vertex we are checking paths _to_
            ਜੇ (!latch_Check_ਅੰਦਰੂਨੀ(castVertexp(vertices().frontp()))) latch_detected = ਸੱਚ;
            ਜੇ (latch_detected && !latch_expected) {
                nodep->v3warn(
                    LATCH,
                    "Latch inferred for signal "
                        << vrp->prettyNameQ()
                        << " (not all control paths of combinational always assign a value)\n"
                        << nodep->warn_ਹੋਰ()
                        << "... Suggest use of always_latch for intentional latches");
                ਜੇ (dump_ਗ੍ਰਾਫ_Level() >= 9) dumpDotFilePrefixed("latch_" + vrp->name());
            }
            vertp->user(ਝੂਠ);  // Clear again (see above)
            vrp->varp()->isLatched(latch_detected);
        }
        // Should _all_ variables assigned in always_latch be latches? Probably, but this only
        // warns if none of them are
        ਜੇ (latch_expected && !latch_detected)
            nodep->v3warn(NOLATCH, "No latches detected in always_latch block");
    }
};

//######################################################################
// Collect existing active names

ਵਰਗ ਸਰਗਰਮ_Namer final : ਜਨਤਕ VNVisitor {
    // STATE
    AstScope* m_scopep = ਖਾਲੀ_ਸੂਚਕ;  // Current scope to add statement to
    Ast_ਸਰਗਰਮ* m_sActivep = ਖਾਲੀ_ਸੂਚਕ;  // For current scope, the Static active we're building
    Ast_ਸਰਗਰਮ* m_iActivep = ਖਾਲੀ_ਸੂਚਕ;  // For current scope, the Initial active we're building
    Ast_ਸਰਗਰਮ* m_fActivep = ਖਾਲੀ_ਸੂਚਕ;  // For current scope, the Final active we're building
    Ast_ਸਰਗਰਮ* m_cActivep = ਖਾਲੀ_ਸੂਚਕ;  // For current scope, the Combo active we're building

    // Map from AstSenTree (equivalence) to the corresponding AstActive created.
    std::unordered_map<VNRef<Ast_Sen_ਰੁੱਖ>, Ast_ਸਰਗਰਮ*> m_ਸਰਗਰਮ_Map;

    // METHODS
    ਖਾਲੀ ਜੋੜੋ_ਸਰਗਰਮ(Ast_ਸਰਗਰਮ* nodep) {
        UASSERT_OBJ(m_scopep, nodep, "nullptr scope");
        m_scopep->ਜੋੜੋ_Blocksp(nodep);
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstScope* nodep) override {
        m_scopep = nodep;
        m_sActivep = ਖਾਲੀ_ਸੂਚਕ;
        m_iActivep = ਖਾਲੀ_ਸੂਚਕ;
        m_fActivep = ਖਾਲੀ_ਸੂਚਕ;
        m_cActivep = ਖਾਲੀ_ਸੂਚਕ;
        m_ਸਰਗਰਮ_Map.clear();
        iterateChildren(nodep);
        // Don't clear scopep, the namer persists beyond this visit
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_Sen_ਰੁੱਖ* nodep) override {
        // Simplify sensitivity list
        VL_DO_DANGLING(V3Const::constifyExpensiveEdit(nodep), nodep);
    }
    //--------------------
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_Stmt*) override {}  // Accelerate
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

    // Specialized below for the special sensitivity classes
    ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ T_Sen_ਵਸਤੂ_ਕਿਸਮ>
    Ast_ਸਰਗਰਮ*& get_Special_ਸਰਗਰਮ();

ਜਨਤਕ:
    // METHODS
    AstScope* scopep() { ਵਾਪਸ m_scopep; }

    // Make a new AstActive sensitive to the given sentree and return it
    Ast_ਸਰਗਰਮ* make_ਸਰਗਰਮ(File_ਸਤਰ* ਅਚਲ fl, Ast_Sen_ਰੁੱਖ* ਅਚਲ senTreep) {
        Ast_ਸਰਗਰਮ* ਅਚਲ activep = ਨਵਾਂ Ast_ਸਰਗਰਮ{fl, "", senTreep};
        activep->sen_ਰੁੱਖ_Storep(activep->sentreep());
        ਜੋੜੋ_ਸਰਗਰਮ(activep);
        ਵਾਪਸ activep;
    }

    // Make a new AstActive sensitive to the given special sensitivity class and return it
    ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ T_Sen_ਵਸਤੂ_ਕਿਸਮ>
    Ast_ਸਰਗਰਮ* make_Special_ਸਰਗਰਮ(File_ਸਤਰ* ਅਚਲ fl) {
        Ast_Sen_ਰੁੱਖ* ਅਚਲ senTreep = ਨਵਾਂ Ast_Sen_ਰੁੱਖ{fl, ਨਵਾਂ Ast_Sen_ਵਸਤੂ{fl, T_Sen_ਵਸਤੂ_ਕਿਸਮ{}}};
        ਵਾਪਸ make_ਸਰਗਰਮ(fl, senTreep);
    }

    // Return an AstActive sensitive to the given special sensitivity class (possibly pre-created)
    ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ T_Sen_ਵਸਤੂ_ਕਿਸਮ>
    Ast_ਸਰਗਰਮ* get_Special_ਸਰਗਰਮ(File_ਸਤਰ* fl) {
        Ast_ਸਰਗਰਮ*& cachep = get_Special_ਸਰਗਰਮ<T_Sen_ਵਸਤੂ_ਕਿਸਮ>();
        ਜੇ (!cachep) cachep = make_Special_ਸਰਗਰਮ<T_Sen_ਵਸਤੂ_ਕਿਸਮ>(fl);
        ਵਾਪਸ cachep;
    }

    // Return an AstActive that is sensitive to a SenTree equivalent to the given sentreep.
    Ast_ਸਰਗਰਮ* get_ਸਰਗਰਮ(File_ਸਤਰ* fl, Ast_Sen_ਰੁੱਖ* sentreep) {
        UASSERT(sentreep, "Must be non-null");

        ਸਵੈਚਾਲਿਤ it = m_ਸਰਗਰਮ_Map.ਲੱਭੋ(*sentreep);
        // If found matching AstActive, return it
        ਜੇ (it != m_ਸਰਗਰਮ_Map.ਸਿਰਾ()) ਵਾਪਸ it->ਦੂਜਾ;

        // No such AstActive yet, creat it, and add to map.
        Ast_Sen_ਰੁੱਖ* ਅਚਲ newsenp = sentreep->clone_ਰੁੱਖ(ਝੂਠ);
        Ast_ਸਰਗਰਮ* ਅਚਲ activep = ਨਵਾਂ Ast_ਸਰਗਰਮ{fl, "sequent", newsenp};
        activep->sen_ਰੁੱਖ_Storep(activep->sentreep());
        ਜੋੜੋ_ਸਰਗਰਮ(activep);
        m_ਸਰਗਰਮ_Map.emplace(*newsenp, activep);
        ਵਾਪਸ activep;
    }

    // CONSTRUCTORS
    ਸਰਗਰਮ_Namer() = ਮੂਲ;
    ~ਸਰਗਰਮ_Namer() override = ਮੂਲ;
    ਖਾਲੀ ਮੁੱਖ(AstScope* nodep) { iterate(nodep); }
};

ਸਾਂਚਾ <>
Ast_ਸਰਗਰਮ*& ਸਰਗਰਮ_Namer::get_Special_ਸਰਗਰਮ<Ast_Sen_ਵਸਤੂ::Static>() {
    ਵਾਪਸ m_sActivep;
}
ਸਾਂਚਾ <>
Ast_ਸਰਗਰਮ*& ਸਰਗਰਮ_Namer::get_Special_ਸਰਗਰਮ<Ast_Sen_ਵਸਤੂ::Initial>() {
    ਵਾਪਸ m_iActivep;
}
ਸਾਂਚਾ <>
Ast_ਸਰਗਰਮ*& ਸਰਗਰਮ_Namer::get_Special_ਸਰਗਰਮ<Ast_Sen_ਵਸਤੂ::Final>() {
    ਵਾਪਸ m_fActivep;
}
ਸਾਂਚਾ <>
Ast_ਸਰਗਰਮ*& ਸਰਗਰਮ_Namer::get_Special_ਸਰਗਰਮ<Ast_Sen_ਵਸਤੂ::Combo>() {
    ਵਾਪਸ m_cActivep;
}

//######################################################################
// Latch checking visitor

ਵਰਗ ਸਰਗਰਮ_Latch_Check_Visitor final : ਜਨਤਕ VNVisitorConst {
    // NODE STATE
    // Input:
    //  AstVar::user1p // V2LatchGraphVertex* The vertex handling this node
    ਅਚਲ VNUser1_ਵਿੱਚ_Use m_inuser1;
    // STATE
    Latch_Detect_ਗ੍ਰਾਫ m_ਗ੍ਰਾਫ;  // Graph used to detect latches in combo always
    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstVarRef* nodep) override {
        ਅਚਲ AstVar* ਅਚਲ varp = nodep->varp();
        ਜੇ (nodep->access().isWriteOrRW() && varp->isSignal() && !varp->is_ਵਰਤਿਆ_Loop_Idx()
            && !varp->isFuncLocalSticky() && !varp->lifetime().isAutomatic()) {
            m_ਗ੍ਰਾਫ.ਜੋੜੋ_Assignment(nodep);
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ_If* nodep) override {
        ਜੇ (!nodep->isBoundsCheck()) {
            Latch_Detect_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਅਚਲ parentp = m_ਗ੍ਰਾਫ.currentp();
            Latch_Detect_ਗ੍ਰਾਫ_ਸ਼ੀਰਸ਼* ਅਚਲ branchp = m_ਗ੍ਰਾਫ.ਜੋੜੋ_ਰਸਤਾ_ਸ਼ੀਰਸ਼(parentp, "BRANCH", ਸੱਚ);
            m_ਗ੍ਰਾਫ.ਜੋੜੋ_ਰਸਤਾ_ਸ਼ੀਰਸ਼(branchp, "IF");
            iterate_And_ਅਗਲਾ_Const_Null(nodep->thensp());
            m_ਗ੍ਰਾਫ.ਜੋੜੋ_ਰਸਤਾ_ਸ਼ੀਰਸ਼(branchp, "ELSE");
            iterate_And_ਅਗਲਾ_Const_Null(nodep->elsesp());
            m_ਗ੍ਰਾਫ.currentp(parentp);
        } ਨਹੀਂਤਾਂ {
            iterateChildrenConst(nodep);
        }
    }
    //--------------------
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildrenConst(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਰਗਰਮ_Latch_Check_Visitor(Ast_ਗੰਢ* nodep, ਤਾਰਕਿਕ expectLatch) {
        m_ਗ੍ਰਾਫ.begin();
        iterateConst(nodep);
        m_ਗ੍ਰਾਫ.latchCheck(nodep, expectLatch);
    }
    ~ਸਰਗਰਮ_Latch_Check_Visitor() override = ਮੂਲ;
};

//######################################################################
// Replace unsupported non-blocking assignments with blocking assignments

ਵਰਗ ਸਰਗਰਮ_Dly_Visitor final : ਜਨਤਕ VNVisitor {
ਜਨਤਕ:
    ਗਿਣਤੀ CheckType : uint8_t { CT_COMB, CT_FINAL };

ਨਿਜੀ:
    // MEMBERS
    ਅਚਲ CheckType m_check;  // Process type we are checking

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸੌਂਪੋ_Dly* nodep) override {
        // Issue appropriate warning
        ਜੇ (m_check == CT_FINAL) {
            nodep->v3warn(FINALDLY, "Non-blocking assignment '<=' in final block");
        } ਨਹੀਂਤਾਂ {
            nodep->v3warn(COMBDLY,
                          "Non-blocking assignment '<=' in combinational logic process\n"
                              << nodep->warn_ਹੋਰ()
                              << "... This will be executed as a blocking assignment '='!");
        }

        // Convert to blocking assignment
        nodep->replaceWith(ਨਵਾਂ Ast_ਸੌਂਪੋ{
            nodep->fileline(),  //
            nodep->lhsp()->unlink_Fr_ਪਿੱਠ(),  //
            nodep->rhsp()->unlink_Fr_ਪਿੱਠ(),  //
            nodep->timingControlp() ? nodep->timingControlp()->unlink_Fr_ਪਿੱਠ() : ਖਾਲੀ_ਸੂਚਕ});
        VL_DO_DANGLING(nodep->delete_ਰੁੱਖ(), nodep);
    }

    //--------------------
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਰਗਰਮ_Dly_Visitor(Ast_ਗੰਢ* nodep, CheckType check)
        : m_check{check} {
        iterate(nodep);
    }
    ~ਸਰਗਰਮ_Dly_Visitor() override = ਮੂਲ;
};

//######################################################################
// Active class functions

ਵਰਗ ਸਰਗਰਮ_Visitor final : ਜਨਤਕ VNVisitor {
    // NODE STATE
    //  Each call to V3Const::constify
    //   AstVarScope::user1()           bool: This VarScope is referenced in the sensitivity list
    //   AstVarScope::user2()           bool: This VarScope is written in the current process
    //   AstNode::user4()               Used by V3Const::constify, called below

    // STATE
    ਸਰਗਰਮ_Namer m_namer;  // Tracking of active names
    ਤਾਰਕਿਕ m_clockedProcess = ਝੂਠ;  // Whether current process is a clocked process
    ਤਾਰਕਿਕ m_all_ਬਦਲਿਆ = ਝੂਠ;  // Whether all SenItem in the SenTree are ET_CHANGED
    ਤਾਰਕਿਕ m_walkingBody = ਝੂਠ;  // Walking body of a process
    ਤਾਰਕਿਕ m_canBeComb = ਝੂਠ;  // Whether current clocked process can be turned into a comb process

    // METHODS
    ਸਾਂਚਾ <ਕਿਸਮ_ਨਾਂ T>
    ਖਾਲੀ moveUnderSpecial(Ast_ਗੰਢ* nodep) {
        Ast_ਸਰਗਰਮ* ਅਚਲ wantactivep = m_namer.get_Special_ਸਰਗਰਮ<T>(nodep->fileline());
        nodep->unlink_Fr_ਪਿੱਠ();
        wantactivep->ਜੋੜੋ_Stmtsp(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ_Always(Ast_ਗੰਢ* nodep, Ast_Sen_ਰੁੱਖ* oldsentreep, VAlwaysKwd kwd) {
        // Move always to appropriate ACTIVE based on its sense list
        ਜੇ (oldsentreep && oldsentreep->sensesp() && oldsentreep->sensesp()->isNever()) {
            // Never executing.  Kill it.
            UASSERT_OBJ(!oldsentreep->sensesp()->nextp(), nodep,
                        "Never senitem should be alone, else the never should be eliminated.");
            VL_DO_DANGLING(nodep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), nodep);
            ਵਾਪਸ;
        }

        {
            ਅਚਲ VNUser1_ਵਿੱਚ_Use user1_ਵਿੱਚ_Use;

            // Walk sensitivity list
            m_clockedProcess = ਝੂਠ;
            m_all_ਬਦਲਿਆ = ਸੱਚ;
            ਜੇ (oldsentreep) {
                oldsentreep->unlink_Fr_ਪਿੱਠ();
                iterateChildrenConst(oldsentreep);
            }

            // If all SenItems are ET_CHANGE, then walk the body to determine if this process
            // could be turned into a combinational process instead.
            ਜੇ (m_all_ਬਦਲਿਆ) {
                ਅਚਲ VNUser2_ਵਿੱਚ_Use user2_ਵਿੱਚ_Use;
                m_walkingBody = ਸੱਚ;
                m_canBeComb = ਸੱਚ;
                iterateChildrenConst(nodep);
                m_walkingBody = ਝੂਠ;
                ਜੇ (m_canBeComb) m_clockedProcess = ਝੂਠ;
            }
        }

        Ast_ਸਰਗਰਮ* ਅਚਲ wantactivep
            = !m_clockedProcess ? m_namer.get_Special_ਸਰਗਰਮ<Ast_Sen_ਵਸਤੂ::Combo>(nodep->fileline())
              : oldsentreep     ? m_namer.get_ਸਰਗਰਮ(nodep->fileline(), oldsentreep)
                            // Clocked, no sensitivity lists, it's a suspendable, put it in initial
                            : m_namer.get_Special_ਸਰਗਰਮ<Ast_Sen_ਵਸਤੂ::Initial>(nodep->fileline());

        // Move node to new active
        nodep->unlink_Fr_ਪਿੱਠ();
        wantactivep->ਜੋੜੋ_Stmtsp(nodep);

        // Warn and convert any delayed assignments
        ਜੇ (!m_clockedProcess) ਸਰਗਰਮ_Dly_Visitor{nodep, ਸਰਗਰਮ_Dly_Visitor::CT_COMB};

        // Delete sensitivity list
        ਜੇ (oldsentreep) VL_DO_DANGLING(oldsentreep->delete_ਰੁੱਖ(), oldsentreep);

        // check combinational processes for latches
        ਜੇ (!m_clockedProcess || kwd == VAlwaysKwd::ALWAYS_LATCH) {
            ਅਚਲ ਸਰਗਰਮ_Latch_Check_Visitor latchvisitor{nodep, kwd == VAlwaysKwd::ALWAYS_LATCH};
        }
    }

    ਖਾਲੀ ਦੇਖੋ_Sen_ਚੀਜ਼ਾਂ(Ast_ਗੰਢ* nodep) {
        ਜੇ (v3Global.opt.timing().is_ਸੈੱਟ_True()) {
            nodep->foreach([ਇਹ](Ast_Sen_ਵਸਤੂ* senItemp) { ਦੇਖੋ(senItemp); });
        }
    }

    ਸਥਿਰ ਖਾਲੀ mark_Event_ਕਿਨਾਰੇ(Ast_Sen_ਰੁੱਖ* sentreep) {
        ਲਈ (Ast_Sen_ਵਸਤੂ* senip = sentreep->sensesp(); senip;
             senip = VN_AS(senip->nextp(), Sen_ਵਸਤੂ)) {
            ਜੇ (!senip->sensp()) ਜਾਰੀ;
            ਜੇ (ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ dtypep = senip->sensp()->dtypep()) {
                ਜੇ (ਅਚਲ AstBasicDType* ਅਚਲ basicp = dtypep->basicp()) {
                    ਜੇ (basicp->isEvent()) senip->ਕਿਨਾਰਾ_Type(VEdgeType::ET_EVENT);
                }
            }
        }
    }

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstScope* nodep) override {
        m_namer.ਮੁੱਖ(nodep);  // Clear last scope's names, and collect this scope's existing names
        iterateChildren(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸਰਗਰਮ* nodep) override {
        // Actives are being formed, so we can ignore any already made
    }

    ਖਾਲੀ ਦੇਖੋ(AstInitialStatic* nodep) override { moveUnderSpecial<Ast_Sen_ਵਸਤੂ::Static>(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstInitial* nodep) override {
        ਦੇਖੋ_Sen_ਚੀਜ਼ਾਂ(nodep);
        moveUnderSpecial<Ast_Sen_ਵਸਤੂ::Initial>(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstFinal* nodep) override {
        ਅਚਲ ਸਰਗਰਮ_Dly_Visitor dlyvisitor{nodep, ਸਰਗਰਮ_Dly_Visitor::CT_FINAL};
        moveUnderSpecial<Ast_Sen_ਵਸਤੂ::Final>(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstCoverToggle* nodep) override { moveUnderSpecial<Ast_Sen_ਵਸਤੂ::Combo>(nodep); }
    ਖਾਲੀ ਦੇਖੋ(AstAlways* nodep) override {
        ਜੇ (!nodep->stmtsp()) {  // Empty always. Remove it now.
            VL_DO_DANGLING(nodep->unlink_Fr_ਪਿੱਠ()->delete_ਰੁੱਖ(), nodep);
            ਵਾਪਸ;
        }
        ਜੇ (nodep->keyword() == VAlwaysKwd::CONT_ਸੌਂਪੋ) {
            moveUnderSpecial<Ast_Sen_ਵਸਤੂ::Combo>(nodep);
            ਵਾਪਸ;
        }
        ਦੇਖੋ_Sen_ਚੀਜ਼ਾਂ(nodep);
        ਦੇਖੋ_Always(nodep, nodep->sentreep(), nodep->keyword());
    }
    ਖਾਲੀ ਦੇਖੋ(AstAlwaysPostponed* nodep) override {
        // Might be empty with later optimizations, so this assertion can be removed,
        // but for now it is guaranteed to be not empty.
        UASSERT_OBJ(nodep->stmtsp(), nodep, "Should not be empty");
        // Make a new active for it, needs to be the only item under the active for V3Sched
        Ast_ਸਰਗਰਮ* ਅਚਲ activep = m_namer.make_Special_ਸਰਗਰਮ<Ast_Sen_ਵਸਤੂ::Combo>(nodep->fileline());
        activep->ਜੋੜੋ_Stmtsp(nodep->unlink_Fr_ਪਿੱਠ());
    }
    ਖਾਲੀ ਦੇਖੋ(AstAlwaysObserved* nodep) override {
        UASSERT_OBJ(nodep->sentreep(), nodep, "Should have a sentree");
        mark_Event_ਕਿਨਾਰੇ(nodep->sentreep());
        Ast_Sen_ਰੁੱਖ* ਅਚਲ sentreep = nodep->sentreep();
        sentreep->unlink_Fr_ਪਿੱਠ();
        // Make a new active for it, needs to be the only item under the active for V3Sched
        Ast_ਸਰਗਰਮ* ਅਚਲ activep = m_namer.make_ਸਰਗਰਮ(nodep->fileline(), sentreep);
        activep->ਜੋੜੋ_Stmtsp(nodep->unlink_Fr_ਪਿੱਠ());
    }
    ਖਾਲੀ ਦੇਖੋ(AstAlwaysReactive* nodep) override {
        UASSERT_OBJ(nodep->sentreep(), nodep, "Should have a sentree");
        mark_Event_ਕਿਨਾਰੇ(nodep->sentreep());
        Ast_Sen_ਰੁੱਖ* ਅਚਲ sentreep = nodep->sentreep();
        sentreep->unlink_Fr_ਪਿੱਠ();
        // Make a new active for it, needs to be the only item under the active for V3Sched
        Ast_ਸਰਗਰਮ* ਅਚਲ activep = m_namer.make_ਸਰਗਰਮ(nodep->fileline(), sentreep);
        activep->ਜੋੜੋ_Stmtsp(nodep->unlink_Fr_ਪਿੱਠ());
    }
    ਖਾਲੀ ਦੇਖੋ(AstCFunc* nodep) override { ਦੇਖੋ_Sen_ਚੀਜ਼ਾਂ(nodep); }
    ਖਾਲੀ ਦੇਖੋ(Ast_Sen_ਵਸਤੂ* nodep) override {
        UASSERT_OBJ(!m_walkingBody, nodep,
                    "Should not reach here when walking body without --timing");
        ਜੇ (!nodep->sensp()) ਵਾਪਸ;  // Ignore sequential items (e.g.: initial, comb, etc.)

        m_clockedProcess = ਸੱਚ;
        ਜੇ (nodep->ਕਿਨਾਰਾ_Type() != VEdgeType::ET_ਬਦਲਿਆ) m_all_ਬਦਲਿਆ = ਝੂਠ;

        ਜੇ (ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ dtypep = nodep->sensp()->dtypep()) {
            ਜੇ (ਅਚਲ AstBasicDType* ਅਚਲ basicp = dtypep->basicp()) {
                ਜੇ (basicp->isEvent()) nodep->ਕਿਨਾਰਾ_Type(VEdgeType::ET_EVENT);
            }
        }

        nodep->sensp()->foreach([](ਅਚਲ AstVarRef* refp) { refp->varScopep()->user1(ਸੱਚ); });
    }

    ਖਾਲੀ ਦੇਖੋ(AstVarRef* nodep) override {
        AstVarScope* ਅਚਲ vscp = nodep->varScopep();
        ਜੇ (nodep->access().isWriteOnly()) {
            vscp->user2(ਸੱਚ);
        } ਨਹੀਂਤਾਂ {
            // If the variable is read before it is written (and is not a never-changing value,
            // including an X randomization temporary set once at initialization), and is not
            // in the sensitivity list, then this cannot be optimized into a combinational process
            // TODO: live variable analysis would be more precise
            ਅਚਲ AstVar* ਅਚਲ varp = vscp->varp();
            ਜੇ (!vscp->user2() && !varp->valuep() && !varp->isXTemp() && !vscp->user1()) {
                m_canBeComb = ਝੂਠ;
            }
        }
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸੌਂਪੋ_Dly* nodep) override {
        m_canBeComb = ਝੂਠ;
        ਜੇ (nodep->isTimingControl()) m_clockedProcess = ਸੱਚ;
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstFireEvent* nodep) override {
        m_canBeComb = ਝੂਠ;
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਸੌਂਪੋ_Force* nodep) override {
        m_canBeComb = ਝੂਠ;
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstRelease* nodep) override {
        m_canBeComb = ਝੂਠ;
        iterateChildrenConst(nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(AstFork* nodep) override {
        ਜੇ (nodep->isTimingControl()) {
            m_canBeComb = ਝੂਠ;
            m_clockedProcess = ਸੱਚ;
        }
        // Do not iterate children, technically not part of this process
    }

    //--------------------
    ਖਾਲੀ ਦੇਖੋ(AstVar*) override {}  // Accelerate
    ਖਾਲੀ ਦੇਖੋ(AstVarScope*) override {}  // Accelerate
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override {
        ਜੇ (!v3Global.opt.timing().is_ਸੈੱਟ_True() && m_walkingBody && !m_canBeComb) {
            ਵਾਪਸ;  // Accelerate
        }
        ਜੇ (!nodep->is_ਸ਼ੁੱਧ()) m_canBeComb = ਝੂਠ;
        ਜੇ (nodep->isTimingControl()) {
            m_canBeComb = ਝੂਠ;
            m_clockedProcess = ਸੱਚ;
            ਵਾਪਸ;
        }
        iterateChildren(nodep);
    }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ ਸਰਗਰਮ_Visitor(AstNetlist* nodep) { iterate(nodep); }
    ~ਸਰਗਰਮ_Visitor() override = ਮੂਲ;
};

//######################################################################
// Pass 1: collect sample CFuncs and sampling events from covergroup class scopes

ਵਰਗ CovergroupCollectVisitor final : ਜਨਤਕ VNVisitor {
    // NODE STATE
    // Netlist:
    //  AstClass::user1p()  -> AstCFunc*.    The sample() CFunc for this covergroup class
    //  AstClass::user2p()  -> AstSenTree*.  Owned sampling event template (if any)

    // STATE
    AstClass* m_classp = ਖਾਲੀ_ਸੂਚਕ;  // Current covergroup class context, or nullptr

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstClass* nodep) override {
        ਜੇ (!nodep->isCovergroup()) ਵਾਪਸ;
        VL_RESTORER(m_classp);
        m_classp = nodep;
        iterateChildren(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstScope* nodep) override { iterateChildren(nodep); }

    ਖਾਲੀ ਦੇਖੋ(AstCFunc* nodep) override {
        ਜੇ (!m_classp) ਵਾਪਸ;
        ਜੇ (nodep->isCovergroupSample()) m_classp->user1p(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstCovergroup* nodep) override {
        // V3Covergroup guarantees: only supported-event covergroups survive to V3Active,
        // and they are always inside a covergroup class (so m_classp is set).
        // Unlink eventp from cgp so it survives cgp's deletion,
        // then store it in user2p for use during the second pass.
        m_classp->user2p(nodep->eventp()->unlink_Fr_ਪਿੱਠ());
        nodep->unlink_Fr_ਪਿੱਠ();
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ CovergroupCollectVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CovergroupCollectVisitor() override = ਮੂਲ;
};

//######################################################################
// Pass 2: inject automatic sample() calls for covergroup instances

ਵਰਗ CovergroupEventBindVisitor final : ਜਨਤਕ VNVisitor {
    AstVarScope* ਅਚਲ m_instancep;  // Variable scope for the covergroup instance being sampled
    std::unordered_ਸੈੱਟ<ਅਚਲ AstVar*> m_memberps;  // Non-static variables in the covergroup class

    ਖਾਲੀ ਦੇਖੋ(AstVarRef* nodep) override {
        ਜੇ (!m_memberps.ਗਿਣਤੀ_2(nodep->varp())) ਵਾਪਸ;
        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
        AstMemberSel* ਅਚਲ selp
            = ਨਵਾਂ AstMemberSel{fl, ਨਵਾਂ AstVarRef{fl, m_instancep, VAccess::READ}, nodep->varp()};
        selp->access(nodep->access());
        nodep->replaceWith(selp);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(nodep), nodep);
    }
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    CovergroupEventBindVisitor(Ast_Sen_ਰੁੱਖ* eventp, AstClass* classp, AstVarScope* instancep)
        : m_instancep{instancep} {
        classp->foreachMember([&](AstClass* ਅਚਲ, AstVar* ਅਚਲ varp) {  //
            m_memberps.emplace(varp);
        });
        iterate(eventp);
    }
    ~CovergroupEventBindVisitor() override = ਮੂਲ;
};

ਵਰਗ CovergroupInjectVisitor final : ਜਨਤਕ VNVisitor {
    // NODE STATE  (set by CovergroupCollectVisitor, consumed here)
    //  AstClass::user1p()  -> AstCFunc*.    The sample() CFunc for this covergroup class
    //  AstClass::user2p()  -> AstSenTree*.  Owned sampling event template (if any)

    // STATE
    ਸਰਗਰਮ_Namer m_namer;  // Reuse active naming infrastructure

    // VISITORS
    ਖਾਲੀ ਦੇਖੋ(AstScope* nodep) override {
        m_namer.ਮੁੱਖ(nodep);  // Initialize active naming for this scope
        iterateChildren(nodep);
    }

    ਖਾਲੀ ਦੇਖੋ(AstVarScope* nodep) override {
        // Get the underlying var
        AstVar* ਅਚਲ varp = nodep->varp();
        UASSERT_OBJ(varp, nodep, "AstVarScope must have non-null varp");

        // Check if the variable is of covergroup class type
        ਅਚਲ Ast_ਗੰਢ_DType* ਅਚਲ dtypep = varp->dtypep();
        UASSERT_OBJ(dtypep, nodep, "AstVar must have non-null dtypep after V3Width");

        ਅਚਲ AstClassRefDType* ਅਚਲ classRefp = VN_CAST(dtypep, ClassRefDType);
        ਜੇ (!classRefp) ਵਾਪਸ;

        AstClass* ਅਚਲ classp = classRefp->classp();

        // Check if this covergroup has an automatic sampling event
        Ast_Sen_ਰੁੱਖ* ਅਚਲ eventp = VN_CAST(classp->user2p(), Sen_ਰੁੱਖ);
        ਜੇ (!eventp) ਵਾਪਸ;  // No automatic sampling for this covergroup

        // V3Covergroup guarantees every supported-event covergroup has a registered sample CFunc
        AstCFunc* ਅਚਲ sampleCFuncp = VN_AS(classp->user1p(), CFunc);
        UASSERT_OBJ(sampleCFuncp, nodep,
                    "No sample() CFunc found for covergroup " << classp->name());

        // Create a VarRef to the covergroup instance for the method call
        File_ਸਤਰ* ਅਚਲ fl = nodep->fileline();
        AstVarRef* ਅਚਲ varrefp = ਨਵਾਂ AstVarRef{fl, nodep, VAccess::READ};

        // Create the CMethodCall to sample()
        // Note: We don't pass arguments in argsp since vlSymsp is passed via argTypes
        AstCMethodCall* ਅਚਲ cmethodCallp
            = ਨਵਾਂ AstCMethodCall{fl, varrefp, sampleCFuncp, ਖਾਲੀ_ਸੂਚਕ};

        cmethodCallp->dtype_ਸੈੱਟ_Void();
        cmethodCallp->argTypes("vlSymsp");

        // Clone the sensitivity for this active block. References to covergroup members need
        // to select through this particular instance; all other VarRefs retain the VarScopes
        // resolved by V3Scope.
        Ast_Sen_ਰੁੱਖ* senTreep = eventp->clone_ਰੁੱਖ(ਝੂਠ);
        CovergroupEventBindVisitor{senTreep, classp, nodep};

        // Get or create the AstActive node for this sensitivity
        // senTreep is a template used by getActive() which clones it into the AstActive;
        // delete it afterwards as it is not added to the AST directly.
        Ast_ਸਰਗਰਮ* ਅਚਲ activep = m_namer.get_ਸਰਗਰਮ(fl, senTreep);
        VL_DO_DANGLING(ਧੱਕੋ_Deletep(senTreep), senTreep);

        // Wrap the sample() call in an AstAlways so SchedPartition handles it
        // via visit(AstNodeProcedure*) like any other clocked always block.
        activep->ਜੋੜੋ_Stmtsp(
            ਨਵਾਂ AstAlways{fl, VAlwaysKwd::ALWAYS_FF, ਖਾਲੀ_ਸੂਚਕ, cmethodCallp->makeStmt()});
    }

    ਖਾਲੀ ਦੇਖੋ(AstClass* nodep) override {
        iterateChildren(nodep);
        // Delete the owned sampling event template stored during collection
        ਜੇ (Ast_Sen_ਰੁੱਖ* ਅਚਲ eventp = VN_CAST(nodep->user2p(), Sen_ਰੁੱਖ)) {
            VL_DO_DANGLING(ਧੱਕੋ_Deletep(eventp), eventp);
        }
    }

    ਖਾਲੀ ਦੇਖੋ(Ast_ਸਰਗਰਮ*) override {}  // Don't iterate into actives
    ਖਾਲੀ ਦੇਖੋ(Ast_ਗੰਢ* nodep) override { iterateChildren(nodep); }

ਜਨਤਕ:
    // CONSTRUCTORS
    ਸਪਸ਼ਟ CovergroupInjectVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~CovergroupInjectVisitor() override = ਮੂਲ;
};

//######################################################################
// Active class functions

ਖਾਲੀ V3_ਸਰਗਰਮ::ਸਰਗਰਮ_All(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ਸਰਗਰਮ_Visitor{nodep}; }  // Destruct before checking
    ਜੇ (v3Global.useCovergroup()) {
        // Add automatic covergroup sampling in two focused passes.
        // user1p/user2p on AstClass span both passes; guards must outlive both visitors.
        ਅਚਲ VNUser1_ਵਿੱਚ_Use user1_ਵਿੱਚ_Use;
        ਅਚਲ VNUser2_ਵਿੱਚ_Use user2_ਵਿੱਚ_Use;
        CovergroupCollectVisitor{nodep};  // Pass 1: collect CFuncs and events into user#p
        CovergroupInjectVisitor{nodep};  // Pass 2: inject sample() calls, delete user2p events
    }
    V3Global::dump_Check_Global_ਰੁੱਖ("active", 0, dump_ਰੁੱਖ_Either_Level() >= 3);
}
