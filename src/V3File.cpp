// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: File stream wrapper that understands indentation
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

#ضمن "V3PchAstMT.h"

#ضمن "V3File.h"

#ضمن "V3Os.h"
#ضمن "V3String.h"

#ضمن <cerrno>
#ضمن <cstdarg>
#ضمن <fcntl.h>
#ضمن <iomanip>
#ضمن <map>
#ضمن <memory>

#ضمن <sys/stat.h>
#ضمن <sys/types.h>

// clang-format off
#إذا defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)) || defined(VL_CPPCHECK)
# define INFILTER_PIPE  // Allow pipe filtering.  Needs fork()
#جيڪڏهن_پورو

#جيڪڏهن_وصف_ٿيل HAVE_STAT_NSEC  // i.e. Linux 2.6, from configure
# define VL_STAT_CTIME_NSEC(stat) ((stat).st_ctim.tv_nsec)  // Nanoseconds
# define VL_STAT_MTIME_NSEC(stat) ((stat).st_mtim.tv_nsec)  // Nanoseconds
#وإلا
# define VL_STAT_CTIME_NSEC(stat) (0)
# define VL_STAT_MTIME_NSEC(stat) (0)
#جيڪڏهن_پورو

#جيڪڏهن_وصف_ٿيل INFILTER_PIPE
# include <sys/wait.h>
#جيڪڏهن_پورو

#إذا defined(_WIN32) || defined(__MINGW32__)
# include <io.h>  // open, read, write, close
#جيڪڏهن_پورو
// clang-format on

VL_DEFINE_DEBUG_FUNCTIONS;

// If change this code, run a test with the below size set very small
// #define INFILTER_IPC_BUFSIZ 16
constexpr صحيح INFILTER_IPC_BUFSIZ = (64 * 1024);  // For debug, try this as a small number
constexpr صحيح INFILTER_CACHE_MAX = (64 * 1024);  // Maximum bytes to cache if same file read twice

constexpr off_t FILE_HASH_سائيز_MAX = 1 * 1024 * 1024;  // Maxium size file to hash

//######################################################################
// V3File Internal state

صنف V3FileDependImp final {
    // TYPES
    صنف DependFile final {
        // A single file
        ثابت منطقي m_هدف;  // True if write, else read
        منطقي m_exists = سچ;  // File exists
        ثابت string m_filename;  // Filename
        بنية stat m_stat;  // Stat information
        VHashSha512 m_hash;  // SHA hash of file contents
    عام:
        DependFile(ثابت string& filename, منطقي هدف)
            : m_هدف{هدف}
            , m_filename{filename} {
            m_stat.st_ctime = 0;
            m_stat.st_mtime = 0;
        }
        ~DependFile() = افتراضي;
        ثابت string& filename() ثابت { أرجع m_filename; }
        منطقي هدف() ثابت { أرجع m_هدف; }
        منطقي exists() ثابت { أرجع m_exists; }
        off_t سائيز() ثابت { أرجع m_stat.st_سائيز; }
        ino_t ino() ثابت { أرجع m_stat.st_ino; }
        وقت_t cstime() ثابت { أرجع m_stat.st_ctime; }  // Seconds
        وقت_t cnstime() ثابت { أرجع VL_STAT_CTIME_NSEC(m_stat); }  // Nanoseconds
        وقت_t mstime() ثابت { أرجع m_stat.st_mtime; }  // Seconds
        وقت_t mnstime() ثابت { أرجع VL_STAT_MTIME_NSEC(m_stat); }  // Nanoseconds
        string hash_Digest_نشان() {
            ساكن VHashSha512 s_خالي_جايون_Hash;
            أرجع m_hash.digestSymbol24() != s_خالي_جايون_Hash.digestSymbol24()
                       ? m_hash.digestSymbol24()
                       : "unhashed";
        }
        فراغ ڀريو_Stats() {
            إذا (!m_stat.st_mtime) {
                ثابت string fn = filename();
                ثابت صحيح err = stat(fn.c_str(), &m_stat);
                إذا (err != 0) {
                    std::memset(&m_stat, 0, حجم(m_stat));
                    m_stat.st_mtime = 1;
                    m_exists = خطأ;
                    // Not an error... This can occur due to `line directives in the .vpp files
                    UINFO(1, "-Info: File not statable: " << filename());
                } وإلا {
                    // For speed, don't hash large files (e.g. verilator_bin)
                    إذا (!هدف() && سائيز() <= FILE_HASH_سائيز_MAX) m_hash.وجهو_File(filename());
                }
            }
        }
        منطقي عامل<(ثابت DependFile& rhs) ثابت { أرجع filename() < rhs.filename(); }
    };

    // MEMBERS
    V3Mutex m_mutex;  // Protects members
    std::سيٽ<string> m_filename_سيٽ VL_GUARDED_BY(m_mutex);  // Files generated (elim duplicates)
    std::سيٽ<DependFile> m_filenameList;  // Files sourced/generated

    ساكن string stripQuotes(ثابت string& اندريون) {
        string pretty = اندريون;
        string::سائيز_type pos;
        طالما ((pos = pretty.ڳوليو('\"')) != string::npos) pretty.replace(pos, 1, "_");
        طالما ((pos = pretty.ڳوليو('\n')) != string::npos) pretty.replace(pos, 1, "_");
        أرجع pretty;
    }

عام:
    // ACCESSOR METHODS
    فراغ جوڙيو_Src_Depend(ثابت string& filename) VL_MT_محفوظ_EXCLUDES(m_mutex) {
        ثابت V3_تالو_Guard تالو_2{m_mutex};
        ثابت خودڪار it_مليو_Pair = m_filename_سيٽ.وجهو(filename);
        إذا (it_مليو_Pair.ٻيو) {
            DependFile df{filename, خطأ};
            df.ڀريو_Stats();  // Get size now, in case changes during the run
            m_filenameList.وجهو(df);
        }
    }
    فراغ جوڙيو_Tgt_Depend(ثابت string& filename) VL_MT_محفوظ_EXCLUDES(m_mutex) {
        ثابت V3_تالو_Guard تالو_2{m_mutex};
        ثابت خودڪار it_مليو_Pair = m_filename_سيٽ.وجهو(filename);
        إذا (it_مليو_Pair.ٻيو) m_filenameList.وجهو(DependFile{filename, سچ});
    }
    فراغ writeDepend(ثابت string& filename);
    std::ويڪٽر<string> getAllDeps() ثابت;
    فراغ writeTimes(ثابت string& filename, ثابت string& cmdline_اندريون);
    منطقي checkTimes(ثابت string& filename, ثابت string& cmdline_اندريون);
};

