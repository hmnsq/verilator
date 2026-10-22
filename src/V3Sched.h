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

#నిర్వచితం_కాకపోతే VERILATOR_V3SCHED_H_
#నిర్వచించు VERILATOR_V3SCHED_H_

#అన్తర్భావ "config_build.h"
#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Ast.h"

#అన్తర్భావ <array>
#అన్తర్భావ <functional>
#అన్తర్భావ <unordered_map>
#అన్తర్భావ <unordered_set>
#అన్తర్భావ <utility>
#అన్తర్భావ <vector>

class Sen_సమాసం_Builder;

//============================================================================

namespace V3Sched {

namespace util {
using Var_Scope_సమితి = std::unordered_సమితి<స్థిరరాశి AstVarScope*>;

అంతఃవరుస bool isVlForceVec(స్థిరరాశి AstVarScope* vscp) {
    స్థిరరాశి AstCDType* స్థిరరాశి dtypep = VN_CAST(vscp->dtypep()->skipRefp(), CDType);
    తిరిగి dtypep && dtypep->name() == "VlForceVec";
}

అంతఃవరుస bool is_Force_Read_పద్ధతి(VCMethod పద్ధతి) {
    తిరిగి పద్ధతి == VCMethod::FORCE_READ || పద్ధతి == VCMethod::FORCE_READ_సూచీ;
}

అంతఃవరుస శూన్య collect_Force_Read_అంచు_Ignores(Ast_కణుపు* nodep, Var_Scope_సమితి& బహిర్గతం) {
    Var_Scope_సమితి writtenForceVecs;
    Var_Scope_సమితి writtenVars;

    nodep->foreach([&](స్థిరరాశి AstVarRef* refp) {
        AstVarScope* స్థిరరాశి vscp = refp->varScopep();
        ఒకవేళ (!refp->access().isWriteOrRW()) తిరిగి;
        ఒకవేళ (!refp->varp()->ignoreSchedWrite()) writtenVars.emplace(vscp);
        ఒకవేళ (!isVlForceVec(vscp)) తిరిగి;
        writtenForceVecs.emplace(vscp);
        ఒకవేళ (!refp->varp()->ignoreSchedWrite()) బహిర్గతం.emplace(vscp);
    });

    ఒకవేళ (writtenForceVecs.ఖాళీలు() || writtenVars.ఖాళీలు()) తిరిగి;

    nodep->foreach([&](స్థిరరాశి AstCMethodHard* callp) {
        ఒకవేళ (!is_Force_Read_పద్ధతి(callp->పద్ధతి())) తిరిగి;

        స్థిరరాశి AstVarRef* స్థిరరాశి fromRefp = VN_CAST(callp->fromp(), VarRef);
        ఒకవేళ (!fromRefp || !writtenForceVecs.లెక్క(fromRefp->varScopep())) తిరిగి;

        Ast_కణుపు_సమాసం* స్థిరరాశి origp = callp->pinsp();
        ఒకవేళ (!origp) తిరిగి;
        origp->foreach([&](స్థిరరాశి AstVarRef* refp) {
            AstVarScope* స్థిరరాశి vscp = refp->varScopep();
            ఒకవేళ (refp->access().isReadOrRW() && writtenVars.లెక్క(vscp)) బహిర్గతం.emplace(vscp);
        });
    });
}
}  // namespace util

//============================================================================
// Throughout scheduling, we need to keep hold of AstActive nodes, together
// with the AstScope that they are under. LogicByScope is simply a list of
// such pairs, with some additional convenience methods.

class LogicByScope final {
బహిరంగ:
    // TYPES
    // An AstActive and the AstScope it is under
    నిర్మాణం నమోదు_3 final {
        AstScope* m_scopep;  // The scope the AstActive is under
        Ast_చురుకైన* m_activep;  // The AstActive holding the logic
        std::సమితి<uint32_t> m_triggers;  // Trigger indices m_activep is sensitive to
    };
    using iterator = std::సదిశ<నమోదు_3>::iterator;
    using const_iterator = std::సదిశ<నమోదు_3>::const_iterator;

అంతరంగ:
    // STATE
    std::సదిశ<నమోదు_3> m_entries;  // The AstActives held, with their scopes

బహిరంగ:
    // METHODS

