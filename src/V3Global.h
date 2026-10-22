// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Common headers
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

#ifndef VERILATOR_V3GLOBAL_H_
#define VERILATOR_V3GLOBAL_H_

// clang-format off
#અન્તર્ભાવ "config_build.h"
#ifndef HAVE_CONFIG_PACKAGE
# error "Something failed during ./configure as config_package.h is incomplete. Perhaps you used autoreconf, don't."
#endif
// clang-format on

#અન્તર્ભાવ "verilatedos.h"

#અન્તર્ભાવ "V3Error.h"
#અન્તર્ભાવ "V3FileLine.h"
#અન્તર્ભાવ "V3Mutex.h"
#અન્તર્ભાવ "V3Options.h"

#અન્તર્ભાવ <string>
#અન્તર્ભાવ <thread>
#અન્તર્ભાવ <type_traits>
#અન્તર્ભાવ <unordered_map>
#અન્તર્ભાવ <unordered_set>

class AstNetlist;
class V3ConstPool;
class V3HierGraph;
class V3LibMap;
class V3ThreadPool;

//======================================================================
// Restorer

// For all flavours, 'var' must be a named scalar variable (simple identifier).

// Copy given variable's value on the stack, copy saved value back at end-of-scope.
// Only usable for trivially copyable types; use VL_RESTORER_COPY/VL_RESTORER_CLEAR for
// non-trivially copyable types, which are more efficient.
#define VL_RESTORER(var) \
    અચલ VRestorerTrivial<typename std::decay_t<decltype(var)>> restorer_##var(var);

// This is the equivalent of VL_RESTORER for non-trivially copyable types. Still more efficient
// than VL_RESTORER as uses move to restore.
#define VL_RESTORER_COPY(var) \
    અચલ VRestorerCopy<typename std::decay_t<decltype(var)>> restorer_##var(var);

// Swap 'var' with a no-args constructed object of the same type, effectively clearing it, then
// swap back at end-of-scope. No copying involved. Use this e.g. for std containers where they
// would be .clear()'d right after the VL_RESTORER instance.
#define VL_RESTORER_CLEAR(var) \
    અચલ VRestorerClear<typename std::decay_t<decltype(var)>> restorer_##var(var);

// Get const reference to the saved copy
#define VL_RESTORER_PREV(var) restorer_##var.saved()

// Implementation of VL_RESTORER
template <typename T>
class VRestorerTrivial final {
    static_assert(std::is_trivially_copyable<T>::કિંમત,
                  "Use VL_RESTORER_{COPY,CLEAR} for non trivially copyable types");
    T& m_ref;  // Reference to object we're saving and restoring
    અચલ T m_saved;  // Value saved, for later restore

જાહેર:
    explicit VRestorerTrivial(T& val)
        : m_ref{val}
        , m_saved{val} {}
    ~VRestorerTrivial() { m_ref = m_saved; }
    VL_UNCOPYABLE(VRestorerTrivial);
    // Must be stack allocated
    શૂન્ય* operator new(size_t) = delete;
    શૂન્ય operator delete(શૂન્ય*) = delete;

    અચલ T& saved() અચલ { પાછા m_saved; }
};

// Implementation of VL_RESTORER_COPY
template <typename T>
class VRestorerCopy final {
    static_assert(!std::is_trivially_copyable<T>::કિંમત,
                  "Use VL_RESTORER for trivially copyable types");
    T& m_ref;  // Reference to object we're saving and restoring
    T m_saved;  // Value saved, for later restore

જાહેર:
    explicit VRestorerCopy(T& val)
        : m_ref{val}
        , m_saved{val} {}
    ~VRestorerCopy() { m_ref = std::move(m_saved); }
    VL_UNCOPYABLE(VRestorerCopy);
    // Must be stack allocated
    શૂન્ય* operator new(size_t) = delete;
    શૂન્ય operator delete(શૂન્ય*) = delete;

    અચલ T& saved() અચલ { પાછા m_saved; }
};

