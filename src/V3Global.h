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

#నిర్వచితం_కాకపోతే VERILATOR_V3GLOBAL_H_
#నిర్వచించు VERILATOR_V3GLOBAL_H_

// clang-format off
#అన్తర్భావ "config_build.h"
#నిర్వచితం_కాకపోతే HAVE_CONFIG_PACKAGE
# error "Something failed during ./configure as config_package.h is incomplete. Perhaps you used autoreconf, don't."
#అయితే_ముగింపు
// clang-format on

#అన్తర్భావ "verilatedos.h"

#అన్తర్భావ "V3Error.h"
#అన్తర్భావ "V3FileLine.h"
#అన్తర్భావ "V3Mutex.h"
#అన్తర్భావ "V3Options.h"

#అన్తర్భావ <string>
#అన్తర్భావ <thread>
#అన్తర్భావ <type_traits>
#అన్తర్భావ <unordered_map>
#అన్తర్భావ <unordered_set>

class AstNetlist;
class V3ConstPool;
class V3_Hier_లేఖాచిత్రం;
class V3LibMap;
class V3ThreadPool;

//======================================================================
// Restorer

// For all flavours, 'var' must be a named scalar variable (simple identifier).

// Copy given variable's value on the stack, copy saved value back at end-of-scope.
// Only usable for trivially copyable types; use VL_RESTORER_COPY/VL_RESTORER_CLEAR for
// non-trivially copyable types, which are more efficient.
#నిర్వచించు VL_RESTORER(var) \
    స్థిరరాశి VRestorerTrivial<typename std::decay_t<decltype(var)>> restorer_##var(var);

// This is the equivalent of VL_RESTORER for non-trivially copyable types. Still more efficient
// than VL_RESTORER as uses move to restore.
#నిర్వచించు VL_RESTORER_COPY(var) \
    స్థిరరాశి VRestorerCopy<typename std::decay_t<decltype(var)>> restorer_##var(var);

// Swap 'var' with a no-args constructed object of the same type, effectively clearing it, then
// swap back at end-of-scope. No copying involved. Use this e.g. for std containers where they
// would be .clear()'d right after the VL_RESTORER instance.
#నిర్వచించు VL_RESTORER_CLEAR(var) \
    స్థిరరాశి VRestorerClear<typename std::decay_t<decltype(var)>> restorer_##var(var);

// Get const reference to the saved copy
#నిర్వచించు VL_RESTORER_PREV(var) restorer_##var.saved()

// Implementation of VL_RESTORER
మూస <typename T>
class VRestorerTrivial final {
    static_assert(std::is_trivially_copyable<T>::విలువ,
                  "Use VL_RESTORER_{COPY,CLEAR} for non trivially copyable types");
    T& m_ref;  // Reference to object we're saving and restoring
    స్థిరరాశి T m_saved;  // Value saved, for later restore

బహిరంగ:
    explicit VRestorerTrivial(T& val)
        : m_ref{val}
        , m_saved{val} {}
    ~VRestorerTrivial() { m_ref = m_saved; }
    VL_UNCOPYABLE(VRestorerTrivial);
    // Must be stack allocated
    శూన్య* క్రియాకారకం new(కొలత_t) = delete;
    శూన్య క్రియాకారకం delete(శూన్య*) = delete;

    స్థిరరాశి T& saved() స్థిరరాశి { తిరిగి m_saved; }
};

// Implementation of VL_RESTORER_COPY
మూస <typename T>
class VRestorerCopy final {
    static_assert(!std::is_trivially_copyable<T>::విలువ,
                  "Use VL_RESTORER for trivially copyable types");
    T& m_ref;  // Reference to object we're saving and restoring
    T m_saved;  // Value saved, for later restore

బహిరంగ:
    explicit VRestorerCopy(T& val)
        : m_ref{val}
        , m_saved{val} {}
    ~VRestorerCopy() { m_ref = std::move(m_saved); }
    VL_UNCOPYABLE(VRestorerCopy);
    // Must be stack allocated
    శూన్య* క్రియాకారకం new(కొలత_t) = delete;
    శూన్య క్రియాకారకం delete(శూన్య*) = delete;

    స్థిరరాశి T& saved() స్థిరరాశి { తిరిగి m_saved; }
};

