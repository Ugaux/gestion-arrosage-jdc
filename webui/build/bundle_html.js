import { readFileSync, writeFileSync } from "fs";
import { minify } from "html-minifier-terser";
import { dirname } from "path";

const INPUT_FILE = "src/index.html";
const OUTPUT_FILE = "dist/app/index.html";

const html = readFileSync(INPUT_FILE, "utf8");

const start = performance.now();
const result = await minify(html, {
  collapseWhitespace: true,
  removeComments: true,
  minifyCSS: true,
  minifyJS: true,
});
const elapsed = performance.now() - start;

await Bun.$`mkdir -p ${dirname(OUTPUT_FILE)}`;
writeFileSync(OUTPUT_FILE, result);

console.log(`Minified in ${Math.round(elapsed)}ms`);

const sizeKB = (result.length / 1024).toFixed(2);
console.log(`\n  html.index  ${sizeKB} KB\n`);
