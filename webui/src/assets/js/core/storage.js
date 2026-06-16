export function readJsonLocalStorage(key) {
  try {
    return JSON.parse(localStorage.getItem(key) || "{}");
  } catch {
    return {};
  }
}

export function subscribeToLocalStorageJson(key, callback) {
  function handler(event) {
    if (event.storageArea !== localStorage || event.key !== key) return;
    callback(event.newValue ? JSON.parse(event.newValue) : null);
  }

  window.addEventListener("storage", handler);
  return () => removeEventListener("storage", handler);
}

export function subscribeToLocalStorageString(key, callback) {
  function handler(event) {
    if (event.storageArea !== localStorage || event.key !== key) return;
    callback(event.newValue); // raw string (no parsing)
  }

  window.addEventListener("storage", handler);
  return () => removeEventListener("storage", handler);
}
