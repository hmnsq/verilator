// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Verilator Control Files (.vlt) handling
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2010-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************

#ଅନ୍ତର୍ଭାଵ "V3PchAstMT.h"

#ଅନ୍ତର୍ଭାଵ "V3Control.h"

#ଅନ୍ତର୍ଭାଵ "V3InstrCount.h"
#ଅନ୍ତର୍ଭାଵ "V3Os.h"
#ଅନ୍ତର୍ଭାଵ "V3String.h"

#ଅନ୍ତର୍ଭାଵ <memory>
#ଅନ୍ତର୍ଭାଵ <set>
#ଅନ୍ତର୍ଭାଵ <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Resolve wildcards in files, modules, ftasks or variables

// Template for a class that serves as a map for entities that can be specified
// as wildcards and are accessed by a resolved name. It rebuilds a name lookup
// cache of resolved entities. Entities stored in this container need an update
// function that takes a reference of this type to join multiple entities into one.
template <typename T>
ଶ୍ରେଣୀ V3ControlWildcardResolver final {
    mutable V3Mutex m_mutex;  // protects members
    // Pattern strings (wildcard, or simple name) to entities
    std::map<ଅଚଳ std::string, T> m_mapPatterns VL_GUARDED_BY(m_mutex);
    // Resolved strings to converged entities - nullptr, iff none of the patterns applies
    std::map<ଅଚଳ std::string, std::unique_ptr<T>> m_mapResolved VL_GUARDED_BY(m_mutex);

ସର୍ବସାଧାରଣ:
    V3ControlWildcardResolver() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ~V3ControlWildcardResolver() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

    // Update this resolved file's item map by inserting other's (wildcarded filename's) items
    ଶୂନ୍ୟ update(ଅଚଳ V3ControlWildcardResolver& other) VL_MT_SAFE_EXCLUDES(m_mutex)
        VL_EXCLUDES(other.m_mutex) {
        ଅଚଳ V3LockGuard lock{m_mutex};
        ଅଚଳ V3LockGuard otherLock{other.m_mutex};
        // Clear the resolved cache, as 'other' might add new patterns that need to be applied as
        // well.
        m_mapResolved.clear();
        ପାଇଁ (ଅଚଳ auto& itr : other.m_mapPatterns) m_mapPatterns[itr.first].update(itr.second);
    }

    // Access and create a pattern entry
    T& at(ଅଚଳ string& name) VL_MT_SAFE_EXCLUDES(m_mutex) {
        ଅଚଳ V3LockGuard lock{m_mutex};
        // We might be adding a new entry under this, so clear the cache.
        m_mapResolved.clear();
        // Patterns use '/' as separator; normalize Windows backslashes
        ଫେରନ୍ତୁ m_mapPatterns[V3Os::filenameSlashPath(name)];
    }
    // Access an entity and resolve patterns that match it
    T* resolve(ଅଚଳ string& name) VL_MT_SAFE_EXCLUDES(m_mutex) {
        ଅଚଳ V3LockGuard lock{m_mutex};
        // Lookup if it was resolved before, typically not
        ଅଚଳ auto pair = m_mapResolved.emplace(name, nullptr);
        std::unique_ptr<T>& entryr = pair.first->second;
        // Resolve entry when first requested, cache the result
        ଯଦି (pair.second) {
            // Inserted: update the entity with all matches in the patterns.
            // Normalize separators so Windows backslash filenames match the
            // slash-separated patterns (normalized when loaded).
            ଅଚଳ string slashName = V3Os::filenameSlashPath(name);
            ପାଇଁ (ଅଚଳ auto& patEnt : m_mapPatterns) {
                ଯଦି (VString::wildmatch(slashName, patEnt.first)) {
                    ଯଦି (!entryr) entryr.reset(ନୂତନ T{});
                    entryr->update(patEnt.second);
                }
            }
            // Perform final actions that needed all updates completed
            ଯଦି (entryr) entryr->updateFinalize();
        }
        ଫେରନ୍ତୁ entryr.get();
    }
};

// List of attributes for variables
ଶ୍ରେଣୀ V3ControlVar final {
    std::vector<VAttrType> m_attrs;  // The list of attributes
ସର୍ବସାଧାରଣ:
    // Add new attribugte
    ଶୂନ୍ୟ add(VAttrType attr) { m_attrs.emplace_back(attr); }
    // Update from other by copying all attributes
    ଶୂନ୍ୟ update(ଅଚଳ V3ControlVar& other) {
        m_attrs.reserve(m_attrs.size() + other.m_attrs.size());
        m_attrs.ଭର୍ତ୍ତି(m_attrs.end(), other.m_attrs.begin(), other.m_attrs.end());
    }
    ଶୂନ୍ୟ updateFinalize() {}
    // Apply all attributes to the variable
    ଶୂନ୍ୟ apply(AstVar* varp) ଅଚଳ {
        ପାଇଁ (ଅଚଳ VAttrType attr : m_attrs) {
            varp->addAttrsp(ନୂତନ AstAttrOf{varp->fileline(), attr});
        }
    }
};

using V3ControlVarResolver = V3ControlWildcardResolver<V3ControlVar>;

//======================================================================

