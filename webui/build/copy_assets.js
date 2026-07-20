import { cp } from "fs/promises";
import path from "path";

const INPUT_DIR = "src/assets/";
const OUTPUT_DIR = "dist/app/assets/";

await Bun.$`mkdir -p ${OUTPUT_DIR}`;

const start = performance.now();
await cp("src/version.json", "dist/app/version.json", {
  recursive: true,
});
await cp(path.join(INPUT_DIR, "fonts"), path.join(OUTPUT_DIR, "fonts"), {
  recursive: true,
});
await cp(path.join(INPUT_DIR, "img"), path.join(OUTPUT_DIR, "img"), {
  recursive: true,
});
const elapsed = performance.now() - start;

console.log(`Assets copied in ${Math.round(elapsed)}ms\n`);
