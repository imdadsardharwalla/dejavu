import subprocess
from pathlib import Path


ROOT_DIR = Path(__file__).parent.parent
OUT_ROOT_DIR = ROOT_DIR / 'out'


def out_dir(config:str) -> Path:
    return OUT_ROOT_DIR / config


def ensure_directory_exists(path: Path) -> None:
    if not path.is_dir():
        path.mkdir(parents=True)


def cmake_generate(config:str) -> None:
    out_dir_path = out_dir(config)
    ensure_directory_exists(out_dir_path)

    cmake_args = [
        'cmake',
        '-S', ROOT_DIR.as_posix(),
        '-B', out_dir_path.as_posix(),
        '-G', 'Ninja',
        f'-DCMAKE_BUILD_TYPE={config}'
    ]
    subprocess.run(cmake_args, check=True)


def cmake_build(config:str) -> None:
    cmake_generate(config)

    cmake_args = [
        'cmake',
        '--build', out_dir(config).as_posix(),
        '--parallel'
    ]
    subprocess.run(cmake_args, check=True)


if __name__ == '__main__':
    cmake_build(config='Release')
