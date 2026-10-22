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

#அந்தர்பாவ "config_build.h"
#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3Ast.h"

#அந்தர்பாவ <array>
#அந்தர்பாவ <functional>
#அந்தர்பாவ <unordered_map>
#அந்தர்பாவ <unordered_set>
#அந்தர்பாவ <utility>
#அந்தர்பாவ <vector>

class Sen_கோவை_Builder;

//============================================================================

namespace V3Sched {

namespace util {
using VarScopeSet = std::unordered_set<மாறிலி AstVarScope*>;

உள்வரி bool isVlForceVec(மாறிலி AstVarScope* vscp) {
    மாறிலி AstCDType* மாறிலி dtypep = VN_CAST(vscp->dtypep()->skipRefp(), CDType);
    திரும்பு dtypep && dtypep->name() == "VlForceVec";
}

உள்வரி bool isForceReadMethod(VCMethod method) {
    திரும்பு method == VCMethod::FORCE_READ || method == VCMethod::FORCE_READ_INDEX;
}

உள்வரி வெறுமை collectForceReadEdgeIgnores(Ast_முனை* nodep, VarScopeSet& out) {
    VarScopeSet writtenForceVecs;
    VarScopeSet writtenVars;

    nodep->foreach([&](மாறிலி AstVarRef* refp) {
        AstVarScope* மாறிலி vscp = refp->varScopep();
        எனில் (!refp->access().isWriteOrRW()) திரும்பு;
        எனில் (!refp->varp()->ignoreSchedWrite()) writtenVars.emplace(vscp);
        எனில் (!isVlForceVec(vscp)) திரும்பு;
        writtenForceVecs.emplace(vscp);
        எனில் (!refp->varp()->ignoreSchedWrite()) out.emplace(vscp);
    });

    எனில் (writtenForceVecs.empty() || writtenVars.empty()) திரும்பு;

    nodep->foreach([&](மாறிலி AstCMethodHard* callp) {
        எனில் (!isForceReadMethod(callp->method())) திரும்பு;

        மாறிலி AstVarRef* மாறிலி fromRefp = VN_CAST(callp->fromp(), VarRef);
        எனில் (!fromRefp || !writtenForceVecs.மொத்தம்(fromRefp->varScopep())) திரும்பு;

        Ast_முனை_கோவை* மாறிலி origp = callp->pinsp();
        எனில் (!origp) திரும்பு;
        origp->foreach([&](மாறிலி AstVarRef* refp) {
            AstVarScope* மாறிலி vscp = refp->varScopep();
            எனில் (refp->access().isReadOrRW() && writtenVars.மொத்தம்(vscp)) out.emplace(vscp);
        });
    });
}
}  // namespace util

//============================================================================
// Throughout scheduling, we need to keep hold of AstActive nodes, together
// with the AstScope that they are under. LogicByScope is simply a list of
// such pairs, with some additional convenience methods.

class LogicByScope final {
பொது:
    // TYPES
    // An AstActive and the AstScope it is under
    கட்டமைப்பு Entry final {
        AstScope* m_scopep;  // The scope the AstActive is under
        AstActive* m_activep;  // The AstActive holding the logic
        std::set<uint32_t> m_triggers;  // Trigger indices m_activep is sensitive to
    };
    using iterator = std::vector<Entry>::iterator;
    using const_iterator = std::vector<Entry>::const_iterator;

தனிப்பட்ட:
    // STATE
    std::vector<Entry> m_entries;  // The AstActives held, with their scopes

பொது:
    // METHODS

    // Iteration and access
    iterator begin() { திரும்பு m_entries.begin(); }
    iterator end() { திரும்பு m_entries.end(); }
    const_iterator begin() மாறிலி { திரும்பு m_entries.begin(); }
    const_iterator end() மாறிலி { திரும்பு m_entries.end(); }
    bool empty() மாறிலி { திரும்பு m_entries.empty(); }
    size_t size() மாறிலி { திரும்பு m_entries.size(); }
    மாறிலி Entry& operator[](size_t i) மாறிலி { திரும்பு m_entries[i]; }

