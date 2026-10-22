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

#અન્તર્ભાવ "V3PchAstMT.h"

#અન્તર્ભાવ "V3File.h"

#અન્તર્ભાવ "V3Os.h"
#અન્તર્ભાવ "V3String.h"

#અન્તર્ભાવ <cerrno>
#અન્તર્ભાવ <cstdarg>
#અન્તર્ભાવ <fcntl.h>
#અન્તર્ભાવ <iomanip>
#અન્તર્ભાવ <map>
#અન્તર્ભાવ <memory>

#અન્તર્ભાવ <sys/stat.h>
#અન્તર્ભાવ <sys/types.h>

// clang-format off
#જો defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)) || defined(VL_CPPCHECK)
# define INFILTER_PIPE  // Allow pipe filtering.  Needs fork()
#endif

#ifdef HAVE_STAT_NSEC  // i.e. Linux 2.6, from configure
# define VL_STAT_CTIME_NSEC(stat) ((stat).st_ctim.tv_nsec)  // Nanoseconds
# define VL_STAT_MTIME_NSEC(stat) ((stat).st_mtim.tv_nsec)  // Nanoseconds
#નહીંતર
# define VL_STAT_CTIME_NSEC(stat) (0)
# define VL_STAT_MTIME_NSEC(stat) (0)
#endif

#ifdef INFILTER_PIPE
# include <sys/wait.h>
#endif

#જો defined(_WIN32) || defined(__MINGW32__)
# include <io.h>  // open, read, write, close
#endif
// clang-format on

VL_DEFINE_DEBUG_FUNCTIONS;

// If change this code, run a test with the below size set very small
// #define INFILTER_IPC_BUFSIZ 16
constexpr પૂર્ણાંક INFILTER_IPC_BUFSIZ = (64 * 1024);  // For debug, try this as a small number
constexpr પૂર્ણાંક INFILTER_CACHE_MAX = (64 * 1024);  // Maximum bytes to cache if same file read twice

constexpr off_t FILE_HASH_SIZE_MAX = 1 * 1024 * 1024;  // Maxium size file to hash

//######################################################################
// V3File Internal state

વર્ગ V3FileDependImp final {
    // TYPES
    વર્ગ DependFile final {
        // A single file
        અચલ તાર્કિક m_target;  // True if write, else read
        તાર્કિક m_exists = સાચું;  // File exists
        અચલ string m_filename;  // Filename
        સંરચના stat m_stat;  // Stat information
        VHashSha512 m_hash;  // SHA hash of file contents
    જાહેર:
        DependFile(અચલ string& filename, તાર્કિક target)
            : m_target{target}
            , m_filename{filename} {
            m_stat.st_ctime = 0;
            m_stat.st_mtime = 0;
        }
        ~DependFile() = મૂળભૂત;
        અચલ string& filename() અચલ { પાછા m_filename; }
        તાર્કિક target() અચલ { પાછા m_target; }
        તાર્કિક exists() અચલ { પાછા m_exists; }
        off_t size() અચલ { પાછા m_stat.st_size; }
        ino_t ino() અચલ { પાછા m_stat.st_ino; }
        time_t cstime() અચલ { પાછા m_stat.st_ctime; }  // Seconds
        time_t cnstime() અચલ { પાછા VL_STAT_CTIME_NSEC(m_stat); }  // Nanoseconds
        time_t mstime() અચલ { પાછા m_stat.st_mtime; }  // Seconds
        time_t mnstime() અચલ { પાછા VL_STAT_MTIME_NSEC(m_stat); }  // Nanoseconds
        string hash_Digest_પ્રતીક() {
            સ્થિર VHashSha512 s_emptyHash;
            પાછા m_hash.digestSymbol24() != s_emptyHash.digestSymbol24()
                       ? m_hash.digestSymbol24()
                       : "unhashed";
        }
        શૂન્ય loadStats() {
            જો (!m_stat.st_mtime) {
                અચલ string fn = filename();
                અચલ પૂર્ણાંક err = stat(fn.c_str(), &m_stat);
                જો (err != 0) {
                    std::memset(&m_stat, 0, કદ(m_stat));
                    m_stat.st_mtime = 1;
                    m_exists = ખોટું;
                    // Not an error... This can occur due to `line directives in the .vpp files
                    UINFO(1, "-Info: File not statable: " << filename());
                } નહીંતર {
                    // For speed, don't hash large files (e.g. verilator_bin)
                    જો (!target() && size() <= FILE_HASH_SIZE_MAX) m_hash.ઉમેરો_File(filename());
                }
            }
        }
        તાર્કિક operator<(અચલ DependFile& rhs) અચલ { પાછા filename() < rhs.filename(); }
    };

    // MEMBERS
    V3Mutex m_mutex;  // Protects members
    std::set<string> m_filenameSet VL_GUARDED_BY(m_mutex);  // Files generated (elim duplicates)
    std::set<DependFile> m_filenameList;  // Files sourced/generated

    સ્થિર string stripQuotes(અચલ string& in) {
        string pretty = in;
        string::size_type pos;
        જ્યારે ((pos = pretty.find('\"')) != string::npos) pretty.replace(pos, 1, "_");
        જ્યારે ((pos = pretty.find('\n')) != string::npos) pretty.replace(pos, 1, "_");
        પાછા pretty;
    }

જાહેર:
    // ACCESSOR METHODS
    શૂન્ય addSrcDepend(અચલ string& filename) VL_MT_SAFE_EXCLUDES(m_mutex) {
        અચલ V3LockGuard lock{m_mutex};
        અચલ auto itFoundPair = m_filenameSet.ઉમેરો(filename);
        જો (itFoundPair.second) {
            DependFile df{filename, ખોટું};
            df.loadStats();  // Get size now, in case changes during the run
            m_filenameList.ઉમેરો(df);
        }
    }
    શૂન્ય addTgtDepend(અચલ string& filename) VL_MT_SAFE_EXCLUDES(m_mutex) {
        અચલ V3LockGuard lock{m_mutex};
        અચલ auto itFoundPair = m_filenameSet.ઉમેરો(filename);
        જો (itFoundPair.second) m_filenameList.ઉમેરો(DependFile{filename, સાચું});
    }
    શૂન્ય writeDepend(અચલ string& filename);
    std::vector<string> getAllDeps() અચલ;
    શૂન્ય writeTimes(અચલ string& filename, અચલ string& cmdlineIn);
    તાર્કિક checkTimes(અચલ string& filename, અચલ string& cmdlineIn);
};

