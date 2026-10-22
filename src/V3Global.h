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
#அந்தர்பாவ "config_build.h"
#ifndef HAVE_CONFIG_PACKAGE
# error "Something failed during ./configure as config_package.h is incomplete. Perhaps you used autoreconf, don't."
#endif
// clang-format on

#அந்தர்பாவ "verilatedos.h"

#அந்தர்பாவ "V3Error.h"
#அந்தர்பாவ "V3FileLine.h"
#அந்தர்பாவ "V3Mutex.h"
#அந்தர்பாவ "V3Options.h"

#அந்தர்பாவ <string>
#அந்தர்பாவ <thread>
#அந்தர்பாவ <type_traits>
#அந்தர்பாவ <unordered_map>
#அந்தர்பாவ <unordered_set>

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
    மாறிலி VRestorerTrivial<typename std::decay_t<decltype(var)>> restorer_##var(var);

// This is the equivalent of VL_RESTORER for non-trivially copyable types. Still more efficient
// than VL_RESTORER as uses move to restore.
#define VL_RESTORER_COPY(var) \
    மாறிலி VRestorerCopy<typename std::decay_t<decltype(var)>> restorer_##var(var);

// Swap 'var' with a no-args constructed object of the same type, effectively clearing it, then
// swap back at end-of-scope. No copying involved. Use this e.g. for std containers where they
// would be .clear()'d right after the VL_RESTORER instance.
#define VL_RESTORER_CLEAR(var) \
    மாறிலி VRestorerClear<typename std::decay_t<decltype(var)>> restorer_##var(var);

// Get const reference to the saved copy
#define VL_RESTORER_PREV(var) restorer_##var.saved()

// Implementation of VL_RESTORER
template <typename T>
class VRestorerTrivial final {
    static_assert(std::is_trivially_copyable<T>::மதிப்பு,
                  "Use VL_RESTORER_{COPY,CLEAR} for non trivially copyable types");
    T& m_ref;  // Reference to object we're saving and restoring
    மாறிலி T m_saved;  // Value saved, for later restore

பொது:
    explicit VRestorerTrivial(T& val)
        : m_ref{val}
        , m_saved{val} {}
    ~VRestorerTrivial() { m_ref = m_saved; }
    VL_UNCOPYABLE(VRestorerTrivial);
    // Must be stack allocated
    வெறுமை* operator new(size_t) = delete;
    வெறுமை operator delete(வெறுமை*) = delete;

    மாறிலி T& saved() மாறிலி { திரும்பு m_saved; }
};

// Implementation of VL_RESTORER_COPY
template <typename T>
class VRestorerCopy final {
    static_assert(!std::is_trivially_copyable<T>::மதிப்பு,
                  "Use VL_RESTORER for trivially copyable types");
    T& m_ref;  // Reference to object we're saving and restoring
    T m_saved;  // Value saved, for later restore

பொது:
    explicit VRestorerCopy(T& val)
        : m_ref{val}
        , m_saved{val} {}
    ~VRestorerCopy() { m_ref = std::move(m_saved); }
    VL_UNCOPYABLE(VRestorerCopy);
    // Must be stack allocated
    வெறுமை* operator new(size_t) = delete;
    வெறுமை operator delete(வெறுமை*) = delete;

    மாறிலி T& saved() மாறிலி { திரும்பு m_saved; }
};

// Implementation of VL_RESTORER_CLEAR.
template <typename T>
class VRestorerClear final {
    static_assert(!std::is_trivially_copyable<T>::மதிப்பு,
                  "Use VL_RESTORER for trivially copyable types");
    T& m_ref;  // Reference to object we're saving and restoring
    T m_saved{};  // Owns the swapped-out value; starts empty

பொது:
    explicit VRestorerClear(T& val)
        : m_ref{val} {
        std::swap(m_ref, m_saved);
    }
    ~VRestorerClear() { std::swap(m_ref, m_saved); }
    VL_UNCOPYABLE(VRestorerClear);
    // Must be stack allocated
    வெறுமை* operator new(size_t) = delete;
    வெறுமை operator delete(வெறுமை*) = delete;

    மாறிலி T& saved() மாறிலி { திரும்பு m_saved; }
};

//######################################################################

