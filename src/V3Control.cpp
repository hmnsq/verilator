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

#ضمن "V3PchAstMT.h"

#ضمن "V3Control.h"

#ضمن "V3InstrCount.h"
#ضمن "V3Os.h"
#ضمن "V3String.h"

#ضمن <memory>
#ضمن <set>
#ضمن <unordered_map>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Resolve wildcards in files, modules, ftasks or variables

// Template for a class that serves as a map for entities that can be specified
// as wildcards and are accessed by a resolved name. It rebuilds a name lookup
// cache of resolved entities. Entities stored in this container need an update
// function that takes a reference of this type to join multiple entities into one.
سانچو <قسم_نالو T>
صنف V3ControlWildcardResolver final {
    بدلجندڙ V3Mutex m_mutex;  // protects members
    // Pattern strings (wildcard, or simple name) to entities
    std::map<ثابت std::string, T> m_mapPatterns VL_GUARDED_BY(m_mutex);
    // Resolved strings to converged entities - nullptr, iff none of the patterns applies
    std::map<ثابت std::string, std::unique_ptr<T>> m_mapResolved VL_GUARDED_BY(m_mutex);

عام:
    V3ControlWildcardResolver() = افتراضي;
    ~V3ControlWildcardResolver() = افتراضي;

    // Update this resolved file's item map by inserting other's (wildcarded filename's) items
    فراغ update(ثابت V3ControlWildcardResolver& other) VL_MT_محفوظ_EXCLUDES(m_mutex)
        VL_EXCLUDES(other.m_mutex) {
        ثابت V3_تالو_Guard تالو_2{m_mutex};
        ثابت V3_تالو_Guard other_تالو{other.m_mutex};
        // Clear the resolved cache, as 'other' might add new patterns that need to be applied as
        // well.
        m_mapResolved.clear();
        لكل (ثابت خودڪار& itr : other.m_mapPatterns) m_mapPatterns[itr.پهريون].update(itr.ٻيو);
    }

    // Access and create a pattern entry
    T& at(ثابت string& name) VL_MT_محفوظ_EXCLUDES(m_mutex) {
        ثابت V3_تالو_Guard تالو_2{m_mutex};
        // We might be adding a new entry under this, so clear the cache.
        m_mapResolved.clear();
        // Patterns use '/' as separator; normalize Windows backslashes
        أرجع m_mapPatterns[V3Os::filename_Slash_رستو(name)];
    }
    // Access an entity and resolve patterns that match it
    T* resolve(ثابت string& name) VL_MT_محفوظ_EXCLUDES(m_mutex) {
        ثابت V3_تالو_Guard تالو_2{m_mutex};
        // Lookup if it was resolved before, typically not
        ثابت خودڪار pair = m_mapResolved.emplace(name, خالي_اشارو);
        std::unique_ptr<T>& entryr = pair.پهريون->ٻيو;
        // Resolve entry when first requested, cache the result
        إذا (pair.ٻيو) {
            // Inserted: update the entity with all matches in the patterns.
            // Normalize separators so Windows backslash filenames match the
            // slash-separated patterns (normalized when loaded).
            ثابت string slashName = V3Os::filename_Slash_رستو(name);
            لكل (ثابت خودڪار& patEnt : m_mapPatterns) {
                إذا (VString::wildmatch(slashName, patEnt.پهريون)) {
                    إذا (!entryr) entryr.reset(جديد T{});
                    entryr->update(patEnt.ٻيو);
                }
            }
            // Perform final actions that needed all updates completed
            إذا (entryr) entryr->updateFinalize();
        }
        أرجع entryr.get();
    }
};

// List of attributes for variables
صنف V3ControlVar final {
    std::ويڪٽر<VAttrType> m_attrs;  // The list of attributes
عام:
    // Add new attribugte
    فراغ جوڙيو(VAttrType attr) { m_attrs.emplace_پٺ(attr); }
    // Update from other by copying all attributes
    فراغ update(ثابت V3ControlVar& other) {
        m_attrs.reserve(m_attrs.سائيز() + other.m_attrs.سائيز());
        m_attrs.وجهو(m_attrs.پڇاڙي(), other.m_attrs.begin(), other.m_attrs.پڇاڙي());
    }
    فراغ updateFinalize() {}
    // Apply all attributes to the variable
    فراغ apply(AstVar* varp) ثابت {
        لكل (ثابت VAttrType attr : m_attrs) {
            varp->جوڙيو_Attrsp(جديد Ast_Attr_جو{varp->fileline(), attr});
        }
    }
};

استعمال V3ControlVarResolver = V3ControlWildcardResolver<V3ControlVar>;

//======================================================================

صنف WildcardContents final {
    // Not mutex protected, current calling from V3Control::waive is protected by error's mutex
    // MEMBERS
    std::map<ثابت std::string, منطقي> m_mapPatterns;  // Pattern match results
    std::deque<string> m_سٽون;  // Source text lines

    // METHODS
    ساكن WildcardContents& s() {  // Singleton
        ساكن WildcardContents s_s;
        أرجع s_s;
    }
    فراغ clearCacheImp() { m_mapPatterns.clear(); }
    فراغ ڌڪو_متن_Imp(ثابت string& متن) {
        // Similar code in VFileContent::pushText()
        // Any leftover text is stored on largest line (might be "")
        ثابت string leftover = m_سٽون.پٺ() + متن;
        m_سٽون.ڪڍو_پٺ();

        // Insert line-by-line
        string::سائيز_type سٽ_شروعات = 0;
        طالما (سچ) {
            ثابت string::سائيز_type سٽ_پڇاڙي = leftover.ڳوليو('\n', سٽ_شروعات);
            إذا (سٽ_پڇاڙي != string::npos) {
                ثابت string oneline(leftover, سٽ_شروعات, سٽ_پڇاڙي - سٽ_شروعات + 1);
                إذا (oneline.سائيز() > 1) m_سٽون.ڌڪو_پٺ(oneline);  // Keeps newline
                UINFO(9, "Push[+" << (m_سٽون.سائيز() - 1) << "]: " << oneline);
                سٽ_شروعات = سٽ_پڇاڙي + 1;
            } وإلا {
                اكسر;
            }
        }
        // Keep leftover for next time
        m_سٽون.emplace_پٺ(string(leftover, سٽ_شروعات));  // Might be ""
        clearCacheImp();
    }

    منطقي resolveUncachedImp(ثابت string& contentsRegexp) {
        لكل (ثابت string& i : m_سٽون) {
            إذا (VString::wildmatch(i, contentsRegexp)) أرجع سچ;
        }
        أرجع خطأ;
    }
    منطقي resolveCachedImp(ثابت string& contentsRegexp) {
        // Lookup if it was resolved before, typically is
        إذا (contentsRegexp.خالي_جايون()) أرجع سچ;
        ثابت خودڪار pair = m_mapPatterns.emplace(contentsRegexp, خطأ);
        منطقي& entryr = pair.پهريون->ٻيو;
        // Resolve entry when first requested, cache the result
        إذا (pair.ٻيو) entryr = resolveUncachedImp(contentsRegexp);
        أرجع entryr;
    }

عام:
    WildcardContents() {
        m_سٽون.emplace_پٺ("");  // start with no leftover
    }
    ~WildcardContents() = افتراضي;
    // Return true iff contentsRegexp in parsed contents
    ساكن منطقي resolve(ثابت string& contentsRegexp) {
        أرجع s().resolveCachedImp(contentsRegexp);
    }
    // Add arbitrary text (need not be line-by-line)
    ساكن فراغ ڌڪو_متن(ثابت string& متن) { s().ڌڪو_متن_Imp(متن); }
};

