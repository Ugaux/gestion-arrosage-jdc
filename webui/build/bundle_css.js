import { readFileSync, writeFileSync } from "fs";
import { bundle } from "lightningcss";
import path from "path";

const INPUT_FILE = "src/assets/css/app.css";
const OUTPUT_FILE = "dist/app/assets/app.min.css";

function simpleBundle(file) {
  const dir = path.dirname(file);
  let css = readFileSync(file, "utf8");

  return css.replace(/@import\s+['"](.+?)['"]\s*;/g, (_, importPath) => {
    return bundle(path.resolve(dir, importPath));
  });
}

const start = performance.now();
const result = bundle({
  filename: INPUT_FILE,
  minify: true,
  targets: {},
  drafts: {},
});
const elapsed = performance.now() - start;

await Bun.$`mkdir -p ${path.dirname(OUTPUT_FILE)}`;
writeFileSync(OUTPUT_FILE, result.code);

const css = readFileSync(INPUT_FILE, "utf8");
const imports = (css.match(/@import\s+/g) || []).length;
console.log(`Bundled ${imports} imports in ${Math.round(elapsed)}ms`);

const sizeKB = (result.code.length / 1024).toFixed(2);
console.log(`\n  app.min.css  ${sizeKB} KB\n`);
