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
#شامل "config_build.h"
#ifndef HAVE_CONFIG_PACKAGE
# error "Something failed during ./configure as config_package.h is incomplete. Perhaps you used autoreconf, don't."
#endif
// clang-format on

#شامل "verilatedos.h"

#شامل "V3Error.h"
#شامل "V3FileLine.h"
#شامل "V3Mutex.h"
#شامل "V3Options.h"

#شامل <string>
#شامل <thread>
#شامل <type_traits>
#شامل <unordered_map>
#شامل <unordered_set>

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
    ثابت VRestorerTrivial<typename std::decay_t<decltype(var)>> restorer_##var(var);

// This is the equivalent of VL_RESTORER for non-trivially copyable types. Still more efficient
// than VL_RESTORER as uses move to restore.
#define VL_RESTORER_COPY(var) \
    ثابت VRestorerCopy<typename std::decay_t<decltype(var)>> restorer_##var(var);

// Swap 'var' with a no-args constructed object of the same type, effectively clearing it, then
// swap back at end-of-scope. No copying involved. Use this e.g. for std containers where they
// would be .clear()'d right after the VL_RESTORER instance.
#define VL_RESTORER_CLEAR(var) \
    ثابت VRestorerClear<typename std::decay_t<decltype(var)>> restorer_##var(var);

// Get const reference to the saved copy
#define VL_RESTORER_PREV(var) restorer_##var.saved()

// Implementation of VL_RESTORER
template <typename T>
class VRestorerTrivial final {
    static_assert(std::is_trivially_copyable<T>::مقدار,
                  "Use VL_RESTORER_{COPY,CLEAR} for non trivially copyable types");
    T& m_ref;  // Reference to object we're saving and restoring
    ثابت T m_saved;  // Value saved, for later restore

عمومی:
    explicit VRestorerTrivial(T& val)
        : m_ref{val}
        , m_saved{val} {}
    ~VRestorerTrivial() { m_ref = m_saved; }
    VL_UNCOPYABLE(VRestorerTrivial);
    // Must be stack allocated
    تهی* operator new(size_t) = delete;
    تهی operator delete(تهی*) = delete;

    ثابت T& saved() ثابت { بازگشت m_saved; }
};

// Implementation of VL_RESTORER_COPY
template <typename T>
class VRestorerCopy final {
    static_assert(!std::is_trivially_copyable<T>::مقدار,
                  "Use VL_RESTORER for trivially copyable types");
    T& m_ref;  // Reference to object we're saving and restoring
    T m_saved;  // Value saved, for later restore

عمومی:
    explicit VRestorerCopy(T& val)
        : m_ref{val}
        , m_saved{val} {}
    ~VRestorerCopy() { m_ref = std::move(m_saved); }
    VL_UNCOPYABLE(VRestorerCopy);
    // Must be stack allocated
    تهی* operator new(size_t) = delete;
    تهی operator delete(تهی*) = delete;

    ثابت T& saved() ثابت { بازگشت m_saved; }
};

// Implementation of VL_RESTORER_CLEAR.
template <typename T>
class VRestorerClear final {
    static_assert(!std::is_trivially_copyable<T>::مقدار,
                  "Use VL_RESTORER for trivially copyable types");
    T& m_ref;  // Reference to object we're saving and restoring
    T m_saved{};  // Owns the swapped-out value; starts empty

عمومی:
    explicit VRestorerClear(T& val)
        : m_ref{val} {
        std::swap(m_ref, m_saved);
    }
    ~VRestorerClear() { std::swap(m_ref, m_saved); }
    VL_UNCOPYABLE(VRestorerClear);
    // Must be stack allocated
    تهی* operator new(size_t) = delete;
    تهی operator delete(تهی*) = delete;

    ثابت T& saved() ثابت { بازگشت m_saved; }
};

//######################################################################