//######################################################################
// Function or task: Have variables and properties

صنف V3ControlFTask final {
    V3ControlVarResolver m_params;  // Parameters in function/task
    V3ControlVarResolver m_ports;  // Ports in function/task
    V3ControlVarResolver m_vars;  // Variables in function/task
    منطقي m_noinline = خطأ;  // Don't inline function/task
    منطقي m_عوامي = خطأ;  // Public function/task

عام:
    V3ControlFTask() = افتراضي;
    فراغ update(ثابت V3ControlFTask& f) {
        // Don't overwrite true with false
        إذا (f.m_noinline) m_noinline = سچ;
        إذا (f.m_عوامي) m_عوامي = سچ;
        m_params.update(f.m_params);
        m_ports.update(f.m_ports);
        m_vars.update(f.m_vars);
    }
    فراغ updateFinalize() {}

    V3ControlVarResolver& params() { أرجع m_params; }
    V3ControlVarResolver& ports() { أرجع m_ports; }
    V3ControlVarResolver& vars() { أرجع m_vars; }

    فراغ سيٽ_No_Inline(منطقي سيٽ) { m_noinline = سيٽ; }
    فراغ سيٽ_عوامي(منطقي سيٽ) { m_عوامي = سيٽ; }

    فراغ apply(Ast_ڳنڍ_FTask* ftaskp) ثابت {
        إذا (m_noinline)
            ftaskp->جوڙيو_Stmtsp(جديد AstPragma{ftaskp->fileline(), VPragmaType::NO_INLINE_TASK});
        إذا (m_عوامي)
            ftaskp->جوڙيو_Stmtsp(جديد AstPragma{ftaskp->fileline(), VPragmaType::عوامي_TASK});
    }
};

استعمال V3ControlFTaskResolver = V3ControlWildcardResolver<V3ControlFTask>;

//######################################################################
// Modules have tasks, variables, named blocks and properties

صنف V3ControlModule final {
    V3ControlFTaskResolver m_tasks;  // Functions/tasks in module
    V3ControlVarResolver m_params;  // Parameters in module
    V3ControlVarResolver m_ports;  // Ports in module
    V3ControlVarResolver m_vars;  // Variables in module
    V3Control::Fsm_رجسٽر_Wrapper m_fsm_رجسٽر_Wrapper;  // FSM wrapper descriptor
    std::unordered_سيٽ<std::string> m_coverage_Off_بلاڪون;  // List of block names for coverage_off
    std::سيٽ<VPragmaType> m_modPragmas;  // List of Pragmas for modules
    منطقي m_has_Fsm_رجسٽر_Wrapper = خطأ;  // Whether descriptor has been set
    منطقي m_inline = خطأ;  // Whether to force the inline
    منطقي m_inline_قدر = خطأ;  // The inline value (on/off)

عام:
    V3ControlModule() = افتراضي;

    فراغ update(ثابت V3ControlModule& m) {
        m_tasks.update(m.m_tasks);
        m_params.update(m.m_params);
        m_ports.update(m.m_ports);
        m_vars.update(m.m_vars);
        إذا (m.m_has_Fsm_رجسٽر_Wrapper) {
            m_fsm_رجسٽر_Wrapper = m.m_fsm_رجسٽر_Wrapper;
            m_has_Fsm_رجسٽر_Wrapper = سچ;
        }
        لكل (ثابت string& i : m.m_coverage_Off_بلاڪون) m_coverage_Off_بلاڪون.وجهو(i);
        إذا (!m_inline) {
            m_inline = m.m_inline;
            m_inline_قدر = m.m_inline_قدر;
        }
        لكل (خودڪار it = m.m_modPragmas.cbegin(); it != m.m_modPragmas.cend(); ++it) {
            m_modPragmas.وجهو(*it);
        }
    }
    فراغ updateFinalize() {}

    V3ControlFTaskResolver& ftasks() { أرجع m_tasks; }
    V3ControlVarResolver& params() { أرجع m_params; }
    V3ControlVarResolver& ports() { أرجع m_ports; }
    V3ControlVarResolver& vars() { أرجع m_vars; }

    فراغ جوڙيو_Coverage_بلاڪ_Off(ثابت string& name) { m_coverage_Off_بلاڪون.وجهو(name); }
    فراغ سيٽ_Inline(منطقي سيٽ) {
        m_inline = سچ;
        m_inline_قدر = سيٽ;
    }
    فراغ سيٽ_Fsm_رجسٽر_Wrapper(File_سٽ* fl, ثابت V3Control::Fsm_رجسٽر_Wrapper& desc) {
        إذا (m_has_Fsm_رجسٽر_Wrapper) {
            fl->v3warn(BADVLTPRAGMA, "Duplicate fsm_register_wrapper descriptor for module "
                                         << Ast_ڳنڍ::prettyNameQ(desc.moduleName)
                                         << "; replacing previous descriptor");
        }
        m_fsm_رجسٽر_Wrapper = desc;
        m_has_Fsm_رجسٽر_Wrapper = سچ;
    }
    ثابت V3Control::Fsm_رجسٽر_Wrapper* fsm_رجسٽر_Wrapperp() ثابت {
        أرجع m_has_Fsm_رجسٽر_Wrapper ? &m_fsm_رجسٽر_Wrapper : خالي_اشارو;
    }
    فراغ جوڙيو_Module_Pragma(VPragmaType pragma) { m_modPragmas.وجهو(pragma); }

    فراغ apply(Ast_ڳنڍ_Module* modp) {
        إذا (m_inline) {
            ثابت VPragmaType type
                = m_inline_قدر ? VPragmaType::INLINE_MODULE : VPragmaType::NO_INLINE_MODULE;
            Ast_ڳنڍ* ثابت nodep = جديد AstPragma{modp->fileline(), type};
            modp->جوڙيو_Stmtsp(nodep);
        }
        لكل (ثابت خودڪار& itr : m_modPragmas) {
            // Catch hier param modules to mark their attributes before they are
            // flagged dead in LinkDot.
            إذا (itr == VPragmaType::HIER_PARAMS) modp->hierParams(سچ);
            Ast_ڳنڍ* ثابت nodep = جديد AstPragma{modp->fileline(), itr};
            modp->جوڙيو_Stmtsp(nodep);
        }
    }

    فراغ apply_بلاڪ(Ast_Gen_بلاڪ* nodep) {
        ثابت VPragmaType pragma = VPragmaType::COVERAGE_بلاڪ_OFF;
        إذا (!nodep->unnamed()) {
            لكل (ثابت string& i : m_coverage_Off_بلاڪون) {
                إذا (VString::wildmatch(nodep->prettyDehashOrigOrName(), i)) {
                    nodep->جوڙيو_Itemsp(جديد AstPragma{nodep->fileline(), pragma});
                }
            }
        }
    }

    فراغ apply_بلاڪ(Ast_ڳنڍ_بلاڪ* nodep) {
        ثابت VPragmaType pragma = VPragmaType::COVERAGE_بلاڪ_OFF;
        File_سٽ* ثابت flp = nodep->fileline();
        إذا (!nodep->unnamed()) {
            لكل (ثابت string& i : m_coverage_Off_بلاڪون) {
                إذا (VString::wildmatch(nodep->prettyDehashOrigOrName(), i)) {
                    nodep->جوڙيو_Stmtsp(جديد AstStmtPragma{flp, جديد AstPragma{flp, pragma}});
                }
            }
        }
    }
};

استعمال V3ControlModuleResolver = V3ControlWildcardResolver<V3ControlModule>;

//######################################################################
// Files have:
//  - Line ignores (lint/coverage/tracing on/off)
//  - Line attributes: Attributes attached to lines

