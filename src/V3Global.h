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
#অন্তর্ভাব "config_build.h"
#ifndef HAVE_CONFIG_PACKAGE
# error "Something failed during ./configure as config_package.h is incomplete. Perhaps you used autoreconf, don't."
#endif
// clang-format on

#অন্তর্ভাব "verilatedos.h"

#অন্তর্ভাব "V3Error.h"
#অন্তর্ভাব "V3FileLine.h"
#অন্তর্ভাব "V3Mutex.h"
#অন্তর্ভাব "V3Options.h"

#অন্তর্ভাব <string>
#অন্তর্ভাব <thread>
#অন্তর্ভাব <type_traits>
#অন্তর্ভাব <unordered_map>
#অন্তর্ভাব <unordered_set>

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
    ধ্রুবক VRestorerTrivial<typename std::decay_t<decltype(var)>> restorer_##var(var);

// This is the equivalent of VL_RESTORER for non-trivially copyable types. Still more efficient
// than VL_RESTORER as uses move to restore.
#define VL_RESTORER_COPY(var) \
    ধ্রুবক VRestorerCopy<typename std::decay_t<decltype(var)>> restorer_##var(var);

// Swap 'var' with a no-args constructed object of the same type, effectively clearing it, then
// swap back at end-of-scope. No copying involved. Use this e.g. for std containers where they
// would be .clear()'d right after the VL_RESTORER instance.
#define VL_RESTORER_CLEAR(var) \
    ধ্রুবক VRestorerClear<typename std::decay_t<decltype(var)>> restorer_##var(var);

// Get const reference to the saved copy
#define VL_RESTORER_পূর্ব(var) restorer_##var.saved()

// Implementation of VL_RESTORER
template <typename T>
class VRestorerTrivial final {
    static_assert(std::is_trivially_copyable<T>::মান,
                  "Use VL_RESTORER_{COPY,CLEAR} for non trivially copyable types");
    T& m_ref;  // Reference to object we're saving and restoring
    ধ্রুবক T m_saved;  // Value saved, for later restore

সর্বজনীন:
    explicit VRestorerTrivial(T& val)
        : m_ref{val}
        , m_saved{val} {}
    ~VRestorerTrivial() { m_ref = m_saved; }
    VL_UNCOPYABLE(VRestorerTrivial);
    // Must be stack allocated
    শূন্য* operator new(আকার_t) = delete;
    শূন্য operator delete(শূন্য*) = delete;

    ধ্রুবক T& saved() ধ্রুবক { ফেরত m_saved; }
};

// Implementation of VL_RESTORER_COPY
template <typename T>
class VRestorerCopy final {
    static_assert(!std::is_trivially_copyable<T>::মান,
                  "Use VL_RESTORER for trivially copyable types");
    T& m_ref;  // Reference to object we're saving and restoring
    T m_saved;  // Value saved, for later restore

সর্বজনীন:
    explicit VRestorerCopy(T& val)
        : m_ref{val}
        , m_saved{val} {}
    ~VRestorerCopy() { m_ref = std::move(m_saved); }
    VL_UNCOPYABLE(VRestorerCopy);
    // Must be stack allocated
    শূন্য* operator new(আকার_t) = delete;
    শূন্য operator delete(শূন্য*) = delete;

    ধ্রুবক T& saved() ধ্রুবক { ফেরত m_saved; }
};

// Implementation of VL_RESTORER_CLEAR.
template <typename T>
class VRestorerClear final {
    static_assert(!std::is_trivially_copyable<T>::মান,
                  "Use VL_RESTORER for trivially copyable types");
    T& m_ref;  // Reference to object we're saving and restoring
    T m_saved{};  // Owns the swapped-out value; starts empty

সর্বজনীন:
    explicit VRestorerClear(T& val)
        : m_ref{val} {
        std::swap(m_ref, m_saved);
    }
    ~VRestorerClear() { std::swap(m_ref, m_saved); }
    VL_UNCOPYABLE(VRestorerClear);
    // Must be stack allocated
    শূন্য* operator new(আকার_t) = delete;
    শূন্য operator delete(শূন্য*) = delete;

    ধ্রুবক T& saved() ধ্রুবক { ফেরত m_saved; }
};

//######################################################################