// Implementation of VL_RESTORER_CLEAR.
template <typename T>
class VRestorerClear final {
    static_assert(!std::is_trivially_copyable<T>::કિંમત,
                  "Use VL_RESTORER for trivially copyable types");
    T& m_ref;  // Reference to object we're saving and restoring
    T m_saved{};  // Owns the swapped-out value; starts empty

જાહેર:
    explicit VRestorerClear(T& val)
        : m_ref{val} {
        std::swap(m_ref, m_saved);
    }
    ~VRestorerClear() { std::swap(m_ref, m_saved); }
    VL_UNCOPYABLE(VRestorerClear);
    // Must be stack allocated
    શૂન્ય* operator new(size_t) = delete;
    શૂન્ય operator delete(શૂન્ય*) = delete;

    અચલ T& saved() અચલ { પાછા m_saved; }
};

//######################################################################

class VWidthMinUsage final {
જાહેર:
    ગણના en : uint8_t { LINT_WIDTH, MATCHES_WIDTH, VERILOG_WIDTH };
    ગણના en m_e;
    VWidthMinUsage()
        : m_e{LINT_WIDTH} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VWidthMinUsage(en _e) VL_PURE : m_e{_e} {}
    constexpr VWidthMinUsage(અચલ VWidthMinUsage& _e) VL_PURE = મૂળભૂત;
    explicit VWidthMinUsage(પૂર્ણાંક _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() અચલ { પાછા m_e; }
    constexpr VWidthMinUsage& operator=(અચલ VWidthMinUsage& _e) VL_PURE = મૂળભૂત;
};
constexpr bool operator==(અચલ VWidthMinUsage& lhs, અચલ VWidthMinUsage& rhs) {
    પાછા lhs.m_e == rhs.m_e;
}
constexpr bool operator==(અચલ VWidthMinUsage& lhs, VWidthMinUsage::en rhs) VL_PURE {
    પાછા lhs.m_e == rhs;
}
constexpr bool operator==(VWidthMinUsage::en lhs, અચલ VWidthMinUsage& rhs) {
    પાછા lhs == rhs.m_e;
}

//######################################################################
// V3Global - The top level class for the entire program

class V3Global final {
    // Globals
    // Root of entire netlist, created by makeInitNetlist(} so static constructors run first
    AstNetlist* m_rootp = nullptr;
    // Constant pool lookup, see V3ConstPool.h
    V3ConstPool* m_constPoolp = nullptr;
    // Hierarchical block graph (plan) iff hierarchical verilation is performed
    V3HierGraph* m_hierGraphp = nullptr;
    // Thread Pool, nullptr unless 'verilatedJobs' is known, set via threadPoolp(V3ThreadPool*)
    V3ThreadPool* m_threadPoolp = nullptr;
    // Library Mapping, nullptr unless --libmap is used
    V3LibMap* m_libMapp = nullptr;
    VWidthMinUsage m_widthMinUsage
        = VWidthMinUsage::LINT_WIDTH;  // What AstNode::widthMin() is used for

    std::atomic_int m_debug_File_સંખ્યા{0};  // Number to append to debug files created
    bool m_assertDTypesResolved = false;  // Tree should have dtypep()'s
    bool m_assertScoped = false;  // Tree is scoped
    bool m_staticsLifted = false;  // Static variables have been lifted to module scope
    bool m_assignsEvents = false;  // Design uses assignments on SystemVerilog Events
    bool m_constRemoveXs = false;  // Const needs to strip any Xs
    // Experimenting with always requiring heavy, see issue #2701
    bool m_needTraceDumper = false;  // Need __Vm_dumperp in symbols
    bool m_dpi = false;  // Need __Dpi include files
    bool m_hasEvents = false;  // Design uses SystemVerilog named events
    bool m_hasClasses = false;  // Design uses SystemVerilog classes
    bool m_hasSampled = false;  // Design uses SAMPLED expresions
    bool m_hasTable = false;  // Desgin has the UDP Table.
    bool m_hasVirtIfaces = false;  // Design uses virtual interfaces
    bool m_usesProbDist = false;  // Uses $dist_*
    bool m_usesStdPackage = false;  // Design uses the std package
    bool m_usesTiming = false;  // Design uses timing constructs
    bool m_usesForce = false;  // Design uses force/release statements
    bool m_usesZeroDelay = false;  // Design uses #0 delay (or non-constant delay)
    bool m_hasForceableSignals = false;  // Need to apply V3Force pass
    bool m_hasAssignDeassign = false;  // Need to apply V3Force pass for assign/deassign statements
    bool m_useParallelBuild = false;  // Use parallel build for model
    bool m_useRandSequence = false;  // Has `randsequence`
    bool m_useCovergroup = false;  // Has covergroup declarations
    bool m_useRandomizeMethods = false;  // Need to define randomize() class methods
    bool m_hasPrintedObjects = false;  // Design has format args printed with to_string()
    uint64_t m_currentHierBlockCost = 0;  // Total cost of this hier block, used for scheduling

