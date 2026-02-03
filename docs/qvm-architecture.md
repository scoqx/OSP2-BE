# QVM Architecture: Quake III Virtual Machine

Полное руководство по архитектуре QVM для разработчиков OSP2-BE. Документация основана на исследовании проекта q3vm-master и адаптирована для cgame/game/ui модулей.

---

## Содержание

1. [Введение](#введение)
2. [Архитектура VM](#архитектура-vm)
3. [Память и сегменты](#память-и-сегменты)
4. [Toolchain: компиляция C → QVM](#toolchain-компиляция-c--qvm)
5. [Набор инструкций (Opcodes)](#набор-инструкций-opcodes)
6. [Формат файла .qvm](#формат-файла-qvm)
7. [Системные вызовы (Syscalls)](#системные-вызовы-syscalls)
8. [Производительность](#производительность)
9. [Отладка QVM](#отладка-qvm)
10. [Референсы](#референсы)

---

## Введение

### Что такое QVM?

**Q3VM (Quake III Virtual Machine)** — это виртуальная машина, используемая движком Quake III Arena для безопасного выполнения игровых модулей (cgame, game, ui). VM создаёт **sandbox** (песочницу), изолируя код модулей от системы.

### Зачем нужна VM?

- **Безопасность**: код модуля не может повредить систему или получить доступ к системным ресурсам
- **Переносимость**: один `.qvm` файл работает на всех платформах (Windows, Linux, macOS)
- **Модульность**: модули загружаются/выгружаются динамически без пересборки движка
- **Защита от вирусов**: ограничение доступа к памяти и системным функциям

### Ключевые характеристики

- **Stack-based machine**: все операции используют стек, нет CPU-регистров
- **CISC-подобная архитектура**: опкоды разной длины (1 или 5 байт)
- **Interpreted OR JIT**: движок может интерпретировать байткод или JIT-компилировать в x86
- **Separate code/data spaces**: код и данные в разных адресных пространствах (нет self-modifying code)
- **Little-endian**: все многобайтовые значения в формате little-endian

---

## Архитектура VM

### Компоненты VM

```
┌─────────────────────────────────────────────┐
│              Host Application               │
│         (Quake III Engine / ioquake3)       │
├─────────────────────────────────────────────┤
│              VM Interpreter                 │
│  • Opcode dispatcher (switch/computed goto) │
│  • Stack management                         │
│  • Memory protection                        │
├─────────────────────────────────────────────┤
│             .qvm Bytecode File              │
│  • CODE segment (opcodes)                   │
│  • DATA segment (initialized 32-bit vars)   │
│  • LIT segment (strings)                    │
│  • BSS segment (uninitialized vars)         │
└─────────────────────────────────────────────┘
```

### Регистры VM

VM не имеет настоящих регистров CPU, но концептуально использует:

- **PC (Program Counter)**: указатель на текущую инструкцию
- **SP (Stack Pointer)**: указатель на вершину стека операндов
- **FP (Frame Pointer)**: указатель на начало стекового фрейма текущей функции
- **LP (Local Pointer)**: указатель на начало локальных переменных

### Стеки VM

VM использует два стека:

1. **Operation Stack (opstack)**: 1024 байта, для операндов и промежуточных результатов
2. **Program Stack**: 64 КБ (`VM_PROGRAM_STACK_SIZE = 0x10000`), для локальных переменных и аргументов функций

---

## Память и сегменты

### Адресное пространство

VM использует **32-битное плоское адресное пространство** с нулевым базовым адресом. Память разделена на сегменты:

```
┌──────────────────────────────┐ 0x00000000
│        CODE Segment          │  (Opcodes, read-only)
│  - Instructions              │
│  - Jump table                │
├──────────────────────────────┤
│        DATA Segment          │  (Initialized data, 32-bit values)
│  - Global variables          │
│  - Initialized arrays        │
├──────────────────────────────┤
│         LIT Segment          │  (String literals, 8-bit values)
│  - Constant strings          │
│  - Read-only byte data       │
├──────────────────────────────┤
│         BSS Segment          │  (Uninitialized data)
│  - Zero-initialized vars     │
│  - Static arrays             │
├──────────────────────────────┤
│      Program Stack           │  64 KB (grows down)
│  - Local variables           │
│  - Function arguments        │
│  - Return addresses          │
└──────────────────────────────┘ dataAlloc
```

### Сегменты в деталях

#### CODE Segment
- **Содержимое**: опкоды VM (инструкции)
- **Адресация**: по номеру инструкции (не по байтам!)
- **Особенность**: переменная длина инструкций (1 или 5 байт)
- **Доступ**: только на исполнение, read-only

#### DATA Segment
- **Содержимое**: инициализированные глобальные переменные и массивы
- **Формат**: последовательность 32-битных значений (little-endian)
- **Доступ**: read/write через LOAD4/STORE4

#### LIT Segment (Literal)
- **Содержимое**: строковые константы и byte-массивы
- **Формат**: последовательность байт (8-bit)
- **Доступ**: обычно read-only, используется для строк

#### BSS Segment (Block Started by Symbol)
- **Содержимое**: неинициализированные глобальные переменные
- **В файле**: не хранится! Только размер в заголовке
- **Инициализация**: VM может заполнить нулями при загрузке (опционально)
- **Максимум**: 10 МБ (`VM_MAX_BSS_LENGTH = 10485760`)

#### Program Stack
- **Размер**: 64 КБ фиксированно
- **Расположение**: в конце BSS сегмента
- **Направление**: растёт вниз (к меньшим адресам)
- **Содержимое**: локальные переменные, аргументы, return addresses

### Выравнивание памяти

- **DATA segment**: должен начинаться на границе 4 байт
- **LIT segment**: автоматически выровнен после DATA
- **Структуры**: выравниваются согласно их наибольшему полю

---

## Toolchain: компиляция C → QVM

### Обзор процесса

```
┌──────────┐     ┌──────────┐     ┌──────────┐     ┌──────────┐
│  file.c  │ --> │   LCC    │ --> │ file.asm │ --> │  q3asm   │ --> │ module.qvm │
└──────────┘     └──────────┘     └──────────┘     └──────────┘
   C код         Compiler       Stack-machine      Assembler      Bytecode
                                  assembly
```

### Шаг 1: LCC Compiler (C → Assembly)

**LCC** (Little C Compiler) — ретаргетируемый ANSI C компилятор.

**Команда компиляции:**
```bash
lcc -DQ3_VM -S -Wf-target=bytecode -Wf-g -o file.asm file.c
```

**Флаги:**
- `-DQ3_VM`: define макрос для conditional compilation
- `-S`: генерировать assembly (не объектный файл)
- `-Wf-target=bytecode`: целевая архитектура — Q3VM bytecode
- `-Wf-g`: включить debug-информацию (.map файл)

**Особенности LCC:**
- **Нет оптимизаций**: LCC НЕ делает inlining, loop unrolling, constant folding
- **Strict C89**: не поддерживает C99/C11 фичи
- **Stack machine output**: генерирует код для стековой машины

**Пример вывода LCC (.asm файл):**
```assembly
code

export vmMain

proc vmMain 12 0
file "main.c"
line 10
    ADDRFP4 0      ; Получить адрес аргумента 0
    INDIRI4        ; Загрузить значение
    CNSTI4 0       ; Константа 0
    EQI4 $42       ; Сравнить, перейти к $42 если равно
line 11
    ADDRGP4 $43    ; Адрес строки "Hello World"
    ARGI4          ; Передать аргумент
    ADDRGP4 printf ; Адрес функции printf
    CALLI4         ; Вызвать функцию
    pop            ; Выбросить возвращаемое значение
endproc vmMain 12 0
```

### Шаг 2: q3asm Assembler (Assembly → QVM)

**q3asm** — ассемблер/линкер, преобразует `.asm` файлы в `.qvm`.

**Команда:**
```bash
q3asm -f bytecode.q3asm
```

**Файл bytecode.q3asm** (linker script):
```
-o bytecode
main
bg_lib
; ... другие .asm файлы ...
```

**Что делает q3asm:**
1. Парсит все `.asm` файлы
2. Разрешает символы (символьная таблица)
3. Создаёт сегменты CODE, DATA, LIT, BSS
4. Генерирует jump table для инструкций
5. Записывает `.qvm` файл с заголовком
6. Создаёт `.map` файл с символами для отладки

**Важно:**
- q3asm **НЕ оптимизирует** код
- Порядок файлов в `.q3asm` важен: первый файл должен содержать `vmMain`
- `export` директивы делают функции видимыми для других модулей

### Шаг 3: Загрузка и исполнение QVM

Движок загружает `.qvm`:
1. Читает заголовок, проверяет magic number `0x12721444`
2. Выделяет память для сегментов
3. Загружает CODE, DATA, LIT в память
4. Выделяет BSS (может заполнить нулями)
5. Создаёт jump table для инструкций
6. Опционально: JIT-компилирует в x86 (или интерпретирует)
7. Вызывает `vmMain()`

---

## Набор инструкций (Opcodes)

### Категории инструкций

QVM имеет **60 опкодов** (0-59), сгруппированных по функциональности:

#### 1. Control Flow (Управление потоком)
- `OP_ENTER` (3): начало функции, создание стекового фрейма
- `OP_LEAVE` (4): конец функции, удаление фрейма
- `OP_CALL` (5): вызов функции
- `OP_JUMP` (10): безусловный переход
- `OP_EQ`, `OP_NE`, `OP_LTI`, `OP_LEI`, `OP_GTI`, `OP_GEI` (11-16): условные переходы (signed int)
- `OP_LTU`, `OP_LEU`, `OP_GTU`, `OP_GEU` (17-20): условные переходы (unsigned int)
- `OP_EQF`, `OP_NEF`, `OP_LTF`, `OP_LEF`, `OP_GTF`, `OP_GEF` (21-26): условные переходы (float)

#### 2. Stack Operations (Операции со стеком)
- `OP_PUSH` (6): push нулевое значение
- `OP_POP` (7): pop вершину стека
- `OP_CONST` (8): push константу (параметр 4 байта)
- `OP_LOCAL` (9): получить адрес локальной переменной

#### 3. Memory Access (Доступ к памяти)
- `OP_LOAD1` (27): загрузить 1 байт из памяти
- `OP_LOAD2` (28): загрузить 2 байта
- `OP_LOAD4` (29): загрузить 4 байта
- `OP_STORE1` (30): записать 1 байт
- `OP_STORE2` (31): записать 2 байта
- `OP_STORE4` (32): записать 4 байта
- `OP_BLOCK_COPY` (34): memcpy (блочное копирование)

#### 4. Arithmetic (Арифметика)
- `OP_ADD` (38): сложение (int)
- `OP_SUB` (39): вычитание (int)
- `OP_DIVI` (40): 
 (signed int)
- `OP_DIVU` (41): деление (unsigned int)
- `OP_MODI` (42): остаток (signed int)
- `OP_MODU` (43): остаток (unsigned int)
- `OP_MULI` (44): умножение (signed int)
- `OP_MULU` (45): умножение (unsigned int)
- `OP_NEGI` (37): отрицание (int)
- `OP_ADDF`, `OP_SUBF`, `OP_DIVF`, `OP_MULF`, `OP_NEGF` (54-57, 53): операции с float

#### 5. Bitwise Operations (Побитовые операции)
- `OP_BAND` (46): AND
- `OP_BOR` (47): OR
- `OP_BXOR` (48): XOR
- `OP_BCOM` (49): NOT (complement)
- `OP_LSH` (50): left shift
- `OP_RSHI` (51): right shift (arithmetic, signed)
- `OP_RSHU` (52): right shift (logical, unsigned)

#### 6. Type Conversion (Преобразование типов)
- `OP_SEX8` (35): sign-extend 8-bit → 32-bit
- `OP_SEX16` (36): sign-extend 16-bit → 32-bit
- `OP_CVIF` (58): convert int → float
- `OP_CVFI` (59): convert float → int

#### 7. Function Arguments (Аргументы функций)
- `OP_ARG` (33): маршалинг аргумента перед вызовом

#### 8. Debugging
- `OP_BREAK` (2): breakpoint (увеличивает `vm->breakCount`)
- `OP_IGNORE` (1): no-op
- `OP_UNDEF` (0): undefined (ошибка, VM halt)

### Формат инструкций

```
┌─────────────────────────────────────────────┐
│  Opcode without parameter (1 byte)         │
│  ┌──────┐                                   │
│  │ OP   │                                   │
│  └──────┘                                   │
│  Example: OP_ADD, OP_POP, OP_LOAD4         │
└─────────────────────────────────────────────┘

┌─────────────────────────────────────────────┐
│  Opcode with parameter (5 bytes)           │
│  ┌──────┬──────────────────────────────┐   │
│  │ OP   │  Parameter (4 bytes, LE)      │   │
│  └──────┴──────────────────────────────┘   │
│  Example: OP_CONST, OP_ENTER, OP_EQ        │
└─────────────────────────────────────────────┘
```

**Важно:**
- Все многобайтовые параметры в **little-endian**
- Jump адреса указываются **номером инструкции**, не байтовым offset'ом
- Опкоды с параметром: CONST, ENTER, LEAVE, LOCAL, EQ-GEF (условные переходы), ARG

### Примеры инструкций

#### Пример 1: Простое сложение
```c
int result = a + b;
```

**Компилируется в:**
```assembly
ADDRLP4 0      ; Адрес локальной переменной 'a'
INDIRI4        ; Загрузить значение 'a' на стек
ADDRLP4 4      ; Адрес локальной переменной 'b'
INDIRI4        ; Загрузить значение 'b' на стек
ADD            ; TOS = a + b
ASGNI4         ; Записать результат в 'result'
```

**Стек (визуализация):**
```
          ┌─────┐
          │  a  │  <- После INDIRI4
          └─────┘
          ┌─────┐
          │  b  │  <- После второго INDIRI4
          ├─────┤
          │  a  │
          └─────┘
          ┌─────┐
          │ a+b │  <- После ADD
          └─────┘
```

#### Пример 2: Условный переход
```c
if (x > 10) {
    foo();
}
```

**Компилируется в:**
```assembly
    ADDRLP4 0      ; Адрес 'x'
    INDIRI4        ; Загрузить 'x'
    CNSTI4 10      ; Константа 10
    LEI4 $skip     ; Если x <= 10, перейти к $skip
    ADDRGP4 foo    ; Адрес функции foo
    CALLI4         ; Вызвать foo()
    pop            ; Выбросить return value
LABELV $skip
    ; продолжение...
```

#### Пример 3: Вызов функции
```c
int result = myFunc(5, 10);
```

**Компилируется в:**
```assembly
    CNSTI4 5       ; Первый аргумент
    ARGI4          ; Маршалинг аргумента
    CNSTI4 10      ; Второй аргумент
    ARGI4          ; Маршалинг аргумента
    ADDRGP4 myFunc ; Адрес функции
    CALLI4         ; Вызвать функцию, результат на стеке
    ASGNI4         ; Сохранить результат в 'result'
```

---

## Формат файла .qvm

### Структура файла

```
┌────────────────────────────────────────────┐ Offset 0
│           QVM Header (32 bytes)            │
├────────────────────────────────────────────┤
│         CODE Segment (variable size)       │
│  - Opcodes                                 │
│  - Embedded parameters                     │
├────────────────────────────────────────────┤
│         DATA Segment (variable size)       │
│  - 32-bit values (little-endian)           │
├────────────────────────────────────────────┤
│         LIT Segment (variable size)        │
│  - String literals (8-bit bytes)           │
└────────────────────────────────────────────┘
         (BSS не хранится в файле)
```

### QVM Header (vmHeader_t)

```c
typedef struct {
    int32_t vmMagic;          // Magic number: 0x12721444 (LE)
    int32_t instructionCount; // Количество инструкций
    int32_t codeOffset;       // Offset CODE сегмента в файле
    int32_t codeLength;       // Размер CODE в байтах
    int32_t dataOffset;       // Offset DATA сегмента
    int32_t dataLength;       // Размер DATA в байтах
    int32_t litLength;        // Размер LIT сегмента (сразу после DATA)
    int32_t bssLength;        // Размер BSS (НЕ в файле!)
} vmHeader_t;
```

**Важно:**
- Все поля в **little-endian**
- Magic number для проверки валидности: `0x12721444` (LE) = `0x44147212` (BE)
- `instructionCount` ≠ `codeLength` (инструкции разной длины)
- `litLength`: LIT сегмент начинается сразу после DATA
- `bssLength`: BSS НЕ хранится в файле, только размер

### Пример бинарного заголовка

```
Offset  Value (hex)    Meaning
------  -----------    -------
0x00    44 14 72 12    vmMagic (0x12721444)
0x04    3A 02 00 00    instructionCount (570)
0x08    20 00 00 00    codeOffset (32 bytes, после header)
0x0C    B4 08 00 00    codeLength (2228 bytes)
0x10    D4 08 00 00    dataOffset (2260 bytes)
0x14    00 04 00 00    dataLength (1024 bytes)
0x18    3C 01 00 00    litLength (316 bytes)
0x1C    00 80 00 00    bssLength (32768 bytes)
```

### Загрузка QVM движком

```c
// Псевдокод загрузки
int VM_Create(vm_t* vm, const uint8_t* bytecode, int length) {
    vmHeader_t* header = (vmHeader_t*)bytecode;
    
    // 1. Проверка magic
    if (LittleLong(header->vmMagic) != VM_MAGIC) {
        return VM_FAILED_TO_LOAD_BYTECODE;
    }
    
    // 2. Выделение памяти для CODE
    vm->codeLength = LittleLong(header->codeLength);
    vm->codeBase = Com_malloc(vm->codeLength, vm, VM_ALLOC_CODE_SEC);
    memcpy(vm->codeBase, bytecode + header->codeOffset, vm->codeLength);
    
    // 3. Выделение памяти для DATA + LIT + BSS + Stack
    int dataLength = LittleLong(header->dataLength);
    int litLength = LittleLong(header->litLength);
    int bssLength = LittleLong(header->bssLength);
    vm->dataAlloc = dataLength + litLength + bssLength + VM_PROGRAM_STACK_SIZE;
    vm->dataBase = Com_malloc(vm->dataAlloc, vm, VM_ALLOC_DATA_SEC);
    
    // 4. Копирование DATA и LIT
    memcpy(vm->dataBase, bytecode + header->dataOffset, dataLength);
    memcpy(vm->dataBase + dataLength, bytecode + header->dataOffset + dataLength, litLength);
    
    // 5. BSS может быть заполнен нулями (опционально)
    memset(vm->dataBase + dataLength + litLength, 0, bssLength);
    
    // 6. Создание jump table для инструкций
    VM_PrepareInterpreter(vm, header);
    
    return 0;
}
```

---

## Системные вызовы (Syscalls)

### Концепция

**Syscalls (trap_* функции)** — это механизм, позволяющий VM-коду вызывать нативные функции движка. Без syscalls VM изолирована и не может ничего делать (нет I/O, нет рендеринга, нет трассировки).

### Архитектура syscalls

```
┌───────────────────────────────────────────┐
│         VM Code (cgame.qvm)               │
│                                           │
│  trap_R_AddRefEntityToScene(...)         │
│         ↓                                 │
│  g_syscalls.asm:                          │
│    equ trap_R_AddRefEntityToScene  -1    │
│         ↓                                 │
│  CONST -1                                 │
│  CALL (syscall handler)                   │
└───────────────────────────────────────────┘
         ↓ (syscall)
┌───────────────────────────────────────────┐
│      Engine (native C/C++ code)           │
│                                           │
│  intptr_t systemCalls(vm_t* vm, int* args) │
│  {                                        │
│      int id = -1 - args[0]; // Convert    │
│      switch(id) {                         │
│      case 0: // trap_R_AddRefEntityToScene│
│          re.AddRefEntityToScene(VMA(1));  │
│          return 0;                        │
│      }                                    │
│  }                                        │
└───────────────────────────────────────────┘
```

### Определение syscalls

#### В VM-коде (g_syscalls.asm):

```assembly
code

equ trap_Printf                 -1
equ trap_Error                  -2
equ trap_R_AddRefEntityToScene  -3
equ memset                      -4
equ memcpy                      -5
```

**Важно:**
- Syscall ID всегда **отрицательные** числа
- Конвенция: `engine_id = -1 - vm_id` (т.е. `-1` становится `0`, `-2` → `1`, etc.)

#### В движке (systemCalls callback):

```c
intptr_t systemCalls(vm_t* vm, intptr_t* args) {
    const int id = -1 - args[0]; // Преобразование VM ID → engine ID
    
    switch (id) {
    case 0: // trap_Printf (-1 в VM)
        return printf("%s", (const char*)VMA(1, vm));
        
    case 1: // trap_Error (-2 в VM)
        Com_Error(ERR_DROP, "%s", VMA(1, vm));
        return 0;
        
    case 2: // trap_R_AddRefEntityToScene (-3 в VM)
        re.AddRefEntityToScene((refEntity_t*)VMA(1, vm));
        return 0;
        
    case 3: // memset (-4 в VM)
        if (VM_MemoryRangeValid(args[1], args[3], vm) == 0) {
            memset(VMA(1, vm), args[2], args[3]);
        }
        return args[1];
        
    default:
        Com_Error(ERR_DROP, "Bad system call: %i", id);
    }
    return 0;
}
```

### Передача аргументов

**Правило:** аргументы передаются через массив `args[]`:
- `args[0]`: syscall ID (отрицательное число)
- `args[1]`: первый аргумент функции
- `args[2]`: второй аргумент
- ...и т.д.

### Макросы для syscalls

```c
// vm.h
#define VMA(x, vm) VM_ArgPtr(args[x], vm)  // Преобразование VM-адреса в host-адрес
#define VMF(x) VM_IntToFloat(args[x])      // Интерпретация int как float

// Пример использования:
case TRAP_MEMCPY:
    if (VM_MemoryRangeValid(args[1], args[3], vm) == 0 &&
        VM_MemoryRangeValid(args[2], args[3], vm) == 0) {
        memcpy(VMA(1, vm), VMA(2, vm), args[3]);
    }
    return args[1];
```

**VMA (VM Address)**: преобразует VM-адрес (offset в dataBase) в реальный указатель памяти.

**Важно для безопасности:**
- Всегда проверяйте границы памяти с `VM_MemoryRangeValid()`
- Не доверяйте адресам из VM без проверки

### Пример: добавление нового syscall

#### Шаг 1: Добавить в g_syscalls.asm
```assembly
equ trap_MyNewFunction  -100
```

#### Шаг 2: Объявить в VM-коде (cg_local.h)
```c
void trap_MyNewFunction(int arg1, const char* arg2);
```

#### Шаг 3: Реализовать в движке
```c
intptr_t systemCalls(vm_t* vm, intptr_t* args) {
    const int id = -1 - args[0];
    
    switch (id) {
    // ... existing cases ...
    
    case 99: // trap_MyNewFunction (-100 в VM)
        {
            int arg1 = args[1];
            const char* arg2 = (const char*)VMA(2, vm);
            // Проверка валидности строки
            if (VM_MemoryRangeValid(args[2], strlen(arg2)+1, vm) != 0) {
                Com_Error(ERR_DROP, "Invalid string pointer");
                return 0;
            }
            // Ваш код здесь
            MyNativeFunction(arg1, arg2);
            return 0;
        }
    }
}
```

---

## Производительность

### Стоимость операций

#### 1. Function Calls — ДОРОГО

**Каждый вызов функции:**
```assembly
; Caller side:
    CNSTI4 arg1
    ARGI4              ; Marshal argument 1
    CNSTI4 arg2
    ARGI4              ; Marshal argument 2
    ADDRGP4 myFunc
    CALL               ; Jump to function
    
; Callee side:
myFunc:
    ENTER N            ; Create stack frame (N bytes for locals)
    ; ... function body ...
    LEAVE N            ; Destroy stack frame
    ; Return
```

**Стоимость:**
- `ENTER`: ~10-20 циклов (setup stack frame)
- `LEAVE`: ~5-10 циклов (cleanup)
- `CALL`: ~5 циклов (jump + push return address)
- `ARG`: ~2-3 цикла на аргумент

**Итого:** ~40-50+ циклов на один вызов функции!

**LCC НЕ делает inlining** — каждый вызов, даже тривиального getter'а, это полноценный CALL/ENTER/LEAVE.

**Рекомендация:**
- ✅ Inline критическую логику вручную
- ✅ Объединяйте мелкие функции в одну большую для hot path
- ❌ Избегайте цепочек мелких функций (`getX() → checkX() → useX()`)

#### 2. Branches — СРЕДНЕ

**Каждое условие:**
```assembly
    INDIRI4        ; Load condition
    CNSTI4 0
    EQI4 $label    ; Conditional jump
```

**Стоимость:**
- Сравнение: ~2-3 цикла
- Conditional jump: ~5-10 циклов (если branch misprediction — до 20+ циклов)

**Проблема:** много мелких `if` создаёт много jump'ов, VM интерпретатор/JIT плохо предсказывает ветвление.

**Рекомендация:**
- ✅ Линейный код лучше, чем много `if/else if/else`
- ✅ Объединяйте проверки: `if (a && b)` лучше, чем `if (a) { if (b) }`
- ❌ Избегайте deep nesting (вложенность > 3 уровней)

#### 3. Loops — ЗАВИСИТ

**Цикл:**
```assembly
    CNSTI4 0
    ASGNI4         ; i = 0
loop:
    INDIRI4        ; Load i
    CNSTI4 100
    GEI4 end       ; if i >= 100, break
    ; loop body
    INDIRI4        ; Load i
    CNSTI4 1
    ADD
    ASGNI4         ; i++
    JUMP loop
end:
```

**Стоимость цикла:** ~10-15 циклов на итерацию (без тела)

**Проблема:** тело цикла может содержать function calls → убивает производительность.

**Рекомендация:**
- ✅ Один простой цикл лучше, чем несколько проходов с кешами
- ✅ Делайте работу в цикле inline (не вызывайте функции)
- ❌ Избегайте вложенных циклов с function calls

#### 4. Memory Access — ДЁШЕВО

```assembly
ADDRGP4 myVar  ; Address of global
INDIRI4        ; Load 4 bytes
```

**Стоимость:** ~2-3 цикла (LOAD4/STORE4)

**Рекомендация:**
- ✅ Многократные обращения к одной переменной не страшны
- ✅ Struct field access тоже дешёвый (адрес + offset + load)

#### 5. Syscalls — ОЧЕНЬ ДОРОГО (но не из-за VM)

```assembly
CNSTI4 arg
ARGI4
CNSTI4 -1      ; trap_R_AddRefEntityToScene
CALL
```

**Стоимость VM части:** ~5-10 циклов (CALL to negative ID → engine handler)

**Настоящая стоимость:** зависит от нативной функции!
- `trap_R_AddRefEntityToScene`: ~100-1000 циклов (добавление entity в scene)
- `trap_CM_BoxTrace`: ~10000+ циклов (collision trace)

**Рекомендация:**
- ✅ Минимизируйте **количество** syscalls, а не VM-код вокруг них
- ✅ Batch syscalls (например, добавляйте entities списком, если возможно)
- ✅ Кешируйте результаты трассировки, если данные не изменились

### Оптимизации интерпретатора

#### Computed Gotos (GCC)

```c
#ifdef __GNUC__
#define USE_COMPUTED_GOTOS
#endif

// Switch-based interpreter:
while (1) {
    opcode = *pc++;
    switch (opcode) {
    case OP_ADD: /* ... */ break;
    case OP_SUB: /* ... */ break;
    // ... 60 cases ...
    }
}

// Computed goto interpreter:
static void* jumpTable[64] = {
    &&op_undef, &&op_ignore, &&op_break, &&op_enter, /* ... */
};

NEXT_INSTRUCTION:
    opcode = *pc++;
    goto *jumpTable[opcode & 0x3F];

op_add:
    // ADD logic
    goto NEXT_INSTRUCTION;

op_sub:
    // SUB logic
    goto NEXT_INSTRUCTION;
```

**Преимущество computed gotos:**
- Каждый opcode имеет свой прыжок → CPU лучше предсказывает ветвление
- **~1.5-2x быстрее**, чем switch/case

**Benchmark (q3vm-master):**
- Default interpreter: **3.063 s**
- Computed gotos: **1.771 s** (1.7x faster)
- Native x86: **0.307 s** (5.8x faster than gotos)

### Практические советы по производительности

#### ✅ DO: Inline hot paths

**Плохо:**
```c
int GetClientNum(void) {
    return cg.snap->ps.clientNum;
}

qboolean IsSpectator(void) {
    return cg.snap->ps.pm_type == PM_SPECTATOR;
}

void MyHotFunction(void) {
    if (IsSpectator()) {
        int num = GetClientNum();
        // use num
    }
}
```

**Хорошо:**
```c
void MyHotFunction(void) {
    if (cg.snap->ps.pm_type == PM_SPECTATOR) {
        int num = cg.snap->ps.clientNum;
        // use num
    }
}
```

**Экономия:** 2 function calls (80-100 циклов) → 0 calls

#### ✅ DO: Простые циклы вместо кешей

**Плохо:**
```c
static int cachedCount;
static qboolean cacheValid;

void CountEntities(void) {
    if (!cacheValid) {
        cachedCount = 0;
        for (int i = 0; i < cg.snap->numEntities; i++) {
            if (IsValidEntity(i)) cachedCount++; // Function call!
        }
        cacheValid = qtrue;
    }
    // use cachedCount
}
```

**Хорошо:**
```c
void CountEntities(void) {
    int count = 0;
    for (int i = 0; i < cg.snap->numEntities; i++) {
        centity_t* ent = &cg_entities[i];
        if (ent->currentValid && ent->currentState.eType != ET_INVISIBLE) {
            count++; // Inline check, no function call
        }
    }
    // use count
}
```

**Экономия:** убираем cache management (branches, stores) и function calls в цикле

#### ✅ DO: Early exit

```c
void DrawSomething(void) {
    if (!cg_drawSomething.integer) return; // Early exit, skip всё

    // Heavy logic here
}
```

#### ❌ DON'T: Много мелких функций в hot path

```c
// Плохо: 5 function calls за одну итерацию
for (int i = 0; i < MAX_CLIENTS; i++) {
    if (IsValidClient(i)) {           // Call 1
        if (IsAlive(i)) {              // Call 2
            if (IsVisible(i)) {        // Call 3
                vec3_t pos = GetPos(i); // Call 4
                DrawClient(i, pos);     // Call 5
            }
        }
    }
}
```

#### ❌ DON'T: Spreading work over frames

```c
// Плохо: фазовая обработка
static int phase = 0;
void PerFrameUpdate(void) {
    if (phase == 0) {
        BuildCache(); // Function call
        phase = 1;
    } else {
        UseCache();   // Function call
        phase = 0;
    }
}

// Хорошо: простой один проход
void PerFrameUpdate(void) {
    for (int i = 0; i < count; i++) {
        // Process inline
    }
}
```

---

## Отладка QVM

### 1. Использование .map файла

q3asm генерирует `.map` файл с символьной таблицой:

```
// cgame.map
0x00000000  vmMain
0x00000123  CG_Init
0x00000456  CG_DrawActiveFrame
0x00000789  CG_AddRefEntityToScene
...
```

**Использование в DEBUG_VM режиме:**

```c
#define DEBUG_VM
#include "vm.h"

vm_t vm;
VM_Create(&vm, "cgame", bytecode, length, systemCalls);

// Загрузить символы для отладки
VM_LoadSymbols(&vm, "vm/cgame.map");

// Enable debug output (level 1 = info, level 2 = each opcode)
VM_Debug(1);

// Run VM
VM_Call(&vm, CG_INIT, 0, 0, 0, 0);

// Profile функции после сессии
VM_VmProfile_f(&vm);
```

**Вывод профиля:**
```
Function                 Calls    Total cycles
-----------------------------------------------
CG_DrawActiveFrame        1200    45000000
CG_AddRefEntityToScene    96000   12000000
CG_Trace                  5000    150000000
```

### 2. OP_BREAK для breakpoints

Вставьте в C-код:

```c
void MyFunction(void) {
    // ... some code ...
    
    #ifdef DEBUG
    asm("BREAK");  // Вставка OP_BREAK в bytecode
    #endif
    
    // ... more code ...
}
```

Когда VM выполняет `OP_BREAK`, увеличивается `vm->breakCount`. Можно установить `vm->breakFunction` для остановки на конкретной функции.

### 3. Com_Error для assertion'ов

```c
void MyFunction(int index) {
    if (index < 0 || index >= MAX_ENTITIES) {
        trap_Error(va("MyFunction: index %d out of range [0..%d]", 
                      index, MAX_ENTITIES));
        return;
    }
    // ...
}
```

`trap_Error` вызывает `Com_Error` в движке, который может:
- Вывести stack trace (если доступен `.map`)
- Остановить VM
- Показать error dialog

### 4. Printf debugging

```c
void MyHotFunction(void) {
    #ifdef DEBUG_QVM
    trap_Print(va("MyHotFunction: entering, clientNum=%d\n", cg.clientNum));
    #endif
    
    // ... function body ...
}
```

**Важно:** не оставляйте `trap_Print` в production! Каждый call — это syscall (~100+ циклов).

### 5. Valgrind для memory leaks

На Linux можно запустить движок с cgame.qvm под Valgrind:

```bash
valgrind --leak-check=full --track-origins=yes ./ioquake3 +set vm_cgame 2
```

`+set vm_cgame 2` форсирует использование QVM (а не native .so/.dll).

### 6. Дизассемблирование .qvm

Создайте простой скрипт для чтения `.qvm`:

```python
import struct

with open('cgame.qvm', 'rb') as f:
    magic, inst_count, code_off, code_len, data_off, data_len, lit_len, bss_len = \
        struct.unpack('<8I', f.read(32))
    
    print(f"Magic: 0x{magic:08X}")
    print(f"Instructions: {inst_count}")
    print(f"Code: offset={code_off}, size={code_len}")
    print(f"Data: offset={data_off}, size={data_len}")
    print(f"Lit: size={lit_len}")
    print(f"BSS: size={bss_len}")
    
    # Читаем CODE segment
    f.seek(code_off)
    code = f.read(code_len)
    
    # Простой дизассемблер
    opcodes = {
        0: "UNDEF", 1: "IGNORE", 2: "BREAK", 3: "ENTER", 4: "LEAVE",
        5: "CALL", 6: "PUSH", 7: "POP", 8: "CONST", 9: "LOCAL",
        10: "JUMP", # ... и т.д.
    }
    
    pc = 0
    while pc < len(code):
        op = code[pc]
        print(f"{pc:04X}: {opcodes.get(op, f'OP_{op}')} ", end='')
        pc += 1
        
        # Opcodes with 4-byte parameter
        if op in [3, 4, 8, 9, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
                  21, 22, 23, 24, 25, 26, 33]:
            param = struct.unpack('<I', code[pc:pc+4])[0]
            print(f"{param}")
            pc += 4
        else:
            print()
```

---

## Референсы

### Документация

1. **Q3VM Specifications**: http://www.icculus.org/~phaethon/q3mc/q3vm_specs.html
   - Полная спецификация опкодов, памяти, формата файла
   
2. **Fabien Sanglard's Q3 VM Article**: https://fabiensanglard.net/quake3/qvm.php
   - Отличное введение в QVM с диаграммами
   
3. **LCC Compiler Book**: "A Retargetable C Compiler: Design and Implementation" (Fraser & Hanson)
   - ISBN: 0-8053-1670-1

4. **q3vm-master Project**: https://github.com/jnz/q3vm
   - Современная, легковесная имплементация Q3VM
   - Единственный файл `vm.c` для интеграции
   
5. **John Carmack's .plan (Nov 1998)**: оригинальное объяснение Q3VM
   - Цитата: *"The programming language for QuakeArena mods is interpreted ANSI C"*

### Исходный код

- **ioquake3**: https://github.com/ioquake/ioq3
  - `code/qcommon/vm.c`: VM интерпретатор
  - `code/tools/lcc/`: LCC compiler
  - `code/tools/asm/`: q3asm assembler
  
- **id Software Quake III Arena**: https://github.com/id-Software/Quake-III-Arena
  - Оригинальный исходник (GPL 2)

### Инструменты

- **q3vm standalone**: компактный интерпретатор для изучения
- **q3asm-turbo**: быстрая версия ассемблера
- **LCC**: retargetable C compiler

---

## Заключение

Q3VM — это **простая, но эффективная** виртуальная машина для безопасного выполнения игрового кода. Ключ к производительности:

1. **Минимизируйте function calls** — inline критичный код
2. **Простота > сложность** — линейный код быстрее, чем clever branches
3. **Меньше syscalls** — оптимизируйте взаимодействие с движком
4. **Один проход** — избегайте кешей и фазовой обработки
5. **Профилируйте** — используйте DEBUG_VM и .map файлы

**Q3VM не быстрая (в 5-10x медленнее native)**, но при правильном подходе код работает без "затыков". Следуйте принципам из `QVM_LOGIC.md` и пишите простой, предсказуемый код.

---

*Документ создан для OSP2-BE на основе исследования q3vm-master и спецификаций Q3VM.*
*Версия: 1.0 | Дата: 2026-02-01*
