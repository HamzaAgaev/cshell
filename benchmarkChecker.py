from glob import glob
from sys import argv
from collections import Counter


def find_file_by_pattern(pattern):
    found_files = glob(pattern)
    if not found_files:
        raise FileNotFoundError(f"Файлы по шаблону '{pattern}' не найдены.")
    return found_files[0]


def check_is_sorted_file(input_file_, output_file_):
    if input_file_.readline() != output_file_.readline():
        return False
    input_numbers = list(map(int, input_file_.readline().split()))
    output_numbers = list(map(int, output_file_.readline().split()))
    return output_numbers == sorted(input_numbers)


def check_is_dedup_file(input_file_, output_file_):
    if input_file_.readline() != output_file_.readline():
        return False
    input_numbers = list(map(int, input_file_.readline().split()))
    output_numbers = list(map(int, output_file_.readline().split()))
    return Counter(output_numbers) == Counter(set(output_numbers))


if __name__ == "__main__":
    benchmark = argv[1]
    directory_pattern = argv[2]
    input_file = open(find_file_by_pattern(directory_pattern + "/input.txt"), "r")
    output_file = open(find_file_by_pattern(directory_pattern + "/*output.txt"), "r")
    is_success = False
    if benchmark == "benchmark-1":
        is_success = check_is_sorted_file(input_file, output_file)
    elif benchmark == "benchmark-2":
        is_success = check_is_dedup_file(input_file, output_file)
    if not is_success:
        raise Exception(f"{benchmark} отрабатывает неправильно.")
    print("Done!")
