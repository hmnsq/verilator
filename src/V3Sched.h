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

#अगर_अघोषित VERILATOR_V3SCHED_H_
#घोषणा VERILATOR_V3SCHED_H_

#समावेश "config_build.h"
#समावेश "verilatedos.h"

#समावेश "V3Ast.h"

#समावेश <array>
#समावेश <functional>
#समावेश <unordered_map>
#समावेश <unordered_set>
#समावेश <utility>
#समावेश <vector>

class Sen_व्यंजक_Builder;

//============================================================================

namespace V3Sched {

namespace util {
using VarScopeSet = std::unordered_set<स्थिर AstVarScope*>;

अंतरभूत bool isVlForceVec(स्थिर AstVarScope* vscp) {
    स्थिर AstCDType* स्थिर dtypep = VN_CAST(vscp->dtypep()->skipRefp(), CDType);
    वापस dtypep && dtypep->name() == "VlForceVec";
}

अंतरभूत bool isForceReadMethod(VCMethod method) {
    वापस method == VCMethod::FORCE_READ || method == VCMethod::FORCE_READ_सूचकांक;
}

अंतरभूत व्योम collectForceReadEdgeIgnores(Ast_गाँठ* nodep, VarScopeSet& out) {
    VarScopeSet writtenForceVecs;
    VarScopeSet writtenVars;

    nodep->foreach([&](स्थिर AstVarRef* refp) {
        AstVarScope* स्थिर vscp = refp->varScopep();
        अगर (!refp->access().isWriteOrRW()) वापस;
        अगर (!refp->varp()->ignoreSchedWrite()) writtenVars.emplace(vscp);
        अगर (!isVlForceVec(vscp)) वापस;
        writtenForceVecs.emplace(vscp);
        अगर (!refp->varp()->ignoreSchedWrite()) out.emplace(vscp);
    });

    अगर (writtenForceVecs.empty() || writtenVars.empty()) वापस;

    nodep->foreach([&](स्थिर AstCMethodHard* callp) {
        अगर (!isForceReadMethod(callp->method())) वापस;

        स्थिर AstVarRef* स्थिर fromRefp = VN_CAST(callp->fromp(), VarRef);
        अगर (!fromRefp || !writtenForceVecs.गिनती(fromRefp->varScopep())) वापस;

        Ast_गाँठ_व्यंजक* स्थिर origp = callp->pinsp();
        अगर (!origp) वापस;
        origp->foreach([&](स्थिर AstVarRef* refp) {
            AstVarScope* स्थिर vscp = refp->varScopep();
            अगर (refp->access().isReadOrRW() && writtenVars.गिनती(vscp)) out.emplace(vscp);
        });
    });
}
}  // namespace util

//============================================================================
// Throughout scheduling, we need to keep hold of AstActive nodes, together
// with the AstScope that they are under. LogicByScope is simply a list of
// such pairs, with some additional convenience methods.

class LogicByScope final {
खुला:
    // TYPES
    // An AstActive and the AstScope it is under
    काष्ठा Entry final {
        AstScope* m_scopep;  // The scope the AstActive is under
        AstActive* m_activep;  // The AstActive holding the logic
        std::set<uint32_t> m_triggers;  // Trigger indices m_activep is sensitive to
    };
    using iterator = std::vector<Entry>::iterator;
    using const_iterator = std::vector<Entry>::const_iterator;

निजी:
    // STATE
    std::vector<Entry> m_entries;  // The AstActives held, with their scopes

खुला:
    // METHODS

    // Iteration and access
    iterator begin() { वापस m_entries.begin(); }
    iterator end() { वापस m_entries.end(); }
    const_iterator begin() स्थिर { वापस m_entries.begin(); }
    const_iterator end() स्थिर { वापस m_entries.end(); }
    bool empty() स्थिर { वापस m_entries.empty(); }
    माप_प्रकार आकार() स्थिर { वापस m_entries.आकार(); }
    स्थिर Entry& चालक[](माप_प्रकार i) स्थिर { वापस m_entries[i]; }