class VWidthMinUsage final {
সর্বজনীন:
    গণনা en : uint8_t { LINT_WIDTH, MATCHES_WIDTH, VERILOG_WIDTH };
    গণনা en m_e;
    VWidthMinUsage()
        : m_e{LINT_WIDTH} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VWidthMinUsage(en _e) VL_PURE : m_e{_e} {}
    constexpr VWidthMinUsage(ধ্রুবক VWidthMinUsage& _e) VL_PURE = পূর্বনির্ধারিত;
    explicit VWidthMinUsage(পূর্ণসংখ্যা _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ধ্রুবক { ফেরত m_e; }
    constexpr VWidthMinUsage& operator=(ধ্রুবক VWidthMinUsage& _e) VL_PURE = পূর্বনির্ধারিত;
};
constexpr bool operator==(ধ্রুবক VWidthMinUsage& lhs, ধ্রুবক VWidthMinUsage& rhs) {
    ফেরত lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ধ্রুবক VWidthMinUsage& lhs, VWidthMinUsage::en rhs) VL_PURE {
    ফেরত lhs.m_e == rhs;
}
constexpr bool operator==(VWidthMinUsage::en lhs, ধ্রুবক VWidthMinUsage& rhs) {
    ফেরত lhs == rhs.m_e;
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

    std::atomic_int m_debug_File_সংখ্যা{0};  // Number to append to debug files created
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
    std::unordered_map<ধ্রুবক শূন্য*, std::string>
        m_ptrToId;  // The actual 'address' <=> 'short string' bijection

    // Names of fields that were dumped by dumpJsonPtr()
    std::unordered_set<std::string> m_jsonPtrNames;

    // Id of the main thread
    ধ্রুবক std::thread::id m_mainThreadId = std::this_thread::get_id();

সর্বজনীন:
    // Options
    V3Options opt;  // All options; let user see them directly

    // CONSTRUCTORS
    V3Global() = পূর্বনির্ধারিত;
    শূন্য boot();
    শূন্য shutdown();  // Release allocated resources

    শূন্য vlExit(পূর্ণসংখ্যা status);

    // ACCESSORS (general)
    AstNetlist* rootp() ধ্রুবক VL_MT_SAFE { ফেরত m_rootp; }
    // Use V3ConstPool:: static methods instead
    V3ConstPool* constPoolp() ধ্রুবক VL_PURE { ফেরত m_constPoolp; }
    V3LibMap* libMapp() ধ্রুবক VL_PURE { ফেরত m_libMapp; }
    V3ThreadPool* threadPoolp() ধ্রুবক VL_PURE { ফেরত m_threadPoolp; }
    শূন্য threadPoolp(V3ThreadPool* threadPoolp) {
        UASSERT(!m_threadPoolp, "attempted to create multiple threadPool singletons");
        m_threadPoolp = threadPoolp;
    }
    VWidthMinUsage widthMinUsage() ধ্রুবক VL_PURE { ফেরত m_widthMinUsage; }
    bool assertDTypesResolved() ধ্রুবক { ফেরত m_assertDTypesResolved; }
    bool assertScoped() ধ্রুবক { ফেরত m_assertScoped; }
    bool staticsLifted() ধ্রুবক { ফেরত m_staticsLifted; }

    // METHODS
    শূন্য readFiles() VL_MT_DISABLED;
    শূন্য removeStd() VL_MT_DISABLED;
    শূন্য check_বৃক্ষ() ধ্রুবক;
    স্থির শূন্য dump_Check_Global_বৃক্ষ(ধ্রুবক string& stagename, পূর্ণসংখ্যা new_সংখ্যা = 0, bool doDump = true,
                                    bool doCheck = true);
    শূন্য assertDTypesResolved(bool flag) { m_assertDTypesResolved = flag; }
    শূন্য assertScoped(bool flag) { m_assertScoped = flag; }
    শূন্য staticsLifted(bool flag) { m_staticsLifted = flag; }
    শূন্য widthMinUsage(ধ্রুবক VWidthMinUsage& flag) { m_widthMinUsage = flag; }
    bool constRemoveXs() ধ্রুবক { ফেরত m_constRemoveXs; }
    শূন্য constRemoveXs(bool flag) { m_constRemoveXs = flag; }
    string debugFilename(ধ্রুবক string& nameComment, পূর্ণসংখ্যা new_সংখ্যা = 0);
    স্থির string digitsFilename(পূর্ণসংখ্যা সংখ্যা);
    bool needTraceDumper() ধ্রুবক { ফেরত m_needTraceDumper; }
    শূন্য needTraceDumper(bool flag) { m_needTraceDumper = flag; }
    bool dpi() ধ্রুবক VL_MT_SAFE { ফেরত m_dpi; }
    শূন্য dpi(bool flag) { m_dpi = flag; }
    bool assignsEvents() ধ্রুবক { ফেরত m_assignsEvents; }
    শূন্য setAssignsEvents() { m_assignsEvents = true; }
    bool hasEvents() ধ্রুবক { ফেরত m_hasEvents; }
    শূন্য setHasEvents() { m_hasEvents = true; }
    bool hasClasses() ধ্রুবক { ফেরত m_hasClasses; }
    শূন্য setHasClasses() { m_hasClasses = true; }
    bool hasSampled() ধ্রুবক { ফেরত m_hasSampled; }
    শূন্য setHasSampled() { m_hasSampled = true; }
    bool hasTable() ধ্রুবক { ফেরত m_hasTable; }
    শূন্য setHasTable() { m_hasTable = true; }
    bool hasVirtIfaces() ধ্রুবক { ফেরত m_hasVirtIfaces; }
    শূন্য setHasVirtIfaces() { m_hasVirtIfaces = true; }
    bool usesProbDist() ধ্রুবক { ফেরত m_usesProbDist; }
    শূন্য setUsesProbDist() { m_usesProbDist = true; }
    bool usesStdPackage() ধ্রুবক { ফেরত m_usesStdPackage; }
    শূন্য setUsesStdPackage() { m_usesStdPackage = true; }
    bool usesTiming() ধ্রুবক { ফেরত m_usesTiming; }
    শূন্য setUsesTiming() { m_usesTiming = true; }
    bool usesZeroDelay() ধ্রুবক { ফেরত m_usesZeroDelay; }
    শূন্য setUsesZeroDelay() { m_usesZeroDelay = true; }
    bool hasForceableSignals() ধ্রুবক { ফেরত m_hasForceableSignals; }
    শূন্য setHasForceableSignals() { m_hasForceableSignals = true; }
    bool hasAssignDeassign() ধ্রুবক { ফেরত m_hasAssignDeassign; }
    শূন্য setHasAssignDeassign() { m_hasAssignDeassign = true; }
    bool usesForce() ধ্রুবক { ফেরত m_usesForce; }
    শূন্য setUsesForce() { m_usesForce = true; }
    V3HierGraph* hierGraphp() ধ্রুবক { ফেরত m_hierGraphp; }
    শূন্য hierGraphp(V3HierGraph* graphp) { m_hierGraphp = graphp; }
    bool useParallelBuild() ধ্রুবক { ফেরত m_useParallelBuild; }
    শূন্য useParallelBuild(bool flag) { m_useParallelBuild = flag; }
    bool useRandSequence() ধ্রুবক { ফেরত m_useRandSequence; }
    শূন্য useRandSequence(bool flag) { m_useRandSequence = flag; }
    bool useCovergroup() ধ্রুবক { ফেরত m_useCovergroup; }
    শূন্য useCovergroup(bool flag) { m_useCovergroup = flag; }
    bool useRandomizeMethods() ধ্রুবক { ফেরত m_useRandomizeMethods; }
    শূন্য useRandomizeMethods(bool flag) { m_useRandomizeMethods = flag; }
    bool hasPrintedObjects() ধ্রুবক { ফেরত m_hasPrintedObjects; }
    শূন্য hasPrintedObjects(bool flag) { m_hasPrintedObjects = flag; }
    শূন্য saveJsonPtrFieldName(ধ্রুবক std::string& fieldName);
    শূন্য ptrNamesDumpJson(std::ostream& os);
    শূন্য idPtrMapDumpJson(std::ostream& os);
    ধ্রুবক std::string& ptrToId(ধ্রুবক শূন্য* p);
    std::thread::id mainThreadId() ধ্রুবক { ফেরত m_mainThreadId; }
    স্থির std::vector<std::string> verilatedCppFiles();
    uint64_t currentHierBlockCost() ধ্রুবক { ফেরত m_currentHierBlockCost; }
    শূন্য currentHierBlockCost(uint64_t cost) { m_currentHierBlockCost = cost; }
};

বাহ্যিক V3Global v3Global;

//######################################################################

#endif  // guard
