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

#ಅನ್ತರ್ಭಾವ "V3PchAstMT.h"

#ಅನ್ತರ್ಭಾವ "V3Control.h"

#ಅನ್ತರ್ಭಾವ "V3InstrCount.h"
#ಅನ್ತರ್ಭಾವ "V3Os.h"
#ಅನ್ತರ್ಭಾವ "V3String.h"

#ಅನ್ತರ್ಭಾವ <memory>
#ಅನ್ತರ್ಭಾವ <set>
#ಅನ್ತರ್ಭಾವ <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Resolve wildcards in files, modules, ftasks or variables

// Template for a class that serves as a map for entities that can be specified
// as wildcards and are accessed by a resolved name. It rebuilds a name lookup
// cache of resolved entities. Entities stored in this container need an update
// function that takes a reference of this type to join multiple entities into one.
template <typename T>
ವರ್ಗ V3ControlWildcardResolver final {
    mutable V3Mutex m_mutex;  // protects members
    // Pattern strings (wildcard, or simple name) to entities
    std::map<ಧ್ರುವ std::string, T> m_mapPatterns VL_GUARDED_BY(m_mutex);
    // Resolved strings to converged entities - nullptr, iff none of the patterns applies
    std::map<ಧ್ರುವ std::string, std::unique_ptr<T>> m_mapResolved VL_GUARDED_BY(m_mutex);

ಸಾರ್ವಜನಿಕ:
    V3ControlWildcardResolver() = ಸ್ವಭಾವ;
    ~V3ControlWildcardResolver() = ಸ್ವಭಾವ;

    // Update this resolved file's item map by inserting other's (wildcarded filename's) items
    ಶೂನ್ಯ update(ಧ್ರುವ V3ControlWildcardResolver& other) VL_MT_SAFE_EXCLUDES(m_mutex)
        VL_EXCLUDES(other.m_mutex) {
        ಧ್ರುವ V3LockGuard lock{m_mutex};
        ಧ್ರುವ V3LockGuard otherLock{other.m_mutex};
        // Clear the resolved cache, as 'other' might add new patterns that need to be applied as
        // well.
        m_mapResolved.clear();
        ಪ್ರತಿ (ಧ್ರುವ auto& itr : other.m_mapPatterns) m_mapPatterns[itr.first].update(itr.second);
    }

    // Access and create a pattern entry
    T& at(ಧ್ರುವ string& name) VL_MT_SAFE_EXCLUDES(m_mutex) {
        ಧ್ರುವ V3LockGuard lock{m_mutex};
        // We might be adding a new entry under this, so clear the cache.
        m_mapResolved.clear();
        // Patterns use '/' as separator; normalize Windows backslashes
        ಪ್ರತ್ಯಾಗಮ m_mapPatterns[V3Os::filenameSlashPath(name)];
    }
    // Access an entity and resolve patterns that match it
    T* resolve(ಧ್ರುವ string& name) VL_MT_SAFE_EXCLUDES(m_mutex) {
        ಧ್ರುವ V3LockGuard lock{m_mutex};
        // Lookup if it was resolved before, typically not
        ಧ್ರುವ auto pair = m_mapResolved.emplace(name, nullptr);
        std::unique_ptr<T>& entryr = pair.first->second;
        // Resolve entry when first requested, cache the result
        ಯದಿ (pair.second) {
            // Inserted: update the entity with all matches in the patterns.
            // Normalize separators so Windows backslash filenames match the
            // slash-separated patterns (normalized when loaded).
            ಧ್ರುವ string slashName = V3Os::filenameSlashPath(name);
            ಪ್ರತಿ (ಧ್ರುವ auto& patEnt : m_mapPatterns) {
                ಯದಿ (VString::wildmatch(slashName, patEnt.first)) {
                    ಯದಿ (!entryr) entryr.reset(ನೂತನ T{});
                    entryr->update(patEnt.second);
                }
            }
            // Perform final actions that needed all updates completed
            ಯದಿ (entryr) entryr->updateFinalize();
        }
        ಪ್ರತ್ಯಾಗಮ entryr.get();
    }
};

// List of attributes for variables
ವರ್ಗ V3ControlVar final {
    std::vector<VAttrType> m_attrs;  // The list of attributes
ಸಾರ್ವಜನಿಕ:
    // Add new attribugte
    ಶೂನ್ಯ add(VAttrType attr) { m_attrs.emplace_back(attr); }
    // Update from other by copying all attributes
    ಶೂನ್ಯ update(ಧ್ರುವ V3ControlVar& other) {
        m_attrs.reserve(m_attrs.size() + other.m_attrs.size());
        m_attrs.ಸೇರಿಸು(m_attrs.end(), other.m_attrs.begin(), other.m_attrs.end());
    }
    ಶೂನ್ಯ updateFinalize() {}
    // Apply all attributes to the variable
    ಶೂನ್ಯ apply(AstVar* varp) ಧ್ರುವ {
        ಪ್ರತಿ (ಧ್ರುವ VAttrType attr : m_attrs) {
            varp->addAttrsp(ನೂತನ AstAttrOf{varp->fileline(), attr});
        }
    }
};

using V3ControlVarResolver = V3ControlWildcardResolver<V3ControlVar>;

//======================================================================

