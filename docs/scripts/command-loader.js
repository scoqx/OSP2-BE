// i ebal js
async function loadMarkdown() {
  const response = await fetch("commands.md"); // Загружаем файл
  const markdown = await response.text(); // Получаем содержимое

  // Разбиваем на блоки по разделителю (---)
  const sections = markdown.split("---").map(section => section.trim()).filter(Boolean);
  let htmlContent = "";
  
  sections.forEach(section => {
    // Разбиваем блок на непустые строки
    const lines = section.split("\n").filter(line => line.trim() !== "");
    
    // Если блок начинается с "#" – это заголовок
    if (lines[0].startsWith("#")) {
      htmlContent += `<h2>${lines[0].replace(/^#+/, "").trim()}</h2>`;
      return;
    }
  
    // Определяем строки с командами.
    // Если первая строка не начинается с пробела, считаем её командной.
    let commandLines = [];
    let i = 0;
    if (lines.length > 0) {
      if (!/^\s/.test(lines[0])) {
        commandLines.push(lines[0].trim());
        i = 1;
      } else {
        while (i < lines.length && /^\s/.test(lines[i])) {
          commandLines.push(lines[i].trim());
          i++;
        }
      }
    }
  
    let commandPart = "";
    let descriptionPart = "";
  
    // Если единственная строка-команда содержит "—", разделяем её
    if (commandLines.length === 1 && commandLines[0].includes("—")) {
      let parts = commandLines[0].split("—");
      commandPart = parts[0].trim();
      descriptionPart = parts.slice(1).join("—").trim();
      
      // Если описание сразу начинается с группы команд (например, в группе команд часть попала после тире)
      let extraMatch = descriptionPart.match(/^((?:[+\-a-zA-Z0-9_*]+(?:\s*-?\d+\.\.-?\d+)?\s+)+)(.*)$/);
      if (extraMatch) {
        // extraMatch[1] – найденная дополнительная часть (команды), extraMatch[2] – остаток описания
        commandPart += " " + extraMatch[1].trim();
        descriptionPart = extraMatch[2].trim();
      }
    } else {
      // Если несколько команд, то объединяем их
      if (commandLines.length) {
        commandPart = commandLines.join(" ");
      }
      // Остальные строки считаем описанием
      let descLines = lines.slice(i);
      descriptionPart = descLines.join(" ").trim();
    }
  
    // Извлекаем аргументы из описания.
    // Ожидаемый формат: `значение` — описание (тире может быть тире или длинное тире)
    let args = [];
    const argRegex = /`([^`]+)`\s*(?:[-—]\s*)?([^`]+)/g;
    let match;
    while ((match = argRegex.exec(descriptionPart)) !== null) {
      args.push({ value: match[1].trim(), desc: match[2].trim() });
    }
    // Убираем найденные аргументы из описания
    descriptionPart = descriptionPart.replace(argRegex, "").trim();
  
    // Если в блоке несколько строк с командами, выводим каждую отдельно, иначе – всю строку
    let cmdHtml = "";
    if (commandLines.length > 1) {
      cmdHtml = commandLines.map(cmd => `<code>${cmd}</code>`).join(", ");
    } else {
      cmdHtml = `<code>${commandPart}</code>`;
    }
  
    // Формируем HTML для аргументов, если они есть
    let argsHtml = "";
    if (args.length) {
      argsHtml += "<ul>";
      args.forEach(arg => {
        argsHtml += `<li><code>${arg.value}</code> — ${arg.desc}</li>`;
      });
      argsHtml += "</ul>";
    }
  
    // Финальный HTML-блок для данного раздела
    htmlContent += `
      <div class="command-block">
        <p>${cmdHtml} — ${descriptionPart}</p>
        ${argsHtml}
      </div>
    `;
  });
  
  document.getElementById("content").innerHTML = htmlContent;
}
  
document.addEventListener("DOMContentLoaded", loadMarkdown);
