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
#ଅନ୍ତର୍ଭାଵ "config_build.h"
#ifndef HAVE_CONFIG_PACKAGE
# error "Something failed during ./configure as config_package.h is incomplete. Perhaps you used autoreconf, don't."
#endif
// clang-format on

#ଅନ୍ତର୍ଭାଵ "verilatedos.h"

#ଅନ୍ତର୍ଭାଵ "V3Error.h"
#ଅନ୍ତର୍ଭାଵ "V3FileLine.h"
#ଅନ୍ତର୍ଭାଵ "V3Mutex.h"
#ଅନ୍ତର୍ଭାଵ "V3Options.h"

#ଅନ୍ତର୍ଭାଵ <string>
#ଅନ୍ତର୍ଭାଵ <thread>
#ଅନ୍ତର୍ଭାଵ <type_traits>
#ଅନ୍ତର୍ଭାଵ <unordered_map>
#ଅନ୍ତର୍ଭାଵ <unordered_set>

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
    ଅଚଳ VRestorerTrivial<typename std::decay_t<decltype(var)>> restorer_##var(var);

// This is the equivalent of VL_RESTORER for non-trivially copyable types. Still more efficient
// than VL_RESTORER as uses move to restore.
#define VL_RESTORER_COPY(var) \
    ଅଚଳ VRestorerCopy<typename std::decay_t<decltype(var)>> restorer_##var(var);

// Swap 'var' with a no-args constructed object of the same type, effectively clearing it, then
// swap back at end-of-scope. No copying involved. Use this e.g. for std containers where they
// would be .clear()'d right after the VL_RESTORER instance.
#define VL_RESTORER_CLEAR(var) \
    ଅଚଳ VRestorerClear<typename std::decay_t<decltype(var)>> restorer_##var(var);

// Get const reference to the saved copy
#define VL_RESTORER_PREV(var) restorer_##var.saved()

// Implementation of VL_RESTORER
template <typename T>
class VRestorerTrivial final {
    static_assert(std::is_trivially_copyable<T>::ମୂଲ୍ୟ,
                  "Use VL_RESTORER_{COPY,CLEAR} for non trivially copyable types");
    T& m_ref;  // Reference to object we're saving and restoring
    ଅଚଳ T m_saved;  // Value saved, for later restore

ସର୍ବସାଧାରଣ:
    explicit VRestorerTrivial(T& val)
        : m_ref{val}
        , m_saved{val} {}
    ~VRestorerTrivial() { m_ref = m_saved; }
    VL_UNCOPYABLE(VRestorerTrivial);
    // Must be stack allocated
    ଶୂନ୍ୟ* operator new(size_t) = delete;
    ଶୂନ୍ୟ operator delete(ଶୂନ୍ୟ*) = delete;

    ଅଚଳ T& saved() ଅଚଳ { ଫେରନ୍ତୁ m_saved; }
};

// Implementation of VL_RESTORER_COPY
template <typename T>
class VRestorerCopy final {
    static_assert(!std::is_trivially_copyable<T>::ମୂଲ୍ୟ,
                  "Use VL_RESTORER for trivially copyable types");
    T& m_ref;  // Reference to object we're saving and restoring
    T m_saved;  // Value saved, for later restore

ସର୍ବସାଧାରଣ:
    explicit VRestorerCopy(T& val)
        : m_ref{val}
        , m_saved{val} {}
    ~VRestorerCopy() { m_ref = std::move(m_saved); }
    VL_UNCOPYABLE(VRestorerCopy);
    // Must be stack allocated
    ଶୂନ୍ୟ* operator new(size_t) = delete;
    ଶୂନ୍ୟ operator delete(ଶୂନ୍ୟ*) = delete;

    ଅଚଳ T& saved() ଅଚଳ { ଫେରନ୍ତୁ m_saved; }
};