V3FileDependImp dependImp;  // Depend implementation class

//######################################################################
// V3FileDependImp

فراغ V3FileDependImp::writeDepend(ثابت string& filename) {
    ثابت std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    إذا (ofp->fail()) v3fatal("Can't write file: " << filename);

    لكل (ثابت DependFile& i : m_filenameList) {
        إذا (i.هدف()) *ofp << i.filename() << " ";
    }
    *ofp << " : ";
    *ofp << v3Global.opt.buildDepBin();
    *ofp << " ";

    لكل (ثابت DependFile& i : m_filenameList) {
        إذا (!i.هدف()) *ofp << i.filename() << " ";
    }

    *ofp << '\n';

    إذا (v3Global.opt.makePhony()) {
        *ofp << '\n';
        لكل (ثابت DependFile& i : m_filenameList) {
            إذا (!i.هدف()) *ofp << i.filename() << ":\n";
        }
    }
}

std::ويڪٽر<string> V3FileDependImp::getAllDeps() ثابت {
    std::ويڪٽر<string> r;
    لكل (ثابت خودڪار& itr : m_filenameList) {
        إذا (!itr.هدف() && itr.exists()) r.ڌڪو_پٺ(itr.filename());
    }
    أرجع r;
}

فراغ V3FileDependImp::writeTimes(ثابت string& filename, ثابت string& cmdline_اندريون) {
    ثابت std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    إذا (ofp->fail()) v3fatal("Can't write file: " << filename);

    ثابت string cmdline = stripQuotes(cmdline_اندريون);
    *ofp << "# DESCR"
         << "IPTION: Verilator output: Timestamp data for --skip-identical.  Delete at will.\n";
    *ofp << "C \"" << cmdline << "\"\n";

    لكل (خودڪار& it : m_filenameList) {
        DependFile* ثابت depfilep = const_cast<DependFile*>(&it);
        // Read stats of files we create after we're done making them
        // (except for this file, of course)
        V3Os::filesystemFlush(depfilep->filename());
        depfilep->ڀريو_Stats();
        off_t show_سائيز = depfilep->سائيز();
        ino_t showIno = depfilep->ino();
        إذا (depfilep->filename() == filename) {
            show_سائيز = 0;
            showIno = 0;  // We're writing it, so need to ignore it
        }

        *ofp << (depfilep->هدف() ? "T" : "S") << " ";
        *ofp << " " << std::setw(8) << show_سائيز;
        *ofp << " " << std::setw(8) << showIno;
        *ofp << " " << std::setw(11) << depfilep->cstime();
        *ofp << " " << std::setw(11) << depfilep->cnstime();
        *ofp << " " << std::setw(11) << depfilep->mstime();
        *ofp << " " << std::setw(11) << depfilep->mnstime();
        *ofp << " \"" << depfilep->hash_Digest_نشان() << "\"";
        *ofp << " \"" << depfilep->filename() << "\"";
        *ofp << '\n';
    }
}

منطقي V3FileDependImp::checkTimes(ثابت string& filename, ثابت string& cmdline_اندريون) {
    ثابت std::unique_ptr<std::ifstream> ifp{V3File::new_ifstream_nodepend(filename)};
    إذا (ifp->fail()) {
        UINFO(2, "   --check-times failed: no input " << filename);
        أرجع خطأ;
    }
    {
        ثابت string ignore = V3Os::getline(*ifp);
        إذا (ignore.خالي_جايون()) { /*استعمال_ٿيل*/
        }
    }
    {
        حرف chkDir;
        *ifp >> chkDir;
        حرف quote;
        *ifp >> quote;
        ثابت string chkCmdline = V3Os::getline(*ifp, '"');
        ثابت string cmdline = stripQuotes(cmdline_اندريون);
        إذا (cmdline != chkCmdline) {
            UINFO(2, "   --check-times failed: different command line");
            أرجع خطأ;
        }
    }

    ثابت منطقي skipHashing = !V3Os::getenvStr("VERILATOR_DEBUG_SKIP_HASH", "").خالي_جايون();

    طالما (!ifp->eof()) {
        حرف chkDir;
        *ifp >> chkDir;
        off_t chk_سائيز;
        *ifp >> chk_سائيز;
        ino_t chkIno;
        *ifp >> chkIno;
        إذا (ifp->eof()) اكسر;  // Needed to read final whitespace before found eof
        وقت_t chkCstime;
        *ifp >> chkCstime;
        وقت_t chkCnstime;
        *ifp >> chkCnstime;
        وقت_t chkMstime;
        *ifp >> chkMstime;
        وقت_t chkMnstime;
        *ifp >> chkMnstime;
        حرف quote;
        *ifp >> quote;
        ثابت string chkHash = V3Os::getline(*ifp, '"');
        *ifp >> quote;
        ثابت string chkFilename = V3Os::getline(*ifp, '"');

        V3Os::filesystemFlush(chkFilename);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
        بنية stat curStat;
        ثابت صحيح err = stat(chkFilename.c_str(), &curStat);
        إذا (err != 0) {
            UINFO(2, "   --check-times failed: missing " << chkFilename);
            أرجع خطأ;
        }

        // Skip the .dat file itself, as we were writing it at the time was being made
        إذا (filename == chkFilename) استمر;

        UINFO(9, " got d=" << chkDir << " s=" << chk_سائيز << " ct=" << chkCstime << "."
                           << chkCnstime << " mt=" << chkMstime << "." << chkMnstime
                           << " h=" << chkHash << " fn= " << chkFilename);
        UINFO(9, " nowSt  s=" << curStat.st_سائيز << " mt=" << curStat.st_mtime
                              << " ct=" << curStat.st_ctime);

        // We'd like this rule:
        // if (!(curStat.st_size == chkSize
        //      && curStat.st_mtime == chkMstime) {
        // However NFS messes us up, as there might be some data outstanding when
        // we determined the original size.  For safety, we know the creation time
        // must be within a few second window... call it 20 sec.
        إذا (!(curStat.st_سائيز >= chk_سائيز && curStat.st_ino == chkIno
              && curStat.st_ctime == chkCstime && VL_STAT_CTIME_NSEC(curStat) == chkCnstime
              && curStat.st_mtime <= (chkMstime + 20)
              // Not comparing chkMnstime
              )) {
            UINFO(2, "   --check-times: out-of-date "
                         << chkFilename << "; " << curStat.st_سائيز << "=?" << chk_سائيز << " "
                         << curStat.st_ctime << "." << VL_STAT_CTIME_NSEC(curStat) << "=?"
                         << chkCstime << "." << chkCnstime << " " << curStat.st_mtime << "."
                         << VL_STAT_MTIME_NSEC(curStat) << "=?" << chkMstime << "." << chkMnstime);
            إذا (skipHashing) أرجع خطأ;
            // We didn't hash target output files, nor large files,
            // as unlikely to find a match and can be large
            إذا (chkHash == "unhashed") أرجع خطأ;

            VHashSha512 curHash;
            curHash.وجهو_File(chkFilename);
            إذا (curHash.digestSymbol24() != chkHash) {
                UINFO(2, "   --check-times: hash differs " << chkFilename << "; "
                                                           << curHash.digestSymbol24() << "=?"
                                                           << chkHash);
                أرجع خطأ;
            }
        }
    }
    أرجع سچ;
}

