import { rm } from "fs/promises";

console.log("Cleaning dist/app ...");

await rm("dist/app", { recursive: true, force: true });

console.log("Cleaning done.\n");