// Implementation of VL_RESTORER_CLEAR.
మూస <typename T>
class VRestorerClear final {
    static_assert(!std::is_trivially_copyable<T>::విలువ,
                  "Use VL_RESTORER for trivially copyable types");
    T& m_ref;  // Reference to object we're saving and restoring
    T m_saved{};  // Owns the swapped-out value; starts empty

బహిరంగ:
    explicit VRestorerClear(T& val)
        : m_ref{val} {
        std::మార్పిడి(m_ref, m_saved);
    }
    ~VRestorerClear() { std::మార్పిడి(m_ref, m_saved); }
    VL_UNCOPYABLE(VRestorerClear);
    // Must be stack allocated
    శూన్య* క్రియాకారకం new(కొలత_t) = delete;
    శూన్య క్రియాకారకం delete(శూన్య*) = delete;

    స్థిరరాశి T& saved() స్థిరరాశి { తిరిగి m_saved; }
};

//######################################################################

class VWidthMinUsage final {
బహిరంగ:
    గణన en : uint8_t { LINT_వెడల్పు, MATCHES_వెడల్పు, VERILOG_వెడల్పు };
    గణన en m_e;
    VWidthMinUsage()
        : m_e{LINT_వెడల్పు} {}
    // cppcheck-suppress noExplicitConstructor
    constexpr VWidthMinUsage(en _e) VL_శుద్ధ : m_e{_e} {}
    constexpr VWidthMinUsage(స్థిరరాశి VWidthMinUsage& _e) VL_శుద్ధ = అప్రమేయ;
    explicit VWidthMinUsage(పూర్ణాంకం _e)
        : m_e(static_cast<en>(_e)) {}  // Need () or GCC 4.8 false warning
    constexpr క్రియాకారకం en() స్థిరరాశి { తిరిగి m_e; }
    constexpr VWidthMinUsage& క్రియాకారకం=(స్థిరరాశి VWidthMinUsage& _e) VL_శుద్ధ = అప్రమేయ;
};
constexpr bool క్రియాకారకం==(స్థిరరాశి VWidthMinUsage& lhs, స్థిరరాశి VWidthMinUsage& rhs) {
    తిరిగి lhs.m_e == rhs.m_e;
}
constexpr bool క్రియాకారకం==(స్థిరరాశి VWidthMinUsage& lhs, VWidthMinUsage::en rhs) VL_శుద్ధ {
    తిరిగి lhs.m_e == rhs;
}
constexpr bool క్రియాకారకం==(VWidthMinUsage::en lhs, స్థిరరాశి VWidthMinUsage& rhs) {
    తిరిగి lhs == rhs.m_e;
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
    V3_Hier_లేఖాచిత్రం* m_hierGraphp = nullptr;
    // Thread Pool, nullptr unless 'verilatedJobs' is known, set via threadPoolp(V3ThreadPool*)
    V3ThreadPool* m_threadPoolp = nullptr;
    // Library Mapping, nullptr unless --libmap is used
    V3LibMap* m_libMapp = nullptr;
    VWidthMinUsage m_వెడల్పు_Min_Usage
        = VWidthMinUsage::LINT_వెడల్పు;  // What AstNode::widthMin() is used for

    std::atomic_int m_debug_File_సంఖ్య{0};  // Number to append to debug files created
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
    bool m_has_పట్టిక = false;  // Desgin has the UDP Table.
    bool m_hasVirtIfaces = false;  // Design uses virtual interfaces
    bool m_ఉపయోగిస్తుంది_Prob_Dist = false;  // Uses $dist_*
    bool m_ఉపయోగిస్తుంది_Std_Package = false;  // Design uses the std package
    bool m_ఉపయోగిస్తుంది_Timing = false;  // Design uses timing constructs
    bool m_ఉపయోగిస్తుంది_Force = false;  // Design uses force/release statements
    bool m_ఉపయోగిస్తుంది_Zero_Delay = false;  // Design uses #0 delay (or non-constant delay)
    bool m_has_Forceable_సంకేతాలు = false;  // Need to apply V3Force pass
    bool m_has_కేటాయించు_Deassign = false;  // Need to apply V3Force pass for assign/deassign statements
    bool m_useParallelBuild = false;  // Use parallel build for model
    bool m_useRandSequence = false;  // Has `randsequence`
    bool m_useCovergroup = false;  // Has covergroup declarations
    bool m_useRandomizeMethods = false;  // Need to define randomize() class methods
    bool m_hasPrintedObjects = false;  // Design has format args printed with to_string()
    uint64_t m_current_Hier_ఖండం_ఖర్చు = 0;  // Total cost of this hier block, used for scheduling

    // Memory address to short string mapping (for debug)
    std::unordered_map<స్థిరరాశి శూన్య*, std::string>
        m_ptr_గమ్యం_Id;  // The actual 'address' <=> 'short string' bijection

    // Names of fields that were dumped by dumpJsonPtr()
    std::unordered_సమితి<std::string> m_json_Ptr_పేర్లు;

    // Id of the main thread
    స్థిరరాశి std::thread::id m_mainThreadId = std::this_thread::get_id();

బహిరంగ:
    // Options
    V3Options opt;  // All options; let user see them directly

    // CONSTRUCTORS
    V3Global() = అప్రమేయ;
    శూన్య boot();
    శూన్య shutdown();  // Release allocated resources

    శూన్య vlExit(పూర్ణాంకం స్థితి_2);

    // ACCESSORS (general)
    AstNetlist* rootp() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_rootp; }
    // Use V3ConstPool:: static methods instead
    V3ConstPool* constPoolp() స్థిరరాశి VL_శుద్ధ { తిరిగి m_constPoolp; }
    V3LibMap* libMapp() స్థిరరాశి VL_శుద్ధ { తిరిగి m_libMapp; }
    V3ThreadPool* threadPoolp() స్థిరరాశి VL_శుద్ధ { తిరిగి m_threadPoolp; }
    శూన్య threadPoolp(V3ThreadPool* threadPoolp) {
        UASSERT(!m_threadPoolp, "attempted to create multiple threadPool singletons");
        m_threadPoolp = threadPoolp;
    }
    VWidthMinUsage వెడల్పు_Min_Usage() స్థిరరాశి VL_శుద్ధ { తిరిగి m_వెడల్పు_Min_Usage; }
    bool assertDTypesResolved() స్థిరరాశి { తిరిగి m_assertDTypesResolved; }
    bool assertScoped() స్థిరరాశి { తిరిగి m_assertScoped; }
    bool staticsLifted() స్థిరరాశి { తిరిగి m_staticsLifted; }

    // METHODS
    శూన్య readFiles() VL_MT_DISABLED;
    శూన్య removeStd() VL_MT_DISABLED;
    శూన్య check_వృక్షం() స్థిరరాశి;
    స్థిర శూన్య dump_Check_Global_వృక్షం(స్థిరరాశి string& stagename, పూర్ణాంకం new_సంఖ్య = 0, bool doDump = true,
                                    bool doCheck = true);
    శూన్య assertDTypesResolved(bool flag) { m_assertDTypesResolved = flag; }
    శూన్య assertScoped(bool flag) { m_assertScoped = flag; }
    శూన్య staticsLifted(bool flag) { m_staticsLifted = flag; }
    శూన్య వెడల్పు_Min_Usage(స్థిరరాశి VWidthMinUsage& flag) { m_వెడల్పు_Min_Usage = flag; }
    bool constRemoveXs() స్థిరరాశి { తిరిగి m_constRemoveXs; }
    శూన్య constRemoveXs(bool flag) { m_constRemoveXs = flag; }
    string debugFilename(స్థిరరాశి string& nameComment, పూర్ణాంకం new_సంఖ్య = 0);
    స్థిర string digitsFilename(పూర్ణాంకం సంఖ్య);
    bool needTraceDumper() స్థిరరాశి { తిరిగి m_needTraceDumper; }
    శూన్య needTraceDumper(bool flag) { m_needTraceDumper = flag; }
    bool dpi() స్థిరరాశి VL_MT_సురక్షిత { తిరిగి m_dpi; }
    శూన్య dpi(bool flag) { m_dpi = flag; }
    bool assignsEvents() స్థిరరాశి { తిరిగి m_assignsEvents; }
    శూన్య సమితి_Assigns_Events() { m_assignsEvents = true; }
    bool hasEvents() స్థిరరాశి { తిరిగి m_hasEvents; }
    శూన్య సమితి_Has_Events() { m_hasEvents = true; }
    bool hasClasses() స్థిరరాశి { తిరిగి m_hasClasses; }
    శూన్య సమితి_Has_Classes() { m_hasClasses = true; }
    bool hasSampled() స్థిరరాశి { తిరిగి m_hasSampled; }
    శూన్య సమితి_Has_Sampled() { m_hasSampled = true; }
    bool has_పట్టిక() స్థిరరాశి { తిరిగి m_has_పట్టిక; }
    శూన్య సమితి_Has_పట్టిక() { m_has_పట్టిక = true; }
    bool hasVirtIfaces() స్థిరరాశి { తిరిగి m_hasVirtIfaces; }
    శూన్య సమితి_Has_Virt_Ifaces() { m_hasVirtIfaces = true; }
    bool ఉపయోగిస్తుంది_Prob_Dist() స్థిరరాశి { తిరిగి m_ఉపయోగిస్తుంది_Prob_Dist; }
    శూన్య సమితి_ఉపయోగిస్తుంది_Prob_Dist() { m_ఉపయోగిస్తుంది_Prob_Dist = true; }
    bool ఉపయోగిస్తుంది_Std_Package() స్థిరరాశి { తిరిగి m_ఉపయోగిస్తుంది_Std_Package; }
    శూన్య సమితి_ఉపయోగిస్తుంది_Std_Package() { m_ఉపయోగిస్తుంది_Std_Package = true; }
    bool ఉపయోగిస్తుంది_Timing() స్థిరరాశి { తిరిగి m_ఉపయోగిస్తుంది_Timing; }
    శూన్య సమితి_ఉపయోగిస్తుంది_Timing() { m_ఉపయోగిస్తుంది_Timing = true; }
    bool ఉపయోగిస్తుంది_Zero_Delay() స్థిరరాశి { తిరిగి m_ఉపయోగిస్తుంది_Zero_Delay; }
    శూన్య సమితి_ఉపయోగిస్తుంది_Zero_Delay() { m_ఉపయోగిస్తుంది_Zero_Delay = true; }
    bool has_Forceable_సంకేతాలు() స్థిరరాశి { తిరిగి m_has_Forceable_సంకేతాలు; }
    శూన్య సమితి_Has_Forceable_సంకేతాలు() { m_has_Forceable_సంకేతాలు = true; }
    bool has_కేటాయించు_Deassign() స్థిరరాశి { తిరిగి m_has_కేటాయించు_Deassign; }
    శూన్య సమితి_Has_కేటాయించు_Deassign() { m_has_కేటాయించు_Deassign = true; }
    bool ఉపయోగిస్తుంది_Force() స్థిరరాశి { తిరిగి m_ఉపయోగిస్తుంది_Force; }
    శూన్య సమితి_ఉపయోగిస్తుంది_Force() { m_ఉపయోగిస్తుంది_Force = true; }
    V3_Hier_లేఖాచిత్రం* hierGraphp() స్థిరరాశి { తిరిగి m_hierGraphp; }
    శూన్య hierGraphp(V3_Hier_లేఖాచిత్రం* graphp) { m_hierGraphp = graphp; }
    bool useParallelBuild() స్థిరరాశి { తిరిగి m_useParallelBuild; }
    శూన్య useParallelBuild(bool flag) { m_useParallelBuild = flag; }
    bool useRandSequence() స్థిరరాశి { తిరిగి m_useRandSequence; }
    శూన్య useRandSequence(bool flag) { m_useRandSequence = flag; }
    bool useCovergroup() స్థిరరాశి { తిరిగి m_useCovergroup; }
    శూన్య useCovergroup(bool flag) { m_useCovergroup = flag; }
    bool useRandomizeMethods() స్థిరరాశి { తిరిగి m_useRandomizeMethods; }
    శూన్య useRandomizeMethods(bool flag) { m_useRandomizeMethods = flag; }
    bool hasPrintedObjects() స్థిరరాశి { తిరిగి m_hasPrintedObjects; }
    శూన్య hasPrintedObjects(bool flag) { m_hasPrintedObjects = flag; }
    శూన్య save_Json_Ptr_క్షేత్రం_Name(స్థిరరాశి std::string& క్షేత్రం_Name);
    శూన్య ptr_పేర్లు_Dump_Json(std::ostream& os);
    శూన్య idPtrMapDumpJson(std::ostream& os);
    స్థిరరాశి std::string& ptr_గమ్యం_Id(స్థిరరాశి శూన్య* p);
    std::thread::id mainThreadId() స్థిరరాశి { తిరిగి m_mainThreadId; }
    స్థిర std::సదిశ<std::string> verilatedCppFiles();
    uint64_t current_Hier_ఖండం_ఖర్చు() స్థిరరాశి { తిరిగి m_current_Hier_ఖండం_ఖర్చు; }
    శూన్య current_Hier_ఖండం_ఖర్చు(uint64_t ఖర్చు) { m_current_Hier_ఖండం_ఖర్చు = ఖర్చు; }
};

బాహ్య V3Global v3Global;

//######################################################################

#అయితే_ముగింపు  // guard
