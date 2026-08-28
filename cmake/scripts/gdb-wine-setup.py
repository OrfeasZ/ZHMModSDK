import os

import gdb

_HERE = os.path.dirname(os.path.abspath(__file__))

# Load the gdbinit.py script which adds support for PE symbolication.
gdb.execute('source %s' % os.path.join(_HERE, 'gdbinit.py'))

# Ignore WINE traps.
gdb.execute('handle SIGUSR1 nostop noprint pass')
gdb.execute('handle SIGUSR2 nostop noprint pass')

# Load symbols on stop.
def _on_stop(event):
    try:
        if not gdb.selected_inferior().pid:
            return

        gdb.execute('load-symbol-files', to_string=True)
    except Exception as e:
        print('gdb-wine: %s' % e)

gdb.events.stop.connect(_on_stop)
