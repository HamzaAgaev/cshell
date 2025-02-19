from glob import glob
from sys import argv
from collections import Counter


def find_files_by_pattern(pattern):
    found_files = glob(pattern)
    if not found_files:
        raise FileNotFoundError(f"Файлы по шаблону '{pattern}' не найдены.")
    return found_files


def get_file_name(file_path):
    return file_path.split("/")[-1]


def get_benchmark_name(file_path):
    file_name = get_file_name(file_path)
    if file_name.startswith("b1-"):
        return "benchmark-1"
    elif file_name.startswith("b2-"):
        return "benchmark-2"
    return None


def check_is_sorted_file(input_file_, output_file_):
    input_file_.seek(0)
    output_file_.seek(0)
    if input_file_.readline() != output_file_.readline():
        return False
    input_numbers = list(map(int, input_file_.readline().split()))
    output_numbers = list(map(int, output_file_.readline().split()))
    return output_numbers == sorted(input_numbers)


def check_is_dedup_file(input_file_, output_file_):
    input_file_.seek(0)
    output_file_.seek(0)
    if input_file_.readline() != output_file_.readline():
        return False
    input_numbers = list(map(int, input_file_.readline().split()))
    output_numbers = list(map(int, output_file_.readline().split()))
    return Counter(output_numbers) == Counter(set(output_numbers))


if __name__ == "__main__":
    directory_pattern = argv[1]
    input_file_path = find_files_by_pattern(directory_pattern + "/input.txt")[0]
    output_file_paths = find_files_by_pattern(directory_pattern + "/*output.txt")
    input_file = open(input_file_path, "r")
    is_fail = False
    for output_file_path in output_file_paths:
        benchmark = get_benchmark_name(output_file_path)
        output_file = open(output_file_path, "r")
        is_success = True
        if benchmark == "benchmark-1":
            is_success = check_is_sorted_file(input_file, output_file)
        elif benchmark == "benchmark-2":
            is_success = check_is_dedup_file(input_file, output_file)
        else:
            raise Exception("Формат выходных файлов неправильный!")
        if not is_success:
            is_fail = True
            print(f"{benchmark} отрабатывает неправильно для файла {get_file_name(output_file_path)}.")
        output_file.close()
    input_file.close()
    if is_fail:
        raise Exception("Нагрузчики отрабатывают неправильно!")
    print("Done!")
