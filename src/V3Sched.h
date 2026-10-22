// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Scheduling
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

#ਜੇ_ਅਪਰਿਭਾਸ਼ਿਤ VERILATOR_V3SCHED_H_
#ਪਰਿਭਾਸ਼ਿਤ_ਕਰੋ VERILATOR_V3SCHED_H_

#ਅਨ੍ਤਰ੍ਭਾਵ "config_build.h"
#ਅਨ੍ਤਰ੍ਭਾਵ "verilatedos.h"

#ਅਨ੍ਤਰ੍ਭਾਵ "V3Ast.h"

#ਅਨ੍ਤਰ੍ਭਾਵ <array>
#ਅਨ੍ਤਰ੍ਭਾਵ <functional>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_map>
#ਅਨ੍ਤਰ੍ਭਾਵ <unordered_set>
#ਅਨ੍ਤਰ੍ਭਾਵ <utility>
#ਅਨ੍ਤਰ੍ਭਾਵ <vector>

class Sen_ਵਿਅੰਜਕ_Builder;

//============================================================================

namespace V3Sched {

namespace util {
using Var_Scope_ਸੈੱਟ = std::unordered_ਸੈੱਟ<ਅਚਲ AstVarScope*>;

ਸਤਰਅੰਦਰ bool isVlForceVec(ਅਚਲ AstVarScope* vscp) {
    ਅਚਲ AstCDType* ਅਚਲ dtypep = VN_CAST(vscp->dtypep()->skipRefp(), CDType);
    ਵਾਪਸ dtypep && dtypep->name() == "VlForceVec";
}

ਸਤਰਅੰਦਰ bool is_Force_Read_ਵਿਧੀ(VCMethod ਵਿਧੀ) {
    ਵਾਪਸ ਵਿਧੀ == VCMethod::FORCE_READ || ਵਿਧੀ == VCMethod::FORCE_READ_ਸੂਚਕਾਂਕ;
}

ਸਤਰਅੰਦਰ ਖਾਲੀ collect_Force_Read_ਕਿਨਾਰਾ_Ignores(Ast_ਗੰਢ* nodep, Var_Scope_ਸੈੱਟ& ਬਾਹਰਲਾ) {
    Var_Scope_ਸੈੱਟ writtenForceVecs;
    Var_Scope_ਸੈੱਟ writtenVars;

    nodep->foreach([&](ਅਚਲ AstVarRef* refp) {
        AstVarScope* ਅਚਲ vscp = refp->varScopep();
        ਜੇ (!refp->access().isWriteOrRW()) ਵਾਪਸ;
        ਜੇ (!refp->varp()->ignoreSchedWrite()) writtenVars.emplace(vscp);
        ਜੇ (!isVlForceVec(vscp)) ਵਾਪਸ;
        writtenForceVecs.emplace(vscp);
        ਜੇ (!refp->varp()->ignoreSchedWrite()) ਬਾਹਰਲਾ.emplace(vscp);
    });

    ਜੇ (writtenForceVecs.ਖਾਲੀ_ਥਾਵਾਂ() || writtenVars.ਖਾਲੀ_ਥਾਵਾਂ()) ਵਾਪਸ;

    nodep->foreach([&](ਅਚਲ AstCMethodHard* callp) {
        ਜੇ (!is_Force_Read_ਵਿਧੀ(callp->ਵਿਧੀ())) ਵਾਪਸ;

        ਅਚਲ AstVarRef* ਅਚਲ fromRefp = VN_CAST(callp->fromp(), VarRef);
        ਜੇ (!fromRefp || !writtenForceVecs.ਗਿਣਤੀ_2(fromRefp->varScopep())) ਵਾਪਸ;

        Ast_ਗੰਢ_ਵਿਅੰਜਕ* ਅਚਲ origp = callp->pinsp();
        ਜੇ (!origp) ਵਾਪਸ;
        origp->foreach([&](ਅਚਲ AstVarRef* refp) {
            AstVarScope* ਅਚਲ vscp = refp->varScopep();
            ਜੇ (refp->access().isReadOrRW() && writtenVars.ਗਿਣਤੀ_2(vscp)) ਬਾਹਰਲਾ.emplace(vscp);
        });
    });
}
}  // namespace util

//============================================================================
// Throughout scheduling, we need to keep hold of AstActive nodes, together
// with the AstScope that they are under. LogicByScope is simply a list of
// such pairs, with some additional convenience methods.

class LogicByScope final {
ਜਨਤਕ:
    // TYPES
    // An AstActive and the AstScope it is under
    ਬਣਤਰ ਇੰਦਰਾਜ_2 final {
        AstScope* m_scopep;  // The scope the AstActive is under
        Ast_ਸਰਗਰਮ* m_activep;  // The AstActive holding the logic
        std::ਸੈੱਟ<uint32_t> m_triggers;  // Trigger indices m_activep is sensitive to
    };
    using iterator = std::ਸਦਿਸ਼<ਇੰਦਰਾਜ_2>::iterator;
    using const_iterator = std::ਸਦਿਸ਼<ਇੰਦਰਾਜ_2>::const_iterator;

ਨਿਜੀ:
    // STATE
    std::ਸਦਿਸ਼<ਇੰਦਰਾਜ_2> m_entries;  // The AstActives held, with their scopes

ਜਨਤਕ:
    // METHODS