    // Add an AstActive under the given scope
    व्योम add(AstScope* scopep, AstActive* activep) { m_entries.धकेलो_back({scopep, activep, {}}); }

    // Add logic
    व्योम add(AstScope* scopep, Ast_Sen_वृक्ष* senTreep, Ast_गाँठ* logicp) {
        UASSERT_OBJ(!logicp->backp(), logicp, "Already linked");
        अगर (m_entries.empty() || m_entries.back().m_scopep != scopep
            || m_entries.back().m_activep->sentreep() != senTreep) {
            add(scopep, new AstActive{logicp->fileline(), "", senTreep});
        }
        m_entries.back().m_activep->addStmtsp(logicp);
    };

    // Create copy, with the AstActives cloned
    LogicByScope clone() स्थिर {
        LogicByScope परिणाम;
        क्रम (स्थिर Entry& entry : m_entries) {
            परिणाम.add(entry.m_scopep, entry.m_activep->clone_वृक्ष(false));
        }
        वापस परिणाम;
    }

    // Delete actives (they should all be empty)
    व्योम deleteActives() {
        क्रम (स्थिर Entry& entry : m_entries) {
            AstActive* स्थिर activep = entry.m_activep;
            अगर (v3Global.opt.debugCheck()) {
                क्रम (Ast_गाँठ* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) {
                    Ast_गाँठ_Procedure* स्थिर procp = VN_CAST(nodep, गाँठ_Procedure);
                    UASSERT_OBJ(procp && !procp->stmtsp(), activep, "Leftover logic");
                }
            }
            अगर (activep->backp()) activep->unlinkFrBack();
            VL_DO_DANGLING(activep->delete_वृक्ष(), activep);
        }
        m_entries.clear();
    };

