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

#ifndef VERILATOR_V3SCHED_H_
#define VERILATOR_V3SCHED_H_

#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3Ast.h"

#ଅନ୍ତର୍ଭାଵ <array>
#ଅନ୍ତର୍ଭାଵ <functional>
#ଅନ୍ତର୍ଭାଵ <unordered_map>
#ଅନ୍ତର୍ଭାଵ <unordered_set>
#ଅନ୍ତର୍ଭାଵ <utility>
#ଅନ୍ତର୍ଭାଵ <vector>

class Sen_ପଦସମଷ୍ଟି_Builder;

//============================================================================

namespace V3Sched {

namespace util {
using VarScopeSet = std::unordered_set<ଅଚଳ AstVarScope*>;

ଧାଡ଼ିଭିତରେ bool isVlForceVec(ଅଚଳ AstVarScope* vscp) {
    ଅଚଳ AstCDType* ଅଚଳ dtypep = VN_CAST(vscp->dtypep()->skipRefp(), CDType);
    ଫେରନ୍ତୁ dtypep && dtypep->name() == "VlForceVec";
}

ଧାଡ଼ିଭିତରେ bool isForceReadMethod(VCMethod method) {
    ଫେରନ୍ତୁ method == VCMethod::FORCE_READ || method == VCMethod::FORCE_READ_INDEX;
}

ଧାଡ଼ିଭିତରେ ଶୂନ୍ୟ collectForceReadEdgeIgnores(Ast_ଗଣ୍ଠି* nodep, VarScopeSet& out) {
    VarScopeSet writtenForceVecs;
    VarScopeSet writtenVars;

    nodep->foreach([&](ଅଚଳ AstVarRef* refp) {
        AstVarScope* ଅଚଳ vscp = refp->varScopep();
        ଯଦି (!refp->access().isWriteOrRW()) ଫେରନ୍ତୁ;
        ଯଦି (!refp->varp()->ignoreSchedWrite()) writtenVars.emplace(vscp);
        ଯଦି (!isVlForceVec(vscp)) ଫେରନ୍ତୁ;
        writtenForceVecs.emplace(vscp);
        ଯଦି (!refp->varp()->ignoreSchedWrite()) out.emplace(vscp);
    });

    ଯଦି (writtenForceVecs.empty() || writtenVars.empty()) ଫେରନ୍ତୁ;

    nodep->foreach([&](ଅଚଳ AstCMethodHard* callp) {
        ଯଦି (!isForceReadMethod(callp->method())) ଫେରନ୍ତୁ;

        ଅଚଳ AstVarRef* ଅଚଳ fromRefp = VN_CAST(callp->fromp(), VarRef);
        ଯଦି (!fromRefp || !writtenForceVecs.ଗଣନା_2(fromRefp->varScopep())) ଫେରନ୍ତୁ;

        Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* ଅଚଳ origp = callp->pinsp();
        ଯଦି (!origp) ଫେରନ୍ତୁ;
        origp->foreach([&](ଅଚଳ AstVarRef* refp) {
            AstVarScope* ଅଚଳ vscp = refp->varScopep();
            ଯଦି (refp->access().isReadOrRW() && writtenVars.ଗଣନା_2(vscp)) out.emplace(vscp);
        });
    });
}
}  // namespace util

//============================================================================
// Throughout scheduling, we need to keep hold of AstActive nodes, together
// with the AstScope that they are under. LogicByScope is simply a list of
// such pairs, with some additional convenience methods.

class LogicByScope final {
ସର୍ବସାଧାରଣ:
    // TYPES
    // An AstActive and the AstScope it is under
    ସଂରଚନା Entry final {
        AstScope* m_scopep;  // The scope the AstActive is under
        AstActive* m_activep;  // The AstActive holding the logic
        std::set<uint32_t> m_triggers;  // Trigger indices m_activep is sensitive to
    };
    using iterator = std::vector<Entry>::iterator;
    using const_iterator = std::vector<Entry>::const_iterator;

ବ୍ୟକ୍ତିଗତ:
    // STATE
    std::vector<Entry> m_entries;  // The AstActives held, with their scopes

ସର୍ବସାଧାରଣ:
    // METHODS