    // Iteration and access
    iterator begin() { ਵਾਪਸ m_entries.begin(); }
    iterator ਸਿਰਾ() { ਵਾਪਸ m_entries.ਸਿਰਾ(); }
    const_iterator begin() ਅਚਲ { ਵਾਪਸ m_entries.begin(); }
    const_iterator ਸਿਰਾ() ਅਚਲ { ਵਾਪਸ m_entries.ਸਿਰਾ(); }
    bool ਖਾਲੀ_ਥਾਵਾਂ() ਅਚਲ { ਵਾਪਸ m_entries.ਖਾਲੀ_ਥਾਵਾਂ(); }
    ਮਾਪ_t ਮਾਪ() ਅਚਲ { ਵਾਪਸ m_entries.ਮਾਪ(); }
    ਅਚਲ ਇੰਦਰਾਜ_2& ਸੰਚਾਲਕ[](ਮਾਪ_t i) ਅਚਲ { ਵਾਪਸ m_entries[i]; }

    // Add an AstActive under the given scope
    ਖਾਲੀ ਜੋੜੋ(AstScope* scopep, Ast_ਸਰਗਰਮ* activep) { m_entries.ਧੱਕੋ_ਪਿੱਠ({scopep, activep, {}}); }

    // Add logic
    ਖਾਲੀ ਜੋੜੋ(AstScope* scopep, Ast_Sen_ਰੁੱਖ* senTreep, Ast_ਗੰਢ* logicp) {
        UASSERT_OBJ(!logicp->backp(), logicp, "Already linked");
        ਜੇ (m_entries.ਖਾਲੀ_ਥਾਵਾਂ() || m_entries.ਪਿੱਠ().m_scopep != scopep
            || m_entries.ਪਿੱਠ().m_activep->sentreep() != senTreep) {
            ਜੋੜੋ(scopep, new Ast_ਸਰਗਰਮ{logicp->fileline(), "", senTreep});
        }
        m_entries.ਪਿੱਠ().m_activep->ਜੋੜੋ_Stmtsp(logicp);
    };

    // Create copy, with the AstActives cloned
    LogicByScope clone() ਅਚਲ {
        LogicByScope ਨਤੀਜਾ;
        ਲਈ (ਅਚਲ ਇੰਦਰਾਜ_2& ਇੰਦਰਾਜ : m_entries) {
            ਨਤੀਜਾ.ਜੋੜੋ(ਇੰਦਰਾਜ.m_scopep, ਇੰਦਰਾਜ.m_activep->clone_ਰੁੱਖ(false));
        }
        ਵਾਪਸ ਨਤੀਜਾ;
    }

    // Delete actives (they should all be empty)
    ਖਾਲੀ deleteActives() {
        ਲਈ (ਅਚਲ ਇੰਦਰਾਜ_2& ਇੰਦਰਾਜ : m_entries) {
            Ast_ਸਰਗਰਮ* ਅਚਲ activep = ਇੰਦਰਾਜ.m_activep;
            ਜੇ (v3Global.opt.debugCheck()) {
                ਲਈ (Ast_ਗੰਢ* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) {
                    Ast_ਗੰਢ_Procedure* ਅਚਲ procp = VN_CAST(nodep, ਗੰਢ_Procedure);
                    UASSERT_OBJ(procp && !procp->stmtsp(), activep, "Leftover logic");
                }
            }
            ਜੇ (activep->backp()) activep->unlink_Fr_ਪਿੱਠ();
            VL_DO_DANGLING(activep->delete_ਰੁੱਖ(), activep);
        }
        m_entries.clear();
    };