V3FileDependImp dependImp;  // Depend implementation class

//######################################################################
// V3FileDependImp

શૂન્ય V3FileDependImp::writeDepend(અચલ string& filename) {
    અચલ std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    જો (ofp->fail()) v3fatal("Can't write file: " << filename);

    માટે (અચલ DependFile& i : m_filenameList) {
        જો (i.target()) *ofp << i.filename() << " ";
    }
    *ofp << " : ";
    *ofp << v3Global.opt.buildDepBin();
    *ofp << " ";

    માટે (અચલ DependFile& i : m_filenameList) {
        જો (!i.target()) *ofp << i.filename() << " ";
    }

    *ofp << '\n';

    જો (v3Global.opt.makePhony()) {
        *ofp << '\n';
        માટે (અચલ DependFile& i : m_filenameList) {
            જો (!i.target()) *ofp << i.filename() << ":\n";
        }
    }
}

std::vector<string> V3FileDependImp::getAllDeps() અચલ {
    std::vector<string> r;
    માટે (અચલ auto& itr : m_filenameList) {
        જો (!itr.target() && itr.exists()) r.ધકેલો_back(itr.filename());
    }
    પાછા r;
}

શૂન્ય V3FileDependImp::writeTimes(અચલ string& filename, અચલ string& cmdlineIn) {
    અચલ std::unique_ptr<std::ofstream> ofp{V3File::new_ofstream(filename)};
    જો (ofp->fail()) v3fatal("Can't write file: " << filename);

    અચલ string cmdline = stripQuotes(cmdlineIn);
    *ofp << "# DESCR"
         << "IPTION: Verilator output: Timestamp data for --skip-identical.  Delete at will.\n";
    *ofp << "C \"" << cmdline << "\"\n";

    માટે (auto& it : m_filenameList) {
        DependFile* અચલ depfilep = const_cast<DependFile*>(&it);
        // Read stats of files we create after we're done making them
        // (except for this file, of course)
        V3Os::filesystemFlush(depfilep->filename());
        depfilep->loadStats();
        off_t showSize = depfilep->size();
        ino_t showIno = depfilep->ino();
        જો (depfilep->filename() == filename) {
            showSize = 0;
            showIno = 0;  // We're writing it, so need to ignore it
        }

        *ofp << (depfilep->target() ? "T" : "S") << " ";
        *ofp << " " << std::setw(8) << showSize;
        *ofp << " " << std::setw(8) << showIno;
        *ofp << " " << std::setw(11) << depfilep->cstime();
        *ofp << " " << std::setw(11) << depfilep->cnstime();
        *ofp << " " << std::setw(11) << depfilep->mstime();
        *ofp << " " << std::setw(11) << depfilep->mnstime();
        *ofp << " \"" << depfilep->hash_Digest_પ્રતીક() << "\"";
        *ofp << " \"" << depfilep->filename() << "\"";
        *ofp << '\n';
    }
}

તાર્કિક V3FileDependImp::checkTimes(અચલ string& filename, અચલ string& cmdlineIn) {
    અચલ std::unique_ptr<std::ifstream> ifp{V3File::new_ifstream_nodepend(filename)};
    જો (ifp->fail()) {
        UINFO(2, "   --check-times failed: no input " << filename);
        પાછા ખોટું;
    }
    {
        અચલ string ignore = V3Os::getline(*ifp);
        જો (ignore.empty()) { /*used*/
        }
    }
    {
        અક્ષર chkDir;
        *ifp >> chkDir;
        અક્ષર quote;
        *ifp >> quote;
        અચલ string chkCmdline = V3Os::getline(*ifp, '"');
        અચલ string cmdline = stripQuotes(cmdlineIn);
        જો (cmdline != chkCmdline) {
            UINFO(2, "   --check-times failed: different command line");
            પાછા ખોટું;
        }
    }

    અચલ તાર્કિક skipHashing = !V3Os::getenvStr("VERILATOR_DEBUG_SKIP_HASH", "").empty();

    જ્યારે (!ifp->eof()) {
        અક્ષર chkDir;
        *ifp >> chkDir;
        off_t chkSize;
        *ifp >> chkSize;
        ino_t chkIno;
        *ifp >> chkIno;
        જો (ifp->eof()) તોડો;  // Needed to read final whitespace before found eof
        time_t chkCstime;
        *ifp >> chkCstime;
        time_t chkCnstime;
        *ifp >> chkCnstime;
        time_t chkMstime;
        *ifp >> chkMstime;
        time_t chkMnstime;
        *ifp >> chkMnstime;
        અક્ષર quote;
        *ifp >> quote;
        અચલ string chkHash = V3Os::getline(*ifp, '"');
        *ifp >> quote;
        અચલ string chkFilename = V3Os::getline(*ifp, '"');

        V3Os::filesystemFlush(chkFilename);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
        સંરચના stat curStat;
        અચલ પૂર્ણાંક err = stat(chkFilename.c_str(), &curStat);
        જો (err != 0) {
            UINFO(2, "   --check-times failed: missing " << chkFilename);
            પાછા ખોટું;
        }

        // Skip the .dat file itself, as we were writing it at the time was being made
        જો (filename == chkFilename) ચાલુ;

        UINFO(9, " got d=" << chkDir << " s=" << chkSize << " ct=" << chkCstime << "."
                           << chkCnstime << " mt=" << chkMstime << "." << chkMnstime
                           << " h=" << chkHash << " fn= " << chkFilename);
        UINFO(9, " nowSt  s=" << curStat.st_size << " mt=" << curStat.st_mtime
                              << " ct=" << curStat.st_ctime);

        // We'd like this rule:
        // if (!(curStat.st_size == chkSize
        //      && curStat.st_mtime == chkMstime) {
        // However NFS messes us up, as there might be some data outstanding when
        // we determined the original size.  For safety, we know the creation time
        // must be within a few second window... call it 20 sec.
        જો (!(curStat.st_size >= chkSize && curStat.st_ino == chkIno
              && curStat.st_ctime == chkCstime && VL_STAT_CTIME_NSEC(curStat) == chkCnstime
              && curStat.st_mtime <= (chkMstime + 20)
              // Not comparing chkMnstime
              )) {
            UINFO(2, "   --check-times: out-of-date "
                         << chkFilename << "; " << curStat.st_size << "=?" << chkSize << " "
                         << curStat.st_ctime << "." << VL_STAT_CTIME_NSEC(curStat) << "=?"
                         << chkCstime << "." << chkCnstime << " " << curStat.st_mtime << "."
                         << VL_STAT_MTIME_NSEC(curStat) << "=?" << chkMstime << "." << chkMnstime);
            જો (skipHashing) પાછા ખોટું;
            // We didn't hash target output files, nor large files,
            // as unlikely to find a match and can be large
            જો (chkHash == "unhashed") પાછા ખોટું;

            VHashSha512 curHash;
            curHash.ઉમેરો_File(chkFilename);
            જો (curHash.digestSymbol24() != chkHash) {
                UINFO(2, "   --check-times: hash differs " << chkFilename << "; "
                                                           << curHash.digestSymbol24() << "=?"
                                                           << chkHash);
                પાછા ખોટું;
            }
        }
    }
    પાછા સાચું;
}