    // Iteration and access
    iterator begin() { తిరిగి m_entries.begin(); }
    iterator చివర() { తిరిగి m_entries.చివర(); }
    const_iterator begin() స్థిరరాశి { తిరిగి m_entries.begin(); }
    const_iterator చివర() స్థిరరాశి { తిరిగి m_entries.చివర(); }
    bool ఖాళీలు() స్థిరరాశి { తిరిగి m_entries.ఖాళీలు(); }
    కొలత_t కొలత() స్థిరరాశి { తిరిగి m_entries.కొలత(); }
    స్థిరరాశి నమోదు_3& క్రియాకారకం[](కొలత_t i) స్థిరరాశి { తిరిగి m_entries[i]; }

    // Add an AstActive under the given scope
    శూన్య కూడు(AstScope* scopep, Ast_చురుకైన* activep) { m_entries.నెట్టు_వెనుక({scopep, activep, {}}); }

    // Add logic
    శూన్య కూడు(AstScope* scopep, Ast_Sen_వృక్షం* senTreep, Ast_కణుపు* logicp) {
        UASSERT_OBJ(!logicp->backp(), logicp, "Already linked");
        ఒకవేళ (m_entries.ఖాళీలు() || m_entries.వెనుక().m_scopep != scopep
            || m_entries.వెనుక().m_activep->sentreep() != senTreep) {
            కూడు(scopep, new Ast_చురుకైన{logicp->fileline(), "", senTreep});
        }
        m_entries.వెనుక().m_activep->కూడు_Stmtsp(logicp);
    };

    // Create copy, with the AstActives cloned
    LogicByScope clone() స్థిరరాశి {
        LogicByScope ఫలితం;
        కొరకు (స్థిరరాశి నమోదు_3& నమోదు_2 : m_entries) {
            ఫలితం.కూడు(నమోదు_2.m_scopep, నమోదు_2.m_activep->clone_వృక్షం(false));
        }
        తిరిగి ఫలితం;
    }

    // Delete actives (they should all be empty)
    శూన్య deleteActives() {
        కొరకు (స్థిరరాశి నమోదు_3& నమోదు_2 : m_entries) {
            Ast_చురుకైన* స్థిరరాశి activep = నమోదు_2.m_activep;
            ఒకవేళ (v3Global.opt.debugCheck()) {
                కొరకు (Ast_కణుపు* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) {
                    Ast_కణుపు_Procedure* స్థిరరాశి procp = VN_CAST(nodep, కణుపు_Procedure);
                    UASSERT_OBJ(procp && !procp->stmtsp(), activep, "Leftover logic");
                }
            }
            ఒకవేళ (activep->backp()) activep->unlink_Fr_వెనుక();
            VL_DO_DANGLING(activep->delete_వృక్షం(), activep);
        }
        m_entries.clear();
    };

