import { readFileSync, writeFileSync } from "fs";
import path from "path";

function bundle(file) {
  const dir = path.dirname(file);
  let css = readFileSync(file, "utf8");

  return css.replace(/@import\s+['"](.+?)['"]\s*;/g, (_, importPath) => {
    return bundle(path.resolve(dir, importPath));
  });
}

const input = "src/assets/css/app.css";
const output = "src/assets/app.min.css";

writeFileSync(output, bundle(input));

console.log("✓ app.min.css generated");