//######################################################################
// V3File

શૂન્ય V3File::addSrcDepend(અચલ string& filename) VL_MT_SAFE { dependImp.addSrcDepend(filename); }
શૂન્ય V3File::addTgtDepend(અચલ string& filename) VL_MT_SAFE { dependImp.addTgtDepend(filename); }
શૂન્ય V3File::writeDepend(અચલ string& filename) { dependImp.writeDepend(filename); }
std::vector<string> V3File::getAllDeps() { પાછા dependImp.getAllDeps(); }
શૂન્ય V3File::writeTimes(અચલ string& filename, અચલ string& cmdlineIn) {
    dependImp.writeTimes(filename, cmdlineIn);
}
તાર્કિક V3File::checkTimes(અચલ string& filename, અચલ string& cmdlineIn) {
    પાછા dependImp.checkTimes(filename, cmdlineIn);
}
શૂન્ય V3File::createMakeDirFor(અચલ string& filename) {
    જો (filename != VL_DEV_NULL
        // If doesn't start with makeDir then some output file user requested
        && filename.substr(0, v3Global.opt.makeDir().length() + 1)
               == v3Global.opt.makeDir() + "/") {
        createMakeDir();
    }
}
શૂન્ય V3File::createMakeDir() {
    સ્થિર તાર્કિક s_created = ખોટું;
    જો (!s_created) {
        s_created = સાચું;
        V3Os::createDir(v3Global.opt.makeDir());
        જો (v3Global.opt.hier_ટોચ()) V3Os::createDir(v3Global.opt.hier_ટોચ_Data_Dir());
    }
}

//######################################################################
// VInFilterImp