    ਖਾਲੀ foreachLogic(ਅਚਲ std::function<ਖਾਲੀ(Ast_ਗੰਢ*)>& f) ਅਚਲ {
        ਲਈ (ਅਚਲ ਇੰਦਰਾਜ_2& ਇੰਦਰਾਜ : m_entries) {
            ਅਚਲ Ast_ਸਰਗਰਮ* ਅਚਲ activep = ਇੰਦਰਾਜ.m_activep;
            ਲਈ (Ast_ਗੰਢ* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) f(nodep);
        }
    }
};

// Logic in the design is classified based on what can trigger its execution.
// For details see the internals documentation.
ਬਣਤਰ LogicClasses final {
    LogicByScope m_static;  // Static variable initializers
    LogicByScope m_initial;  // initial blocks
    LogicByScope m_final;  // final blocks
    LogicByScope m_comb;  // Combinational logic (logic with implicit sensitivities)
    LogicByScope m_clocked;  // Clocked (or sequential) logic (logic with explicit sensitivities)
    LogicByScope m_hybrid;  // Hybrid logic (combinational logic with some explicit sensitivities)
    LogicByScope m_postponed;  // Postponed logic ($strobe)
    LogicByScope m_observed;  // Observed logic (contains AstAlwaysObserved)
    LogicByScope m_reactive;  // Reactive logic (contains AstAlwaysReactive)

    LogicClasses() = ਮੂਲ;
    VL_UNCOPYABLE(LogicClasses);
    LogicClasses(LogicClasses&&) = ਮੂਲ;
    LogicClasses& ਸੰਚਾਲਕ=(LogicClasses&&) = ਮੂਲ;
};

// Combinational (including hybrid) logic, and clocked logic in partitioned to compute all clock
// signals in the 'act' region. For details see the internals documentation.
ਬਣਤਰ LogicRegions final {
    LogicByScope m_pre;  // AstAlwaysPre logic in 'act' region
    LogicByScope m_act;  // 'act' region logic
    LogicByScope m_nba;  // 'nba' region logic
    LogicByScope m_obs;  // AstAlwaysObserved logic in 'obs' region
    LogicByScope m_react;  // AstAlwaysReactive logic in 're' region

    LogicRegions() = ਮੂਲ;
    VL_UNCOPYABLE(LogicRegions);
    LogicRegions(LogicRegions&&) = ਮੂਲ;
    LogicRegions& ਸੰਚਾਲਕ=(LogicRegions&&) = ਮੂਲ;
};

// Combinational (including hybrid) logic is replicated into the various scheduling regions.
// For details see the internals documentation.
ਬਣਤਰ LogicReplicas final {
    LogicByScope m_ico;  // Logic replicated into the 'ico' (Input Combinational) region
    LogicByScope m_act;  // Logic replicated into the 'act' region
    LogicByScope m_nba;  // Logic replicated into the 'nba' region
    LogicByScope m_obs;  // Logic replicated into the 'obs' region
    LogicByScope m_react;  // Logic replicated into the 're' region

    LogicReplicas() = ਮੂਲ;
    VL_UNCOPYABLE(LogicReplicas);
    LogicReplicas(LogicReplicas&&) = ਮੂਲ;
    LogicReplicas& ਸੰਚਾਲਕ=(LogicReplicas&&) = ਮੂਲ;
};

// A TriggerKit holds all the components related to a scheduling region triggers
//
// Each piece of code is executed when some trigger bits are set. There is no
// code after scheduling which is not conditional on at least one trigger bit.
//
// There are 3 different kinds of triggers
// 1. "Sense" triggers, which correspond to a unique SenItem in the design
// 2. "Extra" triggers, which represent non-SenItem based conditions
// 3. "Pre" triggers, which are only used in the 'act' region. These are a copy
//    of some of the "Sense" triggers but only ever fire during one evaluation
//    of the 'act' loop. They are used for executing AlwaysPre block that
//    reside in the 'act' region. (AlwaysPre in 'nba' use the "Sense" triggers.)
//
// All trigger bits are stored in an unpacked array of fixed sized words, which
// is informally referred to in various places as the "trigger vector".
// There is only one trigger vector for each eval loop (~scheduling region).
//
// The organization of the trigger vector (array) is shown here, LSB on the
// right, MSB on the left. There are 4 main sections:
//
// | <- bit N-1                                                                         bit 0 -> |
// +--------------------+----------------+----------------------------------+--------------------+
// | Pre triggers       | Extra triggers | Sense triggers                   | Pre Sense triggers |
// +--------------------+----------------+----------------------------------+--------------------+
// |        'pre'       |                                 'vec'                                  |
//
// The section labelled "Pre Sense triggers" contains regular "Sense" triggers
// that are also duplicated in the "Pre triggers" section the first time they
// fire.
//
// The "Sense triggers" section contains the rest of the "Sense" triggers that
// do not need a copy in "Pre triggers".
//
// The "Sense triggers" and "Pre Sense triggers" together contain all "Sense"
// triggers described in point 1 above. "Extra triggers" contains the
// non-SenItem based additional conditions described in point 2, and the
// "Per triggers" section contains the "Pre" triggers described in point 3.
//
// All 4 sections in the trigger vector are padded to contain a whole word
// worth of bits (padding bits are always zero). Any one of the 4 sections
// can be empty, but "Pre triggers" and "Pre Sense triggers" are always the
// same size.
//
// The portion holding the Sense triggers and Extra triggers is referred to
// in various places as the 'vec' part, and is also informally referred to as
// the trigger vector.
//
// The portion holding the Pre trigger is named 'pre'
//
// The combination of 'pre' + 'vec' is the "extended trigger vector" referred
// to as 'ext' in various places.
//
// In realistic designs there are often no "Pre" triggers, and only a few
// "Extra" triggers, with "Sense" triggers taking up the bulk of the bits.
//
class TriggerKit final {
ਜਨਤਕ:
    // Triggers are storead as an UnpackedArray with a fixed word size
    ਸਥਿਰ constexpr uint32_t ਸ਼ਬਦ_ਮਾਪ_LOG2 = 6;  // 64-bits / VL_QUADSIZE
    ਸਥਿਰ constexpr uint32_t ਸ਼ਬਦ_ਮਾਪ = 1 << ਸ਼ਬਦ_ਮਾਪ_LOG2;

ਨਿਜੀ:
    ਅਚਲ std::string m_name;  // TriggerKit name
    ਅਚਲ bool m_slow;  // TriggerKit is for schedulign 'slow' code
    ਅਚਲ uint32_t m_n_Sense_ਸ਼ਬਦ;  // Number of words for Sense triggers
    ਅਚਲ uint32_t m_n_Extra_ਸ਼ਬਦ;  // Number of words for Extra triggers
    ਅਚਲ uint32_t m_n_Pre_ਸ਼ਬਦ;  // Number of words for 'pre' part
    ਅਚਲ uint32_t m_n_Vec_ਸ਼ਬਦ = m_n_Sense_ਸ਼ਬਦ + m_n_Extra_ਸ਼ਬਦ;  // Number of words in 'vec' part