ವರ್ಗ WildcardContents final {
    // Not mutex protected, current calling from V3Control::waive is protected by error's mutex
    // MEMBERS
    std::map<ಧ್ರುವ std::string, ತಾರ್ಕಿಕ> m_mapPatterns;  // Pattern match results
    std::deque<string> m_ಸಾಲುಗಳು;  // Source text lines

    // METHODS
    ಸ್ಥಿರ WildcardContents& s() {  // Singleton
        ಸ್ಥಿರ WildcardContents s_s;
        ಪ್ರತ್ಯಾಗಮ s_s;
    }
    ಶೂನ್ಯ clearCacheImp() { m_mapPatterns.clear(); }
    ಶೂನ್ಯ ತಳ್ಳು_Text_Imp(ಧ್ರುವ string& text) {
        // Similar code in VFileContent::pushText()
        // Any leftover text is stored on largest line (might be "")
        ಧ್ರುವ string leftover = m_ಸಾಲುಗಳು.back() + text;
        m_ಸಾಲುಗಳು.ತೆಗೆ_back();

        // Insert line-by-line
        string::size_type ಸಾಲು_start = 0;
        ಯಾವತ್ (ಸತ್ಯ) {
            ಧ್ರುವ string::size_type ಸಾಲು_end = leftover.find('\n', ಸಾಲು_start);
            ಯದಿ (ಸಾಲು_end != string::npos) {
                ಧ್ರುವ string oneline(leftover, ಸಾಲು_start, ಸಾಲು_end - ಸಾಲು_start + 1);
                ಯದಿ (oneline.size() > 1) m_ಸಾಲುಗಳು.ತಳ್ಳು_back(oneline);  // Keeps newline
                UINFO(9, "Push[+" << (m_ಸಾಲುಗಳು.size() - 1) << "]: " << oneline);
                ಸಾಲು_start = ಸಾಲು_end + 1;
            } ಅನ್ಯಥಾ {
                ಭಙ್ಗ;
            }
        }
        // Keep leftover for next time
        m_ಸಾಲುಗಳು.emplace_back(string(leftover, ಸಾಲು_start));  // Might be ""
        clearCacheImp();
    }

    ತಾರ್ಕಿಕ resolveUncachedImp(ಧ್ರುವ string& contentsRegexp) {
        ಪ್ರತಿ (ಧ್ರುವ string& i : m_ಸಾಲುಗಳು) {
            ಯದಿ (VString::wildmatch(i, contentsRegexp)) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }
    ತಾರ್ಕಿಕ resolveCachedImp(ಧ್ರುವ string& contentsRegexp) {
        // Lookup if it was resolved before, typically is
        ಯದಿ (contentsRegexp.empty()) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        ಧ್ರುವ auto pair = m_mapPatterns.emplace(contentsRegexp, ಅಸತ್ಯ);
        ತಾರ್ಕಿಕ& entryr = pair.first->second;
        // Resolve entry when first requested, cache the result
        ಯದಿ (pair.second) entryr = resolveUncachedImp(contentsRegexp);
        ಪ್ರತ್ಯಾಗಮ entryr;
    }

ಸಾರ್ವಜನಿಕ:
    WildcardContents() {
        m_ಸಾಲುಗಳು.emplace_back("");  // start with no leftover
    }
    ~WildcardContents() = ಸ್ವಭಾವ;
    // Return true iff contentsRegexp in parsed contents
    ಸ್ಥಿರ ತಾರ್ಕಿಕ resolve(ಧ್ರುವ string& contentsRegexp) {
        ಪ್ರತ್ಯಾಗಮ s().resolveCachedImp(contentsRegexp);
    }
    // Add arbitrary text (need not be line-by-line)
    ಸ್ಥಿರ ಶೂನ್ಯ ತಳ್ಳು_Text(ಧ್ರುವ string& text) { s().ತಳ್ಳು_Text_Imp(text); }
};

//######################################################################
// Function or task: Have variables and properties

ವರ್ಗ V3ControlFTask final {
    V3ControlVarResolver m_params;  // Parameters in function/task
    V3ControlVarResolver m_ports;  // Ports in function/task
    V3ControlVarResolver m_vars;  // Variables in function/task
    ತಾರ್ಕಿಕ m_noinline = ಅಸತ್ಯ;  // Don't inline function/task
    ತಾರ್ಕಿಕ m_public = ಅಸತ್ಯ;  // Public function/task

ಸಾರ್ವಜನಿಕ:
    V3ControlFTask() = ಸ್ವಭಾವ;
    ಶೂನ್ಯ update(ಧ್ರುವ V3ControlFTask& f) {
        // Don't overwrite true with false
        ಯದಿ (f.m_noinline) m_noinline = ಸತ್ಯ;
        ಯದಿ (f.m_public) m_public = ಸತ್ಯ;
        m_params.update(f.m_params);
        m_ports.update(f.m_ports);
        m_vars.update(f.m_vars);
    }
    ಶೂನ್ಯ updateFinalize() {}

    V3ControlVarResolver& params() { ಪ್ರತ್ಯಾಗಮ m_params; }
    V3ControlVarResolver& ports() { ಪ್ರತ್ಯಾಗಮ m_ports; }
    V3ControlVarResolver& vars() { ಪ್ರತ್ಯಾಗಮ m_vars; }

    ಶೂನ್ಯ setNoInline(ತಾರ್ಕಿಕ set) { m_noinline = set; }
    ಶೂನ್ಯ setPublic(ತಾರ್ಕಿಕ set) { m_public = set; }

    ಶೂನ್ಯ apply(Ast_ಗಂಟು_FTask* ftaskp) ಧ್ರುವ {
        ಯದಿ (m_noinline)
            ftaskp->addStmtsp(ನೂತನ AstPragma{ftaskp->fileline(), VPragmaType::NO_INLINE_TASK});
        ಯದಿ (m_public)
            ftaskp->addStmtsp(ನೂತನ AstPragma{ftaskp->fileline(), VPragmaType::PUBLIC_TASK});
    }
};

using V3ControlFTaskResolver = V3ControlWildcardResolver<V3ControlFTask>;

//######################################################################
// Modules have tasks, variables, named blocks and properties

ವರ್ಗ V3ControlModule final {
    V3ControlFTaskResolver m_tasks;  // Functions/tasks in module
    V3ControlVarResolver m_params;  // Parameters in module
    V3ControlVarResolver m_ports;  // Ports in module
    V3ControlVarResolver m_vars;  // Variables in module
    V3Control::FsmRegisterWrapper m_fsmRegisterWrapper;  // FSM wrapper descriptor
    std::unordered_set<std::string> m_coverageOffBlocks;  // List of block names for coverage_off
    std::set<VPragmaType> m_modPragmas;  // List of Pragmas for modules
    ತಾರ್ಕಿಕ m_hasFsmRegisterWrapper = ಅಸತ್ಯ;  // Whether descriptor has been set
    ತಾರ್ಕಿಕ m_inline = ಅಸತ್ಯ;  // Whether to force the inline
    ತಾರ್ಕಿಕ m_inline_ಮೌಲ್ಯ = ಅಸತ್ಯ;  // The inline value (on/off)

ಸಾರ್ವಜನಿಕ:
    V3ControlModule() = ಸ್ವಭಾವ;

    ಶೂನ್ಯ update(ಧ್ರುವ V3ControlModule& m) {
        m_tasks.update(m.m_tasks);
        m_params.update(m.m_params);
        m_ports.update(m.m_ports);
        m_vars.update(m.m_vars);
        ಯದಿ (m.m_hasFsmRegisterWrapper) {
            m_fsmRegisterWrapper = m.m_fsmRegisterWrapper;
            m_hasFsmRegisterWrapper = ಸತ್ಯ;
        }
        ಪ್ರತಿ (ಧ್ರುವ string& i : m.m_coverageOffBlocks) m_coverageOffBlocks.ಸೇರಿಸು(i);
        ಯದಿ (!m_inline) {
            m_inline = m.m_inline;
            m_inline_ಮೌಲ್ಯ = m.m_inline_ಮೌಲ್ಯ;
        }
        ಪ್ರತಿ (auto it = m.m_modPragmas.cbegin(); it != m.m_modPragmas.cend(); ++it) {
            m_modPragmas.ಸೇರಿಸು(*it);
        }
    }
    ಶೂನ್ಯ updateFinalize() {}

    V3ControlFTaskResolver& ftasks() { ಪ್ರತ್ಯಾಗಮ m_tasks; }
    V3ControlVarResolver& params() { ಪ್ರತ್ಯಾಗಮ m_params; }
    V3ControlVarResolver& ports() { ಪ್ರತ್ಯಾಗಮ m_ports; }
    V3ControlVarResolver& vars() { ಪ್ರತ್ಯಾಗಮ m_vars; }

    ಶೂನ್ಯ addCoverageBlockOff(ಧ್ರುವ string& name) { m_coverageOffBlocks.ಸೇರಿಸು(name); }
    ಶೂನ್ಯ setInline(ತಾರ್ಕಿಕ set) {
        m_inline = ಸತ್ಯ;
        m_inline_ಮೌಲ್ಯ = set;
    }
    ಶೂನ್ಯ setFsmRegisterWrapper(File_ಸಾಲು* fl, ಧ್ರುವ V3Control::FsmRegisterWrapper& desc) {
        ಯದಿ (m_hasFsmRegisterWrapper) {
            fl->v3warn(BADVLTPRAGMA, "Duplicate fsm_register_wrapper descriptor for module "
                                         << Ast_ಗಂಟು::prettyNameQ(desc.moduleName)
                                         << "; replacing previous descriptor");
        }
        m_fsmRegisterWrapper = desc;
        m_hasFsmRegisterWrapper = ಸತ್ಯ;
    }
    ಧ್ರುವ V3Control::FsmRegisterWrapper* fsmRegisterWrapperp() ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ m_hasFsmRegisterWrapper ? &m_fsmRegisterWrapper : nullptr;
    }
    ಶೂನ್ಯ addModulePragma(VPragmaType pragma) { m_modPragmas.ಸೇರಿಸು(pragma); }

    ಶೂನ್ಯ apply(Ast_ಗಂಟು_Module* modp) {
        ಯದಿ (m_inline) {
            ಧ್ರುವ VPragmaType type
                = m_inline_ಮೌಲ್ಯ ? VPragmaType::INLINE_MODULE : VPragmaType::NO_INLINE_MODULE;
            Ast_ಗಂಟು* ಧ್ರುವ nodep = ನೂತನ AstPragma{modp->fileline(), type};
            modp->addStmtsp(nodep);
        }
        ಪ್ರತಿ (ಧ್ರುವ auto& itr : m_modPragmas) {
            // Catch hier param modules to mark their attributes before they are
            // flagged dead in LinkDot.
            ಯದಿ (itr == VPragmaType::HIER_PARAMS) modp->hierParams(ಸತ್ಯ);
            Ast_ಗಂಟು* ಧ್ರುವ nodep = ನೂತನ AstPragma{modp->fileline(), itr};
            modp->addStmtsp(nodep);
        }
    }

    ಶೂನ್ಯ applyBlock(AstGenBlock* nodep) {
        ಧ್ರುವ VPragmaType pragma = VPragmaType::COVERAGE_BLOCK_OFF;
        ಯದಿ (!nodep->unnamed()) {
            ಪ್ರತಿ (ಧ್ರುವ string& i : m_coverageOffBlocks) {
                ಯದಿ (VString::wildmatch(nodep->prettyDehashOrigOrName(), i)) {
                    nodep->addItemsp(ನೂತನ AstPragma{nodep->fileline(), pragma});
                }
            }
        }
    }

    ಶೂನ್ಯ applyBlock(Ast_ಗಂಟು_Block* nodep) {
        ಧ್ರುವ VPragmaType pragma = VPragmaType::COVERAGE_BLOCK_OFF;
        File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();
        ಯದಿ (!nodep->unnamed()) {
            ಪ್ರತಿ (ಧ್ರುವ string& i : m_coverageOffBlocks) {
                ಯದಿ (VString::wildmatch(nodep->prettyDehashOrigOrName(), i)) {
                    nodep->addStmtsp(ನೂತನ AstStmtPragma{flp, ನೂತನ AstPragma{flp, pragma}});
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
ವರ್ಗ V3_Control_Ignores_ಸಾಲು final {
ಸಾರ್ವಜನಿಕ:
    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ m_ಸಾಲು_Min;  // Minimum line number to make change at
    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ m_ಸಾಲು_Max;  // Maximum line number to make change at (inclusive)
    ಧ್ರುವ V3ErrorCode m_code;  // Error code
    ಧ್ರುವ ತಾರ್ಕಿಕ m_on;  // True to enable message
    V3_Control_Ignores_ಸಾಲು(V3ErrorCode code, ಪೂರ್ಣಾಙ್ಕ linemin, ಪೂರ್ಣಾಙ್ಕ linemax, ತಾರ್ಕಿಕ on)
        : m_ಸಾಲು_Min{linemin}
        , m_ಸಾಲು_Max{linemax}
        , m_code{code}
        , m_on{on} {}
    ~V3_Control_Ignores_ಸಾಲು() = ಸ್ವಭಾವ;
    ತಾರ್ಕಿಕ every_ಸಾಲು() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_ಸಾಲು_Min == 0 && m_ಸಾಲು_Max == 0; }
    ತಾರ್ಕಿಕ inRange(ಪೂರ್ಣಾಙ್ಕ lineno) ಧ್ರುವ {
        ಪ್ರತ್ಯಾಗಮ (lineno >= m_ಸಾಲು_Min && lineno <= m_ಸಾಲು_Max) || every_ಸಾಲು();
    }
};
std::ostream& operator<<(std::ostream& os, ಧ್ರುವ V3_Control_Ignores_ಸಾಲು& rhs) {
    ಪ್ರತ್ಯಾಗಮ os << rhs.m_ಸಾಲು_Min << "-" << rhs.m_ಸಾಲು_Max << ", " << rhs.m_code << ", " << rhs.m_on;
}

// Ignore line settings, index is parse order
// Single global deque to avoid copying when update()
ಸ್ಥಿರ std::vector<V3_Control_Ignores_ಸಾಲು> control_Ign_ಸಾಲುಗಳು;

using IgnIndices = std::vector<uint32_t>;  // List of {s_ignLines indces}

ವರ್ಗ VInterval_ವೃಕ್ಷ final {
    ಸಂರಚನಾ Entry final {
        // Classic centered interval tree, referencing an index to controlIgnLines
        ಪೂರ್ಣಾಙ್ಕ m_center;  // Line number of central point
        std::unique_ptr<Entry> m_leftp;  // Entry with all its rules/entries < this center
        std::unique_ptr<Entry> m_rightp;  // Entry with all its rules/entries > this center
        IgnIndices m_byMin;  // Rules overlapping center sorted by min line number
        IgnIndices m_byMax;  // Rules overlapping center sorted by max line number
        explicit Entry(ಪೂರ್ಣಾಙ್ಕ center)
            : m_center{center} {}
    };

    std::unique_ptr<Entry> m_rootp;  // Root of interval tree
    IgnIndices m_every_ಸಾಲುಗಳು;  // All-line disables

ಗುಪ್ತ:
    std::unique_ptr<Entry> build_ವೃಕ್ಷ(ಧ್ರುವ IgnIndices& points) {
        ಯದಿ (points.empty()) ಪ್ರತ್ಯಾಗಮ nullptr;

        ಪೂರ್ಣಾಙ್ಕ minval = std::numeric_limits<ಪೂರ್ಣಾಙ್ಕ>::max();
        ಪೂರ್ಣಾಙ್ಕ maxval = std::numeric_limits<ಪೂರ್ಣಾಙ್ಕ>::min();
        ಪ್ರತಿ (ಧ್ರುವ auto& it : points) {
            ಧ್ರುವ V3_Control_Ignores_ಸಾಲು& cign = control_Ign_ಸಾಲುಗಳು[it];
            ಯದಿ (cign.every_ಸಾಲು()) {
                m_every_ಸಾಲುಗಳು.emplace_back(it);
                ಅನುವರ್ತ;
            }
            minval = std::min(minval, cign.m_ಸಾಲು_Min);
            maxval = std::max(maxval, cign.m_ಸಾಲು_Max);
        }

        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ center = minval + (maxval - minval) / 2;
        auto entp = std::unique_ptr<Entry>(ನೂತನ Entry{center});

        IgnIndices ಎಡ_Points;
        IgnIndices ಬಲ_Points;
        ಪ್ರತಿ (ಧ್ರುವ auto& it : points) {
            ಧ್ರುವ V3_Control_Ignores_ಸಾಲು& cign = control_Ign_ಸಾಲುಗಳು[it];
            ಯದಿ (cign.every_ಸಾಲು()) ಅನುವರ್ತ;
            // UINFO(9, "Inserting for center " << center << " point " << it);
            ಯದಿ (cign.m_ಸಾಲು_Max < center)
                ಎಡ_Points.emplace_back(it);
            ಅನ್ಯಥಾ ಯದಿ (cign.m_ಸಾಲು_Min > center)
                ಬಲ_Points.emplace_back(it);
            ಅನ್ಯಥಾ {
                entp->m_byMin.ತಳ್ಳು_back(it);
                entp->m_byMax.ತಳ್ಳು_back(it);
            }
        }

        std::sort(entp->m_byMin.begin(), entp->m_byMin.end(), [](uint32_t a, uint32_t b) {
            ಪ್ರತ್ಯಾಗಮ control_Ign_ಸಾಲುಗಳು[a].m_ಸಾಲು_Min < control_Ign_ಸಾಲುಗಳು[b].m_ಸಾಲು_Min;
        });
        std::sort(entp->m_byMax.begin(), entp->m_byMax.end(), [](uint32_t a, uint32_t b) {
            ಪ್ರತ್ಯಾಗಮ control_Ign_ಸಾಲುಗಳು[a].m_ಸಾಲು_Max > control_Ign_ಸಾಲುಗಳು[b].m_ಸಾಲು_Max;
        });

        entp->m_leftp = build_ವೃಕ್ಷ(ಎಡ_Points);
        entp->m_rightp = build_ವೃಕ್ಷ(ಬಲ_Points);
        ಪ್ರತ್ಯಾಗಮ entp;
    }

    ಶೂನ್ಯ find_ವೃಕ್ಷ(ಪೂರ್ಣಾಙ್ಕ lineno, Entry* entp, IgnIndices& resultsr, ಪೂರ್ಣಾಙ್ಕ& nextChanger) ಧ್ರುವ {
        // UINFO(9, "Find " << lineno << " center " << entp->m_center);
        ಯದಿ (lineno < entp->m_center) {
            nextChanger = std::min(nextChanger, entp->m_center);
            ಪ್ರತಿ (ಧ್ರುವ auto& it : entp->m_byMin) {
                ಧ್ರುವ V3_Control_Ignores_ಸಾಲು& cign = control_Ign_ಸಾಲುಗಳು[it];
                ಯದಿ (cign.m_ಸಾಲು_Min <= lineno) {
                    resultsr.emplace_back(it);
                } ಅನ್ಯಥಾ {
                    nextChanger = std::min(nextChanger, cign.m_ಸಾಲು_Min);
                    ಭಙ್ಗ;
                }
            }
            ಯದಿ (entp->m_leftp) find_ವೃಕ್ಷ(lineno, entp->m_leftp.get(), resultsr, nextChanger);
        } ಅನ್ಯಥಾ ಯದಿ (lineno > entp->m_center) {
            ಪ್ರತಿ (ಧ್ರುವ auto& it : entp->m_byMax) {
                ಧ್ರುವ V3_Control_Ignores_ಸಾಲು& cign = control_Ign_ಸಾಲುಗಳು[it];
                ಯದಿ (cign.m_ಸಾಲು_Max >= lineno) {
                    nextChanger = std::min(nextChanger, cign.m_ಸಾಲು_Max + 1);
                    resultsr.emplace_back(it);
                } ಅನ್ಯಥಾ {
                    ಭಙ್ಗ;
                }
            }
            ಯದಿ (entp->m_rightp) find_ವೃಕ್ಷ(lineno, entp->m_rightp.get(), resultsr, nextChanger);
        } ಅನ್ಯಥಾ {
            ಪ್ರತಿ (ಧ್ರುವ auto& it : entp->m_byMin) {
                ಧ್ರುವ V3_Control_Ignores_ಸಾಲು& cign = control_Ign_ಸಾಲುಗಳು[it];
                resultsr.emplace_back(it);
                ಯದಿ (cign.m_ಸಾಲು_Max >= lineno)
                    nextChanger = std::min(nextChanger, cign.m_ಸಾಲು_Max + 1);
            }
            ಯದಿ (entp->m_leftp) find_ವೃಕ್ಷ(lineno, entp->m_leftp.get(), resultsr, nextChanger);
            ಯದಿ (entp->m_rightp) find_ವೃಕ್ಷ(lineno, entp->m_rightp.get(), resultsr, nextChanger);
        }
    }

ಸಾರ್ವಜನಿಕ:
    // CONSTRUCTORS
    VInterval_ವೃಕ್ಷ() = ಸ್ವಭಾವ;
    ~VInterval_ವೃಕ್ಷ() = ಸ್ವಭಾವ;
    // METHODS
    ಶೂನ್ಯ clear() { m_rootp = nullptr; }
    ಶೂನ್ಯ build(ಧ್ರುವ IgnIndices& points) {
        clear();
        m_rootp = build_ವೃಕ್ಷ(points);
    }
    ಶೂನ್ಯ find(ಪೂರ್ಣಾಙ್ಕ lineno, IgnIndices& resultsr, ಪೂರ್ಣಾಙ್ಕ& nextChanger) ಧ್ರುವ {
        resultsr = m_every_ಸಾಲುಗಳು;
        nextChanger = std::numeric_limits<ಪೂರ್ಣಾಙ್ಕ>::max();

        ಯದಿ (m_rootp) find_ವೃಕ್ಷ(lineno, m_rootp.get(), resultsr, nextChanger);
        // Sort indices, so can process in parse order
        std::sort(resultsr.begin(), resultsr.end());
    }

    ಸ್ಥಿರ ಶೂನ್ಯ selfTest() {
        //            0     10     20     30     40     50
        // i0:        .     10-10  .      .      .      .
        // i1:        .     .      20-20  .      .      .
        // i2:        .     .      .      .      40-40  .
        // i3:        .     10------------30     .      .
        // i4:        .     .      20------------40     .
        // i5:(below) 0---------------------------------------0
        IgnIndices data;
        std::vector<std::pair<ಪೂರ್ಣಾಙ್ಕ, ಪೂರ್ಣಾಙ್ಕ>> points
            = {{10, 10}, {20, 20}, {40, 40}, {10, 30}, {20, 40}};
        ಪ್ರತಿ (ಧ್ರುವ auto& it : points) {
            control_Ign_ಸಾಲುಗಳು.emplace_back(
                V3_Control_Ignores_ಸಾಲು{V3ErrorCode::I_LINT, it.first, it.second, ಸತ್ಯ});
            data.emplace_back(static_cast<uint32_t>(control_Ign_ಸಾಲುಗಳು.size() - 1));
        }
        VInterval_ವೃಕ್ಷ ವೃಕ್ಷ;
        ವೃಕ್ಷ.build(data);

        IgnIndices results;
        ಪೂರ್ಣಾಙ್ಕ nextChange = 0;
        ವೃಕ್ಷ.find(0, results, nextChange);
        UASSERT_SELFTEST(results.size(), 0);
        UASSERT_SELFTEST(nextChange, 10);
        ವೃಕ್ಷ.find(10, results, nextChange);
        UASSERT_SELFTEST(results.size(), 2);
        UASSERT_SELFTEST(results[0], 0);
        UASSERT_SELFTEST(results[1], 3);
        UASSERT_SELFTEST(nextChange, 11);
        ವೃಕ್ಷ.find(11, results, nextChange);
        UASSERT_SELFTEST(results.size(), 1);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(nextChange, 15);  // Center, or would be 20
        ವೃಕ್ಷ.find(20, results, nextChange);
        UASSERT_SELFTEST(results.size(), 3);
        UASSERT_SELFTEST(results[0], 1);
        UASSERT_SELFTEST(results[1], 3);
        UASSERT_SELFTEST(results[2], 4);
        UASSERT_SELFTEST(nextChange, 21);
        ವೃಕ್ಷ.find(21, results, nextChange);
        UASSERT_SELFTEST(results.size(), 2);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(nextChange, 25);  // Center, or would be 30
        ವೃಕ್ಷ.find(30, results, nextChange);
        UASSERT_SELFTEST(results.size(), 2);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(nextChange, 31);
        ವೃಕ್ಷ.find(40, results, nextChange);
        UASSERT_SELFTEST(results.size(), 2);
        UASSERT_SELFTEST(results[0], 2);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(nextChange, 41);
        ವೃಕ್ಷ.find(41, results, nextChange);
        UASSERT_SELFTEST(results.size(), 0);
        UASSERT_SELFTEST(nextChange, std::numeric_limits<ಪೂರ್ಣಾಙ್ಕ>::max());
        //
        points = {{0, 0}};
        ಪ್ರತಿ (ಧ್ರುವ auto& it : points) {
            control_Ign_ಸಾಲುಗಳು.emplace_back(
                V3_Control_Ignores_ಸಾಲು{V3ErrorCode::I_LINT, it.first, it.second, ಸತ್ಯ});
            data.emplace_back(static_cast<uint32_t>(control_Ign_ಸಾಲುಗಳು.size() - 1));
        }
        ವೃಕ್ಷ.build(data);
        //
        ವೃಕ್ಷ.find(50, results, nextChange);
        UASSERT_SELFTEST(results.size(), 1);
        UASSERT_SELFTEST(results[0], 5);
    }
};

// Some attributes are attached to entities of the occur on a fileline
// and multiple attributes can be attached to a line
using V3_Control_ಸಾಲು_Attribute = std::bitset<VPragmaType::_ENUM_SIZE>;

ವರ್ಗ WaiverSetting final {
ಸಾರ್ವಜನಿಕ:
    V3ErrorCode m_code;  // Error code
    string m_contents;  // --contents regexp
    string m_match;  // --match regexp
    WaiverSetting(V3ErrorCode code, ಧ್ರುವ string& contents, ಧ್ರುವ string& match)
        : m_code{code}
        , m_contents{contents}
        , m_match{match} {}
    ~WaiverSetting() = ಸ್ವಭಾವ;
    WaiverSetting& operator=(ಧ್ರುವ WaiverSetting& rhs) = ಸ್ವಭಾವ;
};

// File entity
ವರ್ಗ V3ControlFile final {
    using ಸಾಲು_Attr_Map = std::map<ಪೂರ್ಣಾಙ್ಕ, V3_Control_ಸಾಲು_Attribute>;  // Map line->bitset of attributes
    using Waivers = std::vector<WaiverSetting>;  // List of {code,wildcard string}

    ಸಾಲು_Attr_Map m_ಸಾಲು_Attrs;  // Attributes to line mapping
    IgnIndices m_ignIndices;  // s_ignLines that apply to this specific file
    Waivers m_waivers;  // Waive messages
    VInterval_ವೃಕ್ಷ m_interval_ವೃಕ್ಷ;  // Tree of indices to IgnLines

    ಸಂರಚನಾ {
        ಪೂರ್ಣಾಙ್ಕ filenameno = -1;  // Last filename
        ಪೂರ್ಣಾಙ್ಕ lineno = -1;  // Last linenumber
        ಪೂರ್ಣಾಙ್ಕ nextChange = -1;  // Line number of next change
    } m_lastIgnore;  // Last ignore line run

    // Match a given line and attribute to the map, line 0 is any
    ತಾರ್ಕಿಕ ಸಾಲು_Match(ಪೂರ್ಣಾಙ್ಕ lineno, VPragmaType type) {
        ಯದಿ (m_ಸಾಲು_Attrs.find(0) != m_ಸಾಲು_Attrs.end() && m_ಸಾಲು_Attrs[0][type]) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        ಯದಿ (m_ಸಾಲು_Attrs.find(lineno) == m_ಸಾಲು_Attrs.end()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಪ್ರತ್ಯಾಗಮ m_ಸಾಲು_Attrs[lineno][type];
    }

ಸಾರ್ವಜನಿಕ:
    V3ControlFile() = ಸ್ವಭಾವ;
    ಶೂನ್ಯ update(ಧ್ರುವ V3ControlFile& file) {
        // Copy in all attributes and waivers
        ಪ್ರತಿ (ಧ್ರುವ auto& itr : file.m_ಸಾಲು_Attrs) m_ಸಾಲು_Attrs[itr.first] |= itr.second;
        m_waivers.reserve(m_waivers.size() + file.m_waivers.size());
        m_waivers.ಸೇರಿಸು(m_waivers.end(), file.m_waivers.begin(), file.m_waivers.end());
        // Copy in all ignore references
        m_ignIndices.reserve(m_ignIndices.size() + file.m_ignIndices.size());
        m_ignIndices.ಸೇರಿಸು(m_ignIndices.end(), file.m_ignIndices.begin(),
                            file.m_ignIndices.end());
        // updateFinalize() will soon build tree
    }
    ಶೂನ್ಯ updateFinalize() { m_interval_ವೃಕ್ಷ.build(m_ignIndices); }
    ಶೂನ್ಯ add_ಸಾಲು_Attribute(ಪೂರ್ಣಾಙ್ಕ lineno, VPragmaType attr) { m_ಸಾಲು_Attrs[lineno].set(attr); }
    ಶೂನ್ಯ addIgnore(V3ErrorCode code, ಪೂರ್ಣಾಙ್ಕ min, ಪೂರ್ಣಾಙ್ಕ max, ತಾರ್ಕಿಕ on) {
        control_Ign_ಸಾಲುಗಳು.emplace_back(V3_Control_Ignores_ಸಾಲು{code, min, max, on});
        m_ignIndices.emplace_back(static_cast<uint32_t>(control_Ign_ಸಾಲುಗಳು.size() - 1));
        m_lastIgnore.nextChange = -1;
    }
    ಶೂನ್ಯ addIgnoreMatch(V3ErrorCode code, ಧ್ರುವ string& contents, ಧ್ರುವ string& match) {
        // Since Verilator 5.031 the error message compared has context, so
        // allow old rules to still match using a final '*'
        string newMatch = match;
        ಯದಿ (newMatch.empty() || newMatch.back() != '*') newMatch += '*';
        m_waivers.emplace_back(code, contents, newMatch);
    }

    ಶೂನ್ಯ applyBlock(AstGenBlock* nodep) {
        // Apply to block at this line
        ಧ್ರುವ VPragmaType pragma = VPragmaType::COVERAGE_BLOCK_OFF;
        ಯದಿ (ಸಾಲು_Match(nodep->fileline()->lineno(), pragma)) {
            nodep->addItemsp(ನೂತನ AstPragma{nodep->fileline(), pragma});
        }
    }
    ಶೂನ್ಯ applyBlock(Ast_ಗಂಟು_Block* nodep) {
        // Apply to block at this line
        ಧ್ರುವ VPragmaType pragma = VPragmaType::COVERAGE_BLOCK_OFF;
        File_ಸಾಲು* ಧ್ರುವ flp = nodep->fileline();
        ಯದಿ (ಸಾಲು_Match(flp->lineno(), pragma)) {
            nodep->addStmtsp(ನೂತನ AstStmtPragma{flp, ನೂತನ AstPragma{flp, pragma}});
        }
    }
    ಶೂನ್ಯ applyCase(AstCase* nodep) {
        // Apply to this case at this line
        ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ lineno = nodep->fileline()->lineno();
        ಯದಿ (ಸಾಲು_Match(lineno, VPragmaType::FULL_CASE)) nodep->fullPragma(ಸತ್ಯ);
        ಯದಿ (ಸಾಲು_Match(lineno, VPragmaType::PARALLEL_CASE)) nodep->parallelPragma(ಸತ್ಯ);
    }
    ಶೂನ್ಯ applyIgnores(File_ಸಾಲು* filelinep) {
        // HOT routine, called each parsed token line of this filename
        ಯದಿ (filelinep->filenameno() == m_lastIgnore.filenameno
            && filelinep->lineno() == m_lastIgnore.lineno)
            ಪ್ರತ್ಯಾಗಮ;  // Short circuit, no change, no debug
        // For speed, compute line number of next potential change, and skip if before then
        ಯದಿ (filelinep->filenameno() == m_lastIgnore.filenameno
            && filelinep->lineno() < m_lastIgnore.nextChange) {
            m_lastIgnore.lineno = filelinep->lineno();
            UINFO(9, "   ApplyIgnores for " << filelinep->ascii() << " (no change predicted)");
            ಪ್ರತ್ಯಾಗಮ;
        }
        UINFO(9, "   ApplyIgnores for " << filelinep->ascii());
        m_lastIgnore.filenameno = filelinep->filenameno();
        m_lastIgnore.lineno = filelinep->lineno();
        m_lastIgnore.nextChange = std::numeric_limits<ಪೂರ್ಣಾಙ್ಕ>::max();

        // Find all on/offs.  Unlike lint pragmas we calculate the entire
        // bitset of errors to enable, instead of individual enable/disable changes.
        IgnIndices results;
        m_interval_ವೃಕ್ಷ.find(filelinep->lineno(), results /*ref*/, m_lastIgnore.nextChange /*ref*/);
        // Process all on/offs for lines up to and including the current line
        VErrorBitSet bitset{VErrorBitSet::AllOnes{}};
        ಪ್ರತಿ (ಧ್ರುವ auto& it : results) {
            ಧ್ರುವ V3_Control_Ignores_ಸಾಲು& cign = control_Ign_ಸಾಲುಗಳು[it];
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
    ತಾರ್ಕಿಕ waive(V3ErrorCode code, ಧ್ರುವ string& message) {
        ಯದಿ (code.hardError()) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಪ್ರತಿ (ಧ್ರುವ auto& itr : m_waivers) {
            ಯದಿ ((code.isUnder(itr.m_code) || (itr.m_code == V3ErrorCode::I_LINT))
                && VString::wildmatch(message, itr.m_match)
                && WildcardContents::resolve(itr.m_contents)) {
                ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
            }
        }
        ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    }
};

using V3ControlFileResolver = V3ControlWildcardResolver<V3ControlFile>;

//######################################################################
// ScopeTrace tracking

ವರ್ಗ V3ControlScopeTraceEntry final {
ಸಾರ್ವಜನಿಕ:
    ಧ್ರುವ string m_scope;  // Scope or regexp to match
    ಧ್ರುವ ತಾರ್ಕಿಕ m_on;  // True to enable message
    ಪೂರ್ಣಾಙ್ಕ m_levels = 0;  // # levels, 0 = all, 1 = only this, ...
    // CONSTRUCTORS
    V3ControlScopeTraceEntry(ಧ್ರುವ string& scope, ತಾರ್ಕಿಕ on, ಪೂರ್ಣಾಙ್ಕ levels)
        : m_scope{scope}
        , m_on{on}
        , m_levels{levels} {}
    ~V3ControlScopeTraceEntry() = ಸ್ವಭಾವ;
    ತಾರ್ಕಿಕ operator<(ಧ್ರುವ V3ControlScopeTraceEntry& other) ಧ್ರುವ {
        ಯದಿ (m_on < other.m_on) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        ಯದಿ (m_on > other.m_on) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಯದಿ (m_levels < other.m_levels) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        ಯದಿ (m_levels > other.m_levels) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಪ್ರತ್ಯಾಗಮ m_scope < other.m_scope;
    }
};

// Tracks what matches are known to hit against V3ControlScopeTraceEntries
ವರ್ಗ V3ControlScopeTraceEntryMatch final {
ಸಾರ್ವಜನಿಕ:
    ಧ್ರುವ V3ControlScopeTraceEntry* m_entryp;  // Entry the match refers to
    ಧ್ರುವ string m_scopepart;  // Scope prefix being matched against the entry
    V3ControlScopeTraceEntryMatch(ಧ್ರುವ V3ControlScopeTraceEntry* entryp, ಧ್ರುವ string& scopepart)
        : m_entryp{entryp}
        , m_scopepart{scopepart} {}
    ತಾರ್ಕಿಕ operator<(ಧ್ರುವ V3ControlScopeTraceEntryMatch& other) ಧ್ರುವ {
        ಯದಿ (m_entryp < other.m_entryp) ಪ್ರತ್ಯಾಗಮ ಸತ್ಯ;
        ಯದಿ (m_entryp > other.m_entryp) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
        ಪ್ರತ್ಯಾಗಮ m_scopepart < other.m_scopepart;
    }
};

ವರ್ಗ V3ControlScopeTraceResolver final {
    std::vector<V3ControlScopeTraceEntry> m_entries;  // User specified on/offs and levels
    std::map<V3ControlScopeTraceEntryMatch, ತಾರ್ಕಿಕ> m_matchCache;  // Matching entries for speed

ಸಾರ್ವಜನಿಕ:
    ಶೂನ್ಯ addScopeTraceOn(ತಾರ್ಕಿಕ on, ಧ್ರುವ string& scope, ಪೂರ್ಣಾಙ್ಕ levels) {
        UINFO(9, "addScopeTraceOn " << on << " '" << scope << "' " << " levels=" << levels);
        m_entries.emplace_back(V3ControlScopeTraceEntry{scope, on, levels});
        m_matchCache.clear();
    }

    ತಾರ್ಕಿಕ getEntryMatch(ಧ್ರುವ V3ControlScopeTraceEntry* entp, ಧ್ರುವ string& scopepart) {
        // Return if a entry matches the scopepart, with memoization
        ಧ್ರುವ V3ControlScopeTraceEntryMatch ಕೀಲಿ{entp, scopepart};
        ಧ್ರುವ auto pair = m_matchCache.emplace(ಕೀಲಿ, ಅಸತ್ಯ);
        ಯದಿ (pair.second) pair.first->second = VString::wildmatch(scopepart, entp->m_scope);
        ಪ್ರತ್ಯಾಗಮ pair.first->second;
    }

    ತಾರ್ಕಿಕ getScopeTraceOn(ಧ್ರುವ string& scope) {
        // Apply in the order the user provided them, so they can choose on/off preferences
        ಪೂರ್ಣಾಙ್ಕ maxLevel = 1;
        ಪ್ರತಿ (ಧ್ರುವ auto& ch : scope) {
            ಯದಿ (ch == '.') ++maxLevel;
        }
        UINFO(9, "getScopeTraceOn " << scope << " maxLevel=" << maxLevel);

        ತಾರ್ಕಿಕ enabled = ಸತ್ಯ;
        ಪ್ರತಿ (ಧ್ರುವ auto& ent : m_entries) {
            // We apply shortest match first for each rule component
            // (Otherwise the levels would be useless as "--scope top* --levels 1" would
            // always match at every scopepart, and we wouldn't know how to count levels)
            ಪೂರ್ಣಾಙ್ಕ partLevel = 1;
            ಪ್ರತಿ (string::size_type partEnd = 0; ಸತ್ಯ;) {
                partEnd = scope.find('.', partEnd + 1);
                ಯದಿ (partEnd == string::npos) partEnd = scope.length();
                ಧ್ರುವ std::string scopepart = scope.substr(0, partEnd);
                ಯದಿ (getEntryMatch(&ent, scopepart)) {
                    ಧ್ರುವ ತಾರ್ಕಿಕ levelMatch
                        = !ent.m_levels || (ent.m_levels >= maxLevel - partLevel);
                    ಯದಿ (levelMatch) enabled = ent.m_on;
                    UINFO(9, "getScopeTraceOn-part " << scope << " enabled=" << enabled
                                                     << " @ lev=" << partLevel
                                                     << (levelMatch ? "[match]" : "[miss]")
                                                     << " from scopepart=" << scopepart);
                    ಭಙ್ಗ;
                }
                ಯದಿ (partEnd == scope.length()) ಭಙ್ಗ;
                ++partLevel;
            }
        }
        ಪ್ರತ್ಯಾಗಮ enabled;
    }
};

//######################################################################
// Resolve modules and files in the design

ವರ್ಗ V3ControlResolverHierWorkerEntry final {
    ಧ್ರುವ ಪೂರ್ಣಾಙ್ಕ m_workers;  // Number of workers requested for the hierarchical block
    File_ಸಾಲು* ಧ್ರುವ m_flp;  // Location the worker count was specified at

ಸಾರ್ವಜನಿಕ:
    explicit V3ControlResolverHierWorkerEntry(ಪೂರ್ಣಾಙ್ಕ workers, File_ಸಾಲು* flp)
        : m_workers{workers}
        , m_flp{flp} {}
    ಪೂರ್ಣಾಙ್ಕ workers() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_workers; }
    File_ಸಾಲು* flp() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_flp; }
};

ವರ್ಗ V3ControlResolver final {
    ಗಣನಾ ProfileDataMode : uint8_t { NONE = 0, MTASK = 1, HIER_DPI = 2 };
    V3ControlModuleResolver m_modules;  // Access to module names (with wildcards)
    V3ControlFileResolver m_files;  // Access to file names (with wildcards)
    V3ControlScopeTraceResolver m_scopeTraces;  // Regexp to trace enables
    std::unordered_map<string, std::unordered_map<string, uint64_t>>
        m_profileData;  // Access to profile_data records
    uint8_t m_mode = NONE;  // Kind of profile_data records is currently active
    std::unordered_map<string, V3ControlResolverHierWorkerEntry> m_hierWorkers;
    File_ಸಾಲು* m_profile_File_ಸಾಲು = nullptr;  // Location profile_data was read from

    V3ControlResolver() = ಸ್ವಭಾವ;
    ~V3ControlResolver() = ಸ್ವಭಾವ;

ಸಾರ್ವಜನಿಕ:
    ಸ್ಥಿರ V3ControlResolver& s() {
        ಸ್ಥಿರ V3ControlResolver s_singleton;
        ಪ್ರತ್ಯಾಗಮ s_singleton;
    }
    V3ControlModuleResolver& modules() { ಪ್ರತ್ಯಾಗಮ m_modules; }
    V3ControlFileResolver& files() { ಪ್ರತ್ಯಾಗಮ m_files; }
    V3ControlScopeTraceResolver& scopeTraces() { ಪ್ರತ್ಯಾಗಮ m_scopeTraces; }

    ಶೂನ್ಯ addProfileData(File_ಸಾಲು* fl, ಧ್ರುವ string& hierDpi, uint64_t cost) {
        // Empty key for hierarchical DPI wrapper costs.
        addProfileData(fl, hierDpi, "", cost, HIER_DPI);
    }
    ಶೂನ್ಯ addProfileData(File_ಸಾಲು* fl, ಧ್ರುವ string& model, ಧ್ರುವ string& ಕೀಲಿ, uint64_t cost,
                        ProfileDataMode mode = MTASK) {
        ಯದಿ (!m_profile_File_ಸಾಲು) m_profile_File_ಸಾಲು = fl;
        ಯದಿ (cost == 0) cost = 1;  // Cost 0 means delete (or no data)
        ಯದಿ (mode == MTASK) {
            m_profileData[model][ಕೀಲಿ] += cost;
        } ಅನ್ಯಥಾ ಯದಿ (mode == HIER_DPI) {
            m_profileData[model][ಕೀಲಿ] = std::max(m_profileData[model][ಕೀಲಿ], cost);
        }
        m_mode |= mode;
    }
    ತಾರ್ಕಿಕ containsMTaskProfileData() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_mode & MTASK; }
    uint64_t getProfileData(ಧ್ರುವ string& hierDpi) ಧ್ರುವ {
        // Empty key for hierarchical DPI wrapper costs.
        ಪ್ರತ್ಯಾಗಮ getProfileData(hierDpi, "");
    }
    ಶೂನ್ಯ addHierWorkers(File_ಸಾಲು* flp, ಧ್ರುವ string& model, ಪೂರ್ಣಾಙ್ಕ workers) {
        m_hierWorkers.emplace(std::piecewise_construct, std::forward_as_tuple(model),
                              std::forward_as_tuple(workers, flp));
    }
    ಪೂರ್ಣಾಙ್ಕ getHierWorkers(ಧ್ರುವ string& model) ಧ್ರುವ {
        ಧ್ರುವ auto mit = m_hierWorkers.find(model);
        // Assign a single worker if no specified.
        ಪ್ರತ್ಯಾಗಮ mit != m_hierWorkers.cend() ? mit->second.workers() : 0;
    }
    File_ಸಾಲು* get_Hier_Workers_File_ಸಾಲು(ಧ್ರುವ string& model) ಧ್ರುವ {
        ಧ್ರುವ auto mit = m_hierWorkers.find(model);
        ಪ್ರತ್ಯಾಗಮ mit != m_hierWorkers.cend() ? mit->second.flp() : v3Global.rootp()->fileline();
    }
    uint64_t getProfileData(ಧ್ರುವ string& model, ಧ್ರುವ string& ಕೀಲಿ) ಧ್ರುವ {
        ಧ್ರುವ auto mit = m_profileData.find(model);
        ಯದಿ (mit == m_profileData.cend()) ಪ್ರತ್ಯಾಗಮ 0;
        ಧ್ರುವ auto it = mit->second.find(ಕೀಲಿ);
        ಯದಿ (it == mit->second.cend()) ಪ್ರತ್ಯಾಗಮ 0;
        ಪ್ರತ್ಯಾಗಮ it->second;
    }
    File_ಸಾಲು* get_Profile_Data_File_ಸಾಲು() ಧ್ರುವ { ಪ್ರತ್ಯಾಗಮ m_profile_File_ಸಾಲು; }  // Maybe null
    ಸ್ಥಿರ uint64_t getCurrentHierBlockCost() {
        ಯದಿ (uint64_t cost = V3Control::getProfileData(v3Global.opt.prefix())) {
            UINFO(9, "Fetching cost from profile info: " << cost);
            ಪ್ರತ್ಯಾಗಮ cost;
        }

        // Without profiling data, sum the regions evaluated on each time step
        AstNetlist* ಧ್ರುವ netlistp = v3Global.rootp();
        uint64_t cost = 0;
        ಪ್ರತಿ (ಪೂರ್ಣಾಙ್ಕ i = 0; i < VEval::_ENUM_END; ++i) {
            ಧ್ರುವ VEval eval{i};
            ಯದಿ (eval.slow()) ಅನುವರ್ತ;
            cost += V3_Instr_ಎಣಿಕೆ::ಎಣಿಕೆ(netlistp->evalFuncp(eval), ಅಸತ್ಯ);
        }
        UINFO(9, "Evaluating cost: " << cost);
        ಪ್ರತ್ಯಾಗಮ cost;
    }
};

//######################################################################
// V3Control

ಶೂನ್ಯ V3Control::addCaseFull(ಧ್ರುವ string& filename, ಪೂರ್ಣಾಙ್ಕ lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.add_ಸಾಲು_Attribute(lineno, VPragmaType::FULL_CASE);
}
ಶೂನ್ಯ V3Control::addCaseParallel(ಧ್ರುವ string& filename, ಪೂರ್ಣಾಙ್ಕ lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.add_ಸಾಲು_Attribute(lineno, VPragmaType::PARALLEL_CASE);
}
ಶೂನ್ಯ V3Control::addCoverageBlockOff(ಧ್ರುವ string& filename, ಪೂರ್ಣಾಙ್ಕ lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.add_ಸಾಲು_Attribute(lineno, VPragmaType::COVERAGE_BLOCK_OFF);
}
ಶೂನ್ಯ V3Control::addCoverageBlockOff(ಧ್ರುವ string& module, ಧ್ರುವ string& blockname) {
    V3ControlResolver::s().modules().at(module).addCoverageBlockOff(blockname);
}

ಶೂನ್ಯ V3Control::addHierWorkers(File_ಸಾಲು* fl, ಧ್ರುವ string& model, ಪೂರ್ಣಾಙ್ಕ workers) {
    V3ControlResolver::s().addHierWorkers(fl, model, workers);
}

ಶೂನ್ಯ V3Control::addFsmRegisterWrapper(File_ಸಾಲು* fl, ಧ್ರುವ string& module, ಧ್ರುವ string& d,
                                      ಧ್ರುವ string& q, ಧ್ರುವ string& clock, ಧ್ರುವ string& reset,
                                      ಧ್ರುವ string& reset_ಮೌಲ್ಯ) {
    string missing;
    ಯದಿ (module.empty()) missing += "-module";
    ಯದಿ (d.empty()) missing = VString::dot(missing, ", ", "-d");
    ಯದಿ (q.empty()) missing = VString::dot(missing, ", ", "-q");
    ಯದಿ (clock.empty()) missing = VString::dot(missing, ", ", "-clock");
    ಯದಿ (!missing.empty()) {
        fl->v3error("fsm_register_wrapper missing " << missing);
        ಪ್ರತ್ಯಾಗಮ;
    }

    ಧ್ರುವ FsmRegisterWrapper desc{module, d, q, clock, reset, reset_ಮೌಲ್ಯ};
    V3ControlResolver::s().modules().at(module).setFsmRegisterWrapper(fl, desc);
}

ಶೂನ್ಯ V3Control::addIgnore(V3ErrorCode code, ತಾರ್ಕಿಕ on, ಧ್ರುವ string& filename, ಪೂರ್ಣಾಙ್ಕ min, ಪೂರ್ಣಾಙ್ಕ max) {
    UINFO(9, "addIgnore " << code << " " << min << "-" << max << " fn=" << filename);
    ಯದಿ (filename == "*") {  // For "lint_off/lint_on [--rule x]"
        File_ಸಾಲು::globalWarnOff(code, !on);
    } ಅನ್ಯಥಾ {  // For "lint_off/lint_on [--rule x] --file y [-lines min[-max]]"
        V3ControlResolver::s().files().at(filename).addIgnore(code, min, max, on);
    }
}
ಶೂನ್ಯ V3Control::addIgnoreMatch(V3ErrorCode code, ಧ್ರುವ string& filename, ಧ್ರುವ string& contents,
                               ಧ್ರುವ string& match) {
    // For "lint_off --rule x --file y --match z", no support for lint_on
    V3ControlResolver::s().files().at(filename).addIgnoreMatch(code, contents, match);
}

ಶೂನ್ಯ V3Control::addInline(File_ಸಾಲು* fl, ಧ್ರುವ string& module, ಧ್ರುವ string& ftask, ತಾರ್ಕಿಕ on) {
    ಯದಿ (ftask.empty()) {
        V3ControlResolver::s().modules().at(module).setInline(on);
    } ಅನ್ಯಥಾ {
        ಯದಿ (!on) {
            fl->v3warn(E_UNSUPPORTED, "Unsupported: no_inline for tasks");
        } ಅನ್ಯಥಾ {
            V3ControlResolver::s().modules().at(module).ftasks().at(ftask).setNoInline(on);
        }
    }
}

ಶೂನ್ಯ V3Control::addModulePragma(ಧ್ರುವ string& module, VPragmaType pragma) {
    V3ControlResolver::s().modules().at(module).addModulePragma(pragma);
}

ಶೂನ್ಯ V3Control::addProfileData(File_ಸಾಲು* fl, ಧ್ರುವ string& hierDpi, uint64_t cost) {
    V3ControlResolver::s().addProfileData(fl, hierDpi, cost);
}
ಶೂನ್ಯ V3Control::addProfileData(File_ಸಾಲು* fl, ಧ್ರುವ string& model, ಧ್ರುವ string& ಕೀಲಿ,
                               uint64_t cost) {
    V3ControlResolver::s().addProfileData(fl, model, ಕೀಲಿ, cost);
}

ಶೂನ್ಯ V3Control::addScopeTraceOn(ತಾರ್ಕಿಕ on, ಧ್ರುವ string& scope, ಪೂರ್ಣಾಙ್ಕ levels) {
    V3ControlResolver::s().scopeTraces().addScopeTraceOn(on, scope, levels);
}

ಶೂನ್ಯ V3Control::addVarAttr(File_ಸಾಲು* fl, ಧ್ರುವ string& module, ಧ್ರುವ string& ftask,
                           VarSpecKind kind, ಧ್ರುವ string& pattern, VAttrType attr,
                           Ast_Sen_ವೃಕ್ಷ* sensep) {
    ಯದಿ (sensep) {
        File_ಸಾಲು* ಧ್ರುವ flp = sensep->fileline();
        // Historical, not actually needed, only parsed for compatibility, delete it
        VL_DO_DANGLING(sensep->delete_ವೃಕ್ಷ(), sensep);
        // Used to be only accepted on public_flat_rw
        ಯದಿ (attr != VAttrType::VAR_PUBLIC_FLAT_RW) {
            flp->v3error("sensitivity not expected for attribute");
            ಪ್ರತ್ಯಾಗಮ;
        }
    }

    ಯದಿ (kind != VarSpecKind::VAR) {
        ವಿಕಲ್ಪ (attr) {
        ಪಕ್ಷ VAttrType::VAR_PUBLIC_FLAT:
        ಪಕ್ಷ VAttrType::VAR_PUBLIC_FLAT_RD:
        ಪಕ್ಷ VAttrType::VAR_PUBLIC_FLAT_RW: ಭಙ್ಗ;
        ಸ್ವಭಾವ:
            fl->v3error("'"s + attr.ascii() + "' attribute does not accept -param/-port");
            ಪ್ರತ್ಯಾಗಮ;
        }
    }

    // Semantics: Most of the attributes operate on signals
    ಯದಿ (pattern.empty()) {
        ಯದಿ (attr == VAttrType::VAR_PUBLIC) {
            ಯದಿ (ftask.empty()) {
                // public module, this is the only exception from var here
                V3ControlResolver::s().modules().at(module).addModulePragma(
                    VPragmaType::PUBLIC_MODULE);
            } ಅನ್ಯಥಾ {
                V3ControlResolver::s().modules().at(module).ftasks().at(ftask).setPublic(ಸತ್ಯ);
            }
        } ಅನ್ಯಥಾ {
            fl->v3error("missing -var");
        }
    } ಅನ್ಯಥಾ {
        ಯದಿ (attr == VAttrType::VAR_FORCEABLE) {
            ಯದಿ (module.empty()) {
                fl->v3error("forceable missing -module");
            } ಅನ್ಯಥಾ ಯದಿ (!ftask.empty()) {
                fl->v3error("Signals inside functions/tasks cannot be marked forceable");
            } ಅನ್ಯಥಾ {
                V3ControlResolver::s().modules().at(module).vars().at(pattern).add(attr);
            }
        } ಅನ್ಯಥಾ {
            V3ControlModule& mod = V3ControlResolver::s().modules().at(module);
            V3ControlVar& controlVar = [&]() -> V3ControlVar& {
                ಯದಿ (ftask.empty()) {
                    ಯದಿ (kind == VarSpecKind::PARAM) ಪ್ರತ್ಯಾಗಮ mod.params().at(pattern);
                    ಯದಿ (kind == VarSpecKind::PORT) ಪ್ರತ್ಯಾಗಮ mod.ports().at(pattern);
                    UASSERT_OBJ(kind == VarSpecKind::VAR, fl, "Unexpected VarSpecKind");
                    ಪ್ರತ್ಯಾಗಮ mod.vars().at(pattern);
                }
                ಯದಿ (kind == VarSpecKind::PARAM) ಪ್ರತ್ಯಾಗಮ mod.ftasks().at(ftask).params().at(pattern);
                ಯದಿ (kind == VarSpecKind::PORT) ಪ್ರತ್ಯಾಗಮ mod.ftasks().at(ftask).ports().at(pattern);
                UASSERT_OBJ(kind == VarSpecKind::VAR, fl, "Unexpected VarSpecKind");
                ಪ್ರತ್ಯಾಗಮ mod.ftasks().at(ftask).vars().at(pattern);
            }();
            controlVar.add(attr);
        }
    }
}

ಶೂನ್ಯ V3Control::applyCase(AstCase* nodep) {
    ಧ್ರುವ string& filename = nodep->fileline()->filename();
    V3ControlFile* ಧ್ರುವ filep = V3ControlResolver::s().files().resolve(filename);
    ಯದಿ (filep) filep->applyCase(nodep);
}

ಶೂನ್ಯ V3Control::applyCoverageBlock(Ast_ಗಂಟು_Module* modulep, AstBegin* nodep) {
    ಧ್ರುವ string& filename = nodep->fileline()->filename();
    V3ControlFile* ಧ್ರುವ filep = V3ControlResolver::s().files().resolve(filename);
    ಯದಿ (filep) filep->applyBlock(nodep);
    ಧ್ರುವ string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ಧ್ರುವ modp = V3ControlResolver::s().modules().resolve(modname);
    ಯದಿ (modp) modp->applyBlock(nodep);
}
ಶೂನ್ಯ V3Control::applyCoverageBlock(Ast_ಗಂಟು_Module* modulep, AstGenBlock* nodep) {
    ಧ್ರುವ string& filename = nodep->fileline()->filename();
    V3ControlFile* ಧ್ರುವ filep = V3ControlResolver::s().files().resolve(filename);
    ಯದಿ (filep) filep->applyBlock(nodep);
    ಧ್ರುವ string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ಧ್ರುವ modp = V3ControlResolver::s().modules().resolve(modname);
    ಯದಿ (modp) modp->applyBlock(nodep);
}

ಶೂನ್ಯ V3Control::applyIgnores(File_ಸಾಲು* filelinep) {
    ಧ್ರುವ string& filename = filelinep->filename();
    V3ControlFile* ಧ್ರುವ filep = V3ControlResolver::s().files().resolve(filename);
    ಯದಿ (filep) filep->applyIgnores(filelinep);
}

ಶೂನ್ಯ V3Control::applyModule(Ast_ಗಂಟು_Module* modulep) {
    ಧ್ರುವ string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ಧ್ರುವ modp = V3ControlResolver::s().modules().resolve(modname);
    ಯದಿ (modp) modp->apply(modulep);
}

ಶೂನ್ಯ V3Control::applyFTask(Ast_ಗಂಟು_Module* modulep, Ast_ಗಂಟು_FTask* ftaskp) {
    ಧ್ರುವ string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ಧ್ರುವ modp = V3ControlResolver::s().modules().resolve(modname);
    ಯದಿ (!modp) ಪ್ರತ್ಯಾಗಮ;
    ಧ್ರುವ V3ControlFTask* ಧ್ರುವ ftp = modp->ftasks().resolve(ftaskp->prettyDehashOrigOrName());
    ಯದಿ (ftp) ftp->apply(ftaskp);
}

template <typename T_Resolver>
ಸ್ಥಿರ ಶೂನ್ಯ resolveThenApply(T_Resolver& resolver, AstVar* varp) {
    ಧ್ರುವ std::string name = varp->prettyDehashOrigOrName();
    ಯದಿ (ಧ್ರುವ V3ControlVar* ಧ್ರುವ vp = resolver.vars().resolve(name)) vp->apply(varp);
    ಯದಿ (varp->isParam()) {
        ಯದಿ (ಧ್ರುವ V3ControlVar* ಧ್ರುವ vp = resolver.params().resolve(name)) vp->apply(varp);
    }
    ಯದಿ (varp->isIO()) {
        ಯದಿ (ಧ್ರುವ V3ControlVar* ಧ್ರುವ vp = resolver.ports().resolve(name)) vp->apply(varp);
    }
}

ಶೂನ್ಯ V3Control::applyVarAttr(ಧ್ರುವ Ast_ಗಂಟು_Module* modulep, ಧ್ರುವ Ast_ಗಂಟು_FTask* ftaskp,
                             AstVar* varp) {
    V3ControlModule* ಧ್ರುವ modp
        = V3ControlResolver::s().modules().resolve(modulep->prettyDehashOrigOrName());
    ಯದಿ (!modp) ಪ್ರತ್ಯಾಗಮ;
    ಯದಿ (ftaskp) {
        V3ControlFTask* ಧ್ರುವ ftp = modp->ftasks().resolve(ftaskp->prettyDehashOrigOrName());
        ಯದಿ (!ftp) ಪ್ರತ್ಯಾಗಮ;
        resolveThenApply(*ftp, varp);
    } ಅನ್ಯಥಾ {
        resolveThenApply(*modp, varp);
    }
}

ಪೂರ್ಣಾಙ್ಕ V3Control::getHierWorkers(ಧ್ರುವ string& model) {
    ಪ್ರತ್ಯಾಗಮ V3ControlResolver::s().getHierWorkers(model);
}
File_ಸಾಲು* V3Control::get_Hier_Workers_File_ಸಾಲು(ಧ್ರುವ string& model) {
    ಪ್ರತ್ಯಾಗಮ V3ControlResolver::s().get_Hier_Workers_File_ಸಾಲು(model);
}
ಧ್ರುವ V3Control::FsmRegisterWrapper* V3Control::getFsmRegisterWrapper(ಧ್ರುವ string& module) {
    V3ControlModule* ಧ್ರುವ modp = V3ControlResolver::s().modules().resolve(module);
    ಪ್ರತ್ಯಾಗಮ modp ? modp->fsmRegisterWrapperp() : nullptr;
}
uint64_t V3Control::getProfileData(ಧ್ರುವ string& hierDpi) {
    ಪ್ರತ್ಯಾಗಮ V3ControlResolver::s().getProfileData(hierDpi);
}
uint64_t V3Control::getProfileData(ಧ್ರುವ string& model, ಧ್ರುವ string& ಕೀಲಿ) {
    ಪ್ರತ್ಯಾಗಮ V3ControlResolver::s().getProfileData(model, ಕೀಲಿ);
}
File_ಸಾಲು* V3Control::get_Profile_Data_File_ಸಾಲು() {
    ಪ್ರತ್ಯಾಗಮ V3ControlResolver::s().get_Profile_Data_File_ಸಾಲು();
}
ತಾರ್ಕಿಕ V3Control::getScopeTraceOn(ಧ್ರುವ string& scope) {
    ಪ್ರತ್ಯಾಗಮ V3ControlResolver::s().scopeTraces().getScopeTraceOn(scope);
}

ಶೂನ್ಯ V3Control::contents_ತಳ್ಳು_Text(ಧ್ರುವ string& text) { WildcardContents::ತಳ್ಳು_Text(text); }

ತಾರ್ಕಿಕ V3Control::containsMTaskProfileData() {
    ಪ್ರತ್ಯಾಗಮ V3ControlResolver::s().containsMTaskProfileData();
}
uint64_t V3Control::getCurrentHierBlockCost() {
    ಪ್ರತ್ಯಾಗಮ V3ControlResolver::s().getCurrentHierBlockCost();
}

ತಾರ್ಕಿಕ V3Control::waive(ಧ್ರುವ File_ಸಾಲು* filelinep, V3ErrorCode code, ಧ್ರುವ string& message) {
    V3ControlFile* ಧ್ರುವ filep = V3ControlResolver::s().files().resolve(filelinep->filename());
    ಯದಿ (!filep) ಪ್ರತ್ಯಾಗಮ ಅಸತ್ಯ;
    ಪ್ರತ್ಯಾಗಮ filep->waive(code, message);
}

ಶೂನ್ಯ V3Control::selfTest() { VInterval_ವೃಕ್ಷ::selfTest(); }