વર્ગ VInFilterImp final {
    using StrList = VInFilter::StrList;

    std::map<અચલ std::string, std::string> m_contentsMap;  // Cache of file contents
    તાર્કિક m_readEof = ખોટું;  // Received EOF on read
#ifdef INFILTER_PIPE
    pid_t m_pid = 0;  // fork() process id
#નહીંતર
    પૂર્ણાંક m_pid = 0;  // fork() process id - always zero as disabled
#endif
    તાર્કિક m_pidExited = ખોટું;  //  waitpid() reaped the filter process
    પૂર્ણાંક m_pidStatus = 0;  // Exit status of the filter process
    પૂર્ણાંક m_writeFd = 0;  // File descriptor TO filter
    પૂર્ણાંક m_readFd = 0;  // File descriptor FROM filter

ખાનગી:
    // METHODS

    તાર્કિક readContents(અચલ string& filename, StrList& outl) {
        જો (m_pid) {
            પાછા readContentsFilter(filename, outl);
        } નહીંતર {
            પાછા readContentsFile(filename, outl);
        }
    }
    તાર્કિક readContentsFile(અચલ string& filename, StrList& outl) {
        અચલ પૂર્ણાંક fd = open(filename.c_str(), O_RDONLY);
        જો (fd < 0) પાછા ખોટું;
        m_readEof = ખોટું;
        readBlocks(fd, -1, outl);
        close(fd);
        પાછા સાચું;
    }
    // cppcheck-suppress constParameterReference
    તાર્કિક readContentsFilter(અચલ string& filename, StrList& outl) {
        (શૂન્ય)filename;  // Prevent unused variable warning
        (શૂન્ય)outl;  // Prevent unused variable warning
#ifdef INFILTER_PIPE
        writeFilter("read \"" + filename + "\"\n");
        અચલ string લીટી = read_Filter_લીટી();
        જો (લીટી.find("Content-Length") != string::npos) {
            પૂર્ણાંક len = 0;
            sscanf(લીટી.c_str(), "Content-Length: %d\n", &len);
            readBlocks(m_readFd, len, outl);
            પાછા સાચું;
        } નહીંતર {
            જો (લીટી != "") v3error("--pipe-filter protocol error, unexpected: " << લીટી);
            પાછા ખોટું;
        }
#નહીંતર
        v3fatalSrc("--pipe-filter not implemented on this platform");
        પાછા ખોટું;
#endif
    }

    // cppcheck-suppress functionConst
    શૂન્ય checkFilter(તાર્કિક hang) {
#ifdef INFILTER_PIPE
        જો (!m_pidExited && waitpid(m_pid, &m_pidStatus, hang ? 0 : WNOHANG)) {
            UINFO(1, "--pipe-filter: Exited, status "
                         << m_pidStatus << " exit=" << WEXITSTATUS(m_pidStatus) << " err"
                         << std::strerror(errno));
            m_readEof = સાચું;
            m_pidExited = સાચું;
        }
#endif
    }

    શૂન્ય readBlocks(પૂર્ણાંક fd, પૂર્ણાંક size, StrList& outl) {
        અક્ષર buf[INFILTER_IPC_BUFSIZ];
        ssize_t sizegot = 0;
        જ્યારે (!m_readEof && (size < 0 || size > sizegot)) {
            ssize_t todo = INFILTER_IPC_BUFSIZ;
            જો (size > 0 && size < todo) todo = size;
            errno = 0;
            અચલ ssize_t got = પઠ(fd, buf, todo);
            // UINFO(9, "RD GOT g " << got << " e " << errno << " " << strerror(errno));
            // usleep(50*1000);
            જો (got > 0) {
                outl.emplace_back(string(buf, got));
                sizegot += got;
            } નહીંતર જો (errno == EINTR || errno == EAGAIN
#ifdef EWOULDBLOCK
                       || errno == EWOULDBLOCK
#endif
            ) {
                checkFilter(ખોટું);
                V3Os::u_sleep(1000);
                ચાલુ;
            } નહીંતર {
                m_readEof = સાચું;
                તોડો;
            }
        }
    }
    // cppverilator-suppress unusedFunction unusedPrivateFunction
    string read_Filter_લીટી() {
        // Slow, but we don't need it much
        UINFO(9, "readFilterLine");
        string લીટી;
        જ્યારે (!m_readEof) {
            StrList outl;
            readBlocks(m_readFd, 1, outl);
            અચલ string onechar = listString(outl);
            લીટી += onechar;
            જો (onechar == "\n") {
                જો (લીટી == "\n") {
                    લીટી = "";
                    ચાલુ;
                } નહીંતર {
                    તોડો;
                }
            }
        }
        UINFO(6, "filter-line-in: " << લીટી);
        પાછા લીટી;
    }
    // cppverilator-suppress unusedFunction unusedPrivateFunction
    શૂન્ય writeFilter(અચલ string& out) {
        જો (debug() >= 6) {
            UINFO(6, "filter-out: " << out);
            જો (out[out.length() - 1] != '\n') cout << '\n';
        }
        જો (!m_pid) {
            v3error("--pipe-filter: write to closed file\n");
            m_readEof = સાચું;
            stop();
        }
        unsigned offset = 0;
        જ્યારે (!m_readEof && out.length() > offset) {
            errno = 0;
            અચલ પૂર્ણાંક got = લિખ(m_writeFd, (out.c_str()) + offset, out.length() - offset);
            // UINFO(9, "WR GOT g " << got << " e " << errno << " " << strerror(errno));
            // usleep(50*1000);
            જો (got > 0) {
                offset += got;
            } નહીંતર જો (errno == EINTR || errno == EAGAIN
#ifdef EWOULDBLOCK
                       || errno == EWOULDBLOCK
#endif
            ) {
                checkFilter(ખોટું);
                V3Os::u_sleep(1000);
                ચાલુ;
            } નહીંતર {
                તોડો;
            }
        }
    }

    // Start the filter
    શૂન્ય start(અચલ string& command) {
        જો (command == "") {
            m_pid = 0;  // Disabled
        } નહીંતર {
            startFilter(command);
        }
    }
    શૂન્ય startFilter(અચલ string& command) {
        (શૂન્ય)command;  // Prevent Unused variable warning
#ifdef INFILTER_PIPE
        પૂર્ણાંક fd_stdin[2];  // Can't use std::array
        પૂર્ણાંક fd_stdout[2];  // Can't use std::array
        constexpr પૂર્ણાંક P_RD = 0;
        constexpr પૂર્ણાંક P_WR = 1;

        જો (VL_UNCOVERABLE(પ્રણાલ(fd_stdin) != 0 || પ્રણાલ(fd_stdout) != 0))
            v3fatal("--pipe-filter: Can't pipe: " << std::strerror(errno));  // LCOV_EXCL_LINE
        જો (fd_stdin[P_RD] <= 2 || fd_stdin[P_WR] <= 2 || fd_stdout[P_RD] <= 2
            || fd_stdout[P_WR] <= 2) {
            // We'd have to rearrange all of the FD usages in this case.
            // Too unlikely; verilator isn't a daemon.
            v3fatal("--pipe-filter: stdin/stdout closed before pipe opened\n");
        }

        UINFO(1, "--pipe-filter: /bin/sh -c " << command);

        અચલ pid_t pid = fork();
        જો (VL_UNCOVERABLE(pid < 0))
            v3fatal("--pipe-filter: fork failed: " << std::strerror(errno));  // LCOV_EXCL_LINE
        જો (pid == 0) {  // Child
            UINFO(6, "In child");
            close(fd_stdin[P_WR]);
            dup2(fd_stdin[P_RD], 0);
            close(fd_stdout[P_RD]);
            dup2(fd_stdout[P_WR], 1);
            // And stderr comes from parent

            execl("/bin/sh", "sh", "-c", command.c_str(), static_cast<અક્ષર*>(nullptr));
            // Don't use v3fatal, we don't share the common structures any more
            fprintf(stderr, "--pipe-filter: exec failed: %s\n", std::strerror(errno));
            _exit(1);
        } નહીંતર {  // Parent
            UINFO(6, "In parent, child pid " << pid << " stdin " << fd_stdin[P_WR] << "->"
                                             << fd_stdin[P_RD] << " stdout " << fd_stdout[P_WR]
                                             << "->" << fd_stdout[P_RD]);
            m_pid = pid;
            m_pidExited = ખોટું;
            m_pidStatus = 0;
            m_writeFd = fd_stdin[P_WR];
            m_readFd = fd_stdout[P_RD];
            m_readEof = ખોટું;

            close(fd_stdin[P_RD]);
            close(fd_stdout[P_WR]);

            પૂર્ણાંક flags = fcntl(m_readFd, F_GETFL, 0);
            fcntl(m_readFd, F_SETFL, flags | O_NONBLOCK);

            flags = fcntl(m_writeFd, F_GETFL, 0);
            fcntl(m_writeFd, F_SETFL, flags | O_NONBLOCK);
        }
        UINFO(6, "startFilter complete");
#નહીંતર
        v3fatalSrc("--pipe-filter not implemented on this platform");
#endif
    }

    શૂન્ય stop() {
        જો (m_pid) stopFilter();
    }
    શૂન્ય stopFilter() {
        UINFO(6, "Stopping filter process");
#ifdef INFILTER_PIPE
        close(m_writeFd);
        checkFilter(સાચું);
        જો (!WIFEXITED(m_pidStatus) || WEXITSTATUS(m_pidStatus) != 0) {
            v3fatal("--pipe-filter returned bad status");
        }
        m_pid = 0;
        close(m_readFd);
        UINFO(6, "Closed");
#નહીંતર
        v3fatalSrc("--pipe-filter not implemented on this platform");
#endif
    }

સુરક્ષિત:
    friend વર્ગ VInFilter;
    // Read file contents and return it
    તાર્કિક readWholefile(અચલ string& filename, StrList& outl) {
        અચલ auto it = m_contentsMap.find(filename);
        જો (it != m_contentsMap.end()) {
            outl.ધકેલો_back(it->second);
            પાછા સાચું;
        }
        જો (!readContents(filename, outl)) પાછા ખોટું;
        જો (listSize(outl) < INFILTER_CACHE_MAX) {
            // Cache small files (only to save space)
            // It's quite common to `include "timescale" thousands of times
            // This isn't so important if it's just an open(), but filtering can be slow
            m_contentsMap.emplace(filename, listString(outl));
        }
        પાછા સાચું;
    }
    સ્થિર size_t listSize(અચલ StrList& sl) {
        size_t result = 0;
        માટે (અચલ string& i : sl) result += i.length();
        પાછા result;
    }
    સ્થિર string listString(અચલ StrList& sl) {
        string result;
        માટે (અચલ string& i : sl) result += i;
        પાછા result;
    }
    // CONSTRUCTORS
    explicit VInFilterImp(અચલ string& command) { start(command); }
    ~VInFilterImp() { stop(); }
};