    // Iteration and access
    iterator begin() { ଫେରନ୍ତୁ m_entries.begin(); }
    iterator end() { ଫେରନ୍ତୁ m_entries.end(); }
    const_iterator begin() ଅଚଳ { ଫେରନ୍ତୁ m_entries.begin(); }
    const_iterator end() ଅଚଳ { ଫେରନ୍ତୁ m_entries.end(); }
    bool empty() ଅଚଳ { ଫେରନ୍ତୁ m_entries.empty(); }
    size_t size() ଅଚଳ { ଫେରନ୍ତୁ m_entries.size(); }
    ଅଚଳ Entry& operator[](size_t i) ଅଚଳ { ଫେରନ୍ତୁ m_entries[i]; }

    // Add an AstActive under the given scope
    ଶୂନ୍ୟ add(AstScope* scopep, AstActive* activep) { m_entries.ଠେଲ_back({scopep, activep, {}}); }

    // Add logic
    ଶୂନ୍ୟ add(AstScope* scopep, Ast_Sen_ବୃକ୍ଷ* senTreep, Ast_ଗଣ୍ଠି* logicp) {
        UASSERT_OBJ(!logicp->backp(), logicp, "Already linked");
        ଯଦି (m_entries.empty() || m_entries.back().m_scopep != scopep
            || m_entries.back().m_activep->sentreep() != senTreep) {
            add(scopep, new AstActive{logicp->fileline(), "", senTreep});
        }
        m_entries.back().m_activep->addStmtsp(logicp);
    };

    // Create copy, with the AstActives cloned
    LogicByScope clone() ଅଚଳ {
        LogicByScope result;
        ପାଇଁ (ଅଚଳ Entry& entry : m_entries) {
            result.add(entry.m_scopep, entry.m_activep->clone_ବୃକ୍ଷ(false));
        }
        ଫେରନ୍ତୁ result;
    }

    // Delete actives (they should all be empty)
    ଶୂନ୍ୟ deleteActives() {
        ପାଇଁ (ଅଚଳ Entry& entry : m_entries) {
            AstActive* ଅଚଳ activep = entry.m_activep;
            ଯଦି (v3Global.opt.debugCheck()) {
                ପାଇଁ (Ast_ଗଣ୍ଠି* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) {
                    Ast_ଗଣ୍ଠି_Procedure* ଅଚଳ procp = VN_CAST(nodep, ଗଣ୍ଠି_Procedure);
                    UASSERT_OBJ(procp && !procp->stmtsp(), activep, "Leftover logic");
                }
            }
            ଯଦି (activep->backp()) activep->unlinkFrBack();
            VL_DO_DANGLING(activep->delete_ବୃକ୍ଷ(), activep);
        }
        m_entries.clear();
    };