    శూన్య foreachLogic(స్థిరరాశి std::function<శూన్య(Ast_కణుపు*)>& f) స్థిరరాశి {
        కొరకు (స్థిరరాశి నమోదు_3& నమోదు_2 : m_entries) {
            స్థిరరాశి Ast_చురుకైన* స్థిరరాశి activep = నమోదు_2.m_activep;
            కొరకు (Ast_కణుపు* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) f(nodep);
        }
    }
};

// Logic in the design is classified based on what can trigger its execution.
// For details see the internals documentation.
నిర్మాణం LogicClasses final {
    LogicByScope m_static;  // Static variable initializers
    LogicByScope m_initial;  // initial blocks
    LogicByScope m_final;  // final blocks
    LogicByScope m_comb;  // Combinational logic (logic with implicit sensitivities)
    LogicByScope m_clocked;  // Clocked (or sequential) logic (logic with explicit sensitivities)
    LogicByScope m_hybrid;  // Hybrid logic (combinational logic with some explicit sensitivities)
    LogicByScope m_postponed;  // Postponed logic ($strobe)
    LogicByScope m_observed;  // Observed logic (contains AstAlwaysObserved)
    LogicByScope m_reactive;  // Reactive logic (contains AstAlwaysReactive)

    LogicClasses() = అప్రమేయ;
    VL_UNCOPYABLE(LogicClasses);
    LogicClasses(LogicClasses&&) = అప్రమేయ;
    LogicClasses& క్రియాకారకం=(LogicClasses&&) = అప్రమేయ;
};

// Combinational (including hybrid) logic, and clocked logic in partitioned to compute all clock
// signals in the 'act' region. For details see the internals documentation.
నిర్మాణం LogicRegions final {
    LogicByScope m_pre;  // AstAlwaysPre logic in 'act' region
    LogicByScope m_act;  // 'act' region logic
    LogicByScope m_nba;  // 'nba' region logic
    LogicByScope m_obs;  // AstAlwaysObserved logic in 'obs' region
    LogicByScope m_react;  // AstAlwaysReactive logic in 're' region

    LogicRegions() = అప్రమేయ;
    VL_UNCOPYABLE(LogicRegions);
    LogicRegions(LogicRegions&&) = అప్రమేయ;
    LogicRegions& క్రియాకారకం=(LogicRegions&&) = అప్రమేయ;
};

// Combinational (including hybrid) logic is replicated into the various scheduling regions.
// For details see the internals documentation.
నిర్మాణం LogicReplicas final {
    LogicByScope m_ico;  // Logic replicated into the 'ico' (Input Combinational) region
    LogicByScope m_act;  // Logic replicated into the 'act' region
    LogicByScope m_nba;  // Logic replicated into the 'nba' region
    LogicByScope m_obs;  // Logic replicated into the 'obs' region
    LogicByScope m_react;  // Logic replicated into the 're' region

    LogicReplicas() = అప్రమేయ;
    VL_UNCOPYABLE(LogicReplicas);
    LogicReplicas(LogicReplicas&&) = అప్రమేయ;
    LogicReplicas& క్రియాకారకం=(LogicReplicas&&) = అప్రమేయ;
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
బహిరంగ:
    // Triggers are storead as an UnpackedArray with a fixed word size
    స్థిర constexpr uint32_t మాట_కొలత_LOG2 = 6;  // 64-bits / VL_QUADSIZE
    స్థిర constexpr uint32_t మాట_కొలత = 1 << మాట_కొలత_LOG2;

అంతరంగ:
    స్థిరరాశి std::string m_name;  // TriggerKit name
    స్థిరరాశి bool m_slow;  // TriggerKit is for schedulign 'slow' code
    స్థిరరాశి uint32_t m_n_Sense_పదాలు;  // Number of words for Sense triggers
    స్థిరరాశి uint32_t m_n_Extra_పదాలు;  // Number of words for Extra triggers
    స్థిరరాశి uint32_t m_n_Pre_పదాలు;  // Number of words for 'pre' part
    స్థిరరాశి uint32_t m_n_Vec_పదాలు = m_n_Sense_పదాలు + m_n_Extra_పదాలు;  // Number of words in 'vec' part

    // SenItems to corresponding bit indexes
    std::unordered_map<VNRef<స్థిరరాశి Ast_Sen_అంశం>, కొలత_t> m_senItem2TrigIdx;
    // Bit index to corresponding SenItem, nullptr for padding and Extra triggers
    std::సదిశ<స్థిరరాశి Ast_Sen_అంశం*> m_trig_Idx2_Sen_అంశం;
    // Data type of a single trigger word
    Ast_కణుపు_DType* m_మాట_DTypep = nullptr;
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
    mutable AstCFunc* m_any_సమితి_Vecp = nullptr;
    mutable AstCFunc* m_any_సమితి_Extp = nullptr;
    // The AstCFunc setting bits in a trigger vector that are set in another - create lazily
    mutable std::array<AstCFunc*, 4> m_orIntoVecps = {nullptr};
    // The AstCFunc setting a trigger vector to all zeroes - create lazily
    mutable AstCFunc* m_clearp = nullptr;

    // Methods to lazy construct functions processing trigger vectors
    AstCFunc* createDumpExtFunc() స్థిరరాశి;
    AstCFunc* create_Any_సమితి_Func(AstUnpackArrayDType* స్థిరరాశి dtypep) స్థిరరాశి;
    AstCFunc* createClearFunc() స్థిరరాశి;
    AstCFunc* createOrIntoFunc(AstUnpackArrayDType* స్థిరరాశి oDtypep,
                               AstUnpackArrayDType* స్థిరరాశి iDtypep) స్థిరరాశి;

    TriggerKit(స్థిరరాశి std::string& name, bool slow, uint32_t n_Sense_పదాలు, uint32_t n_Extra_పదాలు,
               uint32_t n_Pre_పదాలు,
               std::unordered_map<VNRef<స్థిరరాశి Ast_Sen_అంశం>, కొలత_t> senItem2TrigIdx,
               స్థిరరాశి std::సదిశ<స్థిరరాశి Ast_Sen_అంశం*>& senItemps, bool useAcc);
    VL_UNCOPYABLE(TriggerKit);

బహిరంగ:
    // Move constructible
    TriggerKit(TriggerKit&&) = అప్రమేయ;
    ~TriggerKit() = అప్రమేయ;
    TriggerKit& క్రియాకారకం=(TriggerKit&&) = delete;

    // Utility for extra trigger allocation
    class ExtraTriggers final {
        మిత్ర class TriggerKit;
        std::సదిశ<string> m_descriptions;  // Human readable description of extra triggers

    బహిరంగ:
        ExtraTriggers() = అప్రమేయ;
        ~ExtraTriggers() = అప్రమేయ;

        uint32_t allocate(స్థిరరాశి string& description) {
            m_descriptions.నెట్టు_వెనుక(description);
            తిరిగి m_descriptions.కొలత() - 1;
        }
        uint32_t కొలత() స్థిరరాశి { తిరిగి m_descriptions.కొలత(); }
    };
    // Generates list of assignments that fills
    స్థిర Ast_కేటాయించు* createSenTrigVecAssignment(AstVarScope* స్థిరరాశి లక్ష్యం,
                                                 std::సదిశ<Ast_కణుపు_సమాసం*>& trigps);

    // Create a TriggerKit for the given AstSenTree vector
    స్థిర TriggerKit create(AstNetlist* netlistp,  //
                             AstCFunc* స్థిరరాశి initFuncp,  //
                             Sen_సమాసం_Builder& sen_సమాసం_Builder,  //
                             స్థిరరాశి std::సదిశ<స్థిరరాశి Ast_Sen_వృక్షం*>& preTreeps,  //
                             స్థిరరాశి std::సదిశ<స్థిరరాశి Ast_Sen_వృక్షం*>& senTreeps,  //
                             స్థిరరాశి string& name,  //
                             స్థిరరాశి ExtraTriggers& extraTriggers,  //
                             bool slow,  //
                             bool useAcc);

    // ACCESSORS
    AstVarScope* vscp() స్థిరరాశి { తిరిగి m_vscp; }
    AstVarScope* vscAccp() స్థిరరాశి { తిరిగి m_vscAccp; }
    కొలత_t senItem2TrigIdx(స్థిరరాశి Ast_Sen_అంశం* senItemp) స్థిరరాశి {
        తిరిగి m_senItem2TrigIdx.at(*senItemp);
    }
    స్థిరరాశి std::సదిశ<స్థిరరాశి Ast_Sen_అంశం*>& trig_Idx2_Sen_అంశం() స్థిరరాశి { తిరిగి m_trig_Idx2_Sen_అంశం; }
    AstCFunc* compBasep() స్థిరరాశి { తిరిగి m_compVecp; }
    స్థిరరాశి std::string& name() స్థిరరాశి { తిరిగి m_name; }

    // Helpers for code generation - lazy construct relevant functions
    Ast_కణుపు_Stmt* newAndNotCall(AstVarScope* op, AstVarScope* ap, AstVarScope* bp) స్థిరరాశి;
    Ast_కణుపు_సమాసం* new_Any_సమితి_Call(AstVarScope* vscp) స్థిరరాశి;
    Ast_కణుపు_Stmt* newClearCall(AstVarScope* vscp) స్థిరరాశి;
    Ast_కణుపు_Stmt* newOrIntoCall(AstVarScope* op, AstVarScope* ip) స్థిరరాశి;
    // Helpers for code generation
    Ast_కణుపు_Stmt* new_Comp_పునాది_Call() స్థిరరాశి;
    Ast_కణుపు_Stmt* newCompExtCall(AstVarScope* vscp) స్థిరరాశి;
    Ast_కణుపు_Stmt* newDumpCall(AstVarScope* vscp, స్థిరరాశి std::string& ట్యాగ్, bool debugOnly) స్థిరరాశి;
    // Create a new (non-extended) trigger vector - might return nullptr if there are no triggers
    AstVarScope* newTrigVec(స్థిరరాశి std::string& name) స్థిరరాశి;

    // Index in the trigger vector of the given Extra trigger
    uint32_t extra_Trigger_సూచీ(uint32_t సూచీ) స్థిరరాశి;

    // Indices in the trigger vector of the items of the given SenTree, either of the 'pre' copy,
    // or of the regular triggers
    std::సమితి<uint32_t> triggerIndices(స్థిరరాశి Ast_Sen_వృక్షం* senTreep, bool pre) స్థిరరాశి;

    // Statement setting the extra trigger bit at 'index' to the value of 'vscp'
    Ast_కణుపు_Stmt* newExtraTriggerAssignment(AstVarScope* vscp, uint32_t సూచీ) స్థిరరాశి;

    // Set the extra trigger bit at 'index' to the value of 'vscp', then set 'vscp' to 0.
    // Prepended to the trigger computation function.
    శూన్య కూడు_Extra_Trigger_Assignment(AstVarScope* vscp, uint32_t సూచీ) స్థిరరాశి;

    // Set trigger bit at 'index' when vscp's value changes from previous evaluation.
    // Creates a prev variable: trigger[bit] = (vscp != prev); prev = vscp
    శూన్య కూడు_విలువ_Change_Trigger_Assignment(AstNetlist* netlistp, AstCFunc* initFuncp,
                                         AstVarScope* vscp, uint32_t సూచీ) స్థిరరాశి;
};

// Everything needed for combining timing with static scheduling.
class TimingKit final {
    AstCFunc* m_resumeFuncp = nullptr;  // Global timing resume function
    AstCFunc* m_readyFuncp = nullptr;  // Global timing ready function