ଶ୍ରେଣୀ WildcardContents final {
    // Not mutex protected, current calling from V3Control::waive is protected by error's mutex
    // MEMBERS
    std::map<ଅଚଳ std::string, ତାର୍କିକ> m_mapPatterns;  // Pattern match results
    std::deque<string> m_ଧାଡ଼ିଗୁଡ଼ିକ;  // Source text lines

    // METHODS
    ସ୍ଥିର WildcardContents& s() {  // Singleton
        ସ୍ଥିର WildcardContents s_s;
        ଫେରନ୍ତୁ s_s;
    }
    ଶୂନ୍ୟ clearCacheImp() { m_mapPatterns.clear(); }
    ଶୂନ୍ୟ ଠେଲ_Text_Imp(ଅଚଳ string& text) {
        // Similar code in VFileContent::pushText()
        // Any leftover text is stored on largest line (might be "")
        ଅଚଳ string leftover = m_ଧାଡ଼ିଗୁଡ଼ିକ.back() + text;
        m_ଧାଡ଼ିଗୁଡ଼ିକ.କାଢ଼_back();

        // Insert line-by-line
        string::size_type ଧାଡ଼ି_start = 0;
        ଯେତେବେଳେ (ସତ୍ୟ) {
            ଅଚଳ string::size_type ଧାଡ଼ି_end = leftover.find('\n', ଧାଡ଼ି_start);
            ଯଦି (ଧାଡ଼ି_end != string::npos) {
                ଅଚଳ string oneline(leftover, ଧାଡ଼ି_start, ଧାଡ଼ି_end - ଧାଡ଼ି_start + 1);
                ଯଦି (oneline.size() > 1) m_ଧାଡ଼ିଗୁଡ଼ିକ.ଠେଲ_back(oneline);  // Keeps newline
                UINFO(9, "Push[+" << (m_ଧାଡ଼ିଗୁଡ଼ିକ.size() - 1) << "]: " << oneline);
                ଧାଡ଼ି_start = ଧାଡ଼ି_end + 1;
            } ଅନ୍ୟଥା {
                ଭାଙ୍ଗ;
            }
        }
        // Keep leftover for next time
        m_ଧାଡ଼ିଗୁଡ଼ିକ.emplace_back(string(leftover, ଧାଡ଼ି_start));  // Might be ""
        clearCacheImp();
    }

    ତାର୍କିକ resolveUncachedImp(ଅଚଳ string& contentsRegexp) {
        ପାଇଁ (ଅଚଳ string& i : m_ଧାଡ଼ିଗୁଡ଼ିକ) {
            ଯଦି (VString::wildmatch(i, contentsRegexp)) ଫେରନ୍ତୁ ସତ୍ୟ;
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }
    ତାର୍କିକ resolveCachedImp(ଅଚଳ string& contentsRegexp) {
        // Lookup if it was resolved before, typically is
        ଯଦି (contentsRegexp.empty()) ଫେରନ୍ତୁ ସତ୍ୟ;
        ଅଚଳ auto pair = m_mapPatterns.emplace(contentsRegexp, ମିଥ୍ୟା);
        ତାର୍କିକ& entryr = pair.first->second;
        // Resolve entry when first requested, cache the result
        ଯଦି (pair.second) entryr = resolveUncachedImp(contentsRegexp);
        ଫେରନ୍ତୁ entryr;
    }

ସର୍ବସାଧାରଣ:
    WildcardContents() {
        m_ଧାଡ଼ିଗୁଡ଼ିକ.emplace_back("");  // start with no leftover
    }
    ~WildcardContents() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    // Return true iff contentsRegexp in parsed contents
    ସ୍ଥିର ତାର୍କିକ resolve(ଅଚଳ string& contentsRegexp) {
        ଫେରନ୍ତୁ s().resolveCachedImp(contentsRegexp);
    }
    // Add arbitrary text (need not be line-by-line)
    ସ୍ଥିର ଶୂନ୍ୟ ଠେଲ_Text(ଅଚଳ string& text) { s().ଠେଲ_Text_Imp(text); }
};

//######################################################################
// Function or task: Have variables and properties

ଶ୍ରେଣୀ V3ControlFTask final {
    V3ControlVarResolver m_params;  // Parameters in function/task
    V3ControlVarResolver m_ports;  // Ports in function/task
    V3ControlVarResolver m_vars;  // Variables in function/task
    ତାର୍କିକ m_noinline = ମିଥ୍ୟା;  // Don't inline function/task
    ତାର୍କିକ m_public = ମିଥ୍ୟା;  // Public function/task

ସର୍ବସାଧାରଣ:
    V3ControlFTask() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ଶୂନ୍ୟ update(ଅଚଳ V3ControlFTask& f) {
        // Don't overwrite true with false
        ଯଦି (f.m_noinline) m_noinline = ସତ୍ୟ;
        ଯଦି (f.m_public) m_public = ସତ୍ୟ;
        m_params.update(f.m_params);
        m_ports.update(f.m_ports);
        m_vars.update(f.m_vars);
    }
    ଶୂନ୍ୟ updateFinalize() {}

    V3ControlVarResolver& params() { ଫେରନ୍ତୁ m_params; }
    V3ControlVarResolver& ports() { ଫେରନ୍ତୁ m_ports; }
    V3ControlVarResolver& vars() { ଫେରନ୍ତୁ m_vars; }

    ଶୂନ୍ୟ setNoInline(ତାର୍କିକ set) { m_noinline = set; }
    ଶୂନ୍ୟ setPublic(ତାର୍କିକ set) { m_public = set; }

    ଶୂନ୍ୟ apply(Ast_ଗଣ୍ଠି_FTask* ftaskp) ଅଚଳ {
        ଯଦି (m_noinline)
            ftaskp->addStmtsp(ନୂତନ AstPragma{ftaskp->fileline(), VPragmaType::NO_INLINE_TASK});
        ଯଦି (m_public)
            ftaskp->addStmtsp(ନୂତନ AstPragma{ftaskp->fileline(), VPragmaType::PUBLIC_TASK});
    }
};

using V3ControlFTaskResolver = V3ControlWildcardResolver<V3ControlFTask>;

//######################################################################
// Modules have tasks, variables, named blocks and properties

ଶ୍ରେଣୀ V3ControlModule final {
    V3ControlFTaskResolver m_tasks;  // Functions/tasks in module
    V3ControlVarResolver m_params;  // Parameters in module
    V3ControlVarResolver m_ports;  // Ports in module
    V3ControlVarResolver m_vars;  // Variables in module
    V3Control::FsmRegisterWrapper m_fsmRegisterWrapper;  // FSM wrapper descriptor
    std::unordered_set<std::string> m_coverageOffBlocks;  // List of block names for coverage_off
    std::set<VPragmaType> m_modPragmas;  // List of Pragmas for modules
    ତାର୍କିକ m_hasFsmRegisterWrapper = ମିଥ୍ୟା;  // Whether descriptor has been set
    ତାର୍କିକ m_inline = ମିଥ୍ୟା;  // Whether to force the inline
    ତାର୍କିକ m_inline_ମୂଲ୍ୟ = ମିଥ୍ୟା;  // The inline value (on/off)

ସର୍ବସାଧାରଣ:
    V3ControlModule() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

    ଶୂନ୍ୟ update(ଅଚଳ V3ControlModule& m) {
        m_tasks.update(m.m_tasks);
        m_params.update(m.m_params);
        m_ports.update(m.m_ports);
        m_vars.update(m.m_vars);
        ଯଦି (m.m_hasFsmRegisterWrapper) {
            m_fsmRegisterWrapper = m.m_fsmRegisterWrapper;
            m_hasFsmRegisterWrapper = ସତ୍ୟ;
        }
        ପାଇଁ (ଅଚଳ string& i : m.m_coverageOffBlocks) m_coverageOffBlocks.ଭର୍ତ୍ତି(i);
        ଯଦି (!m_inline) {
            m_inline = m.m_inline;
            m_inline_ମୂଲ୍ୟ = m.m_inline_ମୂଲ୍ୟ;
        }
        ପାଇଁ (auto it = m.m_modPragmas.cbegin(); it != m.m_modPragmas.cend(); ++it) {
            m_modPragmas.ଭର୍ତ୍ତି(*it);
        }
    }
    ଶୂନ୍ୟ updateFinalize() {}

    V3ControlFTaskResolver& ftasks() { ଫେରନ୍ତୁ m_tasks; }
    V3ControlVarResolver& params() { ଫେରନ୍ତୁ m_params; }
    V3ControlVarResolver& ports() { ଫେରନ୍ତୁ m_ports; }
    V3ControlVarResolver& vars() { ଫେରନ୍ତୁ m_vars; }

    ଶୂନ୍ୟ addCoverageBlockOff(ଅଚଳ string& name) { m_coverageOffBlocks.ଭର୍ତ୍ତି(name); }
    ଶୂନ୍ୟ setInline(ତାର୍କିକ set) {
        m_inline = ସତ୍ୟ;
        m_inline_ମୂଲ୍ୟ = set;
    }
    ଶୂନ୍ୟ setFsmRegisterWrapper(File_ଧାଡ଼ି* fl, ଅଚଳ V3Control::FsmRegisterWrapper& desc) {
        ଯଦି (m_hasFsmRegisterWrapper) {
            fl->v3warn(BADVLTPRAGMA, "Duplicate fsm_register_wrapper descriptor for module "
                                         << Ast_ଗଣ୍ଠି::prettyNameQ(desc.moduleName)
                                         << "; replacing previous descriptor");
        }
        m_fsmRegisterWrapper = desc;
        m_hasFsmRegisterWrapper = ସତ୍ୟ;
    }
    ଅଚଳ V3Control::FsmRegisterWrapper* fsmRegisterWrapperp() ଅଚଳ {
        ଫେରନ୍ତୁ m_hasFsmRegisterWrapper ? &m_fsmRegisterWrapper : nullptr;
    }
    ଶୂନ୍ୟ addModulePragma(VPragmaType pragma) { m_modPragmas.ଭର୍ତ୍ତି(pragma); }

    ଶୂନ୍ୟ apply(Ast_ଗଣ୍ଠି_Module* modp) {
        ଯଦି (m_inline) {
            ଅଚଳ VPragmaType type
                = m_inline_ମୂଲ୍ୟ ? VPragmaType::INLINE_MODULE : VPragmaType::NO_INLINE_MODULE;
            Ast_ଗଣ୍ଠି* ଅଚଳ nodep = ନୂତନ AstPragma{modp->fileline(), type};
            modp->addStmtsp(nodep);
        }
        ପାଇଁ (ଅଚଳ auto& itr : m_modPragmas) {
            // Catch hier param modules to mark their attributes before they are
            // flagged dead in LinkDot.
            ଯଦି (itr == VPragmaType::HIER_PARAMS) modp->hierParams(ସତ୍ୟ);
            Ast_ଗଣ୍ଠି* ଅଚଳ nodep = ନୂତନ AstPragma{modp->fileline(), itr};
            modp->addStmtsp(nodep);
        }
    }

    ଶୂନ୍ୟ applyBlock(AstGenBlock* nodep) {
        ଅଚଳ VPragmaType pragma = VPragmaType::COVERAGE_BLOCK_OFF;
        ଯଦି (!nodep->unnamed()) {
            ପାଇଁ (ଅଚଳ string& i : m_coverageOffBlocks) {
                ଯଦି (VString::wildmatch(nodep->prettyDehashOrigOrName(), i)) {
                    nodep->addItemsp(ନୂତନ AstPragma{nodep->fileline(), pragma});
                }
            }
        }
    }

    ଶୂନ୍ୟ applyBlock(Ast_ଗଣ୍ଠି_Block* nodep) {
        ଅଚଳ VPragmaType pragma = VPragmaType::COVERAGE_BLOCK_OFF;
        File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
        ଯଦି (!nodep->unnamed()) {
            ପାଇଁ (ଅଚଳ string& i : m_coverageOffBlocks) {
                ଯଦି (VString::wildmatch(nodep->prettyDehashOrigOrName(), i)) {
                    nodep->addStmtsp(ନୂତନ AstStmtPragma{flp, ନୂତନ AstPragma{flp, pragma}});
                }
            }
        }
    }
};

using V3ControlModuleResolver = V3ControlWildcardResolver<V3ControlModule>;

//######################################################################
// Files have:
//  - Line ignores (lint/coverage/tracing on/off)
//  - Line attributes: Attributes attached to lines

// lint/coverage/tracing on/off
ଶ୍ରେଣୀ V3_Control_Ignores_ଧାଡ଼ି final {
ସର୍ବସାଧାରଣ:
    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_ଧାଡ଼ି_Min;  // Minimum line number to make change at
    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_ଧାଡ଼ି_Max;  // Maximum line number to make change at (inclusive)
    ଅଚଳ V3ErrorCode m_code;  // Error code
    ଅଚଳ ତାର୍କିକ m_on;  // True to enable message
    V3_Control_Ignores_ଧାଡ଼ି(V3ErrorCode code, ପୂର୍ଣ୍ଣସଂଖ୍ୟା linemin, ପୂର୍ଣ୍ଣସଂଖ୍ୟା linemax, ତାର୍କିକ on)
        : m_ଧାଡ଼ି_Min{linemin}
        , m_ଧାଡ଼ି_Max{linemax}
        , m_code{code}
        , m_on{on} {}
    ~V3_Control_Ignores_ଧାଡ଼ି() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ତାର୍କିକ every_ଧାଡ଼ି() ଅଚଳ { ଫେରନ୍ତୁ m_ଧାଡ଼ି_Min == 0 && m_ଧାଡ଼ି_Max == 0; }
    ତାର୍କିକ inRange(ପୂର୍ଣ୍ଣସଂଖ୍ୟା lineno) ଅଚଳ {
        ଫେରନ୍ତୁ (lineno >= m_ଧାଡ଼ି_Min && lineno <= m_ଧାଡ଼ି_Max) || every_ଧାଡ଼ି();
    }
};
std::ostream& operator<<(std::ostream& os, ଅଚଳ V3_Control_Ignores_ଧାଡ଼ି& rhs) {
    ଫେରନ୍ତୁ os << rhs.m_ଧାଡ଼ି_Min << "-" << rhs.m_ଧାଡ଼ି_Max << ", " << rhs.m_code << ", " << rhs.m_on;
}

// Ignore line settings, index is parse order
// Single global deque to avoid copying when update()
ସ୍ଥିର std::vector<V3_Control_Ignores_ଧାଡ଼ି> control_Ign_ଧାଡ଼ିଗୁଡ଼ିକ;

using IgnIndices = std::vector<uint32_t>;  // List of {s_ignLines indces}

ଶ୍ରେଣୀ VInterval_ବୃକ୍ଷ final {
    ସଂରଚନା Entry final {
        // Classic centered interval tree, referencing an index to controlIgnLines
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_center;  // Line number of central point
        std::unique_ptr<Entry> m_leftp;  // Entry with all its rules/entries < this center
        std::unique_ptr<Entry> m_rightp;  // Entry with all its rules/entries > this center
        IgnIndices m_byMin;  // Rules overlapping center sorted by min line number
        IgnIndices m_byMax;  // Rules overlapping center sorted by max line number
        explicit Entry(ପୂର୍ଣ୍ଣସଂଖ୍ୟା center)
            : m_center{center} {}
    };

    std::unique_ptr<Entry> m_rootp;  // Root of interval tree
    IgnIndices m_every_ଧାଡ଼ିଗୁଡ଼ିକ;  // All-line disables

ବ୍ୟକ୍ତିଗତ:
    std::unique_ptr<Entry> build_ବୃକ୍ଷ(ଅଚଳ IgnIndices& points) {
        ଯଦି (points.empty()) ଫେରନ୍ତୁ nullptr;

        ପୂର୍ଣ୍ଣସଂଖ୍ୟା minval = std::numeric_limits<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>::max();
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା maxval = std::numeric_limits<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>::min();
        ପାଇଁ (ଅଚଳ auto& it : points) {
            ଅଚଳ V3_Control_Ignores_ଧାଡ଼ି& cign = control_Ign_ଧାଡ଼ିଗୁଡ଼ିକ[it];
            ଯଦି (cign.every_ଧାଡ଼ି()) {
                m_every_ଧାଡ଼ିଗୁଡ଼ିକ.emplace_back(it);
                ଜାରି;
            }
            minval = std::min(minval, cign.m_ଧାଡ଼ି_Min);
            maxval = std::max(maxval, cign.m_ଧାଡ଼ି_Max);
        }

        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା center = minval + (maxval - minval) / 2;
        auto entp = std::unique_ptr<Entry>(ନୂତନ Entry{center});

        IgnIndices ବାମ_Points;
        IgnIndices ଡାହାଣ_Points;
        ପାଇଁ (ଅଚଳ auto& it : points) {
            ଅଚଳ V3_Control_Ignores_ଧାଡ଼ି& cign = control_Ign_ଧାଡ଼ିଗୁଡ଼ିକ[it];
            ଯଦି (cign.every_ଧାଡ଼ି()) ଜାରି;
            // UINFO(9, "Inserting for center " << center << " point " << it);
            ଯଦି (cign.m_ଧାଡ଼ି_Max < center)
                ବାମ_Points.emplace_back(it);
            ଅନ୍ୟଥା ଯଦି (cign.m_ଧାଡ଼ି_Min > center)
                ଡାହାଣ_Points.emplace_back(it);
            ଅନ୍ୟଥା {
                entp->m_byMin.ଠେଲ_back(it);
                entp->m_byMax.ଠେଲ_back(it);
            }
        }

        std::sort(entp->m_byMin.begin(), entp->m_byMin.end(), [](uint32_t a, uint32_t b) {
            ଫେରନ୍ତୁ control_Ign_ଧାଡ଼ିଗୁଡ଼ିକ[a].m_ଧାଡ଼ି_Min < control_Ign_ଧାଡ଼ିଗୁଡ଼ିକ[b].m_ଧାଡ଼ି_Min;
        });
        std::sort(entp->m_byMax.begin(), entp->m_byMax.end(), [](uint32_t a, uint32_t b) {
            ଫେରନ୍ତୁ control_Ign_ଧାଡ଼ିଗୁଡ଼ିକ[a].m_ଧାଡ଼ି_Max > control_Ign_ଧାଡ଼ିଗୁଡ଼ିକ[b].m_ଧାଡ଼ି_Max;
        });

        entp->m_leftp = build_ବୃକ୍ଷ(ବାମ_Points);
        entp->m_rightp = build_ବୃକ୍ଷ(ଡାହାଣ_Points);
        ଫେରନ୍ତୁ entp;
    }

    ଶୂନ୍ୟ find_ବୃକ୍ଷ(ପୂର୍ଣ୍ଣସଂଖ୍ୟା lineno, Entry* entp, IgnIndices& resultsr, ପୂର୍ଣ୍ଣସଂଖ୍ୟା& nextChanger) ଅଚଳ {
        // UINFO(9, "Find " << lineno << " center " << entp->m_center);
        ଯଦି (lineno < entp->m_center) {
            nextChanger = std::min(nextChanger, entp->m_center);
            ପାଇଁ (ଅଚଳ auto& it : entp->m_byMin) {
                ଅଚଳ V3_Control_Ignores_ଧାଡ଼ି& cign = control_Ign_ଧାଡ଼ିଗୁଡ଼ିକ[it];
                ଯଦି (cign.m_ଧାଡ଼ି_Min <= lineno) {
                    resultsr.emplace_back(it);
                } ଅନ୍ୟଥା {
                    nextChanger = std::min(nextChanger, cign.m_ଧାଡ଼ି_Min);
                    ଭାଙ୍ଗ;
                }
            }
            ଯଦି (entp->m_leftp) find_ବୃକ୍ଷ(lineno, entp->m_leftp.get(), resultsr, nextChanger);
        } ଅନ୍ୟଥା ଯଦି (lineno > entp->m_center) {
            ପାଇଁ (ଅଚଳ auto& it : entp->m_byMax) {
                ଅଚଳ V3_Control_Ignores_ଧାଡ଼ି& cign = control_Ign_ଧାଡ଼ିଗୁଡ଼ିକ[it];
                ଯଦି (cign.m_ଧାଡ଼ି_Max >= lineno) {
                    nextChanger = std::min(nextChanger, cign.m_ଧାଡ଼ି_Max + 1);
                    resultsr.emplace_back(it);
                } ଅନ୍ୟଥା {
                    ଭାଙ୍ଗ;
                }
            }
            ଯଦି (entp->m_rightp) find_ବୃକ୍ଷ(lineno, entp->m_rightp.get(), resultsr, nextChanger);
        } ଅନ୍ୟଥା {
            ପାଇଁ (ଅଚଳ auto& it : entp->m_byMin) {
                ଅଚଳ V3_Control_Ignores_ଧାଡ଼ି& cign = control_Ign_ଧାଡ଼ିଗୁଡ଼ିକ[it];
                resultsr.emplace_back(it);
                ଯଦି (cign.m_ଧାଡ଼ି_Max >= lineno)
                    nextChanger = std::min(nextChanger, cign.m_ଧାଡ଼ି_Max + 1);
            }
            ଯଦି (entp->m_leftp) find_ବୃକ୍ଷ(lineno, entp->m_leftp.get(), resultsr, nextChanger);
            ଯଦି (entp->m_rightp) find_ବୃକ୍ଷ(lineno, entp->m_rightp.get(), resultsr, nextChanger);
        }
    }

ସର୍ବସାଧାରଣ:
    // CONSTRUCTORS
    VInterval_ବୃକ୍ଷ() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ~VInterval_ବୃକ୍ଷ() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    // METHODS
    ଶୂନ୍ୟ clear() { m_rootp = nullptr; }
    ଶୂନ୍ୟ build(ଅଚଳ IgnIndices& points) {
        clear();
        m_rootp = build_ବୃକ୍ଷ(points);
    }
    ଶୂନ୍ୟ find(ପୂର୍ଣ୍ଣସଂଖ୍ୟା lineno, IgnIndices& resultsr, ପୂର୍ଣ୍ଣସଂଖ୍ୟା& nextChanger) ଅଚଳ {
        resultsr = m_every_ଧାଡ଼ିଗୁଡ଼ିକ;
        nextChanger = std::numeric_limits<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>::max();

        ଯଦି (m_rootp) find_ବୃକ୍ଷ(lineno, m_rootp.get(), resultsr, nextChanger);
        // Sort indices, so can process in parse order
        std::sort(resultsr.begin(), resultsr.end());
    }

    ସ୍ଥିର ଶୂନ୍ୟ selfTest() {
        //            0     10     20     30     40     50
        // i0:        .     10-10  .      .      .      .
        // i1:        .     .      20-20  .      .      .
        // i2:        .     .      .      .      40-40  .
        // i3:        .     10------------30     .      .
        // i4:        .     .      20------------40     .
        // i5:(below) 0---------------------------------------0
        IgnIndices data;
        std::vector<std::pair<ପୂର୍ଣ୍ଣସଂଖ୍ୟା, ପୂର୍ଣ୍ଣସଂଖ୍ୟା>> points
            = {{10, 10}, {20, 20}, {40, 40}, {10, 30}, {20, 40}};
        ପାଇଁ (ଅଚଳ auto& it : points) {
            control_Ign_ଧାଡ଼ିଗୁଡ଼ିକ.emplace_back(
                V3_Control_Ignores_ଧାଡ଼ି{V3ErrorCode::I_LINT, it.first, it.second, ସତ୍ୟ});
            data.emplace_back(static_cast<uint32_t>(control_Ign_ଧାଡ଼ିଗୁଡ଼ିକ.size() - 1));
        }
        VInterval_ବୃକ୍ଷ ବୃକ୍ଷ;
        ବୃକ୍ଷ.build(data);

        IgnIndices results;
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା nextChange = 0;
        ବୃକ୍ଷ.find(0, results, nextChange);
        UASSERT_SELFTEST(results.size(), 0);
        UASSERT_SELFTEST(nextChange, 10);
        ବୃକ୍ଷ.find(10, results, nextChange);
        UASSERT_SELFTEST(results.size(), 2);
        UASSERT_SELFTEST(results[0], 0);
        UASSERT_SELFTEST(results[1], 3);
        UASSERT_SELFTEST(nextChange, 11);
        ବୃକ୍ଷ.find(11, results, nextChange);
        UASSERT_SELFTEST(results.size(), 1);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(nextChange, 15);  // Center, or would be 20
        ବୃକ୍ଷ.find(20, results, nextChange);
        UASSERT_SELFTEST(results.size(), 3);
        UASSERT_SELFTEST(results[0], 1);
        UASSERT_SELFTEST(results[1], 3);
        UASSERT_SELFTEST(results[2], 4);
        UASSERT_SELFTEST(nextChange, 21);
        ବୃକ୍ଷ.find(21, results, nextChange);
        UASSERT_SELFTEST(results.size(), 2);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(nextChange, 25);  // Center, or would be 30
        ବୃକ୍ଷ.find(30, results, nextChange);
        UASSERT_SELFTEST(results.size(), 2);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(nextChange, 31);
        ବୃକ୍ଷ.find(40, results, nextChange);
        UASSERT_SELFTEST(results.size(), 2);
        UASSERT_SELFTEST(results[0], 2);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(nextChange, 41);
        ବୃକ୍ଷ.find(41, results, nextChange);
        UASSERT_SELFTEST(results.size(), 0);
        UASSERT_SELFTEST(nextChange, std::numeric_limits<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>::max());
        //
        points = {{0, 0}};
        ପାଇଁ (ଅଚଳ auto& it : points) {
            control_Ign_ଧାଡ଼ିଗୁଡ଼ିକ.emplace_back(
                V3_Control_Ignores_ଧାଡ଼ି{V3ErrorCode::I_LINT, it.first, it.second, ସତ୍ୟ});
            data.emplace_back(static_cast<uint32_t>(control_Ign_ଧାଡ଼ିଗୁଡ଼ିକ.size() - 1));
        }
        ବୃକ୍ଷ.build(data);
        //
        ବୃକ୍ଷ.find(50, results, nextChange);
        UASSERT_SELFTEST(results.size(), 1);
        UASSERT_SELFTEST(results[0], 5);
    }
};

// Some attributes are attached to entities of the occur on a fileline
// and multiple attributes can be attached to a line
using V3_Control_ଧାଡ଼ି_Attribute = std::bitset<VPragmaType::_ENUM_SIZE>;

ଶ୍ରେଣୀ WaiverSetting final {
ସର୍ବସାଧାରଣ:
    V3ErrorCode m_code;  // Error code
    string m_contents;  // --contents regexp
    string m_match;  // --match regexp
    WaiverSetting(V3ErrorCode code, ଅଚଳ string& contents, ଅଚଳ string& match)
        : m_code{code}
        , m_contents{contents}
        , m_match{match} {}
    ~WaiverSetting() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    WaiverSetting& operator=(ଅଚଳ WaiverSetting& rhs) = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
};

// File entity
ଶ୍ରେଣୀ V3ControlFile final {
    using ଧାଡ଼ି_Attr_Map = std::map<ପୂର୍ଣ୍ଣସଂଖ୍ୟା, V3_Control_ଧାଡ଼ି_Attribute>;  // Map line->bitset of attributes
    using Waivers = std::vector<WaiverSetting>;  // List of {code,wildcard string}

    ଧାଡ଼ି_Attr_Map m_ଧାଡ଼ି_Attrs;  // Attributes to line mapping
    IgnIndices m_ignIndices;  // s_ignLines that apply to this specific file
    Waivers m_waivers;  // Waive messages
    VInterval_ବୃକ୍ଷ m_interval_ବୃକ୍ଷ;  // Tree of indices to IgnLines

    ସଂରଚନା {
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା filenameno = -1;  // Last filename
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା lineno = -1;  // Last linenumber
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା nextChange = -1;  // Line number of next change
    } m_lastIgnore;  // Last ignore line run

    // Match a given line and attribute to the map, line 0 is any
    ତାର୍କିକ ଧାଡ଼ି_Match(ପୂର୍ଣ୍ଣସଂଖ୍ୟା lineno, VPragmaType type) {
        ଯଦି (m_ଧାଡ଼ି_Attrs.find(0) != m_ଧାଡ଼ି_Attrs.end() && m_ଧାଡ଼ି_Attrs[0][type]) ଫେରନ୍ତୁ ସତ୍ୟ;
        ଯଦି (m_ଧାଡ଼ି_Attrs.find(lineno) == m_ଧାଡ଼ି_Attrs.end()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଫେରନ୍ତୁ m_ଧାଡ଼ି_Attrs[lineno][type];
    }

ସର୍ବସାଧାରଣ:
    V3ControlFile() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ଶୂନ୍ୟ update(ଅଚଳ V3ControlFile& file) {
        // Copy in all attributes and waivers
        ପାଇଁ (ଅଚଳ auto& itr : file.m_ଧାଡ଼ି_Attrs) m_ଧାଡ଼ି_Attrs[itr.first] |= itr.second;
        m_waivers.reserve(m_waivers.size() + file.m_waivers.size());
        m_waivers.ଭର୍ତ୍ତି(m_waivers.end(), file.m_waivers.begin(), file.m_waivers.end());
        // Copy in all ignore references
        m_ignIndices.reserve(m_ignIndices.size() + file.m_ignIndices.size());
        m_ignIndices.ଭର୍ତ୍ତି(m_ignIndices.end(), file.m_ignIndices.begin(),
                            file.m_ignIndices.end());
        // updateFinalize() will soon build tree
    }
    ଶୂନ୍ୟ updateFinalize() { m_interval_ବୃକ୍ଷ.build(m_ignIndices); }
    ଶୂନ୍ୟ add_ଧାଡ଼ି_Attribute(ପୂର୍ଣ୍ଣସଂଖ୍ୟା lineno, VPragmaType attr) { m_ଧାଡ଼ି_Attrs[lineno].set(attr); }
    ଶୂନ୍ୟ addIgnore(V3ErrorCode code, ପୂର୍ଣ୍ଣସଂଖ୍ୟା min, ପୂର୍ଣ୍ଣସଂଖ୍ୟା max, ତାର୍କିକ on) {
        control_Ign_ଧାଡ଼ିଗୁଡ଼ିକ.emplace_back(V3_Control_Ignores_ଧାଡ଼ି{code, min, max, on});
        m_ignIndices.emplace_back(static_cast<uint32_t>(control_Ign_ଧାଡ଼ିଗୁଡ଼ିକ.size() - 1));
        m_lastIgnore.nextChange = -1;
    }
    ଶୂନ୍ୟ addIgnoreMatch(V3ErrorCode code, ଅଚଳ string& contents, ଅଚଳ string& match) {
        // Since Verilator 5.031 the error message compared has context, so
        // allow old rules to still match using a final '*'
        string newMatch = match;
        ଯଦି (newMatch.empty() || newMatch.back() != '*') newMatch += '*';
        m_waivers.emplace_back(code, contents, newMatch);
    }

    ଶୂନ୍ୟ applyBlock(AstGenBlock* nodep) {
        // Apply to block at this line
        ଅଚଳ VPragmaType pragma = VPragmaType::COVERAGE_BLOCK_OFF;
        ଯଦି (ଧାଡ଼ି_Match(nodep->fileline()->lineno(), pragma)) {
            nodep->addItemsp(ନୂତନ AstPragma{nodep->fileline(), pragma});
        }
    }
    ଶୂନ୍ୟ applyBlock(Ast_ଗଣ୍ଠି_Block* nodep) {
        // Apply to block at this line
        ଅଚଳ VPragmaType pragma = VPragmaType::COVERAGE_BLOCK_OFF;
        File_ଧାଡ଼ି* ଅଚଳ flp = nodep->fileline();
        ଯଦି (ଧାଡ଼ି_Match(flp->lineno(), pragma)) {
            nodep->addStmtsp(ନୂତନ AstStmtPragma{flp, ନୂତନ AstPragma{flp, pragma}});
        }
    }
    ଶୂନ୍ୟ applyCase(AstCase* nodep) {
        // Apply to this case at this line
        ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା lineno = nodep->fileline()->lineno();
        ଯଦି (ଧାଡ଼ି_Match(lineno, VPragmaType::FULL_CASE)) nodep->fullPragma(ସତ୍ୟ);
        ଯଦି (ଧାଡ଼ି_Match(lineno, VPragmaType::PARALLEL_CASE)) nodep->parallelPragma(ସତ୍ୟ);
    }
    ଶୂନ୍ୟ applyIgnores(File_ଧାଡ଼ି* filelinep) {
        // HOT routine, called each parsed token line of this filename
        ଯଦି (filelinep->filenameno() == m_lastIgnore.filenameno
            && filelinep->lineno() == m_lastIgnore.lineno)
            ଫେରନ୍ତୁ;  // Short circuit, no change, no debug
        // For speed, compute line number of next potential change, and skip if before then
        ଯଦି (filelinep->filenameno() == m_lastIgnore.filenameno
            && filelinep->lineno() < m_lastIgnore.nextChange) {
            m_lastIgnore.lineno = filelinep->lineno();
            UINFO(9, "   ApplyIgnores for " << filelinep->ascii() << " (no change predicted)");
            ଫେରନ୍ତୁ;
        }
        UINFO(9, "   ApplyIgnores for " << filelinep->ascii());
        m_lastIgnore.filenameno = filelinep->filenameno();
        m_lastIgnore.lineno = filelinep->lineno();
        m_lastIgnore.nextChange = std::numeric_limits<ପୂର୍ଣ୍ଣସଂଖ୍ୟା>::max();

        // Find all on/offs.  Unlike lint pragmas we calculate the entire
        // bitset of errors to enable, instead of individual enable/disable changes.
        IgnIndices results;
        m_interval_ବୃକ୍ଷ.find(filelinep->lineno(), results /*ref*/, m_lastIgnore.nextChange /*ref*/);
        // Process all on/offs for lines up to and including the current line
        VErrorBitSet bitset{VErrorBitSet::AllOnes{}};
        ପାଇଁ (ଅଚଳ auto& it : results) {
            ଅଚଳ V3_Control_Ignores_ଧାଡ଼ି& cign = control_Ign_ଧାଡ଼ିଗୁଡ଼ିକ[it];
            UASSERT(cign.inRange(filelinep->lineno()),
                    "Interval tree returned lines outside range");
            UINFO(9, "     Hit " << cign);
            cign.m_code.forDelegateCodes(
                [&](V3ErrorCode subcode) { bitset.set(subcode, cign.m_on); });
        }

        filelinep->warnSetCtrlBitSet(bitset);
        m_lastIgnore.nextChange = std::max(m_lastIgnore.nextChange, filelinep->lineno() + 1);
        UINFO(9, "    AppliedIgnores " << filelinep << " next@ln " << std::dec
                                       << m_lastIgnore.nextChange
                                       << " Messages: " << bitset.ascii());
    }
    ତାର୍କିକ waive(V3ErrorCode code, ଅଚଳ string& message) {
        ଯଦି (code.hardError()) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ପାଇଁ (ଅଚଳ auto& itr : m_waivers) {
            ଯଦି ((code.isUnder(itr.m_code) || (itr.m_code == V3ErrorCode::I_LINT))
                && VString::wildmatch(message, itr.m_match)
                && WildcardContents::resolve(itr.m_contents)) {
                ଫେରନ୍ତୁ ସତ୍ୟ;
            }
        }
        ଫେରନ୍ତୁ ମିଥ୍ୟା;
    }
};

using V3ControlFileResolver = V3ControlWildcardResolver<V3ControlFile>;

//######################################################################
// ScopeTrace tracking

ଶ୍ରେଣୀ V3ControlScopeTraceEntry final {
ସର୍ବସାଧାରଣ:
    ଅଚଳ string m_scope;  // Scope or regexp to match
    ଅଚଳ ତାର୍କିକ m_on;  // True to enable message
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_levels = 0;  // # levels, 0 = all, 1 = only this, ...
    // CONSTRUCTORS
    V3ControlScopeTraceEntry(ଅଚଳ string& scope, ତାର୍କିକ on, ପୂର୍ଣ୍ଣସଂଖ୍ୟା levels)
        : m_scope{scope}
        , m_on{on}
        , m_levels{levels} {}
    ~V3ControlScopeTraceEntry() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ତାର୍କିକ operator<(ଅଚଳ V3ControlScopeTraceEntry& other) ଅଚଳ {
        ଯଦି (m_on < other.m_on) ଫେରନ୍ତୁ ସତ୍ୟ;
        ଯଦି (m_on > other.m_on) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଯଦି (m_levels < other.m_levels) ଫେରନ୍ତୁ ସତ୍ୟ;
        ଯଦି (m_levels > other.m_levels) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଫେରନ୍ତୁ m_scope < other.m_scope;
    }
};

// Tracks what matches are known to hit against V3ControlScopeTraceEntries
ଶ୍ରେଣୀ V3ControlScopeTraceEntryMatch final {
ସର୍ବସାଧାରଣ:
    ଅଚଳ V3ControlScopeTraceEntry* m_entryp;  // Entry the match refers to
    ଅଚଳ string m_scopepart;  // Scope prefix being matched against the entry
    V3ControlScopeTraceEntryMatch(ଅଚଳ V3ControlScopeTraceEntry* entryp, ଅଚଳ string& scopepart)
        : m_entryp{entryp}
        , m_scopepart{scopepart} {}
    ତାର୍କିକ operator<(ଅଚଳ V3ControlScopeTraceEntryMatch& other) ଅଚଳ {
        ଯଦି (m_entryp < other.m_entryp) ଫେରନ୍ତୁ ସତ୍ୟ;
        ଯଦି (m_entryp > other.m_entryp) ଫେରନ୍ତୁ ମିଥ୍ୟା;
        ଫେରନ୍ତୁ m_scopepart < other.m_scopepart;
    }
};

ଶ୍ରେଣୀ V3ControlScopeTraceResolver final {
    std::vector<V3ControlScopeTraceEntry> m_entries;  // User specified on/offs and levels
    std::map<V3ControlScopeTraceEntryMatch, ତାର୍କିକ> m_matchCache;  // Matching entries for speed

ସର୍ବସାଧାରଣ:
    ଶୂନ୍ୟ addScopeTraceOn(ତାର୍କିକ on, ଅଚଳ string& scope, ପୂର୍ଣ୍ଣସଂଖ୍ୟା levels) {
        UINFO(9, "addScopeTraceOn " << on << " '" << scope << "' " << " levels=" << levels);
        m_entries.emplace_back(V3ControlScopeTraceEntry{scope, on, levels});
        m_matchCache.clear();
    }

    ତାର୍କିକ getEntryMatch(ଅଚଳ V3ControlScopeTraceEntry* entp, ଅଚଳ string& scopepart) {
        // Return if a entry matches the scopepart, with memoization
        ଅଚଳ V3ControlScopeTraceEntryMatch ଚାବି{entp, scopepart};
        ଅଚଳ auto pair = m_matchCache.emplace(ଚାବି, ମିଥ୍ୟା);
        ଯଦି (pair.second) pair.first->second = VString::wildmatch(scopepart, entp->m_scope);
        ଫେରନ୍ତୁ pair.first->second;
    }

    ତାର୍କିକ getScopeTraceOn(ଅଚଳ string& scope) {
        // Apply in the order the user provided them, so they can choose on/off preferences
        ପୂର୍ଣ୍ଣସଂଖ୍ୟା maxLevel = 1;
        ପାଇଁ (ଅଚଳ auto& ch : scope) {
            ଯଦି (ch == '.') ++maxLevel;
        }
        UINFO(9, "getScopeTraceOn " << scope << " maxLevel=" << maxLevel);

        ତାର୍କିକ enabled = ସତ୍ୟ;
        ପାଇଁ (ଅଚଳ auto& ent : m_entries) {
            // We apply shortest match first for each rule component
            // (Otherwise the levels would be useless as "--scope top* --levels 1" would
            // always match at every scopepart, and we wouldn't know how to count levels)
            ପୂର୍ଣ୍ଣସଂଖ୍ୟା partLevel = 1;
            ପାଇଁ (string::size_type partEnd = 0; ସତ୍ୟ;) {
                partEnd = scope.find('.', partEnd + 1);
                ଯଦି (partEnd == string::npos) partEnd = scope.length();
                ଅଚଳ std::string scopepart = scope.substr(0, partEnd);
                ଯଦି (getEntryMatch(&ent, scopepart)) {
                    ଅଚଳ ତାର୍କିକ levelMatch
                        = !ent.m_levels || (ent.m_levels >= maxLevel - partLevel);
                    ଯଦି (levelMatch) enabled = ent.m_on;
                    UINFO(9, "getScopeTraceOn-part " << scope << " enabled=" << enabled
                                                     << " @ lev=" << partLevel
                                                     << (levelMatch ? "[match]" : "[miss]")
                                                     << " from scopepart=" << scopepart);
                    ଭାଙ୍ଗ;
                }
                ଯଦି (partEnd == scope.length()) ଭାଙ୍ଗ;
                ++partLevel;
            }
        }
        ଫେରନ୍ତୁ enabled;
    }
};

//######################################################################
// Resolve modules and files in the design

ଶ୍ରେଣୀ V3ControlResolverHierWorkerEntry final {
    ଅଚଳ ପୂର୍ଣ୍ଣସଂଖ୍ୟା m_workers;  // Number of workers requested for the hierarchical block
    File_ଧାଡ଼ି* ଅଚଳ m_flp;  // Location the worker count was specified at

ସର୍ବସାଧାରଣ:
    explicit V3ControlResolverHierWorkerEntry(ପୂର୍ଣ୍ଣସଂଖ୍ୟା workers, File_ଧାଡ଼ି* flp)
        : m_workers{workers}
        , m_flp{flp} {}
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା workers() ଅଚଳ { ଫେରନ୍ତୁ m_workers; }
    File_ଧାଡ଼ି* flp() ଅଚଳ { ଫେରନ୍ତୁ m_flp; }
};

ଶ୍ରେଣୀ V3ControlResolver final {
    ଗଣନା ProfileDataMode : uint8_t { NONE = 0, MTASK = 1, HIER_DPI = 2 };
    V3ControlModuleResolver m_modules;  // Access to module names (with wildcards)
    V3ControlFileResolver m_files;  // Access to file names (with wildcards)
    V3ControlScopeTraceResolver m_scopeTraces;  // Regexp to trace enables
    std::unordered_map<string, std::unordered_map<string, uint64_t>>
        m_profileData;  // Access to profile_data records
    uint8_t m_mode = NONE;  // Kind of profile_data records is currently active
    std::unordered_map<string, V3ControlResolverHierWorkerEntry> m_hierWorkers;
    File_ଧାଡ଼ି* m_profile_File_ଧାଡ଼ି = nullptr;  // Location profile_data was read from

    V3ControlResolver() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;
    ~V3ControlResolver() = ପୂର୍ବନିର୍ଦ୍ଧାରିତ;

ସର୍ବସାଧାରଣ:
    ସ୍ଥିର V3ControlResolver& s() {
        ସ୍ଥିର V3ControlResolver s_singleton;
        ଫେରନ୍ତୁ s_singleton;
    }
    V3ControlModuleResolver& modules() { ଫେରନ୍ତୁ m_modules; }
    V3ControlFileResolver& files() { ଫେରନ୍ତୁ m_files; }
    V3ControlScopeTraceResolver& scopeTraces() { ଫେରନ୍ତୁ m_scopeTraces; }

    ଶୂନ୍ୟ addProfileData(File_ଧାଡ଼ି* fl, ଅଚଳ string& hierDpi, uint64_t cost) {
        // Empty key for hierarchical DPI wrapper costs.
        addProfileData(fl, hierDpi, "", cost, HIER_DPI);
    }
    ଶୂନ୍ୟ addProfileData(File_ଧାଡ଼ି* fl, ଅଚଳ string& model, ଅଚଳ string& ଚାବି, uint64_t cost,
                        ProfileDataMode mode = MTASK) {
        ଯଦି (!m_profile_File_ଧାଡ଼ି) m_profile_File_ଧାଡ଼ି = fl;
        ଯଦି (cost == 0) cost = 1;  // Cost 0 means delete (or no data)
        ଯଦି (mode == MTASK) {
            m_profileData[model][ଚାବି] += cost;
        } ଅନ୍ୟଥା ଯଦି (mode == HIER_DPI) {
            m_profileData[model][ଚାବି] = std::max(m_profileData[model][ଚାବି], cost);
        }
        m_mode |= mode;
    }
    ତାର୍କିକ containsMTaskProfileData() ଅଚଳ { ଫେରନ୍ତୁ m_mode & MTASK; }
    uint64_t getProfileData(ଅଚଳ string& hierDpi) ଅଚଳ {
        // Empty key for hierarchical DPI wrapper costs.
        ଫେରନ୍ତୁ getProfileData(hierDpi, "");
    }
    ଶୂନ୍ୟ addHierWorkers(File_ଧାଡ଼ି* flp, ଅଚଳ string& model, ପୂର୍ଣ୍ଣସଂଖ୍ୟା workers) {
        m_hierWorkers.emplace(std::piecewise_construct, std::forward_as_tuple(model),
                              std::forward_as_tuple(workers, flp));
    }
    ପୂର୍ଣ୍ଣସଂଖ୍ୟା getHierWorkers(ଅଚଳ string& model) ଅଚଳ {
        ଅଚଳ auto mit = m_hierWorkers.find(model);
        // Assign a single worker if no specified.
        ଫେରନ୍ତୁ mit != m_hierWorkers.cend() ? mit->second.workers() : 0;
    }
    File_ଧାଡ଼ି* get_Hier_Workers_File_ଧାଡ଼ି(ଅଚଳ string& model) ଅଚଳ {
        ଅଚଳ auto mit = m_hierWorkers.find(model);
        ଫେରନ୍ତୁ mit != m_hierWorkers.cend() ? mit->second.flp() : v3Global.rootp()->fileline();
    }
    uint64_t getProfileData(ଅଚଳ string& model, ଅଚଳ string& ଚାବି) ଅଚଳ {
        ଅଚଳ auto mit = m_profileData.find(model);
        ଯଦି (mit == m_profileData.cend()) ଫେରନ୍ତୁ 0;
        ଅଚଳ auto it = mit->second.find(ଚାବି);
        ଯଦି (it == mit->second.cend()) ଫେରନ୍ତୁ 0;
        ଫେରନ୍ତୁ it->second;
    }
    File_ଧାଡ଼ି* get_Profile_Data_File_ଧାଡ଼ି() ଅଚଳ { ଫେରନ୍ତୁ m_profile_File_ଧାଡ଼ି; }  // Maybe null
    ସ୍ଥିର uint64_t getCurrentHierBlockCost() {
        ଯଦି (uint64_t cost = V3Control::getProfileData(v3Global.opt.prefix())) {
            UINFO(9, "Fetching cost from profile info: " << cost);
            ଫେରନ୍ତୁ cost;
        }

        // Without profiling data, sum the regions evaluated on each time step
        AstNetlist* ଅଚଳ netlistp = v3Global.rootp();
        uint64_t cost = 0;
        ପାଇଁ (ପୂର୍ଣ୍ଣସଂଖ୍ୟା i = 0; i < VEval::_ENUM_END; ++i) {
            ଅଚଳ VEval eval{i};
            ଯଦି (eval.slow()) ଜାରି;
            cost += V3_Instr_ଗଣନା::ଗଣନା_2(netlistp->evalFuncp(eval), ମିଥ୍ୟା);
        }
        UINFO(9, "Evaluating cost: " << cost);
        ଫେରନ୍ତୁ cost;
    }
};

//######################################################################
// V3Control

ଶୂନ୍ୟ V3Control::addCaseFull(ଅଚଳ string& filename, ପୂର୍ଣ୍ଣସଂଖ୍ୟା lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.add_ଧାଡ଼ି_Attribute(lineno, VPragmaType::FULL_CASE);
}
ଶୂନ୍ୟ V3Control::addCaseParallel(ଅଚଳ string& filename, ପୂର୍ଣ୍ଣସଂଖ୍ୟା lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.add_ଧାଡ଼ି_Attribute(lineno, VPragmaType::PARALLEL_CASE);
}
ଶୂନ୍ୟ V3Control::addCoverageBlockOff(ଅଚଳ string& filename, ପୂର୍ଣ୍ଣସଂଖ୍ୟା lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.add_ଧାଡ଼ି_Attribute(lineno, VPragmaType::COVERAGE_BLOCK_OFF);
}
ଶୂନ୍ୟ V3Control::addCoverageBlockOff(ଅଚଳ string& module, ଅଚଳ string& blockname) {
    V3ControlResolver::s().modules().at(module).addCoverageBlockOff(blockname);
}

ଶୂନ୍ୟ V3Control::addHierWorkers(File_ଧାଡ଼ି* fl, ଅଚଳ string& model, ପୂର୍ଣ୍ଣସଂଖ୍ୟା workers) {
    V3ControlResolver::s().addHierWorkers(fl, model, workers);
}

ଶୂନ୍ୟ V3Control::addFsmRegisterWrapper(File_ଧାଡ଼ି* fl, ଅଚଳ string& module, ଅଚଳ string& d,
                                      ଅଚଳ string& q, ଅଚଳ string& clock, ଅଚଳ string& reset,
                                      ଅଚଳ string& reset_ମୂଲ୍ୟ) {
    string missing;
    ଯଦି (module.empty()) missing += "-module";
    ଯଦି (d.empty()) missing = VString::dot(missing, ", ", "-d");
    ଯଦି (q.empty()) missing = VString::dot(missing, ", ", "-q");
    ଯଦି (clock.empty()) missing = VString::dot(missing, ", ", "-clock");
    ଯଦି (!missing.empty()) {
        fl->v3error("fsm_register_wrapper missing " << missing);
        ଫେରନ୍ତୁ;
    }

    ଅଚଳ FsmRegisterWrapper desc{module, d, q, clock, reset, reset_ମୂଲ୍ୟ};
    V3ControlResolver::s().modules().at(module).setFsmRegisterWrapper(fl, desc);
}

ଶୂନ୍ୟ V3Control::addIgnore(V3ErrorCode code, ତାର୍କିକ on, ଅଚଳ string& filename, ପୂର୍ଣ୍ଣସଂଖ୍ୟା min, ପୂର୍ଣ୍ଣସଂଖ୍ୟା max) {
    UINFO(9, "addIgnore " << code << " " << min << "-" << max << " fn=" << filename);
    ଯଦି (filename == "*") {  // For "lint_off/lint_on [--rule x]"
        File_ଧାଡ଼ି::globalWarnOff(code, !on);
    } ଅନ୍ୟଥା {  // For "lint_off/lint_on [--rule x] --file y [-lines min[-max]]"
        V3ControlResolver::s().files().at(filename).addIgnore(code, min, max, on);
    }
}
ଶୂନ୍ୟ V3Control::addIgnoreMatch(V3ErrorCode code, ଅଚଳ string& filename, ଅଚଳ string& contents,
                               ଅଚଳ string& match) {
    // For "lint_off --rule x --file y --match z", no support for lint_on
    V3ControlResolver::s().files().at(filename).addIgnoreMatch(code, contents, match);
}

ଶୂନ୍ୟ V3Control::addInline(File_ଧାଡ଼ି* fl, ଅଚଳ string& module, ଅଚଳ string& ftask, ତାର୍କିକ on) {
    ଯଦି (ftask.empty()) {
        V3ControlResolver::s().modules().at(module).setInline(on);
    } ଅନ୍ୟଥା {
        ଯଦି (!on) {
            fl->v3warn(E_UNSUPPORTED, "Unsupported: no_inline for tasks");
        } ଅନ୍ୟଥା {
            V3ControlResolver::s().modules().at(module).ftasks().at(ftask).setNoInline(on);
        }
    }
}

ଶୂନ୍ୟ V3Control::addModulePragma(ଅଚଳ string& module, VPragmaType pragma) {
    V3ControlResolver::s().modules().at(module).addModulePragma(pragma);
}

ଶୂନ୍ୟ V3Control::addProfileData(File_ଧାଡ଼ି* fl, ଅଚଳ string& hierDpi, uint64_t cost) {
    V3ControlResolver::s().addProfileData(fl, hierDpi, cost);
}
ଶୂନ୍ୟ V3Control::addProfileData(File_ଧାଡ଼ି* fl, ଅଚଳ string& model, ଅଚଳ string& ଚାବି,
                               uint64_t cost) {
    V3ControlResolver::s().addProfileData(fl, model, ଚାବି, cost);
}

ଶୂନ୍ୟ V3Control::addScopeTraceOn(ତାର୍କିକ on, ଅଚଳ string& scope, ପୂର୍ଣ୍ଣସଂଖ୍ୟା levels) {
    V3ControlResolver::s().scopeTraces().addScopeTraceOn(on, scope, levels);
}

ଶୂନ୍ୟ V3Control::addVarAttr(File_ଧାଡ଼ି* fl, ଅଚଳ string& module, ଅଚଳ string& ftask,
                           VarSpecKind kind, ଅଚଳ string& pattern, VAttrType attr,
                           Ast_Sen_ବୃକ୍ଷ* sensep) {
    ଯଦି (sensep) {
        File_ଧାଡ଼ି* ଅଚଳ flp = sensep->fileline();
        // Historical, not actually needed, only parsed for compatibility, delete it
        VL_DO_DANGLING(sensep->delete_ବୃକ୍ଷ(), sensep);
        // Used to be only accepted on public_flat_rw
        ଯଦି (attr != VAttrType::VAR_PUBLIC_FLAT_RW) {
            flp->v3error("sensitivity not expected for attribute");
            ଫେରନ୍ତୁ;
        }
    }

    ଯଦି (kind != VarSpecKind::VAR) {
        ବାଛ (attr) {
        ସ୍ଥିତି VAttrType::VAR_PUBLIC_FLAT:
        ସ୍ଥିତି VAttrType::VAR_PUBLIC_FLAT_RD:
        ସ୍ଥିତି VAttrType::VAR_PUBLIC_FLAT_RW: ଭାଙ୍ଗ;
        ପୂର୍ବନିର୍ଦ୍ଧାରିତ:
            fl->v3error("'"s + attr.ascii() + "' attribute does not accept -param/-port");
            ଫେରନ୍ତୁ;
        }
    }

    // Semantics: Most of the attributes operate on signals
    ଯଦି (pattern.empty()) {
        ଯଦି (attr == VAttrType::VAR_PUBLIC) {
            ଯଦି (ftask.empty()) {
                // public module, this is the only exception from var here
                V3ControlResolver::s().modules().at(module).addModulePragma(
                    VPragmaType::PUBLIC_MODULE);
            } ଅନ୍ୟଥା {
                V3ControlResolver::s().modules().at(module).ftasks().at(ftask).setPublic(ସତ୍ୟ);
            }
        } ଅନ୍ୟଥା {
            fl->v3error("missing -var");
        }
    } ଅନ୍ୟଥା {
        ଯଦି (attr == VAttrType::VAR_FORCEABLE) {
            ଯଦି (module.empty()) {
                fl->v3error("forceable missing -module");
            } ଅନ୍ୟଥା ଯଦି (!ftask.empty()) {
                fl->v3error("Signals inside functions/tasks cannot be marked forceable");
            } ଅନ୍ୟଥା {
                V3ControlResolver::s().modules().at(module).vars().at(pattern).add(attr);
            }
        } ଅନ୍ୟଥା {
            V3ControlModule& mod = V3ControlResolver::s().modules().at(module);
            V3ControlVar& controlVar = [&]() -> V3ControlVar& {
                ଯଦି (ftask.empty()) {
                    ଯଦି (kind == VarSpecKind::PARAM) ଫେରନ୍ତୁ mod.params().at(pattern);
                    ଯଦି (kind == VarSpecKind::PORT) ଫେରନ୍ତୁ mod.ports().at(pattern);
                    UASSERT_OBJ(kind == VarSpecKind::VAR, fl, "Unexpected VarSpecKind");
                    ଫେରନ୍ତୁ mod.vars().at(pattern);
                }
                ଯଦି (kind == VarSpecKind::PARAM) ଫେରନ୍ତୁ mod.ftasks().at(ftask).params().at(pattern);
                ଯଦି (kind == VarSpecKind::PORT) ଫେରନ୍ତୁ mod.ftasks().at(ftask).ports().at(pattern);
                UASSERT_OBJ(kind == VarSpecKind::VAR, fl, "Unexpected VarSpecKind");
                ଫେରନ୍ତୁ mod.ftasks().at(ftask).vars().at(pattern);
            }();
            controlVar.add(attr);
        }
    }
}

ଶୂନ୍ୟ V3Control::applyCase(AstCase* nodep) {
    ଅଚଳ string& filename = nodep->fileline()->filename();
    V3ControlFile* ଅଚଳ filep = V3ControlResolver::s().files().resolve(filename);
    ଯଦି (filep) filep->applyCase(nodep);
}

ଶୂନ୍ୟ V3Control::applyCoverageBlock(Ast_ଗଣ୍ଠି_Module* modulep, AstBegin* nodep) {
    ଅଚଳ string& filename = nodep->fileline()->filename();
    V3ControlFile* ଅଚଳ filep = V3ControlResolver::s().files().resolve(filename);
    ଯଦି (filep) filep->applyBlock(nodep);
    ଅଚଳ string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ଅଚଳ modp = V3ControlResolver::s().modules().resolve(modname);
    ଯଦି (modp) modp->applyBlock(nodep);
}
ଶୂନ୍ୟ V3Control::applyCoverageBlock(Ast_ଗଣ୍ଠି_Module* modulep, AstGenBlock* nodep) {
    ଅଚଳ string& filename = nodep->fileline()->filename();
    V3ControlFile* ଅଚଳ filep = V3ControlResolver::s().files().resolve(filename);
    ଯଦି (filep) filep->applyBlock(nodep);
    ଅଚଳ string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ଅଚଳ modp = V3ControlResolver::s().modules().resolve(modname);
    ଯଦି (modp) modp->applyBlock(nodep);
}

ଶୂନ୍ୟ V3Control::applyIgnores(File_ଧାଡ଼ି* filelinep) {
    ଅଚଳ string& filename = filelinep->filename();
    V3ControlFile* ଅଚଳ filep = V3ControlResolver::s().files().resolve(filename);
    ଯଦି (filep) filep->applyIgnores(filelinep);
}

ଶୂନ୍ୟ V3Control::applyModule(Ast_ଗଣ୍ଠି_Module* modulep) {
    ଅଚଳ string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ଅଚଳ modp = V3ControlResolver::s().modules().resolve(modname);
    ଯଦି (modp) modp->apply(modulep);
}

ଶୂନ୍ୟ V3Control::applyFTask(Ast_ଗଣ୍ଠି_Module* modulep, Ast_ଗଣ୍ଠି_FTask* ftaskp) {
    ଅଚଳ string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ଅଚଳ modp = V3ControlResolver::s().modules().resolve(modname);
    ଯଦି (!modp) ଫେରନ୍ତୁ;
    ଅଚଳ V3ControlFTask* ଅଚଳ ftp = modp->ftasks().resolve(ftaskp->prettyDehashOrigOrName());
    ଯଦି (ftp) ftp->apply(ftaskp);
}

template <typename T_Resolver>
ସ୍ଥିର ଶୂନ୍ୟ resolveThenApply(T_Resolver& resolver, AstVar* varp) {
    ଅଚଳ std::string name = varp->prettyDehashOrigOrName();
    ଯଦି (ଅଚଳ V3ControlVar* ଅଚଳ vp = resolver.vars().resolve(name)) vp->apply(varp);
    ଯଦି (varp->isParam()) {
        ଯଦି (ଅଚଳ V3ControlVar* ଅଚଳ vp = resolver.params().resolve(name)) vp->apply(varp);
    }
    ଯଦି (varp->isIO()) {
        ଯଦି (ଅଚଳ V3ControlVar* ଅଚଳ vp = resolver.ports().resolve(name)) vp->apply(varp);
    }
}

ଶୂନ୍ୟ V3Control::applyVarAttr(ଅଚଳ Ast_ଗଣ୍ଠି_Module* modulep, ଅଚଳ Ast_ଗଣ୍ଠି_FTask* ftaskp,
                             AstVar* varp) {
    V3ControlModule* ଅଚଳ modp
        = V3ControlResolver::s().modules().resolve(modulep->prettyDehashOrigOrName());
    ଯଦି (!modp) ଫେରନ୍ତୁ;
    ଯଦି (ftaskp) {
        V3ControlFTask* ଅଚଳ ftp = modp->ftasks().resolve(ftaskp->prettyDehashOrigOrName());
        ଯଦି (!ftp) ଫେରନ୍ତୁ;
        resolveThenApply(*ftp, varp);
    } ଅନ୍ୟଥା {
        resolveThenApply(*modp, varp);
    }
}

ପୂର୍ଣ୍ଣସଂଖ୍ୟା V3Control::getHierWorkers(ଅଚଳ string& model) {
    ଫେରନ୍ତୁ V3ControlResolver::s().getHierWorkers(model);
}
File_ଧାଡ଼ି* V3Control::get_Hier_Workers_File_ଧାଡ଼ି(ଅଚଳ string& model) {
    ଫେରନ୍ତୁ V3ControlResolver::s().get_Hier_Workers_File_ଧାଡ଼ି(model);
}
ଅଚଳ V3Control::FsmRegisterWrapper* V3Control::getFsmRegisterWrapper(ଅଚଳ string& module) {
    V3ControlModule* ଅଚଳ modp = V3ControlResolver::s().modules().resolve(module);
    ଫେରନ୍ତୁ modp ? modp->fsmRegisterWrapperp() : nullptr;
}
uint64_t V3Control::getProfileData(ଅଚଳ string& hierDpi) {
    ଫେରନ୍ତୁ V3ControlResolver::s().getProfileData(hierDpi);
}
uint64_t V3Control::getProfileData(ଅଚଳ string& model, ଅଚଳ string& ଚାବି) {
    ଫେରନ୍ତୁ V3ControlResolver::s().getProfileData(model, ଚାବି);
}
File_ଧାଡ଼ି* V3Control::get_Profile_Data_File_ଧାଡ଼ି() {
    ଫେରନ୍ତୁ V3ControlResolver::s().get_Profile_Data_File_ଧାଡ଼ି();
}
ତାର୍କିକ V3Control::getScopeTraceOn(ଅଚଳ string& scope) {
    ଫେରନ୍ତୁ V3ControlResolver::s().scopeTraces().getScopeTraceOn(scope);
}

ଶୂନ୍ୟ V3Control::contents_ଠେଲ_Text(ଅଚଳ string& text) { WildcardContents::ଠେଲ_Text(text); }

ତାର୍କିକ V3Control::containsMTaskProfileData() {
    ଫେରନ୍ତୁ V3ControlResolver::s().containsMTaskProfileData();
}
uint64_t V3Control::getCurrentHierBlockCost() {
    ଫେରନ୍ତୁ V3ControlResolver::s().getCurrentHierBlockCost();
}

ତାର୍କିକ V3Control::waive(ଅଚଳ File_ଧାଡ଼ି* filelinep, V3ErrorCode code, ଅଚଳ string& message) {
    V3ControlFile* ଅଚଳ filep = V3ControlResolver::s().files().resolve(filelinep->filename());
    ଯଦି (!filep) ଫେରନ୍ତୁ ମିଥ୍ୟା;
    ଫେରନ୍ତୁ filep->waive(code, message);
}

ଶୂନ୍ୟ V3Control::selfTest() { VInterval_ବୃକ୍ଷ::selfTest(); }