    // Add an AstActive under the given scope
    வெறுமை add(AstScope* scopep, AstActive* activep) { m_entries.தள்ளு_back({scopep, activep, {}}); }

    // Add logic
    வெறுமை add(AstScope* scopep, Ast_Sen_மரம்* senTreep, Ast_முனை* logicp) {
        UASSERT_OBJ(!logicp->backp(), logicp, "Already linked");
        எனில் (m_entries.empty() || m_entries.back().m_scopep != scopep
            || m_entries.back().m_activep->sentreep() != senTreep) {
            add(scopep, new AstActive{logicp->fileline(), "", senTreep});
        }
        m_entries.back().m_activep->addStmtsp(logicp);
    };

    // Create copy, with the AstActives cloned
    LogicByScope clone() மாறிலி {
        LogicByScope result;
        க்கு (மாறிலி Entry& entry : m_entries) {
            result.add(entry.m_scopep, entry.m_activep->clone_மரம்(false));
        }
        திரும்பு result;
    }

    // Delete actives (they should all be empty)
    வெறுமை deleteActives() {
        க்கு (மாறிலி Entry& entry : m_entries) {
            AstActive* மாறிலி activep = entry.m_activep;
            எனில் (v3Global.opt.debugCheck()) {
                க்கு (Ast_முனை* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) {
                    Ast_முனை_Procedure* மாறிலி procp = VN_CAST(nodep, முனை_Procedure);
                    UASSERT_OBJ(procp && !procp->stmtsp(), activep, "Leftover logic");
                }
            }
            எனில் (activep->backp()) activep->unlinkFrBack();
            VL_DO_DANGLING(activep->delete_மரம்(), activep);
        }
        m_entries.clear();
    };