class VWidthMinUsage final {
عمومی:
    شمارش en : uint8_t { LINT_WIDTH, MATCHES_WIDTH, VERILOG_WIDTH };
    شمارش en m_e;
    VWidthMinUsage()
        : m_e{LINT_WIDTH} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VWidthMinUsage(en _e) VL_PURE : m_e{_e} {}
    constexpr VWidthMinUsage(ثابت VWidthMinUsage& _e) VL_PURE = پیشفرض;
    explicit VWidthMinUsage(صحیح _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ثابت { بازگشت m_e; }
    constexpr VWidthMinUsage& operator=(ثابت VWidthMinUsage& _e) VL_PURE = پیشفرض;
};
constexpr bool operator==(ثابت VWidthMinUsage& lhs, ثابت VWidthMinUsage& rhs) {
    بازگشت lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ثابت VWidthMinUsage& lhs, VWidthMinUsage::en rhs) VL_PURE {
    بازگشت lhs.m_e == rhs;
}
constexpr bool operator==(VWidthMinUsage::en lhs, ثابت VWidthMinUsage& rhs) {
    بازگشت lhs == rhs.m_e;
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

    std::atomic_int m_debug_File_عدد{0};  // Number to append to debug files created
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
    std::unordered_map<ثابت تهی*, std::string>
        m_ptrToId;  // The actual 'address' <=> 'short string' bijection

    // Names of fields that were dumped by dumpJsonPtr()
    std::unordered_set<std::string> m_jsonPtrNames;

    // Id of the main thread
    ثابت std::thread::id m_mainThreadId = std::this_thread::get_id();

عمومی:
    // Options
    V3Options opt;  // All options; let user see them directly

    // CONSTRUCTORS
    V3Global() = پیشفرض;
    تهی boot();
    تهی shutdown();  // Release allocated resources

    تهی vlExit(صحیح status);

    // ACCESSORS (general)
    AstNetlist* rootp() ثابت VL_MT_SAFE { بازگشت m_rootp; }
    // Use V3ConstPool:: static methods instead
    V3ConstPool* constPoolp() ثابت VL_PURE { بازگشت m_constPoolp; }
    V3LibMap* libMapp() ثابت VL_PURE { بازگشت m_libMapp; }
    V3ThreadPool* threadPoolp() ثابت VL_PURE { بازگشت m_threadPoolp; }
    تهی threadPoolp(V3ThreadPool* threadPoolp) {
        UASSERT(!m_threadPoolp, "attempted to create multiple threadPool singletons");
        m_threadPoolp = threadPoolp;
    }
    VWidthMinUsage widthMinUsage() ثابت VL_PURE { بازگشت m_widthMinUsage; }
    bool assertDTypesResolved() ثابت { بازگشت m_assertDTypesResolved; }
    bool assertScoped() ثابت { بازگشت m_assertScoped; }
    bool staticsLifted() ثابت { بازگشت m_staticsLifted; }

    // METHODS
    تهی readFiles() VL_MT_DISABLED;
    تهی removeStd() VL_MT_DISABLED;
    تهی check_درخت() ثابت;
    ایستا تهی dump_Check_Global_درخت(ثابت string& stagename, صحیح new_عدد = 0, bool doDump = true,
                                    bool doCheck = true);
    تهی assertDTypesResolved(bool flag) { m_assertDTypesResolved = flag; }
    تهی assertScoped(bool flag) { m_assertScoped = flag; }
    تهی staticsLifted(bool flag) { m_staticsLifted = flag; }
    تهی widthMinUsage(ثابت VWidthMinUsage& flag) { m_widthMinUsage = flag; }
    bool constRemoveXs() ثابت { بازگشت m_constRemoveXs; }
    تهی constRemoveXs(bool flag) { m_constRemoveXs = flag; }
    string debugFilename(ثابت string& nameComment, صحیح new_عدد = 0);
    ایستا string digitsFilename(صحیح عدد);
    bool needTraceDumper() ثابت { بازگشت m_needTraceDumper; }
    تهی needTraceDumper(bool flag) { m_needTraceDumper = flag; }
    bool dpi() ثابت VL_MT_SAFE { بازگشت m_dpi; }
    تهی dpi(bool flag) { m_dpi = flag; }
    bool assignsEvents() ثابت { بازگشت m_assignsEvents; }
    تهی setAssignsEvents() { m_assignsEvents = true; }
    bool hasEvents() ثابت { بازگشت m_hasEvents; }
    تهی setHasEvents() { m_hasEvents = true; }
    bool hasClasses() ثابت { بازگشت m_hasClasses; }
    تهی setHasClasses() { m_hasClasses = true; }
    bool hasSampled() ثابت { بازگشت m_hasSampled; }
    تهی setHasSampled() { m_hasSampled = true; }
    bool hasTable() ثابت { بازگشت m_hasTable; }
    تهی setHasTable() { m_hasTable = true; }
    bool hasVirtIfaces() ثابت { بازگشت m_hasVirtIfaces; }
    تهی setHasVirtIfaces() { m_hasVirtIfaces = true; }
    bool usesProbDist() ثابت { بازگشت m_usesProbDist; }
    تهی setUsesProbDist() { m_usesProbDist = true; }
    bool usesStdPackage() ثابت { بازگشت m_usesStdPackage; }
    تهی setUsesStdPackage() { m_usesStdPackage = true; }
    bool usesTiming() ثابت { بازگشت m_usesTiming; }
    تهی setUsesTiming() { m_usesTiming = true; }
    bool usesZeroDelay() ثابت { بازگشت m_usesZeroDelay; }
    تهی setUsesZeroDelay() { m_usesZeroDelay = true; }
    bool hasForceableSignals() ثابت { بازگشت m_hasForceableSignals; }
    تهی setHasForceableSignals() { m_hasForceableSignals = true; }
    bool hasAssignDeassign() ثابت { بازگشت m_hasAssignDeassign; }
    تهی setHasAssignDeassign() { m_hasAssignDeassign = true; }
    bool usesForce() ثابت { بازگشت m_usesForce; }
    تهی setUsesForce() { m_usesForce = true; }
    V3HierGraph* hierGraphp() ثابت { بازگشت m_hierGraphp; }
    تهی hierGraphp(V3HierGraph* graphp) { m_hierGraphp = graphp; }
    bool useParallelBuild() ثابت { بازگشت m_useParallelBuild; }
    تهی useParallelBuild(bool flag) { m_useParallelBuild = flag; }
    bool useRandSequence() ثابت { بازگشت m_useRandSequence; }
    تهی useRandSequence(bool flag) { m_useRandSequence = flag; }
    bool useCovergroup() ثابت { بازگشت m_useCovergroup; }
    تهی useCovergroup(bool flag) { m_useCovergroup = flag; }
    bool useRandomizeMethods() ثابت { بازگشت m_useRandomizeMethods; }
    تهی useRandomizeMethods(bool flag) { m_useRandomizeMethods = flag; }
    bool hasPrintedObjects() ثابت { بازگشت m_hasPrintedObjects; }
    تهی hasPrintedObjects(bool flag) { m_hasPrintedObjects = flag; }
    تهی saveJsonPtrFieldName(ثابت std::string& fieldName);
    تهی ptrNamesDumpJson(std::ostream& os);
    تهی idPtrMapDumpJson(std::ostream& os);
    ثابت std::string& ptrToId(ثابت تهی* p);
    std::thread::id mainThreadId() ثابت { بازگشت m_mainThreadId; }
    ایستا std::vector<std::string> verilatedCppFiles();
    uint64_t currentHierBlockCost() ثابت { بازگشت m_currentHierBlockCost; }
    تهی currentHierBlockCost(uint64_t cost) { m_currentHierBlockCost = cost; }
};

بیرونی V3Global v3Global;

//######################################################################

#endif  // guard
