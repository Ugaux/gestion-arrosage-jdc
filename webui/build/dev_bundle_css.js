import { writeFileSync } from "fs";
import { bundle } from "lightningcss";

const input = "src/assets/css/app.css";
const output = "src/assets/app.min.css";

const result = bundle({
  filename: input,
  minify: true,
  targets: {},
});

writeFileSync(output, result.code);

console.log("✓ app.min.css generated");