// lint/coverage/tracing on/off
صنف V3_Control_Ignores_سٽ final {
عام:
    ثابت صحيح m_سٽ_Min;  // Minimum line number to make change at
    ثابت صحيح m_سٽ_Max;  // Maximum line number to make change at (inclusive)
    ثابت V3_Error_ڪوڊ m_ڪوڊ;  // Error code
    ثابت منطقي m_on;  // True to enable message
    V3_Control_Ignores_سٽ(V3_Error_ڪوڊ ڪوڊ, صحيح linemin, صحيح linemax, منطقي on)
        : m_سٽ_Min{linemin}
        , m_سٽ_Max{linemax}
        , m_ڪوڊ{ڪوڊ}
        , m_on{on} {}
    ~V3_Control_Ignores_سٽ() = افتراضي;
    منطقي every_سٽ() ثابت { أرجع m_سٽ_Min == 0 && m_سٽ_Max == 0; }
    منطقي اندريون_Range(صحيح lineno) ثابت {
        أرجع (lineno >= m_سٽ_Min && lineno <= m_سٽ_Max) || every_سٽ();
    }
};
std::ostream& عامل<<(std::ostream& os, ثابت V3_Control_Ignores_سٽ& rhs) {
    أرجع os << rhs.m_سٽ_Min << "-" << rhs.m_سٽ_Max << ", " << rhs.m_ڪوڊ << ", " << rhs.m_on;
}

// Ignore line settings, index is parse order
// Single global deque to avoid copying when update()
ساكن std::ويڪٽر<V3_Control_Ignores_سٽ> control_Ign_سٽون;

استعمال IgnIndices = std::ويڪٽر<uint32_t>;  // List of {s_ignLines indces}

صنف VInterval_وڻ final {
    بنية داخلا_3 final {
        // Classic centered interval tree, referencing an index to controlIgnLines
        صحيح m_center;  // Line number of central point
        std::unique_ptr<داخلا_3> m_leftp;  // Entry with all its rules/entries < this center
        std::unique_ptr<داخلا_3> m_rightp;  // Entry with all its rules/entries > this center
        IgnIndices m_byMin;  // Rules overlapping center sorted by min line number
        IgnIndices m_byMax;  // Rules overlapping center sorted by max line number
        واضح داخلا_3(صحيح center)
            : m_center{center} {}
    };

    std::unique_ptr<داخلا_3> m_rootp;  // Root of interval tree
    IgnIndices m_every_سٽون;  // All-line disables

خاص:
    std::unique_ptr<داخلا_3> build_وڻ(ثابت IgnIndices& points) {
        إذا (points.خالي_جايون()) أرجع خالي_اشارو;

        صحيح minval = std::numeric_limits<صحيح>::max();
        صحيح maxval = std::numeric_limits<صحيح>::min();
        لكل (ثابت خودڪار& it : points) {
            ثابت V3_Control_Ignores_سٽ& cign = control_Ign_سٽون[it];
            إذا (cign.every_سٽ()) {
                m_every_سٽون.emplace_پٺ(it);
                استمر;
            }
            minval = std::min(minval, cign.m_سٽ_Min);
            maxval = std::max(maxval, cign.m_سٽ_Max);
        }

        ثابت صحيح center = minval + (maxval - minval) / 2;
        خودڪار entp = std::unique_ptr<داخلا_3>(جديد داخلا_3{center});

        IgnIndices کاٻو_Points;
        IgnIndices ساڄو_Points;
        لكل (ثابت خودڪار& it : points) {
            ثابت V3_Control_Ignores_سٽ& cign = control_Ign_سٽون[it];
            إذا (cign.every_سٽ()) استمر;
            // UINFO(9, "Inserting for center " << center << " point " << it);
            إذا (cign.m_سٽ_Max < center)
                کاٻو_Points.emplace_پٺ(it);
            وإلا إذا (cign.m_سٽ_Min > center)
                ساڄو_Points.emplace_پٺ(it);
            وإلا {
                entp->m_byMin.ڌڪو_پٺ(it);
                entp->m_byMax.ڌڪو_پٺ(it);
            }
        }

        std::ترتيب_ڏيو(entp->m_byMin.begin(), entp->m_byMin.پڇاڙي(), [](uint32_t a, uint32_t b) {
            أرجع control_Ign_سٽون[a].m_سٽ_Min < control_Ign_سٽون[b].m_سٽ_Min;
        });
        std::ترتيب_ڏيو(entp->m_byMax.begin(), entp->m_byMax.پڇاڙي(), [](uint32_t a, uint32_t b) {
            أرجع control_Ign_سٽون[a].m_سٽ_Max > control_Ign_سٽون[b].m_سٽ_Max;
        });

        entp->m_leftp = build_وڻ(کاٻو_Points);
        entp->m_rightp = build_وڻ(ساڄو_Points);
        أرجع entp;
    }

    فراغ ڳوليو_وڻ(صحيح lineno, داخلا_3* entp, IgnIndices& resultsr, صحيح& ايندڙ_Changer) ثابت {
        // UINFO(9, "Find " << lineno << " center " << entp->m_center);
        إذا (lineno < entp->m_center) {
            ايندڙ_Changer = std::min(ايندڙ_Changer, entp->m_center);
            لكل (ثابت خودڪار& it : entp->m_byMin) {
                ثابت V3_Control_Ignores_سٽ& cign = control_Ign_سٽون[it];
                إذا (cign.m_سٽ_Min <= lineno) {
                    resultsr.emplace_پٺ(it);
                } وإلا {
                    ايندڙ_Changer = std::min(ايندڙ_Changer, cign.m_سٽ_Min);
                    اكسر;
                }
            }
            إذا (entp->m_leftp) ڳوليو_وڻ(lineno, entp->m_leftp.get(), resultsr, ايندڙ_Changer);
        } وإلا إذا (lineno > entp->m_center) {
            لكل (ثابت خودڪار& it : entp->m_byMax) {
                ثابت V3_Control_Ignores_سٽ& cign = control_Ign_سٽون[it];
                إذا (cign.m_سٽ_Max >= lineno) {
                    ايندڙ_Changer = std::min(ايندڙ_Changer, cign.m_سٽ_Max + 1);
                    resultsr.emplace_پٺ(it);
                } وإلا {
                    اكسر;
                }
            }
            إذا (entp->m_rightp) ڳوليو_وڻ(lineno, entp->m_rightp.get(), resultsr, ايندڙ_Changer);
        } وإلا {
            لكل (ثابت خودڪار& it : entp->m_byMin) {
                ثابت V3_Control_Ignores_سٽ& cign = control_Ign_سٽون[it];
                resultsr.emplace_پٺ(it);
                إذا (cign.m_سٽ_Max >= lineno)
                    ايندڙ_Changer = std::min(ايندڙ_Changer, cign.m_سٽ_Max + 1);
            }
            إذا (entp->m_leftp) ڳوليو_وڻ(lineno, entp->m_leftp.get(), resultsr, ايندڙ_Changer);
            إذا (entp->m_rightp) ڳوليو_وڻ(lineno, entp->m_rightp.get(), resultsr, ايندڙ_Changer);
        }
    }

عام:
    // CONSTRUCTORS
    VInterval_وڻ() = افتراضي;
    ~VInterval_وڻ() = افتراضي;
    // METHODS
    فراغ clear() { m_rootp = خالي_اشارو; }
    فراغ build(ثابت IgnIndices& points) {
        clear();
        m_rootp = build_وڻ(points);
    }
    فراغ ڳوليو(صحيح lineno, IgnIndices& resultsr, صحيح& ايندڙ_Changer) ثابت {
        resultsr = m_every_سٽون;
        ايندڙ_Changer = std::numeric_limits<صحيح>::max();

        إذا (m_rootp) ڳوليو_وڻ(lineno, m_rootp.get(), resultsr, ايندڙ_Changer);
        // Sort indices, so can process in parse order
        std::ترتيب_ڏيو(resultsr.begin(), resultsr.پڇاڙي());
    }

    ساكن فراغ selfTest() {
        //            0     10     20     30     40     50
        // i0:        .     10-10  .      .      .      .
        // i1:        .     .      20-20  .      .      .
        // i2:        .     .      .      .      40-40  .
        // i3:        .     10------------30     .      .
        // i4:        .     .      20------------40     .
        // i5:(below) 0---------------------------------------0
        IgnIndices معلومات;
        std::ويڪٽر<std::pair<صحيح, صحيح>> points
            = {{10, 10}, {20, 20}, {40, 40}, {10, 30}, {20, 40}};
        لكل (ثابت خودڪار& it : points) {
            control_Ign_سٽون.emplace_پٺ(
                V3_Control_Ignores_سٽ{V3_Error_ڪوڊ::I_LINT, it.پهريون, it.ٻيو, سچ});
            معلومات.emplace_پٺ(static_cast<uint32_t>(control_Ign_سٽون.سائيز() - 1));
        }
        VInterval_وڻ وڻ;
        وڻ.build(معلومات);

        IgnIndices results;
        صحيح ايندڙ_Change = 0;
        وڻ.ڳوليو(0, results, ايندڙ_Change);
        UASSERT_SELFTEST(results.سائيز(), 0);
        UASSERT_SELFTEST(ايندڙ_Change, 10);
        وڻ.ڳوليو(10, results, ايندڙ_Change);
        UASSERT_SELFTEST(results.سائيز(), 2);
        UASSERT_SELFTEST(results[0], 0);
        UASSERT_SELFTEST(results[1], 3);
        UASSERT_SELFTEST(ايندڙ_Change, 11);
        وڻ.ڳوليو(11, results, ايندڙ_Change);
        UASSERT_SELFTEST(results.سائيز(), 1);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(ايندڙ_Change, 15);  // Center, or would be 20
        وڻ.ڳوليو(20, results, ايندڙ_Change);
        UASSERT_SELFTEST(results.سائيز(), 3);
        UASSERT_SELFTEST(results[0], 1);
        UASSERT_SELFTEST(results[1], 3);
        UASSERT_SELFTEST(results[2], 4);
        UASSERT_SELFTEST(ايندڙ_Change, 21);
        وڻ.ڳوليو(21, results, ايندڙ_Change);
        UASSERT_SELFTEST(results.سائيز(), 2);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(ايندڙ_Change, 25);  // Center, or would be 30
        وڻ.ڳوليو(30, results, ايندڙ_Change);
        UASSERT_SELFTEST(results.سائيز(), 2);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(ايندڙ_Change, 31);
        وڻ.ڳوليو(40, results, ايندڙ_Change);
        UASSERT_SELFTEST(results.سائيز(), 2);
        UASSERT_SELFTEST(results[0], 2);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(ايندڙ_Change, 41);
        وڻ.ڳوليو(41, results, ايندڙ_Change);
        UASSERT_SELFTEST(results.سائيز(), 0);
        UASSERT_SELFTEST(ايندڙ_Change, std::numeric_limits<صحيح>::max());
        //
        points = {{0, 0}};
        لكل (ثابت خودڪار& it : points) {
            control_Ign_سٽون.emplace_پٺ(
                V3_Control_Ignores_سٽ{V3_Error_ڪوڊ::I_LINT, it.پهريون, it.ٻيو, سچ});
            معلومات.emplace_پٺ(static_cast<uint32_t>(control_Ign_سٽون.سائيز() - 1));
        }
        وڻ.build(معلومات);
        //
        وڻ.ڳوليو(50, results, ايندڙ_Change);
        UASSERT_SELFTEST(results.سائيز(), 1);
        UASSERT_SELFTEST(results[0], 5);
    }
};

