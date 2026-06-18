import { Glob } from "bun";
import { dirname, join, relative } from "node:path";

const INPUT_DIR = "fonts";
const OUTPUT_DIR = "dist/fonts_woff2_converted";

// Includes:
// - Basic Latin
// - Latin-1 Supplement
// - Latin Extended-A
// - Latin Extended-B
// - Euro sign
// => for English, French and German
const UNICODES = "U+0000-00FF,U+0100-017F,U+0180-024F,U+20AC";

const CONCURRENCY = navigator.hardwareConcurrency ?? 4;

console.log(`Converting webfonts on ${CONCURRENCY} core(s)`);

await Bun.$`mkdir -p ${OUTPUT_DIR}`;

async function convertFont(file) {
  const input = join(INPUT_DIR, file);
  const output = join(OUTPUT_DIR, file.replace(/\.ttf$/i, ".woff2"));

  const operation = `${input} -> ${output}`;
  const outputFile = Bun.file(output);
  if (await outputFile.exists()) {
    console.log("Skipped " + operation + " (already exists)");
    return;
  }

  console.log("Doing " + operation);

  await Bun.$`mkdir -p ${dirname(output)}`;

  try {
    await Bun.$`
    uv run pyftsubset ${input} \
    --flavor=woff2 \
    --output-file=${output} \
    --unicodes=${UNICODES}
    `;
  } catch (error) {
    console.error(`Failed to convert ${input}`, error);
  }
}

const files = [];

for await (const file of new Glob("**/*.ttf").scan(INPUT_DIR)) {
  files.push(file);
}

let index = 0;

async function worker() {
  while (true) {
    const file = files[index++];

    if (!file) {
      return;
    }

    try {
      await convertFont(file);
    } catch (error) {
      console.error(`Failed to convert ${file}`, error);
    }
  }
}

await Promise.all(Array.from({ length: CONCURRENCY }, () => worker()));