// Implementation of VL_RESTORER_CLEAR.
template <typename T>
class VRestorerClear final {
    static_assert(!std::is_trivially_copyable<T>::ମୂଲ୍ୟ,
                  "Use VL_RESTORER for trivially copyable types");
    T& m_ref;  // Reference to object we're saving and restoring
    T m_saved{};  // Owns the swapped-out value; starts empty

ସର୍ବସାଧାରଣ:
    explicit VRestorerClear(T& val)
        : m_ref{val} {
        std::swap(m_ref, m_saved);
    }
    ~VRestorerClear() { std::swap(m_ref, m_saved); }
    VL_UNCOPYABLE(VRestorerClear);
    // Must be stack allocated
    ଶୂନ୍ୟ* operator new(size_t) = delete;
    ଶୂନ୍ୟ operator delete(ଶୂନ୍ୟ*) = delete;

    ଅଚଳ T& saved() ଅଚଳ { ଫେରନ୍ତୁ m_saved; }
};

//######################################################################

class VWidthMinUsage final {
ସର୍ବସାଧାରଣ:
    ଗଣନା en : uint8_t { LINT_WIDTH, MATCHES_WIDTH, VERILOG_WIDTH };
    ଗଣନା en m_e;
    VWidthMinUsage()
        : m_e{LINT_WIDTH} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VWidthMinUsage(en _e) VL_PURE : m_e{_e} {}
    constexpr VWidthMinUsage(ଅଚଳ VWidthMinUsage& _e) VL_PURE = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    explicit VWidthMinUsage(ପୂର୍ଣ୍ଣସଂଖ୍ୟା _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr operator en() ଅଚଳ { ଫେରନ୍ତୁ m_e; }
    constexpr VWidthMinUsage& operator=(ଅଚଳ VWidthMinUsage& _e) VL_PURE = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};
constexpr bool operator==(ଅଚଳ VWidthMinUsage& lhs, ଅଚଳ VWidthMinUsage& rhs) {
    ଫେରନ୍ତୁ lhs.m_e == rhs.m_e;
}
constexpr bool operator==(ଅଚଳ VWidthMinUsage& lhs, VWidthMinUsage::en rhs) VL_PURE {
    ଫେରନ୍ତୁ lhs.m_e == rhs;
}
constexpr bool operator==(VWidthMinUsage::en lhs, ଅଚଳ VWidthMinUsage& rhs) {
    ଫେରନ୍ତୁ lhs == rhs.m_e;
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

    std::atomic_int m_debug_File_ସଂଖ୍ୟା{0};  // Number to append to debug files created
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
    std::unordered_map<ଅଚଳ ଶୂନ୍ୟ*, std::string>
        m_ptrToId;  // The actual 'address' <=> 'short string' bijection

    // Names of fields that were dumped by dumpJsonPtr()
    std::unordered_set<std::string> m_jsonPtrNames;

    // Id of the main thread
    ଅଚଳ std::thread::id m_mainThreadId = std::this_thread::get_id();

ସର୍ବସାଧାରଣ:
    // Options
    V3Options opt;  // All options; let user see them directly

    // CONSTRUCTORS
    V3Global() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ଶୂନ୍ୟ boot();
    ଶୂନ୍ୟ shutdown();  // Release allocated resources

    ଶୂନ୍ୟ vlExit(ପୂର୍ଣ୍ଣସଂଖ୍ୟା status);

    // ACCESSORS (general)
    AstNetlist* rootp() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_rootp; }
    // Use V3ConstPool:: static methods instead
    V3ConstPool* constPoolp() ଅଚଳ VL_PURE { ଫେରନ୍ତୁ m_constPoolp; }
    V3LibMap* libMapp() ଅଚଳ VL_PURE { ଫେରନ୍ତୁ m_libMapp; }
    V3ThreadPool* threadPoolp() ଅଚଳ VL_PURE { ଫେରନ୍ତୁ m_threadPoolp; }
    ଶୂନ୍ୟ threadPoolp(V3ThreadPool* threadPoolp) {
        UASSERT(!m_threadPoolp, "attempted to create multiple threadPool singletons");
        m_threadPoolp = threadPoolp;
    }
    VWidthMinUsage widthMinUsage() ଅଚଳ VL_PURE { ଫେରନ୍ତୁ m_widthMinUsage; }
    bool assertDTypesResolved() ଅଚଳ { ଫେରନ୍ତୁ m_assertDTypesResolved; }
    bool assertScoped() ଅଚଳ { ଫେରନ୍ତୁ m_assertScoped; }
    bool staticsLifted() ଅଚଳ { ଫେରନ୍ତୁ m_staticsLifted; }