    // Additional var sensitivities for V3Order
    std::map<స్థిరరాశి AstVarScope*, std::సమితి<Ast_Sen_వృక్షం*>> m_externalDomains;

బహిరంగ:
    LogicByScope m_lbs;  // Actives that resume timing schedulers
    Ast_కణుపు_Stmt* m_postUpdates = nullptr;  // Post updates for the trigger eval function

    // Trigger indices of the external domains of each variable
    std::map<స్థిరరాశి AstVarScope*, std::సదిశ<uint32_t>>
    externalTriggerIndices(స్థిరరాశి TriggerKit& trigKit) స్థిరరాశి VL_MT_DISABLED;
    // Get the delay scheduler variable
    AstVarScope* getDelayScheduler(AstNetlist* స్థిరరాశి netlistp) VL_MT_DISABLED;
    // Creates a timing resume call (if needed, else returns null)
    AstCCall* createResume(AstVarScope* trigVscp) VL_MT_DISABLED;
    // Creates a timing ready call (if needed, else returns null)
    AstCCall* createReady(AstVarScope* trigVscp) VL_MT_DISABLED;

    TimingKit() = అప్రమేయ;
    TimingKit(LogicByScope&& lbs, Ast_కణుపు_Stmt* postUpdates,
              std::map<స్థిరరాశి AstVarScope*, std::సమితి<Ast_Sen_వృక్షం*>>&& externalDomains)
        : m_externalDomains{externalDomains}
        , m_lbs{lbs}
        , m_postUpdates{postUpdates} {}
    VL_UNCOPYABLE(TimingKit);
    TimingKit(TimingKit&&) = అప్రమేయ;
    TimingKit& క్రియాకారకం=(TimingKit&&) = అప్రమేయ;
};

class VirtIfaceTriggers final {
బహిరంగ:
    // One entry per VarScope of an interface member that may be written through a virtual
    // interface. Each gets its own extra trigger bit with value-change detection.
    నిర్మాణం Trigger_నమోదు final {
        స్థిరరాశి AstIface* m_ifacep;  // Interface type
        స్థిరరాశి AstVar* m_memberp;  // Member variable in the interface definition
        AstVarScope* m_vscp;  // VarScope to monitor
    };

