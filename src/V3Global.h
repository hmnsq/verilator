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

#अगर_अघोषित VERILATOR_V3GLOBAL_H_
#घोषणा VERILATOR_V3GLOBAL_H_

// clang-format off
#समावेश "config_build.h"
#अगर_अघोषित HAVE_CONFIG_PACKAGE
# error "Something failed during ./configure as config_package.h is incomplete. Perhaps you used autoreconf, don't."
#पूर्ण_अगर
// clang-format on

#समावेश "verilatedos.h"

#समावेश "V3Error.h"
#समावेश "V3FileLine.h"
#समावेश "V3Mutex.h"
#समावेश "V3Options.h"

#समावेश <string>
#समावेश <thread>
#समावेश <type_traits>
#समावेश <unordered_map>
#समावेश <unordered_set>

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
#घोषणा VL_RESTORER(var) \
    स्थिर VRestorerTrivial<typename std::decay_t<decltype(var)>> restorer_##var(var);

// This is the equivalent of VL_RESTORER for non-trivially copyable types. Still more efficient
// than VL_RESTORER as uses move to restore.
#घोषणा VL_RESTORER_COPY(var) \
    स्थिर VRestorerCopy<typename std::decay_t<decltype(var)>> restorer_##var(var);

// Swap 'var' with a no-args constructed object of the same type, effectively clearing it, then
// swap back at end-of-scope. No copying involved. Use this e.g. for std containers where they
// would be .clear()'d right after the VL_RESTORER instance.
#घोषणा VL_RESTORER_CLEAR(var) \
    स्थिर VRestorerClear<typename std::decay_t<decltype(var)>> restorer_##var(var);

// Get const reference to the saved copy
#घोषणा VL_RESTORER_पूर्व(var) restorer_##var.saved()

// Implementation of VL_RESTORER
ढाँचा <typename T>
class VRestorerTrivial final {
    static_assert(std::is_trivially_copyable<T>::मूल्य,
                  "Use VL_RESTORER_{COPY,CLEAR} for non trivially copyable types");
    T& m_ref;  // Reference to object we're saving and restoring
    स्थिर T m_saved;  // Value saved, for later restore

खुला:
    explicit VRestorerTrivial(T& val)
        : m_ref{val}
        , m_saved{val} {}
    ~VRestorerTrivial() { m_ref = m_saved; }
    VL_UNCOPYABLE(VRestorerTrivial);
    // Must be stack allocated
    व्योम* चालक new(माप_प्रकार) = delete;
    व्योम चालक delete(व्योम*) = delete;

    स्थिर T& saved() स्थिर { वापस m_saved; }
};

// Implementation of VL_RESTORER_COPY
ढाँचा <typename T>
class VRestorerCopy final {
    static_assert(!std::is_trivially_copyable<T>::मूल्य,
                  "Use VL_RESTORER for trivially copyable types");
    T& m_ref;  // Reference to object we're saving and restoring
    T m_saved;  // Value saved, for later restore

खुला:
    explicit VRestorerCopy(T& val)
        : m_ref{val}
        , m_saved{val} {}
    ~VRestorerCopy() { m_ref = std::move(m_saved); }
    VL_UNCOPYABLE(VRestorerCopy);
    // Must be stack allocated
    व्योम* चालक new(माप_प्रकार) = delete;
    व्योम चालक delete(व्योम*) = delete;

    स्थिर T& saved() स्थिर { वापस m_saved; }
};

// Implementation of VL_RESTORER_CLEAR.
ढाँचा <typename T>
class VRestorerClear final {
    static_assert(!std::is_trivially_copyable<T>::मूल्य,
                  "Use VL_RESTORER for trivially copyable types");
    T& m_ref;  // Reference to object we're saving and restoring
    T m_saved{};  // Owns the swapped-out value; starts empty

खुला:
    explicit VRestorerClear(T& val)
        : m_ref{val} {
        std::swap(m_ref, m_saved);
    }
    ~VRestorerClear() { std::swap(m_ref, m_saved); }
    VL_UNCOPYABLE(VRestorerClear);
    // Must be stack allocated
    व्योम* चालक new(माप_प्रकार) = delete;
    व्योम चालक delete(व्योम*) = delete;