    வெறுமை foreachLogic(மாறிலி std::function<வெறுமை(Ast_முனை*)>& f) மாறிலி {
        க்கு (மாறிலி Entry& entry : m_entries) {
            மாறிலி AstActive* மாறிலி activep = entry.m_activep;
            க்கு (Ast_முனை* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) f(nodep);
        }
    }
};

// Logic in the design is classified based on what can trigger its execution.
// For details see the internals documentation.
கட்டமைப்பு LogicClasses final {
    LogicByScope m_static;  // Static variable initializers
    LogicByScope m_initial;  // initial blocks
    LogicByScope m_final;  // final blocks
    LogicByScope m_comb;  // Combinational logic (logic with implicit sensitivities)
    LogicByScope m_clocked;  // Clocked (or sequential) logic (logic with explicit sensitivities)
    LogicByScope m_hybrid;  // Hybrid logic (combinational logic with some explicit sensitivities)
    LogicByScope m_postponed;  // Postponed logic ($strobe)
    LogicByScope m_observed;  // Observed logic (contains AstAlwaysObserved)
    LogicByScope m_reactive;  // Reactive logic (contains AstAlwaysReactive)

    LogicClasses() = இயல்பு;
    VL_UNCOPYABLE(LogicClasses);
    LogicClasses(LogicClasses&&) = இயல்பு;
    LogicClasses& operator=(LogicClasses&&) = இயல்பு;
};

// Combinational (including hybrid) logic, and clocked logic in partitioned to compute all clock
// signals in the 'act' region. For details see the internals documentation.
கட்டமைப்பு LogicRegions final {
    LogicByScope m_pre;  // AstAlwaysPre logic in 'act' region
    LogicByScope m_act;  // 'act' region logic
    LogicByScope m_nba;  // 'nba' region logic
    LogicByScope m_obs;  // AstAlwaysObserved logic in 'obs' region
    LogicByScope m_react;  // AstAlwaysReactive logic in 're' region

    LogicRegions() = இயல்பு;
    VL_UNCOPYABLE(LogicRegions);
    LogicRegions(LogicRegions&&) = இயல்பு;
    LogicRegions& operator=(LogicRegions&&) = இயல்பு;
};

// Combinational (including hybrid) logic is replicated into the various scheduling regions.
// For details see the internals documentation.
கட்டமைப்பு LogicReplicas final {
    LogicByScope m_ico;  // Logic replicated into the 'ico' (Input Combinational) region
    LogicByScope m_act;  // Logic replicated into the 'act' region
    LogicByScope m_nba;  // Logic replicated into the 'nba' region
    LogicByScope m_obs;  // Logic replicated into the 'obs' region
    LogicByScope m_react;  // Logic replicated into the 're' region

    LogicReplicas() = இயல்பு;
    VL_UNCOPYABLE(LogicReplicas);
    LogicReplicas(LogicReplicas&&) = இயல்பு;
    LogicReplicas& operator=(LogicReplicas&&) = இயல்பு;
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
பொது:
    // Triggers are storead as an UnpackedArray with a fixed word size
    நிலையான constexpr uint32_t WORD_SIZE_LOG2 = 6;  // 64-bits / VL_QUADSIZE
    நிலையான constexpr uint32_t WORD_SIZE = 1 << WORD_SIZE_LOG2;

தனிப்பட்ட:
    மாறிலி std::string m_name;  // TriggerKit name
    மாறிலி bool m_slow;  // TriggerKit is for schedulign 'slow' code
    மாறிலி uint32_t m_n_Sense_சொற்கள்;  // Number of words for Sense triggers
    மாறிலி uint32_t m_n_Extra_சொற்கள்;  // Number of words for Extra triggers
    மாறிலி uint32_t m_n_Pre_சொற்கள்;  // Number of words for 'pre' part
    மாறிலி uint32_t m_n_Vec_சொற்கள் = m_n_Sense_சொற்கள் + m_n_Extra_சொற்கள்;  // Number of words in 'vec' part

    // SenItems to corresponding bit indexes
    std::unordered_map<VNRef<மாறிலி AstSenItem>, size_t> m_senItem2TrigIdx;
    // Bit index to corresponding SenItem, nullptr for padding and Extra triggers
    std::vector<மாறிலி AstSenItem*> m_trigIdx2SenItem;
    // Data type of a single trigger word
    Ast_முனை_DType* m_wordDTypep = nullptr;
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
    AstCFunc* createDumpExtFunc() மாறிலி;
    AstCFunc* createAnySetFunc(AstUnpackArrayDType* மாறிலி dtypep) மாறிலி;
    AstCFunc* createClearFunc() மாறிலி;
    AstCFunc* createOrIntoFunc(AstUnpackArrayDType* மாறிலி oDtypep,
                               AstUnpackArrayDType* மாறிலி iDtypep) மாறிலி;

    TriggerKit(மாறிலி std::string& name, bool slow, uint32_t n_Sense_சொற்கள், uint32_t n_Extra_சொற்கள்,
               uint32_t n_Pre_சொற்கள்,
               std::unordered_map<VNRef<மாறிலி AstSenItem>, size_t> senItem2TrigIdx,
               மாறிலி std::vector<மாறிலி AstSenItem*>& senItemps, bool useAcc);
    VL_UNCOPYABLE(TriggerKit);

பொது:
    // Move constructible
    TriggerKit(TriggerKit&&) = இயல்பு;
    ~TriggerKit() = இயல்பு;
    TriggerKit& operator=(TriggerKit&&) = delete;

    // Utility for extra trigger allocation
    class ExtraTriggers final {
        friend class TriggerKit;
        std::vector<string> m_descriptions;  // Human readable description of extra triggers

    பொது:
        ExtraTriggers() = இயல்பு;
        ~ExtraTriggers() = இயல்பு;

        uint32_t allocate(மாறிலி string& description) {
            m_descriptions.தள்ளு_back(description);
            திரும்பு m_descriptions.size() - 1;
        }
        uint32_t size() மாறிலி { திரும்பு m_descriptions.size(); }
    };
    // Generates list of assignments that fills
    நிலையான AstAssign* createSenTrigVecAssignment(AstVarScope* மாறிலி target,
                                                 std::vector<Ast_முனை_கோவை*>& trigps);

    // Create a TriggerKit for the given AstSenTree vector
    நிலையான TriggerKit create(AstNetlist* netlistp,  //
                             AstCFunc* மாறிலி initFuncp,  //
                             Sen_கோவை_Builder& sen_கோவை_Builder,  //
                             மாறிலி std::vector<மாறிலி Ast_Sen_மரம்*>& preTreeps,  //
                             மாறிலி std::vector<மாறிலி Ast_Sen_மரம்*>& senTreeps,  //
                             மாறிலி string& name,  //
                             மாறிலி ExtraTriggers& extraTriggers,  //
                             bool slow,  //
                             bool useAcc);

    // ACCESSORS
    AstVarScope* vscp() மாறிலி { திரும்பு m_vscp; }
    AstVarScope* vscAccp() மாறிலி { திரும்பு m_vscAccp; }
    size_t senItem2TrigIdx(மாறிலி AstSenItem* senItemp) மாறிலி {
        திரும்பு m_senItem2TrigIdx.at(*senItemp);
    }
    மாறிலி std::vector<மாறிலி AstSenItem*>& trigIdx2SenItem() மாறிலி { திரும்பு m_trigIdx2SenItem; }
    AstCFunc* compBasep() மாறிலி { திரும்பு m_compVecp; }
    மாறிலி std::string& name() மாறிலி { திரும்பு m_name; }

    // Helpers for code generation - lazy construct relevant functions
    Ast_முனை_Stmt* newAndNotCall(AstVarScope* op, AstVarScope* ap, AstVarScope* bp) மாறிலி;
    Ast_முனை_கோவை* newAnySetCall(AstVarScope* vscp) மாறிலி;
    Ast_முனை_Stmt* newClearCall(AstVarScope* vscp) மாறிலி;
    Ast_முனை_Stmt* newOrIntoCall(AstVarScope* op, AstVarScope* ip) மாறிலி;
    // Helpers for code generation
    Ast_முனை_Stmt* newCompBaseCall() மாறிலி;
    Ast_முனை_Stmt* newCompExtCall(AstVarScope* vscp) மாறிலி;
    Ast_முனை_Stmt* newDumpCall(AstVarScope* vscp, மாறிலி std::string& tag, bool debugOnly) மாறிலி;
    // Create a new (non-extended) trigger vector - might return nullptr if there are no triggers
    AstVarScope* newTrigVec(மாறிலி std::string& name) மாறிலி;

    // Index in the trigger vector of the given Extra trigger
    uint32_t extraTriggerIndex(uint32_t index) மாறிலி;

    // Indices in the trigger vector of the items of the given SenTree, either of the 'pre' copy,
    // or of the regular triggers
    std::set<uint32_t> triggerIndices(மாறிலி Ast_Sen_மரம்* senTreep, bool pre) மாறிலி;

    // Statement setting the extra trigger bit at 'index' to the value of 'vscp'
    Ast_முனை_Stmt* newExtraTriggerAssignment(AstVarScope* vscp, uint32_t index) மாறிலி;

    // Set the extra trigger bit at 'index' to the value of 'vscp', then set 'vscp' to 0.
    // Prepended to the trigger computation function.
    வெறுமை addExtraTriggerAssignment(AstVarScope* vscp, uint32_t index) மாறிலி;

    // Set trigger bit at 'index' when vscp's value changes from previous evaluation.
    // Creates a prev variable: trigger[bit] = (vscp != prev); prev = vscp
    வெறுமை add_மதிப்பு_Change_Trigger_Assignment(AstNetlist* netlistp, AstCFunc* initFuncp,
                                         AstVarScope* vscp, uint32_t index) மாறிலி;
};

// Everything needed for combining timing with static scheduling.
class TimingKit final {
    AstCFunc* m_resumeFuncp = nullptr;  // Global timing resume function
    AstCFunc* m_readyFuncp = nullptr;  // Global timing ready function