//######################################################################
// VInFilter
// Just dispatch to the implementation

VInFilter::VInFilter(અચલ string& command) { m_impp = નવું VInFilterImp{command}; }
VInFilter::~VInFilter() {
    જો (m_impp) VL_DO_CLEAR(delete m_impp, m_impp = nullptr);
}

તાર્કિક VInFilter::readWholefile(અચલ string& filename, VInFilter::StrList& outl) {
    UASSERT(m_impp, "readWholefile on invalid filter");
    પાછા m_impp->readWholefile(filename, outl);
}

//######################################################################
// V3OutFormatter: A class for printing code with automatic indentation.

V3OutFormatter::V3OutFormatter(V3OutFormatter::Language lang)
    : m_lang{lang} {
    m_blockIndent = v3Global.opt.decoration() ? 4 : 1;
    m_commaWidth = v3Global.opt.decoration() ? 50 : 150;
}

//----------------------------------------------------------------------

સ્થિર constexpr પૂર્ણાંક MAXSPACE = 80;  // After this indent, stop indenting more

// Table of spaces, so we don't have to alloc/free them all the time
સ્થિર અચલ std::array<std::string, MAXSPACE + 1> s_indentSpaces = []() {
    std::array<std::string, MAXSPACE + 1> table;
    માટે (પૂર્ણાંક i = 0; i <= MAXSPACE; ++i) table[i] = std::string(static_cast<size_t>(i), ' ');
    પાછા table;
}();

અચલ std::string& V3OutFormatter::indentSpaces(પૂર્ણાંક અંક) {
    // Indent the specified number of spaces.
    પાછા s_indentSpaces[std::max(0, std::min(અંક, MAXSPACE))];
}

તાર્કિક V3OutFormatter::tokenMatch(અચલ અક્ષર* cp, અચલ અક્ષર* cmp) {
    જ્યારે (*cmp && *cmp == *cp) {
        ++cp;
        ++cmp;
    }
    જો (*cmp) પાછા ખોટું;
    જો (*cp && !std::isspace(*cp)) પાછા ખોટું;
    પાછા સાચું;
}

તાર્કિક V3OutFormatter::tokenStart(અચલ અક્ષર* cp) {
    પાછા (tokenMatch(cp, "begin") || tokenMatch(cp, "case") || tokenMatch(cp, "casex")
            || tokenMatch(cp, "casez") || tokenMatch(cp, "class") || tokenMatch(cp, "function")
            || tokenMatch(cp, "interface") || tokenMatch(cp, "module") || tokenMatch(cp, "package")
            || tokenMatch(cp, "task"));
}

તાર્કિક V3OutFormatter::tokenEnd(અચલ અક્ષર* cp) {
    પાછા (tokenMatch(cp, "end") || tokenMatch(cp, "endcase") || tokenMatch(cp, "endclass")
            || tokenMatch(cp, "endfunction") || tokenMatch(cp, "endinterface")
            || tokenMatch(cp, "endmodule") || tokenMatch(cp, "endpackage")
            || tokenMatch(cp, "endtask"));
}

તાર્કિક V3OutFormatter::tokenNotStart(અચલ અક્ષર* cp) {
    પાછા (tokenMatch(cp, "export") || tokenMatch(cp, "import"));
}