    // SenItems to corresponding bit indexes
    std::unordered_map<VNRef<ਅਚਲ Ast_Sen_ਵਸਤੂ>, ਮਾਪ_t> m_senItem2TrigIdx;
    // Bit index to corresponding SenItem, nullptr for padding and Extra triggers
    std::ਸਦਿਸ਼<ਅਚਲ Ast_Sen_ਵਸਤੂ*> m_trig_Idx2_Sen_ਵਸਤੂ;
    // Data type of a single trigger word
    Ast_ਗੰਢ_DType* m_ਸ਼ਬਦ_DTypep = nullptr;
    // Data type of a trigger vector holding one copy of all triggers
    AstUnpackArrayDType* m_trigVecDTypep = nullptr;
    // Data type of an extended trigger vector holding one copy of all triggers
    // + additional copy of 'pre' triggers
    AstUnpackArrayDType* m_trigExtDTypep = nullptr;
    // The AstVarScope representing the extended trigger vector
    AstVarScope* m_vscp = nullptr;
    // The AstVarScope representing the trigger accumulator vector
    // It is used to accumulate triggers that were found fired and cleared in beforeTrigger's
    // in current 'act' region iteration
    AstVarScope* m_vscAccp = nullptr;
    // The AstCFunc that computes the current active base triggers
    AstCFunc* m_compVecp = nullptr;
    // The AstCFunc that computes the current active extended triggers
    AstCFunc* m_compExtp = nullptr;
    // The AstCFunc that dumps a trigger vector
    AstCFunc* m_dumpp = nullptr;
    // The AstCFunc that dumps an exended trigger vector - create lazily
    mutable AstCFunc* m_dumpExtp = nullptr;
    // The AstCFunc testing if a trigger vector has any bits set - create lazily
    mutable AstCFunc* m_any_ਸੈੱਟ_Vecp = nullptr;
    mutable AstCFunc* m_any_ਸੈੱਟ_Extp = nullptr;
    // The AstCFunc setting bits in a trigger vector that are set in another - create lazily
    mutable std::array<AstCFunc*, 4> m_orIntoVecps = {nullptr};
    // The AstCFunc setting a trigger vector to all zeroes - create lazily
    mutable AstCFunc* m_clearp = nullptr;