    स्थिर T& saved() स्थिर { वापस m_saved; }
};

//######################################################################

class VWidthMinUsage final {
खुला:
    क्रमागत en : uint8_t { LINT_WIDTH, MATCHES_WIDTH, VERILOG_WIDTH };
    क्रमागत en m_e;
    VWidthMinUsage()
        : m_e{LINT_WIDTH} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VWidthMinUsage(en _e) VL_PURE : m_e{_e} {}
    constexpr VWidthMinUsage(स्थिर VWidthMinUsage& _e) VL_PURE = शेष;
    explicit VWidthMinUsage(पूर्णांक _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr चालक en() स्थिर { वापस m_e; }
    constexpr VWidthMinUsage& चालक=(स्थिर VWidthMinUsage& _e) VL_PURE = शेष;
};
constexpr bool चालक==(स्थिर VWidthMinUsage& lhs, स्थिर VWidthMinUsage& rhs) {
    वापस lhs.m_e == rhs.m_e;
}
constexpr bool चालक==(स्थिर VWidthMinUsage& lhs, VWidthMinUsage::en rhs) VL_PURE {
    वापस lhs.m_e == rhs;
}
constexpr bool चालक==(VWidthMinUsage::en lhs, स्थिर VWidthMinUsage& rhs) {
    वापस lhs == rhs.m_e;
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

    std::atomic_int m_debug_File_संख्या{0};  // Number to append to debug files created
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
    std::unordered_map<स्थिर व्योम*, std::string>
        m_ptrToId;  // The actual 'address' <=> 'short string' bijection

    // Names of fields that were dumped by dumpJsonPtr()
    std::unordered_set<std::string> m_jsonPtrNames;

    // Id of the main thread
    स्थिर std::thread::id m_mainThreadId = std::this_thread::get_id();

खुला:
    // Options
    V3Options opt;  // All options; let user see them directly

    // CONSTRUCTORS
    V3Global() = शेष;
    व्योम boot();
    व्योम shutdown();  // Release allocated resources

    व्योम vlExit(पूर्णांक status);

    // ACCESSORS (general)
    AstNetlist* rootp() स्थिर VL_MT_SAFE { वापस m_rootp; }
    // Use V3ConstPool:: static methods instead
    V3ConstPool* constPoolp() स्थिर VL_PURE { वापस m_constPoolp; }
    V3LibMap* libMapp() स्थिर VL_PURE { वापस m_libMapp; }
    V3ThreadPool* threadPoolp() स्थिर VL_PURE { वापस m_threadPoolp; }
    व्योम threadPoolp(V3ThreadPool* threadPoolp) {
        UASSERT(!m_threadPoolp, "attempted to create multiple threadPool singletons");
        m_threadPoolp = threadPoolp;
    }
    VWidthMinUsage widthMinUsage() स्थिर VL_PURE { वापस m_widthMinUsage; }
    bool assertDTypesResolved() स्थिर { वापस m_assertDTypesResolved; }
    bool assertScoped() स्थिर { वापस m_assertScoped; }
    bool staticsLifted() स्थिर { वापस m_staticsLifted; }

    // METHODS
    व्योम readFiles() VL_MT_DISABLED;
    व्योम removeStd() VL_MT_DISABLED;
    व्योम check_वृक्ष() स्थिर;
    अटल व्योम dump_Check_Global_वृक्ष(स्थिर string& stagename, पूर्णांक new_संख्या = 0, bool doDump = true,
                                    bool doCheck = true);
    व्योम assertDTypesResolved(bool flag) { m_assertDTypesResolved = flag; }
    व्योम assertScoped(bool flag) { m_assertScoped = flag; }
    व्योम staticsLifted(bool flag) { m_staticsLifted = flag; }
    व्योम widthMinUsage(स्थिर VWidthMinUsage& flag) { m_widthMinUsage = flag; }
    bool constRemoveXs() स्थिर { वापस m_constRemoveXs; }
    व्योम constRemoveXs(bool flag) { m_constRemoveXs = flag; }
    string debugFilename(स्थिर string& nameComment, पूर्णांक new_संख्या = 0);
    अटल string digitsFilename(पूर्णांक संख्या);
    bool needTraceDumper() स्थिर { वापस m_needTraceDumper; }
    व्योम needTraceDumper(bool flag) { m_needTraceDumper = flag; }
    bool dpi() स्थिर VL_MT_SAFE { वापस m_dpi; }
    व्योम dpi(bool flag) { m_dpi = flag; }
    bool assignsEvents() स्थिर { वापस m_assignsEvents; }
    व्योम setAssignsEvents() { m_assignsEvents = true; }
    bool hasEvents() स्थिर { वापस m_hasEvents; }
    व्योम setHasEvents() { m_hasEvents = true; }
    bool hasClasses() स्थिर { वापस m_hasClasses; }
    व्योम setHasClasses() { m_hasClasses = true; }
    bool hasSampled() स्थिर { वापस m_hasSampled; }
    व्योम setHasSampled() { m_hasSampled = true; }
    bool hasTable() स्थिर { वापस m_hasTable; }
    व्योम setHasTable() { m_hasTable = true; }
    bool hasVirtIfaces() स्थिर { वापस m_hasVirtIfaces; }
    व्योम setHasVirtIfaces() { m_hasVirtIfaces = true; }
    bool usesProbDist() स्थिर { वापस m_usesProbDist; }
    व्योम setUsesProbDist() { m_usesProbDist = true; }
    bool usesStdPackage() स्थिर { वापस m_usesStdPackage; }
    व्योम setUsesStdPackage() { m_usesStdPackage = true; }
    bool usesTiming() स्थिर { वापस m_usesTiming; }
    व्योम setUsesTiming() { m_usesTiming = true; }
    bool usesZeroDelay() स्थिर { वापस m_usesZeroDelay; }
    व्योम setUsesZeroDelay() { m_usesZeroDelay = true; }
    bool hasForceableSignals() स्थिर { वापस m_hasForceableSignals; }
    व्योम setHasForceableSignals() { m_hasForceableSignals = true; }
    bool hasAssignDeassign() स्थिर { वापस m_hasAssignDeassign; }
    व्योम setHasAssignDeassign() { m_hasAssignDeassign = true; }
    bool usesForce() स्थिर { वापस m_usesForce; }
    व्योम setUsesForce() { m_usesForce = true; }
    V3HierGraph* hierGraphp() स्थिर { वापस m_hierGraphp; }
    व्योम hierGraphp(V3HierGraph* graphp) { m_hierGraphp = graphp; }
    bool useParallelBuild() स्थिर { वापस m_useParallelBuild; }
    व्योम useParallelBuild(bool flag) { m_useParallelBuild = flag; }
    bool useRandSequence() स्थिर { वापस m_useRandSequence; }
    व्योम useRandSequence(bool flag) { m_useRandSequence = flag; }
    bool useCovergroup() स्थिर { वापस m_useCovergroup; }
    व्योम useCovergroup(bool flag) { m_useCovergroup = flag; }
    bool useRandomizeMethods() स्थिर { वापस m_useRandomizeMethods; }
    व्योम useRandomizeMethods(bool flag) { m_useRandomizeMethods = flag; }
    bool hasPrintedObjects() स्थिर { वापस m_hasPrintedObjects; }
    व्योम hasPrintedObjects(bool flag) { m_hasPrintedObjects = flag; }
    व्योम saveJsonPtrFieldName(स्थिर std::string& fieldName);
    व्योम ptrNamesDumpJson(std::ostream& os);
    व्योम idPtrMapDumpJson(std::ostream& os);
    स्थिर std::string& ptrToId(स्थिर व्योम* p);
    std::thread::id mainThreadId() स्थिर { वापस m_mainThreadId; }
    अटल std::vector<std::string> verilatedCppFiles();
    uint64_t currentHierBlockCost() स्थिर { वापस m_currentHierBlockCost; }
    व्योम currentHierBlockCost(uint64_t cost) { m_currentHierBlockCost = cost; }
};

बाह्य V3Global v3Global;

//######################################################################

#पूर्ण_अगर  // guard