    // Memory address to short string mapping (for debug)
    std::unordered_map<અચલ શૂન્ય*, std::string>
        m_ptrToId;  // The actual 'address' <=> 'short string' bijection

    // Names of fields that were dumped by dumpJsonPtr()
    std::unordered_set<std::string> m_jsonPtrNames;

    // Id of the main thread
    અચલ std::thread::id m_mainThreadId = std::this_thread::get_id();

જાહેર:
    // Options
    V3Options opt;  // All options; let user see them directly

    // CONSTRUCTORS
    V3Global() = મૂળભૂત;
    શૂન્ય boot();
    શૂન્ય shutdown();  // Release allocated resources

    શૂન્ય vlExit(પૂર્ણાંક status);

    // ACCESSORS (general)
    AstNetlist* rootp() અચલ VL_MT_SAFE { પાછા m_rootp; }
    // Use V3ConstPool:: static methods instead
    V3ConstPool* constPoolp() અચલ VL_PURE { પાછા m_constPoolp; }
    V3LibMap* libMapp() અચલ VL_PURE { પાછા m_libMapp; }
    V3ThreadPool* threadPoolp() અચલ VL_PURE { પાછા m_threadPoolp; }
    શૂન્ય threadPoolp(V3ThreadPool* threadPoolp) {
        UASSERT(!m_threadPoolp, "attempted to create multiple threadPool singletons");
        m_threadPoolp = threadPoolp;
    }
    VWidthMinUsage widthMinUsage() અચલ VL_PURE { પાછા m_widthMinUsage; }
    bool assertDTypesResolved() અચલ { પાછા m_assertDTypesResolved; }
    bool assertScoped() અચલ { પાછા m_assertScoped; }
    bool staticsLifted() અચલ { પાછા m_staticsLifted; }