    // Methods to lazy construct functions processing trigger vectors
    AstCFunc* createDumpExtFunc() ਅਚਲ;
    AstCFunc* create_Any_ਸੈੱਟ_Func(AstUnpackArrayDType* ਅਚਲ dtypep) ਅਚਲ;
    AstCFunc* createClearFunc() ਅਚਲ;
    AstCFunc* createOrIntoFunc(AstUnpackArrayDType* ਅਚਲ oDtypep,
                               AstUnpackArrayDType* ਅਚਲ iDtypep) ਅਚਲ;

    TriggerKit(ਅਚਲ std::string& name, bool slow, uint32_t n_Sense_ਸ਼ਬਦ, uint32_t n_Extra_ਸ਼ਬਦ,
               uint32_t n_Pre_ਸ਼ਬਦ,
               std::unordered_map<VNRef<ਅਚਲ Ast_Sen_ਵਸਤੂ>, ਮਾਪ_t> senItem2TrigIdx,
               ਅਚਲ std::ਸਦਿਸ਼<ਅਚਲ Ast_Sen_ਵਸਤੂ*>& senItemps, bool useAcc);
    VL_UNCOPYABLE(TriggerKit);

ਜਨਤਕ:
    // Move constructible
    TriggerKit(TriggerKit&&) = ਮੂਲ;
    ~TriggerKit() = ਮੂਲ;
    TriggerKit& ਸੰਚਾਲਕ=(TriggerKit&&) = delete;

    // Utility for extra trigger allocation
    class ExtraTriggers final {
        ਮਿੱਤਰ class TriggerKit;
        std::ਸਦਿਸ਼<string> m_descriptions;  // Human readable description of extra triggers

    ਜਨਤਕ:
        ExtraTriggers() = ਮੂਲ;
        ~ExtraTriggers() = ਮੂਲ;

        uint32_t allocate(ਅਚਲ string& description) {
            m_descriptions.ਧੱਕੋ_ਪਿੱਠ(description);
            ਵਾਪਸ m_descriptions.ਮਾਪ() - 1;
        }
        uint32_t ਮਾਪ() ਅਚਲ { ਵਾਪਸ m_descriptions.ਮਾਪ(); }
    };
    // Generates list of assignments that fills
    ਸਥਿਰ Ast_ਸੌਂਪੋ* createSenTrigVecAssignment(AstVarScope* ਅਚਲ ਨਿਸ਼ਾਨਾ,
                                                 std::ਸਦਿਸ਼<Ast_ਗੰਢ_ਵਿਅੰਜਕ*>& trigps);

    // Create a TriggerKit for the given AstSenTree vector
    ਸਥਿਰ TriggerKit create(AstNetlist* netlistp,  //
                             AstCFunc* ਅਚਲ initFuncp,  //
                             Sen_ਵਿਅੰਜਕ_Builder& sen_ਵਿਅੰਜਕ_Builder,  //
                             ਅਚਲ std::ਸਦਿਸ਼<ਅਚਲ Ast_Sen_ਰੁੱਖ*>& preTreeps,  //
                             ਅਚਲ std::ਸਦਿਸ਼<ਅਚਲ Ast_Sen_ਰੁੱਖ*>& senTreeps,  //
                             ਅਚਲ string& name,  //
                             ਅਚਲ ExtraTriggers& extraTriggers,  //
                             bool slow,  //
                             bool useAcc);

    // ACCESSORS
    AstVarScope* vscp() ਅਚਲ { ਵਾਪਸ m_vscp; }
    AstVarScope* vscAccp() ਅਚਲ { ਵਾਪਸ m_vscAccp; }
    ਮਾਪ_t senItem2TrigIdx(ਅਚਲ Ast_Sen_ਵਸਤੂ* senItemp) ਅਚਲ {
        ਵਾਪਸ m_senItem2TrigIdx.at(*senItemp);
    }
    ਅਚਲ std::ਸਦਿਸ਼<ਅਚਲ Ast_Sen_ਵਸਤੂ*>& trig_Idx2_Sen_ਵਸਤੂ() ਅਚਲ { ਵਾਪਸ m_trig_Idx2_Sen_ਵਸਤੂ; }
    AstCFunc* compBasep() ਅਚਲ { ਵਾਪਸ m_compVecp; }
    ਅਚਲ std::string& name() ਅਚਲ { ਵਾਪਸ m_name; }