    std::సదిశ<Trigger_నమోదు> m_triggers;

    శూన్య కూడు_Trigger(స్థిరరాశి AstIface* ifacep, స్థిరరాశి AstVar* memberp, AstVarScope* vscp) {
        m_triggers.నెట్టు_వెనుక(Trigger_నమోదు{ifacep, memberp, vscp});
    }

    // Maps each triggered VarScope to its trigger index. addTriggeredIface() uses this
    // to look up triggers -- by specific VarScope for non-virtual reads, or by iterating
    // all entries of the same interface type for virtual interface reads.
    using VscpTrigMap = std::map<స్థిరరాశి AstVarScope*, uint32_t>;
    VscpTrigMap make_Vscp_గమ్యం_Trig_Map(స్థిరరాశి TriggerKit& trigKit, uint32_t మొదటి_సూచీ) స్థిరరాశి;

    VL_UNCOPYABLE(VirtIfaceTriggers);
    VirtIfaceTriggers() = అప్రమేయ;
    VirtIfaceTriggers(VirtIfaceTriggers&&) = అప్రమేయ;
    VirtIfaceTriggers& క్రియాకారకం=(VirtIfaceTriggers&&) = అప్రమేయ;
};

// Design variables bound to covergroup 'ref'/'const ref' formal arguments at construction.
//
// Such a formal becomes a pointer member, bound when the covergroup is constructed, so a
// sample() reading through it holds no AstVarRef naming the design variable. V3Order has to
// model those reads, but by the time it runs the constructions sit in logic it does not order,
// so they are collected before ordering and handed to it.
class CovergroupRefBindings final {
బహిరంగ:
    // The design variables one covergroup sample() may read through its reference formals
    using Bindings = std::సదిశ<AstVarScope*>;

అంతరంగ:
    // Bindings of one covergroup handle. Exact, and so only present for a handle whose every
    // write is a construction we recognized -- see dropInstance().
    std::unordered_map<స్థిరరాశి AstVarScope*, Bindings> m_byInstance;
    // Bindings over every construction of a covergroup class, used where the handle a sample()
    // call reaches is not known. An over-approximation, which is safe.
    std::unordered_map<స్థిరరాశి AstClass*, Bindings> m_byClass;
    // How the sample() calls seen so far resolved. Statistics only, hence mutable: ordering
    // runs once per scheduling region and must hold this by const reference.
    mutable uint32_t m_అంకె_Exact_Calls = 0;
    mutable uint32_t m_అంకె_Union_Calls = 0;
    // Returned for a covergroup with no reference formal at all.
    స్థిర స్థిరరాశి Bindings s_none;

బహిరంగ:
    // Record one construction of covergroup 'classp' binding reference formals to 'bindings'.
    // 'instp' is the handle assigned, or nullptr if the destination could not be identified.
    శూన్య కూడు_Construction(స్థిరరాశి AstClass* classp, స్థిరరాశి AstVarScope* instp,
                         స్థిరరాశి Bindings& bindings);
    // Forget the exact bindings of a handle, which turned out to be written by something other
    // than a construction we recognized. Its sample() calls fall back to the class union.
    శూన్య dropInstance(స్థిరరాశి AstVarScope* instp) { m_byInstance.erase(instp); }
    // What a sample() of covergroup 'classp' may read through reference formals, when called on
    // handle 'instp' (nullptr if the call is not on a plain handle reference)
    స్థిరరాశి Bindings& forSample(స్థిరరాశి AstVarScope* instp, స్థిరరాశి AstClass* classp) స్థిరరాశి;
    // Sample calls of a covergroup with a reference formal resolved to one handle's bindings
    uint32_t అంకె_Exact_Calls() స్థిరరాశి { తిరిగి m_అంకె_Exact_Calls; }
    // ... and those that had to take the class union instead
    uint32_t అంకె_Union_Calls() స్థిరరాశి { తిరిగి m_అంకె_Union_Calls; }

