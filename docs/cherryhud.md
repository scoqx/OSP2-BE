# CherryHUD Documentation

## Обзор

CherryHUD - это система настройки интерфейса для Quake 3 Arena, основанная на SuperHUD. Она позволяет создавать кастомные HUD элементы, таблицы и конфигурации для отображения игровой информации.

## Структура системы

### Типы элементов

CherryHUD поддерживает несколько типов элементов:

1. **Элементы (Elements)** - базовые компоненты для отображения данных
2. **Блоки (Blocks)** - контейнеры для группировки элементов
3. **Контейнеры (Containers)** - основные контейнеры для организации интерфейса

### Основные контейнеры

- `!scoreboard` - главный контейнер для табло
- `playersRows` - контейнер для строк игроков
- `spectatorsRows` - контейнер для строк наблюдателей

### Блоки-шаблоны

- `playerRow` - шаблон для строки игрока
- `spectatorRow` - шаблон для строки наблюдателя
- `title` - заголовок

## Элементы

### Универсальный элемент `decor`

Основной элемент для отображения данных. Поддерживает автоматическое определение типа контента.

- `type <datatype>` - тип данных для отображения
- `weapon <weaponname>` - оружие

#### Алиасы для удобства
- `print` - алиас для `decor`
- `text` - алиас для `decor`
- `draw` - алиас для `decor`
- `decor` - алиас для `decor`

#### Типы данных <datatype>

##### Игроки (player.*)
- `player.name` - имя игрока
- `player.score` - очки игрока
- `player.ping` - пинг игрока
- `player.deaths` - количество смертей
- `player.kd` - соотношение убийств/смертей
- `player.damageGiven` - нанесенный урон
- `player.damageReceived` - полученный урон
- `player.damageRatio` - соотношение урона
- `player.id` - ID игрока
- `player.time` - время в игре
- `player.thaws` - количество разморозок (только в freeze режиме)
- `player.xid` - XID игрока
- `player.skill` - навык игрока
- `player.head` - голова игрока (для отображения модели)

##### Игра (game.*)
- `game.serverName` - название сервера
- `game.map` - название карты
- `game.gameType` - тип игры
- `game.timelimit` - лимит времени
- `game.fraglimit` - лимит фрагов
- `game.capturelimit` - лимит захватов

##### Оружие (weapon.*)
- `weapon.[название].kills` - убийства с оружия
- `weapon.[название].accuracy` - точность с оружием
- `weapon.[название].shots` - выстрелы из оружия
- `weapon.[название].icon` - иконка оружия

Доступные типы оружия: `gauntlet`, `machinegun`, `shotgun`, `grenade`, `rocket`, `lightning`, `railgun`, `plasma`, `bfg`

Для некоторых элементов по datatype можно указать image (score, ping, death, time)
## Параметры конфигурации

### Позиционирование и размеры

- `pos <x> <y>` - позиция элемента (x, y координаты)
- `size <width> <height>` - размер элемента
- `offset <x> <y> <z>` - смещение элемента
- `margins <left> <top> <right> <bottom>` - отступы

### Выравнивание

- `alignh <l|c|r>` - горизонтальное выравнивание (left/center/right)
- `alignv <t|c|b>` - вертикальное выравнивание (top/center/bottom)

### Цвета

- `color <r> <g> <b> <a>` - основной цвет (RGBA)
- `color <t|e|u>` - специальные цвета (team/enemy/unique)
- `color2 <r> <g> <b> <a>` - вторичный цвет (может использоваться для задания прозрачности при color e t u)

### Фон и границы

- `bgcolor <color>` - цвет фона
- `bgcolor2 <color>` - вторичный цвет фона
- `border <left> <top> <right> <bottom>` - размеры границ
- `borderColor <color>` - цвет границ
- `borderColor2 <color>` - вторичный цвет границ

### Текст

- `text <string>` - текст для отображения
- `font <fontname>` - шрифт (аналог text.font)
- `monospace` - моноширинный шрифт
- ВРОДЕ НЕ РАБОТАЕТ `textoffset <x> <y>` - смещение текста
- НЕ РАБОТАЕТ `hlsize <size>` - размер подсветки

### Расширенные параметры текста (text.*)

- `text.size <width> <height>` - размер текстовой области
- `text.pos <x> <y>` - позиция текста
- `text.color <color>` - цвет текста
- `text.color2 <color>` - вторичный цвет текста
- `text.background <color>` - фон текста
- `text.border <sizes>` - границы текста
- `text.bordercolor <color>` - цвет границ текста
- `text.alignh <l|c|r>` - горизонтальное выравнивание текста
- `text.alignv <t|c|b>` - вертикальное выравнивание текста
- `text.shadow` - тень текста (включить и использовать цвет по умолчанию (black))
- `text.shadow.color <color>` - цвет тени
- `text.font <fontname>` - шрифт текста
- `text.format "<format>"` - формат отображения (например .2f - два символа после точки)

### Изображения