    // Helpers for code generation - lazy construct relevant functions
    Ast_ਗੰਢ_Stmt* newAndNotCall(AstVarScope* op, AstVarScope* ap, AstVarScope* bp) ਅਚਲ;
    Ast_ਗੰਢ_ਵਿਅੰਜਕ* new_Any_ਸੈੱਟ_Call(AstVarScope* vscp) ਅਚਲ;
    Ast_ਗੰਢ_Stmt* newClearCall(AstVarScope* vscp) ਅਚਲ;
    Ast_ਗੰਢ_Stmt* newOrIntoCall(AstVarScope* op, AstVarScope* ip) ਅਚਲ;
    // Helpers for code generation
    Ast_ਗੰਢ_Stmt* new_Comp_ਅਧਾਰ_Call() ਅਚਲ;
    Ast_ਗੰਢ_Stmt* newCompExtCall(AstVarScope* vscp) ਅਚਲ;
    Ast_ਗੰਢ_Stmt* newDumpCall(AstVarScope* vscp, ਅਚਲ std::string& ਟੈਗ, bool debugOnly) ਅਚਲ;
    // Create a new (non-extended) trigger vector - might return nullptr if there are no triggers
    AstVarScope* newTrigVec(ਅਚਲ std::string& name) ਅਚਲ;

    // Index in the trigger vector of the given Extra trigger
    uint32_t extra_Trigger_ਸੂਚਕਾਂਕ(uint32_t ਸੂਚਕਾਂਕ) ਅਚਲ;

    // Indices in the trigger vector of the items of the given SenTree, either of the 'pre' copy,
    // or of the regular triggers
    std::ਸੈੱਟ<uint32_t> triggerIndices(ਅਚਲ Ast_Sen_ਰੁੱਖ* senTreep, bool pre) ਅਚਲ;

    // Statement setting the extra trigger bit at 'index' to the value of 'vscp'
    Ast_ਗੰਢ_Stmt* newExtraTriggerAssignment(AstVarScope* vscp, uint32_t ਸੂਚਕਾਂਕ) ਅਚਲ;

    // Set the extra trigger bit at 'index' to the value of 'vscp', then set 'vscp' to 0.
    // Prepended to the trigger computation function.
    ਖਾਲੀ ਜੋੜੋ_Extra_Trigger_Assignment(AstVarScope* vscp, uint32_t ਸੂਚਕਾਂਕ) ਅਚਲ;

    // Set trigger bit at 'index' when vscp's value changes from previous evaluation.
    // Creates a prev variable: trigger[bit] = (vscp != prev); prev = vscp
    ਖਾਲੀ ਜੋੜੋ_ਮੁੱਲ_Change_Trigger_Assignment(AstNetlist* netlistp, AstCFunc* initFuncp,
                                         AstVarScope* vscp, uint32_t ਸੂਚਕਾਂਕ) ਅਚਲ;
};

// Everything needed for combining timing with static scheduling.
class TimingKit final {
    AstCFunc* m_resumeFuncp = nullptr;  // Global timing resume function
    AstCFunc* m_readyFuncp = nullptr;  // Global timing ready function

    // Additional var sensitivities for V3Order
    std::map<ਅਚਲ AstVarScope*, std::ਸੈੱਟ<Ast_Sen_ਰੁੱਖ*>> m_externalDomains;

ਜਨਤਕ:
    LogicByScope m_lbs;  // Actives that resume timing schedulers
    Ast_ਗੰਢ_Stmt* m_postUpdates = nullptr;  // Post updates for the trigger eval function

    // Trigger indices of the external domains of each variable
    std::map<ਅਚਲ AstVarScope*, std::ਸਦਿਸ਼<uint32_t>>
    externalTriggerIndices(ਅਚਲ TriggerKit& trigKit) ਅਚਲ VL_MT_DISABLED;
    // Get the delay scheduler variable
    AstVarScope* getDelayScheduler(AstNetlist* ਅਚਲ netlistp) VL_MT_DISABLED;
    // Creates a timing resume call (if needed, else returns null)
    AstCCall* createResume(AstVarScope* trigVscp) VL_MT_DISABLED;
    // Creates a timing ready call (if needed, else returns null)
    AstCCall* createReady(AstVarScope* trigVscp) VL_MT_DISABLED;