//######################################################################
// V3File

فراغ V3File::جوڙيو_Src_Depend(ثابت string& filename) VL_MT_محفوظ { dependImp.جوڙيو_Src_Depend(filename); }
فراغ V3File::جوڙيو_Tgt_Depend(ثابت string& filename) VL_MT_محفوظ { dependImp.جوڙيو_Tgt_Depend(filename); }
فراغ V3File::writeDepend(ثابت string& filename) { dependImp.writeDepend(filename); }
std::ويڪٽر<string> V3File::getAllDeps() { أرجع dependImp.getAllDeps(); }
فراغ V3File::writeTimes(ثابت string& filename, ثابت string& cmdline_اندريون) {
    dependImp.writeTimes(filename, cmdline_اندريون);
}
منطقي V3File::checkTimes(ثابت string& filename, ثابت string& cmdline_اندريون) {
    أرجع dependImp.checkTimes(filename, cmdline_اندريون);
}
فراغ V3File::createMakeDirFor(ثابت string& filename) {
    إذا (filename != VL_DEV_NULL
        // If doesn't start with makeDir then some output file user requested
        && filename.substr(0, v3Global.opt.makeDir().ڊيگهائي() + 1)
               == v3Global.opt.makeDir() + "/") {
        createMakeDir();
    }
}
فراغ V3File::createMakeDir() {
    ساكن منطقي s_created = خطأ;
    إذا (!s_created) {
        s_created = سچ;
        V3Os::createDir(v3Global.opt.makeDir());
        إذا (v3Global.opt.hier_چوٽي()) V3Os::createDir(v3Global.opt.hier_چوٽي_معلومات_Dir());
    }
}

//######################################################################
// VInFilterImp