    ଶୂନ୍ୟ foreachLogic(ଅଚଳ std::function<ଶୂନ୍ୟ(Ast_ଗଣ୍ଠି*)>& f) ଅଚଳ {
        ପାଇଁ (ଅଚଳ Entry& entry : m_entries) {
            ଅଚଳ AstActive* ଅଚଳ activep = entry.m_activep;
            ପାଇଁ (Ast_ଗଣ୍ଠି* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) f(nodep);
        }
    }
};

// Logic in the design is classified based on what can trigger its execution.
// For details see the internals documentation.
ସଂରଚନା LogicClasses final {
    LogicByScope m_static;  // Static variable initializers
    LogicByScope m_initial;  // initial blocks
    LogicByScope m_final;  // final blocks
    LogicByScope m_comb;  // Combinational logic (logic with implicit sensitivities)
    LogicByScope m_clocked;  // Clocked (or sequential) logic (logic with explicit sensitivities)
    LogicByScope m_hybrid;  // Hybrid logic (combinational logic with some explicit sensitivities)
    LogicByScope m_postponed;  // Postponed logic ($strobe)
    LogicByScope m_observed;  // Observed logic (contains AstAlwaysObserved)
    LogicByScope m_reactive;  // Reactive logic (contains AstAlwaysReactive)

    LogicClasses() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    VL_UNCOPYABLE(LogicClasses);
    LogicClasses(LogicClasses&&) = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    LogicClasses& operator=(LogicClasses&&) = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

// Combinational (including hybrid) logic, and clocked logic in partitioned to compute all clock
// signals in the 'act' region. For details see the internals documentation.
ସଂରଚନା LogicRegions final {
    LogicByScope m_pre;  // AstAlwaysPre logic in 'act' region
    LogicByScope m_act;  // 'act' region logic
    LogicByScope m_nba;  // 'nba' region logic
    LogicByScope m_obs;  // AstAlwaysObserved logic in 'obs' region
    LogicByScope m_react;  // AstAlwaysReactive logic in 're' region

    LogicRegions() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    VL_UNCOPYABLE(LogicRegions);
    LogicRegions(LogicRegions&&) = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    LogicRegions& operator=(LogicRegions&&) = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

// Combinational (including hybrid) logic is replicated into the various scheduling regions.
// For details see the internals documentation.
ସଂରଚନା LogicReplicas final {
    LogicByScope m_ico;  // Logic replicated into the 'ico' (Input Combinational) region
    LogicByScope m_act;  // Logic replicated into the 'act' region
    LogicByScope m_nba;  // Logic replicated into the 'nba' region
    LogicByScope m_obs;  // Logic replicated into the 'obs' region
    LogicByScope m_react;  // Logic replicated into the 're' region

    LogicReplicas() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    VL_UNCOPYABLE(LogicReplicas);
    LogicReplicas(LogicReplicas&&) = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    LogicReplicas& operator=(LogicReplicas&&) = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
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
ସର୍ବସାଧାରଣ:
    // Triggers are storead as an UnpackedArray with a fixed word size
    ସ୍ଥିର constexpr uint32_t WORD_SIZE_LOG2 = 6;  // 64-bits / VL_QUADSIZE
    ସ୍ଥିର constexpr uint32_t WORD_SIZE = 1 << WORD_SIZE_LOG2;

ବ୍ୟକ୍ତିଗତ:
    ଅଚଳ std::string m_name;  // TriggerKit name
    ଅଚଳ bool m_slow;  // TriggerKit is for schedulign 'slow' code
    ଅଚଳ uint32_t m_n_Sense_ଶବ୍ଦଗୁଡ଼ିକ;  // Number of words for Sense triggers
    ଅଚଳ uint32_t m_n_Extra_ଶବ୍ଦଗୁଡ଼ିକ;  // Number of words for Extra triggers
    ଅଚଳ uint32_t m_n_Pre_ଶବ୍ଦଗୁଡ଼ିକ;  // Number of words for 'pre' part
    ଅଚଳ uint32_t m_n_Vec_ଶବ୍ଦଗୁଡ଼ିକ = m_n_Sense_ଶବ୍ଦଗୁଡ଼ିକ + m_n_Extra_ଶବ୍ଦଗୁଡ଼ିକ;  // Number of words in 'vec' part

    // SenItems to corresponding bit indexes
    std::unordered_map<VNRef<ଅଚଳ AstSenItem>, size_t> m_senItem2TrigIdx;
    // Bit index to corresponding SenItem, nullptr for padding and Extra triggers
    std::vector<ଅଚଳ AstSenItem*> m_trigIdx2SenItem;
    // Data type of a single trigger word
    Ast_ଗଣ୍ଠି_DType* m_wordDTypep = nullptr;
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
    mutable AstCFunc* m_anySetVecp = nullptr;
    mutable AstCFunc* m_anySetExtp = nullptr;
    // The AstCFunc setting bits in a trigger vector that are set in another - create lazily
    mutable std::array<AstCFunc*, 4> m_orIntoVecps = {nullptr};
    // The AstCFunc setting a trigger vector to all zeroes - create lazily
    mutable AstCFunc* m_clearp = nullptr;

    // Methods to lazy construct functions processing trigger vectors
    AstCFunc* createDumpExtFunc() ଅଚଳ;
    AstCFunc* createAnySetFunc(AstUnpackArrayDType* ଅଚଳ dtypep) ଅଚଳ;
    AstCFunc* createClearFunc() ଅଚଳ;
    AstCFunc* createOrIntoFunc(AstUnpackArrayDType* ଅଚଳ oDtypep,
                               AstUnpackArrayDType* ଅଚଳ iDtypep) ଅଚଳ;

    TriggerKit(ଅଚଳ std::string& name, bool slow, uint32_t n_Sense_ଶବ୍ଦଗୁଡ଼ିକ, uint32_t n_Extra_ଶବ୍ଦଗୁଡ଼ିକ,
               uint32_t n_Pre_ଶବ୍ଦଗୁଡ଼ିକ,
               std::unordered_map<VNRef<ଅଚଳ AstSenItem>, size_t> senItem2TrigIdx,
               ଅଚଳ std::vector<ଅଚଳ AstSenItem*>& senItemps, bool useAcc);
    VL_UNCOPYABLE(TriggerKit);

ସର୍ବସାଧାରଣ:
    // Move constructible
    TriggerKit(TriggerKit&&) = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ~TriggerKit() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    TriggerKit& operator=(TriggerKit&&) = delete;

    // Utility for extra trigger allocation
    class ExtraTriggers final {
        friend class TriggerKit;
        std::vector<string> m_descriptions;  // Human readable description of extra triggers

    ସର୍ବସାଧାରଣ:
        ExtraTriggers() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
        ~ExtraTriggers() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

        uint32_t allocate(ଅଚଳ string& description) {
            m_descriptions.ଠେଲ_back(description);
            ଫେରନ୍ତୁ m_descriptions.size() - 1;
        }
        uint32_t size() ଅଚଳ { ଫେରନ୍ତୁ m_descriptions.size(); }
    };
    // Generates list of assignments that fills
    ସ୍ଥିର AstAssign* createSenTrigVecAssignment(AstVarScope* ଅଚଳ target,
                                                 std::vector<Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି*>& trigps);

    // Create a TriggerKit for the given AstSenTree vector
    ସ୍ଥିର TriggerKit create(AstNetlist* netlistp,  //
                             AstCFunc* ଅଚଳ initFuncp,  //
                             Sen_ପଦସମଷ୍ଟି_Builder& sen_ପଦସମଷ୍ଟି_Builder,  //
                             ଅଚଳ std::vector<ଅଚଳ Ast_Sen_ବୃକ୍ଷ*>& preTreeps,  //
                             ଅଚଳ std::vector<ଅଚଳ Ast_Sen_ବୃକ୍ଷ*>& senTreeps,  //
                             ଅଚଳ string& name,  //
                             ଅଚଳ ExtraTriggers& extraTriggers,  //
                             bool slow,  //
                             bool useAcc);

    // ACCESSORS
    AstVarScope* vscp() ଅଚଳ { ଫେରନ୍ତୁ m_vscp; }
    AstVarScope* vscAccp() ଅଚଳ { ଫେରନ୍ତୁ m_vscAccp; }
    size_t senItem2TrigIdx(ଅଚଳ AstSenItem* senItemp) ଅଚଳ {
        ଫେରନ୍ତୁ m_senItem2TrigIdx.at(*senItemp);
    }
    ଅଚଳ std::vector<ଅଚଳ AstSenItem*>& trigIdx2SenItem() ଅଚଳ { ଫେରନ୍ତୁ m_trigIdx2SenItem; }
    AstCFunc* compBasep() ଅଚଳ { ଫେରନ୍ତୁ m_compVecp; }
    ଅଚଳ std::string& name() ଅଚଳ { ଫେରନ୍ତୁ m_name; }

    // Helpers for code generation - lazy construct relevant functions
    Ast_ଗଣ୍ଠି_Stmt* newAndNotCall(AstVarScope* op, AstVarScope* ap, AstVarScope* bp) ଅଚଳ;
    Ast_ଗଣ୍ଠି_ପଦସମଷ୍ଟି* newAnySetCall(AstVarScope* vscp) ଅଚଳ;
    Ast_ଗଣ୍ଠି_Stmt* newClearCall(AstVarScope* vscp) ଅଚଳ;
    Ast_ଗଣ୍ଠି_Stmt* newOrIntoCall(AstVarScope* op, AstVarScope* ip) ଅଚଳ;
    // Helpers for code generation
    Ast_ଗଣ୍ଠି_Stmt* newCompBaseCall() ଅଚଳ;
    Ast_ଗଣ୍ଠି_Stmt* newCompExtCall(AstVarScope* vscp) ଅଚଳ;
    Ast_ଗଣ୍ଠି_Stmt* newDumpCall(AstVarScope* vscp, ଅଚଳ std::string& tag, bool debugOnly) ଅଚଳ;
    // Create a new (non-extended) trigger vector - might return nullptr if there are no triggers
    AstVarScope* newTrigVec(ଅଚଳ std::string& name) ଅଚଳ;

    // Index in the trigger vector of the given Extra trigger
    uint32_t extraTriggerIndex(uint32_t index) ଅଚଳ;

    // Indices in the trigger vector of the items of the given SenTree, either of the 'pre' copy,
    // or of the regular triggers
    std::set<uint32_t> triggerIndices(ଅଚଳ Ast_Sen_ବୃକ୍ଷ* senTreep, bool pre) ଅଚଳ;

    // Statement setting the extra trigger bit at 'index' to the value of 'vscp'
    Ast_ଗଣ୍ଠି_Stmt* newExtraTriggerAssignment(AstVarScope* vscp, uint32_t index) ଅଚଳ;

    // Set the extra trigger bit at 'index' to the value of 'vscp', then set 'vscp' to 0.
    // Prepended to the trigger computation function.
    ଶୂନ୍ୟ addExtraTriggerAssignment(AstVarScope* vscp, uint32_t index) ଅଚଳ;

    // Set trigger bit at 'index' when vscp's value changes from previous evaluation.
    // Creates a prev variable: trigger[bit] = (vscp != prev); prev = vscp
    ଶୂନ୍ୟ add_ମୂଲ୍ୟ_Change_Trigger_Assignment(AstNetlist* netlistp, AstCFunc* initFuncp,
                                         AstVarScope* vscp, uint32_t index) ଅଚଳ;
};

// Everything needed for combining timing with static scheduling.
class TimingKit final {
    AstCFunc* m_resumeFuncp = nullptr;  // Global timing resume function
    AstCFunc* m_readyFuncp = nullptr;  // Global timing ready function

    // Additional var sensitivities for V3Order
    std::map<ଅଚଳ AstVarScope*, std::set<Ast_Sen_ବୃକ୍ଷ*>> m_externalDomains;

ସର୍ବସାଧାରଣ:
    LogicByScope m_lbs;  // Actives that resume timing schedulers
    Ast_ଗଣ୍ଠି_Stmt* m_postUpdates = nullptr;  // Post updates for the trigger eval function

    // Trigger indices of the external domains of each variable
    std::map<ଅଚଳ AstVarScope*, std::vector<uint32_t>>
    externalTriggerIndices(ଅଚଳ TriggerKit& trigKit) ଅଚଳ VL_MT_DISABLED;
    // Get the delay scheduler variable
    AstVarScope* getDelayScheduler(AstNetlist* ଅଚଳ netlistp) VL_MT_DISABLED;
    // Creates a timing resume call (if needed, else returns null)
    AstCCall* createResume(AstVarScope* trigVscp) VL_MT_DISABLED;
    // Creates a timing ready call (if needed, else returns null)
    AstCCall* createReady(AstVarScope* trigVscp) VL_MT_DISABLED;

    TimingKit() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    TimingKit(LogicByScope&& lbs, Ast_ଗଣ୍ଠି_Stmt* postUpdates,
              std::map<ଅଚଳ AstVarScope*, std::set<Ast_Sen_ବୃକ୍ଷ*>>&& externalDomains)
        : m_externalDomains{externalDomains}
        , m_lbs{lbs}
        , m_postUpdates{postUpdates} {}
    VL_UNCOPYABLE(TimingKit);
    TimingKit(TimingKit&&) = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    TimingKit& operator=(TimingKit&&) = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

class VirtIfaceTriggers final {
ସର୍ବସାଧାରଣ:
    // One entry per VarScope of an interface member that may be written through a virtual
    // interface. Each gets its own extra trigger bit with value-change detection.
    ସଂରଚନା TriggerEntry final {
        ଅଚଳ AstIface* m_ifacep;  // Interface type
        ଅଚଳ AstVar* m_memberp;  // Member variable in the interface definition
        AstVarScope* m_vscp;  // VarScope to monitor
    };

    std::vector<TriggerEntry> m_triggers;

    ଶୂନ୍ୟ addTrigger(ଅଚଳ AstIface* ifacep, ଅଚଳ AstVar* memberp, AstVarScope* vscp) {
        m_triggers.ଠେଲ_back(TriggerEntry{ifacep, memberp, vscp});
    }

    // Maps each triggered VarScope to its trigger index. addTriggeredIface() uses this
    // to look up triggers -- by specific VarScope for non-virtual reads, or by iterating
    // all entries of the same interface type for virtual interface reads.
    using VscpTrigMap = std::map<ଅଚଳ AstVarScope*, uint32_t>;
    VscpTrigMap makeVscpToTrigMap(ଅଚଳ TriggerKit& trigKit, uint32_t firstIndex) ଅଚଳ;

    VL_UNCOPYABLE(VirtIfaceTriggers);
    VirtIfaceTriggers() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    VirtIfaceTriggers(VirtIfaceTriggers&&) = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    VirtIfaceTriggers& operator=(VirtIfaceTriggers&&) = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

// Design variables bound to covergroup 'ref'/'const ref' formal arguments at construction.
//
// Such a formal becomes a pointer member, bound when the covergroup is constructed, so a
// sample() reading through it holds no AstVarRef naming the design variable. V3Order has to
// model those reads, but by the time it runs the constructions sit in logic it does not order,
// so they are collected before ordering and handed to it.
class CovergroupRefBindings final {
ସର୍ବସାଧାରଣ:
    // The design variables one covergroup sample() may read through its reference formals
    using Bindings = std::vector<AstVarScope*>;

ବ୍ୟକ୍ତିଗତ:
    // Bindings of one covergroup handle. Exact, and so only present for a handle whose every
    // write is a construction we recognized -- see dropInstance().
    std::unordered_map<ଅଚଳ AstVarScope*, Bindings> m_byInstance;
    // Bindings over every construction of a covergroup class, used where the handle a sample()
    // call reaches is not known. An over-approximation, which is safe.
    std::unordered_map<ଅଚଳ AstClass*, Bindings> m_byClass;
    // How the sample() calls seen so far resolved. Statistics only, hence mutable: ordering
    // runs once per scheduling region and must hold this by const reference.
    mutable uint32_t m_ଅଙ୍କ_Exact_Calls = 0;
    mutable uint32_t m_ଅଙ୍କ_Union_Calls = 0;
    // Returned for a covergroup with no reference formal at all.
    ସ୍ଥିର ଅଚଳ Bindings s_none;

ସର୍ବସାଧାରଣ:
    // Record one construction of covergroup 'classp' binding reference formals to 'bindings'.
    // 'instp' is the handle assigned, or nullptr if the destination could not be identified.
    ଶୂନ୍ୟ addConstruction(ଅଚଳ AstClass* classp, ଅଚଳ AstVarScope* instp,
                         ଅଚଳ Bindings& bindings);
    // Forget the exact bindings of a handle, which turned out to be written by something other
    // than a construction we recognized. Its sample() calls fall back to the class union.
    ଶୂନ୍ୟ dropInstance(ଅଚଳ AstVarScope* instp) { m_byInstance.erase(instp); }
    // What a sample() of covergroup 'classp' may read through reference formals, when called on
    // handle 'instp' (nullptr if the call is not on a plain handle reference)
    ଅଚଳ Bindings& forSample(ଅଚଳ AstVarScope* instp, ଅଚଳ AstClass* classp) ଅଚଳ;
    // Sample calls of a covergroup with a reference formal resolved to one handle's bindings
    uint32_t ଅଙ୍କ_Exact_Calls() ଅଚଳ { ଫେରନ୍ତୁ m_ଅଙ୍କ_Exact_Calls; }
    // ... and those that had to take the class union instead
    uint32_t ଅଙ୍କ_Union_Calls() ଅଚଳ { ଫେରନ୍ତୁ m_ଅଙ୍କ_Union_Calls; }

    VL_UNCOPYABLE(CovergroupRefBindings);
    CovergroupRefBindings() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    CovergroupRefBindings(CovergroupRefBindings&&) = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    CovergroupRefBindings& operator=(CovergroupRefBindings&&) = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

// Creates trigger vars for signals driven via virtual interfaces
VirtIfaceTriggers makeVirtIfaceTriggers(AstNetlist* nodep) VL_MT_DISABLED;

// Collects what covergroup reference formal arguments are bound to at construction
ଅଚଳ CovergroupRefBindings makeCovergroupRefBindings(AstNetlist* nodep) VL_MT_DISABLED;

// Creates the timing kit and marks variables written by suspendables
TimingKit prepareTiming(AstNetlist* ଅଚଳ netlistp) VL_MT_DISABLED;

// Transforms fork sub-statements into separate functions
ଶୂନ୍ୟ transformForks(AstNetlist* ଅଚଳ netlistp) VL_MT_DISABLED;

// Top level entry point to scheduling
ଶୂନ୍ୟ schedule(AstNetlist*) VL_MT_DISABLED;

// Sub-steps
LogicByScope breakCycles(AstNetlist* netlistp,
                         ଅଚଳ LogicByScope& combinationalLogic) VL_MT_DISABLED;
LogicRegions partition(LogicByScope& clockedLogic, LogicByScope& combinationalLogic,
                       LogicByScope& hybridLogic) VL_MT_DISABLED;
LogicReplicas replicateLogic(LogicRegions&) VL_MT_DISABLED;

// Utility functions used by various steps in scheduling
namespace util {
// Create a new sub function (not an entry point)
AstCFunc* makeSubFunction(AstNetlist* netlistp, ଅଚଳ string& name, bool slow);
// Add an argument of the given type to the given function
AstVarScope* newArgument(AstCFunc* funcp, Ast_ଗଣ୍ଠି_DType* dtypep, ଅଚଳ string& name,
                         VDirection direction);
// Create statement that sets the given 'vscp' to 'val'
Ast_ଗଣ୍ଠି_Stmt* setVar(AstVarScope* vscp, uint32_t val);
// Create statement that increments the given 'vscp' by one
Ast_ଗଣ୍ଠି_Stmt* incrementVar(AstVarScope* vscp);
// Create statement that calls the given 'void' returning function
Ast_ଗଣ୍ଠି_Stmt* callVoidFunc(AstCFunc* funcp);
// Split large function according to --output-split-cfuncs
ଶୂନ୍ୟ splitCheck(AstCFunc* ofuncp);
}  // namespace util

ଶୂନ୍ୟ beforeTrigVisitor(AstNetlist* netlistp, Sen_ପଦସମଷ୍ଟି_Builder& sen_ପଦସମଷ୍ଟି_Builder,
                       ଅଚଳ TriggerKit& trigKit);

}  // namespace V3Sched

#endif  // Guard
