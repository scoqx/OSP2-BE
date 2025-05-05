const texts = {
  en: {
    title: "OSP2-BE",
    subtitle: "Quake 3 Arena mod",
    desc: "Extended edition of the OSP2.<br/>OSP2 is an attempt to recover the old OSP mod source code and improve upon it.<br/>Based on vanilla Q3 and OSP files analysis.",
    download: "Download",
    galleryTitle: "Gallery",
    downloadTitle: "Download",
    contactTitle: "Contact",
    nav: {
      home: "Home",
      gallery: "Gallery",
      commands: "Console Commands",
      download: "Download",
      contact: "Contact"
    }
  },
  ru: {
    title: "OSP2-BE",
    subtitle: "Quake 3 Arena мод",
    desc: "Расширенная версия мода OSP2.<br/>OSP2 — это попытка восстановить исходный код старого мода OSP и улучшить его.<br/>Основано на анализе файлов ванильного Q3 и OSP.",
    download: "Скачать",
    galleryTitle: "Галерея",
    downloadTitle: "Скачать",
    contactTitle: "Контакты",
    nav: {
      home: "Главная",
      gallery: "Галерея",
      commands: "Консольные команды",
      download: "Скачать",
      contact: "Контакты"
    }
  }
};


document.addEventListener("DOMContentLoaded", () => {
  const langBtn = document.getElementById("langBtn");
  const titleElement = document.getElementById("title");
  const subtitleElement = document.getElementById("subtitle");
  const descElement = document.getElementById("desc");
  const downloadBtn = document.getElementById("downloadBtn");

  const navLinks = {
      home: document.getElementById("nav-home"),
      gallery: document.getElementById("nav-gallery"),
      commands: document.getElementById("nav-commands"),
      download: document.getElementById("nav-download"),
      contact: document.getElementById("nav-contact")
  };

  let currentLang = localStorage.getItem("language") || "en";
  updateLanguage(currentLang);

  if (langBtn) {
      langBtn.addEventListener("click", () => {
          currentLang = currentLang === "en" ? "ru" : "en";
          localStorage.setItem("language", currentLang);
          updateLanguage(currentLang);
      });
  }

  function updateLanguage(lang) {
      if (!texts[lang]) {
          console.error(`Нет перевода для языка: ${lang}`);
          return;
      }

      console.log("Смена языка на:", lang);

      if (langBtn) langBtn.innerText = lang === "en" ? "RU" : "EN";
      if (titleElement) titleElement.innerText = texts[lang].title;
      if (subtitleElement) subtitleElement.innerText = texts[lang].subtitle;
      if (descElement) descElement.innerHTML = texts[lang].desc;
      if (downloadBtn) downloadBtn.innerText = texts[lang].download;

      // Обновляем текст навигационных ссылок, если они существуют на странице
      Object.keys(navLinks).forEach(key => {
          if (navLinks[key]) {
              navLinks[key].innerText = texts[lang].nav[key];
          }
      });
  }
});