    // Additional var sensitivities for V3Order
    std::map<மாறிலி AstVarScope*, std::set<Ast_Sen_மரம்*>> m_externalDomains;

பொது:
    LogicByScope m_lbs;  // Actives that resume timing schedulers
    Ast_முனை_Stmt* m_postUpdates = nullptr;  // Post updates for the trigger eval function

    // Trigger indices of the external domains of each variable
    std::map<மாறிலி AstVarScope*, std::vector<uint32_t>>
    externalTriggerIndices(மாறிலி TriggerKit& trigKit) மாறிலி VL_MT_DISABLED;
    // Get the delay scheduler variable
    AstVarScope* getDelayScheduler(AstNetlist* மாறிலி netlistp) VL_MT_DISABLED;
    // Creates a timing resume call (if needed, else returns null)
    AstCCall* createResume(AstVarScope* trigVscp) VL_MT_DISABLED;
    // Creates a timing ready call (if needed, else returns null)
    AstCCall* createReady(AstVarScope* trigVscp) VL_MT_DISABLED;

    TimingKit() = இயல்பு;
    TimingKit(LogicByScope&& lbs, Ast_முனை_Stmt* postUpdates,
              std::map<மாறிலி AstVarScope*, std::set<Ast_Sen_மரம்*>>&& externalDomains)
        : m_externalDomains{externalDomains}
        , m_lbs{lbs}
        , m_postUpdates{postUpdates} {}
    VL_UNCOPYABLE(TimingKit);
    TimingKit(TimingKit&&) = இயல்பு;
    TimingKit& operator=(TimingKit&&) = இயல்பு;
};

class VirtIfaceTriggers final {
பொது:
    // One entry per VarScope of an interface member that may be written through a virtual
    // interface. Each gets its own extra trigger bit with value-change detection.
    கட்டமைப்பு TriggerEntry final {
        மாறிலி AstIface* m_ifacep;  // Interface type
        மாறிலி AstVar* m_memberp;  // Member variable in the interface definition
        AstVarScope* m_vscp;  // VarScope to monitor
    };

