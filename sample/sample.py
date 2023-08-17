import sys
import sysproxy


if __name__ == '__main__':
    print(f'sys.version: {sys.version}')
    print(f'call sysproxy.off: {sysproxy.off()}')
