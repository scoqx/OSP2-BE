const texts = {
  en: {
    title: "OSP2-BE",
    subtitle: "Quake 3 Arena mod",
    desc: "Extended edition of the OSP2.<br/>OSP2 is an attempt to recover the old OSP mod source code and improve upon it.<br/>Based on vanilla Q3 and OSP files analysis.",
    downloadBtn: "Download",
    lastVersion: "Latest version",
    nav: {
      home: "Home",
      gallery: "Gallery",
      commands: "Console Commands",
      downloads: "Downloads",
      contact: "Contacts"
    }
  },
  ru: {
    title: "OSP2-BE",
    subtitle: "Quake 3 Arena мод",
    desc: "Расширенная версия мода OSP2.<br/>OSP2 — это попытка восстановить исходный код старого мода OSP и улучшить его.<br/>Основано на анализе файлов ванильного Q3 и OSP.",
    downloadBtn: "Скачать",
    lastVersion: "Последняя версия",
    nav: {
      home: "Главная",
      gallery: "Галерея",
      commands: "Консольные команды",
      downloads: "Загрузки",
      contact: "Контакты"
    }
  }
};

document.addEventListener("DOMContentLoaded", () => {
  const langBtn = document.getElementById("langBtn");

  const bindings = {
    title: "title",
    subtitle: "subtitle",
    desc: "desc", // innerHTML
    downloadBtn: "downloadBtn",
    lastVersion: "lastVersion",
    downloadsTitle: "nav.downloads",
    galleryTitle: "nav.gallery",
    contactTitle: "nav.contact",
    commandsTitle: "nav.commands"
  };

  const navLinks = {
    home: document.getElementById("nav-home"),
    gallery: document.getElementById("nav-gallery"),
    commands: document.getElementById("nav-commands"),
    downloads: document.getElementById("nav-downloads"),
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
    const langData = texts[lang];
    if (!langData) {
      console.error(`Нет перевода для языка: ${lang}`);
      return;
    }

    console.log("Смена языка на:", lang);
    if (langBtn) langBtn.innerText = lang === "en" ? "RU" : "EN";

    Object.entries(bindings).forEach(([id, keyPath]) => {
      const element = document.getElementById(id);
      if (!element) return;

      const value = keyPath.split('.').reduce((acc, key) => acc?.[key], langData);
      if (value) {
        if (id === "desc") {
          element.innerHTML = value;
        } else {
          element.innerText = value;
        }
      }
    });

    Object.keys(navLinks).forEach(key => {
      if (navLinks[key]) {
        navLinks[key].innerText = langData.nav[key];
      }
    });
  }
});
