from glob import glob
from sys import argv
from collections import Counter


def find_file_by_pattern(pattern):
    found_files = glob(pattern)
    if not found_files:
        raise FileNotFoundError(f"Файлы по шаблону '{pattern}' не найдены.")
    return found_files[0]


def check_is_sorted_file(file):
    numbers_count = file.readline()
    numbers = list(map(int, file.readline().split()))
    return numbers == sorted(numbers)


def check_is_dedup_file(file):
    numbers_count = file.readline()
    numbers = list(map(int, file.readline().split()))
    return Counter(numbers) == Counter(set(numbers))


if __name__ == "__main__":
    benchmark = argv[1]
    output_pattern = argv[2]
    outputFile = open(find_file_by_pattern(output_pattern), "r")
    is_success = False
    if benchmark == "benchmark-1":
        is_success = check_is_sorted_file(outputFile)
    elif benchmark == "benchmark-2":
        is_success = check_is_dedup_file(outputFile)
    if not is_success:
        raise Exception(f"{benchmark} отрабатывает неправильно.")
    print("Done!")