    VL_UNCOPYABLE(CovergroupRefBindings);
    CovergroupRefBindings() = అప్రమేయ;
    CovergroupRefBindings(CovergroupRefBindings&&) = అప్రమేయ;
    CovergroupRefBindings& క్రియాకారకం=(CovergroupRefBindings&&) = అప్రమేయ;
};

// Creates trigger vars for signals driven via virtual interfaces
VirtIfaceTriggers makeVirtIfaceTriggers(AstNetlist* nodep) VL_MT_DISABLED;

// Collects what covergroup reference formal arguments are bound to at construction
స్థిరరాశి CovergroupRefBindings makeCovergroupRefBindings(AstNetlist* nodep) VL_MT_DISABLED;

// Creates the timing kit and marks variables written by suspendables
TimingKit prepareTiming(AstNetlist* స్థిరరాశి netlistp) VL_MT_DISABLED;

// Transforms fork sub-statements into separate functions
శూన్య transformForks(AstNetlist* స్థిరరాశి netlistp) VL_MT_DISABLED;

// Top level entry point to scheduling
శూన్య కాలపట్టిక(AstNetlist*) VL_MT_DISABLED;

// Sub-steps
LogicByScope break_చక్రాలు(AstNetlist* netlistp,
                         స్థిరరాశి LogicByScope& combinationalLogic) VL_MT_DISABLED;
LogicRegions విభజన(LogicByScope& clockedLogic, LogicByScope& combinationalLogic,
                       LogicByScope& hybridLogic) VL_MT_DISABLED;
LogicReplicas replicateLogic(LogicRegions&) VL_MT_DISABLED;

// Utility functions used by various steps in scheduling
namespace util {
// Create a new sub function (not an entry point)
AstCFunc* makeSubFunction(AstNetlist* netlistp, స్థిరరాశి string& name, bool slow);
// Add an argument of the given type to the given function
AstVarScope* new_వాదం(AstCFunc* funcp, Ast_కణుపు_DType* dtypep, స్థిరరాశి string& name,
                         VDirection direction);
// Create statement that sets the given 'vscp' to 'val'
Ast_కణుపు_Stmt* సమితి_Var(AstVarScope* vscp, uint32_t val);
// Create statement that increments the given 'vscp' by one
Ast_కణుపు_Stmt* incrementVar(AstVarScope* vscp);
// Create statement that calls the given 'void' returning function
Ast_కణుపు_Stmt* callVoidFunc(AstCFunc* funcp);
// Split large function according to --output-split-cfuncs
శూన్య విభజించు_Check(AstCFunc* ofuncp);
}  // namespace util

శూన్య beforeTrigVisitor(AstNetlist* netlistp, Sen_సమాసం_Builder& sen_సమాసం_Builder,
                       స్థిరరాశి TriggerKit& trigKit);

}  // namespace V3Sched

#అయితే_ముగింపు  // Guard