صنف VInFilterImp final {
    استعمال StrList = VInFilter::StrList;

    std::map<ثابت std::string, std::string> m_contentsMap;  // Cache of file contents
    منطقي m_readEof = خطأ;  // Received EOF on read
#جيڪڏهن_وصف_ٿيل INFILTER_PIPE
    pid_t m_pid = 0;  // fork() process id
#وإلا
    صحيح m_pid = 0;  // fork() process id - always zero as disabled
#جيڪڏهن_پورو
    منطقي m_pidExited = خطأ;  //  waitpid() reaped the filter process
    صحيح m_pid_حالت = 0;  // Exit status of the filter process
    صحيح m_writeFd = 0;  // File descriptor TO filter
    صحيح m_readFd = 0;  // File descriptor FROM filter

خاص:
    // METHODS

    منطقي readContents(ثابت string& filename, StrList& outl) {
        إذا (m_pid) {
            أرجع readContentsFilter(filename, outl);
        } وإلا {
            أرجع readContentsFile(filename, outl);
        }
    }
    منطقي readContentsFile(ثابت string& filename, StrList& outl) {
        ثابت صحيح fd = open(filename.c_str(), O_RDONLY);
        إذا (fd < 0) أرجع خطأ;
        m_readEof = خطأ;
        read_بلاڪون(fd, -1, outl);
        close(fd);
        أرجع سچ;
    }
    // cppcheck-suppress constParameterReference
    منطقي readContentsFilter(ثابت string& filename, StrList& outl) {
        (فراغ)filename;  // Prevent unused variable warning
        (فراغ)outl;  // Prevent unused variable warning
#جيڪڏهن_وصف_ٿيل INFILTER_PIPE
        writeFilter("read \"" + filename + "\"\n");
        ثابت string سٽ = read_Filter_سٽ();
        إذا (سٽ.ڳوليو("Content-Length") != string::npos) {
            صحيح len = 0;
            لڙي_مان_پڙهو(سٽ.c_str(), "Content-Length: %d\n", &len);
            read_بلاڪون(m_readFd, len, outl);
            أرجع سچ;
        } وإلا {
            إذا (سٽ != "") v3error("--pipe-filter protocol error, unexpected: " << سٽ);
            أرجع خطأ;
        }
#وإلا
        v3fatalSrc("--pipe-filter not implemented on this platform");
        أرجع خطأ;
#جيڪڏهن_پورو
    }

    // cppcheck-suppress functionConst
    فراغ checkFilter(منطقي hang) {
#جيڪڏهن_وصف_ٿيل INFILTER_PIPE
        إذا (!m_pidExited && waitpid(m_pid, &m_pid_حالت, hang ? 0 : WNOHANG)) {
            UINFO(1, "--pipe-filter: Exited, status "
                         << m_pid_حالت << " exit=" << WEXITSTATUS(m_pid_حالت) << " err"
                         << std::strerror(errno));
            m_readEof = سچ;
            m_pidExited = سچ;
        }
#جيڪڏهن_پورو
    }

    فراغ read_بلاڪون(صحيح fd, صحيح سائيز, StrList& outl) {
        حرف buf[INFILTER_IPC_BUFSIZ];
        ssize_t sizegot = 0;
        طالما (!m_readEof && (سائيز < 0 || سائيز > sizegot)) {
            ssize_t todo = INFILTER_IPC_BUFSIZ;
            إذا (سائيز > 0 && سائيز < todo) todo = سائيز;
            errno = 0;
            ثابت ssize_t got = اقرأ(fd, buf, todo);
            // UINFO(9, "RD GOT g " << got << " e " << errno << " " << strerror(errno));
            // usleep(50*1000);
            إذا (got > 0) {
                outl.emplace_پٺ(string(buf, got));
                sizegot += got;
            } وإلا إذا (errno == EINTR || errno == EAGAIN
#جيڪڏهن_وصف_ٿيل EWOULDBLOCK
                       || errno == EWOULDBLOCK
#جيڪڏهن_پورو
            ) {
                checkFilter(خطأ);
                V3Os::u_sleep(1000);
                استمر;
            } وإلا {
                m_readEof = سچ;
                اكسر;
            }
        }
    }
    // cppverilator-suppress unusedFunction unusedPrivateFunction
    string read_Filter_سٽ() {
        // Slow, but we don't need it much
        UINFO(9, "readFilterLine");
        string سٽ;
        طالما (!m_readEof) {
            StrList outl;
            read_بلاڪون(m_readFd, 1, outl);
            ثابت string onechar = listString(outl);
            سٽ += onechar;
            إذا (onechar == "\n") {
                إذا (سٽ == "\n") {
                    سٽ = "";
                    استمر;
                } وإلا {
                    اكسر;
                }
            }
        }
        UINFO(6, "filter-line-in: " << سٽ);
        أرجع سٽ;
    }
    // cppverilator-suppress unusedFunction unusedPrivateFunction
    فراغ writeFilter(ثابت string& ٻاهريون) {
        إذا (debug() >= 6) {
            UINFO(6, "filter-out: " << ٻاهريون);
            إذا (ٻاهريون[ٻاهريون.ڊيگهائي() - 1] != '\n') ٻاهر << '\n';
        }
        إذا (!m_pid) {
            v3error("--pipe-filter: write to closed file\n");
            m_readEof = سچ;
            stop();
        }
        بنانشان وٿي = 0;
        طالما (!m_readEof && ٻاهريون.ڊيگهائي() > وٿي) {
            errno = 0;
            ثابت صحيح got = اكتب(m_writeFd, (ٻاهريون.c_str()) + وٿي, ٻاهريون.ڊيگهائي() - وٿي);
            // UINFO(9, "WR GOT g " << got << " e " << errno << " " << strerror(errno));
            // usleep(50*1000);
            إذا (got > 0) {
                وٿي += got;
            } وإلا إذا (errno == EINTR || errno == EAGAIN
#جيڪڏهن_وصف_ٿيل EWOULDBLOCK
                       || errno == EWOULDBLOCK
#جيڪڏهن_پورو
            ) {
                checkFilter(خطأ);
                V3Os::u_sleep(1000);
                استمر;
            } وإلا {
                اكسر;
            }
        }
    }

    // Start the filter
    فراغ شروعات(ثابت string& command) {
        إذا (command == "") {
            m_pid = 0;  // Disabled
        } وإلا {
            شروعات_Filter(command);
        }
    }
    فراغ شروعات_Filter(ثابت string& command) {
        (فراغ)command;  // Prevent Unused variable warning
#جيڪڏهن_وصف_ٿيل INFILTER_PIPE
        صحيح fd_stdin[2];  // Can't use std::array
        صحيح fd_stdout[2];  // Can't use std::array
        constexpr صحيح P_RD = 0;
        constexpr صحيح P_WR = 1;

        إذا (VL_UNCOVERABLE(أنبوب(fd_stdin) != 0 || أنبوب(fd_stdout) != 0))
            v3fatal("--pipe-filter: Can't pipe: " << std::strerror(errno));  // LCOV_EXCL_LINE
        إذا (fd_stdin[P_RD] <= 2 || fd_stdin[P_WR] <= 2 || fd_stdout[P_RD] <= 2
            || fd_stdout[P_WR] <= 2) {
            // We'd have to rearrange all of the FD usages in this case.
            // Too unlikely; verilator isn't a daemon.
            v3fatal("--pipe-filter: stdin/stdout closed before pipe opened\n");
        }

        UINFO(1, "--pipe-filter: /bin/sh -c " << command);

        ثابت pid_t pid = fork();
        إذا (VL_UNCOVERABLE(pid < 0))
            v3fatal("--pipe-filter: fork failed: " << std::strerror(errno));  // LCOV_EXCL_LINE
        إذا (pid == 0) {  //ٻار
            UINFO(6, "In child");
            close(fd_stdin[P_WR]);
            dup2(fd_stdin[P_RD], 0);
            close(fd_stdout[P_RD]);
            dup2(fd_stdout[P_WR], 1);
            // And stderr comes from parent

            execl("/bin/sh", "sh", "-c", command.c_str(), static_cast<حرف*>(خالي_اشارو));
            // Don't use v3fatal, we don't share the common structures any more
            fprintf(stderr, "--pipe-filter: exec failed: %s\n", std::strerror(errno));
            _exit(1);
        } وإلا {  //والد
            UINFO(6, "In parent, child pid " << pid << " stdin " << fd_stdin[P_WR] << "->"
                                             << fd_stdin[P_RD] << " stdout " << fd_stdout[P_WR]
                                             << "->" << fd_stdout[P_RD]);
            m_pid = pid;
            m_pidExited = خطأ;
            m_pid_حالت = 0;
            m_writeFd = fd_stdin[P_WR];
            m_readFd = fd_stdout[P_RD];
            m_readEof = خطأ;

            close(fd_stdin[P_RD]);
            close(fd_stdout[P_WR]);

            صحيح flags = fcntl(m_readFd, F_GETFL, 0);
            fcntl(m_readFd, F_SETFL, flags | O_NONBLOCK);

            flags = fcntl(m_writeFd, F_GETFL, 0);
            fcntl(m_writeFd, F_SETFL, flags | O_NONBLOCK);
        }
        UINFO(6, "startFilter complete");
#وإلا
        v3fatalSrc("--pipe-filter not implemented on this platform");
#جيڪڏهن_پورو
    }

    فراغ stop() {
        إذا (m_pid) stopFilter();
    }
    فراغ stopFilter() {
        UINFO(6, "Stopping filter process");
#جيڪڏهن_وصف_ٿيل INFILTER_PIPE
        close(m_writeFd);
        checkFilter(سچ);
        إذا (!WIFEXITED(m_pid_حالت) || WEXITSTATUS(m_pid_حالت) != 0) {
            v3fatal("--pipe-filter returned bad status");
        }
        m_pid = 0;
        close(m_readFd);
        UINFO(6, "Closed");
#وإلا
        v3fatalSrc("--pipe-filter not implemented on this platform");
#جيڪڏهن_پورو
    }

محمي:
    دوست صنف VInFilter;
    // Read file contents and return it
    منطقي readWholefile(ثابت string& filename, StrList& outl) {
        ثابت خودڪار it = m_contentsMap.ڳوليو(filename);
        إذا (it != m_contentsMap.پڇاڙي()) {
            outl.ڌڪو_پٺ(it->ٻيو);
            أرجع سچ;
        }
        إذا (!readContents(filename, outl)) أرجع خطأ;
        إذا (list_سائيز(outl) < INFILTER_CACHE_MAX) {
            // Cache small files (only to save space)
            // It's quite common to `include "timescale" thousands of times
            // This isn't so important if it's just an open(), but filtering can be slow
            m_contentsMap.emplace(filename, listString(outl));
        }
        أرجع سچ;
    }
    ساكن سائيز_t list_سائيز(ثابت StrList& sl) {
        سائيز_t نتيجو = 0;
        لكل (ثابت string& i : sl) نتيجو += i.ڊيگهائي();
        أرجع نتيجو;
    }
    ساكن string listString(ثابت StrList& sl) {
        string نتيجو;
        لكل (ثابت string& i : sl) نتيجو += i;
        أرجع نتيجو;
    }
    // CONSTRUCTORS
    واضح VInFilterImp(ثابت string& command) { شروعات(command); }
    ~VInFilterImp() { stop(); }
};