- `image <path>` - путь к изображению
- `imagetc <x> <y> <width> <height>` - координаты текстуры
- `image.bgcolor <color>` - фон изображения
- `image.border <sizes>` - границы изображения
- `image.bordercolor <color>` - цвет границ изображения

### Анимация и эффекты

- НЕ РАБОТАЕТ `fade <r> <g> <b> <a>` - параметры затухания
- НЕ РАБОТАЕТ `fadedelay <time>` - задержка затухания (мс)
- НЕ РАБОТАЕТ `time <duration>` - время отображения
- НЕ РАБОТАЕТ `direction <l|r|t|b>` - направление (left/right/top/bottom)
- НЕ РАБОТАЕТ `doublebar` - двойная полоса

### Видимость

- `visflags <flags>` - флаги видимости
- `hideflags <flags>` - флаги скрытия
- Если указать флаги через пробел, будет использован оператор ИЛИ (т.е. если одно из условий выполнено - отображает/скрывает)
- Если указать флаги через +, например gt_ffa+gt_tdm, будет использован оператор И (т.е. если оба условия выполнено - отображает/скрывает)

#### Флаги видимости

- `im` - режим instant message
- `teamonly` - только для команды
- `spectator` - только для наблюдателей
- `dead` - только для мертвых игроков
- `demohide` - скрыть в демо
- LOL, IGNORE `scoreshide` - скрыть в табло
- `key1show` - показать при нажатии key1
- `key2show` - показать при нажатии key2
- `key3show` - показать при нажатии key3
- `key4show` - показать при нажатии key4
- НЕ РАБОТАЕТ `showempty` - показать пустые элементы
- `gt_ffa` - режим Free For All
- `gt_tourney` - турнирный режим
- `gt_tdm` - командный режим
- `gt_ctf` - режим Capture The Flag
- `gt_freezetag` - режим Freeze Tag
- `gt_clanarena` - режим Clan Arena
- `localClient` - локальный клиент
- `activePlayer` - активный игрок (наблюдаемый)
- `ready` - готовый игрок
- `frozen` - замороженный игрок
- `compact` - компактный режим
- НЕ РАБОТАЕТ КАК ЗАДУМАНО `double` - двойной режим

### Дополнительные параметры

- НЕ РАБОТАЕТ `content <string>` - содержимое (альтернатива text)
- `maxwidth <width>` - максимальная ширина для текста
- `maxvalue <value>` - максимальное значение
- НЕТ СТИЛЕЙ `style <number>` - стиль отображения
- НЕ РАБОТАЕТ `angles <pitch> <yaw> <roll> [elevation]` - углы поворота
- НЕ РАБОТАЕТ `model <modelname>` - модель для отображения
- `extendedType <none|double|compact>` - расширенный тип
- `rowSpacing <spacing>` - расстояние между строками
- НЕ РАБОТАЕТ `tableSpacing <spacing>` - расстояние между таблицами для режима double
- НЕ РАБОТАЕТ `itteam <b|r|n|o|e>` - команда (blue/red/neutral/own/enemy)

### Префиксные режимы

Поддерживаются префиксы для адаптивных режимов:

- `compact.*` - параметры для компактного режима
- `double.*` - параметры для двойного режима

Пример:
```
compact.pos 10 20
compact.size 100 50
double.pos 20 30
double.size 200 100
```

## Структура конфигурационного файла

### Основная структура

```
!scoreboard {
    pos 100 100
    size 400 300
    
    !default {
        color 1 1 1 1
        font "bigfont"
    }
    
    title {
        text "Scoreboard"
        pos 0 0
        size 400 20
    }
    
    playerRow {
        pos 0 25
        size 400 20
        
        decor {
            type "player.name"
            pos 0 0
            size 200 20
        }
        
        decor {
            type "player.score"
            pos 200 0
            size 100 20
        }
    }
    
    playersRows {
        // Строки игроков будут созданы автоматически
    }
}
```

### Примеры элементов

#### Отображение имени игрока
```
decor {
    type "player.name"
    pos 10 10
    size 150 20
    color 1 1 1 1
    font "sansman"
}
```

#### Отображение очков с форматированием
```
decor {
    type "player.score"
    pos 160 10
    size 50 20
    color 1 1 0 1
    text.format "%d" 
}
```

#### Отображение статистики оружия
```
decor {
    type "weapon.railgun.accuracy"
    pos 10 40
    size 100 20
    color 0 1 0 1
    text.format "%.1f%%"
}
```

#### Условное отображение
```
decor {
    type "player.thaws"
    pos 10 70
    size 80 20
    visflags "gt_freezetag"
    color 0 0 1 1
}
```

## Советы по использованию

1. **Используйте `!default`** для установки общих параметров
2. **Применяйте префиксные режимы** для адаптивного интерфейса
3. **Используйте `visflags`** для условного отображения элементов
4. **Комбинируйте флаги** с помощью пробелов (OR) или `+` (AND)
5. **Используйте `type`** для автоматического определения контента
6. **Применяйте `text.format`** для форматирования числовых значений