    व्योम foreachLogic(स्थिर std::function<व्योम(Ast_गाँठ*)>& f) स्थिर {
        क्रम (स्थिर Entry& entry : m_entries) {
            स्थिर AstActive* स्थिर activep = entry.m_activep;
            क्रम (Ast_गाँठ* nodep = activep->stmtsp(); nodep; nodep = nodep->nextp()) f(nodep);
        }
    }
};

// Logic in the design is classified based on what can trigger its execution.
// For details see the internals documentation.
काष्ठा LogicClasses final {
    LogicByScope m_static;  // Static variable initializers
    LogicByScope m_initial;  // initial blocks
    LogicByScope m_final;  // final blocks
    LogicByScope m_comb;  // Combinational logic (logic with implicit sensitivities)
    LogicByScope m_clocked;  // Clocked (or sequential) logic (logic with explicit sensitivities)
    LogicByScope m_hybrid;  // Hybrid logic (combinational logic with some explicit sensitivities)
    LogicByScope m_postponed;  // Postponed logic ($strobe)
    LogicByScope m_observed;  // Observed logic (contains AstAlwaysObserved)
    LogicByScope m_reactive;  // Reactive logic (contains AstAlwaysReactive)

    LogicClasses() = शेष;
    VL_UNCOPYABLE(LogicClasses);
    LogicClasses(LogicClasses&&) = शेष;
    LogicClasses& चालक=(LogicClasses&&) = शेष;
};

// Combinational (including hybrid) logic, and clocked logic in partitioned to compute all clock
// signals in the 'act' region. For details see the internals documentation.
काष्ठा LogicRegions final {
    LogicByScope m_pre;  // AstAlwaysPre logic in 'act' region
    LogicByScope m_act;  // 'act' region logic
    LogicByScope m_nba;  // 'nba' region logic
    LogicByScope m_obs;  // AstAlwaysObserved logic in 'obs' region
    LogicByScope m_react;  // AstAlwaysReactive logic in 're' region

    LogicRegions() = शेष;
    VL_UNCOPYABLE(LogicRegions);
    LogicRegions(LogicRegions&&) = शेष;
    LogicRegions& चालक=(LogicRegions&&) = शेष;
};

// Combinational (including hybrid) logic is replicated into the various scheduling regions.
// For details see the internals documentation.
काष्ठा LogicReplicas final {
    LogicByScope m_ico;  // Logic replicated into the 'ico' (Input Combinational) region
    LogicByScope m_act;  // Logic replicated into the 'act' region
    LogicByScope m_nba;  // Logic replicated into the 'nba' region
    LogicByScope m_obs;  // Logic replicated into the 'obs' region
    LogicByScope m_react;  // Logic replicated into the 're' region

    LogicReplicas() = शेष;
    VL_UNCOPYABLE(LogicReplicas);
    LogicReplicas(LogicReplicas&&) = शेष;
    LogicReplicas& चालक=(LogicReplicas&&) = शेष;
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
खुला:
    // Triggers are storead as an UnpackedArray with a fixed word size
    अटल constexpr uint32_t WORD_आकार_LOG2 = 6;  // 64-bits / VL_QUADSIZE
    अटल constexpr uint32_t WORD_आकार = 1 << WORD_आकार_LOG2;

निजी:
    स्थिर std::string m_name;  // TriggerKit name
    स्थिर bool m_slow;  // TriggerKit is for schedulign 'slow' code
    स्थिर uint32_t m_n_Sense_शब्दगण;  // Number of words for Sense triggers
    स्थिर uint32_t m_n_Extra_शब्दगण;  // Number of words for Extra triggers
    स्थिर uint32_t m_n_Pre_शब्दगण;  // Number of words for 'pre' part
    स्थिर uint32_t m_n_Vec_शब्दगण = m_n_Sense_शब्दगण + m_n_Extra_शब्दगण;  // Number of words in 'vec' part

    // SenItems to corresponding bit indexes
    std::unordered_map<VNRef<स्थिर AstSenItem>, माप_प्रकार> m_senItem2TrigIdx;
    // Bit index to corresponding SenItem, nullptr for padding and Extra triggers
    std::vector<स्थिर AstSenItem*> m_trigIdx2SenItem;
    // Data type of a single trigger word
    Ast_गाँठ_DType* m_wordDTypep = nullptr;
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
    AstCFunc* createDumpExtFunc() स्थिर;
    AstCFunc* createAnySetFunc(AstUnpackArrayDType* स्थिर dtypep) स्थिर;
    AstCFunc* createClearFunc() स्थिर;
    AstCFunc* createOrIntoFunc(AstUnpackArrayDType* स्थिर oDtypep,
                               AstUnpackArrayDType* स्थिर iDtypep) स्थिर;

    TriggerKit(स्थिर std::string& name, bool slow, uint32_t n_Sense_शब्दगण, uint32_t n_Extra_शब्दगण,
               uint32_t n_Pre_शब्दगण,
               std::unordered_map<VNRef<स्थिर AstSenItem>, माप_प्रकार> senItem2TrigIdx,
               स्थिर std::vector<स्थिर AstSenItem*>& senItemps, bool useAcc);
    VL_UNCOPYABLE(TriggerKit);

खुला:
    // Move constructible
    TriggerKit(TriggerKit&&) = शेष;
    ~TriggerKit() = शेष;
    TriggerKit& चालक=(TriggerKit&&) = delete;

    // Utility for extra trigger allocation
    class ExtraTriggers final {
        मित्र class TriggerKit;
        std::vector<string> m_descriptions;  // Human readable description of extra triggers

    खुला:
        ExtraTriggers() = शेष;
        ~ExtraTriggers() = शेष;

        uint32_t allocate(स्थिर string& description) {
            m_descriptions.धकेलो_back(description);
            वापस m_descriptions.आकार() - 1;
        }
        uint32_t आकार() स्थिर { वापस m_descriptions.आकार(); }
    };
    // Generates list of assignments that fills
    अटल AstAssign* createSenTrigVecAssignment(AstVarScope* स्थिर target,
                                                 std::vector<Ast_गाँठ_व्यंजक*>& trigps);

    // Create a TriggerKit for the given AstSenTree vector
    अटल TriggerKit create(AstNetlist* netlistp,  //
                             AstCFunc* स्थिर initFuncp,  //
                             Sen_व्यंजक_Builder& sen_व्यंजक_Builder,  //
                             स्थिर std::vector<स्थिर Ast_Sen_वृक्ष*>& preTreeps,  //
                             स्थिर std::vector<स्थिर Ast_Sen_वृक्ष*>& senTreeps,  //
                             स्थिर string& name,  //
                             स्थिर ExtraTriggers& extraTriggers,  //
                             bool slow,  //
                             bool useAcc);

    // ACCESSORS
    AstVarScope* vscp() स्थिर { वापस m_vscp; }
    AstVarScope* vscAccp() स्थिर { वापस m_vscAccp; }
    माप_प्रकार senItem2TrigIdx(स्थिर AstSenItem* senItemp) स्थिर {
        वापस m_senItem2TrigIdx.at(*senItemp);
    }
    स्थिर std::vector<स्थिर AstSenItem*>& trigIdx2SenItem() स्थिर { वापस m_trigIdx2SenItem; }
    AstCFunc* compBasep() स्थिर { वापस m_compVecp; }
    स्थिर std::string& name() स्थिर { वापस m_name; }

    // Helpers for code generation - lazy construct relevant functions
    Ast_गाँठ_Stmt* newAndNotCall(AstVarScope* op, AstVarScope* ap, AstVarScope* bp) स्थिर;
    Ast_गाँठ_व्यंजक* newAnySetCall(AstVarScope* vscp) स्थिर;
    Ast_गाँठ_Stmt* newClearCall(AstVarScope* vscp) स्थिर;
    Ast_गाँठ_Stmt* newOrIntoCall(AstVarScope* op, AstVarScope* ip) स्थिर;
    // Helpers for code generation
    Ast_गाँठ_Stmt* newCompBaseCall() स्थिर;
    Ast_गाँठ_Stmt* newCompExtCall(AstVarScope* vscp) स्थिर;
    Ast_गाँठ_Stmt* newDumpCall(AstVarScope* vscp, स्थिर std::string& tag, bool debugOnly) स्थिर;
    // Create a new (non-extended) trigger vector - might return nullptr if there are no triggers
    AstVarScope* newTrigVec(स्थिर std::string& name) स्थिर;

    // Index in the trigger vector of the given Extra trigger
    uint32_t extra_Trigger_सूचकांक(uint32_t सूचकांक_2) स्थिर;

    // Indices in the trigger vector of the items of the given SenTree, either of the 'pre' copy,
    // or of the regular triggers
    std::set<uint32_t> triggerIndices(स्थिर Ast_Sen_वृक्ष* senTreep, bool pre) स्थिर;

    // Statement setting the extra trigger bit at 'index' to the value of 'vscp'
    Ast_गाँठ_Stmt* newExtraTriggerAssignment(AstVarScope* vscp, uint32_t सूचकांक_2) स्थिर;

    // Set the extra trigger bit at 'index' to the value of 'vscp', then set 'vscp' to 0.
    // Prepended to the trigger computation function.
    व्योम addExtraTriggerAssignment(AstVarScope* vscp, uint32_t सूचकांक_2) स्थिर;

    // Set trigger bit at 'index' when vscp's value changes from previous evaluation.
    // Creates a prev variable: trigger[bit] = (vscp != prev); prev = vscp
    व्योम add_मूल्य_Change_Trigger_Assignment(AstNetlist* netlistp, AstCFunc* initFuncp,
                                         AstVarScope* vscp, uint32_t सूचकांक_2) स्थिर;
};

// Everything needed for combining timing with static scheduling.
class TimingKit final {
    AstCFunc* m_resumeFuncp = nullptr;  // Global timing resume function
    AstCFunc* m_readyFuncp = nullptr;  // Global timing ready function