//######################################################################
// VInFilter
// Just dispatch to the implementation

VInFilter::VInFilter(ثابت string& command) { m_impp = جديد VInFilterImp{command}; }
VInFilter::~VInFilter() {
    إذا (m_impp) VL_DO_CLEAR(ميٽيو m_impp, m_impp = خالي_اشارو);
}

منطقي VInFilter::readWholefile(ثابت string& filename, VInFilter::StrList& outl) {
    UASSERT(m_impp, "readWholefile on invalid filter");
    أرجع m_impp->readWholefile(filename, outl);
}

//######################################################################
// V3OutFormatter: A class for printing code with automatic indentation.

V3_ٻاهريون_Formatter::V3_ٻاهريون_Formatter(V3_ٻاهريون_Formatter::Language lang)
    : m_lang{lang} {
    m_بلاڪ_Indent = v3Global.opt.decoration() ? 4 : 1;
    m_comma_ويڪر = v3Global.opt.decoration() ? 50 : 150;
}

//----------------------------------------------------------------------

ساكن constexpr صحيح MAXSPACE = 80;  // After this indent, stop indenting more

// Table of spaces, so we don't have to alloc/free them all the time
ساكن ثابت std::array<std::string, MAXSPACE + 1> s_indentSpaces = []() {
    std::array<std::string, MAXSPACE + 1> جدول;
    لكل (صحيح i = 0; i <= MAXSPACE; ++i) جدول[i] = std::string(static_cast<سائيز_t>(i), ' ');
    أرجع جدول;
}();

ثابت std::string& V3_ٻاهريون_Formatter::indentSpaces(صحيح انگ) {
    // Indent the specified number of spaces.
    أرجع s_indentSpaces[std::max(0, std::min(انگ, MAXSPACE))];
}

منطقي V3_ٻاهريون_Formatter::token_ميلاپ(ثابت حرف* cp, ثابت حرف* cmp) {
    طالما (*cmp && *cmp == *cp) {
        ++cp;
        ++cmp;
    }
    إذا (*cmp) أرجع خطأ;
    إذا (*cp && !std::خال_آهي(*cp)) أرجع خطأ;
    أرجع سچ;
}

منطقي V3_ٻاهريون_Formatter::token_شروعات(ثابت حرف* cp) {
    أرجع (token_ميلاپ(cp, "begin") || token_ميلاپ(cp, "case") || token_ميلاپ(cp, "casex")
            || token_ميلاپ(cp, "casez") || token_ميلاپ(cp, "class") || token_ميلاپ(cp, "function")
            || token_ميلاپ(cp, "interface") || token_ميلاپ(cp, "module") || token_ميلاپ(cp, "package")
            || token_ميلاپ(cp, "task"));
}

منطقي V3_ٻاهريون_Formatter::token_پڇاڙي(ثابت حرف* cp) {
    أرجع (token_ميلاپ(cp, "end") || token_ميلاپ(cp, "endcase") || token_ميلاپ(cp, "endclass")
            || token_ميلاپ(cp, "endfunction") || token_ميلاپ(cp, "endinterface")
            || token_ميلاپ(cp, "endmodule") || token_ميلاپ(cp, "endpackage")
            || token_ميلاپ(cp, "endtask"));
}

منطقي V3_ٻاهريون_Formatter::token_Not_شروعات(ثابت حرف* cp) {
    أرجع (token_ميلاپ(cp, "export") || token_ميلاپ(cp, "import"));
}

صحيح V3_ٻاهريون_Formatter::پڇاڙي_سطحون(ثابت حرف* strg) {
    صحيح سطحون = m_indentLevel;
    {
        ثابت حرف* cp = strg;
        طالما (std::خال_آهي(*cp)) ++cp;
        بدل (*cp) {
        حالة '\n':  // Newlines.. No need for whitespace before it
            أرجع 0;
        حالة '#':  // Preproc directive
            إذا (m_lang == LA_C) أرجع 0;
        }
        {
            // label/public/private:  Deindent by 2 spaces
            ثابت حرف* mp = cp;
            لكل (; std::اکر_يا_انگ(*mp); ++mp) {}
            إذا (mp[0] == ':' && mp[1] != ':') أرجع (سطحون - m_بلاڪ_Indent / 2);
        }
    }
    // We want "} else {" to be one level to the left of normal
    لكل (ثابت حرف* cp = strg; *cp; ++cp) {
        بدل (*cp) {
        حالة '}':
        حالة ')': سطحون -= m_بلاڪ_Indent; اكسر;
        حالة '<':
            إذا (m_lang == LA_XML) {
                إذا (cp[1] == '/') سطحون -= m_بلاڪ_Indent;
            }
            اكسر;
        حالة 'e':
            إذا (m_lang == LA_VERILOG && token_پڇاڙي(cp)) سطحون -= m_بلاڪ_Indent;
            اكسر;
        حالة '\t':
        حالة ' ': اكسر;  // Continue
        افتراضي: أرجع سطحون;  // Letter
        }
    }
    أرجع سطحون;
}