// Some attributes are attached to entities of the occur on a fileline
// and multiple attributes can be attached to a line
استعمال V3_Control_سٽ_Attribute = std::bitset<VPragmaType::ENUM_سائيز>;

صنف WaiverSetting final {
عام:
    V3_Error_ڪوڊ m_ڪوڊ;  // Error code
    string m_contents;  // --contents regexp
    string m_ميلاپ;  // --match regexp
    WaiverSetting(V3_Error_ڪوڊ ڪوڊ, ثابت string& contents, ثابت string& ميلاپ)
        : m_ڪوڊ{ڪوڊ}
        , m_contents{contents}
        , m_ميلاپ{ميلاپ} {}
    ~WaiverSetting() = افتراضي;
    WaiverSetting& عامل=(ثابت WaiverSetting& rhs) = افتراضي;
};

// File entity
صنف V3ControlFile final {
    استعمال سٽ_Attr_Map = std::map<صحيح, V3_Control_سٽ_Attribute>;  // Map line->bitset of attributes
    استعمال Waivers = std::ويڪٽر<WaiverSetting>;  // List of {code,wildcard string}

    سٽ_Attr_Map m_سٽ_Attrs;  // Attributes to line mapping
    IgnIndices m_ignIndices;  // s_ignLines that apply to this specific file
    Waivers m_waivers;  // Waive messages
    VInterval_وڻ m_interval_وڻ;  // Tree of indices to IgnLines

    بنية {
        صحيح filenameno = -1;  // Last filename
        صحيح lineno = -1;  // Last linenumber
        صحيح ايندڙ_Change = -1;  // Line number of next change
    } m_آخري_Ignore;  // Last ignore line run

    // Match a given line and attribute to the map, line 0 is any
    منطقي سٽ_ميلاپ(صحيح lineno, VPragmaType type) {
        إذا (m_سٽ_Attrs.ڳوليو(0) != m_سٽ_Attrs.پڇاڙي() && m_سٽ_Attrs[0][type]) أرجع سچ;
        إذا (m_سٽ_Attrs.ڳوليو(lineno) == m_سٽ_Attrs.پڇاڙي()) أرجع خطأ;
        أرجع m_سٽ_Attrs[lineno][type];
    }

عام:
    V3ControlFile() = افتراضي;
    فراغ update(ثابت V3ControlFile& file) {
        // Copy in all attributes and waivers
        لكل (ثابت خودڪار& itr : file.m_سٽ_Attrs) m_سٽ_Attrs[itr.پهريون] |= itr.ٻيو;
        m_waivers.reserve(m_waivers.سائيز() + file.m_waivers.سائيز());
        m_waivers.وجهو(m_waivers.پڇاڙي(), file.m_waivers.begin(), file.m_waivers.پڇاڙي());
        // Copy in all ignore references
        m_ignIndices.reserve(m_ignIndices.سائيز() + file.m_ignIndices.سائيز());
        m_ignIndices.وجهو(m_ignIndices.پڇاڙي(), file.m_ignIndices.begin(),
                            file.m_ignIndices.پڇاڙي());
        // updateFinalize() will soon build tree
    }
    فراغ updateFinalize() { m_interval_وڻ.build(m_ignIndices); }
    فراغ جوڙيو_سٽ_Attribute(صحيح lineno, VPragmaType attr) { m_سٽ_Attrs[lineno].سيٽ(attr); }
    فراغ جوڙيو_Ignore(V3_Error_ڪوڊ ڪوڊ, صحيح min, صحيح max, منطقي on) {
        control_Ign_سٽون.emplace_پٺ(V3_Control_Ignores_سٽ{ڪوڊ, min, max, on});
        m_ignIndices.emplace_پٺ(static_cast<uint32_t>(control_Ign_سٽون.سائيز() - 1));
        m_آخري_Ignore.ايندڙ_Change = -1;
    }
    فراغ جوڙيو_Ignore_ميلاپ(V3_Error_ڪوڊ ڪوڊ, ثابت string& contents, ثابت string& ميلاپ) {
        // Since Verilator 5.031 the error message compared has context, so
        // allow old rules to still match using a final '*'
        string new_ميلاپ = ميلاپ;
        إذا (new_ميلاپ.خالي_جايون() || new_ميلاپ.پٺ() != '*') new_ميلاپ += '*';
        m_waivers.emplace_پٺ(ڪوڊ, contents, new_ميلاپ);
    }

    فراغ apply_بلاڪ(Ast_Gen_بلاڪ* nodep) {
        // Apply to block at this line
        ثابت VPragmaType pragma = VPragmaType::COVERAGE_بلاڪ_OFF;
        إذا (سٽ_ميلاپ(nodep->fileline()->lineno(), pragma)) {
            nodep->جوڙيو_Itemsp(جديد AstPragma{nodep->fileline(), pragma});
        }
    }
    فراغ apply_بلاڪ(Ast_ڳنڍ_بلاڪ* nodep) {
        // Apply to block at this line
        ثابت VPragmaType pragma = VPragmaType::COVERAGE_بلاڪ_OFF;
        File_سٽ* ثابت flp = nodep->fileline();
        إذا (سٽ_ميلاپ(flp->lineno(), pragma)) {
            nodep->جوڙيو_Stmtsp(جديد AstStmtPragma{flp, جديد AstPragma{flp, pragma}});
        }
    }
    فراغ applyCase(AstCase* nodep) {
        // Apply to this case at this line
        ثابت صحيح lineno = nodep->fileline()->lineno();
        إذا (سٽ_ميلاپ(lineno, VPragmaType::ڀريل_CASE)) nodep->ڀريل_Pragma(سچ);
        إذا (سٽ_ميلاپ(lineno, VPragmaType::PARALLEL_CASE)) nodep->parallelPragma(سچ);
    }
    فراغ applyIgnores(File_سٽ* filelinep) {
        // HOT routine, called each parsed token line of this filename
        إذا (filelinep->filenameno() == m_آخري_Ignore.filenameno
            && filelinep->lineno() == m_آخري_Ignore.lineno)
            أرجع;  // Short circuit, no change, no debug
        // For speed, compute line number of next potential change, and skip if before then
        إذا (filelinep->filenameno() == m_آخري_Ignore.filenameno
            && filelinep->lineno() < m_آخري_Ignore.ايندڙ_Change) {
            m_آخري_Ignore.lineno = filelinep->lineno();
            UINFO(9, "   ApplyIgnores for " << filelinep->ascii() << " (no change predicted)");
            أرجع;
        }
        UINFO(9, "   ApplyIgnores for " << filelinep->ascii());
        m_آخري_Ignore.filenameno = filelinep->filenameno();
        m_آخري_Ignore.lineno = filelinep->lineno();
        m_آخري_Ignore.ايندڙ_Change = std::numeric_limits<صحيح>::max();

        // Find all on/offs.  Unlike lint pragmas we calculate the entire
        // bitset of errors to enable, instead of individual enable/disable changes.
        IgnIndices results;
        m_interval_وڻ.ڳوليو(filelinep->lineno(), results /*ref*/, m_آخري_Ignore.ايندڙ_Change /*ref*/);
        // Process all on/offs for lines up to and including the current line
        VError_ٻٽو_انگ_سيٽ bitset{VError_ٻٽو_انگ_سيٽ::AllOnes{}};
        لكل (ثابت خودڪار& it : results) {
            ثابت V3_Control_Ignores_سٽ& cign = control_Ign_سٽون[it];
            UASSERT(cign.اندريون_Range(filelinep->lineno()),
                    "Interval tree returned lines outside range");
            UINFO(9, "     Hit " << cign);
            cign.m_ڪوڊ.forDelegateCodes(
                [&](V3_Error_ڪوڊ subcode) { bitset.سيٽ(subcode, cign.m_on); });
        }

        filelinep->warn_سيٽ_Ctrl_ٻٽو_انگ_سيٽ(bitset);
        m_آخري_Ignore.ايندڙ_Change = std::max(m_آخري_Ignore.ايندڙ_Change, filelinep->lineno() + 1);
        UINFO(9, "    AppliedIgnores " << filelinep << " next@ln " << std::dec
                                       << m_آخري_Ignore.ايندڙ_Change
                                       << " Messages: " << bitset.ascii());
    }
    منطقي waive(V3_Error_ڪوڊ ڪوڊ, ثابت string& پيغام) {
        إذا (ڪوڊ.hardError()) أرجع خطأ;
        لكل (ثابت خودڪار& itr : m_waivers) {
            إذا ((ڪوڊ.isUnder(itr.m_ڪوڊ) || (itr.m_ڪوڊ == V3_Error_ڪوڊ::I_LINT))
                && VString::wildmatch(پيغام, itr.m_ميلاپ)
                && WildcardContents::resolve(itr.m_contents)) {
                أرجع سچ;
            }
        }
        أرجع خطأ;
    }
};

