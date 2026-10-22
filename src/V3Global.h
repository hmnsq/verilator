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
#ಅನ್ತರ್ಭಾವ "config_build.h"
#ifndef HAVE_CONFIG_PACKAGE
# error "Something failed during ./configure as config_package.h is incomplete. Perhaps you used autoreconf, don't."
#endif
// clang-format on

#ಅನ್ತರ್ಭಾವ "verilatedos.h"

#ಅನ್ತರ್ಭಾವ "V3Error.h"
#ಅನ್ತರ್ಭಾವ "V3FileLine.h"
#ಅನ್ತರ್ಭಾವ "V3Mutex.h"
#ಅನ್ತರ್ಭಾವ "V3Options.h"

#ಅನ್ತರ್ಭಾವ <string>
#ಅನ್ತರ್ಭಾವ <thread>
#ಅನ್ತರ್ಭಾವ <type_traits>
#ಅನ್ತರ್ಭಾವ <unordered_map>
#ಅನ್ತರ್ಭಾವ <unordered_set>

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
    ಧ್ರುವ VRestorerTrivial<typename std::decay_t<decltype(var)>> restorer_##var(var);

// This is the equivalent of VL_RESTORER for non-trivially copyable types. Still more efficient
// than VL_RESTORER as uses move to restore.
#define VL_RESTORER_COPY(var) \
    ಧ್ರುವ VRestorerCopy<typename std::decay_t<decltype(var)>> restorer_##var(var);

// Swap 'var' with a no-args constructed object of the same type, effectively clearing it, then
// swap back at end-of-scope. No copying involved. Use this e.g. for std containers where they
// would be .clear()'d right after the VL_RESTORER instance.
#define VL_RESTORER_CLEAR(var) \
    ಧ್ರುವ VRestorerClear<typename std::decay_t<decltype(var)>> restorer_##var(var);

// Get const reference to the saved copy
#define VL_RESTORER_PREV(var) restorer_##var.saved()

// Implementation of VL_RESTORER
template <typename T>
class VRestorerTrivial final {
    static_assert(std::is_trivially_copyable<T>::ಮೌಲ್ಯ,
                  "Use VL_RESTORER_{COPY,CLEAR} for non trivially copyable types");
    T& m_ref;  // Reference to object we're saving and restoring
    ಧ್ರುವ T m_saved;  // Value saved, for later restore

ಸಾರ್ವಜನಿಕ:
    explicit VRestorerTrivial(T& val)
        : m_ref{val}
        , m_saved{val} {}
    ~VRestorerTrivial() { m_ref = m_saved; }
    VL_UNCOPYABLE(VRestorerTrivial);
    // Must be stack allocated
    ಶೂನ್ಯ* operator new(size_t) = delete;
    ಶೂನ್ಯ operator delete(ಶೂನ್ಯ*) = delete;

    ಧ್ರುವ T& saved() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_saved; }
};

// Implementation of VL_RESTORER_COPY
template <typename T>
class VRestorerCopy final {
    static_assert(!std::is_trivially_copyable<T>::ಮೌಲ್ಯ,
                  "Use VL_RESTORER for trivially copyable types");
    T& m_ref;  // Reference to object we're saving and restoring
    T m_saved;  // Value saved, for later restore

ಸಾರ್ವಜನಿಕ:
    explicit VRestorerCopy(T& val)
        : m_ref{val}
        , m_saved{val} {}
    ~VRestorerCopy() { m_ref = std::move(m_saved); }
    VL_UNCOPYABLE(VRestorerCopy);
    // Must be stack allocated
    ಶೂನ್ಯ* operator new(size_t) = delete;
    ಶೂನ್ಯ operator delete(ಶೂನ್ಯ*) = delete;

    ಧ್ರುವ T& saved() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_saved; }
};

// Implementation of VL_RESTORER_CLEAR.
template <typename T>
class VRestorerClear final {
    static_assert(!std::is_trivially_copyable<T>::ಮೌಲ್ಯ,
                  "Use VL_RESTORER for trivially copyable types");
    T& m_ref;  // Reference to object we're saving and restoring
    T m_saved{};  // Owns the swapped-out value; starts empty

ಸಾರ್ವಜನಿಕ:
    explicit VRestorerClear(T& val)
        : m_ref{val} {
        std::swap(m_ref, m_saved);
    }
    ~VRestorerClear() { std::swap(m_ref, m_saved); }
    VL_UNCOPYABLE(VRestorerClear);
    // Must be stack allocated
    ಶೂನ್ಯ* operator new(size_t) = delete;
    ಶೂನ್ಯ operator delete(ಶೂನ್ಯ*) = delete;