    // Additional var sensitivities for V3Order
    std::map<स्थिर AstVarScope*, std::set<Ast_Sen_वृक्ष*>> m_externalDomains;

खुला:
    LogicByScope m_lbs;  // Actives that resume timing schedulers
    Ast_गाँठ_Stmt* m_postUpdates = nullptr;  // Post updates for the trigger eval function

    // Trigger indices of the external domains of each variable
    std::map<स्थिर AstVarScope*, std::vector<uint32_t>>
    externalTriggerIndices(स्थिर TriggerKit& trigKit) स्थिर VL_MT_DISABLED;
    // Get the delay scheduler variable
    AstVarScope* getDelayScheduler(AstNetlist* स्थिर netlistp) VL_MT_DISABLED;
    // Creates a timing resume call (if needed, else returns null)
    AstCCall* createResume(AstVarScope* trigVscp) VL_MT_DISABLED;
    // Creates a timing ready call (if needed, else returns null)
    AstCCall* createReady(AstVarScope* trigVscp) VL_MT_DISABLED;

    TimingKit() = शेष;
    TimingKit(LogicByScope&& lbs, Ast_गाँठ_Stmt* postUpdates,
              std::map<स्थिर AstVarScope*, std::set<Ast_Sen_वृक्ष*>>&& externalDomains)
        : m_externalDomains{externalDomains}
        , m_lbs{lbs}
        , m_postUpdates{postUpdates} {}
    VL_UNCOPYABLE(TimingKit);
    TimingKit(TimingKit&&) = शेष;
    TimingKit& चालक=(TimingKit&&) = शेष;
};

class VirtIfaceTriggers final {
खुला:
    // One entry per VarScope of an interface member that may be written through a virtual
    // interface. Each gets its own extra trigger bit with value-change detection.
    काष्ठा TriggerEntry final {
        स्थिर AstIface* m_ifacep;  // Interface type
        स्थिर AstVar* m_memberp;  // Member variable in the interface definition
        AstVarScope* m_vscp;  // VarScope to monitor
    };