استعمال V3ControlFileResolver = V3ControlWildcardResolver<V3ControlFile>;

//######################################################################
// ScopeTrace tracking

صنف V3_Control_Scope_Trace_داخلا final {
عام:
    ثابت string m_scope;  // Scope or regexp to match
    ثابت منطقي m_on;  // True to enable message
    صحيح m_سطحون = 0;  // # levels, 0 = all, 1 = only this, ...
    // CONSTRUCTORS
    V3_Control_Scope_Trace_داخلا(ثابت string& scope, منطقي on, صحيح سطحون)
        : m_scope{scope}
        , m_on{on}
        , m_سطحون{سطحون} {}
    ~V3_Control_Scope_Trace_داخلا() = افتراضي;
    منطقي عامل<(ثابت V3_Control_Scope_Trace_داخلا& other) ثابت {
        إذا (m_on < other.m_on) أرجع سچ;
        إذا (m_on > other.m_on) أرجع خطأ;
        إذا (m_سطحون < other.m_سطحون) أرجع سچ;
        إذا (m_سطحون > other.m_سطحون) أرجع خطأ;
        أرجع m_scope < other.m_scope;
    }
};

// Tracks what matches are known to hit against V3ControlScopeTraceEntries
صنف V3_Control_Scope_Trace_داخلا_ميلاپ final {
عام:
    ثابت V3_Control_Scope_Trace_داخلا* m_entryp;  // Entry the match refers to
    ثابت string m_scopepart;  // Scope prefix being matched against the entry
    V3_Control_Scope_Trace_داخلا_ميلاپ(ثابت V3_Control_Scope_Trace_داخلا* entryp, ثابت string& scopepart)
        : m_entryp{entryp}
        , m_scopepart{scopepart} {}
    منطقي عامل<(ثابت V3_Control_Scope_Trace_داخلا_ميلاپ& other) ثابت {
        إذا (m_entryp < other.m_entryp) أرجع سچ;
        إذا (m_entryp > other.m_entryp) أرجع خطأ;
        أرجع m_scopepart < other.m_scopepart;
    }
};