પૂર્ણાંક V3OutFormatter::endLevels(અચલ અક્ષર* strg) {
    પૂર્ણાંક levels = m_indentLevel;
    {
        અચલ અક્ષર* cp = strg;
        જ્યારે (std::isspace(*cp)) ++cp;
        પસંદગી (*cp) {
        સ્થિતિ '\n':  // Newlines.. No need for whitespace before it
            પાછા 0;
        સ્થિતિ '#':  // Preproc directive
            જો (m_lang == LA_C) પાછા 0;
        }
        {
            // label/public/private:  Deindent by 2 spaces
            અચલ અક્ષર* mp = cp;
            માટે (; std::isalnum(*mp); ++mp) {}
            જો (mp[0] == ':' && mp[1] != ':') પાછા (levels - m_blockIndent / 2);
        }
    }
    // We want "} else {" to be one level to the left of normal
    માટે (અચલ અક્ષર* cp = strg; *cp; ++cp) {
        પસંદગી (*cp) {
        સ્થિતિ '}':
        સ્થિતિ ')': levels -= m_blockIndent; તોડો;
        સ્થિતિ '<':
            જો (m_lang == LA_XML) {
                જો (cp[1] == '/') levels -= m_blockIndent;
            }
            તોડો;
        સ્થિતિ 'e':
            જો (m_lang == LA_VERILOG && tokenEnd(cp)) levels -= m_blockIndent;
            તોડો;
        સ્થિતિ '\t':
        સ્થિતિ ' ': તોડો;  // Continue
        મૂળભૂત: પાછા levels;  // Letter
        }
    }
    પાછા levels;
}

શૂન્ય V3OutFormatter::putns(અચલ Ast_ગાંઠ* nodep, અચલ અક્ષર* strg) {
    જો (!v3Global.opt.decoration()) {
        putsOutput(strg);
        પાછા;
    }

    અચલ તાર્કિક put_ગાંઠ_Decoration =  //
        nodep  //
        && v3Global.opt.decorationNodes()  //
        && !v3Global.opt.protectIds()  //
        && (m_sourceLastFilenameno != nodep->fileline()->filenameno()  //
            || m_sourceLastLineno != nodep->fileline()->firstLineno())  //
        && File_લીટી::builtInFilename() != nodep->fileline()->filename();

    જો (m_prependIndent && ((strg[0] && strg[0] != '\n') || put_ગાંઠ_Decoration)) {
        putsNoTracking(indentSpaces(endLevels(strg)));
        m_prependIndent = ખોટું;
    }

    જો (put_ગાંઠ_Decoration) {
        અચલ File_લીટી* અચલ flp = nodep->fileline();
        m_sourceLastLineno = flp->firstLineno();
        m_sourceLastFilenameno = flp->filenameno();
        અચલ std::string lineno = std::to_string(flp->lineno());
        putsNoTracking("/*" + flp->filename() + ":" + lineno + " " + cvtToHex(nodep) + "*/");
    }

    તાર્કિક notstart = ખોટું;
    તાર્કિક wordstart = સાચું;
    તાર્કિક equalsForBracket = ખોટું;  // Looking for "= {"
    માટે (અચલ અક્ષર* cp = strg; *cp; ++cp) {
        putcNoTracking(*cp);
        જો (std::isalpha(*cp)) {
            જો (wordstart && m_lang == LA_VERILOG && tokenNotStart(cp)) notstart = સાચું;
            જો (wordstart && m_lang == LA_VERILOG && !notstart && tokenStart(cp)) indentInc();
            જો (wordstart && m_lang == LA_VERILOG && tokenEnd(cp)) indentDec();
        }
        અચલ તાર્કિક wasInBackslash = m_inBackslash;
        m_inBackslash = ખોટું;
        પસંદગી (*cp) {
        સ્થિતિ '\n':
            ++m_lineno;
            wordstart = સાચું;
            જો (cp[1] == '\0') {
                // Add the indent later, may be a indentInc/indentDec
                // called between now and then
                m_prependIndent = સાચું;
            } નહીંતર {
                m_prependIndent = ખોટું;
                putsNoTracking(indentSpaces(endLevels(cp + 1)));
            }
            તોડો;
        સ્થિતિ ' ': wordstart = સાચું; તોડો;
        સ્થિતિ '\t': wordstart = સાચું; તોડો;
        સ્થિતિ '"':
            wordstart = ખોટું;
            જો (!wasInBackslash) m_inStringLiteral = !m_inStringLiteral;
            તોડો;
        સ્થિતિ '\\':
            wordstart = ખોટું;
            જો (!wasInBackslash) m_inBackslash = સાચું;
            તોડો;
        સ્થિતિ '/':
            જો (m_lang == LA_C || m_lang == LA_VERILOG) {
                જો (cp > strg && cp[-1] == '/' && !m_inStringLiteral) {
                    // Output ignoring contents to EOL
                    ++cp;
                    જ્યારે (*cp && cp[1] && cp[1] != '\n') putcNoTracking(*cp++);
                    જો (*cp) putcNoTracking(*cp);
                }
            }
            તોડો;
        સ્થિતિ '{':
            જો (!m_inStringLiteral) {
                જો (m_lang == LA_C && (equalsForBracket || m_bracketLevel)) {
                    // Break up large code inside "= { ..."
                    m_parenVec.ધકેલો(m_indentLevel
                                    * m_blockIndent);  // Line up continuation with block+1
                    ++m_bracketLevel;
                }
                indentInc();
            }
            તોડો;
        સ્થિતિ '}':
            જો (!m_inStringLiteral) {
                જો (m_bracketLevel > 0) {
                    m_parenVec.કાઢો();
                    --m_bracketLevel;
                }
                indentDec();
            }
            તોડો;
        સ્થિતિ '(':
            જો (!m_inStringLiteral) {
                indentInc();
                // Line up continuation with open paren, plus one indent
                m_parenVec.ધકેલો(m_column);
            }
            તોડો;
        સ્થિતિ ')':
            જો (!m_inStringLiteral) {
                જો (!m_parenVec.empty()) m_parenVec.કાઢો();
                indentDec();
            }
            તોડો;
        સ્થિતિ '<':
            જો (m_lang == LA_XML) {
                જો (cp[1] == '/') {
                    // Zero as the > will result in net decrease by one
                } નહીંતર જો (cp[1] == '!' || cp[1] == '?') {
                    indentInc();  // net same indent
                } નહીંતર {
                    indentInc();  // net increase by one
                    indentInc();
                }
            }
            તોડો;
        સ્થિતિ '>':
            જો (m_lang == LA_XML) {
                indentDec();
                જો (cp > strg && cp[-1] == '/') indentDec();  // < ..... /> stays same level
            }
            તોડો;
        મૂળભૂત: wordstart = ખોટું; તોડો;
        }

        પસંદગી (*cp) {
        સ્થિતિ '=': equalsForBracket = સાચું; તોડો;
        સ્થિતિ ' ': તોડો;
        મૂળભૂત: equalsForBracket = ખોટું; તોડો;
        }
    }
}

