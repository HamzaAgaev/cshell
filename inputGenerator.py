from random import randint
from sys import argv

numbers_count = 2 ** 25
min_int = -1 * 2 ** 30
max_int = 2 ** 30 - 1


def generate_random_numbers():
    numbers = []
    for i in range(numbers_count):
        numbers.append(randint(min_int, max_int))
    return numbers


if __name__ == "__main__":
    input_file_path = argv[1]  # "cmake-build-debug/benchmark-starter/input.txt"
    input_file = open(input_file_path, "w")
    input_file.write(f"{numbers_count}\n")
    input_file.write(" ".join(map(str, generate_random_numbers())))
