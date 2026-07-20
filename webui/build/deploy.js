import { rm, mkdir, cp, readdir, stat, writeFile } from "fs/promises";
import { gzip } from "zlib";
import { promisify } from "util";
import path from "path";

const INPUT_DIR = "dist/app/";
const OUTPUT_DIR = "../firmware/data/www";
const GZIP_EXTENSIONS = new Set([".html", ".js", ".css", ".json"]);

const gzipAsync = promisify(gzip);

await rm(OUTPUT_DIR, {
  recursive: true,
  force: true,
});

console.log(`Copying and compressing files to firmware data folder...`);
const start = performance.now();
async function copyAndCompress(src, dest) {
  const info = await stat(src);

  if (info.isDirectory()) {
    await mkdir(dest, { recursive: true });

    for (const entry of await readdir(src)) {
      await copyAndCompress(path.join(src, entry), path.join(dest, entry));
    }

    return;
  }

  const ext = path.extname(src);

  if (GZIP_EXTENSIONS.has(ext)) {
    const data = await Bun.file(src).arrayBuffer();

    const gzPath = `${dest}.gz`;

    await writeFile(gzPath, await gzipAsync(Buffer.from(data), { level: 9 }));

    // Remove the uncompressed version
    await rm(dest, { force: true });

    console.log(`gzip ${dest} -> ${gzPath}`);
  } else {
    await cp(src, dest);
  }
}
await copyAndCompress(INPUT_DIR, OUTPUT_DIR);
const elapsed = performance.now() - start;

console.log(`Finished in ${Math.round(elapsed)}ms\n`);
