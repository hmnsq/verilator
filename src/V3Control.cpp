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
سانچا <قسم_ناں T>
صنف V3ControlWildcardResolver final {
    بدلنجوگ V3Mutex m_mutex;  // protects members
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
        ثابت V3_جندرا_Guard جندرا_2{m_mutex};
        ثابت V3_جندرا_Guard other_جندرا{other.m_mutex};
        // Clear the resolved cache, as 'other' might add new patterns that need to be applied as
        // well.
        m_mapResolved.clear();
        لكل (ثابت خودکار& itr : other.m_mapPatterns) m_mapPatterns[itr.پہلا].update(itr.دوجا);
    }

    // Access and create a pattern entry
    T& at(ثابت string& name) VL_MT_محفوظ_EXCLUDES(m_mutex) {
        ثابت V3_جندرا_Guard جندرا_2{m_mutex};
        // We might be adding a new entry under this, so clear the cache.
        m_mapResolved.clear();
        // Patterns use '/' as separator; normalize Windows backslashes
        أرجع m_mapPatterns[V3Os::filename_Slash_رستہ(name)];
    }
    // Access an entity and resolve patterns that match it
    T* resolve(ثابت string& name) VL_MT_محفوظ_EXCLUDES(m_mutex) {
        ثابت V3_جندرا_Guard جندرا_2{m_mutex};
        // Lookup if it was resolved before, typically not
        ثابت خودکار pair = m_mapResolved.emplace(name, خالی_سوچک);
        std::unique_ptr<T>& entryr = pair.پہلا->دوجا;
        // Resolve entry when first requested, cache the result
        إذا (pair.دوجا) {
            // Inserted: update the entity with all matches in the patterns.
            // Normalize separators so Windows backslash filenames match the
            // slash-separated patterns (normalized when loaded).
            ثابت string slashName = V3Os::filename_Slash_رستہ(name);
            لكل (ثابت خودکار& patEnt : m_mapPatterns) {
                إذا (VString::wildmatch(slashName, patEnt.پہلا)) {
                    إذا (!entryr) entryr.reset(جديد T{});
                    entryr->update(patEnt.دوجا);
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
    std::سمتیہ<VAttrType> m_attrs;  // The list of attributes
عام:
    // Add new attribugte
    فراغ جوڑو(VAttrType attr) { m_attrs.emplace_پٹھ(attr); }
    // Update from other by copying all attributes
    فراغ update(ثابت V3ControlVar& other) {
        m_attrs.reserve(m_attrs.ناپ() + other.m_attrs.ناپ());
        m_attrs.داخل_کرو(m_attrs.سرا(), other.m_attrs.begin(), other.m_attrs.سرا());
    }
    فراغ updateFinalize() {}
    // Apply all attributes to the variable
    فراغ apply(AstVar* varp) ثابت {
        لكل (ثابت VAttrType attr : m_attrs) {
            varp->جوڑو_Attrsp(جديد Ast_Attr_دا{varp->fileline(), attr});
        }
    }
};

ورتوں V3ControlVarResolver = V3ControlWildcardResolver<V3ControlVar>;

//======================================================================

صنف WildcardContents final {
    // Not mutex protected, current calling from V3Control::waive is protected by error's mutex
    // MEMBERS
    std::map<ثابت std::string, منطقي> m_mapPatterns;  // Pattern match results
    std::deque<string> m_سطریں;  // Source text lines

    // METHODS
    ساكن WildcardContents& s() {  // Singleton
        ساكن WildcardContents s_s;
        أرجع s_s;
    }
    فراغ clearCacheImp() { m_mapPatterns.clear(); }
    فراغ ڈالو_متن_Imp(ثابت string& متن) {
        // Similar code in VFileContent::pushText()
        // Any leftover text is stored on largest line (might be "")
        ثابت string leftover = m_سطریں.پٹھ() + متن;
        m_سطریں.نکالو_پٹھ();

        // Insert line-by-line
        string::ناپ_type سطر_شروع = 0;
        طالما (سچ) {
            ثابت string::ناپ_type سطر_سرا = leftover.لبھو('\n', سطر_شروع);
            إذا (سطر_سرا != string::npos) {
                ثابت string oneline(leftover, سطر_شروع, سطر_سرا - سطر_شروع + 1);
                إذا (oneline.ناپ() > 1) m_سطریں.ڈالو_پٹھ(oneline);  // Keeps newline
                UINFO(9, "Push[+" << (m_سطریں.ناپ() - 1) << "]: " << oneline);
                سطر_شروع = سطر_سرا + 1;
            } وإلا {
                اكسر;
            }
        }
        // Keep leftover for next time
        m_سطریں.emplace_پٹھ(string(leftover, سطر_شروع));  // Might be ""
        clearCacheImp();
    }

    منطقي resolveUncachedImp(ثابت string& contentsRegexp) {
        لكل (ثابت string& i : m_سطریں) {
            إذا (VString::wildmatch(i, contentsRegexp)) أرجع سچ;
        }
        أرجع خطأ;
    }
    منطقي resolveCachedImp(ثابت string& contentsRegexp) {
        // Lookup if it was resolved before, typically is
        إذا (contentsRegexp.خالی_تھاواں()) أرجع سچ;
        ثابت خودکار pair = m_mapPatterns.emplace(contentsRegexp, خطأ);
        منطقي& entryr = pair.پہلا->دوجا;
        // Resolve entry when first requested, cache the result
        إذا (pair.دوجا) entryr = resolveUncachedImp(contentsRegexp);
        أرجع entryr;
    }

عام:
    WildcardContents() {
        m_سطریں.emplace_پٹھ("");  // start with no leftover
    }
    ~WildcardContents() = افتراضي;
    // Return true iff contentsRegexp in parsed contents
    ساكن منطقي resolve(ثابت string& contentsRegexp) {
        أرجع s().resolveCachedImp(contentsRegexp);
    }
    // Add arbitrary text (need not be line-by-line)
    ساكن فراغ ڈالو_متن(ثابت string& متن) { s().ڈالو_متن_Imp(متن); }
};

//######################################################################
// Function or task: Have variables and properties

صنف V3ControlFTask final {
    V3ControlVarResolver m_params;  // Parameters in function/task
    V3ControlVarResolver m_ports;  // Ports in function/task
    V3ControlVarResolver m_vars;  // Variables in function/task
    منطقي m_noinline = خطأ;  // Don't inline function/task
    منطقي m_عوامی = خطأ;  // Public function/task

عام:
    V3ControlFTask() = افتراضي;
    فراغ update(ثابت V3ControlFTask& f) {
        // Don't overwrite true with false
        إذا (f.m_noinline) m_noinline = سچ;
        إذا (f.m_عوامی) m_عوامی = سچ;
        m_params.update(f.m_params);
        m_ports.update(f.m_ports);
        m_vars.update(f.m_vars);
    }
    فراغ updateFinalize() {}

    V3ControlVarResolver& params() { أرجع m_params; }
    V3ControlVarResolver& ports() { أرجع m_ports; }
    V3ControlVarResolver& vars() { أرجع m_vars; }

    فراغ سیٹ_No_Inline(منطقي سیٹ) { m_noinline = سیٹ; }
    فراغ سیٹ_عوامی(منطقي سیٹ) { m_عوامی = سیٹ; }

    فراغ apply(Ast_گرہ_FTask* ftaskp) ثابت {
        إذا (m_noinline)
            ftaskp->جوڑو_Stmtsp(جديد AstPragma{ftaskp->fileline(), VPragmaType::NO_INLINE_TASK});
        إذا (m_عوامی)
            ftaskp->جوڑو_Stmtsp(جديد AstPragma{ftaskp->fileline(), VPragmaType::عوامی_TASK});
    }
};

ورتوں V3ControlFTaskResolver = V3ControlWildcardResolver<V3ControlFTask>;

//######################################################################
// Modules have tasks, variables, named blocks and properties

صنف V3ControlModule final {
    V3ControlFTaskResolver m_tasks;  // Functions/tasks in module
    V3ControlVarResolver m_params;  // Parameters in module
    V3ControlVarResolver m_ports;  // Ports in module
    V3ControlVarResolver m_vars;  // Variables in module
    V3Control::Fsm_رجسٹر_Wrapper m_fsm_رجسٹر_Wrapper;  // FSM wrapper descriptor
    std::unordered_سیٹ<std::string> m_coverage_Off_بلاکاں;  // List of block names for coverage_off
    std::سیٹ<VPragmaType> m_modPragmas;  // List of Pragmas for modules
    منطقي m_has_Fsm_رجسٹر_Wrapper = خطأ;  // Whether descriptor has been set
    منطقي m_inline = خطأ;  // Whether to force the inline
    منطقي m_inline_قدر = خطأ;  // The inline value (on/off)

عام:
    V3ControlModule() = افتراضي;

    فراغ update(ثابت V3ControlModule& m) {
        m_tasks.update(m.m_tasks);
        m_params.update(m.m_params);
        m_ports.update(m.m_ports);
        m_vars.update(m.m_vars);
        إذا (m.m_has_Fsm_رجسٹر_Wrapper) {
            m_fsm_رجسٹر_Wrapper = m.m_fsm_رجسٹر_Wrapper;
            m_has_Fsm_رجسٹر_Wrapper = سچ;
        }
        لكل (ثابت string& i : m.m_coverage_Off_بلاکاں) m_coverage_Off_بلاکاں.داخل_کرو(i);
        إذا (!m_inline) {
            m_inline = m.m_inline;
            m_inline_قدر = m.m_inline_قدر;
        }
        لكل (خودکار it = m.m_modPragmas.cbegin(); it != m.m_modPragmas.cend(); ++it) {
            m_modPragmas.داخل_کرو(*it);
        }
    }
    فراغ updateFinalize() {}

    V3ControlFTaskResolver& ftasks() { أرجع m_tasks; }
    V3ControlVarResolver& params() { أرجع m_params; }
    V3ControlVarResolver& ports() { أرجع m_ports; }
    V3ControlVarResolver& vars() { أرجع m_vars; }

    فراغ جوڑو_Coverage_بلاک_Off(ثابت string& name) { m_coverage_Off_بلاکاں.داخل_کرو(name); }
    فراغ سیٹ_Inline(منطقي سیٹ) {
        m_inline = سچ;
        m_inline_قدر = سیٹ;
    }
    فراغ سیٹ_Fsm_رجسٹر_Wrapper(File_سطر* fl, ثابت V3Control::Fsm_رجسٹر_Wrapper& desc) {
        إذا (m_has_Fsm_رجسٹر_Wrapper) {
            fl->v3warn(BADVLTPRAGMA, "Duplicate fsm_register_wrapper descriptor for module "
                                         << Ast_گرہ::prettyNameQ(desc.moduleName)
                                         << "; replacing previous descriptor");
        }
        m_fsm_رجسٹر_Wrapper = desc;
        m_has_Fsm_رجسٹر_Wrapper = سچ;
    }
    ثابت V3Control::Fsm_رجسٹر_Wrapper* fsm_رجسٹر_Wrapperp() ثابت {
        أرجع m_has_Fsm_رجسٹر_Wrapper ? &m_fsm_رجسٹر_Wrapper : خالی_سوچک;
    }
    فراغ جوڑو_Module_Pragma(VPragmaType pragma) { m_modPragmas.داخل_کرو(pragma); }

    فراغ apply(Ast_گرہ_Module* modp) {
        إذا (m_inline) {
            ثابت VPragmaType type
                = m_inline_قدر ? VPragmaType::INLINE_MODULE : VPragmaType::NO_INLINE_MODULE;
            Ast_گرہ* ثابت nodep = جديد AstPragma{modp->fileline(), type};
            modp->جوڑو_Stmtsp(nodep);
        }
        لكل (ثابت خودکار& itr : m_modPragmas) {
            // Catch hier param modules to mark their attributes before they are
            // flagged dead in LinkDot.
            إذا (itr == VPragmaType::HIER_PARAMS) modp->hierParams(سچ);
            Ast_گرہ* ثابت nodep = جديد AstPragma{modp->fileline(), itr};
            modp->جوڑو_Stmtsp(nodep);
        }
    }

    فراغ apply_بلاک(Ast_Gen_بلاک* nodep) {
        ثابت VPragmaType pragma = VPragmaType::COVERAGE_بلاک_OFF;
        إذا (!nodep->unnamed()) {
            لكل (ثابت string& i : m_coverage_Off_بلاکاں) {
                إذا (VString::wildmatch(nodep->prettyDehashOrigOrName(), i)) {
                    nodep->جوڑو_Itemsp(جديد AstPragma{nodep->fileline(), pragma});
                }
            }
        }
    }

    فراغ apply_بلاک(Ast_گرہ_بلاک* nodep) {
        ثابت VPragmaType pragma = VPragmaType::COVERAGE_بلاک_OFF;
        File_سطر* ثابت flp = nodep->fileline();
        إذا (!nodep->unnamed()) {
            لكل (ثابت string& i : m_coverage_Off_بلاکاں) {
                إذا (VString::wildmatch(nodep->prettyDehashOrigOrName(), i)) {
                    nodep->جوڑو_Stmtsp(جديد AstStmtPragma{flp, جديد AstPragma{flp, pragma}});
                }
            }
        }
    }
};

ورتوں V3ControlModuleResolver = V3ControlWildcardResolver<V3ControlModule>;

//######################################################################
// Files have:
//  - Line ignores (lint/coverage/tracing on/off)
//  - Line attributes: Attributes attached to lines

// lint/coverage/tracing on/off
صنف V3_Control_Ignores_سطر final {
عام:
    ثابت صحيح m_سطر_Min;  // Minimum line number to make change at
    ثابت صحيح m_سطر_Max;  // Maximum line number to make change at (inclusive)
    ثابت V3_Error_کوڈ m_کوڈ;  // Error code
    ثابت منطقي m_on;  // True to enable message
    V3_Control_Ignores_سطر(V3_Error_کوڈ کوڈ, صحيح linemin, صحيح linemax, منطقي on)
        : m_سطر_Min{linemin}
        , m_سطر_Max{linemax}
        , m_کوڈ{کوڈ}
        , m_on{on} {}
    ~V3_Control_Ignores_سطر() = افتراضي;
    منطقي every_سطر() ثابت { أرجع m_سطر_Min == 0 && m_سطر_Max == 0; }
    منطقي وچ_Range(صحيح lineno) ثابت {
        أرجع (lineno >= m_سطر_Min && lineno <= m_سطر_Max) || every_سطر();
    }
};
std::ostream& عامل<<(std::ostream& os, ثابت V3_Control_Ignores_سطر& rhs) {
    أرجع os << rhs.m_سطر_Min << "-" << rhs.m_سطر_Max << ", " << rhs.m_کوڈ << ", " << rhs.m_on;
}

// Ignore line settings, index is parse order
// Single global deque to avoid copying when update()
ساكن std::سمتیہ<V3_Control_Ignores_سطر> control_Ign_سطریں;

ورتوں IgnIndices = std::سمتیہ<uint32_t>;  // List of {s_ignLines indces}

صنف VInterval_درخت final {
    بنية اندراج_2 final {
        // Classic centered interval tree, referencing an index to controlIgnLines
        صحيح m_center;  // Line number of central point
        std::unique_ptr<اندراج_2> m_leftp;  // Entry with all its rules/entries < this center
        std::unique_ptr<اندراج_2> m_rightp;  // Entry with all its rules/entries > this center
        IgnIndices m_byMin;  // Rules overlapping center sorted by min line number
        IgnIndices m_byMax;  // Rules overlapping center sorted by max line number
        واضح اندراج_2(صحيح center)
            : m_center{center} {}
    };

    std::unique_ptr<اندراج_2> m_rootp;  // Root of interval tree
    IgnIndices m_every_سطریں;  // All-line disables

خاص:
    std::unique_ptr<اندراج_2> build_درخت(ثابت IgnIndices& points) {
        إذا (points.خالی_تھاواں()) أرجع خالی_سوچک;

        صحيح minval = std::numeric_limits<صحيح>::max();
        صحيح maxval = std::numeric_limits<صحيح>::min();
        لكل (ثابت خودکار& it : points) {
            ثابت V3_Control_Ignores_سطر& cign = control_Ign_سطریں[it];
            إذا (cign.every_سطر()) {
                m_every_سطریں.emplace_پٹھ(it);
                استمر;
            }
            minval = std::min(minval, cign.m_سطر_Min);
            maxval = std::max(maxval, cign.m_سطر_Max);
        }

        ثابت صحيح center = minval + (maxval - minval) / 2;
        خودکار entp = std::unique_ptr<اندراج_2>(جديد اندراج_2{center});

        IgnIndices بایاں_Points;
        IgnIndices دایاں_Points;
        لكل (ثابت خودکار& it : points) {
            ثابت V3_Control_Ignores_سطر& cign = control_Ign_سطریں[it];
            إذا (cign.every_سطر()) استمر;
            // UINFO(9, "Inserting for center " << center << " point " << it);
            إذا (cign.m_سطر_Max < center)
                بایاں_Points.emplace_پٹھ(it);
            وإلا إذا (cign.m_سطر_Min > center)
                دایاں_Points.emplace_پٹھ(it);
            وإلا {
                entp->m_byMin.ڈالو_پٹھ(it);
                entp->m_byMax.ڈالو_پٹھ(it);
            }
        }

        std::ترتیب_دیو(entp->m_byMin.begin(), entp->m_byMin.سرا(), [](uint32_t a, uint32_t b) {
            أرجع control_Ign_سطریں[a].m_سطر_Min < control_Ign_سطریں[b].m_سطر_Min;
        });
        std::ترتیب_دیو(entp->m_byMax.begin(), entp->m_byMax.سرا(), [](uint32_t a, uint32_t b) {
            أرجع control_Ign_سطریں[a].m_سطر_Max > control_Ign_سطریں[b].m_سطر_Max;
        });

        entp->m_leftp = build_درخت(بایاں_Points);
        entp->m_rightp = build_درخت(دایاں_Points);
        أرجع entp;
    }

    فراغ لبھو_درخت(صحيح lineno, اندراج_2* entp, IgnIndices& resultsr, صحيح& اگلا_Changer) ثابت {
        // UINFO(9, "Find " << lineno << " center " << entp->m_center);
        إذا (lineno < entp->m_center) {
            اگلا_Changer = std::min(اگلا_Changer, entp->m_center);
            لكل (ثابت خودکار& it : entp->m_byMin) {
                ثابت V3_Control_Ignores_سطر& cign = control_Ign_سطریں[it];
                إذا (cign.m_سطر_Min <= lineno) {
                    resultsr.emplace_پٹھ(it);
                } وإلا {
                    اگلا_Changer = std::min(اگلا_Changer, cign.m_سطر_Min);
                    اكسر;
                }
            }
            إذا (entp->m_leftp) لبھو_درخت(lineno, entp->m_leftp.get(), resultsr, اگلا_Changer);
        } وإلا إذا (lineno > entp->m_center) {
            لكل (ثابت خودکار& it : entp->m_byMax) {
                ثابت V3_Control_Ignores_سطر& cign = control_Ign_سطریں[it];
                إذا (cign.m_سطر_Max >= lineno) {
                    اگلا_Changer = std::min(اگلا_Changer, cign.m_سطر_Max + 1);
                    resultsr.emplace_پٹھ(it);
                } وإلا {
                    اكسر;
                }
            }
            إذا (entp->m_rightp) لبھو_درخت(lineno, entp->m_rightp.get(), resultsr, اگلا_Changer);
        } وإلا {
            لكل (ثابت خودکار& it : entp->m_byMin) {
                ثابت V3_Control_Ignores_سطر& cign = control_Ign_سطریں[it];
                resultsr.emplace_پٹھ(it);
                إذا (cign.m_سطر_Max >= lineno)
                    اگلا_Changer = std::min(اگلا_Changer, cign.m_سطر_Max + 1);
            }
            إذا (entp->m_leftp) لبھو_درخت(lineno, entp->m_leftp.get(), resultsr, اگلا_Changer);
            إذا (entp->m_rightp) لبھو_درخت(lineno, entp->m_rightp.get(), resultsr, اگلا_Changer);
        }
    }

عام:
    // CONSTRUCTORS
    VInterval_درخت() = افتراضي;
    ~VInterval_درخت() = افتراضي;
    // METHODS
    فراغ clear() { m_rootp = خالی_سوچک; }
    فراغ build(ثابت IgnIndices& points) {
        clear();
        m_rootp = build_درخت(points);
    }
    فراغ لبھو(صحيح lineno, IgnIndices& resultsr, صحيح& اگلا_Changer) ثابت {
        resultsr = m_every_سطریں;
        اگلا_Changer = std::numeric_limits<صحيح>::max();

        إذا (m_rootp) لبھو_درخت(lineno, m_rootp.get(), resultsr, اگلا_Changer);
        // Sort indices, so can process in parse order
        std::ترتیب_دیو(resultsr.begin(), resultsr.سرا());
    }

    ساكن فراغ selfTest() {
        //            0     10     20     30     40     50
        // i0:        .     10-10  .      .      .      .
        // i1:        .     .      20-20  .      .      .
        // i2:        .     .      .      .      40-40  .
        // i3:        .     10------------30     .      .
        // i4:        .     .      20------------40     .
        // i5:(below) 0---------------------------------------0
        IgnIndices انکڑے;
        std::سمتیہ<std::pair<صحيح, صحيح>> points
            = {{10, 10}, {20, 20}, {40, 40}, {10, 30}, {20, 40}};
        لكل (ثابت خودکار& it : points) {
            control_Ign_سطریں.emplace_پٹھ(
                V3_Control_Ignores_سطر{V3_Error_کوڈ::I_LINT, it.پہلا, it.دوجا, سچ});
            انکڑے.emplace_پٹھ(static_cast<uint32_t>(control_Ign_سطریں.ناپ() - 1));
        }
        VInterval_درخت درخت;
        درخت.build(انکڑے);

        IgnIndices results;
        صحيح اگلا_Change = 0;
        درخت.لبھو(0, results, اگلا_Change);
        UASSERT_SELFTEST(results.ناپ(), 0);
        UASSERT_SELFTEST(اگلا_Change, 10);
        درخت.لبھو(10, results, اگلا_Change);
        UASSERT_SELFTEST(results.ناپ(), 2);
        UASSERT_SELFTEST(results[0], 0);
        UASSERT_SELFTEST(results[1], 3);
        UASSERT_SELFTEST(اگلا_Change, 11);
        درخت.لبھو(11, results, اگلا_Change);
        UASSERT_SELFTEST(results.ناپ(), 1);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(اگلا_Change, 15);  // Center, or would be 20
        درخت.لبھو(20, results, اگلا_Change);
        UASSERT_SELFTEST(results.ناپ(), 3);
        UASSERT_SELFTEST(results[0], 1);
        UASSERT_SELFTEST(results[1], 3);
        UASSERT_SELFTEST(results[2], 4);
        UASSERT_SELFTEST(اگلا_Change, 21);
        درخت.لبھو(21, results, اگلا_Change);
        UASSERT_SELFTEST(results.ناپ(), 2);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(اگلا_Change, 25);  // Center, or would be 30
        درخت.لبھو(30, results, اگلا_Change);
        UASSERT_SELFTEST(results.ناپ(), 2);
        UASSERT_SELFTEST(results[0], 3);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(اگلا_Change, 31);
        درخت.لبھو(40, results, اگلا_Change);
        UASSERT_SELFTEST(results.ناپ(), 2);
        UASSERT_SELFTEST(results[0], 2);
        UASSERT_SELFTEST(results[1], 4);
        UASSERT_SELFTEST(اگلا_Change, 41);
        درخت.لبھو(41, results, اگلا_Change);
        UASSERT_SELFTEST(results.ناپ(), 0);
        UASSERT_SELFTEST(اگلا_Change, std::numeric_limits<صحيح>::max());
        //
        points = {{0, 0}};
        لكل (ثابت خودکار& it : points) {
            control_Ign_سطریں.emplace_پٹھ(
                V3_Control_Ignores_سطر{V3_Error_کوڈ::I_LINT, it.پہلا, it.دوجا, سچ});
            انکڑے.emplace_پٹھ(static_cast<uint32_t>(control_Ign_سطریں.ناپ() - 1));
        }
        درخت.build(انکڑے);
        //
        درخت.لبھو(50, results, اگلا_Change);
        UASSERT_SELFTEST(results.ناپ(), 1);
        UASSERT_SELFTEST(results[0], 5);
    }
};

// Some attributes are attached to entities of the occur on a fileline
// and multiple attributes can be attached to a line
ورتوں V3_Control_سطر_Attribute = std::bitset<VPragmaType::ENUM_ناپ>;

صنف WaiverSetting final {
عام:
    V3_Error_کوڈ m_کوڈ;  // Error code
    string m_contents;  // --contents regexp
    string m_میل;  // --match regexp
    WaiverSetting(V3_Error_کوڈ کوڈ, ثابت string& contents, ثابت string& میل)
        : m_کوڈ{کوڈ}
        , m_contents{contents}
        , m_میل{میل} {}
    ~WaiverSetting() = افتراضي;
    WaiverSetting& عامل=(ثابت WaiverSetting& rhs) = افتراضي;
};

// File entity
صنف V3ControlFile final {
    ورتوں سطر_Attr_Map = std::map<صحيح, V3_Control_سطر_Attribute>;  // Map line->bitset of attributes
    ورتوں Waivers = std::سمتیہ<WaiverSetting>;  // List of {code,wildcard string}

    سطر_Attr_Map m_سطر_Attrs;  // Attributes to line mapping
    IgnIndices m_ignIndices;  // s_ignLines that apply to this specific file
    Waivers m_waivers;  // Waive messages
    VInterval_درخت m_interval_درخت;  // Tree of indices to IgnLines

    بنية {
        صحيح filenameno = -1;  // Last filename
        صحيح lineno = -1;  // Last linenumber
        صحيح اگلا_Change = -1;  // Line number of next change
    } m_آخری_Ignore;  // Last ignore line run

    // Match a given line and attribute to the map, line 0 is any
    منطقي سطر_میل(صحيح lineno, VPragmaType type) {
        إذا (m_سطر_Attrs.لبھو(0) != m_سطر_Attrs.سرا() && m_سطر_Attrs[0][type]) أرجع سچ;
        إذا (m_سطر_Attrs.لبھو(lineno) == m_سطر_Attrs.سرا()) أرجع خطأ;
        أرجع m_سطر_Attrs[lineno][type];
    }

عام:
    V3ControlFile() = افتراضي;
    فراغ update(ثابت V3ControlFile& file) {
        // Copy in all attributes and waivers
        لكل (ثابت خودکار& itr : file.m_سطر_Attrs) m_سطر_Attrs[itr.پہلا] |= itr.دوجا;
        m_waivers.reserve(m_waivers.ناپ() + file.m_waivers.ناپ());
        m_waivers.داخل_کرو(m_waivers.سرا(), file.m_waivers.begin(), file.m_waivers.سرا());
        // Copy in all ignore references
        m_ignIndices.reserve(m_ignIndices.ناپ() + file.m_ignIndices.ناپ());
        m_ignIndices.داخل_کرو(m_ignIndices.سرا(), file.m_ignIndices.begin(),
                            file.m_ignIndices.سرا());
        // updateFinalize() will soon build tree
    }
    فراغ updateFinalize() { m_interval_درخت.build(m_ignIndices); }
    فراغ جوڑو_سطر_Attribute(صحيح lineno, VPragmaType attr) { m_سطر_Attrs[lineno].سیٹ(attr); }
    فراغ جوڑو_Ignore(V3_Error_کوڈ کوڈ, صحيح min, صحيح max, منطقي on) {
        control_Ign_سطریں.emplace_پٹھ(V3_Control_Ignores_سطر{کوڈ, min, max, on});
        m_ignIndices.emplace_پٹھ(static_cast<uint32_t>(control_Ign_سطریں.ناپ() - 1));
        m_آخری_Ignore.اگلا_Change = -1;
    }
    فراغ جوڑو_Ignore_میل(V3_Error_کوڈ کوڈ, ثابت string& contents, ثابت string& میل) {
        // Since Verilator 5.031 the error message compared has context, so
        // allow old rules to still match using a final '*'
        string new_میل = میل;
        إذا (new_میل.خالی_تھاواں() || new_میل.پٹھ() != '*') new_میل += '*';
        m_waivers.emplace_پٹھ(کوڈ, contents, new_میل);
    }

    فراغ apply_بلاک(Ast_Gen_بلاک* nodep) {
        // Apply to block at this line
        ثابت VPragmaType pragma = VPragmaType::COVERAGE_بلاک_OFF;
        إذا (سطر_میل(nodep->fileline()->lineno(), pragma)) {
            nodep->جوڑو_Itemsp(جديد AstPragma{nodep->fileline(), pragma});
        }
    }
    فراغ apply_بلاک(Ast_گرہ_بلاک* nodep) {
        // Apply to block at this line
        ثابت VPragmaType pragma = VPragmaType::COVERAGE_بلاک_OFF;
        File_سطر* ثابت flp = nodep->fileline();
        إذا (سطر_میل(flp->lineno(), pragma)) {
            nodep->جوڑو_Stmtsp(جديد AstStmtPragma{flp, جديد AstPragma{flp, pragma}});
        }
    }
    فراغ applyCase(AstCase* nodep) {
        // Apply to this case at this line
        ثابت صحيح lineno = nodep->fileline()->lineno();
        إذا (سطر_میل(lineno, VPragmaType::بھریا_CASE)) nodep->بھریا_Pragma(سچ);
        إذا (سطر_میل(lineno, VPragmaType::PARALLEL_CASE)) nodep->parallelPragma(سچ);
    }
    فراغ applyIgnores(File_سطر* filelinep) {
        // HOT routine, called each parsed token line of this filename
        إذا (filelinep->filenameno() == m_آخری_Ignore.filenameno
            && filelinep->lineno() == m_آخری_Ignore.lineno)
            أرجع;  // Short circuit, no change, no debug
        // For speed, compute line number of next potential change, and skip if before then
        إذا (filelinep->filenameno() == m_آخری_Ignore.filenameno
            && filelinep->lineno() < m_آخری_Ignore.اگلا_Change) {
            m_آخری_Ignore.lineno = filelinep->lineno();
            UINFO(9, "   ApplyIgnores for " << filelinep->ascii() << " (no change predicted)");
            أرجع;
        }
        UINFO(9, "   ApplyIgnores for " << filelinep->ascii());
        m_آخری_Ignore.filenameno = filelinep->filenameno();
        m_آخری_Ignore.lineno = filelinep->lineno();
        m_آخری_Ignore.اگلا_Change = std::numeric_limits<صحيح>::max();

        // Find all on/offs.  Unlike lint pragmas we calculate the entire
        // bitset of errors to enable, instead of individual enable/disable changes.
        IgnIndices results;
        m_interval_درخت.لبھو(filelinep->lineno(), results /*ref*/, m_آخری_Ignore.اگلا_Change /*ref*/);
        // Process all on/offs for lines up to and including the current line
        VError_ثنائی_ہندسہ_سیٹ bitset{VError_ثنائی_ہندسہ_سیٹ::AllOnes{}};
        لكل (ثابت خودکار& it : results) {
            ثابت V3_Control_Ignores_سطر& cign = control_Ign_سطریں[it];
            UASSERT(cign.وچ_Range(filelinep->lineno()),
                    "Interval tree returned lines outside range");
            UINFO(9, "     Hit " << cign);
            cign.m_کوڈ.forDelegateCodes(
                [&](V3_Error_کوڈ subcode) { bitset.سیٹ(subcode, cign.m_on); });
        }

        filelinep->warn_سیٹ_Ctrl_ثنائی_ہندسہ_سیٹ(bitset);
        m_آخری_Ignore.اگلا_Change = std::max(m_آخری_Ignore.اگلا_Change, filelinep->lineno() + 1);
        UINFO(9, "    AppliedIgnores " << filelinep << " next@ln " << std::dec
                                       << m_آخری_Ignore.اگلا_Change
                                       << " Messages: " << bitset.ascii());
    }
    منطقي waive(V3_Error_کوڈ کوڈ, ثابت string& سنیہا) {
        إذا (کوڈ.hardError()) أرجع خطأ;
        لكل (ثابت خودکار& itr : m_waivers) {
            إذا ((کوڈ.isUnder(itr.m_کوڈ) || (itr.m_کوڈ == V3_Error_کوڈ::I_LINT))
                && VString::wildmatch(سنیہا, itr.m_میل)
                && WildcardContents::resolve(itr.m_contents)) {
                أرجع سچ;
            }
        }
        أرجع خطأ;
    }
};

ورتوں V3ControlFileResolver = V3ControlWildcardResolver<V3ControlFile>;

//######################################################################
// ScopeTrace tracking

صنف V3_Control_Scope_Trace_اندراج final {
عام:
    ثابت string m_scope;  // Scope or regexp to match
    ثابت منطقي m_on;  // True to enable message
    صحيح m_پدھراں = 0;  // # levels, 0 = all, 1 = only this, ...
    // CONSTRUCTORS
    V3_Control_Scope_Trace_اندراج(ثابت string& scope, منطقي on, صحيح پدھراں)
        : m_scope{scope}
        , m_on{on}
        , m_پدھراں{پدھراں} {}
    ~V3_Control_Scope_Trace_اندراج() = افتراضي;
    منطقي عامل<(ثابت V3_Control_Scope_Trace_اندراج& other) ثابت {
        إذا (m_on < other.m_on) أرجع سچ;
        إذا (m_on > other.m_on) أرجع خطأ;
        إذا (m_پدھراں < other.m_پدھراں) أرجع سچ;
        إذا (m_پدھراں > other.m_پدھراں) أرجع خطأ;
        أرجع m_scope < other.m_scope;
    }
};

// Tracks what matches are known to hit against V3ControlScopeTraceEntries
صنف V3_Control_Scope_Trace_اندراج_میل final {
عام:
    ثابت V3_Control_Scope_Trace_اندراج* m_entryp;  // Entry the match refers to
    ثابت string m_scopepart;  // Scope prefix being matched against the entry
    V3_Control_Scope_Trace_اندراج_میل(ثابت V3_Control_Scope_Trace_اندراج* entryp, ثابت string& scopepart)
        : m_entryp{entryp}
        , m_scopepart{scopepart} {}
    منطقي عامل<(ثابت V3_Control_Scope_Trace_اندراج_میل& other) ثابت {
        إذا (m_entryp < other.m_entryp) أرجع سچ;
        إذا (m_entryp > other.m_entryp) أرجع خطأ;
        أرجع m_scopepart < other.m_scopepart;
    }
};

صنف V3ControlScopeTraceResolver final {
    std::سمتیہ<V3_Control_Scope_Trace_اندراج> m_entries;  // User specified on/offs and levels
    std::map<V3_Control_Scope_Trace_اندراج_میل, منطقي> m_میل_Cache;  // Matching entries for speed

عام:
    فراغ جوڑو_Scope_Trace_On(منطقي on, ثابت string& scope, صحيح پدھراں) {
        UINFO(9, "addScopeTraceOn " << on << " '" << scope << "' " << " levels=" << پدھراں);
        m_entries.emplace_پٹھ(V3_Control_Scope_Trace_اندراج{scope, on, پدھراں});
        m_میل_Cache.clear();
    }

    منطقي get_اندراج_میل(ثابت V3_Control_Scope_Trace_اندراج* entp, ثابت string& scopepart) {
        // Return if a entry matches the scopepart, with memoization
        ثابت V3_Control_Scope_Trace_اندراج_میل کلید{entp, scopepart};
        ثابت خودکار pair = m_میل_Cache.emplace(کلید, خطأ);
        إذا (pair.دوجا) pair.پہلا->دوجا = VString::wildmatch(scopepart, entp->m_scope);
        أرجع pair.پہلا->دوجا;
    }

    منطقي getScopeTraceOn(ثابت string& scope) {
        // Apply in the order the user provided them, so they can choose on/off preferences
        صحيح maxLevel = 1;
        لكل (ثابت خودکار& ch : scope) {
            إذا (ch == '.') ++maxLevel;
        }
        UINFO(9, "getScopeTraceOn " << scope << " maxLevel=" << maxLevel);

        منطقي enabled = سچ;
        لكل (ثابت خودکار& ent : m_entries) {
            // We apply shortest match first for each rule component
            // (Otherwise the levels would be useless as "--scope top* --levels 1" would
            // always match at every scopepart, and we wouldn't know how to count levels)
            صحيح partLevel = 1;
            لكل (string::ناپ_type part_سرا = 0; سچ;) {
                part_سرا = scope.لبھو('.', part_سرا + 1);
                إذا (part_سرا == string::npos) part_سرا = scope.لمائی();
                ثابت std::string scopepart = scope.substr(0, part_سرا);
                إذا (get_اندراج_میل(&ent, scopepart)) {
                    ثابت منطقي level_میل
                        = !ent.m_پدھراں || (ent.m_پدھراں >= maxLevel - partLevel);
                    إذا (level_میل) enabled = ent.m_on;
                    UINFO(9, "getScopeTraceOn-part " << scope << " enabled=" << enabled
                                                     << " @ lev=" << partLevel
                                                     << (level_میل ? "[match]" : "[miss]")
                                                     << " from scopepart=" << scopepart);
                    اكسر;
                }
                إذا (part_سرا == scope.لمائی()) اكسر;
                ++partLevel;
            }
        }
        أرجع enabled;
    }
};

//######################################################################
// Resolve modules and files in the design

صنف V3_Control_Resolver_Hier_کامہ_اندراج final {
    ثابت صحيح m_workers;  // Number of workers requested for the hierarchical block
    File_سطر* ثابت m_flp;  // Location the worker count was specified at

عام:
    واضح V3_Control_Resolver_Hier_کامہ_اندراج(صحيح workers, File_سطر* flp)
        : m_workers{workers}
        , m_flp{flp} {}
    صحيح workers() ثابت { أرجع m_workers; }
    File_سطر* flp() ثابت { أرجع m_flp; }
};

صنف V3ControlResolver final {
    تعداد Profile_انکڑے_منوال : uint8_t { NONE = 0, MTASK = 1, HIER_DPI = 2 };
    V3ControlModuleResolver m_modules;  // Access to module names (with wildcards)
    V3ControlFileResolver m_files;  // Access to file names (with wildcards)
    V3ControlScopeTraceResolver m_scopeTraces;  // Regexp to trace enables
    std::unordered_map<string, std::unordered_map<string, uint64_t>>
        m_profile_انکڑے;  // Access to profile_data records
    uint8_t m_منوال = NONE;  // Kind of profile_data records is currently active
    std::unordered_map<string, V3_Control_Resolver_Hier_کامہ_اندراج> m_hierWorkers;
    File_سطر* m_profile_File_سطر = خالی_سوچک;  // Location profile_data was read from

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

    فراغ جوڑو_Profile_انکڑے(File_سطر* fl, ثابت string& hierDpi, uint64_t لاگت) {
        // Empty key for hierarchical DPI wrapper costs.
        جوڑو_Profile_انکڑے(fl, hierDpi, "", لاگت, HIER_DPI);
    }
    فراغ جوڑو_Profile_انکڑے(File_سطر* fl, ثابت string& model, ثابت string& کلید, uint64_t لاگت,
                        Profile_انکڑے_منوال منوال = MTASK) {
        إذا (!m_profile_File_سطر) m_profile_File_سطر = fl;
        إذا (لاگت == 0) لاگت = 1;  // Cost 0 means delete (or no data)
        إذا (منوال == MTASK) {
            m_profile_انکڑے[model][کلید] += لاگت;
        } وإلا إذا (منوال == HIER_DPI) {
            m_profile_انکڑے[model][کلید] = std::max(m_profile_انکڑے[model][کلید], لاگت);
        }
        m_منوال |= منوال;
    }
    منطقي شامل_اے_MTask_Profile_انکڑے() ثابت { أرجع m_منوال & MTASK; }
    uint64_t get_Profile_انکڑے(ثابت string& hierDpi) ثابت {
        // Empty key for hierarchical DPI wrapper costs.
        أرجع get_Profile_انکڑے(hierDpi, "");
    }
    فراغ جوڑو_Hier_Workers(File_سطر* flp, ثابت string& model, صحيح workers) {
        m_hierWorkers.emplace(std::piecewise_construct, std::forward_as_tuple(model),
                              std::forward_as_tuple(workers, flp));
    }
    صحيح getHierWorkers(ثابت string& model) ثابت {
        ثابت خودکار mit = m_hierWorkers.لبھو(model);
        // Assign a single worker if no specified.
        أرجع mit != m_hierWorkers.cend() ? mit->دوجا.workers() : 0;
    }
    File_سطر* get_Hier_Workers_File_سطر(ثابت string& model) ثابت {
        ثابت خودکار mit = m_hierWorkers.لبھو(model);
        أرجع mit != m_hierWorkers.cend() ? mit->دوجا.flp() : v3Global.rootp()->fileline();
    }
    uint64_t get_Profile_انکڑے(ثابت string& model, ثابت string& کلید) ثابت {
        ثابت خودکار mit = m_profile_انکڑے.لبھو(model);
        إذا (mit == m_profile_انکڑے.cend()) أرجع 0;
        ثابت خودکار it = mit->دوجا.لبھو(کلید);
        إذا (it == mit->دوجا.cend()) أرجع 0;
        أرجع it->دوجا;
    }
    File_سطر* get_Profile_انکڑے_File_سطر() ثابت { أرجع m_profile_File_سطر; }  // Maybe null
    ساكن uint64_t get_Current_Hier_بلاک_لاگت() {
        إذا (uint64_t لاگت = V3Control::get_Profile_انکڑے(v3Global.opt.اگیتر())) {
            UINFO(9, "Fetching cost from profile info: " << لاگت);
            أرجع لاگت;
        }

        // Without profiling data, sum the regions evaluated on each time step
        AstNetlist* ثابت netlistp = v3Global.rootp();
        uint64_t لاگت = 0;
        لكل (صحيح i = 0; i < VEval::ENUM_سرا; ++i) {
            ثابت VEval eval{i};
            إذا (eval.slow()) استمر;
            لاگت += V3_Instr_گنتی::گنتی(netlistp->evalFuncp(eval), خطأ);
        }
        UINFO(9, "Evaluating cost: " << لاگت);
        أرجع لاگت;
    }
};

//######################################################################
// V3Control

فراغ V3Control::جوڑو_Case_بھریا(ثابت string& filename, صحيح lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.جوڑو_سطر_Attribute(lineno, VPragmaType::بھریا_CASE);
}
فراغ V3Control::جوڑو_Case_Parallel(ثابت string& filename, صحيح lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.جوڑو_سطر_Attribute(lineno, VPragmaType::PARALLEL_CASE);
}
فراغ V3Control::جوڑو_Coverage_بلاک_Off(ثابت string& filename, صحيح lineno) {
    V3ControlFile& file = V3ControlResolver::s().files().at(filename);
    file.جوڑو_سطر_Attribute(lineno, VPragmaType::COVERAGE_بلاک_OFF);
}
فراغ V3Control::جوڑو_Coverage_بلاک_Off(ثابت string& module, ثابت string& blockname) {
    V3ControlResolver::s().modules().at(module).جوڑو_Coverage_بلاک_Off(blockname);
}

فراغ V3Control::جوڑو_Hier_Workers(File_سطر* fl, ثابت string& model, صحيح workers) {
    V3ControlResolver::s().جوڑو_Hier_Workers(fl, model, workers);
}

فراغ V3Control::جوڑو_Fsm_رجسٹر_Wrapper(File_سطر* fl, ثابت string& module, ثابت string& d,
                                      ثابت string& q, ثابت string& clock, ثابت string& reset,
                                      ثابت string& reset_قدر) {
    string missing;
    إذا (module.خالی_تھاواں()) missing += "-module";
    إذا (d.خالی_تھاواں()) missing = VString::dot(missing, ", ", "-d");
    إذا (q.خالی_تھاواں()) missing = VString::dot(missing, ", ", "-q");
    إذا (clock.خالی_تھاواں()) missing = VString::dot(missing, ", ", "-clock");
    إذا (!missing.خالی_تھاواں()) {
        fl->v3error("fsm_register_wrapper missing " << missing);
        أرجع;
    }

    ثابت Fsm_رجسٹر_Wrapper desc{module, d, q, clock, reset, reset_قدر};
    V3ControlResolver::s().modules().at(module).سیٹ_Fsm_رجسٹر_Wrapper(fl, desc);
}

فراغ V3Control::جوڑو_Ignore(V3_Error_کوڈ کوڈ, منطقي on, ثابت string& filename, صحيح min, صحيح max) {
    UINFO(9, "addIgnore " << کوڈ << " " << min << "-" << max << " fn=" << filename);
    إذا (filename == "*") {  // For "lint_off/lint_on [--rule x]"
        File_سطر::globalWarnOff(کوڈ, !on);
    } وإلا {  // For "lint_off/lint_on [--rule x] --file y [-lines min[-max]]"
        V3ControlResolver::s().files().at(filename).جوڑو_Ignore(کوڈ, min, max, on);
    }
}
فراغ V3Control::جوڑو_Ignore_میل(V3_Error_کوڈ کوڈ, ثابت string& filename, ثابت string& contents,
                               ثابت string& میل) {
    // For "lint_off --rule x --file y --match z", no support for lint_on
    V3ControlResolver::s().files().at(filename).جوڑو_Ignore_میل(کوڈ, contents, میل);
}

فراغ V3Control::جوڑو_Inline(File_سطر* fl, ثابت string& module, ثابت string& ftask, منطقي on) {
    إذا (ftask.خالی_تھاواں()) {
        V3ControlResolver::s().modules().at(module).سیٹ_Inline(on);
    } وإلا {
        إذا (!on) {
            fl->v3warn(E_UNSUPPORTED, "Unsupported: no_inline for tasks");
        } وإلا {
            V3ControlResolver::s().modules().at(module).ftasks().at(ftask).سیٹ_No_Inline(on);
        }
    }
}

فراغ V3Control::جوڑو_Module_Pragma(ثابت string& module, VPragmaType pragma) {
    V3ControlResolver::s().modules().at(module).جوڑو_Module_Pragma(pragma);
}

فراغ V3Control::جوڑو_Profile_انکڑے(File_سطر* fl, ثابت string& hierDpi, uint64_t لاگت) {
    V3ControlResolver::s().جوڑو_Profile_انکڑے(fl, hierDpi, لاگت);
}
فراغ V3Control::جوڑو_Profile_انکڑے(File_سطر* fl, ثابت string& model, ثابت string& کلید,
                               uint64_t لاگت) {
    V3ControlResolver::s().جوڑو_Profile_انکڑے(fl, model, کلید, لاگت);
}

فراغ V3Control::جوڑو_Scope_Trace_On(منطقي on, ثابت string& scope, صحيح پدھراں) {
    V3ControlResolver::s().scopeTraces().جوڑو_Scope_Trace_On(on, scope, پدھراں);
}

فراغ V3Control::جوڑو_Var_Attr(File_سطر* fl, ثابت string& module, ثابت string& ftask,
                           Var_Spec_قسم قسم, ثابت string& نمونہ_2, VAttrType attr,
                           Ast_Sen_درخت* sensep) {
    إذا (sensep) {
        File_سطر* ثابت flp = sensep->fileline();
        // Historical, not actually needed, only parsed for compatibility, delete it
        VL_DO_DANGLING(sensep->delete_درخت(), sensep);
        // Used to be only accepted on public_flat_rw
        إذا (attr != VAttrType::VAR_عوامی_FLAT_RW) {
            flp->v3error("sensitivity not expected for attribute");
            أرجع;
        }
    }

    إذا (قسم != Var_Spec_قسم::VAR) {
        بدل (attr) {
        حالة VAttrType::VAR_عوامی_FLAT:
        حالة VAttrType::VAR_عوامی_FLAT_RD:
        حالة VAttrType::VAR_عوامی_FLAT_RW: اكسر;
        افتراضي:
            fl->v3error("'"s + attr.ascii() + "' attribute does not accept -param/-port");
            أرجع;
        }
    }

    // Semantics: Most of the attributes operate on signals
    إذا (نمونہ_2.خالی_تھاواں()) {
        إذا (attr == VAttrType::VAR_عوامی) {
            إذا (ftask.خالی_تھاواں()) {
                // public module, this is the only exception from var here
                V3ControlResolver::s().modules().at(module).جوڑو_Module_Pragma(
                    VPragmaType::عوامی_MODULE);
            } وإلا {
                V3ControlResolver::s().modules().at(module).ftasks().at(ftask).سیٹ_عوامی(سچ);
            }
        } وإلا {
            fl->v3error("missing -var");
        }
    } وإلا {
        إذا (attr == VAttrType::VAR_FORCEABLE) {
            إذا (module.خالی_تھاواں()) {
                fl->v3error("forceable missing -module");
            } وإلا إذا (!ftask.خالی_تھاواں()) {
                fl->v3error("Signals inside functions/tasks cannot be marked forceable");
            } وإلا {
                V3ControlResolver::s().modules().at(module).vars().at(نمونہ_2).جوڑو(attr);
            }
        } وإلا {
            V3ControlModule& mod = V3ControlResolver::s().modules().at(module);
            V3ControlVar& controlVar = [&]() -> V3ControlVar& {
                إذا (ftask.خالی_تھاواں()) {
                    إذا (قسم == Var_Spec_قسم::PARAM) أرجع mod.params().at(نمونہ_2);
                    إذا (قسم == Var_Spec_قسم::پورٹ) أرجع mod.ports().at(نمونہ_2);
                    UASSERT_OBJ(قسم == Var_Spec_قسم::VAR, fl, "Unexpected VarSpecKind");
                    أرجع mod.vars().at(نمونہ_2);
                }
                إذا (قسم == Var_Spec_قسم::PARAM) أرجع mod.ftasks().at(ftask).params().at(نمونہ_2);
                إذا (قسم == Var_Spec_قسم::پورٹ) أرجع mod.ftasks().at(ftask).ports().at(نمونہ_2);
                UASSERT_OBJ(قسم == Var_Spec_قسم::VAR, fl, "Unexpected VarSpecKind");
                أرجع mod.ftasks().at(ftask).vars().at(نمونہ_2);
            }();
            controlVar.جوڑو(attr);
        }
    }
}

فراغ V3Control::applyCase(AstCase* nodep) {
    ثابت string& filename = nodep->fileline()->filename();
    V3ControlFile* ثابت filep = V3ControlResolver::s().files().resolve(filename);
    إذا (filep) filep->applyCase(nodep);
}

فراغ V3Control::apply_Coverage_بلاک(Ast_گرہ_Module* modulep, AstBegin* nodep) {
    ثابت string& filename = nodep->fileline()->filename();
    V3ControlFile* ثابت filep = V3ControlResolver::s().files().resolve(filename);
    إذا (filep) filep->apply_بلاک(nodep);
    ثابت string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ثابت modp = V3ControlResolver::s().modules().resolve(modname);
    إذا (modp) modp->apply_بلاک(nodep);
}
فراغ V3Control::apply_Coverage_بلاک(Ast_گرہ_Module* modulep, Ast_Gen_بلاک* nodep) {
    ثابت string& filename = nodep->fileline()->filename();
    V3ControlFile* ثابت filep = V3ControlResolver::s().files().resolve(filename);
    إذا (filep) filep->apply_بلاک(nodep);
    ثابت string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ثابت modp = V3ControlResolver::s().modules().resolve(modname);
    إذا (modp) modp->apply_بلاک(nodep);
}

فراغ V3Control::applyIgnores(File_سطر* filelinep) {
    ثابت string& filename = filelinep->filename();
    V3ControlFile* ثابت filep = V3ControlResolver::s().files().resolve(filename);
    إذا (filep) filep->applyIgnores(filelinep);
}

فراغ V3Control::applyModule(Ast_گرہ_Module* modulep) {
    ثابت string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ثابت modp = V3ControlResolver::s().modules().resolve(modname);
    إذا (modp) modp->apply(modulep);
}

فراغ V3Control::applyFTask(Ast_گرہ_Module* modulep, Ast_گرہ_FTask* ftaskp) {
    ثابت string& modname = modulep->prettyDehashOrigOrName();
    V3ControlModule* ثابت modp = V3ControlResolver::s().modules().resolve(modname);
    إذا (!modp) أرجع;
    ثابت V3ControlFTask* ثابت ftp = modp->ftasks().resolve(ftaskp->prettyDehashOrigOrName());
    إذا (ftp) ftp->apply(ftaskp);
}

سانچا <قسم_ناں T_Resolver>
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

فراغ V3Control::applyVarAttr(ثابت Ast_گرہ_Module* modulep, ثابت Ast_گرہ_FTask* ftaskp,
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
File_سطر* V3Control::get_Hier_Workers_File_سطر(ثابت string& model) {
    أرجع V3ControlResolver::s().get_Hier_Workers_File_سطر(model);
}
ثابت V3Control::Fsm_رجسٹر_Wrapper* V3Control::get_Fsm_رجسٹر_Wrapper(ثابت string& module) {
    V3ControlModule* ثابت modp = V3ControlResolver::s().modules().resolve(module);
    أرجع modp ? modp->fsm_رجسٹر_Wrapperp() : خالی_سوچک;
}
uint64_t V3Control::get_Profile_انکڑے(ثابت string& hierDpi) {
    أرجع V3ControlResolver::s().get_Profile_انکڑے(hierDpi);
}
uint64_t V3Control::get_Profile_انکڑے(ثابت string& model, ثابت string& کلید) {
    أرجع V3ControlResolver::s().get_Profile_انکڑے(model, کلید);
}
File_سطر* V3Control::get_Profile_انکڑے_File_سطر() {
    أرجع V3ControlResolver::s().get_Profile_انکڑے_File_سطر();
}
منطقي V3Control::getScopeTraceOn(ثابت string& scope) {
    أرجع V3ControlResolver::s().scopeTraces().getScopeTraceOn(scope);
}

فراغ V3Control::contents_ڈالو_متن(ثابت string& متن) { WildcardContents::ڈالو_متن(متن); }

منطقي V3Control::شامل_اے_MTask_Profile_انکڑے() {
    أرجع V3ControlResolver::s().شامل_اے_MTask_Profile_انکڑے();
}
uint64_t V3Control::get_Current_Hier_بلاک_لاگت() {
    أرجع V3ControlResolver::s().get_Current_Hier_بلاک_لاگت();
}

منطقي V3Control::waive(ثابت File_سطر* filelinep, V3_Error_کوڈ کوڈ, ثابت string& سنیہا) {
    V3ControlFile* ثابت filep = V3ControlResolver::s().files().resolve(filelinep->filename());
    إذا (!filep) أرجع خطأ;
    أرجع filep->waive(کوڈ, سنیہا);
}

فراغ V3Control::selfTest() { VInterval_درخت::selfTest(); }