    std::vector<TriggerEntry> m_triggers;

    व्योम addTrigger(स्थिर AstIface* ifacep, स्थिर AstVar* memberp, AstVarScope* vscp) {
        m_triggers.धकेलो_back(TriggerEntry{ifacep, memberp, vscp});
    }

    // Maps each triggered VarScope to its trigger index. addTriggeredIface() uses this
    // to look up triggers -- by specific VarScope for non-virtual reads, or by iterating
    // all entries of the same interface type for virtual interface reads.
    using VscpTrigMap = std::map<स्थिर AstVarScope*, uint32_t>;
    VscpTrigMap makeVscpToTrigMap(स्थिर TriggerKit& trigKit, uint32_t first_सूचकांक) स्थिर;

    VL_UNCOPYABLE(VirtIfaceTriggers);
    VirtIfaceTriggers() = शेष;
    VirtIfaceTriggers(VirtIfaceTriggers&&) = शेष;
    VirtIfaceTriggers& चालक=(VirtIfaceTriggers&&) = शेष;
};

// Design variables bound to covergroup 'ref'/'const ref' formal arguments at construction.
//
// Such a formal becomes a pointer member, bound when the covergroup is constructed, so a
// sample() reading through it holds no AstVarRef naming the design variable. V3Order has to
// model those reads, but by the time it runs the constructions sit in logic it does not order,
// so they are collected before ordering and handed to it.
class CovergroupRefBindings final {
खुला:
    // The design variables one covergroup sample() may read through its reference formals
    using Bindings = std::vector<AstVarScope*>;

निजी:
    // Bindings of one covergroup handle. Exact, and so only present for a handle whose every
    // write is a construction we recognized -- see dropInstance().
    std::unordered_map<स्थिर AstVarScope*, Bindings> m_byInstance;
    // Bindings over every construction of a covergroup class, used where the handle a sample()
    // call reaches is not known. An over-approximation, which is safe.
    std::unordered_map<स्थिर AstClass*, Bindings> m_byClass;
    // How the sample() calls seen so far resolved. Statistics only, hence mutable: ordering
    // runs once per scheduling region and must hold this by const reference.
    mutable uint32_t m_अंक_Exact_Calls = 0;
    mutable uint32_t m_अंक_Union_Calls = 0;
    // Returned for a covergroup with no reference formal at all.
    अटल स्थिर Bindings s_none;

खुला:
    // Record one construction of covergroup 'classp' binding reference formals to 'bindings'.
    // 'instp' is the handle assigned, or nullptr if the destination could not be identified.
    व्योम addConstruction(स्थिर AstClass* classp, स्थिर AstVarScope* instp,
                         स्थिर Bindings& bindings);
    // Forget the exact bindings of a handle, which turned out to be written by something other
    // than a construction we recognized. Its sample() calls fall back to the class union.
    व्योम dropInstance(स्थिर AstVarScope* instp) { m_byInstance.erase(instp); }
    // What a sample() of covergroup 'classp' may read through reference formals, when called on
    // handle 'instp' (nullptr if the call is not on a plain handle reference)
    स्थिर Bindings& forSample(स्थिर AstVarScope* instp, स्थिर AstClass* classp) स्थिर;
    // Sample calls of a covergroup with a reference formal resolved to one handle's bindings
    uint32_t अंक_Exact_Calls() स्थिर { वापस m_अंक_Exact_Calls; }
    // ... and those that had to take the class union instead
    uint32_t अंक_Union_Calls() स्थिर { वापस m_अंक_Union_Calls; }