class VWidthMinUsage final {
பொது:
    எண்ணிக்கை en : uint8_t { LINT_WIDTH, MATCHES_WIDTH, VERILOG_WIDTH };
    எண்ணிக்கை en m_e;
    VWidthMinUsage()
        : m_e{LINT_WIDTH} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VWidthMinUsage(en _e) VL_PURE : m_e{_e} {}
    constexpr VWidthMinUsage(மாறிலி VWidthMinUsage& _e) VL_PURE = இயல்பு;
    explicit VWidthMinUsage(முழு _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() மாறிலி { திரும்பு m_e; }
    constexpr VWidthMinUsage& operator=(மாறிலி VWidthMinUsage& _e) VL_PURE = இயல்பு;
};
constexpr bool operator==(மாறிலி VWidthMinUsage& lhs, மாறிலி VWidthMinUsage& rhs) {
    திரும்பு lhs.m_e == rhs.m_e;
}
constexpr bool operator==(மாறிலி VWidthMinUsage& lhs, VWidthMinUsage::en rhs) VL_PURE {
    திரும்பு lhs.m_e == rhs;
}
constexpr bool operator==(VWidthMinUsage::en lhs, மாறிலி VWidthMinUsage& rhs) {
    திரும்பு lhs == rhs.m_e;
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

    std::atomic_int m_debug_File_எண்{0};  // Number to append to debug files created
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
    std::unordered_map<மாறிலி வெறுமை*, std::string>
        m_ptrToId;  // The actual 'address' <=> 'short string' bijection

    // Names of fields that were dumped by dumpJsonPtr()
    std::unordered_set<std::string> m_jsonPtrNames;

    // Id of the main thread
    மாறிலி std::thread::id m_mainThreadId = std::this_thread::get_id();

பொது:
    // Options
    V3Options opt;  // All options; let user see them directly

    // CONSTRUCTORS
    V3Global() = இயல்பு;
    வெறுமை boot();
    வெறுமை shutdown();  // Release allocated resources

    வெறுமை vlExit(முழு status);

    // ACCESSORS (general)
    AstNetlist* rootp() மாறிலி VL_MT_SAFE { திரும்பு m_rootp; }
    // Use V3ConstPool:: static methods instead
    V3ConstPool* constPoolp() மாறிலி VL_PURE { திரும்பு m_constPoolp; }
    V3LibMap* libMapp() மாறிலி VL_PURE { திரும்பு m_libMapp; }
    V3ThreadPool* threadPoolp() மாறிலி VL_PURE { திரும்பு m_threadPoolp; }
    வெறுமை threadPoolp(V3ThreadPool* threadPoolp) {
        UASSERT(!m_threadPoolp, "attempted to create multiple threadPool singletons");
        m_threadPoolp = threadPoolp;
    }
    VWidthMinUsage widthMinUsage() மாறிலி VL_PURE { திரும்பு m_widthMinUsage; }
    bool assertDTypesResolved() மாறிலி { திரும்பு m_assertDTypesResolved; }
    bool assertScoped() மாறிலி { திரும்பு m_assertScoped; }
    bool staticsLifted() மாறிலி { திரும்பு m_staticsLifted; }

    // METHODS
    வெறுமை readFiles() VL_MT_DISABLED;
    வெறுமை removeStd() VL_MT_DISABLED;
    வெறுமை check_மரம்() மாறிலி;
    நிலையான வெறுமை dump_Check_Global_மரம்(மாறிலி string& stagename, முழு new_எண் = 0, bool doDump = true,
                                    bool doCheck = true);
    வெறுமை assertDTypesResolved(bool flag) { m_assertDTypesResolved = flag; }
    வெறுமை assertScoped(bool flag) { m_assertScoped = flag; }
    வெறுமை staticsLifted(bool flag) { m_staticsLifted = flag; }
    வெறுமை widthMinUsage(மாறிலி VWidthMinUsage& flag) { m_widthMinUsage = flag; }
    bool constRemoveXs() மாறிலி { திரும்பு m_constRemoveXs; }
    வெறுமை constRemoveXs(bool flag) { m_constRemoveXs = flag; }
    string debugFilename(மாறிலி string& nameComment, முழு new_எண் = 0);
    நிலையான string digitsFilename(முழு எண்);
    bool needTraceDumper() மாறிலி { திரும்பு m_needTraceDumper; }
    வெறுமை needTraceDumper(bool flag) { m_needTraceDumper = flag; }
    bool dpi() மாறிலி VL_MT_SAFE { திரும்பு m_dpi; }
    வெறுமை dpi(bool flag) { m_dpi = flag; }
    bool assignsEvents() மாறிலி { திரும்பு m_assignsEvents; }
    வெறுமை setAssignsEvents() { m_assignsEvents = true; }
    bool hasEvents() மாறிலி { திரும்பு m_hasEvents; }
    வெறுமை setHasEvents() { m_hasEvents = true; }
    bool hasClasses() மாறிலி { திரும்பு m_hasClasses; }
    வெறுமை setHasClasses() { m_hasClasses = true; }
    bool hasSampled() மாறிலி { திரும்பு m_hasSampled; }
    வெறுமை setHasSampled() { m_hasSampled = true; }
    bool hasTable() மாறிலி { திரும்பு m_hasTable; }
    வெறுமை setHasTable() { m_hasTable = true; }
    bool hasVirtIfaces() மாறிலி { திரும்பு m_hasVirtIfaces; }
    வெறுமை setHasVirtIfaces() { m_hasVirtIfaces = true; }
    bool usesProbDist() மாறிலி { திரும்பு m_usesProbDist; }
    வெறுமை setUsesProbDist() { m_usesProbDist = true; }
    bool usesStdPackage() மாறிலி { திரும்பு m_usesStdPackage; }
    வெறுமை setUsesStdPackage() { m_usesStdPackage = true; }
    bool usesTiming() மாறிலி { திரும்பு m_usesTiming; }
    வெறுமை setUsesTiming() { m_usesTiming = true; }
    bool usesZeroDelay() மாறிலி { திரும்பு m_usesZeroDelay; }
    வெறுமை setUsesZeroDelay() { m_usesZeroDelay = true; }
    bool hasForceableSignals() மாறிலி { திரும்பு m_hasForceableSignals; }
    வெறுமை setHasForceableSignals() { m_hasForceableSignals = true; }
    bool hasAssignDeassign() மாறிலி { திரும்பு m_hasAssignDeassign; }
    வெறுமை setHasAssignDeassign() { m_hasAssignDeassign = true; }
    bool usesForce() மாறிலி { திரும்பு m_usesForce; }
    வெறுமை setUsesForce() { m_usesForce = true; }
    V3HierGraph* hierGraphp() மாறிலி { திரும்பு m_hierGraphp; }
    வெறுமை hierGraphp(V3HierGraph* graphp) { m_hierGraphp = graphp; }
    bool useParallelBuild() மாறிலி { திரும்பு m_useParallelBuild; }
    வெறுமை useParallelBuild(bool flag) { m_useParallelBuild = flag; }
    bool useRandSequence() மாறிலி { திரும்பு m_useRandSequence; }
    வெறுமை useRandSequence(bool flag) { m_useRandSequence = flag; }
    bool useCovergroup() மாறிலி { திரும்பு m_useCovergroup; }
    வெறுமை useCovergroup(bool flag) { m_useCovergroup = flag; }
    bool useRandomizeMethods() மாறிலி { திரும்பு m_useRandomizeMethods; }
    வெறுமை useRandomizeMethods(bool flag) { m_useRandomizeMethods = flag; }
    bool hasPrintedObjects() மாறிலி { திரும்பு m_hasPrintedObjects; }
    வெறுமை hasPrintedObjects(bool flag) { m_hasPrintedObjects = flag; }
    வெறுமை saveJsonPtrFieldName(மாறிலி std::string& fieldName);
    வெறுமை ptrNamesDumpJson(std::ostream& os);
    வெறுமை idPtrMapDumpJson(std::ostream& os);
    மாறிலி std::string& ptrToId(மாறிலி வெறுமை* p);
    std::thread::id mainThreadId() மாறிலி { திரும்பு m_mainThreadId; }
    நிலையான std::vector<std::string> verilatedCppFiles();
    uint64_t currentHierBlockCost() மாறிலி { திரும்பு m_currentHierBlockCost; }
    வெறுமை currentHierBlockCost(uint64_t cost) { m_currentHierBlockCost = cost; }
};

வெளி V3Global v3Global;

//######################################################################

#endif  // guard