    // METHODS
    ଶୂନ୍ୟ readFiles() VL_MT_DISABLED;
    ଶୂନ୍ୟ removeStd() VL_MT_DISABLED;
    ଶୂନ୍ୟ check_ବୃକ୍ଷ() ଅଚଳ;
    ସ୍ଥିର ଶୂନ୍ୟ dump_Check_Global_ବୃକ୍ଷ(ଅଚଳ string& stagename, ପୂର୍ଣ୍ଣସଂଖ୍ୟା new_ସଂଖ୍ୟା = 0, bool doDump = true,
                                    bool doCheck = true);
    ଶୂନ୍ୟ assertDTypesResolved(bool flag) { m_assertDTypesResolved = flag; }
    ଶୂନ୍ୟ assertScoped(bool flag) { m_assertScoped = flag; }
    ଶୂନ୍ୟ staticsLifted(bool flag) { m_staticsLifted = flag; }
    ଶୂନ୍ୟ widthMinUsage(ଅଚଳ VWidthMinUsage& flag) { m_widthMinUsage = flag; }
    bool constRemoveXs() ଅଚଳ { ଫେରନ୍ତୁ m_constRemoveXs; }
    ଶୂନ୍ୟ constRemoveXs(bool flag) { m_constRemoveXs = flag; }
    string debugFilename(ଅଚଳ string& nameComment, ପୂର୍ଣ୍ଣସଂଖ୍ୟା new_ସଂଖ୍ୟା = 0);
    ସ୍ଥିର string digitsFilename(ପୂର୍ଣ୍ଣସଂଖ୍ୟା ସଂଖ୍ୟା);
    bool needTraceDumper() ଅଚଳ { ଫେରନ୍ତୁ m_needTraceDumper; }
    ଶୂନ୍ୟ needTraceDumper(bool flag) { m_needTraceDumper = flag; }
    bool dpi() ଅଚଳ VL_MT_SAFE { ଫେରନ୍ତୁ m_dpi; }
    ଶୂନ୍ୟ dpi(bool flag) { m_dpi = flag; }
    bool assignsEvents() ଅଚଳ { ଫେରନ୍ତୁ m_assignsEvents; }
    ଶୂନ୍ୟ setAssignsEvents() { m_assignsEvents = true; }
    bool hasEvents() ଅଚଳ { ଫେରନ୍ତୁ m_hasEvents; }
    ଶୂନ୍ୟ setHasEvents() { m_hasEvents = true; }
    bool hasClasses() ଅଚଳ { ଫେରନ୍ତୁ m_hasClasses; }
    ଶୂନ୍ୟ setHasClasses() { m_hasClasses = true; }
    bool hasSampled() ଅଚଳ { ଫେରନ୍ତୁ m_hasSampled; }
    ଶୂନ୍ୟ setHasSampled() { m_hasSampled = true; }
    bool hasTable() ଅଚଳ { ଫେରନ୍ତୁ m_hasTable; }
    ଶୂନ୍ୟ setHasTable() { m_hasTable = true; }
    bool hasVirtIfaces() ଅଚଳ { ଫେରନ୍ତୁ m_hasVirtIfaces; }
    ଶୂନ୍ୟ setHasVirtIfaces() { m_hasVirtIfaces = true; }
    bool usesProbDist() ଅଚଳ { ଫେରନ୍ତୁ m_usesProbDist; }
    ଶୂନ୍ୟ setUsesProbDist() { m_usesProbDist = true; }
    bool usesStdPackage() ଅଚଳ { ଫେରନ୍ତୁ m_usesStdPackage; }
    ଶୂନ୍ୟ setUsesStdPackage() { m_usesStdPackage = true; }
    bool usesTiming() ଅଚଳ { ଫେରନ୍ତୁ m_usesTiming; }
    ଶୂନ୍ୟ setUsesTiming() { m_usesTiming = true; }
    bool usesZeroDelay() ଅଚଳ { ଫେରନ୍ତୁ m_usesZeroDelay; }
    ଶୂନ୍ୟ setUsesZeroDelay() { m_usesZeroDelay = true; }
    bool hasForceableSignals() ଅଚଳ { ଫେରନ୍ତୁ m_hasForceableSignals; }
    ଶୂନ୍ୟ setHasForceableSignals() { m_hasForceableSignals = true; }
    bool hasAssignDeassign() ଅଚଳ { ଫେରନ୍ତୁ m_hasAssignDeassign; }
    ଶୂନ୍ୟ setHasAssignDeassign() { m_hasAssignDeassign = true; }
    bool usesForce() ଅଚଳ { ଫେରନ୍ତୁ m_usesForce; }
    ଶୂନ୍ୟ setUsesForce() { m_usesForce = true; }
    V3HierGraph* hierGraphp() ଅଚଳ { ଫେରନ୍ତୁ m_hierGraphp; }
    ଶୂନ୍ୟ hierGraphp(V3HierGraph* graphp) { m_hierGraphp = graphp; }
    bool useParallelBuild() ଅଚଳ { ଫେରନ୍ତୁ m_useParallelBuild; }
    ଶୂନ୍ୟ useParallelBuild(bool flag) { m_useParallelBuild = flag; }
    bool useRandSequence() ଅଚଳ { ଫେରନ୍ତୁ m_useRandSequence; }
    ଶୂନ୍ୟ useRandSequence(bool flag) { m_useRandSequence = flag; }
    bool useCovergroup() ଅଚଳ { ଫେରନ୍ତୁ m_useCovergroup; }
    ଶୂନ୍ୟ useCovergroup(bool flag) { m_useCovergroup = flag; }
    bool useRandomizeMethods() ଅଚଳ { ଫେରନ୍ତୁ m_useRandomizeMethods; }
    ଶୂନ୍ୟ useRandomizeMethods(bool flag) { m_useRandomizeMethods = flag; }
    bool hasPrintedObjects() ଅଚଳ { ଫେରନ୍ତୁ m_hasPrintedObjects; }
    ଶୂନ୍ୟ hasPrintedObjects(bool flag) { m_hasPrintedObjects = flag; }
    ଶୂନ୍ୟ saveJsonPtrFieldName(ଅଚଳ std::string& fieldName);
    ଶୂନ୍ୟ ptrNamesDumpJson(std::ostream& os);
    ଶୂନ୍ୟ idPtrMapDumpJson(std::ostream& os);
    ଅଚଳ std::string& ptrToId(ଅଚଳ ଶୂନ୍ୟ* p);
    std::thread::id mainThreadId() ଅଚଳ { ଫେରନ୍ତୁ m_mainThreadId; }
    ସ୍ଥିର std::vector<std::string> verilatedCppFiles();
    uint64_t currentHierBlockCost() ଅଚଳ { ଫେରନ୍ତୁ m_currentHierBlockCost; }
    ଶୂନ୍ୟ currentHierBlockCost(uint64_t cost) { m_currentHierBlockCost = cost; }
};

ବାହ୍ୟ V3Global v3Global;

//######################################################################

#endif  // guard