صنف V3ControlScopeTraceResolver final {
    std::ويڪٽر<V3_Control_Scope_Trace_داخلا> m_entries;  // User specified on/offs and levels
    std::map<V3_Control_Scope_Trace_داخلا_ميلاپ, منطقي> m_ميلاپ_Cache;  // Matching entries for speed

عام:
    فراغ جوڙيو_Scope_Trace_On(منطقي on, ثابت string& scope, صحيح سطحون) {
        UINFO(9, "addScopeTraceOn " << on << " '" << scope << "' " << " levels=" << سطحون);
        m_entries.emplace_پٺ(V3_Control_Scope_Trace_داخلا{scope, on, سطحون});
        m_ميلاپ_Cache.clear();
    }

    منطقي get_داخلا_ميلاپ(ثابت V3_Control_Scope_Trace_داخلا* entp, ثابت string& scopepart) {
        // Return if a entry matches the scopepart, with memoization
        ثابت V3_Control_Scope_Trace_داخلا_ميلاپ ڪنجي{entp, scopepart};
        ثابت خودڪار pair = m_ميلاپ_Cache.emplace(ڪنجي, خطأ);
        إذا (pair.ٻيو) pair.پهريون->ٻيو = VString::wildmatch(scopepart, entp->m_scope);
        أرجع pair.پهريون->ٻيو;
    }

    منطقي getScopeTraceOn(ثابت string& scope) {
        // Apply in the order the user provided them, so they can choose on/off preferences
        صحيح maxLevel = 1;
        لكل (ثابت خودڪار& ch : scope) {
            إذا (ch == '.') ++maxLevel;
        }
        UINFO(9, "getScopeTraceOn " << scope << " maxLevel=" << maxLevel);

        منطقي enabled = سچ;
        لكل (ثابت خودڪار& ent : m_entries) {
            // We apply shortest match first for each rule component
            // (Otherwise the levels would be useless as "--scope top* --levels 1" would
            // always match at every scopepart, and we wouldn't know how to count levels)
            صحيح partLevel = 1;
            لكل (string::سائيز_type part_پڇاڙي = 0; سچ;) {
                part_پڇاڙي = scope.ڳوليو('.', part_پڇاڙي + 1);
                إذا (part_پڇاڙي == string::npos) part_پڇاڙي = scope.ڊيگهائي();
                ثابت std::string scopepart = scope.substr(0, part_پڇاڙي);
                إذا (get_داخلا_ميلاپ(&ent, scopepart)) {
                    ثابت منطقي level_ميلاپ
                        = !ent.m_سطحون || (ent.m_سطحون >= maxLevel - partLevel);
                    إذا (level_ميلاپ) enabled = ent.m_on;
                    UINFO(9, "getScopeTraceOn-part " << scope << " enabled=" << enabled
                                                     << " @ lev=" << partLevel
                                                     << (level_ميلاپ ? "[match]" : "[miss]")
                                                     << " from scopepart=" << scopepart);
                    اكسر;
                }
                إذا (part_پڇاڙي == scope.ڊيگهائي()) اكسر;
                ++partLevel;
            }
        }
        أرجع enabled;
    }
};

//######################################################################
// Resolve modules and files in the design

صنف V3_Control_Resolver_Hier_ڪم_ڪندڙ_داخلا final {
    ثابت صحيح m_workers;  // Number of workers requested for the hierarchical block
    File_سٽ* ثابت m_flp;  // Location the worker count was specified at

عام:
    واضح V3_Control_Resolver_Hier_ڪم_ڪندڙ_داخلا(صحيح workers, File_سٽ* flp)
        : m_workers{workers}
        , m_flp{flp} {}
    صحيح workers() ثابت { أرجع m_workers; }
    File_سٽ* flp() ثابت { أرجع m_flp; }
};

صنف V3ControlResolver final {
    تعداد Profile_معلومات_گهڻائي : uint8_t { NONE = 0, MTASK = 1, HIER_DPI = 2 };
    V3ControlModuleResolver m_modules;  // Access to module names (with wildcards)
    V3ControlFileResolver m_files;  // Access to file names (with wildcards)
    V3ControlScopeTraceResolver m_scopeTraces;  // Regexp to trace enables
    std::unordered_map<string, std::unordered_map<string, uint64_t>>
        m_profile_معلومات;  // Access to profile_data records
    uint8_t m_گهڻائي = NONE;  // Kind of profile_data records is currently active
    std::unordered_map<string, V3_Control_Resolver_Hier_ڪم_ڪندڙ_داخلا> m_hierWorkers;
    File_سٽ* m_profile_File_سٽ = خالي_اشارو;  // Location profile_data was read from

    V3ControlResolver() = افتراضي;
    ~V3ControlResolver() = افتراضي;

عام:
    ساكن V3ControlResolver& s() {
        ساكن V3ControlResolver s_singleton;
        أرجع s_singleton;
    }
    V3ControlModuleResolver& modules() { أرجع m_modules; }
    V3ControlFileResolver& files() { أرجع m_files; }
    V3ControlScopeTraceResolver& scopeTraces() { أرجع m_scopeTraces; }

    فراغ جوڙيو_Profile_معلومات(File_سٽ* fl, ثابت string& hierDpi, uint64_t لاڳت) {
        // Empty key for hierarchical DPI wrapper costs.
        جوڙيو_Profile_معلومات(fl, hierDpi, "", لاڳت, HIER_DPI);
    }
    فراغ جوڙيو_Profile_معلومات(File_سٽ* fl, ثابت string& model, ثابت string& ڪنجي, uint64_t لاڳت,
                        Profile_معلومات_گهڻائي گهڻائي = MTASK) {
        إذا (!m_profile_File_سٽ) m_profile_File_سٽ = fl;
        إذا (لاڳت == 0) لاڳت = 1;  // Cost 0 means delete (or no data)
        إذا (گهڻائي == MTASK) {
            m_profile_معلومات[model][ڪنجي] += لاڳت;
        } وإلا إذا (گهڻائي == HIER_DPI) {
            m_profile_معلومات[model][ڪنجي] = std::max(m_profile_معلومات[model][ڪنجي], لاڳت);
        }
        m_گهڻائي |= گهڻائي;
    }
    منطقي شامل_آهي_MTask_Profile_معلومات() ثابت { أرجع m_گهڻائي & MTASK; }
    uint64_t get_Profile_معلومات(ثابت string& hierDpi) ثابت {
        // Empty key for hierarchical DPI wrapper costs.
        أرجع get_Profile_معلومات(hierDpi, "");
    }
    فراغ جوڙيو_Hier_Workers(File_سٽ* flp, ثابت string& model, صحيح workers) {
        m_hierWorkers.emplace(std::piecewise_construct, std::forward_as_tuple(model),
                              std::forward_as_tuple(workers, flp));
    }
    صحيح getHierWorkers(ثابت string& model) ثابت {
        ثابت خودڪار mit = m_hierWorkers.ڳوليو(model);
        // Assign a single worker if no specified.
        أرجع mit != m_hierWorkers.cend() ? mit->ٻيو.workers() : 0;
    }
    File_سٽ* get_Hier_Workers_File_سٽ(ثابت string& model) ثابت {
        ثابت خودڪار mit = m_hierWorkers.ڳوليو(model);
        أرجع mit != m_hierWorkers.cend() ? mit->ٻيو.flp() : v3Global.rootp()->fileline();
    }
    uint64_t get_Profile_معلومات(ثابت string& model, ثابت string& ڪنجي) ثابت {
        ثابت خودڪار mit = m_profile_معلومات.ڳوليو(model);
        إذا (mit == m_profile_معلومات.cend()) أرجع 0;
        ثابت خودڪار it = mit->ٻيو.ڳوليو(ڪنجي);
        إذا (it == mit->ٻيو.cend()) أرجع 0;
        أرجع it->ٻيو;
    }
    File_سٽ* get_Profile_معلومات_File_سٽ() ثابت { أرجع m_profile_File_سٽ; }  // Maybe null
    ساكن uint64_t get_Current_Hier_بلاڪ_لاڳت() {
        إذا (uint64_t لاڳت = V3Control::get_Profile_معلومات(v3Global.opt.اڳياڙي())) {
            UINFO(9, "Fetching cost from profile info: " << لاڳت);
            أرجع لاڳت;
        }

        // Without profiling data, sum the regions evaluated on each time step
        AstNetlist* ثابت netlistp = v3Global.rootp();
        uint64_t لاڳت = 0;
        لكل (صحيح i = 0; i < VEval::ENUM_پڇاڙي; ++i) {
            ثابت VEval eval{i};
            إذا (eval.slow()) استمر;
            لاڳت += V3_Instr_ڳڻپ::ڳڻپ(netlistp->evalFuncp(eval), خطأ);
        }
        UINFO(9, "Evaluating cost: " << لاڳت);
        أرجع لاڳت;
    }
};