    VL_UNCOPYABLE(CovergroupRefBindings);
    CovergroupRefBindings() = शेष;
    CovergroupRefBindings(CovergroupRefBindings&&) = शेष;
    CovergroupRefBindings& चालक=(CovergroupRefBindings&&) = शेष;
};

// Creates trigger vars for signals driven via virtual interfaces
VirtIfaceTriggers makeVirtIfaceTriggers(AstNetlist* nodep) VL_MT_DISABLED;

// Collects what covergroup reference formal arguments are bound to at construction
स्थिर CovergroupRefBindings makeCovergroupRefBindings(AstNetlist* nodep) VL_MT_DISABLED;

// Creates the timing kit and marks variables written by suspendables
TimingKit prepareTiming(AstNetlist* स्थिर netlistp) VL_MT_DISABLED;

// Transforms fork sub-statements into separate functions
व्योम transformForks(AstNetlist* स्थिर netlistp) VL_MT_DISABLED;

// Top level entry point to scheduling
व्योम schedule(AstNetlist*) VL_MT_DISABLED;

// Sub-steps
LogicByScope breakCycles(AstNetlist* netlistp,
                         स्थिर LogicByScope& combinationalLogic) VL_MT_DISABLED;
LogicRegions partition(LogicByScope& clockedLogic, LogicByScope& combinationalLogic,
                       LogicByScope& hybridLogic) VL_MT_DISABLED;
LogicReplicas replicateLogic(LogicRegions&) VL_MT_DISABLED;

// Utility functions used by various steps in scheduling
namespace util {
// Create a new sub function (not an entry point)
AstCFunc* makeSubFunction(AstNetlist* netlistp, स्थिर string& name, bool slow);
// Add an argument of the given type to the given function
AstVarScope* newArgument(AstCFunc* funcp, Ast_गाँठ_DType* dtypep, स्थिर string& name,
                         VDirection direction);
// Create statement that sets the given 'vscp' to 'val'
Ast_गाँठ_Stmt* setVar(AstVarScope* vscp, uint32_t val);
// Create statement that increments the given 'vscp' by one
Ast_गाँठ_Stmt* incrementVar(AstVarScope* vscp);
// Create statement that calls the given 'void' returning function
Ast_गाँठ_Stmt* callVoidFunc(AstCFunc* funcp);
// Split large function according to --output-split-cfuncs
व्योम splitCheck(AstCFunc* ofuncp);
}  // namespace util

व्योम beforeTrigVisitor(AstNetlist* netlistp, Sen_व्यंजक_Builder& sen_व्यंजक_Builder,
                       स्थिर TriggerKit& trigKit);

}  // namespace V3Sched

#पूर्ण_अगर  // Guard
