cmake_minimum_required(VERSION 2.8) # Проверка версии CMake.
									# Если версия установленой программы
									# старее указаной, произайдёт аварийный выход.

add_executable(bizzbuzz main.c)		# Создает исполняемый файл с именем bizzbuzz
									# из исходника main.c