    std::vector<TriggerEntry> m_triggers;

    வெறுமை addTrigger(மாறிலி AstIface* ifacep, மாறிலி AstVar* memberp, AstVarScope* vscp) {
        m_triggers.தள்ளு_back(TriggerEntry{ifacep, memberp, vscp});
    }

    // Maps each triggered VarScope to its trigger index. addTriggeredIface() uses this
    // to look up triggers -- by specific VarScope for non-virtual reads, or by iterating
    // all entries of the same interface type for virtual interface reads.
    using VscpTrigMap = std::map<மாறிலி AstVarScope*, uint32_t>;
    VscpTrigMap makeVscpToTrigMap(மாறிலி TriggerKit& trigKit, uint32_t firstIndex) மாறிலி;

    VL_UNCOPYABLE(VirtIfaceTriggers);
    VirtIfaceTriggers() = இயல்பு;
    VirtIfaceTriggers(VirtIfaceTriggers&&) = இயல்பு;
    VirtIfaceTriggers& operator=(VirtIfaceTriggers&&) = இயல்பு;
};

// Design variables bound to covergroup 'ref'/'const ref' formal arguments at construction.
//
// Such a formal becomes a pointer member, bound when the covergroup is constructed, so a
// sample() reading through it holds no AstVarRef naming the design variable. V3Order has to
// model those reads, but by the time it runs the constructions sit in logic it does not order,
// so they are collected before ordering and handed to it.
class CovergroupRefBindings final {
பொது:
    // The design variables one covergroup sample() may read through its reference formals
    using Bindings = std::vector<AstVarScope*>;

தனிப்பட்ட:
    // Bindings of one covergroup handle. Exact, and so only present for a handle whose every
    // write is a construction we recognized -- see dropInstance().
    std::unordered_map<மாறிலி AstVarScope*, Bindings> m_byInstance;
    // Bindings over every construction of a covergroup class, used where the handle a sample()
    // call reaches is not known. An over-approximation, which is safe.
    std::unordered_map<மாறிலி AstClass*, Bindings> m_byClass;
    // How the sample() calls seen so far resolved. Statistics only, hence mutable: ordering
    // runs once per scheduling region and must hold this by const reference.
    mutable uint32_t m_இலக்கம்_Exact_Calls = 0;
    mutable uint32_t m_இலக்கம்_Union_Calls = 0;
    // Returned for a covergroup with no reference formal at all.
    நிலையான மாறிலி Bindings s_none;

பொது:
    // Record one construction of covergroup 'classp' binding reference formals to 'bindings'.
    // 'instp' is the handle assigned, or nullptr if the destination could not be identified.
    வெறுமை addConstruction(மாறிலி AstClass* classp, மாறிலி AstVarScope* instp,
                         மாறிலி Bindings& bindings);
    // Forget the exact bindings of a handle, which turned out to be written by something other
    // than a construction we recognized. Its sample() calls fall back to the class union.
    வெறுமை dropInstance(மாறிலி AstVarScope* instp) { m_byInstance.erase(instp); }
    // What a sample() of covergroup 'classp' may read through reference formals, when called on
    // handle 'instp' (nullptr if the call is not on a plain handle reference)
    மாறிலி Bindings& forSample(மாறிலி AstVarScope* instp, மாறிலி AstClass* classp) மாறிலி;
    // Sample calls of a covergroup with a reference formal resolved to one handle's bindings
    uint32_t இலக்கம்_Exact_Calls() மாறிலி { திரும்பு m_இலக்கம்_Exact_Calls; }
    // ... and those that had to take the class union instead
    uint32_t இலக்கம்_Union_Calls() மாறிலி { திரும்பு m_இலக்கம்_Union_Calls; }