    TimingKit() = ਮੂਲ;
    TimingKit(LogicByScope&& lbs, Ast_ਗੰਢ_Stmt* postUpdates,
              std::map<ਅਚਲ AstVarScope*, std::ਸੈੱਟ<Ast_Sen_ਰੁੱਖ*>>&& externalDomains)
        : m_externalDomains{externalDomains}
        , m_lbs{lbs}
        , m_postUpdates{postUpdates} {}
    VL_UNCOPYABLE(TimingKit);
    TimingKit(TimingKit&&) = ਮੂਲ;
    TimingKit& ਸੰਚਾਲਕ=(TimingKit&&) = ਮੂਲ;
};

class VirtIfaceTriggers final {
ਜਨਤਕ:
    // One entry per VarScope of an interface member that may be written through a virtual
    // interface. Each gets its own extra trigger bit with value-change detection.
    ਬਣਤਰ Trigger_ਇੰਦਰਾਜ final {
        ਅਚਲ AstIface* m_ifacep;  // Interface type
        ਅਚਲ AstVar* m_memberp;  // Member variable in the interface definition
        AstVarScope* m_vscp;  // VarScope to monitor
    };

    std::ਸਦਿਸ਼<Trigger_ਇੰਦਰਾਜ> m_triggers;

    ਖਾਲੀ ਜੋੜੋ_Trigger(ਅਚਲ AstIface* ifacep, ਅਚਲ AstVar* memberp, AstVarScope* vscp) {
        m_triggers.ਧੱਕੋ_ਪਿੱਠ(Trigger_ਇੰਦਰਾਜ{ifacep, memberp, vscp});
    }

    // Maps each triggered VarScope to its trigger index. addTriggeredIface() uses this
    // to look up triggers -- by specific VarScope for non-virtual reads, or by iterating
    // all entries of the same interface type for virtual interface reads.
    using VscpTrigMap = std::map<ਅਚਲ AstVarScope*, uint32_t>;
    VscpTrigMap make_Vscp_ਮੰਜ਼ਿਲ_Trig_Map(ਅਚਲ TriggerKit& trigKit, uint32_t ਪਹਿਲਾ_ਸੂਚਕਾਂਕ) ਅਚਲ;

    VL_UNCOPYABLE(VirtIfaceTriggers);
    VirtIfaceTriggers() = ਮੂਲ;
    VirtIfaceTriggers(VirtIfaceTriggers&&) = ਮੂਲ;
    VirtIfaceTriggers& ਸੰਚਾਲਕ=(VirtIfaceTriggers&&) = ਮੂਲ;
};

// Design variables bound to covergroup 'ref'/'const ref' formal arguments at construction.
//
// Such a formal becomes a pointer member, bound when the covergroup is constructed, so a
// sample() reading through it holds no AstVarRef naming the design variable. V3Order has to
// model those reads, but by the time it runs the constructions sit in logic it does not order,
// so they are collected before ordering and handed to it.
class CovergroupRefBindings final {
ਜਨਤਕ:
    // The design variables one covergroup sample() may read through its reference formals
    using Bindings = std::ਸਦਿਸ਼<AstVarScope*>;

ਨਿਜੀ:
    // Bindings of one covergroup handle. Exact, and so only present for a handle whose every
    // write is a construction we recognized -- see dropInstance().
    std::unordered_map<ਅਚਲ AstVarScope*, Bindings> m_byInstance;
    // Bindings over every construction of a covergroup class, used where the handle a sample()
    // call reaches is not known. An over-approximation, which is safe.
    std::unordered_map<ਅਚਲ AstClass*, Bindings> m_byClass;
    // How the sample() calls seen so far resolved. Statistics only, hence mutable: ordering
    // runs once per scheduling region and must hold this by const reference.
    mutable uint32_t m_ਅੰਕ_Exact_Calls = 0;
    mutable uint32_t m_ਅੰਕ_Union_Calls = 0;
    // Returned for a covergroup with no reference formal at all.
    ਸਥਿਰ ਅਚਲ Bindings s_none;

ਜਨਤਕ:
    // Record one construction of covergroup 'classp' binding reference formals to 'bindings'.
    // 'instp' is the handle assigned, or nullptr if the destination could not be identified.
    ਖਾਲੀ ਜੋੜੋ_Construction(ਅਚਲ AstClass* classp, ਅਚਲ AstVarScope* instp,
                         ਅਚਲ Bindings& bindings);
    // Forget the exact bindings of a handle, which turned out to be written by something other
    // than a construction we recognized. Its sample() calls fall back to the class union.
    ਖਾਲੀ dropInstance(ਅਚਲ AstVarScope* instp) { m_byInstance.erase(instp); }
    // What a sample() of covergroup 'classp' may read through reference formals, when called on
    // handle 'instp' (nullptr if the call is not on a plain handle reference)
    ਅਚਲ Bindings& forSample(ਅਚਲ AstVarScope* instp, ਅਚਲ AstClass* classp) ਅਚਲ;
    // Sample calls of a covergroup with a reference formal resolved to one handle's bindings
    uint32_t ਅੰਕ_Exact_Calls() ਅਚਲ { ਵਾਪਸ m_ਅੰਕ_Exact_Calls; }
    // ... and those that had to take the class union instead
    uint32_t ਅੰਕ_Union_Calls() ਅਚਲ { ਵਾਪਸ m_ਅੰਕ_Union_Calls; }