//######################################################################
// V3Control

فراغ V3Control::جوڙيو_Case_ڀريل(ثابت string& filename, صحيح lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.جوڙيو_سٽ_Attribute(lineno, VPragmaType::ڀريل_CASE);
}
فراغ V3Control::جوڙيو_Case_Parallel(ثابت string& filename, صحيح lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.جوڙيو_سٽ_Attribute(lineno, VPragmaType::PARALLEL_CASE);
}
فراغ V3Control::جوڙيو_Coverage_بلاڪ_Off(ثابت string& filename, صحيح lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.جوڙيو_سٽ_Attribute(lineno, VPragmaType::COVERAGE_بلاڪ_OFF);
}
فراغ V3Control::جوڙيو_Coverage_بلاڪ_Off(ثابت string& module, ثابت string& blockname) {
    V3ControlResolver::s().modules().at(module).جوڙيو_Coverage_بلاڪ_Off(blockname);
}

فراغ V3Control::جوڙيو_Hier_Workers(File_سٽ* fl, ثابت string& model, صحيح workers) {
    V3ControlResolver::s().جوڙيو_Hier_Workers(fl, model, workers);
}

فراغ V3Control::جوڙيو_Fsm_رجسٽر_Wrapper(File_سٽ* fl, ثابت string& module, ثابت string& d,
                                      ثابت string& q, ثابت string& clock, ثابت string& reset,
                                      ثابت string& reset_قدر) {
    string missing;
    إذا (module.خالي_جايون()) missing += "-module";
    إذا (d.خالي_جايون()) missing = VString::dot(missing, ", ", "-d");
    إذا (q.خالي_جايون()) missing = VString::dot(missing, ", ", "-q");
    إذا (clock.خالي_جايون()) missing = VString::dot(missing, ", ", "-clock");
    إذا (!missing.خالي_جايون()) {
        fl->v3error("fsm_register_wrapper missing " << missing);
        أرجع;
    }

    ثابت Fsm_رجسٽر_Wrapper desc{module, d, q, clock, reset, reset_قدر};
    V3ControlResolver::s().modules().at(module).سيٽ_Fsm_رجسٽر_Wrapper(fl, desc);
}

فراغ V3Control::جوڙيو_Ignore(V3_Error_ڪوڊ ڪوڊ, منطقي on, ثابت string& filename, صحيح min, صحيح max) {
    UINFO(9, "addIgnore " << ڪوڊ << " " << min << "-" << max << " fn=" << filename);
    إذا (filename == "*") {  // For "lint_off/lint_on [--rule x]"
        File_سٽ::globalWarnOff(ڪوڊ, !on);
    } وإلا {  // For "lint_off/lint_on [--rule x] --file y [-lines min[-max]]"
        V3ControlResolver::s().files().at(filename).جوڙيو_Ignore(ڪوڊ, min, max, on);
    }
}
فراغ V3Control::جوڙيو_Ignore_ميلاپ(V3_Error_ڪوڊ ڪوڊ, ثابت string& filename, ثابت string& contents,
                               ثابت string& ميلاپ) {
    // For "lint_off --rule x --file y --match z", no support for lint_on
    V3ControlResolver::s().files().at(filename).جوڙيو_Ignore_ميلاپ(ڪوڊ, contents, ميلاپ);
}

فراغ V3Control::جوڙيو_Inline(File_سٽ* fl, ثابت string& module, ثابت string& ftask, منطقي on) {
    إذا (ftask.خالي_جايون()) {
        V3ControlResolver::s().modules().at(module).سيٽ_Inline(on);
    } وإلا {
        إذا (!on) {
            fl->v3warn(E_UNSUPPORTED, "Unsupported: no_inline for tasks");
        } وإلا {
            V3ControlResolver::s().modules().at(module).ftasks().at(ftask).سيٽ_No_Inline(on);
        }
    }
}

فراغ V3Control::جوڙيو_Module_Pragma(ثابت string& module, VPragmaType pragma) {
    V3ControlResolver::s().modules().at(module).جوڙيو_Module_Pragma(pragma);
}

فراغ V3Control::جوڙيو_Profile_معلومات(File_سٽ* fl, ثابت string& hierDpi, uint64_t لاڳت) {
    V3ControlResolver::s().جوڙيو_Profile_معلومات(fl, hierDpi, لاڳت);
}
فراغ V3Control::جوڙيو_Profile_معلومات(File_سٽ* fl, ثابت string& model, ثابت string& ڪنجي,
                               uint64_t لاڳت) {
    V3ControlResolver::s().جوڙيو_Profile_معلومات(fl, model, ڪنجي, لاڳت);
}

فراغ V3Control::جوڙيو_Scope_Trace_On(منطقي on, ثابت string& scope, صحيح سطحون) {
    V3ControlResolver::s().scopeTraces().جوڙيو_Scope_Trace_On(on, scope, سطحون);
}

فراغ V3Control::جوڙيو_Var_Attr(File_سٽ* fl, ثابت string& module, ثابت string& ftask,
                           Var_Spec_قسم قسم, ثابت string& نمونو_2, VAttrType attr,
                           Ast_Sen_وڻ* sensep) {
    إذا (sensep) {
        File_سٽ* ثابت flp = sensep->fileline();
        // Historical, not actually needed, only parsed for compatibility, delete it
        VL_DO_DANGLING(sensep->delete_وڻ(), sensep);
        // Used to be only accepted on public_flat_rw
        إذا (attr != VAttrType::VAR_عوامي_FLAT_RW) {
            flp->v3error("sensitivity not expected for attribute");
            أرجع;
        }
    }

    إذا (قسم != Var_Spec_قسم::VAR) {
        بدل (attr) {
        حالة VAttrType::VAR_عوامي_FLAT:
        حالة VAttrType::VAR_عوامي_FLAT_RD:
        حالة VAttrType::VAR_عوامي_FLAT_RW: اكسر;
        افتراضي:
            fl->v3error("'"s + attr.ascii() + "' attribute does not accept -param/-port");
            أرجع;
        }
    }

    // Semantics: Most of the attributes operate on signals
    إذا (نمونو_2.خالي_جايون()) {
        إذا (attr == VAttrType::VAR_عوامي) {
            إذا (ftask.خالي_جايون()) {
                // public module, this is the only exception from var here
                V3ControlResolver::s().modules().at(module).جوڙيو_Module_Pragma(
                    VPragmaType::عوامي_MODULE);
            } وإلا {
                V3ControlResolver::s().modules().at(module).ftasks().at(ftask).سيٽ_عوامي(سچ);
            }
        } وإلا {
            fl->v3error("missing -var");
        }
    } وإلا {
        إذا (attr == VAttrType::VAR_FORCEABLE) {
            إذا (module.خالي_جايون()) {
                fl->v3error("forceable missing -module");
            } وإلا إذا (!ftask.خالي_جايون()) {
                fl->v3error("Signals inside functions/tasks cannot be marked forceable");
            } وإلا {
                V3ControlResolver::s().modules().at(module).vars().at(نمونو_2).جوڙيو(attr);
            }
        } وإلا {
            V3ControlModule& mod = V3ControlResolver::s().modules().at(module);
            V3ControlVar& controlVar = [&]() -> V3ControlVar& {
                إذا (ftask.خالي_جايون()) {
                    إذا (قسم == Var_Spec_قسم::PARAM) أرجع mod.params().at(نمونو_2);
                    إذا (قسم == Var_Spec_قسم::پورٽ) أرجع mod.ports().at(نمونو_2);
                    UASSERT_OBJ(قسم == Var_Spec_قسم::VAR, fl, "Unexpected VarSpecKind");
                    أرجع mod.vars().at(نمونو_2);
                }
                إذا (قسم == Var_Spec_قسم::PARAM) أرجع mod.ftasks().at(ftask).params().at(نمونو_2);
                إذا (قسم == Var_Spec_قسم::پورٽ) أرجع mod.ftasks().at(ftask).ports().at(نمونو_2);
                UASSERT_OBJ(قسم == Var_Spec_قسم::VAR, fl, "Unexpected VarSpecKind");
                أرجع mod.ftasks().at(ftask).vars().at(نمونو_2);
            }();
            controlVar.جوڙيو(attr);
        }
    }
}

