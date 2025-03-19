import subprocess
from pathlib import Path


ROOT_DIR = Path(__file__).parent.parent
OUT_DIR = ROOT_DIR / 'out'


def ensure_directory_exists(path: Path) -> None:
    if not path.is_dir():
        path.mkdir(parents=True)


def cmake_generate() -> None:
    ensure_directory_exists(OUT_DIR)

    cmake_args = [
        'cmake',
        '-S', ROOT_DIR,
        '-B', OUT_DIR,
        '-G', 'Visual Studio 17 2022',
        '-A', 'x64'
    ]
    subprocess.run(cmake_args, check=True)


def cmake_build(config:str='Release') -> None:
    ensure_directory_exists(OUT_DIR)
    cmake_generate()

    cmake_args = [
        'cmake',
        '--build', OUT_DIR,
        '--config', 'Release',
        '--parallel'
    ]
    subprocess.run(cmake_args, check=True)


if __name__ == '__main__':
    cmake_build()