    VL_UNCOPYABLE(CovergroupRefBindings);
    CovergroupRefBindings() = ਮੂਲ;
    CovergroupRefBindings(CovergroupRefBindings&&) = ਮੂਲ;
    CovergroupRefBindings& ਸੰਚਾਲਕ=(CovergroupRefBindings&&) = ਮੂਲ;
};

// Creates trigger vars for signals driven via virtual interfaces
VirtIfaceTriggers makeVirtIfaceTriggers(AstNetlist* nodep) VL_MT_DISABLED;

// Collects what covergroup reference formal arguments are bound to at construction
ਅਚਲ CovergroupRefBindings makeCovergroupRefBindings(AstNetlist* nodep) VL_MT_DISABLED;

// Creates the timing kit and marks variables written by suspendables
TimingKit prepareTiming(AstNetlist* ਅਚਲ netlistp) VL_MT_DISABLED;

// Transforms fork sub-statements into separate functions
ਖਾਲੀ transformForks(AstNetlist* ਅਚਲ netlistp) VL_MT_DISABLED;

// Top level entry point to scheduling
ਖਾਲੀ ਸਮਾਂ_ਸਾਰਣੀ(AstNetlist*) VL_MT_DISABLED;

// Sub-steps
LogicByScope break_ਚੱਕਰਾਂ(AstNetlist* netlistp,
                         ਅਚਲ LogicByScope& combinationalLogic) VL_MT_DISABLED;
LogicRegions ਹਿੱਸੇ_ਕਰੋ(LogicByScope& clockedLogic, LogicByScope& combinationalLogic,
                       LogicByScope& hybridLogic) VL_MT_DISABLED;
LogicReplicas replicateLogic(LogicRegions&) VL_MT_DISABLED;

// Utility functions used by various steps in scheduling
namespace util {
// Create a new sub function (not an entry point)
AstCFunc* makeSubFunction(AstNetlist* netlistp, ਅਚਲ string& name, bool slow);
// Add an argument of the given type to the given function
AstVarScope* new_ਦਲੀਲ(AstCFunc* funcp, Ast_ਗੰਢ_DType* dtypep, ਅਚਲ string& name,
                         VDirection direction);
// Create statement that sets the given 'vscp' to 'val'
Ast_ਗੰਢ_Stmt* ਸੈੱਟ_Var(AstVarScope* vscp, uint32_t val);
// Create statement that increments the given 'vscp' by one
Ast_ਗੰਢ_Stmt* incrementVar(AstVarScope* vscp);
// Create statement that calls the given 'void' returning function
Ast_ਗੰਢ_Stmt* callVoidFunc(AstCFunc* funcp);
// Split large function according to --output-split-cfuncs
ਖਾਲੀ ਵੰਡੋ_Check(AstCFunc* ofuncp);
}  // namespace util

ਖਾਲੀ beforeTrigVisitor(AstNetlist* netlistp, Sen_ਵਿਅੰਜਕ_Builder& sen_ਵਿਅੰਜਕ_Builder,
                       ਅਚਲ TriggerKit& trigKit);

}  // namespace V3Sched

#ਜੇ_ਅੰਤ  // Guard
