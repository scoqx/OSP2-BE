#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Скрипт для переворота changelog_be.txt по блокам версий.
Каждый блок начинается с даты и версии (DD.MM.YYYY be v X.XX)
"""

import re
import os

def reverse_changelog(input_file, output_file):
    """
    Читает changelog, разбивает на блоки по версиям и переворачивает порядок.
    """
    with open(input_file, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # Паттерн для начала блока версии: дата + "be v" + версия
    # Пример: "27.05.2025 be v 0.01" или "03.04.2025 be v0.02"
    version_pattern = re.compile(r'^\d{2}\.\d{2}\.\d{4}\s+be\s+v\s*', re.MULTILINE)
    
    # Находим все позиции начала блоков
    matches = list(version_pattern.finditer(content))
    
    if not matches:
        print("Не найдено блоков версий в файле!")
        return
    
    # Разбиваем на блоки
    blocks = []
    for i, match in enumerate(matches):
        start_pos = match.start()
        # Конец блока - начало следующего блока или конец файла
        if i + 1 < len(matches):
            end_pos = matches[i + 1].start()
        else:
            end_pos = len(content)
        
        block = content[start_pos:end_pos].rstrip()
        if block:
            blocks.append(block)
    
    # Переворачиваем порядок блоков
    blocks.reverse()
    
    # Объединяем блоки с двойным переводом строки между ними
    reversed_content = '\n\n'.join(blocks)
    
    # Сохраняем в новый файл
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write(reversed_content)
    
    print(f"Обработано блоков: {len(blocks)}")
    print(f"Результат сохранен в: {output_file}")

if __name__ == '__main__':
    # Путь к корневой директории проекта (на уровень выше scripts/)
    project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    
    # Путь к исходному файлу в корне проекта
    input_file = os.path.join(project_root, 'changelog_be.txt')
    
    # Путь к выходному файлу (добавляем _r перед расширением)
    base_name = os.path.splitext(input_file)[0]
    ext = os.path.splitext(input_file)[1]
    output_file = f"{base_name}_r{ext}"
    
    if not os.path.exists(input_file):
        print(f"Ошибка: файл {input_file} не найден!")
        exit(1)
    
    reverse_changelog(input_file, output_file)