    ಧ್ರುವ T& saved() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_saved; }
};

//######################################################################

class VWidthMinUsage final {
ಸಾರ್ವಜನಿಕ:
    ಗಣನಾ en : uint8_t { LINT_WIDTH, MATCHES_WIDTH, VERILOG_WIDTH };
    ಗಣನಾ en m_e;
    VWidthMinUsage()
        : m_e{LINT_WIDTH} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VWidthMinUsage(en _e) VL_PURE : m_e{_e} {}
    constexpr VWidthMinUsage(ಧ್ರುವ VWidthMinUsage& _e) VL_PURE = ಸ್ವಭಾವ;
    explicit VWidthMinUsage(ಪೂರ್ಣಾಙ್ಕ _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_e; }
    constexpr VWidthMinUsage& operator=(ಧ್ರುವ VWidthMinUsage& _e) VL_PURE = ಸ್ವಭಾವ;
};
constexpr bool operator==(ಧ್ರುವ VWidthMinUsage& lhs, ಧ್ರುವ VWidthMinUsage& rhs) {
    ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ಧ್ರುವ VWidthMinUsage& lhs, VWidthMinUsage::en rhs) VL_PURE {
    ಪ್ರತ್ಯಾಗಮ lhs.m_e == rhs;
}
constexpr bool operator==(VWidthMinUsage::en lhs, ಧ್ರುವ VWidthMinUsage& rhs) {
    ಪ್ರತ್ಯಾಗಮ lhs == rhs.m_e;
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

    std::atomic_int m_debug_File_ಸಂಖ್ಯೆ{0};  // Number to append to debug files created
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
    std::unordered_map<ಧ್ರುವ ಶೂನ್ಯ*, std::string>
        m_ptrToId;  // The actual 'address' <=> 'short string' bijection

    // Names of fields that were dumped by dumpJsonPtr()
    std::unordered_set<std::string> m_jsonPtrNames;

    // Id of the main thread
    ಧ್ರುವ std::thread::id m_mainThreadId = std::this_thread::get_id();

ಸಾರ್ವಜನಿಕ:
    // Options
    V3Options opt;  // All options; let user see them directly

    // CONSTRUCTORS
    V3Global() = ಸ್ವಭಾವ;
    ಶೂನ್ಯ boot();
    ಶೂನ್ಯ shutdown();  // Release allocated resources

    ಶೂನ್ಯ vlExit(ಪೂರ್ಣಾಙ್ಕ status);

    // ACCESSORS (general)
    AstNetlist* rootp() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_rootp; }
    // Use V3ConstPool:: static methods instead
    V3ConstPool* constPoolp() ಧ್ರುವ VL_PURE { ಪ್ರತ್ಯಾಗಮ m_constPoolp; }
    V3LibMap* libMapp() ಧ್ರುವ VL_PURE { ಪ್ರತ್ಯಾಗಮ m_libMapp; }
    V3ThreadPool* threadPoolp() ಧ್ರುವ VL_PURE { ಪ್ರತ್ಯಾಗಮ m_threadPoolp; }
    ಶೂನ್ಯ threadPoolp(V3ThreadPool* threadPoolp) {
        UASSERT(!m_threadPoolp, "attempted to create multiple threadPool singletons");
        m_threadPoolp = threadPoolp;
    }
    VWidthMinUsage widthMinUsage() ಧ್ರುವ VL_PURE { ಪ್ರತ್ಯಾಗಮ m_widthMinUsage; }
    bool assertDTypesResolved() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_assertDTypesResolved; }
    bool assertScoped() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_assertScoped; }
    bool staticsLifted() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_staticsLifted; }

    // METHODS
    ಶೂನ್ಯ readFiles() VL_MT_DISABLED;
    ಶೂನ್ಯ removeStd() VL_MT_DISABLED;
    ಶೂನ್ಯ check_ವೃಕ್ಷ() ಧ್ರುವ;
    ಸ್ಥಿರ ಶೂನ್ಯ dump_Check_Global_ವೃಕ್ಷ(ಧ್ರುವ string& stagename, ಪೂರ್ಣಾಙ್ಕ new_ಸಂಖ್ಯೆ = 0, bool doDump = true,
                                    bool doCheck = true);
    ಶೂನ್ಯ assertDTypesResolved(bool flag) { m_assertDTypesResolved = flag; }
    ಶೂನ್ಯ assertScoped(bool flag) { m_assertScoped = flag; }
    ಶೂನ್ಯ staticsLifted(bool flag) { m_staticsLifted = flag; }
    ಶೂನ್ಯ widthMinUsage(ಧ್ರುವ VWidthMinUsage& flag) { m_widthMinUsage = flag; }
    bool constRemoveXs() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_constRemoveXs; }
    ಶೂನ್ಯ constRemoveXs(bool flag) { m_constRemoveXs = flag; }
    string debugFilename(ಧ್ರುವ string& nameComment, ಪೂರ್ಣಾಙ್ಕ new_ಸಂಖ್ಯೆ = 0);
    ಸ್ಥಿರ string digitsFilename(ಪೂರ್ಣಾಙ್ಕ ಸಂಖ್ಯೆ);
    bool needTraceDumper() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_needTraceDumper; }
    ಶೂನ್ಯ needTraceDumper(bool flag) { m_needTraceDumper = flag; }
    bool dpi() ಧ್ರುವ VL_MT_SAFE { ಪ್ರತ್ಯಾಗಮ m_dpi; }
    ಶೂನ್ಯ dpi(bool flag) { m_dpi = flag; }
    bool assignsEvents() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_assignsEvents; }
    ಶೂನ್ಯ setAssignsEvents() { m_assignsEvents = true; }
    bool hasEvents() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_hasEvents; }
    ಶೂನ್ಯ setHasEvents() { m_hasEvents = true; }
    bool hasClasses() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_hasClasses; }
    ಶೂನ್ಯ setHasClasses() { m_hasClasses = true; }
    bool hasSampled() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_hasSampled; }
    ಶೂನ್ಯ setHasSampled() { m_hasSampled = true; }
    bool hasTable() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_hasTable; }
    ಶೂನ್ಯ setHasTable() { m_hasTable = true; }
    bool hasVirtIfaces() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_hasVirtIfaces; }
    ಶೂನ್ಯ setHasVirtIfaces() { m_hasVirtIfaces = true; }
    bool usesProbDist() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_usesProbDist; }
    ಶೂನ್ಯ setUsesProbDist() { m_usesProbDist = true; }
    bool usesStdPackage() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_usesStdPackage; }
    ಶೂನ್ಯ setUsesStdPackage() { m_usesStdPackage = true; }
    bool usesTiming() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_usesTiming; }
    ಶೂನ್ಯ setUsesTiming() { m_usesTiming = true; }
    bool usesZeroDelay() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_usesZeroDelay; }
    ಶೂನ್ಯ setUsesZeroDelay() { m_usesZeroDelay = true; }
    bool hasForceableSignals() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_hasForceableSignals; }
    ಶೂನ್ಯ setHasForceableSignals() { m_hasForceableSignals = true; }
    bool hasAssignDeassign() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_hasAssignDeassign; }
    ಶೂನ್ಯ setHasAssignDeassign() { m_hasAssignDeassign = true; }
    bool usesForce() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_usesForce; }
    ಶೂನ್ಯ setUsesForce() { m_usesForce = true; }
    V3HierGraph* hierGraphp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_hierGraphp; }
    ಶೂನ್ಯ hierGraphp(V3HierGraph* graphp) { m_hierGraphp = graphp; }
    bool useParallelBuild() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_useParallelBuild; }
    ಶೂನ್ಯ useParallelBuild(bool flag) { m_useParallelBuild = flag; }
    bool useRandSequence() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_useRandSequence; }
    ಶೂನ್ಯ useRandSequence(bool flag) { m_useRandSequence = flag; }
    bool useCovergroup() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_useCovergroup; }
    ಶೂನ್ಯ useCovergroup(bool flag) { m_useCovergroup = flag; }
    bool useRandomizeMethods() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_useRandomizeMethods; }
    ಶೂನ್ಯ useRandomizeMethods(bool flag) { m_useRandomizeMethods = flag; }
    bool hasPrintedObjects() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_hasPrintedObjects; }
    ಶೂನ್ಯ hasPrintedObjects(bool flag) { m_hasPrintedObjects = flag; }
    ಶೂನ್ಯ saveJsonPtrFieldName(ಧ್ರುವ std::string& fieldName);
    ಶೂನ್ಯ ptrNamesDumpJson(std::ostream& os);
    ಶೂನ್ಯ idPtrMapDumpJson(std::ostream& os);
    ಧ್ರುವ std::string& ptrToId(ಧ್ರುವ ಶೂನ್ಯ* p);
    std::thread::id mainThreadId() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_mainThreadId; }
    ಸ್ಥಿರ std::vector<std::string> verilatedCppFiles();
    uint64_t currentHierBlockCost() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_currentHierBlockCost; }
    ಶೂನ್ಯ currentHierBlockCost(uint64_t cost) { m_currentHierBlockCost = cost; }
};

ಬಾಹ್ಯ V3Global v3Global;

//######################################################################

#endif  // guard
