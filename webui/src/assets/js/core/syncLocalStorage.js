export function syncJsonLocalStorage(key, callback) {
  window.addEventListener("storage", (e) => {
    if (e.key !== key) return;
    callback(e.newValue ? JSON.parse(e.newValue) : null);
  });
}