    VL_UNCOPYABLE(CovergroupRefBindings);
    CovergroupRefBindings() = இயல்பு;
    CovergroupRefBindings(CovergroupRefBindings&&) = இயல்பு;
    CovergroupRefBindings& operator=(CovergroupRefBindings&&) = இயல்பு;
};

// Creates trigger vars for signals driven via virtual interfaces
VirtIfaceTriggers makeVirtIfaceTriggers(AstNetlist* nodep) VL_MT_DISABLED;

// Collects what covergroup reference formal arguments are bound to at construction
மாறிலி CovergroupRefBindings makeCovergroupRefBindings(AstNetlist* nodep) VL_MT_DISABLED;

// Creates the timing kit and marks variables written by suspendables
TimingKit prepareTiming(AstNetlist* மாறிலி netlistp) VL_MT_DISABLED;

// Transforms fork sub-statements into separate functions
வெறுமை transformForks(AstNetlist* மாறிலி netlistp) VL_MT_DISABLED;

// Top level entry point to scheduling
வெறுமை schedule(AstNetlist*) VL_MT_DISABLED;

// Sub-steps
LogicByScope breakCycles(AstNetlist* netlistp,
                         மாறிலி LogicByScope& combinationalLogic) VL_MT_DISABLED;
LogicRegions partition(LogicByScope& clockedLogic, LogicByScope& combinationalLogic,
                       LogicByScope& hybridLogic) VL_MT_DISABLED;
LogicReplicas replicateLogic(LogicRegions&) VL_MT_DISABLED;

// Utility functions used by various steps in scheduling
namespace util {
// Create a new sub function (not an entry point)
AstCFunc* makeSubFunction(AstNetlist* netlistp, மாறிலி string& name, bool slow);
// Add an argument of the given type to the given function
AstVarScope* newArgument(AstCFunc* funcp, Ast_முனை_DType* dtypep, மாறிலி string& name,
                         VDirection direction);
// Create statement that sets the given 'vscp' to 'val'
Ast_முனை_Stmt* setVar(AstVarScope* vscp, uint32_t val);
// Create statement that increments the given 'vscp' by one
Ast_முனை_Stmt* incrementVar(AstVarScope* vscp);
// Create statement that calls the given 'void' returning function
Ast_முனை_Stmt* callVoidFunc(AstCFunc* funcp);
// Split large function according to --output-split-cfuncs
வெறுமை splitCheck(AstCFunc* ofuncp);
}  // namespace util

வெறுமை beforeTrigVisitor(AstNetlist* netlistp, Sen_கோவை_Builder& sen_கோவை_Builder,
                       மாறிலி TriggerKit& trigKit);

}  // namespace V3Sched

#endif  // Guard
