import os

try:
    from gppylib.commands import gp
    from gppylib.commands.base import Command
    from gppylib.operations import Operation
except ImportError as ex:
    sys.exit(
        'Operation: Cannot import modules.  Please check that you have sourced greenplum_path.sh.  Detail: ' + str(ex))

def dereference_symlink(path):
    """
    MPP-15429: rpm is funky with symlinks...
    During an rpm -e invocation, rpm mucks with the /usr/local/greenplum-db symlink.
    From strace output, it appears that rpm tries to rmdir any directories it may have created during
    package installation. And, in the case of our GPHOME symlink, rpm will actually try to unlink it.
    To avoid this scenario, we perform all rpm actions against the "symlink dereferenced" $GPHOME.
    """
    path = os.path.normpath(path)
    if not os.path.islink(path):
        return path
    link = os.path.normpath(os.readlink(path))
    if os.path.isabs(link):
        return link
    return os.path.join(os.path.dirname(path), link)

GPHOME = dereference_symlink(gp.get_gphome()).rstrip('/')

class SyncPackages(Operation):
    """
    Synchronizes packages from coordinator to a remote host
    """

    def __init__(self, host):
        self.host = host

    def execute(self):
        sync_command = Command(name="Sync GPHOME",
            cmdStr='rsync --archive --checksum --quiet {}/ {}:{}'.format(GPHOME, self.host, GPHOME))

        sync_command.run()