فراغ V3_ٻاهريون_Formatter::putns(ثابت Ast_ڳنڍ* nodep, ثابت حرف* strg) {
    إذا (!v3Global.opt.decoration()) {
        putsOutput(strg);
        أرجع;
    }

    ثابت منطقي put_ڳنڍ_Decoration =  //
        nodep  //
        && v3Global.opt.decorationNodes()  //
        && !v3Global.opt.protectIds()  //
        && (m_ذريعو_آخري_Filenameno != nodep->fileline()->filenameno()  //
            || m_ذريعو_آخري_Lineno != nodep->fileline()->پهريون_Lineno())  //
        && File_سٽ::built_اندريون_Filename() != nodep->fileline()->filename();

    إذا (m_prependIndent && ((strg[0] && strg[0] != '\n') || put_ڳنڍ_Decoration)) {
        putsNoTracking(indentSpaces(پڇاڙي_سطحون(strg)));
        m_prependIndent = خطأ;
    }

    إذا (put_ڳنڍ_Decoration) {
        ثابت File_سٽ* ثابت flp = nodep->fileline();
        m_ذريعو_آخري_Lineno = flp->پهريون_Lineno();
        m_ذريعو_آخري_Filenameno = flp->filenameno();
        ثابت std::string lineno = std::منزل_string(flp->lineno());
        putsNoTracking("/*" + flp->filename() + ":" + lineno + " " + cvt_منزل_Hex(nodep) + "*/");
    }

    منطقي notstart = خطأ;
    منطقي wordstart = سچ;
    منطقي equalsForBracket = خطأ;  // Looking for "= {"
    لكل (ثابت حرف* cp = strg; *cp; ++cp) {
        putcNoTracking(*cp);
        إذا (std::اکر_آهي(*cp)) {
            إذا (wordstart && m_lang == LA_VERILOG && token_Not_شروعات(cp)) notstart = سچ;
            إذا (wordstart && m_lang == LA_VERILOG && !notstart && token_شروعات(cp)) indentInc();
            إذا (wordstart && m_lang == LA_VERILOG && token_پڇاڙي(cp)) indentDec();
        }
        ثابت منطقي was_اندريون_Backslash = m_اندريون_Backslash;
        m_اندريون_Backslash = خطأ;
        بدل (*cp) {
        حالة '\n':
            ++m_lineno;
            wordstart = سچ;
            إذا (cp[1] == '\0') {
                // Add the indent later, may be a indentInc/indentDec
                // called between now and then
                m_prependIndent = سچ;
            } وإلا {
                m_prependIndent = خطأ;
                putsNoTracking(indentSpaces(پڇاڙي_سطحون(cp + 1)));
            }
            اكسر;
        حالة ' ': wordstart = سچ; اكسر;
        حالة '\t': wordstart = سچ; اكسر;
        حالة '"':
            wordstart = خطأ;
            إذا (!was_اندريون_Backslash) m_اندريون_String_Literal = !m_اندريون_String_Literal;
            اكسر;
        حالة '\\':
            wordstart = خطأ;
            إذا (!was_اندريون_Backslash) m_اندريون_Backslash = سچ;
            اكسر;
        حالة '/':
            إذا (m_lang == LA_C || m_lang == LA_VERILOG) {
                إذا (cp > strg && cp[-1] == '/' && !m_اندريون_String_Literal) {
                    // Output ignoring contents to EOL
                    ++cp;
                    طالما (*cp && cp[1] && cp[1] != '\n') putcNoTracking(*cp++);
                    إذا (*cp) putcNoTracking(*cp);
                }
            }
            اكسر;
        حالة '{':
            إذا (!m_اندريون_String_Literal) {
                إذا (m_lang == LA_C && (equalsForBracket || m_bracketLevel)) {
                    // Break up large code inside "= { ..."
                    m_parenVec.ڌڪو(m_indentLevel
                                    * m_بلاڪ_Indent);  // Line up continuation with block+1
                    ++m_bracketLevel;
                }
                indentInc();
            }
            اكسر;
        حالة '}':
            إذا (!m_اندريون_String_Literal) {
                إذا (m_bracketLevel > 0) {
                    m_parenVec.ڪڍو();
                    --m_bracketLevel;
                }
                indentDec();
            }
            اكسر;
        حالة '(':
            إذا (!m_اندريون_String_Literal) {
                indentInc();
                // Line up continuation with open paren, plus one indent
                m_parenVec.ڌڪو(m_ڪالم);
            }
            اكسر;
        حالة ')':
            إذا (!m_اندريون_String_Literal) {
                إذا (!m_parenVec.خالي_جايون()) m_parenVec.ڪڍو();
                indentDec();
            }
            اكسر;
        حالة '<':
            إذا (m_lang == LA_XML) {
                إذا (cp[1] == '/') {
                    // Zero as the > will result in net decrease by one
                } وإلا إذا (cp[1] == '!' || cp[1] == '?') {
                    indentInc();  // net same indent
                } وإلا {
                    indentInc();  // net increase by one
                    indentInc();
                }
            }
            اكسر;
        حالة '>':
            إذا (m_lang == LA_XML) {
                indentDec();
                إذا (cp > strg && cp[-1] == '/') indentDec();  // < ..... /> stays same level
            }
            اكسر;
        افتراضي: wordstart = خطأ; اكسر;
        }

        بدل (*cp) {
        حالة '=': equalsForBracket = سچ; اكسر;
        حالة ' ': اكسر;
        افتراضي: equalsForBracket = خطأ; اكسر;
        }
    }
}