فراغ V3Control::applyCase(AstCase* nodep) {
    ثابت string& filename = nodep->fileline()->filename();
    V3ControlFile* ثابت filep = V3ControlResolver::s().files().resolve(filename);
    إذا (filep) filep->applyCase(nodep);
}

فراغ V3Control::apply_Coverage_بلاڪ(Ast_ڳنڍ_Module* modulep, AstBegin* nodep) {
    ثابت string& filename = nodep->fileline()->filename();
    V3ControlFile* ثابت filep = V3ControlResolver::s().files().resolve(filename);
    إذا (filep) filep->apply_بلاڪ(nodep);
    ثابت string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ثابت modp = V3ControlResolver::s().modules().resolve(modname);
    إذا (modp) modp->apply_بلاڪ(nodep);
}
فراغ V3Control::apply_Coverage_بلاڪ(Ast_ڳنڍ_Module* modulep, Ast_Gen_بلاڪ* nodep) {
    ثابت string& filename = nodep->fileline()->filename();
    V3ControlFile* ثابت filep = V3ControlResolver::s().files().resolve(filename);
    إذا (filep) filep->apply_بلاڪ(nodep);
    ثابت string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ثابت modp = V3ControlResolver::s().modules().resolve(modname);
    إذا (modp) modp->apply_بلاڪ(nodep);
}

فراغ V3Control::applyIgnores(File_سٽ* filelinep) {
    ثابت string& filename = filelinep->filename();
    V3ControlFile* ثابت filep = V3ControlResolver::s().files().resolve(filename);
    إذا (filep) filep->applyIgnores(filelinep);
}

فراغ V3Control::applyModule(Ast_ڳنڍ_Module* modulep) {
    ثابت string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ثابت modp = V3ControlResolver::s().modules().resolve(modname);
    إذا (modp) modp->apply(modulep);
}

فراغ V3Control::applyFTask(Ast_ڳنڍ_Module* modulep, Ast_ڳنڍ_FTask* ftaskp) {
    ثابت string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ثابت modp = V3ControlResolver::s().modules().resolve(modname);
    إذا (!modp) أرجع;
    ثابت V3ControlFTask* ثابت ftp = modp->ftasks().resolve(ftaskp->prettyDehashOrigOrName());
    إذا (ftp) ftp->apply(ftaskp);
}

سانچو <قسم_نالو T_Resolver>
ساكن فراغ resolveThenApply(T_Resolver& resolver, AstVar* varp) {
    ثابت std::string name = varp->prettyDehashOrigOrName();
    إذا (ثابت V3ControlVar* ثابت vp = resolver.vars().resolve(name)) vp->apply(varp);
    إذا (varp->isParam()) {
        إذا (ثابت V3ControlVar* ثابت vp = resolver.params().resolve(name)) vp->apply(varp);
    }
    إذا (varp->isIO()) {
        إذا (ثابت V3ControlVar* ثابت vp = resolver.ports().resolve(name)) vp->apply(varp);
    }
}

فراغ V3Control::applyVarAttr(ثابت Ast_ڳنڍ_Module* modulep, ثابت Ast_ڳنڍ_FTask* ftaskp,
                             AstVar* varp) {
    V3ControlModule* ثابت modp
        = V3ControlResolver::s().modules().resolve(modulep->prettyDehashOrigOrName());
    إذا (!modp) أرجع;
    إذا (ftaskp) {
        V3ControlFTask* ثابت ftp = modp->ftasks().resolve(ftaskp->prettyDehashOrigOrName());
        إذا (!ftp) أرجع;
        resolveThenApply(*ftp, varp);
    } وإلا {
        resolveThenApply(*modp, varp);
    }
}

صحيح V3Control::getHierWorkers(ثابت string& model) {
    أرجع V3ControlResolver::s().getHierWorkers(model);
}
File_سٽ* V3Control::get_Hier_Workers_File_سٽ(ثابت string& model) {
    أرجع V3ControlResolver::s().get_Hier_Workers_File_سٽ(model);
}
ثابت V3Control::Fsm_رجسٽر_Wrapper* V3Control::get_Fsm_رجسٽر_Wrapper(ثابت string& module) {
    V3ControlModule* ثابت modp = V3ControlResolver::s().modules().resolve(module);
    أرجع modp ? modp->fsm_رجسٽر_Wrapperp() : خالي_اشارو;
}
uint64_t V3Control::get_Profile_معلومات(ثابت string& hierDpi) {
    أرجع V3ControlResolver::s().get_Profile_معلومات(hierDpi);
}
uint64_t V3Control::get_Profile_معلومات(ثابت string& model, ثابت string& ڪنجي) {
    أرجع V3ControlResolver::s().get_Profile_معلومات(model, ڪنجي);
}
File_سٽ* V3Control::get_Profile_معلومات_File_سٽ() {
    أرجع V3ControlResolver::s().get_Profile_معلومات_File_سٽ();
}
منطقي V3Control::getScopeTraceOn(ثابت string& scope) {
    أرجع V3ControlResolver::s().scopeTraces().getScopeTraceOn(scope);
}

فراغ V3Control::contents_ڌڪو_متن(ثابت string& متن) { WildcardContents::ڌڪو_متن(متن); }

منطقي V3Control::شامل_آهي_MTask_Profile_معلومات() {
    أرجع V3ControlResolver::s().شامل_آهي_MTask_Profile_معلومات();
}
uint64_t V3Control::get_Current_Hier_بلاڪ_لاڳت() {
    أرجع V3ControlResolver::s().get_Current_Hier_بلاڪ_لاڳت();
}

منطقي V3Control::waive(ثابت File_سٽ* filelinep, V3_Error_ڪوڊ ڪوڊ, ثابت string& پيغام) {
    V3ControlFile* ثابت filep = V3ControlResolver::s().files().resolve(filelinep->filename());
    إذا (!filep) أرجع خطأ;
    أرجع filep->waive(ڪوڊ, پيغام);
}

فراغ V3Control::selfTest() { VInterval_وڻ::selfTest(); }
