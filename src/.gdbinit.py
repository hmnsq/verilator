# pylint: disable=line-too-long,invalid-name,multiple-statements,missing-function-docstring,missing-class-docstring,missing-module-docstring,no-else-return,too-few-public-methods,unused-argument
# DESCRIPTION: Verilator: GDB startup file with useful defines
# SPDX-FileCopyrightText: 2023 Wilson Snyder
# SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
######################################################################

आनयन tempfile
आनयन gdb  # pylint: disable=import-error


def _vltgdb_get_dump(गाठ):
    gdb.execute(f'set $_gdb_dump_json_str = AstNode::dumpTreeJsonGdb({node})')
    dump = gdb.execute('printf "%s", $_gdb_dump_json_str', to_string=सत्य)
    gdb.execute('call (void)free($_gdb_dump_json_str)')
    प्रत्यागम dump


def _vltgdb_tmpfile():
    प्रत्यागम tempfile.NamedTemporaryFile(mode="wt")  # write, text mode


def _vltgdb_fwrite(file, s):
    """Write to file and flush buffer before passing the file to astsee"""
    file.लिख(s)
    file.flush()


वर्ग AstseeCmd(gdb.Command):
    """Verilator: Pretty print or diff nodes using `astsee`. A node can be:
    * an pointer identifier,
    * an address literal,
    * a GDB value (like `$1`) that stores a dump previously done by the `jstash` command.
    Apart from not taking input from a file, it works exactly like `verilator_astsee`:
    * passing one node gives you a pretty print,
    * passing two nodes gives you a diff,
    * for more options see `astsee` readme/help.
    For examples see internals.rst
    """

    def __init__(self):
        पर().__init__("jtree", gdb.COMMAND_USER, gdb.COMPLETE_EXPRESSION)

    def _null_check(self, old, नूतन):
        err = ""
        यदि old == "<nullptr>\n":
            err += "old == <nullptr>\n"
        यदि नूतन == "<nullptr>\n":
            err += "new == <nullptr>"
        यदि err:
            उत्थापय gdb.GdbError(err.strip("\n"))

    def invoke(self, arg_str, from_tty):
        तः astsee आनयन verilator_cli इव astsee  # pylint: disable=import-error,import-outside-toplevel
        # we import it here so "no module X" error would occur on typing `jtree` rather than on every gdb start

        # We hack `astsee_verilator`'s arg parser to find arguments with nodes
        # After finding them, we replace them with proper files
        astsee_args = astsee.parser.parse_args(gdb.string_to_argv(arg_str))
        with _vltgdb_tmpfile() इव oldfile, _vltgdb_tmpfile() इव newfile, _vltgdb_tmpfile(
        ) इव metafile:
            यदि astsee_args.file:
                _vltgdb_fwrite(oldfile, _vltgdb_get_dump(astsee_args.file))
                astsee_args.file = oldfile.name
            यदि astsee_args.newfile:
                _vltgdb_fwrite(newfile, _vltgdb_get_dump(astsee_args.newfile))
                astsee_args.newfile = newfile.name
            यदि astsee_args.meta is None:
                # pass
                gdb.execute(f'call AstNode::dumpJsonMetaFileGdb("{metafile.name}")')
                astsee_args.meta = metafile.name
            यत्न:
                astsee.main(astsee_args)
            अपवाद SystemExit इव e:  # astsee prints nice errmsgs on exit(), so rethrow it as GdbError to suppress cryptic python trace
                उत्थापय gdb.GdbError("astsee exited with non-zero code") तः e


AstseeCmd()