શૂન્ય V3OutFormatter::put_Break_પદાવલિ() {
    જો (!m_parenVec.empty()) putBreak();
}

// Add a line break if too wide
શૂન્ય V3OutFormatter::putBreak() {
    જો (!v3Global.opt.decoration()) પાછા;
    જો (!m_nobreak) {
        // char s[1000]; sprintf(s, "{%d,%d}", m_column, m_parenVec.top()); putsNoTracking(s);
        જો (exceededWidth()) {
            putcNoTracking('\n');
            જો (!m_parenVec.empty()) putsNoTracking(indentSpaces(m_parenVec.ટોચ()));
        }
    }
}

શૂન્ય V3OutFormatter::putsQuoted(અચલ string& strg) {
    // Quote \ and " for use inside C programs
    // Don't use to quote a filename for #include - #include doesn't \ escape.
    અચલ string quoted = quoteNameControls(strg);
    putcNoTracking('"');
    માટે (અચલ અક્ષર c : quoted) putcNoTracking(c);
    putcNoTracking('"');
    જો (strg.find('\0') != std::string::npos) putcNoTracking('s');  // C++14 std::string
}
શૂન્ય V3OutFormatter::putsNoTracking(અચલ string& strg) {
    જો (!v3Global.opt.decoration()) {
        putsOutput(strg.c_str());
        પાછા;
    }
    // Don't track {}'s, probably because it's a $display format string
    માટે (અચલ અક્ષર c : strg) putcNoTracking(c);
}

શૂન્ય V3OutFormatter::putcNoTracking(અક્ષર chr) {
    જો (!v3Global.opt.decoration()) {
        putcOutput(chr);
        પાછા;
    }
    પસંદગી (chr) {
    સ્થિતિ '\n':
        ++m_lineno;
        m_column = 0;
        m_nobreak = સાચું;
        તોડો;
    સ્થિતિ '\t': m_column = ((m_column + 9) / 8) * 8; તોડો;
    સ્થિતિ ' ':
    સ્થિતિ '(':
    સ્થિતિ '|':
    સ્થિતિ '&': ++m_column; તોડો;
    મૂળભૂત:
        ++m_column;
        m_nobreak = ખોટું;
        તોડો;
    }
    putcOutput(chr);
}

string V3OutFormatter::quoteNameControls(અચલ string& namein,
                                         V3OutFormatter::Language lang) VL_PURE {
    // Encode control chars into output-appropriate escapes
    // Reverse is VString::unquoteSVString
    string out;
    જો (lang == LA_XML) {
        // Encode chars into XML string
        માટે (અચલ અક્ષર c : namein) {
            જો (c == '"') {
                out += "&quot;"s;
            } નહીંતર જો (c == '\'') {
                out += "&apos;"s;
            } નહીંતર જો (c == '<') {
                out += "&lt;"s;
            } નહીંતર જો (c == '>') {
                out += "&gt;"s;
            } નહીંતર જો (c == '&') {
                out += "&amp;"s;
            } નહીંતર જો (std::isprint(c)) {
                out += c;
            } નહીંતર {
                out += "&#"s + cvtToStr(static_cast<unsigned પૂર્ણાંક>(c & 0xff)) + ";";
            }
        }
    } નહીંતર {
        // Encode control chars into C style escapes
        માટે (અચલ અક્ષર c : namein) {
            જો (c == '\\' || c == '"') {
                out += "\\"s + c;
            } નહીંતર જો (c == '\n') {
                out += "\\n";
            } નહીંતર જો (c == '\r') {
                out += "\\r";
            } નહીંતર જો (c == '\t') {
                out += "\\t";
            } નહીંતર જો (std::isprint(c)) {
                out += c;
            } નહીંતર {
                // This will also cover \a etc
                અચલ string octal
                    = "\\"s + cvtToStr((c >> 6) & 3) + cvtToStr((c >> 3) & 7) + cvtToStr(c & 7);
                out += octal;
            }
        }
    }
    પાછા out;
}

//----------------------------------------------------------------------
// Simple wrappers

શૂન્ય V3OutFormatter::printf(અચલ અક્ષર* fmt...) {
    constexpr size_t bufsize = 5000;
    અક્ષર sbuff[bufsize];
    va_list ap;
    va_start(ap, fmt);
    VL_VSNPRINTF(sbuff, bufsize, fmt, ap);
    va_end(ap);
    આ->puts(sbuff);
}

//######################################################################
// V3OutFormatter: A class for printing to a file, with automatic indentation of C++ code.

V3OutFile::V3OutFile(અચલ string& filename, V3OutFormatter::Language lang)
    : V3OutFormatter{lang}
    , m_filename{filename}
    , m_bufferp{નવું std::array<અક્ષર, WRITE_BUFFER_SIZE_BYTES>{}} {
    જો ((m_fp = V3File::new_fopen_w(filename)) == nullptr) {
        v3fatal("Can't write file: " << filename);
    }
}

V3OutFile::~V3OutFile() {
    writeBlock();

    જો (m_fp) fclose(m_fp);
    m_fp = nullptr;
}

શૂન્ય V3OutFile::putsForceIncs() {
    અચલ VStringList& forceIncs = v3Global.opt.forceIncs();
    માટે (અચલ string& i : forceIncs) puts("#include \"" + i + "\"\n");
}

શૂન્ય V3OutCFile::putsGuard() {
    UASSERT(!m_guard, "Already called putsGuard in emit file");
    m_guard = સાચું;
    string var = VString::upcase("VERILATED_"s + V3Os::filenameNonDir(filename()) + "_");
    માટે (અક્ષર& c : var) {
        જો (!std::isalnum(c)) c = '_';
    }
    puts("\n#ifndef " + var + "\n");
    puts("#define " + var + "  // guard\n");
}