فراغ V3_ٻاهريون_Formatter::put_Break_اظهار() {
    إذا (!m_parenVec.خالي_جايون()) putBreak();
}

// Add a line break if too wide
فراغ V3_ٻاهريون_Formatter::putBreak() {
    إذا (!v3Global.opt.decoration()) أرجع;
    إذا (!m_nobreak) {
        // char s[1000]; sprintf(s, "{%d,%d}", m_column, m_parenVec.top()); putsNoTracking(s);
        إذا (exceeded_ويڪر()) {
            putcNoTracking('\n');
            إذا (!m_parenVec.خالي_جايون()) putsNoTracking(indentSpaces(m_parenVec.چوٽي()));
        }
    }
}

فراغ V3_ٻاهريون_Formatter::putsQuoted(ثابت string& strg) {
    // Quote \ and " for use inside C programs
    // Don't use to quote a filename for #include - #include doesn't \ escape.
    ثابت string quoted = quoteNameControls(strg);
    putcNoTracking('"');
    لكل (ثابت حرف c : quoted) putcNoTracking(c);
    putcNoTracking('"');
    إذا (strg.ڳوليو('\0') != std::string::npos) putcNoTracking('s');  // C++14 std::string
}
فراغ V3_ٻاهريون_Formatter::putsNoTracking(ثابت string& strg) {
    إذا (!v3Global.opt.decoration()) {
        putsOutput(strg.c_str());
        أرجع;
    }
    // Don't track {}'s, probably because it's a $display format string
    لكل (ثابت حرف c : strg) putcNoTracking(c);
}

فراغ V3_ٻاهريون_Formatter::putcNoTracking(حرف chr) {
    إذا (!v3Global.opt.decoration()) {
        putcOutput(chr);
        أرجع;
    }
    بدل (chr) {
    حالة '\n':
        ++m_lineno;
        m_ڪالم = 0;
        m_nobreak = سچ;
        اكسر;
    حالة '\t': m_ڪالم = ((m_ڪالم + 9) / 8) * 8; اكسر;
    حالة ' ':
    حالة '(':
    حالة '|':
    حالة '&': ++m_ڪالم; اكسر;
    افتراضي:
        ++m_ڪالم;
        m_nobreak = خطأ;
        اكسر;
    }
    putcOutput(chr);
}

string V3_ٻاهريون_Formatter::quoteNameControls(ثابت string& namein,
                                         V3_ٻاهريون_Formatter::Language lang) VL_خالص {
    // Encode control chars into output-appropriate escapes
    // Reverse is VString::unquoteSVString
    string ٻاهريون;
    إذا (lang == LA_XML) {
        // Encode chars into XML string
        لكل (ثابت حرف c : namein) {
            إذا (c == '"') {
                ٻاهريون += "&quot;"s;
            } وإلا إذا (c == '\'') {
                ٻاهريون += "&apos;"s;
            } وإلا إذا (c == '<') {
                ٻاهريون += "&lt;"s;
            } وإلا إذا (c == '>') {
                ٻاهريون += "&gt;"s;
            } وإلا إذا (c == '&') {
                ٻاهريون += "&amp;"s;
            } وإلا إذا (std::isprint(c)) {
                ٻاهريون += c;
            } وإلا {
                ٻاهريون += "&#"s + cvt_منزل_Str(static_cast<بنانشان صحيح>(c & 0xff)) + ";";
            }
        }
    } وإلا {
        // Encode control chars into C style escapes
        لكل (ثابت حرف c : namein) {
            إذا (c == '\\' || c == '"') {
                ٻاهريون += "\\"s + c;
            } وإلا إذا (c == '\n') {
                ٻاهريون += "\\n";
            } وإلا إذا (c == '\r') {
                ٻاهريون += "\\r";
            } وإلا إذا (c == '\t') {
                ٻاهريون += "\\t";
            } وإلا إذا (std::isprint(c)) {
                ٻاهريون += c;
            } وإلا {
                // This will also cover \a etc
                ثابت string octal
                    = "\\"s + cvt_منزل_Str((c >> 6) & 3) + cvt_منزل_Str((c >> 3) & 7) + cvt_منزل_Str(c & 7);
                ٻاهريون += octal;
            }
        }
    }
    أرجع ٻاهريون;
}

//----------------------------------------------------------------------
// Simple wrappers

فراغ V3_ٻاهريون_Formatter::لکو(ثابت حرف* fmt...) {
    constexpr سائيز_t bufsize = 5000;
    حرف sbuff[bufsize];
    va_list ap;
    va_شروعات(ap, fmt);
    VL_VSNPRINTF(sbuff, bufsize, fmt, ap);
    va_پڇاڙي(ap);
    هذا->puts(sbuff);
}

//######################################################################
// V3OutFormatter: A class for printing to a file, with automatic indentation of C++ code.

V3_ٻاهريون_File::V3_ٻاهريون_File(ثابت string& filename, V3_ٻاهريون_Formatter::Language lang)
    : V3_ٻاهريون_Formatter{lang}
    , m_filename{filename}
    , m_bufferp{جديد std::array<حرف, WRITE_بفر_سائيز_بائيٽون>{}} {
    إذا ((m_fp = V3File::new_fopen_w(filename)) == خالي_اشارو) {
        v3fatal("Can't write file: " << filename);
    }
}

V3_ٻاهريون_File::~V3_ٻاهريون_File() {
    write_بلاڪ();

    إذا (m_fp) fclose(m_fp);
    m_fp = خالي_اشارو;
}

فراغ V3_ٻاهريون_File::putsForceIncs() {
    ثابت VStringList& forceIncs = v3Global.opt.forceIncs();
    لكل (ثابت string& i : forceIncs) puts("#include \"" + i + "\"\n");
}

فراغ V3_ٻاهريون_CFile::putsGuard() {
    UASSERT(!m_guard, "Already called putsGuard in emit file");
    m_guard = سچ;
    string var = VString::upcase("VERILATED_"s + V3Os::filenameNonDir(filename()) + "_");
    لكل (حرف& c : var) {
        إذا (!std::اکر_يا_انگ(c)) c = '_';
    }
    puts("\n#ifndef " + var + "\n");
    puts("#define " + var + "  // guard\n");
}

//######################################################################
// V3OutStream

