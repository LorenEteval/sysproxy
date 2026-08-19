import subprocess
import sys
import threading
import time
from pathlib import Path

import sysproxy


def exercise_daemon_lifecycle():
    results = []

    thread = threading.Thread(
        target=lambda: results.append(sysproxy.daemon_on_()),
        name='sysproxy-sample-daemon',
    )
    thread.daemon = True

    thread.start()

    deadline = time.time() + 10

    while thread.is_alive() and time.time() < deadline:
        if not sysproxy.daemon_off():
            raise RuntimeError('sysproxy.daemon_off() failed')

        thread.join(0.05)

    if thread.is_alive():
        raise RuntimeError('sysproxy daemon did not stop')

    if results != [True]:
        raise RuntimeError('sysproxy.daemon_on_() failed: {!r}'.format(results))


def exercise_interpreter_finalization():
    sample = str(Path(__file__).resolve())
    command = [sys.executable, sample, '--finalization-child']

    for attempt in range(3):
        completed = subprocess.run(
            command,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            universal_newlines=True,
            timeout=30,
        )

        if completed.returncode:
            raise RuntimeError(
                'finalization child {} failed with exit code {}\nstdout:\n{}\nstderr:\n{}'.format(
                    attempt + 1,
                    completed.returncode,
                    completed.stdout,
                    completed.stderr,
                )
            )


if __name__ == '__main__' and '--finalization-child' in sys.argv:
    # Exercise capsule teardown in a real interpreter, where finalization bugs
    # in native module state cause a fatal process exit rather than an exception.
    if not sysproxy.daemon_off():
        raise RuntimeError('child sysproxy.daemon_off() failed')
elif __name__ == '__main__':
    print(f'sys.version: {sys.version}')
    print(f'call sysproxy.off: {sysproxy.off()}')

    exercise_daemon_lifecycle()
    exercise_interpreter_finalization()

    print('daemon lifecycle and interpreter finalization checks passed')
