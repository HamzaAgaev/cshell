from os import remove
from glob import glob
from sys import argv


def delete_files_by_pattern(pattern):
    files_to_delete = glob(pattern)
    if not files_to_delete:
        raise FileNotFoundError(f"Файлы по шаблону '{pattern}' не найдены.")
    for file in files_to_delete:
        try:
            remove(file)
            print(f"Удалён файл: {file}.")
        except Exception as e:
            print(f"Не удалось удалить файл {file}: {e}.")


if __name__ == "__main__":
    input_pattern = argv[1]  # "cmake-build-debug/benchmark-starter/*output.txt"
    delete_files_by_pattern(input_pattern)