V3_ٻاهريون_وهڪرو::V3_ٻاهريون_وهڪرو(std::ostream& ostream, V3_ٻاهريون_Formatter::Language lang)
    : V3_ٻاهريون_Formatter{lang}
    , m_ostream{ostream} {}

//######################################################################
// VIdProtect

صنف VIdProtectImp final {
    // MEMBERS
    V3Mutex m_mutex;  // Protects members
    std::map<ثابت std::string, std::string> m_nameMap;  // Map of old name into new name
    std::unordered_سيٽ<std::string> m_new_Id_سيٽ VL_GUARDED_BY(m_mutex);  // Which new names exist
محمي:
    // CONSTRUCTORS
    دوست صنف VIdProtect;
    ساكن VIdProtectImp& singleton() VL_MT_محفوظ {
        ساكن VIdProtectImp s;
        أرجع s;
    }

عام:
    VIdProtectImp() {
        passthru("this");
        passthru("TOP");
        passthru("vlInst");
        passthru("vlSelf");
        passthru("vlSymsp");
    }
    ~VIdProtectImp() = افتراضي;
    // METHODS
    string passthru(ثابت string& old) VL_MT_محفوظ_EXCLUDES(m_mutex) {
        إذا (!v3Global.opt.protectIds()) أرجع old;
        ثابت V3_تالو_Guard تالو_2{m_mutex};
        ثابت خودڪار it = m_nameMap.ڳوليو(old);
        إذا (it != m_nameMap.پڇاڙي()) {
            // No way to go back and correct the older crypt name
            UASSERT(old == it->ٻيو,
                    "Pass-thru request for '" + old + "' after already --protect-ids of it.");
        } وإلا {
            m_nameMap.emplace(old, old);
            m_new_Id_سيٽ.وجهو(old);
        }
        أرجع old;
    }
    string protectIf(ثابت string& old, منطقي doIt) VL_MT_محفوظ_EXCLUDES(m_mutex) {
        إذا (!v3Global.opt.protectIds() || old.خالي_جايون() || !doIt) أرجع old;
        ثابت V3_تالو_Guard تالو_2{m_mutex};
        ثابت خودڪار pair = m_nameMap.emplace(old, "");
        إذا (pair.ٻيو) {
            string ٻاهريون;
            إذا (v3Global.opt.debugProtect()) {
                // This lets us see the symbol being protected to debug cases
                // where e.g. the definition is protect() but reference is
                // missing a protect()
                ٻاهريون = "PS" + old;
            } وإلا {
                VHashSha512 digest{v3Global.opt.protect_ڪنجي_Defaulted()};
                digest.وجهو(old);
                // Add "PS" prefix (Protect Symbols) as cannot start symbol with number
                ٻاهريون = "PS" + digest.digestSymbol24();
                // See if we can shrink the digest symbol to something smaller
                لكل (سائيز_t len = 6; len < ٻاهريون.سائيز() - 3; len += 3) {
                    ثابت string tryout = ٻاهريون.substr(0, len);
                    إذا (m_new_Id_سيٽ.وجهو(tryout).ٻيو) {
                        ٻاهريون = tryout;
                        اكسر;
                    }
                }
            }
            pair.پهريون->ٻيو = ٻاهريون;
        }
        أرجع pair.پهريون->ٻيو;
    }
    string protect_لفظ_If(ثابت string& old, منطقي doIt) VL_MT_محفوظ {
        // Split at " " (for traces), "." (for scopes), "->", "(", "&", ")" (for self pointers)
        إذا (!(doIt && v3Global.opt.protectIds())) أرجع old;
        string ٻاهريون;
        string::سائيز_type شروعات = 0;
        // space, ., ->
        طالما (سچ) {
            // When C++11, use find_if and lambda
            string::سائيز_type pos = string::npos;
            string separator;
            trySep(old, شروعات, " ", pos /*ref*/, separator /*ref*/);
            trySep(old, شروعات, ".", pos /*ref*/, separator /*ref*/);
            trySep(old, شروعات, "->", pos /*ref*/, separator /*ref*/);
            trySep(old, شروعات, "(", pos /*ref*/, separator /*ref*/);
            trySep(old, شروعات, "&", pos /*ref*/, separator /*ref*/);
            trySep(old, شروعات, ")", pos /*ref*/, separator /*ref*/);
            إذا (pos == string::npos) اكسر;
            ٻاهريون += protectIf(old.substr(شروعات, pos - شروعات), سچ) + separator;
            شروعات = pos + separator.ڊيگهائي();
        }
        ٻاهريون += protectIf(old.substr(شروعات), سچ);
        أرجع ٻاهريون;
    }
    فراغ writeMapFile(ثابت string& filename) ثابت {
        V3_ٻاهريون_Xml_File جو{filename};
        جو.puts_سرو();
        جو.puts("<!-- DESCR"
                "IPTION: Verilator output: XML representation of netlist -->\n");
        جو.puts("<verilator_id_map>\n");
        {
            لكل (ثابت خودڪار& itr : m_nameMap) {
                جو.puts("<map from=\"" + itr.ٻيو + "\" to=\"" + itr.پهريون + "\"/>\n");
            }
        }
        جو.puts("</verilator_id_map>\n");
    }

خاص:
    فراغ trySep(ثابت string& old, string::سائيز_type شروعات, ثابت string& trySep,
                string::سائيز_type& posr, string& separatorr) VL_خالص {
        ثابت string::سائيز_type trypos = old.ڳوليو(trySep, شروعات);
        إذا (trypos != string::npos) {
            إذا (posr == string::npos || (posr > trypos)) {
                posr = trypos;
                separatorr = trySep;
            }
        }
    }
};

string VIdProtect::ifNoProtect(ثابت string& اندريون) VL_MT_محفوظ {
    أرجع v3Global.opt.protectIds() ? "" : اندريون;
}
string VIdProtect::protectIf(ثابت string& old, منطقي doIt) VL_MT_محفوظ {
    أرجع VIdProtectImp::singleton().protectIf(old, doIt);
}
string VIdProtect::protect_لفظ_If(ثابت string& old, منطقي doIt) VL_MT_محفوظ {
    أرجع VIdProtectImp::singleton().protect_لفظ_If(old, doIt);
}
فراغ VIdProtect::writeMapFile(ثابت string& filename) {
    VIdProtectImp::singleton().writeMapFile(filename);
}