//######################################################################
// V3OutStream

V3OutStream::V3OutStream(std::ostream& ostream, V3OutFormatter::Language lang)
    : V3OutFormatter{lang}
    , m_ostream{ostream} {}

//######################################################################
// VIdProtect

વર્ગ VIdProtectImp final {
    // MEMBERS
    V3Mutex m_mutex;  // Protects members
    std::map<અચલ std::string, std::string> m_nameMap;  // Map of old name into new name
    std::unordered_set<std::string> m_newIdSet VL_GUARDED_BY(m_mutex);  // Which new names exist
સુરક્ષિત:
    // CONSTRUCTORS
    friend વર્ગ VIdProtect;
    સ્થિર VIdProtectImp& singleton() VL_MT_SAFE {
        સ્થિર VIdProtectImp s;
        પાછા s;
    }

જાહેર:
    VIdProtectImp() {
        passthru("this");
        passthru("TOP");
        passthru("vlInst");
        passthru("vlSelf");
        passthru("vlSymsp");
    }
    ~VIdProtectImp() = મૂળભૂત;
    // METHODS
    string passthru(અચલ string& old) VL_MT_SAFE_EXCLUDES(m_mutex) {
        જો (!v3Global.opt.protectIds()) પાછા old;
        અચલ V3LockGuard lock{m_mutex};
        અચલ auto it = m_nameMap.find(old);
        જો (it != m_nameMap.end()) {
            // No way to go back and correct the older crypt name
            UASSERT(old == it->second,
                    "Pass-thru request for '" + old + "' after already --protect-ids of it.");
        } નહીંતર {
            m_nameMap.emplace(old, old);
            m_newIdSet.ઉમેરો(old);
        }
        પાછા old;
    }
    string protectIf(અચલ string& old, તાર્કિક doIt) VL_MT_SAFE_EXCLUDES(m_mutex) {
        જો (!v3Global.opt.protectIds() || old.empty() || !doIt) પાછા old;
        અચલ V3LockGuard lock{m_mutex};
        અચલ auto pair = m_nameMap.emplace(old, "");
        જો (pair.second) {
            string out;
            જો (v3Global.opt.debugProtect()) {
                // This lets us see the symbol being protected to debug cases
                // where e.g. the definition is protect() but reference is
                // missing a protect()
                out = "PS" + old;
            } નહીંતર {
                VHashSha512 digest{v3Global.opt.protect_ચાવી_Defaulted()};
                digest.ઉમેરો(old);
                // Add "PS" prefix (Protect Symbols) as cannot start symbol with number
                out = "PS" + digest.digestSymbol24();
                // See if we can shrink the digest symbol to something smaller
                માટે (size_t len = 6; len < out.size() - 3; len += 3) {
                    અચલ string tryout = out.substr(0, len);
                    જો (m_newIdSet.ઉમેરો(tryout).second) {
                        out = tryout;
                        તોડો;
                    }
                }
            }
            pair.first->second = out;
        }
        પાછા pair.first->second;
    }
    string protect_શબ્દો_If(અચલ string& old, તાર્કિક doIt) VL_MT_SAFE {
        // Split at " " (for traces), "." (for scopes), "->", "(", "&", ")" (for self pointers)
        જો (!(doIt && v3Global.opt.protectIds())) પાછા old;
        string out;
        string::size_type start = 0;
        // space, ., ->
        જ્યારે (સાચું) {
            // When C++11, use find_if and lambda
            string::size_type pos = string::npos;
            string separator;
            trySep(old, start, " ", pos /*ref*/, separator /*ref*/);
            trySep(old, start, ".", pos /*ref*/, separator /*ref*/);
            trySep(old, start, "->", pos /*ref*/, separator /*ref*/);
            trySep(old, start, "(", pos /*ref*/, separator /*ref*/);
            trySep(old, start, "&", pos /*ref*/, separator /*ref*/);
            trySep(old, start, ")", pos /*ref*/, separator /*ref*/);
            જો (pos == string::npos) તોડો;
            out += protectIf(old.substr(start, pos - start), સાચું) + separator;
            start = pos + separator.length();
        }
        out += protectIf(old.substr(start), સાચું);
        પાછા out;
    }
    શૂન્ય writeMapFile(અચલ string& filename) અચલ {
        V3OutXmlFile of{filename};
        of.putsHeader();
        of.puts("<!-- DESCR"
                "IPTION: Verilator output: XML representation of netlist -->\n");
        of.puts("<verilator_id_map>\n");
        {
            માટે (અચલ auto& itr : m_nameMap) {
                of.puts("<map from=\"" + itr.second + "\" to=\"" + itr.first + "\"/>\n");
            }
        }
        of.puts("</verilator_id_map>\n");
    }

ખાનગી:
    શૂન્ય trySep(અચલ string& old, string::size_type start, અચલ string& trySep,
                string::size_type& posr, string& separatorr) VL_PURE {
        અચલ string::size_type trypos = old.find(trySep, start);
        જો (trypos != string::npos) {
            જો (posr == string::npos || (posr > trypos)) {
                posr = trypos;
                separatorr = trySep;
            }
        }
    }
};

string VIdProtect::ifNoProtect(અચલ string& in) VL_MT_SAFE {
    પાછા v3Global.opt.protectIds() ? "" : in;
}
string VIdProtect::protectIf(અચલ string& old, તાર્કિક doIt) VL_MT_SAFE {
    પાછા VIdProtectImp::singleton().protectIf(old, doIt);
}
string VIdProtect::protect_શબ્દો_If(અચલ string& old, તાર્કિક doIt) VL_MT_SAFE {
    પાછા VIdProtectImp::singleton().protect_શબ્દો_If(old, doIt);
}
શૂન્ય VIdProtect::writeMapFile(અચલ string& filename) {
    VIdProtectImp::singleton().writeMapFile(filename);
}
