document.addEventListener("DOMContentLoaded", () => {
  fetch("/docs/version.json")
    .then(response => response.json())
    .then(data => {
      const version = data.version;
      document.getElementById("version-text").textContent = `${version}`;
      document.getElementById("downloadBtn").href =
        `https://github.com/scoqx/OSP2-BE/releases/download/latest/zz-osp-pak8.pk3`;
    })
    .catch(error => {
      document.getElementById("version-text").textContent = "Ошибка загрузки версии";
      console.error(error);
    });
});