    // METHODS
    શૂન્ય readFiles() VL_MT_DISABLED;
    શૂન્ય removeStd() VL_MT_DISABLED;
    શૂન્ય check_વૃક્ષ() અચલ;
    સ્થિર શૂન્ય dump_Check_Global_વૃક્ષ(અચલ string& stagename, પૂર્ણાંક new_સંખ્યા = 0, bool doDump = true,
                                    bool doCheck = true);
    શૂન્ય assertDTypesResolved(bool flag) { m_assertDTypesResolved = flag; }
    શૂન્ય assertScoped(bool flag) { m_assertScoped = flag; }
    શૂન્ય staticsLifted(bool flag) { m_staticsLifted = flag; }
    શૂન્ય widthMinUsage(અચલ VWidthMinUsage& flag) { m_widthMinUsage = flag; }
    bool constRemoveXs() અચલ { પાછા m_constRemoveXs; }
    શૂન્ય constRemoveXs(bool flag) { m_constRemoveXs = flag; }
    string debugFilename(અચલ string& nameComment, પૂર્ણાંક new_સંખ્યા = 0);
    સ્થિર string digitsFilename(પૂર્ણાંક સંખ્યા);
    bool needTraceDumper() અચલ { પાછા m_needTraceDumper; }
    શૂન્ય needTraceDumper(bool flag) { m_needTraceDumper = flag; }
    bool dpi() અચલ VL_MT_SAFE { પાછા m_dpi; }
    શૂન્ય dpi(bool flag) { m_dpi = flag; }
    bool assignsEvents() અચલ { પાછા m_assignsEvents; }
    શૂન્ય setAssignsEvents() { m_assignsEvents = true; }
    bool hasEvents() અચલ { પાછા m_hasEvents; }
    શૂન્ય setHasEvents() { m_hasEvents = true; }
    bool hasClasses() અચલ { પાછા m_hasClasses; }
    શૂન્ય setHasClasses() { m_hasClasses = true; }
    bool hasSampled() અચલ { પાછા m_hasSampled; }
    શૂન્ય setHasSampled() { m_hasSampled = true; }
    bool hasTable() અચલ { પાછા m_hasTable; }
    શૂન્ય setHasTable() { m_hasTable = true; }
    bool hasVirtIfaces() અચલ { પાછા m_hasVirtIfaces; }
    શૂન્ય setHasVirtIfaces() { m_hasVirtIfaces = true; }
    bool usesProbDist() અચલ { પાછા m_usesProbDist; }
    શૂન્ય setUsesProbDist() { m_usesProbDist = true; }
    bool usesStdPackage() અચલ { પાછા m_usesStdPackage; }
    શૂન્ય setUsesStdPackage() { m_usesStdPackage = true; }
    bool usesTiming() અચલ { પાછા m_usesTiming; }
    શૂન્ય setUsesTiming() { m_usesTiming = true; }
    bool usesZeroDelay() અચલ { પાછા m_usesZeroDelay; }
    શૂન્ય setUsesZeroDelay() { m_usesZeroDelay = true; }
    bool hasForceableSignals() અચલ { પાછા m_hasForceableSignals; }
    શૂન્ય setHasForceableSignals() { m_hasForceableSignals = true; }
    bool hasAssignDeassign() અચલ { પાછા m_hasAssignDeassign; }
    શૂન્ય setHasAssignDeassign() { m_hasAssignDeassign = true; }
    bool usesForce() અચલ { પાછા m_usesForce; }
    શૂન્ય setUsesForce() { m_usesForce = true; }
    V3HierGraph* hierGraphp() અચલ { પાછા m_hierGraphp; }
    શૂન્ય hierGraphp(V3HierGraph* graphp) { m_hierGraphp = graphp; }
    bool useParallelBuild() અચલ { પાછા m_useParallelBuild; }
    શૂન્ય useParallelBuild(bool flag) { m_useParallelBuild = flag; }
    bool useRandSequence() અચલ { પાછા m_useRandSequence; }
    શૂન્ય useRandSequence(bool flag) { m_useRandSequence = flag; }
    bool useCovergroup() અચલ { પાછા m_useCovergroup; }
    શૂન્ય useCovergroup(bool flag) { m_useCovergroup = flag; }
    bool useRandomizeMethods() અચલ { પાછા m_useRandomizeMethods; }
    શૂન્ય useRandomizeMethods(bool flag) { m_useRandomizeMethods = flag; }
    bool hasPrintedObjects() અચલ { પાછા m_hasPrintedObjects; }
    શૂન્ય hasPrintedObjects(bool flag) { m_hasPrintedObjects = flag; }
    શૂન્ય saveJsonPtrFieldName(અચલ std::string& fieldName);
    શૂન્ય ptrNamesDumpJson(std::ostream& os);
    શૂન્ય idPtrMapDumpJson(std::ostream& os);
    અચલ std::string& ptrToId(અચલ શૂન્ય* p);
    std::thread::id mainThreadId() અચલ { પાછા m_mainThreadId; }
    સ્થિર std::vector<std::string> verilatedCppFiles();
    uint64_t currentHierBlockCost() અચલ { પાછા m_currentHierBlockCost; }
    શૂન્ય currentHierBlockCost(uint64_t cost) { m_currentHierBlockCost = cost; }
};

બાહ્ય V3Global v3Global;

//######################################################################

#endif  // guard
